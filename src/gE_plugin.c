/* gE_plugin.c - implements plugin features using gmodule
 *
 * Copyright (C) 1998 The Free Software Foundation.
 * Contributed by Martin Baulig <martin@home-of-linux.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <dirent.h>
#include <gtk/gtk.h>

#include "main.h"

#include <gE_plugin.h>
#include <gE_plugin_api.h>

#include <gnome.h>

GHashTable *shlib_hash = NULL;

/*
 * gE_Plugin_Init - Get a list of all plugins and add them to the menu.
 *
 */

void
gE_Plugin_Query_All (void)
{
	DIR *dir = opendir (PLUGINLIBDIR);
	struct dirent *direntry;
	gchar *shortname;

	if (!dir) return;

	while ((direntry = readdir (dir))) {
		gE_Plugin_Object *plug;
		gchar *suffix;
		
		if (strrchr (direntry->d_name, '/'))
			shortname = strrchr (direntry->d_name, '/') + 1;
		else
			shortname = direntry->d_name;

		if (!strcmp (shortname, ".") || !strcmp (shortname, ".."))
			continue;

		suffix = strrchr (direntry->d_name, '.');
		if (!suffix || strcmp (suffix, ".plugin"))
			continue;

		fprintf (stderr, "Loading plugin description from `%s'.\n",
			 direntry->d_name);

		plug = gE_Plugin_Query (direntry->d_name);
		if (!plug) continue;

		gE_Plugin_Register (plug);
	}

	closedir (dir);
}

static gint
compare_func (gconstpointer a, gconstpointer b)
{
	return strcmp (a, b);
}

static void
load_library (gchar *key, gE_Plugin_Object *plug)
{
	gchar *filename = gnome_config_get_string (key);
	gchar *pathname;
	GModule *module;

	if (*filename == '/')
		pathname = filename;
	else {
		pathname = g_strconcat (PLUGINLIBDIR, "/", filename, NULL);
		g_free (filename);
	}

	if (!shlib_hash) {
		shlib_hash = g_hash_table_new (NULL, NULL);
	} else if (g_hash_table_lookup (shlib_hash, pathname)) {
		fprintf (stderr, "Library %s already loaded.\n", pathname);
		return;
	}

	fprintf (stderr, "Loading %s ...\n", pathname);

	module = g_module_open (pathname, G_MODULE_BIND_LAZY);
	if (!module) {
		g_print ("error: %s\n", g_module_error ());
		return;
	}

	g_hash_table_insert (shlib_hash, pathname, module);

	/* Only set this if we are loading the "real" plugin and
	 * no dependency library. */

	if (!strcmp (key, "library_name"))
		plug->module = module;
}

/*
 * gE_Plugin_Query - Read description file of a Plugin and add it
 *                   to the plugins menu, but do not load the Plugin.
 */

gE_Plugin_Object *
gE_Plugin_Query (gchar *plugin_name)
{
	gE_Plugin_Object *new_plugin = g_new0 (gE_Plugin_Object, 1);

	gchar *key, *value;
	GString *dummy;
	gpointer iter;

	/* Set up path names. */
  
	new_plugin->name = g_strdup (strrchr (plugin_name, '/') ?
				     strrchr (plugin_name, '/') + 1 :
				     plugin_name);

	new_plugin->config_path = g_strconcat
		("=", PLUGINLIBDIR, "/", plugin_name, "=/", "Plugin/", NULL);

	dummy = g_string_new ("");

	/* Get new config iterator. */

	iter = gnome_config_init_iterator (new_plugin->config_path);

	if (!iter) {
		g_warning ("Invalid description file for plugin `%s'.\n",
			   plugin_name);
		goto load_error;
	}

	/* Look up dependency libraries in the description file. */

	while ((iter = gnome_config_iterator_next (iter, &key, &value))) {
		if (!strncmp (key, "deplib_", 7))
			new_plugin->dependency_libs = g_list_insert_sorted
				(new_plugin->dependency_libs, key, compare_func);
	}

	/* Read additional config keys. */

	gnome_config_push_prefix (new_plugin->config_path);

	new_plugin->plugin_name = gnome_config_get_string ("name");

	gnome_config_pop_prefix ();

	/* Free unused memory and return. */

	g_string_free (dummy, TRUE);

	return new_plugin;

 load_error:
	g_warning ("Loading of plugin `%s' failed.\n", plugin_name);

 error:
	g_free (new_plugin->name);
	g_free (new_plugin->library_name);
	g_free (new_plugin->config_path);
	g_list_free (new_plugin->dependency_libs);
	g_free (new_plugin);

	return NULL;
}

/*
 * gE_Plugin_Register - Register Plugin.
 */

void
gE_Plugin_Register (gE_Plugin_Object *plugin)
{
	plugin_info *info = g_new0 (plugin_info, 1);

	info->type = PLUGIN_GMODULE;	
	info->user_data = (gpointer) plugin;
	info->plugin_name = plugin->plugin_name;
	info->menu_location = plugin->plugin_name;

	gE_plugin_program_register (info);
}

/*
 * gE_Plugin_Load - Actually load the Plugin and all its dependency libraries
 *                  and call the Plugin init function.
 */

gboolean
gE_Plugin_Load (gE_Plugin_Object *plugin, gint context)
{
	if (plugin->module)
		if (plugin->info->start_func)
			return plugin->info->start_func (plugin, context);
		else
			return FALSE;

	/* Push config prefix. */

	gnome_config_push_prefix (plugin->config_path);

	/* Load all required libraries. */

	g_list_foreach (plugin->dependency_libs, (GFunc) load_library, plugin);
	
	load_library ("library_name", plugin);

	/* Pop config prefix. */

	gnome_config_pop_prefix ();

	/* Test if plugin has been loaded. */

	if (!plugin->module)
		goto load_error;

	if (!g_module_symbol (plugin->module, GEDIT_PLUGIN_INFO_KEY,
			      (gpointer) &plugin->info))
		goto module_error;

	fprintf (stderr, "Successfully loaded plugin `%s'.\n",
		 plugin->name);

	fprintf (stderr, "Plugin Name: %s\n", plugin->info->plugin_name);

	if (plugin->info->init_func)
		plugin->info->init_func (plugin, context);

	return TRUE;

 module_error:
	/* Loading of the plugin failed. */
	g_print ("error: %s\n", g_module_error ());
 load_error:
	g_warning ("Loading of plugin `%s' failed.\n", plugin->name);
 error:
	return FALSE;
}
