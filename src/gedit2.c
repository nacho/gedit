/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit2.c
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
#include <libgnomeui/gnome-window-icon.h>

#include "gedit2.h"
#include "gedit-mdi.h"
#include "gedit-prefs.h"
#include "gedit-debug.h"
#include "gedit-file.h"

#ifndef GNOME_ICONDIR
#define GNOME_ICONDIR "" 
#endif

GeditMDI *gedit_mdi = NULL; 

static void gedit_set_default_icon ();
static void gedit_load_file_list (GList *file_list);

static const struct poptOption options [] =
{
	{ "debug-mdi", '\0', 0, &debug_mdi, 0,
	  N_("Show mdi debugging messages."), NULL },

	{ "debug-commands", '\0', 0, &debug_commands, 0,
	  N_("Show commands debugging messages."), NULL },

	{ "debug-document", '\0', 0, &debug_document, 0,
	  N_("Show document debugging messages."), NULL },

	{ "debug-file", '\0', 0, &debug_file, 0,
	  N_("Show file debugging messages."), NULL },

	{ "debug-plugins", '\0', 0, &debug_plugins, 0,
	  N_("Show plugin debugging messages."), NULL },

	{ "debug-prefs", '\0', 0, &debug_prefs, 0,
	  N_("Show prefs debugging messages."), NULL },

	{ "debug-print", '\0', 0, &debug_print, 0,
	  N_("Show printing debugging messages."), NULL },

	{ "debug-search", '\0', 0, &debug_search, 0,
	  N_("Show search debugging messages."), NULL },

	{ "debug-undo", '\0', 0, &debug_undo, 0,
	  N_("Show undo debugging messages."), NULL },

	{ "debug-view", '\0', 0, &debug_view, 0,
	  N_("Show view debugging messages."), NULL },

	{ "debug-recent", '\0', 0, &debug_recent, 0,
	  N_("Show recent debugging messages."), NULL },

	{ "debug", '\0', 0, &debug, 0,
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
gedit_load_file_list (GList *file_list)
{
	/* Load files */
	if (!gedit_file_open_uri_list (file_list))
		/* If no file is opened that create a new empty untitled document */
		gedit_file_new ();

	g_list_free (file_list);
}

int
main (int argc, char **argv)
{
	GValue value = { 0, };
    	GnomeProgram *program;
	poptContext ctx;
	char **args;

	GList *file_list = NULL;
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
	gedit_prefs_load_settings ();

	/* Parse args and build the list of files to be loaded at startup */
	g_value_init (&value, G_TYPE_POINTER);
    	g_object_get_property (G_OBJECT (program), GNOME_PARAM_POPT_CONTEXT, &value);
    	ctx = g_value_get_pointer (&value);
    	g_value_unset (&value);

	args = (char**) poptGetArgs(ctx);
	
	for (i = 0; args && args[i]; i++)
		file_list = g_list_append (file_list, args[i]);

	/* Create gedit_mdi and open the first top level window */
	gedit_mdi = gedit_mdi_new ();
	bonobo_mdi_open_toplevel (BONOBO_MDI (gedit_mdi)); 

	gtk_init_add ((GtkFunction)gedit_load_file_list, (gpointer)file_list);

	gtk_main();
	
	return 0;
}
