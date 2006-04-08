/*
 * gedit-help-commands.c
 * This file is part of gedit
 *
 * Copyright (C) 1998, 1999 Alex Roberts, Evan Lawrence
 * Copyright (C) 2000, 2001 Chema Celorio, Paolo Maggi
 * Copyright (C) 2002-2005 Paolo Maggi
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
 * Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the gedit Team, 1998-2005. See the AUTHORS file for a
 * list of people on the gedit Team.
 * See the ChangeLog files for a list of changes.
 *
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "gedit-commands.h"
#include "gedit-debug.h"
#include "gedit-help.h"

void
gedit_cmd_help_contents (GtkAction   *action,
			 GeditWindow *window)
{
	gedit_debug (DEBUG_COMMANDS);

	gedit_help_display (GTK_WINDOW (window), NULL, NULL);
}

void
gedit_cmd_help_about (GtkAction   *action,
		      GeditWindow *window)
{
	static const gchar * const authors[] = {
		"Paolo Maggi <paolo@gnome.org>",
		"Paolo Borelli <pborelli@katamail.com>",
		"James Willcox <jwillcox@gnome.org>",
		"Chema Celorio",
		"Federico Mena Quintero <federico@novell.com>",
		NULL
	};

	static const gchar * const documenters[] = {
		"Sun GNOME Documentation Team <gdocteam@sun.com>",
		"Eric Baudais <baudais@okstate.edu>",
		NULL
	};

	static const gchar copyright[] = \
		"Copyright \xc2\xa9 1998-2000 Evan Lawrence, Alex Robert\n"
		"Copyright \xc2\xa9 2000-2002 Chema Celorio, Paolo Maggi\n"
		"Copyright \xc2\xa9 2003-2006 Paolo Maggi";

	static const gchar comments[] = \
		N_("gedit is a small and lightweight text editor for the "
		   "GNOME Desktop");

	GdkPixbuf *logo;

	gedit_debug (DEBUG_COMMANDS);

	logo = gdk_pixbuf_new_from_file (GNOME_ICONDIR "/gedit-logo.png", NULL);

	gtk_show_about_dialog (GTK_WINDOW (window),
			       "authors", authors,
			       "comments", _(comments),
			       "copyright", copyright,
			       "documenters", documenters,
			       "logo", logo,
			       "translator-credits", _("translator-credits"),
			       "version", VERSION,
			       "website", "http://www.gedit.org",
			       "name", _("gedit"),
			       NULL);

	if (logo)
		g_object_unref (logo);
}
