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


static GtkWidget *command;
static GtkWidget *dialog;

static void
destroy_plugin (PluginData *pd)
{
	g_free (pd->name);
}

static void
close_shell_output (void)
{
	gnome_dialog_close (GNOME_DIALOG (dialog));
}

static void
scan_output_text (void){

	Document *doc = gedit_document_current ();
	gchar buffer [1025];
	gchar *command_string=NULL ;
	gchar **arg=NULL;
	gint fdpipe [2];
	gint pid ;
	guint length , pos ;
	
	if ((command_string = gtk_entry_get_text (GTK_ENTRY (command)))==NULL)
	{
		return ;
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
		execvp (arg[0],arg);
		_exit (1);
	}
	close (fdpipe[1]);
	g_strfreev (arg);
	length = 1;
	pos = 0;
	while (length > 0)
	{
		buffer [ length = read (fdpipe[0], buffer, 1024) ] = 0;
		if (length > 0)
		{
		     	gedit_document_insert_text (doc, buffer, pos, FALSE);
			pos += length;
		}
	}

}

static void
scan_output_text_and_close (void){

	scan_output_text ();
	close_shell_output ();
}



static void
shell_output (void){

     GladeXML *gui;

     GtkWidget *ok;
     GtkWidget *apply;
     GtkWidget *cancel;
    
     
     gui = glade_xml_new (GEDIT_GLADEDIR "/shell_output.glade",NULL);

     if (!gui)
     {
	     g_warning ("Could not find shell_output.glade");
	     return;
     }

     dialog = glade_xml_get_widget (gui,"shell_output_dialog");
     ok = glade_xml_get_widget (gui,"ok_button");
     apply = glade_xml_get_widget (gui,"apply_button");
     cancel = glade_xml_get_widget (gui,"cancel_button");
     command = glade_xml_get_widget (gui,"command_entry");

     gtk_signal_connect (GTK_OBJECT ( ok ) , "clicked" , GTK_SIGNAL_FUNC(scan_output_text_and_close) , NULL);
     gtk_signal_connect (GTK_OBJECT ( apply ) , "clicked" , GTK_SIGNAL_FUNC(scan_output_text) , NULL);
     gtk_signal_connect (GTK_OBJECT ( cancel ) , "clicked" , GTK_SIGNAL_FUNC(close_shell_output) , NULL);
     gtk_signal_connect (GTK_OBJECT ( dialog ) , "delete_event" , GTK_SIGNAL_FUNC(close_shell_output) , NULL);

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
	
	return PLUGIN_OK;
}
