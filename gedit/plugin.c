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
#include <glade/glade.h>

#include "dialogs/dialogs.h"

/* FIXME: Do we need this files ?? verify that we do Chema */
#include <gmodule.h>
#include <dirent.h> 

#include "file.h"
#include "plugin.h"
#include "document.h"
#include "utils.h"
#include "window.h"

GSList	*plugin_list = NULL;

gchar * gedit_plugin_program_location_get (gchar *program_name, gchar *plugin_name);
gchar * gedit_plugin_program_location_dialog (void);
gchar * gedit_plugin_program_location_guess (gchar *program_name);

PluginData *
plugin_load (const gchar *file)
{
	PluginData *pd;
	guint res;

	gedit_debug ("", DEBUG_PLUGINS);
	
	g_return_val_if_fail (file != NULL, NULL);

	if (!(pd = g_new0 (PluginData, 1)))
	{
		g_warning ("plugin allocation error");
		return NULL;
	}
	
	pd->file = g_strdup (file);
	pd->handle = g_module_open (file, 0);

	if (!pd->handle)
	{
		g_warning (_("Error, unable to open module file, %s"),
			   g_module_error ());
		
		g_free (pd);
		return NULL;
	}
	
	if (!g_module_symbol (pd->handle, "init_plugin", 
			      (gpointer*)&pd->init_plugin))
	{
		
		g_warning (_("Error, plugin does not contain init_plugin function."));
		goto error;
	}
	
	res = pd->init_plugin (pd);
	if (res != PLUGIN_OK)
	{
		g_warning (_("Error, init_plugin returned an error"));
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
	
	gedit_debug ("", DEBUG_PLUGINS);

	g_return_if_fail (pd != NULL);
	
	if (pd->can_unload && !pd->can_unload (pd))
	{
		g_warning (_("Error, plugin is still in use"));
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

	gedit_debug ("", DEBUG_PLUGINS);
	
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
	
	gedit_debug ("", DEBUG_PLUGINS);

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

	gedit_debug ("", DEBUG_PLUGINS);
	
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

	gedit_debug ("start", DEBUG_PLUGINS);

	g_return_if_fail (app != NULL);

	n = g_slist_length (plugin_list) + 1 ;

	menu = g_malloc0 ( n * sizeof (GnomeUIInfo));
	path = g_strdup_printf ("%s/", _("_Plugins"));
	
	for (n = 0; n < g_slist_length (plugin_list); n++)
	{
		pd = g_slist_nth_data (plugin_list, n);

		menu[n].type = GNOME_APP_UI_ITEM;
		menu[n].label = g_strdup (pd->name);
		menu[n].hint = g_strdup (pd->desc);
		menu[n].moreinfo = pd->private_data;
		menu[n].pixmap_type = GNOME_APP_PIXMAP_NONE;
		menu[n+1].type = GNOME_APP_UI_ENDOFINFO;
	}

	gnome_app_insert_menus (app, path, menu);
	g_free (path);
	gnome_app_install_menu_hints (app, menu);

	for (n = 0; n < g_slist_length (plugin_list); n++)
	{
		g_free (menu[n].label);
	}
	
	g_free (menu);

	gedit_debug ("end", DEBUG_PLUGINS);
}



static gchar*
gedit_plugin_guess_program_location (gchar *program_name)
{
	gchar * location = NULL;
	gchar * config_string = NULL;

	config_string = g_strdup_printf ("/gedit/plugin_programs/%s", program_name);
	/* get the program pointed by the config */
	if (gnome_config_get_string (config_string))
		location = g_strdup (gnome_config_get_string (config_string));

	/* If there was a program in the config, but it is no good. Clear "sendmail" */
	if (location)
		if (gedit_utils_is_program (location, program_name) != GEDIT_PROGRAM_OK)
		{
			g_free (location);
			location = NULL;
			/* If this was not a good location, clear the bad location on
			   the config file */
			gnome_config_set_string (config_string, "");
			gnome_config_sync ();
		}
	g_free (config_string);

	/* If we have no 1st choice yet, get from path */
	if (!location)
		location = g_strdup (gnome_is_program_in_path (program_name));

	/* If it isn't on path, look for common places */
	if (!location)
	{
		gchar * look_here [2];

		look_here [0] = g_strdup_printf ("/usr/sbin/%s", program_name);
		look_here [1] = g_strdup_printf ("/usr/lib/%s",  program_name);

		if (g_file_exists (look_here[0]))
			location = g_strdup (look_here[0]);
		else if (g_file_exists (look_here[1]))
			location = g_strdup (look_here[1]);

		g_free (look_here[0]);
		g_free (look_here[1]);
	}

	return location;
}


gchar *
gedit_plugin_program_location_get (gchar *program_name, gchar *plugin_name)
{
	gchar* program_location = NULL;
	gchar* config_string = NULL;
	gint error_code;
	

	program_location = gedit_plugin_guess_program_location (program_name);
	
	/* While "sendmail" is not valid, display error messages */
	while ((error_code = gedit_utils_is_program (program_location, program_name))!=GEDIT_PROGRAM_OK)
	{
		gchar *message = NULL;
		gchar *message_full;
		GtkWidget *dialog;

		if (program_location == NULL)
			message = g_strdup_printf (_("The program %s could not be found.\n\n"), program_name);
		else if (error_code == GEDIT_PROGRAM_IS_INSIDE_DIRECTORY)
		{
			/* the user chose a directory and "sendmail" was found inside it */
			/* message is use as a temp holder for the program */
			message = g_strdup (program_location);
			g_free (program_location);
			program_location = g_strdup_printf ("%s%s%s",
						    message,
						    (program_location [strlen(program_location)-1] == '/')?"":"/",
						    program_name);
			g_free (message);
			continue;
		}
		else if (error_code == GEDIT_PROGRAM_NOT_EXISTS)
			message = g_strdup_printf (_("'%s' doesn't seem to exist.\n\n"), program_location);
		else if (error_code == GEDIT_PROGRAM_IS_DIRECTORY)
			message = g_strdup_printf (_("'%s' seems to be a directory.\n"
						     "but '%s' could not be found inside it.\n\n"), program_location, program_name);
		else if (error_code == GEDIT_PROGRAM_NOT_EXECUTABLE)
			message = g_strdup_printf (_("'%s' doesn't seem to be a program.\n\n"), program_location);
		else
			g_return_val_if_fail (FALSE, NULL);

		message_full = g_strdup_printf (_("%sYou won't be able to use the %s "
						"plugin without %s. \n Do you want to "
						"specify a new location for %s?"),
						message,
						plugin_name,
						program_name,
						program_name);
		
		dialog = gnome_question_dialog (message_full, NULL, NULL);
		gnome_dialog_set_parent (GNOME_DIALOG(dialog), gedit_window_active());

		g_free (message);
		g_free (message_full);
		if (GNOME_YES == gnome_dialog_run_and_close (GNOME_DIALOG (dialog)))
		{
			g_free (program_location);  
			program_location = g_strdup (gedit_plugin_program_location_dialog ());
			continue;
		}
		else
			return NULL;
	}


	config_string = g_strdup_printf ("/gedit/plugin_programs/%s", program_name);
	gnome_config_set_string (config_string, program_location);
	gnome_config_sync ();
	g_free (config_string);
	
	return program_location;
}




