/*
 * gedit-theatrics-stage.c
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

#include "gedit-theatrics-stage.h"


#define DEFAULT_UPDATE_FREQUENCY 30
#define DEFAULT_DURATION 1000

#define GEDIT_THEATRICS_STAGE_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE((object), GEDIT_TYPE_THEATRICS_STAGE, GeditTheatricsStagePrivate))

struct _GeditTheatricsStagePrivate
{
	GHashTable *actors;
	guint timeout_id;
	guint update_frequency;
	guint actor_duration;

	guint playing : 1;
};

enum
{
	PROP_0,
	PROP_ACTOR_DURATION
};

enum
{
	ACTOR_STEP,
	ITERATION,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (GeditTheatricsStage, gedit_theatrics_stage, G_TYPE_OBJECT)

static void
gedit_theatrics_stage_dispose (GObject *object)
{
	GeditTheatricsStage *stage = GEDIT_THEATRICS_STAGE (object);

	if (stage->priv->timeout_id != 0)
	{
		g_source_remove (stage->priv->timeout_id);
		stage->priv->timeout_id = 0;
	}

	G_OBJECT_CLASS (gedit_theatrics_stage_parent_class)->dispose (object);
}

static void
gedit_theatrics_stage_finalize (GObject *object)
{
	GeditTheatricsStage *stage = GEDIT_THEATRICS_STAGE (object);

	g_hash_table_destroy (stage->priv->actors);

	G_OBJECT_CLASS (gedit_theatrics_stage_parent_class)->finalize (object);
}

static void
gedit_theatrics_stage_get_property (GObject    *object,
				    guint       prop_id,
				    GValue     *value,
				    GParamSpec *pspec)
{
	GeditTheatricsStage *stage = GEDIT_THEATRICS_STAGE (object);

	switch (prop_id)
	{
		case PROP_ACTOR_DURATION:
			g_value_set_uint (value, stage->priv->actor_duration);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_theatrics_stage_set_property (GObject      *object,
				    guint         prop_id,
				    const GValue *value,
				    GParamSpec   *pspec)
{
	GeditTheatricsStage *stage = GEDIT_THEATRICS_STAGE (object);

	switch (prop_id)
	{
		case PROP_ACTOR_DURATION:
			stage->priv->actor_duration = g_value_get_uint (value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_theatrics_stage_class_init (GeditTheatricsStageClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = gedit_theatrics_stage_dispose;
	object_class->finalize = gedit_theatrics_stage_finalize;
	object_class->get_property = gedit_theatrics_stage_get_property;
	object_class->set_property = gedit_theatrics_stage_set_property;

	g_object_class_install_property (object_class, PROP_ACTOR_DURATION,
	                                 g_param_spec_uint ("actor-duration",
	                                                    "Actor Duration",
	                                                    "The actor duration",
	                                                    0,
	                                                    G_MAXUINT,
	                                                    DEFAULT_DURATION,
	                                                    G_PARAM_READWRITE |
	                                                    G_PARAM_CONSTRUCT |
	                                                    G_PARAM_STATIC_STRINGS));

	signals[ACTOR_STEP] =
		g_signal_new ("actor-step",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GeditTheatricsStageClass, actor_step),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE,
			      1,
			      GEDIT_TYPE_THEATRICS_ACTOR);

	signals[ITERATION] =
		g_signal_new ("iteration",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GeditTheatricsStageClass, iteration),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);

	g_type_class_add_private (object_class, sizeof (GeditTheatricsStagePrivate));
}

static void
gedit_theatrics_stage_init (GeditTheatricsStage *stage)
{
	stage->priv = GEDIT_THEATRICS_STAGE_GET_PRIVATE (stage);

	stage->priv->update_frequency = DEFAULT_UPDATE_FREQUENCY;
	stage->priv->actor_duration = DEFAULT_DURATION;
	stage->priv->playing = TRUE;

	stage->priv->actors = g_hash_table_new (g_direct_hash,
						g_direct_equal);
}

static void
iterate_actors (gpointer             key,
		gpointer             value,
		GeditTheatricsStage *stage)
{
	GeditTheatricsActor *actor = GEDIT_THEATRICS_ACTOR (value);

	gedit_theatrics_actor_step (actor);

	g_signal_emit (G_OBJECT (stage), signals[ACTOR_STEP], 0, actor);

	if (gedit_theatrics_actor_get_expired (actor))
	{
		g_hash_table_remove (stage->priv->actors, key);
	}
}

static gboolean
on_timeout (GeditTheatricsStage *stage)
{
	if (!stage->priv->playing || g_hash_table_size (stage->priv->actors) == 0)
	{
		stage->priv->timeout_id = 0;
		return FALSE;
	}

	g_hash_table_foreach (stage->priv->actors,
			      (GHFunc)iterate_actors,
			      stage);

	g_signal_emit (G_OBJECT (stage), signals[ITERATION], 0);

	return TRUE;
}

static void
check_timeout (GeditTheatricsStage *stage)
{
	if ((!stage->priv->playing ||
	     g_hash_table_size (stage->priv->actors) == 0) &&
	    stage->priv->timeout_id)
	{
		g_source_remove (stage->priv->timeout_id);
		stage->priv->timeout_id = 0;
	}
	else if ((stage->priv->playing &&
	          g_hash_table_size (stage->priv->actors) > 0) &&
	         stage->priv->timeout_id <= 0)
	{
		stage->priv->timeout_id = g_timeout_add (stage->priv->update_frequency,
							 (GSourceFunc) on_timeout,
							 stage);
	}
}

GeditTheatricsStage *
gedit_theatrics_stage_new ()
{
	return g_object_new (GEDIT_TYPE_THEATRICS_STAGE, NULL);
}

GeditTheatricsStage *
gedit_theatrics_stage_new_with_duration (guint actor_duration)
{
	return g_object_new (GEDIT_TYPE_THEATRICS_STAGE,
			     "actor-duration", actor_duration,
			     NULL);
}

GeditTheatricsActor *
gedit_theatrics_stage_add (GeditTheatricsStage *stage,
			   GObject             *target)
{
	g_return_val_if_fail (GEDIT_IS_THEATRICS_STAGE (stage), NULL);

	return gedit_theatrics_stage_add_with_duration (stage, target,
							stage->priv->actor_duration);
}

GeditTheatricsActor *
gedit_theatrics_stage_add_with_duration (GeditTheatricsStage *stage,
					 GObject             *target,
					 guint                duration)
{
	GeditTheatricsActor *actor;

	g_return_val_if_fail (GEDIT_IS_THEATRICS_STAGE (stage), NULL);

	actor = g_hash_table_lookup (stage->priv->actors,
				     target);

	if (actor != NULL)
	{
		g_warning ("Stage already contains this actor");
		return NULL;
	}

	actor = gedit_theatrics_actor_new (target, duration);

	g_hash_table_insert (stage->priv->actors,
			     target, actor);

	check_timeout (stage);

	return actor;
}

void
gedit_theatrics_stage_remove (GeditTheatricsStage *stage,
			      GObject             *target)
{
	g_return_if_fail (GEDIT_IS_THEATRICS_STAGE (stage));

	g_hash_table_remove (stage->priv->actors, target);
}

void
gedit_theatrics_stage_set_playing (GeditTheatricsStage *stage,
				   gboolean             playing)
{
	g_return_if_fail (GEDIT_IS_THEATRICS_STAGE (stage));

	stage->priv->playing = playing;
}

gboolean
gedit_theatrics_stage_get_playing (GeditTheatricsStage *stage)
{
	g_return_val_if_fail (GEDIT_IS_THEATRICS_STAGE (stage), FALSE);

	return stage->priv->playing;
}

/* ex:ts=8:noet: */
