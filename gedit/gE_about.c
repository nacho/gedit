/* vi:set ts=4 sts=0 sw=4:
 *
 * gEdit About Box
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <assert.h>
#include <gtk/gtk.h>
#include <config.h>
#ifndef WITHOUT_GNOME
#include <gnome.h>
#else
#include "gE_icon.xpm"
#endif
#include "main.h"


#ifdef WITHOUT_GNOME
static void close_about(GtkWidget *widget, gpointer data);
static int animate_thanks (gpointer data);

static gint timeout_id;
static GtkWidget *about_window = NULL;
static gchar *thanks[] = {
	"Will LaShell",
	"Chris Lahey",
	"Andy Kahn",
	"Miguel de Icaza",
	NULL
};


static void
close_about(GtkWidget *widget, gpointer data)
{
	gtk_timeout_remove (*((gint *)(data)));
	gtk_widget_destroy(about_window);
	about_window = NULL;
}

static int
animate_thanks (gpointer data)
{
	static gchar **current_person = thanks;
	GtkWidget *label = (GtkWidget *)data;

	assert(*current_person != NULL);
	gtk_label_set (GTK_LABEL (label), *current_person);
	current_person++;
	if (*current_person == NULL)
		current_person = thanks;

	return 1;	/* for some unknown Gtk reason, must return non-zero */
}


void
gE_about_box(GtkWidget *w, gpointer cbdata)
{
	GtkWidget *hbox, *vbox, *tmp;
	GdkPixmap *pixmap;
	GdkBitmap *mask;
	GtkStyle  *style;

	if (about_window)
		return;

	about_window = gtk_dialog_new ();
	gtk_window_set_title (GTK_WINDOW (about_window), "About gEdit");

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG (about_window)->vbox), hbox);
	gtk_container_border_width (GTK_CONTAINER (hbox), 7);

	style = gtk_widget_get_default_style();

	gtk_widget_realize(about_window);
	pixmap = gdk_pixmap_create_from_xpm_d (about_window->window, &mask,
			&style->bg[GTK_STATE_NORMAL], (gchar **)gE_icon);
	tmp = gtk_pixmap_new (pixmap, mask);
	gtk_box_pack_start (GTK_BOX (hbox), tmp, TRUE, TRUE, 0);
	gtk_widget_show (tmp);

	vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (hbox), vbox);
	gtk_container_border_width (GTK_CONTAINER (vbox), 5);
	

	tmp = gtk_label_new (GEDIT_ID);
	gtk_misc_set_alignment(GTK_MISC(tmp), 0.0, 0.5);
	gtk_box_pack_start (GTK_BOX (vbox), tmp, TRUE, TRUE, 1);
	gtk_widget_show (tmp);

	tmp = gtk_label_new ("(C) Alex Roberts and Evan Lawrence, 1998");
	gtk_box_pack_start (GTK_BOX (vbox), tmp, TRUE, TRUE, 1);
	gtk_misc_set_alignment(GTK_MISC(tmp), 0.0, 0.5);
	gtk_widget_show (tmp);

	tmp = gtk_label_new ("http://gedit.pn.org");
	gtk_box_pack_start (GTK_BOX (vbox), tmp, TRUE, TRUE, 1);
	gtk_misc_set_alignment(GTK_MISC(tmp), 0.0, 0.5);
	gtk_widget_show (tmp);


	gtk_widget_show (vbox);
	gtk_widget_show (hbox);

	tmp = gtk_label_new ("Special Thanks to: ");
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (about_window)->vbox),
		tmp, TRUE, TRUE, 2);
	gtk_misc_set_alignment (GTK_MISC (tmp), 0.5, 0.5);
	gtk_widget_show (tmp);
	tmp = gtk_label_new (" ");
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG(about_window)->vbox),
		tmp, TRUE, TRUE, 1);
	gtk_widget_show (tmp);
	timeout_id = gtk_timeout_add (1000, (GtkFunction)animate_thanks, tmp);
	
	tmp = gtk_button_new_with_label ("OK");
	gtk_signal_connect (GTK_OBJECT (tmp), "clicked",
		GTK_SIGNAL_FUNC (close_about), (gpointer)&timeout_id);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (about_window)->action_area),
		tmp, TRUE, TRUE, 0);
	GTK_WIDGET_SET_FLAGS (tmp, GTK_CAN_DEFAULT);
	gtk_widget_grab_default (tmp);
	gtk_widget_show (tmp);

	gtk_widget_show (about_window);
} /* gE_about_box */

#else	/* USING GNOME */

void gE_about_box(GtkWidget *w, gpointer cbdata)
{
	GtkWidget *about;
	gchar *authors[] = {
		"Alex Roberts",
		"Evan Lawrence",
		"http://gedit.pn.org",
		NULL
	};

	about = gnome_about_new (GEDIT_ID, NULL,
			"(C) 1998 Alex Roberts and Evan Lawrence",
			authors,
			_("gEdit is a small and lightweight text editor for GNOME/Gtk+"),
			NULL);
	gtk_widget_show (about);
}
#endif	/* #ifdef WITHOUT_GNOME */
