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


#define GEDIT_TYPE_DOCUMENT             (gedit_document_get_type ())
#define GEDIT_DOCUMENT(obj)		(GTK_CHECK_CAST ((obj), GEDIT_TYPE_DOCUMENT, GeditDocument))
#define GEDIT_DOCUMENT_CLASS(klass)	(GTK_CHECK_CLASS_CAST ((klass), GEDIT_TYPE_DOCUMENT, GeditDocumentClass))
#define GEDIT_IS_DOCUMENT(obj)		(GTK_CHECK_TYPE ((obj), GEDIT_TYPE_DOCUMENT))
#define GEDIT_IS_DOCUMENT_CLASS(klass)  (GTK_CHECK_CLASS_TYPE ((klass), GEDIT_TYPE_DOCUMENT))
#define GEDIT_DOCUMENT_GET_CLASS(obj)  	(GTK_CHECK_GET_CLASS ((obj), GEDIT_TYPE_DOCUMENT, GeditDocumentClass))


typedef struct _GeditDocument           GeditDocument;
typedef struct _GeditDocumentClass 	GeditDocumentClass;

typedef struct _GeditDocumentPrivate    GeditDocumentPrivate;

struct _GeditDocument
{
	GtkTextBuffer buffer;
	
	GeditDocumentPrivate *priv;
};

struct _GeditDocumentClass
{
	GtkTextBufferClass parent_class;

	GtkTextTagTable *tag_table;

	/* File name (uri) changed */
	void (* name_changed)		(GeditDocument *document);

	/* Document loaded */
	void (* loaded)			(GeditDocument *document);

	/* Document saved */
	void (* saved)  		(GeditDocument *document);	

	/* Readonly flag changed */
	void (* readonly_changed)	(GeditDocument *document,
					 gboolean readonly);

	void (* can_undo)		(GeditDocument *document,
					 gboolean can_undo);
	void (* can_redo)		(GeditDocument *document,
					 gboolean can_redo);

};

#define GEDIT_DOCUMENT_IO_ERROR gedit_document_io_error_quark()
GQuark 		gedit_document_io_error_quark();

GType        	gedit_document_get_type      	(void) G_GNUC_CONST;

GeditDocument*	gedit_document_new 		(void);
GeditDocument* 	gedit_document_new_with_uri 	(const gchar *uri, GError **error);

void 		gedit_document_set_readonly 	(GeditDocument *doc, gboolean readonly);
gboolean	gedit_document_is_readonly 	(GeditDocument *doc);

gchar*		gedit_document_get_uri 		(const GeditDocument* doc);
gchar*		gedit_document_get_short_name 	(const GeditDocument* doc);

gboolean	gedit_document_load 		(GeditDocument* doc, 
						 const gchar *uri, GError **error);
gboolean	gedit_document_load_from_stdin 	(GeditDocument* doc, GError **error);

gboolean 	gedit_document_is_untouched 	(const GeditDocument *doc);

gboolean	gedit_document_save 		(GeditDocument* doc, GError **error);
gboolean	gedit_document_save_as 		(GeditDocument* doc, 
						 const gchar *uri, GError **error);

gboolean 	gedit_document_revert 		(GeditDocument *doc,  GError **error);

gboolean	gedit_document_is_untitled 	(const GeditDocument* doc);
gboolean	gedit_document_get_modified 	(const GeditDocument* doc);

gchar* 		gedit_document_get_buffer 	(const GeditDocument *doc);
gint		gedit_document_get_char_count	(const GeditDocument *doc);
gint		gedit_document_get_line_count 	(const GeditDocument *doc);
		
void 		gedit_document_delete_all_text 	(GeditDocument *doc);

void		gedit_document_insert_text	(GeditDocument *doc, 
						 gint	        pos,
                                                 const gchar   *text,
                                                 gint           len);

void		gedit_document_insert_text_at_cursor (GeditDocument *doc, 
                                                 const gchar   *text,
                                                 gint           len);

void 		gedit_document_delete_text 	(GeditDocument *doc, 
		                                 gint start, gint end);

gchar*		gedit_document_get_chars 	(GeditDocument *doc, 
					         gint start, gint end);

gchar*		gedit_document_get_selected_text (GeditDocument *doc,
						  gint* start, gint* end);

gboolean	gedit_document_has_selected_text (GeditDocument *doc);

/* Multi-level Undo/Redo operations */
gboolean	gedit_document_can_undo		(const GeditDocument *doc);
gboolean	gedit_document_can_redo 	(const GeditDocument *doc);

void		gedit_document_undo 		(GeditDocument *doc);
void		gedit_document_redo 		(GeditDocument *doc);

void		gedit_document_begin_not_undoable_action 	(GeditDocument *doc);
void		gedit_document_end_not_undoable_action		(GeditDocument *doc);

void		gedit_document_begin_user_action (GeditDocument *doc);
void		gedit_document_end_user_action	 (GeditDocument *doc);

void		gedit_document_goto_line 	(GeditDocument* doc, guint line);

gchar* 		gedit_document_get_last_searched_text (GeditDocument* doc);
gchar* 		gedit_document_get_last_replace_text  (GeditDocument* doc);

gboolean	gedit_document_find 		(GeditDocument* doc, const gchar* str, 
						 gboolean from_cursor, 
						 gboolean case_sensitive);
gboolean	gedit_document_find_again	(GeditDocument* doc);

void		gedit_document_replace_selected_text (GeditDocument *doc, 
						      const gchar *replace);
gboolean	gedit_document_replace_all (GeditDocument *doc,
				            const gchar *find, const gchar *replace, 
					    gboolean case_sensitive);
guint		gedit_document_get_line_at_offset (const GeditDocument *doc, guint offset);

#endif /* __GEDIT_DOCUMENT_H__ */



