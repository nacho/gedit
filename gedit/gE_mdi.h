/* vi:set ts=4 sts=0 sw=4:
 *
 * gEdit
 * Copyright (C) 1998, 1999 Alex Roberts and Evan Lawrence
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef __GE_MDI_H__
#define __GE_MDI_H__

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include <gnome.h>

#include "main.h"

BEGIN_GNOME_DECLS

#define GE_DOCUMENT(obj)			GTK_CHECK_CAST (obj, gE_document_get_type (), gE_document)
#define GE_DOCUMENT_CLASS(klass)	GTK_CHECK_CLASS_CAST (klass, gE_document_get_type (), gE_document_class)
#define IS_GE_DOCUMENT(obj)			GTK_CHECK_TYPE (obj, gE_document_get_type ())

typedef struct _gE_document_class 
{
	GnomeMDIChildClass parent_class;
	
	void (*document_changed)(gE_document *, gpointer);

} gE_document_class;

#define NUM_MDI_MODES		4
extern guint mdi_type [NUM_MDI_MODES];

GtkType gE_document_get_type ();
gE_document *gE_document_new ();
gE_document *gE_document_new_with_title (gchar *title);
gE_document *gE_document_new_with_file (gchar *filename);
gE_document *gE_document_current ();
void gE_add_view (GtkWidget *w, gpointer data);
void gE_remove_view (GtkWidget *w, gpointer data);
gint remove_doc_cb (GnomeMDI *mdi, gE_document *doc);
void mdi_view_changed_cb (GnomeMDI *mdi, GtkWidget *old_view);
void add_view_cb (GnomeMDI *mdi, gE_document *doc);
gint add_child_cb (GnomeMDI *mdi, gE_document *doc);
END_GNOME_DECLS

#endif /* __GE_MDI_H__ */
