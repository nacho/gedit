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


static gint file_saveas_destroy(GtkWidget *w, GtkWidget **sel);
static gint file_cancel_sel (GtkWidget *w, GtkFileSelection *fs);
static void file_sel_destroy (GtkWidget *w, GtkFileSelection *fs);

/* static GtkWidget *open_fs, *save_fs; */
GtkWidget *col_label;
gchar *oname = NULL;


/* popup to handle new file creation from nothing. */
gboolean
popup_create_new_file (GtkWidget *w, gchar *title)
{
	GtkWidget *msgbox;
	Document *doc;
	int ret;
	char *msg;

	msg = g_strdup_printf (_("The file ``%s'' does not exist.  Would you like to create it?"),
			       title);

	msgbox = gnome_message_box_new (msg,
					GNOME_MESSAGE_BOX_QUESTION,
					GNOME_STOCK_BUTTON_YES,
					GNOME_STOCK_BUTTON_NO,
					NULL);
	
	ret = gnome_dialog_run_and_close (GNOME_DIALOG (msgbox));

	switch (ret)
	{
	/* yes */
	case 0 : 
		doc = gedit_document_new_with_title (title);
		gnome_mdi_add_child (mdi, GNOME_MDI_CHILD (doc));
		gnome_mdi_add_view (mdi, GNOME_MDI_CHILD (doc));
		
		if (gedit_file_save (doc, title))
		{
			gedit_flash (_("Could not save file!"));
			return FALSE;
		}
		return TRUE;
	/* no */
	case 1 : 
		return FALSE;

	/* error */
	default: 
		g_assert_not_reached ();
	} 
}

/* --- Notebook Tab Stuff --- */

void
tab_pos (GtkPositionType pos)
{
	gint i;
	GnomeApp *app;
	GtkWidget *book;
	
	if (mdiMode == GNOME_MDI_NOTEBOOK)
		for (i = 0; i < g_list_length (mdi->windows); i++)
		{
			app = g_list_nth_data (mdi->windows, i);
	  
			book = app->contents;
	  
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

/* ---- Auto-indent Callback(s) --- */

void
auto_indent_toggle_cb (GtkWidget *w, gpointer cbdata)
{
	gedit_window_set_auto_indent (!settings->auto_indent);
}


/* --- Drag and Drop Callback(s) --- */

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

#if 0
/*
 * file save-as callback : user selects "Ok"
 *
 * data->temp1 must be the file saveas dialog box
 */
static void
file_saveas_ok_sel (GtkWidget *w, gedit_data *data)
{
	gchar *fname = g_strdup(gtk_file_selection_get_filename (GTK_FILE_SELECTION(ssel)));
	Document *doc;

	if (mdi->active_child == NULL)
	{
		g_free (fname);
		return;
	}		  	
	
	doc = gedit_document_current ();
	
	if (fname)
	{
		if (gedit_file_save(doc, fname) != 0) 
			gedit_flash (_("Error saving file!"));
	}

	g_free (fname);
	gtk_widget_destroy (GTK_WIDGET (ssel));
	ssel = NULL;
}

/*
 * destroy the "save as" dialog box
 */
static gint
file_saveas_destroy (GtkWidget *w, GtkWidget **sel)
{
	gtk_widget_destroy (*sel);
	*sel = NULL;
	
	return TRUE;
}

void
file_save_as_cb (GtkWidget *widget, gpointer cbdata)
{
	gchar *title;
	
	title = g_strdup_printf (_("Save %s As..."), (gchar*) cbdata);

	ssel = gtk_file_selection_new (title);

	gtk_signal_connect (GTK_OBJECT(GTK_FILE_SELECTION(ssel)->ok_button),
			    "clicked",
			    GTK_SIGNAL_FUNC (file_saveas_ok_sel),
			    NULL);

	gtk_signal_connect (GTK_OBJECT(GTK_FILE_SELECTION(ssel)->cancel_button),
			    "clicked",
			    GTK_SIGNAL_FUNC (file_saveas_destroy),
			    &ssel);


	gtk_widget_show (ssel);

	g_free (title);
}

/*
 * file save-all-as callback : user selects "Ok"
 *
 * data->temp1 must be the file saveas dialog box
 */
static void
file_save_all_as_ok_sel (GtkWidget *w, GtkFileSelection *fs)
{
	gchar *fname = g_strdup(gtk_file_selection_get_filename (GTK_FILE_SELECTION(fs)));
	Document *doc;

	if (mdi->active_child == NULL)
	{
		g_free (fname);
		return;
	}	  
	
	doc = gedit_document_current();
	
	if (fname)
	{
		if (gedit_file_save(doc, fname) != 0) 
			gnome_app_flash (mdi->active_window, _("Error saving file!"));
	}

	gtk_widget_destroy (GTK_WIDGET (fs)); 
	fs = NULL;
	g_free (fname);	
} 

/* Destroy the "Save All As" dialog box */
static gint
file_save_all_as_destroy (GtkWidget *w, GtkFileSelection *fs)
{
	gtk_widget_destroy (GTK_WIDGET (fs)); 
	fs = NULL;
	
	return TRUE;
}

/* "Save All As" dialog box */
void 
file_save_all_as_cb (GtkWidget *widget, gpointer cbdata)
{
	GtkWidget *fs = NULL; 
	gchar *title;

	title = g_strdup_printf (_("Save %s As ..."), (gchar *)cbdata);
	fs = gtk_file_selection_new (title);

	gtk_signal_connect (GTK_OBJECT(GTK_FILE_SELECTION(fs)->ok_button),
			    "clicked",
			    GTK_SIGNAL_FUNC (file_save_all_as_ok_sel),
			    fs);
						
	gtk_signal_connect (GTK_OBJECT(GTK_FILE_SELECTION(fs)->cancel_button),
			    "clicked",
			    GTK_SIGNAL_FUNC (file_save_all_as_destroy),
			    fs);


	gtk_widget_show(fs);

	g_free (title);	
}

/*
 * file close callback (used from menus.c)
 */
void
file_close_cb (GtkWidget *widget, gpointer cbdata)
{
	Document *doc;
	
	if (mdi->active_child == NULL)
		return;
	
	if (gnome_mdi_remove_child (mdi, mdi->active_child, FALSE))
	{
		if (mdi->active_child == NULL)
		{
			if (!settings->close_doc)
			{
				doc = gedit_document_new ();
				gnome_mdi_add_child (mdi, GNOME_MDI_CHILD (doc));
				gnome_mdi_add_view (mdi, GNOME_MDI_CHILD (doc));
			}
			else
				g_assert_not_reached ();
		}
	}
}

/*
 * file close all callback (used from menus.c)
 */
void
file_close_all_cb (GtkWidget *widget, gpointer cbdata)
{
	Document *doc;

	if (gnome_mdi_remove_all (mdi, FALSE))
	{
		if (mdi->active_child == NULL)
		{
			/* if there are no open documents create a blank one */
			if (g_list_length(mdi->children) == 0)
			{
				doc = gedit_document_new ();
				gnome_mdi_add_child (mdi, GNOME_MDI_CHILD (doc));
				gnome_mdi_add_view  (mdi, GNOME_MDI_CHILD (doc));
			}
		}
	}
}

gboolean
file_revert_do (Document *doc)
{
	GtkWidget *msgbox;
	gchar *msg;
	gint ret;

	msg = g_strdup_printf (_("Are you sure you wish to revert all changes?\n(%s)"),
			       doc->filename);
	
	msgbox = gnome_message_box_new (msg,
					GNOME_MESSAGE_BOX_QUESTION,
					GNOME_STOCK_BUTTON_YES,
					GNOME_STOCK_BUTTON_NO,
					NULL);
	    							  
	    							  
	gnome_dialog_set_default (GNOME_DIALOG (msgbox), 2);
	ret = gnome_dialog_run_and_close (GNOME_DIALOG (msgbox));
	    	    
	switch (ret)
	{
	case 0:
		gedit_file_open (doc, doc->filename);
		return TRUE;
	
	case 1:
		return FALSE;
	
	default:
		return FALSE;
			
	}
}

/* File revertion callback */
void
file_revert_cb (GtkWidget *widget, gpointer cbdata)
{
	Document *doc;
	
	doc = gedit_document_current ();

	if (doc->filename)
	{
		if (doc->changed)
		{
			if ((file_revert_do (doc)) == 0)
				return;
		}
		else
			gnome_app_flash (mdi->active_window, _("Document Unchanged..."));
	  
	}
	else
		gnome_app_flash (mdi->active_window, _("Document Unsaved..."));
}
#endif


void
gedit_shutdown (GtkWidget *widget, gpointer data)
{
	gedit_save_settings ();

	if (gnome_mdi_remove_all (mdi, FALSE))
		gtk_object_destroy (GTK_OBJECT (mdi));
	else
		return;

	gtk_main_quit ();
}


/* ---- Clipboard Callbacks ---- */

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

/*
void
tab_toggle_cb(GtkWidget *widget, gpointer cbwindow)
{

	w->show_tabs = !w->show_tabs;
	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(w->notebook), w->show_tabs);
}
*/
