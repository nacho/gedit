/* plugin.h - plugin header files.
 *
 * Copyright (C) 1998 Chris Lahey.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __PLUGIN_H__
#define __PLUGIN_H__

#include <unistd.h>
#include <glib.h>

typedef struct
{
  (int *create_callback) ( gchar *title );
  (void *append_callback) ( gint id, gchar *data, gint length );
  (void *show_callback) ( gint id );
} plugin_callback_struct;

typedef struct
{
  int pipe_to;
  int pipe_from;
  int pipe_data;
  int pid;
  char *name;
  int in_call;
  plugin_callback_struct callbacks;
  int docid;
} plugin;

typedef void plugin_callback( plugin *, gchar *, int length, gpointer data );

plugin *plugin_new( gchar * );
void plugin_send( plugin *, gchar *, gint length );
void plugin_send_int( plugin *, gint );
void plugin_send_data( plugin *, gchar *, gint length );
void plugin_send_data_int( plugin *, gint );
void plugin_get( plugin *, gchar *, gint length );
void plugin_get_all( plugin *, gchar *, gint length, plugin_callback *finished, gpointer data );
void plugin_register_document_create( plugin *, (int *) () )
void plugin_register_document_append( plugin *, (void *) (int, char *, int) );
void plugin_register_document_show( plugin *, (void *) (int) );
#endif
