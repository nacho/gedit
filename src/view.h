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

typedef struct _GeditToolbar GeditToolbar;

struct _GeditToolbar
{
	/*
	GtkWidget *new_button;
	GtkWidget *open_button;
	GtkWidget *save_button;
	GtkWidget *close_button;
	GtkWidget *print_button;
	*/
	GtkWidget *undo_button;
	GtkWidget *redo_button;
	/*
	GtkWidget *cut_button;
	GtkWidget *copy_button;
	GtkWidget *paste_button;
	GtkWidget *find_button;
	GtkWidget *info_button;
	GtkWidget *exit_button;
	*/

	/*
	gint new : 1;
	gint open : 1;
	gint save : 1;
	gint close : 1;
	gint print : 1;
	*/
	gint undo : 1;
	gint redo : 1;
	/*
	gint cut : 1;
	gint copy : 1;
	gint paste : 1;
	gint find : 1;
	gint info : 1;
	gint exit : 1;
	*/
};

struct _View
{
	GnomeApp *gnome_app;

	GtkVBox box;
	
	Document *doc;
	
	gchar *font;
	
	GtkWidget *vbox;
	GtkWidget *text;
	GtkWidget *viewport;
	GtkWidget *pane;
	GtkWidget *window;

	gint view_text_changed_signal;
	
#ifdef ENABLE_SPLIT_SCREEN	
	gint split;
	gint splitscreen;
	GtkWidget *split_parent;
	GtkWidget *split_viewport;
	GtkWidget *split_screen;
#endif	
	guint changed : 1;
	guint word_wrap : 1;
	guint line_wrap : 1;
	guint readonly : 1;

	/* We need to have different toolbars
	   since mdi_mode =TOP_LEVEL will have multiple
	   toolbars. In most cases the widgets pointed by the
	   different views will be the same. */
	GeditToolbar *toolbar;
};

struct _ViewClass
{
	GtkVBoxClass parent_class;
	void (*cursor_moved)(View *view);
};

/* callback */
void	gedit_view_text_changed_cb (GtkWidget *w, gpointer cbdata);
void	gedit_view_changed_cb (GnomeMDI *mdi, GtkWidget *old_view);
void	gedit_view_add_cb (GtkWidget *widget, gpointer data);
void	gedit_view_remove_cb (GtkWidget *widget, gpointer data);

/* General utils */
guint	   	gedit_view_get_type	(void);
GtkWidget*	gedit_view_new		(Document *doc);
View *		gedit_view_current	(void);

/* View settings */
void	gedit_view_set_font		(View *view, gchar *font);
void	gedit_view_set_word_wrap	(View *view, gint word_wrap);
void	gedit_view_set_readonly		(View *view, gint readonly);
void	gedit_view_set_split_screen	(View *view, gint split_screen);

/* Scrolled window */
gfloat	gedit_view_get_window_position	(View *view);
void	gedit_view_set_window_position	(View *view, gfloat position);
void	gedit_view_set_window_position_from_lines (View *view, guint line, guint lines);

/* Insert/delete text */
void	doc_delete_text_cb		(GtkWidget *editable, int start_pos, int end_pos, View *view, gint exclude_this_view, gint undo);
void	doc_insert_text_cb		(GtkWidget *editable, const guchar *insertion_text, int length, int *pos, View *view, gint exclude_this_view, gint undo);

/* selection and position */
void	gedit_view_set_selection	(View *view, guint start, guint end);
gint	gedit_view_get_selection	(View *view, guint *start, guint *end);
void	gedit_view_set_position		(View *view, gint pos);
guint	gedit_view_get_position		(View *view);

/* toolbar */
void	gedit_view_load_toolbar_widgets (View *view);
void	gedit_view_set_undo (View *view, gint undo_state, gint redo_state);


#endif /* __VIEW_H__ */
