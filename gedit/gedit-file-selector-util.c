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

/*
 * TODO: remove unused code
 */

#include <config.h>

#include <bonobo/bonobo-file-selector-util.h>

#include <string.h>

#include <bonobo/bonobo-event-source.h>
#include <bonobo/bonobo-exception.h>
#include <bonobo/bonobo-widget.h>

#include <gtk/gtkmain.h>
#include <gtk/gtkclist.h>
#include <gtk/gtkfilesel.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkmessagedialog.h>
#include <gtk/gtkeditable.h>
#include <gtk/gtkstock.h>

#include <bonobo/bonobo-i18n.h>

#include <libgnomevfs/gnome-vfs-uri.h>
#include <libgnomevfs/gnome-vfs-ops.h>

#include "gnome-vfs-helpers.h"
#include "gedit-utils.h"

#define GET_MODE(w) (GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (w), "GnomeFileSelectorMode")))
#define SET_MODE(w, m) (gtk_object_set_data (GTK_OBJECT (w), "GnomeFileSelectorMode", GINT_TO_POINTER (m)))

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

static gboolean
replace_existing_file (GtkWindow *parent, const gchar* file_name)
{
	GtkWidget *msgbox;
	gint ret;
	gchar* uri;
	
	uri = gnome_vfs_x_format_uri_for_display (file_name);
	msgbox = gtk_message_dialog_new (parent,
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_MESSAGE_QUESTION,
			GTK_BUTTONS_NONE,
			_("A file named ''%s'' already exists.\n"
			  "Do you want to replace it with the "
			  "one you are saving?"), 
			uri);
	g_free (uri);

	/* Add Don't Replace button */
	gedit_dialog_add_button (GTK_DIALOG (msgbox), 
			_("Do_n't replace"), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);

	/* Add Replace button */
	gedit_dialog_add_button (GTK_DIALOG (msgbox), 
			_("_Replace"), GTK_STOCK_REFRESH, GTK_RESPONSE_YES);

	gtk_dialog_set_default_response	(GTK_DIALOG (msgbox), GTK_RESPONSE_CANCEL);

	gtk_window_set_resizable (GTK_WINDOW (msgbox), FALSE);

	ret = gtk_dialog_run (GTK_DIALOG (msgbox));
		
	gtk_widget_destroy (msgbox);

	return (ret == GTK_RESPONSE_YES);
}

static void
listener_cb (BonoboListener *listener, 
	     const gchar *event_name,
	     const CORBA_any *any,
	     CORBA_Environment *ev,
	     gpointer data)
{
	GtkWidget *dialog;
	CORBA_sequence_CORBA_string *seq;
	char *subtype;

	dialog = data;
	
	subtype = bonobo_event_subtype (event_name);
	if (!strcmp (subtype, "Cancel"))
		goto cancel_clicked;

	seq = any->_value;
	if (seq->_length < 1)
		goto cancel_clicked;

	if (GET_MODE (dialog) == FILESEL_OPEN_MULTI) {
		char **strv;
		int i;

		if (seq->_length == 0)
			goto cancel_clicked;

		strv = g_new (char *, seq->_length + 1);
		
		for (i = 0; i < seq->_length; i++)
			strv[i] = g_strdup (seq->_buffer[i]);
		strv[i] = NULL;
		
		gtk_object_set_user_data (GTK_OBJECT (dialog), strv);
	} else if (GET_MODE (dialog) == FILESEL_SAVE) {
		/* FIXME: does seq->_buffer[0] represent a canonical URI ?*/
		GnomeVFSURI *uri = gnome_vfs_uri_new (seq->_buffer[0]);

		if (gnome_vfs_uri_exists (uri)) 
		{
			if (!replace_existing_file (GTK_WINDOW (dialog), seq->_buffer[0])) 
			{
				gnome_vfs_uri_unref (uri);
				g_free (subtype);
				return;
			}
		}
			
		gnome_vfs_uri_unref (uri);

		gtk_object_set_user_data (GTK_OBJECT (dialog),
					  g_strdup (seq->_buffer[0]));
	} else {
		gtk_object_set_user_data (GTK_OBJECT (dialog),
					  g_strdup (seq->_buffer[0]));
	}

 cancel_clicked:
	gtk_widget_hide (dialog);

	g_free (subtype);
	gtk_main_quit ();
}

static BonoboWidget *
create_control (gboolean enable_vfs, FileselMode mode)
{
	GtkWidget *control;
	char *moniker;

	moniker = g_strdup_printf (
		"OAFIID:GNOME_FileSelector_Control!"
		"Application=%s;"
		"EnableVFS=%d;"
		"MultipleSelection=%d;"
		"SaveMode=%d",
		g_get_prgname (),
		enable_vfs,
		mode == FILESEL_OPEN_MULTI, 
		mode == FILESEL_SAVE);

	control = bonobo_widget_new_control (moniker, CORBA_OBJECT_NIL);
	g_free (moniker);

	return control ? BONOBO_WIDGET (control) : NULL;
}

static GtkWindow *
create_bonobo_selector (gboolean    enable_vfs,
			FileselMode mode,
			const char *mime_types,
			const char *default_path, 
			const char *default_filename)

{
	GtkWidget    *dialog;
	BonoboWidget *control;

	control = create_control (enable_vfs, mode);
	if (!control)
		return NULL;

	dialog = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_container_add (GTK_CONTAINER (dialog), GTK_WIDGET (control));
	gtk_widget_set_usize (dialog, 560, 450);

	bonobo_event_source_client_add_listener (
		bonobo_widget_get_objref (control), 
		listener_cb, 
		"GNOME/FileSelector/Control:ButtonClicked",
		NULL, dialog);

	if (mime_types)
		bonobo_widget_set_property (
			control, "MimeTypes", mime_types, NULL);

	if (default_path)
		bonobo_widget_set_property (
			control, "DefaultLocation", default_path, NULL);

	if (default_filename)
		bonobo_widget_set_property (
			control, "DefaultFileName", default_filename, NULL);
	
	return GTK_WINDOW (dialog);
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
	} else if (GET_MODE (fsel) == FILESEL_OPEN_MULTI) {
		GtkCList *clist;
		GList  *row;
		char **strv;
		char *filedirname;
		int i, rows, rownum;

		gtk_widget_hide (GTK_WIDGET (fsel));
		
		clist = GTK_CLIST (fsel->file_list);
		rows = g_list_length (clist->selection);
		
		strv = g_new (char *, rows + 2);
		strv[rows] = g_strdup (file_name);

		filedirname = g_dirname (file_name);

		rownum = 0;
		i = 0;
		row = clist->row_list;
		
		for ( ; row; row = g_list_next (row)) {
			
			if ((GTK_CLIST_ROW (row)->state == GTK_STATE_SELECTED) &&
			    (gtk_clist_get_cell_type (clist, rownum, 0) == GTK_CELL_TEXT)) {		
				gchar* f;

				gtk_clist_get_text (clist, rownum, 0, &f);
				strv[i] = concat_dir_and_file (filedirname, f);

				/* avoid duplicates */
				if (strv[rows] && (strcmp (strv[i], strv[rows]) == 0)) {
					g_free (strv[rows]);
					strv[rows] = NULL;
				}

				++i;
			}

			++rownum;
		}
		
		/* g_assert (i == rows); */
		
		strv[rows + 1] = NULL;

		g_free (filedirname);

		gtk_object_set_user_data (GTK_OBJECT (fsel), strv);
		gtk_main_quit ();
	} else if (GET_MODE (fsel) == FILESEL_SAVE) {
		if (g_file_test (file_name, G_FILE_TEST_EXISTS)) 
			if (!replace_existing_file (GTK_WINDOW (fsel), file_name))
				return;

		gtk_widget_hide (GTK_WIDGET (fsel));

		gtk_object_set_user_data (GTK_OBJECT (fsel),
					  g_strdup (file_name));
		gtk_main_quit ();
	} else {
		gtk_widget_hide (GTK_WIDGET (fsel));

		gtk_object_set_user_data (GTK_OBJECT (fsel),
					  g_strdup (file_name));
		gtk_main_quit ();
	}
}

static void
cancel_clicked_cb (GtkWidget *widget, gpointer data)
{
	gtk_widget_hide (GTK_WIDGET (data));
	gtk_main_quit ();
}


static GtkWindow *
create_gtk_selector (FileselMode mode,
		     const char *default_path,
		     const char *default_filename)
{
	GtkWidget *filesel;
	
	gchar* path;

	filesel = gtk_file_selection_new (NULL);

	gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (filesel)->ok_button),
			    "clicked", GTK_SIGNAL_FUNC (ok_clicked_cb),
			    filesel);

	gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (filesel)->cancel_button),
			    "clicked", GTK_SIGNAL_FUNC (cancel_clicked_cb),
			    filesel);

	if (default_path)
		path = g_strconcat (default_path, 
				    default_path[strlen (default_path) - 1] == '/' ? NULL : 
				    "/", NULL);
	else
		path = g_strdup ("./");

	if (default_filename) {
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

	if (mode == FILESEL_OPEN_MULTI) {
		gtk_clist_set_selection_mode (GTK_CLIST (GTK_FILE_SELECTION (filesel)->file_list),
					      GTK_SELECTION_EXTENDED);
	}

	return GTK_WINDOW (filesel);
}

static gpointer
run_file_selector (GtkWindow  *parent,
		   gboolean    enable_vfs,
		   FileselMode mode, 
		   const char *title,
		   const char *mime_types,
		   const char *default_path, 
		   const char *default_filename)

{
	GtkWindow *dialog = NULL;
	gpointer   retval;

	if (!g_getenv ("GNOME_FILESEL_DISABLE_BONOBO"))
		dialog = create_bonobo_selector (enable_vfs, mode, mime_types, 
						 default_path, default_filename);
	if (!dialog)
		dialog = create_gtk_selector (mode, default_path, default_filename);

	SET_MODE (dialog, mode);

	gtk_window_set_title (dialog, title);
	gtk_window_set_modal (dialog, TRUE);

	if (parent)
		gtk_window_set_transient_for (dialog, parent);
	
	gtk_signal_connect (GTK_OBJECT (dialog), "delete_event",
			    GTK_SIGNAL_FUNC (delete_file_selector),
			    dialog);

	gtk_widget_show_all (GTK_WIDGET (dialog));

	gtk_main ();

	retval = gtk_object_get_user_data (GTK_OBJECT (dialog));

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
			   const char *default_path)
{
	return bonobo_file_selector_open (parent, enable_vfs, 
			title, mime_types, default_path);
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
				const char *default_path)
{
	return bonobo_file_selector_open_multi (parent, enable_vfs, 
			title, mime_types, default_path);
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
			   const char *default_filename)
{
	return run_file_selector (parent, enable_vfs, FILESEL_SAVE,
				  title ? title : _("Select a filename to save"),
				  mime_types, default_path, default_filename);
}
