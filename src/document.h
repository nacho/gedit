/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit
 *
 * Copyright (C) 1998, 1999 Alex Roberts, Evan Lawrence
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __DOCUMENT_H__
#define __DOCUMENT_H__

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

	gchar *filename;
	GtkWidget *tab_label;

	/* flags. FIXME: use only one bit of the flags
	   with << ask federico about it. Chema */
	gint changed;
	gint readonly;
	gint untitled_number;

	/* Undo/Redo Lists */
	GList *undo;
	GList *redo;
};


#define NUM_MDI_MODES 4
extern guint mdi_type [NUM_MDI_MODES];
extern GnomeMDI *mdi;

void gedit_document_insert_text (Document *doc, guchar *text, guint position, gint undoable);
void gedit_document_delete_text (Document *doc, guint position, gint length, gint undoable);
void gedit_document_set_readonly (Document *doc, gint readonly);

gchar*	gedit_document_get_tab_name (Document *doc);
guchar*	gedit_document_get_buffer (Document * doc);
guint	gedit_document_get_buffer_length (Document * doc);

Document * gedit_document_new (void);
Document * gedit_document_new_with_title (gchar *title);
Document * gedit_document_new_with_file (gchar *filename);
Document * gedit_document_current (void);

GtkType gedit_document_get_type (void);

void gedit_mdi_init (void);
void gedit_document_load ( GList *file_list);
void gedit_document_set_title (Document *doc);

#endif /* __DOCUMENT_H__ */
