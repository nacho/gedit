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
 *
 * Authors:
 *   Chema Celorio <chema@celorio.com>
 */

#include <config.h>
#include <gnome.h>
#include <sys/stat.h> /* for stat() */

#include "gedit.h"
#include "commands.h"
#include "document.h"
#include "view.h" 
#include "prefs.h"
#include "file.h"
#include "utils.h"
#include "recent.h"

GtkWidget *save_file_selector = NULL;
GtkWidget *open_file_selector = NULL;

gint gedit_file_open (Document *doc, gchar *fname);
gint gedit_file_save (Document *doc, gchar *fname);
void file_new_cb (GtkWidget *widget, gpointer cbdata);
void file_open_cb (GtkWidget *widget, gpointer cbdata);
void file_save_cb (GtkWidget *widget);
void file_save_as_cb (GtkWidget *widget, gpointer cbdata);
void file_save_all_cb (GtkWidget *widget, gpointer cbdata);
void file_close_cb(GtkWidget *widget, gpointer cbdata);
void file_close_all_cb(GtkWidget *widget, gpointer cbdata);
void file_revert_cb (GtkWidget *widget, gpointer cbdata);
void file_quit_cb (GtkWidget *widget, gpointer cbdata);

   gboolean popup_create_new_file (GtkWidget *w, gchar *title);
static void file_open_ok_sel   (GtkWidget *widget, GtkFileSelection *files);
static void file_saveas_ok_sel (GtkWidget *w, gedit_data *data);
static gint delete_event_cb (GtkWidget *w, GdkEventAny *e);
static void cancel_cb (GtkWidget *w, gpointer data);

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
	gint i;
	View *nth_view;
	
	gedit_debug ("F:gedit_file_open.\n", DEBUG_FILE);
	g_return_val_if_fail (fname != NULL, 1);
	g_return_val_if_fail (doc != NULL, 1);

	currentdoc = gedit_document_current();

	if (stat(fname, &stats) ||  !S_ISREG(stats.st_mode))
	{
		gnome_app_error (mdi->active_window, _("An error was encountered while opening the file."
						       "\nPlease make sure the file exists."));
		return 1;
	}

	if (stats.st_size  == 0)
	{
		gchar *errstr = g_strdup_printf (_("An error was encountered while opening the file:\n\n%s\n\n"
						    "\nPlease make sure the file is not being used by another application\n"
						    "and that the file is not empty."), fname);
		gnome_app_error (mdi->active_window, errstr);
		g_free (errstr);
		return 1;
	}
	
	doc->buf_size = stats.st_size;

	if ((tmp_buf = g_new0 (gchar, doc->buf_size + 1)) == NULL)
	{
		gnome_app_error (mdi->active_window, _("Could not allocate the required memory."));
		return 1;
	}

	if ((fp = fopen (fname, "r")) == NULL)
	{
		gchar *errstr = g_strdup_printf (_("gedit was unable to open the file: \n\n%s\n\n"
						   "Make sure that you have permissions for opening the file."), fname);
		gnome_app_error (mdi->active_window, errstr);
		g_free (errstr);
		return 1;
	}

	if (currentdoc != NULL && currentdoc->filename == NULL && !currentdoc->changed )
	{
		gnome_mdi_remove_child (mdi, mdi->active_child, FALSE);
	}

	doc->buf_size = fread (tmp_buf, 1, doc->buf_size, fp);
	doc->buf = g_string_new (tmp_buf);
	g_free (tmp_buf);
	gnome_mdi_child_set_name (GNOME_MDI_CHILD (doc),
				  g_basename (fname));
	fclose (fp);
	
	doc->filename = g_strdup (fname);
	doc->readonly = access (fname, W_OK) ? TRUE : FALSE;

	for (i = 0; i < g_list_length (doc->views); i++) 
	{
		nth_view = g_list_nth_data (doc->views, i);
		gedit_view_refresh (nth_view);
		gedit_view_set_read_only (nth_view, access (fname, W_OK)  != 0);
		if (!nth_view->changed_id)
			nth_view->changed_id = gtk_signal_connect (GTK_OBJECT(nth_view->text), "changed",
								   GTK_SIGNAL_FUNC(view_changed_cb), nth_view);
		gedit_set_title (nth_view->document);


	}
	
	gedit_flash_va ("%s %s", _(MSGBAR_FILE_OPENED), fname);
	recent_add (fname);
	recent_update (GNOME_APP (mdi->active_window));

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
	View *view = VIEW ( g_list_nth_data(doc->views, 0) );

	gedit_debug ("F:gedit_file_save.\n", DEBUG_FILE);

	if (fname == NULL)
		fname = doc->filename;

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
		return 1;
	}
	
	tmpstr = gtk_editable_get_chars (GTK_EDITABLE (view->text), 0,
					 gtk_text_get_length (GTK_TEXT (view->text)));

	if (fputs (tmpstr, fp) == EOF)
	{
		gchar *errstr = g_strdup_printf (_("gedit was unable to save the file :"
						   "\n\n %s \n\n"
						   "Because of an unknown reason (1). Please report this "
						   "Problem to submit@bugs.gnome.org"), fname);
		gnome_app_error (mdi->active_window, errstr);
		fclose (fp);
		g_free (errstr);
		return 1;
	}
	
	g_free (tmpstr);
	
	if (fclose (fp) != 0)
	{
		gchar *errstr = g_strdup_printf (_("gedit was unable to save the file :"
						   "\n\n %s \n\n"
						   "Because of an unknown reason (1). Please report this "
						   "Problem to submit@bugs.gnome.org"), fname);
		gnome_app_error (mdi->active_window, errstr);
		fclose (fp);
		g_free (errstr);
		return 1;
	}

	if (doc->filename != fname)
	{
		g_free (doc->filename);
		doc->filename = g_strdup (fname);
	}
	
	doc->changed = FALSE;
	
	gnome_mdi_child_set_name (GNOME_MDI_CHILD (doc), g_basename (doc->filename));

	/* Set the title to indicate the document is no longer modified */
	gedit_set_title (doc);

	if (!view->changed_id)
		view->changed_id = gtk_signal_connect (GTK_OBJECT(view->text), "changed",
						       GTK_SIGNAL_FUNC(view_changed_cb), view);

	gedit_flash (_(MSGBAR_FILE_SAVED));
	return 0;
}


void
file_new_cb (GtkWidget *widget, gpointer cbdata)
{
	Document *doc;

	gedit_debug("F:file_new_cb\n", DEBUG_FILE);
	
	gedit_flash (_(MSGBAR_FILE_NEW));
	doc = gedit_document_new ();
	
	gnome_mdi_add_child (mdi, GNOME_MDI_CHILD (doc));
	gnome_mdi_add_view (mdi, GNOME_MDI_CHILD (doc));
}

void
file_open_cb (GtkWidget *widget, gpointer cbdata)
{

	gedit_debug("F:file_open_cb\n", DEBUG_FILE);
	
	if (open_file_selector && GTK_WIDGET_VISIBLE (open_file_selector))
		return;

	if (open_file_selector == NULL)
	{
		open_file_selector = gtk_file_selection_new(NULL);
		gtk_signal_connect(GTK_OBJECT(open_file_selector), "delete_event",
				   GTK_SIGNAL_FUNC(delete_event_cb), open_file_selector);
		gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(open_file_selector)->ok_button),
				   "clicked", GTK_SIGNAL_FUNC(file_open_ok_sel), open_file_selector);
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

	gedit_debug("F:file_save_as_cb\n", DEBUG_FILE);
	
	if (!gedit_document_current())
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

		gtk_signal_connect (GTK_OBJECT(GTK_FILE_SELECTION(save_file_selector)->ok_button),
				    "clicked",
				    GTK_SIGNAL_FUNC (file_saveas_ok_sel),
				    NULL);

		gtk_signal_connect (GTK_OBJECT(GTK_FILE_SELECTION(save_file_selector)->cancel_button),
				    "clicked",
				    GTK_SIGNAL_FUNC(cancel_cb),
				    save_file_selector);
	}

	gtk_window_set_title (GTK_WINDOW(save_file_selector), _("Save As..."));

	if (!GTK_WIDGET_VISIBLE (save_file_selector))
	{
		gtk_window_position (GTK_WINDOW (save_file_selector), GTK_WIN_POS_MOUSE);
		gtk_widget_show(save_file_selector);
	}
	return;
}

static gint
delete_event_cb (GtkWidget *widget, GdkEventAny *event)
{
	gedit_debug("F:(file) delete event cb\n", DEBUG_FILE);
	
	gtk_widget_hide (widget);
	return TRUE;
}
 
static void
cancel_cb (GtkWidget *w, gpointer data)
{
	gedit_debug("F:(file) cance_cb\n", DEBUG_FILE);
	
	gtk_widget_hide (data);
}

static void
file_open_ok_sel (GtkWidget *widget, GtkFileSelection *files)
{
	Document *doc;

	gedit_debug("f:file_open_ok_sel\n", DEBUG_FILE);

	if ((doc = gedit_document_new_with_file((gtk_file_selection_get_filename (GTK_FILE_SELECTION (open_file_selector))))) != NULL)
	{
		gnome_mdi_add_child (mdi, GNOME_MDI_CHILD (doc));
		gnome_mdi_add_view (mdi, GNOME_MDI_CHILD (doc));
		gedit_flash_va (_("Loaded file %s"), doc->filename);
	}

	gtk_widget_hide (GTK_WIDGET(open_file_selector));
	return;
}

void
file_save_cb (GtkWidget *widget)
{
	Document *doc;

	gedit_debug("f:file_save_cb\n", DEBUG_FILE);

	if (gnome_mdi_get_active_child(mdi) == NULL)
		return;

	doc = gedit_document_current ();
	/* if (doc->changed) */
	/* We want to save even if the doc has not been modified
	   since the doc on disk might have been modified externally
	   and the user wants to overwrite it. Chema */
	if (doc->filename == NULL)
		file_save_as_cb (widget, NULL);
	else
		gedit_file_save (doc, NULL);
}


void 
file_save_all_cb (GtkWidget *widget, gpointer cbdata)
{
	int i;
	Document *doc;

	gedit_debug ("F:file_save_all_cb.\n", DEBUG_FILE);
	
        for (i = 0; i < g_list_length (mdi->children); i++)
	{
		doc = (Document *)g_list_nth_data (mdi->children, i);
		gedit_file_save( doc, NULL);
	}
}


static void
file_saveas_ok_sel (GtkWidget *w, gedit_data *data)
{

	Document *doc;
	gchar *fname = g_strdup(gtk_file_selection_get_filename (GTK_FILE_SELECTION(save_file_selector)));

	gedit_debug("f:file_saveas_ok_sel\n", DEBUG_FILE);
	
	doc = gedit_document_current();
	if (!doc)
		return;
	
	gtk_widget_hide (GTK_WIDGET (save_file_selector));
	save_file_selector = NULL;
	
        if (g_file_exists (fname))
	{
		guchar * msg;
		GtkWidget *msgbox;
		gint ret;
		msg = g_strdup_printf (_("``%s'' is about to be overwritten. Do you want to continue ?"), fname);
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
			gedit_flash (_("Document has not been saved."));
			return;
		default:
			return;
		}
	}
	    
	if (gedit_file_save(doc, fname) != 0) 
		gedit_flash (_("Error saving file!"));

	g_free (fname);
}


void
file_close_cb (GtkWidget *widget, gpointer cbdata)
{

	gedit_debug("f:file_close_cb\n", DEBUG_FILE);
	
	if (mdi->active_child == NULL)
		return;
	gnome_mdi_remove_child (mdi, mdi->active_child, FALSE);
}

void
file_close_all_cb (GtkWidget *widget, gpointer cbdata)
{
	gedit_debug("f:file_close_all\n", DEBUG_FILE);
	
	if (mdi->active_child == NULL)
		return;
	gnome_mdi_remove_all (mdi, FALSE);
}

void
file_quit_cb (GtkWidget *widget, gpointer cbdata)
{
	gedit_debug("f:file_quit_cb\n", DEBUG_FILE);
	
	gedit_save_settings ();

	if (gnome_mdi_remove_all (mdi, FALSE))
		gtk_object_destroy (GTK_OBJECT (mdi));
	else
		return;

	gedit_shutdown ();
}

void
file_revert_cb (GtkWidget *widget, gpointer data)
{
	GtkWidget *msgbox;
	gchar * msg;
	Document *doc = gedit_document_current ();

	gedit_debug("f:file_revert_cb\n", DEBUG_FILE);
	
	if (!doc)
		return;
	
	msg = g_strdup_printf (_("Are you sure you wish to revert all changes?\n(%s)"),
			       doc->filename);
	msgbox = gnome_message_box_new (msg, GNOME_MESSAGE_BOX_QUESTION, GNOME_STOCK_BUTTON_YES,
					GNOME_STOCK_BUTTON_NO, NULL);
	gnome_dialog_set_default (GNOME_DIALOG (msgbox), 2);
	    	    
	switch (gnome_dialog_run_and_close (GNOME_DIALOG (msgbox)))
	{
	case 0:
		gedit_file_open (doc, doc->filename);
		break;
	default:
		break;
	}
	g_free (msg);
}


gboolean
popup_create_new_file (GtkWidget *w, gchar *title)
{
	GtkWidget *msgbox;
	Document *doc;
	int ret;
	char *msg;

	gedit_debug("f:popup_create_new_file\n", DEBUG_FILE);
	
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
		gnome_mdi_add_child (mdi, GNOME_MDI_CHILD (doc));
		gnome_mdi_add_view (mdi, GNOME_MDI_CHILD (doc));
		/* gedit_file_save (doc, title); Why save it ? Chema */
		doc->filename = g_strdup (title);
		doc->changed = FALSE;
		return TRUE;
	case 1 : 
		return FALSE;
	}

	g_assert_not_reached ();
	return FALSE;
}
	
















