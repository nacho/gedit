/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit
 *
 * Copyright (C) 1998, 1999, 2000 Alex Roberts, Evan Lawrence, Jason Leach, Jose M Celorio
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
 *   Chema Celorio  <chema@celorio.com>
 */

#include <config.h>
#include <gnome.h>
#include <sys/stat.h> /* for stat() */
#include <unistd.h>  /* for gedit_stdin_open () */
#include <stdio.h>   /* for gedit_stdin_open () */

#include <libgnomevfs/gnome-vfs.h>

#include "commands.h"
#include "document.h"
#include "view.h" 
#include "prefs.h"
#include "file.h"
#include "utils.h"
#include "recent.h"
#include "window.h"
#include "print.h"
#include "undo.h"
#include "search.h"
#include "dialogs/dialogs.h"

#define GEDIT_STDIN_BUFSIZE 1024
#define PREALLOCATED_BUF_LEN 4096

GtkWidget *save_file_selector = NULL;
#if 0
/* Don't use public variables */
GtkWidget *open_file_selector = NULL;
#endif

void file_new_cb (GtkWidget *widget, gpointer cbdata);
void file_open_cb (GtkWidget *widget, gpointer cbdata);
gint file_save_document (GeditDocument *doc);
void file_save_cb (GtkWidget *widget, gpointer cbdata);
void file_save_as_cb (GtkWidget *widget, gpointer cbdata);
void file_save_all_cb (GtkWidget *widget, gpointer cbdata);
void file_close_cb(GtkWidget *widget, gpointer cbdata);
void file_close_all_cb(GtkWidget *widget, gpointer cbdata);
void file_revert_cb (GtkWidget *widget, gpointer cbdata);
void file_quit_cb (GtkWidget *widget, gpointer cbdata);
void uri_open_cb (GtkWidget *widget, gpointer cbdata);

       gint gedit_file_create_popup (const gchar *title);
#if 0
/* don't add prototypes for public functions */
static void gedit_file_open_ok_sel   (GtkWidget *widget, GtkFileSelection *files);
#endif
static void gedit_file_save_as_ok_sel (GtkWidget *w, gpointer cbdata);
static gint delete_event_cb (GtkWidget *w, GdkEventAny *e);
static void cancel_cb (GtkWidget *w, gpointer cbdata);

/* Close all flag */
       void gedit_close_all_flag_clear (void);
static void gedit_close_all_flag_status (guchar *function);
static void gedit_close_all_flag_verify (guchar *function);

gchar * gedit_file_convert_to_full_pathname (const gchar * fname);


static void  save_all_continue (GtkWidget *widget, gpointer cbdata);

typedef enum {
	SAVE_ALL_RUNNING,
	SAVE_ALL_ENDED
} GeditSaveAllFlagStates;
;

static gint save_all_flag = SAVE_ALL_ENDED;

typedef struct {
	GeditDocument *doc;
	GeditView *view;
	gchar *tmp_buf;
} HackyStruct;

static gboolean 
gedit_view_insert_if_mapped (HackyStruct *hack)
{
	g_return_val_if_fail (GEDIT_IS_VIEW (hack->view), FALSE);

	if (!GTK_WIDGET_REALIZED (hack->view->text)) {
		g_print ("not mapped, returning\n");
		return TRUE;
	}
	
	gtk_text_insert (GTK_TEXT (hack->view->text), NULL,
			 NULL, NULL, hack->tmp_buf, strlen (hack->tmp_buf));

	gedit_view_set_position (hack->view, 0);
	
	g_free (hack->tmp_buf);
	g_free (hack);

	return FALSE;
}

static void
gedit_document_insert_text_when_mapped (GeditDocument *doc, const gchar * tmp_buf,
					gint pos, gboolean something)
{
	HackyStruct *hack;

	gedit_debug (DEBUG_FILE, "");

	hack = g_new (HackyStruct, 1);
	hack->doc = doc;
	hack->view = g_list_nth_data (doc->views, 0);
	hack->tmp_buf = g_strdup (tmp_buf);

	gtk_idle_add ((GtkFunction) gedit_view_insert_if_mapped,
		      hack);
		

}

	


/* TODO: add flash on all operations ....Chema*/
/*        what happens when you open the same doc
	  twice. I think we should add another view
          of the same doc versus opening it twice. Chema*/

/**
 * gedit_file_open:
 * @doc: Document window to fill with text
 * @fname: Filename to open
 *
 * Open a file and read it into the text widget.
 *
 * Return value: 0 on success, 1 on error.
 */

gint
gedit_file_open (GeditDocument *doc, const gchar *fname)
{
	GnomeVFSFileInfo *info;
	GnomeVFSHandle *from_handle;
	GnomeVFSResult  result;

	GnomeVFSURI* uri;
	const gchar* scheme;

	GString *tmp_buf = NULL;
		
	GeditView *view;
			
	gedit_debug (DEBUG_FILE, "");

	g_return_val_if_fail (fname != NULL, 1);
	
	if (doc != NULL) 
	{
		g_return_val_if_fail (doc->filename != fname, -1);
	}

	gedit_flash_va ("%s %s", _(MSGBAR_LOADING_FILE), fname);
	
	/* Update UI */
	while (gtk_events_pending ())
		  gtk_main_iteration ();

	info = gnome_vfs_file_info_new ();
	result = gnome_vfs_get_file_info (fname, 
					  info,
					  (GNOME_VFS_FILE_INFO_GET_MIME_TYPE
					   | GNOME_VFS_FILE_INFO_FOLLOW_LINKS));

	if ((result != GNOME_VFS_OK) || 
	    !(info->valid_fields & GNOME_VFS_FILE_INFO_FIELDS_TYPE) ||
	    (info->type != GNOME_VFS_FILE_TYPE_REGULAR))
	{
		gchar *errstr = g_strdup_printf (_("An error was encountered while opening the file \"%s\"."
					"\nPlease make sure the file exists."), fname);
		gnome_app_error (gedit_window_active_app(), errstr);
		g_free (errstr);

		if(result == GNOME_VFS_OK)
			gnome_vfs_file_info_clear (info);

		return 1;
	}

	if(info->valid_fields & GNOME_VFS_FILE_INFO_FIELDS_SIZE)
	{
		tmp_buf = g_string_sized_new (info->size + 1);
	}
	else
	{
		tmp_buf = g_string_sized_new (PREALLOCATED_BUF_LEN);
	}

	if (tmp_buf == NULL)
	{
		gchar *errstr = g_strdup_printf (_("An error was encountered while opening the file \"%s\"."
						   "\nCould not allocate the required memory."), fname);
		gnome_app_error (gedit_window_active_app(), errstr);
		g_free (errstr);
		gnome_vfs_file_info_clear (info);

		return 1;	
	}

	result = gnome_vfs_open (&from_handle, fname, GNOME_VFS_OPEN_READ);

	if (result != GNOME_VFS_OK)
	{
		gchar *errstr = g_strdup_printf (_("An error was encountered while reading the file: \n\n%s\n\n"
						   "Make sure that you have permissions for opening the file. "), 
						   fname);
		gnome_app_error (gedit_window_active_app(), errstr);
		g_free (errstr);
		g_string_free(tmp_buf, TRUE);
		gnome_vfs_file_info_clear (info);

		return 1;
	}
	
	while (1) 
	{
		GnomeVFSFileSize bytes_read;
		guint8           data [1025];
		
		result = gnome_vfs_read (from_handle, data, 1024, &bytes_read);
		
		if ((result != GNOME_VFS_OK) && (result != GNOME_VFS_ERROR_EOF))
		{
			gchar *errstr = g_strdup_printf (_("An error was encountered while reading the file: \n\n%s"), 
						   	fname);
			gnome_app_error (gedit_window_active_app(), errstr);
			g_free (errstr);
			g_string_free(tmp_buf, TRUE);
			gnome_vfs_file_info_clear (info);

			return 1;
		}

		if (bytes_read == 0)
		{
			break;
		}
		
		if (bytes_read >  0 && bytes_read <= 1024)
		{
			data [bytes_read] = '\0';
		}
		else 
		{
			gchar *errstr = g_strdup_printf (_("Internal error reading the file: \n\n%s\n\n"
							   "Please, report this error to submit@bugs.gnome.org."), fname);
			gnome_app_error (gedit_window_active_app(), errstr);
			g_free (errstr);
			g_string_free(tmp_buf, TRUE);
			gnome_vfs_file_info_clear (info);

			return 1;
		}
		
		g_string_append(tmp_buf, data);
	}

	result = gnome_vfs_close (from_handle);
	
	if (result != GNOME_VFS_OK)
	{
		gchar *errstr = g_strdup_printf (_("An error was encountered while closing the file: \n\n%s"), 
						   fname);
		gnome_app_error (gedit_window_active_app(), errstr);
		g_free (errstr);
		g_string_free(tmp_buf, TRUE);
		gnome_vfs_file_info_clear (info);

		return 1;
	}

	if (doc==NULL) 
	{
		doc = gedit_document_new ();
		doc->filename = g_strdup (fname);
		doc->untitled_number = 0;
		gedit_document_insert_text_when_mapped (doc, tmp_buf->str, 0, FALSE);
	} 
	else 
	{
		if (doc->filename != NULL)
			g_free (doc->filename);
		doc->filename = g_strdup (fname);
		doc->untitled_number = 0;
		gedit_document_insert_text (doc, tmp_buf->str, 0, FALSE);

		/* Set the cursor position to the start */
		view = g_list_nth_data (doc->views, 0);
		g_return_val_if_fail (view!=NULL, 1);
		gedit_view_set_position (view, 0);
	}
	

	/* Conservatively set doc to readonly */
	gedit_document_set_readonly (doc, TRUE);

	uri = gnome_vfs_uri_new (fname);

	if (uri != NULL)
	{
		scheme = gnome_vfs_uri_get_scheme(uri);

		/* FIXME: all remote files are marked as readonly */
		if ((scheme != NULL) && (strcmp (scheme, "file") == 0) && GNOME_VFS_FILE_INFO_LOCAL (info))
		{
			gchar* tmp_str;
		        gchar* tmp_str2;
												
			tmp_str = gnome_vfs_uri_to_string (uri, GNOME_VFS_URI_HIDE_TOPLEVEL_METHOD);				
			tmp_str2 = gnome_vfs_unescape_string_for_display (tmp_str);
	
			if (tmp_str2 != NULL)
			{
				gedit_document_set_readonly (doc, access (tmp_str2, W_OK) ? TRUE : FALSE);
			}			

			g_free (tmp_str2);
			g_free (tmp_str);
		}
		
		gnome_vfs_uri_unref (uri);
	}
	
	g_string_free (tmp_buf, TRUE);
	gnome_vfs_file_info_clear (info);

	doc->changed = FALSE;
	gedit_document_set_title (doc);
	gedit_document_text_changed_signal_connect (doc);
	
	gedit_flash_va ("%s %s", _(MSGBAR_FILE_OPENED), fname);
	gedit_recent_add (fname);
	gedit_recent_update_all_windows (mdi); 

	return 0;
}

static gint
gedit_file_selector_key_event (GtkFileSelection *fsel, GdkEventKey *event)
{
	if (event->keyval == GDK_Escape) {
		gtk_button_clicked (GTK_BUTTON (fsel->cancel_button));
		return 1;
	} else
		return 0;
}


/**
 * gedit_file_save_as:
 * @doc: 
 * 
 * creates the save as dialog and connects the signals to
 * the button
 **/
static void
gedit_file_save_as (GeditDocument *doc)
{
	static gint ok_signal = 0;
	
	gedit_debug (DEBUG_FILE, "");

	if (doc == NULL)
		doc = gedit_document_current();

	if (doc == NULL)
		return;
			
	if (save_file_selector && GTK_WIDGET_VISIBLE (save_file_selector))
		return;

	if (save_file_selector == NULL)
	{
		save_file_selector = gtk_file_selection_new (NULL);
		
		gtk_signal_connect(GTK_OBJECT(save_file_selector),
				   "delete_event",
				   GTK_SIGNAL_FUNC(delete_event_cb),
				   save_file_selector);
		gtk_signal_connect (GTK_OBJECT(GTK_FILE_SELECTION(save_file_selector)->cancel_button),
				    "clicked",
				    GTK_SIGNAL_FUNC(cancel_cb),
				    save_file_selector);
		gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION(save_file_selector)),
				    "key_press_event",
				    GTK_SIGNAL_FUNC (gedit_file_selector_key_event),
				    NULL);

		/* If this save as was the result of a close all, and the user cancels the
		   save, clear the flag */
		gtk_signal_connect(GTK_OBJECT(save_file_selector),
				   "delete_event",
				   GTK_SIGNAL_FUNC(gedit_close_all_flag_clear),
				   NULL);
		gtk_signal_connect (GTK_OBJECT(GTK_FILE_SELECTION(save_file_selector)->cancel_button),
				    "clicked",
				    GTK_SIGNAL_FUNC(gedit_close_all_flag_clear),
				    NULL);

		/* If this save as was the result of a save all, and the user cancels the
		   save, clear the flag */
		gtk_signal_connect(GTK_OBJECT(save_file_selector),
				   "delete_event",
				   GTK_SIGNAL_FUNC(save_all_continue),
				   NULL);
		gtk_signal_connect (GTK_OBJECT(GTK_FILE_SELECTION(save_file_selector)->cancel_button),
				    "clicked",
				    GTK_SIGNAL_FUNC(save_all_continue),
				    NULL);

		/*
		g_print ("1. Doc->filename %s untitled #%i\n", doc->filename, doc->untitled_number);
		*/
		/* OK clicked */
		ok_signal = gtk_signal_connect (GTK_OBJECT(GTK_FILE_SELECTION(save_file_selector)->ok_button),
				    "clicked",
				    GTK_SIGNAL_FUNC (gedit_file_save_as_ok_sel),
				    doc);

	}

	gtk_signal_disconnect (GTK_OBJECT(GTK_FILE_SELECTION(save_file_selector)->ok_button), ok_signal);

	/* OK clicked */
	ok_signal = gtk_signal_connect (GTK_OBJECT(GTK_FILE_SELECTION(save_file_selector)->ok_button),
			    "clicked",
			    GTK_SIGNAL_FUNC (gedit_file_save_as_ok_sel),
			    doc);

	if (doc->filename == NULL)
	{
		gchar* name = gedit_document_get_tab_name (doc, FALSE);
		gtk_file_selection_set_filename (GTK_FILE_SELECTION (save_file_selector), name);
		g_free (name);
	}
	else
	{
		/* FIXME: does not work well with URI */
		gchar* name = gnome_vfs_unescape_string_for_display (doc->filename);
		g_return_if_fail (name != NULL);

		gtk_file_selection_set_filename (GTK_FILE_SELECTION (save_file_selector), name);
		g_free (name);
/*
		gtk_file_selection_set_filename (GTK_FILE_SELECTION (save_file_selector), 		
						 doc->filename);
*/												
	}

	gtk_window_set_transient_for (GTK_WINDOW (save_file_selector), gedit_window_active ());

	gtk_window_set_modal (GTK_WINDOW (save_file_selector), TRUE);
	gtk_window_set_title (GTK_WINDOW (save_file_selector), _("Save As..."));

	if (!GTK_WIDGET_VISIBLE (save_file_selector))
	{
		gtk_window_position (GTK_WINDOW (save_file_selector), GTK_WIN_POS_MOUSE);
		gtk_widget_show(save_file_selector);
	}
	return;
}

/**
 * gedit_file_save:
 * @doc: Document window that contains the text we want to save
 * @fname: Filename to save as
 *
 * Dump the text from a document window out to a file, uses a
 * single fputs() call.
 *
 * Return value: 0 on success, 1 on error.
 */
gint
gedit_file_save (GeditDocument *doc, const gchar *fname)
{
	FILE  *file_pointer;
	gchar *buffer;
	GeditView  *view;
	gchar* unescaped_name = NULL; 
	const gchar* old_name; 

	gedit_debug (DEBUG_FILE, "");

	g_return_val_if_fail (doc!=NULL, 1);
	
	view  = GEDIT_VIEW ( g_list_nth_data(doc->views, 0) );
	
	g_return_val_if_fail (view!=NULL, 1);
	
	old_name = fname;
	
	if (fname == NULL)
	{
		if (doc->filename == NULL)
		{
			GtkWidget *w = GTK_WIDGET (g_list_nth_data(doc->views, 0));
			
			if(w != NULL)
				gnome_mdi_set_active_view (mdi, w);

			gedit_file_save_as (doc);
			return 1;
		}
		else
		{
			unescaped_name = gnome_vfs_unescape_string_for_display (doc->filename);
			fname = unescaped_name;
			old_name = doc->filename;
		}
	}

	g_return_val_if_fail (doc   != NULL, 1);
	g_return_val_if_fail (fname != NULL, 1);
	
	buffer = gedit_document_get_buffer (view->doc);

	g_return_val_if_fail (buffer != NULL, 1);

#if 0
	/* Allow the user to save 0 bytes files */
	if (strlen(buffer) == 0)
	{
		gchar *errstr = g_strdup_printf (_("The document contains no data."), fname);
		gnome_app_error (mdi->active_window, errstr);
		g_free (errstr);
		g_free (buffer);
		gedit_close_all_flag_clear();
		return 1;
	}
#endif	

	if ((file_pointer = fopen (fname, "w")) == NULL)
	{
		gchar *errstr = g_strdup_printf (_("gedit was unable to save the file: "
						   "\n\n %s \n\n"
						   "Make sure that the path you provided exists,"
						   "and that you have the appropriate write permissions."), fname);
		gnome_app_error (mdi->active_window, errstr);
		g_free (errstr);
		g_free (buffer);
		g_free (unescaped_name);
		gedit_close_all_flag_clear();
		return 1;
	}
	
	if (fputs (buffer, file_pointer) == EOF)
	{
		gchar *errstr = g_strdup_printf (_("gedit was unable to save the file:"
						   "\n\n %s \n\n"
						   "Because of an unknown reason (1). Please report this "
						   "Problem to submit@bugs.gnome.org"), fname);
		gnome_app_error (mdi->active_window, errstr);
		fclose (file_pointer);
		g_free (errstr);
		g_free (buffer);
		g_free (unescaped_name);
		gedit_close_all_flag_clear();
		return 1;
	}

	if (buffer != NULL)
		g_free (buffer);
	
	if (fclose (file_pointer) != 0)
	{
		gchar *errstr = g_strdup_printf (_("gedit was unable to save the file:"
						   "\n\n %s \n\n"
						   "Because of an unknown reason (2). Please report this "
						   "Problem to submit@bugs.gnome.org"), fname);
		gnome_app_error (mdi->active_window, errstr);
		g_free (errstr);
		g_free (unescaped_name);
		gedit_close_all_flag_clear();
		return 1;
	}

	if (doc->filename != old_name)
	{
		if (doc->filename != NULL)
			g_free (doc->filename);
		doc->filename = g_strdup (old_name);
	}

	doc->changed = FALSE;
	doc->untitled_number = 0;
	gedit_document_set_readonly (doc, access (fname, W_OK) ? TRUE : FALSE);
	gedit_document_set_title (doc);

	gedit_document_text_changed_signal_connect (doc);

	gedit_flash (_(MSGBAR_FILE_SAVED));

	if (unescaped_name != NULL) g_free (unescaped_name);

	return 0;
}

static void 
save_all_continue (GtkWidget *widget, gpointer cbdata)
{
	if (save_all_flag == SAVE_ALL_RUNNING)
		file_save_all ();
}

void 
file_save_all (void)
{
	static gint last_file_saved = -1;
	gint i;
	GeditDocument *doc;
	
	save_all_flag = SAVE_ALL_RUNNING;
	
	gedit_debug (DEBUG_FILE, "");		
	
        for (i = last_file_saved + 1; i < g_list_length (mdi->children); i++)
	{
		doc = (GeditDocument *)g_list_nth_data (mdi->children, i);
		
		if ((!doc->readonly && doc->changed))
		{
			gchar* fname = doc->filename;
			
			gedit_file_save (doc, NULL);
			
			if(fname == NULL)
			{
				last_file_saved = i;	
				return;
			}
		}
	}

	last_file_saved = -1;
	save_all_flag = SAVE_ALL_ENDED;
}

/**
 * gedit_file_stdin:
 * @doc: Document window to fill with text
 *
 * Open stdin and read it into the text widget.
 *
 * Return value: 0 on success, 1 on error.
 */
gint
gedit_file_stdin (GeditDocument *doc)
{
	gchar *tmp_buf;
	struct stat stats;
	guint buffer_length;
	guint pos = 0;
	GeditView *view;

	gedit_debug (DEBUG_FILE, "");

	fstat(STDIN_FILENO, &stats);
	
	if (stats.st_size  == 0)
	{
		
		gedit_debug (DEBUG_FILE, "size = 0. end");
		return 1;
	}
	
	if ((tmp_buf = g_new0 (gchar, GEDIT_STDIN_BUFSIZE+1)) == NULL)
	{
		gnome_app_error (mdi->active_window, _("Could not allocate the required memory."));
		gedit_debug (DEBUG_FILE, "mem alloca error. returning");
		return 1;
	}

	if (doc==NULL)
		doc = gedit_document_new ();

	view = g_list_nth_data (doc->views, 0);

	g_return_val_if_fail (view!=NULL, 1);

	while (feof(stdin)==0)
	{
		buffer_length = fread (tmp_buf,1,GEDIT_STDIN_BUFSIZE, stdin);
		tmp_buf[buffer_length+1]='\0';
		gedit_document_insert_text (doc,tmp_buf,pos,FALSE);
		pos= pos + buffer_length ;
		if (ferror(stdin)!=0)
		{
			gnome_app_error (mdi->active_window, _("Error in the pipe."));
			break ;
		}
	}
	
	fclose (stdin);
        doc->changed = TRUE;
	gedit_document_set_title (doc);

	/* Move the window to the top after inserting */
	gedit_view_set_window_position (view, 0);

	g_free (tmp_buf);
       
	gedit_flash_va ("%s %s", _(MSGBAR_FILE_OPENED), "STDIN");

	return 0;
}

void
file_new_cb (GtkWidget *widget, gpointer cbdata)
{
	GeditDocument *doc;

	gedit_debug (DEBUG_FILE, "");
	
	gedit_flash (_(MSGBAR_FILE_NEW));

	gedit_close_all_flag_verify ("file_new_cb");

	doc = gedit_document_new ();
}

static GSList *
gedit_file_selector_get_filenames (GtkFileSelection *file_selector,
				   gchar **directory_)
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
	for (; list != NULL; list = list->next) {
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
	static gchar *selected_file;
	gchar *directory;
	gint length;

	selected_file = gtk_file_selection_get_filename (file_selector);

	if (!g_file_test (selected_file, G_FILE_TEST_ISDIR))
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
gedit_file_open_ok_sel (GtkWidget *widget, GtkWidget *file_selector_)
{
	GtkFileSelection *file_selector;
	gchar * file_name;
	gchar * full_path;
	gchar * directory;
	GSList *files;
	GSList *list;

	gedit_debug (DEBUG_FILE, "");

	g_return_if_fail (GTK_IS_FILE_SELECTION (file_selector_));
	file_selector = GTK_FILE_SELECTION (file_selector_);

	if (gedit_file_open_is_dir (file_selector))
		return;

	file_name = gtk_file_selection_get_filename (file_selector);
	directory = g_dirname (file_name);

	files = gedit_file_selector_get_filenames (file_selector, &directory);

	if (files == NULL) {
		if (gedit_document_new_with_file (file_name))
			gedit_flash_va (_("Loaded file %s"), file_name);
	} else {
		list = files;
		for (; list != NULL; list = list->next) {
			file_name = list->data;
			full_path = g_concat_dir_and_file (directory, file_name);
			if (gedit_document_new_with_file (full_path))
				gedit_flash_va (_("Loaded file %s"), full_path);
			g_free (full_path);
			g_free (file_name);
		}
	}

	if (g_slist_length (files) > 1)
		gedit_flash_va (_("Loaded %i files"), g_slist_length (files));

	g_free (directory);
	
	gtk_widget_hide (GTK_WIDGET(file_selector));
	
	if (gedit_document_current())
	{
		g_return_if_fail(gedit_window_active_app());
		gedit_window_set_widgets_sensitivity_ro (gedit_window_active_app(), 
				gedit_document_current()->readonly);
	}	

	return;
}

void
file_open_cb (GtkWidget *widget, gpointer cbdata)
{
	static GtkWidget *open_file_selector;

	gedit_debug (DEBUG_FILE, "");

	gedit_close_all_flag_verify ("file_open_cb");

	if (open_file_selector && GTK_WIDGET_VISIBLE (open_file_selector))
		return;

	if (open_file_selector == NULL)
	{
		open_file_selector = gtk_file_selection_new(NULL);
		gtk_signal_connect(GTK_OBJECT(open_file_selector),
				   "delete_event",
				   GTK_SIGNAL_FUNC (delete_event_cb),
				   open_file_selector);
		gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(open_file_selector)->ok_button),
				   "clicked",
				   GTK_SIGNAL_FUNC (gedit_file_open_ok_sel),
				   open_file_selector);
		gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(open_file_selector)->cancel_button),
				   "clicked",
				   GTK_SIGNAL_FUNC (cancel_cb),
				   open_file_selector);
		gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION(open_file_selector)),
				    "key_press_event",
				    GTK_SIGNAL_FUNC (gedit_file_selector_key_event),
				    NULL);
		gtk_clist_set_selection_mode(
			GTK_CLIST(GTK_FILE_SELECTION(open_file_selector)->file_list),
			GTK_SELECTION_EXTENDED);
	}
	else
	{
		GtkFileSelection *file_selector;

		g_return_if_fail (GTK_IS_FILE_SELECTION (open_file_selector));

		/* Clear the selection & file from the last open */
		file_selector = GTK_FILE_SELECTION(open_file_selector);
		gtk_clist_unselect_all (GTK_CLIST(file_selector->file_list));
		gtk_entry_set_text (GTK_ENTRY(file_selector->selection_entry), "");
	}

	gtk_window_set_transient_for (GTK_WINDOW (open_file_selector), gedit_window_active ());

	gtk_window_set_modal (GTK_WINDOW (open_file_selector), TRUE);
	gtk_window_set_title (GTK_WINDOW (open_file_selector), _("Open File ..."));

	if (!GTK_WIDGET_VISIBLE (open_file_selector))
	{
		gtk_window_position (GTK_WINDOW (open_file_selector), GTK_WIN_POS_MOUSE);
		gtk_widget_show(open_file_selector);
	}

	return;
}


void
file_save_as_cb (GtkWidget *widget, gpointer cbdata)
{
	GeditDocument *doc;
	
	gedit_debug (DEBUG_FILE, "");

	gedit_close_all_flag_verify ("file_save_as_cb");
	
	doc = gedit_document_current();
		
	if (doc==NULL)
		return;

	gedit_file_save_as (doc);
}


static gint
delete_event_cb (GtkWidget *widget, GdkEventAny *event)
{
	gedit_debug (DEBUG_FILE, "");

	g_print ("DElete event callback ........\n");
	gtk_widget_hide (widget);
	return TRUE;
}
 
static void
cancel_cb (GtkWidget *w, gpointer data)
{
	gedit_debug (DEBUG_FILE, "");
	
	gtk_widget_hide (data);
}

gint
file_save_document (GeditDocument *doc)
{
	gedit_debug (DEBUG_FILE, "");

	if (doc==NULL)
		return FALSE;

	if (doc->filename == NULL)
	{
		GtkWidget *w = GTK_WIDGET (g_list_nth_data(doc->views, 0));
			
		if(w != NULL)
			gnome_mdi_set_active_view (mdi, w);

		gedit_file_save_as (doc);
		return FALSE;
	}
	else
	{
		gedit_file_save (doc, NULL);
		return TRUE;
	}
}

void
file_save_cb (GtkWidget *widget, gpointer cbdata)
{
	GeditDocument *doc;
	
	gedit_debug (DEBUG_FILE, "");

	gedit_close_all_flag_verify ("file_save_cb");

	doc = gedit_document_current ();

	if (doc==NULL)
		return;

	file_save_document (doc);
}


void 
file_save_all_cb (GtkWidget *widget, gpointer cbdata)
{
	file_save_all();
}


void 
uri_open_cb (GtkWidget *widget, gpointer cbdata)
{
	gedit_dialog_open_uri ();
}

static void
gedit_file_save_as_ok_sel (GtkWidget *w, gpointer cbdata)
{
	GeditDocument *doc;
	GeditView *nth_view;
	gchar *file_name;
	gint i;

	gedit_debug (DEBUG_FILE, "");

	doc = cbdata;

	/*
	g_print ("Doc->filename %s untitled #%i\n", doc->filename, doc->untitled_number);
	*/
		 
	if (!doc)
		return;
	
	file_name = g_strdup(gtk_file_selection_get_filename (GTK_FILE_SELECTION(save_file_selector)));
	
	gtk_widget_hide (GTK_WIDGET (save_file_selector));
	save_file_selector = NULL;
	
        if (g_file_exists (file_name))
	{
		guchar * msg;
		GtkWidget *msgbox;
		gint ret;
		msg = g_strdup_printf (_("``%s'' is about to be overwritten. Do you want to continue ?"), file_name);
		msgbox = gnome_message_box_new (msg, GNOME_MESSAGE_BOX_QUESTION, GNOME_STOCK_BUTTON_YES,
						GNOME_STOCK_BUTTON_NO, GNOME_STOCK_BUTTON_CANCEL, NULL);
		gnome_dialog_set_default (GNOME_DIALOG (msgbox), 2);
		ret = gnome_dialog_run_and_close (GNOME_DIALOG (msgbox));
		g_free (msg);
		switch (ret)
		{
		case 0:
			break;
		case 1:
			gedit_close_all_flag_clear();
			gedit_flash (_("Document has not been saved."));
			return;
		default:
			gedit_close_all_flag_clear();
			return;
		}
	}
	    
	if (gedit_file_save (doc, file_name) != 0)
	{
		gedit_flash (_("Error saving file!"));
		g_free (file_name);
		gedit_close_all_flag_clear();
		return;
	}

        /* If file save was succesfull, then we should turn the readonly flag off */
	for (i = 0; i < g_list_length (doc->views); i++)
	{
		nth_view = g_list_nth_data (doc->views, i);
		gedit_view_set_readonly (nth_view, FALSE);
	}

	/* Add the saved as file to the recent files ( history ) menu */
	gedit_recent_add (file_name);
	gedit_recent_update_all_windows (mdi);

	gedit_flash_va ("%s %s", _(MSGBAR_FILE_SAVED), file_name);

	g_free (file_name);

	gedit_close_all_flag_status ("gedit_file_save_ok_sel");
	switch (gedit_close_all_flag){
	case GEDIT_CLOSE_ALL_FLAG_NORMAL:
		break;
	case GEDIT_CLOSE_ALL_FLAG_CLOSE_ALL:
		file_close_all_cb (NULL, NULL);
		break;
	case GEDIT_CLOSE_ALL_FLAG_QUIT:
		file_quit_cb (NULL, NULL);
		break;
	default:
		g_return_if_fail (FALSE);
	}

	if(save_all_flag == SAVE_ALL_RUNNING)
	{
		file_save_all();
	}
	
	/* We need to set the revert menu item sensitivity if we where saving
	   as from an Untitled doc */
	gedit_window_set_view_menu_sensitivity (gedit_window_active_app ());

	if(gedit_window_active_app () != NULL)
	{
		gedit_window_set_widgets_sensitivity_ro (gedit_window_active_app (), FALSE);	
	}
}


void
file_close_cb (GtkWidget *widget, gpointer cbdata)
{

	gedit_debug (DEBUG_FILE, "");

	if (mdi->active_child == NULL)
		return;
	gnome_mdi_remove_child (mdi, mdi->active_child, FALSE);

	gedit_document_set_title (gedit_document_current());

	gedit_window_set_widgets_sensitivity (FALSE);
}

void
file_close_all_cb (GtkWidget *widget, gpointer cbdata)
{
	gedit_debug (DEBUG_FILE, "");

	gedit_close_all_flag_status("File_close_all_cb. Before");
	
	if (gedit_close_all_flag != GEDIT_CLOSE_ALL_FLAG_QUIT)
		gedit_close_all_flag = GEDIT_CLOSE_ALL_FLAG_CLOSE_ALL;

	if (mdi->active_child == NULL)
		return;

	gnome_mdi_remove_all (mdi, FALSE);

	/* If this close all was successful, clear the flag */
	if (gedit_document_current()==NULL)
		gedit_close_all_flag_clear();

	gedit_window_set_widgets_sensitivity (FALSE);
	
}

void
file_quit_cb (GtkWidget *widget, gpointer cbdata)
{
	gedit_debug (DEBUG_FILE, "");

	gedit_close_all_flag = GEDIT_CLOSE_ALL_FLAG_QUIT;
	file_close_all_cb (NULL, NULL);

	if (gedit_document_current()!=NULL)
		return;

	/* We need to disconnect the signal because mdi "destroy" event
	   is connected to file_quit_cb ( i.e. this function ). Chema */
	gtk_signal_disconnect (GTK_OBJECT(mdi), gedit_mdi_destroy_signal);
	
	gtk_object_destroy (GTK_OBJECT (mdi));
	gedit_prefs_save_settings ();
	gedit_recent_history_write_config ();

	gtk_main_quit ();
}

void
file_revert_cb (GtkWidget *widget, gpointer data)
{
	GtkWidget *msgbox;
	GeditDocument *doc = gedit_document_current ();
	gchar * msg;
	gchar * file_name;

	gedit_debug(DEBUG_FILE, "");

	gedit_close_all_flag_verify ("file_save_cb");

	if (!doc)
		return;

	if (doc->filename==NULL)
	{
		
		msg = g_strdup (_("You can't revert an Untitled document\n"));
		msgbox = gnome_message_box_new (msg,
						GNOME_MESSAGE_BOX_INFO,
						GNOME_STOCK_BUTTON_OK,
						NULL);
		gnome_dialog_set_parent (GNOME_DIALOG (msgbox),
					 GTK_WINDOW (mdi->active_window));
		gnome_dialog_run_and_close (GNOME_DIALOG(msgbox));
		g_free (msg);
		return;
	}
		
	
	msg = g_strdup_printf (_("Are you sure you wish to revert all changes?\n(%s)"),
			       doc->filename);
	msgbox = gnome_message_box_new (msg, GNOME_MESSAGE_BOX_QUESTION, GNOME_STOCK_BUTTON_YES,
					GNOME_STOCK_BUTTON_NO, NULL);
	gnome_dialog_set_default (GNOME_DIALOG (msgbox), 2);
	    	    
	switch (gnome_dialog_run_and_close (GNOME_DIALOG (msgbox)))
	{
	case 0:
		gedit_document_delete_text (doc, 0, gedit_document_get_buffer_length(doc), FALSE);
		file_name = g_strdup (doc->filename);
		gedit_file_open (doc, file_name);
		g_free (file_name);
		break;
	default:
		break;
	}
	g_free (msg);
}


gint 
gedit_file_create_popup (const gchar *title)
{
	GtkWidget *msgbox;
	GeditDocument *doc;
	int ret;
	char *msg;

	gedit_debug(DEBUG_FILE, "");
	
	msg = g_strdup_printf (_("The file ``%s'' does not exist.  Would you like to create it?"),
			       title);
	msgbox = gnome_message_box_new (msg,
					GNOME_MESSAGE_BOX_QUESTION,
					GNOME_STOCK_BUTTON_YES,
					GNOME_STOCK_BUTTON_NO,
					NULL);
	ret = gnome_dialog_run_and_close (GNOME_DIALOG (msgbox));
	g_free (msg);
	switch (ret)
	{
	case 0: 
		doc = gedit_document_new_with_title (title);
		doc->changed = TRUE;
		return TRUE;
	case 1: 
		return FALSE;
	default:
		return FALSE;
	}

	return FALSE;
}

void 
gedit_close_all_flag_clear (void)
{
	gedit_debug (DEBUG_FILE, "");

	gedit_close_all_flag = GEDIT_CLOSE_ALL_FLAG_NORMAL;
	
}

static void
gedit_close_all_flag_status (guchar * function)
{
	gedit_debug(DEBUG_FILE, "");

	g_return_if_fail (function!=NULL);

	return;
	
	g_print("Close all flag state is: ");
	switch (gedit_close_all_flag){
	case GEDIT_CLOSE_ALL_FLAG_NORMAL:
		g_print("NORMAL");
		break;
	case GEDIT_CLOSE_ALL_FLAG_CLOSE_ALL:
		g_print("CLOSE ALL");
		break;
	case GEDIT_CLOSE_ALL_FLAG_QUIT:
		g_print("QUIT");
		break;
	}
	g_print("  %s \n", function);
}

static void
gedit_close_all_flag_verify (guchar *function)
{
	gedit_debug(DEBUG_FILE, "");

	if (gedit_close_all_flag != GEDIT_CLOSE_ALL_FLAG_NORMAL)
		g_warning ("The close all flag was set !!!!! Func: %s", function);
}


/**
 * gedit_file_convert_to_full_pathname: convert a file_name to a full path
 * @file_name_to_convert: the file name to convert
 * 
 *
  Test strings
   1. /home/user/./dir/././././file.txt			/home/user/dir/file.txt
   2. /home/user/cvs/gedit/../../docs/file.txt		/home/user/docs/file.txt
   3. File name in: /home/chema/cvs/gedit/../../cvs/../docs/././././././temp_32avo.txt
   File name out: /home/chema/docs/temp_32avo.txt
 *
 * 
 * Return Value: a pointer to a allocated string. The calling function is responsible
 *               o freeing the string
 **/
gchar *
gedit_file_convert_to_full_pathname (const gchar * file_name_to_convert)
{
	gint file_name_in_length;
	
	gchar *file_name_in;
	gchar *file_name_out;

	gint i, j=0;

	g_return_val_if_fail (file_name_to_convert != NULL, NULL);

	if (g_path_is_absolute(file_name_to_convert))
		file_name_in = g_strdup (file_name_to_convert);
	else
		file_name_in = g_strdup_printf ("%s/%s",
						g_get_current_dir(),
						file_name_to_convert);

	file_name_in_length = strlen (file_name_in);
	file_name_out = g_malloc (file_name_in_length + 1);

	g_return_val_if_fail (file_name_in[0] == '/', NULL);

	for (i = 0; i < file_name_in_length - 1; i++)
	{
		/* remove all the "../" from file_name */
		if (file_name_in[i] == '.' && file_name_in[i+1] == '.')
		{
			if (i < file_name_in_length-1)
				if (file_name_in[i+2] == '/')
				{
					/* Remove the last directory in file_name_out */
					i+=2;
					j--;
					while (file_name_out [j-1] != '/')
						j--;
					j--;
				}
		}
		
		/* remove all the "./"'s from file_name */
		if (file_name_in[i] == '.' && file_name_in[i+1] == '/')
		{
			i++;
			continue;
		}

		file_name_out [j++] = file_name_in [i];
	}
	
	file_name_out [j++] = file_name_in [i];
	file_name_out [j] = '\0';

	return file_name_out;

} 

