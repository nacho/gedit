/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-file.c
 * This file is part of gedit
 *
 * Copyright (C) 1998, 1999 Alex Roberts, Evan Lawrence
 * Copyright (C) 2000, 2001 Chema Celorio, Paolo Maggi 
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
 * Boston, MA 02111-1307, USA. * *
 */
 
/*
 * Modified by the gedit Team, 1998-2001. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 */

#include <libgnome/libgnome.h>
#include <libgnomeui/libgnomeui.h>
#include <libgnomevfs/gnome-vfs.h>

#include "gnome-vfs-helpers.h"
#include "gedit2.h"
#include "gedit-file.h"
#include "gedit-debug.h"
#include "gedit-utils.h"
#include "gedit-mdi.h"
#include "gedit-recent.h" 
#include "gedit-prefs.h"
#include "gedit-file-selector-util.h"

#define USE_BONOBO_FILE_SEL_API	

static gint gedit_file_selector_delete_event_handler (GtkWidget *widget, 
						      GdkEventAny *event);
static void gedit_file_selector_cancel_button_clicked_handler (GtkWidget *w,
						      gpointer data);
static gint gedit_file_selector_key_press_event_handler (GtkFileSelection *fsel, 
						   GdkEventKey *event);
static void gedit_file_open_ok_button_clicked_handler (GtkWidget *widget, 
						   GeditMDIChild *active_child);
static void gedit_file_save_as_ok_button_clicked_handler (GtkWidget *widget, 
							GeditMDIChild *child);

static GSList*  gedit_file_selector_get_filenames (GtkFileSelection *file_selector);
static gboolean gedit_file_open_is_dir (GtkFileSelection *file_selector);

static gboolean gedit_file_open_real (const gchar* file_name, GeditMDIChild* child);

static GtkWidget *open_file_selector = NULL;
static GtkWidget *save_file_selector = NULL;

void 
gedit_file_new (void)
{
	gint ret;
	GeditMDIChild* new_child = NULL;

	gedit_debug (DEBUG_FILE, "");

	new_child = gedit_mdi_child_new ();

	g_return_if_fail (new_child != NULL);
	g_return_if_fail (gedit_mdi != NULL);

	ret = bonobo_mdi_add_child (BONOBO_MDI (gedit_mdi), BONOBO_MDI_CHILD (new_child));
	g_return_if_fail (ret != FALSE);
	gedit_debug (DEBUG_COMMANDS, "Child added.");

	ret = bonobo_mdi_add_view (BONOBO_MDI (gedit_mdi), BONOBO_MDI_CHILD (new_child));
	g_return_if_fail (ret != FALSE);
	gedit_debug (DEBUG_COMMANDS, "View added.");
}

void 
gedit_file_close (GtkWidget *view)
{
	gint ret;
	BonoboMDIChild* child;

	gedit_debug (DEBUG_FILE, "");

	g_return_if_fail (view != NULL);

	child = bonobo_mdi_get_child_from_view (view);
	g_return_if_fail (child != NULL);

	if (g_list_length (bonobo_mdi_child_get_views (child)) > 1)
	{
		ret = bonobo_mdi_remove_view (BONOBO_MDI (gedit_mdi), view, FALSE);
		gedit_debug (DEBUG_COMMANDS, "View removed.");
	}
	else
	{
		ret = bonobo_mdi_remove_child (BONOBO_MDI (gedit_mdi), child, FALSE);
		gedit_debug (DEBUG_COMMANDS, "Child removed.");
	}

	if (ret)
		gedit_mdi_set_active_window_title (BONOBO_MDI (gedit_mdi));

	if (bonobo_mdi_get_active_child (BONOBO_MDI (gedit_mdi)) == NULL)
		gedit_mdi_set_active_window_verbs_sensitivity (BONOBO_MDI (gedit_mdi));
}

static gint
gedit_file_selector_delete_event_handler (GtkWidget *widget, GdkEventAny *event)
{
	gedit_debug (DEBUG_FILE, "");

	gtk_widget_hide (widget);
	
	gtk_main_quit ();
	
	return TRUE;
}
 
static void
gedit_file_selector_cancel_button_clicked_handler (GtkWidget *w, gpointer data)
{
	gedit_debug (DEBUG_FILE, "");
	
	gtk_widget_hide (data);

	gtk_main_quit();
}

static gint
gedit_file_selector_key_press_event_handler (GtkFileSelection *fsel, GdkEventKey *event)
{
	if (event->keyval == GDK_Escape) {
		gtk_button_clicked (GTK_BUTTON (fsel->cancel_button));
		return 1;
	} else
		return 0;
}

static GSList *
gedit_file_selector_get_filenames (GtkFileSelection *file_selector)
{
	GtkCListRow *row;
	GtkCList *rows;
	GSList *files = NULL;
	GList *list;
	gchar *file;
	gint row_num = 0;

	g_return_val_if_fail (GTK_IS_FILE_SELECTION (file_selector), NULL);

	rows = GTK_CLIST (file_selector->file_list);
	list = rows->row_list;
	for (; list != NULL; list = list->next) 
	{
		row = list->data;
		gtk_clist_get_text (rows, row_num, 0, &file);
		if (row->state == GTK_STATE_SELECTED)
			files = g_slist_prepend (files, g_strdup(file));
		row_num++;
	}
						 
	return files;
}


static gboolean
gedit_file_open_is_dir (GtkFileSelection *file_selector)
{
	static const gchar *selected_file;
	gchar *directory;
	gint length;

	selected_file = gtk_file_selection_get_filename (file_selector);

	if (!g_file_test (selected_file, G_FILE_TEST_IS_DIR))
		return FALSE;
	
	length = strlen (selected_file);

	if (selected_file [length-1] == '/')
		directory = g_strdup (selected_file);
	else
		directory = g_strdup_printf ("%s%c", selected_file, '/');

	gtk_file_selection_set_filename (file_selector, directory);
	gtk_entry_set_text (GTK_ENTRY(file_selector->selection_entry), "");

	g_free (directory);
	
	return TRUE;
}

static void
gedit_file_open_ok_button_clicked_handler (GtkWidget *widget, GeditMDIChild *active_child)
{
	GtkFileSelection *file_selector;
	const gchar * file_name;
	gchar * full_path;
	gchar * directory;
	GSList *files;
	
	gedit_debug (DEBUG_FILE, "");

	g_return_if_fail (GTK_IS_FILE_SELECTION (open_file_selector));
	file_selector = GTK_FILE_SELECTION (open_file_selector);

	if (gedit_file_open_is_dir (file_selector))
		return;

	file_name = gtk_file_selection_get_filename (file_selector);
	directory = g_dirname (file_name);

	files = gedit_file_selector_get_filenames (file_selector);

	if (files == NULL) 
	{
		if (gedit_file_open_real (file_name, active_child))
			gedit_utils_flash_va (_("Loaded file %s"), file_name);
		
		gedit_debug (DEBUG_FILE, "File: %s", file_name);
	} 
	else 
	{		
		for (; files != NULL; files = files->next) 
		{
			full_path = g_concat_dir_and_file (directory, files->data);
			
			if (gedit_file_open_real (full_path, active_child))
				gedit_utils_flash_va (_("Loaded file %s"), full_path);
			
			gedit_debug (DEBUG_FILE, "File: %s", full_path);

			g_free (files->data);
			g_free (full_path);
		}
	}

	if (g_slist_length (files) > 1)
		gedit_utils_flash_va (_("Loaded %i files"), g_slist_length (files));

	g_free (directory);
	g_slist_free (files);
	
	gtk_widget_hide (GTK_WIDGET (file_selector));

	gtk_main_quit ();
		
	return;
}


void
gedit_file_open (GeditMDIChild *active_child)
{
#ifndef USE_BONOBO_FILE_SEL_API

	gint ok_signal_id = 0;
	
	gedit_debug (DEBUG_FILE, "");

	g_return_if_fail (!(open_file_selector && GTK_WIDGET_VISIBLE (open_file_selector)));

	if (open_file_selector == NULL)
	{
		open_file_selector = gtk_file_selection_new (NULL);
		
		gtk_signal_connect (GTK_OBJECT (open_file_selector),
				    "delete_event",
				    GTK_SIGNAL_FUNC (gedit_file_selector_delete_event_handler),
				    open_file_selector);
		gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (open_file_selector)->cancel_button),
				    "clicked",
				    GTK_SIGNAL_FUNC (gedit_file_selector_cancel_button_clicked_handler),
				    open_file_selector);
		gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (open_file_selector)),
				    "key_press_event",
				    GTK_SIGNAL_FUNC (gedit_file_selector_key_press_event_handler),
				    NULL);
		gtk_clist_set_selection_mode (
			GTK_CLIST(GTK_FILE_SELECTION (open_file_selector)->file_list),
			GTK_SELECTION_EXTENDED);

		gtk_file_selection_hide_fileop_buttons (GTK_FILE_SELECTION (open_file_selector));
	}
	else
	{
		GtkFileSelection *file_selector;

		g_return_if_fail (GTK_IS_FILE_SELECTION (open_file_selector));

		/* Clear the selection & file from the last open */
		file_selector = GTK_FILE_SELECTION (open_file_selector);
		gtk_clist_unselect_all (GTK_CLIST (file_selector->file_list));
		gtk_entry_set_text (GTK_ENTRY (file_selector->selection_entry), "");
	}

	ok_signal_id = gtk_signal_connect (
			    GTK_OBJECT (GTK_FILE_SELECTION (open_file_selector)->ok_button),
			    "clicked",
			    GTK_SIGNAL_FUNC (gedit_file_open_ok_button_clicked_handler),
			    active_child);


	/* FIXME: is this right? */
	gtk_window_set_transient_for (GTK_WINDOW (open_file_selector), 
			              GTK_WINDOW (bonobo_mdi_get_active_window (BONOBO_MDI (gedit_mdi))));

	gtk_window_set_modal (GTK_WINDOW (open_file_selector), TRUE);
	gtk_window_set_title (GTK_WINDOW (open_file_selector), _("Open File ..."));

	if (!GTK_WIDGET_VISIBLE (open_file_selector))
	{
		gtk_window_position (GTK_WINDOW (open_file_selector), GTK_WIN_POS_MOUSE);
		gtk_widget_show (open_file_selector);
	}

	gtk_main ();

	gtk_signal_disconnect (GTK_OBJECT (GTK_FILE_SELECTION (open_file_selector)->ok_button),
			ok_signal_id);

	return;
#else
	gchar** files;
	
	gedit_debug (DEBUG_FILE, "");

	files = gedit_file_selector_open_multi (
			GTK_WINDOW (bonobo_mdi_get_active_window (BONOBO_MDI (gedit_mdi))),
			TRUE,
		        _("Open File ..."), 
			NULL, 
			NULL);
	
	if (files) 
	{
		gint i;
		for (i = 0; files[i]; ++i)
		{
			if (gedit_file_open_real (files[i], active_child))
				gedit_utils_flash_va (_("Loaded file %s"), files[i]);
			
			gedit_debug (DEBUG_FILE, "File: %s", files[i]);
		}

		g_strfreev (files);
	}
#endif	
}

static gboolean
gedit_file_open_real (const gchar* file_name, GeditMDIChild* active_child)
{
	GError *error = NULL;
	gchar* uri;

	gedit_debug (DEBUG_FILE, "File name: %s", file_name);

	uri = gnome_vfs_x_make_uri_canonical (file_name);
	g_return_val_if_fail (uri != NULL, FALSE);
	gedit_debug (DEBUG_FILE, "URI: %s", uri);

	if (active_child == NULL ||
	    !gedit_document_is_untouched (active_child->document))	     
	{
		gint ret;
		GeditMDIChild* new_child = NULL;

		new_child = gedit_mdi_child_new_with_uri (uri, &error);

		if (error)
		{
			GtkWidget *dialog;
			/* FIXME: do a more user friendly error reporting */
			gchar *errstr;
			gchar *formatted_uri = gnome_vfs_x_format_uri_for_display (uri);
		       	
			errstr = g_strdup_printf (_("An error occurred while "
							   "opening the file \"%s\".\n\n%s."),
							   formatted_uri, error->message);
		
			dialog = gtk_message_dialog_new (
					GTK_WINDOW (bonobo_mdi_get_active_window (BONOBO_MDI (gedit_mdi))),
					GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				   	GTK_MESSAGE_ERROR,
				   	GTK_BUTTONS_OK,
					errstr);
			
			gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

			gtk_dialog_run (GTK_DIALOG (dialog));
  			gtk_widget_destroy (dialog);
			
			g_free (formatted_uri);
			g_free (errstr);

			g_error_free (error);
			g_free (uri);
			return FALSE;
		}

		g_return_val_if_fail (new_child != NULL, FALSE);
		g_return_val_if_fail (gedit_mdi != NULL, FALSE);

		ret = bonobo_mdi_add_child (BONOBO_MDI (gedit_mdi), BONOBO_MDI_CHILD (new_child));
		g_return_val_if_fail (ret != FALSE, FALSE);
		gedit_debug (DEBUG_FILE, "Child added.");

		ret = bonobo_mdi_add_view (BONOBO_MDI (gedit_mdi), BONOBO_MDI_CHILD (new_child));
		g_return_val_if_fail (ret != FALSE, FALSE);
		gedit_debug (DEBUG_FILE, "View added.");
	}
	else
	{		
		gedit_document_load (active_child->document, uri, &error);

		if (error)
		{
			GtkWidget *dialog;
			/* FIXME: do a more user friendly error reporting */
			gchar *errstr;
			gchar *formatted_uri = gnome_vfs_x_format_uri_for_display (uri);
		       	
			errstr = g_strdup_printf (_("An error occurred while "
							   "opening the file \"%s\".\n\n%s."),
							   formatted_uri, error->message);
		
			dialog = gtk_message_dialog_new (
					GTK_WINDOW (bonobo_mdi_get_active_window (BONOBO_MDI (gedit_mdi))),
					GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				   	GTK_MESSAGE_ERROR,
				   	GTK_BUTTONS_OK,
					errstr);
			
			gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

			gtk_dialog_run (GTK_DIALOG (dialog));
  			gtk_widget_destroy (dialog);

			g_free (formatted_uri);
			g_free (errstr);

			g_error_free (error);
			g_free (uri);
			return FALSE;
		}	
	}

	gedit_recent_add (uri);
	gedit_recent_update_all_windows (BONOBO_MDI (gedit_mdi)); 
	
	g_free (uri);

	gedit_debug (DEBUG_FILE, "END");

	return TRUE;
}

gboolean 
gedit_file_save (GeditMDIChild* child)
{
	gint ret;
	GeditDocument* doc = NULL;
	GError *error = NULL;
	gchar *uri = NULL;
	
	gedit_debug (DEBUG_FILE, "");

	g_return_val_if_fail (child != NULL, FALSE);
	
	doc = child->document;
	g_return_val_if_fail (doc != NULL, FALSE);
	
	if (gedit_document_is_untitled (doc))
	{
		gedit_debug (DEBUG_FILE, "Untitled");

		return gedit_file_save_as (child);
	}

	if (!gedit_document_get_modified (doc))	
	{
		gedit_debug (DEBUG_FILE, "Not modified");

		return TRUE;
	}
	
	uri = gedit_document_get_uri (doc);
	g_return_val_if_fail (uri != NULL, FALSE);
	
	gedit_utils_flash_va (_("Saving file '%s' ..."), uri);	
	
	ret = gedit_document_save (doc, &error);

	if (!ret)
	{
		
		gchar *error_message;
		gchar *errstr;
		GtkWidget *view, *dialog;

		gedit_debug (DEBUG_FILE, "FAILED");
	       	
		if (error == NULL)
		{
			error_message = _("Unknown error");
		}
		else
		{
			error_message = error->message;
		}
		
		errstr = g_strdup_printf (_("An error occurred while "
					    "saving the file \"%s\".\n\n%s."),
					    uri, error_message);
		
		view = GTK_WIDGET (g_list_nth_data (
					bonobo_mdi_child_get_views (BONOBO_MDI_CHILD (child)), 0));
		if (view != NULL)
			bonobo_mdi_set_active_view (BONOBO_MDI (gedit_mdi), view);
			
		dialog = gtk_message_dialog_new (
				GTK_WINDOW (bonobo_mdi_get_active_window (BONOBO_MDI (gedit_mdi))),
				GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_MESSAGE_ERROR,
				GTK_BUTTONS_OK,
				errstr);
			
		gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

		gtk_dialog_run (GTK_DIALOG (dialog));
  		gtk_widget_destroy (dialog);

		g_free (errstr);

		if (error != NULL)
			g_error_free (error);

		gedit_utils_flash_va (_("The document has not been saved."));

		g_free (uri);

		return FALSE;
	}	
	else
	{
		gedit_debug (DEBUG_FILE, "OK");

		gedit_utils_flash_va (_("File '%s' saved."), uri);

		gedit_recent_add (uri);
		gedit_recent_update_all_windows (BONOBO_MDI (gedit_mdi)); 

		g_free (uri);

		return TRUE;
	}
}

gboolean
gedit_file_save_as (GeditMDIChild *child)
{
	gint ok_signal_id = 0;
	GeditDocument* doc = NULL;
	gchar *fname = NULL;
	GtkWidget *view = NULL;
	
	gedit_debug (DEBUG_FILE, "");

	g_return_val_if_fail (child != NULL, FALSE);

	doc = child->document;
	g_return_val_if_fail (doc != NULL, FALSE);

	g_return_val_if_fail (!(save_file_selector && GTK_WIDGET_VISIBLE (save_file_selector)), FALSE);

	view = GTK_WIDGET (g_list_nth_data (
				bonobo_mdi_child_get_views (BONOBO_MDI_CHILD (child)), 0));
	if (view != NULL)
		bonobo_mdi_set_active_view (BONOBO_MDI (gedit_mdi), view);

	if (save_file_selector == NULL)
	{
		save_file_selector = gtk_file_selection_new (NULL);
		
		gtk_signal_connect (GTK_OBJECT (save_file_selector),
				    "delete_event",
				    GTK_SIGNAL_FUNC (gedit_file_selector_delete_event_handler),
				    save_file_selector);
		gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (save_file_selector)->cancel_button),
				    "clicked",
				    GTK_SIGNAL_FUNC (gedit_file_selector_cancel_button_clicked_handler),
				    save_file_selector);
		gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION(save_file_selector)),
				    "key_press_event",
				    GTK_SIGNAL_FUNC (gedit_file_selector_key_press_event_handler),
				    NULL);
		
		gtk_file_selection_hide_fileop_buttons (GTK_FILE_SELECTION (save_file_selector));
	}
	
	ok_signal_id = gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION(save_file_selector)->ok_button),
				    "clicked",
				    GTK_SIGNAL_FUNC (gedit_file_save_as_ok_button_clicked_handler),
				    child);
		
	fname = gedit_document_get_uri (doc);
	
	gtk_file_selection_set_filename (GTK_FILE_SELECTION (save_file_selector), fname);
	g_free (fname);

	gtk_window_set_transient_for (GTK_WINDOW (save_file_selector), 
				      GTK_WINDOW (bonobo_mdi_get_active_window (BONOBO_MDI (gedit_mdi))));

	gtk_window_set_modal (GTK_WINDOW (save_file_selector), TRUE);
	gtk_window_set_title (GTK_WINDOW (save_file_selector), _("Save As..."));

	if (!GTK_WIDGET_VISIBLE (save_file_selector))
	{
		gtk_window_position (GTK_WINDOW (save_file_selector), GTK_WIN_POS_MOUSE);
		gtk_widget_show(save_file_selector);
	}

	gtk_main ();

	gtk_signal_disconnect (GTK_OBJECT(GTK_FILE_SELECTION(save_file_selector)->ok_button),
			ok_signal_id);

	return !gedit_document_get_modified (child->document);
}

static void
gedit_file_save_as_ok_button_clicked_handler (GtkWidget *widget, GeditMDIChild *child)
{
	GtkFileSelection *file_selector;
	const gchar *file_name;
	gchar *uri;
	gboolean ret;
	GeditDocument *doc = NULL;
	GError *error = NULL;

	gedit_debug (DEBUG_FILE, "");

	g_return_if_fail (child != NULL);

	doc = child->document;
	g_return_if_fail (doc != NULL);

	g_return_if_fail (GTK_IS_FILE_SELECTION (save_file_selector));
	file_selector = GTK_FILE_SELECTION (save_file_selector);

	if (gedit_file_open_is_dir (file_selector))
		return;

	file_name = gtk_file_selection_get_filename (file_selector);
	g_return_if_fail (file_name != NULL);
	
	if (g_file_test (file_name, G_FILE_TEST_EXISTS))
	{
		GtkWidget *msgbox;
		gint ret;
		
		msgbox = gtk_message_dialog_new (GTK_WINDOW (file_selector),
				GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_MESSAGE_QUESTION,
				GTK_BUTTONS_CANCEL,
				_("A file named ``%s'' already exists in this location. Do you want to "
				  "replace it with the one you are saving?"), 
				file_name);

		gtk_dialog_add_button (GTK_DIALOG (msgbox),
                             _("_Replace"),
                             GTK_RESPONSE_YES);

		gtk_dialog_set_default_response	(GTK_DIALOG (msgbox), GTK_RESPONSE_CANCEL);

		ret = gtk_dialog_run (GTK_DIALOG (msgbox));
		
		gtk_widget_destroy (msgbox);

		if (ret != GTK_RESPONSE_YES)
			return;
	}

	gtk_widget_hide (GTK_WIDGET (file_selector));

	uri = gnome_vfs_x_make_uri_canonical (file_name);
	g_return_if_fail (uri != NULL);
	
	gedit_utils_flash_va (_("Saving file '%s' ..."), file_name);	
	
	ret = gedit_document_save_as (doc, uri, &error);

	if (!ret)
	{
		GtkWidget *dialog;
		gchar *error_message;
		gchar *errstr;

		gedit_debug (DEBUG_FILE, "FAILED");
	       	
		if (error == NULL)
		{
			error_message = _("Unknown error");
		}
		else
		{
			error_message = error->message;
		}
		
		errstr = g_strdup_printf (_("An error occurred while "
					    "saving the file \"%s\".\n\n%s."),
					    file_name, error_message);
		
		dialog = gtk_message_dialog_new (
					GTK_WINDOW (bonobo_mdi_get_active_window (BONOBO_MDI (gedit_mdi))),
					GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				   	GTK_MESSAGE_ERROR,
				   	GTK_BUTTONS_OK,
					errstr);

		gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

		gtk_dialog_run (GTK_DIALOG (dialog));
  		gtk_widget_destroy (dialog);

		g_free (errstr);

		if (error != NULL)
			g_error_free (error);

		gedit_utils_flash_va (_("The document has not been saved."));
		
	}	
	else
	{
		gedit_debug (DEBUG_FILE, "OK");

		gedit_utils_flash_va (_("File '%s' saved."), file_name);

		gedit_recent_add (uri);
		gedit_recent_update_all_windows (BONOBO_MDI (gedit_mdi)); 
	}

	g_free (uri);
	
	gtk_main_quit ();
	
	return;
}

gboolean
gedit_file_close_all (void)
{
	gboolean ret;
	gedit_debug (DEBUG_FILE, "");

	ret = bonobo_mdi_remove_all (BONOBO_MDI (gedit_mdi), FALSE);

	if (bonobo_mdi_get_active_child (BONOBO_MDI (gedit_mdi)) == NULL)
		gedit_mdi_set_active_window_verbs_sensitivity (BONOBO_MDI (gedit_mdi));

	return ret;
}

void
gedit_file_exit (void)
{
	gedit_debug (DEBUG_FILE, "");

	if (!gedit_file_close_all ())
		return;

	gedit_debug (DEBUG_FILE, "All files closed.");
	
	/* We need to disconnect the signal because mdi "destroy" event
	   is connected to gedit_file_exit ( i.e. this function ). */
	gtk_signal_disconnect_by_func (GTK_OBJECT (gedit_mdi),
			    GTK_SIGNAL_FUNC (gedit_file_exit), NULL);

	/*
	gtk_object_destroy (GTK_OBJECT (gedit_mdi));
	*/
	
	gedit_prefs_save_settings ();

	gedit_recent_history_write_config ();

	gedit_debug (DEBUG_FILE, "Unref gedit_mdi.");

	g_object_unref (G_OBJECT (gedit_mdi));

	gedit_debug (DEBUG_FILE, "Unref gedit_mdi: DONE");

	gtk_main_quit ();
}

void
gedit_file_save_all (void)
{
	gint i = 0;
	GeditMDIChild* child;
	GtkWidget* view;

	gedit_debug (DEBUG_FILE, "");

	view = bonobo_mdi_get_active_view (BONOBO_MDI (gedit_mdi));

	for (i = 0; i < g_list_length (bonobo_mdi_get_children (BONOBO_MDI (gedit_mdi))); i++)
	{
		child = GEDIT_MDI_CHILD (g_list_nth_data (
				bonobo_mdi_get_children (BONOBO_MDI (gedit_mdi)), i));

		if (gedit_document_get_modified (child->document))
		{
			gedit_file_save (child);	
		}
	}

	if (view !=  bonobo_mdi_get_active_view (BONOBO_MDI (gedit_mdi)))
		bonobo_mdi_set_active_view (BONOBO_MDI (gedit_mdi), view);
}

gboolean
gedit_file_revert (GeditMDIChild *child)
{
	gint ret;
	GeditDocument* doc = NULL;
	GError *error = NULL;
	gchar* uri = NULL;

	gedit_debug (DEBUG_FILE, "");

	g_return_val_if_fail (child != NULL, FALSE);
	
	doc = child->document;
	g_return_val_if_fail (doc != NULL, FALSE);

	uri = gedit_document_get_uri (doc);
	g_return_val_if_fail (uri != NULL, FALSE);

	gedit_utils_flash_va (_("Reverting file '%s' ..."), uri);	
	
	ret = gedit_document_revert (doc, &error);

	if (!ret)
	{
		gchar *error_message;
		gchar *errstr;
		GtkWidget *view, *dialog;

		gedit_debug (DEBUG_FILE, "FAILED");
	       	
		if (error == NULL)
		{
			error_message = _("Unknown error");
		}
		else
		{
			error_message = error->message;
		}
		
		errstr = g_strdup_printf (_("An error occurred while "
					    "reverting the file \"%s\".\n\n%s."),
					    uri, error_message);
		
		view = GTK_WIDGET (g_list_nth_data (
					bonobo_mdi_child_get_views (BONOBO_MDI_CHILD (child)), 0));
		if (view != NULL)
			bonobo_mdi_set_active_view (BONOBO_MDI (gedit_mdi), view);
		
		dialog = gtk_message_dialog_new (
					GTK_WINDOW (bonobo_mdi_get_active_window (BONOBO_MDI (gedit_mdi))),
					GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				   	GTK_MESSAGE_ERROR,
				   	GTK_BUTTONS_OK,
					errstr);
		
		gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
	
		gtk_dialog_run (GTK_DIALOG (dialog));
  		gtk_widget_destroy (dialog);

		g_free (errstr);

		if (error != NULL)
			g_error_free (error);

		gedit_utils_flash_va (_("The document has not been reverted."));

		g_free (uri);

		return FALSE;
	}	
	else
	{
		gedit_debug (DEBUG_FILE, "OK");

		gedit_utils_flash_va (_("File '%s' reverted."), uri);

		g_free (uri);

		return TRUE;
	}
}

static gchar *
gedit_file_make_canonical_uri_from_shell_arg (const char *location)
{
	gchar* uri;
	gchar* canonical_uri;
	
	gedit_debug (DEBUG_FILE, "");
	
	uri = gnome_vfs_x_make_uri_from_shell_arg (location);

	canonical_uri = gnome_vfs_x_make_uri_canonical (uri);

	g_free (uri);

	return canonical_uri;
}

gboolean 
gedit_file_open_uri_list (GList* uri_list)
{
	gchar *full_path;
	gboolean ret = TRUE;
	
	BonoboMDIChild *active_child = NULL;
	
	gedit_debug (DEBUG_FILE, "");
	
	if (uri_list == NULL) 
		return FALSE;

	active_child = bonobo_mdi_get_active_child (BONOBO_MDI (gedit_mdi));

        /* create a file for each document in the parameter list */
	for (;uri_list; uri_list = uri_list->next)
	{
		full_path = gedit_file_make_canonical_uri_from_shell_arg (uri_list->data);
		if (full_path != NULL) {
			ret |= gedit_file_open_real (full_path, 
					(active_child != NULL) ? GEDIT_MDI_CHILD (active_child): NULL);
			g_free (full_path);
		}
	}

	return ret;
}

gboolean 
gedit_file_open_recent (GeditMDIChild *child, gchar* uri)
{
	gboolean ret;
	gedit_debug (DEBUG_FILE, "");
	
	ret = gedit_file_open_real (uri, child);	

	gedit_debug (DEBUG_FILE, "END");

	return ret;
}


static gchar *
gedit_file_make_canonical_uri_from_input (const char *location)
{
	gchar* uri;
	gchar* canonical_uri;
	
	gedit_debug (DEBUG_FILE, "");
	
	uri = gnome_vfs_x_make_uri_from_input (location);

	canonical_uri = gnome_vfs_x_make_uri_canonical (uri);

	g_free (uri);

	return canonical_uri;
}

gboolean 
gedit_file_open_single_uri (const gchar* uri)
{
	gchar *full_path;
	gboolean ret = TRUE;
	
	gedit_debug (DEBUG_FILE, "");
	
	if (uri == NULL) return FALSE;
	
	full_path = gedit_file_make_canonical_uri_from_input (uri);

	if (full_path != NULL) 
	{
		ret = gedit_file_open_real (full_path, NULL);
		g_free (full_path);
	}

	return ret;
}

/* FIXME: it is broken */
gboolean
gedit_file_open_from_stdin (GeditMDIChild *active_child)
{
	struct stat stats;
	gboolean ret = TRUE;
	GeditDocument *doc = NULL;
	GError *error = NULL;
	GeditMDIChild *child;
	GeditMDIChild* new_child = NULL;

	gedit_debug (DEBUG_FILE, "");
	
	fstat(STDIN_FILENO, &stats);
	
	if (stats.st_size  == 0)
		return FALSE;

	child = active_child;
	
	if (active_child == NULL ||
	    !gedit_document_is_untouched (active_child->document))	     
	{
		new_child = gedit_mdi_child_new ();

		g_return_val_if_fail (new_child != NULL, FALSE);
		g_return_val_if_fail (gedit_mdi != NULL, FALSE);

		ret = bonobo_mdi_add_child (BONOBO_MDI (gedit_mdi), BONOBO_MDI_CHILD (new_child));
		g_return_val_if_fail (ret != FALSE, FALSE);
		gedit_debug (DEBUG_FILE, "Child added.");

		child= new_child;
	}

	doc = child->document;
	g_return_val_if_fail (doc != NULL, FALSE);

	ret = gedit_document_load_from_stdin (doc, &error);

	if (error)
	{
		GtkWidget *dialog;
		/* FIXME: do a more user friendly error reporting */
		gchar *errstr;
	       	
		errstr = g_strdup_printf (_("An error occurred while "
						   "reading data from stdin.\n\n%s."),
						   error->message);
		
		dialog = gtk_message_dialog_new (
				GTK_WINDOW (bonobo_mdi_get_active_window (BONOBO_MDI (gedit_mdi))),
				GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			   	GTK_MESSAGE_ERROR,
			   	GTK_BUTTONS_OK,
				errstr);

		gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);

		g_free (errstr);
		g_error_free (error);

		ret = FALSE;
	}
	
	if (new_child != NULL)
	{
		ret = bonobo_mdi_add_view (BONOBO_MDI (gedit_mdi), BONOBO_MDI_CHILD (new_child));
		g_return_val_if_fail (ret != FALSE, FALSE);
		gedit_debug (DEBUG_FILE, "View added.");
	}
	
	return ret;
}

