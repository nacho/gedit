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

/* Roberto : no utilices defines que sean comunes puesto que se pueden duplicar,
   cambia este por GEDIT_PLUGIN_SHELL_BUFFER_SIZE */
#define BUFFER_SIZE 1024

static GtkWidget *directory;
static GtkWidget *command;
static GtkWidget *dialog;

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

	Document *doc              = gedit_document_current ();
	gchar    *buffer_in        = NULL ;
	GString  *buffer_out       = g_string_new (NULL) ;
	gchar    *command_string   = NULL ;
	gchar    *directory_string = NULL ;
	gchar    **arg             = NULL ;
	gint     fdpipe [2];
	gint     pid ;
	gint     buffer_length ;
	gint     position ;
	
	
	if ((command_string = gtk_entry_get_text (GTK_ENTRY (command)))==NULL)
	{
		return ;
	}

	if ( (directory_string = gtk_entry_get_text (GTK_ENTRY (directory))) == NULL ) 
	{
		directory_string =  gnome_config_get_string ("/Editor_Plugins/shell_output/directory");
	}
	else
	{
		 gnome_config_set_string ("/Editor_Plugins/shell_output/directory", directory_string );
		 gnome_config_sync ();
	}
	
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

		chdir (directory_string);
			
		execvp (*arg,arg);
		
		_exit (1);
	}
	close (fdpipe[1]);

	g_strfreev (arg);

	buffer_length = 1;
	
	while (buffer_length > 0)
	{

		buffer_in = g_malloc (BUFFER_SIZE);
		memset ( buffer_in , 0 , BUFFER_SIZE );
		
		if ( (buffer_length  = read (fdpipe[0], buffer_in , BUFFER_SIZE)) > 0  )
		{
			buffer_out = g_string_append ( buffer_out , buffer_in );
			
		}

		g_free ( buffer_in ) ;
	}
	
	position = gedit_view_get_position (gedit_view_current ());
	 
	gedit_document_insert_text (doc, buffer_out->str , position , TRUE);

	gedit_view_set_position (gedit_view_current (), position + buffer_out->len);
			
	gnome_dialog_close (GNOME_DIALOG (dialog));

	g_string_free ( buffer_out , TRUE ) ;

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

     gtk_entry_set_text (GTK_ENTRY (directory) ,  gnome_config_get_string ("/Editor_Plugins/shell_output/directory") ) ;
     
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

	gnome_config_set_string ("/Editor_Plugins/shell_output/directory", g_get_current_dir () );
	
	return PLUGIN_OK;
}
