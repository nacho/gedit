/* convert.c - number conversion plugin.
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

void conv_hex( GtkWidget *widget, gpointer data )
{
  int start;
  char value[255];

  start = atoi( gtk_entry_get_text( GTK_ENTRY( entry1 ) ));
  
  sprintf(value,"%X",start);
  gtk_entry_set_text(GTK_ENTRY(entry2), value);
}


void conv_oct( GtkWidget *widget, gpointer data )
{
  int start;
  char value[255];

  start = atoi( gtk_entry_get_text( GTK_ENTRY( entry1 ) ));
  
  sprintf(value,"%o",start);
  gtk_entry_set_text(GTK_ENTRY(entry2), value);
}


void conv_dec( GtkWidget *widget, gpointer data )
{
  long start;
  char value[255];

  start = strtol(gtk_entry_get_text( GTK_ENTRY( entry1 )), NULL, 16);


  sprintf(value,"%d",start);
  gtk_entry_set_text(GTK_ENTRY(entry2), value);
}

int main( int argc, char *argv[] )
{
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *button;
  GtkWidget *dialog;
  GtkWidget *label;
  client_info info;

  info.menu_location = "[Plugins]Convert";

  context = client_init( &argc, &argv, &info );
  
  gtk_init( &argc, &argv );

  dialog = gtk_dialog_new();
  gtk_window_set_title( GTK_WINDOW( dialog ), "Number Converter" );
  gtk_signal_connect( GTK_OBJECT( dialog ), "destroy", gtk_main_quit, NULL );
  gtk_container_border_width( GTK_CONTAINER( GTK_DIALOG( dialog )->vbox ), 10 );


  hbox = gtk_hbox_new( FALSE, 5 );
  gtk_box_pack_start( GTK_BOX( GTK_DIALOG( dialog )->vbox ), hbox, FALSE, FALSE, 0 );

  label = gtk_label_new( "Value to be converted:" );  
    gtk_box_pack_start( GTK_BOX( hbox ), label, TRUE, TRUE, 0 );
  entry1 = gtk_entry_new();
  gtk_box_pack_start( GTK_BOX( hbox ), entry1, TRUE, TRUE, 0 );

  
  entry2 = gtk_entry_new();
  gtk_box_pack_start( GTK_BOX( hbox ), entry2, TRUE, TRUE, 0 );


  vbox = gtk_vbox_new (FALSE, 0);
  button = gtk_button_new_with_label( "Convert Decimal to Hex" );
  gtk_signal_connect( GTK_OBJECT( button ), "clicked", GTK_SIGNAL_FUNC( conv_hex ), NULL );
  gtk_box_pack_start( GTK_BOX( GTK_DIALOG( dialog )->vbox ), button, FALSE, TRUE, 0 );

  button = gtk_button_new_with_label( "Convert Decimal to Octal" );
  gtk_signal_connect( GTK_OBJECT( button ), "clicked", GTK_SIGNAL_FUNC( conv_oct ), NULL );
  gtk_box_pack_start( GTK_BOX( GTK_DIALOG( dialog )->vbox ), button, FALSE, TRUE, 0 );

  button = gtk_button_new_with_label( "Convert Hex to Decimal" );
  gtk_signal_connect( GTK_OBJECT( button ), "clicked", GTK_SIGNAL_FUNC( conv_dec ), NULL );
  gtk_box_pack_start( GTK_BOX( GTK_DIALOG( dialog )->vbox ), button, FALSE, TRUE, 0 );

  button = gtk_button_new_with_label( "Close" );
  gtk_signal_connect( GTK_OBJECT( button ), "clicked", gtk_main_quit, NULL );
  gtk_box_pack_start( GTK_BOX( GTK_DIALOG( dialog )->vbox), button, FALSE, TRUE, 0 );

  gtk_widget_show_all( dialog );

  gtk_main();

  exit( 0 );
}

