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
#include "gE_plugin.h"
#endif
#include "gE_plugin_api.h"
#include "gE_window.h"
#include "gE_view.h"
#include "gE_files.h"
#include "commands.h"
#include "gE_mdi.h"

GList *plugins;
GHashTable *win_int_to_pointer,
*win_pointer_to_int,
*doc_pointer_to_int,
*doc_int_to_pointer;
int last_assigned_integer;

/* --- Accesory functions for gedit's handling of the plugins --- */

void 
start_plugin(GtkWidget * widget, gE_data * data)
{

	plugin_callback_struct callbacks;
	plugin *plug = plugin_new(data->temp1);
	plugin_info *info = data->temp2;

	memset(&callbacks, 0, sizeof(plugin_callback_struct));

	callbacks.document.create = gE_plugin_document_create;
	callbacks.text.append = gE_plugin_text_append;
	callbacks.text.insert = gE_plugin_text_insert;
	callbacks.document.show = gE_plugin_document_show;
	callbacks.document.current = gE_plugin_document_current;
	callbacks.document.filename = gE_plugin_document_filename;
	callbacks.document.open = gE_plugin_document_open;
	callbacks.document.close = gE_plugin_document_close;
	callbacks.document.set_auto_indent = gE_plugin_set_auto_indent;
	callbacks.document.set_status_bar = gE_plugin_set_status_bar;
	callbacks.document.set_word_wrap = gE_plugin_set_word_wrap;
	callbacks.document.set_line_wrap = gE_plugin_set_line_wrap;
	callbacks.document.set_read_only = gE_plugin_set_read_only;
	callbacks.document.set_split_screen = gE_plugin_set_split_screen;

	callbacks.text.get = gE_plugin_text_get;
	callbacks.program.quit = gE_plugin_program_quit;

#if 0
	callbacks.document.open = NULL;
	callbacks.document.close = NULL;
#endif
	callbacks.text.get_selected_text = gE_plugin_text_get_selected_text;
	callbacks.text.set_selected_text = gE_plugin_text_set_selected_text;
	callbacks.document.get_position = gE_plugin_document_get_position;
	callbacks.document.get_selection = gE_plugin_document_get_selection;

	plugin_register(plug, &callbacks, *(int *) g_hash_table_lookup(win_pointer_to_int, mdi->active_window));
   
}

void 
add_plugin_to_menu(GnomeApp *app, plugin_info * info)
{

	gE_data *data = g_malloc0(sizeof(gE_data));

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
gE_plugin_text_insert(gint docid, gchar * buffer, gint length, gint position)
{

	gE_document *document = gE_document_current();
	gE_view *view = GE_VIEW (mdi->active_view);

	if (position >= gtk_text_get_length(GTK_TEXT(view->text)))
	  position = gtk_text_get_length(GTK_TEXT(view->text));
	
	/*position = gE_view_get_position (view);*/
	position = GTK_TEXT(view->text)->cursor_pos_x/6;		gtk_text_freeze(GTK_TEXT(view->text));
	gtk_editable_insert_text (GTK_EDITABLE (view->text), buffer, length, &position);
	gtk_text_thaw(GTK_TEXT(view->text));
	
	view->changed = 1;
	
}

void 
gE_plugin_text_append(gint docid, gchar * buffer, gint length)
{

	gE_document *document = gE_document_current();
	gE_view *view = GE_VIEW (mdi->active_view);
	gint position;
   
	position = gtk_text_get_length(GTK_TEXT(view->text));
   
	gtk_text_freeze(GTK_TEXT(view->text));
	gtk_editable_insert_text (GTK_EDITABLE (view->text), buffer, length, &position);
	gtk_text_thaw(GTK_TEXT(view->text));
	view->changed = 1;
	
}

char *
gE_plugin_text_get(gint docid)
{

	gE_document *document = gE_document_current();
	gE_view *view = GE_VIEW (mdi->active_view);

	return gtk_editable_get_chars(GTK_EDITABLE(view->text), 0, -1);
	
}

gchar *
gE_plugin_text_get_selected_text (gint docid)
{

	gE_document *document = gE_document_current();
	gE_view *view = GE_VIEW (mdi->active_view);
   
	return gtk_editable_get_chars (GTK_EDITABLE (view->text),
		GTK_EDITABLE (view->text)->selection_start_pos,
		GTK_EDITABLE (view->text)->selection_end_pos);

}


void
gE_plugin_text_set_selected_text (gint docid, gchar *text)
{

	GtkEditable *editable;
	selection_range selection;
	/*   gE_document *document = (gE_document *) g_hash_table_lookup(doc_int_to_pointer, &docid);*/
 	gE_document *document = gE_document_current();
	gE_view *view = GE_VIEW (mdi->active_view);

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
gE_plugin_document_create(gint context, gchar * title)
{

	gE_document *doc;
   
	doc = gE_document_new ();
	gnome_mdi_add_child (mdi, GNOME_MDI_CHILD (doc));
	gnome_mdi_add_view (mdi, GNOME_MDI_CHILD (doc));
    
	return (int) doc;
	
}

void 
gE_plugin_document_show(gint docid)
{
}

int 
gE_plugin_document_current(gint context)
{

  return *(int *) gE_document_current();

}

gchar *
gE_plugin_document_filename(gint docid)
{

	 gE_document *document = gE_document_current();

	if (document->filename == NULL)
	  return "";
	else
	  return document->filename;
	  
}

int 
gE_plugin_document_open(gint context, gchar * fname)
{

	gchar *newfname;
	gE_document *doc;

	newfname = g_strdup(fname);
	doc = gE_document_new_with_file(newfname);

	return *(int *) g_hash_table_lookup(doc_pointer_to_int, (doc));

}

gboolean 
gE_plugin_document_close(gint docid)
{

	gE_document *document = gE_document_current();
	gE_view *view = GE_VIEW (mdi->active_view);

	file_close_cb (NULL, NULL);

	return TRUE;
	
}

gint
gE_plugin_document_get_position (gint docid)
{

   	gE_document *document = gE_document_current();
 	gE_view *view = GE_VIEW (mdi->active_view);
   	
	g_return_val_if_fail (document != NULL, 0);
	return gE_view_get_position (view);
}

selection_range
gE_plugin_document_get_selection (gint docid)
{
	GtkEditable *editable;
	selection_range selection;
   	gE_document *document = gE_document_current();
	gE_view *view = GE_VIEW (mdi->active_view);
   	   	
   	
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
gE_plugin_set_auto_indent(gint docid, gint auto_indent)
{

	gE_document *document = gE_document_current();

	gE_window_set_auto_indent(auto_indent);
	
}

void 
gE_plugin_set_status_bar(gint docid, gint status_bar)
{

	gE_document *document = gE_document_current();

	gE_window_set_status_bar(status_bar);

}

void 
gE_plugin_set_word_wrap(gint docid, gint word_wrap)
{

	gE_view *view = GE_VIEW (mdi->active_view);

	gE_view_set_word_wrap(view, word_wrap);
	
}

void 
gE_plugin_set_line_wrap(gint docid, gint line_wrap)
{

	gE_view *view = GE_VIEW (mdi->active_view);

	gE_view_set_line_wrap(view, line_wrap);
	
}

void 
gE_plugin_set_read_only(gint docid, gint read_only)
{

	gE_view *view = GE_VIEW (mdi->active_view);

	gE_view_set_read_only(view, read_only);
	
}

void 
gE_plugin_set_split_screen(gint docid, gint split_screen)
{
/*   gE_document *document = (gE_document *) g_hash_table_lookup(doc_int_to_pointer, &docid);*/
   gE_view *view = GE_VIEW (mdi->active_view);

   gE_view_set_split_screen(view, split_screen);
}


/* Program Related functions */

gboolean 
gE_plugin_program_quit()
{

	gE_data *data;
	gE_window *window;

	data = g_malloc0(sizeof(gE_data));
	window = g_list_nth_data(window_list, 1);
	data->window = window;
	data->temp1 = window;
	file_close_cb(NULL, data);
	
	return TRUE;
	
}


/* mercilessly lifted right out of go.. */

void 
gE_plugin_program_register(plugin_info * info)
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
