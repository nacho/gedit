/* 
 * Sample plugin demo
 * Alex Roberts <bse@error.fsnet.co.uk>
 *
 * Prints "Hello World" into the current document
 */
 
#include <gnome.h>
#include <config.h>
#include <stdio.h>
#include <string.h>
#include <pwd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <glib.h>

#include "../../src/main.h"
#include "../../src/gE_mdi.h"
#include "../../src/gE_plugin.h"


#ifndef MAILER
#define MAILER "/usr/lib/sendmail"
#endif
static GtkWidget *from_entry, *subject_entry, *to_entry;

/* first the gE_plugin centric code */

static void destroy_plugin (gE_Plugin_Data *pd)
{
	g_free (pd->name);
	
}

static void
email_finish (GtkWidget *w, gpointer data )
{
	gnome_dialog_close( GNOME_DIALOG( w ) );

}

/* the function that actually does the wrok */
static void email_clicked (GtkWidget *w, gint button, gpointer data)
{
	gint i;
	gE_document *doc = gE_document_current();
	FILE *sendmail;
	gchar *subject, *from, *to, *buffer, *command;
	
	if ( button == 0 )
	  {
	    
	    to = gtk_entry_get_text (GTK_ENTRY (to_entry));
	    from = gtk_entry_get_text (GTK_ENTRY (from_entry));
	    subject = gtk_entry_get_text (GTK_ENTRY (subject_entry));
	    buffer = strdup(doc->buf->str);
	    
	    command = g_malloc0 (strlen (MAILER) + strlen (to) + 2);
	    sprintf (command, "%s %s", MAILER, to);
	    
	    if ((sendmail = popen (command, "w")) == NULL)
	      {
		printf ("Couldn't open stream to /usr/bin/sendmail\n");
		g_free (command);
		g_free (buffer);
		return;
	      }
	    
	    g_free (command);
	    
	    fprintf (sendmail, "To: %s\n", to);
	    fprintf (sendmail, "From: %s\n", from);
	    fprintf (sendmail, "Subject: %s\n", subject);
	    fprintf (sendmail, "X-Mailer: gEdit email plugin v 0.2\n");
	    fflush (sendmail);
	    fprintf (sendmail, "%s\n", buffer);
	    fflush (sendmail);
	    
	    pclose (sendmail);
	    
	    gnome_config_set_string ("/Editor_Plugins/Email/From", from);
	    gnome_config_sync ();
	  }
	gnome_dialog_close( GNOME_DIALOG( w ) );
			
}
	
static void email ()
{
	GtkWidget *window, *from_label, *to_label, *subject_label,
	  *file_label, *file;
	GtkWidget *hbox, *vbox;
	gchar *filename, *from;
	char *user, *hostname;
	struct passwd *pw;
	gE_document *doc = gE_document_current();
	
	if (!(filename = strdup (doc->filename))) 
		return;

	window = gnome_dialog_new( _( "The gEdit Email Plugin" ),
				   GNOME_STOCK_BUTTON_OK,
				   GNOME_STOCK_BUTTON_CANCEL,
				   NULL );
	gnome_dialog_set_default( GNOME_DIALOG( window ), 0 );

	gtk_signal_connect (GTK_OBJECT (window), "destroy", email_finish, NULL);
	vbox = gtk_vbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (GNOME_DIALOG (window)->vbox), vbox,
			    FALSE, TRUE, 2);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 10);
	gtk_widget_show (vbox);
	
	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
	gtk_widget_show (hbox);	
	
	from_label = gtk_label_new ("From:     ");
	gtk_box_pack_start (GTK_BOX (hbox), from_label, FALSE, TRUE, 0);
	gtk_widget_show (from_label);
	
	from_entry = gtk_entry_new ();
	gtk_box_pack_start (GTK_BOX (hbox), from_entry, TRUE, TRUE, 0);
	gtk_widget_show (from_entry);
	gtk_widget_show (hbox);
	
	user = getenv ("USER");
	hostname = getenv ("HOSTNAME");
	
	if (gnome_config_get_string ("/Editor_Plugins/Email/From")) {
		gtk_entry_set_text (GTK_ENTRY (from_entry), 
	    					gnome_config_get_string ("/Editor_Plugins/Email/From"));

	} else if (user) {
		pw = getpwnam (user);
		if ((pw) && (hostname))
		{
			from = g_malloc0 (strlen (pw->pw_gecos) +
					  strlen (user) +
					  strlen (hostname) +
					  5);
			sprintf (from, "%s <%s@%s>",
				 pw->pw_gecos, user, hostname);
			gtk_entry_set_text (GTK_ENTRY (from_entry), from);
		}
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
	
	gtk_signal_connect (GTK_OBJECT (window), "clicked",
			    GTK_SIGNAL_FUNC (email_clicked), NULL);
	
	gtk_widget_show_all (window);
}

gint init_plugin (gE_Plugin_Data *pd)
{
	/* initialise */
	pd->destroy_plugin = destroy_plugin;
	pd->name = _("Email");
	pd->desc = _("Email the current document");
	pd->author = "Alex Roberts <bse@error.fsnet.co.uk>";
	
	pd->private_data = (gpointer)email;
	
	return PLUGIN_OK;
}
