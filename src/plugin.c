/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit
 *
 * Copyright (C) 1998, 1999 Alex Roberts, Evan Lawrence
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
 
/* Plugins system based on that used in Gnumeric */
 
#include <config.h>
#include <gnome.h>
#include <gmodule.h>
#include <dirent.h> 

#include "file.h"
#include "plugin.h"
#include "document.h"


GSList	*plugin_list = NULL;


PluginData *
plugin_load (const gchar *file)
{
	PluginData *pd;
	guint res;
	
	g_return_val_if_fail (file != NULL, NULL);

	if (!(pd = g_new0 (PluginData, 1)))
	{
		g_print ("plugin allocation error");
		return NULL;
	}
	
	pd->file = g_strdup (file);
	pd->handle = g_module_open (file, 0);

	if (!pd->handle)
	{
		g_print (_("Error, unable to open module file, %s"),
			 g_module_error ());
		
		g_free (pd);
		return NULL;
	}
	
	if (!g_module_symbol (pd->handle, "init_plugin", 
			      (gpointer*)&pd->init_plugin))
	{
		
		g_print (_("Error, plugin does not contain init_plugin function."));

		goto error;
	}
	
	res = pd->init_plugin (pd);
	if (res != PLUGIN_OK)
	{
		g_print (_("Error, init_plugin returned an error"));
		
		goto error;
	}
	
	plugin_list = g_slist_append (plugin_list, pd);
	return pd;
	
error:
	g_module_close (pd->handle);
	g_free (pd->file);
	g_free (pd);
	return NULL;
}

void
plugin_unload (PluginData *pd)
{
	int w, n;
	char *path;
	GnomeApp *app;
	
	g_return_if_fail (pd != NULL);
	
	if (pd->can_unload && !pd->can_unload (pd))
	{
		g_print (_("Error, plugin is still in use"));
		return;
	}
	
	if (pd->destroy_plugin)
		pd->destroy_plugin (pd);
	
	n = g_slist_index (plugin_list, pd);
	plugin_list = g_slist_remove (plugin_list, pd);
	
	path = g_strdup_printf ("%s/", _("_Plugins"));
	
	for (w = 0; w < g_list_length (mdi->windows); w++)
	{
		app = g_list_nth_data (mdi->windows, w);
		
		gnome_app_remove_menu_range (app, path, n, 1);
	}
	
	g_module_close (pd->handle);
	g_free (pd->file);
	g_free (pd);
}

static void
plugin_load_plugins_in_dir (char *dir)
{
	DIR *d;
	struct dirent *e;
	
	if ((d = opendir (dir)) == NULL)
		return;
	
	while ((e = readdir (d)) != NULL)
	{
		if (strncmp (e->d_name + strlen (e->d_name) - 3, ".so", 3) == 0)
		{
			char *plugin = g_strconcat (dir, e->d_name, NULL);

			plugin_load (plugin);
			g_free (plugin);
		}
	}
	closedir (d);
}

static void
load_all_plugins (void)
{
	char *pdir;
	char const * const home = g_get_home_dir ();
	
	/* load user plugins */
	if (home != NULL)
	{
		pdir = gnome_util_prepend_user_home (".gedit/plugins/");
/*		pdir = g_strconcat (home, "/.gedit/plugins/", NULL); */
		plugin_load_plugins_in_dir (pdir);
		g_free (pdir);
	}
	
	/* load system plgins */
	plugin_load_plugins_in_dir (GEDIT_PLUGINDIR "/");
}

/**
 * gedit_plugins_init:
 *
 */
void
gedit_plugins_init (void)
{
	if (!g_module_supported ())
		return;
	
	load_all_plugins ();
}

/**
 * gedit_plugins_window_add:
 * @app:
 *
 *
 */
void
gedit_plugins_window_add (GnomeApp *app)
{
	PluginData  *pd;
	gint         n;
	gchar       *path;
	GnomeUIInfo *menu;

	g_return_if_fail (app != NULL);

	menu = g_malloc0 (2 * sizeof (GnomeUIInfo));

	for (n = 0; n < g_slist_length (plugin_list); n++)
	{
		pd = g_slist_nth_data (plugin_list, n);

		path = g_strdup_printf ("%s/", _("_Plugins"));
		
		menu->label = g_strdup (pd->name);
		menu->type = GNOME_APP_UI_ITEM;
		menu->hint = NULL;
		menu->moreinfo = pd->private_data;
		
		(menu + 1)->type = GNOME_APP_UI_ENDOFINFO;
		
		gnome_app_insert_menus (app, path, menu);

		g_free (menu->label);
		g_free (path);
	}
	g_free (menu);
}
