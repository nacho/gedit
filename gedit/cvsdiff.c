/* diff.c - diff plugin.
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include <string.h>

int fd;
int fdsend;
int fddata;

static int getnumber( int fd )
{
  int number = 0;
  int length = sizeof( int );
  char *buff = (char *) &number;
  buff += sizeof( int );
  while( length -= read( fd, buff - length, length ) )
    /* Empty statement */;
  return number;
}

static void putnumber( int fd, int number )
{
  write( fd, &number, sizeof( number ) );
}

int main( int argc, char *argv[] )
{

  char buff[1025];
  int fdpipe[2];
  int pid;
  char *filename = NULL;
  int docid;
  int count;
  int length;

  if( argc < 5 || strcmp( argv[1], "-go" ) )
    {
      printf( "Must be run as a plugin.\n" );
      _exit(1);
    }

  fd = atoi( argv[2] );
  fdsend = atoi( argv[3] );
  fddata = atoi( argv[4] );

  g_print ( "Requesting current: \n" ); 
  write( fdsend, "c", 1 );
  docid = getnumber( fddata );
  g_print ( "Recieved %d.\n", docid );

  g_print ( "Requesting filename: \n" );
  write( fdsend, "f", 1 );
  putnumber( fdsend, docid );
  length = getnumber( fddata );
  filename = g_malloc0( length + 1 );
  filename[ read( fddata, filename, length ) ] = 0;
  g_print ( "Recieved: %s.\n", filename );
  
  if( pipe( fdpipe ) == -1 )
    {
      _exit( 1 );
    } 
 
  pid = fork();
  if ( pid == 0 )
    {
      /* New process. */
      char *argv[4];

      if( strrchr( filename, '/' ) )
	{
	  *strrchr( filename, '/' ) = 0;
	  chdir( filename );
	  filename += strlen( filename );
	  filename ++;
	}

      close( 1 );
      dup( fdpipe[1] );
      close( fdpipe[0] );
      close( fdpipe[1] );
      
      argv[0] = g_strdup( "cvs" );
      argv[1] = g_strdup( "diff" );
      argv[2] = filename;
      argv[3] = NULL;
      execv( "/usr/bin/cvs", argv );
      /* This is only reached if something goes wrong. */
      _exit( 1 );
    }
  close( fdpipe[1] );

  write( fdsend, "n", 1 );
  docid = getnumber( fddata );

  count = 1;
  while( count > 0 )
    {
      buff[ count = read( fdpipe[0], buff, 1024 ) ] = 0;
      if( count > 0 )
	{
	  write( fdsend, "a", 1 );
	  putnumber( fdsend, docid );
	  putnumber( fdsend, count );
	  write( fdsend, buff, count );
	}
    }
  write( fdsend, "s", 1 );
  putnumber( fdsend, docid );
  
  _exit(0);
}
