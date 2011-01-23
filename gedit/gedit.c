/*
 * gedit.c
 * This file is part of gedit
 *
 * Copyright (C) 2005 - Paolo Maggi 
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
 * Modified by the gedit Team, 2005. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 *
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <locale.h>

#include <glib.h>
#include <glib/gi18n.h>

#include "gedit-command-line.h"
#include "gedit-dbus.h"

#include "gedit-app.h"
#include "gedit-encodings.h"

#include "gedit-commands.h"
#include "gedit-debug.h"
#include "gedit-dirs.h"
#include "gedit-encodings.h"
#include "gedit-plugins-engine.h"
#include "gedit-session.h"
#include "gedit-utils.h"
#include "gedit-window.h"

#ifndef ENABLE_GVFS_METADATA
#include "gedit-metadata-manager.h"
#define METADATA_FILE "gedit-metadata.xml"
#endif

#ifdef G_OS_UNIX
#include <gio/gunixinputstream.h>
#include <unistd.h>
#endif

static gboolean
gedit_main_load_from_stdin (GeditWindow *window,
                            gboolean     jump_to)
{
#ifdef G_OS_UNIX
	GInputStream *stream;
	const GeditEncoding *encoding;
	gint line_position;
	gint column_position;
	GeditCommandLine *command_line;

	command_line = gedit_command_line_get_default ();

	encoding = gedit_command_line_get_encoding (command_line);
	line_position = gedit_command_line_get_line_position (command_line);
	column_position = gedit_command_line_get_column_position (command_line);

	/* Construct a stream for stdin */
	stream = g_unix_input_stream_new (STDIN_FILENO, TRUE);

	gedit_window_create_tab_from_stream (window,
	                                     stream,
	                                     encoding,
	                                     line_position,
	                                     column_position,
	                                     jump_to);
	g_object_unref (stream);
	return TRUE;
#else
	return FALSE;
#endif
}

static void
gedit_main_window (void)
{
	GSList *file_list;
	GeditWindow *window;
	GeditCommandLine *command_line;
	GeditApp *app;
	gboolean doc_created = FALSE;
	const gchar *geometry;

	app = gedit_app_get_default ();

	gedit_debug_message (DEBUG_APP, "Create main window");
	window = gedit_app_create_window (app, NULL);

	command_line = gedit_command_line_get_default ();
	file_list = gedit_command_line_get_file_list (command_line);

	if (file_list != NULL)
	{
		GSList *loaded;
		const GeditEncoding *encoding;
		gint line_position;
		gint column_position;

		encoding = gedit_command_line_get_encoding (command_line);
		line_position = gedit_command_line_get_line_position (command_line);
		column_position = gedit_command_line_get_column_position (command_line);

		gedit_debug_message (DEBUG_APP, "Load files");
		loaded = _gedit_cmd_load_files_from_prompt (window,
		                                            file_list,
		                                            encoding,
		                                            line_position,
		                                            column_position);

		doc_created = loaded != NULL;
		g_slist_free (loaded);
	}

	if (gedit_utils_can_read_from_stdin ())
	{
		doc_created = gedit_main_load_from_stdin (window, !doc_created);
	}

	if (!doc_created || gedit_command_line_get_new_document (command_line))
	{
		gedit_debug_message (DEBUG_APP, "Create tab");
		gedit_window_create_tab (window, TRUE);
	}

	geometry = gedit_command_line_get_geometry (command_line);

	gedit_debug_message (DEBUG_APP, "Show window");
	gtk_widget_show (GTK_WIDGET (window));

	if (geometry)
	{
		gtk_window_parse_geometry (GTK_WINDOW (window),
		                           geometry);
	}
}

static void
gedit_main (gboolean service)
{
	GeditPluginsEngine *engine;
	GeditApp *app;
	gboolean restored = FALSE;
	const gchar *dir;
	gchar *icon_dir;

	gedit_debug_message (DEBUG_APP, "Set icon");

	dir = gedit_dirs_get_gedit_data_dir ();
	icon_dir = g_build_filename (dir, "icons", NULL);

	gtk_icon_theme_append_search_path (gtk_icon_theme_get_default (), icon_dir);
	g_free (icon_dir);

	/* Init plugins engine */
	gedit_debug_message (DEBUG_APP, "Init plugins");
	engine = gedit_plugins_engine_get_default ();

	app = gedit_app_get_default ();

	/* Initialize session management */
	gedit_debug_message (DEBUG_APP, "Init session manager");
	gedit_session_init ();

	if (!service && gedit_session_is_restored ())
	{
		restored = gedit_session_load ();
	}

	if (!service && !restored)
	{
		gedit_main_window ();
	}

	gedit_debug_message (DEBUG_APP, "Start gtk-main");
	gtk_main ();

	/* Cleanup */
	g_object_unref (engine);
	g_object_unref (app);

	gedit_dirs_shutdown ();

#ifndef ENABLE_GVFS_METADATA
	gedit_metadata_manager_shutdown ();
#endif
}

int
main (int argc, char *argv[])
{
	const gchar *dir;
	GeditCommandLine *command_line;
	gboolean ret;
	GeditDBus *dbus;
	GeditDBusResult dbusret;
	gboolean service = FALSE;

#ifndef ENABLE_GVFS_METADATA
	const gchar *cache_dir;
	gchar *metadata_filename;
#endif

	/* Init type system as soon as possible */
	g_type_init ();

	/* Init glib threads asap */
	g_thread_init (NULL);

	/* Setup debugging */
	gedit_debug_init ();
	gedit_debug_message (DEBUG_APP, "Startup");

	/* Setup locale/gettext */
	setlocale (LC_ALL, "");

	gedit_dirs_init ();

	dir = gedit_dirs_get_gedit_locale_dir ();
	bindtextdomain (GETTEXT_PACKAGE, dir);

	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

#ifndef ENABLE_GVFS_METADATA
	/* Setup metadata-manager */
	cache_dir = gedit_dirs_get_user_cache_dir ();

	metadata_filename = g_build_filename (cache_dir, METADATA_FILE, NULL);

	gedit_metadata_manager_init (metadata_filename);

	g_free (metadata_filename);
#endif

	/* Parse command line arguments */
	command_line = gedit_command_line_get_default ();

	ret = gedit_command_line_parse (command_line, &argc, &argv);

	if (!ret)
	{
		g_object_unref (command_line);
		return 1;
	}

	/* Run over dbus */
	dbus = gedit_dbus_new ();
	dbusret = gedit_dbus_run (dbus);

	switch (dbusret)
	{
		case GEDIT_DBUS_RESULT_SUCCESS:
		case GEDIT_DBUS_RESULT_FAILED: /* fallthrough */
			g_object_unref (command_line);
			g_object_unref (dbus);

			return dbusret == GEDIT_DBUS_RESULT_SUCCESS ? 0 : 1;
		break;
		case GEDIT_DBUS_RESULT_PROCEED_SERVICE:
			service = TRUE;
		break;
		case GEDIT_DBUS_RESULT_PROCEED:
		break;
	}

	gedit_main (service);

	g_object_unref (dbus);
	g_object_unref (command_line);

	return 0;
}

/* ex:set ts=8 noet: */
