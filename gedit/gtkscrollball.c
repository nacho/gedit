/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include <math.h>
#include <stdio.h>
#include <gtk/gtkmain.h>
#include <gtk/gtksignal.h>

#include "gtkscrollball.h"

#define SCROLL_DELAY_LENGTH  300
#define SCROLLBALL_DEFAULT_SIZE 28

/* Forward declararations */

static void gtk_scrollball_class_init               (GtkScrollballClass    *klass);
static void gtk_scrollball_init                     (GtkScrollball         *scrollball);
static void gtk_scrollball_destroy                  (GtkObject        *object);
static void gtk_scrollball_realize                  (GtkWidget        *widget);
static void gtk_scrollball_size_request             (GtkWidget      *widget,
						     GtkRequisition *requisition);
static void gtk_scrollball_size_allocate            (GtkWidget     *widget,
						     GtkAllocation *allocation);
static gint gtk_scrollball_expose                   (GtkWidget        *widget,
						     GdkEventExpose   *event);
static gint gtk_scrollball_button_press             (GtkWidget *widget, GdkEventButton *event);
static gint gtk_scrollball_button_release           (GtkWidget *widget, GdkEventButton *event);
static gint gtk_scrollball_motion_notify            (GtkWidget *widget, GdkEventMotion *event);
static void gtk_scrollball_update                   (GtkScrollball *scball);
static void gtk_scrollball_adjustment_value_changed (GtkAdjustment *adjustment, gpointer *data);
static void gtk_scrollball_adjustment_changed       (GtkAdjustment *adjustment, gpointer *data);

/* Local data */

static GtkWidgetClass *parent_class = NULL;

guint gtk_scrollball_get_type () {
  static guint scrollball_type = 0;

  if (!scrollball_type)
    {
      GtkTypeInfo scrollball_info =
      {
	"GtkScrollball",
	sizeof (GtkScrollball),
	sizeof (GtkScrollballClass),
	(GtkClassInitFunc) gtk_scrollball_class_init,
	(GtkObjectInitFunc) gtk_scrollball_init,
	(GtkArgSetFunc) NULL,
	(GtkArgGetFunc) NULL,
      };

      scrollball_type = gtk_type_unique (gtk_widget_get_type (), &scrollball_info);
    }

  return scrollball_type;
}

static void gtk_scrollball_class_init (GtkScrollballClass *class) {
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;

  object_class = (GtkObjectClass*) class;
  widget_class = (GtkWidgetClass*) class;

  parent_class = gtk_type_class (gtk_widget_get_type ());

  object_class->destroy = gtk_scrollball_destroy;

  widget_class->realize = gtk_scrollball_realize;
  widget_class->expose_event = gtk_scrollball_expose;
  widget_class->size_request = gtk_scrollball_size_request;
  widget_class->size_allocate = gtk_scrollball_size_allocate;
  widget_class->button_press_event = gtk_scrollball_button_press;
  widget_class->motion_notify_event = gtk_scrollball_motion_notify;
  widget_class->button_release_event = gtk_scrollball_button_release;
}

static void gtk_scrollball_init (GtkScrollball *scrollball) {
  scrollball->button = 0;
  scrollball->hadjust = NULL;
  scrollball->vadjust = NULL;
}

GtkWidget* gtk_scrollball_new (GtkAdjustment *hadjust, GtkAdjustment *vadjust) {
  GtkScrollball *scrollball;

  scrollball = gtk_type_new (gtk_scrollball_get_type ());

  if (!hadjust)
    hadjust = (GtkAdjustment*) gtk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0);
  if (!vadjust)
    vadjust = (GtkAdjustment*) gtk_adjustment_new (0.0, 0.0, 0.0, 0.0, 0.0, 0.0);

  gtk_scrollball_set_hadjustment (scrollball, hadjust);
  gtk_scrollball_set_vadjustment (scrollball, vadjust);
  scrollball->acceleration=20;
  scrollball->xoff=0;
  scrollball->yoff=0;

  return GTK_WIDGET (scrollball);
}

static void gtk_scrollball_destroy (GtkObject *object) {
  GtkScrollball *scrollball;

  g_return_if_fail (object != NULL);
  g_return_if_fail (GTK_IS_SCROLLBALL (object));

  scrollball = GTK_SCROLLBALL (object);

  if (scrollball->hadjust)
    gtk_object_unref (GTK_OBJECT (scrollball->hadjust));
  if (scrollball->vadjust)
    gtk_object_unref (GTK_OBJECT (scrollball->vadjust));

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

GtkAdjustment* gtk_scrollball_get_hadjustment (GtkScrollball *scrollball) {
  g_return_val_if_fail (scrollball != NULL, NULL);
  g_return_val_if_fail (GTK_IS_SCROLLBALL (scrollball), NULL);

  return scrollball->hadjust;
}

GtkAdjustment* gtk_scrollball_get_vadjustment (GtkScrollball *scrollball) {
  g_return_val_if_fail (scrollball != NULL, NULL);
  g_return_val_if_fail (GTK_IS_SCROLLBALL (scrollball), NULL);

  return scrollball->vadjust;
}

void gtk_scrollball_set_hadjustment (GtkScrollball *scrollball, GtkAdjustment *hadjust) {
  g_return_if_fail (scrollball != NULL);
  g_return_if_fail (GTK_IS_SCROLLBALL (scrollball));

  if (hadjust) {
    if (scrollball->hadjust) {
	gtk_signal_disconnect_by_data (GTK_OBJECT (scrollball->hadjust), (gpointer) scrollball);
	gtk_object_unref (GTK_OBJECT (scrollball->hadjust));
    }

    scrollball->hadjust = hadjust;
    gtk_object_ref (GTK_OBJECT (scrollball->hadjust));
  }

  gtk_signal_connect (GTK_OBJECT (hadjust), "changed",
		      (GtkSignalFunc) gtk_scrollball_adjustment_changed,
                      (gpointer) scrollball);
  gtk_signal_connect (GTK_OBJECT (hadjust), "value_changed",
                      (GtkSignalFunc) gtk_scrollball_adjustment_value_changed,
                      (gpointer) scrollball);
}

void gtk_scrollball_set_vadjustment (GtkScrollball *scrollball, GtkAdjustment *vadjust) {
  g_return_if_fail (scrollball != NULL);
  g_return_if_fail (GTK_IS_SCROLLBALL (scrollball));

  if (vadjust) {
    if (scrollball->vadjust) {
	gtk_signal_disconnect_by_data (GTK_OBJECT (scrollball->vadjust), (gpointer) scrollball);
	gtk_object_unref (GTK_OBJECT (scrollball->vadjust));
    }

    scrollball->vadjust = vadjust;
    gtk_object_ref (GTK_OBJECT (scrollball->vadjust));
  }

  gtk_signal_connect (GTK_OBJECT (vadjust), "changed",
		      (GtkSignalFunc) gtk_scrollball_adjustment_changed,
                      (gpointer) scrollball);
  gtk_signal_connect (GTK_OBJECT (vadjust), "value_changed",
                      (GtkSignalFunc) gtk_scrollball_adjustment_value_changed,
                      (gpointer) scrollball);
}

static void
gtk_scrollball_realize (GtkWidget *widget)
{
  GtkScrollball *scrollball;
  GdkWindowAttr attributes;
  gint attributes_mask;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_SCROLLBALL (widget));

  GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);
  scrollball = GTK_SCROLLBALL (widget);

  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.event_mask = gtk_widget_get_events (widget) | GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | 
    GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK;
  attributes.visual = gtk_widget_get_visual (widget);
  attributes.colormap = gtk_widget_get_colormap (widget);

  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
  widget->window = gdk_window_new (widget->parent->window, &attributes, attributes_mask);

  widget->style = gtk_style_attach (widget->style, widget->window);

  gdk_window_set_user_data (widget->window, widget);

  gtk_style_set_background (widget->style, widget->window, GTK_STATE_ACTIVE);
}

static void gtk_scrollball_size_request (GtkWidget *widget, GtkRequisition *requisition) {
  requisition->width = SCROLLBALL_DEFAULT_SIZE;
  requisition->height = SCROLLBALL_DEFAULT_SIZE;
}

static void gtk_scrollball_size_allocate (GtkWidget *widget, GtkAllocation *allocation) {
  GtkScrollball *scrollball;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_SCROLLBALL (widget));
  g_return_if_fail (allocation != NULL);

  widget->allocation = *allocation;
  scrollball = GTK_SCROLLBALL (widget);

  if (GTK_WIDGET_REALIZED (widget))
    gdk_window_move_resize (widget->window, allocation->x, allocation->y,
			    allocation->width, allocation->height);
}

static gint gtk_scrollball_expose (GtkWidget *widget, GdkEventExpose *event) {
  gint hline, vline;
  gint xoff, yoff;

  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (GTK_IS_SCROLLBALL (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  if (event->count > 0)
    return FALSE;
  
  gdk_window_clear_area (widget->window, 0, 0, widget->allocation.width, widget->allocation.height);

  gdk_draw_rectangle (widget->window, widget->style->fg_gc[5], 1, 0, 0, 
		      widget->allocation.width, widget->allocation.height);

  /* Determine where to place everything */

  xoff = (GTK_SCROLLBALL(widget)->xoff = (int) (widget->allocation.width - 25)/2);
  yoff = (GTK_SCROLLBALL(widget)->yoff = (int) (widget->allocation.height - 25)/2);

  /* Draw our beautiful little arrows */

  gdk_draw_line (widget->window, widget->style->black_gc, xoff+10, yoff+2, xoff+13, yoff+5);
  gdk_draw_line (widget->window, widget->style->black_gc, xoff+10, yoff+2, xoff+7, yoff+5);
  gdk_draw_line (widget->window, widget->style->black_gc, xoff+7, yoff+5, xoff+13, yoff+5);

  gdk_draw_line (widget->window, widget->style->black_gc, xoff+2, yoff+10, xoff+5, yoff+13);
  gdk_draw_line (widget->window, widget->style->black_gc, xoff+2, yoff+10, xoff+5, yoff+7);
  gdk_draw_line (widget->window, widget->style->black_gc, xoff+5, yoff+7, xoff+5, yoff+13);

  gdk_draw_line (widget->window, widget->style->black_gc, xoff+10, yoff+18, xoff+13, yoff+15);
  gdk_draw_line (widget->window, widget->style->black_gc, xoff+10, yoff+18, xoff+7, yoff+15);
  gdk_draw_line (widget->window, widget->style->black_gc, xoff+7, yoff+15, xoff+13, yoff+15);

  gdk_draw_line (widget->window, widget->style->black_gc, xoff+18, yoff+10, xoff+15, yoff+13);
  gdk_draw_line (widget->window, widget->style->black_gc, xoff+18, yoff+10, xoff+15, yoff+7);
  gdk_draw_line (widget->window, widget->style->black_gc, xoff+15, yoff+7, xoff+15, yoff+13);

  /* The inner rectangles for the scrollbars */

  gdk_draw_rectangle (widget->window, widget->style->bg_gc[1], 1, xoff+3,yoff+22,17,4);
  gdk_draw_rectangle (widget->window, widget->style->bg_gc[1], 1, xoff+22,yoff+3,4,17);

  /* And the lines in the scrollbar thingies to indicate position */

  hline = ((GTK_SCROLLBALL(widget)->hadjust->value - GTK_SCROLLBALL(widget)->hadjust->lower)/
    (GTK_SCROLLBALL(widget)->hadjust->upper - GTK_SCROLLBALL(widget)->hadjust->lower -
     GTK_SCROLLBALL(widget)->hadjust->page_size))*17;
  gdk_draw_line (widget->window, widget->style->fg_gc[5], xoff+3+hline-1,yoff+22,
		 xoff+3+hline-1,yoff+25);
  gdk_draw_line (widget->window, widget->style->white_gc, xoff+3+hline,yoff+22,xoff+3+hline,yoff+25);
  gdk_draw_line (widget->window, widget->style->fg_gc[5], xoff+3+hline+1,yoff+22,
		 xoff+3+hline+1,yoff+25);

  vline = ((GTK_SCROLLBALL(widget)->vadjust->value - GTK_SCROLLBALL(widget)->vadjust->lower)/
    (GTK_SCROLLBALL(widget)->vadjust->upper - GTK_SCROLLBALL(widget)->vadjust->lower))*17;
  gdk_draw_line (widget->window, widget->style->fg_gc[5], xoff+22,yoff+3+vline-1,xoff+25,
		 yoff+3+vline-1);
  gdk_draw_line (widget->window, widget->style->white_gc, xoff+22,yoff+3+vline,xoff+25,yoff+3+vline);
  gdk_draw_line (widget->window, widget->style->fg_gc[5], xoff+22,yoff+3+vline+1,xoff+25,
		 yoff+3+vline+1);

  /* And the scrollbar thingie outlines, drawn after to make sure our lines don't overflow */

  gdk_draw_rectangle (widget->window, widget->style->fg_gc[4], 0, xoff+2,yoff+21,18,5);

  gdk_draw_rectangle (widget->window, widget->style->fg_gc[4], 0, xoff+21,yoff+2,5,18);


  return FALSE;
}

static gint gtk_scrollball_button_press (GtkWidget *widget, GdkEventButton *event) {
  GtkScrollball *scrollball;

  scrollball =   GTK_SCROLLBALL (widget);

  if (event->x-scrollball->xoff >= 0 && event->x-scrollball->xoff <= 21 &&
      event->y-scrollball->yoff >= 0 && event->y-scrollball->yoff <= 21) {
    scrollball->button=event->button;
    
    gtk_grab_add (widget);
  } else if (event->x-scrollball->xoff <= 27 && event->y-scrollball->yoff <= 20 &&
	     event->y-scrollball->yoff >= 2) {
    scrollball->vadjust->value=((event->y-scrollball->yoff-3)/17) * 
      (scrollball->vadjust->upper - scrollball->vadjust->lower);

    if (scrollball->vadjust->value > scrollball->vadjust->upper - scrollball->vadjust->page_size)
      scrollball->vadjust->value = scrollball->vadjust->upper - scrollball->vadjust->page_size;
    if (scrollball->vadjust->value < scrollball->vadjust->lower)
      scrollball->vadjust->value = scrollball->vadjust->lower;

    gtk_signal_emit_by_name (GTK_OBJECT (scrollball->vadjust), "value_changed");
  } else if (event->x-scrollball->xoff <= 20 && event->y-scrollball->yoff <= 27 &&
	     event->x-scrollball->xoff >= 2) {
    scrollball->hadjust->value=((event->x-scrollball->xoff-2)/17) * 
      (scrollball->hadjust->upper - scrollball->hadjust->lower);

    if (scrollball->hadjust->value > scrollball->hadjust->upper - scrollball->hadjust->page_size)
      scrollball->hadjust->value = scrollball->hadjust->upper - scrollball->hadjust->page_size;
    if (scrollball->hadjust->value < scrollball->hadjust->lower)
      scrollball->hadjust->value = scrollball->hadjust->lower;

    gtk_signal_emit_by_name (GTK_OBJECT (scrollball->hadjust), "value_changed");
  }
  return TRUE;
}

static gint gtk_scrollball_button_release (GtkWidget *widget, GdkEventButton *event) {
  GTK_SCROLLBALL (widget)->button = 0;
  gtk_grab_remove (widget);

  return TRUE;
}

static gint gtk_scrollball_motion_notify (GtkWidget *widget, GdkEventMotion *event) {
  GtkScrollball *scrollball;
  gint x, y, value;
  static gint deltax=0, deltay=0;

  scrollball = GTK_SCROLLBALL (widget);

  gtk_widget_get_pointer (widget, &x, &y);

  /*  printf ("motion at %d, %d\n",x,y);*/

  deltax=x-deltax;
  deltay=y-deltay;
  
  if ((deltax != 0 || deltay !=0) && scrollball->button) {
    value=scrollball->hadjust->value + deltax*scrollball->acceleration;
    if (value < scrollball->hadjust->lower)
      value=scrollball->hadjust->lower;
    if (value > scrollball->hadjust->upper - scrollball->hadjust->page_size)
      value=scrollball->hadjust->upper - scrollball->hadjust->page_size;
    scrollball->hadjust->value=value;
    
    value=scrollball->vadjust->value + deltay*scrollball->acceleration;
    if (value < scrollball->vadjust->lower)
      value=scrollball->vadjust->lower;
    if (value > scrollball->vadjust->upper - scrollball->vadjust->page_size)
      value=scrollball->vadjust->upper - scrollball->vadjust->page_size;
    scrollball->vadjust->value=value;
    
    gtk_signal_emit_by_name (GTK_OBJECT (scrollball->hadjust), "value_changed");
    gtk_signal_emit_by_name (GTK_OBJECT (scrollball->vadjust), "value_changed");
  }

  deltax=x;
  deltay=y;

  return TRUE;
}

static void gtk_scrollball_update (GtkScrollball *scball) {
  gfloat new_hvalue;
  gfloat new_vvalue;
  
  g_return_if_fail (scball != NULL);
  g_return_if_fail (GTK_IS_SCROLLBALL (scball));

  new_hvalue = scball->hadjust->value;
  
  if (new_hvalue < scball->hadjust->lower)
    new_hvalue = scball->hadjust->lower;

  if (new_hvalue > scball->hadjust->upper)
    new_hvalue = scball->hadjust->upper;

  if (new_hvalue != scball->hadjust->value) {
    scball->hadjust->value = new_hvalue;
    gtk_signal_emit_by_name (GTK_OBJECT (scball->hadjust), "value_changed");
  }

  new_vvalue = scball->vadjust->value;
  
  if (new_vvalue < scball->vadjust->lower)
    new_vvalue = scball->vadjust->lower;

  if (new_vvalue > scball->vadjust->upper)
    new_vvalue = scball->vadjust->upper;

  if (new_vvalue != scball->vadjust->value) {
    scball->vadjust->value = new_vvalue;
    gtk_signal_emit_by_name (GTK_OBJECT (scball->vadjust), "value_changed");
  }

  gtk_widget_draw (GTK_WIDGET(scball), NULL);
}

static void gtk_scrollball_adjustment_value_changed (GtkAdjustment *adjustment, gpointer *data) {
  GtkScrollball *scrollball;

  g_return_if_fail (adjustment != NULL);
  g_return_if_fail (data != NULL);

  scrollball = GTK_SCROLLBALL (data);

  gtk_scrollball_update (scrollball);
}

static void gtk_scrollball_adjustment_changed (GtkAdjustment *adjustment, gpointer *data) {
  GtkScrollball *scrollball;

  g_return_if_fail (adjustment != NULL);
  g_return_if_fail (data != NULL);

  scrollball = GTK_SCROLLBALL (data);

  gtk_scrollball_update (scrollball);
}

