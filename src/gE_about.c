/*   gEdit About Box */
#ifndef WITHOUT_GNOME
#include <config.h>
#include <gnome.h>
#endif 

#include <gtk/gtk.h>
#include "gE_icon.xpm"
#include "main.h"


#ifdef WITHOUT_GNOME
GtkWidget *about_window;
gint about_counter;
gint current_person;
gint timeout_id;

void close_about(GtkWidget *widget, gpointer *data)
{
   gtk_timeout_remove (timeout_id);
   gtk_widget_destroy(about_window);
   about_window = NULL;
}

int animate_thanks (GtkWidget *label)
{
	gchar *thanks[] = {
	"Will LaShell",
	"Chris Lahey",
	"Andy Kahn",
	"Miguel de Icaza"
	};
	
	if (about_counter > 25) {
		about_counter = 0;
		current_person++;
	}
	
	if (current_person > 3)
		current_person = 0;
	
	gtk_label_set (GTK_LABEL (label), thanks[current_person]);
	about_counter++;
	
	return 1;
}
	
	
void gE_about_box()
{
	GtkWidget *button;
	GtkWidget *id, *copyright, *url, *specthanks, *label;
	GtkWidget *hbox, *vbox;
	GtkWidget *logo;
	GdkPixmap *pixmap;
	GdkBitmap *mask;
	GtkStyle  *style;

	about_window = gtk_dialog_new ();
	gtk_window_set_title (GTK_WINDOW (about_window), "About gEdit");

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG (about_window)->vbox), hbox);
	gtk_container_border_width (GTK_CONTAINER (hbox), 7);

	style = gtk_widget_get_default_style();

	gtk_widget_realize(about_window);
	pixmap = gdk_pixmap_create_from_xpm_d (about_window->window, &mask,
			&style->bg[GTK_STATE_NORMAL], (gchar **)gE_icon);
	logo = gtk_pixmap_new (pixmap, mask);
	gtk_box_pack_start (GTK_BOX (hbox), logo, TRUE, TRUE, 0);

	vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (hbox), vbox);
	gtk_container_border_width (GTK_CONTAINER (vbox), 5);
	
	id = gtk_label_new (GEDIT_ID);
	copyright = gtk_label_new ("(C) Alex Roberts and Evan Lawrence, 1998");
	url = gtk_label_new ("http://melt.home.ml.org/gedit");

	gtk_box_pack_start (GTK_BOX (vbox), id, TRUE, TRUE, 1);
	gtk_box_pack_start (GTK_BOX (vbox), copyright, TRUE, TRUE, 1);
	gtk_box_pack_start (GTK_BOX (vbox), url, TRUE, TRUE, 1);

	gtk_misc_set_alignment(GTK_MISC(id), 0.0, 0.5);
	gtk_misc_set_alignment(GTK_MISC(copyright), 0.0, 0.5);
	gtk_misc_set_alignment(GTK_MISC(url), 0.0, 0.5);

	gtk_widget_show (logo);
	gtk_widget_show (id);
	gtk_widget_show (copyright);
	gtk_widget_show (url);
	gtk_widget_show (vbox);
	gtk_widget_show (hbox);

	specthanks = gtk_label_new ("Special Thanks to: ");
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (about_window)->vbox), specthanks, TRUE, TRUE, 2);
	gtk_misc_set_alignment (GTK_MISC (specthanks), 0.0, 0.5);
	gtk_widget_show (specthanks);
	label = gtk_label_new ("                         ");
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG(about_window)->vbox), label, TRUE, TRUE, 1);
	gtk_widget_show (label);
	timeout_id = gtk_timeout_add (50, animate_thanks, label);
	
	button = gtk_button_new_with_label ("OK");
	gtk_signal_connect (GTK_OBJECT (button), "clicked",
			GTK_SIGNAL_FUNC (close_about), (gpointer) "button");
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (about_window)->action_area),
			button, TRUE, TRUE, 0);
	GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
	gtk_widget_grab_default (button);
	gtk_widget_show (button);

	current_person = 0;
	about_counter = 0;
	gtk_widget_show (about_window);
}
#else


void gE_about_box()
{
        GtkWidget *about;
        gchar *authors[] = {
		"Alex Roberts",
		"Evan Lawrence",
		"http://melt.home.ml.org/gedit",
		NULL
	};

        about = gnome_about_new (GEDIT_ID, NULL,
				 "(C) 1998 Alex Roberts and Evan Lawrence",
				 authors,
				 _("gEdit is a small and lightweight text editor for GNOME/Gtk+"),
				 NULL);
        gtk_widget_show (about);
}
#endif

