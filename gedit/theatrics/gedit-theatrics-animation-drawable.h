/*
 * gedit-theatrics-animation-drawable.h
 * This file is part of gedit
 *
 * Copyright (C) 2010 - Ignacio Casal Quinteiro
 *
 * Based on Mike Krüger <mkrueger@novell.com> work.
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

#ifndef __GEDIT_THEATRICS_ANIMATION_DRAWABLE_H__
#define __GEDIT_THEATRICS_ANIMATION_DRAWABLE_H__

#include <glib-object.h>
#include <gdk/gdk.h>

G_BEGIN_DECLS

#define GEDIT_TYPE_THEATRICS_ANIMATION_DRAWABLE			(gedit_theatrics_animation_drawable_get_type ())
#define GEDIT_THEATRICS_ANIMATION_DRAWABLE(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_THEATRICS_ANIMATION_DRAWABLE, GeditTheatricsAnimationDrawable))
#define GEDIT_IS_THEATRICS_ANIMATION_DRAWABLE(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEDIT_TYPE_THEATRICS_ANIMATION_DRAWABLE))
#define GEDIT_THEATRICS_ANIMATION_DRAWABLE_GET_INTERFACE(obj)	(G_TYPE_INSTANCE_GET_INTERFACE ((obj), GEDIT_TYPE_THEATRICS_ANIMATION_DRAWABLE, GeditTheatricsAnimationDrawableInterface))

typedef struct _GeditTheatricsAnimationDrawable			GeditTheatricsAnimationDrawable;
typedef struct _GeditTheatricsAnimationDrawableInterface	GeditTheatricsAnimationDrawableInterface;

struct _GeditTheatricsAnimationDrawableInterface
{
	GTypeInterface parent;

	GdkRectangle (* get_animation_bounds) (GeditTheatricsAnimationDrawable *ad);

	gdouble (* get_percent) (GeditTheatricsAnimationDrawable *ad);
	void (* set_percent) (GeditTheatricsAnimationDrawable *ad,
			      gdouble                          percent);

	void (* draw) (GeditTheatricsAnimationDrawable *ad,
	               GdkDrawable                     *drawable);
};

GType gedit_theatrics_animation_drawable_get_type (void) G_GNUC_CONST;

GdkRectangle gedit_theatrics_animation_drawable_get_animation_bounds (GeditTheatricsAnimationDrawable *ad);

gdouble gedit_theatrics_animation_drawable_get_percent (GeditTheatricsAnimationDrawable *ad);
void gedit_theatrics_animation_drawable_set_percent (GeditTheatricsAnimationDrawable *ad,
						     gdouble                          percent);

void gedit_theatrics_animation_drawable_draw (GeditTheatricsAnimationDrawable *ad,
					      GdkDrawable                     *drawable);

G_END_DECLS

#endif /* __GEDIT_THEATRICS_ANIMATION_DRAWABLE_H__ */
