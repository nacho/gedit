/* client.c - libraries for acting as a plugin.
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
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "client.h"
#include <glib.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

client_info empty_info = { NULL, NULL };

static gchar *
get_physical_block( gint length, gint fd )
{
  gint bytes;
  gchar *buffer, *start;
  start = buffer = g_malloc0( length + 1 );
  while( length > 0 )
    {
      do
	{
	  bytes = read( fd, buffer, length );
	} while ( ( bytes == -1 ) && ( ( errno==EAGAIN ) || ( errno==EINTR ) ) );

      if ( bytes == -1 )
	{
	  g_warning( "Plugin: Error reading from application\n" );
	  return NULL;
	}

      if ( bytes == 0 )
	{
	  g_warning( "Plugin: Error EOF read?\n" );
	  return NULL;
	}

      length -= bytes;
      buffer += bytes;
    }
  return start;
}

static void
send_physical_block( gchar *buffer, gint length, gint fd )
{
  gint bytes;
  while( length > 0 )
    {
      do
	{
	  bytes = write( fd, buffer, length );
	} while ( ( bytes == -1 ) && ( ( errno==EAGAIN ) || ( errno==EINTR ) ) );

      if ( bytes == -1 )
	{
	  g_warning( "Plugin: Error writing to application\n" );
	  return;
	}

      if ( bytes == 0 )
	{
	  g_warning( "Plugin: Error EOF write?\n" );
	  return;
	}

      length -= bytes;
      buffer += bytes;
    }
  return;
}

static void
sendcommand( gchar command, gint fd )
{
  send_physical_block( &command, sizeof( command ), fd );
#ifdef DEBUG
  printf( "From: %c\n", command );
#endif
}

static int
getnumber( int fd )
{
  int number = 0;
  int length = sizeof( int );
  char *buff = (char *) &number;
  buff += sizeof( int );
  while( length -= read( fd, buff - length, length ) )
    /* Empty statement */;
#ifdef DEBUG
  printf( "To: %d\n", number );
#endif
  return number;
}

static gchar *
getblock( int fd )
{
  gchar *buffer = get_physical_block( getnumber( fd ), fd );
#ifdef DEBUG
  printf( "To: %s\n", buffer );
#endif
  return buffer;
}

static gboolean
getbool( int fd )
{
  unsigned char number = 0;
  if ( read( fd, &number, 1 ) )
    {
#ifdef DEBUG
      printf( "To: %s\n", number ? "TRUE" : "FALSE" );
#endif
      return number;
    }
  else return TRUE;
}

static void
sendnumber( int fd, int number )
{
  write( fd, &number, sizeof( number ) );
#ifdef DEBUG
  printf( "From: %d\n", number );
#endif
}

static void
sendblock( int fd, gchar *buffer, gint length )
{
  sendnumber( fd, length );
#ifdef DEBUG
  printf( "From: %s\n", buffer );
#endif
  send_physical_block( buffer, length, fd );
}

static int fd;
static int fdsend;
static int fddata;

gint client_init( gint *argc, gchar **argv[], client_info *info )
{
  int context;

  if( *argc < 5 || strcmp( (*argv)[1], "-go" ) )
    {
      printf( "Must be run as a plugin.\n" );
      _exit(1);
    }

  fd = atoi( (*argv)[2] );
  fdsend = atoi( (*argv)[3] );
  fddata = atoi( (*argv)[4] );

  context = getnumber( fd );

  if( *argc > 5 && ! strcmp( (*argv)[5], "--query" ) )
    {
      if( info->menu_location || info->suggested_accelerator)
	{
	  sendcommand( 'r', fdsend );
	  if( info->menu_location )
	    sendblock( fdsend, info->menu_location, strlen( info->menu_location ) );
	  else
	    sendblock( fdsend, "", 0 );
	  if( info->suggested_accelerator )	    
	    sendblock( fdsend, info->suggested_accelerator, strlen( info->suggested_accelerator ) );
	  else
	    sendblock( fdsend, "", 0 );
	}
      sendcommand( 'd', fdsend );
      exit( 0 );
    }
  argv[4] = argv[0];
  *argv += 4;
  *argc -= 4;

  return context;

}

gint client_document_current( gint context )
{
  sendcommand( 'c', fdsend );
  sendnumber( fdsend, context );
  return getnumber( fddata );
}

gchar *client_document_filename( gint docid )
{
  gchar *filename;
  sendcommand( 'f', fdsend );
  sendnumber( fdsend, docid );
  filename = getblock( fddata );
  return filename;
}

gint client_document_new( gint context, gchar *title )
{
  sendcommand( 'n', fdsend );
  sendnumber( fdsend, context );
  sendblock( fdsend, title, strlen( title ) );
  return getnumber( fddata );
}

gint client_document_open( gint context, gchar *title )
{
  sendcommand( 'o', fdsend );
  sendnumber( fdsend, context );
  sendblock( fdsend, title, strlen( title ) );
  return getnumber( fddata );
}

/* Returns true if document was actually closed. */
gboolean client_document_close( gint docid )
{
  sendcommand( 'l', fdsend );
  sendnumber( fdsend, docid );
  return getbool( fddata );
}

void client_text_append( gint docid, gchar *buff, gint length )
{
  sendcommand( 'a', fdsend );
  sendnumber( fdsend, docid );
  sendblock( fdsend, buff, length );
}

void client_text_insert( gint docid, gchar *buff, gint length, gint position )
{
  sendcommand( 'i', fdsend );
  sendnumber( fdsend, docid );
  sendnumber( fdsend, position );
  sendblock( fdsend, buff, length );
}

gchar *client_text_get( gint docid )
{
  sendcommand( 'g', fdsend );
  sendnumber( fdsend, docid );
  return getblock( fddata );
}

void client_document_show( gint docid )
{
  sendcommand( 's', fdsend );
  sendnumber( fdsend, docid );
}

void client_finish( gint context )
{
  sendcommand( 'd', fdsend );
}

/* Returns true if program actually quit. */
gboolean client_program_quit()
{
  sendcommand( 'q', fdsend );
  return getbool( fddata );
}

gint client_document_get_position( gint docid )
{
  sendcommand( 'p', fdsend );
  sendnumber( fdsend, docid );
  return getnumber( fddata );
}

gchar *client_text_get_selection_text( gint docid )
{
  sendcommand( 'e', fdsend );
  sendcommand( 't', fdsend );
  sendnumber( fdsend, docid );
  return getblock( fddata );
}

selection_range client_document_get_selection_range( gint docid )
{
  selection_range return_val;
  sendcommand( 'e', fdsend );
  sendcommand( 'r', fdsend );
  sendnumber( fdsend, docid );
  return_val.start = getnumber( fddata );
  return_val.end = getnumber( fddata );
  return return_val;
}

void client_text_set_selection_text( gint docid, gchar *buffer, gint length )
{
  sendcommand( 'e', fdsend );
  sendcommand( 's', fdsend );
  sendnumber( fdsend, docid );
  sendblock( fdsend, buffer, length );
}

void client_document_set_auto_indent( gint docid, gint auto_indent )
{
  sendcommand( 't', fdsend );
  sendcommand( 'i', fdsend );
  sendnumber( fdsend, docid );
  sendnumber( fdsend, auto_indent );
}

void client_document_set_status_bar( gint docid, gint status_bar )
{
  sendcommand( 't', fdsend );
  sendcommand( 'b', fdsend );
  sendnumber( fdsend, docid );
  sendnumber( fdsend, status_bar );
}

void client_document_set_word_wrap( gint docid, gint word_wrap )
{
  sendcommand( 't', fdsend );
  sendcommand( 'w', fdsend );
  sendnumber( fdsend, docid );
  sendnumber( fdsend, word_wrap );
}

void client_document_set_read_only( gint docid, gint read_only )
{
  sendcommand( 't', fdsend );
  sendcommand( 'r', fdsend );
  sendnumber( fdsend, docid );
  sendnumber( fdsend, read_only );
}

void client_document_set_split_screen( gint docid, gint split_screen )
{
  sendcommand( 't', fdsend );
  sendcommand( 't', fdsend );
  sendnumber( fdsend, docid );
  sendnumber( fdsend, split_screen );
}

void client_document_set_scroll_ball( gint docid, gint scroll_ball )
{
  sendcommand( 't', fdsend );
  sendcommand( 'r', fdsend );
  sendnumber( fdsend, docid );
  sendnumber( fdsend, scroll_ball );
}
