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
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <dirent.h>
#include <gtk/gtk.h>
#include "plugin.h"

/* static void process_command( plugin *plug, gchar *buffer, int length, gpointer data ); */
/* static void *plugin_parse(plugin *plug); */
static void
process_command( plugin *plug, gchar *buffer, int length, gpointer data );

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

plugin *plugin_new_with_query( gchar *plugin_name, gboolean query )
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
      argv[0] = g_malloc0( 10 + strlen( new_plugin->name ) );
      sprintf( argv[0], "go-plugin-%s", new_plugin->name );
      argv[1] = g_strdup( "-go" );
      argv[2] = g_malloc0( 15 );
      g_snprintf( argv[2], 15, "%d", toline[0] );
      argv[3] = g_malloc0( 15 );
      g_snprintf( argv[3], 15, "%d", fromline[1] );
      argv[4] = g_malloc0( 15 );
      g_snprintf( argv[4], 15, "%d", dataline[0] );
      if ( query )
	{
	  argv[5] = g_strdup( "--query" );
	  argv[6] = NULL;
	}
      else
	argv[5] = NULL;
      if ( *plugin_name != '/' )
	{
	  gchar *temp = plugin_name;
	  plugin_name = g_malloc0( strlen( temp ) + strlen( PLUGINDIR ) + 2 );
	  sprintf( plugin_name, "%s/%s", PLUGINDIR, temp );
	}
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

plugin *plugin_new( gchar *plugin_name )
{
  return plugin_new_with_query( plugin_name, FALSE );
}

plugin *plugin_query( gchar *plugin_name )
{
  return plugin_new_with_query( plugin_name, TRUE );
}

plugin *plugin_new_with_param( gchar *plugin_name, int argc, gchar *arg[] )
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
      char **argv = g_malloc0 ( sizeof(char *) * argc + 6 );
      int i;

      close( new_plugin->pipe_to );
      close( new_plugin->pipe_from );
      close( new_plugin->pipe_data );
      argv[0] = g_malloc0( 10 + strlen( new_plugin->name ) );
      sprintf( argv[0], "go-plugin-%s", new_plugin->name );
      argv[1] = g_strdup( "-go" );
      argv[2] = g_malloc0( 15 );
      g_snprintf( argv[2], 15, "%d", toline[0] );
      argv[3] = g_malloc0( 15 );
      g_snprintf( argv[3], 15, "%d", fromline[1] );
      argv[4] = g_malloc0( 15 );
      g_snprintf( argv[4], 15, "%d", dataline[0] );
      for( i = 0; i < argc; i++ )
	{
	  argv[i + 5] = arg[i];
	}
      argv[5+argc] = NULL;
      if ( *plugin_name != '/' )
	{
	  gchar *temp = plugin_name;
	  plugin_name = g_malloc0( strlen( temp ) + strlen( PLUGINDIR ) + 2 );
	  sprintf( plugin_name, "%s/%s", PLUGINDIR, temp );
	}
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

void plugin_query_all( plugin_callback_struct *callbacks )
{
  DIR *dir = opendir( PLUGINDIR );
  struct dirent *direntry;
  gchar *shortname;

  if ( dir )
    {
      while ( ( direntry = readdir( dir ) ) )
	{
	  plugin *plug;
	  if ( strrchr( direntry->d_name, '/' ) )
	    shortname = strrchr( direntry->d_name, '/' ) + 1;
	  else
	    shortname = direntry->d_name;     
	  if ( strcmp( shortname, "." ) && strcmp( shortname, ".." ) )
	    {
	      plug = plugin_query( direntry->d_name );
	      plug->callbacks = *callbacks;
#if 0 		
	      plug->context = 0;
	      pthread_create( &plug->thread, NULL, (void *(*)(void *)) plugin_parse, plug );
#else
	      plugin_get_all( plug, 1, process_command, NULL ); 
	      plugin_send_int( plug, 0 );
#endif
	    }
	}
      closedir( dir );
    }
}


void plugin_finish( plugin *the_plugin )
{
  close( the_plugin->pipe_to );
  close( the_plugin->pipe_from );
  close( the_plugin->pipe_data );
  waitpid( the_plugin->pid, NULL, 0 );
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

void plugin_send_data_bool( plugin *the_plugin, gboolean bool)
{
  unsigned char ch = bool ? 1 : 0;
  write( the_plugin->pipe_data, &ch, sizeof( ch ) );
}

void plugin_real_get( plugin *the_plugin, gchar *buffer, gint length )
{
  gint bytes;
  gchar *start = buffer;
  while( length > 0 )
    {
      do
	{
	  bytes = read( the_plugin->pipe_from, buffer, length );
	} while ( ( bytes == -1 ) && ( ( errno==EAGAIN ) || ( errno==EINTR ) ) );

      if ( bytes == -1 )
	{
	  g_warning( "Go: Error reading from plugin." );
	  *start = 0;
	  return;
	}

      if ( bytes == 0 )
	{
	  g_warning( "Go: Error EOF read?" );
	  *start = 0;
	  return;
	}

      length -= bytes;
      buffer += bytes;
    }
}

void plugin_get( plugin *the_plugin, gchar *buffer, gint length )
{
  buffer[ length ] = 0;
  plugin_real_get( the_plugin, buffer, length );
}

void plugin_get_int( plugin *the_plugin, gint *number )
{
  plugin_real_get( the_plugin, (gchar *) number, sizeof( gint ) );
}

#if 0
static void *plugin_parse(plugin *plug)
{
  gchar command;
  gint docid;
  gint contextid;
  gint length;
  gint position;
  gchar *buffer;
  gchar *buffer2;
  plugin_info *info;

  plugin_send_int( plug, plug->context );

  while( 1 )
    {
      plugin_real_get( plug, &command, 1 );

      switch(command)
	{
	case 'a':
	  plugin_get_int( plug, &docid );
	  plugin_get_int( plug, &length );
	  buffer = g_malloc0( length + 1 );
	  plugin_get( plug, buffer, length );
	  {
	    gdk_threads_enter();
	    if ( plug->callbacks.text.append )
	      plug->callbacks.text.append( docid, buffer, length );
	    gdk_threads_leave();
	  }
	  g_free( buffer );
	  break;
	case 'c':
	  {
	    plugin_get_int( plug, &contextid );
	    gdk_threads_enter();
	    if ( plug->callbacks.document.current )
	      plugin_send_data_int( plug, plug->callbacks.document.current( contextid ) );
	    else
	      plugin_send_data_int( plug, 0 );
	    gdk_threads_leave();
	  }
	  break;
	case 'd':
	  plugin_finish( plug );
	  g_free( plug );
	  return NULL;                            /* Exit point. */
	case 'e':
	  plugin_real_get( plug, &command, 1 );
	  switch(command)
	    {
	    case 'r':
	      plugin_get_int( plug, &docid );
	      {
		selection_range selection = { 0, 0 };
		{
		  gdk_threads_enter();
		  if ( plug->callbacks.document.get_selection )
		    selection = plug->callbacks.document.get_selection( docid );
		  gdk_threads_leave();		
		}
		plugin_send_data_int( plug, selection.start );
		plugin_send_data_int( plug, selection.end );
	      }
	      break;
	    case 's':
	      plugin_get_int( plug, &docid );
	      plugin_get_int( plug, &length );
	      buffer = g_malloc0( length + 1 );
	      plugin_get( plug, buffer, length );
	      {
		gdk_threads_enter();
		if ( plug->callbacks.text.set_selected_text )
		  plug->callbacks.text.set_selected_text( docid, buffer, length );
		gdk_threads_leave();
	      }
	      g_free( buffer );
	      break;
	    case 't':
	      plugin_get_int( plug, &docid );
	      {
		gdk_threads_enter();
		if ( plug->callbacks.text.get_selected_text )
		  buffer = plug->callbacks.text.get_selected_text( docid );
		else
		  buffer = NULL;
		gdk_threads_leave();
	      }
	      if( buffer != NULL )
		{
		  plugin_send_data_with_length( plug, buffer, strlen( buffer ) );
		  g_free( buffer );
		}
	      else
		plugin_send_data_with_length( plug, "", 0 );
	      break;	      
	    }
	  break;
	case 'f':
	  plugin_get_int( plug, &docid );
	  {
	    gdk_threads_enter();
	    if ( plug->callbacks.document.filename )
	      {
		char *filename = plug->callbacks.document.filename( docid );
		plugin_send_data_with_length( plug, filename, strlen( filename ) );
	      }
	    else
	      plugin_send_data_with_length( plug, "", 0 );
	    gdk_threads_leave();
	  }
	  break;
	case 'g':
	  plugin_get_int( plug, &docid );
	  {
	    gdk_threads_enter();
	    if ( plug->callbacks.text.get )
	      buffer = plug->callbacks.text.get( docid );
	    else
	      buffer = NULL;
	    gdk_threads_leave();
	  }
	  if( buffer != NULL )
	    {
	      plugin_send_data_with_length( plug, buffer, strlen( buffer ) );
	      g_free( buffer );
	    }
	  else
	    plugin_send_data_with_length( plug, "", 0 );
	  break;
	case 'i':
	  plugin_get_int( plug, &docid );
	  plugin_get_int( plug, &position );
	  plugin_get_int( plug, &length );
	  buffer = g_malloc0( length + 1 );
	  plugin_get( plug, buffer, length );
	  {
	    gdk_threads_enter();
	    if ( plug->callbacks.text.insert )
	      plug->callbacks.text.insert( docid, buffer, length, position );
	    gdk_threads_leave();
	  }
	  g_free( buffer );
	  break;
	case 'l':
	  plugin_get_int( plug, &docid );
	  {
	    gdk_threads_enter();
	    if ( plug->callbacks.document.close )
	      {	
		plugin_send_data_bool( plug, plug->callbacks.document.close( docid ) );
	      }
	    else
	      plugin_send_data_bool( plug, FALSE );
	    gdk_threads_leave();
	  }
	  break;
	case 'n':
	  plugin_get_int( plug, &contextid );
	  plugin_get_int( plug, &length );
	  buffer = g_malloc0( length + 1 );
	  plugin_get( plug, buffer, length );
	  {
	    gdk_threads_enter();
	    if ( plug->callbacks.document.create )
	      plugin_send_data_int( plug, plug->callbacks.document.create( contextid, buffer ) );
	    else
	      plugin_send_data_int( plug, 0 );
	    gdk_threads_leave();
	  }
	  g_free( buffer );
	  break;
	case 'o':
	  plugin_get_int( plug, &contextid );
	  plugin_get_int( plug, &length );
	  buffer = g_malloc0( length + 1 );
	  plugin_get( plug, buffer, length );
	  {
	    gdk_threads_enter();
	    if ( plug->callbacks.document.open )
	      plugin_send_data_int( plug, plug->callbacks.document.open( contextid, buffer ) );
	    else
	      plugin_send_data_int( plug, 0 );
	    gdk_threads_leave();
	  }
	  g_free( buffer );
	  break;
	case 'p':
	  plugin_get_int( plug, &docid );
	  {
	    gdk_threads_enter();
	    if ( plug->callbacks.document.get_position )
	      {
		plugin_send_data_int( plug, plug->callbacks.document.get_position( docid ) );
	      }
	    else
	      plugin_send_data_int( plug, 0 );
	    gdk_threads_leave();
	  }
	  break;
	case 'q':
	  {
	    gdk_threads_enter();
	    if ( plug->callbacks.program.quit )
	      plugin_send_data_bool( plug, plug->callbacks.program.quit() );
	    else
	      plugin_send_data_bool( plug, FALSE );
	    gdk_threads_leave();
	  }
	  break;
	case 'r':
	  plugin_get_int( plug, &length );
	  buffer = g_malloc0( length + 1 );
	  plugin_get( plug, buffer, length );
	  plugin_get_int( plug, &length );
	  buffer2 = g_malloc0( length + 1 );
	  plugin_get( plug, buffer2, length );
	  {
	    gdk_threads_enter();
	    if ( plug->callbacks.program.reg )
	      {
		if( buffer[0] != '[' )
		  {
		    g_free( buffer );
		    break;
		  }
		buffer ++;
		if ( strchr( buffer, ']' ) == 0 )
		  {
		    g_free( -- buffer );
		    break;
		  }
		*strchr( buffer, ']' ) = 0;
		if ( strcmp( buffer, "Plugins" ) )
		  {
		    g_free( -- buffer );
		    break;
		  }
		info = g_malloc0( sizeof( plugin_info ) );
		info->type = PLUGIN_STANDARD;
		info->menu_location = buffer + strlen( buffer ) + 1;
		info->suggested_accelerator = buffer2;
		info->plugin_name = plug->name;
		plug->callbacks.program.reg( info );
		g_free( info );
		buffer --;
	      }
	    gdk_threads_leave();
	  }
	  g_free( buffer );
	  g_free( buffer2 );
	  break;
	case 's':
	  plugin_get_int( plug, &docid );
	  {
	    gdk_threads_enter();
	    if ( plug->callbacks.document.show )
	      plug->callbacks.document.show( docid );
	    gdk_threads_leave();
	  }
	  break;
	case 0:
	  g_warning("Go: Bad read.  Plugin died?");
	  plugin_finish( plug );
	  g_free( plug );
	  return NULL;                            /* Exit point. */	  
	default:
	  break;
	}
    }
  return NULL;
}

void plugin_register( plugin *plug, plugin_callback_struct *callbacks, gint context )
{
  plug->callbacks = *callbacks;
  plug->context = context;
  pthread_create( &plug->thread, NULL, (void *(*)(void *)) plugin_parse, plug );
}
#endif

void plugin_register( plugin *plug, plugin_callback_struct *callbacks, gint context )
{
  plug->callbacks = *callbacks;
  plugin_get_all( plug, 1, process_command, NULL ); 
  plugin_send_int( plug, context ); 
}

static void plugin_get_more( gpointer data, gint source, GdkInputCondition condition )
{
  partly_read *partly = (partly_read *) data;
  int count;

  partly->sofar += (count = read( partly->plug->pipe_from, partly->buff + partly->sofar, partly->length - partly->sofar ) );
  if( partly->length - partly->sofar == 0 )
    {
      gdk_input_remove( partly->incall );
      if( partly->finished )
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

void
plugin_get_all( plugin *plug, gint length, plugin_callback *finished, gpointer data )
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

static void
plugin_send_more( gpointer data, gint source, GdkInputCondition condition )
{
  partly_read *partly = (partly_read *) data;
  int count;
  
  partly->sofar += (count = write( source, partly->buff + partly->sofar, partly->length - partly->sofar ) );
  if( partly->length - partly->sofar == 0 )
    {
      gdk_input_remove( partly->incall );
      if( partly->finished )
	partly->finished( partly->plug, partly->buff, partly->length, partly->data );
      g_free( partly->buff );
      g_free( partly );
    }
  else if( count == -1 )
    {
      gdk_input_remove( partly->incall );
      g_free( partly->buff );
      g_free( partly );
    }
}

void
plugin_send_data_all_with_length( plugin *plug, gchar *buffer, gint length, plugin_callback *finished, gpointer data )
{
  partly_read *partly = g_malloc0( sizeof( partly_read ) );
  partly->plug = plug;
  partly->buff = g_malloc( length + sizeof( int ) + 1 );
  *( (int *) partly->buff) = length;
  strcpy( partly->buff + sizeof( int ), buffer );
  partly->length = length + sizeof( int );
  partly->sofar = 0;
  partly->incall = gdk_input_add( plug->pipe_data, GDK_INPUT_WRITE,
				  plugin_send_more, partly );
  partly->finished = finished;
  partly->data = data;
}

static void
plugin_get_command( plugin *plug, gchar *buffer, int length, gpointer data )
{
  plugin_get_all( plug, 1, process_command, NULL );
}

static void
process_next( plugin *plug, gchar *buffer, int length, gpointer data )
{
  plugin_parse_state *state = data;

  if( state->command_current_count < state->command_count )
    {
      state->command[state->command_current_count ++] = *buffer;
      switch( state->command[0] )
	{
	case 'e':
	  switch( state->command[1] )
	    {
	    case 'r': /* Fall through. */
	    case 't':
	      state->int_count = 1;
	      state->char_count = 0;
	      break;
	    case 's':
	      state->int_count = 1;
	      state->char_count = 1;
	      break;
	    }
	  break;
	case 't':
	  switch( state->command[1] )
	    {
	    case 'i': /* Fall through. */
	    case 'b': /* Fall through. */
	    case 'w': /* Fall through. */
	    case 'r': /* Fall through. */
	    case 't': /* Fall through. */
	    case 'c': /* Fall through. */
	      state->int_count = 2;
	      state->char_count = 0;
	      break;
	    }
	  break;
	case 0:
	  g_warning("Go: Bad read.  Plugin died?");
	  plugin_finish( plug );
	  g_free( plug );
	  return;                            /* Exit point. */	  
	  break;
	}
    }
  else if ( state->int_current_count < state->int_count )
    {
      state->ints[state->int_current_count ++] = *( (int *) buffer );
    }
  else if ( state->char_current_count < state->char_count )
    {
      if ( state->getting_int )
	{
	  state->getting_int = FALSE;
	  plugin_get_all( plug, *( (int *) buffer ), process_next, state );
	  return;   /* Exit point. */
	}
      else
	{
	  state->chars[state->char_current_count ++] = g_strdup( buffer );
	}
    }

  if( state->command_current_count < state->command_count )
    {
      plugin_get_all( plug, 1, process_next, state );
    }
  else if ( state->int_current_count < state->int_count )
    {
      plugin_get_all( plug, sizeof( gint ), process_next, state );
    }
  else if ( state->char_current_count < state->char_count )
    {
      state->getting_int = TRUE;
      plugin_get_all( plug, sizeof( gint ), process_next, state );
    }
  else
    {
      switch( state->command[0] )
	{
	case 'a':
	  if ( plug->callbacks.text.append )
	    plug->callbacks.text.append( state->ints[0], state->chars[0], strlen( state->chars[0] ) );
	  break;
	case 'c':
	  if ( plug->callbacks.document.current )
	    plugin_send_data_int( plug, plug->callbacks.document.current( state->ints[0] ) );
	  else
	    plugin_send_data_int( plug, 0 );
	  break;
	case 'e':
	  switch(state->command[1])
	    {
	    case 'r':
	      {
		selection_range selection = { 0, 0 };
		if ( plug->callbacks.document.get_selection )
		  selection = plug->callbacks.document.get_selection( state->ints[0] );
		plugin_send_data_int( plug, selection.start );
		plugin_send_data_int( plug, selection.end );
	      }
	      break;
	    case 's':
	      if ( plug->callbacks.text.set_selected_text )
		plug->callbacks.text.set_selected_text( state->ints[0], state->chars[0], strlen( state->chars[0] ) );
	      break;
	    case 't':
	      {
		char *buffer;
		if ( plug->callbacks.text.get_selected_text )
		  {
		    buffer = plug->callbacks.text.get_selected_text( state->ints[0] );
		  }
		else
		 buffer = NULL;
		if( buffer != NULL )
		  plugin_send_data_with_length( plug, buffer, strlen( buffer ) );
		else
		  plugin_send_data_with_length( plug, "", 0 );
		break;
	      }
	    }
	  break;
	case 't':
	  switch(state->command[1])
	    {
	    case 'i':
	      if ( plug->callbacks.document.set_auto_indent )
		plug->callbacks.document.set_auto_indent( state->ints[0], state->ints[1]);
	      break;
	    case 'b':
	      if ( plug->callbacks.document.set_status_bar )
		plug->callbacks.document.set_status_bar( state->ints[0], state->ints[1]);
	      break;
	    case 'w':
	      if ( plug->callbacks.document.set_word_wrap )
		plug->callbacks.document.set_word_wrap( state->ints[0], state->ints[1]);
	      break;
	    case 'r':
	      if ( plug->callbacks.document.set_read_only )
		plug->callbacks.document.set_read_only( state->ints[0], state->ints[1]);
	      break;
	    case 't':
	      if ( plug->callbacks.document.set_split_screen )
		plug->callbacks.document.set_split_screen( state->ints[0], state->ints[1]);
	      break;
	    case 'c':
	      if ( plug->callbacks.document.set_scroll_ball )
		plug->callbacks.document.set_scroll_ball( state->ints[0], state->ints[1]);
	      break;
	    }
	  break;
	case 'f':
	  if ( plug->callbacks.document.filename )
	    {
	      char *filename = plug->callbacks.document.filename( state->ints[0] );
	      plugin_send_data_with_length( plug, filename, strlen( filename ) );
	    }
	  else
	    plugin_send_data_with_length( plug, "", 0 );
	  break;
	case 'g':
	  {
	    char *buffer;
	    if ( plug->callbacks.text.get )
	      buffer = plug->callbacks.text.get( state->ints[0] );
	    else
	      buffer = NULL;
	    if( buffer != NULL )
	      plugin_send_data_with_length( plug, buffer, strlen( buffer ) );
	    else
	      plugin_send_data_with_length( plug, "", 0 );
	  }
	  break;
	case 'i':
	  if ( plug->callbacks.text.insert )
	    plug->callbacks.text.insert( state->ints[0], state->chars[0], strlen( state->chars[0] ), state->ints[1] );
	  break;
	case 'l':
	  if ( plug->callbacks.document.close )
	    plugin_send_data_bool( plug, plug->callbacks.document.close( state->ints[0] ) );
	  else
	    plugin_send_data_bool( plug, FALSE );
	  break;
	case 'n':
	  if ( plug->callbacks.document.create )
	    plugin_send_data_int( plug, plug->callbacks.document.create( state->ints[0], state->chars[0] ) );
	  else
	    plugin_send_data_int( plug, 0 );
	  break;
	case 'o':
	  if ( plug->callbacks.document.open )
	    plugin_send_data_int( plug, plug->callbacks.document.open( state->ints[0], state->chars[0] ) );
	  else
	    plugin_send_data_int( plug, 0 );
	  break;
	case 'p':
	  if ( plug->callbacks.document.get_position )
	    plugin_send_data_int( plug, plug->callbacks.document.get_position( state->ints[0] ) );
	  else
	    plugin_send_data_int( plug, 0 );
	  break;
	case 'r':
	  {
	    char *buffer = state->chars[0];
	    char *buffer2 = state->chars[1];
	    plugin_info *info;
	    if ( plug->callbacks.program.reg )
	      {
		if( buffer[0] != '[' )
		  break;
		buffer ++;
		if ( strchr( buffer, ']' ) == 0 )
		  break;
		*strchr( buffer, ']' ) = 0;
		if ( strcmp( buffer, "Plugins" ) )
		  break;
		info = g_malloc0( sizeof( plugin_info ) );
		info->type = PLUGIN_STANDARD;
		info->menu_location = buffer + strlen( buffer ) + 1;
		info->suggested_accelerator = buffer2;
		info->plugin_name = plug->name;
		plug->callbacks.program.reg( info );
		g_free( info );
	      }
	  }
	  break;
	case 's':
	  if ( plug->callbacks.document.show )
	    plug->callbacks.document.show( state->ints[0] );
	  break;
	case 0:
	  g_warning("Go: Bad read.  Plugin died?");
	  plugin_finish( plug );
	  g_free( plug );
	  return;                            /* Exit point. */	  
	default:
	  break;
	}
      {
	int i;
	for ( i = 0; i < state->char_count; i++ )
	  {
	    g_free( state->chars[ i ] );
	  }
      }
      g_free( state );
      plugin_get_all( plug, 1, process_command, NULL );
    }
}

static void
process_command( plugin *plug, gchar *buffer, int length, gpointer data )
{
  plugin_parse_state *state = g_new( plugin_parse_state, 1 );
  int command_count = 1, int_count = 0, char_count = 0;
  state->command_current_count = 1;
  state->int_current_count = 0;
  state->char_current_count = 0;
  state->command[0] = *buffer;
  switch( *buffer )
    {
    case 'a': /* Fall through. */
    case 'n': /* Fall through. */
    case 'o':
      command_count = 1;
      int_count = 1;
      char_count = 1;
      break;
    case 'c': /* Fall through. */
    case 'f': /* Fall through. */
    case 'g': /* Fall through. */
    case 'l': /* Fall through. */
    case 'p': /* Fall through. */
    case 's':
      command_count = 1;
      int_count = 1;
      char_count = 0;
      break;
    case 'r':
      command_count = 1;
      int_count = 0;
      char_count = 2;
      break;
    case 'i':
      command_count = 1;
      int_count = 2;
      char_count = 1;
      break;
    case 'd': /* Fall through. */
    case 'q':
      command_count = 1;
      int_count = 0;
      char_count = 0;
      break;
    case 'e': /* Fall through. */
    case 't':
      command_count = 2;
      break;
    case 0:
      g_warning("Go: Bad read.  Plugin died?");
      plugin_finish( plug );
      g_free( plug );
      return;                            /* Exit point. */
    default:
      g_warning("Go: Bad command.  Plugin malfunction?");
      plugin_finish( plug );
      g_free( plug );
      return;                            /* Exit point. */
    }
  state->command_count = command_count;
  state->int_count = int_count;
  state->char_count = char_count;
  if( state->command_current_count < state->command_count )
    {
      plugin_get_all( plug, 1, process_next, state );
    }
  else if ( state->int_current_count < state->int_count )
    {
      plugin_get_all( plug, sizeof( gint ), process_next, state );
    }
  else if ( state->char_current_count < state->char_count )
    {
      state->getting_int = TRUE;
      plugin_get_all( plug, sizeof( gint ), process_next, state );
    }
  else
    {
      switch( state->command[0] )
	{
	case 'd':
	  plugin_finish( plug );
	  g_free( plug );
	  return;
	  break;
	case 'q':
	  if ( plug->callbacks.program.quit )
	    plugin_send_data_bool( plug, plug->callbacks.program.quit() );
	  else
	    plugin_send_data_bool( plug, FALSE );
	  break;
	}
      g_free( state );
      plugin_get_all( plug, 1, process_command, NULL );
    }
}
