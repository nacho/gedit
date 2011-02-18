#include <gtk/gtk.h>
#include <glib.h>
#include "gedit-animated-overlay.h"
#include "gedit-rounded-frame.h"
#include "gedit-floating-slider.h"

static GtkWidget *overlay;

static void
on_button_clicked (GtkWidget *button,
                   GtkWidget *slider)
{
	g_object_set (G_OBJECT (slider),
	              "animation-state", GEDIT_THEATRICS_ANIMATION_STATE_INTENDING_TO_GO,
	              NULL);
}

gint
main ()
{
	GtkWidget *window;
	GtkWidget *textview;
	GtkWidget *frame;
	GtkWidget *entry;
	GtkWidget *vbox;
	GtkWidget *button;
	GtkWidget *slider;

	gtk_init (NULL, NULL);

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size (GTK_WINDOW (window), 300, 200);
	vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (window), vbox);

	textview = gtk_text_view_new ();
	overlay = gedit_animated_overlay_new (textview, NULL);

	gtk_box_pack_start (GTK_BOX (vbox), overlay, TRUE, TRUE, 0);

	frame = gedit_rounded_frame_new ();
	gtk_widget_show (frame);

	entry = gtk_entry_new ();

	gtk_container_add (GTK_CONTAINER (frame), entry);

	slider = gedit_floating_slider_new (frame);

	g_object_set (G_OBJECT (slider),
	              "duration", 1000,
	              "easing", GEDIT_THEATRICS_CHOREOGRAPHER_EASING_EXPONENTIAL_IN_OUT,
	              "blocking", GEDIT_THEATRICS_CHOREOGRAPHER_BLOCKING_DOWNSTAGE,
	              "orientation", GTK_ORIENTATION_VERTICAL,
	              NULL);

	gedit_animated_overlay_add (GEDIT_ANIMATED_OVERLAY (overlay),
	                            GEDIT_ANIMATABLE (slider));

	/* set the animation state after it is added the widget */
	g_object_set (G_OBJECT (slider),
	              "animation-state", GEDIT_THEATRICS_ANIMATION_STATE_COMING,
	              NULL);

	button = gtk_button_new_with_label ("Hide");
	gtk_box_pack_end (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	g_signal_connect (button, "clicked",
	                  G_CALLBACK (on_button_clicked),
	                  slider);

	gtk_widget_show_all (window);

	gtk_main ();

	return 0;
}
