/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* gedit
 * Copyright (C) 1998 Alex Roberts and Evan Lawrence
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
#include <glade/glade.h>

#include "window.h"
#include "gedit.h"
#include "commands.h"
#include "document.h"
#include "prefs.h"
#include "file.h"
#include "menus.h"
#include "plugin.h"


#ifdef HAVE_LIBGNORBA
#include <libgnorba/gnorba.h>
#endif

static const struct poptOption options[] =
{
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

int
main (int argc, char **argv)
{
	char **args;
	poptContext ctx;
	int i;
	GtkWidget *dummy_widget;
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

	/* Determine we use fonts or fontsets. If a fontset is supplied
	   for text widgets, we use fontsets for drawing texts. Otherwise
	   we use normal fonts instead. */
	dummy_widget = gtk_text_new (NULL, NULL);
	gtk_widget_ensure_style (dummy_widget);
	if (dummy_widget->style->font->type == GDK_FONT_FONTSET)
		use_fontset = TRUE;
	gtk_widget_destroy (dummy_widget);

	args = (char**) poptGetArgs(ctx);

	for (i = 0; args && args[i]; i++)
		file_list = g_list_append (file_list, args[i]);
	
	poptFreeContext (ctx);
	
	/*data = g_malloc (sizeof (gedit_data));*/

	/* Init plugins */
	gedit_plugins_init ();

	gedit_mdi_init ();
	
	gedit_load_settings ();

	gnome_mdi_open_toplevel (mdi);

	doc = gedit_document_new ();
	gnome_mdi_add_child (mdi, GNOME_MDI_CHILD (doc));
	gnome_mdi_add_view (mdi, GNOME_MDI_CHILD (doc));

	if (file_list)
	{
		if (mdi->active_child == NULL)
			return 1;
		gnome_mdi_remove_child (mdi, mdi->active_child, FALSE);
		for (;file_list; file_list = file_list->next)
		{
			if (g_file_exists (file_list->data))
			{
				doc = gedit_document_new_with_file (file_list->data);
				if (doc->buf != NULL)
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
                /* if there are no open documents create a blank one */
		if (g_list_length(mdi->children) == 0)
		{
			doc = gedit_document_new ();
			gnome_mdi_add_child (mdi, GNOME_MDI_CHILD (doc));
			gnome_mdi_add_view (mdi, GNOME_MDI_CHILD (doc));
		}
	}
	
	
	glade_gnome_init ();

	gtk_main ();
	return 0;
}
