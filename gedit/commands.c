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
#include <gnome.h>

#include <unistd.h>
#define __need_sigset_t
#include <signal.h>
#define __need_timespec
#include <time.h>
#include <signal.h>


#include "window.h"
#include "view.h"
#include "commands.h"
#include "document.h"
#include "prefs.h"
#include "file.h"
#include "search.h"
#include "utils.h"

#include "dialogs/dialogs.h"

GtkWidget *col_label;
gchar *oname = NULL;


void
tab_pos (GtkPositionType pos)
{
	gint i;
	GnomeApp *app;
	GtkWidget *book;
	
	if ((settings->mdi_mode != GNOME_MDI_NOTEBOOK)
	    && (settings->mdi_mode != GNOME_MDI_DEFAULT_MODE))
		return;
	
	for (i = 0; i < g_list_length (mdi->windows); i++)
	{
		app = g_list_nth_data (mdi->windows, i);
		book = app->contents;
		if (GTK_IS_NOTEBOOK (book))
			gtk_notebook_set_tab_pos (GTK_NOTEBOOK(book), pos);
	}
}
   
void
tab_top_cb (GtkWidget *widget, gpointer cbwindow)
{
	mdi->tab_pos = GTK_POS_TOP;
	settings->tab_pos = GTK_POS_TOP;
	
	tab_pos (GTK_POS_TOP);
}

void
tab_bottom_cb (GtkWidget *widget, gpointer cbwindow)
{
	mdi->tab_pos =  GTK_POS_BOTTOM;
	settings->tab_pos = GTK_POS_BOTTOM;
	
	tab_pos (GTK_POS_BOTTOM);
}

void
tab_left_cb (GtkWidget *widget, gpointer cbwindow)
{
	mdi->tab_pos =  GTK_POS_LEFT;
	settings->tab_pos = GTK_POS_LEFT;
	
	tab_pos (GTK_POS_LEFT);
}

void
tab_right_cb (GtkWidget *widget, gpointer cbwindow)
{
	mdi->tab_pos =  GTK_POS_RIGHT;
	settings->tab_pos = GTK_POS_RIGHT;
	
	tab_pos (GTK_POS_RIGHT);
}

void
filenames_dropped (GtkWidget        *widget, GdkDragContext   *context, gint x, gint y,
                   GtkSelectionData *selection_data, guint info, guint time)
{
	GList *names, *list;

	gedit_debug (DEBUG_SEARCH, "");

	list = gnome_uri_list_extract_filenames (selection_data->data);
	names = list;

	while (names != NULL)
	{
		gchar *filename;
		filename = g_strdup ((gchar *)names->data);
		gedit_document_new_with_file (filename);
		g_free (filename);
		names = names->next;
	}

	gnome_uri_list_free_strings (list);

}

void
window_new_cb (GtkWidget *widget, gpointer cbdata)
{
	/* I'm not sure about this.. it appears correct */
	gnome_mdi_open_toplevel (mdi);
}

void
edit_cut_cb (GtkWidget *widget, gpointer data)
{
	if (!gedit_document_current())
		return;

	gtk_editable_cut_clipboard (gedit_editable_active());

	gnome_app_flash (gedit_window_active_app(), MSGBAR_CUT);
}

void
edit_copy_cb (GtkWidget *widget, gpointer data)
{
	GeditView *view;
	GtkEditable *editable;

	if (!gedit_document_current())
		return;
		
	view = gedit_view_active();

	if (view == NULL)
		return;

	if (!GEDIT_IS_VIEW (view)) {
		g_warning ("Error while pasting. \"view\" is not a Gedit View");
		return;
	}

	editable = GTK_EDITABLE (view->text);
	if (!GTK_IS_EDITABLE (editable)) {
		g_warning ("Error while pasting. \"editable\" is not a GtkEditable");
		return;
	}

	gtk_editable_copy_clipboard (editable);

	gnome_app_flash (gedit_window_active_app(), MSGBAR_COPY);
}
	
void
edit_paste_cb (GtkWidget *widget, gpointer data)
{
	GeditView *view;
	GtkEditable *editable;

	if (!gedit_document_current())
		return;

	view = gedit_view_active();

	if (view == NULL)
		return;

	if (!GEDIT_IS_VIEW (view)) {
		g_warning ("Error while pasting. \"view\" is not a Gedit View");
		return;
	}

	editable = GTK_EDITABLE (view->text);
	if (!GTK_IS_EDITABLE (editable)) {
		g_warning ("Error while pasting. \"editable\" is not a GtkEditable");
		return;
	}

	gtk_editable_paste_clipboard (editable);

	gnome_app_flash (gedit_window_active_app(), MSGBAR_PASTE);
}

void
edit_select_all_cb (GtkWidget *widget, gpointer data)
{
	GeditDocument *doc = gedit_document_current();
	
	if (doc == NULL)
		return;
	
	gtk_editable_select_region (gedit_editable_active(), 0,
				    gedit_document_get_buffer_length(doc));

	gnome_app_flash (gedit_window_active_app(), MSGBAR_SELECT_ALL);
}


void
options_toggle_read_only_cb (GtkWidget *widget, gpointer data)
{
	GeditView *view = gedit_view_active();

	if (!gedit_document_current())
		return;

	gedit_view_set_readonly (view, !view->readonly);
}

void
options_toggle_status_bar_cb (GtkWidget *w, gpointer data)
{
	if (!gedit_document_current())
		return;

	gedit_window_set_status_bar (gedit_window_active_app());

}
