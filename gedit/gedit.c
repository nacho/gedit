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
#ifndef WITHOUT_GNOME
#include <gnome.h>
#endif
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <gtk/gtk.h>

#include "main.h"
#include "commands.h"
#include "gE_document.h"
#include "gE_prefs.h"
#include "gE_plugin_api.h"
#include "msgbox.h"
#include "gE_files.h"

#ifdef WITH_GMODULE_PLUGINS
#include <gE_plugin.h>
#endif

#ifdef HAVE_LIBGNORBA
#include <libgnorba/gnorba.h>
#endif

GList *window_list;
extern GList *plugins;

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

gint file_open_wrapper (gE_data *data)
{
	char *nfile, *name;

	name = data->temp2;
	gE_document_new(data->window);
	nfile = g_malloc(strlen(name)+1);
	strcpy(nfile, name);
	(gint) gE_file_open (data->window, gE_document_current(data->window), nfile);

	return FALSE;
}

#ifdef WITHOUT_GNOME 

/* 
 *  GTK-ONLY CODE 
 *  STARTS HERE
 */

static void gE_show_version(void);

void prog_init(char **file)
{
	gE_window *window;
	gE_document *doc;
	gE_data *data;
	plugin_callback_struct callbacks;
	data = g_malloc0 (sizeof (gE_data));
#ifdef DEBUG
	g_print("Initialising gEdit...\n");

	g_print("%s\n",*file);
#endif

	gE_prefs_open();
	msgbox_create();

#ifdef GTK_HAVE_FEATURES_1_1_0	
	doc_pointer_to_int = g_hash_table_new (g_direct_hash, g_direct_equal);
	doc_int_to_pointer = g_hash_table_new (g_int_hash, g_int_equal);
	win_pointer_to_int = g_hash_table_new (g_direct_hash, g_direct_equal);
	win_int_to_pointer = g_hash_table_new (g_int_hash, g_int_equal);
#endif

	window_list = NULL;
	window = gE_window_new();
	data->window = window;
	if (*file != NULL) {
		doc = gE_document_current(window);
		gtk_notebook_remove_page(GTK_NOTEBOOK(window->notebook),
			gtk_notebook_current_page (GTK_NOTEBOOK(window->notebook)));
		window->documents = g_list_remove(window->documents, doc);
		if (doc->filename != NULL)
			g_free (doc->filename);
		g_free (doc);

		while (*file) {
			data->temp2 = *file;
			file_open_wrapper (data);
			file++;
		}
	}

	/* Init plugins... */
	plugins = NULL;
	
	setup_callbacks (&callbacks);
	
	plugin_query_all (&callbacks);
	
#ifdef WITH_GMODULE_PLUGINS
	gE_Plugin_Query_All ();
#endif
	
}

int
main (int argc, char **argv)
{
	int x;

	/* gtk_set_locale (); */

	gtk_init (&argc, &argv);

	for (x = 1; x < argc; x++)
	{
		if (strcmp (argv[x], "--help") == 0)
		{
			g_print("gEdit\nUsage: gedit [--help] [--version] [file...]\n");

			exit(0);
		}

		else if (strcmp (argv[x], "--version") == 0)
		{
			gE_show_version();
			exit(0);
		}
	}

	gE_rc_parse();

	prog_init(argv + 1);
	
	gtk_main ();

	gE_prefs_close ();
	return 0;

} /* main */


static void
gE_show_version(void)
{
    g_print ("%s\n", GEDIT_ID);
}


#else


/* 
 *  GTK-ONLY CODE ENDS 
 *  GNOME CODE BEGINS
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
	
	/*gE_rc_parse();
*/
	doc_pointer_to_int = g_hash_table_new (g_direct_hash, g_direct_equal);
	doc_int_to_pointer = g_hash_table_new (g_int_hash, g_int_equal);
	win_pointer_to_int = g_hash_table_new (g_direct_hash, g_direct_equal);
	win_int_to_pointer = g_hash_table_new (g_int_hash, g_int_equal);

	msgbox_create ();
	data = g_malloc (sizeof (gE_data));
	window_list = NULL;
	window = gE_window_new();

	data->window = window;

	#ifdef DEBUG
	  g_print("...\n");
	#endif
	
	if (file_list){
		gE_document *doc;

		doc = gE_document_current(window);
		gtk_notebook_remove_page(GTK_NOTEBOOK(window->notebook),
					 gtk_notebook_current_page (GTK_NOTEBOOK(window->notebook)));
		window->documents = g_list_remove(window->documents, doc);
		if (doc->filename != NULL)
			g_free (doc->filename);
		g_free (doc);

		for (;file_list; file_list = file_list->next)
		{
			data->temp2 = file_list->data;
			file_open_wrapper (data);
		}
	}

	g_free (data);
	/* Init plugins... */
	plugins = NULL;
	
	setup_callbacks (&callbacks);
	
	plugin_query_all (&callbacks);
      
#ifdef WITH_GMODULE_PLUGINS
	gE_Plugin_Query_All ();
#endif
	
	gtk_main ();
	return 0;
}

#endif	/* #ifdef WITHOUT_GNOME */

