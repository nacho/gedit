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
#define SCROLLBALL_DEFAULT_SIZE 21

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
	NULL,
	NULL,
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

  gtk_scrollball_set_hadjustment (scrollball, hadjust);
  gtk_scrollball_set_vadjustment (scrollball, vadjust);
  scrollball->acceleration=20;
  scrollball->hmove=0;
  scrollball->vmove=0;

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

    gtk_signal_connect (GTK_OBJECT (hadjust), "changed",
			(GtkSignalFunc) gtk_scrollball_adjustment_changed,
			(gpointer) scrollball);
    gtk_signal_connect (GTK_OBJECT (hadjust), "value_changed",
			(GtkSignalFunc) gtk_scrollball_adjustment_value_changed,
			(gpointer) scrollball);
  }
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

    gtk_signal_connect (GTK_OBJECT (vadjust), "changed",
			(GtkSignalFunc) gtk_scrollball_adjustment_changed,
			(gpointer) scrollball);
    gtk_signal_connect (GTK_OBJECT (vadjust), "value_changed",
			(GtkSignalFunc) gtk_scrollball_adjustment_value_changed,
			(gpointer) scrollball);
  }
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
  /* All the variable for the placement of the miniwindow */
  gint mwwidth, mwheight, mwx, mwy;
  static gint lasthm=0, lastvm=0;
  
  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (GTK_IS_SCROLLBALL (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  if (event->count > 0)
    return FALSE;
  
  gdk_window_clear_area (widget->window, 0, 0, widget->allocation.width, widget->allocation.height);

  gdk_draw_rectangle (widget->window, widget->style->fg_gc[5], 1, 0, 0, 
		      widget->allocation.width, widget->allocation.height);

  /* Draw our beautiful little arrows (which were triangles, but clahey didn't like em ;) */

  if (GTK_SCROLLBALL(widget)->vadjust) {
    gdk_draw_line (widget->window, widget->style->black_gc, 10, 0, 13, 3);
    gdk_draw_line (widget->window, widget->style->black_gc, 10, 0, 7, 3);
    if (GTK_SCROLLBALL(widget)->vmove < 0 || (GTK_SCROLLBALL(widget)->vmove==0 && lastvm < 0)) {
      gdk_draw_line (widget->window, widget->style->black_gc, 9, 1, 11, 1);
      gdk_draw_line (widget->window, widget->style->black_gc, 8, 2, 12, 2);
      gdk_draw_line (widget->window, widget->style->black_gc, 7, 3, 13, 3);
    }
      
    gdk_draw_line (widget->window, widget->style->black_gc, 10, 20, 13, 17);
    gdk_draw_line (widget->window, widget->style->black_gc, 10, 20, 7, 17);
    if (GTK_SCROLLBALL(widget)->vmove > 0 || (GTK_SCROLLBALL(widget)->vmove==0 && lastvm > 0)) {
      gdk_draw_line (widget->window, widget->style->black_gc, 9, 19, 11, 19);
      gdk_draw_line (widget->window, widget->style->black_gc, 8, 18, 12, 18);
      gdk_draw_line (widget->window, widget->style->black_gc, 7, 17, 13, 17);
    }

    if (GTK_SCROLLBALL(widget)->vmove != 0)
      lastvm = GTK_SCROLLBALL(widget)->vmove;
  }
  
  if (GTK_SCROLLBALL(widget)->button == 0) {
    lasthm=0;
    lastvm=0;
  }

  if (GTK_SCROLLBALL(widget)->hadjust) {
    gdk_draw_line (widget->window, widget->style->black_gc, 0, 10, 3, 13);
    gdk_draw_line (widget->window, widget->style->black_gc, 0, 10, 3, 7);
    if (GTK_SCROLLBALL(widget)->hmove < 0 || (GTK_SCROLLBALL(widget)->hmove==0 && lasthm < 0)) {
      gdk_draw_line (widget->window, widget->style->black_gc, 1, 9, 1, 11);
      gdk_draw_line (widget->window, widget->style->black_gc, 2, 8, 2, 12);
      gdk_draw_line (widget->window, widget->style->black_gc, 3, 7, 3, 13);
    }
    
    gdk_draw_line (widget->window, widget->style->black_gc, 20, 10, 17, 13);
    gdk_draw_line (widget->window, widget->style->black_gc, 20, 10, 17, 7);
    if (GTK_SCROLLBALL(widget)->hmove > 0 || (GTK_SCROLLBALL(widget)->hmove==0 && lasthm > 0)) {
      gdk_draw_line (widget->window, widget->style->black_gc, 19, 9, 19, 11);
      gdk_draw_line (widget->window, widget->style->black_gc, 18, 8, 18, 12);
      gdk_draw_line (widget->window, widget->style->black_gc, 17, 7, 17, 13);
    }

    if (GTK_SCROLLBALL(widget)->hmove != 0)
      lasthm = GTK_SCROLLBALL(widget)->hmove;
  }

  /* And the stuff inside them.... a window to display the current location */

  gdk_draw_rectangle (widget->window, widget->style->black_gc, 0, 4, 4, 12, 12);
  gdk_draw_rectangle (widget->window, widget->style->bg_gc[1], 1, 5, 5, 11, 11);
 
  /* And the little mini-window which displays where we are w/ the window */

  if (GTK_SCROLLBALL(widget)->hadjust) {
    mwwidth = (GTK_SCROLLBALL(widget)->hadjust->page_size /
	       (GTK_SCROLLBALL(widget)->hadjust->upper - GTK_SCROLLBALL(widget)->hadjust->lower)) * 11;
    if (mwwidth <= 0)
      mwwidth=1;

    mwx = (GTK_SCROLLBALL(widget)->hadjust->value /
	   (GTK_SCROLLBALL(widget)->hadjust->upper - GTK_SCROLLBALL(widget)->hadjust->lower)) * 11;
  } else {
    mwwidth = 11;
    mwx = 0;
  }

  if (GTK_SCROLLBALL(widget)->vadjust) {
    mwheight = (GTK_SCROLLBALL(widget)->vadjust->page_size /
		(GTK_SCROLLBALL(widget)->vadjust->upper - GTK_SCROLLBALL(widget)->vadjust->lower)) * 11;
    if (mwheight <= 0)
      mwheight=1;

    mwy = (GTK_SCROLLBALL(widget)->vadjust->value /
	   (GTK_SCROLLBALL(widget)->vadjust->upper - GTK_SCROLLBALL(widget)->vadjust->lower)) * 11;
  } else {
    mwheight = 11;
    mwy = 0;
  }

  gdk_draw_rectangle (widget->window, widget->style->white_gc, 1, 5+mwx,5+mwy,mwwidth,mwheight);

  return FALSE;
}

static gint gtk_scrollball_button_press (GtkWidget *widget, GdkEventButton *event) {
  GtkScrollball *scrollball;

  scrollball =   GTK_SCROLLBALL (widget);

  if (event->x >= 0 && event->x <= 21 &&
      event->y >= 0 && event->y <= 21) {
    scrollball->button=event->button;
    
    gtk_grab_add (widget);
  }

  return TRUE;
}

static gint gtk_scrollball_button_release (GtkWidget *widget, GdkEventButton *event) {
  GTK_SCROLLBALL (widget)->button = 0;
  gtk_grab_remove (widget);

  if (GTK_SCROLLBALL(widget)->hadjust) {
    GTK_SCROLLBALL (widget)->hmove = 0;
    gtk_signal_emit_by_name (GTK_OBJECT (GTK_SCROLLBALL(widget)->hadjust), "value_changed");
  }

  if (GTK_SCROLLBALL(widget)->vadjust) {
    GTK_SCROLLBALL (widget)->vmove = 0;
    gtk_signal_emit_by_name (GTK_OBJECT (GTK_SCROLLBALL(widget)->vadjust), "value_changed");
  }

  return TRUE;
}

static gint gtk_scrollball_motion_notify (GtkWidget *widget, GdkEventMotion *event) {
  GtkScrollball *scrollball;
  gint x, y, value;

  scrollball = GTK_SCROLLBALL (widget);

  gtk_widget_get_pointer (widget, &x, &y);

  /*  printf ("motion at %d, %d\n",x,y);*/

  scrollball->hmove=x-scrollball->hmove;
  scrollball->vmove=y-scrollball->vmove;
  
  if ((scrollball->hmove != 0 || scrollball->vmove !=0) && scrollball->button) {
    if (scrollball->hadjust) {
      value=scrollball->hadjust->value + scrollball->hmove*scrollball->acceleration;
      if (value < scrollball->hadjust->lower)
	value=scrollball->hadjust->lower;
      if (value > scrollball->hadjust->upper - scrollball->hadjust->page_size)
	value=scrollball->hadjust->upper - scrollball->hadjust->page_size;
      scrollball->hadjust->value=value;

      gtk_signal_emit_by_name (GTK_OBJECT (scrollball->hadjust), "value_changed");
    }
    
    if (scrollball->vadjust) {
      value=scrollball->vadjust->value + scrollball->vmove*scrollball->acceleration;
      if (value < scrollball->vadjust->lower)
	value=scrollball->vadjust->lower;
      if (value > scrollball->vadjust->upper - scrollball->vadjust->page_size)
	value=scrollball->vadjust->upper - scrollball->vadjust->page_size;
      scrollball->vadjust->value=value;
      
      gtk_signal_emit_by_name (GTK_OBJECT (scrollball->vadjust), "value_changed");
    }
  }

  scrollball->hmove=x;
  scrollball->vmove=y;

  return TRUE;
}

static void gtk_scrollball_update (GtkScrollball *scball) {
  gfloat new_hvalue;
  gfloat new_vvalue;
  
  g_return_if_fail (scball != NULL);
  g_return_if_fail (GTK_IS_SCROLLBALL (scball));

  if (scball->hadjust) {
    new_hvalue = scball->hadjust->value;
    
    if (new_hvalue < scball->hadjust->lower)
      new_hvalue = scball->hadjust->lower;
    
    if (new_hvalue > scball->hadjust->upper)
      new_hvalue = scball->hadjust->upper;
    
    if (new_hvalue != scball->hadjust->value) {
      scball->hadjust->value = new_hvalue;
      gtk_signal_emit_by_name (GTK_OBJECT (scball->hadjust), "value_changed");
    }
  }

  if (scball->vadjust) {
    new_vvalue = scball->vadjust->value;
    
    if (new_vvalue < scball->vadjust->lower)
      new_vvalue = scball->vadjust->lower;
    
    if (new_vvalue > scball->vadjust->upper)
      new_vvalue = scball->vadjust->upper;
    
    if (new_vvalue != scball->vadjust->value) {
      scball->vadjust->value = new_vvalue;
      gtk_signal_emit_by_name (GTK_OBJECT (scball->vadjust), "value_changed");
    }
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

