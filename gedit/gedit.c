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

void line_pos_callback(GtkWidget *w, GtkWidget *text)
{
static char statustext[32];

if (main_window->documents > 0)
{
/*print("%s\n", GTK_TEXT(text)->current_line);*/

sprintf (statustext,"Line: %d, Col: %d", 
				GTK_TEXT(text)->cursor_pos_y/13,
				GTK_TEXT(text)->cursor_pos_x/7);

	gtk_statusbar_push (GTK_STATUSBAR(main_window->statusbar), 1, statustext);
}
	

}


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
		      		      
