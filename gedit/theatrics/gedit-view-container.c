/*
 * gedit-view-container.c
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

#include "gedit-view-container.h"
#include "gedit-theatrics-animation.h"
#include "gedit-theatrics-animated-widget.h"
#include "gedit-theatrics-stage.h"
#include "gedit-theatrics-marshal.h"

#define GEDIT_VIEW_CONTAINER_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE((object), GEDIT_TYPE_VIEW_CONTAINER, GeditViewContainerPrivate))

typedef struct _ContainerChild
{
	GtkWidget *child;
	gint x;
	gint y;

	guint fixed_position : 1;
	guint is_animated : 1;
} ContainerChild;

struct _GeditViewContainerPrivate
{
	GtkWidget *main_widget;
	GSList *children;

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

G_DEFINE_TYPE (GeditViewContainer, gedit_view_container, GTK_TYPE_CONTAINER)

static void
free_container_child (ContainerChild *child)
{
	g_slice_free (ContainerChild, child);
}

static void
add_toplevel_widget (GeditViewContainer *container,
                     GtkWidget          *widget,
                     gboolean            fixed_position,
                     gboolean            is_animated,
                     gint                x,
                     gint                y)
{
	ContainerChild *child = g_slice_new (ContainerChild);

	gtk_widget_set_parent (widget, GTK_WIDGET (container));
	child->child = widget;
	child->x = x;
	child->y = y;
	child->fixed_position = fixed_position;
	child->is_animated = is_animated;

	container->priv->children = g_slist_append (container->priv->children,
	                                            child);
}

static void
gedit_view_container_finalize (GObject *object)
{
	GeditViewContainer *container = GEDIT_VIEW_CONTAINER (object);

	g_slist_free (container->priv->children);

	G_OBJECT_CLASS (gedit_view_container_parent_class)->finalize (object);
}

static void
gedit_view_container_dispose (GObject *object)
{
	GeditViewContainer *container = GEDIT_VIEW_CONTAINER (object);

	if (container->priv->stage != NULL)
	{
		g_object_unref (container->priv->stage);
		container->priv->stage = NULL;
	}

	G_OBJECT_CLASS (gedit_view_container_parent_class)->dispose (object);
}

static void
gedit_view_container_get_property (GObject    *object,
                                   guint       prop_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
	GeditViewContainer *container = GEDIT_VIEW_CONTAINER (object);

	switch (prop_id)
	{
		case PROP_MAIN_WIDGET:
			g_value_set_object (value, container->priv->main_widget);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_view_container_set_property (GObject      *object,
                                   guint         prop_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
	GeditViewContainer *container = GEDIT_VIEW_CONTAINER (object);

	switch (prop_id)
	{
		case PROP_MAIN_WIDGET:
		{
			container->priv->main_widget = g_value_get_object (value);
			add_toplevel_widget (container,
			                     container->priv->main_widget,
			                     TRUE, FALSE, 0, 0);
			break;
		}
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_view_container_destroy (GtkObject *object)
{
	GeditViewContainer *container = GEDIT_VIEW_CONTAINER (object);

	if (container->priv->hadjustment != NULL)
	{
		g_signal_handler_disconnect (container->priv->hadjustment,
		                             container->priv->hadjustment_signal_id);
		container->priv->hadjustment = NULL;
	}

	if (container->priv->vadjustment != NULL)
	{
		g_signal_handler_disconnect (container->priv->vadjustment,
		                             container->priv->vadjustment_signal_id);
		container->priv->vadjustment = NULL;
	}

	GTK_OBJECT_CLASS (gedit_view_container_parent_class)->destroy (object);
}

static void
gedit_view_container_realize (GtkWidget *widget)
{
	GdkWindowAttr attributes;
	GtkAllocation allocation;
	GdkWindowAttributesType mask;
	GdkWindow *parent_window;
	GdkWindow *window;
	GtkStyle *style;

	gtk_widget_set_realized (widget, TRUE);
	gtk_widget_get_allocation (widget, &allocation);

	parent_window = gtk_widget_get_parent_window (widget);
	style = gtk_widget_get_style (widget);

	attributes.window_type = GDK_WINDOW_CHILD;
	attributes.x = allocation.x;
	attributes.y = allocation.y;
	attributes.width = allocation.width;
	attributes.height = allocation.height;
	attributes.wclass = GDK_INPUT_OUTPUT;
	attributes.visual = gtk_widget_get_visual (widget);
	attributes.colormap = gtk_widget_get_colormap (widget);
	attributes.event_mask = gtk_widget_get_events (widget) | GDK_EXPOSURE_MASK;

	mask = GDK_WA_X | GDK_WA_Y | GDK_WA_COLORMAP | GDK_WA_VISUAL;
	window = gdk_window_new (parent_window, &attributes, mask);
	gdk_window_set_user_data (window, widget);
	gtk_widget_set_window (widget, window);

	style = gtk_style_attach (style, window);
	gtk_widget_set_style (widget, style);
	gtk_style_set_background (style, window, GTK_STATE_NORMAL);
}

static void
set_children_positions (GeditViewContainer *container)
{
	GSList *l;

	for (l = container->priv->children; l != NULL; l = g_slist_next (l))
	{
		ContainerChild *child = (ContainerChild *)l->data;
		GtkRequisition req;
		GtkAllocation alloc;

		if (child->child == container->priv->main_widget)
			continue;

		gtk_widget_size_request (child->child, &req);

		alloc.x = child->fixed_position ? child->x :
		          child->x * gtk_adjustment_get_value (container->priv->hadjustment);
		alloc.y = child->fixed_position ? child->y :
		          child->y * gtk_adjustment_get_value (container->priv->vadjustment);
		alloc.width = req.width;
		alloc.height = req.height;

		gtk_widget_size_allocate (child->child, &alloc);
	}
}

static void
gedit_view_container_size_allocate (GtkWidget     *widget,
                                    GtkAllocation *allocation)
{
	GeditViewContainer *container = GEDIT_VIEW_CONTAINER (widget);
	GtkAllocation alloc;

	GTK_WIDGET_CLASS (gedit_view_container_parent_class)->size_allocate (widget, allocation);

	alloc.x = 0;
	alloc.y = 0;
	alloc.width = allocation->width;
	alloc.height = allocation->height;

	gtk_widget_size_allocate (container->priv->main_widget,
	                          &alloc);
	set_children_positions (container);
}

static void
gedit_view_container_add (GtkContainer *container,
                          GtkWidget    *widget)
{
	add_toplevel_widget (GEDIT_VIEW_CONTAINER (container), widget,
	                     FALSE, FALSE, 0, 0);
}

static void
gedit_view_container_remove (GtkContainer *container,
                             GtkWidget    *widget)
{
	GeditViewContainer *vcontainer = GEDIT_VIEW_CONTAINER (container);
	GSList *l;

	for (l = vcontainer->priv->children; l != NULL; l = g_slist_next (l))
	{
		ContainerChild *child = (ContainerChild *)l->data;

		if (child->child == widget)
		{
			gtk_widget_unparent (widget);
			vcontainer->priv->children = g_slist_remove (vcontainer->priv->children,
			                                             child);
			free_container_child (child);
		}
	}
}

static void
gedit_view_container_forall (GtkContainer *container,
                             gboolean      include_internals,
                             GtkCallback   callback,
                             gpointer      callback_data)
{
	GeditViewContainer *vcontainer = GEDIT_VIEW_CONTAINER (container);
	GSList *l;

	for (l = vcontainer->priv->children; l != NULL; l = g_slist_next (l))
	{
		ContainerChild *child = (ContainerChild *)l->data;

		callback (child->child, callback_data);
	}
}

static GType
gedit_view_container_child_type (GtkContainer *container)
{
	return GTK_TYPE_WIDGET;
}

static void
adjustment_value_changed (GtkAdjustment      *adjustment,
                          GeditViewContainer *container)
{
	set_children_positions (container);
}

static void
gedit_view_container_set_scroll_adjustments (GeditViewContainer *container,
                                             GtkAdjustment      *hadjustment,
                                             GtkAdjustment      *vadjustment)
{
	if (container->priv->hadjustment != NULL)
	{
		g_signal_handler_disconnect (container->priv->hadjustment,
		                             container->priv->hadjustment_signal_id);
	}

	if (container->priv->vadjustment != NULL)
	{
		g_signal_handler_disconnect (container->priv->vadjustment,
		                             container->priv->vadjustment_signal_id);
	}

	if (hadjustment != NULL)
	{
		container->priv->hadjustment_signal_id =
			g_signal_connect (hadjustment,
			                  "value-changed",
			                  G_CALLBACK (adjustment_value_changed),
			                  container);
	}

	if (vadjustment != NULL)
	{
		container->priv->vadjustment_signal_id =
			g_signal_connect (vadjustment,
			                  "value-changed",
			                  G_CALLBACK (adjustment_value_changed),
			                  container);
	}

	container->priv->hadjustment = hadjustment;
	container->priv->vadjustment = vadjustment;

	gtk_widget_set_scroll_adjustments (container->priv->main_widget,
	                                   hadjustment,
	                                   vadjustment);
}

static void
gedit_view_container_class_init (GeditViewContainerClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkObjectClass *gtkobject_class = GTK_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
	GtkContainerClass *container_class = GTK_CONTAINER_CLASS (klass);

	object_class->finalize = gedit_view_container_finalize;
	object_class->dispose = gedit_view_container_dispose;
	object_class->get_property = gedit_view_container_get_property;
	object_class->set_property = gedit_view_container_set_property;

	gtkobject_class->destroy = gedit_view_container_destroy;

	widget_class->realize = gedit_view_container_realize;
	widget_class->size_allocate = gedit_view_container_size_allocate;

	container_class->add = gedit_view_container_add;
	container_class->remove = gedit_view_container_remove;
	container_class->forall = gedit_view_container_forall;
	container_class->child_type = gedit_view_container_child_type;

	klass->set_scroll_adjustments = gedit_view_container_set_scroll_adjustments;

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
		              G_STRUCT_OFFSET (GeditViewContainerClass, set_scroll_adjustments),
		              NULL, NULL,
		              gedit_theatrics_marshal_VOID__OBJECT_OBJECT,
		              G_TYPE_NONE, 2,
		              GTK_TYPE_ADJUSTMENT,
		              GTK_TYPE_ADJUSTMENT);
	widget_class->set_scroll_adjustments_signal = signals[SET_SCROLL_ADJUSTMENTS];

	g_type_class_add_private (object_class, sizeof (GeditViewContainerPrivate));
}

static void
on_actor_step (GeditTheatricsStage *stage,
               GeditTheatricsActor *actor,
               GeditViewContainer  *container)
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
				gtk_container_remove (GTK_CONTAINER (container),
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
remove_core (GeditViewContainer           *container,
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
		gedit_theatrics_stage_add_with_duration (container->priv->stage,
		                                         G_OBJECT (aw),
		                                         gedit_theatrics_animated_widget_get_duration (aw));
	}
}

static void
on_remove_core (GeditTheatricsAnimatedWidget *aw,
                GeditViewContainer           *container)
{
	remove_core (container, aw);
}

static void
gedit_view_container_init (GeditViewContainer *container)
{
	container->priv = GEDIT_VIEW_CONTAINER_GET_PRIVATE (container);

	container->priv->stage = gedit_theatrics_stage_new ();

	g_signal_connect (container->priv->stage,
	                  "actor-step",
	                  G_CALLBACK (on_actor_step),
	                  container);
}

GtkWidget *
gedit_view_container_new (GtkWidget *main_widget)
{
	return GTK_WIDGET (g_object_new (GEDIT_TYPE_VIEW_CONTAINER,
	                                 "main-widget", main_widget,
	                                 NULL));
}

void
gedit_view_container_add_animated_widget (GeditViewContainer                 *container,
                                          GtkWidget                          *widget,
                                          guint                               duration,
                                          GeditTheatricsChoreographerEasing   easing,
                                          GeditTheatricsChoreographerBlocking blocking,
                                          GtkOrientation                      orientation,
                                          gint                                x,
                                          gint                                y)
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
	                  container);

	gtk_widget_get_allocation (widget, &widget_alloc);
	gedit_theatrics_animated_widget_set_end_padding (anim_widget,
	                                                 widget_alloc.height);

	actor = gedit_theatrics_stage_add_with_duration (container->priv->stage,
	                                                 G_OBJECT (anim_widget),
	                                                 duration);

	add_toplevel_widget (container, GTK_WIDGET (anim_widget), TRUE,
	                     TRUE, x, y);
}

void
gedit_view_container_move_widget (GeditViewContainer *container,
                                  GtkWidget          *widget,
                                  gint                x,
                                  gint                y)
{
	GSList *l;

	for (l = container->priv->children; l != NULL; l = g_slist_next (l))
	{
		ContainerChild *child = (ContainerChild *)l->data;
		GtkWidget *w;

		if (child->is_animated)
		{
			w = gedit_theatrics_animated_widget_get_widget (GEDIT_THEATRICS_ANIMATED_WIDGET (child->child));
		}
		else
		{
			w = child->child;
		}

		if (w == widget)
		{
			child->x = x;
			child->y = y;
		}
	}
}
