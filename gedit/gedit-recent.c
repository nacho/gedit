/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-recent.c
 * This file is part of gedit
 *
 * Copyright (C) 1998, 1999 Alex Roberts, Evan Lawrence, Jason Leach
 * Copyright (C) 2000, 2001 Chema Celorio, Paolo Maggi 
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
 * Modified by the gedit Team, 1998-2001. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libgnome/libgnome.h>
#include <libgnomeui/libgnomeui.h>

#include "gnome-vfs-helpers.h"
#include "gedit-recent.h"
#include "gedit-mdi.h"
#include "gedit2.h"
#include "gedit-debug.h"
#include "gedit-file.h"
#include "gedit-utils.h"


#ifndef MAX_RECENT
#define MAX_RECENT 4
#endif

static void 	gedit_recent_update_menus 		(BonoboWindow *win, GList *recent_files);
static void 	gedit_recent_cb 			(BonoboUIComponent *uic, gpointer user_data, 
							 const gchar* verbname);
static void 	gedit_recent_remove 			(char *filename);
static GList   *gedit_recent_history_get_list 		(void);
static gchar   *gedit_recent_history_update_list 	(const gchar *filename);

static GList *gedit_recent_history_list = NULL;


static gchar* 
escape_underscores (const gchar* text)
{
	GString *str;
	gint length;
	const gchar *p;
 	const gchar *end;

  	g_return_val_if_fail (text != NULL, NULL);

    	length = strlen (text);

	str = g_string_new ("");

  	p = text;
  	end = text + length;

  	while (p != end)
    	{
      		const gchar *next;
      		next = g_utf8_next_char (p);

		switch (*p)
        	{
       			case '_':
          			g_string_append (str, "__");
          			break;
        		default:
          			g_string_append_len (str, p, next - p);
          			break;
        	}

      		p = next;
    	}

	return g_string_free (str, FALSE);
}

static GList *
gedit_recent_history_get_list (void)
{
        gchar *filename, *key;
        gint max_entries, i;
        gboolean do_set = FALSE;

	gedit_debug (DEBUG_RECENT, "");
	
	if (gedit_recent_history_list)
		return gedit_recent_history_list;

	gnome_config_push_prefix ("/gedit2/History/");

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
        gnome_config_push_prefix ("/gedit2/History/");
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

        max_entries = gnome_config_get_int ("/gedit2/History/MaxFiles=4");
        gnome_config_clean_section ("/gedit2/History");
	
        gnome_config_push_prefix ("/gedit2/History/");
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


/**
 * gedit_recent_update_menus:
 * @app: 
 * @recent_files: 
 * 
 * Update the gui part of the recently-used menu
 * 
 **/
static void
gedit_recent_update_menus (BonoboWindow *win, GList *recent_files)
{
	BonoboUIComponent* ui_component;
	int i;

	gedit_debug (DEBUG_RECENT, "");

	g_return_if_fail (win != NULL);

	ui_component = bonobo_mdi_get_ui_component_from_window (win);
	g_return_if_fail (BONOBO_IS_UI_COMPONENT (ui_component));
	
	bonobo_ui_component_freeze (ui_component, NULL);

	for (i = 1; i <= g_list_length (recent_files); ++i)
	{
		gchar *label = NULL;
		gchar *verb_name = NULL;
		gchar *tip = NULL;
		gchar *escaped_name = NULL;
		gchar *item_path = NULL;
		gchar *uri;

		uri = gnome_vfs_x_format_uri_for_display (g_list_nth_data (recent_files, i - 1));
	
		escaped_name = escape_underscores (uri);

		tip =  g_strdup_printf (_("Open file %s"), uri);

		verb_name = g_strdup_printf ("FileRecentOpen%d", i);
	        
		label = g_strdup_printf ("_%d. %s", i, escaped_name);

		item_path = g_strdup_printf ("/menu/File/Recents/%s", verb_name);

		if (bonobo_ui_component_path_exists (ui_component, item_path, NULL))
		{
			bonobo_ui_component_set_prop (ui_component, item_path, 
					              "label", label, NULL);

			bonobo_ui_component_set_prop (ui_component, item_path, 
					              "tip", tip, NULL);
		}
		else
		{
			gchar *xml;
			
			xml = g_strdup_printf ("<menuitem name=\"%s\" verb=\"%s\""
						" _label=\"%s\"  _tip=\"%s \" hidden=\"0\" />", 
						verb_name, verb_name, label, tip);

			bonobo_ui_component_set_translate (ui_component, "/menu/File/Recents/", xml, NULL);

			g_free (xml); 
		}
		
		g_free (uri);
		g_free (tip);
		g_free (escaped_name);
		g_free (label); 
		g_free (verb_name);
		g_free (item_path);
	}

	for (i = g_list_length (recent_files) + 1; i <= MAX_RECENT; ++i)
	{
		gchar *item_path = g_strdup_printf ("/menu/File/Recents/FileRecentOpen%d", i);

		if (bonobo_ui_component_path_exists (ui_component, item_path, NULL))
			bonobo_ui_component_rm (ui_component, item_path, NULL);

		g_free (item_path);
	}

	bonobo_ui_component_thaw (ui_component, NULL);
}
	
/* Callback for a click on a file in the recently used menu */
static void
gedit_recent_cb (BonoboUIComponent *uic, gpointer user_data, const gchar* verbname)
{
	BonoboMDIChild *active_child;
	gchar *uri;
	gint i;
	
	gedit_debug (DEBUG_RECENT, "%s", verbname);

	i = GPOINTER_TO_INT (user_data);

	uri = g_list_nth_data (gedit_recent_history_list, i - 1);
	active_child = bonobo_mdi_get_active_child (BONOBO_MDI (gedit_mdi));

	if (!gedit_file_open_recent (active_child != NULL ? GEDIT_MDI_CHILD (active_child) : NULL, uri))
	{
		gchar* t = gnome_vfs_x_format_uri_for_display (uri);
		gedit_utils_flash_va (_("Unable to open recent file: %s"), t);
		g_free (t);
		gedit_recent_remove (uri);
		gedit_recent_history_write_config ();
		gedit_recent_update_all_windows (BONOBO_MDI (gedit_mdi));		
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
gedit_recent_update (BonoboWindow *win)
{
	GList *filelist = NULL;

	gedit_debug (DEBUG_RECENT, "");

	filelist = gedit_recent_history_get_list ();

	gedit_recent_update_menus (win, filelist);
}

/**
 * recent_update_all_windows:
 * @app: 
 *
 * Updates the recent files menus in all open windows.
 * Should be called after each addition to the recent documents list.
 **/
void
gedit_recent_update_all_windows (BonoboMDI *mdi)
{
	gint i;
	GList *windows;
	
	gedit_debug (DEBUG_RECENT, "");
	
	g_return_if_fail (mdi != NULL);

	windows = bonobo_mdi_get_windows (mdi);
       	g_return_if_fail (windows != NULL);
       
	for (i = 0; i < g_list_length (windows); i++)
        {
                gedit_recent_update (BONOBO_WINDOW (g_list_nth_data (windows, i)));
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
	gchar *uri;

	gedit_debug (DEBUG_RECENT, "");

	g_return_if_fail (file_name != NULL);

	uri = gnome_vfs_x_make_uri_canonical (file_name);
	g_return_if_fail (uri != NULL);

	del_name = gedit_recent_history_update_list (uri);

	g_free (uri);
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

void 
gedit_recent_init (BonoboWindow *win)
{
	BonoboUIComponent* ui_component;
	int i;

	ui_component = bonobo_mdi_get_ui_component_from_window (win);
	g_return_if_fail (BONOBO_IS_UI_COMPONENT (ui_component));
	
	bonobo_ui_component_freeze (ui_component, NULL);

	for (i = 1; i <= MAX_RECENT; ++i)
	{
		gchar* cmd;
		gchar* verb_name;
		
		verb_name = g_strdup_printf ("FileRecentOpen%d", i);
		cmd = g_strdup_printf ("<cmd name = \"%s\" /> ", verb_name);
		
		bonobo_ui_component_set_translate (ui_component, "/commands/", cmd, NULL);
		bonobo_ui_component_add_verb (ui_component, verb_name, gedit_recent_cb, 
				GINT_TO_POINTER (i)); 

		g_free (cmd);
		g_free (verb_name);
	}

	
	bonobo_ui_component_thaw (ui_component, NULL);

	gedit_recent_update (win);
}

	

			

