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
  int pipe_to;
  int pipe_from;
  int pipe_data;
  int pid;
  char *name;
  int in_call;
} plugin;

plugin *plugin_new(gchar *);
void plugin_send(plugin *, gchar *data, gint length);
#if 0
void receive_from_plug_in(plugin *);
void send_as_plug_in(plugin *);
void receieve_as_plug_in(plugin *);
#endif

#endif
