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

#include <gtk/gtk.h>
#include "main.h"

gint file_open_wrapper (char *name)
{
char *nfile;

	gE_document_new(main_window);
    nfile = g_malloc(strlen(name)+1);
    strcpy(nfile, name);
    gE_file_open (gE_document_current(main_window), nfile);
	
	return FALSE;
}

void prog_init(int fnum, char **file)
{
int x;
char nfile;
char *filename;
	gE_document *doc;

#ifdef DEBUG
g_print("Initialising gEdit...\n");

		g_print("%d\n",fnum);	

		g_print("%s\n",*file);
#endif
	if (*file != NULL)
	  {
	   main_window = gE_window_new();
		g_print("Opening files...\n");

	doc = gE_document_current(main_window);
			gtk_notebook_remove_page(GTK_NOTEBOOK(main_window->notebook),
				gtk_notebook_current_page (GTK_NOTEBOOK(main_window->notebook)));
			g_list_remove(main_window->documents, doc);
			if (doc->filename != NULL)
				g_free (doc->filename);
			g_free (doc);

	if (fnum > 0)
	   while(fnum>0)
           {
		if (*file)
		  gtk_idle_add ((GtkFunction) file_open_wrapper, *file);
		file++;
		fnum--;

	   }
	  }
         else
	  {
	   main_window = gE_window_new();
	  }
}


 