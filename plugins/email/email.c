/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * Email plugin.
 * Alex Roberts <bse@error.fsnet.co.uk>
 * Chema Celorio <chema@celorio.com>
 *
 */
 
#include <config.h>
#include <gnome.h>
#include <glade/glade.h>

#include "document.h"
#include "plugin.h"
#include "window.h"
#include "utils.h"

static GtkWidget *from_entry, *subject_entry, *to_entry, *sendmail_label;

static void
gedit_plugin_email_destroy (PluginData *pd)
{
	g_free (pd->name);
}

static void
gedit_plugin_email_finish (GtkWidget *widget, gpointer data)
{
	gnome_dialog_close (GNOME_DIALOG (widget));
}

static void
gedit_plugin_email_execute (GtkWidget *widget, gint button, gpointer data)
{
	Document *doc = gedit_document_current();
	FILE *sendmail;
	gchar *subject, *from, *to, *command;
	guchar * buffer;
	gchar *mailer_location = NULL;

	if (button == 0)
	{
		to = gtk_entry_get_text (GTK_ENTRY (to_entry));
		from = gtk_entry_get_text (GTK_ENTRY (from_entry));
		subject = gtk_entry_get_text (GTK_ENTRY (subject_entry));
		mailer_location = g_strdup (GTK_LABEL(sendmail_label)->label);
		g_return_if_fail (mailer_location != NULL);
		command = g_strdup_printf ("%s %s", mailer_location, to);
		g_free (mailer_location);

		gedit_flash_va (_("Executing command : %s"), command);
		
		if (!from || strlen (from) == 0 || !to || strlen (to)==0)
		{
			GnomeDialog *error_dialog;
			error_dialog = GNOME_DIALOG (gnome_error_dialog_parented ("Please provide a valid email address.",
								    gedit_window_active()));
			gnome_dialog_run_and_close (error_dialog);
			gdk_window_raise (widget->window);
			return;
		}

		if ((sendmail = popen (command, "w")) == NULL)
		{
			g_warning ("Couldn't open stream to %s\n", mailer_location);
			g_free (command);
			return;
		}
	    
		fprintf (sendmail, "To: %s\n", to);
		fprintf (sendmail, "From: %s\n", from);
		fprintf (sendmail, "Subject: %s\n", subject);
		fprintf (sendmail, "X-Mailer: gedit email plugin v 0.2\n");
		fflush (sendmail);
		
		buffer = gedit_document_get_buffer (doc);
		fprintf (sendmail, "%s\n", buffer);
		g_free (buffer);
		
		fflush (sendmail);
		pclose (sendmail);
	    
		gnome_config_set_string ("/gedit/email_plugin/From", from);
		gnome_config_sync ();
		
		g_free (command);
	}

	gnome_dialog_close (GNOME_DIALOG (widget));
}

static void
gedit_plugin_email_change_location (GtkWidget *button, gpointer userdata)
{
	GtkWidget *dialog;
	GtkWidget *label;
	gchar * new_location;

	gedit_debug ("start", DEBUG_PLUGINS);
	dialog = userdata;

	/* xgettext translators : !!!!!!!!!!!---------> the name of the plugin only.
	   it is used to display "you can not use the [name] plugin without this program... */
	new_location = gedit_plugin_program_location_change ("sendmail", _("email"));

	if ( new_location == NULL)
	{
		gdk_window_raise (dialog->window);
		return;
	}

	/* We need to update the label */
	label  = gtk_object_get_data (GTK_OBJECT (dialog), "sendmail_label");
	g_return_if_fail (label!=NULL);
	gtk_label_set_text (GTK_LABEL (label),
			    new_location);
	g_free (new_location);

	gdk_window_raise (dialog->window);	

	gedit_debug ("end", DEBUG_PLUGINS);
}


static void
gedit_plugin_email_create_dialog (void)
{
	GladeXML *gui;
	GtkWidget *dialog;
	GtkWidget *filename_label;
	GtkWidget *change_location;
	Document *doc = gedit_document_current ();
	gchar *username, *fullname, *hostname;
	gchar *from;
	gchar *filename_label_label;
	gchar *sendmail_label_label;
	gchar *mailer_location;

	if (!doc)
	     return;
	
        /* xgettext translators : !!!!!!!!!!!---------> the name of the plugin only.
	 it is used to display "you can not use the [name] plugin without this program... */
	mailer_location = gedit_plugin_program_location_get ("sendmail",  _("email"), FALSE);
	
	if (mailer_location == NULL)
		return;
	
	if (!g_file_exists (GEDIT_GLADEDIR "/email.glade"))
	{
		g_warning ("Could not find %s", GEDIT_GLADEDIR "/email.glade");
		return;
	}

	gui = glade_xml_new (GEDIT_GLADEDIR "/email.glade", NULL);

	if (!gui)
	{
		g_warning ("Could not find email.glade");
		return;
	}

	dialog          = glade_xml_get_widget (gui, "dialog");
	to_entry        = glade_xml_get_widget (gui, "to_entry");
	from_entry      = glade_xml_get_widget (gui, "from_entry");
	subject_entry   = glade_xml_get_widget (gui, "subject_entry");
	filename_label  = glade_xml_get_widget (gui, "filename_label");
	sendmail_label  = glade_xml_get_widget (gui, "sendmail_label");
	change_location = glade_xml_get_widget (gui, "change_button");

	g_return_if_fail (dialog != NULL);
	g_return_if_fail (to_entry != NULL);
	g_return_if_fail (from_entry  != NULL);
	g_return_if_fail (subject_entry != NULL);
	g_return_if_fail (filename_label != NULL);
	g_return_if_fail (sendmail_label != NULL);
	g_return_if_fail (change_location != NULL);


	username = g_get_user_name ();
	fullname = g_get_real_name ();
	hostname = getenv ("HOSTNAME");

	if (gnome_config_get_string ("/gedit/email_plugin/From"))
	{
		gtk_entry_set_text (GTK_ENTRY (glade_xml_get_widget (gui, "from_entry")), 
				    gnome_config_get_string ("/gedit/email_plugin/From"));
	}
	else if (fullname && hostname)
	{
		from = g_strdup_printf ("%s <%s@%s>",
					fullname,
					username,
					hostname);

		gtk_entry_set_text (GTK_ENTRY (glade_xml_get_widget (gui, "from_entry")),
				    from);
		g_free (from);
	}

	/* Set the subject entry box */
	if (doc->filename)
		gtk_entry_set_text (GTK_ENTRY (subject_entry), g_basename(doc->filename));
	else
		gtk_entry_set_text (GTK_ENTRY (subject_entry), _("Untitled"));

	/* Set the filename label */
	if (doc->filename)
		filename_label_label = g_strdup (g_basename(doc->filename));
	else
		filename_label_label = g_strdup (_("Untitled"));
	gtk_label_set_text (GTK_LABEL (filename_label),
			    filename_label_label);
	g_free (filename_label_label);


	
        /* Set the sendmail location label */
	gtk_object_set_data (GTK_OBJECT (dialog), "sendmail_label", sendmail_label);
	sendmail_label_label = g_strdup (mailer_location);
	gtk_label_set_text (GTK_LABEL (glade_xml_get_widget (gui, "sendmail_label")),
			    sendmail_label_label);
	g_free (sendmail_label_label);
	

	/* Connect the signals */
	gtk_signal_connect (GTK_OBJECT (dialog), "clicked",
			    GTK_SIGNAL_FUNC (gedit_plugin_email_execute), NULL);
	gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
			    GTK_SIGNAL_FUNC (gedit_plugin_email_finish), NULL);
	gtk_signal_connect (GTK_OBJECT (change_location), "clicked",
			    GTK_SIGNAL_FUNC (gedit_plugin_email_change_location), dialog);

	/* Set the dialog parent and modal type */ 
	gnome_dialog_set_parent (GNOME_DIALOG (dialog),
				 gedit_window_active());
	gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);

	/* Show everything then free the GladeXML memmory */
	gtk_widget_show_all (dialog);
	gtk_object_unref (GTK_OBJECT (gui));
}

gint
init_plugin (PluginData *pd)
{
	/* initialise */
	pd->destroy_plugin = gedit_plugin_email_destroy;
	pd->name = _("Email");
	pd->desc = _("Email the current document");
	pd->author = "Alex Roberts <bse@error.fsnet.co.uk>";
	
	pd->private_data = (gpointer)gedit_plugin_email_create_dialog;

	return PLUGIN_OK;
}
