/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-document.c
 * This file is part of gedit
 *
 * Copyright (C) 1998, 1999 Alex Roberts, Evan Lawrence
 * Copyright (C) 2000, 2001 Chema Celorio, Paolo Maggi 
 * Copyright (C) 2002, 2003, 2004 Paolo Maggi 
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
 * Modified by the gedit Team, 1998-2004. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h>  
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <fcntl.h>

#include <libgnome/libgnome.h>
#include <libgnomevfs/gnome-vfs.h>
#include <eel/eel-vfs-extensions.h>
#include <eel/eel-glib-extensions.h>

#include <libgnomevfs/gnome-vfs-mime-utils.h>

#include "gedit-prefs-manager-app.h"
#include "gedit-document.h"
#include "gedit-debug.h"
#include "gedit-utils.h"
#include "gedit-convert.h"
#include "gedit-metadata-manager.h"
#include "gedit-languages-manager.h"

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

struct _GeditDocumentPrivate
{
	gchar		    *uri;
	gint 		     untitled_number;	

	gchar		    *encoding;

	gchar		    *last_searched_text;
	gchar		    *last_replace_text;
	gboolean  	     last_search_was_case_sensitive;
	gboolean  	     last_search_was_entire_word;

	guint		     auto_save_timeout;
	gboolean	     last_save_was_manually; 

	GTimeVal 	     time_of_last_save_or_load;

	gboolean 	     readonly;

	/* Info needed for implementing async loading */
	EelReadFileHandle   *read_handle;
	GnomeVFSAsyncHandle *info_handle;

	const GeditEncoding *temp_encoding;
	gchar		    *temp_uri;
	gulong		     temp_size;
};

enum {
	NAME_CHANGED,
	SAVED,
	LOADED,
	LOADING,
	READONLY_CHANGED,
	CAN_FIND_AGAIN,
	LAST_SIGNAL
};

static void gedit_document_class_init 		(GeditDocumentClass 	*klass);
static void gedit_document_init 		(GeditDocument 		*document);
static void gedit_document_finalize 		(GObject 		*object);

static void gedit_document_real_name_changed		(GeditDocument *document);
static void gedit_document_real_loaded			(GeditDocument *document,
							 const GError  *error);
static void gedit_document_real_saved			(GeditDocument *document);
static void gedit_document_real_readonly_changed	(GeditDocument *document,
							 gboolean readonly);
static void gedit_document_real_can_find_again		(GeditDocument *document);

static gboolean	gedit_document_save_as_real (GeditDocument *doc, const gchar *uri,
	       				     const GeditEncoding *encoding,	
					     gboolean create_backup_copy, GError **error);
static void gedit_document_set_uri (GeditDocument* doc, const gchar* uri);

static gboolean gedit_document_auto_save (GeditDocument *doc, GError **error);
static gboolean gedit_document_auto_save_timeout (GeditDocument *doc);

static GtkTextBufferClass *parent_class 	= NULL;
static guint document_signals[LAST_SIGNAL] 	= { 0 };

static GHashTable* allocated_untitled_numbers = NULL;

static gint gedit_document_get_untitled_number (void);
static void gedit_document_release_untitled_number (gint n);

static void gedit_document_set_encoding (GeditDocument *doc, const GeditEncoding *encoding);

static gint
gedit_document_get_untitled_number (void)
{
	gint i = 1;
	
	gedit_debug (DEBUG_DOCUMENT, "");

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
gedit_document_release_untitled_number (gint n)
{
	gboolean ret;
	
	g_return_if_fail (allocated_untitled_numbers != NULL);

	gedit_debug (DEBUG_DOCUMENT, "");

	ret = g_hash_table_remove (allocated_untitled_numbers, GINT_TO_POINTER (n));
	g_return_if_fail (ret);	
}



GType
gedit_document_get_type (void)
{
	static GType document_type = 0;

  	if (document_type == 0)
    	{
      		static const GTypeInfo our_info =
      		{
        		sizeof (GeditDocumentClass),
        		NULL, 		/* base_init,*/
        		NULL, 		/* base_finalize, */
        		(GClassInitFunc) gedit_document_class_init,
        		NULL,           /* class_finalize */
        		NULL,           /* class_data */
        		sizeof (GeditDocument),
        		0,              /* n_preallocs */
        		(GInstanceInitFunc) gedit_document_init
      		};

      		document_type = g_type_register_static (GTK_TYPE_SOURCE_BUFFER,
                					"GeditDocument",
                                           	 	&our_info,
                                           		0);
    	}

	return document_type;
}

	
static void
gedit_document_class_init (GeditDocumentClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

  	parent_class = g_type_class_peek_parent (klass);

  	object_class->finalize = gedit_document_finalize;

        klass->name_changed 	= gedit_document_real_name_changed;
	klass->loaded 	    	= gedit_document_real_loaded;
	klass->saved        	= gedit_document_real_saved;
	klass->readonly_changed = gedit_document_real_readonly_changed;
	klass->can_find_again	= gedit_document_real_can_find_again;

  	document_signals[NAME_CHANGED] =
   		g_signal_new ("name_changed",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GeditDocumentClass, name_changed),
			      NULL, NULL,
			      gedit_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
	
	document_signals[LOADED] =
   		g_signal_new ("loaded",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GeditDocumentClass, loaded),
			      NULL, NULL,
			      gedit_marshal_VOID__POINTER,
			      G_TYPE_NONE,
			      1,
			      G_TYPE_POINTER);

	document_signals[LOADING] =
   		g_signal_new ("loading",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GeditDocumentClass, loading),
			      NULL, NULL,
			      gedit_marshal_VOID__ULONG_ULONG,
			      G_TYPE_NONE,
			      2,
			      G_TYPE_ULONG,
			      G_TYPE_ULONG);

	
	document_signals[SAVED] =
   		g_signal_new ("saved",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GeditDocumentClass, saved),
			      NULL, NULL,
			      gedit_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);

	document_signals[READONLY_CHANGED] =
   		g_signal_new ("readonly_changed",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GeditDocumentClass, readonly_changed),
			      NULL, NULL,
			      gedit_marshal_VOID__BOOLEAN,
			      G_TYPE_NONE,
			      1,
			      G_TYPE_BOOLEAN);

	document_signals[CAN_FIND_AGAIN] =
   		g_signal_new ("can_find_again",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GeditDocumentClass, can_find_again),
			      NULL, NULL,
			      gedit_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
}

static void
gedit_document_init (GeditDocument *document)
{
	const GeditEncoding *enc;
	
	gedit_debug (DEBUG_DOCUMENT, "");

	document->priv = g_new0 (GeditDocumentPrivate, 1);
	
	document->priv->uri = NULL;
	document->priv->untitled_number = 0;

	document->priv->readonly = FALSE;

	document->priv->last_save_was_manually = TRUE;

	g_get_current_time (&document->priv->time_of_last_save_or_load);

	enc = gedit_encoding_get_utf8 ();
		
	document->priv->encoding = g_strdup (
				gedit_encoding_get_charset (enc));

	gedit_document_set_max_undo_levels (document,
					    gedit_prefs_manager_get_undo_actions_limit ());    

	gtk_source_buffer_set_check_brackets (GTK_SOURCE_BUFFER (document), FALSE);
}

static void
gedit_document_finalize (GObject *object)
{
	GeditDocument *document;

	gedit_debug (DEBUG_DOCUMENT, "");

	g_return_if_fail (object != NULL);
	g_return_if_fail (GEDIT_IS_DOCUMENT (object));

   	document = GEDIT_DOCUMENT (object);

	g_return_if_fail (document->priv != NULL);

	if (document->priv->auto_save_timeout > 0)
		g_source_remove (document->priv->auto_save_timeout);

	if (document->priv->untitled_number > 0)
	{
		g_return_if_fail (document->priv->uri == NULL);
		gedit_document_release_untitled_number (
				document->priv->untitled_number);
	}

	if (document->priv->uri != NULL)
	{
		gchar *position;
		gchar *lang_id = NULL;
		GtkSourceLanguage *lang;

		position = g_strdup_printf ("%d", 
					    gedit_document_get_cursor (document));
		
		gedit_metadata_manager_set (document->priv->uri,
					    "position",
					    position);

		g_free (position);

		gedit_metadata_manager_set (document->priv->uri,
					    "last_searched_text", 
					    document->priv->last_searched_text);

		gedit_metadata_manager_set (document->priv->uri,
					    "last_replaced_text", 
					    document->priv->last_replace_text);

		lang = gedit_document_get_language (document);

		if (lang != NULL)
		{
			lang_id = gtk_source_language_get_id (lang);
			g_return_if_fail (lang_id != NULL);
		}
	
		gedit_metadata_manager_set (document->priv->uri,
					    "language",
					    (lang_id == NULL) ? "_NORMAL_" : lang_id);
			
		g_free (lang_id);
	}

	g_free (document->priv->uri);
	g_free (document->priv->last_searched_text);
	g_free (document->priv->last_replace_text);
	g_free (document->priv->encoding);

	g_free (document->priv);
	document->priv = NULL;
	
	G_OBJECT_CLASS (parent_class)->finalize (object);
}

/**
 * gedit_document_new:
 *
 * Creates a new untitled document.
 *
 * Return value: a new untitled document
 **/
GeditDocument*
gedit_document_new (void)
{
 	GeditDocument *document;

	gedit_debug (DEBUG_DOCUMENT, "");

	document = GEDIT_DOCUMENT (g_object_new (GEDIT_TYPE_DOCUMENT, NULL));

	g_return_val_if_fail (document->priv != NULL, NULL);
  	
	document->priv->untitled_number = gedit_document_get_untitled_number ();
	g_return_val_if_fail (document->priv->untitled_number > 0, NULL);
	
	return document;
}

/**
 * gedit_document_new_with_uri:
 * @uri: the URI of the file that has to be loaded
 * @error: return location for error or NULL
 * 
 * Creates a new document.
 *
 * Return value: a new document
 **/
GeditDocument*
gedit_document_new_with_uri (const gchar          *uri,  
			     const GeditEncoding  *encoding)
{
 	GeditDocument *document;

	gedit_debug (DEBUG_DOCUMENT, "");

	g_return_val_if_fail (uri != NULL, NULL);
	
	document = GEDIT_DOCUMENT (g_object_new (GEDIT_TYPE_DOCUMENT, NULL));

	g_return_val_if_fail (document->priv != NULL, NULL);
	document->priv->uri = g_strdup (uri);

	gedit_document_load (document, uri, encoding);
	
	return document;
}

/**
 * gedit_document_set_readonly:
 * @document: a #GeditDocument
 * @readonly: if TRUE (FALSE) the @document will be set as (not) readonly
 * 
 * Set the value of the readonly flag.
 **/
void 		
gedit_document_set_readonly (GeditDocument *document, gboolean readonly)
{
	gboolean auto_save;
	
	gedit_debug (DEBUG_DOCUMENT, "");

	g_return_if_fail (document != NULL);
	g_return_if_fail (document->priv != NULL);

	auto_save = gedit_prefs_manager_get_auto_save ();
	
	if (readonly) 
	{
		if (auto_save && (document->priv->auto_save_timeout > 0))
		{
			gedit_debug (DEBUG_DOCUMENT, "Remove autosave timeout");

			g_source_remove (document->priv->auto_save_timeout);
			document->priv->auto_save_timeout = 0;
		}
	}
	else
	{
		if (auto_save && (document->priv->auto_save_timeout <= 0))
		{
			gint auto_save_interval;
			
			gedit_debug (DEBUG_DOCUMENT, "Install autosave timeout");

			auto_save_interval = 
				gedit_prefs_manager_get_auto_save_interval ();
				
			document->priv->auto_save_timeout = g_timeout_add 
				(auto_save_interval * 1000 * 60,
		 		 (GSourceFunc)gedit_document_auto_save_timeout,
		  		 document);		
		}
	}

	if (document->priv->readonly == readonly) 
		return;

	document->priv->readonly = readonly;

	g_signal_emit (G_OBJECT (document),
                       document_signals[READONLY_CHANGED],
                       0,
		       readonly);
}

/**
 * gedit_document_is_readonly:
 * @document: a #GeditDocument
 *
 * Returns TRUE is @document is readonly. 
 * 
 * Return value: TRUE if @document is readonly. FALSE otherwise.
 **/
gboolean
gedit_document_is_readonly (GeditDocument *document)
{
	gedit_debug (DEBUG_DOCUMENT, "");

	g_return_val_if_fail (document != NULL, TRUE);
	g_return_val_if_fail (document->priv != NULL, TRUE);

	return document->priv->readonly;	
}

static void 
gedit_document_real_name_changed (GeditDocument *document)
{
	gedit_debug (DEBUG_DOCUMENT, "");

	g_return_if_fail (document != NULL);
}

static void 
gedit_document_real_loaded (GeditDocument *document, const GError *error)
{
	gchar *data;

	gedit_debug (DEBUG_DOCUMENT, "");

	g_return_if_fail (GEDIT_IS_DOCUMENT (document));

	if (error != NULL)
		return;

	g_return_if_fail (document->priv->uri != NULL);

	/* FIXME: commented since it does not work as expected - Paolo */
	/*
	data = gedit_metadata_manager_get (document->priv->uri,
					   "position");
	if (data != NULL)
	{
		gedit_document_set_cursor (document, atoi (data));

		g_free (data);
	}
	*/

	data = gedit_metadata_manager_get (document->priv->uri,
					   "last_searched_text");
	if (data != NULL)
	{
		if (document->priv->last_searched_text == NULL)
			gedit_document_set_last_searched_text (document,
							       data);

		g_free (data);
	}

	data = gedit_metadata_manager_get (document->priv->uri,
					   "last_replaced_text");
	if (data != NULL)
	{
		if (document->priv->last_replace_text == NULL)
			gedit_document_set_last_replace_text (document,
							      data);

		g_free (data);
	}
}

static void 
gedit_document_real_saved (GeditDocument *document)
{
	gedit_debug (DEBUG_DOCUMENT, "");

	g_return_if_fail (document != NULL);
}	

static void 
gedit_document_real_readonly_changed (GeditDocument *document, gboolean readonly)
{
	gedit_debug (DEBUG_DOCUMENT, "");

	g_return_if_fail (document != NULL);
}

static void 
gedit_document_real_can_find_again (GeditDocument *document)
{
	gedit_debug (DEBUG_DOCUMENT, "");

	g_return_if_fail (document != NULL);
}


gchar*
gedit_document_get_raw_uri (const GeditDocument* doc)
{
	gedit_debug (DEBUG_DOCUMENT, "");

	if (doc->priv->uri == NULL)
		return NULL;
	else
		return g_strdup (doc->priv->uri);
}

/* 
 * Returns a well formatted (ready to display) URI in UTF-8 format 
 * See: gedit_document_get_raw_uri to have a raw uri (non UTF-8)
 */
gchar*
gedit_document_get_uri (const GeditDocument* doc)
{
	gedit_debug (DEBUG_DOCUMENT, "");

	if (doc->priv->uri == NULL)
		return g_strdup_printf (_("%s %d"), _("Untitled"), doc->priv->untitled_number);
	else
	{
		gchar *res;

		res = eel_format_uri_for_display (doc->priv->uri);
		g_return_val_if_fail (res != NULL, g_strdup (_("Invalid file name")));
		
		return res;
	}
}

gchar*
gedit_document_get_short_name (const GeditDocument* doc)
{
	gedit_debug (DEBUG_DOCUMENT, "");

	if (doc->priv->uri == NULL)
		return g_strdup_printf (_("%s %d"), _("Untitled"), doc->priv->untitled_number);
	else
	{
		gchar *basename;
		gchar *utf8_basename;
		
		basename = eel_uri_get_basename (doc->priv->uri);

		if (basename != NULL) 
		{
			gboolean filenames_are_locale_encoded;
		       	filenames_are_locale_encoded = g_getenv ("G_BROKEN_FILENAMES") != NULL;

			if (filenames_are_locale_encoded) 
			{
				utf8_basename = g_locale_to_utf8 (basename, -1, NULL, NULL, NULL);
				
				if (utf8_basename != NULL) 
				{
					g_free (basename);
					return utf8_basename;
				} 
			}
			else 
			{
				if (g_utf8_validate (basename, -1, NULL)) 
					return basename;
			}
			
			/* there are problems */
			utf8_basename = eel_make_valid_utf8 (basename);
			g_free (basename);
			return utf8_basename;
		}
		else
			return 	g_strdup (_("Invalid file name"));
	}
}

GQuark 
gedit_document_io_error_quark (void)
{
	static GQuark quark;
	
	if (!quark)
		quark = g_quark_from_static_string ("gedit_io_load_error");

	return quark;
}

static gboolean
gedit_document_auto_save_timeout (GeditDocument *doc)
{
	GError *error = NULL;

	gedit_debug (DEBUG_DOCUMENT, "");

	g_return_val_if_fail (!gedit_document_is_readonly (doc), FALSE);

	/* Remove timeout if now auto_save is FALSE */
	if (!gedit_prefs_manager_get_auto_save ()) 
		return FALSE;

	if (!gedit_document_get_modified (doc))
		return TRUE;

	gedit_document_auto_save (doc, &error);

	if (error) 
	{
		/* FIXME - Should we actually tell 
		 * the user there was an error? - James */
		g_error_free (error);
		return TRUE;
	}

	return TRUE;
}

static gboolean
gedit_document_auto_save (GeditDocument* doc, GError **error)
{
	const GeditEncoding *encoding;
	
	gedit_debug (DEBUG_DOCUMENT, "");
	
	g_return_val_if_fail (doc != NULL, FALSE);

	encoding = gedit_document_get_encoding (doc);
		
	if (gedit_document_save_as_real (doc, doc->priv->uri, encoding,
					 doc->priv->last_save_was_manually, NULL))
	{
		doc->priv->last_save_was_manually = FALSE;
		g_get_current_time (&doc->priv->time_of_last_save_or_load);
	}

	return TRUE;
}

/* NOTE: thie function frees file_contents */
static gboolean
update_document_contents (GeditDocument        *doc, 
			  const gchar          *uri,
		          const gchar          *file_contents,
			  gint                  file_size,
			  const GeditEncoding  *encoding,
			  GError              **error)
{
	GtkTextIter iter, end;

	PROFILE (
		g_message ("Delete previous text %s: %.3f", uri, g_timer_elapsed (timer, NULL));
	)
	
	PROFILE (
		g_message ("Deleted: %.3f", g_timer_elapsed (timer, NULL));
	)

	if (file_size > 0)
	{
		GError *conv_error = NULL;
		gchar  *converted_text;
		gint    len = -1;

		g_return_val_if_fail (file_contents != NULL, FALSE);

		if (encoding == gedit_encoding_get_utf8 ())
		{
			if (g_utf8_validate (file_contents, file_size, NULL))
			{
				converted_text = (gchar *)file_contents;
				len = file_size;
				file_contents = NULL;
			}
			else
			{
				converted_text = NULL;
				conv_error = g_error_new (GEDIT_CONVERT_ERROR, 
						          GEDIT_CONVERT_ERROR_ILLEGAL_SEQUENCE,
							  "Invalid byte sequence");
			}
		}
		else
		{
			converted_text = NULL;
			
			if (encoding == NULL)
			{
				const GeditEncoding *enc;
				gchar *charset;

				charset = gedit_metadata_manager_get (uri, "encoding");

				if (charset != NULL)
				{
					enc = gedit_encoding_get_from_charset (charset);

					if (enc != NULL)
					{	
						converted_text = gedit_convert_to_utf8 (file_contents,
										file_size,
										&enc,
										NULL);

						if (converted_text != NULL)
							encoding = enc;
					}

					g_free (charset);
				}
			}

			if (converted_text == NULL)				
				converted_text = gedit_convert_to_utf8 (file_contents,
								file_size,
								&encoding,
								&conv_error);
		}
		
		if (converted_text == NULL)
		{
			/* bail out */
			if (conv_error == NULL)
				g_set_error (error, GEDIT_DOCUMENT_IO_ERROR,
					     GEDIT_ERROR_INVALID_UTF8_DATA,
					     _("Invalid UTF-8 data"));
			else
				g_propagate_error (error, conv_error);

			return FALSE;
		}

		PROFILE (
			g_message ("Text converted: %.3f", g_timer_elapsed (timer, NULL));
		)

		gedit_document_begin_not_undoable_action (doc);

		gedit_document_delete_text (doc, 0, -1);

		/* Insert text in the buffer */
		gtk_text_buffer_get_iter_at_offset (GTK_TEXT_BUFFER (doc), &iter, 0);
		gtk_text_buffer_insert (GTK_TEXT_BUFFER (doc), &iter, converted_text, len);

		PROFILE (
			g_message ("Text inserted: %.3f", g_timer_elapsed (timer, NULL));
		)

		if (file_contents != NULL)
		{
			/* If file_contents == NULL then converted_text is only an alias of
			 * file_contents and hence it must not be freed */
			g_free (converted_text);
		}

		/* We had a newline in the buffer to begin with. (The buffer always contains
   		 * a newline, so we delete to the end of the buffer to clean up. 
		 * FIXME: Is this really needed? - Paolo (Jan 8, 2004) */
		gtk_text_buffer_get_end_iter (GTK_TEXT_BUFFER (doc), &end);
 		gtk_text_buffer_delete (GTK_TEXT_BUFFER (doc), &iter, &end);

		/* Place the cursor at the start of the document or and old cursor position
		 * if reverting */
		gedit_document_set_cursor (doc, 0);

		gedit_document_end_not_undoable_action (doc);

		PROFILE (
			g_message ("Text inserted all: %.3f", g_timer_elapsed (timer, NULL));
		)

	}
	else
		encoding = gedit_encoding_get_utf8 ();

	if (gedit_utils_is_uri_read_only (uri))
	{
		gedit_debug (DEBUG_DOCUMENT, "READ-ONLY");

		gedit_document_set_readonly (doc, TRUE);
	}
	else
	{
		gedit_debug (DEBUG_DOCUMENT, "NOT READ-ONLY");

		gedit_document_set_readonly (doc, FALSE);
	}

	gtk_text_buffer_set_modified (GTK_TEXT_BUFFER (doc), FALSE);

	gedit_document_set_uri (doc, uri);

	gedit_document_set_encoding (doc, encoding);

	g_get_current_time (&doc->priv->time_of_last_save_or_load);
	
	return TRUE;
}

static void
reset_temp_data (GeditDocument *doc)
{
	g_free (doc->priv->temp_uri);
	
	doc->priv->temp_uri = NULL;
	doc->priv->temp_encoding = NULL;
	doc->priv->temp_size = 0;
	
	doc->priv->read_handle = NULL;
	doc->priv->info_handle = NULL;
}

static void
file_loaded_cb (GnomeVFSResult    result,
                GnomeVFSFileSize  file_size,
                gchar            *file_contents,
                gpointer          callback_data)
{
	GError *error = NULL;
	gboolean ret;
	GeditDocument *doc;

	gedit_debug (DEBUG_DOCUMENT, "");

	doc = GEDIT_DOCUMENT (callback_data);
	
	/*
	g_print ("Loaded URI: %s\n", doc->priv->temp_uri);
	g_print ("Final file size: %d\n", (int)file_size);
	*/

	PROFILE (
		g_message ("Loaded Document %s: %.3f", doc->priv->temp_uri, g_timer_elapsed (timer, NULL));
	)

	if (result != GNOME_VFS_OK) 
	{
		reset_temp_data (doc);
		
		g_free (file_contents);

		error = g_error_new (GEDIT_DOCUMENT_IO_ERROR,
				     result,
				     gnome_vfs_result_to_string (result));

		g_signal_emit (G_OBJECT (doc), document_signals[LOADED], 0, error);

		g_error_free (error);

                return;
        }

	ret = update_document_contents (doc, 
					doc->priv->temp_uri, 
					file_contents, 
					file_size, 
					doc->priv->temp_encoding, 
					&error);

	PROFILE (
		g_message ("Update Document %s: %.3f", doc->priv->uri, g_timer_elapsed (timer, NULL));
	)

	if (error) 
	{
		g_signal_emit (G_OBJECT (doc), document_signals[LOADED], 0, error);

		g_error_free (error);
	}
	else
	{
		g_signal_emit (G_OBJECT (doc), 
			       document_signals[LOADED], 
			       0, 
			       NULL);
	}

	PROFILE ({
		g_message ("Return from signal LAODED %s: %.3f", doc->priv->uri, g_timer_elapsed (timer, NULL));
		g_timer_destroy (timer);
	})

	reset_temp_data (doc);

	g_free (file_contents);	
}

static gboolean
read_more_cb (GnomeVFSFileSize  file_size,
	      const char       *file_contents,
	      gpointer          callback_data)
{
	GeditDocument *doc;

	doc = GEDIT_DOCUMENT (callback_data);

	/*
	g_print ("Loading URI: %s\n", doc->priv->temp_uri);
	g_print ("File size: %ld/%ld\n", (gulong)file_size, doc->priv->temp_size);
	*/

	g_signal_emit (G_OBJECT (doc), 
		       document_signals[LOADING], 
		       0, 
		       (gulong)file_size,
		       doc->priv->temp_size);

	return TRUE;
}

static void
get_info_cb (GnomeVFSAsyncHandle *handle,
	     GList               *results,
	     gpointer             callback_data)
{
	GeditDocument *doc;
	GnomeVFSGetFileInfoResult *info_result;

	gedit_debug (DEBUG_DOCUMENT, "");

	g_return_if_fail (eel_g_list_exactly_one_item (results));

	doc = GEDIT_DOCUMENT (callback_data);
	doc->priv->info_handle = NULL;

	info_result = (GnomeVFSGetFileInfoResult *)results->data;
	g_return_if_fail (info_result != NULL);

	if (info_result->result != GNOME_VFS_OK)
	{
		GError *error = NULL;
		
		gedit_debug (DEBUG_DOCUMENT, "Error reading %s : %s",
			     doc->priv->temp_uri,
			     gnome_vfs_result_to_string (info_result->result));

		reset_temp_data (doc);

		error =  g_error_new (GEDIT_DOCUMENT_IO_ERROR, 
				      info_result->result,
				      gnome_vfs_result_to_string (info_result->result));

		g_signal_emit (G_OBJECT (doc), 
			       document_signals[LOADED], 
			       0, 
			       error);

		g_error_free (error);
		
		return;
	}

	if (info_result->file_info->valid_fields & GNOME_VFS_FILE_INFO_FIELDS_SIZE)
	{
		doc->priv->temp_size = (gulong)info_result->file_info->size;
	}
	else
	{
		doc->priv->temp_size = 0;
	}
	
	PROFILE (
		g_message ("Got Info on Document %s: %.3f", 
			doc->priv->temp_uri, g_timer_elapsed (timer, NULL));
	)

	doc->priv->read_handle = eel_read_file_async (doc->priv->temp_uri, 
						      GNOME_VFS_PRIORITY_MAX,
						      file_loaded_cb,
						      read_more_cb,
						      doc);
}

static gboolean
load_local (GeditDocument *doc)
{
	GError *error = NULL;
	GnomeVFSResult res;
	gint file_size;
	gchar *file_contents;
	gboolean ret;
	
	PROFILE (
		g_message ("Start loading entire file %s: %.3f", 
			doc->priv->uri, g_timer_elapsed (timer, NULL));
	)

	res = gnome_vfs_read_entire_file (doc->priv->temp_uri, 
					  &file_size, 
					  &file_contents);

	if (res != GNOME_VFS_OK)
	{		
		reset_temp_data (doc);
		
		g_free (file_contents);

		error = g_error_new (GEDIT_DOCUMENT_IO_ERROR, 
				     res,
				     gnome_vfs_result_to_string (res));

		g_signal_emit (G_OBJECT (doc), 
			       document_signals[LOADED], 
			       0, 
			       error);

		g_error_free (error);
		
		return FALSE;
	}

	ret = update_document_contents (doc, 
					doc->priv->temp_uri, 
					file_contents, 
					file_size, 
					doc->priv->temp_encoding, 
					&error);

	PROFILE (
		g_message ("Update Document %s: %.3f", doc->priv->uri, g_timer_elapsed (timer, NULL));
	)

	reset_temp_data (doc);

	if (error) 
	{
		g_signal_emit (G_OBJECT (doc), document_signals[LOADED], 0, error);

		g_error_free (error);
	}
	else
	{
		g_signal_emit (G_OBJECT (doc), 
			       document_signals[LOADED], 
			       0, 
			       NULL);
	}

	PROFILE ({
		g_message ("Return from signal LAODED : %.3f",  
			   g_timer_elapsed (timer, NULL));
		g_timer_destroy (timer);
	})

	g_free (file_contents);

	return FALSE;
}

static gboolean
emit_invalid_uri (GeditDocument *doc)
{
	GError *error = NULL;
	GnomeVFSResult res;

	reset_temp_data (doc);
	
	res = GNOME_VFS_ERROR_INVALID_URI;

	error = g_error_new (GEDIT_DOCUMENT_IO_ERROR, 
			     res,
			     gnome_vfs_result_to_string (res));

	g_signal_emit (G_OBJECT (doc), 
		       document_signals[LOADED], 
		       0, 
		       error);

	g_error_free (error);
		
	return FALSE;
}

/**
 * gedit_document_load:
 * @doc: a GeditDocument
 * @uri: the URI of the file that has to be loaded
 * 
 * Read a document from a file
 *
 * Return value: TRUE if the file is correctly loaded
 **/
void
gedit_document_load (GeditDocument        *doc, 
		     const gchar          *uri, 
		     const GeditEncoding  *encoding)
{	
	GnomeVFSURI *u;
		
	g_return_if_fail (GEDIT_IS_DOCUMENT (doc));
	g_return_if_fail (uri != NULL);

	PROFILE ({
		timer = g_timer_new ();
		g_message ("Start loading %s", uri);
	})

	gedit_debug (DEBUG_DOCUMENT, "File to load: %s", uri);

	u = gnome_vfs_uri_new (uri);
	if (u == NULL)
	{
		g_timeout_add_full (G_PRIORITY_HIGH,
				    0,
				    (GSourceFunc)emit_invalid_uri,
				    doc,
				    NULL);

		return;
	}

	doc->priv->temp_encoding = encoding;
	doc->priv->temp_uri = g_strdup (uri);

	if (!eel_vfs_has_capability (doc->priv->temp_uri, EEL_VFS_CAPABILITY_IS_REMOTE_AND_SLOW))
	{
		doc->priv->temp_size = 0;

		g_timeout_add_full (G_PRIORITY_HIGH,
				    0,
				    (GSourceFunc)load_local,
				    doc,
				    NULL);

		gnome_vfs_uri_unref (u);
	}
	else
	{
		GList *uri_list = NULL;

		uri_list = g_list_prepend (uri_list, u);
	
		gnome_vfs_async_get_file_info (&doc->priv->info_handle,
					       uri_list,
					       GNOME_VFS_FILE_INFO_DEFAULT |
					       GNOME_VFS_FILE_INFO_FOLLOW_LINKS,
					       GNOME_VFS_PRIORITY_MAX,
					       get_info_cb,
					       doc);

		g_signal_emit (G_OBJECT (doc), 
			       document_signals[LOADING], 
			       0, 
			       0,
			       0);

		gnome_vfs_uri_unref (u);
		
		g_list_free (uri_list);
	}
}

gboolean
gedit_document_load_from_stdin (GeditDocument* doc, GError **error)
{
	gchar *stdin_data;
	GtkTextIter iter, end;
	
	gedit_debug (DEBUG_DOCUMENT, "");

	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), FALSE);
	
	stdin_data = gedit_utils_get_stdin ();

	if (stdin_data != NULL && strlen (stdin_data) > 0)
	{
		GError *conv_error = NULL;
		const GeditEncoding *encoding;
		gchar *converted_text;

		if (gedit_encoding_get_current () != gedit_encoding_get_utf8 ())
		{
			if (g_utf8_validate (stdin_data, -1, NULL))
			{
				converted_text = stdin_data;
				stdin_data = NULL;

				encoding = gedit_encoding_get_utf8 ();
			}
			else
			{
				converted_text = NULL;
			}
		}
		else
		{	
			encoding = gedit_encoding_get_current ();
				
			converted_text = gedit_convert_to_utf8 (stdin_data,
								-1,
								&encoding,
								&conv_error);
		}	
		
		if (converted_text == NULL)
		{
			/* bail out */
			if (conv_error == NULL)
				g_set_error (error, GEDIT_DOCUMENT_IO_ERROR,
					     GEDIT_ERROR_INVALID_UTF8_DATA,
					     _("Invalid UTF-8 data"));
			else
				g_propagate_error (error, conv_error);

			g_free (stdin_data);

			return FALSE;
		}

		gedit_document_begin_not_undoable_action (doc);
		/* Insert text in the buffer */
		gtk_text_buffer_get_iter_at_offset (GTK_TEXT_BUFFER (doc), &iter, 0);
		gtk_text_buffer_insert (GTK_TEXT_BUFFER (doc), &iter, converted_text, -1);

		/* We had a newline in the buffer to begin with. (The buffer always contains
   		 * a newline, so we delete to the end of the buffer to clean up. */
		gtk_text_buffer_get_end_iter (GTK_TEXT_BUFFER (doc), &end);
 		gtk_text_buffer_delete (GTK_TEXT_BUFFER (doc), &iter, &end);

		/* Place the cursor at the start of the document */
		gtk_text_buffer_get_iter_at_offset (GTK_TEXT_BUFFER (doc), &iter, 0);
		gtk_text_buffer_place_cursor (GTK_TEXT_BUFFER (doc), &iter);

		gedit_document_end_not_undoable_action (doc);
		
		g_free (converted_text);
	}

	if (stdin_data != NULL)
		g_free (stdin_data);
			

	gedit_document_set_readonly (doc, FALSE);
	gtk_text_buffer_set_modified (GTK_TEXT_BUFFER (doc), TRUE);
	g_get_current_time (&doc->priv->time_of_last_save_or_load);

	g_signal_emit (G_OBJECT (doc), document_signals [LOADED], 0, NULL);

	return TRUE;
}


gboolean 
gedit_document_is_untouched (const GeditDocument *doc)
{
	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), TRUE);

	gedit_debug (DEBUG_DOCUMENT, "");

	return (doc->priv->uri == NULL) && 
		(!gtk_text_buffer_get_modified (GTK_TEXT_BUFFER (doc)));
}

/* FIXME: Remove this function when gnome-vfs will add an equivalent public
   function - Paolo (Mar 05, 2004) */
static gchar *
get_slow_mime_type (const char *text_uri)
{
	GnomeVFSFileInfo *info;
	char *mime_type;
	GnomeVFSResult result;

	info = gnome_vfs_file_info_new ();
	result = gnome_vfs_get_file_info (text_uri, info,
					  GNOME_VFS_FILE_INFO_GET_MIME_TYPE |
					  GNOME_VFS_FILE_INFO_FORCE_SLOW_MIME_TYPE |
					  GNOME_VFS_FILE_INFO_FOLLOW_LINKS);
	if (info->mime_type == NULL || result != GNOME_VFS_OK) {
		mime_type = NULL;
	} else {
		mime_type = g_strdup (info->mime_type);
	}
	gnome_vfs_file_info_unref (info);

	return mime_type;
}

static void
gedit_document_set_uri (GeditDocument* doc, const gchar* uri)
{
	GtkSourceLanguage *language;
	gchar *data;
	
	gedit_debug (DEBUG_DOCUMENT, "");

	g_return_if_fail (GEDIT_IS_DOCUMENT (doc));
	g_return_if_fail (uri != NULL);

	if (doc->priv->uri == uri)
		return;

	if (doc->priv->uri != NULL)
		g_free (doc->priv->uri);
		
	g_return_if_fail (uri != NULL);

	doc->priv->uri = g_strdup (uri);

	if (doc->priv->untitled_number > 0)
	{
		gedit_document_release_untitled_number (doc->priv->untitled_number);
		doc->priv->untitled_number = 0;
	}

	data = gedit_metadata_manager_get (uri, "language");

	if (data != NULL)
	{
		gedit_debug (DEBUG_DOCUMENT, "Language: %s", data);

		if (strcmp (data, "_NORMAL_") == 0)
			language = NULL;
		else
			language = gedit_languages_manager_get_language_from_id (
						gedit_get_languages_manager (),
						data);

		gedit_document_set_language (doc, language);

		g_free (data);
	}
	else
	{
		gchar *mime_type;

		mime_type = get_slow_mime_type (uri);
	
		if (mime_type != NULL)
		{
			GtkSourceLanguage *language;
			
			gedit_debug (DEBUG_DOCUMENT, "MIME-TYPE: %s", mime_type);

			language = gtk_source_languages_manager_get_language_from_mime_type (
						gedit_get_languages_manager (),
						mime_type);
	
			gedit_document_set_language (doc, language);
	
			g_free (mime_type);
		}
		else
		{
			g_warning ("Couldn't get mime type for file `%s'", uri);
		}

		
	}

	g_signal_emit (G_OBJECT (doc), document_signals[NAME_CHANGED], 0);
}
	
gboolean 
gedit_document_save (GeditDocument *doc, GError **error)
{	
	gboolean auto_save;
	gboolean create_backup_copy;
	const GeditEncoding *encoding;
	
	gboolean ret;

	gedit_debug (DEBUG_DOCUMENT, "");

	g_return_val_if_fail (doc != NULL, FALSE);
	g_return_val_if_fail (doc->priv != NULL, FALSE);
	g_return_val_if_fail (!doc->priv->readonly, FALSE);
	g_return_val_if_fail (doc->priv->uri != NULL, FALSE);
	
	encoding = gedit_document_get_encoding (doc);
	g_return_val_if_fail (encoding != NULL, FALSE);
	
	auto_save = gedit_prefs_manager_get_auto_save ();
	create_backup_copy = gedit_prefs_manager_get_create_backup_copy ();
		
	if (auto_save) 
	{
		if (doc->priv->auto_save_timeout > 0)
		{
			g_source_remove (doc->priv->auto_save_timeout);
			doc->priv->auto_save_timeout = 0;
		}
	}
	
	ret = gedit_document_save_as_real (doc, 
					   doc->priv->uri, 
					   encoding,
					   create_backup_copy, 
					   error);

	if (ret)
	{
		doc->priv->last_save_was_manually = TRUE;
		g_get_current_time (&doc->priv->time_of_last_save_or_load);
	}

	if (auto_save && (doc->priv->auto_save_timeout <= 0)) 
	{
		gint auto_save_interval = 
			gedit_prefs_manager_get_auto_save_interval ();

		doc->priv->auto_save_timeout =
			g_timeout_add (auto_save_interval * 1000 * 60,
				       (GSourceFunc) gedit_document_auto_save, 
				       doc);
	}

	return ret;
}

gboolean	
gedit_document_save_as (GeditDocument* doc, const gchar *uri, 
			const GeditEncoding *encoding, GError **error)
{	
	gboolean auto_save;
	
	gboolean ret = FALSE;
	
	gedit_debug (DEBUG_DOCUMENT, "");

	g_return_val_if_fail (doc != NULL, FALSE);
	g_return_val_if_fail (doc->priv != NULL, FALSE);
	g_return_val_if_fail (uri != NULL, FALSE);

	if (encoding == NULL)
		encoding = gedit_document_get_encoding (doc);

	auto_save = gedit_prefs_manager_get_auto_save ();

	if (auto_save) 
	{

		if (doc->priv->auto_save_timeout > 0)
		{
			g_source_remove (doc->priv->auto_save_timeout);
			doc->priv->auto_save_timeout = 0;
		}
	}

	if (gedit_document_save_as_real (doc, uri, encoding, TRUE, error))
	{
		gedit_document_set_uri (doc, uri);
		gedit_document_set_readonly (doc, FALSE);
		
		g_get_current_time (&doc->priv->time_of_last_save_or_load);
		doc->priv->last_save_was_manually = TRUE;
		
		gedit_document_set_encoding (doc, encoding);
			
		ret = TRUE;
	}		

	if (auto_save && (doc->priv->auto_save_timeout <= 0)) 
	{
		gint auto_save_interval =
			gedit_prefs_manager_get_auto_save_interval ();
		
		doc->priv->auto_save_timeout =
			g_timeout_add (auto_save_interval * 1000 * 60,
				       (GSourceFunc)gedit_document_auto_save, doc);
	}

	return ret;
}

gboolean	
gedit_document_save_a_copy_as (GeditDocument *doc, const gchar *uri, 
			       const GeditEncoding *encoding, GError **error)
{
	gboolean m;
	gboolean ret;

	gedit_debug (DEBUG_DOCUMENT, "");

	g_return_val_if_fail (doc != NULL, FALSE);
	g_return_val_if_fail (doc->priv != NULL, FALSE);
	g_return_val_if_fail (uri != NULL, FALSE);

	if (encoding == NULL)
		encoding = gedit_document_get_encoding (doc);
	
	m = gedit_document_get_modified (doc);

	ret = gedit_document_save_as_real (doc, uri, encoding, FALSE, error);
	
	gtk_text_buffer_set_modified (GTK_TEXT_BUFFER (doc), m);	 

	return ret;
}

#define MAX_LINK_LEVEL 256

/* Does readlink() recursively until we find a real filename. */
static char *
follow_symlinks (const gchar *filename, GError **error)
{
	gchar *followed_filename;
	gint link_count;

	g_return_val_if_fail (filename != NULL, NULL);
	
	gedit_debug (DEBUG_DOCUMENT, "Filename: %s", filename); 
	g_return_val_if_fail (strlen (filename) + 1 <= GEDIT_MAX_PATH_LEN, NULL);

	followed_filename = g_strdup (filename);
	link_count = 0;

	while (link_count < MAX_LINK_LEVEL)
	{
		struct stat st;

		if (lstat (followed_filename, &st) != 0)
			/* We could not access the file, so perhaps it does not
			 * exist.  Return this as a real name so that we can
			 * attempt to create the file.
			 */
			return followed_filename;

		if (S_ISLNK (st.st_mode))
		{
			gint len;
			gchar linkname[GEDIT_MAX_PATH_LEN];

			link_count++;

			len = readlink (followed_filename, linkname, GEDIT_MAX_PATH_LEN - 1);

			if (len == -1)
			{
				g_set_error (error, GEDIT_DOCUMENT_IO_ERROR, errno,
					     _("Could not read symbolic link information "
					       "for %s"), followed_filename);
				g_free (followed_filename);
				return NULL;
			}

			linkname[len] = '\0';

			/* If the linkname is not an absolute path name, append
			 * it to the directory name of the followed filename.  E.g.
			 * we may have /foo/bar/baz.lnk -> eek.txt, which really
			 * is /foo/bar/eek.txt.
			 */

			if (linkname[0] != G_DIR_SEPARATOR)
			{
				gchar *slashpos;
				gchar *tmp;

				slashpos = strrchr (followed_filename, G_DIR_SEPARATOR);

				if (slashpos)
					*slashpos = '\0';
				else
				{
					tmp = g_strconcat ("./", followed_filename, NULL);
					g_free (followed_filename);
					followed_filename = tmp;
				}

				tmp = g_build_filename (followed_filename, linkname, NULL);
				g_free (followed_filename);
				followed_filename = tmp;
			}
			else
			{
				g_free (followed_filename);
				followed_filename = g_strdup (linkname);
			}
		} else
			return followed_filename;
	}

	/* Too many symlinks */

	g_set_error (error, GEDIT_DOCUMENT_IO_ERROR, ELOOP,
		     _("The file has too many symbolic links."));

	return NULL;
}

/* FIXME: define new ERROR_CODE and remove the error 
 * strings from here -- Paolo
 */

static gboolean	
gedit_document_save_as_real (GeditDocument* doc, const gchar *uri, const GeditEncoding *encoding,
			     gboolean create_backup_copy, GError **error)
{
	gchar *filename; /* Filename without URI scheme */
	gchar *real_filename; /* Final filename with no symlinks */
	gchar *backup_filename; /* Backup filename, like real_filename.bak */
	gchar *temp_filename; /* Filename for saving */
	gchar *slashpos;
	gchar *dirname;
	mode_t saved_umask;
	struct stat st;
	gchar *chars = NULL;
	gint chars_len;
	gint fd;
	gint retval;
	gboolean res;
	gboolean add_cr;
	
	gedit_debug (DEBUG_DOCUMENT, "");

	g_return_val_if_fail (doc != NULL, FALSE);
	g_return_val_if_fail (doc->priv != NULL, FALSE);
	g_return_val_if_fail (uri != NULL, FALSE);
	g_return_val_if_fail (encoding != NULL, FALSE);

	retval = FALSE;

	filename = NULL;
	real_filename = NULL;
	backup_filename = NULL;
	temp_filename = NULL;

	/* We don't support non-file:/// stuff */

	if (!gedit_utils_uri_has_file_scheme (uri))
	{
		gchar *error_message;
		gchar *scheme_string;

		gchar *temp = eel_uri_get_scheme (uri);
                scheme_string = eel_make_valid_utf8 (temp);
		g_free (temp);

		if (scheme_string != NULL)
		{
			error_message = g_strdup_printf (
				_("gedit cannot handle %s: locations in write mode."),
                               	scheme_string);

			g_free (scheme_string);
		}
		else
			error_message = g_strdup_printf (
				_("gedit cannot handle this kind of location in write mode."));

		g_set_error (error, GEDIT_DOCUMENT_IO_ERROR, EROFS, error_message);
		g_free (error_message);
		return FALSE;
	}

	/* Get filename from uri */
	filename = gnome_vfs_get_local_path_from_uri (uri);

	if (!filename)
	{
		g_set_error (error, GEDIT_DOCUMENT_IO_ERROR, 0,
			     _("Invalid filename."));
		goto out;
	}

	/* Get the real filename and file permissions */

	real_filename = follow_symlinks (filename, error);

	if (!real_filename)
		goto out;

	/* Get the directory in which the real filename lives */

	slashpos = strrchr (real_filename, G_DIR_SEPARATOR);

	if (slashpos)
	{
		dirname = g_strdup (real_filename);
		dirname[slashpos - real_filename + 1] = '\0';
	}
	else
		dirname = g_strdup (".");

	/* If there is not an existing file with that name, compute the
	 * permissions and uid/gid that we will use for the newly-created file.
	 */

	if (stat (real_filename, &st) != 0)
	{
		struct stat dir_st;
		int result;

		/* File does not exist? */
		create_backup_copy = FALSE;

		/* Use default permissions */
		saved_umask = umask(0);
		st.st_mode = 0666 & ~saved_umask;
		umask(saved_umask);
		st.st_uid = getuid ();

		result = stat (dirname, &dir_st);

		if (result == 0 && (dir_st.st_mode & S_ISGID))
			st.st_gid = dir_st.st_gid;
		else
			st.st_gid = getgid ();
	}

	/* Save to a temporary file.  We set the umask because some (buggy)
	 * implementations of mkstemp() use permissions 0666 and we want 0600.
	 */

	temp_filename = g_build_filename (dirname, ".gedit-save-XXXXXX", NULL);
	g_free (dirname);

	saved_umask = umask (0077);
	fd = g_mkstemp (temp_filename); /* this modifies temp_filename to the used name */
	umask (saved_umask);

	if (fd == -1)
	{
		g_set_error (error, GEDIT_DOCUMENT_IO_ERROR, errno, " ");
		goto out;
	}

	chars = gedit_document_get_chars (doc, 0, -1);
	
	if (encoding != gedit_encoding_get_utf8 ())
	{
		GError *conv_error = NULL;
		gchar* converted_file_contents = NULL;

		converted_file_contents = gedit_convert_from_utf8 (chars, 
								   -1, 
								   encoding,
								   &conv_error);

		if (conv_error != NULL)
		{
			/* Conversion error */
			g_propagate_error (error, conv_error);

			close (fd);
			unlink (temp_filename);
			goto out;
		}
		else
		{
			g_free (chars);
			chars = converted_file_contents;
		}
	}

	chars_len = strlen (chars);

	add_cr = (*(chars + chars_len - 1) != '\n');

	/* Save the file content */
	res = (write (fd, chars, chars_len) == chars_len);

	if (res && add_cr)
		/* Add \n if needed */
		res = (write (fd, "\n", 1) == 1);

	if (!res)
	{
		gchar *msg;

		switch (errno)
		{
			case ENOSPC:
				msg = _("There is not enough disk space to save the file.\n"
					"Please free some disk space and try again.");
				break;

			case EFBIG:
				msg = _("The disk where you are trying to save the file has "
					"a limitation on file sizes.  Please try saving "
					"a smaller file or saving it to a disk that does not "
					"have this limitation.");
				break;

			default:
				msg = " ";
				break;
		}

		g_set_error (error, GEDIT_DOCUMENT_IO_ERROR, errno, msg);
		close (fd);
		unlink (temp_filename);
		goto out;
	}

	if (close (fd) != 0)
	{
		g_set_error (error, GEDIT_DOCUMENT_IO_ERROR, errno, " ");
		unlink (temp_filename);
		goto out;
	}

	/* Move the original file to a backup */

	if (create_backup_copy)
	{
		gchar *bak_ext;
		gint result;

		bak_ext = gedit_prefs_manager_get_backup_extension ();
		
		backup_filename = g_strconcat (real_filename, 
					       bak_ext,
					       NULL);

		g_free (bak_ext);
		
		result = rename (real_filename, backup_filename);

		if (result != 0)
		{
			g_set_error (error, GEDIT_DOCUMENT_IO_ERROR, errno,
				     _("Could not create a backup file."));
			unlink (temp_filename);
			goto out;
		}
	}

	/* Move the temp file to the original file */

	if (rename (temp_filename, real_filename) != 0)
	{
		gint saved_errno;

		saved_errno = errno;

		if (create_backup_copy && rename (backup_filename, real_filename) != 0)
			g_set_error (error, GEDIT_DOCUMENT_IO_ERROR, errno,
				     " ");
		else
			g_set_error (error, GEDIT_DOCUMENT_IO_ERROR, saved_errno,
				     " ");

		goto out;
	}

	/* Restore permissions.  There is not much error checking we can do
	 * here, I'm afraid.  The final data is saved anyways.
	 */

	chmod (real_filename, st.st_mode);
	chown (real_filename, st.st_uid, st.st_gid);

	gtk_text_buffer_set_modified (GTK_TEXT_BUFFER (doc), FALSE);

	retval = TRUE;

	/* Done */

 out:

	g_free (chars);

	g_free (filename);
	g_free (real_filename);
	g_free (backup_filename);
	g_free (temp_filename);

	return retval;
}

gboolean	
gedit_document_is_untitled (const GeditDocument* doc)
{
	gedit_debug (DEBUG_DOCUMENT, "");

	g_return_val_if_fail (doc != NULL, FALSE);
	g_return_val_if_fail (doc->priv != NULL, FALSE);

	return (doc->priv->uri == NULL);
}

gboolean	
gedit_document_get_modified (const GeditDocument* doc)
{
	gedit_debug (DEBUG_DOCUMENT, "");

	g_return_val_if_fail (doc != NULL, FALSE);

	return gtk_text_buffer_get_modified (GTK_TEXT_BUFFER (doc));
}

gboolean
gedit_document_get_deleted (GeditDocument *doc)
{
	gchar *raw_uri;
	gboolean deleted = FALSE;

	gedit_debug (DEBUG_DOCUMENT, "");

	g_return_val_if_fail (doc != NULL, FALSE);

	raw_uri = gedit_document_get_raw_uri (doc); 
	if (raw_uri)
	{
		if (gedit_document_is_readonly (doc))
			deleted = FALSE;
		else
			deleted = !gedit_utils_uri_exists (raw_uri);
	}
	g_free (raw_uri);

	return deleted;
}

/**
 * gedit_document_get_char_count:
 * @doc: a #GeditDocument 
 * 
 * Gets the number of characters in the buffer; note that characters
 * and bytes are not the same, you can't e.g. expect the contents of
 * the buffer in string form to be this many bytes long. 
 * 
 * Return value: number of characters in the document
 **/
gint 
gedit_document_get_char_count (const GeditDocument *doc)
{
	gedit_debug (DEBUG_DOCUMENT, "");

	g_return_val_if_fail (doc != NULL, FALSE);

	return gtk_text_buffer_get_char_count (GTK_TEXT_BUFFER (doc));
}

/**
 * gedit_document_get_line_count:
 * @doc: a #GeditDocument 
 * 	
 * Obtains the number of lines in the buffer. 
 * 
 * Return value: number of lines in the document
 **/
gint 
gedit_document_get_line_count (const GeditDocument *doc)
{
	gedit_debug (DEBUG_DOCUMENT, "");

	g_return_val_if_fail (doc != NULL, FALSE);

	return gtk_text_buffer_get_line_count (GTK_TEXT_BUFFER (doc));
}

void 
gedit_document_revert (GeditDocument *doc)
{
	gedit_debug (DEBUG_DOCUMENT, "");

	g_return_if_fail (doc != NULL);
	g_return_if_fail (!gedit_document_is_untitled (doc));

	gedit_document_load (doc, doc->priv->uri, gedit_document_get_encoding (doc));

	return;
}

void 
gedit_document_insert_text (GeditDocument *doc, gint pos, const gchar *text, gint len)
{
	GtkTextIter iter;
	
	gedit_debug (DEBUG_DOCUMENT, "");

	g_return_if_fail (GEDIT_IS_DOCUMENT (doc));
	g_return_if_fail (pos >= 0);
	g_return_if_fail (text != NULL);
	g_return_if_fail (g_utf8_validate (text, len, NULL));

	gtk_text_buffer_get_iter_at_offset (GTK_TEXT_BUFFER (doc), &iter, pos);
	
	gtk_text_buffer_insert (GTK_TEXT_BUFFER (doc), &iter, text, len);
}

void 
gedit_document_insert_text_at_cursor (GeditDocument *doc, const gchar *text, gint len)
{
	gedit_debug (DEBUG_DOCUMENT, "");

	g_return_if_fail (GEDIT_IS_DOCUMENT (doc));
	g_return_if_fail (text != NULL);
	g_return_if_fail (g_utf8_validate (text, len, NULL));

	gtk_text_buffer_insert_at_cursor (GTK_TEXT_BUFFER (doc), text, len);
}


void 
gedit_document_delete_text (GeditDocument *doc, gint start, gint end)
{
	GtkTextIter start_iter;
	GtkTextIter end_iter;

	gedit_debug (DEBUG_DOCUMENT, "");

	g_return_if_fail (GEDIT_IS_DOCUMENT (doc));
	g_return_if_fail (start >= 0);
	g_return_if_fail ((end > start) || end < 0);

	gtk_text_buffer_get_iter_at_offset (GTK_TEXT_BUFFER (doc), &start_iter, start);

	if (end < 0)
		gtk_text_buffer_get_end_iter (GTK_TEXT_BUFFER (doc), &end_iter);
	else
		gtk_text_buffer_get_iter_at_offset (GTK_TEXT_BUFFER (doc), &end_iter, end);

	gtk_text_buffer_delete (GTK_TEXT_BUFFER (doc), &start_iter, &end_iter);
}

gchar*
gedit_document_get_chars (GeditDocument *doc, gint start, gint end)
{
	GtkTextIter start_iter;
	GtkTextIter end_iter;

	gedit_debug (DEBUG_DOCUMENT, "");

	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), NULL);
	g_return_val_if_fail (start >= 0, NULL);
	g_return_val_if_fail ((end > start) || (end < 0), NULL);

	gtk_text_buffer_get_iter_at_offset (GTK_TEXT_BUFFER (doc), &start_iter, start);

	if (end < 0)
		gtk_text_buffer_get_end_iter (GTK_TEXT_BUFFER (doc), &end_iter);
	else
		gtk_text_buffer_get_iter_at_offset (GTK_TEXT_BUFFER (doc), &end_iter, end);

	return gtk_text_buffer_get_slice (GTK_TEXT_BUFFER (doc), &start_iter, &end_iter, TRUE);
}

void
gedit_document_set_max_undo_levels (GeditDocument *doc, gint max_undo_levels)
{
	gedit_debug (DEBUG_DOCUMENT, "");

	g_return_if_fail (GEDIT_IS_DOCUMENT (doc));

	gtk_source_buffer_set_max_undo_levels (GTK_SOURCE_BUFFER (doc), 
					       max_undo_levels);
}

gboolean
gedit_document_can_undo	(const GeditDocument *doc)
{
	gedit_debug (DEBUG_DOCUMENT, "");

	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), FALSE);

	return gtk_source_buffer_can_undo (GTK_SOURCE_BUFFER (doc));
}

gboolean
gedit_document_can_redo (const GeditDocument *doc)
{
	gedit_debug (DEBUG_DOCUMENT, "");

	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), FALSE);

	return gtk_source_buffer_can_redo (GTK_SOURCE_BUFFER (doc));
}

/**
 *  A function to determine whether or not the "FindNext/Prev"
 *  functions are executable
 */
gboolean
gedit_document_can_find_again (const GeditDocument *doc)
{
	gedit_debug (DEBUG_DOCUMENT, "");

	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), FALSE);

	return (doc->priv->last_searched_text != NULL);
}

void 
gedit_document_undo (GeditDocument *doc)
{
	gedit_debug (DEBUG_DOCUMENT, "");

	g_return_if_fail (GEDIT_IS_DOCUMENT (doc));
	g_return_if_fail (gedit_document_can_undo (doc));

	gtk_source_buffer_undo (GTK_SOURCE_BUFFER (doc));
}

void 
gedit_document_redo (GeditDocument *doc)
{
	gedit_debug (DEBUG_DOCUMENT, "");

	g_return_if_fail (GEDIT_IS_DOCUMENT (doc));
	g_return_if_fail (gedit_document_can_redo (doc));

	gtk_source_buffer_redo (GTK_SOURCE_BUFFER (doc));
}

void
gedit_document_begin_not_undoable_action (GeditDocument *doc)
{
	gedit_debug (DEBUG_DOCUMENT, "");

	g_return_if_fail (GEDIT_IS_DOCUMENT (doc));

	gtk_source_buffer_begin_not_undoable_action (GTK_SOURCE_BUFFER (doc));
}

void	
gedit_document_end_not_undoable_action	(GeditDocument *doc)
{
	gedit_debug (DEBUG_DOCUMENT, "");

	g_return_if_fail (GEDIT_IS_DOCUMENT (doc));

	gtk_source_buffer_end_not_undoable_action (GTK_SOURCE_BUFFER (doc));
}

void		
gedit_document_begin_user_action (GeditDocument *doc)
{
	gedit_debug (DEBUG_DOCUMENT, "");

	g_return_if_fail (GEDIT_IS_DOCUMENT (doc));

	gtk_text_buffer_begin_user_action (GTK_TEXT_BUFFER (doc));
}	

void		
gedit_document_end_user_action (GeditDocument *doc)
{
	gedit_debug (DEBUG_DOCUMENT, "");

	g_return_if_fail (GEDIT_IS_DOCUMENT (doc));

	gtk_text_buffer_end_user_action (GTK_TEXT_BUFFER (doc));
}


void
gedit_document_goto_line (GeditDocument* doc, guint line)
{
	guint line_count;
	GtkTextIter iter;
	
	gedit_debug (DEBUG_DOCUMENT, "");

	g_return_if_fail (GEDIT_IS_DOCUMENT (doc));
	g_return_if_fail (doc->priv != NULL);
	
	line_count = gtk_text_buffer_get_line_count (GTK_TEXT_BUFFER (doc));
	
	if (line > line_count)
		line = line_count;

	gtk_text_buffer_get_iter_at_line (GTK_TEXT_BUFFER (doc), &iter, line);
	gtk_text_buffer_place_cursor (GTK_TEXT_BUFFER (doc), &iter);
}

gchar* 
gedit_document_get_last_searched_text (GeditDocument* doc)
{
	gedit_debug (DEBUG_DOCUMENT, "");

	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), FALSE);
	g_return_val_if_fail (doc->priv != NULL, FALSE);
	
	return (doc->priv->last_searched_text != NULL) ? 
		g_strdup (doc->priv->last_searched_text) : NULL;	
}

gchar*
gedit_document_get_last_replace_text (GeditDocument* doc)
{
	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), FALSE);
	g_return_val_if_fail (doc->priv != NULL, FALSE);

	return (doc->priv->last_replace_text != NULL) ? 
		g_strdup (doc->priv->last_replace_text) : NULL;	
}

void 
gedit_document_set_last_searched_text (GeditDocument* doc, const gchar *text)
{
	gedit_debug (DEBUG_DOCUMENT, "");

	g_return_if_fail (GEDIT_IS_DOCUMENT (doc));
	g_return_if_fail (doc->priv != NULL);
	g_return_if_fail (text != NULL);

	if (doc->priv->last_searched_text != NULL)
		g_free (doc->priv->last_searched_text);

	doc->priv->last_searched_text = g_strdup (text);
}

void 
gedit_document_set_last_replace_text (GeditDocument* doc, const gchar *text)
{
	gedit_debug (DEBUG_DOCUMENT, "");

	g_return_if_fail (GEDIT_IS_DOCUMENT (doc));
	g_return_if_fail (doc->priv != NULL);
	g_return_if_fail (text != NULL);

	if (doc->priv->last_replace_text != NULL)
		g_free (doc->priv->last_replace_text);

	doc->priv->last_replace_text = g_strdup (text);
}

gboolean
gedit_document_find (GeditDocument* doc, const gchar* str, gint flags)
{
	GtkTextIter iter;
	gboolean found = FALSE;
	GtkSourceSearchFlags search_flags;
	gchar *converted_str;
	gboolean is_first_find = FALSE;

	gedit_debug (DEBUG_DOCUMENT, "");

	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), FALSE);
	g_return_val_if_fail (str != NULL, FALSE);

	converted_str = gedit_utils_convert_search_text (str);
	g_return_val_if_fail (converted_str != NULL, FALSE);
	
	gedit_debug (DEBUG_DOCUMENT, "str: %s", str);
	gedit_debug (DEBUG_DOCUMENT, "converted_str: %s", converted_str);

	search_flags = GTK_SOURCE_SEARCH_VISIBLE_ONLY | GTK_SOURCE_SEARCH_TEXT_ONLY;
	
	if (!GEDIT_SEARCH_IS_CASE_SENSITIVE (flags))
	{
		search_flags = search_flags | GTK_SOURCE_SEARCH_CASE_INSENSITIVE;
	}

	if (GEDIT_SEARCH_IS_FROM_CURSOR (flags))
	{
		GtkTextIter sel_bound;
		
		gtk_text_buffer_get_iter_at_mark (GTK_TEXT_BUFFER (doc), &iter,
                                    gtk_text_buffer_get_mark (GTK_TEXT_BUFFER (doc),
					                      "insert"));
		
		gtk_text_buffer_get_iter_at_mark (GTK_TEXT_BUFFER (doc), &sel_bound,
                                    gtk_text_buffer_get_mark (GTK_TEXT_BUFFER (doc),
					                      "selection_bound"));
		
		if (!GEDIT_SEARCH_IS_BACKWARDS (flags))
			gtk_text_iter_order (&sel_bound, &iter);		
		else
			gtk_text_iter_order (&iter, &sel_bound);		
	}
	else		
	{
		if (GEDIT_SEARCH_IS_BACKWARDS (flags))
		{
			/* get an iterator at the end of the document */
			gtk_text_buffer_get_iter_at_offset (GTK_TEXT_BUFFER (doc),
							    &iter, -1);
		}
		else
		{
			/* get an iterator at the beginning of the document */
			gtk_text_buffer_get_iter_at_offset (GTK_TEXT_BUFFER (doc),
							    &iter, 0);
		}
	}
	
	if (*converted_str != '\0')
    	{
      		GtkTextIter match_start, match_end;

		while (!found)
		{
			if (GEDIT_SEARCH_IS_BACKWARDS (flags))
			{
				/*gtk_text_iter_backward_char (&iter);*/
				
	          		found = gtk_source_iter_backward_search (&iter,
							converted_str, search_flags,
                        	                	&match_start, &match_end,
                                	               	NULL);	
			}
			else
			{
	          		found = gtk_source_iter_forward_search (&iter,
							converted_str, search_flags,
                        	                	&match_start, &match_end,
                                	               	NULL);
			}

			if (found && GEDIT_SEARCH_IS_ENTIRE_WORD (flags))
			{
				found = gtk_text_iter_starts_word (&match_start) && 
					gtk_text_iter_ends_word (&match_end);

				if (!found) 
					iter = match_end;
			}
			else
				break;
		}
		
		if (found)
		{
			gtk_text_buffer_place_cursor (GTK_TEXT_BUFFER (doc),
					&match_start);

			gtk_text_buffer_move_mark_by_name (GTK_TEXT_BUFFER (doc),
					"selection_bound", &match_end);
		}

		if (gedit_document_can_find_again (doc))
		{
			g_free (doc->priv->last_searched_text);
		}
		else
		{
			is_first_find = TRUE;
		}

		doc->priv->last_searched_text = g_strdup (str);
		doc->priv->last_search_was_case_sensitive = GEDIT_SEARCH_IS_CASE_SENSITIVE (flags);
		doc->priv->last_search_was_entire_word = GEDIT_SEARCH_IS_ENTIRE_WORD (flags);

		if (is_first_find)
		{
			g_signal_emit (G_OBJECT (doc), document_signals[CAN_FIND_AGAIN], 0);
		}
	}

	g_free (converted_str);

	g_signal_emit (G_OBJECT (doc), document_signals[CAN_FIND_AGAIN], 0);

	return found;
}

static gboolean
gedit_document_find_again (GeditDocument* doc, gint flags)
{
	gchar* last_searched_text;
	gboolean found;
	
	gedit_debug (DEBUG_DOCUMENT, "");

	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), FALSE);
	g_return_val_if_fail (doc->priv != NULL, FALSE);

	last_searched_text = gedit_document_get_last_searched_text (doc);
	
	if (last_searched_text == NULL)
		return FALSE;
	
	/* pack bitflag array with previous data */
	GEDIT_SEARCH_SET_CASE_SENSITIVE (flags, doc->priv->last_search_was_case_sensitive);
	GEDIT_SEARCH_SET_ENTIRE_WORD (flags, doc->priv->last_search_was_entire_word);

	/* run the search */
	found = gedit_document_find (doc, last_searched_text, flags);

	g_free (last_searched_text);

	return found;
}

gboolean 
gedit_document_find_next (GeditDocument* doc, gint flags)
{
	gedit_debug (DEBUG_DOCUMENT, "");

	GEDIT_SEARCH_SET_BACKWARDS (flags, FALSE);

	/* run the search */
	return gedit_document_find_again (doc, flags);
}

gboolean 
gedit_document_find_prev (GeditDocument* doc, gint flags)
{
	gedit_debug (DEBUG_DOCUMENT, "");

	/* pack bitflag array with the backwards searching operative */
	GEDIT_SEARCH_SET_BACKWARDS (flags, TRUE);

	/* run the search */
	return gedit_document_find_again (doc, flags);
}

gboolean 
gedit_document_get_selection (GeditDocument *doc, gint *start, gint *end)
{
	GtkTextIter iter;
	GtkTextIter sel_bound;

	gedit_debug (DEBUG_DOCUMENT, "");

	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), FALSE);

	gtk_text_buffer_get_iter_at_mark (GTK_TEXT_BUFFER (doc),			
                                    &iter,
                                    gtk_text_buffer_get_mark (GTK_TEXT_BUFFER (doc),
					                      "insert"));
		
	gtk_text_buffer_get_iter_at_mark (GTK_TEXT_BUFFER (doc),			
                                    &sel_bound,
                                    gtk_text_buffer_get_mark (GTK_TEXT_BUFFER (doc),
					                      "selection_bound"));
	gtk_text_iter_order (&iter, &sel_bound);	

	if (start != NULL)
		*start = gtk_text_iter_get_offset (&iter); 

	if (end != NULL)
		*end = gtk_text_iter_get_offset (&sel_bound); 

	return !gtk_text_iter_equal (&sel_bound, &iter);	
}

void
gedit_document_replace_selected_text (GeditDocument *doc, const gchar *replace)
{
	GtkTextIter iter;
	GtkTextIter sel_bound;
	gchar *converted_replace;

	gedit_debug (DEBUG_DOCUMENT, "");

	g_return_if_fail (GEDIT_IS_DOCUMENT (doc));
	g_return_if_fail (replace != NULL);	

	converted_replace = gedit_utils_convert_search_text (replace);

	gtk_text_buffer_get_iter_at_mark (GTK_TEXT_BUFFER (doc),			
                                    &iter,
                                    gtk_text_buffer_get_mark (GTK_TEXT_BUFFER (doc),
					                      "insert"));
		
	gtk_text_buffer_get_iter_at_mark (GTK_TEXT_BUFFER (doc),			
                                    &sel_bound,
                                    gtk_text_buffer_get_mark (GTK_TEXT_BUFFER (doc),
					                      "selection_bound"));

	if (gtk_text_iter_equal (&sel_bound, &iter))
	{
		gedit_debug (DEBUG_DOCUMENT, "There is no selected text");

		return;
	}
	
	gtk_text_iter_order (&sel_bound, &iter);		

	gedit_document_begin_user_action (doc);

	gtk_text_buffer_delete (GTK_TEXT_BUFFER (doc),
				&iter,
				&sel_bound);

	gtk_text_buffer_get_iter_at_mark (GTK_TEXT_BUFFER (doc),			
                                    &iter,
                                    gtk_text_buffer_get_mark (GTK_TEXT_BUFFER (doc),
					                      "insert"));
	if (*converted_replace != '\0')
		gtk_text_buffer_insert (GTK_TEXT_BUFFER (doc),
					&iter,
					converted_replace, strlen (converted_replace));

	if (doc->priv->last_replace_text != NULL)
		g_free (doc->priv->last_replace_text);

	doc->priv->last_replace_text = g_strdup (replace);

	gedit_document_end_user_action (doc);
	g_free (converted_replace);
}

gboolean
gedit_document_replace_all (GeditDocument *doc,
			    const gchar *find, const gchar *replace,
			    gint flags)
{
	gboolean cont = 0;

	gedit_debug (DEBUG_DOCUMENT, "");

	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), FALSE);
	g_return_val_if_fail (find != NULL, FALSE);
	g_return_val_if_fail (replace != NULL, FALSE);

	gedit_document_begin_user_action (doc);

	GEDIT_SEARCH_SET_BACKWARDS (flags, FALSE);
	GEDIT_SEARCH_SET_FROM_CURSOR (flags, FALSE);

	while (gedit_document_find (doc, find, flags)) 
	{
		gedit_document_replace_selected_text (doc, replace);
		
		GEDIT_SEARCH_SET_FROM_CURSOR (flags, TRUE);
		
		++cont;
	}

	gedit_document_end_user_action (doc);

	return cont;
}

guint
gedit_document_get_line_at_offset (const GeditDocument *doc, guint offset)
{
	GtkTextIter iter;
	
	gedit_debug (DEBUG_DOCUMENT, "");

	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), 0);

	gtk_text_buffer_get_iter_at_offset (GTK_TEXT_BUFFER (doc), &iter, offset);
	
	return gtk_text_iter_get_line (&iter);
}

gint 
gedit_document_get_cursor (GeditDocument *doc)
{
	GtkTextIter iter;
	
	gedit_debug (DEBUG_DOCUMENT, "");

	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), 0);

	gtk_text_buffer_get_iter_at_mark (GTK_TEXT_BUFFER (doc),			
                                    &iter,
                                    gtk_text_buffer_get_mark (GTK_TEXT_BUFFER (doc),
					                      "insert"));

	return gtk_text_iter_get_offset (&iter); 
}

void
gedit_document_set_cursor (GeditDocument *doc, gint cursor)
{
	GtkTextIter iter;
	
	gedit_debug (DEBUG_DOCUMENT, "");

	g_return_if_fail (GEDIT_IS_DOCUMENT (doc));
	
	/* Place the cursor at the requested position */
	gtk_text_buffer_get_iter_at_offset (GTK_TEXT_BUFFER (doc), &iter, cursor);
	gtk_text_buffer_place_cursor (GTK_TEXT_BUFFER (doc), &iter);
}

void
gedit_document_set_selection (GeditDocument *doc, gint start, gint end)
{
 	GtkTextIter start_iter;
	GtkTextIter end_iter;

	gedit_debug (DEBUG_DOCUMENT, "");

	g_return_if_fail (GEDIT_IS_DOCUMENT (doc));
	g_return_if_fail (start >= 0);
	g_return_if_fail ((end > start) || (end < 0));

	gtk_text_buffer_get_iter_at_offset (GTK_TEXT_BUFFER (doc), &start_iter, start);

	if (end < 0)
		gtk_text_buffer_get_end_iter (GTK_TEXT_BUFFER (doc), &end_iter);
	else
		gtk_text_buffer_get_iter_at_offset (GTK_TEXT_BUFFER (doc), &end_iter, end);

	gtk_text_buffer_place_cursor (GTK_TEXT_BUFFER (doc), &end_iter);

	gtk_text_buffer_move_mark_by_name (GTK_TEXT_BUFFER (doc),
					"selection_bound", &start_iter);
}

void 
gedit_document_set_language (GeditDocument *doc, GtkSourceLanguage *lang)
{
	g_return_if_fail (GEDIT_IS_DOCUMENT (doc));

	if (lang != NULL)
		gtk_source_buffer_set_highlight (GTK_SOURCE_BUFFER (doc), TRUE);
	else
		gtk_source_buffer_set_highlight (GTK_SOURCE_BUFFER (doc), FALSE);

	gtk_source_buffer_set_language (GTK_SOURCE_BUFFER (doc), lang);

	if (lang != NULL)
		gedit_language_init_tag_styles (lang);

	if (doc->priv->uri != NULL)
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

	return gedit_encoding_get_from_charset (doc->priv->encoding);
}

static void
gedit_document_set_encoding (GeditDocument *doc, const GeditEncoding *encoding)
{
	g_return_if_fail (encoding != NULL);

	if (doc->priv->encoding != NULL)
		g_free (doc->priv->encoding);

	doc->priv->encoding = g_strdup (gedit_encoding_get_charset (encoding));

	gedit_metadata_manager_set (doc->priv->uri,
				    "encoding",
				    doc->priv->encoding);

	/* FIXME: do we need a encoding changed signal ? - Paolo */
	g_signal_emit (G_OBJECT (doc), document_signals[NAME_CHANGED], 0);

}

glong
gedit_document_get_seconds_since_last_save_or_load (GeditDocument *doc)
{
	GTimeVal current_time;

	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), -1);

	g_get_current_time(&current_time);

	return (current_time.tv_sec - doc->priv->time_of_last_save_or_load.tv_sec);
}

