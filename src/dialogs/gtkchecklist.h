/* gtk-check_list.h
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

#ifndef __GTK_CHECK_LIST_H__
#define __GTK_CHECK_LIST_H__

#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#pragma }
#endif /* __cplusplus */

#define GTK_TYPE_CHECK_LIST			(gtk_check_list_get_type ())
#define GTK_CHECK_LIST(obj)			(GTK_CHECK_CAST ((obj), GTK_TYPE_CHECK_LIST, GtkCheckList))
#define GTK_CHECK_LIST_CLASS(klass)		(GTK_CHECK_CLASS_CAST ((klass), GTK_TYPE_CHECK_LIST, GtkCheckListClass))
#define GTK_IS_CHECK_LIST(obj)			(GTK_CHECK_TYPE ((obj), GTK_TYPE_CHECK_LIST))
#define GTK_IS_CHECK_LIST_CLASS(klass)		(GTK_CHECK_CLASS_TYPE ((obj), GTK_TYPE_CHECK_LIST))


typedef struct _GtkCheckList       GtkCheckList;
typedef struct _GtkCheckListClass  GtkCheckListClass;

struct _GtkCheckList
{
  GtkCList parent;

  GdkPixmap *off_pixmap;
  GdkPixmap *on_pixmap;
  GdkPixmap *mask;

  gint check_size;
  gint n_rows;
};

struct _GtkCheckListClass
{
  GtkCListClass parent_class;

  void (* toggled) (GtkCheckList *check_list,
		    gint          row,
		    gboolean      toggled);
};


GtkType    gtk_check_list_get_type    (void);
GtkWidget *gtk_check_list_new         (void);

gint       gtk_check_list_append_row  (GtkCheckList *check_list,
				       const gchar  *text,
				       gboolean      init_value,
				       gpointer      row_data);
void       gtk_check_list_toggle_row  (GtkCheckList *check_list,
				       gint          row);
void       gtk_check_list_set_toggled (GtkCheckList *check_list,
				       gint          row,
				       gboolean      toggled);
gboolean   gtk_check_list_get_toggled (GtkCheckList *check_list,
				       gint          row);
gpointer   gtk_check_list_get_row_data     (GtkCheckList *check_list,
				       gint          row);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __GTK_CHECK_LIST_H__ */
