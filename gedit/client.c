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
#ifdef DEBUG
  printf( "To: %d\n", number );
#endif
  return number;
}

static void putnumber( int fd, int number )
{
  write( fd, &number, sizeof( number ) );
#ifdef DEBUG
  printf( "From: %d\n", number );
#endif
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

  if( *argc > 5 && ! strcmp( (*argv)[5], "-query" ) )
    {
      
    }

  fd = atoi( (*argv)[2] );
  fdsend = atoi( (*argv)[3] );
  fddata = atoi( (*argv)[4] );

  return getnumber( fd );
}

gint client_document_current( gint context )
{
#ifdef DEBUG
  printf( "From: c\n" );
#endif
  write( fdsend, "c", 1 );
  putnumber( fdsend, context );
  return getnumber( fddata );
}

gchar *client_document_filename( gint docid )
{
  gchar *filename;
  gint length;
#ifdef DEBUG
  printf( "From: f\n" );
#endif
  write( fdsend, "f", 1 );
  putnumber( fdsend, docid );
  length = getnumber( fddata );
  filename = g_malloc0( length + 1 );
  filename[ read( fddata, filename, length ) ] = 0;
#ifdef DEBUG
  printf( "To: %s\n", filename );
#endif
  return filename;
}

gint client_document_new( gint context, gchar *title )
{
#ifdef DEBUG
  printf( "From: n\n" );
#endif
  write( fdsend, "n", 1 );
  putnumber( fdsend, context );
  putnumber( fdsend, strlen( title ) );
  write( fdsend, title, strlen( title ) );
#ifdef DEBUG
  printf( "From: %s\n", title );
#endif
  return getnumber( fddata );
}

gint client_document_open( gint context, gchar *title )
{
#ifdef DEBUG
  printf( "From: o\n" );
#endif
  write( fdsend, "o", 1 );
  putnumber( fdsend, context );
  putnumber( fdsend, strlen( title ) );
  write( fdsend, title, strlen( title ) );
#ifdef DEBUG
  printf( "From: %s\n", title );
#endif
  return getnumber( fddata );
}

void client_text_append( gint docid, gchar *buff, gint length )
{
#ifdef DEBUG
  printf( "From: a\n" );
#endif
  write( fdsend, "a", 1 );
  putnumber( fdsend, docid );
  putnumber( fdsend, length );
#ifdef DEBUG
  printf( "From: %s\n", buff );
#endif
  write( fdsend, buff, length );
}

gchar *client_text_get( gint docid )
{
  gchar *buffer;
  gint length;
#ifdef DEBUG
  printf( "From: g\n" );
#endif
  write( fdsend, "g", 1 );
  putnumber( fdsend, docid );
  length = getnumber( fddata );
  buffer = g_malloc0( length + 1 );
  buffer[ read( fddata, buffer, length ) ] = 0;
#ifdef DEBUG
  printf( "To: %s\n", buffer );
#endif
  return buffer;
}

void client_document_show( gint docid )
{
#ifdef DEBUG
  printf( "From: s\n" );
#endif
  write( fdsend, "s", 1 );
  putnumber( fdsend, docid );
}

void client_finish( gint context )
{
#ifdef DEBUG
  printf( "From: d\n" );
#endif
  write( fdsend, "d", 1 );
}

