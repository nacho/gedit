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
	plugin->handle 	= g_module_open (file, 0);

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

gboolean 	 
gedit_plugins_engine_activate_plugin (GeditPlugin *plugin)
{
	gboolean res;

	gedit_debug (DEBUG_PLUGINS, "");

	res = plugin->activate (plugin);
	if (res != PLUGIN_OK)
	{
		g_warning (_("Error, impossible to activate plugin '%s'"),
			   plugin->file);
		
		return FALSE;
	}

	/* TODO: Update plugin list */
	
	return FALSE;
}

gboolean
gedit_plugins_engine_deactivate_plugin (GeditPlugin *plugin)
{
	gedit_debug (DEBUG_PLUGINS, "");

	/* TODO */
	return FALSE;
}
	

gboolean	 
gedit_plugins_engine_update_plugins_ui (gboolean new_window)
{
	/*
	if (new_window)
		gedit_plugins_engine_reactivate_all ();

*/
	/* TODO: updated ui di tutti i plugin che hanno
	 * la funzione update_ui
	 */

	return FALSE;
}

