
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

void send_mail (GnomeDialog *window)
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
	gnome_dialog_close (GNOME_DIALOG (window));
}


int main (int argc, char *argv[])
{
	GnomeDialog *window;
	GtkWidget *from_label, *to_label, *subject_label, *file_label, *file;
	GtkWidget *hbox, *vbox, *table;
	gchar *filename, *from;
	gint butnnum;
	char *user, *hostname;
	client_info info = empty_info;
	struct passwd *pw;
	
	info.menu_location = "[Plugins]Email";
	
	context = client_init (&argc, &argv, &info);
	bindtextdomain(PACKAGE, GNOMELOCALEDIR);
  	textdomain(PACKAGE);

  	gnome_init("email", VERSION, argc, argv);
	
	docid = client_document_current (context);
	filename = client_document_filename (docid);

	window = gnome_dialog_new ("Email",
		GNOME_STOCK_BUTTON_OK,
		GNOME_STOCK_BUTTON_CANCEL,
		NULL);

	vbox = gtk_vbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (
		GNOME_DIALOG (window)->vbox),
		vbox, FALSE, TRUE, 2);

	gtk_container_set_border_width (GTK_CONTAINER (vbox), 10);
	
	table = gtk_table_new (3, 2, FALSE);
	gtk_box_pack_start_defaults (GTK_BOX (vbox), table);

	from_label = gtk_label_new ("From:     ");
	gtk_table_attach_defaults (GTK_TABLE (table), from_label, 0, 1, 0, 1);
	gtk_widget_show (from_label);
	
	from_entry = gtk_entry_new ();
	gtk_table_attach_defaults (GTK_TABLE (table), from_entry, 1, 2, 0, 1);
	gtk_widget_show (from_entry);
	

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

	to_label = gtk_label_new ("To:        ");
	gtk_table_attach_defaults (GTK_TABLE (table), to_label, 0, 1, 1, 2);
	gtk_widget_show (to_label);
	
	to_entry = gtk_entry_new ();
	gtk_table_attach_defaults (GTK_TABLE (table), to_entry, 1, 2, 1, 2);
	gtk_widget_show (to_entry);
	
	subject_label = gtk_label_new ("Subject: ");
	gtk_table_attach_defaults (GTK_TABLE (table), subject_label, 0, 1, 2, 3);
	gtk_widget_show (subject_label);
	
	subject_entry = gtk_entry_new ();
	gtk_table_attach_defaults (GTK_TABLE (table), subject_entry, 1, 2, 2, 3);
	if (strlen (filename) < 1)
		gtk_entry_set_text (GTK_ENTRY (subject_entry), "Untitled");
	else
		gtk_entry_set_text (GTK_ENTRY (subject_entry), filename);
	gtk_widget_show (subject_entry);
	
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
	gtk_widget_show (vbox);
	
	butnnum = gnome_dialog_run (GNOME_DIALOG (window));
	
	if (butnnum == 0)
		send_mail (window);
	
	return 0;
}

