/*
 * gedit-floating-slider.h
 * This file is part of gedit
 *
 * Copyright (C) 2010 - Ignacio Casal Quinteiro
 *
 * Based on Scott Peterson <lunchtimemama@gmail.com> work.
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

#ifndef __GEDIT_FLOATING_SLIDER_H__
#define __GEDIT_FLOATING_SLIDER_H__

#include <glib-object.h>
#include <gtk/gtk.h>
#include "gedit-theatrics-choreographer.h"
#include "gedit-overlay-child.h"

G_BEGIN_DECLS

#define GEDIT_TYPE_FLOATING_SLIDER		(gedit_floating_slider_get_type ())
#define GEDIT_FLOATING_SLIDER(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_FLOATING_SLIDER, GeditFloatingSlider))
#define GEDIT_FLOATING_SLIDER_CONST(obj)	(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_FLOATING_SLIDER, GeditFloatingSlider const))
#define GEDIT_FLOATING_SLIDER_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GEDIT_TYPE_FLOATING_SLIDER, GeditFloatingSliderClass))
#define GEDIT_IS_FLOATING_SLIDER(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEDIT_TYPE_FLOATING_SLIDER))
#define GEDIT_IS_FLOATING_SLIDER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEDIT_TYPE_FLOATING_SLIDER))
#define GEDIT_FLOATING_SLIDER_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GEDIT_TYPE_FLOATING_SLIDER, GeditFloatingSliderClass))

typedef struct _GeditFloatingSlider		GeditFloatingSlider;
typedef struct _GeditFloatingSliderClass	GeditFloatingSliderClass;
typedef struct _GeditFloatingSliderPrivate	GeditFloatingSliderPrivate;

struct _GeditFloatingSlider
{
	GeditOverlayChild parent;
	
	GeditFloatingSliderPrivate *priv;
};

struct _GeditFloatingSliderClass
{
	GeditOverlayChildClass parent_class;
};

GType			 gedit_floating_slider_get_type	(void) G_GNUC_CONST;

GeditFloatingSlider	*gedit_floating_slider_new		(GtkWidget                          *widget,
								 guint                               duration,
								 GeditTheatricsChoreographerEasing   easing,
								 GeditTheatricsChoreographerBlocking blocking,
								 GtkOrientation                      orientation);

G_END_DECLS

#endif /* __GEDIT_FLOATING_SLIDER_H__ */
