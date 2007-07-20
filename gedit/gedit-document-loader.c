/*
 * gedit-document-loader.c
 * This file is part of gedit
 *
 * Copyright (C) 2005 - Paolo Maggi
 * Copyright (C) 2007 - Paolo Maggi, Steve Frécinaux
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
 * Modified by the gedit Team, 2005-2007. See the AUTHORS file for a
 * list of people on the gedit Team.
 * See the ChangeLog files for a list of changes.
 *
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n.h>

#include "gedit-document-loader.h"
#include "gedit-debug.h"
#include "gedit-metadata-manager.h"
#include "gedit-utils.h"
#include "gedit-convert.h"
#include "gedit-marshal.h"

/* Those are for the the gedit_document_loader_new() factory */
#include "gedit-mmap-document-loader.h"
#include "gedit-gnomevfs-document-loader.h"

G_DEFINE_ABSTRACT_TYPE(GeditDocumentLoader, gedit_document_loader, G_TYPE_OBJECT)

/* Signals */

enum {
	LOADING,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

/* Properties */

enum
{
	PROP_0,
	PROP_DOCUMENT,
	PROP_URI
};

static void
gedit_document_loader_set_property (GObject      *object,
				    guint         prop_id,
				    const GValue *value,
				    GParamSpec   *pspec)
{
	GeditDocumentLoader *loader = GEDIT_DOCUMENT_LOADER (object);

	switch (prop_id)
	{
		case PROP_DOCUMENT:
			g_return_if_fail (loader->document == NULL);
			loader->document = g_value_get_object (value);
			break;
		case PROP_URI:
			g_return_if_fail (loader->uri == NULL);
			loader->uri = g_value_dup_string (value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_document_loader_get_property (GObject    *object,
				    guint       prop_id,
				    GValue     *value,
				    GParamSpec *pspec)
{
	GeditDocumentLoader *loader = GEDIT_DOCUMENT_LOADER (object);

	switch (prop_id)
	{
		case PROP_DOCUMENT:
			g_value_set_object (value, loader->document);
			break;
		case PROP_URI:
			g_value_set_string (value, loader->uri);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_document_loader_finalize (GObject *object)
{
	GeditDocumentLoader *loader = GEDIT_DOCUMENT_LOADER (object);

	g_free (loader->uri);

	G_OBJECT_CLASS (gedit_document_loader_parent_class)->finalize (object);
}

static void
gedit_document_loader_class_init (GeditDocumentLoaderClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = gedit_document_loader_finalize;
	object_class->get_property = gedit_document_loader_get_property;
	object_class->set_property = gedit_document_loader_set_property;

	g_object_class_install_property (object_class,
					 PROP_DOCUMENT,
					 g_param_spec_object ("document",
							 "Document",
							 "The GeditDocument this GeditDocumentLoader is associated with",
							 GEDIT_TYPE_DOCUMENT,
							 G_PARAM_READWRITE |
							 G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property (object_class,
					 PROP_URI,
					 g_param_spec_string ("uri",
							 "URI",
							 "The URI this GeditDocumentLoader loads the document from",
							 "",
							 G_PARAM_READWRITE |
							 G_PARAM_CONSTRUCT_ONLY));

	signals[LOADING] =
		g_signal_new ("loading",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GeditDocumentLoaderClass, loading),
			      NULL, NULL,
			      gedit_marshal_VOID__BOOLEAN_POINTER,
			      G_TYPE_NONE,
			      2,
			      G_TYPE_BOOLEAN,
			      G_TYPE_POINTER);
}

static void
gedit_document_loader_init (GeditDocumentLoader *loader)
{
	loader->used = FALSE;
}

static void
insert_text_in_document (GeditDocumentLoader *loader,
			 const gchar         *text,
			 gint                 len)
{
	GeditDocument *doc = loader->document;

	g_return_if_fail (text != NULL);

	gtk_source_buffer_begin_not_undoable_action (GTK_SOURCE_BUFFER (doc));

	/* If the last char is a newline, don't add it to the buffer (otherwise
	   GtkTextView shows it as an empty line). See bug #324942. */
	if ((len > 0) && (text[len-1] == '\n'))
		len--;

	/* Insert text in the buffer */
	gtk_text_buffer_set_text (GTK_TEXT_BUFFER (doc), text, len);

	gtk_text_buffer_set_modified (GTK_TEXT_BUFFER (doc), FALSE);

	gtk_source_buffer_end_not_undoable_action (GTK_SOURCE_BUFFER (doc));
}

/* This function is only meant to be called by child classes */
gboolean
gedit_document_loader_update_document_contents (GeditDocumentLoader  *loader,
					        const gchar          *file_contents,
					        gint                  file_size,
					        GError              **error)
{
	gedit_debug (DEBUG_LOADER);

	g_return_val_if_fail (file_size > 0, FALSE);
	g_return_val_if_fail (file_contents != NULL, FALSE);

	/* short-circuit the case where the file is empty */
	if (file_size == 0)
	{
		if (loader->encoding == NULL)
			loader->auto_detected_encoding = gedit_encoding_get_current ();
		insert_text_in_document (loader, "", 0);
		return TRUE;
	}

	if (loader->encoding == gedit_encoding_get_utf8 ())
	{
		if (g_utf8_validate (file_contents, file_size, NULL))
		{
			insert_text_in_document (loader,
						 file_contents,
						 file_size);
			return TRUE;
		}
		else
		{
			g_set_error (error,
				     G_CONVERT_ERROR,
				     G_CONVERT_ERROR_ILLEGAL_SEQUENCE,
				     "The file you are trying to open contains an invalid byte sequence.");
			return FALSE;
		}
	}
	else
	{
		GError *conv_error = NULL;
		gchar *converted_text = NULL;
		gsize new_len = file_size;

		if (loader->encoding == NULL)
		{
			/* Autodetecting the encoding: first try with the encoding
			stored in the metadata, if any */

			if (loader->metadata_encoding != NULL)
			{
				converted_text = gedit_convert_to_utf8 (
								file_contents,
								file_size,
								&loader->metadata_encoding,
								&new_len,
								NULL);

				if (converted_text != NULL)
					loader->auto_detected_encoding = loader->metadata_encoding;
			}
		}

		if (converted_text == NULL)
		{
			loader->auto_detected_encoding = loader->encoding;

			converted_text = gedit_convert_to_utf8 (
							file_contents,
							file_size,
							&loader->auto_detected_encoding,
							&new_len,
							&conv_error);
		}

		if (converted_text == NULL)
		{
			g_return_val_if_fail (conv_error != NULL, FALSE);

			g_propagate_error (error, conv_error);

			return FALSE;
		}
		else
		{
			insert_text_in_document (loader,
						 converted_text,
						 new_len);

			g_free (converted_text);

			return TRUE;
		}
	}

	g_return_val_if_reached (FALSE);
}

void
gedit_document_loader_loading (GeditDocumentLoader *loader,
			       gboolean             completed,
			       GError              *error)
{
	/* the object will be unrefed in the callback of the loading signal
	 * (when completed == TRUE), so we need to prevent finalization.
	 */
	if (completed)
	{
		g_object_ref (loader);
	}

	g_signal_emit (loader, signals[LOADING], 0, completed, error);

	if (completed)
	{
		if (error == NULL)
			gedit_debug_message (DEBUG_LOADER, "load completed");
		else
			gedit_debug_message (DEBUG_LOADER, "load failed");

		g_object_unref (loader);
	}
}

static const GeditEncoding *
get_metadata_encoding (const gchar *uri)
{
	const GeditEncoding *enc;
	gchar *charset;

	charset = gedit_metadata_manager_get (uri, "encoding");

	if (charset == NULL)
		return NULL;

	enc = gedit_encoding_get_from_charset (charset);

	g_free (charset);

	return enc;
}

GeditDocumentLoader *
gedit_document_loader_new (GeditDocument       *doc,
			   const gchar         *uri,
			   const GeditEncoding *encoding)
{
	GeditDocumentLoader *loader;
	GType loader_type;

	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), NULL);

	if (gedit_utils_uri_has_file_scheme (uri))
		loader_type = GEDIT_TYPE_MMAP_DOCUMENT_LOADER;
	else
		loader_type = GEDIT_TYPE_GNOMEVFS_DOCUMENT_LOADER;

	loader = GEDIT_DOCUMENT_LOADER (g_object_new (loader_type,
						      "document", doc,
						      "uri", uri,
						      NULL));

	loader->encoding = encoding;

	if (encoding == NULL)
	{
		loader->metadata_encoding = get_metadata_encoding (uri);
	}

	return loader;
}

/* If enconding == NULL, the encoding will be autodetected */
void
gedit_document_loader_load (GeditDocumentLoader *loader)
{
	gedit_debug (DEBUG_LOADER);

	g_return_if_fail (GEDIT_IS_DOCUMENT_LOADER (loader));

	/* the loader can be used just once, then it must be thrown away */
	g_return_if_fail (loader->used == FALSE);
	loader->used = TRUE;

	GEDIT_DOCUMENT_LOADER_GET_CLASS (loader)->load (loader);
}

gboolean 
gedit_document_loader_cancel (GeditDocumentLoader *loader)
{
	gedit_debug (DEBUG_LOADER);

	g_return_val_if_fail (GEDIT_IS_DOCUMENT_LOADER (loader), FALSE);

	return GEDIT_DOCUMENT_LOADER_GET_CLASS (loader)->cancel (loader);
}

/* Returns STDIN_URI if loading from stdin */
const gchar *
gedit_document_loader_get_uri (GeditDocumentLoader *loader)
{
	g_return_val_if_fail (GEDIT_IS_DOCUMENT_LOADER (loader), NULL);

	return loader->uri;
}

/* it may return NULL, it's up to gedit-document handle it */
const gchar *
gedit_document_loader_get_mime_type (GeditDocumentLoader *loader)
{
	g_return_val_if_fail (GEDIT_IS_DOCUMENT_LOADER (loader), NULL);

	return GEDIT_DOCUMENT_LOADER_GET_CLASS (loader)->get_mime_type (loader);
}

time_t
gedit_document_loader_get_mtime (GeditDocumentLoader *loader)
{
	g_return_val_if_fail (GEDIT_IS_DOCUMENT_LOADER (loader), 0);

	return GEDIT_DOCUMENT_LOADER_GET_CLASS (loader)->get_mtime (loader);
}

/* Returns 0 if file size is unknown */
GnomeVFSFileSize
gedit_document_loader_get_file_size (GeditDocumentLoader *loader)
{
	g_return_val_if_fail (GEDIT_IS_DOCUMENT_LOADER (loader), 0);

	return GEDIT_DOCUMENT_LOADER_GET_CLASS (loader)->get_file_size (loader);
}

GnomeVFSFileSize
gedit_document_loader_get_bytes_read (GeditDocumentLoader *loader)
{
	g_return_val_if_fail (GEDIT_IS_DOCUMENT_LOADER (loader), 0);

	return GEDIT_DOCUMENT_LOADER_GET_CLASS (loader)->get_bytes_read (loader);
}

/* In the case the loader does not know if the file is readonly, for example 
   for most remote files, the function returns FALSE, so that we can try writing
   and if needed handle the error. */
gboolean
gedit_document_loader_get_readonly (GeditDocumentLoader *loader)
{
	g_return_val_if_fail (GEDIT_IS_DOCUMENT_LOADER (loader), FALSE);

	/* if configuration says the scheme is not writable, do not query the
	   actual loader object, and return TRUE */
	if (!gedit_utils_uri_has_writable_scheme (loader->uri))
		return TRUE;

	return GEDIT_DOCUMENT_LOADER_GET_CLASS (loader)->get_readonly (loader);
}

const GeditEncoding *
gedit_document_loader_get_encoding (GeditDocumentLoader *loader)
{
	g_return_val_if_fail (GEDIT_IS_DOCUMENT_LOADER (loader), NULL);

	if (loader->encoding != NULL)
		return loader->encoding;

	g_return_val_if_fail (loader->auto_detected_encoding != NULL, 
			      gedit_encoding_get_current ());
			  
	return loader->auto_detected_encoding;
}
