/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * Shell output plugin.
 * Roberto Majadas <phoenix@nova.es>
 *
 * Print the shell output in the current document
 */
 
#include <config.h>
#include <gnome.h>
#include <glade/glade.h>

#include "window.h"
#include "document.h"
#include "plugin.h"
#include "view.h"

#define BUFFER_SIZE 1024

static GtkWidget *directory;
static GtkWidget *command;
static GtkWidget *dialog;

static gchar *directory_entry_path ;

static void
destroy_plugin (PluginData *pd)
{
	g_free (pd->name);
}

static void
shell_output_finish (GtkWidget *w , gpointer data)
{
	gnome_dialog_close (GNOME_DIALOG (dialog));
}

static void
shell_output_scan_text (GtkWidget *w , gpointer data)
{

	Document *doc            = gedit_document_current ();
	gchar    *buffer         = NULL ;
	gchar    *command_string = NULL ;
	gchar    **arg           = NULL;
	gint     fdpipe [2];
	gint     pid ;
	gint     buffer_length_in , buffer_length_out ;
	gint     position ;
	
	if ((command_string = gtk_entry_get_text (GTK_ENTRY (command)))==NULL)
	{
		return ;
	}

	directory_entry_path = gtk_entry_get_text (GTK_ENTRY (directory)) ;
	printf ("%s\n",directory_entry_path);
	
	
	if (pipe (fdpipe) == -1)
	{
		_exit (1);
	}
  
	pid = fork();
	if (pid == 0)
	{
		/* New process. */

		close (1);
		dup (fdpipe[1]);
		close (fdpipe[0]);
		close (fdpipe[1]);

		arg = g_strsplit (command_string," ",-1);
		execvp (*arg,arg);
		_exit (1);
	}
	close (fdpipe[1]);
	g_strfreev (arg);

	buffer_length_in = 1;
	buffer_length_out = 0;
	buffer = g_malloc (BUFFER_SIZE);

	while (buffer_length_in > 0)
	{
		
		if ( (buffer_length_in = read (fdpipe[0], buffer , BUFFER_SIZE)) == BUFFER_SIZE )
		{
			
			buffer_length_out += buffer_length_in;
			buffer = g_realloc (buffer , buffer_length_out + BUFFER_SIZE );
		}
		else
		{
			buffer_length_out += buffer_length_in;
			buffer [buffer_length_out] = 0 ;
		}

	}
	
	position = gedit_view_get_position (gedit_view_current ());
	 
	gedit_document_insert_text (doc, buffer, position , TRUE);
			
	gnome_dialog_close (GNOME_DIALOG (dialog));
	g_free (buffer);
	

}



static void
shell_output (void){

     GladeXML *gui;

     GtkWidget *ok;
     GtkWidget *cancel;
    
     
     gui = glade_xml_new (GEDIT_GLADEDIR "/shell_output.glade",NULL);

     if (!gui)
     {
	     g_warning ("Could not find shell_output.glade");
	     return;
     }

     dialog     = glade_xml_get_widget (gui,"shell_output_dialog");
     ok         = glade_xml_get_widget (gui,"ok_button");
     cancel     = glade_xml_get_widget (gui,"cancel_button");
     command    = glade_xml_get_widget (gui,"command_entry");
     directory  = glade_xml_get_widget (gui,"directory_entry");

     gtk_entry_set_text (GTK_ENTRY (directory) , directory_entry_path ) ;
     
     gtk_signal_connect (GTK_OBJECT ( ok ) , "clicked" , GTK_SIGNAL_FUNC(shell_output_scan_text) , NULL);
     gtk_signal_connect (GTK_OBJECT ( command ) , "activate" , GTK_SIGNAL_FUNC(shell_output_scan_text) , NULL);
     gtk_signal_connect (GTK_OBJECT ( cancel ) , "clicked" , GTK_SIGNAL_FUNC(shell_output_finish) , NULL);
     gtk_signal_connect (GTK_OBJECT ( dialog ) , "delete_event" , GTK_SIGNAL_FUNC(shell_output_finish) , NULL);
     
     gnome_dialog_set_parent (GNOME_DIALOG (dialog), gedit_window_active());
     gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);

     gtk_widget_show_all (dialog);
     gtk_object_destroy (GTK_OBJECT (gui));

}



gint
init_plugin (PluginData *pd)
{
	/* initialise */
	pd->destroy_plugin = destroy_plugin;
	pd->name = _("Shell Output");
	pd->desc = _("Insert the shell output in the document");
	pd->author = "Roberto Majadas <phoenix@nova.es>";
	
	pd->private_data = (gpointer)shell_output;

	directory_entry_path = g_get_current_dir ();
	return PLUGIN_OK;
}
