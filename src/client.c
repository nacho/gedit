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

static int getnumber( int fd )
{
  int number = 0;
  int length = sizeof( int );
  char *buff = (char *) &number;
  buff += sizeof( int );
  while( length -= read( fd, buff - length, length ) )
    /* Empty statement */;
  /*  printf( "To: %d\n", number );*/
  return number;
}

static void putnumber( int fd, int number )
{
  write( fd, &number, sizeof( number ) );
  /*  printf( "From: %d\n", number );*/
}

static int fd;
static int fdsend;
static int fddata;

gint client_init( gint *argc, gchar **argv[] )
{

  if( *argc < 5 || strcmp( (*argv)[1], "-go" ) )
    {
      printf( "Must be run as a plugin.\n" );
      _exit(1);
    }

  fd = atoi( (*argv)[2] );
  fdsend = atoi( (*argv)[3] );
  fddata = atoi( (*argv)[4] );

  return getnumber( fd );
}

gint client_document_current( gint context )
{
  /*  printf( "From: c\n" );*/
  write( fdsend, "c", 1 );
  putnumber( fdsend, context );
  return getnumber( fddata );
}

gchar *client_document_filename( gint docid )
{
  gchar *filename;
  gint length;
  /*  printf( "From: f\n" );*/
  write( fdsend, "f", 1 );
  putnumber( fdsend, docid );
  length = getnumber( fddata );
  filename = g_malloc0( length + 1 );
  filename[ read( fddata, filename, length ) ] = 0;
  /*  printf( "To: %s\n", filename );*/
  return filename;
}

gint client_document_new( gint context )
{
  /*  printf( "From: n\n" );*/
  write( fdsend, "n", 1 );
  putnumber( fdsend, context );
  return getnumber( fddata );
}

void client_text_append( gint docid, gchar *buff, gint length )
{
  /*  printf( "From: a\n" );*/
  write( fdsend, "a", 1 );
  putnumber( fdsend, docid );
  putnumber( fdsend, length );
  /*  printf( "From: %s\n", buff );*/
  write( fdsend, buff, length );
}

gchar *client_text_get( gint docid )
{
  gchar *buffer;
  gint length;
  /*  printf( "From: g\n" );*/
  write( fdsend, "g", 1 );
  putnumber( fdsend, docid );
  length = getnumber( fddata );
  buffer = g_malloc0( length + 1 );
  buffer[ read( fddata, buffer, length ) ] = 0;
  /*  printf( "To: %s\n", buffer );*/
  return buffer;
}

void client_document_show( gint docid )
{
  /*  printf( "From: s\n" );*/
  write( fdsend, "s", 1 );
  putnumber( fdsend, docid );
}

void client_finish( gint context )
{
  /*  printf( "From: d\n" );*/
  write( fdsend, "d", 1 );
}
