/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * gedit - About dialog
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

#include <config.h>
#include <gnome.h>
#include <glade/glade.h>

void dialog_about (void);

/**
 * gedit_about:
 *
 * Show the user the information about the program and its authors.
 */
void
dialog_about (void)
{
#if 0
	GladeXML *gui = glade_xml_new ("/home/elerium/cvs/gedit/src/dialogs/about.glade", NULL);

	if (!gui)
	{
		g_warning ("Could not find about.glade");
		return;
	}
	gtk_widget_show (glade_xml_get_widget (gui, "about"));
	gtk_object_destroy (GTK_OBJECT (gui));
#else
	static GtkWidget *about;
	
	const gchar *authors[] = {
		"Alex Roberts",
		"Evan Lawrence",
		"http://gedit.pn.org",
		"",
		N_("With special thanks to:"),
		"     Will LaShell, Chris Lahey, Andy Kahn,",
		"     Miguel de Icaza, Martin Baulig,",
		"     Thomas Holmgren, Martijn van Beers",
		NULL
	};

	if (about != NULL)
	{
		gdk_window_show (about->window);
		gdk_window_raise (about->window);
		return;
	}

	about = gnome_about_new ("gedit", VERSION,
				 _("(C) 1998, 1999 Alex Roberts and Evan Lawrence"),
				 authors,
				 _("gedit is a small and lightweight text "
				   "editor for GNOME/Gtk+"),
				 "gedit-logo.png");

	gtk_signal_connect (GTK_OBJECT (about), "destroy",
			    GTK_SIGNAL_FUNC (gtk_widget_destroyed), &about);

	gtk_widget_show (about);
#endif
}
