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
	GtkWidget *widget;
	GeditTheatricsChoreographerEasing easing;
	GeditTheatricsChoreographerBlocking blocking;
	GeditTheatricsAnimationState animation_state;
	GtkOrientation orientation;
	guint duration;
	gdouble bias;
	gdouble percent;
	GtkAllocation widget_alloc;

	cairo_surface_t *surface;

	gint width;
	gint height;
	gint start_padding;
	gint end_padding;
};

enum
{
	PROP_0,
	PROP_WIDGET,
	PROP_EASING,
	PROP_BLOCKING,
	PROP_ANIMATION_STATE,
	PROP_DURATION,
	PROP_PERCENT,
	PROP_ORIENTATION
};

enum
{
	REMOVE_CORE,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE_EXTENDED (GeditTheatricsAnimatedWidget,
			gedit_theatrics_animated_widget,
			GTK_TYPE_BIN,
			0,
			G_IMPLEMENT_INTERFACE (GTK_TYPE_ORIENTABLE,
					       NULL))

static void
on_widget_destroyed (GtkWidget                    *widget,
		     GeditTheatricsAnimatedWidget *aw)
{
	GdkWindow *window;
	cairo_t *img_cr;
	cairo_t *cr;
	cairo_surface_t *surface;

	if (!gtk_widget_get_realized (GTK_WIDGET (aw)))
		return;

	aw->priv->width = aw->priv->widget_alloc.width;
	aw->priv->height = aw->priv->widget_alloc.height;

	aw->priv->surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
							aw->priv->width,
							aw->priv->height);

	img_cr = cairo_create (aw->priv->surface);

	window = gtk_widget_get_window (GTK_WIDGET (aw));

	cr = gdk_cairo_create (GDK_DRAWABLE (window));
	surface = cairo_get_target (cr);

	cairo_set_source_surface (img_cr, surface,
	                          aw->priv->widget_alloc.x,
	                          aw->priv->widget_alloc.y);

	cairo_paint (img_cr);

	cairo_destroy (img_cr);
	cairo_destroy (cr);

	if (aw->priv->animation_state != GEDIT_THEATRICS_ANIMATION_STATE_GOING)
	{
		g_signal_emit (G_OBJECT (aw), signals[REMOVE_CORE], 0);
	}
}

static void
set_widget (GeditTheatricsAnimatedWidget *aw,
	    GtkWidget                    *widget)
{
	if (widget == NULL)
		return;

	aw->priv->widget = widget;
	gtk_widget_set_parent (widget, GTK_WIDGET (aw));

	g_signal_connect (widget, "destroy",
			  G_CALLBACK (on_widget_destroyed),
			  aw);
}

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
		case PROP_WIDGET:
			g_value_set_object (value, aw->priv->widget);
			break;
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
		case PROP_WIDGET:
		{
			set_widget (aw, g_value_get_object (value));
			break;
		}
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
	GtkStyle *style;

	gtk_widget_set_realized (widget, TRUE);

	parent_window = gtk_widget_get_parent_window (widget);
	style = gtk_widget_get_style (widget);

	attributes.window_type = GDK_WINDOW_CHILD;
	attributes.wclass = GDK_INPUT_OUTPUT;
	attributes.event_mask = GDK_EXPOSURE_MASK;

	window = gdk_window_new (parent_window, &attributes, 0);
	gdk_window_set_user_data (window, widget);
	gtk_widget_set_window (widget, window);
	gdk_window_set_back_pixmap (window, NULL, FALSE);
	style = gtk_style_attach (style, window);
	gtk_widget_set_style (widget, style);
	gtk_style_set_background (style, window, GTK_STATE_NORMAL);
}

static void
gedit_theatrics_animated_widget_size_request (GtkWidget      *widget,
					      GtkRequisition *requisition)
{
	GeditTheatricsAnimatedWidget *aw = GEDIT_THEATRICS_ANIMATED_WIDGET (widget);
	gint width;
	gint height;

	if (aw->priv->widget != NULL)
	{
		GtkRequisition req;

		gtk_widget_size_request (aw->priv->widget, &req);
		aw->priv->widget_alloc.width = req.width;
		aw->priv->widget_alloc.height = req.height;
	}

	if (aw->priv->orientation == GTK_ORIENTATION_HORIZONTAL)
	{
		width = gedit_theatrics_choreographer_pixel_compose (aw->priv->percent,
								     aw->priv->widget_alloc.width +
								     aw->priv->start_padding +
								     aw->priv->end_padding,
								     aw->priv->easing);
		height = aw->priv->widget_alloc.height;
	}
	else
	{
		width = aw->priv->widget_alloc.width;
		height = gedit_theatrics_choreographer_pixel_compose (aw->priv->percent,
								      aw->priv->widget_alloc.height +
								      aw->priv->start_padding +
								      aw->priv->end_padding,
								      aw->priv->easing);
	}

	requisition->width = width;
	requisition->height = height;
}

static void
gedit_theatrics_animated_widget_size_allocate (GtkWidget     *widget,
					       GtkAllocation *allocation)
{
	GeditTheatricsAnimatedWidget *aw = GEDIT_THEATRICS_ANIMATED_WIDGET (widget);

	GTK_WIDGET_CLASS (gedit_theatrics_animated_widget_parent_class)->size_allocate (widget, allocation);

	if (aw->priv->widget != NULL)
	{
		if (aw->priv->orientation == GTK_ORIENTATION_HORIZONTAL)
		{
			aw->priv->widget_alloc.height = allocation->height;
			aw->priv->widget_alloc.x = aw->priv->start_padding;

			if (aw->priv->blocking == GEDIT_THEATRICS_CHOREOGRAPHER_BLOCKING_DOWNSTAGE)
			{
				aw->priv->widget_alloc.x += allocation->width - aw->priv->widget_alloc.width;
			}
		}
		else
		{
			aw->priv->widget_alloc.width = allocation->width;
			aw->priv->widget_alloc.y = aw->priv->start_padding;

			if (aw->priv->blocking == GEDIT_THEATRICS_CHOREOGRAPHER_BLOCKING_DOWNSTAGE)
			{
				aw->priv->widget_alloc.y = allocation->height - aw->priv->widget_alloc.height;
			}
		}

		if (aw->priv->widget_alloc.height > 0 && aw->priv->widget_alloc.width > 0)
		{
			gtk_widget_size_allocate (aw->priv->widget,
						  &aw->priv->widget_alloc);
		}
	}
}

static gboolean
gedit_theatrics_animated_widget_expose_event (GtkWidget      *widget,
					      GdkEventExpose *event)
{
	GeditTheatricsAnimatedWidget *aw = GEDIT_THEATRICS_ANIMATED_WIDGET (widget);

	if (aw->priv->surface != NULL)
	{
		/* Do not scale if the size is 0 */
		if (aw->priv->width > 0 && aw->priv->height > 0)
		{
			cairo_t *cr;
			cr = gdk_cairo_create (event->window);

			cairo_scale (cr,
				     aw->priv->widget_alloc.width / aw->priv->width,
				     aw->priv->widget_alloc.height / aw->priv->height);
			cairo_set_source_surface (cr, aw->priv->surface, 0, 0);

			cairo_paint (cr);
			cairo_destroy (cr);
		}

		return TRUE;
	}
	else
	{
		return GTK_WIDGET_CLASS (gedit_theatrics_animated_widget_parent_class)->expose_event (widget, event);
	}
}

static void
gedit_theatrics_animated_widget_remove (GtkContainer *container,
					GtkWidget    *widget)
{
	GeditTheatricsAnimatedWidget *aw = GEDIT_THEATRICS_ANIMATED_WIDGET (container);

	gtk_widget_unparent (widget);
	aw->priv->widget = NULL;
}

static void
gedit_theatrics_animated_widget_forall (GtkContainer *container,
					gboolean      include_internals,
					GtkCallback   callback,
					gpointer      callback_data)
{
	GeditTheatricsAnimatedWidget *aw = GEDIT_THEATRICS_ANIMATED_WIDGET (container);

	if (aw->priv->widget != NULL)
	{
		callback (aw->priv->widget, callback_data);
	}
}

static void
gedit_theatrics_animated_widget_class_init (GeditTheatricsAnimatedWidgetClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
	GtkContainerClass *container_class = GTK_CONTAINER_CLASS (klass);
	
	object_class->finalize = gedit_theatrics_animated_widget_finalize;
	object_class->get_property = gedit_theatrics_animated_widget_get_property;
	object_class->set_property = gedit_theatrics_animated_widget_set_property;

	widget_class->realize = gedit_theatrics_animated_widget_realize;
	widget_class->size_request = gedit_theatrics_animated_widget_size_request;
	widget_class->size_allocate = gedit_theatrics_animated_widget_size_allocate;
	widget_class->expose_event = gedit_theatrics_animated_widget_expose_event;

	container_class->remove = gedit_theatrics_animated_widget_remove;
	container_class->forall = gedit_theatrics_animated_widget_forall;

	g_object_class_install_property (object_class, PROP_WIDGET,
	                                 g_param_spec_object ("widget",
	                                                      "Widget",
	                                                      "The Widget",
	                                                      GTK_TYPE_WIDGET,
	                                                      G_PARAM_READWRITE |
	                                                      G_PARAM_CONSTRUCT_ONLY |
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

	signals[REMOVE_CORE] =
		g_signal_new ("remove-core",
		              G_OBJECT_CLASS_TYPE (object_class),
		              G_SIGNAL_RUN_LAST,
		              G_STRUCT_OFFSET (GeditTheatricsAnimatedWidgetClass, remove_core),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__VOID,
		              G_TYPE_NONE,
		              0);

	g_type_class_add_private (object_class, sizeof (GeditTheatricsAnimatedWidgetPrivate));
}

static void
gedit_theatrics_animated_widget_init (GeditTheatricsAnimatedWidget *aw)
{
	aw->priv = GEDIT_THEATRICS_ANIMATED_WIDGET_GET_PRIVATE (aw);

	gtk_widget_set_has_window (GTK_WIDGET (aw), TRUE);

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

GtkWidget *
gedit_theatrics_animated_widget_get_widget (GeditTheatricsAnimatedWidget *aw)
{
	g_return_val_if_fail (GEDIT_IS_THEATRICS_ANIMATED_WIDGET (aw), NULL);

	return aw->priv->widget;
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

void
gedit_theatrics_animated_widget_set_end_padding (GeditTheatricsAnimatedWidget *aw,
                                                 gint                          end_padding)
{
	g_return_if_fail (GEDIT_IS_THEATRICS_ANIMATED_WIDGET (aw));

	aw->priv->end_padding = end_padding;
}

/* ex:ts=8:noet: */
