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

#define GEDIT_STDIN_BUFSIZE 1024

GtkWidget *save_file_selector = NULL;
GtkWidget *open_file_selector = NULL;

gint gedit_file_open (Document *doc, gchar *fname);
gint gedit_file_save (Document *doc, gchar *fname);
void gedit_file_save_as (Document *doc);
gint gedit_file_stdin (Document *doc);

void file_new_cb (GtkWidget *widget, gpointer cbdata);
void file_open_cb (GtkWidget *widget, gpointer cbdata);
gint file_save_document (Document * doc);
void file_save_cb (GtkWidget *widget, gpointer cbdata);
void file_save_as_cb (GtkWidget *widget, gpointer cbdata);
void file_save_all_cb (GtkWidget *widget, gpointer cbdata);
void file_close_cb(GtkWidget *widget, gpointer cbdata);
void file_close_all_cb(GtkWidget *widget, gpointer cbdata);
void file_revert_cb (GtkWidget *widget, gpointer cbdata);
void file_quit_cb (GtkWidget *widget, gpointer cbdata);

       gint gedit_file_create_popup (guchar *title);
static void gedit_file_open_ok_sel   (GtkWidget *widget, GtkFileSelection *files);
static void gedit_file_save_as_ok_sel (GtkWidget *w, gpointer cbdata);
static gint delete_event_cb (GtkWidget *w, GdkEventAny *e);
static void cancel_cb (GtkWidget *w, gpointer cbdata);

/* Close all flag */
       void gedit_close_all_flag_clear (void);
static void gedit_close_all_flag_status (guchar *function);
static void gedit_close_all_flag_verify (guchar *function);

gchar * gedit_file_convert_to_full_pathname (const gchar * fname);


/* TODO : add flash on all operations ....Chema*/
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
gedit_file_open (Document *doc, gchar *fname)
{
	gchar *tmp_buf;
	struct stat stats;
	FILE *fp;
	Document *currentdoc;
	View *view;
	
	gedit_debug ("start", DEBUG_FILE);
	
	g_return_val_if_fail (fname != NULL, 1);

	currentdoc = gedit_document_current();

	if (stat(fname, &stats) ||  !S_ISREG(stats.st_mode))
	{
		gchar *errstr = g_strdup_printf (_("An error was encountered while opening the file \"%s\"."
							      "\nPlease make sure the file exists."), fname);
		gnome_app_error (gedit_window_active_app(), errstr);
		g_free (errstr);
		return 1;
	}

	if (stats.st_size  == 0)
	{
		gchar *errstr = g_strdup_printf (_("An error was encountered while opening the file:\n\n%s\n\n"
						    "\nPlease make sure the file is not being used by another application\n"
						    "and that the file is not empty."), fname);
		gnome_app_error (gedit_window_active_app(), errstr);
		g_free (errstr);
		return 1;
	}
	
	if ((tmp_buf = g_new0 (gchar, stats.st_size + 1)) == NULL)
	{
		gnome_app_error (gedit_window_active_app(), _("Could not allocate the required memory."));
		return 1;
	}

	if ((fp = fopen (fname, "r")) == NULL)
	{
		gchar *errstr = g_strdup_printf (_("gedit was unable to open the file: \n\n%s\n\n"
						   "Make sure that you have permissions for opening the file."), fname);
		gnome_app_error (gedit_window_active_app(), errstr);
		g_free (errstr);
		return 1;
	}

	fread (tmp_buf, 1, stats.st_size, fp);
	fclose (fp);

	if (doc==NULL)
		doc = gedit_document_new ();

	doc->filename = g_strdup (fname);
	doc->untitled_number = 0;

	gedit_document_insert_text (doc, tmp_buf, 0, FALSE);
	gedit_document_set_readonly (doc, access (fname, W_OK) ? TRUE : FALSE);
	
	/* Set the cursor position to the start */
	view = g_list_nth_data (doc->views, 0);
	g_return_val_if_fail (view!=NULL, 1);
	gedit_view_set_position (view, 0);

	doc->changed = FALSE;
	gedit_document_set_title (doc);
	gedit_document_text_changed_signal_connect (doc);

	g_free (tmp_buf);
	
	gedit_flash_va ("%s %s", _(MSGBAR_FILE_OPENED), fname);
	gedit_recent_add (fname);
	gedit_recent_update (gedit_window_active_app());
	gedit_debug ("end", DEBUG_FILE);

	return 0;
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
gedit_file_save (Document *doc, gchar *fname)
{
	FILE *fp;
	gchar *tmpstr;
	View *view;

	gedit_debug ("", DEBUG_FILE);

	g_return_val_if_fail (doc!=NULL, 1);
	
	view  = VIEW ( g_list_nth_data(doc->views, 0) );
	
	g_return_val_if_fail (view!=NULL, 1);
	
	if (fname == NULL)
	{
		if (doc->filename == NULL)
		{
			gedit_file_save_as (doc);
			return 1;
		}
		else
		{
			fname = doc->filename;
		}
	}

	g_return_val_if_fail (doc != NULL, 1);
	g_return_val_if_fail (fname != NULL, 1);
	
	if ((fp = fopen (fname, "w")) == NULL)
	{
		gchar *errstr = g_strdup_printf (_("gedit was unable to save the file: "
						   "\n\n %s \n\n"
						   "Make sure that the path you provided exists,"
						   "and that you have the appropriate write permissions."), fname);
		gnome_app_error (mdi->active_window, errstr);
		g_free (errstr);
		gedit_close_all_flag_clear();
		return 1;
	}
	
	tmpstr = gedit_document_get_buffer (view->doc);

	if (fputs (tmpstr, fp) == EOF)
	{
		gchar *errstr = g_strdup_printf (_("gedit was unable to save the file :"
						   "\n\n %s \n\n"
						   "Because of an unknown reason (1). Please report this "
						   "Problem to submit@bugs.gnome.org"), fname);
		gnome_app_error (mdi->active_window, errstr);
		fclose (fp);
		g_free (errstr);
		gedit_close_all_flag_clear();
		return 1;
	}
	
	g_free (tmpstr);
	
	if (fclose (fp) != 0)
	{
		gchar *errstr = g_strdup_printf (_("gedit was unable to save the file :"
						   "\n\n %s \n\n"
						   "Because of an unknown reason (2). Please report this "
						   "Problem to submit@bugs.gnome.org"), fname);
		gnome_app_error (mdi->active_window, errstr);
		fclose (fp);
		g_free (errstr);
		gedit_close_all_flag_clear();
		return 1;
	}

	if (doc->filename != fname)
	{
		g_free (doc->filename);
		doc->filename = g_strdup (fname);
	}
	
	doc->changed = FALSE;
	doc->untitled_number = 0;
	gedit_document_set_readonly (doc, access (fname, W_OK) ? TRUE : FALSE);
	gedit_document_set_title (doc);

	gedit_document_text_changed_signal_connect(doc);

	gedit_flash (_(MSGBAR_FILE_SAVED));

	return 0;
}


/**
 * gedit_file_save_as:
 * @doc: 
 * 
 * creates the save as dialog and connects the signals to
 * the button
 **/
void
gedit_file_save_as (Document *doc)
{

	gedit_debug("", DEBUG_FILE);

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

		/*
		g_print ("1. Doc->filename %s untitled #%i\n", doc->filename, doc->untitled_number);
		*/
		/* OK clicked */
		gtk_signal_connect (GTK_OBJECT(GTK_FILE_SELECTION(save_file_selector)->ok_button),
				    "clicked",
				    GTK_SIGNAL_FUNC (gedit_file_save_as_ok_sel),
				    doc);

	}

	gtk_window_set_title (GTK_WINDOW(save_file_selector), _("Save As..."));

	if (!GTK_WIDGET_VISIBLE (save_file_selector))
	{
		gtk_window_position (GTK_WINDOW (save_file_selector), GTK_WIN_POS_MOUSE);
		gtk_widget_show(save_file_selector);
	}
	return;
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
gedit_file_stdin (Document *doc)
{
	gchar *tmp_buf;
	struct stat stats;
	guint buffer_length;
	guint pos = 0;
	View *view;

	gedit_debug ("start", DEBUG_FILE);

	fstat(STDIN_FILENO, &stats);
	
	if (stats.st_size  == 0)
	{
		gedit_debug ("size = 0. end", DEBUG_FILE);
		return 1;
	}
	
	if ((tmp_buf = g_new0 (gchar, GEDIT_STDIN_BUFSIZE+1)) == NULL)
	{
		gnome_app_error (mdi->active_window, _("Could not allocate the required memory."));
		gedit_debug ("mem alloc error. end", DEBUG_FILE);
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
			gnome_app_error (mdi->active_window, _("Error in the pipe ."));
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
	Document *doc;

	gedit_debug("", DEBUG_FILE);
	
	gedit_flash (_(MSGBAR_FILE_NEW));

	gedit_close_all_flag_verify ("file_new_cb");

	doc = gedit_document_new ();
}

void
file_open_cb (GtkWidget *widget, gpointer cbdata)
{

	gedit_debug("", DEBUG_FILE);

	gedit_close_all_flag_verify ("file_open_cb");

	if (open_file_selector && GTK_WIDGET_VISIBLE (open_file_selector))
		return;

	if (open_file_selector == NULL)
	{
		open_file_selector = gtk_file_selection_new(NULL);
		gtk_signal_connect(GTK_OBJECT(open_file_selector), "delete_event",
				   GTK_SIGNAL_FUNC(delete_event_cb), open_file_selector);
		gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(open_file_selector)->ok_button),
				   "clicked", GTK_SIGNAL_FUNC(gedit_file_open_ok_sel), open_file_selector);
		gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(open_file_selector)->cancel_button),
				   "clicked", GTK_SIGNAL_FUNC(cancel_cb), open_file_selector);
	}

	gtk_window_set_title (GTK_WINDOW(open_file_selector), _("Open File ..."));

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
	Document *doc;
	
	gedit_debug("", DEBUG_FILE);

	gedit_close_all_flag_verify ("file_save_as_cb");
	
	doc = gedit_document_current();
		
	if (doc==NULL)
		return;

	gedit_file_save_as (doc);
}


static gint
delete_event_cb (GtkWidget *widget, GdkEventAny *event)
{
	gedit_debug("", DEBUG_FILE);
	
	gtk_widget_hide (widget);
	return TRUE;
}
 
static void
cancel_cb (GtkWidget *w, gpointer data)
{
	gedit_debug("", DEBUG_FILE);
	
	gtk_widget_hide (data);
}

static void
gedit_file_open_ok_sel (GtkWidget *widget, GtkFileSelection *files)
{
	guchar * file_name;
	
	gedit_debug("", DEBUG_FILE);

	file_name = g_strdup(gtk_file_selection_get_filename (GTK_FILE_SELECTION(open_file_selector)));

	if (gedit_document_new_with_file (file_name))
	{
		gedit_flash_va (_("Loaded file %s"), file_name);
	}

	g_free (file_name);
	gtk_widget_hide (GTK_WIDGET(open_file_selector));
	return;
}

gint
file_save_document (Document * doc)
{
	gedit_debug("", DEBUG_FILE);

	if (doc==NULL)
		return FALSE;

	if (doc->filename == NULL)
	{
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
	Document *doc;
	
	gedit_debug("", DEBUG_FILE);

	gedit_close_all_flag_verify ("file_save_cb");

	doc = gedit_document_current ();

	if (doc==NULL)
		return;

	file_save_document (doc);
}


void 
file_save_all_cb (GtkWidget *widget, gpointer cbdata)
{
	int i;
	Document *doc;

	gedit_debug ("", DEBUG_FILE);
	
        for (i = 0; i < g_list_length (mdi->children); i++)
	{
		doc = (Document *)g_list_nth_data (mdi->children, i);
		gedit_file_save (doc, NULL);
	}
}


static void
gedit_file_save_as_ok_sel (GtkWidget *w, gpointer cbdata)
{
	Document *doc;
	gchar *file_name;
	gint i;
	View *nth_view;

	gedit_debug("", DEBUG_FILE);

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
	    
	if (gedit_file_save(doc, file_name) != 0)
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
	gedit_recent_update (gedit_window_active_app());

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
}


void
file_close_cb (GtkWidget *widget, gpointer cbdata)
{

	gedit_debug("start", DEBUG_FILE);

	if (mdi->active_child == NULL)
		return;
	gnome_mdi_remove_child (mdi, mdi->active_child, FALSE);

	gedit_document_set_title (gedit_document_current());

	gedit_window_set_widgets_sensitivity (FALSE);
}

void
file_close_all_cb (GtkWidget *widget, gpointer cbdata)
{
	gedit_debug("start", DEBUG_FILE);

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
	
	gedit_debug("end", DEBUG_FILE);
}

void
file_quit_cb (GtkWidget *widget, gpointer cbdata)
{
	gedit_debug("start", DEBUG_FILE);

	gedit_close_all_flag = GEDIT_CLOSE_ALL_FLAG_QUIT;
	file_close_all_cb (NULL, NULL);

	gedit_debug("1", DEBUG_FILE);
	
	if (gedit_document_current()!=NULL)
		return;

	gedit_debug("2", DEBUG_FILE);

	/* We need to disconnect the signal because mdi "destroy" event
	   is connected to file_quit_cb ( i.e. this function ). Chema */
	gtk_signal_disconnect (GTK_OBJECT(mdi), gedit_mdi_destroy_signal);
	
	gedit_debug("3", DEBUG_FILE);
	gtk_object_destroy (GTK_OBJECT (mdi));

	gedit_debug("3", DEBUG_FILE);
	gedit_prefs_save_settings ();
	gedit_recent_history_write_config ();

	gtk_main_quit ();
}

void
file_revert_cb (GtkWidget *widget, gpointer data)
{
	GtkWidget *msgbox;
	gchar * msg;
	Document *doc = gedit_document_current ();

	gedit_debug("", DEBUG_FILE);

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
		gedit_file_open (doc, doc->filename);
		break;
	default:
		break;
	}
	g_free (msg);
}


gint 
gedit_file_create_popup (guchar *title)
{
	GtkWidget *msgbox;
	Document *doc;
	int ret;
	char *msg;

	gedit_debug("", DEBUG_FILE);
	
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
	case 0 : 
		doc = gedit_document_new_with_title (title);
		doc->changed = FALSE;
		return TRUE;
	case 1 : 
		return FALSE;
	}

	g_assert_not_reached ();
	return FALSE;
}

void 
gedit_close_all_flag_clear (void)
{
	gedit_debug("", DEBUG_FILE);

	gedit_close_all_flag = GEDIT_CLOSE_ALL_FLAG_NORMAL;
	
}

static void
gedit_close_all_flag_status (guchar * function)
{
	gedit_debug("", DEBUG_FILE);

	g_return_if_fail (function!=NULL);

	return;
	
	g_print("Close all flag state is : ");
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
	gedit_debug("", DEBUG_FILE);

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

