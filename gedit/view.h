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

#ifndef __GEDIT_VIEW_H__
#define __GEDIT_VIEW_H__

#include "document.h"

#define GEDIT_VIEW(obj)		GTK_CHECK_CAST (obj, gedit_view_get_type (), GeditView)
#define GEDIT_VIEW_CLASS(klass)	GTK_CHECK_CLASS_CAST (klass, gedit_view_get_type (), GeditViewClass)
#define GEDIT_IS_VIEW(obj)	GTK_CHECK_TYPE (obj, gedit_view_get_type ())

typedef struct _GeditView	  GeditView;
typedef struct _GeditViewClass    GeditViewClass;

typedef struct _GeditToolbar GeditToolbar;

struct _GeditToolbar
{
	GtkWidget *undo_button;
	GtkWidget *redo_button;

	GtkWidget *undo_menu_item;
	GtkWidget *redo_menu_item;

	gboolean undo : 1;
	gboolean redo : 1;
};

struct _GeditView
{
	GtkVBox box;

	GtkText *text;
	GnomeApp *app;
	GeditDocument *doc;

	gint view_text_changed_signal;
	
	gboolean changed : 1;
	gboolean readonly : 1;

	/* We need to have different toolbars
	   since mdi_mode =TOP_LEVEL will have multiple
	   toolbars. In most cases the widgets pointed by the
	   different views will be the same. */
	GeditToolbar *toolbar;
};

/* callback */
void	gedit_view_text_changed_cb (GtkWidget *w, gpointer cbdata);
void	gedit_view_changed_cb (GnomeMDI *mdi, GtkWidget *old_view);
void	gedit_view_add_cb (GtkWidget *widget, gpointer data);
void	gedit_view_remove_cb (GtkWidget *widget, gpointer data);

void	gedit_view_remove (GeditView *view);

/* General utils */
guint	   	gedit_view_get_type	(void);
GtkWidget*	gedit_view_new		(GeditDocument *doc);
GeditView *	gedit_view_active	(void);

/* View settings */
void	gedit_view_set_font		(GeditView *view, gchar *font);
void	gedit_view_set_word_wrap	(GeditView *view, gint word_wrap);
void	gedit_view_set_readonly		(GeditView *view, gint readonly);
void	gedit_view_set_split_screen	(GeditView *view, gint split_screen);
void    gedit_view_set_tab_size         (GeditView *view, gint tab_size);

/* Scrolled window */
gfloat	gedit_view_get_window_position	(GeditView *view);
void	gedit_view_set_window_position	(GeditView *view, gfloat position);
void	gedit_view_set_window_position_from_lines (GeditView *view, guint line, guint lines);

/* Insert/delete text */
void	doc_delete_text_real_cb		(GtkWidget *editable, int start_pos, int end_pos, GeditView *view, gint exclude_this_view, gint undo);
/*void	doc_delete_text_cb		(GtkWidget *editable, int start_pos, int end_pos, View *view);*/
void	doc_insert_text_real_cb		(GtkWidget *editable, const guchar *insertion_text, int length, int *pos, GeditView *view, gint exclude_this_view, gint undo);
/*void	doc_insert_text_cb		(GtkWidget *editable, const guchar *insertion_text, int length, int *pos, View *view);*/


/* selection and position */
void	gedit_view_set_selection	(GeditView *view, guint start, guint end);
gint	gedit_view_get_selection	(GeditView *view, guint *start, guint *end);
void	gedit_view_set_position		(GeditView *view, gint pos);
guint	gedit_view_get_position		(GeditView *view);
GeditDocument * gedit_view_get_document (GeditView *view);

/* toolbar */
void	gedit_view_load_widgets (GeditView *view);
void	gedit_view_set_undo (GeditView *view, gint undo_state, gint redo_state);


#endif /* __GEDIT_VIEW_H__ */
