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
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <gtk/gtk.h>

#include "main.h"
#include "gE_prefs.h"
#include "menus.h"

gE_window *main_window;
char home[STRING_LENGTH_MAX];
char *homea;
char rc[STRING_LENGTH_MAX];
char fname[STRING_LENGTH_MAX];

void  destroy_window (GtkWidget *widget, GtkWidget **window)
{
   *window = NULL;
   gtk_exit(0);
}


void file_quit_cmd_callback (GtkWidget *widget, gpointer data)
{
   /* g_print ("%s\n", (char *) data);*/
   gtk_exit(0);

}

/*
void file_newwindow_cmd_callback (GtkWidget *widget, gpointer data)
{
  gE_window_new();
}

*/

#ifdef WITHOUT_GNOME
int main (int argc, char **argv)
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
 	// g_print ("gEdit\n\n");
 	gE_show_version();
 	exit(0);
      }
     }


	gE_get_rc_file();
	gE_rc_parse();
   

 argv++;
 prog_init(argc,argv);
  

		
  		//  gtk_idle_add ((GtkFunction) linepos, NULL);
		//	gtk_timeout_add (5, (GtkFunction) linepos, NULL);

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
	argp_program_version = VERSION;
	bindtextdomain(PACKAGE, GNOMELOCALEDIR);
	textdomain(PACKAGE);

	gnome_init ("gEdit", &parser, argc, argv, 0, NULL);

	gE_rc_parse();

	main_window = gE_window_new ();
	auto_indent_toggle_callback (NULL,NULL);
	if (file_list){
		gE_document *doc;

		doc = gE_document_current(main_window);
		gtk_notebook_remove_page(GTK_NOTEBOOK(main_window->notebook),
					 gtk_notebook_current_page (GTK_NOTEBOOK(main_window->notebook)));
		g_list_remove(main_window->documents, doc);
		if (doc->filename != NULL)
			g_free (doc->filename);
		g_free (doc);

		for (;file_list; file_list = file_list->next)
			gtk_idle_add ((GtkFunction) file_open_wrapper, file_list->data);
	}

	gtk_main ();
	return 0;
}

#endif

