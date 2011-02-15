/*
 * gedit-theatrics-animated-widget.c
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

#include "gedit-theatrics-animated-widget.h"
#include "gedit-theatrics-enum-types.h"

#define GEDIT_THEATRICS_ANIMATED_WIDGET_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE((object), GEDIT_TYPE_THEATRICS_ANIMATED_WIDGET, GeditTheatricsAnimatedWidgetPrivate))

struct _GeditTheatricsAnimatedWidgetPrivate
{
	GeditTheatricsChoreographerEasing easing;
	GeditTheatricsChoreographerBlocking blocking;
	GeditTheatricsAnimationState animation_state;
	GtkOrientation orientation;
	guint duration;
	gdouble bias;
	gdouble percent;
	GtkAllocation widget_alloc;
};

enum
{
	PROP_0,
	PROP_EASING,
	PROP_BLOCKING,
	PROP_ANIMATION_STATE,
	PROP_DURATION,
	PROP_PERCENT,
	PROP_ORIENTATION
};

G_DEFINE_TYPE_EXTENDED (GeditTheatricsAnimatedWidget,
			gedit_theatrics_animated_widget,
			GEDIT_TYPE_OVERLAY_CHILD,
			0,
			G_IMPLEMENT_INTERFACE (GTK_TYPE_ORIENTABLE,
					       NULL))

static void
gedit_theatrics_animated_widget_finalize (GObject *object)
{
	G_OBJECT_CLASS (gedit_theatrics_animated_widget_parent_class)->finalize (object);
}

static void
gedit_theatrics_animated_widget_get_property (GObject    *object,
					      guint       prop_id,
					      GValue     *value,
					      GParamSpec *pspec)
{
	GeditTheatricsAnimatedWidget *aw = GEDIT_THEATRICS_ANIMATED_WIDGET (object);

	switch (prop_id)
	{
		case PROP_EASING:
			g_value_set_enum (value, aw->priv->easing);
			break;
		case PROP_BLOCKING:
			g_value_set_enum (value, aw->priv->blocking);
			break;
		case PROP_ANIMATION_STATE:
			g_value_set_enum (value, aw->priv->animation_state);
			break;
		case PROP_DURATION:
			g_value_set_uint (value, aw->priv->duration);
			break;
		case PROP_PERCENT:
			g_value_set_double (value, aw->priv->percent);
			break;
		case PROP_ORIENTATION:
			g_value_set_enum (value, aw->priv->orientation);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_theatrics_animated_widget_set_property (GObject      *object,
					      guint         prop_id,
					      const GValue *value,
					      GParamSpec   *pspec)
{
	GeditTheatricsAnimatedWidget *aw = GEDIT_THEATRICS_ANIMATED_WIDGET (object);

	switch (prop_id)
	{
		case PROP_EASING:
			gedit_theatrics_animated_widget_set_easing (aw,
			                                            g_value_get_enum (value));
			break;
		case PROP_BLOCKING:
			gedit_theatrics_animated_widget_set_blocking (aw,
			                                              g_value_get_enum (value));
			break;
		case PROP_ANIMATION_STATE:
			gedit_theatrics_animated_widget_set_animation_state (aw,
			                                                     g_value_get_enum (value));
			break;
		case PROP_DURATION:
			gedit_theatrics_animated_widget_set_duration (aw,
			                                              g_value_get_uint (value));
			break;
		case PROP_PERCENT:
			gedit_theatrics_animated_widget_set_percent (aw,
			                                             g_value_get_double (value));
			break;
		case PROP_ORIENTATION:
			aw->priv->orientation = g_value_get_enum (value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_theatrics_animated_widget_realize (GtkWidget *widget)
{
	GdkWindowAttr attributes;
	GdkWindow *parent_window;
	GdkWindow *window;
	GtkStyleContext *context;

	gtk_widget_set_realized (widget, TRUE);

	parent_window = gtk_widget_get_parent_window (widget);
	context = gtk_widget_get_style_context (widget);

	attributes.window_type = GDK_WINDOW_CHILD;
	attributes.wclass = GDK_INPUT_OUTPUT;
	attributes.event_mask = GDK_EXPOSURE_MASK;

	window = gdk_window_new (parent_window, &attributes, 0);
	gdk_window_set_user_data (window, widget);
	gtk_widget_set_window (widget, window);
	gtk_style_context_set_state (context, GTK_STATE_FLAG_NORMAL);
	gtk_style_context_set_background (context, window);
}

static void
gedit_theatrics_animated_widget_get_preferred_width (GtkWidget *widget,
                                                     gint      *minimum,
                                                     gint      *natural)
{
	GeditTheatricsAnimatedWidget *aw = GEDIT_THEATRICS_ANIMATED_WIDGET (widget);
	GtkWidget *child;
	gint width;

	child = gtk_bin_get_child (GTK_BIN (widget));

	if (child != NULL)
	{
		gint child_min, child_nat;

		gtk_widget_get_preferred_width (child, &child_min, &child_nat);
		aw->priv->widget_alloc.width = child_min;
	}

	if (aw->priv->orientation == GTK_ORIENTATION_HORIZONTAL)
	{
		width = gedit_theatrics_choreographer_pixel_compose (aw->priv->percent,
		                                                     aw->priv->widget_alloc.width,
		                                                     aw->priv->easing);
	}
	else
	{
		width = aw->priv->widget_alloc.width;
	}

	*minimum = *natural = width;
}

static void
gedit_theatrics_animated_widget_get_preferred_height (GtkWidget *widget,
                                                      gint      *minimum,
                                                      gint      *natural)
{
	GeditTheatricsAnimatedWidget *aw = GEDIT_THEATRICS_ANIMATED_WIDGET (widget);
	GtkWidget *child;
	gint height;

	child = gtk_bin_get_child (GTK_BIN (widget));

	if (child != NULL)
	{
		gint child_min, child_nat;

		gtk_widget_get_preferred_height (child, &child_min, &child_nat);
		aw->priv->widget_alloc.height = child_min;
	}

	if (aw->priv->orientation == GTK_ORIENTATION_HORIZONTAL)
	{
		height = aw->priv->widget_alloc.height;
	}
	else
	{
		height = gedit_theatrics_choreographer_pixel_compose (aw->priv->percent,
		                                                      aw->priv->widget_alloc.height,
		                                                      aw->priv->easing);
	}

	*minimum = *natural = height;
}

static void
gedit_theatrics_animated_widget_size_allocate (GtkWidget     *widget,
					       GtkAllocation *allocation)
{
	GeditTheatricsAnimatedWidget *aw = GEDIT_THEATRICS_ANIMATED_WIDGET (widget);
	GtkWidget *child;

	//GTK_WIDGET_CLASS (gedit_theatrics_animated_widget_parent_class)->size_allocate (widget, allocation);

	child = gtk_bin_get_child (GTK_BIN (widget));

	if (child != NULL)
	{
		if (aw->priv->orientation == GTK_ORIENTATION_HORIZONTAL)
		{
			aw->priv->widget_alloc.height = allocation->height;
			aw->priv->widget_alloc.x = 0;

			if (aw->priv->blocking == GEDIT_THEATRICS_CHOREOGRAPHER_BLOCKING_DOWNSTAGE)
			{
				aw->priv->widget_alloc.x = allocation->width - aw->priv->widget_alloc.width;
			}
		}
		else
		{
			aw->priv->widget_alloc.width = allocation->width;
			aw->priv->widget_alloc.y = 0;

			if (aw->priv->blocking == GEDIT_THEATRICS_CHOREOGRAPHER_BLOCKING_DOWNSTAGE)
			{
				aw->priv->widget_alloc.y = allocation->height - aw->priv->widget_alloc.height;
			}
		}

		if (aw->priv->widget_alloc.height > 0 && aw->priv->widget_alloc.width > 0)
		{
			gtk_widget_size_allocate (child, &aw->priv->widget_alloc);
		}
	}
}

static void
gedit_theatrics_animated_widget_class_init (GeditTheatricsAnimatedWidgetClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
	
	object_class->finalize = gedit_theatrics_animated_widget_finalize;
	object_class->get_property = gedit_theatrics_animated_widget_get_property;
	object_class->set_property = gedit_theatrics_animated_widget_set_property;

	widget_class->realize = gedit_theatrics_animated_widget_realize;
	widget_class->get_preferred_width = gedit_theatrics_animated_widget_get_preferred_width;
	widget_class->get_preferred_height = gedit_theatrics_animated_widget_get_preferred_height;
	widget_class->size_allocate = gedit_theatrics_animated_widget_size_allocate;

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

	g_object_class_install_property (object_class, PROP_PERCENT,
	                                 g_param_spec_double ("percent",
	                                                      "Percent",
	                                                      "The percent",
	                                                      0,
	                                                      G_MAXDOUBLE,
	                                                      0.0,
	                                                      G_PARAM_READWRITE |
	                                                      G_PARAM_CONSTRUCT |
	                                                      G_PARAM_STATIC_STRINGS));

	g_object_class_override_property (object_class,
	                                  PROP_ORIENTATION,
	                                  "orientation");

	g_type_class_add_private (object_class, sizeof (GeditTheatricsAnimatedWidgetPrivate));
}

static void
gedit_theatrics_animated_widget_init (GeditTheatricsAnimatedWidget *aw)
{
	aw->priv = GEDIT_THEATRICS_ANIMATED_WIDGET_GET_PRIVATE (aw);

	aw->priv->orientation = GTK_ORIENTATION_HORIZONTAL;
	aw->priv->bias = 1.0;
}

GeditTheatricsAnimatedWidget *
gedit_theatrics_animated_widget_new (GtkWidget                          *widget,
				     guint                               duration,
				     GeditTheatricsChoreographerEasing   easing,
				     GeditTheatricsChoreographerBlocking blocking,
				     GtkOrientation                      orientation)
{
	return g_object_new (GEDIT_TYPE_THEATRICS_ANIMATED_WIDGET,
			     "widget", widget,
			     "duration", duration,
			     "easing", easing,
			     "blocking", blocking,
			     "orientation", orientation,
			     NULL);
}

GeditTheatricsChoreographerEasing
gedit_theatrics_animated_widget_get_easing (GeditTheatricsAnimatedWidget *aw)
{
	g_return_val_if_fail (GEDIT_IS_THEATRICS_ANIMATED_WIDGET (aw), GEDIT_THEATRICS_CHOREOGRAPHER_EASING_LINEAR);

	return aw->priv->easing;
}

void
gedit_theatrics_animated_widget_set_easing (GeditTheatricsAnimatedWidget     *aw,
					    GeditTheatricsChoreographerEasing easing)
{
	g_return_if_fail (GEDIT_IS_THEATRICS_ANIMATED_WIDGET (aw));

	if (aw->priv->easing != easing)
	{
		aw->priv->easing = easing;

		g_object_notify (G_OBJECT (aw), "easing");
	}
}

GeditTheatricsChoreographerBlocking
gedit_theatrics_animated_widget_get_blocking (GeditTheatricsAnimatedWidget *aw)
{
	g_return_val_if_fail (GEDIT_IS_THEATRICS_ANIMATED_WIDGET (aw), GEDIT_THEATRICS_CHOREOGRAPHER_BLOCKING_DOWNSTAGE);

	return aw->priv->blocking;
}

void
gedit_theatrics_animated_widget_set_blocking (GeditTheatricsAnimatedWidget       *aw,
					      GeditTheatricsChoreographerBlocking blocking)
{
	g_return_if_fail (GEDIT_IS_THEATRICS_ANIMATED_WIDGET (aw));

	if (aw->priv->blocking != blocking)
	{
		aw->priv->blocking = blocking;

		g_object_notify (G_OBJECT (aw), "blocking");
	}
}

GeditTheatricsAnimationState
gedit_theatrics_animated_widget_get_animation_state (GeditTheatricsAnimatedWidget *aw)
{
	g_return_val_if_fail (GEDIT_IS_THEATRICS_ANIMATED_WIDGET (aw), GEDIT_THEATRICS_ANIMATION_STATE_COMING);

	return aw->priv->animation_state;
}

void
gedit_theatrics_animated_widget_set_animation_state (GeditTheatricsAnimatedWidget *aw,
						     GeditTheatricsAnimationState  animation_state)
{
	g_return_if_fail (GEDIT_IS_THEATRICS_ANIMATED_WIDGET (aw));

	if (aw->priv->animation_state != animation_state)
	{
		aw->priv->animation_state = animation_state;

		g_object_notify (G_OBJECT (aw), "animation-state");
	}
}

guint
gedit_theatrics_animated_widget_get_duration (GeditTheatricsAnimatedWidget *aw)
{
	g_return_val_if_fail (GEDIT_IS_THEATRICS_ANIMATED_WIDGET (aw), 0);

	return aw->priv->duration;
}

void
gedit_theatrics_animated_widget_set_duration (GeditTheatricsAnimatedWidget *aw,
					      guint                         duration)
{
	g_return_if_fail (GEDIT_IS_THEATRICS_ANIMATED_WIDGET (aw));

	if (aw->priv->duration != duration)
	{
		aw->priv->duration = duration;

		g_object_notify (G_OBJECT (aw), "duration");
	}
}

gdouble
gedit_theatrics_animated_widget_get_percent (GeditTheatricsAnimatedWidget *aw)
{
	g_return_val_if_fail (GEDIT_IS_THEATRICS_ANIMATED_WIDGET (aw), 0.0);

	return aw->priv->duration;
}

void
gedit_theatrics_animated_widget_set_percent (GeditTheatricsAnimatedWidget *aw,
					     gdouble                       percent)
{
	g_return_if_fail (GEDIT_IS_THEATRICS_ANIMATED_WIDGET (aw));

	if (aw->priv->percent != percent)
	{
		aw->priv->percent = aw->priv->bias * percent;

		g_object_notify (G_OBJECT (aw), "percent");

		gtk_widget_queue_resize_no_redraw (GTK_WIDGET (aw));
	}
}

void
gedit_theatrics_animated_widget_set_bias (GeditTheatricsAnimatedWidget *aw,
                                          gdouble                       bias)
{
	g_return_if_fail (GEDIT_IS_THEATRICS_ANIMATED_WIDGET (aw));

	aw->priv->bias = bias;
}

/* ex:set ts=8 noet: */
