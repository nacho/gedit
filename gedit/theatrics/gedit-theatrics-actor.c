/*
 * gedit-theatrics-actor.c
 * This file is part of gedit
 *
 * Copyright (C) 2010 - Ignacio Casal Quinteiro
 *
 * Based on Aaron Bockover <abockover@novell.com> work.
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

#include "gedit-theatrics-actor.h"


#define GEDIT_THEATRICS_ACTOR_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE((object), GEDIT_TYPE_THEATRICS_ACTOR, GeditTheatricsActorPrivate))

struct _GeditTheatricsActorPrivate
{
	GObject *target;

	guint duration;
	gdouble frames;
	gdouble percent;

	GTimeVal start_time;

	guint can_expire : 1;
};

enum
{
	PROP_0,
	PROP_TARGET,
	PROP_DURATION
};

G_DEFINE_TYPE (GeditTheatricsActor, gedit_theatrics_actor, G_TYPE_OBJECT)

static void
gedit_theatrics_actor_finalize (GObject *object)
{
	G_OBJECT_CLASS (gedit_theatrics_actor_parent_class)->finalize (object);
}

static void
gedit_theatrics_actor_get_property (GObject    *object,
				    guint       prop_id,
				    GValue     *value,
				    GParamSpec *pspec)
{
	GeditTheatricsActor *actor = GEDIT_THEATRICS_ACTOR (object);

	switch (prop_id)
	{
		case PROP_TARGET:
			g_value_set_object (value, actor->priv->target);
			break;
		case PROP_DURATION:
			g_value_set_uint (value, actor->priv->duration);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_theatrics_actor_set_property (GObject      *object,
				    guint         prop_id,
				    const GValue *value,
				    GParamSpec   *pspec)
{
	GeditTheatricsActor *actor = GEDIT_THEATRICS_ACTOR (object);

	switch (prop_id)
	{
		case PROP_TARGET:
		{
			GObject *target;

			target = g_value_get_object (value);

			if (target != NULL)
			{
				actor->priv->target = target;
			}

			break;
		}
		case PROP_DURATION:
			actor->priv->duration = g_value_get_uint (value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_theatrics_actor_class_init (GeditTheatricsActorClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	
	object_class->finalize = gedit_theatrics_actor_finalize;
	object_class->get_property = gedit_theatrics_actor_get_property;
	object_class->set_property = gedit_theatrics_actor_set_property;

	g_object_class_install_property (object_class, PROP_TARGET,
	                                 g_param_spec_object ("target",
	                                                      "Target",
	                                                      "The Target",
	                                                      G_TYPE_OBJECT,
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

	g_type_class_add_private (object_class, sizeof (GeditTheatricsActorPrivate));
}

static void
gedit_theatrics_actor_init (GeditTheatricsActor *actor)
{
	actor->priv = GEDIT_THEATRICS_ACTOR_GET_PRIVATE (actor);

	actor->priv->can_expire = TRUE;

	gedit_theatrics_actor_reset (actor, actor->priv->duration);
}

GeditTheatricsActor *
gedit_theatrics_actor_new (GObject *target,
			   guint    duration)
{
	return g_object_new (GEDIT_TYPE_THEATRICS_ACTOR,
			     "target", target,
			     "duration", duration,
			     NULL);
}

/* Use duration = -1 to not set a new duration */
void
gedit_theatrics_actor_reset (GeditTheatricsActor *actor,
			     guint                duration)
{
	g_return_if_fail (GEDIT_IS_THEATRICS_ACTOR (actor));

	g_get_current_time (&actor->priv->start_time);
	actor->priv->frames = 0.0;
	actor->priv->percent = 0.0;

	if (duration > -1)
	{
		actor->priv->duration = duration;
	}
}

void
gedit_theatrics_actor_step (GeditTheatricsActor *actor)
{
	GTimeVal now;
	gdouble now_num, start_time;

	g_return_if_fail (GEDIT_IS_THEATRICS_ACTOR (actor));

	if (!actor->priv->can_expire && actor->priv->percent >= 1.0)
	{
		gedit_theatrics_actor_reset (actor, actor->priv->duration);
	}

	g_get_current_time (&now);

	now_num = now.tv_sec * 1000 + now.tv_usec / 1000;
	start_time = actor->priv->start_time.tv_sec * 1000 + actor->priv->start_time.tv_usec / 1000;

	actor->priv->percent = ((now_num - start_time) / actor->priv->duration);

	actor->priv->frames++;
}

GObject *
gedit_theatrics_actor_get_target (GeditTheatricsActor *actor)
{
	g_return_val_if_fail (GEDIT_IS_THEATRICS_ACTOR (actor), NULL);

	return actor->priv->target;
}

gboolean
gedit_theatrics_actor_get_expired (GeditTheatricsActor *actor)
{
	g_return_val_if_fail (GEDIT_IS_THEATRICS_ACTOR (actor), FALSE);

	return (actor->priv->can_expire && actor->priv->percent >= 1.0);
}

gboolean
gedit_theatrics_actor_get_can_expire (GeditTheatricsActor *actor)
{
	g_return_val_if_fail (GEDIT_IS_THEATRICS_ACTOR (actor), FALSE);

	return actor->priv->can_expire;
}

void
gedit_theatrics_actor_set_can_expire (GeditTheatricsActor *actor,
				      gboolean             can_expire)
{
	g_return_if_fail (GEDIT_IS_THEATRICS_ACTOR (actor));

	actor->priv->can_expire = can_expire;
}

guint
gedit_theatrics_actor_get_duration (GeditTheatricsActor *actor)
{
	g_return_val_if_fail (GEDIT_IS_THEATRICS_ACTOR (actor), 0.0);

	return actor->priv->duration;
}

GTimeVal
gedit_theatrics_actor_get_start_time (GeditTheatricsActor *actor)
{
	GTimeVal r = {0, };

	g_return_val_if_fail (GEDIT_IS_THEATRICS_ACTOR (actor), r);

	return actor->priv->start_time;
}

gdouble
gedit_theatrics_actor_get_frames (GeditTheatricsActor *actor)
{
	g_return_val_if_fail (GEDIT_IS_THEATRICS_ACTOR (actor), 0.0);

	return actor->priv->frames;
}

gdouble
gedit_theatrics_actor_get_frames_per_second (GeditTheatricsActor *actor)
{
	g_return_val_if_fail (GEDIT_IS_THEATRICS_ACTOR (actor), 0.0);

	return (actor->priv->frames / (actor->priv->duration / 1000.0));
}

gdouble
gedit_theatrics_actor_get_percent (GeditTheatricsActor *actor)
{
	g_return_val_if_fail (GEDIT_IS_THEATRICS_ACTOR (actor), 0.0);

	return MAX (0.0, MIN (1.0, actor->priv->percent));
}

/* ex:set ts=8 noet: */
