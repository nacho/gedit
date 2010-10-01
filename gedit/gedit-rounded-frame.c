/*
 * gedit-rounded-frame.c
 * This file is part of gedit
 *
 * Copyright (C) 2010 - Ignacio Casal Quinteiro
 *
 * Work based on Aaron Bockover <abockover@novell.com>
 *               Gabriel Burt <gburt@novell.com>
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

#include "gedit-rounded-frame.h"
#include "theatrics/gedit-theatrics-utils.h"


#define GEDIT_ROUNDED_FRAME_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE((object), GEDIT_TYPE_ROUNDED_FRAME, GeditRoundedFramePrivate))

struct _GeditRoundedFramePrivate
{
	GtkWidget *child;

	GtkAllocation child_allocation;
	guint frame_width;
};

G_DEFINE_TYPE (GeditRoundedFrame, gedit_rounded_frame, GTK_TYPE_BIN)

static void
gedit_rounded_frame_finalize (GObject *object)
{
	G_OBJECT_CLASS (gedit_rounded_frame_parent_class)->finalize (object);
}

static void
gedit_rounded_frame_size_request (GtkWidget      *widget,
                                  GtkRequisition *requisition)
{
	GeditRoundedFrame *frame = GEDIT_ROUNDED_FRAME (widget);
	gint border_width;

	if (frame->priv->child != NULL &&
	    gtk_widget_get_visible (frame->priv->child))
	{
		GtkRequisition child_requisition;

		/* Add the child's width/height */
		gtk_widget_get_preferred_size (frame->priv->child,
		                               &child_requisition, NULL);

		requisition->width = MAX (0, child_requisition.width);
		requisition->height = child_requisition.height;
	}
	else
	{
		requisition->width = 0;
		requisition->height = 0;
	}

	/* Add the border */
	border_width = gtk_container_get_border_width (GTK_CONTAINER (widget));
	requisition->width += (border_width + frame->priv->frame_width) * 2;
	requisition->height += (border_width + frame->priv->frame_width) * 2;
}

static void
gedit_rounded_frame_size_allocate (GtkWidget     *widget,
                                   GtkAllocation *allocation)
{
	GeditRoundedFrame *frame = GEDIT_ROUNDED_FRAME (widget);
	GtkAllocation *child_allocation;
	int border;

	child_allocation = &frame->priv->child_allocation;

	GTK_WIDGET_CLASS (gedit_rounded_frame_parent_class)->size_allocate (widget, allocation);

	if (frame->priv->child == NULL ||
	    !gtk_widget_get_visible (frame->priv->child))
	{
		return;
	}

	border = gtk_container_get_border_width (GTK_CONTAINER (widget)) + 
	         frame->priv->frame_width;
	child_allocation->x = allocation->x + border;
	child_allocation->y = allocation->y + border;
	child_allocation->width = MAX (1, allocation->width - border * 2);
	child_allocation->height = MAX (1, allocation->height - border * 2);

	gtk_widget_size_allocate (frame->priv->child, child_allocation);
}

static void
draw_frame (GeditRoundedFrame *frame,
            cairo_t           *cr,
            GdkRectangle      *area)
{
	GtkStyle *style;
	GdkColor bg_color;
	GdkColor border_color;

	gedit_theatrics_utils_draw_round_rectangle (cr,
	                                            FALSE,
	                                            FALSE,
	                                            TRUE,
	                                            TRUE,
	                                            area->x,
	                                            area->y,
	                                            MIN (0.30 * area->width, 0.30 * area->height),
	                                            area->width,
	                                            area->height);

	style = gtk_widget_get_style (GTK_WIDGET (frame));
	bg_color = style->bg[GTK_STATE_NORMAL];
	border_color = style->dark[GTK_STATE_ACTIVE];

	gdk_cairo_set_source_color (cr, &bg_color);
	cairo_fill_preserve (cr);

	gdk_cairo_set_source_color (cr, &border_color);
	cairo_set_line_width (cr, frame->priv->frame_width / 2);
	cairo_stroke (cr);
}

static gboolean
gedit_rounded_frame_draw (GtkWidget      *widget,
                          cairo_t        *cr)
{
	GeditRoundedFrame *frame = GEDIT_ROUNDED_FRAME (widget);
	GdkRectangle area;

	if (!gtk_widget_is_drawable (widget))
	{
		return FALSE;
	}

	area.x = frame->priv->child_allocation.x - frame->priv->frame_width;
	area.y = frame->priv->child_allocation.y - 2 * frame->priv->frame_width - 1;
	area.width = frame->priv->child_allocation.width + 2 * frame->priv->frame_width;
	area.height = frame->priv->child_allocation.height + 3 * frame->priv->frame_width;

	draw_frame (frame, cr, &area);

	return GTK_WIDGET_CLASS (gedit_rounded_frame_parent_class)->draw (widget, cr);
}

static void
gedit_rounded_frame_add (GtkContainer *container,
                         GtkWidget    *widget)
{
	GeditRoundedFrame *frame = GEDIT_ROUNDED_FRAME (container);

	frame->priv->child = widget;

	GTK_CONTAINER_CLASS (gedit_rounded_frame_parent_class)->add (container, widget);
}

static void
gedit_rounded_frame_remove (GtkContainer *container,
                            GtkWidget    *widget)
{
	GeditRoundedFrame *frame = GEDIT_ROUNDED_FRAME (container);

	if (frame->priv->child == widget)
	{
		frame->priv->child = NULL;
	}

	GTK_CONTAINER_CLASS (gedit_rounded_frame_parent_class)->remove (container, widget);
}

static void
gedit_rounded_frame_class_init (GeditRoundedFrameClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
	GtkContainerClass *container_class = GTK_CONTAINER_CLASS (klass);

	object_class->finalize = gedit_rounded_frame_finalize;

	widget_class->size_request = gedit_rounded_frame_size_request;
	widget_class->size_allocate = gedit_rounded_frame_size_allocate;
	widget_class->draw = gedit_rounded_frame_draw;

	container_class->add = gedit_rounded_frame_add;
	container_class->remove = gedit_rounded_frame_remove;

	g_type_class_add_private (object_class, sizeof (GeditRoundedFramePrivate));
}

static void
gedit_rounded_frame_init (GeditRoundedFrame *frame)
{
	frame->priv = GEDIT_ROUNDED_FRAME_GET_PRIVATE (frame);

	frame->priv->frame_width = 3; /* Make it a prop */

	gtk_widget_set_has_window (GTK_WIDGET (frame), FALSE);
	gtk_widget_set_app_paintable (GTK_WIDGET (frame), FALSE);
}

GtkWidget *
gedit_rounded_frame_new ()
{
	return GTK_WIDGET (g_object_new (GEDIT_TYPE_ROUNDED_FRAME, NULL));
}
