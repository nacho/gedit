/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* vi:set ts=8 sts=0 sw=8:
 *
 * gEdit - File Input/Output routines
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
#include <sys/stat.h> /* for stat() */

#include "commands.h"
#include "gedit.h"
#include "gE_mdi.h"
#include "gE_view.h"
#include "gE_prefs.h"

#include "gedit-window.h"
#include "gedit-file-io.h"
#include "gedit-utils.h"

GtkWidget *save_file_selector = NULL;
GtkWidget *open_file_selector = NULL;

/* not yet rewriten . Chema */
gint gedit_file_open (gedit_document *doc, gchar *fname);
gint gedit_file_save (gedit_document *doc, gchar *fname);
/* rewriten functions . Chema  ..*/ 
void file_new_cb (GtkWidget *widget, gpointer cbdata);
void file_open_cb (GtkWidget *widget, gpointer cbdata);
void file_save_cb (GtkWidget *widget);
void file_save_as_cb (GtkWidget *widget, gpointer cbdata);
void file_save_all_cb (GtkWidget *widget, gpointer cbdata);
void file_close_cb(GtkWidget *widget, gpointer cbdata);
void file_close_all_cb(GtkWidget *widget, gpointer cbdata);
void file_revert_cb (GtkWidget *widget, gpointer cbdata);
void file_quit_cb (GtkWidget *widget, gpointer cbdata);

static void file_open_ok_sel   (GtkWidget *widget, GtkFileSelection *files);
static void file_saveas_ok_sel (GtkWidget *w, gedit_data *data);
static gint delete_event_cb (GtkWidget *w, GdkEventAny *e);
static void cancel_cb (GtkWidget *w, GtkWidget *me);

/* TODO : add flash on all operations ....Chema*/

/**
 * gedit_file_open:
 * @doc: Document window to fill with text
 * @fname: Filename to open
 *
 * Open a file and read it into the text widget.
 *
 * Return value: 0 on success, 1 on error.
 */
/* TODO: lock/unlock file before/after. Jason*/
gint
gedit_file_open (gedit_document *doc, gchar *fname)
{
	gchar *tmp_buf, *flash;
	struct stat stats;
	gint i;
	gedit_view *nth_view;
	FILE *fp;

	/* FIXME : this function is beeing called
	   many times */

	/* FIXME : deal with ReadOnly files,
	   we should have a flag in gedit_document
	   to know if it is readonly */
	gedit_debug_mess ("F:Entering gedit_file_open ..\n", DEBUG_FILE);
	g_return_val_if_fail (fname != NULL, 1);
/*	g_return_val_if_fail (doc != NULL, 1); */

	if (!stat(fname, &stats) && S_ISREG(stats.st_mode))
	{
		doc->buf_size = stats.st_size;
		if ((tmp_buf = g_new0 (gchar, doc->buf_size + 1)) != NULL)
		{
			if ((fp = fopen (fname, "r")) != NULL)
			{
				doc->buf_size = fread (tmp_buf, 1, doc->buf_size, fp);
				doc->buf = g_string_new (tmp_buf);
				g_free (tmp_buf);

				gnome_mdi_child_set_name (GNOME_MDI_CHILD(doc),
							  g_basename (fname));

				fclose (fp);
				doc->filename = g_strdup (fname);

				for (i = 0; i < g_list_length (doc->views); i++)
				{
					nth_view = g_list_nth_data (doc->views, i);
					gedit_view_refresh (nth_view);
					gedit_set_title (nth_view->document);

					/* Make the document readonly if you can't write to the file. */
					gedit_view_set_read_only (nth_view, access (fname, W_OK) != 0);
					if (!nth_view->changed_id)
						nth_view->changed_id = gtk_signal_connect (GTK_OBJECT(nth_view->text), "changed",
											   GTK_SIGNAL_FUNC(view_changed_cb), nth_view);
				}

				gedit_flash_va ("%s %s", _(MSGBAR_FILE_OPENED), fname); 

				/* update the recent files menu */
				recent_add (fname);
				recent_update (GNOME_APP (mdi->active_window));
				return 0;
			}
			else
			{
				gchar *errstr = g_strdup_printf (_("gedit was unable to open the file: "
								   "\n\n%s\n\n"
								   "Make sure that you have read access permissions for the file."), fname);
				gnome_app_error (mdi->active_window, errstr);
				return 0;
			}
		}
	}

	return 1;
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
gedit_file_save (gedit_document *doc, gchar *fname)
{
	/* FIXME : deal with ReadOnly files,
	   we should have a flag in gedit_document
	   to know if it is readonly, if it is
	   just return or display a mesage */
	FILE *fp;
	gchar *tmpstr;
	gedit_view *view = GE_VIEW ( g_list_nth_data(doc->views, 0) );
	/* Since all the views contain the same data, we can grab
	   view #1. Chema */
	gedit_debug_mess ("F:Entering gedit_file_save.\n", DEBUG_FILE);

	if( fname == NULL )
		fname = doc->filename;

	/* sanity checks */
	g_return_val_if_fail (doc != NULL, 1);
	/* FIXME : We should not fail if we don't have
	   a filename we should call save as.
	   Chema */
	g_return_val_if_fail (fname != NULL, 1);
	
	if ((fp = fopen (fname, "w")) == NULL)
	{
		gchar *errstr = g_strdup_printf (_("gedit was unable to save the file: "
						   "\n\n %s \n\n"
						   "Make sure that the path you provided exits,"
						   "and that you have the appropiate write permissions."), fname);
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
	gedit_document *doc;

	gedit_flash (_(MSGBAR_FILE_NEW));
	doc = gedit_document_new ();
	
	gnome_mdi_add_child (mdi, GNOME_MDI_CHILD (doc));
	gnome_mdi_add_view (mdi, GNOME_MDI_CHILD (doc));
}

void
file_open_cb (GtkWidget *widget, gpointer cbdata)
{

	gedit_debug_mess("F:Entering file open cb\n", DEBUG_FILE);
	
	if (open_file_selector && GTK_WIDGET_VISIBLE( open_file_selector))
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

	gtk_window_set_title(GTK_WINDOW(open_file_selector), _("Open File ..."));

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

	if (save_file_selector && GTK_WIDGET_VISIBLE( save_file_selector))
		return;
	if (save_file_selector == NULL)
	{
		save_file_selector = gtk_file_selection_new(NULL);
		gtk_signal_connect(GTK_OBJECT(save_file_selector), "delete_event",
				   GTK_SIGNAL_FUNC(delete_event_cb), save_file_selector);
		gtk_signal_connect (GTK_OBJECT(GTK_FILE_SELECTION(save_file_selector)->ok_button),
				    "clicked", GTK_SIGNAL_FUNC (file_saveas_ok_sel), NULL);
		gtk_signal_connect (GTK_OBJECT(GTK_FILE_SELECTION(save_file_selector)->cancel_button),
				    "clicked", GTK_SIGNAL_FUNC(cancel_cb), save_file_selector);
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
delete_event_cb(GtkWidget *w, GdkEventAny *e)
{
	gtk_widget_hide(w);
	return TRUE;
}
 
static void
cancel_cb(GtkWidget *w, GtkWidget *me)
{
	gtk_widget_hide(me);
}

static void
file_open_ok_sel (GtkWidget *widget, GtkFileSelection *files)
{
	gedit_document *doc;
	gchar *filename;
	gchar *flash;

	gedit_debug_mess("file-io.c - file open ok sel\n", DEBUG_FILE);
	if((doc = gedit_document_new_with_file((gtk_file_selection_get_filename (GTK_FILE_SELECTION (open_file_selector))))) != NULL) {
		gnome_mdi_add_child (mdi, GNOME_MDI_CHILD (doc));
		gnome_mdi_add_view (mdi, GNOME_MDI_CHILD (doc));
		flash = g_strdup_printf(_("Loaded file %s"), doc->filename);
		gnome_app_flash(mdi->active_window, flash);
		g_free(flash);
	}
	else
		gnome_app_error(mdi->active_window, _("Can not open file!"));

	gtk_widget_hide(GTK_WIDGET(open_file_selector));
	/* FIXME : do not destroy it but
	gtk_widget_destroy(GTK_WIDGET(open_file_selector));
	open_file_selector = NULL;*/
	return;
}

void
file_save_cb (GtkWidget *widget)
{
	gedit_document *doc;

	if( gnome_mdi_get_active_child(mdi) == NULL)
		return;

	doc = gedit_document_current();

	if (doc->changed)
		gedit_file_save( doc, NULL);
}


void 
file_save_all_cb (GtkWidget *widget, gpointer cbdata)
{
	int i;
	gchar *fname, *title;
	gedit_document *doc;

	gedit_debug_mess ("F:Entering file_save_all_cb.\n", DEBUG_FILE);
	
        for (i = 0; i < g_list_length (mdi->children); i++)
	{
		doc = (gedit_document *)g_list_nth_data (mdi->children, i);
		gedit_file_save( doc, NULL);
	}
}


static void
file_saveas_ok_sel (GtkWidget *w, gedit_data *data)
{

	gedit_document *doc;
	gchar *fname = g_strdup(gtk_file_selection_get_filename (GTK_FILE_SELECTION(save_file_selector)));

	/* FIXME: we need to popup a window
	   if the file is about to be overwritten */
	doc = gedit_document_current();
	
	if (gedit_file_save(doc, fname) != 0) 
		gedit_flash (_("Error saving file!"));

	gtk_widget_hide (GTK_WIDGET (save_file_selector));
	save_file_selector = NULL;
	g_free (fname);
}


void
file_close_cb(GtkWidget *widget, gpointer cbdata)
{
	if (mdi->active_child == NULL)
		return;
	gnome_mdi_remove_child (mdi, mdi->active_child, FALSE);
}

void
file_close_all_cb(GtkWidget *widget, gpointer cbdata)
{
	if (mdi->active_child == NULL)
		return;
	gnome_mdi_remove_all (mdi, FALSE);
}

void
file_quit_cb (GtkWidget *widget, gpointer cbdata)
{
	gedit_save_settings ();

	if (gnome_mdi_remove_all (mdi, FALSE))
		gtk_object_destroy (GTK_OBJECT (mdi));
	else
		return;

	gtk_main_quit ();
}


void
file_revert_cb (GtkWidget *widget, gpointer cbdata)
{
	GtkWidget *msgbox;
	gchar * msg;
	gedit_document *doc = gedit_document_current ();

	msg = g_strdup_printf( "Are you sure you wish to revert all changes?\n(%s)",
			       doc->filename);

	msgbox = gnome_message_box_new (msg, GNOME_MESSAGE_BOX_QUESTION,
					GNOME_STOCK_BUTTON_YES,	GNOME_STOCK_BUTTON_NO, NULL);
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




