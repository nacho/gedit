
#include <stdio.h>
#include <string.h>
#include <pwd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <glib.h>
#include <config.h>
#include <gnome.h>
#include "client.h"

#ifndef MAILER
#define MAILER "/usr/lib/sendmail"
#endif

static GtkWidget *from_entry, *subject_entry, *to_entry;
static int docid, context;

void send_mail (GtkWidget *w, gpointer data)
{
	FILE *sendmail;
	gchar *subject, *from, *to, *buffer, *command;

	to = gtk_entry_get_text (GTK_ENTRY (to_entry));
	from = gtk_entry_get_text (GTK_ENTRY (from_entry));
	subject = gtk_entry_get_text (GTK_ENTRY (subject_entry));
	buffer = client_text_get (docid);
	
	command = g_malloc0 (strlen (MAILER) + strlen (to) + 2);
	sprintf (command, "%s %s", MAILER, to);
	
	if ((sendmail = popen (command, "w")) == NULL)
	{
		printf ("Couldn't open stream to /usr/bin/sendmail\n");
		exit (1);
	}
	
	g_free (command);
	
	fprintf (sendmail, "To: %s\n", to);
	fprintf (sendmail, "From: %s\n", from);
	fprintf (sendmail, "Subject: %s\n", subject);
	fprintf (sendmail, "X-Mailer: gEdit email plugin v 0.1\n");
	fflush (sendmail);
	fprintf (sendmail, "%s\n", buffer);
	fflush (sendmail);
	
	pclose (sendmail);
	client_finish (context);
	gtk_main_quit ();
}


int main (int argc, char *argv[])
{
	GtkWidget *window, *from_label, *to_label, *subject_label, *file_label, *file;
	GtkWidget *ok, *cancel;
	GtkWidget *hbox, *vbox;
	gchar *filename, *from;
	char *user, *hostname;
	client_info info = empty_info;
	struct passwd *pw;
	
	info.menu_location = "[Plugins]Email";
	
	context = client_init (&argc, &argv, &info);
/*	gtk_init (&argc, &argv);*/
	bindtextdomain(PACKAGE, GNOMELOCALEDIR);
  	textdomain(PACKAGE);

  	gnome_init("email", VERSION, argc, argv);
	
	docid = client_document_current (context);
	filename = client_document_filename (docid);

	window = gtk_dialog_new ();
	gtk_window_set_title (GTK_WINDOW (window), "The gEdit Email Plugin");
	gtk_signal_connect (GTK_OBJECT (window), "destroy", gtk_main_quit, NULL);
	vbox = gtk_vbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), vbox, FALSE, TRUE, 2);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 10);
	
	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
	
	from_label = gtk_label_new ("From:     ");
	gtk_box_pack_start (GTK_BOX (hbox), from_label, FALSE, TRUE, 0);
	gtk_widget_show (from_label);
	
	from_entry = gtk_entry_new ();
	gtk_box_pack_start (GTK_BOX (hbox), from_entry, TRUE, TRUE, 0);
	gtk_widget_show (from_entry);
	gtk_widget_show (hbox);
	

	user = getenv ("USER");
	/*printf ("%s\n", user);*/
	hostname = getenv ("HOSTNAME");
	/*printf ("%s\n", hostname);*/
	
	if (user)
	{
		pw = getpwnam (user);
		if ((pw) && (hostname))
		{
			from = g_malloc0 (strlen (pw->pw_gecos) + strlen (user) + strlen (hostname) + 5);
			sprintf (from, "%s <%s@%s>", pw->pw_gecos, user, hostname);
			/*printf ("%s\n", from);*/
			gtk_entry_set_text (GTK_ENTRY (from_entry), from);
			/*g_free (from);*/
			/*g_free (pw);*/
			/*g_free (hostname);*/
		}
		/*g_free (user);*/
	}

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
	
	to_label = gtk_label_new ("To:        ");
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
	
	ok = gnome_stock_button (GNOME_STOCK_BUTTON_OK);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), ok, FALSE, TRUE, 0);
	gtk_signal_connect (GTK_OBJECT (ok), "clicked", GTK_SIGNAL_FUNC (send_mail), NULL);
	gtk_widget_show (ok);
	
	cancel = gnome_stock_button (GNOME_STOCK_BUTTON_CANCEL);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), cancel, FALSE, TRUE, 0);
	gtk_signal_connect (GTK_OBJECT (cancel), "clicked", gtk_main_quit, NULL);
	gtk_widget_show (cancel);
	
	gtk_widget_show (vbox);
	gtk_widget_show (window);
	
	gtk_main ();
	exit (0);
}

