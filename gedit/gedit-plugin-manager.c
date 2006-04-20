/*
 * gedit-plugin-manager.c
 * This file is part of gedit
 *
 * Copyright (C) 2002 Paolo Maggi and James Willcox
 * Copyright (C) 2003-2005 Paolo Maggi
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
 * Modified by the gedit Team, 1998-2005. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 *
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include <glib/gi18n.h>
#include <glade/glade-xml.h>
#include <libgnome/gnome-url.h>

#include "gedit-plugin-manager.h"
#include "gedit-utils.h"
#include "gedit-plugins-engine.h"
#include "gedit-plugin.h"
#include "gedit-debug.h"

enum
{
	ACTIVE_COLUMN,
	NAME_COLUMN,
	N_COLUMNS
};

#define PLUGIN_MANAGER_NAME_TITLE _("Plugin")
#define PLUGIN_MANAGER_ACTIVE_TITLE _("Enabled")

#define GEDIT_PLUGIN_MANAGER_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), GEDIT_TYPE_PLUGIN_MANAGER, GeditPluginManagerPrivate))

struct _GeditPluginManagerPrivate
{
	GtkWidget	*tree;

	GtkWidget	*about_button;
	GtkWidget	*configure_button;

	const GList	*plugins;

	GtkWidget 	*about;
};

G_DEFINE_TYPE(GeditPluginManager, gedit_plugin_manager, GTK_TYPE_VBOX)

static GeditPluginInfo *plugin_manager_get_selected_plugin (GeditPluginManager *pm); 
static void plugin_manager_toggle_active (GtkTreeIter *iter, GtkTreeModel *model); 
static void plugin_manager_toggle_all (GeditPluginManager *pm); 

static void 
gedit_plugin_manager_class_init (GeditPluginManagerClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (object_class, sizeof (GeditPluginManagerPrivate));
}

static void
about_button_cb (GtkWidget          *button,
		 GeditPluginManager *pm)
{
	GeditPluginInfo *info;

	gedit_debug (DEBUG_PLUGINS);

	info = plugin_manager_get_selected_plugin (pm);

	g_return_if_fail (info != NULL);

	/* if there is another about dialog already open destroy it */
	if (pm->priv->about)
		gtk_widget_destroy (pm->priv->about);

	pm->priv->about = g_object_new (GTK_TYPE_ABOUT_DIALOG,
		"name", gedit_plugins_engine_get_plugin_name (info),
		"copyright", gedit_plugins_engine_get_plugin_copyright (info),
		"authors", gedit_plugins_engine_get_plugin_authors (info),
		"comments", gedit_plugins_engine_get_plugin_description (info),
		"website", gedit_plugins_engine_get_plugin_website (info),
		"logo-icon-name", "gedit-plugin",
		NULL);

	gtk_window_set_destroy_with_parent (GTK_WINDOW (pm->priv->about),
					    TRUE);

	g_signal_connect (pm->priv->about,
			  "response",
			  G_CALLBACK (gtk_widget_destroy),
			  NULL);
	g_signal_connect (pm->priv->about,
			  "destroy",
			  G_CALLBACK (gtk_widget_destroyed),
			  &pm->priv->about);

	gtk_window_set_transient_for (GTK_WINDOW (pm->priv->about),
				      GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET(pm))));
	gtk_widget_show (pm->priv->about);
}

static void
configure_button_cb (GtkWidget          *button,
		     GeditPluginManager *pm)
{
	GeditPluginInfo *info;
	GtkWindow *toplevel;

	gedit_debug (DEBUG_PLUGINS);

	info = plugin_manager_get_selected_plugin (pm);

	g_return_if_fail (info != NULL);

	gedit_debug_message (DEBUG_PLUGINS, "Configuring: %s\n", 
			     gedit_plugins_engine_get_plugin_name (info));

	toplevel = GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET(pm)));

	gedit_plugins_engine_configure_plugin (info, toplevel);

	gedit_debug_message (DEBUG_PLUGINS, "Done");	
}

static void
plugin_manager_view_cell_cb (GtkTreeViewColumn *tree_column,
			     GtkCellRenderer   *cell,
			     GtkTreeModel      *tree_model,
			     GtkTreeIter       *iter,
			     gpointer           data)
{
	GeditPluginInfo *info;
	gchar *text;
	
	g_return_if_fail (tree_model != NULL);
	g_return_if_fail (tree_column != NULL);

	gtk_tree_model_get (tree_model, iter, NAME_COLUMN, &info, -1);

	if (info == NULL)
		return;

	text = g_markup_printf_escaped ("<b>%s</b>\n%s",
					gedit_plugins_engine_get_plugin_name (info),
					gedit_plugins_engine_get_plugin_description (info));
	g_object_set (G_OBJECT (cell),
		      "markup",
		      text,
		      NULL);
	g_free (text);
}

static void
active_toggled_cb (GtkCellRendererToggle *cell,
		   gchar                 *path_str,
		   GeditPluginManager    *pm)
{
	GtkTreeIter iter;
	GtkTreePath *path;
	GtkTreeModel *model;

	gedit_debug (DEBUG_PLUGINS);

	path = gtk_tree_path_new_from_string (path_str);

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (pm->priv->tree));
	g_return_if_fail (model != NULL);

	gtk_tree_model_get_iter (model, &iter, path);

	if (&iter != NULL)
		plugin_manager_toggle_active (&iter, model);

	gtk_tree_path_free (path);
}

static void
cursor_changed_cb (GtkTreeView *view,
		   gpointer     data)
{
	GeditPluginManager *pm = data;
	GeditPluginInfo *info;

	gedit_debug (DEBUG_PLUGINS);

	info = plugin_manager_get_selected_plugin (pm);

	gtk_widget_set_sensitive (GTK_WIDGET (pm->priv->about_button),
				  info != NULL);
	gtk_widget_set_sensitive (GTK_WIDGET (pm->priv->configure_button),
				  (info != NULL) && 
				   gedit_plugins_engine_plugin_is_configurable (info));
}

static void
row_activated_cb (GtkTreeView       *tree_view,
		  GtkTreePath       *path,
		  GtkTreeViewColumn *column,
		  gpointer           data)
{
	GeditPluginManager *pm = data;
	GtkTreeIter iter;
	GtkTreeModel *model;

	gedit_debug (DEBUG_PLUGINS);

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (pm->priv->tree));

	g_return_if_fail (model != NULL);

	gtk_tree_model_get_iter (model, &iter, path);

	g_return_if_fail (&iter != NULL);

	plugin_manager_toggle_active (&iter, model);
}

static void
column_clicked_cb (GtkTreeViewColumn *tree_column,
		   gpointer           data)
{
	GeditPluginManager *pm = data;

	gedit_debug (DEBUG_PLUGINS);

	g_return_if_fail (pm != NULL);

	plugin_manager_toggle_all (pm);
}

static void
plugin_manager_populate_lists (GeditPluginManager *pm)
{
	const GList *plugins;
	GtkListStore *model;
	GtkTreeIter iter;

	gedit_debug (DEBUG_PLUGINS);

	plugins = pm->priv->plugins;

	model = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (pm->priv->tree)));

	while (plugins)
	{
		GeditPluginInfo *info;
		info = (GeditPluginInfo *)plugins->data;

		gtk_list_store_append (model, &iter);
		gtk_list_store_set (model, &iter,
				    ACTIVE_COLUMN, gedit_plugins_engine_plugin_is_active (info),
				    NAME_COLUMN, info,
				    -1);

		plugins = plugins->next;
	}

	if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (model), &iter))
	{
		GtkTreeSelection *selection;
		GeditPluginInfo* info;

		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (pm->priv->tree));
		g_return_if_fail (selection != NULL);
		
		gtk_tree_selection_select_iter (selection, &iter);

		gtk_tree_model_get (GTK_TREE_MODEL (model), &iter,
				    NAME_COLUMN, &info, -1);

		gtk_widget_set_sensitive (GTK_WIDGET (pm->priv->configure_button),
					  gedit_plugins_engine_plugin_is_configurable (info));
	}
}

static void
plugin_manager_set_active (GtkTreeIter  *iter,
			   GtkTreeModel *model,
			   gboolean      active)
{
	GeditPluginInfo *info;
	
	gedit_debug (DEBUG_PLUGINS);

	gtk_tree_model_get (model, iter, NAME_COLUMN, &info, -1);

	g_return_if_fail (info != NULL);

	if (active)
	{
		/* activate the plugin */
		if (!gedit_plugins_engine_activate_plugin (info)) {
			gedit_debug_message (DEBUG_PLUGINS, "Could not activate %s.\n", 
					     gedit_plugins_engine_get_plugin_name (info));
			active ^= 1;
		}
	}
	else
	{
		/* deactivate the plugin */
		if (!gedit_plugins_engine_deactivate_plugin (info)) {
			gedit_debug_message (DEBUG_PLUGINS, "Could not deactivate %s.\n", 
					     gedit_plugins_engine_get_plugin_name (info));

			active ^= 1;
		}
	}
  
	/* set new value */
	gtk_list_store_set (GTK_LIST_STORE (model), 
			    iter, 
			    ACTIVE_COLUMN,
			    gedit_plugins_engine_plugin_is_active (info), 
			    -1);
}

static void
plugin_manager_toggle_active (GtkTreeIter  *iter,
			      GtkTreeModel *model)
{
	gboolean active;
	
	gedit_debug (DEBUG_PLUGINS);

	gtk_tree_model_get (model, iter, ACTIVE_COLUMN, &active, -1);

	active ^= 1;

	plugin_manager_set_active (iter, model, active);
}

static GeditPluginInfo *
plugin_manager_get_selected_plugin (GeditPluginManager *pm)
{
	GeditPluginInfo *info = NULL;
	GtkTreeModel *model;
	GtkTreeIter iter;
	GtkTreeSelection *selection;

	gedit_debug (DEBUG_PLUGINS);

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (pm->priv->tree));
	g_return_val_if_fail (model != NULL, NULL);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (pm->priv->tree));
	g_return_val_if_fail (selection != NULL, NULL);

	if (gtk_tree_selection_get_selected (selection, NULL, &iter))
	{
		gtk_tree_model_get (model, &iter, NAME_COLUMN, &info, -1);
	}
	
	return info;
}

static void
plugin_manager_toggle_all (GeditPluginManager *pm)
{
	GtkTreeModel *model;
	GtkTreeIter iter;
	static gboolean active;

	gedit_debug (DEBUG_PLUGINS);

	active ^= 1;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (pm->priv->tree));

	g_return_if_fail (model != NULL);

	gtk_tree_model_get_iter_first (model, &iter);

	do {
		plugin_manager_set_active (&iter, model, active);		
	}
	while (gtk_tree_model_iter_next (model, &iter));
}

/* Callback used as the interactive search comparison function */
static gboolean
name_search_cb (GtkTreeModel *model,
		gint          column,
		const gchar  *key,
		GtkTreeIter  *iter,
		gpointer      data)
{
	GeditPluginInfo *info;
	gchar *normalized_string;
	gchar *normalized_key;
	gchar *case_normalized_string;
	gchar *case_normalized_key;
	gint key_len;
	gboolean retval;

	gtk_tree_model_get (model, iter, NAME_COLUMN, &info, -1);
	if (!info)
		return FALSE;

	normalized_string = g_utf8_normalize (gedit_plugins_engine_get_plugin_name (info), -1, G_NORMALIZE_ALL);
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
plugin_manager_construct_tree (GeditPluginManager *pm)
{
	GtkTreeViewColumn *column;
	GtkCellRenderer *cell;
	GtkListStore *model;

	gedit_debug (DEBUG_PLUGINS);

	model = gtk_list_store_new (N_COLUMNS, G_TYPE_BOOLEAN, G_TYPE_POINTER);

	gtk_tree_view_set_model (GTK_TREE_VIEW (pm->priv->tree), GTK_TREE_MODEL (model));
	g_object_unref (model);

	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (pm->priv->tree), TRUE);
	gtk_tree_view_set_headers_clickable (GTK_TREE_VIEW (pm->priv->tree), TRUE);

	/* first column */
	cell = gtk_cell_renderer_toggle_new ();
	g_signal_connect (cell,
			  "toggled",
			  G_CALLBACK (active_toggled_cb),
			  pm);
	column = gtk_tree_view_column_new_with_attributes (PLUGIN_MANAGER_ACTIVE_TITLE,
							   cell,
							  "active",
							   ACTIVE_COLUMN,
							   NULL);
	gtk_tree_view_column_set_clickable (column, TRUE);
	gtk_tree_view_column_set_resizable (column, TRUE);
	g_signal_connect (column, "clicked", G_CALLBACK (column_clicked_cb), pm);
	gtk_tree_view_append_column (GTK_TREE_VIEW (pm->priv->tree), column);

	/* second column */
	cell = gtk_cell_renderer_text_new ();
	g_object_set (cell, "ellipsize", PANGO_ELLIPSIZE_END, NULL);
	column = gtk_tree_view_column_new_with_attributes (PLUGIN_MANAGER_NAME_TITLE, cell, NULL);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_cell_data_func (column, cell, plugin_manager_view_cell_cb,
						 pm, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (pm->priv->tree), column);

	/* Enable search for our non-string column */
	gtk_tree_view_set_search_column (GTK_TREE_VIEW (pm->priv->tree), NAME_COLUMN);
	gtk_tree_view_set_search_equal_func (GTK_TREE_VIEW (pm->priv->tree),
					     name_search_cb,
					     NULL,
					     NULL);

	g_signal_connect (pm->priv->tree,
			  "cursor_changed",
			  G_CALLBACK (cursor_changed_cb),
			  pm);
	g_signal_connect (pm->priv->tree,
			  "row_activated",
			  G_CALLBACK (row_activated_cb),
			  pm);

	gtk_widget_show (pm->priv->tree);
}

static void 
gedit_plugin_manager_init (GeditPluginManager *pm)
{
	GtkWidget *viewport;
	GtkWidget *hbuttonbox;

	gedit_debug (DEBUG_PLUGINS);

	pm->priv = GEDIT_PLUGIN_MANAGER_GET_PRIVATE (pm);

	gtk_box_set_spacing (GTK_BOX (pm), 6);

	viewport = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (viewport),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (viewport), 
					     GTK_SHADOW_IN);

	gtk_box_pack_start (GTK_BOX (pm), viewport, TRUE, TRUE, 0);

	pm->priv->tree = gtk_tree_view_new ();
	gtk_container_add (GTK_CONTAINER (viewport), pm->priv->tree);

	hbuttonbox = gtk_hbutton_box_new ();
	gtk_box_pack_start (GTK_BOX (pm), hbuttonbox, FALSE, FALSE, 0);
	gtk_button_box_set_layout (GTK_BUTTON_BOX (hbuttonbox), GTK_BUTTONBOX_END);
	gtk_box_set_spacing (GTK_BOX (hbuttonbox), 8);

	pm->priv->about_button = gedit_gtk_button_new_with_stock_icon (_("_About Plugin"),
								       GTK_STOCK_ABOUT);
	gtk_container_add (GTK_CONTAINER (hbuttonbox), pm->priv->about_button);

	pm->priv->configure_button = gedit_gtk_button_new_with_stock_icon (_("C_onfigure Plugin"),
									   GTK_STOCK_PREFERENCES);
	gtk_container_add (GTK_CONTAINER (hbuttonbox), pm->priv->configure_button);

	/* setup a window of a sane size. */
	gtk_widget_set_size_request (GTK_WIDGET (viewport), 270, 100);

	g_signal_connect (pm->priv->about_button,
			  "clicked",
			  G_CALLBACK (about_button_cb),
			  pm);
	g_signal_connect (pm->priv->configure_button,
			  "clicked",
			  G_CALLBACK (configure_button_cb),
			  pm);

	plugin_manager_construct_tree (pm);

	/* get the list of available plugins (or installed) */
	pm->priv->plugins = gedit_plugins_engine_get_plugins_list ();

	if (pm->priv->plugins != NULL)
	{
		plugin_manager_populate_lists (pm);
	}
	else
	{
		gtk_widget_set_sensitive (pm->priv->about_button, FALSE);
		gtk_widget_set_sensitive (pm->priv->configure_button, FALSE);		
	}
}

GtkWidget *gedit_plugin_manager_new (void)
{
	return g_object_new (GEDIT_TYPE_PLUGIN_MANAGER,0);
}
