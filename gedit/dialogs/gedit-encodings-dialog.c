/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-encodings-dialog.c
 * This file is part of gedit
 *
 * Copyright (C) 2002 Paolo Maggi 
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
 * Foundation, Inc., 59 Temple Place, Suite 330, 
 * Boston, MA 02111-1307, USA. 
 */

/*
 * Modified by the gedit Team, 2002. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 */

#include "gedit-encodings-dialog.h"

#include <glade/glade-xml.h>
#include <libgnome/libgnome.h>
#include <gtk/gtkwidget.h>
#include <gtk/gtkwindow.h>

#include <glib/gslist.h>

#include "gedit-encodings.h"

#include "gedit-debug.h"

enum
{
	COLUMN_ENCODING_NAME = 0,
	COLUMN_ENCODING_INDEX,
	ENCODING_NUM_COLS
};


typedef struct _GeditEncodingsDialog GeditEncodingsDialog;

struct _GeditEncodingsDialog {
	GtkWidget *dialog;
	
	GtkWidget *encodings_treeview;
	GtkTreeModel *model;

	GeditPreferencesDialog *prefs_dlg;
};

static void add_button_pressed (GeditEncodingsDialog * dialog);
static GeditEncodingsDialog *get_encodings_dialog (GeditPreferencesDialog *dlg);
static void dialog_destroyed (GtkObject *obj,  void **dialog_pointer);

static void
dialog_destroyed (GtkObject *obj,  void **dialog_pointer)
{
	gedit_debug (DEBUG_PREFS, "");

	if (dialog_pointer != NULL)
	{
		g_free (*dialog_pointer);
		*dialog_pointer = NULL;
	}	
}

static void
dialog_response_handler (GtkDialog *dlg, gint res_id,  GeditEncodingsDialog *dialog)
{
	GError *error = NULL;
	
	gedit_debug (DEBUG_PREFS, "");

	switch (res_id) {
		case GTK_RESPONSE_OK:
			add_button_pressed (dialog);

			gtk_widget_destroy (dialog->dialog);

			break;
			
		case GTK_RESPONSE_HELP:
			
			gnome_help_display ("gedit.xml", "gedit-prefs", &error);
	
			if (error != NULL)
			{
				g_warning (error->message);
				g_error_free (error);
			}

			break;

	
		default:
			gtk_widget_destroy (dialog->dialog);
	}
}

static void 
add_button_pressed (GeditEncodingsDialog * dialog)
{
	GValue value = {0, };
	const GeditEncoding* enc;
	GSList *encs = NULL;
	
	GtkTreeIter iter;
	GtkTreeSelection *selection;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (dialog->encodings_treeview));
	g_return_if_fail (selection != NULL);
	
	if (!gtk_tree_model_get_iter_first (dialog->model, &iter))
		return;

	if (gtk_tree_selection_iter_is_selected (selection, &iter))
	{
		gtk_tree_model_get_value (dialog->model, &iter,
			    COLUMN_ENCODING_INDEX, &value);

		enc = gedit_encoding_get_from_index (g_value_get_int (&value));
		g_return_if_fail (enc != NULL);
	
		encs = g_slist_prepend (encs, (gpointer)enc);
		
		g_value_unset (&value);
	}

	while (gtk_tree_model_iter_next (dialog->model, &iter))
	{
		if (gtk_tree_selection_iter_is_selected (selection, &iter))
		{
			gtk_tree_model_get_value (dialog->model, &iter,
				    COLUMN_ENCODING_INDEX, &value);

			enc = gedit_encoding_get_from_index (g_value_get_int (&value));
			g_return_if_fail (enc != NULL);
	
			encs = g_slist_prepend (encs, (gpointer)enc);
	
			g_value_unset (&value);
		}
	}

	if (encs != NULL)
	{
		encs = g_slist_reverse (encs);
		gedit_preferences_dialog_add_encodings (dialog->prefs_dlg, encs);

		g_slist_free (encs);
	}
}

static GtkTreeModel*
create_encodings_treeview_model (void)
{
	GtkListStore *store;
	GtkTreeIter iter;
	gint i;
	const GeditEncoding* enc;

	gedit_debug (DEBUG_PREFS, "");

	/* create list store */
	store = gtk_list_store_new (ENCODING_NUM_COLS, G_TYPE_STRING, G_TYPE_INT);

	i = 0;
	
	while ((enc = gedit_encoding_get_from_index (i)) != NULL)
	{
		gchar *name;

		enc = gedit_encoding_get_from_index (i);
		name = gedit_encoding_to_string (enc);
		
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter,
				    COLUMN_ENCODING_NAME, name,
				    COLUMN_ENCODING_INDEX, i,
				    -1);
		g_free (name);

		++i;
	}
	
	return GTK_TREE_MODEL (store);
}

static GeditEncodingsDialog *
get_encodings_dialog (GeditPreferencesDialog *dlg)
{
	GladeXML *gui;
	static GeditEncodingsDialog *dialog = NULL;	
	GtkTreeViewColumn *column;
	GtkCellRenderer *cell;
	GtkTreeSelection *selection;

	gedit_debug (DEBUG_PREFS, "");

	if (dialog != NULL)
	{
		gtk_window_present (GTK_WINDOW (dialog->dialog));
	}

	gui = glade_xml_new (GEDIT_GLADEDIR "gedit-encodings-dialog.glade2",
			     "encodings_dialog", NULL);

	if (!gui) 
	{
		g_warning
		    ("Could not find gedit-encodings-dialog.glade2, reinstall gedit.\n");
		return NULL;
	}

	dialog = g_new0 (GeditEncodingsDialog, 1);

	dialog->prefs_dlg = dlg;

	dialog->dialog = glade_xml_get_widget (gui, "encodings_dialog");
	
	dialog->encodings_treeview = glade_xml_get_widget (gui, "encodings_treeview");

	if (!dialog->dialog || 
	    !dialog->encodings_treeview) 
	{
		g_warning (
			_("Could not find the required widgets inside %s."),
			"gedit-encodings-dialog.glade2");
		g_object_unref (gui);
		return NULL;
	}
	
	g_signal_connect(G_OBJECT (dialog->dialog), "destroy",
			 G_CALLBACK (dialog_destroyed), &dialog);

	g_signal_connect(G_OBJECT (dialog->dialog), "response",
			 G_CALLBACK (dialog_response_handler), dialog);

	dialog->model = create_encodings_treeview_model ();
	g_return_val_if_fail (dialog->model != NULL, FALSE);

	gtk_tree_view_set_model (GTK_TREE_VIEW (dialog->encodings_treeview), dialog->model);

	/* Add the encoding column */
	cell = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Encodings"), cell, 
			"text", COLUMN_ENCODING_NAME, NULL);
	
	gtk_tree_view_append_column (GTK_TREE_VIEW (dialog->encodings_treeview), column);

	gtk_tree_view_set_search_column (GTK_TREE_VIEW (dialog->encodings_treeview),
			COLUMN_ENCODING_NAME);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (dialog->encodings_treeview));
	g_return_val_if_fail (selection != NULL, NULL);

	gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);

	g_object_unref (gui);

	gtk_window_set_resizable (GTK_WINDOW (dialog->dialog), FALSE);

	return dialog;

}


void 
gedit_encodings_dialog_run (GeditPreferencesDialog *dlg)
{
	GeditEncodingsDialog* dialog;

	g_return_if_fail (dlg != NULL);

	dialog = get_encodings_dialog (dlg);

	gtk_window_set_transient_for (GTK_WINDOW (dialog->dialog), GTK_WINDOW (dlg));

	gtk_widget_grab_focus (dialog->encodings_treeview);

	if (!GTK_WIDGET_VISIBLE (dialog->dialog))
		gtk_widget_show (dialog->dialog);
}
