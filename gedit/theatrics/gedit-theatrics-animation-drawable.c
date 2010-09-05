/*
 * gedit-theatrics-animation-drawable.c
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

#include "gedit-theatrics-animation-drawable.h"

G_DEFINE_INTERFACE (GeditTheatricsAnimationDrawable, gedit_theatrics_animation_drawable, G_TYPE_OBJECT)

/* Default implementation */
static GdkRectangle
gedit_theatrics_animation_drawable_get_animation_bounds_default (GeditTheatricsAnimationDrawable *ad)
{
	GdkRectangle r = {0, };

	g_return_val_if_reached (r);
}

static gdouble
gedit_theatrics_animation_drawable_get_percent_default (GeditTheatricsAnimationDrawable *ad)
{
	g_return_val_if_reached (0.0);
}

static void
gedit_theatrics_animation_drawable_set_percent_default (GeditTheatricsAnimationDrawable *ad,
							gdouble                          percent)
{
	g_return_if_reached ();
}

static void
gedit_theatrics_animation_drawable_draw_default (GeditTheatricsAnimationDrawable *ad,
						 GdkDrawable                     *drawable)
{
	g_return_if_reached ();
}

static void 
gedit_theatrics_animation_drawable_default_init (GeditTheatricsAnimationDrawableInterface *iface)
{
	static gboolean initialized = FALSE;

	iface->get_animation_bounds = gedit_theatrics_animation_drawable_get_animation_bounds_default;
	iface->get_percent = gedit_theatrics_animation_drawable_get_percent_default;
	iface->set_percent = gedit_theatrics_animation_drawable_set_percent_default;
	iface->draw = gedit_theatrics_animation_drawable_draw_default;

	if (!initialized)
	{
		initialized = TRUE;
	}
}

GdkRectangle
gedit_theatrics_animation_drawable_get_animation_bounds (GeditTheatricsAnimationDrawable *ad)
{
	GdkRectangle r = {0, };

	g_return_val_if_fail (GEDIT_THEATRICS_ANIMATION_DRAWABLE (ad), r);
	return GEDIT_THEATRICS_ANIMATION_DRAWABLE_GET_INTERFACE (ad)->get_animation_bounds (ad);
}

gdouble
gedit_theatrics_animation_drawable_get_percent (GeditTheatricsAnimationDrawable *ad)
{
	g_return_val_if_fail (GEDIT_THEATRICS_ANIMATION_DRAWABLE (ad), 0.0);
	return GEDIT_THEATRICS_ANIMATION_DRAWABLE_GET_INTERFACE (ad)->get_percent (ad);
}

void
gedit_theatrics_animation_drawable_set_percent (GeditTheatricsAnimationDrawable *ad,
						gdouble                          percent)
{
	g_return_if_fail (GEDIT_THEATRICS_ANIMATION_DRAWABLE (ad));
	GEDIT_THEATRICS_ANIMATION_DRAWABLE_GET_INTERFACE (ad)->set_percent (ad, percent);
}

void
gedit_theatrics_animation_drawable_draw (GeditTheatricsAnimationDrawable *ad,
					 GdkDrawable                     *drawable)
{
	g_return_if_fail (GEDIT_THEATRICS_ANIMATION_DRAWABLE (ad));
	GEDIT_THEATRICS_ANIMATION_DRAWABLE_GET_INTERFACE (ad)->draw (ad, drawable);
}
