/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 2 -*-
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

#include <string.h>
#include <gtk/gtk.h>
#include "main.h"
#include <config.h>
#ifdef WITH_GMODULE_PLUGINS
#include "gE_plugin.h"
#endif
#include "gE_plugin_api.h"
#include "gE_document.h"
#include "gE_files.h"
#include "commands.h"

#ifndef WITHOUT_GNOME
#include <gnome.h>
#endif

GList *plugins;
GHashTable *win_int_to_pointer, *win_pointer_to_int, *doc_pointer_to_int, *doc_int_to_pointer;
int last_assigned_integer;

/* --- Accesory functions for gedit's handling of the plugins --- */

void start_plugin( GtkWidget *widget, gE_data *data )
{
  plugin_callback_struct callbacks;
  plugin *plug = plugin_new( data->temp1 );
  plugin_info *info = data->temp2;

  memset (&callbacks, 0, sizeof (plugin_callback_struct));

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
#ifndef WITHOUT_GNOME
  callbacks.document.set_scroll_ball = gE_plugin_set_scroll_ball;
#endif
  callbacks.text.get = gE_plugin_text_get;
  callbacks.program.quit = gE_plugin_program_quit;

#if 0
  callbacks.document.open = NULL;
  callbacks.document.close = NULL;
#endif
  callbacks.text.get_selected_text = NULL;
  callbacks.text.set_selected_text = NULL;
  callbacks.document.get_position = NULL;
  callbacks.document.get_selection = NULL;

#ifdef WITH_GMODULE_PLUGINS
  if ( info->type == PLUGIN_GMODULE ) {
      gE_Plugin_Load ( (gE_Plugin_Object *)info->user_data,
		       *(int *)g_hash_table_lookup (win_pointer_to_int,
						    data->window) );
      return;
  }
#endif

  plugin_register( plug, &callbacks, *(int *)g_hash_table_lookup (win_pointer_to_int, data->window ) );
}

void add_plugin_to_menu (gE_window *window, plugin_info *info)
{
	gE_data *data = g_malloc0 (sizeof (gE_data));
#ifdef WITHOUT_GNOME
	GtkMenuEntry *entry = g_malloc0 (sizeof (GtkMenuEntry));
	
	entry->path = g_malloc0 (strlen (info->menu_location) + strlen ("Plugins/") + 1);
	sprintf (entry->path, "Plugins/%s", info->menu_location);
	entry->accelerator = NULL;
	entry->callback = (GtkMenuCallback)(GTK_SIGNAL_FUNC (start_plugin));
	data->temp1 = g_strdup (info->plugin_name);
	data->window = window;
	entry->callback_data = data;
	
	gtk_menu_factory_add_entries((GtkMenuFactory *)(window->factory), entry, 1);
#else
	gchar *path;
	GnomeUIInfo *menu = g_malloc0 (2 * sizeof (GnomeUIInfo));
	
	data->temp1 = g_strdup (info->plugin_name);
	data->temp2 = info;
	data->window = window;
	path = g_new (gchar, strlen (_("Plugins") ) + 2 );
	sprintf (path, "%s/", _("Plugins"));
	menu->label = g_strdup (info->menu_location);
	menu->type = GNOME_APP_UI_ITEM;
	menu->hint = NULL;
	menu->moreinfo = start_plugin;
	menu->user_data = data;
	menu->unused_data = NULL;
	menu->pixmap_type = 0;
	menu->pixmap_info = NULL;
	menu->accelerator_key = 0;

	(menu + 1)->type = GNOME_APP_UI_ENDOFINFO;
	
	gnome_app_insert_menus_with_data (GNOME_APP(window->window), path, menu, data);
#endif

}

void add_plugins_to_window (plugin_info *info, gE_window *window)
{
	add_plugin_to_menu (window, info);
}


/* --- Direct interface to the plugins API --- */

int gE_plugin_document_create( gint context, gchar *title )
{
  return *(int *)g_hash_table_lookup( doc_pointer_to_int, gE_document_new( (gE_window *) g_hash_table_lookup( win_int_to_pointer, &context )));
}

void gE_plugin_text_insert( gint docid, gchar *buffer, gint length, gint position )
{
  gE_document *document = (gE_document *) g_hash_table_lookup( doc_int_to_pointer, &docid );
  GtkText *text = GTK_TEXT( document->text );
  if( position >= gtk_text_get_length( text ) )
    position = gtk_text_get_length( text );
  gtk_text_freeze( text );
  gtk_text_set_point( text, position );
  gtk_text_insert( text, NULL, NULL, NULL, buffer, length );
  gtk_text_thaw( text );
  document->changed = 1;
}

void gE_plugin_text_append( gint docid, gchar *buffer, gint length )
{
  gE_document *document = (gE_document *) g_hash_table_lookup( doc_int_to_pointer, &docid );
  GtkText *text = GTK_TEXT( document->text );
  gtk_text_freeze( text );
  gtk_text_set_point( text, gtk_text_get_length( text ) );
  gtk_text_insert( text, NULL, NULL, NULL, buffer, length );
  gtk_text_thaw( text );
  document->changed = 1;
}

void gE_plugin_document_show( gint docid )
{
}

int gE_plugin_document_current( gint context )
{
  return *(int *)g_hash_table_lookup (doc_pointer_to_int, ( gE_document_current( g_hash_table_lookup( win_int_to_pointer, &context ) ) ) );
}

gchar *gE_plugin_document_filename( gint docid )
{
  gE_document *document = (gE_document *) g_hash_table_lookup (doc_int_to_pointer, &docid);
  if (document->filename == NULL)
  	return "";
  else
  	return document->filename;
}

int gE_plugin_document_open(gint context,gchar *fname)
{
      gchar *newfname;
      gE_document *doc;

      newfname=g_strdup(fname);
      doc=gE_document_new( g_hash_table_lookup (win_int_to_pointer, &context )) ;

      gE_file_open(g_hash_table_lookup (win_int_to_pointer, &context) ,doc,newfname);
      return *(int *)g_hash_table_lookup (doc_pointer_to_int, (doc));
}

gboolean gE_plugin_document_close (gint docid)
{
	gE_document *document = (gE_document *) g_hash_table_lookup (doc_int_to_pointer, &docid);
	gE_data *data = g_malloc (sizeof (gE_data));
	gboolean flag;

	data->document = document;

	if (document->changed)
		popup_close_verify (document, data);
	else {
		close_doc_execute (document, data);
		data->flag = TRUE;
	}

	flag = data->flag;
	g_free (data);

	return flag;	
}

void gE_plugin_set_auto_indent (gint docid, gint auto_indent)
{
	gE_document *document = (gE_document *) g_hash_table_lookup (doc_int_to_pointer, &docid);

	gE_window_set_auto_indent (document->window, auto_indent);
}

void gE_plugin_set_status_bar (gint docid, gint status_bar)
{
	gE_document *document = (gE_document *) g_hash_table_lookup (doc_int_to_pointer, &docid);

	gE_window_set_status_bar (document->window, status_bar);
}

void gE_plugin_set_word_wrap (gint docid, gint word_wrap)
{
	gE_document *document = (gE_document *) g_hash_table_lookup (doc_int_to_pointer, &docid);

	gE_document_set_word_wrap (document, word_wrap);
}

void gE_plugin_set_line_wrap (gint docid, gint line_wrap)
{
	gE_document *document = (gE_document *) g_hash_table_lookup (doc_int_to_pointer, &docid);

	gE_document_set_line_wrap (document, line_wrap);
}

void gE_plugin_set_read_only (gint docid, gint read_only)
{
	gE_document *document = (gE_document *) g_hash_table_lookup (doc_int_to_pointer, &docid);

	gE_document_set_read_only (document, read_only);
}

void gE_plugin_set_split_screen (gint docid, gint split_screen)
{
	gE_document *document = (gE_document *) g_hash_table_lookup (doc_int_to_pointer, &docid);

	gE_document_set_split_screen (document, split_screen);
}

#ifndef WITHOUT_GNOME
void gE_plugin_set_scroll_ball (gint docid, gint scroll_ball)
{
	gE_document *document = (gE_document *) g_hash_table_lookup (doc_int_to_pointer, &docid);

	gE_document_set_scroll_ball (document, scroll_ball);
}
#endif

char *gE_plugin_text_get( gint docid )
{
  gE_document *document = (gE_document *) g_hash_table_lookup (doc_int_to_pointer, &docid);
  return gtk_editable_get_chars( GTK_EDITABLE( document->text ), 0, -1 );
}


gboolean gE_plugin_program_quit ()
{
	gE_data *data;
	gE_window *window;
	data = g_malloc0 (sizeof (gE_data));
	window = g_list_nth_data (window_list, 1);
	data->window = window;
	data->temp1 = window;
	file_close_cb (NULL, data);
	return TRUE;
}

/* mercilessly lifted right out of go.. */

void gE_plugin_program_register (plugin_info *info)
{
  plugin_info *temp;

  temp = info;
  info = g_malloc0( sizeof( plugin_info ) );
  info->type = temp->type;
  info->user_data = temp->user_data;
  info->menu_location = g_malloc0( strlen( temp->menu_location ) + 1 );
  strcpy( info->menu_location, temp->menu_location );
  info->suggested_accelerator = NULL;
  info->plugin_name = g_malloc0( strlen( temp->plugin_name ) + 1 );
  strcpy( info->plugin_name, temp->plugin_name );
  plugins = g_list_append( plugins, info );

  g_list_foreach( window_list, (GFunc)add_plugin_to_menu, info );
}

GtkText *gE_plugin_get_widget( gint docid )
{
  return ((gE_document *) g_hash_table_lookup (doc_int_to_pointer, &docid))->text;
}

