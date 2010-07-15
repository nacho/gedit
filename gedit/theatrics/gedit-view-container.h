/*
 * gedit-view-container.h
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

#ifndef __GEDIT_VIEW_CONTAINER_H__
#define __GEDIT_VIEW_CONTAINER_H__

#include <glib-object.h>
#include <gtk/gtk.h>
#include "gedit-theatrics-choreographer.h"

G_BEGIN_DECLS

#define GEDIT_TYPE_VIEW_CONTAINER		(gedit_view_container_get_type ())
#define GEDIT_VIEW_CONTAINER(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_VIEW_CONTAINER, GeditViewContainer))
#define GEDIT_VIEW_CONTAINER_CONST(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_VIEW_CONTAINER, GeditViewContainer const))
#define GEDIT_VIEW_CONTAINER_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GEDIT_TYPE_VIEW_CONTAINER, GeditViewContainerClass))
#define GEDIT_IS_VIEW_CONTAINER(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEDIT_TYPE_VIEW_CONTAINER))
#define GEDIT_IS_VIEW_CONTAINER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEDIT_TYPE_VIEW_CONTAINER))
#define GEDIT_VIEW_CONTAINER_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GEDIT_TYPE_VIEW_CONTAINER, GeditViewContainerClass))

typedef struct _GeditViewContainer		GeditViewContainer;
typedef struct _GeditViewContainerClass		GeditViewContainerClass;
typedef struct _GeditViewContainerPrivate	GeditViewContainerPrivate;

struct _GeditViewContainer
{
	GtkContainer parent;

	GeditViewContainerPrivate *priv;
};

struct _GeditViewContainerClass
{
	GtkContainerClass parent_class;

	void (* set_scroll_adjustments)   (GeditViewContainer *container,
	                                   GtkAdjustment      *hadjustment,
	                                   GtkAdjustment      *vadjustment);
};

GType		 gedit_view_container_get_type			(void) G_GNUC_CONST;

GtkWidget	*gedit_view_container_new			(GtkWidget *main_widget);

void		 gedit_view_container_add_animated_widget	(GeditViewContainer                 *container,
								 GtkWidget                          *widget,
								 guint                               duration,
								 GeditTheatricsChoreographerEasing   easing,
								 GeditTheatricsChoreographerBlocking blocking,
								 GtkOrientation                      orientation,
								 gint                                x,
								 gint                                y);

void		 gedit_view_container_move_widget		(GeditViewContainer *container,
								 GtkWidget          *widget,
								 gint                x,
								 gint                y);

G_END_DECLS

#endif /* __GEDIT_VIEW_CONTAINER_H__ */
