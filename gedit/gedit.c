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
#ifndef WITHOUT_GNOME
#include <config.h>
#include <gnome.h>
#endif
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <gtk/gtk.h>

#include "main.h"
#include "gE_prefs.h"
#include "menus.h"
#define PLUGIN_TEST 1
#if PLUGIN_TEST
#include "plugin.h"
#include "gE_plugin_api.h"
#endif

GList *window_list;
extern GList *plugins;
char home[STRING_LENGTH_MAX];
char *homea;
char rc[STRING_LENGTH_MAX];
char fname[STRING_LENGTH_MAX];

void destroy_window (GtkWidget *widget, GdkEvent *event, gE_data *data)
{
  file_close_window_cmd_callback (widget, data);

}


void gE_quit ()
{
	gtk_exit (0);
}	




#ifdef WITHOUT_GNOME
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

	/*
	 * gdk is so braindead in that it catches these "fatal" signals,
	 * but does absolutely nothing with them unless you have compiled
	 * it with a debugging flag (e.g., ./configure --enable-debug=yes).
	 * we'll simply re-establish the signals to their default behavior.
	 * this should produce a core dump in the normal case, which is a
	 * lot more useful than what gdk does.
	 *
	 * if you've compiled GDK with debugging or simply don't care,
	 * then you don't need this.
	 */
#ifdef GDK_IS_BRAINDEAD
	signal(SIGHUP, SIG_DFL);
	signal(SIGINT, SIG_DFL);
	signal(SIGQUIT, SIG_DFL);
	signal(SIGBUS, SIG_DFL);
	signal(SIGSEGV, SIG_DFL);
	signal(SIGPIPE, SIG_DFL);
	signal(SIGTERM, SIG_DFL);
#endif

	window_list = NULL;
	gE_get_rc_file();
	gE_rc_parse();

	prog_init(argv + 1);
	
	gtk_main ();

	return 0;
}
#else
static struct argp_option argp_options [] = {
	{ NULL, 0, NULL, 0, NULL, 0 },
};

static GList *file_list;

static error_t
parse_an_arg (int key, char *arg, struct argp_state *state)
{
	
	if (key == ARGP_KEY_ARG)
		file_list = g_list_append (file_list, arg);
	return 0;
}

static struct argp parser =
{
	NULL, parse_an_arg, NULL, NULL, NULL, NULL, NULL
};

int main (int argc, char **argv)
{

	gE_window *window;
	gE_data *data;
	plugin_callback_struct callbacks;
	argp_program_version = VERSION;
	bindtextdomain(PACKAGE, GNOMELOCALEDIR);
	textdomain(PACKAGE);

	gnome_init ("gEdit", &parser, argc, argv, 0, NULL);

	gE_rc_parse();

	data = g_malloc0 (sizeof (gE_data));
	window_list = NULL;
	window = gE_window_new();

	data->window = window;
	window->show_status = 1;
	gE_get_settings(window);

	g_print("...\n");
	if (file_list){
		gE_document *doc;

		doc = gE_document_current(window);
		gtk_notebook_remove_page(GTK_NOTEBOOK(window->notebook),
					 gtk_notebook_current_page (GTK_NOTEBOOK(window->notebook)));
		g_list_remove(window->documents, doc);
		if (doc->filename != NULL)
			g_free (doc->filename);
		g_free (doc);

		for (;file_list; file_list = file_list->next)
		{
			data->temp2 = file_list->data;
			gtk_idle_add ((GtkFunction) file_open_wrapper, data);
		}
	}

	/* Init plugins... */
	plugins = NULL;
	
	callbacks.document.create = gE_plugin_document_create;
	callbacks.text.append = gE_plugin_text_append;
	callbacks.document.show = gE_plugin_document_show;
	callbacks.document.current = gE_plugin_document_current;
	callbacks.document.filename = gE_plugin_document_filename;
	callbacks.text.get = gE_plugin_text_get;
	callbacks.program.quit = NULL;
	callbacks.program.reg = gE_plugin_program_register;
	
	plugin_query_all (&callbacks);
	
	gtk_main ();
	return 0;
}

#endif

