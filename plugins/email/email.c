/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * Email plugin.
 * Alex Roberts <bse@error.fsnet.co.uk>
 * Chema Celorio <chema@celorio.com>
 *
 * Prints "Hello World" into the current document
 */
 
#include <config.h>
#include <gnome.h>
#include <glade/glade.h>

#include "document.h"
#include "plugin.h"
#include "window.h"

#ifndef GEDIT_DEFAULT_MAILER_LOCATION
#define GEDIT_DEFAULT_MAILER_LOCATION "/usr/lib/sendmail"
#endif

static GtkWidget *from_entry, *subject_entry, *to_entry;
static gchar *mailer_location;

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



static gchar *
gedit_plugin_email_sendmail_location_dialog (void)
{
	GladeXML *gui;
	GtkWidget *sendmail_location_dialog;
	GtkWidget *sendmail_location_entry;
	gchar * location = NULL;

	gui = glade_xml_new (GEDIT_GLADEDIR "/sendmail.glade", NULL);

	if (!gui)
	{
		g_warning ("Could not find sendmail.glade");
		return NULL;
	}

	sendmail_location_dialog = glade_xml_get_widget (gui, "sendmail_location_dialog");
	sendmail_location_entry  = glade_xml_get_widget (gui, "sendmail_location_file_entry");

	if (!sendmail_location_dialog || !sendmail_location_entry)
	{
		g_warning ("Could not get the sendmail location dialog from email.glade");
		return NULL;
	}

	switch (gnome_dialog_run (GNOME_DIALOG (sendmail_location_dialog)))
	{
	case 0:
		location = gnome_file_entry_get_full_path(GNOME_FILE_ENTRY(sendmail_location_entry), FALSE);
		gnome_dialog_close (GNOME_DIALOG(sendmail_location_dialog));
		break;
	case 1:
		gnome_dialog_close (GNOME_DIALOG(sendmail_location_dialog));
	default:
		/* User pressed ESC or pressed NO*/
		location = NULL;
		break;
	}

	g_print ("outside of the loop !\n");
	
	gtk_object_unref (GTK_OBJECT (gui));

	return location;
}

static gchar *
gedit_plugin_email_get_mailer_location (void)
{
	gchar* sendmail = NULL;

	/* FIXME SAVE the location of sendmail for the next time !!! */
	sendmail = gnome_is_program_in_path ("sendmail");
	if (!sendmail) {
		if (g_file_exists ("/usr/sbin/sendmail"))
			sendmail = g_strdup ("/usr/sbin/sendmail");
		else if (g_file_exists ("/usr/lib/sendmail"))
			sendmail = g_strdup ("/usr/lib/sendmail");
		else
			sendmail = g_strdup (GEDIT_DEFAULT_MAILER_LOCATION);
	}

	/* We also need to test that
	   a) sendmail is not a directory and 
	   b) sendmail is executable
	   c) we have permision to write to sendmail */
	while (!sendmail || strlen(sendmail)==0 || !g_file_exists(sendmail))
	{
		gchar *message;
		GtkWidget *dialog;

		if (sendmail == NULL)
			message = g_strdup_printf (_("You won't be able to use the email "
						     "plugin without sendmail. \n Do you want to "
						     "specify a new location for sendmail?"));
		else
			message = g_strdup_printf (_("'%s' doesn't seem to exist.\n "
						     "You won't be able to use the email "
						     "plugin without sendmail. \n Do you want to "
						     "specify a new location for sendmail?"),
						   sendmail);
		
		dialog = gnome_question_dialog (message, NULL, NULL);
		gnome_dialog_set_parent (GNOME_DIALOG(dialog), gedit_window_active());

		g_free (message);
		if (GNOME_YES == gnome_dialog_run_and_close (GNOME_DIALOG (dialog)))
		{
			g_free (sendmail);
			sendmail = g_strdup (gedit_plugin_email_sendmail_location_dialog ());
			continue;
		}
		else
			return NULL;
	}
	return sendmail;
}

/* the function that actually does the wrok */
static void
email_clicked (GtkWidget *w, gint button, gpointer data)
{
	Document *doc = gedit_document_current();
	FILE *sendmail;
	gchar *subject, *from, *to, *command;
	guchar * buffer;

	if (button == 0)
	{
		to = gtk_entry_get_text (GTK_ENTRY (to_entry));
		from = gtk_entry_get_text (GTK_ENTRY (from_entry));
		subject = gtk_entry_get_text (GTK_ENTRY (subject_entry));

		command = g_strdup_printf ("%s %s", mailer_location, to);
		g_print ("Email command :%s\n", command);
	    
		if ((sendmail = popen (command, "w")) == NULL)
		{
			printf ("Couldn't open stream to /usr/bin/sendmail\n");
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
	    
		gnome_config_set_string ("/Editor_Plugins/Email/From", from);
		gnome_config_sync ();
		
		g_free (command);
	}

	gnome_dialog_close (GNOME_DIALOG (w));
}
	
static void
email (void)
{
	GladeXML *gui;
	GtkWidget *dialog;
	Document *doc = gedit_document_current ();
	gchar *username, *fullname, *hostname;
	gchar *from;
	gchar *filename_label;

	if (!doc)
	     return;

	mailer_location = gedit_plugin_email_get_mailer_location();
	if (mailer_location == NULL)
		return;
	
	g_print ("Mailer location :%s\n", mailer_location);
	
	gui = glade_xml_new (GEDIT_GLADEDIR "/email.glade", NULL);

	if (!gui)
	{
		g_warning ("Could not find email.glade");
		return;
	}

	dialog        = glade_xml_get_widget (gui, "dialog");
	to_entry      = glade_xml_get_widget (gui, "to_entry");
	from_entry    = glade_xml_get_widget (gui, "from_entry");
	subject_entry = glade_xml_get_widget (gui, "subject_entry");

	g_return_if_fail (dialog &&
			  to_entry &&
			  from_entry &&
			  subject_entry);

	username = g_get_user_name ();
	fullname = g_get_real_name ();
	hostname = getenv ("HOSTNAME");

	if (gnome_config_get_string ("/Editor_Plugins/Email/From"))
	{
		gtk_entry_set_text (GTK_ENTRY (glade_xml_get_widget (gui, "from_entry")), 
				    gnome_config_get_string ("/Editor_Plugins/Email/From"));
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

	filename_label = GTK_LABEL (glade_xml_get_widget (gui, "filename_label"))->label;

	if (doc->filename)
		filename_label = g_strconcat (filename_label, doc->filename, NULL);
	else
		filename_label = g_strconcat (filename_label, _("Untitled"), NULL);
	gtk_label_set_text (GTK_LABEL (glade_xml_get_widget (gui, "filename_label")),
			    filename_label);
	g_free (filename_label);
	
	/* Connect the signals */
	gtk_signal_connect (GTK_OBJECT (dialog), "clicked",
			    GTK_SIGNAL_FUNC (email_clicked), NULL);

	gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
			    GTK_SIGNAL_FUNC (email_finish), NULL);

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
