/* gEdit
 * Copyright (C) 1998 Alex Roberts and Evan Lawrence
 *
 * gE_print.c - Print Functions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
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

#ifndef WITHOUT_GNOME
#include <config.h>
#include <gnome.h>
#endif
#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>
#include <glib.h>
#include "main.h"
#include "commands.h"

GtkWidget *print_dialog;
GtkWidget *print_cmd_entry;

static void print_destroy( GtkWidget *widget, gpointer data )
{
    gtk_widget_destroy( print_dialog );
    print_dialog = NULL;
}

/*
static void print_show_lpr( GtkWidget *widget, gpointer data )
{
    gtk_widget_set_sensitive( e_print_other, FALSE );
    print_lpr = TRUE;
}


static void print_show_other( GtkWidget *widget, gpointer data )
{
    gtk_widget_set_sensitive( e_print_other, TRUE );
    print_lpr = FALSE;
}
*/

static void _file_print(gpointer cbdata )
{
    FILE *file;
    gE_document *current;
    long length,i;
    char filename[STRING_LENGTH_MAX];
    char tmpstring[STRING_LENGTH_MAX];
    char _tmpstring[STRING_LENGTH_MAX];
    char *ptr;
    char *var_ptr;
    
    gE_data *data = (gE_data *)cbdata;
    gE_window *window;
 	g_print("11..\n");       
 
 
    current = gE_document_current (window);
 	g_print("11..\n"); 
	strcpy(data->window->print_cmd,gtk_entry_get_text(GTK_ENTRY(print_cmd_entry)));

   	/* print using specified command */
        ptr = gtk_entry_get_text( GTK_ENTRY( print_cmd_entry ) );
        if( !ptr )
        {
            return;
        }

        /* parse for "file variable" */
        var_ptr = strstr( ptr, "%s" );
        if( !var_ptr )
        {
            return;
        }

        *var_ptr = ' ';
        var_ptr++;
        *var_ptr = ' ';

        var_ptr--;

        strcpy( _tmpstring, var_ptr );

        *var_ptr = '\0';

        /* write to temp file */
        strcpy( filename, "/tmp/" );
        sprintf( tmpstring, ".print.%d%d", (int) time(0), getpid() );
        strcat( filename, tmpstring );

        file = fopen( filename, "w" );
        if( file == NULL )
        {
            /* error dialog */
            sprintf( tmpstring, "\n    Unable to Open '%s'    \n", filename );
            g_error("umm... %s",tmpstring);
            return;
        }

        length = gtk_text_get_length( GTK_TEXT( current->text ) );

        for(i=1;i<=length;i++)
            fputc( gtk_editable_get_chars( GTK_EDITABLE( current->text ), i-1, i )[0], file );

        fclose( file );

        /* build string */
        strcpy( tmpstring, ptr );
        strcat( tmpstring, filename );
        strcat( tmpstring, _tmpstring );

        /* execute */
        system( tmpstring );
    }


/* ---- Print Function ---- */

void file_print_cmd_callback (GtkWidget *widget, gpointer cbdata)
{
GtkWidget *hbox;
GtkWidget *vbox;
GtkWidget *hsep;
GtkWidget *button;
GtkWidget *label;
GtkWidget *print_command;
GtkWidget *print_dialog;
char print[256];
gE_data *data = (gE_data *)cbdata;
/*
     file_save_cmd_callback(NULL,NULL);*/
     
     
/*
	strcpy(print, "");
	strcpy(print, data->window->print_cmd);

                      
	strcat(print, gE_document_current(data->window)->filename);
	system (print);   
	
   			gtk_label_set (GTK_LABEL(data->window->statusbar), ("File Printed..."));*/
   /*system("rm -f temp001");*/
   
   /* New Print Function -- It's a dialog! :) */
   g_print("hmm...\n");
    print_dialog = gtk_window_new( GTK_WINDOW_TOPLEVEL );
    gtk_widget_set_usize( print_dialog, 300, 175 );
    gtk_window_set_title( GTK_WINDOW( print_dialog ), "Print" );
    gtk_signal_connect( GTK_OBJECT( print_dialog ), "destroy",
                        GTK_SIGNAL_FUNC( print_destroy ), NULL );

    vbox = gtk_vbox_new( FALSE, 0 );
    gtk_container_add( GTK_CONTAINER( print_dialog ), vbox );
    gtk_widget_show( vbox );

    hbox = gtk_hbox_new( FALSE, 0 );
    gtk_box_pack_start( GTK_BOX( vbox ), hbox, FALSE, TRUE, 10 );
    gtk_widget_show( hbox );

    label = gtk_label_new( "Print Using: " );
    gtk_box_pack_start( GTK_BOX( hbox ), label, FALSE, TRUE, 5 );
    gtk_widget_show( label );

    hbox = gtk_hbox_new( FALSE, 0 );
    gtk_box_pack_start( GTK_BOX( vbox ), hbox, FALSE, TRUE, 5 );
    gtk_widget_show( hbox );

    print_command = gtk_label_new ("Print Command:");
    gtk_box_pack_start( GTK_BOX( hbox ), print_command, FALSE, TRUE, 5 );
   gtk_widget_show( print_command);

    print_cmd_entry = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(print_cmd_entry), data->window->print_cmd);
    gtk_box_pack_start( GTK_BOX( hbox ), print_cmd_entry, FALSE, TRUE, 10 );
    gtk_entry_set_editable(GTK_ENTRY(print_cmd_entry),TRUE);
    
    gtk_widget_show( print_cmd_entry );
    gtk_widget_set_sensitive( print_cmd_entry, FALSE );

    hsep = gtk_hseparator_new();
    gtk_box_pack_start( GTK_BOX( vbox ), hsep, FALSE, TRUE, 10 );
    gtk_widget_show( hsep );

    hbox = gtk_hbox_new( FALSE, 0 );
    gtk_box_pack_start( GTK_BOX( vbox ), hbox, FALSE, TRUE, 5 );
    gtk_widget_show( hbox );

    button = gtk_button_new_with_label( "OK" );
    gtk_box_pack_start( GTK_BOX( hbox ), button, TRUE, TRUE, 15 );
    gtk_signal_connect( GTK_OBJECT( button ), "clicked",
                        GTK_SIGNAL_FUNC( _file_print ), (gpointer)1 );
    gtk_widget_show( button );

    button = gtk_button_new_with_label( "Cancel" );
    gtk_box_pack_start( GTK_BOX( hbox ), button, TRUE, TRUE, 15 );
    gtk_signal_connect( GTK_OBJECT( button ), "clicked",
                        GTK_SIGNAL_FUNC( print_destroy ), NULL );
    gtk_widget_show( button );

    gtk_widget_show( print_dialog );

}