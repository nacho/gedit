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
#include <string.h>

#include <glib/gi18n.h>
#include <glib/goption.h>
#include <gdk/gdkx.h>
#include <libgnome/libgnome.h>
#include <libgnomeui/libgnomeui.h>
#include <libgnomevfs/gnome-vfs-utils.h>

#include "gedit-app.h"
#include "gedit-commands.h"
#include "gedit-convert.h"
#include "gedit-debug.h"
#include "gedit-encodings.h"
#include "gedit-metadata-manager.h"
#include "gedit-plugins-engine.h"
#include "gedit-prefs-manager-app.h"
#include "gedit-recent.h"
#include "gedit-session.h"
#include "gedit-utils.h"
#include "gedit-window.h"

#include "bacon-message-connection.h"

static guint32 startup_timestamp = 0;
static BaconMessageConnection *connection;

/* command line */
static gint line_position = 0;
static gchar *encoding_charset = NULL;
static const GeditEncoding *encoding;
static gboolean new_window_option = FALSE;
static gboolean new_document_option = FALSE;
static gchar **remaining_args = NULL;
static GSList *file_list = NULL;

static const GOptionEntry options [] =
{
	{ "encoding", '\0', 0, G_OPTION_ARG_STRING, &encoding_charset,
	  N_("Set the character encoding to be used to open the files listed on the command line"), "ENCODING" /* When out of string freeze, use N_("ENCODING") */ },

	{ "new-window", '\0', 0, G_OPTION_ARG_NONE, &new_window_option,
	  N_("Create a new toplevel window in an existing instance of gedit"), NULL },

	{ "new-document", '\0', 0, G_OPTION_ARG_NONE, &new_document_option,
	  N_("Create a new document in an existing instance of gedit"), NULL },

	{ G_OPTION_REMAINING, '\0', 0, G_OPTION_ARG_FILENAME_ARRAY, &remaining_args,
	  NULL, NULL }, /* collects file arguments */

	{NULL}
};

static void
gedit_get_command_line_data (GnomeProgram *program)
{
	if (remaining_args)
	{
		gint i;

		for (i = 0; remaining_args[i]; i++) 
		{
			if (*remaining_args[i] == '+')
			{
				if (*(remaining_args[i] + 1) == '\0')
					/* goto the last line of the document */
					line_position = G_MAXINT;
				else
					line_position = atoi (remaining_args[i] + 1);
			}
			else
			{
				gchar *canonical_uri;
				
				canonical_uri = gedit_utils_make_canonical_uri_from_shell_arg (remaining_args[i]);
				
				if (canonical_uri != NULL)
					file_list = g_slist_prepend (file_list, 
								     canonical_uri);
				else
					g_print (_("%s: malformed file name or URI.\n"),
						 remaining_args[i]);
			} 
		}

		file_list = g_slist_reverse (file_list);
	}

	if (encoding_charset)
	{
		encoding = gedit_encoding_get_from_charset (encoding_charset);
		if (encoding == NULL)
			g_print (_("%s: invalid encoding.\n"),
				 encoding_charset);

		g_free (encoding_charset);
		encoding_charset = NULL;
	}
}

static guint32
get_startup_timestamp ()
{
	const gchar *startup_id_env;
	gchar *startup_id = NULL;
	gchar *time_str;
	gchar *end;
	gulong retval = 0;

	/* we don't unset the env, since startup-notification
	 * may still need it */
	startup_id_env = g_getenv ("DESKTOP_STARTUP_ID");
	if (startup_id_env == NULL)
		goto out;

	startup_id = g_strdup (startup_id_env);

	time_str = g_strrstr (startup_id, "_TIME");
	if (time_str == NULL)
		goto out;

	errno = 0;

	/* Skip past the "_TIME" part */
	time_str += 5;

	retval = strtoul (time_str, &end, 0);
	if (end == time_str || errno != 0)
		retval = 0;

 out:
	g_free (startup_id);

	return (retval > 0) ? retval : 0;
}

static GdkDisplay *
display_open_if_needed (const gchar *name)
{
	GSList *displays;
	GSList *l;
	GdkDisplay *display = NULL;

	displays = gdk_display_manager_list_displays (gdk_display_manager_get ());

	for (l = displays; l != NULL; l = l->next)
	{
		display = l->data;

		if (strcmp (gdk_display_get_name (display), name) == 0)
			break;
	}

	g_slist_free (displays);

	return display != NULL ? display : gdk_display_open (name);
}

/* serverside */
static void
on_message_received (const char *message,
		     gpointer    data)
{
	gchar **commands;
	gchar **params;
	gint workspace;
	gchar *display_name;
	gint screen_number;
	gint i;
	GeditApp *app;
	GeditWindow *window;
	GdkDisplay *display;
	GdkScreen *screen;

	g_return_if_fail (message != NULL);

	gedit_debug_message (DEBUG_APP, "Received message:\n%s\n", message);

	commands = g_strsplit (message, "\v", -1);

	/* header */
	params = g_strsplit (commands[0], "\t", 4);
	startup_timestamp = atoi (params[0]); /* CHECK if this is safe */
	display_name = g_strdup (params[1]);
	screen_number = atoi (params[2]);
	workspace = atoi (params[3]);

	display = display_open_if_needed (display_name);
	screen = gdk_display_get_screen (display, screen_number);
	g_free (display_name);

	g_strfreev (params);

	/* body */
	i = 1;
	while (commands[i])
	{
		params = g_strsplit (commands[i], "\t", -1);

		if (strcmp (params[0], "NEW-WINDOW") == 0)
		{
			new_window_option = TRUE;
		}
		else if (strcmp (params[0], "NEW-DOCUMENT") == 0)
		{
			new_document_option = TRUE;
		}
		else if (strcmp (params[0], "OPEN-URIS") == 0)
		{
			gint n_uris, i;
			gchar **uris;

			line_position = atoi (params[1]);
			if (params[2] != '\0');
				encoding = gedit_encoding_get_from_charset (params[2]);

			n_uris = atoi (params[3]);
			uris = g_strsplit (params[4], " ", n_uris);

			for (i = 0; i < n_uris; i++)
				file_list = g_slist_prepend (file_list, uris[i]);
			file_list = g_slist_reverse (file_list);

			/* the list takes ownerhip of the strings,
			 * only free the array */
			g_free (uris);
		}
		else
		{
			g_warning ("Unexpected bacon command");
		}

		g_strfreev (params);
		++i;
	}

	g_strfreev (commands);

	/* execute the commands */

	app = gedit_app_get_default ();

	if (new_window_option)
	{
		window = gedit_app_create_window (app, screen);
	}
	else
	{
		/* get a window in the current workspace (if exists) and raise it */
		window = _gedit_app_get_window_in_workspace (app,
							     screen,
							     workspace);
	}

	if (file_list != NULL)
	{
		gedit_cmd_load_files_from_prompt (window,
						  file_list,
						  encoding,
						  line_position);

		if (new_document_option)
			gedit_window_create_tab (window, TRUE);
	}
	else
	{
		GeditDocument *doc;
		doc = gedit_window_get_active_document (window);

		if (doc == NULL ||
		    !gedit_document_is_untouched (doc) ||
		    new_document_option)
			gedit_window_create_tab (window, TRUE);
	}

	/* set the proper interaction time on the window.
	 * Fall back to roundtripping to the X server when we
	 * don't have the timestamp, e.g. when launched from
	 * terminal. We also need to make sure that the window
	 * has been realized otherwise it will not work. lame.
	 */
	if (!GTK_WIDGET_REALIZED (window))
		gtk_widget_realize (GTK_WIDGET (window));

	if (startup_timestamp <= 0)
		startup_timestamp = gdk_x11_get_server_time (GTK_WIDGET (window)->window);

	gdk_x11_window_set_user_time (GTK_WIDGET (window)->window,
				      startup_timestamp);

	gtk_window_present (GTK_WINDOW (window));

	/* free the file list and reset to default */
	g_slist_foreach (file_list, (GFunc) g_free, NULL);
	g_slist_free (file_list);
	file_list = NULL;

	new_window_option = FALSE;
	new_document_option = FALSE;
	encoding = NULL;
	line_position = 0;
}

/* clientside */
static void
send_bacon_message (void)
{
	GdkScreen *screen;
	GdkDisplay *display;
	const gchar *display_name;
	gint screen_number;
	gint ws;
	GString *command;

	/* the messages have the following format:
	 * <---                   header                       ---> <----            body             ----->
	 * timestamp \t display_name \t screen_number \t workspace \v OP1 \t arg \t arg \v OP2 \t arg \t arg|...
	 *
	 * when the arg is a list of uri, they are separated by a space.
	 * So the delimiters are \v for the commands, \t for the tokens in
	 * a command and ' ' for the uris: note that such delimiters cannot
	 * be part of an uri, this way parsing is easier.
	 */

	gedit_debug (DEBUG_APP);

	screen = gdk_screen_get_default ();
	display = gdk_screen_get_display (screen);

	display_name = gdk_display_get_name (display);
	screen_number = gdk_screen_get_number (screen);

	gedit_debug_message (DEBUG_APP, "Display: %s", display_name);
	gedit_debug_message (DEBUG_APP, "Screen: %d", screen_number);	

	ws = gedit_utils_get_current_workspace (screen);

	command = g_string_new (NULL);

	/* header */
	g_string_append_printf (command,
				"%" G_GUINT32_FORMAT "\t%s\t%d\t%d",
				startup_timestamp,
				display_name,
				screen_number,
				ws);

	/* NEW-WINDOW command */
	if (new_window_option)
	{
		command = g_string_append_c (command, '\v');
		command = g_string_append (command, "NEW-WINDOW");
	}

	/* NEW-DOCUMENT command */
	if (new_document_option)
	{
		command = g_string_append_c (command, '\v');
		command = g_string_append (command, "NEW-DOCUMENT");
	}

	/* OPEN_URIS command, optionally specify line_num and encoding */
	if (file_list)
	{
		GSList *l;

		command = g_string_append_c (command, '\v');
		command = g_string_append (command, "OPEN-URIS");

		g_string_append_printf (command,
					"\t%d\t%s\t%d\t",
					line_position,
					encoding_charset ? encoding_charset : "",
					g_slist_length (file_list));

		for (l = file_list; l != NULL; l = l->next)
		{
			command = g_string_append (command, l->data);
			if (l->next != NULL)
				command = g_string_append_c (command, ' ');
		}
	}

	gedit_debug_message (DEBUG_APP, "Bacon Message: %s", command->str);
	
	bacon_message_connection_send (connection,
				       command->str);

	g_string_free (command, TRUE);
}

int
main (int argc, char *argv[])
{
	GnomeProgram *program;
	GOptionContext *context;
	GeditWindow *window;
	GeditApp *app;
	gboolean restored = FALSE;

	/* Setup debugging */
	gedit_debug_init ();
	gedit_debug_message (DEBUG_APP, "Startup");
	
	setlocale (LC_ALL, "");

	bindtextdomain (GETTEXT_PACKAGE, GEDIT_LOCALEDIR);
	textdomain (GETTEXT_PACKAGE);

	startup_timestamp = get_startup_timestamp();

	gedit_debug_message (DEBUG_APP, "Run gnome_program_init");

	/* Setup command line options */
	context = g_option_context_new ("[FILE...]"); /* Should be translated when out of string freeze */
	g_option_context_add_main_entries (context, options, GETTEXT_PACKAGE);
	
	/* Initialize gnome program */
	program = gnome_program_init ("gedit", VERSION,
			    LIBGNOMEUI_MODULE, argc, argv,
			    GNOME_PARAM_GOPTION_CONTEXT, context,
			    GNOME_PARAM_HUMAN_READABLE_NAME,
		            _("Text Editor"),
			    GNOME_PARAM_APP_DATADIR, DATADIR,
			    NULL);

	gedit_debug_message (DEBUG_APP, "Done gnome_program_init");

	/* Must be called after gnome_program_init to avoid problem with the
         * translation of --help messages */
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");

	gedit_debug_message (DEBUG_APP, "Create bacon connection");

	connection = bacon_message_connection_new ("gedit");

	if (connection != NULL)
	{
		if (!bacon_message_connection_get_is_server (connection)) 
		{
			gedit_debug_message (DEBUG_APP, "I'm a client");

			gedit_get_command_line_data (program);

			send_bacon_message ();

			/* we never popup a window... tell startup-notification
			 * that we are done.
			 */
			gdk_notify_startup_complete ();

			bacon_message_connection_free (connection);

			exit (0);
		}
		else 
		{
		  	gedit_debug_message (DEBUG_APP, "I'm a server");

			bacon_message_connection_set_callback (connection,
							       on_message_received,
							       NULL);
		}
	}
	else
		g_warning ("Cannot create the 'gedit' connection.");

	gedit_debug_message (DEBUG_APP, "Set icon");
	
	/* Set default icon */
	gtk_window_set_default_icon_name ("text-editor");

	/* Load user preferences */
	gedit_debug_message (DEBUG_APP, "Init prefs manager");
	gedit_prefs_manager_app_init ();

	gedit_debug_message (DEBUG_APP, "Init recent files");
	gedit_recent_init ();

	/* Init plugins engine */
	gedit_debug_message (DEBUG_APP, "Init plugins");	
	gedit_plugins_engine_init ();

	gedit_debug_message (DEBUG_APP, "Init authentication manager");	
	gnome_authentication_manager_init ();
	gtk_about_dialog_set_url_hook (gedit_utils_activate_url, NULL, NULL);
	
	/* Initialize session management */
	gedit_debug_message (DEBUG_APP, "Init session manager");		
	gedit_session_init (argv[0]);

	if (gedit_session_is_restored ())
		restored = gedit_session_load ();

	if (!restored)
	{
		gedit_debug_message (DEBUG_APP, "Analyze command line data");
		gedit_get_command_line_data (program);
		
		gedit_debug_message (DEBUG_APP, "Get default app");
		app = gedit_app_get_default ();

		gedit_debug_message (DEBUG_APP, "Create main window");
		window = gedit_app_create_window (app, NULL);

		if (file_list != NULL)
		{
			gedit_debug_message (DEBUG_APP, "Load files");
			gedit_cmd_load_files_from_prompt (window, file_list, encoding, line_position);
		}
		else
		{
			gedit_debug_message (DEBUG_APP, "Create tab");
			gedit_window_create_tab (window, TRUE);
		}
		
		gedit_debug_message (DEBUG_APP, "Show window");
		gtk_widget_show (GTK_WIDGET (window));

		g_slist_foreach (file_list, (GFunc) g_free, NULL);
		g_slist_free (file_list);
		file_list = NULL;

		g_free (encoding_charset);

		new_window_option = FALSE;
		new_document_option = FALSE;
		encoding = NULL;
		line_position = 0;
	}

	gedit_debug_message (DEBUG_APP, "Start gtk-main");		
	gtk_main();

	bacon_message_connection_free (connection);

	gedit_plugins_engine_shutdown ();
	gedit_prefs_manager_app_shutdown ();
	gedit_metadata_manager_shutdown ();

	gnome_accelerators_sync ();

	g_object_unref (program);

	return 0;
}
