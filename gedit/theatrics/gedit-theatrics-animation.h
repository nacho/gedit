/*
 * gedit-theatrics-animation.h
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with gedit; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 */

#ifndef __GEDIT_THEATRICS_ANIMATION_H__
#define __GEDIT_THEATRICS_ANIMATION_H__

#include <glib-object.h>
#include "gedit-theatrics-animation-drawable.h"
#include "gedit-theatrics-choreographer.h"

G_BEGIN_DECLS

#define GEDIT_TYPE_THEATRICS_ANIMATION			(gedit_theatrics_animation_get_type ())
#define GEDIT_THEATRICS_ANIMATION(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_THEATRICS_ANIMATION, GeditTheatricsAnimation))
#define GEDIT_THEATRICS_ANIMATION_CONST(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_THEATRICS_ANIMATION, GeditTheatricsAnimation const))
#define GEDIT_THEATRICS_ANIMATION_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), GEDIT_TYPE_THEATRICS_ANIMATION, GeditTheatricsAnimationClass))
#define GEDIT_IS_THEATRICS_ANIMATION(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEDIT_TYPE_THEATRICS_ANIMATION))
#define GEDIT_IS_THEATRICS_ANIMATION_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEDIT_TYPE_THEATRICS_ANIMATION))
#define GEDIT_THEATRICS_ANIMATION_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GEDIT_TYPE_THEATRICS_ANIMATION, GeditTheatricsAnimationClass))

typedef struct _GeditTheatricsAnimation		GeditTheatricsAnimation;
typedef struct _GeditTheatricsAnimationClass	GeditTheatricsAnimationClass;
typedef struct _GeditTheatricsAnimationPrivate	GeditTheatricsAnimationPrivate;

struct _GeditTheatricsAnimation
{
	GObject parent;

	GeditTheatricsAnimationPrivate *priv;
};

struct _GeditTheatricsAnimationClass
{
	GObjectClass parent_class;
};

GType			 gedit_theatrics_animation_get_type		(void) G_GNUC_CONST;

GeditTheatricsAnimation *gedit_theatrics_animation_new			(GeditTheatricsAnimationDrawable    *drawable,
									 guint                               duration,
									 GeditTheatricsChoreographerEasing   easing,
									 GeditTheatricsChoreographerBlocking blocking);

GeditTheatricsAnimationDrawable *
			gedit_theatrics_animation_get_drawable		(GeditTheatricsAnimation *anim);

void			gedit_theatrics_animation_set_drawable		(GeditTheatricsAnimation         *anim,
									 GeditTheatricsAnimationDrawable *drawable);

GeditTheatricsChoreographerEasing
			gedit_theatrics_animation_get_easing		(GeditTheatricsAnimation *anim);

void			gedit_theatrics_animation_set_easing		(GeditTheatricsAnimation          *anim,
									 GeditTheatricsChoreographerEasing easing);

GeditTheatricsChoreographerBlocking
			gedit_theatrics_animation_get_blocking		(GeditTheatricsAnimation *anim);

void			gedit_theatrics_animation_set_blocking		(GeditTheatricsAnimation            *anim,
									 GeditTheatricsChoreographerBlocking blocking);

GeditTheatricsAnimationState
			gedit_theatrics_animation_get_animation_state	(GeditTheatricsAnimation *anim);

void			gedit_theatrics_animation_set_animation_state	(GeditTheatricsAnimation     *anim,
									 GeditTheatricsAnimationState animation_state);

gdouble			gedit_theatrics_animation_get_duration		(GeditTheatricsAnimation *anim);

void			gedit_theatrics_animation_set_duration		(GeditTheatricsAnimation *anim,
									 gdouble                  duration);

G_END_DECLS

#endif /* __GEDIT_THEATRICS_ANIMATION_H__ */

/* ex:ts=8:noet: */
