/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-plugins-engine.c
 * This file is part of gedit
 *
 * Copyright (C) 2002-2004 Paolo Maggi 
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
 * Modified by the gedit Team, 2002-2004. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include <glib/gi18n.h>
#include <glib/gkeyfile.h>
#include <libgnome/gnome-util.h>
#include <gconf/gconf-client.h>

#include "gedit-plugins-engine.h"
#include "gedit-debug.h"

#define USER_GEDIT_PLUGINS_LOCATION ".gedit-2/plugins/"

#define GEDIT_PLUGINS_ENGINE_BASE_KEY "/apps/gedit-2/plugins"
#define GEDIT_PLUGINS_ENGINE_KEY GEDIT_PLUGINS_ENGINE_BASE_KEY "/active-plugins"

#define SOEXT 		("." G_MODULE_SUFFIX)
#define SOEXT_LEN 	(strlen (SOEXT))
#define PLUGIN_EXT	".gedit-plugin"


static void 		 gedit_plugins_engine_active_plugins_changed (GConfClient *client,
								      guint cnxn_id, 
								      GConfEntry *entry, 
								      gpointer user_data);

static GList *gedit_plugins_list = NULL;

static GConfClient *gedit_plugins_engine_gconf_client = NULL;

GSList *active_plugins = NULL;

static GeditPlugin *
gedit_plugins_engine_load (const gchar *file)
{
	GeditPlugin *plugin;
	GKeyFile *plugin_file = NULL;
	gchar *str;

	g_return_val_if_fail (file != NULL, NULL);

	gedit_debug (DEBUG_PLUGINS, "Loading plugin: %s", file);

	plugin = g_new0 (GeditPlugin, 1);
	plugin->file = g_strdup (file);

	plugin_file = g_key_file_new ();
	if (!g_key_file_load_from_file (plugin_file, file, G_KEY_FILE_NONE, NULL))
	{
		g_warning ("Bad plugin file: %s", file);
		goto error;
	}

	/* Get Location */
	str = g_key_file_get_string (plugin_file,
				     "Gedit Plugin",
				     "Location",
				     NULL);
	if (str)
	{
		plugin->location = str;
	}
	else
	{
		g_warning ("Could not find 'Location' in %s", file);
		goto error;
	}

	/* Get Name */
	str = g_key_file_get_locale_string (plugin_file,
					    "Gedit Plugin",
					    "Name",
					    NULL, NULL);
	if (str)
	{
		plugin->name = str;
	}
	else
	{
		g_warning ("Could not find 'Name' in %s", file);
		goto error;
	}

	/* Get Description */
	str = g_key_file_get_locale_string (plugin_file,
					    "Gedit Plugin",
					    "Description",
					    NULL, NULL);
	if (str)
	{
		plugin->desc = str;
	}
	else
	{
		g_warning ("Could not find 'Description' in %s", file);
		goto error;
	}

	/* Get Author */
	str = g_key_file_get_string (plugin_file,
				     "Gedit Plugin",
				     "Author",
				     NULL);
	if (str)
	{
		plugin->author = str;
	}
	else
	{
		g_warning ("Could not find 'Author' in %s", file);
		goto error;
	}

	/* Get Copyright */
	str = g_key_file_get_string (plugin_file,
				     "Gedit Plugin",
				     "Copyright",
				     NULL);
	if (str)
	{
		plugin->copyright = str;
	}
	else
	{
		g_warning ("Could not find 'Copyright' in %s", file);
		goto error;
	}

	g_key_file_free (plugin_file);

	return plugin;

error:
	g_free (plugin->file);
	g_free (plugin->location);
	g_free (plugin->name);
	g_free (plugin->desc);
	g_free (plugin->author);
	g_free (plugin->copyright);
	g_free (plugin);
	g_key_file_free (plugin_file);

	return NULL;
}

static void
gedit_plugins_engine_load_dir (const gchar *dir)
{
	GError *error = NULL;
	GDir *d;
	const gchar *dirent;

	g_return_if_fail (gedit_plugins_engine_gconf_client != NULL);

	gedit_debug (DEBUG_PLUGINS, "DIR: %s", dir);

	d = g_dir_open (dir, 0, &error);
	if (!d)
	{
		gedit_debug (DEBUG_PLUGINS, "%s", error->message);
		g_error_free (error);
		return;
	}

	while ((dirent = g_dir_read_name (d)))
	{
		if (g_str_has_suffix (dirent, PLUGIN_EXT))
		{
			gchar *plugin_file;
			GeditPlugin *plugin;
			GeditPluginInfo *info;
			gboolean to_be_activated;

			plugin_file = g_build_filename (dir, dirent, NULL);
			plugin = gedit_plugins_engine_load (plugin_file);
			g_free (plugin_file);

			if (plugin == NULL)
				continue;

			info = g_new0 (GeditPluginInfo, 1);
			info->plugin = plugin;

			to_be_activated = (g_slist_find_custom (active_plugins,
								plugin->location,
								(GCompareFunc)strcmp) != NULL);

			/* Actually, the plugin will be activated when reactivate_all
			 * will be called for the first time.
			 * */
			if (to_be_activated)
				info->state = GEDIT_PLUGIN_ACTIVATED;
			else
				info->state = GEDIT_PLUGIN_DEACTIVATED;

			gedit_plugins_list = g_list_prepend (gedit_plugins_list, info);

			gedit_debug (DEBUG_PLUGINS, "Plugin %s loaded", plugin->name);
		}
	}

	gedit_plugins_list = g_list_reverse (gedit_plugins_list);

	g_dir_close (d);
}

static void
gedit_plugins_engine_load_all (void)
{
	gchar const *home = g_get_home_dir ();

	/* load user's plugins */
	if (home != NULL)
	{
		gchar *pdir;

		pdir = gnome_util_prepend_user_home (USER_GEDIT_PLUGINS_LOCATION);
		gedit_plugins_engine_load_dir (pdir);
		g_free (pdir);
	}

	/* load system plugins */
	gedit_plugins_engine_load_dir (GEDIT_PLUGINDIR "/");
}

gboolean
gedit_plugins_engine_init (void)
{
	gedit_debug (DEBUG_PLUGINS, "");
	
	if (!g_module_supported ())
		return FALSE;

	gedit_plugins_engine_gconf_client = gconf_client_get_default ();
	g_return_val_if_fail (gedit_plugins_engine_gconf_client != NULL, FALSE);

	gconf_client_add_dir (gedit_plugins_engine_gconf_client,
			      GEDIT_PLUGINS_ENGINE_BASE_KEY,
			      GCONF_CLIENT_PRELOAD_ONELEVEL,
			      NULL);

	gconf_client_notify_add (gedit_plugins_engine_gconf_client,
				 GEDIT_PLUGINS_ENGINE_KEY,
				 gedit_plugins_engine_active_plugins_changed,
				 NULL, NULL, NULL);


	active_plugins = gconf_client_get_list (gedit_plugins_engine_gconf_client,
						GEDIT_PLUGINS_ENGINE_KEY,
						GCONF_VALUE_STRING,
						NULL);

	gedit_plugins_engine_load_all ();

	return TRUE;
}

void
gedit_plugins_engine_shutdown (void)
{
	GList *pl;

	gedit_debug (DEBUG_PLUGINS, "");

	g_return_if_fail (gedit_plugins_engine_gconf_client != NULL);

	for (pl = gedit_plugins_list; pl; pl = pl->next)
	{
		GeditPluginInfo *info = (GeditPluginInfo*)pl->data;

		if (info->plugin->destroy != NULL)
		{
			gint r;
		       	gedit_debug (DEBUG_PLUGINS, "Destroy plugin %s", info->plugin->name);

			r = info->plugin->destroy (info->plugin);
			
			if (r != PLUGIN_OK)
			{
				g_warning ("Error, impossible to destroy plugin '%s'",
			   		info->plugin->name);
			}
		}

		if (info->plugin->handle != NULL)
			g_module_close (info->plugin->handle);

		g_free (info->plugin->file);
		g_free (info->plugin->location);
		g_free (info->plugin->name);
		g_free (info->plugin->desc);
		g_free (info->plugin->author);
		g_free (info->plugin->copyright);
	
		g_free (info->plugin);
	
		g_free (info);
	}

	g_slist_foreach (active_plugins, (GFunc)g_free, NULL);
	g_slist_free (active_plugins);

	active_plugins = NULL;

	g_list_free (gedit_plugins_list);
	gedit_plugins_list = NULL;

	g_object_unref (gedit_plugins_engine_gconf_client);
	gedit_plugins_engine_gconf_client = NULL;
}

const GList *
gedit_plugins_engine_get_plugins_list (void)
{
	gedit_debug (DEBUG_PLUGINS, "");

	return gedit_plugins_list;
}

static GeditPluginInfo *
gedit_plugins_engine_find_plugin_info (GeditPlugin *plugin)
{
	GList *pl;

	gedit_debug (DEBUG_PLUGINS, "");

	for (pl = gedit_plugins_list; pl; pl = pl->next)
	{
		GeditPluginInfo *info = (GeditPluginInfo*)pl->data;

		if (info->plugin == plugin)
			return info;
	}

	return NULL;
}

static gboolean
load_plugin_module (GeditPlugin *plugin)
{
	gchar *path;
	gchar *dirname;
	gint res;

	g_return_val_if_fail (plugin != NULL, FALSE);
	g_return_val_if_fail (plugin->file != NULL, FALSE);
	g_return_val_if_fail (plugin->location != NULL, FALSE);

	dirname = g_path_get_dirname (plugin->file);	
	g_return_val_if_fail (dirname != NULL, FALSE);

	path = g_module_build_path (dirname, plugin->location);
	g_free (dirname);
	g_return_val_if_fail (path != NULL, FALSE);
	
	plugin->handle 	= g_module_open (path, G_MODULE_BIND_LAZY);
	g_free (path);

	if (plugin->handle == NULL)
	{
		g_warning ("Error, unable to open module file '%s'\n",
			   g_module_error ());
		
		return FALSE;
	}
	
	/* Load "init" symbol */
	if (!g_module_symbol (plugin->handle, "init", 
			      (gpointer*)&plugin->init))
	{
		g_warning ("Error, plugin '%s' does not contain init function.",
			   plugin->name);

		goto error_2;
	}

	/* Load "activate" symbol */
	if (!g_module_symbol (plugin->handle, "activate", 
			      (gpointer*)&plugin->activate))
	{
		g_warning ("Error, plugin '%s' does not contain activate function.",
			   plugin->name);
		
		goto error_2;
	}

	/* Load "deactivate" symbol */
	if (!g_module_symbol (plugin->handle, "deactivate", 
			      (gpointer*)&plugin->deactivate))
	{
		g_warning ("Error, plugin '%s' does not contain deactivate function.",
			   plugin->name);
		
		goto error_2;
	}

	/* Load "configure" symbol */
	if (!g_module_symbol (plugin->handle, "configure", 
			      (gpointer*)&plugin->configure))
		plugin->configure = NULL;

	/* Load "update_ui" symbol */
	if (!g_module_symbol (plugin->handle, "update_ui", 
			      (gpointer*)&plugin->update_ui))
		plugin->update_ui = NULL;

	/* Load "destroy" symbol */
	if (!g_module_symbol (plugin->handle, "destroy", 
			      (gpointer*)&plugin->destroy))
		plugin->destroy = NULL;
	
	/* Initialize plugin */
	res = plugin->init (plugin);
	if (res != PLUGIN_OK)
	{
		g_warning ("Error, impossible to initialize plugin '%s'",
			   plugin->name);

		goto error_2;
	}		

	return TRUE;

error_2:
	g_module_close (plugin->handle);

	plugin->handle = NULL;
	
	plugin->init = NULL;	
	plugin->activate = NULL;
	plugin->deactivate = NULL;

	plugin->update_ui = NULL;
	plugin->destroy = NULL;
	plugin->configure = NULL;

	return FALSE;
}

static gboolean 	 
gedit_plugins_engine_activate_plugin_real (GeditPlugin *plugin)
{
	gboolean res = TRUE;
	gint r = PLUGIN_OK;

	if (plugin->handle == NULL)
		res = load_plugin_module (plugin);

	if (res)
		r = plugin->activate (plugin);

	if (!res || (r != PLUGIN_OK))
	{
		g_warning ("Error, impossible to activate plugin '%s'",
			   plugin->name);

		return FALSE;
	}

	return TRUE;
}

gboolean 	 
gedit_plugins_engine_activate_plugin (GeditPlugin *plugin)
{
	GeditPluginInfo *info;

	gedit_debug (DEBUG_PLUGINS, "");

	g_return_val_if_fail (plugin != NULL, FALSE);

	info = gedit_plugins_engine_find_plugin_info (plugin);
	g_return_val_if_fail (info != NULL, FALSE);

	if (info->state == GEDIT_PLUGIN_ACTIVATED)
		return TRUE;

	if (gedit_plugins_engine_activate_plugin_real (plugin))
	{
		gboolean res;
		GSList *list;
		
		/* Update plugin state */
		info->state = GEDIT_PLUGIN_ACTIVATED;

		/* I want to be really sure :) */
		list = active_plugins;
		while (list != NULL)
		{
			if (strcmp (plugin->location, (gchar *)list->data) == 0)
			{
				g_warning ("Plugin %s is already activated.", plugin->location);
				return TRUE;
			}

			list = g_slist_next (list);
		}
	
		active_plugins = g_slist_insert_sorted (active_plugins, 
						        g_strdup (plugin->location), 
						        (GCompareFunc)strcmp);
		
		res = gconf_client_set_list (gedit_plugins_engine_gconf_client,
		    			     GEDIT_PLUGINS_ENGINE_KEY,
					     GCONF_VALUE_STRING,
					     active_plugins,
					     NULL);
		
		if (!res)
			g_warning ("Error saving the list of active plugins.");

		return TRUE;
	}

	return FALSE;
}

static gboolean
gedit_plugins_engine_deactivate_plugin_real (GeditPlugin *plugin)
{
	gint res;
	
	res = plugin->deactivate (plugin);

	if (res != PLUGIN_OK)
	{
		g_warning ("Error, impossible to deactivate plugin '%s'",
			   plugin->name);

		return FALSE;
	}

	return TRUE;
}

gboolean
gedit_plugins_engine_deactivate_plugin (GeditPlugin *plugin)
{
	GeditPluginInfo *info;

	gedit_debug (DEBUG_PLUGINS, "");

	g_return_val_if_fail (plugin != NULL, FALSE);

	info = gedit_plugins_engine_find_plugin_info (plugin);
	g_return_val_if_fail (info != NULL, FALSE);

	if (info->state == GEDIT_PLUGIN_DEACTIVATED)
		return TRUE;

	if (gedit_plugins_engine_deactivate_plugin_real (plugin))
	{
		gboolean res;
		GSList *list;
		
		/* Update plugin state */
		info->state = GEDIT_PLUGIN_DEACTIVATED;

		list = active_plugins;
		res = (list == NULL);

		while (list != NULL)
		{
			if (strcmp (plugin->location, (gchar *)list->data) == 0)
			{
				g_free (list->data);
				active_plugins = g_slist_delete_link (active_plugins, list);
				list = NULL;
				res = TRUE;
			}
			else
				list = g_slist_next (list);
		}

		if (!res)
		{
			g_warning ("Plugin %s is already deactivated.", plugin->location);
			return TRUE;
		}

		res = gconf_client_set_list (gedit_plugins_engine_gconf_client,
		    			     GEDIT_PLUGINS_ENGINE_KEY,
					     GCONF_VALUE_STRING,
					     active_plugins,
					     NULL);
		
		if (!res)
			g_warning ("Error saving the list of active plugins.");

		return TRUE;
	}

	return FALSE;
}

static void
gedit_plugins_engine_reactivate_all (void)
{
	GList *pl;

	gedit_debug (DEBUG_PLUGINS, "");

	for (pl = gedit_plugins_list; pl; pl = pl->next)
	{
		GeditPluginInfo *info = (GeditPluginInfo*)pl->data;

		if (info->state == GEDIT_PLUGIN_ACTIVATED)
		{
			if (!gedit_plugins_engine_activate_plugin_real (info->plugin))
				info->state = GEDIT_PLUGIN_DEACTIVATED;			
		}
	}
}

void
gedit_plugins_engine_update_plugins_ui (BonoboWindow* window, gboolean new_window)
{
	GList *pl;

	gedit_debug (DEBUG_PLUGINS, "");

	g_return_if_fail (window != NULL);
	g_return_if_fail (BONOBO_IS_WINDOW (window));

	if (new_window)
		gedit_plugins_engine_reactivate_all ();

	/* updated ui of all the plugins that implement update_ui */
	for (pl = gedit_plugins_list; pl; pl = pl->next)
	{
		gint r;
		GeditPluginInfo *info = (GeditPluginInfo*)pl->data;

		if ((info->state != GEDIT_PLUGIN_ACTIVATED) ||
		    (info->plugin->update_ui == NULL))
			continue;

	       	gedit_debug (DEBUG_PLUGINS, "Updating UI of %s", info->plugin->name);

		r = info->plugin->update_ui (info->plugin, window);
		if (r != PLUGIN_OK)
		{
			g_warning ("Error, impossible to update ui of the plugin '%s'",
				   info->plugin->name);
		}
	}
}

gboolean
gedit_plugins_engine_is_a_configurable_plugin (GeditPlugin *plugin)
{
	GeditPluginInfo *info;

	gedit_debug (DEBUG_PLUGINS, "");

	g_return_val_if_fail (plugin != NULL, FALSE);

	info = gedit_plugins_engine_find_plugin_info (plugin);
	g_return_val_if_fail (info != NULL, FALSE);

	return (plugin->configure != NULL) && (info->state == GEDIT_PLUGIN_ACTIVATED);	
}

gboolean 	 
gedit_plugins_engine_configure_plugin (GeditPlugin *plugin, GtkWidget* parent)
{
	gedit_debug (DEBUG_PLUGINS, "");

	g_return_val_if_fail (plugin != NULL, FALSE);
	g_return_val_if_fail (plugin->configure != NULL, FALSE);

	return (plugin->configure (plugin, parent) == PLUGIN_OK);
}

static void 
gedit_plugins_engine_active_plugins_changed (GConfClient *client,
					     guint cnxn_id,
					     GConfEntry *entry,
					     gpointer user_data)
{
	GList *pl;
	gboolean to_activate;

	gedit_debug (DEBUG_PLUGINS, "");

	g_return_if_fail (entry->key != NULL);
	g_return_if_fail (entry->value != NULL);

	
	if (!((entry->value->type == GCONF_VALUE_LIST) && (gconf_value_get_list_type (entry->value) == GCONF_VALUE_STRING)))
	{
		g_warning ("You may have a corrupted gconf key: %s", GEDIT_PLUGINS_ENGINE_KEY);
		return;
	}
	
	active_plugins = gconf_client_get_list (gedit_plugins_engine_gconf_client,
						GEDIT_PLUGINS_ENGINE_KEY,
						GCONF_VALUE_STRING,
						NULL);

	for (pl = gedit_plugins_list; pl; pl = pl->next)
	{
		GeditPluginInfo *info = (GeditPluginInfo*)pl->data;

		to_activate = (g_slist_find_custom (active_plugins,
						    info->plugin->location,
						    (GCompareFunc)strcmp) != NULL);

		if ((info->state == GEDIT_PLUGIN_DEACTIVATED) && to_activate)
		{
			/* Activate plugin */
			if (gedit_plugins_engine_activate_plugin_real (info->plugin))
				/* Update plugin state */
				info->state = GEDIT_PLUGIN_ACTIVATED;
		}
		else
		{
			if ((info->state == GEDIT_PLUGIN_ACTIVATED) && !to_activate)
			{
				if (gedit_plugins_engine_deactivate_plugin_real (info->plugin))	
					/* Update plugin state */
					info->state = GEDIT_PLUGIN_DEACTIVATED;
			}
		}
	}
}

