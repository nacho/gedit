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
#include "ctag.h"

#include "client.h"

static gint context;

void search(gpointer data, gpointer user_data)
{
	printf("searching for %s\n",((tagType *)data)->tagName);
}

int colorise( GList *list, gchar *thetext )
{
	printf("colorising...\n");
	g_list_foreach (list, search, thetext);
	return 0;
}

GList *addtag( GList *list, gchar *tagname, gchar *file_name, \
	gchar *excmd, gchar *xflags)
{
	tagType	*newTag;

	printf ("Adding Tag %s (%s)\n",tagname, file_name);
	newTag = (tagType *)malloc(sizeof(tagType));
	strcpy (newTag->tagName,tagname);
	strcpy (newTag->fileName,file_name);
	strcpy (newTag->exCmd,excmd);
	strcpy (newTag->xflags,xflags);
	list = g_list_append(list, newTag);
	return list;
}

GList *readtags( gchar *filename, GList *taglist )
{
	gchar	*dirname,*tagfile, \
		buf[1024], \
		tagname[256],file_name[256],excmd[256],xflags[256];
	FILE	*fp;
	int	args;

	dirname = g_dirname( filename );
	tagfile = (gchar *)strcat( (char *)dirname, "/tags" );
	if ((fp = fopen ((char *)tagfile, "r")) == NULL)
		return NULL;
	
	while(fgets(buf, 1024, fp)) {
		args = sscanf(buf, "%s\t%s\t%s\t%s\n", \
			tagname, file_name, excmd, xflags);
		if(!strncmp(tagname,"!_",2))
			continue;
		taglist = addtag (taglist, tagname, file_name, excmd, xflags);
	}
	fclose (fp);
	return taglist;
}

int main( int argc, char *argv[] )
{
	int docid;
	int length;
	gchar *buff,*filename,*doctext;
	client_info info = empty_info;
	GList	*tagList=NULL;
  
	/* Location of the plugin in the 'Plugin's' menu in gEdit */
	info.menu_location = "[Plugins]Ctag";
  
	/* Initialisation of the Plugin itself, checking if being
	   run as a plugin (ie, fromg gEdit, not from command line */	
	context = client_init (&argc, &argv, &info);
	docid = client_document_current (context);
	length = strlen (buff = client_text_get (docid));

	/* Get the current document's filename, we need to find the
	   dirname */
	filename = client_document_filename (docid);	

	/* Now get the document text */
	doctext = client_text_get (docid);
   	tagList = readtags (filename, tagList);
    	
	if (colorise (tagList, doctext))
		printf ("couldn't colorise\n");	

	g_list_free (tagList);
	client_finish (context);
 
	exit(0);
}
