/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * Email plugin.
 * Alex Roberts <bse@error.fsnet.co.uk>
 *
 * Check and ask for the location of sendmail :
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
destroy_plugin (PluginData *pd)
{
	g_free (pd->name);
}

static void
email_finish (GtkWidget *w, gpointer data)
{
	gnome_dialog_close (GNOME_DIALOG (w));
}

/* the function that actually does the work */
static void
email_clicked (GtkWidget *w, gint button, gpointer data)
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
			gdk_window_raise (w->window);
			return;
		}

		if ((sendmail = popen (command, "w")) == NULL)
		{
			g_warning ("Couldn't open stream to /usr/bin/sendmail\n");
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

	gnome_dialog_close (GNOME_DIALOG (w));
}

static void
change_location_cb (GtkWidget *button, gpointer userdata)
{
	gchar * new_location;
	GtkWidget *dialog;
	GtkWidget *label;
	gchar * config_string;
	gchar * old_location = NULL;

	gedit_debug ("start", DEBUG_PLUGINS);

	dialog = userdata;

	/* Save a copy of the old location, in case the user cancels the dialog */
	config_string = gedit_plugin_program_location_string ("sendmail");
	if (gnome_config_get_string (config_string))
		old_location = g_strdup (gnome_config_get_string (config_string));
	g_free (config_string);

	/* xgettext translators : !!!!!!!!!!!---------> the name of the plugin only.
	   it is used to display "you can not use the [name] plugin without this program... */
	gedit_plugin_program_location_clear ("sendmail");
	new_location  = gedit_plugin_program_location_get ("sendmail",  _("email"), TRUE);

	g_print ("New location : *%s* !!!!!!!!!!!!\n", new_location);

	if (!new_location)
	{
		gdk_window_raise (dialog->window);
		if (old_location != NULL)
		{
			config_string = gedit_plugin_program_location_string ("sendmail");
			g_print ("Setting config string to %s\n", old_location);
			gnome_config_set_string (config_string, old_location);
			g_free (config_string);
			gnome_config_sync ();
			g_free (old_location);
		}
		gedit_debug ("return", DEBUG_PLUGINS);
		return;
	}

	/* We need to update the label */
	label  = gtk_object_get_data (GTK_OBJECT (dialog), "sendmail_label");
	g_return_if_fail (label!=NULL);
	gtk_label_set_text (GTK_LABEL (label),
			    new_location);
	g_free (new_location);
	g_free (old_location);
	
	gdk_window_raise (dialog->window);	

	gedit_debug ("end", DEBUG_PLUGINS);
}

static void
email (void)
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
	change_location = glade_xml_get_widget (gui, "change_location_button");

	g_return_if_fail (dialog &&
			  to_entry &&
			  from_entry &&
			  subject_entry &&
			  filename_label &&
			  sendmail_label &&
			  change_location );


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
			    GTK_SIGNAL_FUNC (email_clicked), NULL);
	gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
			    GTK_SIGNAL_FUNC (email_finish), NULL);
	gtk_signal_connect (GTK_OBJECT (change_location), "clicked",
			    GTK_SIGNAL_FUNC (change_location_cb), dialog);

	/* Set the dialog parent and modal type */ 
	gnome_dialog_set_parent (GNOME_DIALOG (dialog),
				 gedit_window_active());
	gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);

	/* Show everything then free the GladeXML memmory */
	gtk_widget_show_all (dialog);
	gtk_object_destroy (GTK_OBJECT (gui));
}

gint
init_plugin (PluginData *pd)
{
	/* initialise */
	pd->destroy_plugin = destroy_plugin;
	pd->name = _("Email");
	pd->desc = _("Email the current document");
	pd->author = "Alex Roberts <bse@error.fsnet.co.uk>";
	
	pd->private_data = (gpointer)email;

	return PLUGIN_OK;
}
