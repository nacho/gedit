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

#include <glib.h>
#include <gtk/gtkwidget.h>
#include <glade/glade.h>

#include "dialogs/dialogs.h"

/**
 * dialog_about:
 *
 * Show the user the information about the program and its authors.
 */
void
gedit_dialog_about (void)
{
	GladeXML *gui = glade_xml_new (GEDIT_GLADEDIR "about.glade", NULL);
	GtkWidget *about;

	if (!gui)
	{
		g_warning ("Could not find about.glade");
		return;
	}

	about = glade_xml_get_widget (gui, "about");

	gtk_widget_show (about);
	
	gtk_object_unref (GTK_OBJECT (gui));
}
