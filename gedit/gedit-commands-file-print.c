/*
 * gedit-commands-file-print.c
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
#include "gedit-window.h"
#include "gedit-debug.h"
#include "gedit-print.h"
#include "dialogs/gedit-page-setup-dialog.h"


void
_gedit_cmd_file_page_setup (GtkAction   *action,
			   GeditWindow *window)
{
	gedit_debug (DEBUG_COMMANDS);

	gedit_show_page_setup_dialog (GTK_WINDOW (window));
}

void
_gedit_cmd_file_print_preview (GtkAction   *action,
			      GeditWindow *window)
{
	GeditDocument *doc;
	GeditTab      *tab;
	GeditPrintJob *pjob;
	GtkTextIter    start;
	GtkTextIter    end;

	gedit_debug (DEBUG_COMMANDS);

	tab = gedit_window_get_active_tab (window);
	if (tab == NULL)
		return;

	doc = gedit_tab_get_document (tab);

	pjob = gedit_print_job_new (doc);

	gtk_text_buffer_get_bounds (GTK_TEXT_BUFFER (doc) , &start, &end);

	_gedit_tab_print_preview (tab, pjob, &start, &end);
	g_object_unref (pjob);
}

static void
print_dialog_response_cb (GtkWidget *dialog,
			  gint response,
			  GeditPrintJob *pjob)
{
	GtkTextIter          start;
	GtkTextIter          end;
	gint                 line_start;
	gint                 line_end;
	GnomePrintRangeType  range_type;
	GtkTextBuffer       *buffer;
	GeditTab            *tab;

	gedit_debug (DEBUG_COMMANDS);

	range_type = gnome_print_dialog_get_range (GNOME_PRINT_DIALOG (dialog));

	buffer = GTK_TEXT_BUFFER (
			gtk_source_print_job_get_buffer (GTK_SOURCE_PRINT_JOB (pjob)));

	gtk_text_buffer_get_bounds (buffer, &start, &end);

	tab = gedit_tab_get_from_document (GEDIT_DOCUMENT (buffer));

	switch (range_type)
	{
		case GNOME_PRINT_RANGE_ALL:
			break;

		case GNOME_PRINT_RANGE_SELECTION:
			gtk_text_buffer_get_selection_bounds (buffer,
							      &start,
							      &end);
			break;

		case GNOME_PRINT_RANGE_RANGE:
			gnome_print_dialog_get_range_page (GNOME_PRINT_DIALOG (dialog),
							   &line_start,
							   &line_end);

			/* The print dialog should ensure to set the
			 * sensitivity of the spin buttons so that
			 * the start and end lines are in ascending
			 * order, but it doesn't.
			 * We reorder them if needed */
			if (line_start > line_end)
			{
				gint tmp;

				gedit_debug_message (DEBUG_PRINT,
						     "line start: %d, line end: %d. Swapping.",
						     line_start,
						     line_end);

				tmp = line_start;
				line_start = line_end;
				line_end = tmp;
			}

			gtk_text_iter_set_line (&start, line_start - 1);
			gtk_text_iter_set_line (&end, line_end - 1);

			gtk_text_iter_forward_to_line_end (&end);

			break;

		default:
			g_return_if_reached ();
	}

	switch (response)
	{
		case GNOME_PRINT_DIALOG_RESPONSE_PRINT:
			gedit_debug_message (DEBUG_PRINT,
					     "Print button pressed.");

			_gedit_tab_print (tab, pjob, &start, &end);

			break;

		case GNOME_PRINT_DIALOG_RESPONSE_PREVIEW:
			gedit_debug_message (DEBUG_PRINT,
					     "Preview button pressed.");

			_gedit_tab_print_preview (tab, pjob, &start, &end);

			break;
        }

        g_object_unref (pjob);
	gtk_widget_destroy (dialog);
}

void
_gedit_cmd_file_print (GtkAction   *action,
		      GeditWindow *window)
{
	GeditDocument *doc;
	GeditPrintJob *pjob;
	GtkWidget *print_dialog;
	GtkWindowGroup *wg;

	gedit_debug (DEBUG_COMMANDS);

	doc = gedit_window_get_active_document (window);
	if (doc == NULL)
		return;

	pjob = gedit_print_job_new (doc);

	print_dialog = gedit_print_dialog_new (pjob);

	wg = gedit_window_get_group (window);

	gtk_window_group_add_window (wg,
				     GTK_WINDOW (print_dialog));

	gtk_window_set_transient_for (GTK_WINDOW (print_dialog),
				      GTK_WINDOW (window));
	gtk_window_set_modal (GTK_WINDOW (print_dialog), TRUE);

	g_signal_connect (print_dialog,
			  "response",
			  G_CALLBACK (print_dialog_response_cb),
			  pjob);

	gtk_widget_show (print_dialog);
}

