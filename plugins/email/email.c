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
#include <libgnomevfs/gnome-vfs.h>

#include "document.h"
#include "plugin.h"
#include "window.h"
#include "utils.h"

#define GEDIT_PLUGIN_PROGRAM "sendmail"
/* xgettext translators: !!!!!!!!!!!---------> the name of the plugin only.
   it is used to display "you can not use the [name] plugin without this program... */
#define GEDIT_PLUGIN_NAME  _("email")
#define GEDIT_PLUGIN_GLADE_FILE "/email.glade"

static GtkWidget *from_entry = NULL;
static GtkWidget *subject_entry = NULL;
static GtkWidget *to_entry = NULL;
static GtkWidget *location_label = NULL;

static void
gedit_plugin_destroy (PluginData *pd)
{
	g_free (pd->name);
}

static void
gedit_plugin_finish (GtkWidget *widget, gpointer data)
{
	gnome_dialog_close (GNOME_DIALOG (widget));
}

static void
cancel_button_pressed (GtkWidget *widget, GtkWidget* data)
{
	gnome_dialog_close (GNOME_DIALOG (data));
}

static void
help_button_pressed (GtkWidget *widget, gpointer data)
{
	/* FIXME: Paolo - change to point to the right help page */

	static GnomeHelpMenuEntry help_entry = { "gedit", "plugins.html" };

	gnome_help_display (NULL, &help_entry);
}

static void
gedit_plugin_execute (GtkWidget *widget, /*gint button,*/ GtkWidget* data)
{
	const gchar *subject, *from, *to;
	const gchar *program_location = NULL;
	GeditDocument *doc = gedit_document_current();
	FILE *sendmail;
	guchar * buffer;
	gchar *command;
		
	to = gtk_entry_get_text (GTK_ENTRY (to_entry));
	from = gtk_entry_get_text (GTK_ENTRY (from_entry));
	subject = gtk_entry_get_text (GTK_ENTRY (subject_entry));
	program_location = GTK_LABEL(location_label)->label;
	
	g_return_if_fail (program_location != NULL);
	command = g_strdup_printf ("%s %s", program_location, to);

	gedit_flash_va (_("Executing command: %s"), command);
		
	if (!from || strlen (from) == 0 || !to || strlen (to)==0)
	{
		GnomeDialog *error_dialog;
		error_dialog = GNOME_DIALOG (gnome_error_dialog_parented ("Please provide a valid email address.",
									  gedit_window_active()));
		gnome_dialog_run_and_close (error_dialog);
		gdk_window_raise (data->window);
		g_free (command);
		return;
	}

	if ((sendmail = popen (command, "w")) == NULL)
	{
		g_warning ("Couldn't open stream to %s\n", program_location);
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

	gnome_dialog_close (GNOME_DIALOG (data));
}

static void
gedit_plugin_change_location (GtkWidget *button, gpointer userdata)
{
	GtkWidget *dialog;
	GtkWidget *label;
	gchar * new_location;

	gedit_debug (DEBUG_PLUGINS, "");
	dialog = userdata;

	new_location = gedit_plugin_program_location_change (GEDIT_PLUGIN_PROGRAM,
							     GEDIT_PLUGIN_NAME);

	if ( new_location == NULL)
	{
		gdk_window_raise (dialog->window);
		return;
	}

	/* We need to update the label */
	label  = gtk_object_get_data (GTK_OBJECT (dialog), "location_label");
	g_return_if_fail (label!=NULL);
	gtk_label_set_text (GTK_LABEL (label),
			    new_location);
	g_free (new_location);

	gdk_window_raise (dialog->window);	

}


static void
gedit_plugin_create_dialog (void)
{
	GladeXML *gui = NULL;
	GtkWidget *dialog = NULL;
	GtkWidget *filename_label = NULL;
	GtkWidget *change_location = NULL;
	GtkWidget *send_button = NULL;
	GtkWidget *cancel_button = NULL;
	GtkWidget *help_button = NULL;
	
	gchar *username, *fullname, *hostname;
	gchar *from;
	gchar *program_location;
	gchar *config_string;
	gchar *docname;

	GeditDocument *doc = gedit_document_current ();

	g_return_if_fail (doc != NULL);
	
	program_location = gedit_plugin_program_location_get (GEDIT_PLUGIN_PROGRAM,
							     GEDIT_PLUGIN_NAME,
							     FALSE);
	
	g_return_if_fail(program_location != NULL);
	
	gui = glade_xml_new (GEDIT_GLADEDIR
			     GEDIT_PLUGIN_GLADE_FILE,
			     "dialog");

	if (!gui) {
		g_warning ("Could not find %s, reinstall gedit.\n",
			   GEDIT_GLADEDIR
			   GEDIT_PLUGIN_GLADE_FILE);
		return;
	}

	dialog          = glade_xml_get_widget (gui, "dialog");
	to_entry        = glade_xml_get_widget (gui, "to_entry");
	from_entry      = glade_xml_get_widget (gui, "from_entry");
	subject_entry   = glade_xml_get_widget (gui, "subject_entry");
	filename_label  = glade_xml_get_widget (gui, "filename_label");
	location_label  = glade_xml_get_widget (gui, "location_label");
	change_location = glade_xml_get_widget (gui, "change_button");
	send_button     = glade_xml_get_widget (gui, "button0");
	cancel_button   = glade_xml_get_widget (gui, "button1");
	help_button     = glade_xml_get_widget (gui, "button2");

	g_return_if_fail (dialog != NULL);
	g_return_if_fail (to_entry != NULL);
	g_return_if_fail (from_entry  != NULL);
	g_return_if_fail (subject_entry != NULL);
	g_return_if_fail (filename_label != NULL);
	g_return_if_fail (location_label != NULL);
	g_return_if_fail (change_location != NULL);
	g_return_if_fail (send_button != NULL);
	g_return_if_fail (cancel_button != NULL);
	g_return_if_fail (help_button != NULL);

	username = g_get_user_name ();
	fullname = g_get_real_name ();
	hostname = getenv ("HOSTNAME");

	config_string = gnome_config_get_string ("/gedit/email_plugin/From");
	if (config_string)
	{
		gtk_entry_set_text (GTK_ENTRY (from_entry), config_string);
		g_free (config_string);
	}
	else if (fullname && hostname)
	{
		from = g_strdup_printf ("%s <%s@%s>",
					fullname,
					username,
					hostname);

		gtk_entry_set_text (GTK_ENTRY (from_entry), from);
		g_free (from);
	}

	if (doc->filename == NULL) {
		docname = g_strdup_printf (_("Untitled %i"), doc->untitled_number);
	} else {
		docname = gnome_vfs_unescape_string_for_display (doc->filename);		
	}

	/* Set the subject entry box */
	gtk_entry_set_text (GTK_ENTRY (subject_entry), g_basename(docname));

	/* Set the filename label */
	gtk_label_set_text (GTK_LABEL (filename_label), docname);
	
        /* Set the sendmail location label */
	gtk_object_set_data (GTK_OBJECT (dialog), "location_label", location_label);
	gtk_label_set_text (GTK_LABEL (location_label), program_location);
	g_free (program_location);
	

	/* Connect the signals */
	gtk_signal_connect (GTK_OBJECT (send_button), "clicked",
			    GTK_SIGNAL_FUNC (gedit_plugin_execute), dialog);
	gtk_signal_connect (GTK_OBJECT (cancel_button), "clicked",
			    GTK_SIGNAL_FUNC (cancel_button_pressed), dialog);
	gtk_signal_connect (GTK_OBJECT (help_button), "clicked",
			    GTK_SIGNAL_FUNC (help_button_pressed), NULL);
	

	gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
			    GTK_SIGNAL_FUNC (gedit_plugin_finish), NULL);
	gtk_signal_connect (GTK_OBJECT (change_location), "clicked",
			    GTK_SIGNAL_FUNC (gedit_plugin_change_location), dialog);

	/* Set the dialog parent and modal type */ 
	gnome_dialog_set_parent      (GNOME_DIALOG (dialog), gedit_window_active());
	gtk_window_set_modal         (GTK_WINDOW (dialog), TRUE);
	gnome_dialog_set_default     (GNOME_DIALOG (dialog), 0);

	/* Show everything then free the GladeXML memmory */
	gtk_widget_show_all (dialog);
	gtk_object_unref (GTK_OBJECT (gui));
}

gint
init_plugin (PluginData *pd)
{
	/* initialise */
	pd->destroy_plugin = gedit_plugin_destroy;
	pd->name = _("Email");
	pd->desc = _("Email the current document");
	pd->long_desc = _("Email the current document to a specified email address\n"
			  "gedit searches for sendmail to use this plugin.");
	pd->author = "Alex Roberts <bse@error.fsnet.co.uk>";
	pd->needs_a_document = TRUE;
	pd->modifies_an_existing_doc = FALSE;
	pd->private_data = (gpointer)gedit_plugin_create_dialog;
	pd->installed_by_default = TRUE;

	return PLUGIN_OK;
}
