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

#ifndef DATA_PLUGINS_TO_REMOVE
#define DATA_PLUGINS_TO_REMOVE "plugins_to_remove"
#endif


GList	*plugins_list = NULL;

void			gedit_plugins_init (void);
void			gedit_plugins_menu_add (GnomeApp *app);

gchar *			gedit_plugin_program_location_get (gchar *program_name, gchar *plugin_name, gint dont_guess);
gchar *			gedit_plugin_program_location_change (gchar * program_name, gchar * plugin_name);

static PluginData *	gedit_plugin_load (const gchar *file);
#if 0
static void		gedit_plugin_unload (PluginData *pd);
#endif
static void		gedit_plugin_load_dir (char *dir);
static void		gedit_plugin_load_all (void);
static gchar*		gedit_plugin_program_location_string (gchar *program_name);
static gchar*		gedit_plugin_guess_program_location (gchar *program_name);
static void		gedit_plugin_program_location_clear (gchar *program_name);




void
gedit_plugins_init (void)
{

	gedit_debug (DEBUG_PLUGINS, "");
	
	if (!g_module_supported ())
		return;
	
	gedit_plugin_load_all ();
}

void
gedit_plugins_menu_add (GnomeApp *app)
{
	PluginData  *pd;
	gint         n;
	gchar       *path;
	GnomeUIInfo *menu;
	gint *items_to_remove;
	gint menu_pos;

	gedit_debug (DEBUG_PLUGINS, "");

	g_return_if_fail (app != NULL);

	path = g_strdup_printf ("%s/", _("_Plugins"));
	
	items_to_remove = gtk_object_get_data (GTK_OBJECT(app), DATA_PLUGINS_TO_REMOVE);

	if(items_to_remove != NULL) {

		gnome_app_remove_menus (app, path, *items_to_remove);
	
		gtk_object_remove_data(GTK_OBJECT(app), DATA_PLUGINS_TO_REMOVE);
	}

	n = g_list_length (plugins_list) + 1 ;

	menu = g_malloc0 ( (n + 3)* sizeof (GnomeUIInfo));

	/* Add the plugin manager item */
	menu[0].type = GNOME_APP_UI_ITEM;
	menu[0].label = _("Manager ...");
	menu[0].hint  = _("Add/Remove installed plugins");
	menu[0].moreinfo = gedit_plugin_manager_create;
	menu[0].pixmap_type = GNOME_APP_PIXMAP_STOCK;
	menu[0].pixmap_info = GNOME_STOCK_MENU_BOOK_OPEN;

	/* Add a Separator*/
	menu[1].type = GNOME_APP_UI_SEPARATOR;

	menu_pos = 2;
	
	for (n = 0; n < g_list_length (plugins_list); n++)
	{
		pd = g_list_nth_data (plugins_list, n);

		if (!pd->installed)
			continue;
		
		menu [menu_pos].type = GNOME_APP_UI_ITEM;
		menu [menu_pos].label = pd->name;
		menu [menu_pos].hint = pd->desc;
		menu [menu_pos].moreinfo = pd->private_data;
		menu [menu_pos].pixmap_type = GNOME_APP_PIXMAP_NONE;
		menu_pos++;
	}
	
	menu [menu_pos].type = GNOME_APP_UI_ENDOFINFO;

	gnome_app_insert_menus (app, path, menu);
	gnome_app_install_menu_hints (app, menu);

	g_free (path);
	g_free (menu);

	items_to_remove = g_new0 (gint, 1);
	*items_to_remove = menu_pos;
	gtk_object_set_data_full (GTK_OBJECT(app), DATA_PLUGINS_TO_REMOVE, items_to_remove, g_free);
}

void
gedit_plugin_save_settings (void)
{
	gint n;
	gchar * config_string;
	PluginData *nth_plugin;
	
	for (n = 0; n < g_list_length (plugins_list); n++)
	{
		nth_plugin = g_list_nth_data (plugins_list, n);
		
		config_string = g_strdup_printf ("/gedit/installed_plugins/%s=",
						 g_basename(nth_plugin->file));
		gnome_config_set_bool (config_string, nth_plugin->installed);
		g_free (config_string);
	}
}
	
gchar *
gedit_plugin_program_location_get (gchar *program_name, gchar *plugin_name, gint dont_guess)
{
	gchar* program_location = NULL;
	gchar* config_string = NULL;
	gint error_code = 100;

	gedit_debug (DEBUG_PLUGINS, "");

	if (!dont_guess)
		program_location = gedit_plugin_guess_program_location (program_name);

	/* While "sendmail" is not valid, display error messages */
	while (dont_guess || (error_code = gedit_utils_is_program (program_location, program_name)) != GEDIT_PROGRAM_OK)
	{
		gchar *message = NULL;
		gchar *message_full;
		GtkWidget *dialog;

		if (dont_guess)
		{
			g_free (program_location);
			program_location = gedit_plugin_program_location_dialog ();
			dont_guess = FALSE;
			/* If the user cancelled or pressed ESC */
			if (program_location == NULL)
				return NULL;
			continue;
		}
		
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
						    (message [strlen(message)-1] == '/')?"":"/",
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
		/*
		gnome_dialog_set_parent (GNOME_DIALOG(dialog), gedit_window_active());
		*/

		g_free (message);
		g_free (message_full);
		if (GNOME_YES == gnome_dialog_run_and_close (GNOME_DIALOG (dialog)))
		{
			g_free (program_location);
			program_location = g_strdup (gedit_plugin_program_location_dialog ());
			continue;
		}
		else
		{
			return NULL;
		}
	}


	config_string = gedit_plugin_program_location_string (program_name);
	gnome_config_set_string (config_string, program_location);
	gnome_config_sync ();
	g_free (config_string);

	return program_location;
}



gchar *
gedit_plugin_program_location_change (gchar * program_name, gchar * plugin_name)
{
	gchar * config_string;
	gchar * old_location = NULL;
	gchar * new_location = NULL;
	gchar * temp_string;

	gedit_debug (DEBUG_PLUGINS, "");

	/* Save a copy of the old location, in case the user cancels the dialog */
	config_string = gedit_plugin_program_location_string (program_name);
	temp_string = gnome_config_get_string (config_string);
	if (temp_string)
		old_location = g_strdup (temp_string);
	g_free (temp_string);
	g_free (config_string);

	gedit_plugin_program_location_clear (program_name);
	new_location  = gedit_plugin_program_location_get (program_name,  plugin_name, TRUE);

	if (!new_location)
	{
		if (old_location != NULL)
		{
			config_string = gedit_plugin_program_location_string (program_name);
			gnome_config_set_string (config_string, old_location);
			g_free (config_string);
			gnome_config_sync ();
			g_free (old_location);
		}
		return NULL;
	}

	if (old_location)
		g_free (old_location);

	return new_location;
}

/* --------------------------------------- Private functions ---------------------------------*/
static PluginData *
gedit_plugin_load (const gchar *file)
{
	PluginData *pd;
	guint res;

	gchar * config_string;

	gedit_debug (DEBUG_PLUGINS, "");

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

	pd->needs_a_document = TRUE;
	pd->installed_by_default = FALSE;
	pd->desc = NULL;
	pd->long_desc = NULL;
	
	res = pd->init_plugin (pd);
	if (res != PLUGIN_OK)
	{
		g_warning (_("Error, init_plugin returned an error"));
		goto error;
	}
	if (pd->desc == NULL)
	{
		g_warning ("Error, the plugin did not specified a description" );
		goto error;
	}
	if (pd->long_desc == NULL)
	{
		g_warning ("Error, the plugin did not specified a long description" );
		goto error;
	}

	
	config_string = g_strdup_printf ("/gedit/installed_plugins/%s=%s",
					 g_basename(file),
					 pd->installed_by_default?"TRUE":"FALSE");
	pd->installed = gnome_config_get_bool(config_string);
	g_free (config_string);

	plugins_list = g_list_append (plugins_list, pd);

	return pd;
	
error:
	g_module_close (pd->handle);
	g_free (pd->file);
	g_free (pd);
	return NULL;
}

#if 0
/* NOt used, but we migth need it latter */
static void
gedit_plugin_unload (PluginData *pd)
{
	int w, n;
	char *path;
	GnomeApp *app;
	
	gedit_debug (DEBUG_PLUGINS, "");

	g_return_if_fail (pd != NULL);
	
	if (pd->can_unload && !pd->can_unload (pd))
	{
		g_warning (_("Error, plugin is still in use"));
		return;
	}
	
	if (pd->destroy_plugin)
		pd->destroy_plugin (pd);
	
	n = g_slist_index (plugins_list, pd);
	plugins_list = g_slist_remove (plugins_list, pd);
	
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
#endif 

static void
gedit_plugin_load_dir (char *dir)
{
	DIR *d;
	struct dirent *e;

	gedit_debug (DEBUG_PLUGINS, "");
	
	if ((d = opendir (dir)) == NULL)
		return;
	
	while ((e = readdir (d)) != NULL)
	{
		if (strncmp (e->d_name + strlen (e->d_name) - 3, ".so", 3) == 0)
		{
			char *plugin = g_strconcat (dir, e->d_name, NULL);
			gedit_plugin_load (plugin);
			g_free (plugin);
		}
	}
	closedir (d);
}

static void
gedit_plugin_load_all (void)
{
	char *pdir;
	char const * const home = g_get_home_dir ();
	
	gedit_debug (DEBUG_PLUGINS, "");

	/* load user plugins */
	if (home != NULL)
	{
		pdir = gnome_util_prepend_user_home (".gedit/plugins/");
		gedit_plugin_load_dir (pdir);
		g_free (pdir);
	}
	
	/* load system plgins */
	gedit_plugin_load_dir (GEDIT_PLUGINDIR "/");
}

static gchar*
gedit_plugin_program_location_string (gchar *program_name)
{
	gedit_debug (DEBUG_PLUGINS, "");
	return g_strdup_printf ("/gedit/plugin_programs/%s", program_name);
}

static gchar*
gedit_plugin_guess_program_location (gchar *program_name)
{
	gchar * location = NULL;
	gchar * config_string = NULL;

	gedit_debug (DEBUG_PLUGINS, "");

	config_string = gedit_plugin_program_location_string (program_name);

	location = gnome_config_get_string (config_string);

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
		location = gnome_is_program_in_path (program_name);

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

static void
gedit_plugin_program_location_clear (gchar *program_name)
{
	gchar *config_string;

	gedit_debug (DEBUG_PLUGINS, "");

	config_string = gedit_plugin_program_location_string (program_name);
	gnome_config_set_string (config_string, "");
	gnome_config_sync ();
	g_free (config_string);
}

