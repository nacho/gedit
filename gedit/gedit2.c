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

#include <libgnome/libgnome.h>
#include <libgnomeui/libgnomeui.h>
#include <libgnomeui/gnome-window-icon.h>

#include "gedit2.h"
#include "gedit-mdi.h"
#include "gedit-prefs.h"
#include "gedit-debug.h"
#include "gedit-file.h"
#include "gedit-utils.h"

#include "gedit-plugins-engine.h"

#ifndef GNOME_ICONDIR
#define GNOME_ICONDIR "" 
#endif

GeditMDI *gedit_mdi = NULL;
gboolean gedit_close_x_button_pressed = FALSE;
gboolean gedit_exit_button_pressed = FALSE; 

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

	{ "debug", '\0', POPT_ARG_NONE, &debug, 0,
	  N_("Turn on all debugging messages."), NULL },

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
	/* Update UI */
	while (gtk_events_pending ())
		gtk_main_iteration ();

	/* Load files */
	if (!gedit_file_open_uri_list (data->file_list, data->line_pos) && 
	    !gedit_file_open_from_stdin (NULL))
		/* If no file is opened then create a new empty untitled document */
		gedit_file_new ();

	g_list_free (data->file_list);
	g_free (data);
}

int
main (int argc, char **argv)
{
	GValue value = { 0, };
    	GnomeProgram *program;
	poptContext ctx;
	char **args;

	CommandLineData *data = NULL;
	gint i;

#ifdef ENABLE_NLS
	setlocale (LC_ALL, "");
	bindtextdomain (GETTEXT_PACKAGE, GEDIT_LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
#endif

	/* Initialize gnome program */
	program = gnome_program_init ("gedit2", VERSION,
			    LIBGNOMEUI_MODULE, argc, argv,
			    GNOME_PARAM_POPT_TABLE, options,
			    GNOME_PARAM_HUMAN_READABLE_NAME,
		            _("The gnome text editor"),
			    NULL);

	/* Set default icon */
	gedit_set_default_icon ();
	
	/* Load user preferences */
	gedit_prefs_init ();
	gedit_prefs_load_settings ();

	/* Init plugins engine */
	gedit_plugins_engine_init ();

	/* Parse args and build the list of files to be loaded at startup */
	g_value_init (&value, G_TYPE_POINTER);
    	g_object_get_property (G_OBJECT (program), GNOME_PARAM_POPT_CONTEXT, &value);
    	ctx = g_value_get_pointer (&value);
    	g_value_unset (&value);

	args = (char**) poptGetArgs(ctx);
	
	data = g_new0 (CommandLineData, 1);

	if (args)	
		for (i = 0; args[i]; i++) 
		{
			if (strstr (args[i], "+")) 
				data->line_pos = atoi (args[i] + 1);		
			else
				data->file_list = g_list_append (data->file_list, args[i]);
		}

	/* Create gedit_mdi and open the first top level window */
	gedit_mdi = gedit_mdi_new ();
	bonobo_mdi_open_toplevel (BONOBO_MDI (gedit_mdi)); 
	
	gtk_init_add ((GtkFunction)gedit_load_file_list, (gpointer)data);

	gtk_main();
		
	return 0;
}


BonoboWindow*
gedit_get_active_window ()
{
	g_return_val_if_fail (gedit_mdi != NULL, NULL);

	return	bonobo_mdi_get_active_window (BONOBO_MDI (gedit_mdi));
}

GeditDocument*
gedit_get_active_document ()
{
	BonoboMDIChild *active_child;

	g_return_val_if_fail (gedit_mdi != NULL, NULL);

	active_child = bonobo_mdi_get_active_child (BONOBO_MDI (gedit_mdi));

	if (active_child == NULL)
		return NULL;

	return GEDIT_MDI_CHILD (active_child)->document;
}

GeditView*
gedit_get_active_view ()
{
	GtkWidget *active_view;

	g_return_val_if_fail (gedit_mdi != NULL, NULL);

	active_view = bonobo_mdi_get_active_view (BONOBO_MDI (gedit_mdi));
	
	if (active_view == NULL)
		return NULL;

	return GEDIT_VIEW (active_view);
}

GList* 
gedit_get_top_windows ()
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

	
