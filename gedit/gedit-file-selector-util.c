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
#include <eel/eel-alert-dialog.h>

#include "gedit-utils.h"
#include "gedit-encodings-option-menu.h"

#define GET_MODE(w) (GPOINTER_TO_INT (g_object_get_data (G_OBJECT (w), "GnomeFileSelectorMode")))
#define SET_MODE(w, m) (g_object_set_data (G_OBJECT (w), "GnomeFileSelectorMode", GINT_TO_POINTER (m)))

#define GET_ENABLE_VFS(w) (GPOINTER_TO_INT (g_object_get_data (G_OBJECT (w), "GnomeFileSelectorEnableVFS")))
#define SET_ENABLE_VFS(w, m) (g_object_set_data (G_OBJECT (w), "GnomeFileSelectorEnableVFS", GINT_TO_POINTER (m)))


typedef enum {
	FILESEL_OPEN,
	FILESEL_OPEN_MULTI,
	FILESEL_SAVE
} FileselMode;

/* Displays a confirmation dialog for whether to replace a file.  The message
 * should contain a %s to include the file name.
 */
static gboolean
replace_dialog (GtkWindow *parent,
		const gchar *primary_message,
		const gchar *file_name,
		const gchar *secondary_message)
{
	GtkWidget *msgbox;
	AtkObject *obj;
	gint ret;
	gchar *full_formatted_uri;
       	gchar *uri_for_display	;
	gchar *uri;
	gchar *message_with_uri;

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

	message_with_uri = g_strdup_printf (primary_message, uri_for_display);
	g_free (uri_for_display);

	msgbox = eel_alert_dialog_new (parent,
				       GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				       GTK_MESSAGE_QUESTION,
				       GTK_BUTTONS_NONE,
				       message_with_uri,
				       secondary_message,
				       NULL);
	g_free (message_with_uri);

	/* Add Cancel button */
	gtk_dialog_add_button (GTK_DIALOG (msgbox), 
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);

	/* Add Replace button */
	gedit_dialog_add_button (GTK_DIALOG (msgbox), 
			_("_Replace"), GTK_STOCK_REFRESH, GTK_RESPONSE_YES);

	gtk_dialog_set_default_response	(GTK_DIALOG (msgbox), GTK_RESPONSE_CANCEL);

	obj = gtk_widget_get_accessible (msgbox);
	if (GTK_IS_ACCESSIBLE (obj))
		atk_object_set_name (obj, _("Question"));

	ret = gtk_dialog_run (GTK_DIALOG (msgbox));
	gtk_widget_destroy (msgbox);

	return (ret == GTK_RESPONSE_YES);
}

/* Presents a confirmation dialog for whether to replace an existing file */
static gboolean
replace_existing_file (GtkWindow *parent, const gchar* file_name)
{
	return replace_dialog (parent,
			       _("A file named \"%s\" already exists.\n"), file_name,
			       _("Do you want to replace it with the "
			         "one you are saving?"));
}

/* Presents a confirmation dialog for whether to replace a read-only file */
static gboolean
replace_read_only_file (GtkWindow *parent, const gchar* file_name)
{
	return replace_dialog (parent,
			       _("The file \"%s\" is read-only.\n"), file_name,
			       _("Do you want to try to replace it with the "
			         "one you are saving?"));
}

/* Tests whether we have write permissions for a file */
static gboolean
is_read_only (const gchar *name)
{
	gboolean                 ret;
	GnomeVFSFileInfo        *info;

	g_return_val_if_fail (name != NULL, FALSE);
	
	info = gnome_vfs_file_info_new ();
	
	/* FIXME: is GNOME_VFS_FILE_INFO_FOLLOW_LINKS right in this case? - Paolo */
	if (gnome_vfs_get_file_info (name,
				     info, 
				     GNOME_VFS_FILE_INFO_FOLLOW_LINKS | 
				     GNOME_VFS_FILE_INFO_GET_ACCESS_RIGHTS) != GNOME_VFS_OK)
	{
		gnome_vfs_file_info_unref (info);

		return TRUE;
	}
		
	if (info->valid_fields & GNOME_VFS_FILE_INFO_FIELDS_ACCESS)
	{
		ret = !(info->permissions & GNOME_VFS_PERM_ACCESS_WRITABLE);
	}
	else
	{
		ret = TRUE;
	}
	
	gnome_vfs_file_info_unref (info);

	return ret;	
}

static gboolean 
file_exists (gchar *name)
{
	GnomeVFSURI *uri;
	gboolean ret;
	
	g_return_val_if_fail (name != NULL, FALSE);

	uri = gnome_vfs_uri_new (name);
	g_return_val_if_fail (uri != NULL, FALSE);

	ret = gnome_vfs_uri_exists (uri);

	gnome_vfs_uri_unref (uri);

	return ret;
}

static gpointer
analyze_response (GtkFileChooser *chooser, gint response)
{
	gchar *file_name;
	gchar *uri;
	gboolean enable_vfs;
	
	if (response == GTK_RESPONSE_CANCEL ||
	    response == GTK_RESPONSE_DELETE_EVENT) 
	{
		gtk_widget_hide (GTK_WIDGET (chooser));
		
		return NULL;
	}

	enable_vfs = GET_ENABLE_VFS (chooser);

	if (enable_vfs)
	{
		file_name = gtk_file_chooser_get_uri (chooser);
	}
	else
	{
		file_name = gtk_file_chooser_get_filename (chooser);
	}

	if ((file_name == NULL) || (strlen (file_name) == 0)) 
	{
		g_free (file_name);
		return NULL;
	}

	if (enable_vfs)
	{
		uri = g_strdup (file_name);
	}
	else
	{
		uri = gnome_vfs_get_uri_from_local_path (file_name);
	}

	if (GET_MODE (chooser) == FILESEL_OPEN_MULTI) 
	{
		GSList *files;
	       
		g_free (file_name);
		g_free (uri);

		if (enable_vfs)
			files = gtk_file_chooser_get_uris (chooser);
		else
			files = gtk_file_chooser_get_filenames (chooser);

		gtk_widget_hide (GTK_WIDGET (chooser));

		return files;
	}
	else	
	{	
		if (GET_MODE (chooser) == FILESEL_SAVE)
		{
			if (file_exists (uri))
			{
				if (is_read_only (uri))
				{
					if (!replace_read_only_file (GTK_WINDOW (chooser), file_name)) 
					{
						g_free (file_name);
						g_free (uri);

						return NULL;
					}
				}
				else if (!replace_existing_file (GTK_WINDOW (chooser), file_name)) 
				{
					g_free (file_name);
					g_free (uri);

					return NULL;
				}
			}
		}

		gtk_widget_hide (GTK_WIDGET (chooser));

		g_free (uri);

		return file_name;
	} 

	return NULL;
}

static gboolean
all_text_files_filter (const GtkFileFilterInfo *filter_info,
		       gpointer                 data)
{
	if (filter_info->mime_type == NULL)
		return TRUE;
	
	if ((strncmp (filter_info->mime_type, "text/", 5) == 0) ||
            (strcmp (filter_info->mime_type, "application/x-desktop") == 0) ||
	    (strcmp (filter_info->mime_type, "application/x-perl") == 0) ||
            (strcmp (filter_info->mime_type, "application/x-python") == 0) ||
	    (strcmp (filter_info->mime_type, "application/x-php") == 0))
	{
	    return TRUE;
	}

	return FALSE;
}

static GtkWindow *
create_gtk_selector (GtkWindow *parent,
		     FileselMode mode,
		     const char *title,
		     const char *default_path,
		     const char *default_filename,
		     gboolean use_encoding,
		     const GeditEncoding *encoding)
{
	GtkWidget     *filesel;
	GtkFileFilter *filter;
	
	filesel = gtk_file_chooser_dialog_new (title,
					       parent,
					       (mode == FILESEL_SAVE) ? GTK_FILE_CHOOSER_ACTION_SAVE
					       			    : GTK_FILE_CHOOSER_ACTION_OPEN,
					       GTK_STOCK_CANCEL,
					       GTK_RESPONSE_CANCEL,
					       (mode == FILESEL_SAVE) ? GTK_STOCK_SAVE
								    : GTK_STOCK_OPEN,
					       GTK_RESPONSE_OK,
					       NULL);
	
	gtk_dialog_set_default_response (GTK_DIALOG (filesel), GTK_RESPONSE_OK);

	/* Filters */
	filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, _("All Files"));
	gtk_file_filter_add_pattern (filter, "*");
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (filesel), filter);

	/* Make this filter the default */
	gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (filesel), filter);

	filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, _("All Text Files"));
	gtk_file_filter_add_custom (filter, 
				    GTK_FILE_FILTER_MIME_TYPE,
				    all_text_files_filter, 
				    NULL, 
				    NULL);
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (filesel), filter);

	/* TODO: Add filters for all supported languages - Paolo - Feb 21, 2004 */

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

		gtk_file_chooser_set_extra_widget (GTK_FILE_CHOOSER (filesel), 
						   hbox);

		gtk_widget_show_all (hbox);

		g_object_set_data (G_OBJECT (filesel), 
				   "encodings-option_menu",
				   menu);

		if (encoding != NULL)
			gedit_encodings_option_menu_set_selected_encoding (
					GEDIT_ENCODINGS_OPTION_MENU (menu),
					encoding);

	}
	
	if (default_path)
		gtk_file_chooser_set_current_folder_uri (GTK_FILE_CHOOSER (filesel), default_path);

	if (default_filename)
		gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (filesel), default_filename);

	if (mode == FILESEL_OPEN_MULTI) 
		gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (filesel), TRUE);

	return GTK_WINDOW (filesel);
}

static gpointer
run_file_selector (GtkWindow  *parent,
		   gboolean    enable_vfs,
		   FileselMode mode, 
		   const char *title,
		   const char *default_path, 
		   const char *default_filename,
		   const GeditEncoding **encoding)

{
	GtkWindow *dialog = NULL;
	gpointer   retval;
	gint res;

	dialog = create_gtk_selector (parent,
				      mode,
				      title,
				      default_path, 
				      default_filename, 
				      (encoding != NULL),
				      (encoding != NULL) ? *encoding : NULL);

	SET_MODE (dialog, mode);
	SET_ENABLE_VFS (dialog, enable_vfs);
	
	gtk_window_set_modal (dialog, TRUE);

	gtk_widget_show_all (GTK_WIDGET (dialog));

	do 
	{
		res = gtk_dialog_run (GTK_DIALOG (dialog));

		retval = analyze_response (GTK_FILE_CHOOSER (dialog), res);
	}
	while (GTK_WIDGET_VISIBLE (dialog));

	if ((retval != NULL) && (encoding != NULL))
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
 * @default_path: optional directory to start in (must be an URI)
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
			   const char *default_path,
			   const GeditEncoding **encoding)
{
	return run_file_selector (parent, enable_vfs, FILESEL_OPEN, 
				  title ? title : _("Select a file to open"),
				  default_path, NULL, encoding);
}

/**
 * gedit_file_selector_open_multi:
 * @parent: optional window the dialog should be a transient for
 * @enable_vfs: if FALSE, restrict files to local paths.
 * @title: optional window title to use
 * @default_path: optional directory to start in (must be an URI)
 *
 * Creates and shows a modal open file dialog, waiting for the user to
 * select a file or cancel before returning.
 *
 * Return value: a GSList list of the selected URIs 
 * (or local file paths if @enable_vfs is FALSE), 
 * or NULL if cancel was pressed.
 **/
GSList *
gedit_file_selector_open_multi (GtkWindow  *parent,
				gboolean    enable_vfs,
				const char *title,
				const char *default_path,
				const GeditEncoding **encoding)
{
	return run_file_selector (parent, enable_vfs, FILESEL_OPEN_MULTI,
				  title ? title : _("Select files to open"),
				  default_path, NULL, encoding);
}

/**
 * gedit_file_selector_save:
 * @parent: optional window the dialog should be a transient for
 * @enable_vfs: if FALSE, restrict files to local paths.
 * @title: optional window title to use
 * @default_path: optional directory to start in (must be an URI)
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
			   const char *default_path, 
			   const char *default_filename,
			   const GeditEncoding **encoding)
{
	return run_file_selector (parent, enable_vfs, FILESEL_SAVE,
				  title ? title : _("Select a filename to save"),
				  default_path, default_filename, encoding);
}
