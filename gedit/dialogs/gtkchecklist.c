/* gtk-check_list.c
 * Copyright (C) 2001  Jonathan Blandford
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

/*
 * Based on CheckList.py from the eider module, written by Owen Taylor.
 */

/*
 * Modified by: Paolo Maggi <maggi@athena.polito.it>
 */

/*
 * TODO: make more robust with gtkclist functions.  Maybe override
 * GtkCList::append etc.
 */
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "gtkchecklist.h"

enum {
  TOGGLED,
  LAST_SIGNAL
};

static void gtk_check_list_init          (GtkCheckList      *check_list);
static void gtk_check_list_class_init    (GtkCheckListClass *class);

static void gtk_check_list_finalize      (GtkObject         *object);
static void gtk_check_list_realize       (GtkWidget         *widget);
static void gtk_check_list_style_set     (GtkWidget         *widget,
					  GtkStyle          *old_style);
static gint gtk_check_list_button_press  (GtkWidget         *widget,
					  GdkEventButton    *event,
					  gpointer           data);
static gint gtk_check_list_key_press     (GtkWidget         *widget,
					  GdkEventKey       *event,
					  gpointer           data);
static void gtk_check_list_clear         (GtkCList          *clist);
static void gtk_check_list_update_row    (GtkCheckList      *check_list,
					  gint               row);
static void gtk_check_list_color_pixmaps (GtkCheckList      *check_list);


static GtkCListClass *parent_class = NULL;
static guint check_list_signals [LAST_SIGNAL] = { 0 };


typedef struct _GtkCheckListTuple GtkCheckListTuple;
struct _GtkCheckListTuple
{
  gboolean toggled;
  gpointer user_data;
};

GtkType
gtk_check_list_get_type (void)
{
  static GtkType check_list_type = 0;

  if (!check_list_type)
    {
      static const GtkTypeInfo check_list_info =
      {
        "GtkCheckList",
        sizeof (GtkCheckList),
        sizeof (GtkCheckListClass),
        (GtkClassInitFunc) gtk_check_list_class_init,
        (GtkObjectInitFunc) gtk_check_list_init,
        /* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL,
      };

      check_list_type = gtk_type_unique (gtk_clist_get_type (), &check_list_info);
    }

  return check_list_type;
}

static void
gtk_check_list_class_init (GtkCheckListClass *class)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;
  GtkCListClass *clist_class;

  parent_class = gtk_type_class (gtk_clist_get_type ());

  object_class = (GtkObjectClass *) class;
  widget_class = (GtkWidgetClass *) class;
  clist_class = (GtkCListClass *) class;

  object_class->finalize = gtk_check_list_finalize;
  widget_class->realize = gtk_check_list_realize;
  widget_class->style_set = gtk_check_list_style_set;
  clist_class->clear = gtk_check_list_clear;

  check_list_signals [TOGGLED] =
    gtk_signal_new ("toggled",
		    GTK_RUN_LAST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GtkCheckListClass, toggled),
		    gtk_marshal_NONE__INT_INT,
		    GTK_TYPE_NONE, 2,
		    GTK_TYPE_INT,
		    GTK_TYPE_INT);

  gtk_object_class_add_signals (object_class, check_list_signals, LAST_SIGNAL);
}

static void
gtk_check_list_init (GtkCheckList *check_list)
{
  check_list->off_pixmap = NULL;
  check_list->on_pixmap = NULL;

  check_list->check_size = 13;
  check_list->n_rows = 0;
}

static void
gtk_check_list_finalize (GtkObject *object)
{
  (* GTK_OBJECT_CLASS (parent_class)->finalize) (object);

  /* FIXME:  Free all list_data */
}

static void
gtk_check_list_realize (GtkWidget *widget)
{
  GtkCheckList *check_list = (GtkCheckList *) widget;
  gint i;
  
  (* GTK_WIDGET_CLASS (parent_class)->realize) (widget);

  check_list->on_pixmap = gdk_pixmap_new (widget->window,
					  check_list->check_size,
					  check_list->check_size,
					  -1);
  check_list->off_pixmap = gdk_pixmap_new (widget->window,
					   check_list->check_size,
					   check_list->check_size,
					   -1);
  check_list->mask = gdk_pixmap_new (widget->window,
				     check_list->check_size,
				     check_list->check_size,
				     1);

  gtk_check_list_color_pixmaps (GTK_CHECK_LIST (widget));

  for (i = 0; i < check_list->n_rows; i++)
    gtk_check_list_update_row (check_list, i);

}
static void
gtk_check_list_style_set (GtkWidget *widget,
			  GtkStyle  *old_style)
{
  (* GTK_WIDGET_CLASS (parent_class)->style_set) (widget, old_style);

  if (GTK_WIDGET_REALIZED (widget))
    {
      gint i;
      gtk_check_list_color_pixmaps (GTK_CHECK_LIST (widget));

      for (i = 0; i < GTK_CHECK_LIST (widget)->n_rows; i++)
	gtk_check_list_update_row (GTK_CHECK_LIST (widget), i);
    }
}

static gint
gtk_check_list_button_press (GtkWidget      *widget,
			     GdkEventButton *event,
			     gpointer        data)
{
  gint row, column;

  gtk_clist_get_selection_info (GTK_CLIST (widget),
				    event->x,
				    event->y,
				    &row, &column);

  GTK_CLIST (widget)->focus_row = row;

  gtk_widget_grab_focus (widget);
  
  if (column == 0)
  {
      gtk_check_list_toggle_row (GTK_CHECK_LIST (widget), row);
      gtk_signal_emit_stop_by_name (GTK_OBJECT (widget), "button_press_event");
      gtk_clist_select_row (GTK_CLIST (widget), row, 1);
      return TRUE;     
  }

  if (column == 1)	
  {	  
      gtk_signal_emit_stop_by_name (GTK_OBJECT (widget), "button_press_event");
      gtk_clist_select_row (GTK_CLIST (widget), row, 1);
      return TRUE;
  }

  return FALSE;
}

static gint
gtk_check_list_key_press (GtkWidget   *widget,
			  GdkEventKey *event,
			  gpointer     data)
{
  if ((event->keyval == GDK_space) &&
      (GTK_CLIST (widget)->focus_row != -1))
    {
      gtk_check_list_toggle_row (GTK_CHECK_LIST (widget),
				 GTK_CLIST (widget)->focus_row);
      gtk_signal_emit_stop_by_name (GTK_OBJECT (widget), "key_press_event");

      gtk_clist_select_row (GTK_CLIST (widget), GTK_CLIST (widget)->focus_row, 1);

      return TRUE;
    }
  
  return FALSE;
}

static void
gtk_check_list_clear (GtkCList *clist)
{
  (* parent_class->clear) (clist);

  GTK_CHECK_LIST (clist)->n_rows = 0;
}

static void
gtk_check_list_update_row (GtkCheckList *check_list,
			   gint          row)
{
  GtkCheckListTuple *tuple;

  tuple = gtk_clist_get_row_data (GTK_CLIST (check_list), row);

  if (tuple->toggled)
    gtk_clist_set_pixmap (GTK_CLIST (check_list), row, 0,
			  check_list->on_pixmap,
			  check_list->mask);
  else
    gtk_clist_set_pixmap (GTK_CLIST (check_list), row, 0,
			  check_list->off_pixmap,
			  check_list->mask);
}

static void
gtk_check_list_color_pixmaps (GtkCheckList *check_list)
{
  GtkStyle *style;

  GdkGC *base_gc;
  GdkGC *text_gc;
  GdkGC *mask_gc;

  style = gtk_widget_get_style (GTK_WIDGET (check_list));

  base_gc = style->base_gc[GTK_STATE_NORMAL];
  text_gc = style->text_gc[GTK_STATE_NORMAL];

  mask_gc = gdk_gc_new (check_list->mask);
  gdk_gc_set_foreground	  (mask_gc, &(style->white));

  gdk_draw_rectangle (check_list->mask, mask_gc,
		      TRUE, 0, 0,
		      check_list->check_size,
		      check_list->check_size);

  gdk_draw_rectangle (check_list->on_pixmap, base_gc,
		      TRUE, 0, 0,
		      check_list->check_size,
		      check_list->check_size);
  gdk_draw_rectangle (check_list->on_pixmap, text_gc,
		      FALSE, 0, 0,
		      check_list->check_size - 1,
		      check_list->check_size -1);

  gdk_draw_line (check_list->on_pixmap, text_gc,
		 2, check_list->check_size/2,
		 check_list->check_size/3, check_list->check_size - 5);
  gdk_draw_line (check_list->on_pixmap, text_gc,
		 2, check_list->check_size/2 + 1,
		 check_list->check_size/3, check_list->check_size - 4);
  gdk_draw_line (check_list->on_pixmap, text_gc,
		 check_list->check_size/3, check_list->check_size - 5,
		 check_list->check_size-3, 3);
  gdk_draw_line (check_list->on_pixmap, text_gc,
		 check_list->check_size/3, check_list->check_size - 4,
		 check_list->check_size-3, 2);


  gdk_draw_rectangle (check_list->off_pixmap, base_gc,
		      TRUE, 0, 0,
		      check_list->check_size,
		      check_list->check_size);
  gdk_draw_rectangle (check_list->off_pixmap, text_gc,
		      FALSE, 0, 0,
		      check_list->check_size - 1,
		      check_list->check_size - 1);

  gdk_gc_unref (mask_gc);
}


/* Public functions */
GtkWidget *
gtk_check_list_new (void)
{
  GtkWidget *widget;

  widget = gtk_type_new (GTK_TYPE_CHECK_LIST);
  gtk_clist_construct (GTK_CLIST (widget), 2, NULL);
  gtk_clist_set_column_auto_resize (GTK_CLIST (widget), 0, TRUE);
  gtk_clist_set_column_auto_resize (GTK_CLIST (widget), 1, TRUE);
  gtk_clist_column_titles_passive (GTK_CLIST (widget));
  gtk_clist_set_selection_mode (GTK_CLIST (widget), GTK_SELECTION_BROWSE);

  gtk_signal_connect (GTK_OBJECT (widget), "button_press_event",
		      GTK_SIGNAL_FUNC (gtk_check_list_button_press),
		      NULL);
  gtk_signal_connect (GTK_OBJECT (widget), "key_press_event",
		      GTK_SIGNAL_FUNC (gtk_check_list_key_press),
		      NULL);
  return widget;
}



gint
gtk_check_list_append_row (GtkCheckList *check_list,
			   const gchar  *text,
			   gboolean      init_value,
			   gpointer      row_data)
{
  GtkCheckListTuple *tuple;
  gchar *data[2];
  gint row;

  g_return_val_if_fail (check_list != NULL, -1);
  g_return_val_if_fail (GTK_IS_CHECK_LIST (check_list), -1);

  data[0] = "";
  data[1] = g_strdup (text);
  row = gtk_clist_append (GTK_CLIST (check_list), data);
  g_free (data[1]);

  tuple = g_new (GtkCheckListTuple, 1);
  tuple->toggled = init_value;
  tuple->user_data = row_data;

  gtk_clist_set_row_data (GTK_CLIST (check_list), row, tuple);
  check_list->n_rows++;

  if (GTK_WIDGET_REALIZED (check_list))
    gtk_check_list_update_row (check_list, row);

  return row;
}

void
gtk_check_list_toggle_row (GtkCheckList *check_list,
			   gint          row)
{
  GtkCheckListTuple *tuple;

  tuple = gtk_clist_get_row_data (GTK_CLIST (check_list), row);
  tuple->toggled = ! tuple->toggled;
  gtk_check_list_update_row (check_list, row);
  gtk_signal_emit (GTK_OBJECT (check_list),
		   check_list_signals [TOGGLED],
		   row, tuple->toggled);
}

void
gtk_check_list_set_toggled (GtkCheckList *check_list,
			    gint          row,
			    gboolean      toggled)
{
  GtkCheckListTuple *tuple;

  tuple = gtk_clist_get_row_data (GTK_CLIST (check_list), row);
  if (tuple->toggled == toggled)
	  return;
  tuple->toggled = toggled;
  gtk_check_list_update_row (check_list, row);

  gtk_signal_emit (GTK_OBJECT (check_list),
		   check_list_signals [TOGGLED],
		   row, tuple->toggled);
}

gboolean
gtk_check_list_get_toggled (GtkCheckList *check_list,
			    gint          row)
{
  GtkCheckListTuple *tuple;

  tuple = gtk_clist_get_row_data (GTK_CLIST (check_list), row);

  return tuple->toggled;
}

gpointer   
gtk_check_list_get_row_data (GtkCheckList *check_list,
                        gint          row)
{
  GtkCheckListTuple *tuple;

  tuple = gtk_clist_get_row_data (GTK_CLIST (check_list), row);

  return tuple->user_data;  
}


