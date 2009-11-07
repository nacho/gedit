/*
 * gedit-page.h
 * This file is part of gedit
 *
 * Copyright (C) 2009 - Ignacio Casal Quinteiro
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


#ifndef __GEDIT_PAGE_H__
#define __GEDIT_PAGE_H__

#include <gtk/gtk.h>
#include <gedit/gedit-view-container.h>

G_BEGIN_DECLS

#define GEDIT_TYPE_PAGE			(gedit_page_get_type ())
#define GEDIT_PAGE(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_PAGE, GeditPage))
#define GEDIT_PAGE_CONST(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_PAGE, GeditPage const))
#define GEDIT_PAGE_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), GEDIT_TYPE_PAGE, GeditPageClass))
#define GEDIT_IS_PAGE(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEDIT_TYPE_PAGE))
#define GEDIT_IS_PAGE_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEDIT_TYPE_PAGE))
#define GEDIT_PAGE_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GEDIT_TYPE_PAGE, GeditPageClass))

typedef struct _GeditPage		GeditPage;
typedef struct _GeditPageClass		GeditPageClass;
typedef struct _GeditPagePrivate	GeditPagePrivate;

struct _GeditPage
{
	GtkVBox parent;
	
	GeditPagePrivate *priv;
};

struct _GeditPageClass
{
	GtkVBoxClass parent_class;
	
	void		(*container_added)	(GeditPage          *page,
						 GeditViewContainer *container);
	void		(*container_removed)	(GeditPage          *page,
						 GeditViewContainer *container);
	void		(*active_container_changed)
						(GeditPage          *page,
						 GeditViewContainer *container);
};

GType			 gedit_page_get_type			(void) G_GNUC_CONST;

GtkWidget		*_gedit_page_new			(GeditViewContainer *container);

GeditViewContainer	*gedit_page_get_active_view_container	(GeditPage *page);

GList			*gedit_page_get_view_containers		(GeditPage *page);

GeditPage		*gedit_page_get_from_container		(GeditViewContainer *container);

void			_gedit_page_switch_between_splits	(GeditPage *page);

void			_gedit_page_unsplit			(GeditPage *page);

gboolean		_gedit_page_can_unsplit			(GeditPage *page);

gboolean		_gedit_page_can_split			(GeditPage *page);

void			_gedit_page_split_horizontally		(GeditPage *page);

void			_gedit_page_split_vertically		(GeditPage *page);

G_END_DECLS

#endif /* __GEDIT_PAGE_H__ */
