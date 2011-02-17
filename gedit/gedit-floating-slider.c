/*
 * gedit-floating-slider.c
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

struct _GeditFloatingSliderPrivate
{
	GtkAllocation widget_alloc;
	GeditTheatricsChoreographerEasing easing;
	GeditTheatricsChoreographerBlocking blocking;
	GeditTheatricsAnimationState animation_state;
	GtkOrientation orientation;
	guint duration;
	gdouble bias;
	gdouble percent;
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

G_DEFINE_TYPE_EXTENDED (GeditFloatingSlider,
			gedit_floating_slider,
			GEDIT_TYPE_OVERLAY_CHILD,
			0,
			G_IMPLEMENT_INTERFACE (GEDIT_TYPE_ANIMATABLE,
			                       NULL)
			G_IMPLEMENT_INTERFACE (GTK_TYPE_ORIENTABLE,
					       NULL))

static void
gedit_floating_slider_finalize (GObject *object)
{
	G_OBJECT_CLASS (gedit_floating_slider_parent_class)->finalize (object);
}

static void
gedit_floating_slider_get_property (GObject    *object,
                                    guint       prop_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
	GeditFloatingSliderPrivate *priv = GEDIT_FLOATING_SLIDER (object)->priv;

	switch (prop_id)
	{
		case PROP_EASING:
			g_value_set_enum (value, priv->easing);
			break;
		case PROP_BLOCKING:
			g_value_set_enum (value, priv->blocking);
			break;
		case PROP_ANIMATION_STATE:
			g_value_set_enum (value, priv->animation_state);
			break;
		case PROP_DURATION:
			g_value_set_uint (value, priv->duration);
			break;
		case PROP_PERCENT:
			g_value_set_double (value, priv->percent);
			break;
		case PROP_ORIENTATION:
			g_value_set_enum (value, priv->orientation);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_floating_slider_set_property (GObject      *object,
                                    guint         prop_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
	GeditFloatingSliderPrivate *priv = GEDIT_FLOATING_SLIDER (object)->priv;

	switch (prop_id)
	{
		case PROP_EASING:
			priv->easing = g_value_get_enum (value);
			break;
		case PROP_BLOCKING:
			priv->blocking = g_value_get_enum (value);
			break;
		case PROP_ANIMATION_STATE:
			priv->animation_state = g_value_get_enum (value);
			break;
		case PROP_DURATION:
			priv->duration = g_value_get_uint (value);
			break;
		case PROP_PERCENT:
			priv->percent = g_value_get_double (value);
			break;
		case PROP_ORIENTATION:
			priv->orientation = g_value_get_enum (value);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_floating_slider_get_preferred_width (GtkWidget *widget,
                                           gint      *minimum,
                                           gint      *natural)
{
	GeditFloatingSliderPrivate *priv = GEDIT_FLOATING_SLIDER (widget)->priv;
	GtkWidget *child;
	gint width;

	GTK_WIDGET_CLASS (gedit_floating_slider_parent_class)->get_preferred_width (widget, minimum, natural);

	child = gtk_bin_get_child (GTK_BIN (widget));

	if (child != NULL)
	{
		gint child_min, child_nat;

		gtk_widget_get_preferred_width (child, &child_min, &child_nat);
		priv->widget_alloc.width = child_min;
	}

	if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
	{
		width = gedit_theatrics_choreographer_pixel_compose (priv->percent,
		                                                     priv->widget_alloc.width,
		                                                     priv->easing);
	}
	else
	{
		width = priv->widget_alloc.width;
	}

	*minimum = *natural = width;
}

static void
gedit_floating_slider_get_preferred_height (GtkWidget *widget,
                                            gint      *minimum,
                                            gint      *natural)
{
	GeditFloatingSliderPrivate *priv = GEDIT_FLOATING_SLIDER (widget)->priv;
	GtkWidget *child;
	gint height;

	GTK_WIDGET_CLASS (gedit_floating_slider_parent_class)->get_preferred_height (widget, minimum, natural);

	child = gtk_bin_get_child (GTK_BIN (widget));

	if (child != NULL)
	{
		gint child_min, child_nat;

		gtk_widget_get_preferred_height (child, &child_min, &child_nat);
		priv->widget_alloc.height = child_min;
	}

	if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
	{
		height = priv->widget_alloc.height;
	}
	else
	{
		height = gedit_theatrics_choreographer_pixel_compose (priv->percent,
		                                                      priv->widget_alloc.height,
		                                                      priv->easing);
	}

	*minimum = *natural = height;
}

static void
gedit_floating_slider_size_allocate (GtkWidget     *widget,
                                     GtkAllocation *allocation)
{
	GeditFloatingSliderPrivate *priv = GEDIT_FLOATING_SLIDER (widget)->priv;
	GtkWidget *child;

	GTK_WIDGET_CLASS (gedit_floating_slider_parent_class)->size_allocate (widget, allocation);

	child = gtk_bin_get_child (GTK_BIN (widget));

	if (child != NULL)
	{
		if (priv->orientation == GTK_ORIENTATION_HORIZONTAL)
		{
			priv->widget_alloc.height = allocation->height;
			priv->widget_alloc.x = 0;

			if (priv->blocking == GEDIT_THEATRICS_CHOREOGRAPHER_BLOCKING_DOWNSTAGE)
			{
				priv->widget_alloc.x = allocation->width - priv->widget_alloc.width;
			}
		}
		else
		{
			priv->widget_alloc.width = allocation->width;
			priv->widget_alloc.y = 0;

			if (priv->blocking == GEDIT_THEATRICS_CHOREOGRAPHER_BLOCKING_DOWNSTAGE)
			{
				priv->widget_alloc.y = allocation->height - priv->widget_alloc.height;
			}
		}

		if (priv->widget_alloc.height > 0 && priv->widget_alloc.width > 0)
		{
			gtk_widget_size_allocate (child, &priv->widget_alloc);
		}
	}
}

static void
gedit_floating_slider_class_init (GeditFloatingSliderClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
	
	object_class->finalize = gedit_floating_slider_finalize;
	object_class->get_property = gedit_floating_slider_get_property;
	object_class->set_property = gedit_floating_slider_set_property;

	widget_class->get_preferred_width = gedit_floating_slider_get_preferred_width;
	widget_class->get_preferred_height = gedit_floating_slider_get_preferred_height;
	widget_class->size_allocate = gedit_floating_slider_size_allocate;

	g_object_class_override_property (object_class,
	                                  PROP_EASING,
	                                  "easing");

	g_object_class_override_property (object_class,
	                                  PROP_BLOCKING,
	                                  "blocking");

	g_object_class_override_property (object_class,
	                                  PROP_ANIMATION_STATE,
	                                  "animation-state");

	g_object_class_override_property (object_class,
	                                  PROP_DURATION,
	                                  "duration");

	g_object_class_override_property (object_class,
	                                  PROP_PERCENT,
	                                  "percent");

	g_object_class_override_property (object_class,
	                                  PROP_ORIENTATION,
	                                  "orientation");

	g_object_class_override_property (object_class,
	                                  PROP_ORIENTATION,
	                                  "orientation");

	g_type_class_add_private (object_class, sizeof (GeditFloatingSliderPrivate));
}

static void
gedit_floating_slider_init (GeditFloatingSlider *slider)
{
	slider->priv = G_TYPE_INSTANCE_GET_PRIVATE (slider,
	                                            GEDIT_TYPE_FLOATING_SLIDER,
	                                            GeditFloatingSliderPrivate);

	slider->priv->orientation = GTK_ORIENTATION_VERTICAL;
	slider->priv->bias = 1.0;
}

GeditFloatingSlider *
gedit_floating_slider_new (GtkWidget *widget)
{
	return g_object_new (GEDIT_TYPE_FLOATING_SLIDER,
	                     "widget", widget,
	                     NULL);
}

/* ex:set ts=8 noet: */
