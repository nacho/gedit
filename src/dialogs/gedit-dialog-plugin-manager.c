/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-dialog-uri.c
 * This file is part of gedit
 *
 * Copyright (C) 2002 Paolo Maggi and James Willcox
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
 * Modified by the gedit Team, 1998-2002. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 */

#include <libgnome/libgnome.h>
#include <libgnomeui/libgnomeui.h>
#include <glade/glade-xml.h>

#include "gedit2.h"
#include "gedit-mdi.h"
#include "gedit-utils.h"
#include "gedit-dialogs.h"
#include "gedit-plugins-engine.h"
#include "gedit-plugin.h"
#include "gedit-debug.h"

#define PLUGIN_MANAGER_ACTIVE_COLUMN 0
#define PLUGIN_MANAGER_NAME_COLUMN 1

#define PLUGIN_MANAGER_NAME_TITLE _("Plugin")
#define PLUGIN_MANAGER_ACTIVE_TITLE _("Load")

#define PLUGIN_MANAGER_LOGO "/gedit-plugin-manager.png"

typedef struct _GeditDialogPluginManager GeditDialogPluginManager;

struct _GeditDialogPluginManager {
	GtkWidget *dialog; 	/* a GtkDialog */

	GtkWidget *tree; 	/* a GtkTreeView, shows plugins */
	GtkWidget *notebook; 	/* a GtkNotebook, shows info about plugins */
	
	GtkWidget *author; 	/* a GtkLabel, shows the author of the plugin */
	GtkWidget *filename; 	/* a GtkLabel, shows the filename of the plugin */
	GtkWidget *desc; 	/* a GtkLabel, shows the description of the plugin */
	GtkWidget *name; 	/* a GtkLabel, shows the name of the plugin */
	GtkWidget *copyright; 	/* a GtkLabel, shows the copyright info of the plugin */
	GtkWidget *logo; 	/* a GtkImage, a small logo */
	GtkWidget *configure_button; /* a GtkButton,configures a plugin when clicked */

	const GList *plugins; 	/* a list of type GeditPlugin  */
};

static GeditPluginInfo *dialog_plugin_manager_get_selected_plugin (GeditDialogPluginManager *dialog);
static void dialog_plugin_manager_toggle_active (GtkTreeIter *iter, GtkTreeModel *model);
static void dialog_plugin_manager_toggle_all (GeditDialogPluginManager *dialog);
static void dialog_plugin_manager_update_info (GeditDialogPluginManager *dialog,
					       GeditPluginInfo *info);
static void dialog_destroyed (GtkObject *obj,  void **dialog_pointer);
static void dialog_response_handler (GtkDialog *dlg, gint res_id,  GeditDialogPluginManager *dialog);


static void
configure_button_cb (GtkWidget *button, gpointer data)
{
	GeditDialogPluginManager *dialog = data;
	GeditPluginInfo *info;

	gedit_debug (DEBUG_PLUGINS, "");
	
	info = dialog_plugin_manager_get_selected_plugin (dialog);

	g_return_if_fail (info != NULL);

	gedit_debug (DEBUG_PLUGINS, "Configuring: %s\n", info->plugin->name);

	gedit_plugins_engine_configure_plugin (info->plugin, dialog->dialog);

	gedit_debug (DEBUG_PLUGINS, "Done");	
}

static void
dialog_plugin_manager_view_cell_cb (GtkTreeViewColumn *tree_column,
				    GtkCellRenderer   *cell,
				    GtkTreeModel      *tree_model,
				    GtkTreeIter       *iter,
				    gpointer           data)
{
	GeditPluginInfo *info;
	const gchar *title;

	/*
	gedit_debug (DEBUG_PLUGINS, "");
	*/
	
	g_return_if_fail (tree_model != NULL);
	g_return_if_fail (tree_column != NULL);

	gtk_tree_model_get (tree_model, iter, PLUGIN_MANAGER_NAME_COLUMN, &info, -1);

	if (info == NULL)
		return;

	title = gtk_tree_view_column_get_title (tree_column);

	/* this string comparison stuff sucks.  is there a better way? */
	if (!strcmp (title, PLUGIN_MANAGER_NAME_TITLE))
		g_object_set (G_OBJECT (cell), "text", info->plugin->name, NULL);
}

static void
active_toggled_cb (GtkCellRendererToggle     *cell,
		   gchar                 *path_str,
		   gpointer               data)
{
	GeditDialogPluginManager *dialog = (GeditDialogPluginManager *)data;
	GtkTreeIter  iter;
	GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
	GtkTreeModel *model;
	GeditPluginInfo *info;

	gedit_debug (DEBUG_PLUGINS, "");

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (dialog->tree));
	g_return_if_fail (model != NULL);

	/* get toggled iter */
	gtk_tree_model_get_iter (model, &iter, path);

	if (&iter != NULL) {
		dialog_plugin_manager_toggle_active (&iter, model);
	}
  
	/* clean up */
	gtk_tree_path_free (path);
}

static void
cursor_changed_cb (GtkTreeView  *view, gpointer data)
{
	GeditDialogPluginManager *dialog = data;
	GeditPluginInfo *info;

	gedit_debug (DEBUG_PLUGINS, "");

	info = dialog_plugin_manager_get_selected_plugin (dialog);
	g_return_if_fail (info != NULL);

	gtk_widget_set_sensitive (GTK_WIDGET (dialog->configure_button),
				  gedit_plugins_engine_is_a_configurable_plugin (info->plugin));
		
	dialog_plugin_manager_update_info (dialog, info);
}

static void
row_activated_cb (GtkTreeView *tree_view,
		  GtkTreePath *path,
		  GtkTreeViewColumn *column,
		  gpointer data)
{
	GeditDialogPluginManager *dialog = data;
	GtkTreeIter iter;
	GtkTreeModel *model;

	gedit_debug (DEBUG_PLUGINS, "");

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (dialog->tree));

	g_return_if_fail (model != NULL);
	
	gtk_tree_model_get_iter (model, &iter, path);

	g_return_if_fail (&iter != NULL);
	
	dialog_plugin_manager_toggle_active (&iter, model);
}

static void
column_clicked_cb (GtkTreeViewColumn *tree_column, gpointer data)
{
	GeditDialogPluginManager *dialog = data;

	gedit_debug (DEBUG_PLUGINS, "");

	g_return_if_fail (dialog != NULL);

	dialog_plugin_manager_toggle_all (dialog);
}


static void
dialog_plugin_manager_populate_lists (GeditDialogPluginManager *dialog)
{
	const GList *plugins;
	GtkListStore *model;
	GtkTreeIter iter;
	
	gedit_debug (DEBUG_PLUGINS, "");

	plugins = dialog->plugins;

	model = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (dialog->tree)));

	while (plugins) {
		GeditPluginInfo *info;
		info = (GeditPluginInfo *)plugins->data;

		gtk_list_store_append (model, &iter);
		gtk_list_store_set (model, &iter, PLUGIN_MANAGER_NAME_COLUMN, info,
				    PLUGIN_MANAGER_ACTIVE_COLUMN,(info->state == GEDIT_PLUGIN_ACTIVATED),
				    -1);

		plugins = plugins->next;
	}

	if (gtk_tree_model_get_iter_root (GTK_TREE_MODEL (model), &iter))
	{
		GtkTreeSelection *selection;
		GeditPluginInfo* info;
		
		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (dialog->tree));
		g_return_if_fail (selection != NULL);
		gtk_tree_selection_select_iter (selection, &iter);

		gtk_tree_model_get (GTK_TREE_MODEL (model), &iter,
				    PLUGIN_MANAGER_NAME_COLUMN, &info, -1);

		gtk_widget_set_sensitive (GTK_WIDGET (dialog->configure_button),
				  gedit_plugins_engine_is_a_configurable_plugin (info->plugin));

		dialog_plugin_manager_update_info (dialog, info);
	}
}

static void
dialog_plugin_manager_set_active (GtkTreeIter *iter, GtkTreeModel *model, gboolean active)
{
	GeditPluginInfo *info;
	
	gedit_debug (DEBUG_PLUGINS, "");

	gtk_tree_model_get (model, iter, PLUGIN_MANAGER_NAME_COLUMN, &info, -1);

	g_return_if_fail (info != NULL);

	if (active) {
		/* activate the plugin */
		if (!gedit_plugins_engine_activate_plugin (info->plugin)) {
			gedit_debug (DEBUG_PLUGINS, "Could not activate %s.\n", info->plugin->name);
			active ^= 1;
		}
	}
	else {
		/* deactivate the plugin */
		if (!gedit_plugins_engine_deactivate_plugin (info->plugin)) {
			gedit_debug (DEBUG_PLUGINS, "Could not deactivate %s.\n", info->plugin->name);
			active ^= 1;
		}
	}
  
	/* set new value */
	gtk_list_store_set (GTK_LIST_STORE (model), iter,
			    PLUGIN_MANAGER_ACTIVE_COLUMN,
			    (info->state == GEDIT_PLUGIN_ACTIVATED), -1);

}

static void
dialog_plugin_manager_toggle_active (GtkTreeIter *iter, GtkTreeModel *model)
{
	GeditPluginInfo *info;
	gboolean active;
	
	gedit_debug (DEBUG_PLUGINS, "");

	gtk_tree_model_get (model, iter, PLUGIN_MANAGER_ACTIVE_COLUMN, &active, -1);

	g_return_if_fail (info != NULL);

	active ^= 1;

	dialog_plugin_manager_set_active (iter, model, active);
}



static void
dialog_plugin_manager_update_info (GeditDialogPluginManager *dialog, GeditPluginInfo *info)
{
	gchar *t;
	gchar *filename;
	gchar *author;
	gchar *name;

	gedit_debug (DEBUG_PLUGINS, "");

	/* maybe we should put the full path?  It's pretty long.... */
	t = g_path_get_basename (info->plugin->file);
	g_return_if_fail (t != NULL);

	filename = g_strdup_printf ("%s: %s", _("Module file name"), t);
	author = g_strdup_printf ("%s: %s", _("Author(s)"), info->plugin->author);
	name = g_strdup_printf ("%s plugin", info->plugin->name);

	gtk_label_set_text (GTK_LABEL (dialog->desc), info->plugin->desc);
	gtk_label_set_text (GTK_LABEL (dialog->author), author);
	gtk_label_set_text (GTK_LABEL (dialog->filename), filename);
	gtk_label_set_text (GTK_LABEL (dialog->copyright), info->plugin->copyright);
	gtk_label_set_text (GTK_LABEL (dialog->name), name);

	g_free (t);
	g_free (author);
	g_free (filename);
	g_free (name);
}

static GeditPluginInfo *
dialog_plugin_manager_get_selected_plugin (GeditDialogPluginManager *dialog)
{
	GeditPluginInfo *info;
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	GtkTreeViewColumn *column;
	GtkTreePath *path;

	gedit_debug (DEBUG_PLUGINS, "");

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (dialog->tree));

	gtk_tree_view_get_cursor (GTK_TREE_VIEW (dialog->tree), &path, &column);

	g_return_val_if_fail ((path != NULL) && (column != NULL), NULL);
	
	gtk_tree_model_get_iter (model, &iter, path);

	gtk_tree_model_get (model, &iter, PLUGIN_MANAGER_NAME_COLUMN, &info, -1);

	return info;
}


static void
dialog_plugin_manager_toggle_all (GeditDialogPluginManager *dialog)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	static gboolean active;

	gedit_debug (DEBUG_PLUGINS, "");

	active ^= 1;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (dialog->tree));

	g_return_if_fail (model != NULL);

	gtk_tree_model_get_iter_root (model, &iter);

	do {
		dialog_plugin_manager_set_active (&iter, model, active);		
	}
	while (gtk_tree_model_iter_next (model, &iter));
}


static void
dialog_plugin_manager_construct_tree (GeditDialogPluginManager *dialog)
{
	GtkTreeViewColumn *column;
	GtkCellRenderer *cell;
	GtkListStore *model;
	GtkWidget *button;

	gedit_debug (DEBUG_PLUGINS, "");

	model = gtk_list_store_new (2, G_TYPE_BOOLEAN, G_TYPE_POINTER);
	gtk_tree_view_set_model (GTK_TREE_VIEW (dialog->tree), GTK_TREE_MODEL (model));
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (dialog->tree), TRUE);
	gtk_tree_view_set_headers_clickable (GTK_TREE_VIEW (dialog->tree), TRUE);

	/* first column */
	cell = gtk_cell_renderer_toggle_new ();
	g_signal_connect (G_OBJECT (cell), "toggled", G_CALLBACK (active_toggled_cb), dialog);
	column = gtk_tree_view_column_new_with_attributes (PLUGIN_MANAGER_ACTIVE_TITLE,
							   cell, "active",
							   PLUGIN_MANAGER_ACTIVE_COLUMN, NULL);
	gtk_tree_view_column_set_clickable (column, TRUE);
	gtk_tree_view_column_set_resizable (column, TRUE);
	g_signal_connect (G_OBJECT (column), "clicked", G_CALLBACK (column_clicked_cb), dialog);
	gtk_tree_view_append_column (GTK_TREE_VIEW (dialog->tree), column);

	/* the second column */
	cell = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (PLUGIN_MANAGER_NAME_TITLE, cell, NULL);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_cell_data_func (column, cell, dialog_plugin_manager_view_cell_cb,
						 dialog, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (dialog->tree), column);


	g_signal_connect (G_OBJECT (dialog->tree), "cursor_changed",
			  G_CALLBACK (cursor_changed_cb), dialog);
	g_signal_connect (G_OBJECT (dialog->tree), "row_activated",
			  G_CALLBACK (row_activated_cb), dialog);


	gtk_widget_show (dialog->tree);
}

static GeditDialogPluginManager *
dialog_plugin_manager_get_dialog (void)
{
	static GeditDialogPluginManager *dialog = NULL;
	GladeXML *gui;
	GtkWindow *window;
	GtkWidget *content;
	GtkWidget *button;
	GtkWidget *viewport;

	gedit_debug (DEBUG_PLUGINS, "");

	if (dialog != NULL)
	{
		gdk_window_show (dialog->dialog->window);
		gdk_window_raise (dialog->dialog->window);

		return dialog;
	}

	gui = glade_xml_new (GEDIT_GLADEDIR "plugin-manager.glade2",
			     "plugin_manager_dialog_content", NULL);

	if (!gui) {
		g_warning (_("Could not find plugin-manager.glade2, reinstall gedit.\n"));
		return NULL;
	}

	window =  GTK_WINDOW (gedit_get_active_window ());

	dialog = g_new0 (GeditDialogPluginManager, 1);

	dialog->dialog = gtk_dialog_new_with_buttons (_("Plugin Manager"),
						      window,
						      GTK_DIALOG_DESTROY_WITH_PARENT,
						      GTK_STOCK_CLOSE,
						      GTK_RESPONSE_CLOSE,
						      GTK_STOCK_HELP,
						      GTK_RESPONSE_HELP,
						      NULL);

	g_return_val_if_fail (dialog->dialog != NULL, NULL);

	content = glade_xml_get_widget (gui, "plugin_manager_dialog_content");
	dialog->tree = glade_xml_get_widget (gui, "plugin_tree");
	dialog->notebook = glade_xml_get_widget (gui, "plugin_notebook");
	dialog->desc = glade_xml_get_widget (gui, "desc_label");
	dialog->author = glade_xml_get_widget (gui, "author_label");
	dialog->filename = glade_xml_get_widget (gui, "file_label");
	dialog->name = glade_xml_get_widget (gui, "name_label");
	dialog->copyright = glade_xml_get_widget (gui, "copyright_label");
	dialog->configure_button = glade_xml_get_widget (gui, "configure_button");
	dialog->logo = glade_xml_get_widget (gui, "plugin_logo");
	viewport = glade_xml_get_widget (gui, "plugin_viewport");

	if (!(content && dialog->tree && dialog->notebook && dialog->desc &&
	      dialog->author && dialog->filename && dialog->configure_button &&
	      dialog->logo && viewport && dialog->name && dialog->copyright)) {

		g_warning (_("Invalid glade file for plugin manager -- not all widgets found.\n"));
		g_object_unref (gui);

		return NULL;
	}

	/* setup a window of a sane size. */
	gtk_widget_set_size_request (GTK_WIDGET (dialog->notebook), 270, 140);
	gtk_widget_set_size_request (GTK_WIDGET (viewport), 270, 140);
	
	/* stick the plugin manager logo in there */
	gtk_image_set_from_file (GTK_IMAGE (dialog->logo), GNOME_ICONDIR PLUGIN_MANAGER_LOGO);

	/* connect something to the "configure" button */
	g_signal_connect (G_OBJECT (dialog->configure_button), "clicked",
			  G_CALLBACK (configure_button_cb), dialog);

	dialog_plugin_manager_construct_tree (dialog);

	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog->dialog)->vbox),
			    content, FALSE, FALSE, 0);

	gtk_dialog_set_default_response (GTK_DIALOG (dialog->dialog),
					 GTK_RESPONSE_CLOSE);

	gtk_signal_connect(GTK_OBJECT (dialog->dialog), "destroy",
			   GTK_SIGNAL_FUNC (dialog_destroyed), &dialog);

	gtk_signal_connect(GTK_OBJECT (dialog->dialog), "response",
			   GTK_SIGNAL_FUNC (dialog_response_handler), dialog);

	g_object_unref (gui);

	gtk_window_set_resizable (GTK_WINDOW (dialog->dialog), FALSE);

	/* get the list of available plugins (or installed) */
	dialog->plugins = gedit_plugins_engine_get_plugins_list ();

	dialog_plugin_manager_populate_lists (dialog);

	return dialog;
}

static void
help_button_pressed (GeditDialogPluginManager * dialog)
{
	/* FIXME */
	gedit_debug (DEBUG_PLUGINS, "");

}

static void
dialog_destroyed (GtkObject *obj,  void **dialog_pointer)
{
	gedit_debug (DEBUG_PLUGINS, "");

	if (dialog_pointer != NULL)
	{
		g_free (*dialog_pointer);
		*dialog_pointer = NULL;
	}	
}

static void
dialog_response_handler (GtkDialog *dlg, gint res_id,  GeditDialogPluginManager *dialog)
{
	gedit_debug (DEBUG_PLUGINS, "");

	switch (res_id) {
		case GTK_RESPONSE_HELP:
			help_button_pressed (dialog);
			break;
			
		default:
			gtk_widget_destroy (dialog->dialog);
	}
}


void
gedit_dialog_plugin_manager (void)
{
	GeditDialogPluginManager *dialog;
	gint response;

	gedit_debug (DEBUG_PLUGINS, "");

	dialog = dialog_plugin_manager_get_dialog ();
	if (dialog == NULL) {
		g_warning (_("Could not create the Plugin Manager dialog"));
		return;
	}
	
	gtk_window_set_transient_for (GTK_WINDOW (dialog->dialog),
				      GTK_WINDOW (gedit_get_active_window ()));

	if (!GTK_WIDGET_VISIBLE (dialog->dialog))
		gtk_widget_show (dialog->dialog);

	gtk_widget_grab_focus (dialog->tree);
}


