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
#include <gnome.h>
#include "main.h"

void gE_about_box(GtkWidget *w, gpointer cbdata)
{
	GtkWidget *about;
	const gchar *authors[] = {
		N_("Alex Roberts"),
		N_("Evan Lawrence"),
		N_("http://gedit.pn.org"),
		"",
		N_("With special thanks to:"),
		N_("       Will LaShell"),
		N_("       Chris Lahey"),
		N_("       Andy Kahn"),
		N_("       Miguel de Icaza"),
		N_("       Martin Baulig"),
		NULL
	};

#ifdef ENABLE_NLS
    {
	    int i=0;
	    while (authors[i] != NULL) authors[i]=_(authors[i]);
    }
#endif

	about = gnome_about_new ("gEdit", VERSION,
			_("(C) 1998, 1999 Alex Roberts and Evan Lawrence"),
			authors,
			_("gEdit is a small and lightweight text editor for GNOME/Gtk+"),
			NULL);
	gtk_widget_show (about);
}
