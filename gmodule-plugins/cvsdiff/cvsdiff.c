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
#include <unistd.h>
#include <gtk/gtk.h>
#include <string.h>

#include "client.h"
#include <glib.h>

int main( int argc, char *argv[] )
{

  char buff[1025];
  int fdpipe[2];
  int pid;
  char *filename = NULL;
  int docid;
  int length;
  int context;
  client_info info;

  info.menu_location = "[Plugins]CVS Diff";

  context = client_init( &argc, &argv, &info );

  docid = client_document_current( context );
  filename = client_document_filename( docid );
  
  if( pipe( fdpipe ) == -1 )
    {
      exit( 1 );
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

  docid = client_document_new( context, "CVS difference" );

  length = 1;
  while( length > 0 )
    {
      buff[ length = read( fdpipe[0], buff, 1024 ) ] = 0;
      if( length > 0 )
	client_text_append( docid, buff, length );
    }

  client_document_show( docid );
  client_finish( context );
  
  exit(0);
}
