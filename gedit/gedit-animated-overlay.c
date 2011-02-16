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
	GeditTheatricsAnimatedWidget *anim_widget;

	anim_widget = GEDIT_THEATRICS_ANIMATED_WIDGET (gedit_theatrics_actor_get_target (actor));
	animation_state = gedit_theatrics_animated_widget_get_animation_state (anim_widget);

	switch (animation_state)
	{
		case GEDIT_THEATRICS_ANIMATION_STATE_COMING:
			gtk_widget_queue_draw (GTK_WIDGET (anim_widget));
			gedit_theatrics_animated_widget_set_percent (anim_widget,
			                                             gedit_theatrics_actor_get_percent (actor));
			if (gedit_theatrics_actor_get_expired (actor))
			{
				gedit_theatrics_animated_widget_set_animation_state (anim_widget,
				                                                     GEDIT_THEATRICS_ANIMATION_STATE_IDLE);
			}
			break;
		case GEDIT_THEATRICS_ANIMATION_STATE_INTENDING_TO_GO:
			gedit_theatrics_animated_widget_set_animation_state (anim_widget,
			                                                     GEDIT_THEATRICS_ANIMATION_STATE_GOING);
			gedit_theatrics_animated_widget_set_bias (anim_widget,
			                                          gedit_theatrics_actor_get_percent (actor));
			gedit_theatrics_actor_reset (actor, gedit_theatrics_animated_widget_get_duration (anim_widget) *
			                                    gedit_theatrics_actor_get_percent (actor));
			break;
		case GEDIT_THEATRICS_ANIMATION_STATE_GOING:
			if (gedit_theatrics_actor_get_expired (actor))
			{
				gtk_widget_destroy (GTK_WIDGET (anim_widget));
				return;
			}
			gtk_widget_queue_draw (GTK_WIDGET (anim_widget));
			gedit_theatrics_animated_widget_set_percent (anim_widget, 1.0 - gedit_theatrics_actor_get_percent (actor));
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

static GeditTheatricsAnimatedWidget *
get_animated_widget (GeditAnimatedOverlay *overlay,
                     GtkWidget            *widget)
{
	GeditTheatricsAnimatedWidget *anim_widget = NULL;
	GtkWidget *main_widget;
	GList *children, *l;

	children = gtk_container_get_children (GTK_CONTAINER (overlay));
	g_object_get (G_OBJECT (overlay), "main-widget", &main_widget, NULL);

	for (l = children; l != NULL; l = g_list_next (l))
	{
		GtkWidget *child = GTK_WIDGET (l->data);

		/* skip the main widget as it is not a OverlayChild */
		if (child == main_widget)
			continue;

		if (child == widget)
		{
			anim_widget = GEDIT_THEATRICS_ANIMATED_WIDGET (child);
			break;
		}
		else
		{
			GtkWidget *in_widget;

			/* let's try also with the internal widget */
			g_object_get (child, "widget", &in_widget, NULL);
			g_assert (in_widget != NULL);

			if (in_widget == widget)
			{
				anim_widget = GEDIT_THEATRICS_ANIMATED_WIDGET (child);
				g_object_unref (in_widget);

				break;
			}

			g_object_unref (in_widget);
		}
	}

	g_object_unref (main_widget);
	g_list_free (children);

	return anim_widget;
}

/**
 * gedit_animated_overlay_slide:
 * @overlay: a #GeditAnimatedOverlay
 * @widget: a #GtkWidget to add to @overlay
 * @position: a #GeditOverlayChildPosition
 * @offset: offset for @widget
 * @duration: the duration of the animation
 * @easing: a #GeditTheatricsChoreographerEasing
 * @blocking: a #GeditTheatricsChoreographerBlocking
 * @orientation: the orientation of the animation
 * @in: if %TRUE slide in if %FALSE slide out
 *
 * Adds @widget in @overlay with a slide in/out animation depending on @in.
 */
void
gedit_animated_overlay_slide (GeditAnimatedOverlay               *overlay,
                              GtkWidget                          *widget,
                              GeditOverlayChildPosition           position,
                              guint                               offset,
                              guint                               duration,
                              GeditTheatricsChoreographerEasing   easing,
                              GeditTheatricsChoreographerBlocking blocking,
                              GtkOrientation                      orientation,
                              gboolean                            in)
{
	GeditTheatricsAnimatedWidget *anim_widget;

	anim_widget = get_animated_widget (overlay, widget);

	if (anim_widget == NULL)
	{
		anim_widget = gedit_theatrics_animated_widget_new (widget, duration,
		                                                   easing,
		                                                   blocking,
		                                                   orientation);
		gtk_widget_show (GTK_WIDGET (anim_widget));

		gedit_overlay_add (GEDIT_OVERLAY (overlay),
		                   GTK_WIDGET (anim_widget),
		                   position, offset);
	}
	else
	{
		/* we are only interested in the easing and the blocking */
		gedit_theatrics_animated_widget_set_easing (anim_widget, easing);
		gedit_theatrics_animated_widget_set_blocking (anim_widget, blocking);
	}

	if (!in)
	{
		gedit_theatrics_animated_widget_set_animation_state (anim_widget, GEDIT_THEATRICS_ANIMATION_STATE_GOING);
	}

	gedit_theatrics_stage_add_with_duration (overlay->priv->stage,
	                                         G_OBJECT (anim_widget),
	                                         duration);
}
