/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-document.h
 * This file is part of gedit
 *
 * Copyright (C) 1998, 1999 Alex Roberts, Evan Lawrence
 * Copyright (C) 2000, 2002 Chema Celorio, Paolo Maggi 
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
 * Modified by the gedit Team, 1998-2002. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 */

#ifndef __GEDIT_DOCUMENT_H__
#define __GEDIT_DOCUMENT_H__


#include <gtk/gtk.h>
#include <gtksourceview/gtksourcebuffer.h>

#include <gedit/gedit-encodings.h>

#define GEDIT_TYPE_DOCUMENT             (gedit_document_get_type ())
#define GEDIT_DOCUMENT(obj)		(GTK_CHECK_CAST ((obj), GEDIT_TYPE_DOCUMENT, GeditDocument))
#define GEDIT_DOCUMENT_CLASS(klass)	(GTK_CHECK_CLASS_CAST ((klass), GEDIT_TYPE_DOCUMENT, GeditDocumentClass))
#define GEDIT_IS_DOCUMENT(obj)		(GTK_CHECK_TYPE ((obj), GEDIT_TYPE_DOCUMENT))
#define GEDIT_IS_DOCUMENT_CLASS(klass)  (GTK_CHECK_CLASS_TYPE ((klass), GEDIT_TYPE_DOCUMENT))
#define GEDIT_DOCUMENT_GET_CLASS(obj)  	(GTK_CHECK_GET_CLASS ((obj), GEDIT_TYPE_DOCUMENT, GeditDocumentClass))


#define GEDIT_SEARCH_ENTIRE_WORD   	(1 << 0 )
#define GEDIT_SEARCH_BACKWARDS     	(1 << 1 )
#define GEDIT_SEARCH_CASE_SENSITIVE	(1 << 2 )
#define GEDIT_SEARCH_FROM_CURSOR   	(1 << 3 )

#define GEDIT_SEARCH_IS_ENTIRE_WORD(sflags) ((sflags & GEDIT_SEARCH_ENTIRE_WORD) != 0)
#define GEDIT_SEARCH_SET_ENTIRE_WORD(sflags,state) ((state == TRUE) ? \
(sflags |= GEDIT_SEARCH_ENTIRE_WORD) : (sflags &= ~GEDIT_SEARCH_ENTIRE_WORD))

#define GEDIT_SEARCH_IS_BACKWARDS(sflags) ((sflags & GEDIT_SEARCH_BACKWARDS) != 0)
#define GEDIT_SEARCH_SET_BACKWARDS(sflags,state) ((state == TRUE) ? \
(sflags |= GEDIT_SEARCH_BACKWARDS) : (sflags &= ~GEDIT_SEARCH_BACKWARDS))

#define GEDIT_SEARCH_IS_CASE_SENSITIVE(sflags) ((sflags &  GEDIT_SEARCH_CASE_SENSITIVE) != 0)
#define GEDIT_SEARCH_SET_CASE_SENSITIVE(sflags,state) ((state == TRUE) ? \
(sflags |= GEDIT_SEARCH_CASE_SENSITIVE) : (sflags &= ~GEDIT_SEARCH_CASE_SENSITIVE))

#define GEDIT_SEARCH_IS_FROM_CURSOR(sflags) ((sflags &  GEDIT_SEARCH_FROM_CURSOR) != 0)
#define GEDIT_SEARCH_SET_FROM_CURSOR(sflags,state) ((state == TRUE) ? \
(sflags |= GEDIT_SEARCH_FROM_CURSOR) : (sflags &= ~GEDIT_SEARCH_FROM_CURSOR))


typedef struct _GeditDocument           GeditDocument;
typedef struct _GeditDocumentClass 	GeditDocumentClass;

typedef struct _GeditDocumentPrivate    GeditDocumentPrivate;

struct _GeditDocument
{
	GtkSourceBuffer buffer;
	GeditDocumentPrivate *priv;
};

struct _GeditDocumentClass
{
	GtkSourceBufferClass parent_class;

	/* File name (uri) changed */
	void (* name_changed)		(GeditDocument *document);

	/* Document loaded */
	void (* loaded)			(GeditDocument *document);

	/* Document saved */
	void (* saved)  		(GeditDocument *document);	

	/* Readonly flag changed */
	void (* readonly_changed)	(GeditDocument *document,
					 gboolean readonly);

	/* Find state udpated */
	void (* can_find_again) 	(GeditDocument *document);
};

#define GEDIT_ERROR_INVALID_UTF8_DATA 	1024
#define GEDIT_ERROR_UNTITLED		1025	
#define GEDIT_DOCUMENT_IO_ERROR gedit_document_io_error_quark ()

GQuark 		gedit_document_io_error_quark (void);

GType        	gedit_document_get_type      	(void) G_GNUC_CONST;

GeditDocument*	gedit_document_new 		(void);
GeditDocument* 	gedit_document_new_with_uri 	(const gchar          *uri, 
						 const GeditEncoding  *encoding,
						 GError              **error);

void 		gedit_document_set_readonly 	(GeditDocument        *doc, 
						 gboolean              readonly);
gboolean	gedit_document_is_readonly 	(GeditDocument        *doc);

gchar*		gedit_document_get_raw_uri 	(const GeditDocument *doc);
gchar*		gedit_document_get_uri 		(const GeditDocument *doc);
gchar*		gedit_document_get_short_name 	(const GeditDocument *doc);

gboolean	gedit_document_load 		(GeditDocument        *doc, 
						 const gchar          *uri, 
						 const GeditEncoding  *encoding,
						 GError              **error);

gboolean	gedit_document_load_from_stdin 	(GeditDocument        *doc, 
						 GError              **error);

gboolean 	gedit_document_is_untouched 	(const GeditDocument *doc);

gboolean	gedit_document_save 		(GeditDocument        *doc, 
						 GError              **error);
gboolean	gedit_document_save_as 		(GeditDocument        *doc,	
						 const gchar          *uri, 
						 const GeditEncoding  *encoding,
						 GError              **error);

gboolean	gedit_document_save_a_copy_as 	(GeditDocument        *doc,
						 const gchar          *uri, 
						 const GeditEncoding  *encoding,
						 GError              **error);

gboolean 	gedit_document_revert 		(GeditDocument *doc,  GError **error);

gboolean	gedit_document_is_untitled 	(const GeditDocument* doc);
gboolean	gedit_document_get_modified 	(const GeditDocument* doc);

gint		gedit_document_get_char_count	(const GeditDocument *doc);
gint		gedit_document_get_line_count 	(const GeditDocument *doc);
		
void		gedit_document_insert_text	(GeditDocument *doc, 
						 gint	        pos,
                                                 const gchar   *text,
                                                 gint           len);

void		gedit_document_insert_text_at_cursor (GeditDocument *doc, 
                                                 const gchar   *text,
                                                 gint           len);

gint		gedit_document_get_cursor (GeditDocument *doc);
void		gedit_document_set_cursor (GeditDocument *doc, gint cursor);

void 		gedit_document_delete_text 	(GeditDocument *doc, 
		                                 gint start, gint end);

gchar*		gedit_document_get_chars 	(GeditDocument *doc, 
					         gint start, gint end);

gboolean	gedit_document_get_selection 	(GeditDocument *doc,
						 gint* start, gint* end);

/* Multi-level Undo/Redo operations */
void		gedit_document_set_max_undo_levels (GeditDocument *doc, 
						    gint max_undo_levels);

gboolean	gedit_document_can_undo		(const GeditDocument *doc);
gboolean	gedit_document_can_redo 	(const GeditDocument *doc);
gboolean	gedit_document_can_find_again	(const GeditDocument *doc);

void		gedit_document_undo 		(GeditDocument *doc);
void		gedit_document_redo 		(GeditDocument *doc);

void		gedit_document_begin_not_undoable_action 	(GeditDocument *doc);
void		gedit_document_end_not_undoable_action		(GeditDocument *doc);

void		gedit_document_begin_user_action (GeditDocument *doc);
void		gedit_document_end_user_action	 (GeditDocument *doc);

void		gedit_document_goto_line 	(GeditDocument* doc, guint line);

gchar* 		gedit_document_get_last_searched_text (GeditDocument* doc);
gchar* 		gedit_document_get_last_replace_text  (GeditDocument* doc);

void 		gedit_document_set_last_searched_text (GeditDocument* doc, const gchar *text);
void 		gedit_document_set_last_replace_text  (GeditDocument* doc, const gchar *text);

gboolean	gedit_document_find 		(GeditDocument* doc, const gchar* str, 
						 gint flags);
gboolean	gedit_document_find_next	(GeditDocument* doc, gint flags);
gboolean	gedit_document_find_prev	(GeditDocument* doc, gint flags);

void		gedit_document_replace_selected_text (GeditDocument *doc, 
						      const gchar *replace);
gboolean	gedit_document_replace_all 	(GeditDocument *doc,
				            	 const gchar *find, 
						 const gchar *replace, 
					    	 gint flags);
guint		gedit_document_get_line_at_offset (const GeditDocument *doc, guint offset);

void		gedit_document_set_selection 	(GeditDocument *doc, 
						 gint start, 
						 gint end);

void 		gedit_document_set_language 	(GeditDocument *doc,
						 GtkSourceLanguage *lang);
GtkSourceLanguage *gedit_document_get_language 	(GeditDocument *doc);

const GeditEncoding *gedit_document_get_encoding (GeditDocument *doc);

glong		gedit_document_get_seconds_since_last_save_or_load 
						(GeditDocument *doc);

#endif /* __GEDIT_DOCUMENT_H__ */



