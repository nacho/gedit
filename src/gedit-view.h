/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-view.h
 * This file is part of gedit
 *
 * Copyright (C) 1998, 1999 Alex Roberts, Evan Lawrence
 * Copyright (C) 2000, 2001 Chema Celorio, Paolo Maggi 
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
 * Boston, MA 02111-1307, USA. * *
 */
 
/*
 * Modified by the gedit Team, 1998-2001. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 */

#ifndef __GEDIT_VIEW_H__
#define __GEDIT_VIEW_H__


#include <gtk/gtk.h>
#include "gedit-document.h"

#define GEDIT_TYPE_VIEW			(gedit_view_get_type ())
#define GEDIT_VIEW(obj)			(GTK_CHECK_CAST ((obj), GEDIT_TYPE_VIEW, GeditView))
#define GEDIT_VIEW_CLASS(klass)		(GTK_CHECK_CLASS_CAST ((klass), GEDIT_TYPE_VIEW, GeditViewClass))
#define GEDIT_IS_VIEW(obj)		(GTK_CHECK_TYPE ((obj), GEDIT_TYPE_VIEW))
#define GEDIT_IS_VIEW_CLASS(klass)  	(GTK_CHECK_CLASS_TYPE ((klass), GEDIT_TYPE_VIEW))
#define GEDIT_VIEW_GET_CLASS(obj)  	(GTK_CHECK_GET_CLASS ((obj), GEDIT_TYPE_VIEW, GeditViewClass))


typedef struct _GeditView		GeditView;
typedef struct _GeditViewClass		GeditViewClass;

typedef struct _GeditViewPrivate	GeditViewPrivate;

struct _GeditView
{
	GtkVBox box;
	
	GeditViewPrivate *priv;
};

struct _GeditViewClass
{
	GtkVBoxClass parent_class;
};


GtkType        	gedit_view_get_type     	(void) G_GNUC_CONST;

GeditView*	gedit_view_new			(GeditDocument *doc);

void 		gedit_view_cut_clipboard 	(GeditView *view);
void 		gedit_view_copy_clipboard 	(GeditView *view);
void 		gedit_view_paste_clipboard	(GeditView *view);
void 		gedit_view_delete_selection	(GeditView *view);
void 		gedit_view_select_all 		(GeditView *view);

void		gedit_view_scroll_to_cursor 	(GeditView *view);

GeditDocument*	gedit_view_get_document		(const GeditView *view);

void 		gedit_view_set_colors 		(GeditView* view, 
						 GdkColor* backgroud, GdkColor* text,
						 GdkColor* selection, GdkColor* sel_text);

void 		gedit_view_set_font		(GeditView* view,
						 const gchar* font_name);

void		gedit_view_set_wrap_mode 	(GeditView* view, GtkWrapMode wrap_mode);
void		gedit_view_set_tab_size  	(GeditView* view, gint tab_size);

void		gedit_view_show_line_numbers 	(GeditView* view, gboolean visible);

#endif /* __GEDIT_VIEW_H__ */

