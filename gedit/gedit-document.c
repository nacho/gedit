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
#include <gtksourceview/gtksourceiter.h>

#include "gedit-prefs-manager-app.h"
#include "gedit-document.h"
#include "gedit-debug.h"
#include "gedit-utils.h"
#include "gedit-metadata-manager.h"
#include "gedit-language-manager.h"
#include "gedit-source-style-manager.h"
#include "gedit-document-loader.h"
#include "gedit-document-saver.h"
#include "gedit-marshal.h"
#include "gedittextregion.h"

#undef ENABLE_PROFILE 

#ifdef ENABLE_PROFILE
#define PROFILE(x) x
#else
#define PROFILE(x)
#endif

PROFILE (static GTimer *timer = NULL)

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
static void	to_search_region_range 		(GeditDocument *doc,
						 GtkTextIter   *start, 
						 GtkTextIter   *end);
static void 	insert_text_cb		 	(GeditDocument *doc, 
						 GtkTextIter   *pos,
						 const gchar   *text, 
						 gint           length);
						 
static void	delete_range_cb 		(GeditDocument *doc, 
						 GtkTextIter   *start,
						 GtkTextIter   *end);
			     
struct _GeditDocumentPrivate
{
	gint	     readonly : 1;
	gint	     last_save_was_manually : 1; 	
	gint	     language_set_by_user : 1;
	gint         is_saving_as : 1;
	gint         stop_cursor_moved_emission : 1;

	gchar	    *uri;
	gint 	     untitled_number;

	GnomeVFSURI *vfs_uri;

	const GeditEncoding *encoding;

	gchar	    *mime_type;

	time_t       mtime;

	GTimeVal     time_of_last_save_or_load;

	guint        search_flags;
	gchar       *search_text;
	gint	     num_of_lines_search_text;

	/* Temp data while loading */
	GeditDocumentLoader *loader;
	gboolean             create; /* Create file if uri points 
	                              * to a non existing file */
	const GeditEncoding *requested_encoding;
	gint                 requested_line_pos;

	/* Saving stuff */
	GeditDocumentSaver *saver;

	/* Search highlighting support variables */	
	GeditTextRegion *to_search_region;
	GtkTextTag      *found_tag;
};

enum {
	PROP_0,

	PROP_URI,
	PROP_SHORTNAME,
	PROP_MIME_TYPE,
	PROP_READ_ONLY,
	PROP_ENCODING,
	PROP_CAN_SEARCH_AGAIN,
	PROP_ENABLE_SEARCH_HIGHLIGHTING
};

enum {
	CURSOR_MOVED,
	LOADING,
	LOADED,
	SAVING,
	SAVED,
	SEARCH_HIGHLIGHT_UPDATED,
	LAST_SIGNAL
};

static guint document_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE(GeditDocument, gedit_document, GTK_TYPE_SOURCE_BUFFER)

GQuark
gedit_document_error_quark (void)
{
	static GQuark quark = 0;

	if (G_UNLIKELY (quark == 0))
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

	if (doc->priv->untitled_number > 0)
	{
		g_return_if_fail (doc->priv->uri == NULL);
		release_untitled_number (doc->priv->untitled_number);
	}

	if (doc->priv->uri != NULL)
	{
		GtkTextIter iter;
		gchar *position;

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
			GtkSourceLanguage *lang;

			lang = gedit_document_get_language (doc);

			gedit_metadata_manager_set (doc->priv->uri,
				    "language",
				    (lang == NULL) ? "_NORMAL_" : gtk_source_language_get_id (lang));
		}
	}

	g_free (doc->priv->uri);
	if (doc->priv->vfs_uri != NULL)
		gnome_vfs_uri_unref (doc->priv->vfs_uri);

	g_free (doc->priv->mime_type);

	if (doc->priv->loader)
		g_object_unref (doc->priv->loader);

	g_free (doc->priv->search_text);
	
	if (doc->priv->to_search_region != NULL)
	{
		/* we can't delete marks if we're finalizing the buffer */
		gedit_text_region_destroy (doc->priv->to_search_region, FALSE);
	}

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
		case PROP_ENABLE_SEARCH_HIGHLIGHTING:
			g_value_set_boolean (value, gedit_document_get_enable_search_highlighting (doc));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_document_set_property (GObject      *object,
			     guint         prop_id,
			     const GValue *value,
			     GParamSpec   *pspec)
{
	GeditDocument *doc = GEDIT_DOCUMENT (object);

	switch (prop_id)
	{
		case PROP_ENABLE_SEARCH_HIGHLIGHTING:
			gedit_document_set_enable_search_highlighting (doc,
								       g_value_get_boolean (value));
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
	object_class->set_property = gedit_document_set_property;	

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

	g_object_class_install_property (object_class, PROP_ENABLE_SEARCH_HIGHLIGHTING,
					 g_param_spec_boolean ("enable-search-highlighting",
					 		       "Enable Search Highlighting",
					 		       "Whether all the occurences of the searched string must be highlighted",
					 		       FALSE,
					 		       G_PARAM_READWRITE));

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

	document_signals[SEARCH_HIGHLIGHT_UPDATED] =
	    	g_signal_new ("search_highlight_updated",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GeditDocumentClass, search_highlight_updated),
			      NULL, NULL,
			      gedit_marshal_VOID__BOXED_BOXED,
			      G_TYPE_NONE, 
			      2, 
			      GTK_TYPE_TEXT_ITER | G_SIGNAL_TYPE_STATIC_SCOPE,
			      GTK_TYPE_TEXT_ITER | G_SIGNAL_TYPE_STATIC_SCOPE);

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
		gtk_source_buffer_set_highlight (GTK_SOURCE_BUFFER (doc),
						 gedit_prefs_manager_get_enable_syntax_highlighting ());
	else
		gtk_source_buffer_set_highlight (GTK_SOURCE_BUFFER (doc), 
						 FALSE);

	if (set_by_user && (doc->priv->uri != NULL))
	{
		gedit_metadata_manager_set (doc->priv->uri,
			    "language",
			    (lang == NULL) ? "_NORMAL_" : gtk_source_language_get_id (lang));
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
	GtkSourceStyleManager *style_manager;
	GtkSourceStyleScheme *style_scheme;

	gedit_debug (DEBUG_DOCUMENT);

	doc->priv = GEDIT_DOCUMENT_GET_PRIVATE (doc);

	doc->priv->uri = NULL;
	doc->priv->vfs_uri = NULL;
	doc->priv->untitled_number = get_untitled_number ();

	doc->priv->mime_type = g_strdup ("text/plain");

	doc->priv->readonly = FALSE;

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

	gedit_document_set_enable_search_highlighting (doc,
						       gedit_prefs_manager_get_enable_search_highlighting ());

	style_manager = gedit_get_source_style_manager ();
	style_scheme = gedit_source_style_manager_get_default_scheme (style_manager);
	gtk_source_buffer_set_style_scheme (GTK_SOURCE_BUFFER (doc),
					    style_scheme);

	g_signal_connect_after (doc, 
			  	"insert-text",
			  	G_CALLBACK (insert_text_cb),
			  	NULL);

	g_signal_connect_after (doc, 
			  	"delete-range",
			  	G_CALLBACK (delete_range_cb),
			  	NULL);		
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

		/* Note: vfs_uri may be NULL for some valid but
		 * unsupported uris */
		doc->priv->vfs_uri = gnome_vfs_uri_new (uri);

		if (doc->priv->untitled_number > 0)
		{
			release_untitled_number (doc->priv->untitled_number);
			doc->priv->untitled_number = 0;
		}
	}

	g_free (doc->priv->mime_type);
	if (mime_type != NULL)
	{
		doc->priv->mime_type = g_strdup (mime_type);
	}
	else
	{
		gchar *base_name = NULL;

		/* Guess the mime type from file extension or fallback to "text/plain" */
		if (doc->priv->vfs_uri != NULL)
			base_name = gnome_vfs_uri_extract_short_path_name (doc->priv->vfs_uri);
		if (base_name != NULL)
		{
			const gchar *detected_mime;

			detected_mime = gnome_vfs_get_mime_type_for_name (base_name);
			if (detected_mime == NULL ||
			    strcmp (GNOME_VFS_MIME_TYPE_UNKNOWN, detected_mime) == 0)
				detected_mime = "text/plain";

			doc->priv->mime_type = g_strdup (detected_mime);

			g_free (base_name);
		}
		else
		{
			doc->priv->mime_type = g_strdup ("text/plain");
		}
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
				language = gtk_source_language_manager_get_language_by_id (
							gedit_get_language_manager (),
							data);
			}

			g_free (data);
		}
		else
		{
			gedit_debug_message (DEBUG_DOCUMENT, "Language Normal");

			if (strcmp (doc->priv->mime_type, "text/plain") != 0)
			{
				language = gedit_language_manager_get_language_from_mime_type (
							gedit_get_language_manager (),
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
	else if (doc->priv->vfs_uri == NULL)
		return gnome_vfs_format_uri_for_display (doc->priv->uri);
	else
	{	
		gchar *name;
		gchar *uri_for_display;
		
		name = gnome_vfs_uri_to_string (doc->priv->vfs_uri, GNOME_VFS_URI_HIDE_PASSWORD);
		g_return_val_if_fail (name != NULL, gnome_vfs_format_uri_for_display (doc->priv->uri));
		
		uri_for_display = gnome_vfs_format_uri_for_display (name);
		g_free (name);
		
		return uri_for_display;
	}
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
	else if (doc->priv->vfs_uri == NULL)
		return g_strdup (doc->priv->uri);
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

gboolean
_gedit_document_check_externally_modified (GeditDocument *doc)
{
	GnomeVFSFileInfo *info;
	GnomeVFSResult result;
	gint file_mtime;

	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), FALSE);

	if (doc->priv->uri == NULL)
	{
		return FALSE;
	}

	info = gnome_vfs_file_info_new ();

	result = gnome_vfs_get_file_info_uri (doc->priv->vfs_uri,
					      info,
					      GNOME_VFS_FILE_INFO_DEFAULT|
					      GNOME_VFS_FILE_INFO_FOLLOW_LINKS);
	if (result != GNOME_VFS_OK)
	{
		gnome_vfs_file_info_unref (info);

		return FALSE;
	}

	file_mtime = info->mtime;

	gnome_vfs_file_info_unref (info);

	return (file_mtime > doc->priv->mtime);
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
		else if (gedit_prefs_manager_get_restore_cursor_position ())
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

			/* make sure it's a valid position, if the file
			 * changed we may have ended up in the middle of
			 * a utf8 character cluster */
			if (!gtk_text_iter_is_cursor_position (&iter))
			{
				gtk_text_buffer_get_start_iter (GTK_TEXT_BUFFER (doc),
								&iter);
			}
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
	         (error->code == GNOME_VFS_ERROR_NOT_FOUND) &&
	         (gedit_utils_uri_has_file_scheme (doc->priv->uri)))
	{
		reset_temp_loading_data (doc);

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

void
gedit_document_load (GeditDocument       *doc,
		     const gchar         *uri,
		     const GeditEncoding *encoding,
		     gint                 line_pos,
		     gboolean             create)
{
	g_return_if_fail (GEDIT_IS_DOCUMENT (doc));
	g_return_if_fail (uri != NULL);
	g_return_if_fail (gedit_utils_is_valid_uri (uri));

	g_return_if_fail (doc->priv->loader == NULL);

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

	gedit_document_loader_load (doc->priv->loader,
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
	gedit_debug (DEBUG_DOCUMENT);

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

	/* TODO */

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
gedit_document_is_local (GeditDocument *doc)
{
	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), FALSE);

	if (doc->priv->uri == NULL)
	{
		return FALSE;
	}

	return gedit_utils_uri_has_file_scheme (doc->priv->uri);
}

gboolean
gedit_document_get_deleted (GeditDocument *doc)
{
	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), FALSE);

	if (doc->priv->uri == NULL || doc->priv->vfs_uri == NULL)
		return FALSE;
		
	return !gnome_vfs_uri_exists (doc->priv->vfs_uri);
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

	if (line >= line_count)
	{
		ret = FALSE;
		gtk_text_buffer_get_end_iter (GTK_TEXT_BUFFER (doc),
					      &iter);
	}
	else
	{
		gtk_text_buffer_get_iter_at_line (GTK_TEXT_BUFFER (doc),
						  &iter,
						  line);
	}

	gtk_text_buffer_place_cursor (GTK_TEXT_BUFFER (doc), &iter);

	return ret;
}

static gint
compute_num_of_lines (const gchar *text)
{
	const gchar *p;
	gint len;
	gint n = 1;

	g_return_val_if_fail (text != NULL, 0);

	len = strlen (text);
	p = text;

	while (len > 0)
	{
		gint del, par;

		pango_find_paragraph_boundary (p, len, &del, &par);

		if (del == par) /* not found */
			break;

		p += par;
		len -= par;
		++n;
	}

	return n;
}

void
gedit_document_set_search_text (GeditDocument *doc,
				const gchar   *text,
				guint          flags)
{
	gchar *converted_text;
	gboolean notify = FALSE;
	gboolean update_to_search_region = FALSE;
	
	g_return_if_fail (GEDIT_IS_DOCUMENT (doc));
	g_return_if_fail ((text == NULL) || (doc->priv->search_text != text));
	g_return_if_fail ((text == NULL) || g_utf8_validate (text, -1, NULL));

	gedit_debug_message (DEBUG_DOCUMENT, "text = %s", text);

	if (text != NULL)
	{
		if (*text != '\0')
		{
			converted_text = gedit_utils_unescape_search_text (text);
			notify = !gedit_document_get_can_search_again (doc);
		}
		else
		{
			converted_text = g_strdup("");
			notify = gedit_document_get_can_search_again (doc);
		}
		
		g_free (doc->priv->search_text);
	
		doc->priv->search_text = converted_text;
		doc->priv->num_of_lines_search_text = compute_num_of_lines (doc->priv->search_text);
		update_to_search_region = TRUE;
	}
	
	if (!GEDIT_SEARCH_IS_DONT_SET_FLAGS (flags))
	{
		if (doc->priv->search_flags != flags)
			update_to_search_region = TRUE;
			
		doc->priv->search_flags = flags;

	}

	if (update_to_search_region)
	{
		GtkTextIter begin;
		GtkTextIter end;
		
		gtk_text_buffer_get_bounds (GTK_TEXT_BUFFER (doc),
					    &begin,
					    &end);
					    
		to_search_region_range (doc,
					&begin,
					&end);
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

	return ((doc->priv->search_text != NULL) && 
	        (*doc->priv->search_text != '\0'));
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
	gboolean check_brackets;
	gboolean search_highliting;

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

	/* also avoid spending time matching brackets */
	check_brackets = gtk_source_buffer_get_check_brackets (GTK_SOURCE_BUFFER (buffer));
	gtk_source_buffer_set_check_brackets (GTK_SOURCE_BUFFER (buffer), FALSE);

	/* and do search highliting later */
	search_highliting = gedit_document_get_enable_search_highlighting (doc);
	gedit_document_set_enable_search_highlighting (doc, FALSE);

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

	gtk_source_buffer_set_check_brackets (GTK_SOURCE_BUFFER (buffer),
					      check_brackets);
	gedit_document_set_enable_search_highlighting (doc, search_highliting);

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

glong
_gedit_document_get_seconds_since_last_save_or_load (GeditDocument *doc)
{
	GTimeVal current_time;

	gedit_debug (DEBUG_DOCUMENT);
	
	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), -1);

	g_get_current_time (&current_time);

	return (current_time.tv_sec - doc->priv->time_of_last_save_or_load.tv_sec);
}

gboolean
_gedit_document_is_saving_as (GeditDocument *doc)
{
	gedit_debug (DEBUG_DOCUMENT);
	
	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), FALSE);
	
	return (doc->priv->is_saving_as);
}

static void
search_region (GeditDocument *doc,
	       GtkTextIter   *start,
	       GtkTextIter   *end)
{
	GtkTextIter iter;
	GtkTextIter m_start;
	GtkTextIter m_end;	
	GtkSourceSearchFlags search_flags = 0;
	gboolean found = TRUE;

	GtkTextBuffer *buffer;	

	gedit_debug (DEBUG_DOCUMENT);
	
	buffer = GTK_TEXT_BUFFER (doc);
	
	if (doc->priv->found_tag == NULL)
	{
		/* FIXME: the colors are hardcoded */
		doc->priv->found_tag = gtk_text_buffer_create_tag (GTK_TEXT_BUFFER (doc), 
								   "found",
								   "background", "#FFFF78",
								   //"foreground", "black",
								   NULL);
	}

	if (doc->priv->search_text == NULL)
		return;
		
	g_return_if_fail (doc->priv->num_of_lines_search_text > 0);
	
	gtk_text_iter_backward_lines (start, doc->priv->num_of_lines_search_text);
	gtk_text_iter_forward_lines (end, doc->priv->num_of_lines_search_text);
	
	if (gtk_text_iter_has_tag (start, doc->priv->found_tag) &&
	    !gtk_text_iter_begins_tag (start, doc->priv->found_tag))
		gtk_text_iter_backward_to_tag_toggle (start, doc->priv->found_tag);
		
	if (gtk_text_iter_has_tag (end, doc->priv->found_tag) &&
	    !gtk_text_iter_ends_tag (end, doc->priv->found_tag))
		gtk_text_iter_forward_to_tag_toggle (end, doc->priv->found_tag);
		
	/*
	g_print ("[%u (%u), %u (%u)]\n", gtk_text_iter_get_line (start), gtk_text_iter_get_offset (start),	
					   gtk_text_iter_get_line (end), gtk_text_iter_get_offset (end));
	*/

	gtk_text_buffer_remove_tag (buffer,
				    doc->priv->found_tag,
				    start,
				    end);

	iter = *start;
		
	search_flags = GTK_SOURCE_SEARCH_VISIBLE_ONLY | GTK_SOURCE_SEARCH_TEXT_ONLY;

	if (!GEDIT_SEARCH_IS_CASE_SENSITIVE (doc->priv->search_flags))
	{
		search_flags = search_flags | GTK_SOURCE_SEARCH_CASE_INSENSITIVE;
	}
	
	do
	{
		if ((end != NULL) && gtk_text_iter_is_end (end))
			end = NULL;
			
		found = gtk_source_iter_forward_search (&iter,
							doc->priv->search_text, 
							search_flags,
                        	                	&m_start, 
                        	                	&m_end,
                                	               	end);
				
		iter = m_end;
						      	               	
		if (found && GEDIT_SEARCH_IS_ENTIRE_WORD (doc->priv->search_flags))
		{
			gboolean word;
						
			word = gtk_text_iter_starts_word (&m_start) && 
			       gtk_text_iter_ends_word (&m_end);
				
			if (!word)
				continue;
		}
		
		if (found)
		{
			/* g_print ("FOUND\n"); */
			gtk_text_buffer_apply_tag (buffer,
						   doc->priv->found_tag,
						   &m_start,
						   &m_end);
		}		

	} while (found);		
}

static void
to_search_region_range (GeditDocument *doc,
			GtkTextIter   *start, 
			GtkTextIter   *end)
{
	gedit_debug (DEBUG_DOCUMENT);
	
	if (doc->priv->to_search_region == NULL)
		return;
		
	gtk_text_iter_set_line_offset (start, 0);
	gtk_text_iter_forward_to_line_end (end);
	
	/*
	g_print ("+ [%u (%u), %u (%u)]\n", gtk_text_iter_get_line (start), gtk_text_iter_get_offset (start),
					   gtk_text_iter_get_line (end), gtk_text_iter_get_offset (end));
	*/

	/* Add the region to the refresh region */
	gedit_text_region_add (doc->priv->to_search_region, start, end);

	/* Notify views of the updated highlight region */
	gtk_text_iter_backward_lines (start, doc->priv->num_of_lines_search_text);
	gtk_text_iter_forward_lines (end, doc->priv->num_of_lines_search_text);
	
	g_signal_emit (doc, document_signals [SEARCH_HIGHLIGHT_UPDATED], 0, start, end);
}

void
_gedit_document_search_region (GeditDocument     *doc,
			       const GtkTextIter *start,
			       const GtkTextIter *end)
{
	GeditTextRegion *region;

	gedit_debug (DEBUG_DOCUMENT);
		
	g_return_if_fail (GEDIT_IS_DOCUMENT (doc));
	g_return_if_fail (start != NULL);
	g_return_if_fail (end != NULL);
	
	if (doc->priv->to_search_region == NULL)
		return;

	/*
	g_print ("U [%u (%u), %u (%u)]\n", gtk_text_iter_get_line (start), gtk_text_iter_get_offset (start),
					   gtk_text_iter_get_line (end), gtk_text_iter_get_offset (end));
	*/
		
	/* get the subregions not yet highlighted */
	region = gedit_text_region_intersect (doc->priv->to_search_region, 
					      start,
					      end);
	if (region) 
	{
		gint i;
		GtkTextIter start_search;
		GtkTextIter end_search;

		i = gedit_text_region_subregions (region);
		gedit_text_region_nth_subregion (region, 
						 0,
						 &start_search,
						 NULL);

		gedit_text_region_nth_subregion (region, 
						 i - 1,
						 NULL,
						 &end_search);

		gedit_text_region_destroy (region, TRUE);

		gtk_text_iter_order (&start_search, &end_search);

		search_region (doc, &start_search, &end_search);

		/* remove the just highlighted region */
		gedit_text_region_subtract (doc->priv->to_search_region,
					    start, 
					    end);
	}
}

static void
insert_text_cb (GeditDocument *doc, 
		GtkTextIter   *pos,
		const gchar   *text, 
		gint           length)
{
	GtkTextIter start;
	GtkTextIter end;

	gedit_debug (DEBUG_DOCUMENT);
		
	start = end = *pos;

	/*
	 * pos is invalidated when
	 * insertion occurs (because the buffer contents change), but the
	 * default signal handler revalidates it to point to the end of the
	 * inserted text 
	 */
	gtk_text_iter_backward_chars (&start,
				      g_utf8_strlen (text, length));
				     
	to_search_region_range (doc, &start, &end);
}
						 
static void	
delete_range_cb (GeditDocument *doc, 
		 GtkTextIter   *start,
		 GtkTextIter   *end)
{
	GtkTextIter d_start;
	GtkTextIter d_end;

	gedit_debug (DEBUG_DOCUMENT);
		
	d_start = *start;
	d_end = *end;
	
	to_search_region_range (doc, &d_start, &d_end);
}

void
gedit_document_set_enable_search_highlighting (GeditDocument *doc,
					       gboolean       enable)
{
	g_return_if_fail (GEDIT_IS_DOCUMENT (doc));
	
	enable = enable != FALSE;
	
	if ((doc->priv->to_search_region != NULL) == enable)
		return;
	
	if (doc->priv->to_search_region != NULL)
	{
		/* Disable search highlighting */
		if (doc->priv->found_tag != NULL)
		{
			/* If needed remove the found_tag */
			GtkTextIter begin;
			GtkTextIter end;
		
			gtk_text_buffer_get_bounds (GTK_TEXT_BUFFER (doc),
						    &begin,
						    &end);

			gtk_text_buffer_remove_tag (GTK_TEXT_BUFFER (doc),
				    		    doc->priv->found_tag,
				    		    &begin,
				    		    &end);
		}
		
		gedit_text_region_destroy (doc->priv->to_search_region,
					   TRUE);
		doc->priv->to_search_region = NULL;
	}
	else
	{
		doc->priv->to_search_region = gedit_text_region_new (GTK_TEXT_BUFFER (doc));
		if (gedit_document_get_can_search_again (doc))
		{
			/* If search_text is not empty, highligth all its occurrences */
			GtkTextIter begin;
			GtkTextIter end;
		
			gtk_text_buffer_get_bounds (GTK_TEXT_BUFFER (doc),
						    &begin,
						    &end);
					    
			to_search_region_range (doc,
						&begin,
						&end);
		}
	}
}

gboolean
gedit_document_get_enable_search_highlighting (GeditDocument *doc)
{
	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), FALSE);
	
	return (doc->priv->to_search_region != NULL);
}
