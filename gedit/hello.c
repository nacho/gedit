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

int main( int argc, char *argv[] )
{
  int fd = atoi( argv[2] );
  int undone = 1;
  char buff[1025];

  if( strcmp( argv[1], "-go" ) )
    {
      printf( "Must be run as a plugin.\n" );
      _exit(1);
    }
  else
    {
      while( undone > 0 )
	{
	  buff[ undone = read( fd, buff, 1024 ) ] = 0;
      printf( buff );
	}
    }
  _exit(0);
}
