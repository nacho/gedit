/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gEdit - Utility functions
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
 *
 * Utility functions by Jason Leach <leach@wam.umd.edu>
 */

#include <gnome.h>
#include "gedit-utils.h"
#include "main.h"
#include "gE_mdi.h"

/**
 * gedit_set_title:
 * @doc: Document that's active right now, what we're setting the
 * title to
 *
 * Set the title to "$filename - $gedit_ver" and if the document has
 * changed, lets show that it has. 
 */
void
gedit_set_title (gedit_document *doc)
{
	gchar *title;

	g_return_if_fail (doc != NULL);

	if (doc->changed)
		title = g_strdup_printf ("%s (modified) - %s", GNOME_MDI_CHILD (gedit_document_current())->name, GEDIT_ID);
	else
		title = g_strdup_printf ("%s - %s", GNOME_MDI_CHILD (gedit_document_current())->name, GEDIT_ID);

	gtk_window_set_title (GTK_WINDOW (mdi->active_window), title);

	g_free (title);
}

/**
 * gedit_flash:
 * @msg: Message to flash on the statusbar
 *
 * Flash a temporary message on the statusbar of gedit.
 */
void
gedit_flash (gchar *msg)
{
	g_return_if_fail (msg != NULL);

	gnome_app_flash (mdi->active_window, msg);
}

/**
 * gedit_flash_va:
 * @format
 */
void
gedit_flash_va (gchar *format, ...)
{
	va_list args;
	gchar *msg;

	g_return_if_fail (format != NULL);

	va_start (args, format);
	msg = g_strdup_vprintf (format, args);
	va_end (args);

	gedit_flash (msg);
	g_free (msg);
}

/**
 * gedit_debug_mess:
 * @msg: Message to flash on the statusbar
 *
 * Flash a temporary message on the statusbar of gedit.
 */
void gedit_debug_mess( gchar * message, gint type )
{
     g_print( message);
}









