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
#include <string.h>
#include <stdio.h>
#include <sys/wait.h>

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
  new_plugin->name = g_strdup( strrchr( plugin_name, '/' ) ? strrchr( plugin_name, '/' ) + 1 : plugin_name );
  new_plugin->pid = fork();
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
  return new_plugin;
}

void plugin_finish( plugin *the_plugin )
{
  close( the_plugin->pipe_to );
  close( the_plugin->pipe_from );
  close( the_plugin->pipe_data );
  waitpid( the_plugin->pid, NULL, WNOHANG );
}

void plugin_send(plugin *the_plugin, gchar *buffer, gint length)
{
  write( the_plugin->pipe_to, buffer, length );
}

void plugin_send_int( plugin *the_plugin, gint number)
{
  write( the_plugin->pipe_to, &number, sizeof( number ) );
}

void plugin_send_with_length( plugin *plug, gchar *buffer, gint length )
{
  plugin_send_int( plug, length );
  plugin_send( plug, buffer, length );
}

void plugin_send_data(plugin *the_plugin, gchar *buffer, gint length)
{
  write( the_plugin->pipe_data, buffer, length );
}

void plugin_send_data_int( plugin *the_plugin, gint number)
{
  write( the_plugin->pipe_data, &number, sizeof( number ) );
}

void plugin_send_data_with_length( plugin *plug, gchar *buffer, gint length )
{
  plugin_send_data_int( plug, length );
  plugin_send_data( plug, buffer, length );
}

void plugin_get( plugin *the_plugin, gchar *buffer, gint length )
{
  read( the_plugin->pipe_from, buffer, length );
}

typedef struct
{
  plugin *plug;
  gchar *buff;
  int length;
  int sofar;
  int incall;
  plugin_callback *finished;
  gpointer data;
} partly_read;

static void plugin_get_more( gpointer data, gint source, GdkInputCondition condition )
{
  partly_read *partly = (partly_read *) data;
  int count;

  partly->sofar += (count = read( partly->plug->pipe_from, partly->buff + partly->sofar, partly->length - partly->sofar ) );
  if( partly->length - partly->sofar == 0 )
    {
      gdk_input_remove( partly->incall );
      partly->finished( partly->plug, partly->buff, partly->length, partly->data );
      g_free( partly->buff );
      g_free( partly );
    }
  else if( count == 0 )
    {
      gdk_input_remove( partly->incall );
      g_free( partly->buff );
      g_free( partly );
    }
}

void plugin_get_all( plugin *plug, gint length, plugin_callback *finished, gpointer data )
{
  partly_read *partly = g_malloc0( sizeof( partly_read ) );
  partly->plug = plug;
  partly->buff = g_malloc0( length + 1 );
  partly->length = length;
  partly->sofar = 0;
  partly->incall = gdk_input_add( plug->pipe_from, GDK_INPUT_READ,
				  plugin_get_more, partly );
  partly->finished = finished;
  partly->data = data;
}

static void process_command( plugin *plug, gchar *buffer, int length, gpointer data );

static void process_next( plugin *plug, gchar *buffer, int length, gpointer data )
{
  gint next = GPOINTER_TO_INT( data );
  switch( next )
    {
    case 1:
      plug->docid = *( (int *) buffer );
      plugin_get_all( plug, sizeof( int ), process_next, GINT_TO_POINTER( 2 ) );
      break;
    case 2:
      plugin_get_all( plug, *( (int *) buffer ), process_next, GINT_TO_POINTER( 3 ) );
      break;
    case 3:
      if ( plug->callbacks.text.append )
	plug->callbacks.text.append( plug->docid, buffer, length );
      plugin_get_all( plug, 1, process_command, NULL );
      break;
    case 4:
      if ( plug->callbacks.document.show )
	plug->callbacks.document.show( *( (int *) buffer ) );
      plugin_get_all( plug, 1, process_command, NULL );
      break;
    case 5:
      if ( plug->callbacks.document.filename )
	{
	  char *filename = plug->callbacks.document.filename( *( (int *) buffer ) );
	  plugin_send_data_with_length( plug, filename, strlen( filename ) );
	}
      else
	plugin_send_data_with_length( plug, "", 0 );
      plugin_get_all( plug, 1, process_command, NULL );
      break;
    case 6:
      if ( plug->callbacks.document.current )
	plugin_send_data_int( plug, plug->callbacks.document.current( *( (int *) buffer ) ) );
      else
	plugin_send_data_int( plug, 0 );
      plugin_get_all( plug, 1, process_command, NULL );
      break;
    case 7:
    case 11:
      plug->docid = *( (int *) buffer );
      plugin_get_all( plug, sizeof( int ), process_next, GINT_TO_POINTER( next + 2 ) );
      break;
    case 8:
      if ( plug->callbacks.text.get )
	{
	  char *data = plug->callbacks.text.get( *( (int *) buffer ) );
	  plugin_send_data_with_length( plug, data, strlen( data ) );
	}
      else
	plugin_send_data_with_length( plug, "", 0 );
      plugin_get_all( plug, 1, process_command, NULL );
      break;
    case 9:
    case 13:
      plugin_get_all( plug, *( (int *) buffer ), process_next, GINT_TO_POINTER( next + 1 ) );
      break;
    case 10:
      if ( plug->callbacks.document.create )
	plugin_send_data_int( plug, plug->callbacks.document.create( plug->docid, buffer ) );
      else
	plugin_send_data_int( plug, 0 );
      plugin_get_all( plug, 1, process_command, NULL );
      break;
    case 14:
      if ( plug->callbacks.document.open )
	plugin_send_data_int( plug, plug->callbacks.document.open( plug->docid, buffer ) );
      else
	plugin_send_data_int( plug, 0 );
      plugin_get_all( plug, 1, process_command, NULL );
      break;
     }
}

static void process_command( plugin *plug, gchar *buffer, int length, gpointer data )
{
  switch ( *buffer )
    {
    case 'a': /* append */  /* Get docid */
      plugin_get_all( plug, sizeof( int ), process_next, GINT_TO_POINTER( 1 ) );
      break;
    case 's': /* show */ /* Get docid */
      plugin_get_all( plug, sizeof( int ), process_next, GINT_TO_POINTER( 4 ) );
      break;
    case 'f': /* filename */ /* Get docid */
      plugin_get_all( plug, sizeof( int ), process_next, GINT_TO_POINTER( 5 ) );
      break;
    case 'c': /* current */ /* Get context */
      plugin_get_all( plug, sizeof( int ), process_next, GINT_TO_POINTER( 6 ) );
      break;
    case 'n': /* create */ /* Get context */
      plugin_get_all( plug, sizeof( int ), process_next, GINT_TO_POINTER( 7 ) );
      break;
    case 'o': /* open */ /* Get context */
      plugin_get_all( plug, sizeof( int ), process_next, GINT_TO_POINTER( 11 ) );
      break;
    case 'g': /* get */ /* Get docid */
      plugin_get_all( plug, sizeof( int ), process_next, GINT_TO_POINTER( 8 ) );
      break;
    case 'd':
      plugin_finish( plug );
      g_free( plug );
      break;
    }
}

void plugin_register( plugin *plug, plugin_callback_struct *callbacks, gint context )
{
  plug->callbacks = *callbacks;
  plugin_get_all( plug, 1, process_command, NULL );
  plugin_send_int( plug, context );
}
