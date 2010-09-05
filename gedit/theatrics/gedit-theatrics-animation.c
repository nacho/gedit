/*
 * gedit-theatrics-animation.c
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

#include "gedit-theatrics-animation.h"
#include "gedit-theatrics-enum-types.h"


#define GEDIT_THEATRICS_ANIMATION_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE((object), GEDIT_TYPE_THEATRICS_ANIMATION, GeditTheatricsAnimationPrivate))

struct _GeditTheatricsAnimationPrivate
{
	GeditTheatricsAnimationDrawable *drawable;
	GeditTheatricsChoreographerEasing easing;
	GeditTheatricsChoreographerBlocking blocking;
	GeditTheatricsAnimationState animation_state;
	guint duration;
};

enum
{
	PROP_0,
	PROP_DRAWABLE,
	PROP_EASING,
	PROP_BLOCKING,
	PROP_ANIMATION_STATE,
	PROP_DURATION
};

G_DEFINE_TYPE (GeditTheatricsAnimation, gedit_theatrics_animation, G_TYPE_OBJECT)

static void
gedit_theatrics_animation_finalize (GObject *object)
{
	G_OBJECT_CLASS (gedit_theatrics_animation_parent_class)->finalize (object);
}

static void
gedit_theatrics_animation_get_property (GObject    *object,
					guint       prop_id,
					GValue     *value,
					GParamSpec *pspec)
{
	GeditTheatricsAnimation *anim = GEDIT_THEATRICS_ANIMATION (object);

	switch (prop_id)
	{
		case PROP_DRAWABLE:
			g_value_set_object (value, anim->priv->drawable);
			break;
		case PROP_EASING:
			g_value_set_enum (value, anim->priv->easing);
			break;
		case PROP_BLOCKING:
			g_value_set_enum (value, anim->priv->blocking);
			break;
		case PROP_ANIMATION_STATE:
			g_value_set_enum (value, anim->priv->animation_state);
			break;
		case PROP_DURATION:
			g_value_set_uint (value, anim->priv->duration);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_theatrics_animation_set_property (GObject      *object,
					guint         prop_id,
					const GValue *value,
					GParamSpec   *pspec)
{
	GeditTheatricsAnimation *anim = GEDIT_THEATRICS_ANIMATION (object);

	switch (prop_id)
	{
		case PROP_DRAWABLE:
		{
			GeditTheatricsAnimationDrawable *ad;

			ad = g_value_get_object (value);

			if (ad != NULL)
			{
				gedit_theatrics_animation_set_drawable (anim, ad);
			}

			break;
		}
		case PROP_EASING:
			gedit_theatrics_animation_set_easing (anim,
			                                      g_value_get_enum (value));
			break;
		case PROP_BLOCKING:
			gedit_theatrics_animation_set_blocking (anim,
			                                        g_value_get_enum (value));
			break;
		case PROP_ANIMATION_STATE:
			gedit_theatrics_animation_set_easing (anim,
			                                      g_value_get_enum (value));
			break;
		case PROP_DURATION:
			gedit_theatrics_animation_set_duration (anim,
			                                        g_value_get_uint (value));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_theatrics_animation_class_init (GeditTheatricsAnimationClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = gedit_theatrics_animation_finalize;
	object_class->get_property = gedit_theatrics_animation_get_property;
	object_class->set_property = gedit_theatrics_animation_set_property;

	g_object_class_install_property (object_class, PROP_DRAWABLE,
	                                 g_param_spec_object ("drawable",
	                                                      "Drawable",
	                                                      "The Drawable",
	                                                      GEDIT_TYPE_THEATRICS_ANIMATION_DRAWABLE,
	                                                      G_PARAM_READWRITE |
	                                                      G_PARAM_CONSTRUCT |
	                                                      G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (object_class, PROP_EASING,
	                                 g_param_spec_enum ("easing",
	                                                    "Easing",
	                                                    "The Easing",
	                                                    GEDIT_TYPE_THEATRICS_CHOREOGRAPHER_EASING,
	                                                    GEDIT_THEATRICS_CHOREOGRAPHER_EASING_LINEAR,
	                                                    G_PARAM_READWRITE |
	                                                    G_PARAM_CONSTRUCT |
	                                                    G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (object_class, PROP_BLOCKING,
	                                 g_param_spec_enum ("blocking",
	                                                    "Blocking",
	                                                    "The Blocking",
	                                                    GEDIT_TYPE_THEATRICS_CHOREOGRAPHER_BLOCKING,
	                                                    GEDIT_THEATRICS_CHOREOGRAPHER_BLOCKING_DOWNSTAGE,
	                                                    G_PARAM_READWRITE |
	                                                    G_PARAM_CONSTRUCT |
	                                                    G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (object_class, PROP_ANIMATION_STATE,
	                                 g_param_spec_enum ("animation-state",
	                                                    "Animation State",
	                                                    "The Animation State",
	                                                    GEDIT_TYPE_THEATRICS_ANIMATION_STATE,
	                                                    GEDIT_THEATRICS_ANIMATION_STATE_COMING,
	                                                    G_PARAM_READWRITE |
	                                                    G_PARAM_CONSTRUCT |
	                                                    G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (object_class, PROP_DURATION,
	                                 g_param_spec_uint ("duration",
	                                                    "Duration",
	                                                    "The duration",
	                                                    0,
	                                                    G_MAXUINT,
	                                                    0,
	                                                    G_PARAM_READWRITE |
	                                                    G_PARAM_CONSTRUCT |
	                                                    G_PARAM_STATIC_STRINGS));

	g_type_class_add_private (object_class, sizeof (GeditTheatricsAnimationPrivate));
}

static void
gedit_theatrics_animation_init (GeditTheatricsAnimation *anim)
{
	anim->priv = GEDIT_THEATRICS_ANIMATION_GET_PRIVATE (anim);

	anim->priv->animation_state = GEDIT_THEATRICS_ANIMATION_STATE_COMING;
}

GeditTheatricsAnimation *
gedit_theatrics_animation_new (GeditTheatricsAnimationDrawable    *drawable,
			       guint                               duration,
			       GeditTheatricsChoreographerEasing   easing,
			       GeditTheatricsChoreographerBlocking blocking)
{
	return g_object_new (GEDIT_TYPE_THEATRICS_ANIMATION,
			     "drawable", drawable,
			     "duration", duration,
			     "easing", easing,
			     "blocking", blocking,
			     NULL);
}

GeditTheatricsAnimationDrawable *
gedit_theatrics_animation_get_drawable (GeditTheatricsAnimation *anim)
{
	g_return_val_if_fail (GEDIT_IS_THEATRICS_ANIMATION (anim), NULL);

	return anim->priv->drawable;
}

void
gedit_theatrics_animation_set_drawable (GeditTheatricsAnimation         *anim,
					GeditTheatricsAnimationDrawable *drawable)
{
	g_return_if_fail (GEDIT_IS_THEATRICS_ANIMATION (anim));

	if (drawable != anim->priv->drawable)
	{
		anim->priv->drawable = drawable;

		g_object_notify (G_OBJECT (anim), "drawable");
	}
}

GeditTheatricsChoreographerEasing
gedit_theatrics_animation_get_easing (GeditTheatricsAnimation *anim)
{
	g_return_val_if_fail (GEDIT_IS_THEATRICS_ANIMATION (anim), GEDIT_THEATRICS_CHOREOGRAPHER_EASING_LINEAR);

	return anim->priv->easing;
}

void
gedit_theatrics_animation_set_easing (GeditTheatricsAnimation          *anim,
				      GeditTheatricsChoreographerEasing easing)
{
	g_return_if_fail (GEDIT_IS_THEATRICS_ANIMATION (anim));

	if (anim->priv->easing != easing)
	{
		anim->priv->easing = easing;

		g_object_notify (G_OBJECT (anim), "easing");
	}
}

GeditTheatricsChoreographerBlocking
gedit_theatrics_animation_get_blocking (GeditTheatricsAnimation *anim)
{
	g_return_val_if_fail (GEDIT_IS_THEATRICS_ANIMATION (anim), GEDIT_THEATRICS_CHOREOGRAPHER_BLOCKING_DOWNSTAGE);

	return anim->priv->blocking;
}

void
gedit_theatrics_animation_set_blocking (GeditTheatricsAnimation            *anim,
					GeditTheatricsChoreographerBlocking blocking)
{
	g_return_if_fail (GEDIT_IS_THEATRICS_ANIMATION (anim));

	if (anim->priv->blocking != blocking)
	{
		anim->priv->blocking = blocking;

		g_object_notify (G_OBJECT (anim), "blocking");
	}
}

GeditTheatricsAnimationState
gedit_theatrics_animation_get_animation_state (GeditTheatricsAnimation *anim)
{
	g_return_val_if_fail (GEDIT_IS_THEATRICS_ANIMATION (anim), GEDIT_THEATRICS_ANIMATION_STATE_COMING);

	return anim->priv->animation_state;
}

void
gedit_theatrics_animation_set_animation_state (GeditTheatricsAnimation     *anim,
					       GeditTheatricsAnimationState animation_state)
{
	g_return_if_fail (GEDIT_IS_THEATRICS_ANIMATION (anim));

	if (anim->priv->animation_state != animation_state)
	{
		anim->priv->animation_state = animation_state;

		g_object_notify (G_OBJECT (anim), "animation-state");
	}
}

gdouble
gedit_theatrics_animation_get_duration (GeditTheatricsAnimation *anim)
{
	g_return_val_if_fail (GEDIT_IS_THEATRICS_ANIMATION (anim), 0.0);

	return anim->priv->duration;
}

void
gedit_theatrics_animation_set_duration (GeditTheatricsAnimation *anim,
					gdouble                  duration)
{
	g_return_if_fail (GEDIT_IS_THEATRICS_ANIMATION (anim));

	if (anim->priv->duration != duration)
	{
		anim->priv->duration = duration;

		g_object_notify (G_OBJECT (anim), "duration");
	}
}

/* ex:ts=8:noet: */
