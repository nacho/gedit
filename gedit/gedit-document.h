/* 
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

#ifndef __GEDIT_DOCUMENT_H__
#define __GEDIT_DOCUMENT_H__

#define DOCUMENT(obj)		GTK_CHECK_CAST (obj, gedit_document_get_type (), Document)
#define DOCUMENT_CLASS(klass)	GTK_CHECK_CLASS_CAST (klass, gedit_document_get_type (), DocumentClass)
#define IS_GE_DOCUMENT(obj)	GTK_CHECK_TYPE (obj, gedit_document_get_type ())

typedef struct _Document Document;
typedef struct _DocumentClass DocumentClass;

struct _DocumentClass
{
	GnomeMDIChildClass parent_class;
	
	void (*document_changed)(Document *, gpointer);

};

struct _Document
{
	GnomeMDIChild mdi_child;
	
	GList *views;
	
	GtkWidget *tab_label;
	gchar *filename;

	/* guchar *buf; */
	GString *buf;
	guint buf_size;

	gint changed_id;
	gint changed;

	struct stat *sb;

	/* Undo/Redo GLists */
	GList *undo;		/* Undo Stack */
	GList *undo_top;	
	GList *redo;		/* Redo Stack */
};


#define NUM_MDI_MODES		4
extern guint mdi_type [NUM_MDI_MODES];
extern GnomeMDI *mdi;

GtkType gedit_document_get_type (void);
Document* gedit_document_new (void);
Document* gedit_document_new_with_title (gchar *title);
Document* gedit_document_new_with_file (gchar *filename);
Document* gedit_document_current (void);

void gedit_add_view     (GtkWidget *w, gpointer data);
void gedit_remove_view  (GtkWidget *w, gpointer data);

gint remove_doc_cb       (GnomeMDI *mdi, Document *doc);
void mdi_view_changed_cb (GnomeMDI *mdi, GtkWidget *old_view);
void add_view_cb         (GnomeMDI *mdi, Document *doc);
gint add_child_cb        (GnomeMDI *mdi, Document *doc);

gchar* gedit_get_document_tab_name (void);


#endif /* __GEDIT_DOCUMENT_H__ */
