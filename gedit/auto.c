/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit
 *
 * Copyright (C) 1998, 1999 Alex Roberts, Evan Lawrence
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include <glib.h>
#include <string.h> /* For strcat() */
#include <libgnome/gnome-defs.h>
#include <gtk/gtkwidget.h>


#include "auto.h"
#include "view.h"
#include "document.h"
#include "utils.h"
#include "prefs.h"

/* This function is so ugly that I had to move it somewhere else where I wont see it
   again. Chema */

gboolean
auto_indent_cb (GtkWidget *text, char *insertion_text, int length,
			 int *pos, gpointer data)
{
	int i, newlines, newline_1;
	gchar *buffer, *whitespace;
	GeditView *view; 
	GeditDocument *doc;

	gedit_debug (DEBUG_VIEW, "");

	view = GEDIT_VIEW (data);

	g_return_val_if_fail (view != NULL, FALSE);

	if (!settings->auto_indent)
		return FALSE;
	if ((length != 1) || (insertion_text[0] != '\n'))
		return FALSE;
	if (gtk_text_get_length (GTK_TEXT (text)) <=1)
		return FALSE;

	doc = view->doc;

	newlines = 0;
	newline_1 = 0;
	
	for (i = *pos; i > 0; i--)
	{
		buffer = gtk_editable_get_chars (GTK_EDITABLE (text), i-1, i);
		
		if (buffer == NULL)
			continue;
		if (buffer[0] == 10)
		{
			if (newlines > 0)
			{
				g_free (buffer);
				buffer = NULL;
				break;
			}
			else
			{
				newlines++;
				newline_1 = i;
				g_free (buffer);
				buffer = NULL;
			}
		}
		
		g_free (buffer);
		buffer = NULL;
	}

	whitespace = g_malloc0 (newline_1 - i + 2);

	for (i = i; i <= newline_1; i++)
	{
		buffer = gtk_editable_get_chars (GTK_EDITABLE (text), i, i+1);
		
		if ((buffer[0] != 32) & (buffer[0] != 9))
		{
			g_free (buffer);
			buffer = NULL;
			break;
		}
		
		strncat (whitespace, buffer, 1);
		g_free (buffer);
		
	}

	if (strlen(whitespace) > 0)
	{
		i = *pos;
		gtk_text_set_point (GTK_TEXT (text), i);
		gtk_text_insert (GTK_TEXT (text), NULL, NULL, NULL,
				 whitespace, strlen (whitespace));
	}
	g_free (whitespace);

	return TRUE;
}
