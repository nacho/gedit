/* 	- New Document interface  
 *
 * gEdit
 * Copyright (C) 1999 Alex Roberts and Evan Lawrence
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __GE_VIEW_H__
#define __GE_VIEW_H__

#include "main.h"

#define GE_VIEW(obj)			GTK_CHECK_CAST (obj, gedit_view_get_type (), gE_view)
#define GE_VIEW_CLASS(klass)	GTK_CHECK_CLASS_CAST (klass, gedit_view_get_type (), gE_view_class)
#define GE_IS_VIEW(obj)			GTK_CHECK_TYPE (obj, gedit_view_get_type ())

typedef struct _gedit_view gE_view;
typedef struct _gedit_view_class gE_view_class;

struct _gedit_view {
	/*GtkFixed fixed;*/
	GtkVBox box;
	
	gedit_document *document;
	
	gchar *font;
	
	GtkWidget *vbox;
	
	GtkWidget *text;
	GtkWidget *viewport;
	GtkWidget *pane;
	GtkWidget *scrwindow[2];
	
	gint split, splitscreen;
	GtkWidget *split_parent;
	GtkWidget *split_viewport;
	GtkWidget *split_screen;
	
	gint changed_id;
	gint changed;
	
	/* GtkText Signal id's */
	gint insert, delete, indent;
	gint s_insert, s_delete, s_indent;
	
	guint group_type;
	
	gpointer flag;
	
	gint word_wrap;
	gint line_wrap;
	gint read_only;
	
	/* Temporary flags */
	gpointer temp1;
	gpointer temp2;
	
};


struct _gedit_view_class {
	/* dunno */
	GtkVBoxClass parent_class;
	
	void (*cursor_moved)(gedit_view *view);
	
};


/* Ok, i thought i might as well make at least ONE header look nice =) */

/* General utils */
guint 		gedit_view_get_type 			();
GtkWidget 	*gedit_view_new 				(gE_document *doc);

/* View settings */
void 		gedit_view_set_font 			(gE_view *view, gchar *font);
void 		gedit_view_set_word_wrap 		(gE_view *view, gint word_wrap);
void 		gedit_view_set_line_wrap 		(gE_view *view, gint line_wrap);
void 		gedit_view_set_read_only 		(gE_view *view, gint read_only);
void 		gedit_view_set_split_screen 	(gE_view *view, gint split_screen);

/* Should we have teh GtkText fucntions? */

/* hmm.. we are basically reimplenting a text widget for the
 * needs of gEdit.. so.. any util functions that could be used
 * should be implemented.. or atleast prototyped in case.. we can
 * always remove them later.. */

/* This is a function to insert text into the buffer, used for the GList of views 
   in a gedit_document */
void		gedit_view_list_insert			(gE_view *view, gE_data *data);
 
void 		gedit_view_insert_text 		(gE_view *view, const gchar *text,
									 gint length, gint pos);

void		gedit_view_set_selection		(gE_view *view, gint start, gint end);

guint 		gedit_view_get_position		(gE_view *view);
void		gedit_view_set_position		(gE_view *view, gint pos);
guint 		gedit_view_get_length 			(gE_view *view);

void		gedit_view_buffer_sync			(gE_view *view);

void 		view_changed_cb				(GtkWidget *w, gpointer cbdata);

void		gedit_view_set_group_type		(gE_view *view, guint type);

void		gedit_view_refresh				(gE_view *view);

/*void gedit_view_set_color (gE_view *view , teh Gdk colour thngies we need for a
								func like this..  ); */

/* At some point in the future we will have an Un/Re-do feature */
/*
void gedit_view_undo (gE_view *view);
void gedit_view_redo (gE_view *view);
*/

#endif /* __GE_VIEW_H__ */
