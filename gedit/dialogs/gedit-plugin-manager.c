/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-plugin-manager.c
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libgnome/libgnome.h>
#include <libgnomeui/libgnomeui.h>
#include <glade/glade-xml.h>

#include <string.h>

#include "gedit-plugin-manager.h"

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
#define PLUGIN_MANAGER_ACTIVE_TITLE _("Enabled")

#define PLUGIN_MANAGER_LOGO "/gedit-plugin-manager.png"

typedef struct _GeditPluginManager GeditPluginManager;

struct _GeditPluginManager {
	GtkWidget *page; 	/* a GtkNotebook page */

	GtkWidget *tree; 	/* a GtkTreeView, shows plugins */
	GtkWidget *notebook; 	/* a GtkNotebook, shows info about plugins */

	GtkWidget *about_button; /* a GtkButton, show info about a plugin when clicked */

	GtkWidget *configure_button; /* a GtkButton, configures a plugin when clicked */

	const GList *plugins; 	/* a list of type GeditPlugin  */
};

static GeditPluginInfo *plugin_manager_get_selected_plugin (GeditPluginManager *dialog);
static void plugin_manager_toggle_active (GtkTreeIter *iter, GtkTreeModel *model);
static void plugin_manager_toggle_all (GeditPluginManager *dialog);

static void plugin_manager_destroyed (GtkObject *obj,  void *dialog_pointer);


static void
about_button_cb (GtkWidget *button, GeditPluginManager *pm)
{
	static GtkWidget *about;
	GeditPluginInfo *info;
	gchar **authors;
	GdkPixbuf* pixbuf = NULL;

	gedit_debug (DEBUG_PLUGINS, "");

	info = plugin_manager_get_selected_plugin (pm);

	g_return_if_fail (info != NULL);

	gedit_debug (DEBUG_PLUGINS, "About: %s\n", info->plugin->name);

	authors = g_strsplit (info->plugin->author, ", ", 0); 

	pixbuf = gdk_pixbuf_new_from_file ( GNOME_ICONDIR "/gedit-plugin-manager.png", NULL);

	/* if there is another about dialog already open destroy it */
	if (about)
		gtk_widget_destroy (about);

	about = gnome_about_new (info->plugin->name,
				 NULL,
				 info->plugin->copyright,
				 info->plugin->desc,
				 (const gchar**) authors,
				 NULL,
				 NULL,
				 pixbuf);

	g_signal_connect (G_OBJECT (about), "destroy",
			  G_CALLBACK (gtk_widget_destroyed), &about);

	if (pixbuf != NULL)
		g_object_unref (pixbuf);

	if (authors != NULL)
		g_strfreev (authors);

	gtk_window_set_transient_for (GTK_WINDOW (about),
			GTK_WINDOW (gtk_widget_get_toplevel (pm->page)));

	gtk_window_set_destroy_with_parent (GTK_WINDOW (about), TRUE);

	gtk_widget_show (about);

	gedit_debug (DEBUG_PLUGINS, "Done");	
}

static void
configure_button_cb (GtkWidget *button, gpointer data)
{
	GeditPluginManager *pm = data;
	GeditPluginInfo *info;

	gedit_debug (DEBUG_PLUGINS, "");
	
	info = plugin_manager_get_selected_plugin (pm);

	g_return_if_fail (info != NULL);

	gedit_debug (DEBUG_PLUGINS, "Configuring: %s\n", info->plugin->name);

	gedit_plugins_engine_configure_plugin (info->plugin, 
			gtk_widget_get_toplevel (pm->page));

	gedit_debug (DEBUG_PLUGINS, "Done");	
}

static void
plugin_manager_view_cell_cb (GtkTreeViewColumn *tree_column,
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

	/* FIXME: this string comparison stuff sucks.  is there a better way? */
	if (!strcmp (title, PLUGIN_MANAGER_NAME_TITLE))
		g_object_set (G_OBJECT (cell), "text", info->plugin->name, NULL);
}

static void
active_toggled_cb (GtkCellRendererToggle     *cell,
		   gchar                 *path_str,
		   gpointer               data)
{
	GeditPluginManager *dialog = (GeditPluginManager *)data;
	GtkTreeIter  iter;
	GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
	GtkTreeModel *model;

	gedit_debug (DEBUG_PLUGINS, "");

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (dialog->tree));
	g_return_if_fail (model != NULL);

	/* get toggled iter */
	gtk_tree_model_get_iter (model, &iter, path);

	if (&iter != NULL) {
		plugin_manager_toggle_active (&iter, model);
	}
  
	/* clean up */
	gtk_tree_path_free (path);
}

static void
cursor_changed_cb (GtkTreeView  *view, gpointer data)
{
	GeditPluginManager *dialog = data;
	GeditPluginInfo *info;

	gedit_debug (DEBUG_PLUGINS, "");

	info = plugin_manager_get_selected_plugin (dialog);
	g_return_if_fail (info != NULL);

	gtk_widget_set_sensitive (GTK_WIDGET (dialog->configure_button),
				  gedit_plugins_engine_is_a_configurable_plugin (info->plugin));
}

static void
row_activated_cb (GtkTreeView *tree_view,
		  GtkTreePath *path,
		  GtkTreeViewColumn *column,
		  gpointer data)
{
	GeditPluginManager *dialog = data;
	GtkTreeIter iter;
	GtkTreeModel *model;

	gedit_debug (DEBUG_PLUGINS, "");

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (dialog->tree));

	g_return_if_fail (model != NULL);
	
	gtk_tree_model_get_iter (model, &iter, path);

	g_return_if_fail (&iter != NULL);
	
	plugin_manager_toggle_active (&iter, model);
}

static void
column_clicked_cb (GtkTreeViewColumn *tree_column, gpointer data)
{
	GeditPluginManager *dialog = data;

	gedit_debug (DEBUG_PLUGINS, "");

	g_return_if_fail (dialog != NULL);

	plugin_manager_toggle_all (dialog);
}


static void
plugin_manager_populate_lists (GeditPluginManager *dialog)
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
	}
}

static void
plugin_manager_set_active (GtkTreeIter *iter, GtkTreeModel *model, gboolean active)
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
plugin_manager_toggle_active (GtkTreeIter *iter, GtkTreeModel *model)
{
	gboolean active;
	
	gedit_debug (DEBUG_PLUGINS, "");

	gtk_tree_model_get (model, iter, PLUGIN_MANAGER_ACTIVE_COLUMN, &active, -1);

	active ^= 1;

	plugin_manager_set_active (iter, model, active);
}


static GeditPluginInfo *
plugin_manager_get_selected_plugin (GeditPluginManager *dialog)
{
	GeditPluginInfo *info = NULL;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreeSelection *selection;

	gedit_debug (DEBUG_PLUGINS, "");

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (dialog->tree));
	g_return_val_if_fail (model != NULL, NULL);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (dialog->tree));
	g_return_val_if_fail (selection != NULL, NULL);

	if (gtk_tree_selection_get_selected (selection, NULL, &iter))
	{
		gtk_tree_model_get (model, &iter, PLUGIN_MANAGER_NAME_COLUMN, &info, -1);
	}
	
	return info;
}

static void
plugin_manager_toggle_all (GeditPluginManager *dialog)
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
		plugin_manager_set_active (&iter, model, active);		
	}
	while (gtk_tree_model_iter_next (model, &iter));
}

/* Callback used as the interactive search comparison function */
static gboolean
name_search_cb (GtkTreeModel *model,
		gint column,
		const gchar *key,
		GtkTreeIter *iter,
		gpointer data)
{
	GeditPluginInfo *info;
	gchar *normalized_string;
	gchar *normalized_key;
	gchar *case_normalized_string;
	gchar *case_normalized_key;
	gint key_len;
	gboolean retval;

	gtk_tree_model_get (model, iter, PLUGIN_MANAGER_NAME_COLUMN, &info, -1);
	if (!info)
		return FALSE;

	normalized_string = g_utf8_normalize (info->plugin->name, -1, G_NORMALIZE_ALL);
	normalized_key = g_utf8_normalize (key, -1, G_NORMALIZE_ALL);
	case_normalized_string = g_utf8_casefold (normalized_string, -1);
	case_normalized_key = g_utf8_casefold (normalized_key, -1);

	key_len = strlen (case_normalized_key);

	/* Oddly enough, this callback must return whether to stop the search
	 * because we found a match, not whether we actually matched.
	 */
	retval = (strncmp (case_normalized_key, case_normalized_string, key_len) != 0);

	g_free (normalized_key);
	g_free (normalized_string);
	g_free (case_normalized_key);
	g_free (case_normalized_string);

	return retval;
}

static void
plugin_manager_construct_tree (GeditPluginManager *dialog)
{
	GtkTreeViewColumn *column;
	GtkCellRenderer *cell;
	GtkListStore *model;

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
	gtk_tree_view_column_set_cell_data_func (column, cell, plugin_manager_view_cell_cb,
						 dialog, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (dialog->tree), column);

	/* Enable search for our non-string column */
	gtk_tree_view_set_search_column (GTK_TREE_VIEW (dialog->tree), PLUGIN_MANAGER_NAME_COLUMN);
	gtk_tree_view_set_search_equal_func (GTK_TREE_VIEW (dialog->tree),
					     name_search_cb,
					     NULL,
					     NULL);

	g_signal_connect (G_OBJECT (dialog->tree), "cursor_changed",
			  G_CALLBACK (cursor_changed_cb), dialog);
	g_signal_connect (G_OBJECT (dialog->tree), "row_activated",
			  G_CALLBACK (row_activated_cb), dialog);

	g_object_unref (G_OBJECT (model));
	
	gtk_widget_show (dialog->tree);
}

GtkWidget *
gedit_plugin_manager_get_page (void)
{
	GeditPluginManager *pm = NULL;
	GladeXML *gui;
	GtkWidget *content;
	GtkWidget *viewport;

	gedit_debug (DEBUG_PLUGINS, "");

	gui = glade_xml_new (GEDIT_GLADEDIR "plugin-manager.glade2",
			     "plugin_manager_dialog_content", NULL);

	if (!gui) {
		g_warning (_("Could not find plugin-manager.glade2, reinstall gedit.\n"));
		return NULL;
	}

	pm = g_new0 (GeditPluginManager, 1);

	pm->page = gtk_vbox_new (FALSE, 0);

	content = glade_xml_get_widget (gui, "plugin_manager_dialog_content");
	pm->tree = glade_xml_get_widget (gui, "plugin_tree");

	pm->about_button = glade_xml_get_widget (gui, "about_button");
	pm->configure_button = glade_xml_get_widget (gui, "configure_button");
	viewport = glade_xml_get_widget (gui, "plugin_viewport");

	if (!(content && pm->tree && pm->configure_button &&
	      viewport && pm->about_button)) {

		g_warning (_("Invalid glade file for plugin manager -- not all widgets found.\n"));
		g_object_unref (gui);

		return NULL;
	}

	/* setup a window of a sane size. */
	gtk_widget_set_size_request (GTK_WIDGET (viewport), 270, 100);

	/* connect something to the "about" button */
	g_signal_connect (G_OBJECT (pm->about_button), "clicked",
			  G_CALLBACK (about_button_cb), pm);

	/* connect something to the "configure" button */
	g_signal_connect (G_OBJECT (pm->configure_button), "clicked",
			  G_CALLBACK (configure_button_cb), pm);

	plugin_manager_construct_tree (pm);

	gtk_box_pack_start (GTK_BOX (pm->page),
			    content, TRUE, TRUE, 0);

	g_signal_connect(G_OBJECT (pm->page), "destroy",
			 G_CALLBACK (plugin_manager_destroyed), pm);

	g_object_unref (gui);

	/* get the list of available plugins (or installed) */
	pm->plugins = gedit_plugins_engine_get_plugins_list ();

	plugin_manager_populate_lists (pm);

	return pm->page;
}

static void
plugin_manager_destroyed (GtkObject *obj,  void *pm_pointer)
{
	gedit_debug (DEBUG_PLUGINS, "");

	if (pm_pointer != NULL)
		g_free (pm_pointer);

}

