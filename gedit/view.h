/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit
 *
 * Copyright (C) 1998, 1999 Alex Roberts, Evan Lawrence
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __VIEW_H__
#define __VIEW_H__

#include "document.h"

#define VIEW(obj)		GTK_CHECK_CAST (obj, gedit_view_get_type (), View)
#define VIEW_CLASS(klass)	GTK_CHECK_CLASS_CAST (klass, gedit_view_get_type (), ViewClass)
#define IS_VIEW(obj)		GTK_CHECK_TYPE (obj, gedit_view_get_type ())

typedef struct _View	  View;
typedef struct _ViewClass ViewClass;

struct _View
{
	/*GtkFixed fixed;*/
	GtkVBox box;
	
	Document *document;
	
	gchar *font;
	
	GtkWidget *vbox;
	
	GtkWidget *text;
	GtkWidget *viewport;
	GtkWidget *pane;
	GtkWidget *scrwindow[2];
	
	gint split;
	gint splitscreen;
	GtkWidget *split_parent;
	GtkWidget *split_viewport;
#ifdef ENABLE_SPLIT_SCREEN	
	GtkWidget *split_screen;
#endif	
	
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

struct _ViewClass
{
	GtkVBoxClass parent_class;
	
	void (*cursor_moved)(View *view);
};


/* General utils */
guint	   gedit_view_get_type 		(void);
GtkWidget* gedit_view_new 		(Document *doc);

/* View settings */
void 	   gedit_view_set_font 		(View *view, gchar *font);
void 	   gedit_view_set_word_wrap 	(View *view, gint word_wrap);
void 	   gedit_view_set_read_only 	(View *view, gint read_only);
void 	   gedit_view_set_split_screen 	(View *view, gint split_screen);

/* Should we have the GtkText fucntions? */

/* hmm.. we are basically reimplenting a text widget for the
 * needs of gedit.. so.. any util functions that could be used
 * should be implemented.. or atleast prototyped in case.. we can
 * always remove them later.. */

/* This is a function to insert text into the buffer, used for the GList of views 
   in a gedit_document */
void 	   gedit_view_insert_text 	(View *view, const gchar *text,
					 gint length, gint pos);

guint 	   gedit_view_get_position	(View *view);
void	   gedit_view_set_position	(View *view, gint pos);
guint 	   gedit_view_get_length 	(View *view);

void	   gedit_view_buffer_sync	(View *view);

void 	   view_changed_cb		(GtkWidget *w, gpointer cbdata);

void	   gedit_view_set_group_type	(View *view, guint type);

void	   gedit_view_refresh		(View *view);

/*void gedit_view_set_color (View *view , teh Gdk colour thngies we need for a
								func like this..  ); */

/* At some point in the future we will have an Un/Re-do feature */
/*
void gedit_view_undo (View *view);
void gedit_view_redo (View *view);
*/

extern void options_toggle_line_wrap_cb (GtkWidget *widget, gpointer data);

#if 0
void	   gedit_view_set_selection	(View *view, gint start, gint end);
#endif


void doc_delete_text_cb (GtkWidget *editable, int start_pos, int end_pos, View *view);
void doc_insert_text_cb (GtkWidget *editable, const guchar *insertion_text, int length, int *pos, View *view);

#endif /* __VIEW_H__ */
