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

void close_about(GtkWidget *widget, gpointer *data)
{
   gtk_widget_destroy(about_window);
   about_window = NULL;
}

void gE_about_box()
{
   GtkWidget *button;
   GtkWidget *label;
   GtkWidget *hbox;
   GtkWidget *logo;
   GdkPixmap *pixmap;
   GdkBitmap *mask;
   GtkStyle  *style;
   gchar buffer[64];
   
   about_window = gtk_dialog_new ();
   gtk_window_set_title (GTK_WINDOW (about_window), "About");

  hbox = gtk_hbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (about_window)->vbox), hbox);
  gtk_widget_show (hbox);

     style=gtk_widget_get_style(button);

      pixmap = gdk_pixmap_create_from_xpm_d (about_window->window, &mask, 
					   &style->bg[GTK_STATE_NORMAL],
   				      (gchar **)gE_icon);
      logo = gtk_pixmap_new (pixmap, mask);
  gtk_widget_show (logo);
   gtk_box_pack_start (GTK_BOX (hbox), logo, TRUE,
		       TRUE, 0);
   
   label = gtk_label_new ("gEdit By Alex Roberts and Evan Lawrence, 1998");
   gtk_box_pack_start (GTK_BOX (hbox), label, TRUE,
		       TRUE, 8);
      gtk_container_border_width (GTK_CONTAINER (hbox), 10);
      gtk_container_add (GTK_CONTAINER (hbox), logo);
      gtk_container_add (GTK_CONTAINER (hbox), label);
  gtk_widget_show (logo);
   gtk_widget_show (label);
   gtk_widget_show (hbox);

 hbox = gtk_hbox_new (FALSE, 0);
      gtk_container_border_width (GTK_CONTAINER (hbox), 8);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (about_window)->vbox), hbox);
  gtk_widget_show (hbox);


    sprintf (buffer,
	     "Gtk+ v%d.%d.%d",
	     gtk_major_version,
	     gtk_minor_version,
	     gtk_micro_version);


 label = gtk_label_new ("http://melt.home.ml.org/gedit");
  gtk_widget_show (label);
   gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0); 
 label = gtk_label_new (buffer);
  gtk_widget_show (label);
   gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);


   gtk_widget_show (hbox);

   button = gtk_button_new_with_label ("Ok");
   gtk_signal_connect (GTK_OBJECT (button), "clicked",
		       GTK_SIGNAL_FUNC (close_about), (gpointer) "button");
   gtk_box_pack_start (GTK_BOX (GTK_DIALOG (about_window)->action_area), button, TRUE,
		       TRUE, 0);
   GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
   gtk_widget_grab_default (button);
   gtk_widget_show (button);
   
   
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

        about = gnome_about_new (_(gEdit_ID), NULL,
				 "(C) 1998 Alex Roberts and Evan Lawrence",
				 authors,
				 _("gEdit is a small and lightweight text editor for GNOME/Gtk+"),
				 NULL);
        gtk_widget_show (about);
}
#endif

