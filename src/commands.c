/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * gedit
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

#include <unistd.h>
#define __need_sigset_t
#include <signal.h>
#define __need_timespec
#include <time.h>
#include <signal.h>

#include "window.h"
#include "gedit.h"
#include "view.h"
#include "commands.h"
#include "document.h"
#include "prefs.h"
#include "file.h"
#include "search.h"
#include "utils.h"


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
tab_bot_cb (GtkWidget *widget, gpointer cbwindow)
{
	mdi->tab_pos =  GTK_POS_BOTTOM;
	settings->tab_pos = GTK_POS_BOTTOM;
	
	tab_pos (GTK_POS_BOTTOM);
}

void
tab_lef_cb (GtkWidget *widget, gpointer cbwindow)
{
	mdi->tab_pos =  GTK_POS_LEFT;
	settings->tab_pos = GTK_POS_LEFT;
	
	tab_pos (GTK_POS_LEFT);
}

void
tab_rgt_cb (GtkWidget *widget, gpointer cbwindow)
{
	mdi->tab_pos =  GTK_POS_RIGHT;
	settings->tab_pos = GTK_POS_RIGHT;
	
	tab_pos (GTK_POS_RIGHT);
}

/* FIXME: implement it. Chema *
void
tab_none_cb(GtkWidget *widget, gpointer cbwindow)
{
	w->show_tabs = !w->show_tabs;
	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(w->notebook), w->show_tabs);
}*/ 

void
auto_indent_toggle_cb (GtkWidget *w, gpointer cbdata)
{
	gedit_window_set_auto_indent (!settings->auto_indent);
}


void
filenames_dropped (GtkWidget        *widget,
                   GdkDragContext   *context,
                   gint              x,
                   gint              y,
                   GtkSelectionData *selection_data,
                   guint             info,
                   guint             time)
{
	GList *names, *tmp_list;
	Document *doc;

	gedit_debug_mess("commands.c : filenames_dropped", DEBUG_FILE);
	
	names = gnome_uri_list_extract_filenames ((char *)selection_data->data);
	tmp_list = names;

	while (tmp_list)
	{
		doc = gedit_document_new_with_file ((gchar *)tmp_list->data);
		gnome_mdi_add_child (mdi, GNOME_MDI_CHILD (doc));
	        gnome_mdi_add_view (mdi, GNOME_MDI_CHILD (doc));
	        
		tmp_list = tmp_list->next;
	}
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
	gtk_editable_cut_clipboard(GTK_EDITABLE(
		VIEW (mdi->active_view)->text));

	gnome_app_flash (mdi->active_window, MSGBAR_CUT);
}

void
edit_copy_cb (GtkWidget *widget, gpointer data)
{
	gtk_editable_copy_clipboard(
		GTK_EDITABLE(VIEW (mdi->active_view)->text));

	gnome_app_flash (mdi->active_window, MSGBAR_COPY);
}
	
void
edit_paste_cb (GtkWidget *widget, gpointer data)
{
	gtk_editable_paste_clipboard(
		GTK_EDITABLE(VIEW (mdi->active_view)->text));

	gnome_app_flash (mdi->active_window, MSGBAR_PASTE);
}

void
edit_selall_cb (GtkWidget *widget, gpointer data)
{
	gtk_editable_select_region(GTK_EDITABLE(VIEW (mdi->active_view)->text), 0,
				   gtk_text_get_length (GTK_TEXT(VIEW (mdi->active_view)->text)));

	gnome_app_flash (mdi->active_window, MSGBAR_SELECT_ALL);
}



void
options_toggle_split_screen_cb (GtkWidget *widget, gpointer data)
{
	View *view = VIEW (mdi->active_view);

	if (!view->split_parent)
		return;

	gedit_view_set_split_screen
		(view, !GTK_WIDGET_VISIBLE (view->split_parent));
}

void
options_toggle_read_only_cb (GtkWidget *widget, gpointer data)
{
	View *view = VIEW (mdi->active_view);
	
	gedit_view_set_read_only (view, !view->read_only);
}

void
options_toggle_word_wrap_cb (GtkWidget *widget, gpointer data)
{
	View *view = VIEW (mdi->active_view);
	
	gedit_view_set_word_wrap (view, !view->word_wrap);
}

void
options_toggle_line_wrap_cb (GtkWidget *widget, gpointer data)
{
	View *view = VIEW (mdi->active_view);

	gedit_view_set_line_wrap (view, !view->line_wrap);
}

void
options_toggle_status_bar_cb (GtkWidget *w, gpointer data)
{
	gedit_window_set_status_bar (!settings->show_status);
}


