/* plugin.c implements plugin features for the caller of the plugin.
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
#include "plugin.h"
#include <unistd.h>
#include <gtk/gtk.h>

plugin *plugin_new( gchar *plugin_name )
{
  int toline[2]; /* Commands to the plugin. */
  int fromline[2]; /* Commands from the plugin. */
  int dataline[2]; /* Data to the plugin. */
  plugin *new_plugin = g_new( plugin, 1 ); /* The plugin. */
  
  if ( pipe( toline ) == -1 || pipe( fromline ) == -1 || pipe( dataline ) == -1 )
    {
      g_free( new_plugin );
      return 0;
    }
  new_plugin->pipe_to = toline[1];
  new_plugin->pipe_from = fromline[0];
  new_plugin->pipe_data = dataline[1];
  new_plugin->pid = fork();
  new_plugin->name = g_strdup( strrchr( plugin_name, '/' ) ? strrchr( plugin_name, '/' ) + 1 : plugin_name );
  if ( new_plugin->pid == 0 )
    {
      /* New process. */
      char *argv[7];

      close( new_plugin->pipe_to );
      close( new_plugin->pipe_from );
      close( new_plugin->pipe_data );
      argv[0] = g_malloc0( 13 + strlen( new_plugin->name ) );
      sprintf( argv[0], "gedit-plugin-%s", new_plugin->name );
      argv[1] = g_strdup( "-go" );
      argv[2] = g_malloc0( 15 );
      g_snprintf( argv[2], 15, "%d", toline[0] );
      argv[3] = g_malloc0( 15 );
      g_snprintf( argv[3], 15, "%d", fromline[1] );
      argv[4] = g_malloc0( 15 );
      g_snprintf( argv[4], 15, "%d", dataline[0] );
      argv[5] = NULL;
      execv(plugin_name, argv);
      /* This is only reached if something goes wrong. */
      _exit( 1 );
    }
  else if ( new_plugin->pid == -1 )
    {
      /* Failure. */
      g_free( new_plugin );
      return 0;
    }
  /* Success. */

  close( toline[0] );
  close( fromline[1] );
  close( dataline[0] );
#if 0
  new_plugin->in_call = gdk_input_add( new_plugin->pipe_from,
				       GDK_INPUT_READ,
				       callback_function,
				       new_plugin );
#endif
  return new_plugin;
}

void plugin_finish( plugin *the_plugin )
{
  close( the_plugin->pipe_to );
  close( the_plugin->pipe_from );
  close( the_plugin->pipe_data );
}

void plugin_send(plugin *the_plugin, gchar *buffer, gint length)
{
  write( the_plugin->pipe_to, buffer, length );
}
