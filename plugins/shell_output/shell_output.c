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


#define GEDIT_PLUGIN_SHELL_BUFFER_SIZE 1024

static GtkWidget *directory;
static GtkWidget *command;
static GtkWidget *dialog;

static void
destroy_plugin (PluginData *pd)
{
}

static void
shell_output_help (GtkWidget *widget, gpointer data)
{
	static GnomeHelpMenuEntry help_entry = { "gedit", "plugins.html" };

	gnome_help_display (NULL, &help_entry);
}

static void
shell_output_finish (GtkWidget *w , gpointer data)
{
	gnome_dialog_close (GNOME_DIALOG (dialog));
}

static void
shell_output_scan_text (GtkWidget *w , gpointer data)
{

	GeditDocument *doc         = gedit_document_current ();
	gchar    *buffer_in        = NULL ;
	GString  *buffer_out       = g_string_new (NULL) ;
	gchar    *command_string   = NULL ;
	gchar    *directory_string = NULL ;
	gchar    **arg             = NULL ;
	gint     fdpipe [2];
	gint     pid ;
	gint     buffer_length ;
	gint     position ;
	

	command_string = gtk_entry_get_text (GTK_ENTRY (command));

	if (command_string == NULL || (strlen (command_string) == 0))
	{
		gedit_utils_error_dialog (_("The shell command entry is empty.\n\n"
					    "Please, insert a valid shell command."), data);
		return;
	}

	directory_string = gtk_entry_get_text (GTK_ENTRY (directory));
	
	if (directory_string == NULL || (strlen (directory_string) == 0)) 
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
		
		/* FIXME: do a better error report... PAOLO */
		g_warning ("A undetermined PIPE problem occurred");

		/* This is only reached if something goes wrong. */
		_exit (1);
	}
	close (fdpipe[1]);

	g_strfreev (arg);

	buffer_length = 1;
	
	while (buffer_length > 0)
	{

		buffer_in = g_malloc (GEDIT_PLUGIN_SHELL_BUFFER_SIZE);
		memset ( buffer_in , 0 ,GEDIT_PLUGIN_SHELL_BUFFER_SIZE );
		
		if ( (buffer_length  = read (fdpipe[0], buffer_in , GEDIT_PLUGIN_SHELL_BUFFER_SIZE)) > 0  )
		{
			buffer_out = g_string_append ( buffer_out , buffer_in );
			
		}

		g_free ( buffer_in ) ;
	}
	
	position = gedit_view_get_position (gedit_view_active ());
	 
	gedit_document_insert_text (doc, buffer_out->str , position , TRUE);

	gedit_view_set_position (gedit_view_active (), position + buffer_out->len);
			
	gnome_dialog_close (GNOME_DIALOG (dialog));

	g_string_free ( buffer_out , TRUE ) ;

}



static void
shell_output (void){

     GladeXML *gui;

     GtkWidget *ok;
     GtkWidget *cancel;
     GtkWidget *help;

     gchar *text;

     if (gedit_document_current () == NULL)
	     return;
     
     gui = glade_xml_new (GEDIT_GLADEDIR "/shell_output.glade",NULL);

     if (!gui)
     {
	     g_warning ("Could not find shell_output.glade");
	     return;
     }

     dialog     = glade_xml_get_widget (gui,"shell_output_dialog");
 
     ok         = glade_xml_get_widget (gui,"ok_button");
     cancel     = glade_xml_get_widget (gui,"cancel_button");
     help       = glade_xml_get_widget (gui,"help_button");
 
     command    = glade_xml_get_widget (gui,"command_entry");
     directory  = glade_xml_get_widget (gui,"directory_entry");

     g_return_if_fail (dialog    != NULL);
     g_return_if_fail (ok        != NULL);
     g_return_if_fail (cancel    != NULL);
     g_return_if_fail (help      != NULL);
     g_return_if_fail (command   != NULL);
     g_return_if_fail (directory != NULL);

     text = gnome_config_get_string ("/Editor_Plugins/shell_output/directory");
     gtk_entry_set_text (GTK_ENTRY (directory), text);
     g_free (text);
     
     gtk_signal_connect (GTK_OBJECT (ok), "clicked",
			 GTK_SIGNAL_FUNC(shell_output_scan_text), NULL);
     gtk_signal_connect (GTK_OBJECT (cancel), "clicked",
			 GTK_SIGNAL_FUNC(shell_output_finish), NULL);
     gtk_signal_connect (GTK_OBJECT (help), "clicked",
			 GTK_SIGNAL_FUNC(shell_output_help), NULL);
     gtk_signal_connect (GTK_OBJECT (dialog), "delete_event",
			 GTK_SIGNAL_FUNC(shell_output_finish), NULL);
     
     gnome_dialog_set_parent      (GNOME_DIALOG (dialog), gedit_window_active());
     gtk_window_set_modal         (GTK_WINDOW (dialog), TRUE);
     gnome_dialog_set_default     (GNOME_DIALOG (dialog), 0);
     gnome_dialog_editable_enters (GNOME_DIALOG (dialog), GTK_EDITABLE (command));

     gtk_widget_show_all (dialog);
     
     gtk_object_unref (GTK_OBJECT (gui));
}



gint
init_plugin (PluginData *pd)
{
	gchar *current_directory;
	
	/* initialize */
	pd->destroy_plugin = destroy_plugin;
	pd->name = _("Shell Output");
	pd->desc = _("Insert the shell output in the document");
	pd->long_desc = _("Insert the shell output in the document");
	pd->author = "Roberto Majadas <phoenix@nova.es>";
	pd->needs_a_document = TRUE;
	pd->modifies_an_existing_doc = TRUE;

	pd->private_data = (gpointer)shell_output;
	pd->installed_by_default = TRUE;

	current_directory = g_get_current_dir ();
	gnome_config_set_string ("/Editor_Plugins/shell_output/directory", current_directory);
	g_free (current_directory);
	
	return PLUGIN_OK;
}
