/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* gEdit
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

#include "main.h"
#include "commands.h"
#include "gE_mdi.h"
#include "gE_prefs.h"
#include "gedit-file-io.h"
#include "gedit-menus.h"
#include "gE_plugin.h"
#include "gE_window.h"

#ifdef HAVE_LIBGNORBA
#include <libgnorba/gnorba.h>
#endif

GList *window_list;
GnomeMDI *mdi;
gedit_window *window;
gedit_preference *settings;

gint mdiMode = GNOME_MDI_DEFAULT_MODE;
/*gint mdiMode = GNOME_MDI_NOTEBOOK;*/
gboolean use_fontset = FALSE;



static const struct poptOption options[] = {

    {NULL, '\0', 0, NULL, 0}
};

static GList *file_list;

#ifdef HAVE_LIBGNORBA

CORBA_ORB global_orb;
PortableServer_POA root_poa;
PortableServer_POAManager root_poa_manager;
CORBA_Environment *global_ev;
CORBA_Object name_service;

void
corba_exception (CORBA_Environment* ev)
{
	switch (ev->_major) {
	case CORBA_SYSTEM_EXCEPTION:
		g_log ("GEdit CORBA", G_LOG_LEVEL_DEBUG,
		       "CORBA system exception %s.\n",
		       CORBA_exception_id (ev));
		break;
	case CORBA_USER_EXCEPTION:
		g_log ("GEdit CORBA", G_LOG_LEVEL_DEBUG,
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
	gedit_document *doc;
/*
	gedit_window *window;
	gedit_data *data;
*/
	char **args;
	poptContext ctx;
	int i;
	GtkWidget *dummy_widget;

	bindtextdomain(PACKAGE, GNOMELOCALEDIR);
	textdomain(PACKAGE);


#ifdef HAVE_LIBGNORBA

	global_ev = g_new0 (CORBA_Environment, 1);

	CORBA_exception_init (global_ev);
	global_orb = gnome_CORBA_init ("gEdit", VERSION, &argc, argv,
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
	gnome_init_with_popt_table ("gEdit", VERSION, argc, argv, options, 0, &ctx);
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
	
	poptFreeContext(ctx);
	
	
	/*data = g_malloc (sizeof (gedit_data));*/
	window_list = NULL;
	settings = g_malloc (sizeof (gedit_preference));
	
	settings->num_recent = 0;

	mdi = GNOME_MDI(gnome_mdi_new ("gEdit", GEDIT_ID));
	mdi->tab_pos = GTK_POS_TOP;
	
	gnome_mdi_set_menubar_template (mdi, gedit_menu);
	gnome_mdi_set_toolbar_template (mdi, toolbar_data);
	
	gnome_mdi_set_child_menu_path (mdi, GNOME_MENU_FILE_STRING);
	gnome_mdi_set_child_list_path (mdi, GNOME_MENU_FILES_PATH);
	


	/* Init plugins... */
	gedit_plugins_init ();
	
	/* new plugins system init will be here */
	/* connect signals -- FIXME -- We'll do the rest later */
	gtk_signal_connect (GTK_OBJECT(mdi), "remove_child",
			    GTK_SIGNAL_FUNC (remove_doc_cb), NULL);
	gtk_signal_connect (GTK_OBJECT(mdi), "destroy",
			    GTK_SIGNAL_FUNC (file_quit_cb), NULL);
/*	gtk_signal_connect(GTK_OBJECT(mdi), "view_changed", GTK_SIGNAL_FUNC(mdi_view_changed_cb), NULL);*/
	gtk_signal_connect(GTK_OBJECT(mdi), "child_changed",
			   GTK_SIGNAL_FUNC(child_switch), NULL);
        gtk_signal_connect(GTK_OBJECT(mdi), "app_created",
			   GTK_SIGNAL_FUNC(gedit_window_new), NULL);
/*	gtk_signal_connect(GTK_OBJECT(mdi), "add_view", GTK_SIGNAL_FUNC(add_view_cb), NULL);*/
/*	gtk_signal_connect(GTK_OBJECT(mdi), "add_child", GTK_SIGNAL_FUNC(add_child_cb), NULL);*/
	gedit_get_settings();
	gnome_mdi_set_mode (mdi, mdiMode);	
/*	gnome_mdi_set_mode (mdi, GNOME_MDI_NOTEBOOK);	*/
	gnome_mdi_open_toplevel(mdi);

	doc = gedit_document_new ();
	gnome_mdi_add_child (mdi, GNOME_MDI_CHILD (doc));
	gnome_mdi_add_view  (mdi, GNOME_MDI_CHILD (doc));

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
		if (g_list_length(mdi->children) == 0 && FALSE )
		{
			doc = gedit_document_new ();
			gnome_mdi_add_child (mdi, GNOME_MDI_CHILD (doc));
			gnome_mdi_add_view  (mdi, GNOME_MDI_CHILD (doc));
		}
	}
	
	/*g_free (data);*/
	
	gtk_main ();
	return 0;
}




