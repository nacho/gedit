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

#ifndef __GTK_SCROLLBALL_H__
#define __GTK_SCROLLBALL_H__

#include <gdk/gdk.h>
#include <gtk/gtkadjustment.h>
#include <gtk/gtkwidget.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define GTK_SCROLLBALL(obj)          GTK_CHECK_CAST (obj, gtk_scrollball_get_type (), GtkScrollball)
#define GTK_SCROLLBALL_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, gtk_scrollball_get_type (), GtkScrollballClass)
#define GTK_IS_SCROLLBALL(obj)       GTK_CHECK_TYPE (obj, gtk_scrollball_get_type ())


typedef struct _GtkScrollball        GtkScrollball;
typedef struct _GtkScrollballClass   GtkScrollballClass;

struct _GtkScrollball {
  GtkWidget widget;

  guint8 button;

  guint8 acceleration;

  GtkAdjustment *hadjust;
  GtkAdjustment *vadjust;

  gint hmove;
  gint vmove;
};

struct _GtkScrollballClass {
  GtkWidgetClass parent_class;
};


GtkWidget*     gtk_scrollball_new                     (GtkAdjustment *hadjust, GtkAdjustment *vadjust);
guint          gtk_scrollball_get_type                (void);
GtkAdjustment* gtk_scrollball_get_hadjustment         (GtkScrollball      *scrollball);
GtkAdjustment* gtk_scrollball_get_vadjustment         (GtkScrollball      *scrollball);
void           gtk_scrollball_set_update_policy       (GtkScrollball      *scrollball, 
						       GtkUpdateType  policy);
void           gtk_scrollball_set_hadjustment         (GtkScrollball      *scrollball, 
						       GtkAdjustment *hadjust);
void           gtk_scrollball_set_vadjustment         (GtkScrollball      *scrollball,
						       GtkAdjustment *vadjust);
#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GTK_SCROLLBALL_H__ */
