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
  gint (*create) ( gint context, gchar *title );
  void (*show) ( gint id );
  gchar* (*filename) ( gint id );
  gint (*current) ( gint context );
} plugin_document_callbacks;

typedef struct
{
  void (*append) ( gint id, gchar *data, gint length );
  gchar* (*get) ( gint id );
} plugin_text_callbacks;

typedef struct
{
  plugin_document_callbacks document;
  plugin_text_callbacks text;
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
void plugin_get_all( plugin *, gint length, plugin_callback *finished, gpointer data );
void plugin_register( plugin *, plugin_callback_struct *, gint context );
#endif
