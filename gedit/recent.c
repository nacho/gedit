/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * gedit
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <config.h>
#include <gnome.h>
#include <libgnome/gnome-history.h>

#include "window.h"
#include "document.h"
#include "view.h"
#include "prefs.h"
#include "file.h"

#ifndef MAX_RECENT
#define MAX_RECENT 4
#endif

       void recent_update (GnomeApp *app);
static void recent_update_menus (GnomeApp *app, GList *recent_files);
static void recent_cb (GtkWidget *w, gedit_data *data);


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
	GList *gnome_recent_list;
	GnomeHistoryEntry histentry;
	char *filename;
	int i, j;
	int nrecentdocs = 0;

	filelist = NULL;

#if 0
	filelist = gnome_history_get_most_recent_files_for_app ("gedit",
								MAX_RECENT);
#endif
#if 1
	gnome_recent_list = gnome_history_get_recently_used ();

	if (g_list_length (gnome_recent_list) <= 0)
	{
		return;
	}

	for (i = g_list_length (gnome_recent_list) - 1; i >= 0; i--)
	{
		histentry = g_list_nth_data (gnome_recent_list, i);
     
		if (strcmp ("gedit", histentry->creator) != 0)
			break;

		/* FIXME : if file_1 is in the 3rd line of the recent
		   files and I reopen it, it should be in the 1st line now.Chema*/
		if (g_list_length (filelist) > 0)
		{
			for (j = g_list_length (filelist) - 1; j >= 0; j--)
			{
				if (strcmp (histentry->filename, g_list_nth_data (filelist, j)) == 0)
				{
					filelist = g_list_remove (filelist, g_list_nth_data (filelist, j));
					nrecentdocs--;
				}
			}
		}

		filename = g_malloc0 (strlen (histentry->filename) + 1);
		strcpy (filename, histentry->filename);
		filelist = g_list_append (filelist, filename);
		nrecentdocs++;
				
		/* For recent-directories, not yet fully implemented...
		   end_path = strrchr (histentry->filename, '/');
		   if (end_path) {
		   for (i = 0; i < strlen (histentry->filename); i++)
		   if ((histentry->filename + i) == end_path)
		   break;
		   directory = g_malloc0 (i + 2);
		   strcat (directory, histentry->filename, i);
		   }*/
                   
		if (nrecentdocs == MAX_RECENT)
/*			if (g_list_length (filelist) == MAX_RECENT) */
				break;
	}
	
	gnome_history_free_recently_used_list (gnome_recent_list);
#endif
	
	recent_update_menus (app, filelist);
}

static void
recent_update_menus (GnomeApp *app, GList *recent_files)
{
	GnomeUIInfo *menu;
	gedit_data *data;
	gchar *path;
	int i;

	g_return_if_fail (app != NULL);

	if (settings->num_recent)
		gnome_app_remove_menu_range (app, _("_File/"), 6, settings->num_recent + 1);

	if (recent_files == NULL)
		return;

	/* insert a separator at the beginning */
	menu = g_malloc0 (2 * sizeof (GnomeUIInfo));
	path = g_new (gchar, strlen (_("_File")) + strlen ("<Separator>") + 3 );
	sprintf (path, "%s/%s", _("_File"), "<Separator>");
	menu->type = GNOME_APP_UI_SEPARATOR;

	(menu + 1)->type = GNOME_APP_UI_ENDOFINFO;
	gnome_app_insert_menus (GNOME_APP(app), path, menu);

	for (i = g_list_length (recent_files) - 1; i >= 0;  i--)
	{
		menu = g_malloc0 (2 * sizeof (GnomeUIInfo));
	
		data = g_malloc0 (sizeof (gedit_data));
		data->temp1 = g_strdup (g_list_nth_data (recent_files, i));
	
		menu->label = g_new (gchar, strlen (g_list_nth_data (recent_files, i)) + 5);
		sprintf (menu->label, "_%i. %s", i+1, (gchar*)g_list_nth_data (recent_files, i));
		menu->type = GNOME_APP_UI_ITEM;
		menu->hint = NULL;
		menu->moreinfo = (gpointer) recent_cb;
		menu->user_data = data;
		menu->unused_data = NULL;
		menu->pixmap_type = 0;
		menu->pixmap_info = NULL;
		menu->accelerator_key = 0;

		(menu + 1)->type = GNOME_APP_UI_ENDOFINFO;
	
		gnome_app_insert_menus (GNOME_APP(app), path, menu);
		g_free (g_list_nth_data (recent_files, i));	 
	}
	
	g_free (menu);
	settings->num_recent = g_list_length (recent_files);
	g_list_free (recent_files);

	g_free (path);
}

static void
recent_cb (GtkWidget *widget, gedit_data *data)
{
	Document *doc = gedit_document_current ();
	
	g_return_if_fail (data != NULL);

	doc = gedit_document_new_with_file (data->temp1);
	if (doc)
	{
		gnome_mdi_add_child (mdi, GNOME_MDI_CHILD (doc));
		gnome_mdi_add_view (mdi, GNOME_MDI_CHILD (doc));
	}
	else
	{
		/* FIXME :
		   If an error was encountered the delete the entry
		   from the menu ... Chema  */
	}

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
	g_return_if_fail (filename != NULL);

	gnome_history_recently_used (filename, "text/plain",
				     "gedit", "gedit_document");
}





















