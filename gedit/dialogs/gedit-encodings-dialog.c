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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include "gedit-encodings-dialog.h"

#include <glade/glade-xml.h>
#include <libgnome/libgnome.h>
#include <gtk/gtk.h>

#include <glib/gslist.h>

#include "gedit-encodings.h"
#include "gedit-prefs-manager.h"

#include "gedit-debug.h"


enum {
	COLUMN_NAME,
	COLUMN_CHARSET,
	N_COLUMNS
};

GSList *show_in_menu_list = NULL;

static void
show_help (GtkWidget *window)
{

	GError *err;
	err = NULL;
		
	gnome_help_display ("gedit", NULL,
			    &err);
      
	if (err)
	{
		GtkWidget *dialog;
         
		dialog = gtk_message_dialog_new (GTK_WINDOW (window),
						 GTK_DIALOG_DESTROY_WITH_PARENT,
						 GTK_MESSAGE_ERROR,
						 GTK_BUTTONS_CLOSE,
						 _("There was an error displaying help: %s"),
						 err->message);
         
		g_signal_connect (G_OBJECT (dialog), 
				  "response", 
				  G_CALLBACK (gtk_widget_destroy), 
				  NULL);
         
		gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
         
		gtk_widget_show (dialog);
         
		g_error_free (err);
	}
}

static void
count_selected_items_func (GtkTreeModel * model,
			   GtkTreePath * path,
			   GtkTreeIter * iter, gpointer data)
{
	int *count = data;

	*count += 1;
}

static void
available_selection_changed_callback (GtkTreeSelection * selection,
				      void *data)
{
	int count;
	GtkWidget *add_button;
	GtkWidget *dialog;

	dialog = data;

	count = 0;
	gtk_tree_selection_selected_foreach (selection,
					     count_selected_items_func,
					     &count);

	add_button =
	    g_object_get_data (G_OBJECT (dialog), "encoding-dialog-add");

	gtk_widget_set_sensitive (add_button, count > 0);
}

static void
displayed_selection_changed_callback (GtkTreeSelection * selection,
				      void *data)
{
	int count;
	GtkWidget *remove_button;
	GtkWidget *dialog;

	dialog = data;

	count = 0;
	gtk_tree_selection_selected_foreach (selection,
					     count_selected_items_func,
					     &count);

	remove_button =
	    g_object_get_data (G_OBJECT (dialog),
			       "encoding-dialog-remove");

	gtk_widget_set_sensitive (remove_button, count > 0);
}


static void
get_selected_encodings_func (GtkTreeModel * model,
			     GtkTreePath * path,
			     GtkTreeIter * iter, gpointer data)
{
	GSList **list = data;
	gchar *charset;
	const GeditEncoding *enc;

	charset = NULL;
	gtk_tree_model_get (model, iter, COLUMN_CHARSET, &charset, -1);

	enc = gedit_encoding_get_from_charset (charset);
	g_free (charset);
	
	*list = g_slist_prepend (*list, (gpointer)enc);
}


static gboolean
slist_find (GSList *list, const gpointer data)
{
	while (list != NULL)
	{
      		if (list->data == data)
			return TRUE;

		list = g_slist_next (list);
    	}

  	return FALSE;
}

static void
update_shown_in_menu_tree_model (GtkListStore * store, GSList *list)
{
	GtkTreeIter iter;
	
	gedit_debug (DEBUG_PREFS, "");

	gtk_list_store_clear (store);

	while (list != NULL)
	{
		const GeditEncoding* enc;

		enc = (const GeditEncoding*) list->data;
		
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter,
				    COLUMN_CHARSET,
				    gedit_encoding_get_charset (enc),
				    COLUMN_NAME,
				    gedit_encoding_get_name (enc), -1);

		list = g_slist_next (list);
	}	
}

static void
add_button_clicked_callback (GtkWidget * button, void *data)
{
	GtkWidget *dialog;
	GtkWidget *treeview;
	GtkTreeSelection *selection;
	GSList *encodings;
	GSList *tmp;

	dialog = data;

	treeview = g_object_get_data (G_OBJECT (dialog),
				      "encoding-dialog-available-treeview");
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));

	encodings = NULL;
	gtk_tree_selection_selected_foreach (selection,
					     get_selected_encodings_func,
					     &encodings);

	tmp = encodings;
	while (tmp != NULL)
	{
		if (!slist_find (show_in_menu_list, tmp->data))
		{
			show_in_menu_list = g_slist_prepend (show_in_menu_list, tmp->data);
		}

		tmp = g_slist_next (tmp);
	}

	g_slist_free (encodings);

	update_shown_in_menu_tree_model (
			GTK_LIST_STORE (
				g_object_get_data (G_OBJECT (dialog), "encoding-dialog-displayed-liststore")),
			show_in_menu_list);
}

static void
remove_button_clicked_callback (GtkWidget * button, void *data)
{
	GtkWidget *dialog;
	GtkWidget *treeview;
	GtkTreeSelection *selection;
	GSList *encodings;
	GSList *tmp;

	dialog = data;

	treeview = g_object_get_data (G_OBJECT (dialog),
				      "encoding-dialog-displayed-treeview");
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));

	encodings = NULL;
	gtk_tree_selection_selected_foreach (selection,
					     get_selected_encodings_func,
					     &encodings);

	tmp = encodings;
	while (tmp != NULL) 
	{
		show_in_menu_list = g_slist_remove (show_in_menu_list, tmp->data);

		tmp = g_slist_next (tmp);
	}

	g_slist_free (encodings);

	update_shown_in_menu_tree_model (
			GTK_LIST_STORE (
				g_object_get_data (G_OBJECT (dialog), "encoding-dialog-displayed-liststore")),
			show_in_menu_list);
}

static void
dialog_destroyed (void *data, GObject * where_object_was)
{
	if (show_in_menu_list != NULL)
		g_slist_free (show_in_menu_list);

	show_in_menu_list = NULL;
}


static void
init_shown_in_menu_tree_model (GtkListStore * store)
{
	GtkTreeIter iter;
	GSList *list, *tmp;
	
	gedit_debug (DEBUG_PREFS, "");

	/* add data to the list store */
	list = gedit_prefs_manager_get_shown_in_menu_encodings ();
	
	tmp = list;
	
	while (tmp != NULL)
	{
		const GeditEncoding* enc;

		enc = (const GeditEncoding*) tmp->data;
		
		show_in_menu_list = g_slist_prepend (show_in_menu_list, tmp->data);
		
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter,
				    COLUMN_CHARSET,
				    gedit_encoding_get_charset (enc),
				    COLUMN_NAME,
				    gedit_encoding_get_name (enc), -1);

		tmp = g_slist_next (tmp);
	}

	g_slist_free (list);
	
}

static GtkWidget *
gedit_encoding_dialog_new (GtkWindow *transient_parent)
{
	GladeXML *xml;
	GtkWidget *w;
	GtkCellRenderer *cell_renderer;
	int i;
	GtkTreeModel *sort_model;
	GtkListStore *tree;
	GtkTreeViewColumn *column;
	GtkTreeIter parent_iter;
	GtkTreeSelection *selection;
	GtkWidget *dialog;
	const GeditEncoding *enc;

	xml = glade_xml_new (GEDIT_GLADEDIR "gedit-encodings-dialog.glade2",
			     "encodings-dialog", NULL);

	if (!xml) 
	{
		g_warning ("Could not find gedit-encodings-dialog.glade2, reinstall gedit.\n");
		return NULL;
	}


	/* The dialog itself */
	dialog = glade_xml_get_widget (xml, "encodings-dialog");

	if (transient_parent != NULL)
		gtk_window_set_transient_for (GTK_WINDOW (dialog), transient_parent);

	/* buttons */
	w = glade_xml_get_widget (xml, "add-button");
	g_object_set_data (G_OBJECT (dialog), "encoding-dialog-add", w);

	g_signal_connect (G_OBJECT (w), "clicked",
			  G_CALLBACK (add_button_clicked_callback),
			  dialog);

	w = glade_xml_get_widget (xml, "remove-button");
	g_object_set_data (G_OBJECT (dialog), "encoding-dialog-remove", w);

	g_signal_connect (G_OBJECT (w), "clicked",
			  G_CALLBACK (remove_button_clicked_callback),
			  dialog);

	/* Tree view of available encodings */

	w = glade_xml_get_widget (xml, "available-treeview");
	g_object_set_data (G_OBJECT (dialog),
			   "encoding-dialog-available-treeview", w);

	tree = gtk_list_store_new (N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING);

	/* Column 1 */
	cell_renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("_Description"),
							   cell_renderer,
							   "text", COLUMN_NAME,
							   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (w), column);
	gtk_tree_view_column_set_sort_column_id (column, COLUMN_NAME);

	/* Column 2 */
	cell_renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("_Encoding"),
							   cell_renderer,
							   "text",
							   COLUMN_CHARSET,
							   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (w), column);
	gtk_tree_view_column_set_sort_column_id (column, COLUMN_CHARSET);

	/* Add the data */

	i = 0;
	while ((enc = gedit_encoding_get_from_index (i)) != NULL) 
	{
		gtk_list_store_append (tree, &parent_iter);
		gtk_list_store_set (tree, &parent_iter,
				    COLUMN_CHARSET,
				    gedit_encoding_get_charset (enc),
				    COLUMN_NAME,
				    gedit_encoding_get_name (enc), -1);

		++i;
	}

	/* Sort model */
	sort_model =
	    gtk_tree_model_sort_new_with_model (GTK_TREE_MODEL (tree));

	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE
					      (sort_model), COLUMN_NAME,
					      GTK_SORT_ASCENDING);

	gtk_tree_view_set_model (GTK_TREE_VIEW (w), sort_model);
	g_object_unref (G_OBJECT (tree));
	g_object_unref (G_OBJECT (sort_model));

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (w));
	gtk_tree_selection_set_mode (GTK_TREE_SELECTION (selection),
				     GTK_SELECTION_MULTIPLE);

	available_selection_changed_callback (selection, dialog);
	g_signal_connect (G_OBJECT (selection), "changed",
			  G_CALLBACK
			  (available_selection_changed_callback), dialog);

	/* Tree view of selected encodings */

	w = glade_xml_get_widget (xml, "displayed-treeview");
	g_object_set_data (G_OBJECT (dialog),
			   "encoding-dialog-displayed-treeview", w);

	tree =
	    gtk_list_store_new (N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING);

	g_object_set_data (G_OBJECT (dialog),
			   "encoding-dialog-displayed-liststore", tree);

	/* Column 1 */
	cell_renderer = gtk_cell_renderer_text_new ();
	column =
	    gtk_tree_view_column_new_with_attributes (_("_Description"),
						      cell_renderer,
						      "text", COLUMN_NAME,
						      NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (w), column);
	gtk_tree_view_column_set_sort_column_id (column, COLUMN_NAME);

	/* Column 2 */
	cell_renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("_Encoding"),
							   cell_renderer,
							   "text",
							   COLUMN_CHARSET,
							   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (w), column);
	gtk_tree_view_column_set_sort_column_id (column, COLUMN_CHARSET);

	/* Add the data */
	init_shown_in_menu_tree_model (tree);
	
	/* Sort model */
	sort_model =
	    gtk_tree_model_sort_new_with_model (GTK_TREE_MODEL (tree));

	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE
					      (sort_model), COLUMN_NAME,
					      GTK_SORT_ASCENDING);

	gtk_tree_view_set_model (GTK_TREE_VIEW (w), sort_model);
	g_object_unref (G_OBJECT (sort_model));
	g_object_unref (G_OBJECT (tree));

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (w));
	gtk_tree_selection_set_mode (GTK_TREE_SELECTION (selection),
				     GTK_SELECTION_MULTIPLE);

	displayed_selection_changed_callback (selection, dialog);
	g_signal_connect (G_OBJECT (selection), "changed",
			  G_CALLBACK
			  (displayed_selection_changed_callback), dialog);

	g_object_unref (G_OBJECT (xml));

	g_object_weak_ref (G_OBJECT (dialog), dialog_destroyed, NULL);

	return dialog;
}

static void
update_list ()
{
	g_return_if_fail (gedit_prefs_manager_shown_in_menu_encodings_can_set ());
	
	gedit_prefs_manager_set_shown_in_menu_encodings (show_in_menu_list);
}

gboolean
gedit_encodings_dialog_run (GtkWindow *parent)
{
	gint id;
	GtkWidget *dialog;
       
	dialog	= gedit_encoding_dialog_new (parent);

	do
	{
		id = gtk_dialog_run (GTK_DIALOG (dialog));

		switch (id) {
			case GTK_RESPONSE_HELP:
				show_help (dialog);
				break;
			
			case GTK_RESPONSE_OK:
				update_list (dialog);
				gtk_widget_hide (dialog);

				break;

			default:
				gtk_widget_hide (dialog);
		}
		
	} while (GTK_WIDGET_VISIBLE (dialog));

	gtk_widget_destroy (dialog);

	return (id == GTK_RESPONSE_OK);
}

