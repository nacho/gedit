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

/* first the gE_plugin centric code */

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
#if 1
	GladeXML *gui;
	GtkWidget *dialog;
	Document *doc = gedit_document_current ();
	gchar *username, *fullname, *hostname;
	gchar *from;
	gchar *filename_label;

	gui = glade_xml_new ("/home/elerium/cvs/gedit/plugins/email/email.glade", NULL);

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

#else
	GtkWidget *window, *from_label, *to_label, *subject_label;
	GtkWidget *file_label, *file;
	GtkWidget *hbox, *vbox;
	gchar *filename, *from;
	char *user, *hostname;
	struct passwd *pw;
	Document *doc = gedit_document_current();

	filename = g_strdup (doc->filename);
	if (!filename)
		return;

	window = gnome_dialog_new (_("The gEdit Email Plugin"),
				   GNOME_STOCK_BUTTON_OK,
				   GNOME_STOCK_BUTTON_CANCEL,
				   NULL);

	gnome_dialog_set_default (GNOME_DIALOG (window), 0);

	gtk_signal_connect (GTK_OBJECT (window), "destroy", email_finish, NULL);
	vbox = gtk_vbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (GNOME_DIALOG (window)->vbox), vbox,
			    FALSE, TRUE, 2);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), GNOME_PAD);
	gtk_widget_show (vbox);
	
	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
	gtk_widget_show (hbox);	
	
	from_label = gtk_label_new ("From: ");
	gtk_box_pack_start (GTK_BOX (hbox), from_label, FALSE, TRUE, 0);
	gtk_widget_show (from_label);
	
	from_entry = gtk_entry_new ();
	gtk_box_pack_start (GTK_BOX (hbox), from_entry, TRUE, TRUE, 0);
	gtk_widget_show (from_entry);
	gtk_widget_show (hbox);
	
	user = getenv ("USER");
	hostname = getenv ("HOSTNAME");
	
	if (gnome_config_get_string ("/Editor_Plugins/Email/From"))
	{
		gtk_entry_set_text (GTK_ENTRY (from_entry), 
				    gnome_config_get_string ("/Editor_Plugins/Email/From"));

	}
	else if (user)
	{
		pw = getpwnam (user);
		if ((pw) && (hostname))
		{
			from = g_strdup_printf ("%s <%s@%s>",
						pw->pw_gecos,
						user,
						hostname);

			gtk_entry_set_text (GTK_ENTRY (from_entry), from);
		}
	}

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
	
	to_label = gtk_label_new ("To: ");
	gtk_box_pack_start (GTK_BOX (hbox), to_label, FALSE, FALSE, 0);
	gtk_widget_show (to_label);
	
	to_entry = gtk_entry_new ();
	gtk_box_pack_start (GTK_BOX (hbox), to_entry, TRUE, TRUE, 0);
	gtk_widget_show (to_entry);
	gtk_widget_show (hbox);
	
	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
	
	subject_label = gtk_label_new ("Subject: ");
	gtk_box_pack_start (GTK_BOX (hbox), subject_label, FALSE, FALSE, 0);
	gtk_widget_show (subject_label);
	
	subject_entry = gtk_entry_new ();
	gtk_box_pack_start (GTK_BOX (hbox), subject_entry, TRUE, TRUE, 0);
	if (strlen (filename) < 1)
		gtk_entry_set_text (GTK_ENTRY (subject_entry), "Untitled");
	else
		gtk_entry_set_text (GTK_ENTRY (subject_entry), filename);
	gtk_widget_show (subject_entry);
	gtk_widget_show (hbox);
	
	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
	
	file_label = gtk_label_new ("File to use as body of message: ");
	gtk_box_pack_start (GTK_BOX (hbox), file_label, FALSE, FALSE, 0);
	gtk_widget_show (file_label);
	
	if (strlen (filename) < 1)
		file = gtk_label_new ("Untitled");
	else
		file = gtk_label_new (filename);

	gtk_box_pack_start (GTK_BOX (hbox), file, TRUE, TRUE, 0);
	gtk_widget_show (file);
	gtk_widget_show (hbox);
	
	gtk_signal_connect (GTK_OBJECT (window), "clicked",
			    GTK_SIGNAL_FUNC (email_clicked), NULL);
	
	gtk_widget_show_all (window);
#endif
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
