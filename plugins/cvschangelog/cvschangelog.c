/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * cvschangelog.c
 * This file is part of gedit
 *
 * Copyright (C) 2002 James Willcox
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
 *
 * AUTHORS :
 *   James Willcox <jwillcox@cs.indiana.edu>
 */

#include <libgnome/gnome-i18n.h>
#include <errno.h>
#include "gedit-plugin.h"
#include "gedit-file.h"
#include "gedit-debug.h"

#define GEDIT_CVSCHANGELOG_PLUGIN_PATH_STRING_SIZE 1

static gchar *
get_cwd (void)
{
	gint bufsize = GEDIT_CVSCHANGELOG_PLUGIN_PATH_STRING_SIZE * sizeof (gchar);
	gchar *buf = g_malloc (bufsize);
	
	while (!(getcwd (buf, bufsize))) {
		if (errno == ERANGE) {
			bufsize = bufsize * 2;
			buf = g_realloc (buf, bufsize);
		} else {
			/* what the heck happened? */
			g_free(buf);
			return NULL;
		}
	}
	
	return buf;
}

static gint
is_changelog (const gchar * path)
{
	int index = strlen (path) - 9;

	if (index >= 0) {
		if (!strcasecmp (&path[index], "changelog"))
			return TRUE;
	}

	return FALSE;
}

static void
cvs_changelogs_cb (BonoboUIComponent *uic, gpointer user_data, const gchar *verbname)
{
	GeditView *view = gedit_get_active_view ();
	GeditDocument *doc;
	gchar *buf;
	guchar *filename;
	gchar *current_dir;
	gchar *full_uri;
	gchar **split_file;
	gchar **split_line;
	gint i, j;


	gedit_debug (DEBUG_PLUGINS, "");

	g_return_if_fail (view);

	doc = gedit_view_get_document (view);

	buf = gedit_document_get_buffer (doc);

	/* split the file up line by line */
	split_file = g_strsplit (buf, "\n", 0);

	for (i = 0; split_file[i]; i++) {
		/* now split the line up by spaces */
		split_line = g_strsplit (split_file[i], " ", 0);

		for (j = 0; split_line[j]; j++) {
			filename = g_strstrip (split_line[j]);

			if (filename[strlen (filename) - 1] == '\n')
				filename[strlen (filename) - 1] = '\0';

			if (is_changelog (filename)) {
				current_dir = get_cwd();
				
				if(!current_dir)
					continue;
				
				full_uri = g_strdup_printf("file:///%s/%s", current_dir, filename);

				/* open the changelog */
				gedit_file_open_single_uri (full_uri);
				
				/* cleanup. */
				g_free(full_uri);
				g_free(current_dir);
			}
		}

		g_strfreev (split_line);
	}

	g_strfreev (split_file);
}

G_MODULE_EXPORT GeditPluginState
destroy (GeditPlugin *plugin)
{
	gedit_debug (DEBUG_PLUGINS, "");
}

G_MODULE_EXPORT GeditPluginState
activate (GeditPlugin *pd)
{
	GList* top_windows;
	
	gedit_debug (DEBUG_PLUGINS, "");

	top_windows = gedit_get_top_windows ();
	g_return_if_fail (top_windows != NULL);
       
	while (top_windows)
	{
		BonoboUIComponent* ui_component;
		gchar *item_path;
		ui_component = gedit_get_ui_component_from_window (
					BONOBO_WINDOW (top_windows->data));
		
		item_path = g_strdup ("/menu/File/FileOps/CVSChangelog");

		if (!bonobo_ui_component_path_exists (ui_component, item_path, NULL))
		{
			gchar *xml;
					
			xml = g_strdup ("<menuitem name=\"CVSChangelog\" verb=\"\""
					" _label=\"Open _CVS Changelogs\""
				        " _tip=\"Opens changelogs from CVS commit logs\" hidden=\"0\" />");

			bonobo_ui_component_set_translate (ui_component, 
					"/menu/File/FileOps/", xml, NULL);

			bonobo_ui_component_set_translate (ui_component, 
					"/commands/", "<cmd name = \"CVSChangelog\" />", NULL);

			bonobo_ui_component_add_verb (ui_component, 
					"CVSChangelog", cvs_changelogs_cb, 
					      NULL); 

			g_free (xml);
		}
		
		g_free (item_path);
		
		top_windows = g_list_next (top_windows);
	}

	return PLUGIN_OK;
}



G_MODULE_EXPORT GeditPluginState
deactivate (GeditPlugin *plugin)
{
	GList* top_windows;
	
	gedit_debug (DEBUG_PLUGINS, "");

	top_windows = gedit_get_top_windows ();
	g_return_if_fail (top_windows != NULL);
       
	while (top_windows)
	{
		BonoboUIComponent* ui_component;
		gchar *item_path;
		ui_component = gedit_get_ui_component_from_window (
					BONOBO_WINDOW (top_windows->data));
		
		item_path = g_strdup ("/menu/File/FileOps/CVSChangelog");

		if (bonobo_ui_component_path_exists (ui_component, item_path, NULL))
		{
			bonobo_ui_component_rm (ui_component, item_path, NULL);
			bonobo_ui_component_rm (ui_component, "/commands/CVSChangelog", NULL);
		}
		
		g_free (item_path);
		
		top_windows = g_list_next (top_windows);
	}
}

G_MODULE_EXPORT GeditPluginState
init (GeditPlugin* plugin)
{
	/* initialize */
	gedit_debug (DEBUG_PLUGINS, "");

	plugin->destroy = destroy;
	plugin->name = _("CVS ChangeLog");
	plugin->desc = _("A plugin that opens ChangeLogs found in CVS commit messages.");
	plugin->author = "James Willcox <jwillcox@cs.indiana.edu>";
	plugin->copyright = _("Copyright (C) 2002 - James Willcox");

	plugin->private_data = NULL;

	return PLUGIN_OK;
}
