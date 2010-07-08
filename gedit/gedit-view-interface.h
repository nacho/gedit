/*
 * gedit-view.h
 * This file is part of gedit
 *
 * Copyright (C) 2010 - Ignacio Casal Quinteiro
 *
 * gedit is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * gedit is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with gedit; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */


#ifndef __GEDIT_VIEW_H__
#define __GEDIT_VIEW_H__

#include <glib-object.h>
#include "gedit-document.h"

G_BEGIN_DECLS

#define GEDIT_TYPE_VIEW			(gedit_view_get_type ())
#define GEDIT_VIEW(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_VIEW, GeditView))
#define GEDIT_IS_VIEW(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEDIT_TYPE_VIEW))
#define GEDIT_VIEW_GET_INTERFACE(obj)	(G_TYPE_INSTANCE_GET_INTERFACE ((obj), GEDIT_TYPE_VIEW, GeditViewIface))

typedef struct _GeditView	GeditView;
typedef struct _GeditViewIface	GeditViewIface;

struct _GeditViewIface
{
	GTypeInterface parent;
	
	/* Signals */
	void			(* drop_uris)		(GeditView *view,
							 gchar    **uri_list);
	
	/* Virtual Table */
	const gchar *		(*get_name)		(GeditView *view);
	GeditDocument *		(*get_document)		(GeditView *view);
	void			(*set_editable)		(GeditView *view,
							 gboolean   setting);
	gboolean		(*get_editable)		(GeditView *view);
	gboolean		(*get_overwrite)	(GeditView *view);
	void			(*cut_clipboard)	(GeditView *view);
	void			(*copy_clipboard)	(GeditView *view);
	void			(*paste_clipboard)	(GeditView *view);
	void			(*delete_selection)	(GeditView *view);
	void			(*select_all)		(GeditView *view);
	void			(*scroll_to_cursor)	(GeditView *view);
	void			(*set_font)		(GeditView *view,
							 gboolean     def,
							 const gchar *font_name);
};

GType			 gedit_view_get_type		(void) G_GNUC_CONST;

const gchar		*gedit_view_get_name		(GeditView   *view);

GeditDocument		*gedit_view_get_document	(GeditView   *view);

void			 gedit_view_set_editable	(GeditView   *view,
							 gboolean     setting);

gboolean		 gedit_view_get_editable	(GeditView   *view);

gboolean		 gedit_view_get_overwrite	(GeditView   *view);

void			 gedit_view_cut_clipboard	(GeditView   *view);

void			 gedit_view_copy_clipboard	(GeditView   *view);

void			 gedit_view_paste_clipboard	(GeditView   *view);

void			 gedit_view_delete_selection	(GeditView   *view);

void			 gedit_view_select_all		(GeditView   *view);

void			 gedit_view_scroll_to_cursor	(GeditView   *view);

void			 gedit_view_set_font		(GeditView   *view,
							 gboolean     def,
							 const gchar *font_name);

G_END_DECLS

#endif /* __GEDIT_VIEW_H__ */
