/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit2.c
 * This file is part of gedit
 *
 * Copyright (C) 1998, 1999 Alex Roberts, Evan Lawrence, Jason Leach
 * Copyright (C) 2000, 2002 Chema Celorio, Paolo Maggi 
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
 * Modified by the gedit Team, 1998-2002. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include <libgnome/libgnome.h>
#include <libgnomeui/libgnomeui.h>
#include <libgnomeui/gnome-window-icon.h>
#include <eel/eel-vfs-extensions.h>

#include "gedit2.h"
#include "gedit-mdi.h"
#include "gedit-prefs-manager-app.h"
#include "gedit-debug.h"
#include "gedit-file.h"
#include "gedit-utils.h"
#include "gedit-session.h"
#include "gedit-plugins-engine.h"
#include "gedit-application-server.h"
#include "gedit-convert.h"
#include "gedit-metadata-manager.h"

#ifndef GNOME_ICONDIR
#define GNOME_ICONDIR "" 
#endif

GeditMDI *gedit_mdi = NULL;
gboolean gedit_close_x_button_pressed = FALSE;
gboolean gedit_exit_button_pressed = FALSE; 
BonoboObject *gedit_app_server = NULL;

static gboolean quit_option = FALSE;
static gboolean new_window_option = FALSE;
static gboolean new_document_option = FALSE;

typedef struct _CommandLineData CommandLineData;

struct _CommandLineData
{
	GList* file_list;
	gint line_pos;
};

static void gedit_set_default_icon ();
static void gedit_load_file_list (CommandLineData *data);

static const struct poptOption options [] =
{
	{ "debug-utils", '\0', POPT_ARG_NONE, &debug_utils, 0,
	  N_("Show utility debugging messages."), NULL },

	{ "debug-metadata", '\0', POPT_ARG_NONE, &debug_utils, 0,
	  N_("Show metadata debugging messages."), NULL },

	{ "debug-mdi", '\0', POPT_ARG_NONE, &debug_mdi, 0,
	  N_("Show mdi debugging messages."), NULL },

	{ "debug-commands", '\0', POPT_ARG_NONE, &debug_commands, 0,
	  N_("Show commands debugging messages."), NULL },

	{ "debug-document", '\0', POPT_ARG_NONE, &debug_document, 0,
	  N_("Show document debugging messages."), NULL },

	{ "debug-file", '\0', POPT_ARG_NONE, &debug_file, 0,
	  N_("Show file debugging messages."), NULL },

	{ "debug-plugins", '\0', POPT_ARG_NONE, &debug_plugins, 0,
	  N_("Show plugin debugging messages."), NULL },

	{ "debug-prefs", '\0', POPT_ARG_NONE, &debug_prefs, 0,
	  N_("Show prefs debugging messages."), NULL },

	{ "debug-print", '\0', POPT_ARG_NONE, &debug_print, 0,
	  N_("Show printing debugging messages."), NULL },

	{ "debug-search", '\0', POPT_ARG_NONE, &debug_search, 0,
	  N_("Show search debugging messages."), NULL },

	{ "debug-undo", '\0', POPT_ARG_NONE, &debug_undo, 0,
	  N_("Show undo debugging messages."), NULL },

	{ "debug-view", '\0', POPT_ARG_NONE, &debug_view, 0,
	  N_("Show view debugging messages."), NULL },

	{ "debug-recent", '\0', POPT_ARG_NONE, &debug_recent, 0,
	  N_("Show recent debugging messages."), NULL },

	 { "debug-session", '\0', POPT_ARG_NONE, &debug_session, 0,
	  N_("Show session management debugging messages."), NULL },

	{ "debug", '\0', POPT_ARG_NONE, &debug, 0,
	  N_("Turn on all debugging messages."), NULL },

	{ "quit", '\0', POPT_ARG_NONE, &quit_option, 0,
	  N_("Quit an existing instance of gedit"), NULL },

	{ "new-window", '\0', POPT_ARG_NONE, &new_window_option, 0,
	  N_("Create a new toplevel window in an existing instance of gedit"), NULL },

	{ "new-document", '\0', POPT_ARG_NONE, &new_document_option, 0,
	  N_("Create a new document in an existing instance of gedit"), NULL },

	{NULL, '\0', 0, NULL, 0}
};

static void 
gedit_set_default_icon ()
{
	if (!g_file_test (GNOME_ICONDIR "/gedit-icon.png", G_FILE_TEST_EXISTS))
	{
		g_warning ("Could not find %s", GNOME_ICONDIR "/gedit-icon.png");
	    
		/* In case we haven't yet been installed */
		gnome_window_icon_set_default_from_file ("../pixmaps/gedit-icon.png");
	}
	else
	{
		gnome_window_icon_set_default_from_file (GNOME_ICONDIR "/gedit-icon.png");
	}
}

static void 
gedit_load_file_list (CommandLineData *data)
{	
	gboolean res;

	res = gedit_file_open_from_stdin (NULL);

	if (!data) 
	{
		if (!res)
			gedit_file_new ();
		return;
	}
	
	/* Update UI */
	while (gtk_events_pending ())
		gtk_main_iteration ();

	/* Load files */
	if (!gedit_file_open_uri_list (data->file_list, data->line_pos, TRUE) && !res)
		/* If no file is opened then create a new empty untitled document */
		gedit_file_new ();

	g_list_free (data->file_list);
	g_free (data);
}


static CommandLineData *
gedit_get_command_line_data (GnomeProgram *program)
{
	GValue value = { 0, };
	poptContext ctx;
	char **args;
	CommandLineData *data = NULL;
	int i;

	g_value_init (&value, G_TYPE_POINTER);
	g_object_get_property (G_OBJECT (program), GNOME_PARAM_POPT_CONTEXT, &value);
	ctx = g_value_get_pointer (&value);
	g_value_unset (&value);

	args = (char**) poptGetArgs(ctx);


	if (args) 
	{	
		data = g_new0 (CommandLineData, 1);
		for (i = 0; args[i]; i++) 
		{
			if (*args[i] == '+') 
				data->line_pos = atoi (args[i] + 1);		
			else
				data->file_list = g_list_append (data->file_list, 
								 eel_make_uri_from_shell_arg (args[i]));
		}
	}
	
	return data;
}



static void
gedit_handle_automation_cmdline (GnomeProgram *program)
{
        CORBA_Environment env;
        GNOME_Gedit_Application server;
	GNOME_Gedit_Window window;
	GNOME_Gedit_Document document;
	CommandLineData *data;
	GNOME_Gedit_URIList *uri_list;
	GList *list;
	gchar *stdin_data;
	int i;
	
        CORBA_exception_init (&env);

        server = bonobo_activation_activate_from_id ("OAFIID:GNOME_Gedit_Application",
                                                     0, NULL, &env);
	g_return_if_fail (server != NULL);


	if (quit_option)
		GNOME_Gedit_Application_quit (server, &env);
		
	
	if (new_window_option) 
		GNOME_Gedit_Application_newWindow (server, &env);

	if (new_document_option) 
	{
		window = GNOME_Gedit_Application_getActiveWindow (server, &env);
		GNOME_Gedit_Window_newDocument (window, &env);
	}

	data = gedit_get_command_line_data (program);

	if (data) 
	{
		window = GNOME_Gedit_Application_getActiveWindow (server, &env);

		/* convert the GList of files into a CORBA sequence */

		uri_list = GNOME_Gedit_URIList__alloc ();
		uri_list->_maximum = g_list_length (data->file_list);
		uri_list->_length = uri_list->_maximum;
		uri_list->_buffer = CORBA_sequence_GNOME_Gedit_URI_allocbuf (uri_list->_length);

		list = data->file_list;
		i=0;
		while (list != NULL) 
		{
			uri_list->_buffer[i] = CORBA_string_dup ((gchar*)list->data);
			list = list->next;
			i++;
		}

		CORBA_sequence_set_release (uri_list, CORBA_TRUE);
		GNOME_Gedit_Window_openURIList (window, uri_list, &env);

		document = GNOME_Gedit_Application_getActiveDocument (server, &env);

		GNOME_Gedit_Document_setLinePosition (document, data->line_pos, &env);

		g_list_foreach (data->file_list, (GFunc)g_free, NULL);
		g_list_free (data->file_list);
		g_free (data);
	}


	stdin_data = gedit_utils_get_stdin ();

	if (stdin_data != NULL && strlen (stdin_data) > 0)
	{
		GError *conv_error = NULL;
		const GeditEncoding *encoding;
		gchar *converted_text;

		if (gedit_encoding_get_current () != gedit_encoding_get_utf8 ())
		{
			if (g_utf8_validate (stdin_data, -1, NULL))
			{
				converted_text = stdin_data;
				stdin_data = NULL;

				encoding = gedit_encoding_get_utf8 ();
			}
			else
			{
				converted_text = NULL;
			}
		}
		else
		{	
			encoding = gedit_encoding_get_current ();
				
			converted_text = gedit_convert_to_utf8 (stdin_data,
								-1,
								&encoding,
								&conv_error);
		}	
		
		if (converted_text != NULL)
		{
			window = GNOME_Gedit_Application_getActiveWindow (server, &env);
			document = GNOME_Gedit_Window_newDocument (window, &env);

			GNOME_Gedit_Document_insert (document, 0, converted_text,
						     strlen (converted_text), &env);

			g_free (converted_text);
		}
	}

	if (stdin_data != NULL)
		g_free (stdin_data);

	if (!quit_option)
	{
		window = GNOME_Gedit_Application_getActiveWindow (server, &env);

		/* at the very least, we focus the active window */
		GNOME_Gedit_Window_grabFocus (window, &env);
	}

	bonobo_object_release_unref (server, &env);
        CORBA_exception_free (&env);

	/* we never popup a window, so tell startup-notification that
	 * we're done */
	gdk_notify_startup_complete ();
}

int
main (int argc, char **argv)
{
    	GnomeProgram *program;
	gboolean restored = FALSE;
	CORBA_Object factory;

	bindtextdomain (GETTEXT_PACKAGE, GEDIT_LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	/* Initialize gnome program */
	program = gnome_program_init ("gedit", VERSION,
			    LIBGNOMEUI_MODULE, argc, argv,
			    GNOME_PARAM_POPT_TABLE, options,			    
			    GNOME_PARAM_HUMAN_READABLE_NAME,
		            _("Text Editor"),
			    GNOME_PARAM_APP_DATADIR, DATADIR,
			    NULL);

	/* check whether we are running already */
        factory = bonobo_activation_activate_from_id
                ("OAFIID:GNOME_Gedit_Factory",
                 Bonobo_ACTIVATION_FLAG_EXISTING_ONLY,
                 NULL, NULL);

        if (factory != NULL) 
	{
		/* there is an instance already running, so send
		 * commands to it if needed
		 */
                gedit_handle_automation_cmdline (program);

                /* and we're done */
                exit (0);
        }

	/* Set default icon */
	gedit_set_default_icon ();
	
	/* Load user preferences */
	gedit_prefs_manager_app_init ();
	gedit_recent_init ();
	gedit_metadata_manager_init ();

	/* Init plugins engine */
	gedit_plugins_engine_init ();

	/* Initialize session management */
	gedit_session_init (argv[0]);

	/* Create our MDI object */
	gedit_mdi = gedit_mdi_new ();

	if (gedit_session_is_restored ())
		restored = gedit_session_load ();

	if (!restored) 
	{
		CommandLineData *data;

		data = gedit_get_command_line_data (program);
		
		gtk_init_add ((GtkFunction)gedit_load_file_list, (gpointer)data);

		/* Open the first top level window */
		bonobo_mdi_open_toplevel (BONOBO_MDI (gedit_mdi), NULL);
	}

	gedit_app_server = gedit_application_server_new (gdk_screen_get_default ());
	
	gtk_main();
	
	return 0;
}


BonoboWindow*
gedit_get_active_window (void)
{
	g_return_val_if_fail (gedit_mdi != NULL, NULL);

	return	bonobo_mdi_get_active_window (BONOBO_MDI (gedit_mdi));
}

GeditDocument*
gedit_get_active_document (void)
{
	BonoboMDIChild *active_child;

	g_return_val_if_fail (gedit_mdi != NULL, NULL);

	active_child = bonobo_mdi_get_active_child (BONOBO_MDI (gedit_mdi));

	if (active_child == NULL)
		return NULL;

	return GEDIT_MDI_CHILD (active_child)->document;
}

GeditView*
gedit_get_active_view (void)
{
	GtkWidget *active_view;

	g_return_val_if_fail (gedit_mdi != NULL, NULL);

	active_view = bonobo_mdi_get_active_view (BONOBO_MDI (gedit_mdi));
	
	if (active_view == NULL)
		return NULL;

	return GEDIT_VIEW (active_view);
}

GList* 
gedit_get_top_windows (void)
{
	g_return_val_if_fail (gedit_mdi != NULL, NULL);

	return	bonobo_mdi_get_windows (BONOBO_MDI (gedit_mdi));
}

BonoboUIComponent*
gedit_get_ui_component_from_window (BonoboWindow* win)
{
	g_return_val_if_fail (win != NULL, NULL);

	return bonobo_mdi_get_ui_component_from_window (win);
}

/* Return a newly allocated list */
GList*
gedit_get_open_documents (void)
{
	GList* children;
	GList* docs = NULL;
	g_return_val_if_fail (gedit_mdi != NULL, NULL);

	children = bonobo_mdi_get_children (BONOBO_MDI (gedit_mdi));

	while (children != NULL)
	{
		GeditMDIChild *child;

		child = GEDIT_MDI_CHILD (children->data);
		
		docs = g_list_append (docs, child->document);
		children = g_list_next (children);
	}
	
	return docs;
}
