/*
 * gedit-animated-overlay.c
 * This file is part of gedit
 *
 * Copyright (C) 2011 - Ignacio Casal Quinteiro
 *
 * Based on Mike Krüger <mkrueger@novell.com> work.
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

#include "gedit-animated-overlay.h"
#include "theatrics/gedit-theatrics-stage.h"
#include "theatrics/gedit-theatrics-animated-widget.h"

struct _GeditAnimatedOverlayPrivate
{
	GeditTheatricsStage *stage;
};

G_DEFINE_TYPE (GeditAnimatedOverlay, gedit_animated_overlay, GEDIT_TYPE_OVERLAY)

static void
gedit_animated_overlay_dispose (GObject *object)
{
	GeditAnimatedOverlayPrivate *priv = GEDIT_ANIMATED_OVERLAY (object)->priv;

	if (priv->stage != NULL)
	{
		g_object_unref (priv->stage);
		priv->stage = NULL;
	}

	G_OBJECT_CLASS (gedit_animated_overlay_parent_class)->dispose (object);
}

static void
gedit_animated_overlay_class_init (GeditAnimatedOverlayClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	
	object_class->dispose = gedit_animated_overlay_dispose;

	g_type_class_add_private (object_class, sizeof (GeditAnimatedOverlayPrivate));
}

static void
on_actor_step (GeditTheatricsStage  *stage,
               GeditTheatricsActor  *actor,
               GeditAnimatedOverlay *overlay)
{
	GeditTheatricsAnimationState animation_state;
	GObject *anim_widget;
	guint duration;

	anim_widget = gedit_theatrics_actor_get_target (actor);
	g_assert (GEDIT_IS_ANIMATABLE (anim_widget));

	g_object_get (anim_widget, "animation-state", &animation_state,
	              "duration", &duration, NULL);

	switch (animation_state)
	{
		case GEDIT_THEATRICS_ANIMATION_STATE_COMING:
			gtk_widget_queue_draw (GTK_WIDGET (anim_widget));

			g_object_set (anim_widget, "percent",
			              gedit_theatrics_actor_get_percent (actor),
			              NULL);

			if (gedit_theatrics_actor_get_expired (actor))
			{
				g_object_set (anim_widget, "animation-state",
				              GEDIT_THEATRICS_ANIMATION_STATE_IDLE, NULL);
			}
			break;
		case GEDIT_THEATRICS_ANIMATION_STATE_INTENDING_TO_GO:
			g_object_set (anim_widget,
			              "animation-state", GEDIT_THEATRICS_ANIMATION_STATE_GOING,
			              "bias", gedit_theatrics_actor_get_percent (actor),
			               NULL);
			gedit_theatrics_actor_reset (actor, duration * gedit_theatrics_actor_get_percent (actor));
			break;
		case GEDIT_THEATRICS_ANIMATION_STATE_GOING:
			if (gedit_theatrics_actor_get_expired (actor))
			{
				/* let the widget manage it? and maybe set it as idle*/
				//gtk_widget_destroy (GTK_WIDGET (anim_widget));
				return;
			}
			gtk_widget_queue_draw (GTK_WIDGET (anim_widget));
			g_object_set (anim_widget, "percent",
			              1.0 - gedit_theatrics_actor_get_percent (actor),
			              NULL);
			break;
		default:
			break;
	}
}

static void
gedit_animated_overlay_init (GeditAnimatedOverlay *overlay)
{
	overlay->priv = G_TYPE_INSTANCE_GET_PRIVATE (overlay,
	                                             GEDIT_TYPE_ANIMATED_OVERLAY,
	                                             GeditAnimatedOverlayPrivate);

	overlay->priv->stage = gedit_theatrics_stage_new ();

	g_signal_connect (overlay->priv->stage,
	                  "actor-step",
	                  G_CALLBACK (on_actor_step),
	                  overlay);
}

static void
on_animation_state_changed (GeditAnimatable      *animatable,
                            GParamSpec           *pspec,
                            GeditAnimatedOverlay *overlay)
{
	
}

/**
 * gedit_animated_overlay_new:
 * @main_widget: a #GtkWidget
 * @relative_widget: (allow-none): a #Gtkwidget
 *
 * Creates a new #GeditAnimatedOverlay. If @relative_widget is not %NULL the
 * floating widgets will be placed in relation to it, if not @main_widget will
 * be use for this purpose.
 *
 * Returns: a new #GeditAnimatedOverlay object.
 */
GtkWidget *
gedit_animated_overlay_new (GtkWidget *main_widget,
                            GtkWidget *relative_widget)
{
	return g_object_new (GEDIT_TYPE_ANIMATED_OVERLAY,
	                     "main-widget", main_widget,
	                     "relative-widget", relative_widget,
	                     NULL);
}

void
gedit_animated_overlay_add (GeditAnimatedOverlay     *overlay,
                            GeditAnimatable          *animatable,
                            GeditOverlayChildPosition position,
                            guint                     offset)
{
	guint duration;

	g_return_if_fail (GEDIT_IS_OVERLAY (overlay));
	g_return_if_fail (GEDIT_IS_ANIMATABLE (animatable));

	gedit_overlay_add (GEDIT_OVERLAY (overlay),
	                   GTK_WIDGET (animatable),
	                   position, offset);

	g_object_get (G_OBJECT (animatable), "duration", &duration, NULL);

	gedit_theatrics_stage_add_with_duration (overlay->priv->stage,
	                                         G_OBJECT (animatable),
	                                         duration);

	g_signal_connect (animatable,
	                  "notify::animation-state",
	                  G_CALLBACK (on_animation_state_changed)
	                  overlay);
}
