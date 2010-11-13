/*
 * gedit-theatrics-choreographer.h
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

#ifndef __GEDIT_THEATRICS_CHOREOGRAPHER_H__
#define __GEDIT_THEATRICS_CHOREOGRAPHER_H__

#include <glib.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef enum
{
	GEDIT_THEATRICS_ANIMATION_STATE_COMING,
	GEDIT_THEATRICS_ANIMATION_STATE_IDLE,
	GEDIT_THEATRICS_ANIMATION_STATE_INTENDING_TO_GO,
	GEDIT_THEATRICS_ANIMATION_STATE_GOING
} GeditTheatricsAnimationState;

typedef enum
{
	GEDIT_THEATRICS_CHOREOGRAPHER_BLOCKING_UPSTAGE,
	GEDIT_THEATRICS_CHOREOGRAPHER_BLOCKING_DOWNSTAGE
} GeditTheatricsChoreographerBlocking;

typedef enum
{
	GEDIT_THEATRICS_CHOREOGRAPHER_EASING_LINEAR,
	GEDIT_THEATRICS_CHOREOGRAPHER_EASING_QUADRATIC_IN,
	GEDIT_THEATRICS_CHOREOGRAPHER_EASING_QUADRATIC_OUT,
	GEDIT_THEATRICS_CHOREOGRAPHER_EASING_QUADRATIC_IN_OUT,
	GEDIT_THEATRICS_CHOREOGRAPHER_EASING_EXPONENTIAL_IN,
	GEDIT_THEATRICS_CHOREOGRAPHER_EASING_EXPONENTIAL_OUT,
	GEDIT_THEATRICS_CHOREOGRAPHER_EASING_EXPONENTIAL_IN_OUT,
	GEDIT_THEATRICS_CHOREOGRAPHER_EASING_SINE
} GeditTheatricsChoreographerEasing;

gint	gedit_theatrics_choreographer_pixel_compose		(gdouble                           percent,
								 gint                              size,
								 GeditTheatricsChoreographerEasing easing);

gdouble	gedit_theatrics_choreographer_compose_with_scale	(gdouble                           percent,
								 gdouble                           scale,
								 GeditTheatricsChoreographerEasing easing);

gdouble	gedit_theatrics_choreographer_compose			(gdouble                           percent,
								 GeditTheatricsChoreographerEasing easing);

G_END_DECLS

#endif /* __GEDIT_THEATRICS_CHOREOGRAPHER_H__ */

/* ex:set ts=8 noet: */
