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

/**
 * gedit_plugin_email_sendmail_location_dialog:
 * @void: 
 * 
 * it displays and pop-up a dialog so that the user can specify a
 * location for sendmail.
 * 
 * Return Value: a string with the path specified by the user
 **/
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

	gnome_dialog_set_parent (GNOME_DIALOG(sendmail_location_dialog), gedit_window_active());
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

	gtk_object_unref (GTK_OBJECT (gui));

	return location;
}

static gchar*
gedit_plugin_email_get_sendmail (void)
{
	gchar * sendmail = NULL;

	/* get the program pointed by the config */
	if (gnome_config_get_string ("/gedit/email_plugin/sendmail_location"))
		sendmail = g_strdup (gnome_config_get_string ("/gedit/email_plugin/sendmail_location"));

	/* If there was a program in the config, but it is no good. Clear "sendmail" */
	if (sendmail)
		if (gedit_utils_is_program (sendmail, "sendmail") != GEDIT_PROGRAM_OK)
		{
			g_free (sendmail);
			sendmail = NULL;
			gnome_config_set_string ("/gedit/email_plugin/sendmail_location", "");
			gnome_config_sync ();
		}

	/* If we have no 1st choice yet, get from path */
	if (!sendmail)
		sendmail = g_strdup (gnome_is_program_in_path ("sendmail"));

	/* If it isn't on path, look for common places */
	if (!sendmail) {
		if (g_file_exists ("/usr/sbin/sendmail"))
			sendmail = g_strdup ("/usr/sbin/sendmail");
		else if (g_file_exists ("/usr/lib/sendmail"))
			sendmail = g_strdup ("/usr/lib/sendmail");
	}

	return sendmail;
}

static gchar *
gedit_plugin_email_get_mailer_location (void)
{
	gchar* sendmail = NULL;
	gint error_code;

	sendmail = gedit_plugin_email_get_sendmail ();
	
	/* While "sendmail" is not valid, display error messages */
	while ((error_code = gedit_utils_is_program (sendmail, "sendmail"))!=GEDIT_PROGRAM_OK)
	{
		gchar *message = NULL;
		gchar *message_full;
		GtkWidget *dialog;

		if (sendmail == NULL)
			message = g_strdup_printf (_("The program sendmail could not be found.\n\n"));
		else if (error_code == GEDIT_PROGRAM_IS_INSIDE_DIRECTORY)
		{
			/* the user chose a directory and "sendmail" was found inside it */
			message = g_strdup (sendmail);
			if (sendmail)
				g_free (sendmail);
			sendmail = g_strdup_printf ("%s%s%s",
						    message,
						    (sendmail [strlen(sendmail)-1] == '/')?"":"/",
						    "sendmail");
			g_free (message);
			continue;
		}
		else if (error_code == GEDIT_PROGRAM_NOT_EXISTS)
			message = g_strdup_printf (_("'%s' doesn't seem to exist.\n\n"), sendmail);
		else if (error_code == GEDIT_PROGRAM_IS_DIRECTORY)
			message = g_strdup_printf (_("'%s' seems to be a directory.\n"
						     "but 'sendmail' could not be found inside it.\n\n"), sendmail);
		else if (error_code == GEDIT_PROGRAM_NOT_EXECUTABLE)
			message = g_strdup_printf (_("'%s' doesn't seem to be a program.\n\n"), sendmail);
		else
			g_return_val_if_fail (FALSE, NULL);

		message_full = g_strdup_printf (_("%sYou won't be able to use the email "
						"plugin without sendmail. \n Do you want to "
						"specify a new location for sendmail?"), message);
		
		dialog = gnome_question_dialog (message_full, NULL, NULL);
		gnome_dialog_set_parent (GNOME_DIALOG(dialog), gedit_window_active());

		g_free (message);
		g_free (message_full);
		if (GNOME_YES == gnome_dialog_run_and_close (GNOME_DIALOG (dialog)))
		{
			g_free (sendmail);
			sendmail = g_strdup (gedit_plugin_email_sendmail_location_dialog ());
			continue;
		}
		else
			return NULL;
	}

	gnome_config_set_string ("/gedit/email_plugin/sendmail_location", sendmail);
	gnome_config_sync ();
	
	return sendmail;
}

/* the function that actually does the work */
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
email (void)
{
	GladeXML *gui;
	GtkWidget *dialog;
	Document *doc = gedit_document_current ();
	gchar *username, *fullname, *hostname;
	gchar *from;
	gchar *filename_label;
	gchar *sendmail_label;

	if (!doc)
	     return;

	mailer_location = gedit_plugin_email_get_mailer_location();
	if (mailer_location == NULL)
		return;
	
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
	filename_label = GTK_LABEL (glade_xml_get_widget (gui, "filename_label"))->label;
	if (doc->filename)
		filename_label = g_strconcat (filename_label, doc->filename, NULL);
	else
		filename_label = g_strconcat (filename_label, _("Untitled"), NULL);
	gtk_label_set_text (GTK_LABEL (glade_xml_get_widget (gui, "filename_label")),
			    filename_label);
	g_free (filename_label);

	/* Set the sendmail location label */
	sendmail_label = GTK_LABEL (glade_xml_get_widget (gui, "sendmail_label"))->label;
	sendmail_label = g_strconcat (sendmail_label, mailer_location, NULL);
	gtk_label_set_text (GTK_LABEL (glade_xml_get_widget (gui, "sendmail_label")),
			    sendmail_label);
	g_free (sendmail_label);

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
