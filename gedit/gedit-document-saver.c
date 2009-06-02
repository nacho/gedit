/*
 * gedit-document-saver.c
 * This file is part of gedit
 *
 * Copyright (C) 2005-2006 - Paolo Borelli and Paolo Maggi
 * Copyright (C) 2007 - Paolo Borelli, Paolo Maggi, Steve Frécinaux
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the gedit Team, 2005-2006. See the AUTHORS file for a
 * list of people on the gedit Team.
 * See the ChangeLog files for a list of changes.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <glib/gi18n.h>

#include "gedit-document-saver.h"
#include "gedit-debug.h"
#include "gedit-convert.h"
#include "gedit-prefs-manager.h"
#include "gedit-marshal.h"
#include "gedit-utils.h"
#include "gedit-enum-types.h"

#include "gedit-local-document-saver.h"
#include "gedit-gio-document-saver.h"

G_DEFINE_ABSTRACT_TYPE(GeditDocumentSaver, gedit_document_saver, G_TYPE_OBJECT)

/* Signals */

enum {
	SAVING,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

/* Properties */

enum {
	PROP_0,
	PROP_DOCUMENT,
	PROP_URI,
	PROP_ENCODING,
	PROP_FLAGS
};

static void
gedit_document_saver_set_property (GObject      *object,
				   guint         prop_id,
				   const GValue *value,
				   GParamSpec   *pspec)
{
	GeditDocumentSaver *saver = GEDIT_DOCUMENT_SAVER (object);

	switch (prop_id)
	{
		case PROP_DOCUMENT:
			g_return_if_fail (saver->document == NULL);
			saver->document = g_value_get_object (value);
			break;
		case PROP_URI:
			g_return_if_fail (saver->uri == NULL);
			saver->uri = g_value_dup_string (value);
			break;
		case PROP_ENCODING:
			g_return_if_fail (saver->encoding == NULL);
			saver->encoding = g_value_get_boxed (value);
			break;
		case PROP_FLAGS:
			saver->flags = g_value_get_flags (value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_document_saver_get_property (GObject    *object,
				   guint       prop_id,
				   GValue     *value,
				   GParamSpec *pspec)
{
	GeditDocumentSaver *saver = GEDIT_DOCUMENT_SAVER (object);

	switch (prop_id)
	{
		case PROP_DOCUMENT:
			g_value_set_object (value, saver->document);
			break;
		case PROP_URI:
			g_value_set_string (value, saver->uri);
			break;
		case PROP_ENCODING:
			g_value_set_boxed (value, saver->encoding);
			break;
		case PROP_FLAGS:
			g_value_set_flags (value, saver->flags);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_document_saver_finalize (GObject *object)
{
	GeditDocumentSaver *saver = GEDIT_DOCUMENT_SAVER (object);

	g_free (saver->uri);
	g_free (saver->backup_ext);

	G_OBJECT_CLASS (gedit_document_saver_parent_class)->finalize (object);
}

static void 
gedit_document_saver_class_init (GeditDocumentSaverClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = gedit_document_saver_finalize;
	object_class->set_property = gedit_document_saver_set_property;
	object_class->get_property = gedit_document_saver_get_property;

	g_object_class_install_property (object_class,
					 PROP_DOCUMENT,
					 g_param_spec_object ("document",
							      "Document",
							      "The GeditDocument this GeditDocumentSaver is associated with",
							      GEDIT_TYPE_DOCUMENT,
							      G_PARAM_READWRITE |
							      G_PARAM_CONSTRUCT_ONLY |
							      G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (object_class,
					 PROP_URI,
					 g_param_spec_string ("uri",
							      "URI",
							      "The URI this GeditDocumentSaver saves the document to",
							      "",
							      G_PARAM_READWRITE |
							      G_PARAM_CONSTRUCT_ONLY |
							      G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (object_class,
					 PROP_ENCODING,
					 g_param_spec_boxed ("encoding",
							     "URI",
							     "The encoding of the saved file",
							     GEDIT_TYPE_ENCODING,
							     G_PARAM_READWRITE |
							     G_PARAM_CONSTRUCT_ONLY |
							     G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (object_class,
					 PROP_FLAGS,
					 g_param_spec_flags ("flags",
							     "Flags",
							     "The flags for the saving operation",
							     GEDIT_TYPE_DOCUMENT_SAVE_FLAGS,
							     0,
							     G_PARAM_READWRITE |
							     G_PARAM_CONSTRUCT_ONLY));

	signals[SAVING] =
		g_signal_new ("saving",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GeditDocumentSaverClass, saving),
			      NULL, NULL,
			      gedit_marshal_VOID__BOOLEAN_POINTER,
			      G_TYPE_NONE,
			      2,
			      G_TYPE_BOOLEAN,
			      G_TYPE_POINTER);
}

static void
gedit_document_saver_init (GeditDocumentSaver *saver)
{
	saver->used = FALSE;
}

GeditDocumentSaver *
gedit_document_saver_new (GeditDocument       *doc,
			  const gchar         *uri,
			  const GeditEncoding *encoding,
			  GeditDocumentSaveFlags flags)
{
	GeditDocumentSaver *saver;
	GType saver_type;

	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), NULL);

#ifndef G_OS_WIN32
	if (gedit_utils_uri_has_file_scheme (uri))
		saver_type = GEDIT_TYPE_LOCAL_DOCUMENT_SAVER;
	else
#endif
		saver_type = GEDIT_TYPE_GIO_DOCUMENT_SAVER;

	if (encoding == NULL)
		encoding = gedit_encoding_get_utf8 ();

	saver = GEDIT_DOCUMENT_SAVER (g_object_new (saver_type,
						    "document", doc,
						    "uri", uri,
						    "encoding", encoding,
						    "flags", flags,
						    NULL));

	return saver;
}

gchar *
gedit_document_saver_get_end_newline (GeditDocumentSaver *saver,
				      gsize 	         *len)
{
	gchar *n_buffer = NULL;
	gsize n_len = 0;

	if (saver->encoding != gedit_encoding_get_utf8 ())
	{
		n_buffer = gedit_convert_from_utf8 ("\n", 
						    -1, 
						    saver->encoding, 
						    &n_len, 
						    NULL);

		if (n_buffer == NULL)
		{
			/* we do not error out for this */
			g_warning ("Cannot convert '\\n' to the desired encoding.");
		}
	}
	else
	{
		n_buffer = g_strdup ("\n");
		n_len = 1;
	}

	*len = n_len;
	return n_buffer;
}

/* FIXME: we should rework the code to not need to fetch the
   whole buffer in memory. Also encoding conversion should
   be done in chunks */
gchar *
gedit_document_saver_get_document_contents (GeditDocumentSaver *saver,
					    gsize 	       *len,
					    GError 	      **error)
{
	GtkTextBuffer *buffer = GTK_TEXT_BUFFER (saver->document);
	GtkTextIter start_iter;
	GtkTextIter end_iter;
	gchar *contents;
	
	gtk_text_buffer_get_bounds (buffer, &start_iter, &end_iter);
	contents = gtk_text_buffer_get_slice (buffer, &start_iter, &end_iter, TRUE);

	*len = strlen (contents);

	if (saver->encoding != gedit_encoding_get_utf8 ())
	{
		gchar *converted_contents;
		gsize new_len;

		converted_contents = gedit_convert_from_utf8 (contents, 
							      *len, 
							      saver->encoding,
							      &new_len,
							      error);
		g_free (contents);

		if (*error != NULL)
		{
			/* Conversion error */
			return NULL;
		}
		else
		{
			contents = converted_contents;
			*len = new_len;
		}
	}
	
	return contents;
}

/*
 * Write the document contents in fd.
 */
gboolean
gedit_document_saver_write_document_contents (GeditDocumentSaver  *saver,
					      gint                 fd,
					      GError             **error)
{
	gsize len;
	gssize written;
	gboolean res;
	gchar *contents;

	gedit_debug (DEBUG_SAVER);

	contents = gedit_document_saver_get_document_contents (saver, &len, error);

	/* make sure we are at the start */
	res = (lseek (fd, 0, SEEK_SET) != -1);

	/* Truncate the file to 0, in case it was not empty */
	if (res)
	{
		res = (ftruncate (fd, 0) == 0);
	}

	/* Save the file content */
	if (len > 0)
	{
		if (res)
		{
			const gchar *write_buffer = contents;
			gssize to_write = len;

			do
			{
				written = write (fd, write_buffer, to_write);
				if (written == -1)
				{
					if (errno == EINTR)
						continue;

					res = FALSE;

					break;
				}

				to_write -= written;
				write_buffer += written;
			}
			while (to_write > 0);
		}

		/* make sure files are always terminated with \n (see bug #95676). Note
		   that we strip the trailing \n when loading the file */
		if (res)
		{
			gchar *n_buf;
			gsize n_len;

			n_buf = gedit_document_saver_get_end_newline (saver, &n_len);
			if (n_buf != NULL)
			{
				written = write (fd, n_buf, n_len);
				res = (written != -1 && (gsize) written == n_len);
				g_free (n_buf);
			}
			else
			{
				g_warning ("Cannot add '\\n' at the end of the file.");
			}
		}
	}

#ifdef HAVE_FSYNC
	/* Ensure that all the data reaches disk */
	if (res && fsync (fd) != 0)
	{
		g_set_error (error,
			     G_IO_ERROR,
			     g_io_error_from_errno (errno),
			     "%s", g_strerror (errno));
		res = FALSE;
	}
#endif

	if (!res)
	{
		g_set_error (error,
			     G_IO_ERROR,
			     g_io_error_from_errno (errno),
			     "%s", g_strerror (errno));
	}

	g_free (contents);

	return res;
}

void
gedit_document_saver_saving (GeditDocumentSaver *saver,
			     gboolean            completed,
			     GError             *error)
{
	/* the object will be unrefed in the callback of the saving
	 * signal, so we need to prevent finalization.
	 */
	if (completed)
	{
		g_object_ref (saver);
	}

	g_signal_emit (saver, signals[SAVING], 0, completed, error);

	if (completed)
	{
		if (error == NULL)
			gedit_debug_message (DEBUG_SAVER, "save completed");
		else
			gedit_debug_message (DEBUG_SAVER, "save failed");

		g_object_unref (saver);
	}
}

void
gedit_document_saver_save (GeditDocumentSaver     *saver,
			   time_t                  old_mtime)
{
	gedit_debug (DEBUG_SAVER);

	g_return_if_fail (GEDIT_IS_DOCUMENT_SAVER (saver));
	g_return_if_fail (saver->uri != NULL && strlen (saver->uri) > 0);

	g_return_if_fail (saver->used == FALSE);
	saver->used = TRUE;

	// CHECK:
	// - sanity check a max len for the uri?
	// report async (in an idle handler) or sync (bool ret)
	// async is extra work here, sync is special casing in the caller

	/* fetch saving options */
	saver->backup_ext = gedit_prefs_manager_get_backup_extension ();

	/* never keep backup of autosaves */
	if ((saver->flags & GEDIT_DOCUMENT_SAVE_PRESERVE_BACKUP) != 0)
		saver->keep_backup = FALSE;
	else
		saver->keep_backup = gedit_prefs_manager_get_create_backup_copy ();

	/* TODO: add support for configurable backup dir */
	saver->backups_in_curr_dir = TRUE;

	GEDIT_DOCUMENT_SAVER_GET_CLASS (saver)->save (saver, old_mtime);
}

GeditDocument *
gedit_document_saver_get_document (GeditDocumentSaver *saver)
{
	g_return_val_if_fail (GEDIT_IS_DOCUMENT_SAVER (saver), NULL);

	return saver->document;
}

const gchar *
gedit_document_saver_get_uri (GeditDocumentSaver *saver)
{
	g_return_val_if_fail (GEDIT_IS_DOCUMENT_SAVER (saver), NULL);

	return saver->uri;
}

const gchar *
gedit_document_saver_get_content_type (GeditDocumentSaver *saver)
{
	g_return_val_if_fail (GEDIT_IS_DOCUMENT_SAVER (saver), NULL);

	return GEDIT_DOCUMENT_SAVER_GET_CLASS (saver)->get_content_type (saver);
}

time_t
gedit_document_saver_get_mtime (GeditDocumentSaver *saver)
{
	g_return_val_if_fail (GEDIT_IS_DOCUMENT_SAVER (saver), 0);

	return GEDIT_DOCUMENT_SAVER_GET_CLASS (saver)->get_mtime (saver);
}

/* Returns 0 if file size is unknown */
goffset
gedit_document_saver_get_file_size (GeditDocumentSaver *saver)
{
	g_return_val_if_fail (GEDIT_IS_DOCUMENT_SAVER (saver), 0);

	return GEDIT_DOCUMENT_SAVER_GET_CLASS (saver)->get_file_size (saver);
}

goffset
gedit_document_saver_get_bytes_written (GeditDocumentSaver *saver)
{
	g_return_val_if_fail (GEDIT_IS_DOCUMENT_SAVER (saver), 0);

	return GEDIT_DOCUMENT_SAVER_GET_CLASS (saver)->get_bytes_written (saver);
}
