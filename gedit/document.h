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

#define GEDIT_DOCUMENT(obj)		GTK_CHECK_CAST (obj, gedit_document_get_type (), GeditDocument)
#define GEDIT_DOCUMENT_CLASS(klass)	GTK_CHECK_CLASS_CAST (klass, gedit_document_get_type (), GeditDocumentClass)
#define IS_GE_DOCUMENT(obj)		GTK_CHECK_TYPE (obj, gedit_document_get_type ())

typedef struct _GeditDocument      GeditDocument;
typedef struct _GeditDocumentClass GeditDocumentClass;

#include <libgnomeui/gnome-mdi.h>
#include <libgnomeui/gnome-mdi-child.h>

struct _GeditDocument
{
	GnomeMDIChild mdi_child;

	GtkWidget *tab_label;
	gchar *filename;

	gint untitled_number;
	
	gboolean changed : 1;
	gboolean readonly : 1;

	/* Views */
	GList *views;

	/* Undo/Redo Lists */
	GList *undo;
	GList *redo;
};

typedef enum {
	GEDIT_CLOSE_ALL_FLAG_NORMAL,
	GEDIT_CLOSE_ALL_FLAG_CLOSE_ALL,
	GEDIT_CLOSE_ALL_FLAG_QUIT
}GeditCloseAllFlagStates;

#define NUM_MDI_MODES 4
extern guint mdi_type [NUM_MDI_MODES];
extern GnomeMDI *mdi;
gint gedit_close_all_flag;
gint gedit_mdi_destroy_signal;

void gedit_document_insert_text  (GeditDocument *doc, const guchar *text,               guint position,              gint undoable);
void gedit_document_delete_text  (GeditDocument *doc,                                   guint position, gint length, gint undoable);
void gedit_document_replace_text (GeditDocument *doc, const guchar *text, gint  length, guint position, gint undoable);

void gedit_document_set_readonly (GeditDocument *doc, gint readonly);
void gedit_document_text_changed_signal_connect (GeditDocument *doc);

gchar*	gedit_document_get_tab_name (GeditDocument *doc);
guchar* gedit_document_get_chars (GeditDocument *doc, guint start_pos, guint end_pos);
guchar*	gedit_document_get_buffer (GeditDocument *doc);
guint	gedit_document_get_buffer_length (GeditDocument *doc);

GeditDocument * gedit_document_new (void);
GeditDocument * gedit_document_new_with_title (const gchar *title);
gint       gedit_document_new_with_file (const gchar *file_name);
GeditDocument * gedit_document_current (void);

GtkType gedit_document_get_type (void);

void gedit_mdi_init (void);
gboolean gedit_document_load ( GList *file_list);
void gedit_document_set_title (GeditDocument *doc);
void gedit_document_swap_hc_cb (GtkWidget *widget, gpointer data);
void gedit_document_set_undo (GeditDocument *doc, gint undo_state, gint redo_state);

#endif /* __DOCUMENT_H__ */
