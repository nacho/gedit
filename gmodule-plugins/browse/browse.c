/* browse.c - web browse plugin.
 *
 * Copyright (C) 1998 Alex Roberts
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
#include "client.h"

static GtkWidget *entry1;
static GtkWidget *entry2;
static gint context;



void goLynx( GtkWidget *widget, gpointer data )
{
  char buff[1025];
  int fdpipe[2];
  int pid;
  char *url[1] = { NULL };
  int docid;
  int length;

  url[0] = gtk_entry_get_text( GTK_ENTRY( entry1 ) );

  if( pipe( fdpipe ) == -1 )
    {
      _exit( 1 );
    }
  
  pid = fork();
  if ( pid == 0 )
    {
      /* New process. */
      char *argv[4];

      close( 1 );
      dup( fdpipe[1] );
      close( fdpipe[0] );
      close( fdpipe[1] );
      
      argv[0] = g_strdup( "lynx" );
      argv[1] = "-dump";
      argv[2] = url[0];
      argv[3] = NULL;
      execv( "/usr/bin/lynx", argv );
      /* This is only reached if something goes wrong. */
      _exit( 1 );
    }
  close( fdpipe[1] );

  docid = client_document_new( context, "lynxed" );
  
  length = 1;
  while( length > 0 )
    {
      buff[ length = read( fdpipe[0], buff, 1024 ) ] = 0;
      if( length > 0 )
	{
	  client_text_append( docid, buff, length );
	}
    }
  client_document_show( docid );
  client_finish( context );
  gtk_main_quit();
}

static void done( GtkWidget *widget, gpointer data )
{
  client_finish(context);
  gtk_main_quit();
}

int main( int argc, char *argv[] )
{
  GtkWidget *label;
  GtkWidget *hbox;
  GtkWidget *button;
  GtkWidget *dialog;
  client_info info = empty_info;

  info.menu_location = "[Plugins]Browse";

  context = client_init( &argc, &argv, &info );
  
  gtk_init( &argc, &argv );

  dialog = gtk_dialog_new();
  gtk_window_set_title( GTK_WINDOW( dialog ), "The gEdit Web Browse Plugin" );
  gtk_widget_set_usize (GTK_WIDGET (dialog), 353, 100);
  gtk_signal_connect( GTK_OBJECT( dialog ), "destroy", GTK_SIGNAL_FUNC( done ), NULL );
  gtk_container_border_width( GTK_CONTAINER( GTK_DIALOG( dialog )->vbox ), 10 );

  label = gtk_label_new( "       Url:  " );
  gtk_box_pack_start( GTK_BOX( GTK_DIALOG( dialog )->vbox ), label, FALSE, FALSE, 0 );

  hbox = gtk_hbox_new( FALSE, 0 );
  gtk_box_pack_start( GTK_BOX( GTK_DIALOG( dialog )->vbox ), hbox, FALSE, FALSE, 0 );
  
  entry1 = gtk_entry_new();
  gtk_box_pack_start( GTK_BOX( hbox ), entry1, TRUE, TRUE, 0 );


  button = gtk_button_new_with_label( "Go!" );
  gtk_signal_connect( GTK_OBJECT( button ), "clicked", GTK_SIGNAL_FUNC( goLynx ), NULL );
  gtk_box_pack_start( GTK_BOX( GTK_DIALOG( dialog )->action_area ), button, FALSE, TRUE, 0 );

  button = gtk_button_new_with_label( "Cancel" );
  gtk_signal_connect( GTK_OBJECT( button ), "clicked", GTK_SIGNAL_FUNC( done ), NULL );
  gtk_box_pack_start( GTK_BOX( GTK_DIALOG( dialog )->action_area ), button, FALSE, TRUE, 0 );

  gtk_widget_show_all( dialog );

  gtk_main();

  exit( 0 );
}
