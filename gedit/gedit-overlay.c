/*
 * gedit-overlay.c
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

#include "gedit-overlay.h"
#include "gedit-marshal.h"
#include "theatrics/gedit-theatrics-animated-widget.h"
#include "theatrics/gedit-theatrics-stage.h"

#define GEDIT_OVERLAY_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE((object), GEDIT_TYPE_OVERLAY, GeditOverlayPrivate))

typedef struct _OverlayChild
{
	GtkWidget *child;
	GdkGravity gravity;

	guint fixed_position : 1;
	guint is_animated : 1;
} OverlayChild;

struct _GeditOverlayPrivate
{
	GtkWidget *main_widget;
	GSList    *children;
	GtkAllocation main_alloc;

	GtkAdjustment *hadjustment;
	GtkAdjustment *vadjustment;
	glong          hadjustment_signal_id;
	glong          vadjustment_signal_id;

	GeditTheatricsStage *stage;
};

enum
{
	PROP_0,
	PROP_MAIN_WIDGET
};

enum
{
	SET_SCROLL_ADJUSTMENTS,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (GeditOverlay, gedit_overlay, GTK_TYPE_CONTAINER)

static void
free_container_child (OverlayChild *child)
{
	g_slice_free (OverlayChild, child);
}

static void
add_toplevel_widget (GeditOverlay *overlay,
                     GtkWidget    *widget,
                     gboolean      fixed_position,
                     gboolean      is_animated,
                     GdkGravity    gravity)
{
	OverlayChild *child = g_slice_new (OverlayChild);

	child->child = widget;
	child->gravity = gravity;
	child->fixed_position = fixed_position;
	child->is_animated = is_animated;

	gtk_widget_set_parent (widget, GTK_WIDGET (overlay));

	overlay->priv->children = g_slist_append (overlay->priv->children,
	                                          child);
}

static void
gedit_overlay_finalize (GObject *object)
{
	GeditOverlay *overlay = GEDIT_OVERLAY (object);

	g_slist_free (overlay->priv->children);

	G_OBJECT_CLASS (gedit_overlay_parent_class)->finalize (object);
}

static void
gedit_overlay_dispose (GObject *object)
{
	GeditOverlay *overlay = GEDIT_OVERLAY (object);

	if (overlay->priv->hadjustment != NULL)
	{
		g_signal_handler_disconnect (overlay->priv->hadjustment,
		                             overlay->priv->hadjustment_signal_id);
		overlay->priv->hadjustment = NULL;
	}

	if (overlay->priv->vadjustment != NULL)
	{
		g_signal_handler_disconnect (overlay->priv->vadjustment,
		                             overlay->priv->vadjustment_signal_id);
		overlay->priv->vadjustment = NULL;
	}

	if (overlay->priv->stage != NULL)
	{
		g_object_unref (overlay->priv->stage);
		overlay->priv->stage = NULL;
	}

	G_OBJECT_CLASS (gedit_overlay_parent_class)->dispose (object);
}

static void
gedit_overlay_get_property (GObject    *object,
                            guint       prop_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
	GeditOverlay *overlay = GEDIT_OVERLAY (object);

	switch (prop_id)
	{
		case PROP_MAIN_WIDGET:
			g_value_set_object (value, overlay->priv->main_widget);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_overlay_set_property (GObject      *object,
                            guint         prop_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
	GeditOverlay *overlay = GEDIT_OVERLAY (object);

	switch (prop_id)
	{
		case PROP_MAIN_WIDGET:
		{
			overlay->priv->main_widget = g_value_get_object (value);
			add_toplevel_widget (overlay,
			                     overlay->priv->main_widget,
			                     TRUE, FALSE, GDK_GRAVITY_STATIC);
			break;
		}
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_overlay_realize (GtkWidget *widget)
{
	GtkAllocation allocation;
	GdkWindow *window;
	GdkWindowAttr attributes;
	gint attributes_mask;

	gtk_widget_set_realized (widget, TRUE);

	gtk_widget_get_allocation (widget, &allocation);

	attributes.window_type = GDK_WINDOW_CHILD;
	attributes.x = allocation.x;
	attributes.y = allocation.y;
	attributes.width = allocation.width;
	attributes.height = allocation.height;
	attributes.wclass = GDK_INPUT_OUTPUT;
	attributes.visual = gtk_widget_get_visual (widget);
	attributes.event_mask = gtk_widget_get_events (widget);
	attributes.event_mask |= GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK;

	attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL;

	window = gdk_window_new (gtk_widget_get_parent_window (widget),
	                         &attributes, attributes_mask);
	gtk_widget_set_window (widget, window);
	gdk_window_set_user_data (window, widget);

	gtk_widget_style_attach (widget);
	gtk_style_set_background (gtk_widget_get_style (widget), window,
	                          GTK_STATE_NORMAL);
}

static void
set_children_positions (GeditOverlay *overlay)
{
	GSList *l;

	for (l = overlay->priv->children; l != NULL; l = g_slist_next (l))
	{
		OverlayChild *child = (OverlayChild *)l->data;
		GtkRequisition req;
		GtkAllocation alloc;

		if (child->child == overlay->priv->main_widget)
			continue;

		gtk_widget_get_preferred_size (child->child, &req, NULL);

		/* FIXME: Add all the gravities here */
		switch (child->gravity)
		{
			/* The gravity is the inverse of the place we want */
			case GDK_GRAVITY_SOUTH_WEST:
				alloc.x = overlay->priv->main_alloc.width - req.width;
				alloc.y = 0;
				break;
			default:
				alloc.x = 0;
				alloc.y = 0;
		}

		if (!child->fixed_position)
		{
			alloc.x *= gtk_adjustment_get_value (overlay->priv->hadjustment);
			alloc.y *= gtk_adjustment_get_value (overlay->priv->vadjustment);
		}

		alloc.width = req.width;
		alloc.height = req.height;

		gtk_widget_size_allocate (child->child, &alloc);
	}
}

static void
gedit_overlay_size_allocate (GtkWidget     *widget,
                             GtkAllocation *allocation)
{
	GeditOverlay *overlay = GEDIT_OVERLAY (widget);

	GTK_WIDGET_CLASS (gedit_overlay_parent_class)->size_allocate (widget, allocation);

	overlay->priv->main_alloc.x = 0;
	overlay->priv->main_alloc.y = 0;
	overlay->priv->main_alloc.width = allocation->width;
	overlay->priv->main_alloc.height = allocation->height;

	gtk_widget_size_allocate (overlay->priv->main_widget,
	                          &overlay->priv->main_alloc);
	set_children_positions (overlay);
}

static void
gedit_overlay_add (GtkContainer *overlay,
                   GtkWidget    *widget)
{
	add_toplevel_widget (GEDIT_OVERLAY (overlay), widget,
	                     FALSE, FALSE, GDK_GRAVITY_STATIC);
}

static void
gedit_overlay_remove (GtkContainer *overlay,
                      GtkWidget    *widget)
{
	GeditOverlay *goverlay = GEDIT_OVERLAY (overlay);
	GSList *l;

	for (l = goverlay->priv->children; l != NULL; l = g_slist_next (l))
	{
		OverlayChild *child = (OverlayChild *)l->data;

		if (child->child == widget)
		{
			gtk_widget_unparent (widget);
			goverlay->priv->children = g_slist_remove_link (goverlay->priv->children,
			                                                l);
			free_container_child (child);
			break;
		}
	}
}

static void
gedit_overlay_forall (GtkContainer *overlay,
                      gboolean      include_internals,
                      GtkCallback   callback,
                      gpointer      callback_data)
{
	GeditOverlay *goverlay = GEDIT_OVERLAY (overlay);
	GSList *l;

	for (l = goverlay->priv->children; l != NULL; l = g_slist_next (l))
	{
		OverlayChild *child = (OverlayChild *)l->data;

		(* callback) (child->child, callback_data);
	}
}

static GType
gedit_overlay_child_type (GtkContainer *overlay)
{
	return GTK_TYPE_WIDGET;
}

static void
adjustment_value_changed (GtkAdjustment *adjustment,
                          GeditOverlay  *overlay)
{
	set_children_positions (overlay);
}

static void
gedit_overlay_set_scroll_adjustments (GeditOverlay  *overlay,
                                      GtkAdjustment *hadjustment,
                                      GtkAdjustment *vadjustment)
{
	if (overlay->priv->hadjustment != NULL)
	{
		g_signal_handler_disconnect (overlay->priv->hadjustment,
		                             overlay->priv->hadjustment_signal_id);
	}

	if (overlay->priv->vadjustment != NULL)
	{
		g_signal_handler_disconnect (overlay->priv->vadjustment,
		                             overlay->priv->vadjustment_signal_id);
	}

	if (hadjustment != NULL)
	{
		overlay->priv->hadjustment_signal_id =
			g_signal_connect (hadjustment,
			                  "value-changed",
			                  G_CALLBACK (adjustment_value_changed),
			                  overlay);
	}

	if (vadjustment != NULL)
	{
		overlay->priv->vadjustment_signal_id =
			g_signal_connect (vadjustment,
			                  "value-changed",
			                  G_CALLBACK (adjustment_value_changed),
			                  overlay);
	}

	overlay->priv->hadjustment = hadjustment;
	overlay->priv->vadjustment = vadjustment;

	gtk_widget_set_scroll_adjustments (overlay->priv->main_widget,
	                                   hadjustment,
	                                   vadjustment);
}

static void
gedit_overlay_class_init (GeditOverlayClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
	GtkContainerClass *container_class = GTK_CONTAINER_CLASS (klass);

	object_class->finalize = gedit_overlay_finalize;
	object_class->dispose = gedit_overlay_dispose;
	object_class->get_property = gedit_overlay_get_property;
	object_class->set_property = gedit_overlay_set_property;

	widget_class->realize = gedit_overlay_realize;
	widget_class->size_allocate = gedit_overlay_size_allocate;

	container_class->add = gedit_overlay_add;
	container_class->remove = gedit_overlay_remove;
	container_class->forall = gedit_overlay_forall;
	container_class->child_type = gedit_overlay_child_type;

	klass->set_scroll_adjustments = gedit_overlay_set_scroll_adjustments;

	g_object_class_install_property (object_class, PROP_MAIN_WIDGET,
	                                 g_param_spec_object ("main-widget",
	                                                      "Main Widget",
	                                                      "The Main Widget",
	                                                      GTK_TYPE_WIDGET,
	                                                      G_PARAM_READWRITE |
	                                                      G_PARAM_CONSTRUCT_ONLY |
	                                                      G_PARAM_STATIC_STRINGS));

	signals[SET_SCROLL_ADJUSTMENTS] =
		g_signal_new ("set-scroll-adjustments",
		              G_OBJECT_CLASS_TYPE (object_class),
		              G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		              G_STRUCT_OFFSET (GeditOverlayClass, set_scroll_adjustments),
		              NULL, NULL,
		              gedit_marshal_VOID__OBJECT_OBJECT,
		              G_TYPE_NONE, 2,
		              GTK_TYPE_ADJUSTMENT,
		              GTK_TYPE_ADJUSTMENT);
	widget_class->set_scroll_adjustments_signal = signals[SET_SCROLL_ADJUSTMENTS];

	g_type_class_add_private (object_class, sizeof (GeditOverlayPrivate));
}

static void
on_actor_step (GeditTheatricsStage *stage,
               GeditTheatricsActor *actor,
               GeditOverlay        *overlay)
{
	GeditTheatricsAnimationState animation_state;
	GeditTheatricsAnimatedWidget *anim_widget;

	anim_widget = GEDIT_THEATRICS_ANIMATED_WIDGET (gedit_theatrics_actor_get_target (actor));
	animation_state = gedit_theatrics_animated_widget_get_animation_state (anim_widget);

	switch (animation_state)
	{
		case GEDIT_THEATRICS_ANIMATION_STATE_COMING:
		{
			gtk_widget_queue_draw (GTK_WIDGET (anim_widget));
			gedit_theatrics_animated_widget_set_percent (anim_widget,
			                                             gedit_theatrics_actor_get_percent (actor));
			if (gedit_theatrics_actor_get_expired (actor))
			{
				gedit_theatrics_animated_widget_set_animation_state (anim_widget,
				                                                     GEDIT_THEATRICS_ANIMATION_STATE_IDLE);
			}
			break;
		}
		case GEDIT_THEATRICS_ANIMATION_STATE_INTENDING_TO_GO:
		{
			gedit_theatrics_animated_widget_set_animation_state (anim_widget,
			                                                     GEDIT_THEATRICS_ANIMATION_STATE_GOING);
			gedit_theatrics_animated_widget_set_bias (anim_widget,
			                                          gedit_theatrics_actor_get_percent (actor));
			gedit_theatrics_actor_reset (actor, gedit_theatrics_animated_widget_get_duration (anim_widget) *
			                                    gedit_theatrics_actor_get_percent (actor));
			break;
		}
		case GEDIT_THEATRICS_ANIMATION_STATE_GOING:
		{
			if (gedit_theatrics_actor_get_expired (actor))
			{
				gtk_container_remove (GTK_CONTAINER (overlay),
				                      GTK_WIDGET (anim_widget));
				return;
			}
			gedit_theatrics_animated_widget_set_percent (anim_widget, 1.0 - gedit_theatrics_actor_get_percent (actor));
			break;
		}
		default:
			break;
	}
}

static void
remove_core (GeditOverlay                 *overlay,
             GeditTheatricsAnimatedWidget *aw)
{
	GeditTheatricsAnimationState animation_state;

	animation_state = gedit_theatrics_animated_widget_get_animation_state (aw);

	if (animation_state == GEDIT_THEATRICS_ANIMATION_STATE_COMING)
	{
		gedit_theatrics_animated_widget_set_animation_state (aw, GEDIT_THEATRICS_ANIMATION_STATE_INTENDING_TO_GO);
	}
	else
	{
		GeditTheatricsChoreographerEasing easing;

		easing = gedit_theatrics_animated_widget_get_easing (aw);

		switch (easing)
		{
			case GEDIT_THEATRICS_CHOREOGRAPHER_EASING_QUADRATIC_IN:
				gedit_theatrics_animated_widget_set_easing (
					aw, GEDIT_THEATRICS_CHOREOGRAPHER_EASING_QUADRATIC_OUT);
				break;
			case GEDIT_THEATRICS_CHOREOGRAPHER_EASING_QUADRATIC_OUT:
				gedit_theatrics_animated_widget_set_easing (
					aw, GEDIT_THEATRICS_CHOREOGRAPHER_EASING_QUADRATIC_IN);
				break;
			case GEDIT_THEATRICS_CHOREOGRAPHER_EASING_EXPONENTIAL_IN:
				gedit_theatrics_animated_widget_set_easing (
					aw, GEDIT_THEATRICS_CHOREOGRAPHER_EASING_EXPONENTIAL_OUT);
				break;
			case GEDIT_THEATRICS_CHOREOGRAPHER_EASING_EXPONENTIAL_OUT:
				gedit_theatrics_animated_widget_set_easing (
					aw, GEDIT_THEATRICS_CHOREOGRAPHER_EASING_EXPONENTIAL_IN);
				break;
			default:
				break;
		}

		gedit_theatrics_animated_widget_set_animation_state (aw, GEDIT_THEATRICS_ANIMATION_STATE_GOING);
		gedit_theatrics_stage_add_with_duration (overlay->priv->stage,
		                                         G_OBJECT (aw),
		                                         gedit_theatrics_animated_widget_get_duration (aw));
	}
}

static void
on_remove_core (GeditTheatricsAnimatedWidget *aw,
                GeditOverlay                 *overlay)
{
	remove_core (overlay, aw);
}

static void
gedit_overlay_init (GeditOverlay *overlay)
{
	overlay->priv = GEDIT_OVERLAY_GET_PRIVATE (overlay);

	overlay->priv->stage = gedit_theatrics_stage_new ();

	g_signal_connect (overlay->priv->stage,
	                  "actor-step",
	                  G_CALLBACK (on_actor_step),
	                  overlay);
}

GtkWidget *
gedit_overlay_new (GtkWidget *main_widget)
{
	return GTK_WIDGET (g_object_new (GEDIT_TYPE_OVERLAY,
	                                 "main-widget", main_widget,
	                                 NULL));
}

void
gedit_overlay_add_animated_widget (GeditOverlay                       *overlay,
                                   GtkWidget                          *widget,
                                   guint                               duration,
                                   GeditTheatricsChoreographerEasing   easing,
                                   GeditTheatricsChoreographerBlocking blocking,
                                   GtkOrientation                      orientation,
                                   GdkGravity                          gravity)
{
	GeditTheatricsAnimatedWidget *anim_widget;
	GeditTheatricsActor *actor;
	GtkAllocation widget_alloc;

	anim_widget = gedit_theatrics_animated_widget_new (widget, duration,
	                                                   easing,
	                                                   blocking,
	                                                   orientation);
	gtk_widget_show (GTK_WIDGET (anim_widget));

	g_signal_connect (anim_widget,
	                  "remove-core",
	                  G_CALLBACK (on_remove_core),
	                  overlay);

	gtk_widget_get_allocation (widget, &widget_alloc);
	gedit_theatrics_animated_widget_set_end_padding (anim_widget,
	                                                 widget_alloc.height);

	actor = gedit_theatrics_stage_add_with_duration (overlay->priv->stage,
	                                                 G_OBJECT (anim_widget),
	                                                 duration);

	add_toplevel_widget (overlay, GTK_WIDGET (anim_widget), TRUE,
	                     TRUE, gravity);
}