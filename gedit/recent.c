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

#include "document.h"
#include "view.h"
#include "prefs.h"
#include "file.h"
#include "utils.h"

#ifndef MAX_RECENT
#define MAX_RECENT 4
#endif

       void recent_update (GnomeApp *app);
static void recent_update_menus (GnomeApp *app, GList *recent_files);
static void recent_cb (GtkWidget *w, gpointer data);
       void recent_add (char *filename);

static GList *	history_get_list (void);
gchar *		history_update_list (gchar *filename);
void		history_write_config (void);


GList *history_list = NULL;

static GList *
history_get_list (void)
{
        gchar *filename, *key;
        gint max_entries, i;
        gboolean do_set = FALSE;

	gedit_debug ("", DEBUG_RECENT);
	
	if (history_list)
		return history_list;

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
                history_list = g_list_append (history_list, filename);
                g_free (key);
        }
        gnome_config_pop_prefix ();

        return history_list;
}

/**
 * history_update_list:
 * @filename: 
 * 
 * This function updates the history list.  The return value is a 
 * pointer to the filename that was removed, if the list was already full
 * or NULL if no items were removed.
 * 
 * Return value: 
 **/
gchar *
history_update_list (gchar *filename)
{
        gchar *name, *old_name = NULL;
        GList *l = NULL;
        GList *new_list = NULL;
        gint max_entries, count = 0;
        gboolean do_set = FALSE;
        gboolean found = FALSE;

	gedit_debug ("", DEBUG_RECENT);

        g_return_val_if_fail (filename != NULL, NULL);

        /* Get maximum list length from config */
        gnome_config_push_prefix ("/gedit/History/");
        max_entries = gnome_config_get_int_with_default ("MaxFiles=4", &do_set);
	if (do_set)
                gnome_config_set_int ("MaxFiles", max_entries);
        gnome_config_pop_prefix ();

        /* Check if this filename already exists in the list */
        for (l = history_list; l && (count < max_entries); l = l->next)
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
        g_list_free (history_list);
        history_list = new_list;

        return old_name;
}

/* Write contents of the history list to the configuration file. */
void
history_write_config (void)
{
        gchar *key; 
        GList *l;
        gint max_entries, i = 0;

	gedit_debug ("", DEBUG_RECENT);

        if (history_list == NULL)
		return;

        max_entries = gnome_config_get_int ("/gedit/History/MaxFiles=4");
        gnome_config_clean_section ("/gedit/History");
        gnome_config_push_prefix ("/gedit/History/");
        gnome_config_set_int ("MaxFiles", max_entries);

        for (l = history_list; l; l = l->next) {
                key = g_strdup_printf ("File%d", i++);
                gnome_config_set_string (key, (gchar *)l->data);
                g_free (l->data);
                g_free (key);
        }
	gnome_config_sync ();
        gnome_config_pop_prefix ();

        g_list_free (history_list);
        history_list = NULL;
}

/* Update the graphical part of the recently-used menu */
static void
recent_update_menus (GnomeApp *app, GList *recent_files)
{
	GnomeUIInfo *menu;
	gchar *filename;
	gchar *path;
	int i;

	gedit_debug ("", DEBUG_RECENT);

	g_return_if_fail (app != NULL);

	if (settings->num_recent)
		gnome_app_remove_menu_range (app, _("_File/"), 6, 1+settings->num_recent);

	if (recent_files == NULL)
		return;

	/* insert a separator at the beginning */
	menu = g_malloc0 (2 * sizeof (GnomeUIInfo));

	path = g_new (gchar, strlen (_("_File")) + strlen ("<Separator>") + 2);
	sprintf (path, "%s/%s", _("_File"), "<Separator>");

	menu->type = GNOME_APP_UI_SEPARATOR;
	(menu + 1)->type = GNOME_APP_UI_ENDOFINFO;
	gnome_app_insert_menus (GNOME_APP(app), path, menu);

	for (i = g_list_length (recent_files) - 1; i >= 0;  i--)
	{
		gchar *data = g_list_nth_data (recent_files, i);
		
		filename = g_strdup (data);

		menu->label = g_strdup_printf ("_%i. %s", i+1, data);
		menu->type = GNOME_APP_UI_ITEM;
		menu->hint = NULL;
		menu->moreinfo = (gpointer) recent_cb;
		menu->user_data = filename;
		menu->unused_data = NULL;
		menu->pixmap_type = 0;
		menu->pixmap_info = NULL;
		menu->accelerator_key = 0;

		(menu + 1)->type = GNOME_APP_UI_ENDOFINFO;

		gnome_app_insert_menus (GNOME_APP(app), path, menu);

		g_free (menu->label);
		/*
		  OK. here is a memleak, BUT if we free filename
		  the recent stuff will not work. FIXME :
		  chema .
		  g_free (filename);
		*/
	}

	g_free (menu);

	settings->num_recent = g_list_length (recent_files);
	g_free (path);
}

/* Callback for a click on a file in the recently used menu */
static void
recent_cb (GtkWidget *widget, gpointer data)
{
	Document *doc;
	
	gedit_debug ("", DEBUG_RECENT);

	g_return_if_fail (data != NULL);

	doc = gedit_document_new_with_file (data);

	if (!doc)
		gedit_flash_va (_("Unable to open recent file : %s"), (gchar *) data);

	/*FIXME: If an error was encountered the delete the
	  entry from the menu. Chema */
}

/**
 * recent_update:
 * @app: 
 *
 * Grabs the list of recently used documents, then updates the menus by
 * calling recent_update_menus().  Should be called after each addition
 * to the recent documents list.
 **/
void
recent_update (GnomeApp *app)
{
	GList *filelist = NULL;

	gedit_debug ("", DEBUG_RECENT);

	filelist = history_get_list ();

	recent_update_menus (app, filelist);
}

/**
 * recent_add:
 * @filename: Filename of document to add to the recently accessed list
 *
 * Record a file in GNOME's recent documents database 
 **/
void
recent_add (char *filename)
{
	gchar *del_name;

	gedit_debug ("", DEBUG_RECENT);

	g_return_if_fail (filename != NULL);

	del_name = history_update_list (filename);
	g_free (del_name);
}





















