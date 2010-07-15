#include <gtk/gtk.h>
#include <glib.h>
#include "gedit-view-container.h"
#include "gedit-rounded-frame.h"

static void
on_button_clicked (GtkWidget *button,
                   GtkWidget *frame)
{
	gtk_widget_destroy (frame);
}

gint
main ()
{
	GtkWidget *window;
	GtkWidget *textview;
	GtkWidget *container;
	GtkWidget *frame;
	GtkWidget *entry;
	GtkWidget *vbox;
	GtkWidget *button;

	gtk_init (NULL, NULL);

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size (GTK_WINDOW (window), 300, 200);
	vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (window), vbox);

	textview = gtk_text_view_new ();
	container = gedit_view_container_new (textview);

	gtk_box_pack_start (GTK_BOX (vbox), container, TRUE, TRUE, 0);

	frame = gedit_rounded_frame_new ();
	gtk_widget_show (frame);

	entry = gtk_entry_new ();

	gtk_container_add (GTK_CONTAINER (frame), entry);

	gedit_view_container_add_animated_widget (GEDIT_VIEW_CONTAINER (container),
	                                          frame,
	                                          1000,
	                                          GEDIT_THEATRICS_CHOREOGRAPHER_EASING_EXPONENTIAL_IN_OUT,
	                                          GEDIT_THEATRICS_CHOREOGRAPHER_BLOCKING_DOWNSTAGE,
	                                          GTK_ORIENTATION_VERTICAL,
	                                          0, 0);

	button = gtk_button_new_with_label ("Hide");
	gtk_box_pack_end (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	g_signal_connect (button, "clicked",
	                  G_CALLBACK (on_button_clicked),
	                  frame);

	gtk_widget_show_all (window);

	gtk_main ();

	return 0;
}
