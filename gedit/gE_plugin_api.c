/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 3 -*-
 *
 * gEdit
 * Copyright (C) 1998 Alex Roberts, Evan Lawrence, and Chris Lahey
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
#include <gtk/gtk.h>
#include <config.h>
#include <gnome.h>

#include "main.h"
#ifdef WITH_GMODULE_PLUGINS
#include "gedit_plugin.h"
#endif
#include "gedit_plugin_api.h"
#include "gedit_window.h"
#include "gedit_view.h"
#include "gedit_files.h"
#include "commands.h"
#include "gedit_mdi.h"

GList *plugins;
GHashTable *win_int_to_pointer,
*win_pointer_to_int,
*doc_pointer_to_int,
*doc_int_to_pointer;
int last_assigned_integer;

/* --- Accesory functions for gedit's handling of the plugins --- */

void 
start_plugin(GtkWidget * widget, gedit_data * data)
{

	plugin_callback_struct callbacks;
	plugin *plug = plugin_new(data->temp1);
	plugin_info *info = data->temp2;

	memset(&callbacks, 0, sizeof(plugin_callback_struct));

	callbacks.document.create = gedit_plugin_document_create;
	callbacks.text.append = gedit_plugin_text_append;
	callbacks.text.insert = gedit_plugin_text_insert;
	callbacks.document.show = gedit_plugin_document_show;
	callbacks.document.current = gedit_plugin_document_current;
	callbacks.document.filename = gedit_plugin_document_filename;
	callbacks.document.open = gedit_plugin_document_open;
	callbacks.document.close = gedit_plugin_document_close;
	callbacks.document.set_auto_indent = gedit_plugin_set_auto_indent;
	callbacks.document.set_status_bar = gedit_plugin_set_status_bar;
	callbacks.document.set_word_wrap = gedit_plugin_set_word_wrap;
	callbacks.document.set_line_wrap = gedit_plugin_set_line_wrap;
	callbacks.document.set_read_only = gedit_plugin_set_read_only;
	callbacks.document.set_split_screen = gedit_plugin_set_split_screen;

	callbacks.text.get = gedit_plugin_text_get;
	callbacks.program.quit = gedit_plugin_program_quit;

#if 0
	callbacks.document.open = NULL;
	callbacks.document.close = NULL;
#endif
	callbacks.text.get_selected_text = gedit_plugin_text_get_selected_text;
	callbacks.text.set_selected_text = gedit_plugin_text_set_selected_text;
	callbacks.document.get_position = gedit_plugin_document_get_position;
	callbacks.document.get_selection = gedit_plugin_document_get_selection;

	plugin_register(plug, &callbacks, *(int *) g_hash_table_lookup(win_pointer_to_int, mdi->active_window));
   
}

void 
add_plugin_to_menu(GnomeApp *app, plugin_info * info)
{

	gedit_data *data = g_malloc0(sizeof(gE_data));

	gchar *path;
	GnomeUIInfo *menu = g_malloc0(2 * sizeof(GnomeUIInfo));

	data->temp1 = g_strdup(info->plugin_name);
	data->temp2 = info;
	path = g_new(gchar, strlen(_("_Plugins")) + 2);
	sprintf(path, "%s/", _("_Plugins"));
	menu->label = g_strdup(info->menu_location);
	menu->type = GNOME_APP_UI_ITEM;
	menu->hint = NULL;
	menu->moreinfo = start_plugin;
	menu->user_data = data;
	menu->unused_data = NULL;
	menu->pixmap_type = 0;
	menu->pixmap_info = NULL;
	menu->accelerator_key = 0;

	(menu + 1)->type = GNOME_APP_UI_ENDOFINFO;

	gnome_app_insert_menus_with_data (app, path, menu, data);

}

void 
add_plugins_to_window(plugin_info * info, GnomeApp *app)
{

	add_plugin_to_menu(app, info);

}

/* --- Direct interface to the plugins API --- */


/* Text related functions */
 
void 
gedit_plugin_text_insert(gint docid, gchar * buffer, gint length, gint position)
{

	gedit_document *document = gE_document_current();
	gedit_view *view = GE_VIEW (mdi->active_view);

	if (position >= gtk_text_get_length(GTK_TEXT(view->text)))
	  position = gtk_text_get_length(GTK_TEXT(view->text));
	
	/*position = gedit_view_get_position (view);*/
	position = GTK_TEXT(view->text)->cursor_pos_x/6;		gtk_text_freeze(GTK_TEXT(view->text));
	gtk_editable_insert_text (GTK_EDITABLE (view->text), buffer, length, &position);
	gtk_text_thaw(GTK_TEXT(view->text));
	
	view->changed = 1;
	
}

void 
gedit_plugin_text_append(gint docid, gchar * buffer, gint length)
{

	gedit_document *document = gE_document_current();
	gedit_view *view = GE_VIEW (mdi->active_view);
	gint position;
   
	position = gtk_text_get_length(GTK_TEXT(view->text));
   
	gtk_text_freeze(GTK_TEXT(view->text));
	gtk_editable_insert_text (GTK_EDITABLE (view->text), buffer, length, &position);
	gtk_text_thaw(GTK_TEXT(view->text));
	view->changed = 1;
	
}

char *
gedit_plugin_text_get(gint docid)
{

	gedit_document *document = gE_document_current();
	gedit_view *view = GE_VIEW (mdi->active_view);

	return gtk_editable_get_chars(GTK_EDITABLE(view->text), 0, -1);
	
}

gchar *
gedit_plugin_text_get_selected_text (gint docid)
{

	gedit_document *document = gE_document_current();
	gedit_view *view = GE_VIEW (mdi->active_view);
   
	return gtk_editable_get_chars (GTK_EDITABLE (view->text),
		GTK_EDITABLE (view->text)->selection_start_pos,
		GTK_EDITABLE (view->text)->selection_end_pos);

}


void
gedit_plugin_text_set_selected_text (gint docid, gchar *text)
{

	GtkEditable *editable;
	selection_range selection;
	/*   gedit_document *document = (gE_document *) g_hash_table_lookup(doc_int_to_pointer, &docid);*/
 	gedit_document *document = gE_document_current();
	gedit_view *view = GE_VIEW (mdi->active_view);

	g_return_if_fail (document != NULL);
	editable = GTK_EDITABLE (view->text);
	if (editable->selection_start_pos <= editable->selection_end_pos) {
	
	  selection.start = editable->selection_start_pos;
	  selection.end = editable->selection_end_pos;
	
	} else {
	
	 selection.start = editable->selection_end_pos;
	 selection.end = editable->selection_start_pos;
	 
	}
	
	gtk_editable_delete_selection (editable);
	selection.end = selection.start;
	gtk_editable_insert_text (editable, text, strlen (text), &selection.end);
	gtk_editable_select_region (editable, selection.start, selection.end);
	
}


/* Document related functions */

int 
gedit_plugin_document_create(gint context, gchar * title)
{

	gedit_document *doc;
   
	doc = gedit_document_new ();
	gnome_mdi_add_child (mdi, GNOME_MDI_CHILD (doc));
	gnome_mdi_add_view (mdi, GNOME_MDI_CHILD (doc));
    
	return (int) doc;
	
}

void 
gedit_plugin_document_show(gint docid)
{
}

int 
gedit_plugin_document_current(gint context)
{

  return *(int *) gedit_document_current();

}

gchar *
gedit_plugin_document_filename(gint docid)
{

	 gedit_document *document = gE_document_current();

	if (document->filename == NULL)
	  return "";
	else
	  return document->filename;
	  
}

int 
gedit_plugin_document_open(gint context, gchar * fname)
{

	gchar *newfname;
	gedit_document *doc;

	newfname = g_strdup(fname);
	doc = gedit_document_new_with_file(newfname);

	return *(int *) g_hash_table_lookup(doc_pointer_to_int, (doc));

}

gboolean 
gedit_plugin_document_close(gint docid)
{

	gedit_document *document = gE_document_current();
	gedit_view *view = GE_VIEW (mdi->active_view);

	file_close_cb (NULL, NULL);

	return TRUE;
	
}

gint
gedit_plugin_document_get_position (gint docid)
{

   	gedit_document *document = gE_document_current();
 	gedit_view *view = GE_VIEW (mdi->active_view);
   	
	g_return_val_if_fail (document != NULL, 0);
	return gedit_view_get_position (view);
}

selection_range
gedit_plugin_document_get_selection (gint docid)
{
	GtkEditable *editable;
	selection_range selection;
   	gedit_document *document = gE_document_current();
	gedit_view *view = GE_VIEW (mdi->active_view);
   	   	
   	
	selection.start = 0;
	selection.end = 0;
	g_return_val_if_fail (document != NULL, selection);
	editable = GTK_EDITABLE (view->text);
	
	if (editable->selection_start_pos <= editable->selection_end_pos) {
	
	  selection.start = editable->selection_start_pos;
	  selection.end = editable->selection_end_pos;
	  
	} else {
	
	 selection.start = editable->selection_end_pos;
	 selection.end = editable->selection_start_pos;
	
	}
	
	return selection;
	
}

/* Misc UI related functions */

void 
gedit_plugin_set_auto_indent(gint docid, gint auto_indent)
{

	gedit_document *document = gE_document_current();

	gedit_window_set_auto_indent(auto_indent);
	
}

void 
gedit_plugin_set_status_bar(gint docid, gint status_bar)
{

	gedit_document *document = gE_document_current();

	gedit_window_set_status_bar(status_bar);

}

void 
gedit_plugin_set_word_wrap(gint docid, gint word_wrap)
{

	gedit_view *view = GE_VIEW (mdi->active_view);

	gedit_view_set_word_wrap(view, word_wrap);
	
}

void 
gedit_plugin_set_line_wrap(gint docid, gint line_wrap)
{

	gedit_view *view = GE_VIEW (mdi->active_view);

	gedit_view_set_line_wrap(view, line_wrap);
	
}

void 
gedit_plugin_set_read_only(gint docid, gint read_only)
{

	gedit_view *view = GE_VIEW (mdi->active_view);

	gedit_view_set_read_only(view, read_only);
	
}

void 
gedit_plugin_set_split_screen(gint docid, gint split_screen)
{
/*   gedit_document *document = (gE_document *) g_hash_table_lookup(doc_int_to_pointer, &docid);*/
   gedit_view *view = GE_VIEW (mdi->active_view);

   gedit_view_set_split_screen(view, split_screen);
}


/* Program Related functions */

gboolean 
gedit_plugin_program_quit()
{

	gedit_data *data;
	gedit_window *window;

	data = g_malloc0(sizeof(gedit_data));
	window = g_list_nth_data(window_list, 1);
	data->window = window;
	data->temp1 = window;
	file_close_cb(NULL, data);
	
	return TRUE;
	
}


/* mercilessly lifted right out of go.. */

void 
gedit_plugin_program_register(plugin_info * info)
{

	plugin_info *temp;

	temp = info;

	info = g_malloc0(sizeof(plugin_info));
	info->type = temp->type;
	info->user_data = temp->user_data;
	info->menu_location = g_malloc0(strlen(temp->menu_location) + 1);
	
	strcpy(info->menu_location, temp->menu_location);
	info->suggested_accelerator = NULL;
	info->plugin_name = g_malloc0(strlen(temp->plugin_name) + 1);
	strcpy(info->plugin_name, temp->plugin_name);
	plugins = g_list_append(plugins, info);

	g_list_foreach(mdi->windows, (GFunc) add_plugin_to_menu, info);
	
}
