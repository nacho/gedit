/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-file-selector-util.c
 * This file is part of gedit
 *
 * Copyright (C) 2001-2002 Paolo Maggi 
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
 * Modified by the gedit Team, 1998-2002. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 */

/* This file is a modified version of bonobo-file-selector-util.c
 * taken from libbonoboui
 */

/*
 * bonobo-file-selector-util.c - functions for getting files from a
 * selector
 *
 * Authors:
 *    Jacob Berkman  <jacob@ximian.com>
 *
 * Copyright 2001 Ximian, Inc.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "gedit-file-selector-util.h"

#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <bonobo/bonobo-event-source.h>
#include <bonobo/bonobo-exception.h>
#include <bonobo/bonobo-widget.h>

#include <gtk/gtk.h>

#include <bonobo/bonobo-i18n.h>

#include <libgnomevfs/gnome-vfs.h>
#include <eel/eel-vfs-extensions.h>
#include <eel/eel-string.h>

#include "gedit-utils.h"
#include "gedit-encodings-option-menu.h"

static GQuark user_data_id = 0;

#define GET_MODE(w) (GPOINTER_TO_INT (g_object_get_data (G_OBJECT (w), "GnomeFileSelectorMode")))
#define SET_MODE(w, m) (g_object_set_data (G_OBJECT (w), "GnomeFileSelectorMode", GINT_TO_POINTER (m)))

typedef enum {
	FILESEL_OPEN,
	FILESEL_OPEN_MULTI,
	FILESEL_SAVE
} FileselMode;

static gint
delete_file_selector (GtkWidget *d, GdkEventAny *e, gpointer data)
{
	gtk_widget_hide (d);
	gtk_main_quit ();
	return TRUE;
}

/* Displays a confirmation dialog for whether to replace a file.  The message
 * should contain a %s to include the file name.
 */
static gboolean
replace_dialog (GtkWindow *parent, const gchar *message, const gchar *file_name)
{
	GtkWidget *msgbox;
	gint ret;
	gchar *full_formatted_uri;
       	gchar *uri_for_display	;
	gchar *uri;

	uri = eel_make_uri_from_shell_arg (file_name);
	g_return_val_if_fail (uri != NULL, FALSE);

	full_formatted_uri = eel_format_uri_for_display (uri);
	g_return_val_if_fail (full_formatted_uri != NULL, FALSE);
	g_free (uri);
	
	/* Truncate the URI so it doesn't get insanely wide. Note that even
	 * though the dialog uses wrapped text, if the URI doesn't contain
	 * white space then the text-wrapping code is too stupid to wrap it.
	 */
        uri_for_display = eel_str_middle_truncate (full_formatted_uri, 50);
	g_return_val_if_fail (uri_for_display != NULL, FALSE);

	g_free (full_formatted_uri);

	msgbox = gtk_message_dialog_new (parent,
					 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
					 GTK_MESSAGE_QUESTION,
					 GTK_BUTTONS_NONE,
					 message, 
					 uri_for_display);
	g_free (uri_for_display);

	/* Add Cancel button */
	gtk_dialog_add_button (GTK_DIALOG (msgbox), 
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);

	/* Add Replace button */
	gedit_dialog_add_button (GTK_DIALOG (msgbox), 
			_("_Replace"), GTK_STOCK_REFRESH, GTK_RESPONSE_YES);

	gtk_dialog_set_default_response	(GTK_DIALOG (msgbox), GTK_RESPONSE_CANCEL);

	gtk_window_set_resizable (GTK_WINDOW (msgbox), FALSE);

	ret = gtk_dialog_run (GTK_DIALOG (msgbox));
		
	gtk_widget_destroy (msgbox);

	return (ret == GTK_RESPONSE_YES);
}

/* Presents a confirmation dialog for whether to replace an existing file */
static gboolean
replace_existing_file (GtkWindow *parent, const gchar* file_name)
{
	return replace_dialog (parent,
			       _("A file named \"%s\" already exists.\n"
				 "Do you want to replace it with the "
				 "one you are saving?"),
			       file_name);
}

/* Presents a confirmation dialog for whether to replace a read-only file */
static gboolean
replace_read_only_file (GtkWindow *parent, const gchar* file_name)
{
	return replace_dialog (parent,
			       _("The file \"%s\" is read-only.\n"
				 "Do you want to try to replace it with the "
				 "one you are saving?"),
			       file_name);
}

static char *
concat_dir_and_file (const char *dir, const char *file)
{
	g_return_val_if_fail (dir != NULL, NULL);
	g_return_val_if_fail (file != NULL, NULL);

        /* If the directory name doesn't have a / on the end, we need
	   to add one so we get a proper path to the file */
	if (dir[0] != '\0' && dir [strlen(dir) - 1] != G_DIR_SEPARATOR)
		return g_strconcat (dir, G_DIR_SEPARATOR_S, file, NULL);
	else
		return g_strconcat (dir, file, NULL);
}

/* Tests whether we have write permissions for a file */
static gboolean
is_read_only (const gchar *filename)
{
	if (access (filename, W_OK) == -1)
		return (errno == EACCES);
	else
		return FALSE;
}

static void
ok_clicked_cb (GtkWidget *widget, gpointer data)
{
	GtkFileSelection *fsel;
	const gchar *file_name;

	fsel = data;

	file_name = gtk_file_selection_get_filename (fsel);

	if (!strlen (file_name))
		return;
	
	/* Change into directory if that's what user selected */
	if (g_file_test (file_name, G_FILE_TEST_IS_DIR)) {
		gint name_len;
		gchar *dir_name;

		name_len = strlen (file_name);
		if (name_len < 1 || file_name [name_len - 1] != '/') {
			/* The file selector needs a '/' at the end of a directory name */
			dir_name = g_strconcat (file_name, "/", NULL);
		} else {
			dir_name = g_strdup (file_name);
		}
		gtk_file_selection_set_filename (fsel, dir_name);
		g_free (dir_name);
	} 
	else if (GET_MODE (fsel) == FILESEL_OPEN_MULTI) 
	{
		gchar **strv = gtk_file_selection_get_selections (fsel);
		
		g_object_set_qdata (G_OBJECT (fsel),
				    user_data_id, strv);
		gtk_main_quit ();
	}
	else	
	{	
		if (GET_MODE (fsel) == FILESEL_SAVE)
		{
			if (g_file_test (file_name, G_FILE_TEST_EXISTS))
			{
				if (is_read_only (file_name))
				{
					if (!replace_read_only_file (GTK_WINDOW (fsel), file_name))
						return;
				}
				else if (!replace_existing_file (GTK_WINDOW (fsel), file_name))
					return;
			}
		}

		gtk_widget_hide (GTK_WIDGET (fsel));

		g_object_set_qdata (G_OBJECT (fsel),
			    	   user_data_id,
				   g_strdup (file_name));
		gtk_main_quit ();
	} 
}

static void
cancel_clicked_cb (GtkWidget *widget, gpointer data)
{
	gtk_widget_hide (GTK_WIDGET (data));
	gtk_main_quit ();

	g_object_set_qdata (G_OBJECT (data),
 		    	    user_data_id,
			    NULL);
}


static GtkWindow *
create_gtk_selector (FileselMode mode,
		     const char *default_path,
		     const char *default_filename,
		     gboolean use_encoding,
		     const GeditEncoding *encoding)
{
	GtkWidget *filesel;
	gchar* path;
		
	filesel = gtk_file_selection_new (NULL);

	if (use_encoding)
	{
		GtkWidget *hbox;
		GtkWidget *label;
		GtkWidget *menu;

		hbox = gtk_hbox_new (FALSE, 6);

		label = gtk_label_new_with_mnemonic (_("Ch_aracter Coding:"));
		menu = gedit_encodings_option_menu_new (mode == FILESEL_SAVE);
		
		gtk_label_set_mnemonic_widget (GTK_LABEL (label), menu);				       		
		gtk_box_pack_start (GTK_BOX (hbox), 
				    label,
				    FALSE,
				    FALSE,
				    0);

		gtk_box_pack_end (GTK_BOX (hbox), 
				  menu,
				  TRUE,
				  TRUE,
				  0);

		gtk_box_pack_end (GTK_BOX (GTK_FILE_SELECTION (filesel)->main_vbox), 
				  hbox,
				  TRUE,
				  TRUE,
				  6);

		g_object_set_data (G_OBJECT (filesel), 
				   "encodings-option_menu",
				   menu);

		if (encoding != NULL)
			gedit_encodings_option_menu_set_selected_encoding (
					GEDIT_ENCODINGS_OPTION_MENU (menu),
					encoding);

	}

	g_signal_connect (G_OBJECT (GTK_FILE_SELECTION (filesel)->ok_button),
			  "clicked", G_CALLBACK (ok_clicked_cb),
			  filesel);

	g_signal_connect (G_OBJECT (GTK_FILE_SELECTION (filesel)->cancel_button),
			  "clicked", G_CALLBACK (cancel_clicked_cb),
			  filesel);

	if (default_path)
		path = g_strconcat (default_path, 
				    default_path[strlen (default_path) - 1] == '/' ? NULL : 
				    "/", NULL);
	else
		path = g_strdup ("./");

	if (default_filename) 
	{
		gchar* file_name = concat_dir_and_file (path, default_filename);
		gtk_file_selection_set_filename (GTK_FILE_SELECTION (filesel), file_name);
		g_free (file_name);

		/* Select file name */
		gtk_editable_select_region (GTK_EDITABLE (
					    GTK_FILE_SELECTION (filesel)->selection_entry), 
					    0, -1);
					    
	}
	else
		gtk_file_selection_set_filename (GTK_FILE_SELECTION (filesel), path);

	g_free (path);

	if (mode == FILESEL_OPEN_MULTI) 
		gtk_file_selection_set_select_multiple (GTK_FILE_SELECTION (filesel), TRUE);

	return GTK_WINDOW (filesel);
}

static gpointer
run_file_selector (GtkWindow  *parent,
		   gboolean    enable_vfs,
		   FileselMode mode, 
		   const char *title,
		   const char *mime_types,
		   const char *default_path, 
		   const char *default_filename,
		   const GeditEncoding **encoding)

{
	GtkWindow *dialog = NULL;
	gpointer   retval;
	gpointer   data;

	if (!user_data_id)
		user_data_id = g_quark_from_static_string ("GeditUserData");

	dialog = create_gtk_selector (mode,
				      default_path, 
				      default_filename, 
				      (encoding != NULL),
				      (encoding != NULL) ? *encoding : NULL);

	SET_MODE (dialog, mode);

	gtk_window_set_title (dialog, title);
	gtk_window_set_modal (dialog, TRUE);

	if (parent)
		gtk_window_set_transient_for (dialog, parent);
	
	g_signal_connect (G_OBJECT (dialog), "delete_event",
			  G_CALLBACK (delete_file_selector),
			  dialog);

	gtk_widget_show_all (GTK_WIDGET (dialog));

	gtk_main ();

	data = g_object_get_qdata (G_OBJECT (dialog), user_data_id);
	
	if (data != NULL) 
	{
		if (enable_vfs && (mode != FILESEL_OPEN_MULTI)) 
		{
	 		retval = gnome_vfs_get_uri_from_local_path (data);
 			g_free (data);
		}
		else if (enable_vfs && (mode == FILESEL_OPEN_MULTI)) 
		{
			gint i;
			gchar **files = data;

			for (i = 0; files[i]; ++i) 
			{
				gchar *tmp = files [i];
				files [i] = gnome_vfs_get_uri_from_local_path (tmp);
				g_free (tmp);
			}

			retval = files;
	 	} else
 			retval = data;
	} else
		retval = NULL;

	if (encoding != NULL)
	{
		GeditEncodingsOptionMenu *menu;

		menu = GEDIT_ENCODINGS_OPTION_MENU (g_object_get_data (G_OBJECT (dialog), 
								       "encodings-option_menu"));

		*encoding = gedit_encodings_option_menu_get_selected_encoding (menu);
	}

	gtk_widget_destroy (GTK_WIDGET (dialog));

	return retval;
}

/**
 * gedit_file_selector_open:
 * @parent: optional window the dialog should be a transient for.
 * @enable_vfs: if FALSE, restrict files to local paths.
 * @title: optional window title to use
 * @mime_types: optional list of mime types to provide filters for.
 *   These are of the form: "HTML Files:text/html|Text Files:text/html,text/plain"
 * @default_path: optional directory to start in
 *
 * Creates and shows a modal open file dialog, waiting for the user to
 * select a file or cancel before returning.
 *
 * Return value: the URI (or plain file path if @enable_vfs is FALSE)
 * of the file selected, or NULL if cancel was pressed.
 **/
char *
gedit_file_selector_open (GtkWindow  *parent,
			   gboolean    enable_vfs,
			   const char *title,
			   const char *mime_types,
			   const char *default_path,
			   const GeditEncoding **encoding)
{
	return run_file_selector (parent, enable_vfs, FILESEL_OPEN, 
				  title ? title : _("Select a file to open"),
				  mime_types, default_path, NULL, encoding);
}

/**
 * gedit_file_selector_open_multi:
 * @parent: optional window the dialog should be a transient for
 * @enable_vfs: if FALSE, restrict files to local paths.
 * @title: optional window title to use
 * @mime_types: optional list of mime types to provide filters for.
 *   These are of the form: "HTML Files:text/html|Text Files:text/html,text/plain"
 * @default_path: optional directory to start in
 *
 * Creates and shows a modal open file dialog, waiting for the user to
 * select a file or cancel before returning.
 *
 * Return value: a NULL terminated string array of the selected URIs
 * (or local file paths if @enable_vfs is FALSE), or NULL if cancel
 * was pressed.
 **/
char **
gedit_file_selector_open_multi (GtkWindow  *parent,
				gboolean    enable_vfs,
				const char *title,
				const char *mime_types,
				const char *default_path,
				const GeditEncoding **encoding)
{
	return run_file_selector (parent, enable_vfs, FILESEL_OPEN_MULTI,
				  title ? title : _("Select files to open"),
				  mime_types, default_path, NULL, encoding);
}

/**
 * gedit_file_selector_save:
 * @parent: optional window the dialog should be a transient for
 * @enable_vfs: if FALSE, restrict files to local paths.
 * @title: optional window title to use
 * @mime_types: optional list of mime types to provide filters for.
 *   These are of the form: "HTML Files:text/html|Text Files:text/html,text/plain"
 * @default_path: optional directory to start in
 * @default_filename: optional file name to default to
 *
 * Creates and shows a modal save file dialog, waiting for the user to
 * select a file or cancel before returning.
 *
 * Return value: the URI (or plain file path if @enable_vfs is FALSE)
 * of the file selected, or NULL if cancel was pressed.
 **/
char *
gedit_file_selector_save (GtkWindow  *parent,
			   gboolean    enable_vfs,
			   const char *title,
			   const char *mime_types,
			   const char *default_path, 
			   const char *default_filename,
			   const GeditEncoding **encoding)
{
	return run_file_selector (parent, enable_vfs, FILESEL_SAVE,
				  title ? title : _("Select a filename to save"),
				  mime_types, default_path, default_filename, encoding);
}
