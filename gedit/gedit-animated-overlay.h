/*
 * gedit-animated-overlay.h
 * This file is part of gedit
 *
 * Copyright (C) 2011 - Ignacio Casal Quinteiro
 *
 * gedit is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * gedit is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef __GEDIT_ANIMATED_OVERLAY_H__
#define __GEDIT_ANIMATED_OVERLAY_H__

#include "gedit-animatable.h"
#include "gedit-overlay.h"
#include "theatrics/gedit-theatrics-choreographer.h"

G_BEGIN_DECLS

#define GEDIT_TYPE_ANIMATED_OVERLAY		(gedit_animated_overlay_get_type ())
#define GEDIT_ANIMATED_OVERLAY(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_ANIMATED_OVERLAY, GeditAnimatedOverlay))
#define GEDIT_ANIMATED_OVERLAY_CONST(obj)	(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_ANIMATED_OVERLAY, GeditAnimatedOverlay const))
#define GEDIT_ANIMATED_OVERLAY_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GEDIT_TYPE_ANIMATED_OVERLAY, GeditAnimatedOverlayClass))
#define GEDIT_IS_ANIMATED_OVERLAY(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEDIT_TYPE_ANIMATED_OVERLAY))
#define GEDIT_IS_ANIMATED_OVERLAY_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEDIT_TYPE_ANIMATED_OVERLAY))
#define GEDIT_ANIMATED_OVERLAY_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GEDIT_TYPE_ANIMATED_OVERLAY, GeditAnimatedOverlayClass))

typedef struct _GeditAnimatedOverlay		GeditAnimatedOverlay;
typedef struct _GeditAnimatedOverlayClass	GeditAnimatedOverlayClass;
typedef struct _GeditAnimatedOverlayPrivate	GeditAnimatedOverlayPrivate;

struct _GeditAnimatedOverlay
{
	GeditOverlay parent;
	
	GeditAnimatedOverlayPrivate *priv;
};

struct _GeditAnimatedOverlayClass
{
	GeditOverlayClass parent_class;
};

GType gedit_animated_overlay_get_type (void) G_GNUC_CONST;

GtkWidget *gedit_animated_overlay_new (GtkWidget *main_widget,
                                       GtkWidget *relative_widget);

void gedit_animated_overlay_add (GeditAnimatedOverlay *overlay,
                                 GeditAnimatable      *animatable);

G_END_DECLS

#endif /* __GEDIT_ANIMATED_OVERLAY_H__ */
