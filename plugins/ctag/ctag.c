/* ctag.c - ctag plugin.
 *
 * Copyright (C) 1998 Fred Leeflang <fredl@dutchie.org.
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
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <glib.h>

#include "client.h"

static gint context;

int readtags( gchar *filename )
{
	gchar	*dirname,*tagfile;

	dirname = g_dirname( filename );
	tagfile = (gchar *)strcat( (char *)dirname, "/tags" );
	printf ("reading %s\n", tagfile);
	return 0;
}

int main( int argc, char *argv[] )
{
	int docid;
	int length;
	gchar *buff,*filename;
	client_info info = empty_info;
  
	/* Location of the plugin in the 'Plugin's' menu in gEdit */
	info.menu_location = "[Plugins]ctag";
  
	/* Initialisation of the Plugin itself, checking if being
	   run as a plugin (ie, fromg gEdit, not from command line */	
	context = client_init( &argc, &argv, &info );
	docid = client_document_current( context );
	length = strlen( buff = client_text_get( docid ));

	/* Get the current document's filename, we need to find the
	   dirname */
	filename = client_document_filename( docid );	
   	readtags( filename ); 
    	
	printf("This plugin is not doing anything yet\n");

	client_finish( context );
 
	exit(0);
}
