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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <dirent.h> 
#include <string.h>

#include <libgnome/gnome-util.h>
#include <libgnome/gnome-i18n.h>

#include <libgnomeui/gnome-theme-parser.h>

#include <gconf/gconf-client.h>

#include "gedit-plugins-engine.h"
#include "gedit-debug.h"

#define USER_GEDIT_PLUGINS_LOCATION ".gedit-2/plugins/"

#define GEDIT_PLUGINS_ENGINE_BASE_KEY "/apps/gedit-2/plugins"

#define SOEXT 		("." G_MODULE_SUFFIX)
#define SOEXT_LEN 	(strlen (SOEXT))

#define PLUGIN_EXT	".gedit-plugin"

static void		 gedit_plugins_engine_load_all 	(void);
static void		 gedit_plugins_engine_load_dir	(const gchar *dir);
static GeditPlugin 	*gedit_plugins_engine_load 	(const gchar *file);

static GeditPluginInfo  *gedit_plugins_engine_find_plugin_info (GeditPlugin *plugin);
static void		 gedit_plugins_engine_reactivate_all (void);

static GList *gedit_plugins_list = NULL;

static GConfClient *gedit_plugins_engine_gconf_client = NULL;

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
	
	gedit_plugins_engine_load_all ();

	return TRUE;
}

static void
gedit_plugins_engine_load_all (void)
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

static gboolean
str_has_suffix (const char *haystack, const char *needle)
{
	const char *h, *n;

	if (needle == NULL) {
		return TRUE;
	}
	if (haystack == NULL) {
		return needle[0] == '\0';
	}
		
	/* Eat one character at a time. */
	h = haystack + strlen(haystack);
	n = needle + strlen(needle);
	do {
		if (n == needle) {
			return TRUE;
		}
		if (h == haystack) {
			return FALSE;
		}
	} while (*--h == *--n);
	return FALSE;
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
		if (str_has_suffix (e->d_name, PLUGIN_EXT))
		{
			gchar *plugin = g_strconcat (dir, e->d_name, NULL);
			gedit_plugins_engine_load (plugin);
			g_free (plugin);
		}
	}
	closedir (d);
}

#define GeditPluginFile GnomeThemeFile
#define gedit_plugin_file_new_from_string  gnome_theme_file_new_from_string
#define gedit_plugin_file_free gnome_theme_file_free
#define gedit_plugin_file_get_string gnome_theme_file_get_string
#define gedit_plugin_file_get_locale_string gnome_theme_file_get_locale_string

static GeditPlugin *
gedit_plugins_engine_load (const gchar *file)
{
	GeditPluginInfo *info;

	GeditPlugin *plugin;

	GeditPluginFile *gedit_plugin_file = NULL;
	gchar *str;
	
	gboolean to_be_activated;
	gchar *key;
	
	gchar *contents;

	g_return_val_if_fail (file != NULL, NULL);
	g_return_val_if_fail (gedit_plugins_engine_gconf_client != NULL, NULL);
	
	gedit_debug (DEBUG_PLUGINS, "Loading plugin: %s", file);

	info = g_new0 (GeditPluginInfo, 1);
	g_return_val_if_fail (info != NULL, NULL);

	plugin = g_new0 (GeditPlugin, 1);
	g_return_val_if_fail (plugin != NULL, NULL);
	
	info->plugin 	= plugin;

	plugin->file 	= g_strdup (file);


	if (!g_file_get_contents (file, &contents, NULL, NULL)) 
	{
		g_warning ("Error loading %s", file);
		goto error;
	}
		
	gedit_plugin_file = gedit_plugin_file_new_from_string (contents, NULL);
	g_free (contents);

	if (gedit_plugin_file == NULL) 
	{
		g_warning ("Bad plugin file: %s", file);
		goto error;
	}

	/* Get Location */
	if (gedit_plugin_file_get_string (gedit_plugin_file, 
					  "Gedit Plugin",
					  "Location",
					  &str)) 
	{
		plugin->location = str;
	} 
	else 
	{
		g_warning ("Couldn't find 'Location' in %s", file);
		goto error;
	}

	/* Get Name */
	if (gedit_plugin_file_get_locale_string (gedit_plugin_file, 
					 	 "Gedit Plugin",
						 "Name",
						 &str)) 
	{
		plugin->name = str;
	} 
	else 
	{
		g_warning ("Couldn't find 'Name' in %s", file);
		goto error;
	}

	/* Get Description */
	if (gedit_plugin_file_get_locale_string (gedit_plugin_file, 
					 	 "Gedit Plugin",
					 	 "Description",
					 	 &str)) 
	{
		plugin->desc = str;
	} 
	else 
	{
		g_warning ("Couldn't find 'Name' in %s", file);
		goto error;
	}


	/* Get Author */
	if (gedit_plugin_file_get_locale_string (gedit_plugin_file, 
						 "Gedit Plugin",
						 "Author",
						 &str)) 
	{
		plugin->author = str;
	} 
	else 
	{
		g_warning ("Couldn't find 'Author' in %s", file);
		goto error;
	}

	/* Get Copyright */
	if (gedit_plugin_file_get_locale_string (gedit_plugin_file, 
						 "Gedit Plugin",
						 "Copyright",
						 &str)) 
	{
		plugin->copyright = str;
	} 
	else 
	{
		g_warning ("Couldn't find 'Copyright' in %s", file);
		goto error;
	}

	key = g_strdup_printf ("%s%s", 
			GEDIT_PLUGINS_ENGINE_BASE_KEY, 
			plugin->location);

	to_be_activated = gconf_client_get_bool (
				gedit_plugins_engine_gconf_client,
				key,
				NULL);
	
	g_free (key);

	/* Actually, the plugin will be activated when reactivate_all
	 * will be called for the first time.
	 * */
	if (to_be_activated)
		info->state = GEDIT_PLUGIN_ACTIVATED;
	else
		info->state = GEDIT_PLUGIN_DEACTIVATED;
	
	gedit_plugins_list = g_list_append (gedit_plugins_list, info);

	gedit_debug (DEBUG_PLUGINS, "Plugin: %s (INSTALLED)", plugin->name);

	gedit_plugin_file_free (gedit_plugin_file);

	return plugin;

error:
	g_free (info);
	
	g_free (plugin->file);
	g_free (plugin->location);
	g_free (plugin->name);
	g_free (plugin->desc);
	g_free (plugin->author);
	g_free (plugin->copyright);
	
	g_free (plugin);
	gedit_plugin_file_free (gedit_plugin_file);

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
		g_warning (_("Error, unable to open module file '%s'\n"),
			   g_module_error ());
		
		return FALSE;
	}
	
	/* Load "init" symbol */
	if (!g_module_symbol (plugin->handle, "init", 
			      (gpointer*)&plugin->init))
	{
		g_warning (_("Error, plugin '%s' does not contain init function."),
			   plugin->name);
		
		goto error_2;
	}

	/* Load "activate" symbol */
	if (!g_module_symbol (plugin->handle, "activate", 
			      (gpointer*)&plugin->activate))
	{
		g_warning (_("Error, plugin '%s' does not contain activate function."),
			   plugin->name);
		
		goto error_2;
	}

	/* Load "deactivate" symbol */
	if (!g_module_symbol (plugin->handle, "deactivate", 
			      (gpointer*)&plugin->deactivate))
	{
		g_warning (_("Error, plugin '%s' does not contain deactivate function."),
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
		g_warning (_("Error, impossible to initialize plugin '%s'"),
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

void
gedit_plugins_engine_shutdown (void)
{
	GList *pl;

	gedit_debug (DEBUG_PLUGINS, "");

	g_return_if_fail (gedit_plugins_engine_gconf_client != NULL);

	/* destroy all the plugins that implement the
	 * destroy function
	 */
	pl = gedit_plugins_list;
	
	while (pl)
	{
		GeditPluginInfo *info = (GeditPluginInfo*)pl->data;
	
		if (info->plugin->destroy != NULL)
		{
			gint r;
		       	gedit_debug (DEBUG_PLUGINS, "Destroy plugin %s", info->plugin->name);

			r = info->plugin->destroy (info->plugin);
			
			if (r != PLUGIN_OK)
			{
				g_warning (_("Error, impossible to destroy plugin '%s'"),
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

		pl = g_list_next (pl);
	}

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
	gboolean res = TRUE;
	gint r = PLUGIN_OK;
	GeditPluginInfo *info;
	gchar *key;
	
	gedit_debug (DEBUG_PLUGINS, "");

	info = gedit_plugins_engine_find_plugin_info (plugin);
	g_return_val_if_fail (info != NULL, FALSE);
	
	/* Activate plugin */
	if (plugin->handle == NULL)
		res = load_plugin_module (plugin);

	if (res)
		r = plugin->activate (plugin);

	if (!res || (r != PLUGIN_OK))
	{
		g_warning (_("Error, impossible to activate plugin '%s'"),
			   plugin->name);
	
		return FALSE;
	}

	/* Update plugin state */
	info->state = GEDIT_PLUGIN_ACTIVATED;

	key = g_strdup_printf ("%s%s", 
			       GEDIT_PLUGINS_ENGINE_BASE_KEY, 
			       plugin->location);

	gconf_client_set_bool (gedit_plugins_engine_gconf_client,
			       key,
			       TRUE,
			       NULL);
	
	g_free (key);

	return TRUE;
}

gboolean
gedit_plugins_engine_deactivate_plugin (GeditPlugin *plugin)
{
	gint res;
	GeditPluginInfo *info;
	gchar *key;
	
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

	key = g_strdup_printf ("%s%s", 
			       GEDIT_PLUGINS_ENGINE_BASE_KEY, 
			       plugin->location);

	gconf_client_set_bool (gedit_plugins_engine_gconf_client,
			       key,
			       FALSE,
			       NULL);
	
	g_free (key);

	return TRUE;
}

static void
gedit_plugins_engine_reactivate_all (void)
{
	GList *pl;
	
	gedit_debug (DEBUG_PLUGINS, "");

	pl = gedit_plugins_list;
	
	while (pl)
	{
		GeditPluginInfo *info = (GeditPluginInfo*)pl->data;

		if (info->state == GEDIT_PLUGIN_ACTIVATED)
		{
			gint r = PLUGIN_OK;
			gboolean res = TRUE;
		       
			if (info->plugin->handle == NULL)
				res = load_plugin_module (info->plugin);

			if (res)
				r = info->plugin->activate (info->plugin);
			
			if (!res || (r != PLUGIN_OK))
			{
				g_warning (_("Error, impossible to activate plugin '%s'"),
			   		info->plugin->name);
				
				info->state = GEDIT_PLUGIN_DEACTIVATED;			
			}
		}
	
		pl = g_list_next (pl);
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

	gedit_debug (DEBUG_PLUGINS, "END");
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
