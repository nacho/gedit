/* hello.c - test plugin.
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
#include <stdlib.h>
#include <unistd.h>
#include <glib.h>

static int getnumber( int fd )
{
  int number = 0;
  int length = sizeof( int );
  char *buff = (char *) &number;
  buff += sizeof( int );
  while( length -= read( fd, buff - length, length ) )
    /* Empty statement */;
  number -= '0';
  return number;
}

static void putnumber( int fd, int number )
{
  write( fd, &number, sizeof( number ) );
}

int main( int argc, char *argv[] )
{
  int undone = 1;
  char buff[1025];
  int fd;
  int fdsend;
  int fddata;
  int fdpipe[2];
  int pid;
  int i = 0;
  char *filenames[2] = { NULL, NULL };
  int docid;
  int count;

  if( argc < 5 || strcmp( argv[1], "-go" ) )
    {
      printf( "Must be run as a plugin.\n" );
      _exit(1);
    }

  fd = atoi( argv[2] );
  fdsend = atoi( argv[3] );
  fddata = atoi( argv[4] );
  
  for( i = 0; i < 2; i++ )
    {
      char *buff;
      int length = getnumber( fd );
      g_print( "Parsed length: %d.\n", length );
      buff = g_malloc0( length + 1 );
#if 0
      buff += length;
      while( length -= read( fd, buff - length, length ) )
	/* Empty statement */;
#endif
      g_print( "Read %d characters.", read( fd, buff, length ) );
      buff[ length ] = 0;
      filenames[ i ] = buff;
      g_print( "Parsed filename: *%p = %d.\n", filenames[ i ], *filenames[ i ] );
    }
  g_print( "Creating pipe.\n" );
  
  if( pipe( fdpipe ) == -1 )
    {
      _exit( 1 );
    }
  
  pid = fork();
  if ( pid == 0 )
    {
      /* New process. */
      char *argv[4];

      g_print( "Redirecting stdout.\n" );
      close( 1 );
      dup( fdpipe[1] );
      close( fdpipe[0] );
      close( fdpipe[1] );
      
      argv[0] = g_strdup( "diff" );
      argv[1] = filenames[0];
      argv[2] = filenames[1];
      argv[3] = NULL;
      execv( "/usr/bin/diff", argv );
      /* This is only reached if something goes wrong. */
      _exit( 1 );
    }
  close( fdpipe[1] );
#if 0
  g_print( "Freeing file names: \n" );
  
  g_free( filenames[0] );

  g_print( "one done.\n" );
  
  g_free( filenames[1] );

  g_print( "both done.\n" );
#endif
  write( fdsend, "n", 1 );
  docid = getnumber( fddata );

  count = 1;
  while( count > 0 )
    {
      buff[ count = read( fdpipe[0], buff, 1024 ) ] = 0;
      g_print( "Read %d characters: %s", count, buff );
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
