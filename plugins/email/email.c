/* 
 * Sample plugin demo
 * Alex Roberts <bse@error.fsnet.co.uk>
 *
 * Prints "Hello World" into the current document
 */
 
#include <config.h>
#include <gnome.h>
#include <glade/glade.h>

#include "../../src/document.h"
#include "../../src/plugin.h"

#ifndef MAILER
#define MAILER "/usr/lib/sendmail"
#endif

static GtkWidget *from_entry, *subject_entry, *to_entry;


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

/* the function that actually does the wrok */
static void
email_clicked (GtkWidget *w, gint button, gpointer data)
{
	Document *doc = gedit_document_current();
	FILE *sendmail;
	gchar *subject, *from, *to, *buffer, *command;

	if (button == 0)
	{
		to = gtk_entry_get_text (GTK_ENTRY (to_entry));
		from = gtk_entry_get_text (GTK_ENTRY (from_entry));
		subject = gtk_entry_get_text (GTK_ENTRY (subject_entry));
		buffer = g_strdup (doc->buf->str);
	    
		command = g_malloc0 (strlen (MAILER) + strlen (to) + 2);
		sprintf (command, "%s %s", MAILER, to);
	    
		if ((sendmail = popen (command, "w")) == NULL)
		{
			printf ("Couldn't open stream to /usr/bin/sendmail\n");
			g_free (command);
			g_free (buffer);
			return;
		}
	    
		fprintf (sendmail, "To: %s\n", to);
		fprintf (sendmail, "From: %s\n", from);
		fprintf (sendmail, "Subject: %s\n", subject);
		fprintf (sendmail, "X-Mailer: gedit email plugin v 0.2\n");
		fflush (sendmail);
		fprintf (sendmail, "%s\n", buffer);
		fflush (sendmail);
	    
		pclose (sendmail);
	    
		gnome_config_set_string ("/Editor_Plugins/Email/From", from);
		gnome_config_sync ();
		
		g_free (command);
		g_free (buffer);
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

	username = g_get_user_name ();
	fullname = g_get_real_name ();
	hostname = getenv ("HOSTNAME");

	if (fullname && hostname)
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
		gtk_entry_set_text (GTK_ENTRY (subject_entry), doc->filename);
	else
		gtk_entry_set_text (GTK_ENTRY (subject_entry), _("Untitled"));

	/* Set the filename label */
	filename_label = g_strdup (GTK_LABEL (glade_xml_get_widget (gui, "filename_label"))->label);
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
