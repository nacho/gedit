/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit
 *
 * Copyright (C) 1998, 1999 Alex Roberts, Evan Lawrence
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
#include <glade/glade.h>

#include "commands.h"
#include "document.h"
#include "prefs.h"
#include "file.h"
#include "menus.h"
#include "plugin.h"
#include "recent.h"
#include "utils.h"

#ifdef HAVE_LIBGNORBA
#include <libgnorba/gnorba.h>
#endif

gint debug = 0;
gint debug_view = 0;
gint debug_undo = 0;
gint debug_search = 0;
gint debug_prefs = 0;
gint debug_print = 0;
gint debug_plugins = 0;
gint debug_file = 0;
gint debug_document = 0;
gint debug_commands = 0;
gint debug_recent = 0;

static const struct poptOption options[] =
{
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

#ifdef HAVE_LIBGNORBA

CORBA_ORB global_orb;
PortableServer_POA root_poa;
PortableServer_POAManager root_poa_manager;
CORBA_Environment *global_ev;
CORBA_Object name_service;

void
corba_exception (CORBA_Environment* ev)
{
	switch (ev->_major)
	{
	case CORBA_SYSTEM_EXCEPTION:
		g_log ("gedit CORBA", G_LOG_LEVEL_DEBUG,
		       "CORBA system exception %s.\n",
		       CORBA_exception_id (ev));
		break;
	case CORBA_USER_EXCEPTION:
		g_log ("gedit CORBA", G_LOG_LEVEL_DEBUG,
		       "CORBA user exception: %s.\n",
		       CORBA_exception_id (ev));
		break;
	default:
		break;
	}
}

#endif /* HAVE_LIBGNORBA */

void gedit_shutdown (void);

void
gedit_shutdown (void)
{
	gedit_save_settings ();
	history_write_config ();

	gtk_main_quit ();
}

int
main (int argc, char **argv)
{
	char **args;
	poptContext ctx;
	int i;
	
	GList *file_list = NULL;
	Document *doc;

	/* Initialize i18n */
	bindtextdomain(PACKAGE, GNOMELOCALEDIR);
	textdomain(PACKAGE);

#ifdef HAVE_LIBGNORBA
	global_ev = g_new0 (CORBA_Environment, 1);
	CORBA_exception_init (global_ev);
	global_orb = gnome_CORBA_init ("gedit", VERSION, &argc, argv,
				       options, 0, &ctx, global_ev);
	corba_exception (global_ev);
	root_poa = CORBA_ORB_resolve_initial_references
		(global_orb, "RootPOA", global_ev);
	corba_exception (global_ev);
	root_poa_manager = PortableServer_POA__get_the_POAManager
		(root_poa, global_ev);
	corba_exception (global_ev);
	PortableServer_POAManager_activate (root_poa_manager, global_ev);
	corba_exception (global_ev);
	name_service = gnome_name_service_get ();
#else
	gnome_init_with_popt_table ("gedit", VERSION, argc, argv, options, 0, &ctx);
#endif /* HAVE_LIBGNORBA */

	args = (char**) poptGetArgs(ctx);

	for (i = 0; args && args[i]; i++)
		file_list = g_list_append (file_list, args[i]);
	
	poptFreeContext (ctx);

	glade_gnome_init ();

	gedit_load_settings ();
	gedit_plugins_init ();
	gedit_mdi_init ();

	gnome_mdi_open_toplevel (mdi);

	
	if (file_list)
	{
		
                for (;file_list; file_list = file_list->next)
		{
			if (g_file_exists (file_list->data))
			{
				doc = gedit_document_new_with_file (file_list->data);
				if (doc != NULL)
				{
					gnome_mdi_add_child (mdi, GNOME_MDI_CHILD (doc));
					gnome_mdi_add_view (mdi, GNOME_MDI_CHILD (doc));
				}
			}
			else
			{
				popup_create_new_file (NULL, file_list->data);
			}
		}
         
	}

	doc = gedit_document_stdin ();
	if (doc != NULL)
	{
		gnome_mdi_add_child (mdi, GNOME_MDI_CHILD (doc));
		gnome_mdi_add_view (mdi, GNOME_MDI_CHILD (doc));
	}

	if (gedit_document_current() == NULL)
	{
		doc = gedit_document_new ();
		
		gnome_mdi_add_child (mdi, GNOME_MDI_CHILD (doc));
		gnome_mdi_add_view (mdi, GNOME_MDI_CHILD (doc));
        }

	gtk_main();

	return 0;
}
