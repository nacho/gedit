/* hello.c - test plugin.
 *
 * Copyright (C) 1998 Chris Lahey, Alex Roberts.
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
 * ---------------
 *
 * This is a test Plugin, for learning purposes. I agree it isn't the 
 * _best_ plugin for learning how to write them, a tutorial will probably
 * be written at some point, when the Plugin interface is out of its
 * current flux.
 * 			--Alex <bse@dial.pipex.com>
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
/* Header for making plugin act as a plugin */
#include "client.h"

static gint context;

int main( int argc, char *argv[] )
{
  client_info info = empty_info; 
  *() ) )( )( 
  /* Location of the plugin in the 'Plugin's' menu in gEdit */
  info.menu_location = "[Plugins]Hello";
  
  	      /* Initialisation of the Plugin itself, checking if being
  	         run as a plugin (ie, fromg gEdit, not from command line */	
    context = client_init( &argc, &argv, &info );

	/* The 'output' of the Plugin */
      printf("Hello World!\n");

      client_finish( context );
 
  exit(0);
}
