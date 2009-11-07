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
#include <gtksourceview/gtksourceiter.h>

#include "gedit-prefs-manager-app.h"
#include "gedit-document-interface.h"
#include "gedit-text-buffer.h"
#include "gedit-debug.h"
#include "gedit-utils.h"
#include "gedit-metadata-manager.h"
#include "gedit-language-manager.h"
#include "gedit-style-scheme-manager.h"
#include "gedit-document-loader.h"
#include "gedit-document-saver.h"
#include "gedit-marshal.h"
#include "gedit-enum-types.h"
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

#define GEDIT_TEXT_BUFFER_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), GEDIT_TYPE_TEXT_BUFFER, GeditTextBufferPrivate))

static void	gedit_text_buffer_load_real	(GeditDocument          *doc,
						 const gchar            *uri,
						 const GeditEncoding    *encoding,
						 gint                    line_pos,
						 gboolean                create);
static void	gedit_text_buffer_save_real	(GeditDocument          *doc,
						 const gchar            *uri,
						 const GeditEncoding    *encoding,
						 GeditDocumentSaveFlags  flags);
static void	gedit_text_buffer_set_readonly	(GeditTextBuffer *doc,
						 gboolean       readonly);
static void	to_search_region_range 		(GeditTextBuffer *doc,
						 GtkTextIter     *start, 
						 GtkTextIter   *end);
static void 	insert_text_cb		 	(GeditTextBuffer *doc, 
						 GtkTextIter     *pos,
						 const gchar     *text, 
						 gint           length);
						 
static void	delete_range_cb 		(GeditTextBuffer *doc, 
						 GtkTextIter     *start,
						 GtkTextIter     *end);
			     
static void	gedit_text_buffer_iface_init	(GeditDocumentIface  *iface);

struct _GeditTextBufferPrivate
{
	gint	     readonly : 1;
	gint	     last_save_was_manually : 1;
	gint	     language_set_by_user : 1;
	gint         stop_cursor_moved_emission : 1;
	gint         dispose_has_run : 1;

	gchar	    *uri;
	gint 	     untitled_number;

	const GeditEncoding *encoding;

	gchar	    *content_type;

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
	
	/* Mount operation factory */
	/*GeditMountOperationFactory  mount_operation_factory;
	gpointer		    mount_operation_userdata;*/
};

enum {
	PROP_0,

	PROP_URI,
	PROP_SHORTNAME,
	PROP_CONTENT_TYPE,
	PROP_READ_ONLY,
	PROP_ENCODING,
	PROP_CAN_SEARCH_AGAIN,
	PROP_ENABLE_SEARCH_HIGHLIGHTING
};

enum {
	CURSOR_MOVED,
	SEARCH_HIGHLIGHT_UPDATED,
	LAST_SIGNAL
};

static guint document_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE_WITH_CODE (GeditTextBuffer,
			 gedit_text_buffer,
			 GTK_TYPE_SOURCE_BUFFER,
			 G_IMPLEMENT_INTERFACE (GEDIT_TYPE_DOCUMENT,
			 			gedit_text_buffer_iface_init))

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
gedit_text_buffer_dispose (GObject *object)
{
	GeditTextBuffer *doc = GEDIT_TEXT_BUFFER (object); 

	gedit_debug (DEBUG_DOCUMENT);

	/* Metadata must be saved here and not in finalize
	 * because the language is gone by the time finalize runs.
	 * beside if some plugin prevents proper finalization by
	 * holding a ref to the doc, we still save the metadata */
	if ((!doc->priv->dispose_has_run) && (doc->priv->uri != NULL))
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

			lang = gedit_text_buffer_get_language (doc);

			gedit_metadata_manager_set (doc->priv->uri,
				    "language",
				    (lang == NULL) ? "_NORMAL_" : gtk_source_language_get_id (lang));
		}
	}

	if (doc->priv->loader)
	{
		g_object_unref (doc->priv->loader);
		doc->priv->loader = NULL;
	}

	doc->priv->dispose_has_run = TRUE;

	G_OBJECT_CLASS (gedit_text_buffer_parent_class)->dispose (object);
}

static void
gedit_text_buffer_finalize (GObject *object)
{
	GeditTextBuffer *doc = GEDIT_TEXT_BUFFER (object); 

	gedit_debug (DEBUG_DOCUMENT);

	if (doc->priv->untitled_number > 0)
	{
		g_return_if_fail (doc->priv->uri == NULL);
		release_untitled_number (doc->priv->untitled_number);
	}

	g_free (doc->priv->uri);
	g_free (doc->priv->content_type);
	g_free (doc->priv->search_text);

	if (doc->priv->to_search_region != NULL)
	{
		/* we can't delete marks if we're finalizing the buffer */
		gedit_text_region_destroy (doc->priv->to_search_region, FALSE);
	}

	G_OBJECT_CLASS (gedit_text_buffer_parent_class)->finalize (object);
}

static void
gedit_text_buffer_get_property (GObject    *object,
				guint       prop_id,
				GValue     *value,
				GParamSpec *pspec)
{
	GeditTextBuffer *buf = GEDIT_TEXT_BUFFER (object);
	GeditDocument *doc = GEDIT_DOCUMENT (object);

	switch (prop_id)
	{
		case PROP_URI:
			g_value_set_string (value, buf->priv->uri);
			break;
		case PROP_SHORTNAME:
			g_value_take_string (value, gedit_document_get_short_name_for_display (doc));
			break;
		case PROP_CONTENT_TYPE:
			g_value_take_string (value, gedit_document_get_content_type (doc));
			break;
		case PROP_READ_ONLY:
			g_value_set_boolean (value, buf->priv->readonly);
			break;
		case PROP_ENCODING:
			g_value_set_boxed (value, buf->priv->encoding);
			break;
		case PROP_CAN_SEARCH_AGAIN:
			g_value_set_boolean (value, gedit_text_buffer_get_can_search_again (buf));
			break;
		case PROP_ENABLE_SEARCH_HIGHLIGHTING:
			g_value_set_boolean (value, gedit_text_buffer_get_enable_search_highlighting (buf));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_text_buffer_set_property (GObject      *object,
				guint         prop_id,
				const GValue *value,
				GParamSpec   *pspec)
{
	GeditTextBuffer *doc = GEDIT_TEXT_BUFFER (object);

	switch (prop_id)
	{
		case PROP_ENABLE_SEARCH_HIGHLIGHTING:
			gedit_text_buffer_set_enable_search_highlighting (doc,
									  g_value_get_boolean (value));
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
emit_cursor_moved (GeditTextBuffer *doc)
{
	if (!doc->priv->stop_cursor_moved_emission)
	{
		g_signal_emit (doc,
			       document_signals[CURSOR_MOVED],
			       0);
	}
}

static void
gedit_text_buffer_mark_set (GtkTextBuffer     *buffer,
                            const GtkTextIter *iter,
                            GtkTextMark       *mark)
{
	GeditTextBuffer *doc = GEDIT_TEXT_BUFFER (buffer);

	if (GTK_TEXT_BUFFER_CLASS (gedit_text_buffer_parent_class)->mark_set)
		GTK_TEXT_BUFFER_CLASS (gedit_text_buffer_parent_class)->mark_set (buffer,
										  iter,
										  mark);

	if (mark == gtk_text_buffer_get_insert (buffer))
	{
		emit_cursor_moved (doc);
	}
}

static void
gedit_text_buffer_changed (GtkTextBuffer *buffer)
{
	emit_cursor_moved (GEDIT_TEXT_BUFFER (buffer));

	GTK_TEXT_BUFFER_CLASS (gedit_text_buffer_parent_class)->changed (buffer);
}

static void 
gedit_text_buffer_class_init (GeditTextBufferClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkTextBufferClass *buf_class = GTK_TEXT_BUFFER_CLASS (klass);

	object_class->dispose = gedit_text_buffer_dispose;
	object_class->finalize = gedit_text_buffer_finalize;
	object_class->get_property = gedit_text_buffer_get_property;
	object_class->set_property = gedit_text_buffer_set_property;

	buf_class->mark_set = gedit_text_buffer_mark_set;
	buf_class->changed = gedit_text_buffer_changed;

	g_object_class_override_property (object_class,
					  PROP_URI,
					  "uri");

	g_object_class_override_property (object_class,
					  PROP_SHORTNAME,
					  "shortname");

	g_object_class_override_property (object_class,
					  PROP_CONTENT_TYPE,
					  "content-type");

	g_object_class_override_property (object_class,
					  PROP_READ_ONLY,
					  "read-only");

	g_object_class_install_property (object_class, PROP_ENCODING,
					 g_param_spec_boxed ("encoding",
							     "Encoding",
							     "The GeditEncoding used for the document",
							     GEDIT_TYPE_ENCODING,
							     G_PARAM_READABLE |
							     G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (object_class, PROP_CAN_SEARCH_AGAIN,
					 g_param_spec_boolean ("can-search-again",
							       "Can search again",
							       "Wheter it's possible to search again in the document",
							       FALSE,
							       G_PARAM_READABLE |
							       G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (object_class, PROP_ENABLE_SEARCH_HIGHLIGHTING,
					 g_param_spec_boolean ("enable-search-highlighting",
							       "Enable Search Highlighting",
							       "Whether all the occurences of the searched string must be highlighted",
							       FALSE,
							       G_PARAM_READWRITE |
							       G_PARAM_STATIC_STRINGS));

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
			      G_STRUCT_OFFSET (GeditTextBufferClass, cursor_moved),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);

	document_signals[SEARCH_HIGHLIGHT_UPDATED] =
		g_signal_new ("search_highlight_updated",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GeditTextBufferClass, search_highlight_updated),
			      NULL, NULL,
			      gedit_marshal_VOID__BOXED_BOXED,
			      G_TYPE_NONE, 
			      2, 
			      GTK_TYPE_TEXT_ITER | G_SIGNAL_TYPE_STATIC_SCOPE,
			      GTK_TYPE_TEXT_ITER | G_SIGNAL_TYPE_STATIC_SCOPE);

	g_type_class_add_private (object_class, sizeof (GeditTextBufferPrivate));
}

static void
set_language (GeditTextBuffer   *doc, 
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
		gtk_source_buffer_set_highlight_syntax (GTK_SOURCE_BUFFER (doc),
				 gedit_prefs_manager_get_enable_syntax_highlighting ());
	else
		gtk_source_buffer_set_highlight_syntax (GTK_SOURCE_BUFFER (doc), 
				 FALSE);

	if (set_by_user && (doc->priv->uri != NULL))
	{
		gedit_metadata_manager_set (doc->priv->uri,
			    "language",
			    (lang == NULL) ? "_NORMAL_" : gtk_source_language_get_id (lang));
	}

	doc->priv->language_set_by_user = set_by_user;
}

static GtkSourceLanguage *
guess_language (const gchar *uri,
		const gchar *content_type)

{
	gchar *data;
	GtkSourceLanguage *language = NULL;

	data = gedit_metadata_manager_get (uri, "language");

	if (data != NULL)
	{
		gedit_debug_message (DEBUG_DOCUMENT, "Language from metadata: %s", data);

		if (strcmp (data, "_NORMAL_") != 0)
		{
			language = gtk_source_language_manager_get_language (
						gedit_get_language_manager (),
						data);
		}

		g_free (data);
	}
	else
	{
		GFile *file;
		gchar *basename;

		gedit_debug_message (DEBUG_DOCUMENT, "Sniffing Language");

		file = g_file_new_for_uri (uri);
		basename = g_file_get_basename (file);

		language = gtk_source_language_manager_guess_language (
					gedit_get_language_manager (),
					basename,
					content_type);

		g_free (basename);
		g_object_unref (file);
	}

	return language;
}

/* If content type is null, we guess from the filename */
/* If uri is null, we only set the content-type */
static void
set_uri (GeditTextBuffer *doc,
	 const gchar     *uri,
	 const gchar     *content_type)
{
	gedit_debug (DEBUG_DOCUMENT);

	g_return_if_fail ((uri == NULL) || gedit_utils_is_valid_uri (uri));

	if (uri != NULL)
	{
		if (doc->priv->uri == uri)
			return;

		g_free (doc->priv->uri);
		doc->priv->uri = g_strdup (uri);

		if (doc->priv->untitled_number > 0)
		{
			release_untitled_number (doc->priv->untitled_number);
			doc->priv->untitled_number = 0;
		}
	}

	g_free (doc->priv->content_type);

	if (content_type != NULL)
	{
		doc->priv->content_type = g_strdup (content_type);
	}
	else
	{
		if (doc->priv->uri != NULL)
		{
			doc->priv->content_type = g_content_type_guess (doc->priv->uri, NULL, 0, NULL);
		}
		else
		{
			doc->priv->content_type = g_content_type_from_mime_type ("text/plain");
		}
	}

	if (!doc->priv->language_set_by_user)
	{
		GtkSourceLanguage *language;

		language = guess_language (doc->priv->uri, doc->priv->content_type);

		gedit_debug_message (DEBUG_DOCUMENT, "Language: %s",
				     language != NULL ? gtk_source_language_get_name (language) : "None");

		set_language (doc, language, FALSE);
	}

	g_object_notify (G_OBJECT (doc), "uri");
	g_object_notify (G_OBJECT (doc), "shortname");
}

static GFile *
gedit_text_buffer_get_location_impl (GeditDocument *doc)
{
	GeditTextBuffer *buf;

	g_return_val_if_fail (GEDIT_IS_TEXT_BUFFER (doc), NULL);
	
	buf = GEDIT_TEXT_BUFFER (doc);

	return buf->priv->uri == NULL ? NULL : g_file_new_for_uri (buf->priv->uri);
}

static gchar *
gedit_text_buffer_get_uri_impl (GeditDocument *doc)
{
	GeditTextBuffer *buf = GEDIT_TEXT_BUFFER (doc);

	return g_strdup (buf->priv->uri);
}

static void
gedit_text_buffer_set_uri_impl (GeditDocument *doc,
				const gchar   *uri)
{
	GeditTextBuffer *buf = GEDIT_TEXT_BUFFER (doc);

	set_uri (buf, uri, buf->priv->content_type);
}

/* Never returns NULL */
static gchar *
gedit_text_buffer_get_uri_for_display_impl (GeditDocument *doc)
{
	GeditTextBuffer *buf = GEDIT_TEXT_BUFFER (doc);

	if (buf->priv->uri == NULL)
		return g_strdup_printf (_("Unsaved Document %d"),
					buf->priv->untitled_number);
	else
		return gedit_utils_uri_for_display (buf->priv->uri);
}

/* Never returns NULL */
static gchar *
gedit_text_buffer_get_short_name_for_display_impl (GeditDocument *doc)
{
	GeditTextBuffer *buf = GEDIT_TEXT_BUFFER (doc);

	if (buf->priv->uri == NULL)
		return g_strdup_printf (_("Unsaved Document %d"),
					buf->priv->untitled_number);
	else
		return gedit_utils_basename_for_display (buf->priv->uri);
}

static gchar *
gedit_text_buffer_get_content_type_impl (GeditDocument *doc)
{
	GeditTextBuffer *buf = GEDIT_TEXT_BUFFER (doc);

	return g_strdup (buf->priv->content_type);
}

/* Never returns NULL */
static gchar *
gedit_text_buffer_get_mime_type_impl (GeditDocument *doc)
{
	GeditTextBuffer *buf = GEDIT_TEXT_BUFFER (doc);
	gchar *mime_type = NULL;

	if ((buf->priv->content_type != NULL) &&
	    (!g_content_type_is_unknown (buf->priv->content_type)))
	{
		mime_type = g_content_type_get_mime_type (buf->priv->content_type);
	}

 	return mime_type != NULL ? mime_type : g_strdup ("text/plain");
}

static gboolean
gedit_text_buffer_get_readonly_impl (GeditDocument *doc)
{
	GeditTextBuffer *buf = GEDIT_TEXT_BUFFER (doc);

	return buf->priv->readonly;
}

static gboolean
gedit_text_buffer_load_cancel_impl (GeditDocument *doc)
{
	GeditTextBuffer *buf = GEDIT_TEXT_BUFFER (doc);

	if (buf->priv->loader == NULL)
		return FALSE;

	return gedit_document_loader_cancel (buf->priv->loader);
}

static gboolean
gedit_text_buffer_is_untouched_impl (GeditDocument *doc)
{
	GeditTextBuffer *buf = GEDIT_TEXT_BUFFER (doc);

	return (buf->priv->uri == NULL) && 
		(!gtk_text_buffer_get_modified (GTK_TEXT_BUFFER (doc)));
}

static gboolean
gedit_text_buffer_is_untitled_impl (GeditDocument *doc)
{
	GeditTextBuffer *buf = GEDIT_TEXT_BUFFER (doc);

	return (buf->priv->uri == NULL);
}

static gboolean
gedit_text_buffer_is_local_impl (GeditDocument *doc)
{
	GeditTextBuffer *buf = GEDIT_TEXT_BUFFER (doc);

	if (buf->priv->uri == NULL)
	{
		return FALSE;
	}

	return gedit_utils_uri_has_file_scheme (buf->priv->uri);
}

static gboolean
gedit_text_buffer_get_deleted_impl (GeditDocument *doc)
{
	GeditTextBuffer *buf = GEDIT_TEXT_BUFFER (doc);

	return buf->priv->uri && !gedit_utils_uri_exists (buf->priv->uri);
}

/*
 * If @line is bigger than the lines of the document, the cursor is moved
 * to the last line and FALSE is returned.
 */
static gboolean
gedit_text_buffer_goto_line_impl (GeditDocument *doc, 
				  gint           line)
{
	gboolean ret = TRUE;
	guint line_count;
	GtkTextIter iter;

	gedit_debug (DEBUG_DOCUMENT);

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

static gboolean
gedit_text_buffer_goto_line_offset_impl (GeditDocument *doc,
					 gint           line,
					 gint           line_offset)
{
	gboolean ret = TRUE;
	guint offset_count;
	GtkTextIter iter;
	
	gtk_text_buffer_get_iter_at_line (GTK_TEXT_BUFFER (doc),
					  &iter,
					  line);

	offset_count = gtk_text_iter_get_chars_in_line (&iter);
	if (line_offset > offset_count)
	{
		ret = FALSE;
	}
	else
	{
		gtk_text_iter_set_line_offset (&iter, line_offset);
	}
	
	gtk_text_buffer_place_cursor (GTK_TEXT_BUFFER (doc), &iter);

	return ret;
}

static const GeditEncoding *
gedit_text_buffer_get_encoding_impl (GeditDocument *doc)
{
	GeditTextBuffer *buf = GEDIT_TEXT_BUFFER (doc);

	return buf->priv->encoding;
}

static glong
gedit_text_buffer_get_seconds_since_last_save_or_load_impl (GeditDocument *doc)
{
	GeditTextBuffer *buf = GEDIT_TEXT_BUFFER (doc);

	GTimeVal current_time;

	gedit_debug (DEBUG_DOCUMENT);

	g_get_current_time (&current_time);

	return (current_time.tv_sec - buf->priv->time_of_last_save_or_load.tv_sec);
}

static gboolean
gedit_text_buffer_check_externally_modified_impl (GeditDocument *doc)
{
	GeditTextBuffer *buf = GEDIT_TEXT_BUFFER (doc);
	GFile *gfile;
	GFileInfo *info;
	GTimeVal timeval;

	if (buf->priv->uri == NULL)
	{
		return FALSE;
	}

	gfile = g_file_new_for_uri (buf->priv->uri);
	info = g_file_query_info (gfile,
				  G_FILE_ATTRIBUTE_TIME_MODIFIED,
				  G_FILE_QUERY_INFO_NONE,
				  NULL, NULL);
	g_object_unref (gfile);

	if (info == NULL ||
	    !g_file_info_has_attribute (info, G_FILE_ATTRIBUTE_TIME_MODIFIED))
	{
		return FALSE;
	}

	g_file_info_get_modification_time (info, &timeval);
	g_object_unref (info);
	
	return timeval.tv_sec > buf->priv->mtime;
}

static void
gedit_text_buffer_undo_impl (GeditDocument *doc)
{
	gtk_source_buffer_undo (GTK_SOURCE_BUFFER (doc));
}

static void
gedit_text_buffer_redo_impl (GeditDocument *doc)
{
	gtk_source_buffer_redo (GTK_SOURCE_BUFFER (doc));
}

static gboolean
gedit_text_buffer_can_undo_impl (GeditDocument *doc)
{
	return gtk_source_buffer_can_undo (GTK_SOURCE_BUFFER (doc));
}

static gboolean
gedit_text_buffer_can_redo_impl (GeditDocument *doc)
{
	return gtk_source_buffer_can_redo (GTK_SOURCE_BUFFER (doc));
}

static void
gedit_text_buffer_begin_not_undoable_action_impl (GeditDocument *doc)
{
	gtk_source_buffer_begin_not_undoable_action (GTK_SOURCE_BUFFER (doc));
}

static void
gedit_text_buffer_end_not_undoable_action_impl (GeditDocument *doc)
{
	gtk_source_buffer_end_not_undoable_action (GTK_SOURCE_BUFFER (doc));
}

static void
gedit_text_buffer_set_text_impl (GeditDocument *doc,
				 const gchar   *text,
				 gint           len)
{
	gtk_text_buffer_set_text (GTK_TEXT_BUFFER (doc), text, len);
}

static void
gedit_text_buffer_set_modified_impl (GeditDocument *doc,
				     gboolean       setting)
{
	gtk_text_buffer_set_modified (GTK_TEXT_BUFFER (doc), setting);
}

static gboolean
gedit_text_buffer_get_modified_impl (GeditDocument *doc)
{
	return gtk_text_buffer_get_modified (GTK_TEXT_BUFFER (doc));
}

static gboolean
gedit_text_buffer_get_has_selection_impl (GeditDocument *doc)
{
	return gtk_text_buffer_get_has_selection (GTK_TEXT_BUFFER (doc));
}

static void
gedit_text_buffer_iface_init (GeditDocumentIface  *iface)
{
	iface->get_location = gedit_text_buffer_get_location_impl;
	iface->get_uri = gedit_text_buffer_get_uri_impl;
	iface->set_uri = gedit_text_buffer_set_uri_impl;
	iface->get_uri_for_display = gedit_text_buffer_get_uri_for_display_impl;
	iface->get_short_name_for_display = gedit_text_buffer_get_short_name_for_display_impl;
	iface->get_content_type = gedit_text_buffer_get_content_type_impl;
	iface->get_mime_type = gedit_text_buffer_get_mime_type_impl;
	iface->get_readonly = gedit_text_buffer_get_readonly_impl;
	iface->load_cancel = gedit_text_buffer_load_cancel_impl;
	iface->is_untouched = gedit_text_buffer_is_untouched_impl;
	iface->is_untitled = gedit_text_buffer_is_untitled_impl;
	iface->is_local = gedit_text_buffer_is_local_impl;
	iface->get_deleted = gedit_text_buffer_get_deleted_impl;
	iface->goto_line = gedit_text_buffer_goto_line_impl;
	iface->goto_line_offset = gedit_text_buffer_goto_line_offset_impl;
	iface->get_encoding = gedit_text_buffer_get_encoding_impl;
	iface->get_seconds_since_last_save_or_load = gedit_text_buffer_get_seconds_since_last_save_or_load_impl;
	iface->check_externally_modified = gedit_text_buffer_check_externally_modified_impl;
	iface->undo = gedit_text_buffer_undo_impl;
	iface->redo = gedit_text_buffer_redo_impl;
	iface->can_undo = gedit_text_buffer_can_undo_impl;
	iface->can_redo = gedit_text_buffer_can_redo_impl;
	iface->begin_not_undoable_action = gedit_text_buffer_begin_not_undoable_action_impl;
	iface->end_not_undoable_action = gedit_text_buffer_end_not_undoable_action_impl;
	iface->set_text = gedit_text_buffer_set_text_impl;
	iface->set_modified = gedit_text_buffer_set_modified_impl;
	iface->get_modified = gedit_text_buffer_get_modified_impl;
	iface->get_has_selection = gedit_text_buffer_get_has_selection_impl;
	
	/* signals */
	iface->load = gedit_text_buffer_load_real;
	iface->save = gedit_text_buffer_save_real;
}

static void
set_encoding (GeditTextBuffer     *doc, 
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

static GtkSourceStyleScheme *
get_default_style_scheme (void)
{
	gchar *scheme_id;
	GtkSourceStyleScheme *def_style;
	GtkSourceStyleSchemeManager *manager;

	manager = gedit_get_style_scheme_manager ();
	scheme_id = gedit_prefs_manager_get_source_style_scheme ();
	def_style = gtk_source_style_scheme_manager_get_scheme (manager,
								scheme_id);

	if (def_style == NULL)
	{
		g_warning ("Default style scheme '%s' cannot be found, falling back to 'classic' style scheme ", scheme_id);

		def_style = gtk_source_style_scheme_manager_get_scheme (manager, "classic");
		if (def_style == NULL) 
		{
			g_warning ("Style scheme 'classic' cannot be found, check your GtkSourceView installation.");
		}
	}

	g_free (scheme_id);

	return def_style;
}

static void
gedit_text_buffer_init (GeditTextBuffer *doc)
{
	GtkSourceStyleScheme *style_scheme;

	gedit_debug (DEBUG_DOCUMENT);

	doc->priv = GEDIT_TEXT_BUFFER_GET_PRIVATE (doc);

	doc->priv->uri = NULL;
	doc->priv->untitled_number = get_untitled_number ();

	doc->priv->content_type = g_content_type_from_mime_type ("text/plain");

	doc->priv->readonly = FALSE;

	doc->priv->stop_cursor_moved_emission = FALSE;

	doc->priv->last_save_was_manually = TRUE;
	doc->priv->language_set_by_user = FALSE;

	doc->priv->dispose_has_run = FALSE;

	doc->priv->mtime = 0;

	g_get_current_time (&doc->priv->time_of_last_save_or_load);

	doc->priv->encoding = gedit_encoding_get_utf8 ();

	gtk_source_buffer_set_max_undo_levels (GTK_SOURCE_BUFFER (doc), 
					       gedit_prefs_manager_get_undo_actions_limit ());

	gtk_source_buffer_set_highlight_matching_brackets (GTK_SOURCE_BUFFER (doc), 
							   gedit_prefs_manager_get_bracket_matching ());

	gedit_text_buffer_set_enable_search_highlighting (doc,
						          gedit_prefs_manager_get_enable_search_highlighting ());

	style_scheme = get_default_style_scheme ();
	if (style_scheme != NULL)
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
gedit_text_buffer_new (void)
{
	gedit_debug (DEBUG_DOCUMENT);

	return GEDIT_DOCUMENT (g_object_new (GEDIT_TYPE_TEXT_BUFFER, NULL));
}

/* Note: do not emit the notify::read-only signal */
static void
set_readonly (GeditTextBuffer *doc,
	      gboolean         readonly)
{
	gedit_debug (DEBUG_DOCUMENT);
	
	readonly = (readonly != FALSE);

	if (doc->priv->readonly == readonly) 
		return;

	doc->priv->readonly = readonly;
}

static void
gedit_text_buffer_set_readonly (GeditTextBuffer *doc,
				gboolean       readonly)
{
	gedit_debug (DEBUG_DOCUMENT);

	g_return_if_fail (GEDIT_IS_TEXT_BUFFER (doc));

	set_readonly (doc, readonly);

	g_object_notify (G_OBJECT (doc), "read-only");
}

static void
reset_temp_loading_data (GeditTextBuffer *doc)
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
			GeditTextBuffer     *doc)
{
	/* load was successful */
	if (error == NULL)
	{
		GtkTextIter iter;
		const gchar *content_type;

		content_type = gedit_document_loader_get_content_type (loader);

		doc->priv->mtime = gedit_document_loader_get_mtime (loader);

		g_get_current_time (&doc->priv->time_of_last_save_or_load);

		set_readonly (doc,
		      gedit_document_loader_get_readonly (loader));

		set_encoding (doc, 
			      gedit_document_loader_get_encoding (loader),
			      (doc->priv->requested_encoding != NULL));
		      
		/* We already set the uri */
		set_uri (doc, NULL, content_type);

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
	         (error->domain == G_IO_ERROR && error->code == G_IO_ERROR_NOT_FOUND) &&
	         (gedit_utils_uri_has_file_scheme (doc->priv->uri)))
	{
		reset_temp_loading_data (doc);

		g_signal_emit_by_name (GEDIT_DOCUMENT (doc),
				       "loaded",
				       NULL);

		return;
	}
	
	g_signal_emit_by_name (GEDIT_DOCUMENT (doc),
			       "loaded",
			       error);

	reset_temp_loading_data (doc);
}

static void
document_loader_loading (GeditDocumentLoader *loader,
			 gboolean             completed,
			 const GError        *error,
			 GeditTextBuffer     *doc)
{
	if (completed)
	{
		document_loader_loaded (loader, error, doc);
	}
	else
	{
		goffset size;
		goffset read;

		size = gedit_document_loader_get_file_size (loader);
		read = gedit_document_loader_get_bytes_read (loader);

		g_signal_emit_by_name (GEDIT_DOCUMENT (doc),
				       "loading",
				       read,
				       size);
	}
}

static void
gedit_text_buffer_load_real (GeditDocument       *doc,
			     const gchar         *uri,
			     const GeditEncoding *encoding,
			     gint                 line_pos,
			     gboolean             create)
{
	GeditTextBuffer *buf = GEDIT_TEXT_BUFFER (doc);

	g_return_if_fail (buf->priv->loader == NULL);

	gedit_debug_message (DEBUG_DOCUMENT, "load_real: uri = %s", uri);

	/* create a loader. It will be destroyed when loading is completed */
	buf->priv->loader = gedit_document_loader_new (doc, uri, encoding);

	g_signal_connect (buf->priv->loader,
			  "loading",
			  G_CALLBACK (document_loader_loading),
			  doc);

	buf->priv->create = create;
	buf->priv->requested_encoding = encoding;
	buf->priv->requested_line_pos = line_pos;

	set_uri (buf, uri, NULL);

	gedit_document_loader_load (buf->priv->loader);
}

static void
document_saver_saving (GeditDocumentSaver *saver,
		       gboolean            completed,
		       const GError       *error,
		       GeditTextBuffer    *doc)
{
	gedit_debug (DEBUG_DOCUMENT);

	if (completed)
	{
		/* save was successful */
		if (error == NULL)
		{
			const gchar *uri;
			const gchar *content_type;

			uri = gedit_document_saver_get_uri (saver);

			content_type = gedit_document_saver_get_content_type (saver);

			doc->priv->mtime = gedit_document_saver_get_mtime (saver);

			g_get_current_time (&doc->priv->time_of_last_save_or_load);

			gedit_text_buffer_set_readonly (doc, FALSE);

			gtk_text_buffer_set_modified (GTK_TEXT_BUFFER (doc),
						      FALSE);

			set_uri (doc, uri, content_type);

			set_encoding (doc, 
				      doc->priv->requested_encoding, 
				      TRUE);
		}

		g_signal_emit_by_name (GEDIT_DOCUMENT (doc),
				       "saved",
				       error);

		/* the saver has been used, throw it away */
		g_object_unref (doc->priv->saver);
		doc->priv->saver = NULL;
	}
	else
	{
		goffset size = 0;
		goffset written = 0;

		size = gedit_document_saver_get_file_size (saver);
		written = gedit_document_saver_get_bytes_written (saver);

		gedit_debug_message (DEBUG_DOCUMENT, "save progress: %" G_GINT64_FORMAT " of %" G_GINT64_FORMAT, written, size);

		g_signal_emit_by_name (GEDIT_DOCUMENT (doc),
				       "saving",
				       written,
				       size);
	}
}

static void
gedit_text_buffer_save_real (GeditDocument          *doc,
			     const gchar            *uri,
			     const GeditEncoding    *encoding,
			     GeditDocumentSaveFlags  flags)
{
	GeditTextBuffer *buf = GEDIT_TEXT_BUFFER (doc);

	g_return_if_fail (buf->priv->saver == NULL);

	/* create a saver, it will be destroyed once saving is complete */
	buf->priv->saver = gedit_document_saver_new (doc, uri, encoding, flags);

	g_signal_connect (buf->priv->saver,
			  "saving",
			  G_CALLBACK (document_saver_saving),
			  doc);

	buf->priv->requested_encoding = encoding;

	gedit_document_saver_save (buf->priv->saver,
				   buf->priv->mtime);
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
gedit_text_buffer_set_search_text (GeditTextBuffer *doc,
				   const gchar     *text,
				   guint            flags)
{
	gchar *converted_text;
	gboolean notify = FALSE;
	gboolean update_to_search_region = FALSE;
	
	g_return_if_fail (GEDIT_IS_TEXT_BUFFER (doc));
	g_return_if_fail ((text == NULL) || (doc->priv->search_text != text));
	g_return_if_fail ((text == NULL) || g_utf8_validate (text, -1, NULL));

	gedit_debug_message (DEBUG_DOCUMENT, "text = %s", text);

	if (text != NULL)
	{
		if (*text != '\0')
		{
			converted_text = gedit_utils_unescape_search_text (text);
			notify = !gedit_text_buffer_get_can_search_again (doc);
		}
		else
		{
			converted_text = g_strdup ("");
			notify = gedit_text_buffer_get_can_search_again (doc);
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
gedit_text_buffer_get_search_text (GeditTextBuffer *doc,
				   guint           *flags)
{
	g_return_val_if_fail (GEDIT_IS_TEXT_BUFFER (doc), NULL);

	if (flags != NULL)
		*flags = doc->priv->search_flags;

	return gedit_utils_escape_search_text (doc->priv->search_text);
}

gboolean
gedit_text_buffer_get_can_search_again (GeditTextBuffer *doc)
{
	g_return_val_if_fail (GEDIT_IS_TEXT_BUFFER (doc), FALSE);

	return ((doc->priv->search_text != NULL) && 
	        (*doc->priv->search_text != '\0'));
}

gboolean
gedit_text_buffer_search_forward (GeditTextBuffer   *doc,
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
	
	g_return_val_if_fail (GEDIT_IS_TEXT_BUFFER (doc), FALSE);
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
gedit_text_buffer_search_backward (GeditTextBuffer   *doc,
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
	
	g_return_val_if_fail (GEDIT_IS_TEXT_BUFFER (doc), FALSE);
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
gedit_text_buffer_replace_all (GeditTextBuffer     *doc,
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
	gboolean brackets_highlighting;
	gboolean search_highliting;

	g_return_val_if_fail (GEDIT_IS_TEXT_BUFFER (doc), 0);
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
	brackets_highlighting = gtk_source_buffer_get_highlight_matching_brackets (GTK_SOURCE_BUFFER (buffer));
	gtk_source_buffer_set_highlight_matching_brackets (GTK_SOURCE_BUFFER (buffer), FALSE);

	/* and do search highliting later */
	search_highliting = gedit_text_buffer_get_enable_search_highlighting (doc);
	gedit_text_buffer_set_enable_search_highlighting (doc, FALSE);

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

	gtk_source_buffer_set_highlight_matching_brackets (GTK_SOURCE_BUFFER (buffer),
							   brackets_highlighting);
	gedit_text_buffer_set_enable_search_highlighting (doc, search_highliting);

	g_free (search_text);
	g_free (replace_text);

	return cont;
}

void
gedit_text_buffer_set_language (GeditTextBuffer   *doc, 
				GtkSourceLanguage *lang)
{
	g_return_if_fail (GEDIT_IS_TEXT_BUFFER (doc));

	set_language (doc, lang, TRUE);
}

GtkSourceLanguage *
gedit_text_buffer_get_language (GeditTextBuffer *doc)
{
	g_return_val_if_fail (GEDIT_IS_TEXT_BUFFER (doc), NULL);

	return gtk_source_buffer_get_language (GTK_SOURCE_BUFFER (doc));
}

static void
get_search_match_colors (GeditTextBuffer *doc,
			 gboolean        *foreground_set,
			 GdkColor        *foreground,
			 gboolean        *background_set,
			 GdkColor        *background)
{
	GtkSourceStyleScheme *style_scheme;
	GtkSourceStyle *style;
	gchar *bg;
	gchar *fg;

	style_scheme = gtk_source_buffer_get_style_scheme (GTK_SOURCE_BUFFER (doc));
	if (style_scheme == NULL)
		goto fallback;

	style = gtk_source_style_scheme_get_style (style_scheme,
						   "search-match");
	if (style == NULL)
		goto fallback;

	g_object_get (style, 
		      "foreground-set", foreground_set, 
		      "foreground", &fg,
		      "background-set", background_set, 
		      "background", &bg,
		      NULL);

	if (*foreground_set)
	{
		if (fg == NULL ||
		    !gdk_color_parse (fg, foreground))
		{
			*foreground_set = FALSE;
		}
	}

	if (*background_set)
	{
		if (bg == NULL ||
		    !gdk_color_parse (bg, background))
		{
			*background_set = FALSE;
		}
	}	

	g_free (fg);
	g_free (bg);

	return;

 fallback:
	gedit_debug_message (DEBUG_DOCUMENT, 
			     "Falling back to hard-coded colors "
			     "for the \"found\" text tag.");

	gdk_color_parse ("#FFFF78", background);
	*background_set = TRUE;
	*foreground_set = FALSE;

	return;
}

static void
sync_found_tag (GeditTextBuffer *doc,
		GParamSpec      *pspec,
		gpointer         data)
{
	GdkColor fg;
	GdkColor bg;
	gboolean fg_set;
	gboolean bg_set;

	gedit_debug (DEBUG_DOCUMENT);

	g_return_if_fail (GTK_TEXT_TAG (doc->priv->found_tag));

	get_search_match_colors (doc,
				 &fg_set, &fg,
				 &bg_set, &bg);

	g_object_set (doc->priv->found_tag,
		      "foreground-gdk", fg_set ? &fg : NULL,
		      NULL);
	g_object_set (doc->priv->found_tag,
		      "background-gdk", bg_set ? &bg : NULL,
		      NULL);
}

static void
text_tag_set_highest_priority (GtkTextTag    *tag,
			       GtkTextBuffer *buffer)
{
	GtkTextTagTable *table;
	gint n;

	table = gtk_text_buffer_get_tag_table (buffer);
	n = gtk_text_tag_table_get_size (table);
	gtk_text_tag_set_priority (tag, n - 1);
}

static void
search_region (GeditTextBuffer *doc,
	       GtkTextIter     *start,
	       GtkTextIter     *end)
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
		doc->priv->found_tag = gtk_text_buffer_create_tag (GTK_TEXT_BUFFER (doc), 
								   "found",
								   NULL);

		sync_found_tag (doc, NULL, NULL);

		g_signal_connect (doc, 
				  "notify::style-scheme",
				  G_CALLBACK (sync_found_tag),
				  NULL);
	}

	/* make sure the 'found' tag has the priority over
	 * syntax highlighting tags */
	text_tag_set_highest_priority (doc->priv->found_tag,
				       GTK_TEXT_BUFFER (doc));
				   

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

	if (*doc->priv->search_text == '\0')
		return;

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
			gtk_text_buffer_apply_tag (buffer,
						   doc->priv->found_tag,
						   &m_start,
						   &m_end);
		}		

	} while (found);
}

static void
to_search_region_range (GeditTextBuffer *doc,
			GtkTextIter     *start, 
			GtkTextIter     *end)
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
_gedit_text_buffer_search_region (GeditTextBuffer    *doc,
				  const GtkTextIter  *start,
				  const GtkTextIter  *end)
{
	GeditTextRegion *region;

	gedit_debug (DEBUG_DOCUMENT);
		
	g_return_if_fail (GEDIT_IS_TEXT_BUFFER (doc));
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
insert_text_cb (GeditTextBuffer *doc, 
		GtkTextIter     *pos,
		const gchar     *text, 
		gint             length)
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
delete_range_cb (GeditTextBuffer *doc, 
		 GtkTextIter     *start,
		 GtkTextIter     *end)
{
	GtkTextIter d_start;
	GtkTextIter d_end;

	gedit_debug (DEBUG_DOCUMENT);
		
	d_start = *start;
	d_end = *end;
	
	to_search_region_range (doc, &d_start, &d_end);
}

void
gedit_text_buffer_set_enable_search_highlighting (GeditTextBuffer *doc,
						  gboolean       enable)
{
	g_return_if_fail (GEDIT_IS_TEXT_BUFFER (doc));
	
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
		if (gedit_text_buffer_get_can_search_again (doc))
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
gedit_text_buffer_get_enable_search_highlighting (GeditTextBuffer *doc)
{
	g_return_val_if_fail (GEDIT_IS_TEXT_BUFFER (doc), FALSE);
	
	return (doc->priv->to_search_region != NULL);
}

/* FIXME
void
_gedit_text_buffer_set_mount_operation_factory (GeditTextBuffer		       *doc,
						GeditMountOperationFactory	callback,
						gpointer	                userdata)
{
	g_return_if_fail (GEDIT_IS_TEXT_BUFFER (doc));
	
	doc->priv->mount_operation_factory = callback;
	doc->priv->mount_operation_userdata = userdata;
}

GMountOperation *
_gedit_text_buffer_create_mount_operation (GeditTextBuffer *doc)
{
	g_return_val_if_fail (GEDIT_IS_TEXT_BUFFER (doc), NULL);
	
	if (doc->priv->mount_operation_factory == NULL)
		return g_mount_operation_new ();
	else
		return doc->priv->mount_operation_factory (doc, 
						           doc->priv->mount_operation_userdata);
}*/

void
gedit_text_buffer_get_cursor_position (GeditTextBuffer *buffer,
				       gint  tab_size,
				       gint *row,
				       gint *col)
{
	GtkTextIter iter;
	GtkTextIter start;

	gtk_text_buffer_get_iter_at_mark (GTK_TEXT_BUFFER (buffer),
					  &iter,
					  gtk_text_buffer_get_insert (GTK_TEXT_BUFFER (buffer)));
	
	*row = gtk_text_iter_get_line (&iter);
	
	start = iter;
	gtk_text_iter_set_line_offset (&start, 0);
	*col = 0;

	while (!gtk_text_iter_equal (&start, &iter))
	{
		/* FIXME: Are we Unicode compliant here? */
		if (gtk_text_iter_get_char (&start) == '\t')
					
			*col += (tab_size - (*col  % tab_size));
		else
			++(*col);

		gtk_text_iter_forward_char (&start);
	}
}
