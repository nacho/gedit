#include <gtk/gtk.h>
#include <glib.h>
#include "gedit-rounded-frame.h"

gint main ()
{
	GtkWidget *window;
	GtkWidget *frame;
	GtkWidget *entry;

	gtk_init (NULL, NULL);

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

	frame = gedit_rounded_frame_new ();

	gtk_container_add (GTK_CONTAINER (window), frame);

	entry = gtk_entry_new ();

	gtk_container_add (GTK_CONTAINER (frame), entry);
	gtk_container_set_border_width (GTK_CONTAINER (frame), 10);

	gtk_widget_show_all (window);

	gtk_main ();

	return 0;
}
