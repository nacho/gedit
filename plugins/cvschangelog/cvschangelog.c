/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit
 *
 * Copyright (C) 1999, 2000 Alex Roberts, Evan Lawrence, Jason Leach & Jose M Celorio
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

#include <config.h>
#include <errno.h>
#include <gnome.h>

#include "window.h"
#include "document.h"
#include "utils.h"
#include "view.h"
#include "plugin.h"

#define GEDIT_CVSCHANGELOG_PLUGIN_PATH_STRING_SIZE 1

/**
 * destroy_plugin:
 * @pd:
 *
 * Plugin destruction.
 *
 * Return Value: void
 **/
static void
destroy_plugin (PluginData * pd)
{
	gedit_debug (DEBUG_PLUGINS, "");
}

/**
 * get_cwd:
 * @:
 *
 * Returns a string containing the current working directory.  
 *
 * Return Value: a pointer the current working directory. Caller should free the string
 **/
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

/**
 * is_changelog:
 * @path: a path to a file
 *
 * Determine if the path is a changelog file
 *
 * Return Value: TRUE if the path is a changelog file, FALSE otherwise
 **/
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

/**
 * open_changelogs:
 * @:
 *
 * Opens all changelogs found in the current buffer.
 *
 * Return Value: void
 **/
static void
open_changelogs (void)
{
	GeditView *view = gedit_view_active ();
	guchar *buf;
	guchar *filename;
	gchar *current_dir;
	gchar *full_filename;
	gchar **split_file;
	gchar **split_line;
	gint i, j;


	gedit_debug (DEBUG_PLUGINS, "");

	if (!view)
		return;

	buf = gedit_document_get_buffer (view->doc);

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
				
				full_filename = g_strdup_printf("%s/%s", current_dir, filename);

				/* open the changelog */
				gedit_document_new_with_file (full_filename);
				
				/* cleanup. */
				g_free(full_filename);
				g_free(current_dir);
			}
		}

		g_strfreev (split_line);
	}

	g_strfreev (split_file);
}

/**
 * init_changelogs:
 * @pd:
 *
 * Plugin initialization.
 *
 * Return Value: gint
 **/
gint
init_plugin (PluginData * pd)
{
	/* initialize */
	gedit_debug (DEBUG_PLUGINS, "");

	pd->destroy_plugin = destroy_plugin;
	pd->name = _("CVS ChangeLog");
	pd->desc = _("A plugin that opens ChangeLogs found in CVS commit log messages.");
	pd->long_desc = _("A plugin that opens ChangeLogs found in CVS commit log messages.");
	pd->author = "James Willcox <jwillcox@cs.indiana.edu>";
	pd->needs_a_document = TRUE;
	pd->modifies_an_existing_doc = TRUE;

	pd->private_data = (gpointer) open_changelogs;

	return PLUGIN_OK;
}
