/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
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

#include <gnome.h>

#include "gedit.h"

BEGIN_GNOME_DECLS

#define GE_DOCUMENT(obj)		GTK_CHECK_CAST (obj, gedit_document_get_type (), gedit_document)
#define GE_DOCUMENT_CLASS(klass)	GTK_CHECK_CLASS_CAST (klass, gedit_document_get_type (), gedit_document_class)
#define IS_GE_DOCUMENT(obj)		GTK_CHECK_TYPE (obj, gedit_document_get_type ())

typedef struct _gedit_document_class 
{
	GnomeMDIChildClass parent_class;
	
	void (*document_changed)(gedit_document *, gpointer);

} gedit_document_class;

#define NUM_MDI_MODES		4
extern guint mdi_type [NUM_MDI_MODES];

GtkType gedit_document_get_type (void);
gedit_document* gedit_document_new (void);
gedit_document* gedit_document_new_with_title (gchar *title);
gedit_document* gedit_document_new_with_file (gchar *filename);
gedit_document* gedit_document_current (void);
void gedit_add_view (GtkWidget *w, gpointer data);
void gedit_remove_view (GtkWidget *w, gpointer data);
gint remove_doc_cb (GnomeMDI *mdi, gedit_document *doc);
void mdi_view_changed_cb (GnomeMDI *mdi, GtkWidget *old_view);
void add_view_cb (GnomeMDI *mdi, gedit_document *doc);
gint add_child_cb (GnomeMDI *mdi, gedit_document *doc);

gchar* gedit_get_document_tab_name (void);


END_GNOME_DECLS

#endif /* __GE_MDI_H__ */
