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

#include <unistd.h>
#include <config.h>
#include <gnome.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <gtk/gtk.h>

#include "main.h"
#include "commands.h"
#include "gE_document.h"
#include "gE_mdi.h"
#include "gE_prefs.h"
#include "gE_plugin_api.h"
#include "gE_files.h"
#include "menus.h"
#include "toolbar.h"

#ifdef WITH_GMODULE_PLUGINS
#include <gE_plugin.h>
#endif

#ifdef HAVE_LIBGNORBA
#include <libgnorba/gnorba.h>
#endif

GList *window_list;
extern GList *plugins;
plugin_callback_struct pl_callbacks;
GnomeMDI *mdi;
gE_window *window;
gE_preference *settings;

gint mdiMode = GNOME_MDI_DEFAULT_MODE;

void setup_callbacks( plugin_callback_struct *callbacks )
{
	callbacks->document.create = gE_plugin_document_create;
	callbacks->text.append = gE_plugin_text_append;
	callbacks->text.insert = gE_plugin_text_insert;
	callbacks->document.show = gE_plugin_document_show;
	callbacks->document.current = gE_plugin_document_current;
	callbacks->document.filename = gE_plugin_document_filename;
	callbacks->text.get = gE_plugin_text_get;
	callbacks->program.quit = gE_plugin_program_quit;
	callbacks->program.reg = gE_plugin_program_register;

	callbacks->document.open = NULL;
	callbacks->document.close = NULL;
	callbacks->text.get_selected_text = NULL;
	callbacks->text.set_selected_text = NULL;
	callbacks->document.get_position = NULL;
	callbacks->document.get_selection = NULL;
}
/* We dont need this!
gint file_open_wrapper (char *fname)
{
	char *nfile;
	gE_document *doc;

	doc = gE_document_new();
	nfile = g_malloc(strlen(fname)+1);
	strcpy(nfile, name);
	(gint) gE_file_open (doc, nfile);

	return FALSE;
}
*/


GSList *launch_plugins = NULL;

static void
add_launch_plugin(poptContext ctx,
		  enum poptCallbackReason reason,
		  const struct poptOption *opt,
		  char *arg, void *data)
{
  if(opt->shortName == 'p') {
    launch_plugins = g_slist_append(launch_plugins, arg);
  } /* else something's weird :) */
}

static void
launch_plugin(char *name, gpointer userdata)
{
  GString *fullname;
  plugin_callback_struct callbacks;
  plugin *plug;

  fullname = g_string_new(NULL);
  g_string_sprintf( fullname, "%s/%s%s", PLUGINDIR, name, "-plugin" );
	      
  plug = plugin_new( fullname->str );

  setup_callbacks( &callbacks );
	      
  plugin_register( plug, &callbacks, 0 );

  g_string_free( fullname, TRUE );
}

static const struct poptOption options[] = {
      /* We want to use a callback for this launch-plugin thing so we
         can get multiple opts */
    {NULL, '\0', POPT_ARG_CALLBACK, &add_launch_plugin, 0 },
    {"launch-plugin", 'p', POPT_ARG_STRING, NULL, 0, N_("Launch a plugin at startup"), N_("PLUGIN-NAME")},
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

int main (int argc, char **argv)
{
	gE_document *doc;
	gE_window *window;
	gE_data *data;
	plugin_callback_struct callbacks;
	char **args;
	poptContext ctx;
	int i;

	bindtextdomain(PACKAGE, GNOMELOCALEDIR);
	textdomain(PACKAGE);


#ifdef HAVE_LIBGNORBA

	global_ev = g_new0 (CORBA_Environment, 1);

	CORBA_exception_init (global_ev);
	global_orb = gnome_CORBA_init
		("gEdit", VERSION, &argc, argv, options, 0, &ctx, global_ev);
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

	g_slist_foreach(launch_plugins, launch_plugin, NULL);
	g_slist_free(launch_plugins);

	args = poptGetArgs(ctx);

	for(i = 0; args && args[i]; i++) {
	  file_list = g_list_append (file_list, args[i]);
	}
	
	poptFreeContext(ctx);
	
/*	gE_rc_parse(); -- THis really isnt needed any more.. but i forget the
				  function itself.. */

	doc_pointer_to_int = g_hash_table_new (g_direct_hash, g_direct_equal);
	doc_int_to_pointer = g_hash_table_new (g_int_hash, g_int_equal);
	win_pointer_to_int = g_hash_table_new (g_direct_hash, g_direct_equal);
	win_int_to_pointer = g_hash_table_new (g_int_hash, g_int_equal);
	
	
	data = g_malloc (sizeof (gE_data));
	window_list = NULL;
	settings = g_malloc (sizeof (gE_preference));
	
	settings->num_recent = 0;

	mdi = GNOME_MDI(gnome_mdi_new ("gEdit", GEDIT_ID));
	mdi->tab_pos = GTK_POS_TOP;
	
	gnome_mdi_set_menubar_template (mdi, gedit_menu);
	gnome_mdi_set_toolbar_template (mdi, toolbar_data);
	
	gnome_mdi_set_child_menu_path (mdi, GNOME_MENU_FILE_STRING);
	gnome_mdi_set_child_list_path (mdi, GNOME_MENU_FILES_PATH);
	
	gnome_mdi_set_mode (mdi, mdiMode);	
	/*window = gE_window_new();

	data->window = window;*/
	window = g_malloc (sizeof (gE_window));

	/* Init plugins... */
	plugins = NULL;
	
	setup_callbacks (&pl_callbacks);
	
	/*plugin_query_all (&pl_callbacks);*/
	/*custom_plugin_query_all ( "/usr/gnome/libexec/plugins", &pl_callbacks);*/
	/*custom_plugin_query ( "/usr/gnome/libexec", "print-plugin", &pl_callbacks);*/
	plugin_load_list("gEdit");

		
    /* connect signals -- FIXME -- We'll do the rest later */
    gtk_signal_connect(GTK_OBJECT(mdi), "remove_child", GTK_SIGNAL_FUNC(remove_doc_cb), NULL);
    gtk_signal_connect(GTK_OBJECT(mdi), "destroy", GTK_SIGNAL_FUNC(file_quit_cb), NULL);
    gtk_signal_connect(GTK_OBJECT(mdi), "view_changed", GTK_SIGNAL_FUNC(view_changed_cb), NULL);
    gtk_signal_connect(GTK_OBJECT(mdi), "child_changed", GTK_SIGNAL_FUNC(child_switch), NULL);
    gtk_signal_connect(GTK_OBJECT(mdi), "app_created", GTK_SIGNAL_FUNC(gE_window_new), NULL);
    gtk_signal_connect(GTK_OBJECT(mdi), "add_view", GTK_SIGNAL_FUNC(add_view_cb), NULL);
    gtk_signal_connect(GTK_OBJECT(mdi), "add_child", GTK_SIGNAL_FUNC(add_child_cb), NULL);		
	gnome_mdi_open_toplevel(mdi);


	if (file_list){


/*		doc = gE_document_current();

		if (doc->filename != NULL)
			g_free (doc->filename);
		g_free (doc);
*/
		for (;file_list; file_list = file_list->next)
		{
			/*data->temp2 = file_list->data;*/
			/*file_open_wrapper (file_list->data);*/
			gE_document_new_with_file (file_list->data);
		}
	}
	else
	{
	  if ((doc = gE_document_new()))
	    {
	      gnome_mdi_add_child (mdi, GNOME_MDI_CHILD (doc));
	      gnome_mdi_add_view (mdi, GNOME_MDI_CHILD (doc));
	    }
	}
	
	g_free (data);

	
      
#ifdef WITH_GMODULE_PLUGINS
	gE_Plugin_Query_All ();
#endif
	

	
	gtk_main ();
	return 0;
}


