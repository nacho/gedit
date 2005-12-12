/*
 * gedit-document.c
 * This file is part of gedit
 *
 * Copyright (C) 1998, 1999 Alex Roberts, Evan Lawrence
 * Copyright (C) 2000, 2001 Chema Celorio, Paolo Maggi 
 * Copyright (C) 2002-2005 Paolo Maggi 
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
 * Modified by the gedit Team, 1998-2005. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 *
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <stdlib.h>

#include <glib/gi18n.h>
#include <libgnomevfs/gnome-vfs.h>

#include "gedit-prefs-manager-app.h"
#include "gedit-document.h"
#include "gedit-debug.h"
#include "gedit-utils.h"
#include "gedit-convert.h"
#include "gedit-metadata-manager.h"
#include "gedit-languages-manager.h"
#include "gedit-document-loader.h"
#include "gedit-document-saver.h"
#include "gedit-marshal.h"

#include <gtksourceview/gtksourceiter.h>

#undef ENABLE_PROFILE 

#ifdef ENABLE_PROFILE
#define PROFILE(x) x
#else
#define PROFILE(x)
#endif

PROFILE (static GTimer *timer = NULL);

#ifdef MAXPATHLEN
#define GEDIT_MAX_PATH_LEN  MAXPATHLEN
#elif defined (PATH_MAX)
#define GEDIT_MAX_PATH_LEN  PATH_MAX
#else
#define GEDIT_MAX_PATH_LEN  2048
#endif

#define GEDIT_DOCUMENT_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), GEDIT_TYPE_DOCUMENT, GeditDocumentPrivate))

static void	gedit_document_set_readonly	(GeditDocument *doc,
						 gboolean       readonly);
			     
struct _GeditDocumentPrivate
{
	gint	     auto_save : 1;
	gint	     readonly : 1;
	gint	     last_save_was_manually : 1; 	
	gint	     language_set_by_user : 1;
	gint         is_saving_as : 1;
	gint         has_selection : 1;
	gint         stop_cursor_moved_emission : 1;

	gchar	    *uri;
	gint 	     untitled_number;

	GnomeVFSURI *vfs_uri;

	const GeditEncoding *encoding;

	gchar	    *mime_type;

	gint	     auto_save_interval;
	guint	     auto_save_timeout;

	time_t       mtime;

	GTimeVal     time_of_last_save_or_load;

	guint        search_flags;
	gchar       *search_text;

	/* Temp data while loading */
	GeditDocumentLoader *loader;
	gboolean             create; /* Create file if uri points 
	                              * to a non existing file */
	const GeditEncoding *requested_encoding;
	gint                 requested_line_pos;

	/* Saving stuff */
	GeditDocumentSaver *saver;
};

enum {
	PROP_0,

	PROP_URI,
	PROP_SHORTNAME,
	PROP_MIME_TYPE,
	PROP_READ_ONLY,
	PROP_ENCODING,
	PROP_CAN_SEARCH_AGAIN,
	PROP_HAS_SELECTION
};

enum {
	CURSOR_MOVED,
	LOADING,
	LOADED,
	SAVING,
	SAVED,
	LAST_SIGNAL
};

static guint document_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE(GeditDocument, gedit_document, GTK_TYPE_SOURCE_BUFFER)

GQuark
gedit_document_error_quark (void)
{
	static GQuark quark;

	if (!quark)
		quark = g_quark_from_static_string ("gedit_io_load_error");

	return quark;
}

static GHashTable *allocated_untitled_numbers = NULL;

static gint
get_untitled_number (void)
{
	gint i = 1;

	if (allocated_untitled_numbers == NULL)
		allocated_untitled_numbers = g_hash_table_new (NULL, NULL);

	g_return_val_if_fail (allocated_untitled_numbers != NULL, -1);

	while (TRUE)
	{
		if (g_hash_table_lookup (allocated_untitled_numbers, GINT_TO_POINTER (i)) == NULL)
		{
			g_hash_table_insert (allocated_untitled_numbers, 
					     GINT_TO_POINTER (i),
					     GINT_TO_POINTER (i));

			return i;
		}

		++i;
	}
}

static void
release_untitled_number (gint n)
{
	g_return_if_fail (allocated_untitled_numbers != NULL);

	g_hash_table_remove (allocated_untitled_numbers, GINT_TO_POINTER (n));
}

static void
gedit_document_finalize (GObject *object)
{
	GeditDocument *doc = GEDIT_DOCUMENT (object); 
	
	gedit_debug (DEBUG_DOCUMENT);

	if (doc->priv->auto_save_timeout > 0)
		g_source_remove (doc->priv->auto_save_timeout);

	if (doc->priv->untitled_number > 0)
	{
		g_return_if_fail (doc->priv->uri == NULL);
		release_untitled_number (doc->priv->untitled_number);
	}

	if (doc->priv->uri != NULL)
	{
		GtkTextIter iter;
		gchar *position;
		gchar *lang_id = NULL;
		GtkSourceLanguage *lang;

		gtk_text_buffer_get_iter_at_mark (
				GTK_TEXT_BUFFER (doc),			
				&iter,
				gtk_text_buffer_get_insert (GTK_TEXT_BUFFER (doc)));

		position = g_strdup_printf ("%d", 
					    gtk_text_iter_get_offset (&iter));

		gedit_metadata_manager_set (doc->priv->uri,
					    "position",
					    position);
		g_free (position);

		if (doc->priv->language_set_by_user)
		{
			lang = gedit_document_get_language (doc);

			if (lang != NULL)
				lang_id = gtk_source_language_get_id (lang);

			gedit_metadata_manager_set (doc->priv->uri,
						    "language",
						    (lang_id == NULL) ? "_NORMAL_" : lang_id);
			g_free (lang_id);
		}
	}

	g_free (doc->priv->uri);
	if (doc->priv->vfs_uri != NULL)
		gnome_vfs_uri_unref (doc->priv->vfs_uri);

	g_free (doc->priv->mime_type);

	if (doc->priv->loader)
		g_object_unref (doc->priv->loader);

	g_free (doc->priv->search_text);

	G_OBJECT_CLASS (gedit_document_parent_class)->finalize (object);
}

static void
gedit_document_get_property (GObject    *object,
			     guint       prop_id,
			     GValue     *value,
			     GParamSpec *pspec)
{
	GeditDocument *doc = GEDIT_DOCUMENT (object);

	switch (prop_id)
	{
		case PROP_URI:
			g_value_set_string (value, doc->priv->uri);
			break;
		case PROP_SHORTNAME:
			g_value_take_string (value, gedit_document_get_short_name_for_display (doc));
			break;
		case PROP_MIME_TYPE:
			g_value_set_string (value, doc->priv->mime_type);
			break;
		case PROP_READ_ONLY:
			g_value_set_boolean (value, doc->priv->readonly);
			break;
		case PROP_ENCODING:
			g_value_set_boxed (value, doc->priv->encoding);
			break;
		case PROP_CAN_SEARCH_AGAIN:
			g_value_set_boolean (value, gedit_document_get_can_search_again (doc));
			break;
		case PROP_HAS_SELECTION:
			g_value_set_boolean (value, doc->priv->has_selection);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
emit_cursor_moved (GeditDocument *doc)
{
	if (!doc->priv->stop_cursor_moved_emission)
	{
		g_signal_emit (doc,
			       document_signals[CURSOR_MOVED],
			       0);
	}
}

static void
gedit_document_mark_set (GtkTextBuffer     *buffer,
                         const GtkTextIter *iter,
                         GtkTextMark       *mark)
{
	GeditDocument *doc = GEDIT_DOCUMENT (buffer);

	if (GTK_TEXT_BUFFER_CLASS (gedit_document_parent_class)->mark_set)
		GTK_TEXT_BUFFER_CLASS (gedit_document_parent_class)->mark_set (buffer,
									       iter,
									       mark);

	if (mark == gtk_text_buffer_get_insert (buffer) ||
	    mark == gtk_text_buffer_get_selection_bound (buffer))
	{
		gboolean has_selection;

		has_selection = gtk_text_buffer_get_selection_bounds (buffer,
								      NULL,
								      NULL);

		if (has_selection != doc->priv->has_selection)
		{
			doc->priv->has_selection = has_selection;
			g_object_notify (G_OBJECT (doc), "has-selection");
		}
	}

	if (mark == gtk_text_buffer_get_insert (buffer))
	{
		emit_cursor_moved (doc);
	}
}

static void
gedit_document_changed (GtkTextBuffer *buffer)
{
	emit_cursor_moved (GEDIT_DOCUMENT (buffer));

	GTK_TEXT_BUFFER_CLASS (gedit_document_parent_class)->changed (buffer);
}

static void 
gedit_document_class_init (GeditDocumentClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkTextBufferClass *buf_class = GTK_TEXT_BUFFER_CLASS (klass);

	object_class->finalize = gedit_document_finalize;
	object_class->get_property = gedit_document_get_property;

	buf_class->mark_set = gedit_document_mark_set;
	buf_class->changed = gedit_document_changed;

	g_object_class_install_property (object_class, PROP_URI,
					 g_param_spec_string ("uri",
					 		      "URI",
					 		      "The document's URI",
					 		      NULL,
					 		      G_PARAM_READABLE));
	g_object_class_install_property (object_class, PROP_SHORTNAME,
					 g_param_spec_string ("shortname",
					 		      "Short Name",
					 		      "The document's short name",
					 		      NULL,
					 		      G_PARAM_READABLE));
	g_object_class_install_property (object_class, PROP_MIME_TYPE,
					 g_param_spec_string ("mime-type",
					 		      "MIME Type",
					 		      "The document's MIME Type",
					 		      "text/plain",
					 		      G_PARAM_READABLE));
	g_object_class_install_property (object_class, PROP_READ_ONLY,
					 g_param_spec_boolean ("read-only",
					 		       "Read Only",
					 		       "Whether the document is read only or not",
					 		       FALSE,
					 		       G_PARAM_READABLE));
	g_object_class_install_property (object_class, PROP_ENCODING,
					 g_param_spec_boxed ("encoding",
					 		     "Encoding",
					 		     "The GeditEncoding used for the document",
					 		     GEDIT_TYPE_ENCODING,
					 		     G_PARAM_READABLE));
	g_object_class_install_property (object_class, PROP_CAN_SEARCH_AGAIN,
					 g_param_spec_boolean ("can-search-again",
					 		       "Can search again",
					 		       "Wheter it's possible to search again in the document",
					 		       FALSE,
					 		       G_PARAM_READABLE));

	/* This has been properly moved in GtkTextBuffer in gtk 2.10, so when
	 * we switch to 2.10 we can remove it and part of with gedit_document_mark_set.
	 */
	g_object_class_install_property (object_class, PROP_HAS_SELECTION,
					 g_param_spec_boolean ("has-selection",
					 		       "Has selection",
					 		       "Wheter the document has selected text",
					 		       FALSE,
					 		       G_PARAM_READABLE));

	/* This signal is used to update the cursor position is the statusbar,
	 * it's emitted either when the insert mark is moved explicitely or
	 * when the buffer changes (insert/delete).
	 * We prevent the emission of the signal during replace_all to 
	 * improve performance.
	 */
	document_signals[CURSOR_MOVED] =
   		g_signal_new ("cursor-moved",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GeditDocumentClass, cursor_moved),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);

	document_signals[LOADING] =
   		g_signal_new ("loading",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GeditDocumentClass, loading),
			      NULL, NULL,
			      gedit_marshal_VOID__UINT64_UINT64,
			      G_TYPE_NONE,
			      2,
			      G_TYPE_UINT64,
			      G_TYPE_UINT64);

	document_signals[LOADED] =
   		g_signal_new ("loaded",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GeditDocumentClass, loaded),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__POINTER,
			      G_TYPE_NONE,
			      1,
			      G_TYPE_POINTER);

	document_signals[SAVING] =
   		g_signal_new ("saving",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GeditDocumentClass, saving),
			      NULL, NULL,
			      gedit_marshal_VOID__UINT64_UINT64,
			      G_TYPE_NONE,
			      2,
			      G_TYPE_UINT64,
			      G_TYPE_UINT64);

	document_signals[SAVED] =
   		g_signal_new ("saved",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GeditDocumentClass, saved),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__POINTER,
			      G_TYPE_NONE,
			      1,
			      G_TYPE_POINTER);

	g_type_class_add_private (object_class, sizeof(GeditDocumentPrivate));
}

static void
set_language (GeditDocument     *doc, 
              GtkSourceLanguage *lang,
              gboolean           set_by_user)
{
	GtkSourceLanguage *old_lang;

	gedit_debug (DEBUG_DOCUMENT);
	
	old_lang = gtk_source_buffer_get_language (GTK_SOURCE_BUFFER (doc));
	
	if (old_lang == lang)
		return;

	gtk_source_buffer_set_language (GTK_SOURCE_BUFFER (doc), lang);
	if (lang != NULL)
		gedit_language_init_tag_styles (lang);
		
	if (lang != NULL)
		gtk_source_buffer_set_highlight (GTK_SOURCE_BUFFER (doc),
						 gedit_prefs_manager_get_enable_syntax_highlighting ());
	else
		gtk_source_buffer_set_highlight (GTK_SOURCE_BUFFER (doc), 
						 FALSE);

	if (set_by_user && (doc->priv->uri != NULL))
	{
		gchar *lang_id = NULL;

		if (lang != NULL)
		{
			lang_id = gtk_source_language_get_id (lang);
			g_return_if_fail (lang_id != NULL);
		}
		
		gedit_metadata_manager_set (doc->priv->uri,
					    "language",
					    (lang_id == NULL) ? "_NORMAL_" : lang_id);

		g_free (lang_id);
	}
	
	doc->priv->language_set_by_user = set_by_user;
}

static void
set_encoding (GeditDocument       *doc, 
	      const GeditEncoding *encoding,
	      gboolean             set_by_user)
{
	g_return_if_fail (encoding != NULL);

	gedit_debug (DEBUG_DOCUMENT);

	if (doc->priv->encoding == encoding)
		return;

	doc->priv->encoding = encoding;

	if (set_by_user)
	{
		const gchar *charset;

		charset = gedit_encoding_get_charset (encoding);

		gedit_metadata_manager_set (doc->priv->uri,
					    "encoding",
					    charset);
	}

	g_object_notify (G_OBJECT (doc), "encoding");
}

static void
gedit_document_init (GeditDocument *doc)
{
	gedit_debug (DEBUG_DOCUMENT);

	doc->priv = GEDIT_DOCUMENT_GET_PRIVATE (doc);

	doc->priv->uri = NULL;
	doc->priv->vfs_uri = NULL;
	doc->priv->untitled_number = get_untitled_number ();

	doc->priv->mime_type = g_strdup ("text/plain");

	doc->priv->readonly = FALSE;

	doc->priv->has_selection = FALSE;
	doc->priv->stop_cursor_moved_emission = FALSE;

	doc->priv->last_save_was_manually = TRUE;
	doc->priv->language_set_by_user = FALSE;

	doc->priv->mtime = 0;

	g_get_current_time (&doc->priv->time_of_last_save_or_load);

	doc->priv->encoding = gedit_encoding_get_utf8 ();

	gtk_source_buffer_set_max_undo_levels (GTK_SOURCE_BUFFER (doc), 
					       gedit_prefs_manager_get_undo_actions_limit ());

	/* TODO: Set the bracket matching tag style -- Paolo (10 Jan. 2005) */
	gtk_source_buffer_set_check_brackets (GTK_SOURCE_BUFFER (doc), 
					      gedit_prefs_manager_get_bracket_matching ());

	doc->priv->auto_save = gedit_prefs_manager_get_auto_save ();
	doc->priv->auto_save = (doc->priv->auto_save != FALSE);

	doc->priv->auto_save_interval = gedit_prefs_manager_get_auto_save_interval ();
	if (doc->priv->auto_save_interval <= 0)
		doc->priv->auto_save_interval = GPM_DEFAULT_AUTO_SAVE_INTERVAL;
}

GeditDocument *
gedit_document_new (void)
{
	gedit_debug (DEBUG_DOCUMENT);

	return GEDIT_DOCUMENT (g_object_new (GEDIT_TYPE_DOCUMENT, NULL));
}

/* If mime type is null, we guess from the filename */
/* If uri is null, we only set the mime-type */
static void
set_uri (GeditDocument *doc,
	 const gchar   *uri,
	 const gchar   *mime_type)
{
	gedit_debug (DEBUG_DOCUMENT);

	g_return_if_fail ((uri == NULL) || gedit_utils_is_valid_uri (uri));

	if (uri != NULL)
	{
		if (doc->priv->uri == uri)
			return;

		g_free (doc->priv->uri);
		doc->priv->uri = g_strdup (uri);

		if (doc->priv->vfs_uri != NULL)
			gnome_vfs_uri_unref (doc->priv->vfs_uri);

		doc->priv->vfs_uri = gnome_vfs_uri_new (uri);
		g_return_if_fail (doc->priv->vfs_uri != NULL);

		if (doc->priv->untitled_number > 0)
		{
			release_untitled_number (doc->priv->untitled_number);
			doc->priv->untitled_number = 0;
		}
	}

	g_return_if_fail (doc->priv->vfs_uri != NULL);

	g_free (doc->priv->mime_type);
	if (mime_type != NULL)
	{
		doc->priv->mime_type = g_strdup (mime_type);
	}
	else
	{
		gchar *base_name;

		/* Set the mime type using the file extension or "text/plain" 
	   	 * if no match. */
		base_name = gnome_vfs_uri_extract_short_path_name (doc->priv->vfs_uri);
		if (base_name != NULL) 
			doc->priv->mime_type = g_strdup ("text/plain"); // FIXME
//			gnome_vfs_mime_type_from_name_or_default (base_name,
//								  "text/plain");
		else
			doc->priv->mime_type = g_strdup ("text/plain");

		g_free (base_name);
	}

	if (!doc->priv->language_set_by_user)
	{
		gchar *data;
		GtkSourceLanguage *language = NULL;

		data = gedit_metadata_manager_get (doc->priv->uri, "language");

		if (data != NULL)
		{
			gedit_debug_message (DEBUG_DOCUMENT, "Language: %s", data);

			if (strcmp (data, "_NORMAL_") != 0)
			{
				language = gedit_languages_manager_get_language_from_id (
							gedit_get_languages_manager (),
							data);
			}

			g_free (data);
		}
		else
		{
			gedit_debug_message (DEBUG_DOCUMENT, "Language Normal");

			if (strcmp (doc->priv->mime_type, "text/plain") != 0)
			{
				language = gtk_source_languages_manager_get_language_from_mime_type (
							gedit_get_languages_manager (),
							doc->priv->mime_type);
			}
		}

		set_language (doc, language, FALSE);
	}

	g_object_notify (G_OBJECT (doc), "uri");
	g_object_notify (G_OBJECT (doc), "shortname");
}

gchar *
gedit_document_get_uri (GeditDocument *doc)
{
	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), NULL);

	return g_strdup (doc->priv->uri);
}

/* Never returns NULL */
gchar *
gedit_document_get_uri_for_display (GeditDocument *doc)
{
	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), "");

	if (doc->priv->uri == NULL)
		return g_strdup_printf (_("Unsaved Document %d"),
					doc->priv->untitled_number);	      
	else
		return gnome_vfs_format_uri_for_display (doc->priv->uri);
}

/* move to gedit-utils? */
static gchar *
get_uri_shortname_for_display (GnomeVFSURI *uri)
{
	gchar *name;	
	gboolean  validated;

	validated = FALSE;
	name = gnome_vfs_uri_extract_short_name (uri);
	
	if (name == NULL)
	{
		name = gnome_vfs_uri_to_string (uri, GNOME_VFS_URI_HIDE_PASSWORD);
	}
	else if (g_ascii_strcasecmp (uri->method_string, "file") == 0)
	{
		gchar *text_uri;
		gchar *local_file;
		text_uri = gnome_vfs_uri_to_string (uri, GNOME_VFS_URI_HIDE_PASSWORD);
		local_file = gnome_vfs_get_local_path_from_uri (text_uri);

		if (local_file != NULL)
		{
			g_free (name);
			name = g_filename_display_basename (local_file);
			validated = TRUE;
		}

		g_free (local_file);
		g_free (text_uri);
	} 
	else if (!gnome_vfs_uri_has_parent (uri)) 
	{
		const gchar *method;

		method = uri->method_string;

		if (name == NULL ||
		    strcmp (name, GNOME_VFS_URI_PATH_STR) == 0) 
		{
			g_free (name);
			name = g_strdup (method);
		} 
		/*
		else 
		{
			gchar *tmp;

			tmp = name;
			name = g_strdup_printf ("%s: %s", method, name);
			g_free (tmp);
		}
		*/
	}

	if (!validated && !g_utf8_validate (name, -1, NULL)) 
	{
		gchar *utf8_name;

		utf8_name = gedit_utils_make_valid_utf8 (name);
		g_free (name);
		name = utf8_name;
	}

	return name;
}

/* Never returns NULL */
gchar *
gedit_document_get_short_name_for_display (GeditDocument *doc)
{
	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), "");

	if (doc->priv->uri == NULL)
		return g_strdup_printf (_("Unsaved Document %d"),
					doc->priv->untitled_number);	      
	else
		return get_uri_shortname_for_display (doc->priv->vfs_uri);
}

/* Never returns NULL */
gchar *
gedit_document_get_mime_type (GeditDocument *doc)
{
	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), "text/plain");
	g_return_val_if_fail (doc->priv->mime_type != NULL, "text/plain");

 	return g_strdup (doc->priv->mime_type);
}

/* Note: do not emit the notify::read-only signal */
static void
set_readonly (GeditDocument *doc,
	      gboolean       readonly)
{
	gedit_debug (DEBUG_DOCUMENT);
	
	readonly = (readonly != FALSE);

	if (readonly) 
	{
		if (doc->priv->auto_save_timeout > 0)
		{
			g_source_remove (doc->priv->auto_save_timeout);
			doc->priv->auto_save_timeout = 0;
		}
	}
	else
	{
		if (doc->priv->auto_save &&
		    (doc->priv->auto_save_timeout <= 0) && 
                    !gedit_document_is_untitled (doc))
		{
/*			doc->priv->auto_save_timeout = g_timeout_add
				 (doc->priv->auto_save_interval * 1000 * 60,
		 		 (GSourceFunc)gedit_document_auto_save,
		  		 doc);
*/
		}
	}

	if (doc->priv->readonly == readonly) 
		return;

	doc->priv->readonly = readonly;
}

static void
gedit_document_set_readonly (GeditDocument *doc,
			     gboolean       readonly)
{
	gedit_debug (DEBUG_DOCUMENT);

	g_return_if_fail (GEDIT_IS_DOCUMENT (doc));

	set_readonly (doc, readonly);

	g_object_notify (G_OBJECT (doc), "read-only");
}

gboolean
gedit_document_get_readonly (GeditDocument *doc)
{
	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), TRUE);

	return doc->priv->readonly;
}

static void
reset_temp_loading_data (GeditDocument       *doc)
{
	/* the loader has been used, throw it away */
	g_object_unref (doc->priv->loader);
	doc->priv->loader = NULL;

	doc->priv->requested_encoding = NULL;
	doc->priv->requested_line_pos = 0;
}

static void
document_loader_loaded (GeditDocumentLoader *loader,
			const GError        *error,
			GeditDocument       *doc)
{
	/* load was successful */
	if (error == NULL)
	{
		GtkTextIter iter;
		const gchar *mime_type;

		mime_type = gedit_document_loader_get_mime_type (loader);

		doc->priv->mtime = gedit_document_loader_get_mtime (loader);

		g_get_current_time (&doc->priv->time_of_last_save_or_load);

		set_readonly (doc,
		      gedit_document_loader_get_readonly (loader));

		set_encoding (doc, 
			      gedit_document_loader_get_encoding (loader),
			      (doc->priv->requested_encoding != NULL));
		      
		/* We already set the uri */
		set_uri (doc, NULL, mime_type);

		/* move the cursor at the requested line if any */
		if (doc->priv->requested_line_pos > 0)
		{
			/* line_pos - 1 because get_iter_at_line counts from 0 */
			gtk_text_buffer_get_iter_at_line (GTK_TEXT_BUFFER (doc),
							  &iter,
							  doc->priv->requested_line_pos - 1);
		}

		/* else, if enabled, to the position stored in the metadata */
		else if (0) // FIXME: should be a GConf option
		{
			gchar *pos;
			gint offset;

			pos = gedit_metadata_manager_get (doc->priv->uri,
							  "position");
			offset = pos ? atoi (pos) : 0;
			g_free (pos);

			gtk_text_buffer_get_iter_at_offset (GTK_TEXT_BUFFER (doc),
							    &iter,
							    MAX (offset, 0));
		}

		/* otherwise to the top */
		else
		{
			gtk_text_buffer_get_start_iter (GTK_TEXT_BUFFER (doc),
							&iter);
		}

		gtk_text_buffer_place_cursor (GTK_TEXT_BUFFER (doc), &iter);
	}

	/* special case creating a named new doc */
	else if (doc->priv->create &&
	         (error->code == GNOME_VFS_ERROR_NOT_FOUND))
	{
		reset_temp_loading_data (doc);
		// FIXME: do other stuff??

		g_signal_emit (doc,
			       document_signals[LOADED],
			       0,
			       NULL);

		return;
	}
	
	g_signal_emit (doc,
		       document_signals[LOADED],
		       0,
		       error);

	reset_temp_loading_data (doc);
}

static void
document_loader_loading (GeditDocumentLoader *loader,
			 gboolean             completed,
			 const GError        *error,
			 GeditDocument       *doc)
{
	if (completed)
	{
		document_loader_loaded (loader, error, doc);
	}
	else
	{
		GnomeVFSFileSize size;
		GnomeVFSFileSize read;

		size = gedit_document_loader_get_file_size (loader);
		read = gedit_document_loader_get_bytes_read (loader);

		g_signal_emit (doc, 
			       document_signals[LOADING],
			       0,
			       read,
			       size);
	}
}

gboolean
gedit_document_load (GeditDocument       *doc,
		     const gchar         *uri,
		     const GeditEncoding *encoding,
		     gint                 line_pos,
		     gboolean             create)
{
	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), FALSE);
	g_return_val_if_fail (uri != NULL, FALSE);
	g_return_val_if_fail (gedit_utils_is_valid_uri (uri), FALSE);

	g_return_val_if_fail (doc->priv->loader == NULL, FALSE);

	/* create a loader. It will be destroyed when loading is completed */
	doc->priv->loader = gedit_document_loader_new (doc);

	g_signal_connect (doc->priv->loader,
			  "loading",
			  G_CALLBACK (document_loader_loading),
			  doc);

	doc->priv->create = create;
	doc->priv->requested_encoding = encoding;
	doc->priv->requested_line_pos = line_pos;

	set_uri (doc, uri, NULL);

	return gedit_document_loader_load (doc->priv->loader,
					   uri,
					   encoding);
}

gboolean
gedit_document_load_cancel (GeditDocument *doc)
{
	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), FALSE);

	if (doc->priv->loader == NULL)
		return FALSE;

	return gedit_document_loader_cancel (doc->priv->loader);
}

static void
document_saver_saving (GeditDocumentSaver *saver,
		       gboolean            completed,
		       const GError       *error,
		       GeditDocument      *doc)
{
	/* FIXME */
	if (error)
		g_print ("error saving: %s\n", error->message);

	if (completed)
	{
		/* save was successful */
		if (error == NULL)
		{
			const gchar *uri;
			const gchar *mime_type;

			uri = gedit_document_saver_get_uri (saver);
			mime_type = gedit_document_saver_get_mime_type (saver);

			doc->priv->mtime = gedit_document_saver_get_mtime (saver);

			g_get_current_time (&doc->priv->time_of_last_save_or_load);

			gedit_document_set_readonly (doc, FALSE);

			gtk_text_buffer_set_modified (GTK_TEXT_BUFFER (doc),
						      FALSE);
			
			set_uri (doc, uri, mime_type);
			
			set_encoding (doc, 
				      doc->priv->requested_encoding, 
				      TRUE);
		}

		g_signal_emit (doc,
			       document_signals[SAVED],
			       0,
			       error);

		/* the saver has been used, throw it away */
		g_object_unref (doc->priv->saver);
		doc->priv->saver = NULL;
		doc->priv->is_saving_as = FALSE;
	}
	else
	{
		GnomeVFSFileSize size = 0;
		GnomeVFSFileSize written = 0;

		size = gedit_document_saver_get_file_size (saver);
		written = gedit_document_saver_get_bytes_written (saver);

		gedit_debug_message (DEBUG_DOCUMENT, "save progress: %Lu of %Lu", written, size);

		g_signal_emit (doc,
			       document_signals[SAVING],
			       0,
			       written,
			       size);
	}
}

static void
document_save_real (GeditDocument          *doc,
		    const gchar            *uri,
		    const GeditEncoding    *encoding,
		    time_t                  mtime,
		    GeditDocumentSaveFlags  flags)
{
	g_return_if_fail (doc->priv->saver == NULL);

	/* create a saver, it will be destroyed once saving is complete */
	doc->priv->saver = gedit_document_saver_new (doc);

	g_signal_connect (doc->priv->saver,
			  "saving",
			  G_CALLBACK (document_saver_saving),
			  doc);

	doc->priv->requested_encoding = encoding;

	gedit_document_saver_save (doc->priv->saver,
				   uri,
				   encoding,
				   mtime,
				   flags);
}

void
gedit_document_save (GeditDocument          *doc,
		     GeditDocumentSaveFlags  flags)
{
	g_return_if_fail (GEDIT_IS_DOCUMENT (doc));
	g_return_if_fail (doc->priv->uri != NULL);

	document_save_real (doc,
			    doc->priv->uri,
			    doc->priv->encoding,
			    doc->priv->mtime,
			    flags);
}

void
gedit_document_save_as (GeditDocument          *doc,
			const gchar            *uri,
			const GeditEncoding    *encoding,
			GeditDocumentSaveFlags  flags)
{
	g_return_if_fail (GEDIT_IS_DOCUMENT (doc));
	g_return_if_fail (uri != NULL);
	g_return_if_fail (encoding != NULL);

	doc->priv->is_saving_as = TRUE;
	
	document_save_real (doc, uri, encoding, 0, flags);
}

gboolean
gedit_document_insert_file (GeditDocument       *doc,
			    GtkTextIter         *iter,
			    const gchar         *uri,
			    const GeditEncoding *encoding)
{
	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), FALSE);
	g_return_val_if_fail (iter != NULL, FALSE);
	g_return_val_if_fail (gtk_text_iter_get_buffer (iter) == 
				GTK_TEXT_BUFFER (doc), FALSE);

	// TODO

	return FALSE;
}

gboolean	 
gedit_document_is_untouched (GeditDocument *doc)
{
	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), TRUE);

	return (doc->priv->uri == NULL) && 
		(!gtk_text_buffer_get_modified (GTK_TEXT_BUFFER (doc)));
}

gboolean 
gedit_document_is_untitled (GeditDocument *doc)
{
	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), TRUE);

	return (doc->priv->uri == NULL);
}

gboolean
gedit_document_get_deleted (GeditDocument *doc)
{
	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), FALSE);

	if (doc->priv->uri == NULL)
		return FALSE;
		
	return !gnome_vfs_uri_exists (doc->priv->vfs_uri);
}

gboolean
_gedit_document_get_has_selection (GeditDocument *doc)
{
	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), FALSE);

	return doc->priv->has_selection;
}

/*
 * If @line is bigger than the lines of the document, the cursor is moved
 * to the last line and FALSE is returned.
 */
gboolean
gedit_document_goto_line (GeditDocument *doc, 
			  gint           line)
{
	gboolean ret = TRUE;
	guint line_count;
	GtkTextIter iter;

	gedit_debug (DEBUG_DOCUMENT);

	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), FALSE);
	g_return_val_if_fail (line >= -1, FALSE);

	line_count = gtk_text_buffer_get_line_count (GTK_TEXT_BUFFER (doc));

	if (line > line_count)
	{
		ret = FALSE;
		line = line_count;
	}

	gtk_text_buffer_get_iter_at_line (GTK_TEXT_BUFFER (doc), &iter, line);
	gtk_text_buffer_place_cursor (GTK_TEXT_BUFFER (doc), &iter);

	return ret;
}

void
gedit_document_set_search_text (GeditDocument *doc,
				const gchar   *text,
				guint          flags)
{
	gchar *converted_text = NULL;
	gboolean notify;

	g_return_if_fail (GEDIT_IS_DOCUMENT (doc));
	g_return_if_fail ((text == NULL) || (doc->priv->search_text != text));
	g_return_if_fail ((text == NULL) || g_utf8_validate (text, -1, NULL));

	gedit_debug_message (DEBUG_DOCUMENT, "text = %s", text);

	g_free (doc->priv->search_text);

	if (text != NULL && *text != '\0')
	{
		converted_text = gedit_utils_unescape_search_text (text);
		notify = doc->priv->search_text == NULL;

		doc->priv->search_text = converted_text;
	}
	else
	{
		notify = doc->priv->search_text != NULL;
		doc->priv->search_text = NULL;
	}

	if (!GEDIT_SEARCH_IS_DONT_SET_FLAGS (flags))
	{
		doc->priv->search_flags = flags;
	}

	if (notify)
		g_object_notify (G_OBJECT (doc), "can-search-again");
}

gchar *
gedit_document_get_search_text (GeditDocument *doc,
				guint         *flags)
{
	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), NULL);

	if (flags != NULL)
		*flags = doc->priv->search_flags;

	return gedit_utils_escape_search_text (doc->priv->search_text);
}

gboolean
gedit_document_get_can_search_again (GeditDocument *doc)
{
	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), FALSE);

	return doc->priv->search_text != NULL;
}

gboolean
gedit_document_search_forward (GeditDocument     *doc,
			       const GtkTextIter *start,
			       const GtkTextIter *end,
			       GtkTextIter       *match_start,
			       GtkTextIter       *match_end)
{
	GtkTextIter iter;
	GtkSourceSearchFlags search_flags;
	gboolean found = FALSE;
	GtkTextIter m_start;
	GtkTextIter m_end;
	
	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), FALSE);
	g_return_val_if_fail ((start == NULL) || 
			      (gtk_text_iter_get_buffer (start) ==  GTK_TEXT_BUFFER (doc)), FALSE);
	g_return_val_if_fail ((end == NULL) || 
			      (gtk_text_iter_get_buffer (end) ==  GTK_TEXT_BUFFER (doc)), FALSE);
	
	if (doc->priv->search_text == NULL)
	{
		gedit_debug_message (DEBUG_DOCUMENT, "doc->priv->search_text == NULL\n");
		return FALSE;
	}
	else
		gedit_debug_message (DEBUG_DOCUMENT, "doc->priv->search_text == \"%s\"\n", doc->priv->search_text);
				      
	if (start == NULL)
		gtk_text_buffer_get_start_iter (GTK_TEXT_BUFFER (doc), &iter);
	else
		iter = *start;
		
	search_flags = GTK_SOURCE_SEARCH_VISIBLE_ONLY | GTK_SOURCE_SEARCH_TEXT_ONLY;

	if (!GEDIT_SEARCH_IS_CASE_SENSITIVE (doc->priv->search_flags))
	{
		search_flags = search_flags | GTK_SOURCE_SEARCH_CASE_INSENSITIVE;
	}
		
	while (!found)
	{
		found = gtk_source_iter_forward_search (&iter,
							doc->priv->search_text, 
							search_flags,
                        	                	&m_start, 
                        	                	&m_end,
                                	               	end);
      	               	
		if (found && GEDIT_SEARCH_IS_ENTIRE_WORD (doc->priv->search_flags))
		{
			found = gtk_text_iter_starts_word (&m_start) && 
					gtk_text_iter_ends_word (&m_end);

			if (!found) 
				iter = m_end;
		}
		else
			break;
	}
	
	if (found && (match_start != NULL))
		*match_start = m_start;
	
	if (found && (match_end != NULL))
		*match_end = m_end;
	
	return found;			    
}
						 
gboolean
gedit_document_search_backward (GeditDocument     *doc,
				const GtkTextIter *start,
				const GtkTextIter *end,
				GtkTextIter       *match_start,
				GtkTextIter       *match_end)
{
	GtkTextIter iter;
	GtkSourceSearchFlags search_flags;
	gboolean found = FALSE;
	GtkTextIter m_start;
	GtkTextIter m_end;
	
	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), FALSE);
	g_return_val_if_fail ((start == NULL) || 
			      (gtk_text_iter_get_buffer (start) ==  GTK_TEXT_BUFFER (doc)), FALSE);
	g_return_val_if_fail ((end == NULL) || 
			      (gtk_text_iter_get_buffer (end) ==  GTK_TEXT_BUFFER (doc)), FALSE);
	
	if (doc->priv->search_text == NULL)
	{
		gedit_debug_message (DEBUG_DOCUMENT, "doc->priv->search_text == NULL\n");
		return FALSE;
	}
	else
		gedit_debug_message (DEBUG_DOCUMENT, "doc->priv->search_text == \"%s\"\n", doc->priv->search_text);
				      
	if (end == NULL)
		gtk_text_buffer_get_end_iter (GTK_TEXT_BUFFER (doc), &iter);
	else
		iter = *end;
		
	search_flags = GTK_SOURCE_SEARCH_VISIBLE_ONLY | GTK_SOURCE_SEARCH_TEXT_ONLY;

	if (!GEDIT_SEARCH_IS_CASE_SENSITIVE (doc->priv->search_flags))
	{
		search_flags = search_flags | GTK_SOURCE_SEARCH_CASE_INSENSITIVE;
	}
		
	while (!found)
	{
		found = gtk_source_iter_backward_search (&iter,
							 doc->priv->search_text, 
							 search_flags,
                        	                	 &m_start, 
                        	                	 &m_end,
                                	               	 start);
      	               	
		if (found && GEDIT_SEARCH_IS_ENTIRE_WORD (doc->priv->search_flags))
		{
			found = gtk_text_iter_starts_word (&m_start) && 
					gtk_text_iter_ends_word (&m_end);

			if (!found) 
				iter = m_start;
		}
		else
			break;
	}
	
	if (found && (match_start != NULL))
		*match_start = m_start;
	
	if (found && (match_end != NULL))
		*match_end = m_end;
	
	return found;		      
}

gint 
gedit_document_replace_all (GeditDocument       *doc,
			    const gchar         *find, 
			    const gchar         *replace, 
			    guint                flags)
{
	GtkTextIter iter;
	GtkTextIter m_start;
	GtkTextIter m_end;
	GtkSourceSearchFlags search_flags = 0;
	gboolean found = TRUE;
	gint cont = 0;
	gchar *search_text;
	gchar *replace_text;
	gint replace_text_len;
	GtkTextBuffer *buffer;	
	
	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), 0);
	g_return_val_if_fail (replace != NULL, 0);
	g_return_val_if_fail ((find != NULL) || (doc->priv->search_text != NULL), 0);

	buffer = GTK_TEXT_BUFFER (doc);

	if (find == NULL)
		search_text = g_strdup (doc->priv->search_text);
	else
		search_text = gedit_utils_unescape_search_text (find);
		
	replace_text = gedit_utils_unescape_search_text (replace);
		
	gtk_text_buffer_get_start_iter (buffer, &iter);
		
	search_flags = GTK_SOURCE_SEARCH_VISIBLE_ONLY | GTK_SOURCE_SEARCH_TEXT_ONLY;

	if (!GEDIT_SEARCH_IS_CASE_SENSITIVE (flags))
	{
		search_flags = search_flags | GTK_SOURCE_SEARCH_CASE_INSENSITIVE;
	}
	
	replace_text_len = strlen (replace_text);

	/* disable cursor_moved emission until the end of the
	 * replace_all so that we don't spend all the time
	 * updating the position in the statusbar
	 */
	doc->priv->stop_cursor_moved_emission = TRUE;

	gtk_text_buffer_begin_user_action (buffer);
	
	do
	{
		found = gtk_source_iter_forward_search (&iter,
							search_text, 
							search_flags,
                        	                	&m_start, 
                        	                	&m_end,
                                	               	NULL);
								      	               	
		if (found && GEDIT_SEARCH_IS_ENTIRE_WORD (flags))
		{
			gboolean word;
						
			word = gtk_text_iter_starts_word (&m_start) && 
			       gtk_text_iter_ends_word (&m_end);
				
			if (!word)
			{
				iter = m_end;
				continue;
			}
		}
		
		if (found)
		{
			++cont;
			
			gtk_text_buffer_delete (buffer, 
						&m_start,
						&m_end);
		
			gtk_text_buffer_insert (buffer,
						&m_start,
						replace_text,
						replace_text_len);

			iter = m_start;
		}		

	} while (found);

	gtk_text_buffer_end_user_action (buffer);

	/* re-enable cursor_moved emission and notify
	 * the current position 
	 */
	doc->priv->stop_cursor_moved_emission = FALSE;
	emit_cursor_moved (doc);

	g_free (search_text);
	g_free (replace_text);	

	return cont;
}			    

void 
gedit_document_set_language (GeditDocument     *doc, 
			     GtkSourceLanguage *lang)
{
	g_return_if_fail (GEDIT_IS_DOCUMENT (doc));

	set_language (doc, lang, TRUE);
}

GtkSourceLanguage *
gedit_document_get_language (GeditDocument *doc)
{
	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), NULL);

	return gtk_source_buffer_get_language (GTK_SOURCE_BUFFER (doc));
}

const GeditEncoding *
gedit_document_get_encoding (GeditDocument *doc)
{
	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), NULL);

	return doc->priv->encoding;
}

void
gedit_document_set_auto_save_enabled (GeditDocument *doc, 
				      gboolean       enable)
{
	gedit_debug (DEBUG_DOCUMENT);

	g_return_if_fail (GEDIT_IS_DOCUMENT (doc));

	enable = (enable != FALSE);
	
	// TODO
	
	return;
}

void
gedit_document_set_auto_save_interval (GeditDocument *doc, 
				       gint           interval)
{
	gedit_debug (DEBUG_DOCUMENT);

	g_return_if_fail (GEDIT_IS_DOCUMENT (doc));
	g_return_if_fail (interval > 0);

	// TODO

	return;
}

glong
_gedit_document_get_seconds_since_last_save_or_load (GeditDocument *doc)
{
	GTimeVal current_time;

	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), -1);

	g_get_current_time (&current_time);

	return (current_time.tv_sec - doc->priv->time_of_last_save_or_load.tv_sec);
}

gboolean
_gedit_document_is_saving_as (GeditDocument *doc)
{
	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), FALSE);
	
	return (doc->priv->is_saving_as);
}
