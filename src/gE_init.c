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
#include <string.h>
#include <stdio.h>
#include "main.h"
#include "gE_init.h"
#include "gE_files.h"
#include "gE_document.h"
#include "gE_plugin_api.h"
#include "msgbox.h"

extern GList *plugins;

gint file_open_wrapper (gE_data *data)
{
	char *nfile, *name;

	name = data->temp2;
	gE_document_new(data->window);
	nfile = g_malloc(strlen(name)+1);
	strcpy(nfile, name);
	gE_file_open (data->window, gE_document_current(data->window), nfile);

	return FALSE;
}

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

	msgbox_create();

	doc_pointer_to_int = g_hash_table_new (g_direct_hash, g_direct_equal);
	doc_int_to_pointer = g_hash_table_new (g_direct_hash, g_direct_equal);
	win_pointer_to_int = g_hash_table_new (g_direct_hash, g_direct_equal);
	win_int_to_pointer = g_hash_table_new (g_direct_hash, g_direct_equal);

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
	
	callbacks.document.create = gE_plugin_document_create;
	callbacks.text.append = gE_plugin_text_append;
	callbacks.document.show = gE_plugin_document_show;
	callbacks.document.current = gE_plugin_document_current;
	callbacks.document.filename = gE_plugin_document_filename;
	callbacks.text.get = gE_plugin_text_get;
	callbacks.program.quit = gE_plugin_program_quit;
	callbacks.program.reg = gE_plugin_program_register;
	
	plugin_query_all (&callbacks);
	
}

/* the end */

