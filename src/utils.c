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
 *
 * Authors: 
 *   Jason Leach <leach@wam.umd.edu>
 *   Chema Celorio <chema@celorio.com>
 */

#include <config.h>
#include <gnome.h>

#include "document.h"
#include "utils.h"


int
gtk_radio_group_get_selected (GSList *radio_group)
{
	GSList *l;
	int i, c;

	g_return_val_if_fail (radio_group != NULL, 0);

	c = g_slist_length (radio_group);

	for (i = 0, l = radio_group; l; l = l->next, i++){
		GtkRadioButton *button = l->data;

		if (GTK_TOGGLE_BUTTON (button)->active)
			return c - i - 1;
	}

	return 0;
}

void
gtk_radio_button_select (GSList *group, int n)
{
	GSList *l;
	int len = g_slist_length (group);

	l = g_slist_nth (group, len - n - 1);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (l->data), 1);
}


/**
 * gedit_set_title:
 * @docname : Document name in a string, the new title
 *
 * Set the title to "$docname - $gedit_ver" and if the document has
 * changed, lets show that it has. 
 **/
void
gedit_set_title (Document *doc)
{
	gchar *title;
	gchar *docname;

	g_return_if_fail (doc != NULL);

	docname = GNOME_MDI_CHILD (doc)->name;

	if (doc->changed)
		title = g_strdup_printf ("gedit: %s (modified)", docname);
	else
		title = g_strdup_printf ("gedit: %s", docname);

	gtk_window_set_title (GTK_WINDOW (mdi->active_window), title);

	g_free (title);
}

/**
 * gedit_flash:
 * @msg: Message to flash on the statusbar
 *
 * Flash a temporary message on the statusbar of gedit.
 **/
void
gedit_flash (gchar *msg)
{
	g_return_if_fail (msg != NULL);

	gnome_app_flash (mdi->active_window, msg);
}

/**
 * gedit_flash_va:
 * @format:
 **/
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








/* This is just a stopwatch. Chema
   I need it to time search functions but
   it can be removed after I am done with
   the search stuff */ 
#include <time.h>

#define MAX_TIME_STRING 100
typedef struct {
	clock_t begin_clock, save_clock;
	time_t  begin_time, save_time;
} time_keeper;

static time_keeper tk;

void start_time (void)
{
	tk.begin_clock = tk.save_clock = clock ();
	tk.begin_time = tk.save_time = time (NULL);
}

double
print_time (void)
{
	gchar s1 [MAX_TIME_STRING], s2 [MAX_TIME_STRING];
	gint  field_width, n1, n2;
	gdouble clocks_per_second = (double) CLOCKS_PER_SEC, user_time, real_time;

	user_time = (clock() - tk.save_clock) / clocks_per_second;
	real_time = difftime (time (NULL), tk.save_time);
	tk.save_clock = clock ();
	tk.save_time = time (NULL);

	/* print the values ... */
	n1 = sprintf (s1, "%.5f", user_time);
	n2 = sprintf (s2, "%.5f", real_time);
	field_width = (n1 > n2)?n1:n2;
	g_print ("%s%*.5f%s\n%s%*.5f%s\n\n",
		 "User time : ", field_width, user_time, " seconds",
		 "Real time : ", field_width, real_time, " seconds");
	return user_time;
}
