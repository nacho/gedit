/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit
 *
 * Copyright (C) 1998, 1999, 2000 Alex Roberts, Evan Lawrence, Jason Leach, Jose M Celorio
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

#include <libgnomevfs/gnome-vfs.h>

#include "document.h"
#include "view.h"
#include "prefs.h"
#include "file.h"
#include "utils.h"
#include "window.h"
#include "recent.h"

#ifndef MAX_RECENT
#define MAX_RECENT 4
#endif

#ifndef DATA_ITEMS_TO_REMOVE
#define DATA_ITEMS_TO_REMOVE "items_to_remove"
#endif

       void gedit_recent_update (GnomeApp *app);
static void gedit_recent_update_menus (GnomeApp *app, GList *recent_files);
static void recent_cb (GtkWidget *w, gpointer data);
       void gedit_recent_remove (char *filename);

static GList *	gedit_recent_history_get_list (void);
static gchar *	gedit_recent_history_update_list (const gchar *filename);
void		gedit_recent_history_write_config (void);

/* FIXME: make the list local */
GList *gedit_recent_history_list = NULL;

static GList *
gedit_recent_history_get_list (void)
{
        gchar *filename, *key;
        gint max_entries, i;
        gboolean do_set = FALSE;

	gedit_debug (DEBUG_RECENT, "");
	
	if (gedit_recent_history_list)
		return gedit_recent_history_list;

	gnome_config_push_prefix ("/gedit/History/");

        /* Get maximum number of history entries.  Write default value to 
         * config file if no entry exists. */
        max_entries = gnome_config_get_int_with_default ("MaxFiles=4", &do_set);
	if (do_set)
                gnome_config_set_int ("MaxFiles", 4);

        /* Read the history filenames from the config file */
        for (i = 0; i < max_entries; i++)
	{
                key = g_strdup_printf ("File%d", i);
                filename = gnome_config_get_string (key);
                if (filename == NULL)
		{
			/* Ran out of filenames. */
			g_free (key);
			break;
                }
                gedit_recent_history_list = g_list_append (gedit_recent_history_list, filename);
                g_free (key);
        }

        gnome_config_pop_prefix ();

        return gedit_recent_history_list;
}

/**
 * gedit_recent_history_update_list:
 * @filename: 
 * 
 * This function updates the history list.  The return value is a 
 * pointer to the filename that was removed, if the list was already full
 * or NULL if no items were removed.
 * 
 * Return value: 
 **/
static gchar *
gedit_recent_history_update_list (const gchar *filename)
{
        gchar *name, *old_name = NULL;
        GList *l = NULL;
        GList *new_list = NULL;
        gint max_entries, count = 0;
        gboolean do_set = FALSE;
        gboolean found = FALSE;

	gedit_debug (DEBUG_RECENT, "");

        g_return_val_if_fail (filename != NULL, NULL);

        /* Get maximum list length from config */
        gnome_config_push_prefix ("/gedit/History/");
        max_entries = gnome_config_get_int_with_default ("MaxFiles=4", &do_set);
	if (do_set)
                gnome_config_set_int ("MaxFiles", max_entries);
        gnome_config_pop_prefix ();

        /* Check if this filename already exists in the list */
        for (l = gedit_recent_history_list; l && (count < max_entries); l = l->next)
	{
                if (!found && (!strcmp ((gchar *)l->data, filename)
			       || (count == max_entries - 1)))
		{
                        /* This is either the last item in the list, or a
                         * duplicate of the requested entry. */
                        old_name = (gchar *)l->data;
                        found = TRUE;
                }
		else  /* Add this item to the new list */
                        new_list = g_list_append (new_list, l->data);

                count++;
        }

        /* Insert the new filename to the new list and free up the old list */
        name = g_strdup (filename);
        new_list = g_list_prepend (new_list, name);
        g_list_free (gedit_recent_history_list);
        gedit_recent_history_list = new_list;

        return old_name;
}

/**
 * gedit_recent_history_write_config:
 * @void: 
 * 
 * Write contents of the history list to the configuration file.
 * 
 * Return Value: 
 **/
void
gedit_recent_history_write_config (void)
{
        gchar *key; 
        GList *l;
        gint max_entries, i = 0;

	gedit_debug (DEBUG_RECENT, "");

        max_entries = gnome_config_get_int ("/gedit/History/MaxFiles=4");
        gnome_config_clean_section ("/gedit/History");
	
        gnome_config_push_prefix ("/gedit/History/");
        gnome_config_set_int ("MaxFiles", max_entries);

        for (l = gedit_recent_history_list; l; l = l->next) {
                key = g_strdup_printf ("File%d", i++);
                gnome_config_set_string (key, (gchar *)l->data);
                g_free (l->data);
                g_free (key);
        }
	gnome_config_sync ();
        gnome_config_pop_prefix ();

        g_list_free (gedit_recent_history_list);
        gedit_recent_history_list = NULL;
}


static void
gedit_recent_add_menu_item (GnomeApp *app, const gchar *file_name, const gchar *path, gint num)
{
	GnomeUIInfo *menu;
	gchar* unescaped_str;

	gedit_debug (DEBUG_RECENT, "");

	g_return_if_fail (file_name != NULL);
	g_return_if_fail (path != NULL);

	unescaped_str = gnome_vfs_unescape_string_for_display (file_name);
	g_return_if_fail (unescaped_str != NULL);
	
	menu = g_malloc0 (2 * sizeof (GnomeUIInfo));
	menu->label = g_strdup_printf ("_%i. %s", num, unescaped_str);
	menu->type = GNOME_APP_UI_ITEM;
	menu->hint = NULL;
	menu->moreinfo = (gpointer) recent_cb;
	menu->user_data = (gchar *) file_name;
	menu->unused_data = NULL;
	menu->pixmap_type = 0;
	menu->pixmap_info = NULL;
	menu->accelerator_key = 0;
	
	(menu + 1)->type = GNOME_APP_UI_ENDOFINFO;

	gnome_app_insert_menus (GNOME_APP(app), path, menu);

	g_free (unescaped_str);
	g_free (menu->label);

	g_free (menu);
}

/**
 * gedit_recent_update_menus:
 * @app: 
 * @recent_files: 
 * 
 * Update the greaphica part of the recently-used menu
 * 
 **/
static void
gedit_recent_update_menus (GnomeApp *app, GList *recent_files)
{	
	static gint items_in_menu = 0;
	const gchar *file_name;
	gchar *path;
	int i;
	gint* items_to_remove = NULL;

	gedit_debug (DEBUG_RECENT, "");

	g_return_if_fail (app != NULL);

	items_to_remove = gtk_object_get_data (GTK_OBJECT(app), DATA_ITEMS_TO_REMOVE);

	if(items_to_remove != NULL) {

	        /* xgettext translators : Use the name of the stock 
		 * Menu item for "File"	if it is not the same, we are going to 
		 * fail to insert the recent menu items.*/
		gnome_app_remove_menu_range (app, _("_File/"), 7, *items_to_remove);
	
		gtk_object_remove_data(GTK_OBJECT(app), DATA_ITEMS_TO_REMOVE);
	}

	items_in_menu = g_list_length (recent_files);
	
        /* xgettext translators : Use the name of the stock Menu item for "File"
	   if it is not the same, we are going to fail to insert the recent menu items. */
	path = g_strdup_printf ("%s/%s", _("_File"), "<Separator>");
	for (i = items_in_menu; i > 0;  i--) {
		file_name = g_list_nth_data (recent_files, i-1);
		gedit_recent_add_menu_item (app, file_name, path, i);
	}
	g_free (path);

	items_to_remove = g_new0 (gint, 1);
	*items_to_remove = items_in_menu;
	gtk_object_set_data_full (GTK_OBJECT(app), DATA_ITEMS_TO_REMOVE,items_to_remove, g_free);
}
	
/* Callback for a click on a file in the recently used menu */
static void
recent_cb (GtkWidget *widget, gpointer data)
{
	gedit_debug (DEBUG_RECENT, "");

	g_return_if_fail (data != NULL);

	if (!gedit_document_new_with_file (data))
	{
		gedit_flash_va (_("Unable to open recent file : %s"), (gchar *) data);
		gedit_recent_remove ((gchar *) data);
		gedit_recent_history_write_config ();
		gedit_recent_update_all_windows (mdi);		
	}
	else
	{
		gedit_window_set_widgets_sensitivity_ro (gedit_window_active_app(), gedit_document_current()->readonly);
	}

}

/**
 * recent_update:
 * @app: 
 *
 * Grabs the list of recently used documents, then updates the menus by
 * calling recent_update_menus().  Should be called when a new
 * window is created. It updates the menu of a single window.
 **/
void
gedit_recent_update (GnomeApp *app)
{
	GList *filelist = NULL;

	gedit_debug (DEBUG_RECENT, "");

	filelist = gedit_recent_history_get_list ();

	gedit_recent_update_menus (app, filelist);
}

/**
 * recent_update_all_windows:
 * @app: 
 *
 * Updates the recent files menus in all open windows.
 * Should be called after each addition to the recent documents list.
 **/
void
gedit_recent_update_all_windows (GnomeMDI *mdi)
{
	gint i;
	
	gedit_debug (DEBUG_RECENT, "");

	g_assert(mdi != NULL);
       	g_assert(mdi->windows != NULL);
       
	for (i = 0; i < g_list_length (mdi->windows); i++)
        {
                gedit_recent_update (GNOME_APP (g_list_nth_data (mdi->windows, i)));
        }
}


/**
 * recent_add:
 * @filename: Filename of document to add to the recently accessed list
 *
 * Record a file in GNOME's recent documents database 
 **/
void
gedit_recent_add (const char *file_name)
{
	gchar *del_name;

	gedit_debug (DEBUG_RECENT, "");

	g_return_if_fail (file_name != NULL);

	del_name = gedit_recent_history_update_list (file_name);

	g_free (del_name);
}


/**
 * recent_remove:
 * @filename: Filename of document to remove from the recently accessed list
 *
 * Record a file in GNOME's recent documents database 
 **/
void
gedit_recent_remove (char *file_name)
{
	gint n;
	guchar * nth_list_item;

	gedit_debug (DEBUG_RECENT, "");

	for (n=0; n < g_list_length (gedit_recent_history_list); n++)
	{
		nth_list_item = g_list_nth_data (gedit_recent_history_list, n);
		if (strcmp (nth_list_item, file_name) == 0)
		{
			gedit_recent_history_list = g_list_remove (gedit_recent_history_list, nth_list_item);
			g_free (nth_list_item);			
			return;
		}
	}
}
