/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-plugins-engine.c
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

#include <dirent.h> 
#include <string.h>

#include <libgnome/gnome-util.h>
#include <libgnome/gnome-i18n.h>

#include "gedit-plugins-engine.h"
#include "gedit-debug.h"

#define USER_GEDIT_PLUGINS_LOCATION ".gedit2/plugins/"

static void		 gedit_plugins_engine_load_all 	();
static void		 gedit_plugins_engine_load_dir	();
static GeditPlugin 	*gedit_plugins_engine_load 	(const gchar *file);

static GeditPluginInfo  *gedit_plugins_engine_find_plugin_info (GeditPlugin *plugin);
static void		 gedit_plugins_engine_reactivate_all ();

static GList *gedit_plugins_list = NULL;

gboolean
gedit_plugins_engine_init ()
{
	gedit_debug (DEBUG_PLUGINS, "");
	
	if (!g_module_supported ())
		return FALSE;
	
	gedit_plugins_engine_load_all ();

	return TRUE;
}

static void
gedit_plugins_engine_load_all ()
{
	gchar *pdir;

	gchar const * const home = g_get_home_dir ();
	
	gedit_debug (DEBUG_PLUGINS, "");

	/* load user's plugins */
	if (home != NULL)
	{
		pdir = gnome_util_prepend_user_home (USER_GEDIT_PLUGINS_LOCATION);
		gedit_plugins_engine_load_dir (pdir);
		g_free (pdir);
	}
	
	/* load system plugins */
	gedit_plugins_engine_load_dir (GEDIT_PLUGINDIR "/");
}

static void
gedit_plugins_engine_load_dir (const gchar *dir)
{
	DIR *d;
	struct dirent *e;

	gedit_debug (DEBUG_PLUGINS, "DIR: %s", dir);

	d = opendir (dir);
	
	if (d == NULL)
	{		
		gedit_debug (DEBUG_PLUGINS, "%s", strerror (errno));
		return;
	}
	
	while ((e = readdir (d)) != NULL)
	{
		if (strncmp (e->d_name + strlen (e->d_name) - 3, ".so", 3) == 0)
		{
			gchar *plugin = g_strconcat (dir, e->d_name, NULL);
			gedit_plugins_engine_load (plugin);
			g_free (plugin);
		}
	}
	closedir (d);
}

static GeditPlugin *
gedit_plugins_engine_load (const gchar *file)
{
	GeditPluginInfo *info;

	GeditPlugin *plugin;
	guint res;

	gedit_debug (DEBUG_PLUGINS, "");

	g_return_val_if_fail (file != NULL, NULL);
	
	info = g_new0 (GeditPluginInfo, 1);
	g_return_val_if_fail (info != NULL, NULL);

	plugin = g_new0 (GeditPlugin, 1);
	g_return_val_if_fail (plugin != NULL, NULL);
	
	info->plugin 	= plugin;

	plugin->file 	= g_strdup (file);
	plugin->handle 	= g_module_open (file, G_MODULE_BIND_LAZY);

	if (plugin->handle == NULL)
	{
		g_warning (_("Error, unable to open module file '%s'\n"),
			   g_module_error ());
		
		g_free (plugin->file);
		g_free (plugin);

		g_free (info);
		
		return NULL;
	}
	
	/* Load "init" symbol */
	if (!g_module_symbol (plugin->handle, "init", 
			      (gpointer*)&plugin->init))
	{
		g_warning (_("Error, plugin '%s' does not contain init function."),
			   file);
		
		goto error;
	}

	/* Load "activate" symbol */
	if (!g_module_symbol (plugin->handle, "activate", 
			      (gpointer*)&plugin->activate))
	{
		g_warning (_("Error, plugin '%s' does not contain activate function."),
			   file);
		
		goto error;
	}

	/* Load "deactivate" symbol */
	if (!g_module_symbol (plugin->handle, "deactivate", 
			      (gpointer*)&plugin->deactivate))
	{
		g_warning (_("Error, plugin '%s' does not contain deactivate function."),
			   file);
		
		goto error;
	}

	/* Load "configure" symbol */
	if (!g_module_symbol (plugin->handle, "configure", 
			      (gpointer*)&plugin->configure))
		plugin->configure = NULL;

	/* Load "save_settings" symbol */
	if (!g_module_symbol (plugin->handle, "save_settings", 
			      (gpointer*)&plugin->save_settings))
		plugin->save_settings = NULL;

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
		g_warning (_("Error, impossible to initialize plugin '%s'"),
			   file);
		
		goto error;
	}		

	if (plugin->name == NULL)
	{
		g_warning (_("Error, the plugin '%s' did not specified a name"),
			   file);

		goto error;
	}

	gedit_debug (DEBUG_PLUGINS, "Plugin: %s (INSTALLED)", plugin->name);

	info->state = GEDIT_PLUGIN_DEACTIVATED;

	gedit_plugins_list = g_list_append (gedit_plugins_list, info);

	return plugin;

error:
	g_free (info);
	
	g_module_close (plugin->handle);
	
	g_free (plugin->file);
	g_free (plugin->name);
	g_free (plugin->desc);
	g_free (plugin->author);
	g_free (plugin->copyright);
	
	g_free (plugin);
	
	return NULL;
}

void
gedit_plugins_engine_save_settings ()
{
	gedit_debug (DEBUG_PLUGINS, "");

	/* TODO */
}


const GList *
gedit_plugins_engine_get_plugins_list ()
{
	gedit_debug (DEBUG_PLUGINS, "");

	return gedit_plugins_list;
}

static GeditPluginInfo *
gedit_plugins_engine_find_plugin_info (GeditPlugin *plugin)
{
	GList *pl;

	gedit_debug (DEBUG_PLUGINS, "");

	pl = gedit_plugins_list;
	
	while (pl)
	{
		GeditPluginInfo *info = (GeditPluginInfo*)pl->data;

		if (info->plugin == plugin)
			return info;

		pl = g_list_next (pl);
	}

	return NULL;
}


gboolean 	 
gedit_plugins_engine_activate_plugin (GeditPlugin *plugin)
{
	gboolean res;
	GeditPluginInfo *info;
	
	gedit_debug (DEBUG_PLUGINS, "");

	info = gedit_plugins_engine_find_plugin_info (plugin);
	g_return_val_if_fail (info != NULL, FALSE);
	
	/* Activate plugin */
	res = plugin->activate (plugin);
	if (res != PLUGIN_OK)
	{
		g_warning (_("Error, impossible to activate plugin '%s'"),
			   plugin->name);
		
		return FALSE;
	}

	/* Update plugin state */
	info->state = GEDIT_PLUGIN_ACTIVATED;

	return TRUE;
}

gboolean
gedit_plugins_engine_deactivate_plugin (GeditPlugin *plugin)
{
	gboolean res;
	GeditPluginInfo *info;
	
	gedit_debug (DEBUG_PLUGINS, "");

	info = gedit_plugins_engine_find_plugin_info (plugin);
	g_return_val_if_fail (info != NULL, FALSE);
	
	/* Activate plugin */
	res = plugin->deactivate (plugin);
	if (res != PLUGIN_OK)
	{
		g_warning (_("Error, impossible to deactivate plugin '%s'"),
			   plugin->name);
		
		return FALSE;
	}

	/* Update plugin state */
	info->state = GEDIT_PLUGIN_DEACTIVATED;

	return TRUE;
}

static void
gedit_plugins_engine_reactivate_all ()
{
	GList *pl;
	
	gedit_debug (DEBUG_PLUGINS, "");

	pl = gedit_plugins_list;
	
	while (pl)
	{
		GeditPluginInfo *info = (GeditPluginInfo*)pl->data;

		/* FIXME: uncomment when we will have a Plugin Manager - Paolo
		if (info->state == GEDIT_PLUGIN_ACTIVATED)
		*/
		{
			gint r = info->plugin->activate (info->plugin);
			
			if (r != PLUGIN_OK)
			{
				g_warning (_("Error, impossible to activate plugin '%s'"),
			   		info->plugin->name);
			}
			/* FIXME: remove when we will have a Plugin Manager - Paolo */
			else
				info->state = GEDIT_PLUGIN_ACTIVATED;
		}
	
		pl = g_list_next (pl);
	}
	
}

void
gedit_plugins_engine_update_plugins_ui (BonoboWindow* window, gboolean new_window)
{
	GList *pl;

	gedit_debug (DEBUG_PLUGINS, "");

	if (new_window)
		gedit_plugins_engine_reactivate_all ();

	/* updated ui of all the plugins that implement thae
	 * update_ui function
	 */
	pl = gedit_plugins_list;
	
	while (pl)
	{
		GeditPluginInfo *info = (GeditPluginInfo*)pl->data;
		
		if ((info->state == GEDIT_PLUGIN_ACTIVATED) &&
		    (info->plugin->update_ui != NULL))
		{
			gint r;
		       	gedit_debug (DEBUG_PLUGINS, "Updating UI of %s", info->plugin->name);

			r = info->plugin->update_ui (info->plugin, window);
			
			if (r != PLUGIN_OK)
			{
				g_warning (_("Error, impossible to update ui of the plugin '%s'"),
			   		info->plugin->name);
			}
		}
	
		pl = g_list_next (pl);
	}
}

gboolean
gedit_plugins_engine_is_a_configurable_plugin (GeditPlugin *plugin)
{
	gedit_debug (DEBUG_PLUGINS, "");

	g_return_val_if_fail (plugin != NULL, FALSE);

	return (plugin->configure != NULL);	
}

gboolean 	 
gedit_plugins_engine_configure_plugin (GeditPlugin *plugin, GtkWidget* parent)
{
	gedit_debug (DEBUG_PLUGINS, "");

	g_return_val_if_fail (plugin != NULL, FALSE);
	g_return_val_if_fail (plugin->configure != NULL, FALSE);
	
	return (plugin->configure (plugin, parent) == PLUGIN_OK);
}
