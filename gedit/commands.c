/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* vi:set ts=8 sts=0 sw=8:
 *
 * gEdit
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
#include <libgnome/gnome-history.h>

#include <unistd.h>
#define __need_sigset_t
#include <signal.h>
#define __need_timespec
#include <time.h>
#include <signal.h>

#include "gedit.h"
#include "gE_view.h"
#include "commands.h"
#include "gE_mdi.h"
#include "gedit-window.h"
#include "gE_prefs.h"
#include "gedit-file-io.h"
#include "gedit-search.h"
#include "gedit-utils.h"


static gint file_saveas_destroy(GtkWidget *w, GtkWidget **sel);
static gint file_cancel_sel (GtkWidget *w, GtkFileSelection *fs);
static void file_sel_destroy (GtkWidget *w, GtkFileSelection *fs);
static void recent_update_menus (GnomeApp *app, GList *recent_files);
static void recent_cb (GtkWidget *w, gedit_data *data);

/* static GtkWidget *open_fs, *save_fs; */
GtkWidget *col_label;
gchar *oname = NULL;


/* popup to handle new file creation from nothing. */
gboolean
popup_create_new_file (GtkWidget *w, gchar *title)
{
	GtkWidget *msgbox;
	gedit_document *doc;
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
	gedit_document *doc;

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
	gedit_document *doc;

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
	gedit_document *doc;

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
	gedit_document *doc;
	
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
	gedit_document *doc;

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
file_revert_do (gedit_document *doc)
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
	gedit_document *doc;
	
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
edit_cut_cb (GtkWidget *widget, gpointer cbdata)
{
	gtk_editable_cut_clipboard(GTK_EDITABLE(
		GE_VIEW (mdi->active_view)->text));

	gnome_app_flash (mdi->active_window, MSGBAR_CUT);
}

void
edit_copy_cb (GtkWidget *widget, gpointer cbdata)
{
	gtk_editable_copy_clipboard(
		GTK_EDITABLE(GE_VIEW (mdi->active_view)->text));

	gnome_app_flash (mdi->active_window, MSGBAR_COPY);
}
	
void
edit_paste_cb (GtkWidget *widget, gpointer cbdata)
{
	gtk_editable_paste_clipboard(
		GTK_EDITABLE(GE_VIEW (mdi->active_view)->text));

	gnome_app_flash (mdi->active_window, MSGBAR_PASTE);
}

void
edit_selall_cb (GtkWidget *widget, gpointer cbdata)
{
	gtk_editable_select_region(GTK_EDITABLE(GE_VIEW (mdi->active_view)->text), 0,
				   gtk_text_get_length( GTK_TEXT(GE_VIEW (mdi->active_view)->text)));

	gnome_app_flash (mdi->active_window, MSGBAR_SELECT_ALL);
}


/**
 * recent_update:
 * @app: 
 *
 * Grabs the list of recently used documents, then updates the menus by
 * calling recent_update_menus().  Should be called after each addition
 * to the recent documents list.
 **/
void
recent_update (GnomeApp *app)
{
	GList *filelist = NULL;
	GList *gnome_recent_list;
	GnomeHistoryEntry histentry;
	char *filename;
	int i, j;
	int nrecentdocs = 0;

	filelist = NULL;
	gnome_recent_list = gnome_history_get_recently_used ();
	
	if (g_list_length (gnome_recent_list) > 0)
	{
		for (i = g_list_length (gnome_recent_list) - 1; i >= 0; i--)
		{
			histentry = g_list_nth_data (gnome_recent_list, i);
	     
			if (strcmp ("gEdit", histentry->creator) == 0)
			{
				/* This is to make sure you don't have more than one
				   file of the same name in the recent list
				*/
	      
				if (g_list_length (filelist) > 0)
				{
					for (j = g_list_length (filelist) - 1; j >= 0; j--)
					{
						if (strcmp (histentry->filename, g_list_nth_data (filelist, j)) == 0)
						{
							filelist = g_list_remove (filelist, g_list_nth_data (filelist, j));
							nrecentdocs--;
						}
					}
				}

				filename = g_malloc0 (strlen (histentry->filename) + 1);
				strcpy (filename, histentry->filename);
				filelist = g_list_append (filelist, filename);
				nrecentdocs++;
				
				/* For recent-directories, not yet fully implemented...
		   
				   end_path = strrchr (histentry->filename, '/');
				   if (end_path) {
		   
				   for (i = 0; i < strlen (histentry->filename); i++)
				   if ((histentry->filename + i) == end_path)
				   break;
				
				   directory = g_malloc0 (i + 2);
				   strcat (directory, histentry->filename, i);
			
				   }
				*/
                   
				if (nrecentdocs == MAX_RECENT)
/*				if (g_list_length (filelist) == MAX_RECENT) */
					break;
			}
		}
	}
	
	gnome_history_free_recently_used_list (gnome_recent_list);
	
	recent_update_menus (app, filelist);
}

/* Actually updates the recent-used menu... */

static void
recent_update_menus (GnomeApp *app, GList *recent_files)
{
	GnomeUIInfo *menu;
	gedit_data *data;
	gchar *path;
	int i;

	g_return_if_fail (app != NULL);

	if (settings->num_recent)
		gnome_app_remove_menu_range (app, _("_File/"), 6, settings->num_recent + 1);

	if (recent_files == NULL)
		return;


	/* insert a separator at the beginning */
	
	menu = g_malloc0 (2 * sizeof (GnomeUIInfo));
	path = g_new (gchar, strlen (_("_File")) + strlen ("<Separator>") + 3 );
	sprintf (path, "%s/%s", _("_File"), "<Separator>");
	menu->type = GNOME_APP_UI_SEPARATOR;

	(menu + 1)->type = GNOME_APP_UI_ENDOFINFO;
	gnome_app_insert_menus (GNOME_APP(app), path, menu);


	for (i = g_list_length (recent_files) - 1; i >= 0;  i--)
	{
		menu = g_malloc0 (2 * sizeof (GnomeUIInfo));
	
		data = g_malloc0 (sizeof (gedit_data));
		data->temp1 = g_strdup (g_list_nth_data (recent_files, i));
	
		menu->label = g_new (gchar, strlen (g_list_nth_data (recent_files, i)) + 5);
		sprintf (menu->label, "_%i. %s", i+1, (gchar*)g_list_nth_data (recent_files, i));
		menu->type = GNOME_APP_UI_ITEM;
		menu->hint = NULL;
		menu->moreinfo = (gpointer) recent_cb;
		menu->user_data = data;
		menu->unused_data = NULL;
		menu->pixmap_type = 0;
		menu->pixmap_info = NULL;
		menu->accelerator_key = 0;

		(menu + 1)->type = GNOME_APP_UI_ENDOFINFO;
	
		gnome_app_insert_menus (GNOME_APP(app), path, menu);
		g_free (g_list_nth_data (recent_files, i));	 
	}
	
	g_free (menu);
	settings->num_recent = g_list_length (recent_files);
	g_list_free (recent_files);

	g_free (path);
}

/* Callback for a users click on one of the recent docs in the File menu */
static void
recent_cb (GtkWidget *widget, gedit_data *data)
{
	gedit_document *doc = gedit_document_current ();
	
	g_return_if_fail (data != NULL);

	if (doc)
	{
		if (doc->filename || GE_VIEW(mdi->active_view)->changed)
		{
			doc = gedit_document_new_with_file (data->temp1);
			gnome_mdi_add_child (mdi, GNOME_MDI_CHILD (doc));
			gnome_mdi_add_view (mdi, GNOME_MDI_CHILD (doc));
	    
		}
		else
		{
			/* FIXME.jel: add some conditional check and
			   an else to handle the case where the file
			   in the recent menu fails to open
			   (deleted/moved/new permissions) */
			gedit_file_open (doc, data->temp1);
		}
	}
}

/**
 * recent_add:
 * @filename: Filename of document to add to the recently accessed list
 *
 * Record a file in GNOME's recent documents database 
 **/
void
recent_add (char *filename)
{
	g_return_if_fail (filename != NULL);

	gnome_history_recently_used (filename, "text/plain",
				     "gEdit", "gEdit document");
}

void
options_toggle_split_screen_cb (GtkWidget *widget, gpointer data)
{
	gedit_view *view = GE_VIEW (mdi->active_view);

	if (!view->split_parent)
		return;

	gedit_view_set_split_screen
		(view, !GTK_WIDGET_VISIBLE (view->split_parent));
}

void
options_toggle_read_only_cb (GtkWidget *widget, gpointer data)
{
	gedit_view *view = GE_VIEW (mdi->active_view);
	
	gedit_view_set_read_only (view, !view->read_only);
}

void
options_toggle_word_wrap_cb (GtkWidget *widget, gpointer data)
{
	gedit_view *view = GE_VIEW (mdi->active_view);
	
	gedit_view_set_word_wrap (view, !view->word_wrap);
}

void
options_toggle_line_wrap_cb (GtkWidget *widget, gpointer data)
{
	gedit_view *view = GE_VIEW (mdi->active_view);

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
