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

/**
 * gedit_file_open:
 * @doc: Document window to fill with text
 * @fname: Filename to open
 *
 * Open a file and read it into the text widget.
 *
 * Return value: 0 on success, 1 on error.
 */
/* TODO: lock/unlock file before/after */
gint
gedit_file_open (gedit_document *doc, gchar *fname)
{
	gchar *tmp_buf, *flash;
	struct stat stats;
	gint i;
	gedit_view *nth_view;
	FILE *fp;

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
	FILE *fp;
	gchar *tmpstr;
	gedit_view *view = GE_VIEW (mdi->active_view);

	gedit_debug_mess ("F:Entering gedit_file_save.\n", DEBUG_FILE);
	
	/* FIXME: not sure what to do with all the gedit_window refs.. 
	          i'll comment them out for now... Who are you Jason ?*/

	/* sanity checks */
	g_return_val_if_fail (doc != NULL, 1);
	g_return_val_if_fail (fname != NULL, 1);

	if ((fp = fopen (fname, "w")) == NULL)
	{
/*		g_warning ("Can't open file %s for saving", fname);*/
		gchar *errstr = g_strdup_printf (_("gedit was unable to save the file: "
						   "\n\n %s \n\n"
						   "Make sure that the path you provided exits,"
						   "and that you have the appropiate write permissions."), fname);
		gnome_app_error (mdi->active_window, errstr);
		return 1;
	}
	
	tmpstr = gtk_editable_get_chars (GTK_EDITABLE (view->text), 0,
					 gtk_text_get_length (GTK_TEXT (view->text)));


	if (fputs (tmpstr, fp) == EOF)
	{
		perror ("Error saving file");
		fclose (fp);
		g_free (tmpstr);
	  
		return 1;
	}
	
	g_free (tmpstr);
	
	if (fclose (fp) != 0)
	{
		perror ("Error saving file");
	  
		return 1;
	}

	if (doc->filename != fname)
	{
		g_free (doc->filename);
		doc->filename = g_strdup (fname);
	}
	
	doc->changed = FALSE;
	
	gnome_mdi_child_set_name (GNOME_MDI_CHILD (doc),
				  g_basename (doc->filename));

	/* Set the title to indicate the document is no longer modified */
	gedit_set_title (doc);

	if (!view->changed_id)
		view->changed_id = gtk_signal_connect (GTK_OBJECT(view->text), "changed",
						       GTK_SIGNAL_FUNC(view_changed_cb), view);

	gedit_flash (_(MSGBAR_FILE_SAVED));
	
	return 0;
}








































 







/* rewriten functions . Chema  ..*/ 
void file_new_cb (GtkWidget *widget, gpointer cbdata);
void file_open_cb (GtkWidget *widget, gpointer cbdata);
void file_save_cb (GtkWidget *widget);

static gint delete_event_cb (GtkWidget *w, GdkEventAny *e);
static void cancel_cb(GtkWidget *w, GtkWidget *me);
static void file_open_ok_sel (GtkWidget *widget, GtkFileSelection *files);

/* Not yet rewriten functions . Chema  ..*/ 
void file_save_as_cb (GtkWidget *widget, gpointer cbdata);
void file_save_all_cb (GtkWidget *widget, gpointer cbdata);
void file_save_all_as_cb (GtkWidget *widget, gpointer cbdata);

void file_close_cb(GtkWidget *widget, gpointer cbdata);
void file_close_all_cb (GtkWidget *widget, gpointer cbdata);




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

	if (open_file_selector && GTK_WIDGET_VISIBLE( open_file_selector))
		return;

	if (open_file_selector == NULL)
		open_file_selector = gtk_file_selection_new(NULL);

	open_file_selector = gtk_file_selection_new(_("Open File..."));
	gtk_signal_connect(GTK_OBJECT(open_file_selector), "delete_event",
			   GTK_SIGNAL_FUNC(delete_event_cb), open_file_selector);
	gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(open_file_selector)->ok_button),
			   "clicked", GTK_SIGNAL_FUNC(file_open_ok_sel), open_file_selector);
	gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(open_file_selector)->cancel_button),
			   "clicked", GTK_SIGNAL_FUNC(cancel_cb), open_file_selector);

	if (!GTK_WIDGET_VISIBLE (open_file_selector))
	{
		gtk_window_position (GTK_WINDOW (open_file_selector), GTK_WIN_POS_MOUSE);
		gtk_widget_show(open_file_selector);
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
	return;
}

void
file_save_cb_NEW (GtkWidget *widget)
{
	gedit_document *doc;

	if( gnome_mdi_get_active_child(mdi) == NULL)
		return;

/*	doc = GEDIT_DOCUMENT( gnome_mdi_get_active_child(mdi));*/
}


void
file_save_cb (GtkWidget *widget)
{
	gchar *fname;
	gedit_view *view;
	gchar *title;
	gedit_document *doc;

	gedit_debug_mess ("F:Entering file_save_cb.\n", DEBUG_FILE);
	
	if (gedit_document_current())
	{
		doc = gedit_document_current();
		view = GE_VIEW (mdi->active_view);
	    
		if (doc->changed)
		{
			fname = g_strdup (doc->filename);

			if (fname == NULL)
			{
				title = g_strdup_printf ("%s", GNOME_MDI_CHILD(doc)->name);
				file_save_as_cb (widget, title);
				g_free (title);
			}
			else if ((gedit_file_save(doc, doc->filename)) != 0)
			{
				gedit_flash (_("Read only file!"));
				file_save_as_cb (widget, NULL);
			}
			g_free (fname);
		}
	}            
}

/*
 * file save-all callback : user selects "Save All"
 *
 * saves all open and changed files
 */
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
		if (doc->changed)
		{
			fname = g_strdup(doc->filename);
			if (fname == NULL)
			{
				title = g_strdup_printf ("%s", GNOME_MDI_CHILD(doc)->name);
				/*gtk_label_get((GtkLabel *)doc->tab_label, &title);*/

				file_save_all_as_cb(widget, title);
				g_free (title);
			}
			else
			{
				if ((gedit_file_save(doc, doc->filename)) != 0)
				{
					gedit_flash (_("Read only file!"));
					file_save_all_as_cb(widget, NULL);
				}
				g_free (fname);
			}
		}
        }
}

/*
 * file save-as callback : user selects "Ok"
 *
 * data->temp1 must be the file saveas dialog box
 */
static void
file_saveas_ok_sel (GtkWidget *w, gedit_data *data)
{
	gchar *fname = g_strdup(gtk_file_selection_get_filename (GTK_FILE_SELECTION(save_file_selector)));
	gedit_document *doc;

	gedit_debug_mess ("F:Entering file_saveas_ok_sel.\n", DEBUG_FILE);

	if (mdi->active_child == NULL)
	{
		g_free (fname);
		return;
	}		  	
	
	doc = gedit_document_current();
	
	if (fname)
	{
		if (gedit_file_save(doc, fname) != 0) 
			gedit_flash (_("Error saving file!"));
	}

	g_free (fname);
	gtk_widget_destroy (GTK_WIDGET (save_file_selector));
	save_file_selector = NULL;
}

/*
 * destroy the "save as" dialog box
 */
static gint
file_saveas_destroy (GtkWidget *w, GtkWidget **sel)
{
	gedit_debug_mess ("F:Entering file_saveas_destroy.\n", DEBUG_FILE);

	gtk_widget_destroy(*sel);
	*sel = NULL;
	
	return TRUE;
}

void
file_save_as_cb (GtkWidget *widget, gpointer cbdata)
{
	gchar *title;

	gedit_debug_mess ("F:Entering file_save_as_cb.\n", DEBUG_FILE);
	title = g_strdup_printf (_("Save %s As..."), (gchar*) cbdata);
	save_file_selector = gtk_file_selection_new (title);
	gtk_signal_connect (GTK_OBJECT(GTK_FILE_SELECTION(save_file_selector)->ok_button),
			    "clicked",
			    GTK_SIGNAL_FUNC (file_saveas_ok_sel),
			    NULL);
	gtk_signal_connect (GTK_OBJECT(GTK_FILE_SELECTION(save_file_selector)->cancel_button),
			    "clicked",
			    GTK_SIGNAL_FUNC (file_saveas_destroy),
			    &save_file_selector);
	gtk_widget_show (save_file_selector);
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

	gedit_debug_mess ("F:Entering file_save_as_ok_sel.\n", DEBUG_FILE);
	
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

/*
 * destroy the "save all as" dialog box
 */
static gint
file_save_all_as_destroy (GtkWidget *w, GtkFileSelection *fs)
{
	gedit_debug_mess ("F:Entering file_save_all_as_destroy.\n", DEBUG_FILE);
	gtk_widget_destroy (GTK_WIDGET (fs)); 
	fs = NULL;
	
	return TRUE;
}

/*
 * save all as callback
 */
void 
file_save_all_as_cb (GtkWidget *widget, gpointer cbdata)
{
	GtkWidget *fs = NULL; 
	gchar *title;

	gedit_debug_mess ("F:Entering file_save_all_as_cb.\n", DEBUG_FILE);
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
file_close_cb(GtkWidget *widget, gpointer cbdata)
{
	gedit_document *doc;

	gedit_debug_mess ("F:Entering file_close_cb.\n", DEBUG_FILE);
	
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
file_close_all_cb(GtkWidget *widget, gpointer cbdata)
{
	gedit_document *doc;

	gedit_debug_mess ("F:Entering file_close_all_cb.\n", DEBUG_FILE);

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
	gchar msg[80];
	gint ret;

	gedit_debug_mess ("F:Entering file_revert_do.\n", DEBUG_FILE);
	
	sprintf (msg, _("Are you sure you wish to revert all changes?\n(%s)"),
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

	gedit_debug_mess ("F:Entering file_revert_cb.\n", DEBUG_FILE);
	
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

/*
 * quits gEdit by closing all windows.  only quits if all windows closed.
 */
void
file_quit_cb (GtkWidget *widget, gpointer cbdata)
{
	gedit_debug_mess ("F:Entering file_quit_callback.\n", DEBUG_FILE);
	
	gedit_save_settings ();

	if (gnome_mdi_remove_all (mdi, FALSE))
		gtk_object_destroy (GTK_OBJECT (mdi));
	else
		return;

	gtk_main_quit ();
}


/* Defined but not used */
#if 0 

/* Maybe move this to gedit-utils.c? - JEL */
static void
clear_text (gedit_view *view)
{
	gint i = gedit_view_get_length (view);

	if (i > 0) {
		gedit_view_set_position (view, i);
		gtk_text_backward_delete (GTK_TEXT(view->text), i);
	}
}

#endif /* #if 0 */
