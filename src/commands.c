/* vi:set ts=4 sts=0 sw=4:
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
#include <assert.h>
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#ifndef WITHOUT_GNOME
#include <config.h>
#include <gnome.h>
#include <libgnome/gnome-history.h>
#endif

#include "main.h"
#include "commands.h"
#include "menus.h"
#include "gE_document.h"
#include "gE_prefs.h"
#include "gE_files.h"
#include "msgbox.h"

/* ---- Misc callbacks ---- */

static void close_file_save_yes_sel (GtkWidget *w, gE_data *data);
static void close_file_save_cancel_sel(GtkWidget *w, gE_data *data);
static void close_file_save_no_sel(GtkWidget *w, gE_data *data);
static void popup_close_verify (gE_document *doc, gE_data *data);
static void close_doc_common(gE_data *data);
static void close_doc_execute(GtkWidget *widget, gpointer cbdata);
static void close_window_common(gE_window *w);
static void file_saveas_destroy(GtkWidget *w, gpointer cbdata);
static void file_cancel_sel (GtkWidget *w, GtkFileSelection *fs);
static void file_sel_destroy (GtkWidget *w, GtkFileSelection *fs);
static void seek_to_line (gE_document *doc, gint line_number);
static int point_to_line (gE_document *doc, gint point);
static void search_create (gE_window *, gE_search *options, gint replace);


/* handles changes in the text widget... */
void doc_changed_callback (GtkWidget *w, gpointer cbdata)
{
	gE_document *doc = (gE_document *) cbdata;

	doc->changed = TRUE;
	gtk_signal_disconnect (GTK_OBJECT(doc->text), (gint) doc->changed_id);
	doc->changed_id = FALSE;
}


/*
 * file save callback : user selected "No"
 */
static void
close_file_save_no_sel(GtkWidget *w, gE_data *data)
{
	assert(data != NULL);
	close_doc_execute(w, data);
	gtk_widget_destroy(data->temp1);	/* verify dialog box */
	data->temp1 = NULL;
	data->temp2 = NULL;
	data->flag = TRUE;
} /* close_file_save_no_sel */


/*
 * file save callback : user selected "Yes"
 */
static void
close_file_save_yes_sel(GtkWidget *w, gE_data *data)
{
	gE_document *doc;

	assert(data != NULL);
	doc = data->document;

	if (doc->filename == NULL) {
		gtk_widget_destroy(data->temp1);	/* del verify box */
		data->temp1 = NULL;
		file_save_as_cmd_callback(w, data);
		if (data->flag == TRUE) /* close document if successful */
			close_doc_execute(NULL, data);
	} else {
		int error;

		error = gE_file_save(data->window, doc, doc->filename);
		if (!error) {
			gtk_widget_destroy(data->temp1);	/* verify box */
			data->temp1 = NULL;
			close_doc_execute(NULL, data);
			data->temp2 = NULL;
			data->flag = TRUE;
		} else
			data->flag = FALSE;
	}
} /* close_file_save_yes_sel */


/*
 * file save callback : user selected "Cancel"
 */
static void
close_file_save_cancel_sel(GtkWidget *w, gE_data *data)
{
	assert(data != NULL);
	gtk_widget_destroy((GtkWidget *)(data->temp1));	/* verify dialog box */
	data->temp1 = NULL;
	data->temp2 = NULL;
	data->flag = FALSE;
} /* close_file_save_cancel_sel */


/*
 * creates file save (yes/no/cancel) dialog box
 *
 * returns TRUE if file was closed
 */
static void
popup_close_verify(gE_document *doc, gE_data *data)
{
	GtkWidget *verify, *yes, *no, *cancel, *label;
	char buf[STRING_LENGTH_MAX];
	char *fname;

	fname = (doc->filename) ? basename(doc->filename) : UNTITLED;
	
	verify = gtk_dialog_new();
	
	g_snprintf(buf, STRING_LENGTH_MAX, "Save File '%s'?", fname);
	gtk_window_set_title(GTK_WINDOW(verify), buf);

	g_snprintf(buf, STRING_LENGTH_MAX,
		" '%s' has been modified.  Do you wish to save it? ", fname);

	label = gtk_label_new(buf);
#ifdef WITHOUT_GNOME
	yes = gtk_button_new_with_label ("Yes");
	no = gtk_button_new_with_label ("No");
	cancel = gtk_button_new_with_label ("Cancel");
#else
	/* This whole code could be enhanced by using gnome_messagebox */
	yes    = gnome_stock_button (GNOME_STOCK_BUTTON_YES);
	no     = gnome_stock_button (GNOME_STOCK_BUTTON_NO);
	cancel = gnome_stock_button (GNOME_STOCK_BUTTON_CANCEL);
#endif
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(verify)->vbox),
		label, TRUE, FALSE, 10);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(verify)->action_area),
		yes, TRUE, TRUE, 2);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(verify)->action_area),
		no, TRUE, TRUE, 2);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(verify)->action_area),
		cancel, TRUE, TRUE, 2);
	gtk_widget_show(label);
	gtk_widget_show(yes);
	gtk_widget_show(no);
	gtk_widget_show(cancel);
	gtk_widget_show(verify);

	/* use data->flag to indicate whether or not to quit */
	data->flag = FALSE;
	data->temp1 = verify;
	gtk_signal_connect(GTK_OBJECT(yes), "clicked",
		GTK_SIGNAL_FUNC(close_file_save_yes_sel), data);
	gtk_signal_connect(GTK_OBJECT(no), "clicked",
		GTK_SIGNAL_FUNC(close_file_save_no_sel), data);
	gtk_signal_connect(GTK_OBJECT(cancel), "clicked",
		GTK_SIGNAL_FUNC(close_file_save_cancel_sel), data);

	gtk_grab_add(verify);

	data->document = doc;

	/* loop around doing nothing until user has decided on file save */
	while (data->temp1 != NULL) {
		gtk_main_iteration_do(TRUE);
		if (data->flag == TRUE)
			break;
	}
} /* popup_close_verify */


/*
 * file open callback : user selects "Ok"
 */
void
file_open_ok_sel(GtkWidget *widget, gE_data *data)
{
	char *filename;
	char *nfile;
	struct stat sb;
	gE_window *w;
	gE_document *curdoc;
	GtkFileSelection *fs;


	assert(data != NULL);
	w = data->window;
	assert(w != NULL);
	fs = GTK_FILE_SELECTION(w->open_fileselector);

	filename = gtk_file_selection_get_filename(fs);

	if (filename != NULL) {
		if (stat(filename, &sb) == -1)
			return;

		if (S_ISDIR(sb.st_mode)) {
			nfile = g_malloc0(strlen (filename) + 3);
			sprintf(nfile, "%s/.", filename);
			gtk_file_selection_set_filename(GTK_FILE_SELECTION(
				w->open_fileselector), nfile);
			g_free(nfile);
			return;
		}

		curdoc = gE_document_current(w);
		if (curdoc->filename || curdoc->changed)
			gE_document_new(data->window);

		nfile = g_strdup(filename);
		gE_file_open(data->window, gE_document_current(w), nfile);
	}
	if (GTK_WIDGET_VISIBLE(fs))
		gtk_widget_hide (GTK_WIDGET(fs));
} /* file_open_ok_sel */

/*
 * file save-as callback : user selects "Ok"
 *
 * data->temp1 must be the file saveas dialog box
 */
void
file_saveas_ok_sel(GtkWidget *w, gE_data *data)
{
	char *fname;
	GtkWidget *safs;

	assert(data != NULL);
	safs = (GtkWidget *)(data->temp1);
	assert(safs != NULL);

	fname = gtk_file_selection_get_filename(GTK_FILE_SELECTION(safs));
	if (fname != NULL) {
		if (gE_file_save(data->window,
			gE_document_current(data->window), fname) == 0) {

			gtk_widget_destroy(data->temp1);
			data->temp1 = NULL;
			data->temp2 = NULL;
			data->flag = TRUE;	/* indicate saved */
		} else
			data->flag = FALSE;	/* indicate not saved */
	}
} /* file_saveas_ok_sel */


/*
 * file open callback : user selects "Cancel"
 */
static void
file_cancel_sel (GtkWidget *w, GtkFileSelection *fs)
{
  if (GTK_WIDGET_VISIBLE(fs))
    gtk_widget_hide (GTK_WIDGET(fs));
}


/*
 * file selection dialog callback
 */
static void
file_sel_destroy (GtkWidget *w, GtkFileSelection *fs)
{
	fs = NULL;
}


void prefs_callback (GtkWidget *widget, gpointer cbwindow)
{
	prefs_window = gE_prefs_window((gE_window *)cbwindow);
}

/* --- Notebook Tab Stuff --- */

void tab_top_cback (GtkWidget *widget, gpointer cbwindow)
{
	gE_window *window = (gE_window *)cbwindow;

	gtk_notebook_set_tab_pos (GTK_NOTEBOOK(window->notebook), GTK_POS_TOP);
	window->tab_pos = GTK_POS_TOP;
}


void tab_bot_cback (GtkWidget *widget, gpointer cbwindow)
{
	gE_window *window = (gE_window *)cbwindow;

	gtk_notebook_set_tab_pos (GTK_NOTEBOOK(window->notebook), GTK_POS_BOTTOM);
	window->tab_pos = GTK_POS_BOTTOM;
}

void tab_lef_cback (GtkWidget *widget, gpointer cbwindow)
{
	gE_window *window = (gE_window *)cbwindow;

	gtk_notebook_set_tab_pos (GTK_NOTEBOOK(window->notebook), GTK_POS_LEFT);
	window->tab_pos = GTK_POS_LEFT;
}

void tab_rgt_cback (GtkWidget *widget, gpointer cbwindow)
{
	gE_window *window = (gE_window *)cbwindow;

	gtk_notebook_set_tab_pos (GTK_NOTEBOOK(window->notebook), GTK_POS_RIGHT);
	window->tab_pos = GTK_POS_RIGHT;
}

void tab_toggle_cback (GtkWidget *widget, gpointer cbwindow)
{
	gE_window *window = (gE_window *)cbwindow;

	window->show_tabs = !window->show_tabs;
	gtk_notebook_set_show_tabs (GTK_NOTEBOOK (window->notebook), window->show_tabs);
}



/* ---- Auto-indent Callback(s) --- */

void auto_indent_toggle_callback (GtkWidget *w, gpointer cbdata)
{
	gE_data *data = (gE_data *)cbdata;

	data->window->auto_indent = !data->window->auto_indent;
}

void auto_indent_callback (GtkWidget *text, GdkEventKey *event, gE_window *window)
{
	int i, newlines, newline_1 = 0;
	gchar *buffer, *whitespace;
	gE_data *data;
	
	data = g_malloc0 (sizeof (gE_data));
	data->temp2 = text;
	data->window = window;
	line_pos_callback (NULL, data);

	if (event->keyval != GDK_Return)
		return;
	if (gtk_text_get_length (GTK_TEXT (text)) <=1)
		return;
	if (!data->window->auto_indent)
		return;

	newlines = 0;
	for (i = GTK_EDITABLE (text)->current_pos; i > 0; i--)
	{
		buffer = gtk_editable_get_chars (GTK_EDITABLE (text), i-1, i);
		if (buffer == NULL)
			continue;
		if (buffer[0] == 10)
		{
			if (newlines > 0)
			{
				g_free (buffer);
				break;
			}
			else {
				newlines++;
				newline_1 = i;
				g_free (buffer);
			}
		}
	}

	whitespace = g_malloc0 (newline_1 - i + 2);

	for (i = i; i <= newline_1; i++)
	{
		buffer = gtk_editable_get_chars (GTK_EDITABLE (text), i, i+1);
		if ((buffer[0] != 32) & (buffer[0] != 9))
			break;
		strncat (whitespace, buffer, 1);
		g_free (buffer);
	}

	if (strlen(whitespace) > 0)
	{
		i = GTK_EDITABLE (text)->current_pos;
		gtk_editable_insert_text (GTK_EDITABLE (text), whitespace, strlen(whitespace), &i);
	}
	
	g_free (whitespace);
	data->temp2 = text;
	line_pos_callback (NULL, data); /* <-- this is so the statusbar updates when it auto-indents */
}


void line_pos_callback(GtkWidget *w, gE_data *data)
{
	static char line [32];
	static char col [32];
	GtkWidget *text = data->temp2;
	
	sprintf (line,"%d", GTK_TEXT(text)->cursor_pos_y/13);
	sprintf (col, "%d", GTK_TEXT(text)->cursor_pos_x/7);
	
	gtk_label_set (GTK_LABEL(data->window->line_label), line);
	gtk_label_set (GTK_LABEL(data->window->col_label), col);

}


void gE_event_button_press (GtkWidget *w, GdkEventButton *event, gE_window *window)
{
	gE_data *data;
	data = g_malloc0 (sizeof (gE_data));
	data->temp2 = w;
	data->window = window;
	line_pos_callback (NULL, data);
}


/* ---- File Menu Callbacks ---- */

void file_new_cmd_callback (GtkWidget *widget, gpointer cbdata)
{
	gE_data *data = (gE_data *)cbdata;
	gE_window *w;
	gE_document *doc;

	assert(data != NULL);
	w = data->window;
	assert(w != NULL);
	gE_msgbar_set(w, MSGBAR_FILE_NEW);
	gE_document_new(w);
	doc = gE_document_current(w);

	if (w->files_list_window)
		flw_append_entry(w, doc,
			g_list_length(GTK_NOTEBOOK(w->notebook)->children) - 1,
			doc->filename);
}


void file_newwindow_cmd_callback (GtkWidget *widget, gpointer cbdata)
{
	gE_window *window;
	gE_data *data = (gE_data *)cbdata;

	window = gE_window_new();
	window->auto_indent = data->window->auto_indent;
	window->show_tabs = data->window->show_tabs;
	window->show_status = data->window->show_status;
	window->tab_pos = data->window->tab_pos;
	window->have_toolbar = data->window->have_toolbar;
#ifndef WITHOUT_GNOME
	gE_get_settings (window);
#endif
}


void file_open_cmd_callback (GtkWidget *widget, gpointer cbdata)
{
	gE_data *data = (gE_data *)cbdata;
	gE_window *w;

	assert(data != NULL);
	w = data->window;
	assert(w != NULL);

	if (w->open_fileselector == NULL) {
		w->open_fileselector = gtk_file_selection_new("Open File...");
		gtk_file_selection_hide_fileop_buttons(
			GTK_FILE_SELECTION(w->open_fileselector));
		gtk_signal_connect(GTK_OBJECT(w->open_fileselector), "destroy",
			(GtkSignalFunc)file_sel_destroy, w->open_fileselector);
		gtk_signal_connect(GTK_OBJECT(
			GTK_FILE_SELECTION(w->open_fileselector)->ok_button), 
			"clicked", (GtkSignalFunc)file_open_ok_sel, data);
		gtk_signal_connect(GTK_OBJECT(
			GTK_FILE_SELECTION(
				w->open_fileselector)->cancel_button),
			"clicked", (GtkSignalFunc)file_cancel_sel,
			w->open_fileselector);
	}

	if (GTK_WIDGET_VISIBLE(w->open_fileselector))
		return;

	gtk_widget_show(w->open_fileselector);
}

void file_save_cmd_callback (GtkWidget *widget, gpointer cbdata)
{
	gchar *fname;
	gE_data *data = (gE_data *)cbdata;

	assert(data != NULL);
	assert(data->window != NULL);
 	fname = gE_document_current(data->window)->filename;
	if (fname == NULL)
		file_save_as_cmd_callback(NULL, data);
	else
		gE_file_save(data->window, gE_document_current(data->window),
			gE_document_current(data->window)->filename);
}

void file_save_as_cmd_callback (GtkWidget *widget, gpointer cbdata)
{
	GtkWidget *safs;
	gE_data *data = (gE_data *)cbdata;

	assert(data != NULL);

	safs = gtk_file_selection_new("Save As...");

	data->temp1 = safs;
	gtk_signal_connect(GTK_OBJECT(safs), "destroy",
		(GtkSignalFunc)file_saveas_destroy, safs);
	gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(safs)->ok_button),
		"clicked", (GtkSignalFunc)file_saveas_ok_sel, data);
	gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(safs)->cancel_button),
		"clicked", (GtkSignalFunc)file_saveas_destroy, safs);

	gtk_widget_show(safs);
}


/*
 * destroy the "save as" dialog box
 */
static void
file_saveas_destroy(GtkWidget *w, gpointer cbdata)
{
	gtk_widget_destroy((GtkWidget *)cbdata);
}


/*
 * file close callback (used from menus.c)
 */
void
file_close_cmd_callback (GtkWidget *widget, gpointer cbdata)
{
	gE_data *data = (gE_data *)cbdata;

	assert(data != NULL);
	close_doc_common(data);
}


/*
 * common routine for closing a file.  will prompt for saving if the
 * file was changed.
 *
 * data->flag is set to TRUE if file closed
 */
static void
close_doc_common(gE_data *data)
{
	gE_document *doc;

	assert(data != NULL);
	assert(data->window != NULL);
	doc = gE_document_current(data->window);
	if (doc->changed)
		popup_close_verify(doc, data);
	else {
		close_doc_execute(NULL, data);
		data->flag = TRUE;	/* indicate closed */
	}
} /* close_doc_common */


/*
 * actually close the document.
 */
static void
close_doc_execute(GtkWidget *widget, gpointer cbdata)
{
	int num, numdoc;
	GtkNotebook *nb;
	gE_window *w;
	gE_document *doc;
	gE_data *data = (gE_data *)cbdata;

	assert(data != NULL);
	w = data->window;
	assert(w != NULL);
	nb = GTK_NOTEBOOK(w->notebook);
	assert(nb != NULL);
	doc = gE_document_current(w);
	assert(doc != NULL);

	/* if all we have is a blank, Untitled doc, then return immediately */
	if (!doc->changed && g_list_length(nb->children) == 1 &&
		doc->filename == NULL)
		return;

	/* remove notebook entry and item from document list */
	num = gtk_notebook_current_page(nb);
	gtk_notebook_remove_page(nb, num);
	w->documents = g_list_remove(w->documents, doc);
	mbprintf("closed %s", (doc->filename) ? doc->filename : UNTITLED);
	if (doc->filename)
		g_free(doc->filename);
	if (doc->sb)
		g_free(doc->sb);
	g_free(doc);

	/* if files list window present, remove corresponding entry */
	flw_remove_entry(w, num);

	/* echo message to user */
	gE_msgbar_set(w, MSGBAR_FILE_CLOSED);

	num = g_list_length(nb->children);
	numdoc = g_list_length(w->documents);
	assert(num == numdoc);

	/*
	 * we always have at least one document (e.g., "Untitled").
	 * so if we just closed the last document, create "Untitled".
	 */
	if (num < 1) {
		g_list_free(w->documents);
		w->documents = NULL;
		gE_document_new(w);
		if (w->files_list_window)
			flw_append_entry(w, doc,
				g_list_length(nb->children) - 1, NULL);
	}

} /* close_doc_execute */


/*
 * close all documents in invoking window
 */
void
file_close_all_cmd_callback(GtkWidget *widget, gpointer cbdata)
{
	gE_data *data = (gE_data *)cbdata;
	GtkNotebook *nb;
	int num, i;
	gboolean allclosed = TRUE;

	assert(data != NULL);
	assert(data->window != NULL);
	nb = GTK_NOTEBOOK(data->window->notebook);
	assert(nb != NULL);
	num = g_list_length(nb->children);
	assert(num > 0);
	gtk_widget_hide(data->window->notebook);

	for (i = 0; i < num; i++) {
		close_doc_common(data);

		/* if a file was not closed, then all files were not closed */
		if (!data->flag) {
			allclosed = FALSE;
			break;
		}
	}

	gtk_widget_show(data->window->notebook);

	data->flag = allclosed;

	if (i == num) {
		assert(allclosed == TRUE);
		gE_msgbar_set(data->window, MSGBAR_FILE_CLOSED_ALL);
		mbprintf("closed all documents");
	}
} /* file_close_all_cmd_callback */


/*
 * closes gEdit window by closing all documents.  only if all documents are
 * closed will the window actually go away.
 */
void
file_close_window_cmd_callback(GtkWidget *widget, gpointer cbdata)
{
	gE_data *data = (gE_data *)cbdata;

	assert(data != NULL);
	assert(data->window != NULL);
	gtk_widget_hide(data->window->window);
	flw_destroy(NULL, data);

	data->flag = FALSE;	/* use flag to indicate all files closed */
	file_close_all_cmd_callback(widget, cbdata);

	if (data->flag) {
		gE_msgbar_clear((gpointer)(data->window));
		mbprintf("window closed");

		close_window_common(data->window);	/* may not return */

		data->window = g_list_nth_data(window_list, 0);
	} else
		gtk_widget_show(data->window->window);
}


/*
 * actually close the window.  exits if last window is closed.
 */
static void
close_window_common(gE_window *w)
{
	assert(w != NULL);
	window_list = g_list_remove(window_list, (gpointer)w);

	if (w->files_list_window)
		gtk_widget_destroy(w->files_list_window);
	gtk_widget_destroy(w->window);
	g_free(w->search);
	g_free(w);

	if (window_list == NULL)
		gtk_exit(0);
}


/*
 * quits gEdit by closing all windows.  only quits if all windows closed.
 */
void
file_quit_cmd_callback(GtkWidget *widget, gpointer cbdata)
{
	gE_data *data = (gE_data *)cbdata;

	assert(data != NULL);

	msgbox_close();
	while (window_list) {
		data->window = g_list_nth_data(window_list, 0);
		gtk_widget_hide(data->window->window);
		file_close_window_cmd_callback(widget, data);
		if (data->flag == FALSE)	/* cancelled by user */
			return;
	}
	gtk_exit(0);	/* should not reach here */
}


/* ---- Clipboard Callbacks ---- */

void edit_cut_cmd_callback (GtkWidget *widget, gpointer cbdata)
{
	gE_data *data = (gE_data *)cbdata;

#if (GTK_MAJOR_VERSION==1) && (GTK_MINOR_VERSION==1)
	gtk_editable_cut_clipboard(
		GTK_EDITABLE(gE_document_current(data->window)->text));
#else
	gtk_editable_cut_clipboard(GTK_EDITABLE(
		gE_document_current(data->window)->text), GDK_CURRENT_TIME);
#endif
	gE_msgbar_set(data->window, MSGBAR_CUT);
}

void edit_copy_cmd_callback (GtkWidget *widget, gpointer cbdata)
{
	gE_data *data = (gE_data *)cbdata;

#if (GTK_MAJOR_VERSION==1) && (GTK_MINOR_VERSION==1)
	gtk_editable_copy_clipboard(
		GTK_EDITABLE(gE_document_current(data->window)->text));
#else
	gtk_editable_copy_clipboard(GTK_EDITABLE(
		gE_document_current(data->window)->text), GDK_CURRENT_TIME);
#endif
	gE_msgbar_set(data->window, MSGBAR_COPY);
}
	
void edit_paste_cmd_callback (GtkWidget *widget, gpointer cbdata)
{
	gE_data *data = (gE_data *)cbdata;

#if (GTK_MAJOR_VERSION==1) && (GTK_MINOR_VERSION==1)
	gtk_editable_paste_clipboard(
		GTK_EDITABLE(gE_document_current(data->window)->text));
#else
	gtk_editable_paste_clipboard(GTK_EDITABLE(
			gE_document_current(data->window)->text), GDK_CURRENT_TIME);
#endif
	gE_msgbar_set(data->window, MSGBAR_PASTE);
}

void edit_selall_cmd_callback (GtkWidget *widget, gpointer cbdata)
{
	gE_data *data = (gE_data *)cbdata;

	gtk_editable_select_region(
		GTK_EDITABLE(gE_document_current(data->window)->text), 0,
		gtk_text_get_length(
			GTK_TEXT(gE_document_current(data->window)->text)));
	gE_msgbar_set(data->window, MSGBAR_SELECT_ALL);
}


/* ---- Search and Replace functions ---- */

/* seek_to_line sets the scrollbar to the *approximate* position of the specified line, so as
   to speed up searching, etc - it's still a good idea (or a must) to insert a char and delete one
   to get it to go to the exact position. */

static void
seek_to_line (gE_document *doc, gint line_number)
{
	gfloat value, ln, tl;
	gchar *c;
	gint i, total_lines = 0;

	c = g_malloc0 (4);
	for (i = 0; i < gtk_text_get_length (GTK_TEXT (doc->text)); i++)
	{
		c = gtk_editable_get_chars (GTK_EDITABLE (doc->text), i, i+1);
		if  (strcmp (c, "\n") == 0)
			total_lines++;
	}
	
	g_free (c);

	if (total_lines < 3)
		return;
	if (line_number > total_lines)
		return;
	tl = total_lines;
	ln = line_number;
	value = (ln * GTK_ADJUSTMENT (GTK_TEXT(doc->text)->vadj)->upper) / tl - GTK_ADJUSTMENT (GTK_TEXT(doc->text)->vadj)->page_increment;

#ifdef DEBUG
	printf ("%i\n", total_lines);
	printf ("%f\n", value);
	printf ("%f, %f\n", GTK_ADJUSTMENT (GTK_TEXT (doc->text)->vadj)->lower, GTK_ADJUSTMENT (GTK_TEXT (doc->text)->vadj)->upper);
#endif

	gtk_adjustment_set_value (GTK_ADJUSTMENT (GTK_TEXT (doc->text)->vadj), value);
}

static gint
point_to_line (gE_document *doc, gint point)
{
	gint i, lines;
	gchar *c = g_malloc0 (3);
	
	lines = 0;
	i = point;
	for (i = point; i > 1; i--)
	{
		c = gtk_editable_get_chars (GTK_EDITABLE (doc->text), i-1, i);
		if (strcmp (c, "\n") == 0)
			lines++;
	}
	
	g_free (c);
	return lines;
}

void popup_replace_window(gE_data *data);

void search_start (GtkWidget *w, gE_data *data)
{
	gE_search *options;
	gE_document *doc;
	gchar *search_for, *buffer, *replace_with;
	gchar bla[] = " ";
	gint len, start_pos, text_len, match, i, search_for_line, cur_line, end_line, oldchanged, replace_diff = 0;
	options = data->temp2;
	doc = gE_document_current (data->window);
	search_for = gtk_entry_get_text (GTK_ENTRY(options->search_entry));
	replace_with = gtk_entry_get_text (GTK_ENTRY(options->replace_entry));
	buffer = g_malloc0 (sizeof(search_for));
	len = strlen(search_for);
	oldchanged = doc->changed;
	
	if (options->again) {
		start_pos = gtk_text_get_point (GTK_TEXT (doc->text));
		options->again = 0;
	}
	else if (GTK_TOGGLE_BUTTON(options->start_at_cursor)->active)
		start_pos = GTK_EDITABLE(doc->text)->current_pos;
	else
		start_pos = 0;
		

	if ((text_len = gtk_text_get_length(GTK_TEXT(doc->text))) < len)
		return;
		


	if (GTK_CHECK_MENU_ITEM (options->line_item)->active || options->line)
	{
		start_pos = 0;
		cur_line = 1;
		sscanf (search_for, "%i", &search_for_line);
		for (i = start_pos; i < text_len; i++)
		{
			if (cur_line == search_for_line)
				break;
			buffer = gtk_editable_get_chars (GTK_EDITABLE (doc->text), i, i+1);
			if (strcmp (buffer, "\n") ==0)
				cur_line++;
		}
		if (i >= text_len)
			return;
		for (end_line = i; end_line < text_len; end_line++)
		{
			buffer = gtk_editable_get_chars (GTK_EDITABLE (doc->text), end_line, end_line+1);
			if (strcmp (buffer, "\n") == 0)
				break;
		}
		seek_to_line (doc, search_for_line);
		gtk_editable_insert_text (GTK_EDITABLE(doc->text), bla, strlen(bla), &i);
		gtk_editable_delete_text (GTK_EDITABLE(doc->text), i-1, i);
		doc->changed = oldchanged;
		gtk_editable_select_region (GTK_EDITABLE (doc->text), i-1, end_line);
		return;
	}

	gtk_text_freeze (GTK_TEXT(doc->text));

/*	for (i = start_pos; i <= (text_len - start_pos - len - replace_diff); i++)
*/
	for (i = start_pos; i <= (text_len - len - replace_diff); i++)
        {
                buffer = gtk_editable_get_chars(GTK_EDITABLE(doc->text), i, i+len);
                if (GTK_TOGGLE_BUTTON(options->case_sensitive)->active)
			match = strcmp(buffer, search_for);
		else
			match = strcasecmp(buffer, search_for);

		if (match == 0) {
			gtk_text_thaw (GTK_TEXT(doc->text));

			/* This is so the text will scroll to the selection no matter what */
			seek_to_line (doc, point_to_line (doc, i));
			gtk_editable_insert_text (GTK_EDITABLE(doc->text), bla, strlen(bla), &i);
			gtk_editable_delete_text (GTK_EDITABLE(doc->text), i-1, i);
			i--;
			doc->changed = oldchanged;

			gtk_text_set_point (GTK_TEXT(doc->text), i+len);
			gtk_editable_select_region (GTK_EDITABLE(doc->text), i, i+len);
			g_free (buffer);
			buffer = NULL;

			
			if (options->replace)
			{
				if (GTK_TOGGLE_BUTTON(options->prompt_before_replacing)->active) {
					gtk_text_set_point (GTK_TEXT(doc->text), i+2);
					popup_replace_window(data);
					return;
				}
				else {
					gtk_editable_delete_selection (GTK_EDITABLE(doc->text));
					gtk_editable_insert_text (GTK_EDITABLE(doc->text), replace_with, strlen(replace_with), &i);
					gtk_text_set_point (GTK_TEXT(doc->text), i+strlen(replace_with));
					replace_diff = replace_diff + (strlen(search_for) - strlen(replace_with));
					gtk_text_freeze (GTK_TEXT(doc->text));
				}
			}
			else
				break;
		}
		g_free (buffer);
	}
	gtk_text_thaw (GTK_TEXT (doc->text));
	
}


void search_for_text (GtkWidget *w, gE_search *options)
{
	gtk_widget_show (options->start_at_cursor);
	gtk_widget_show (options->start_at_beginning);
	gtk_widget_show (options->case_sensitive);
	options->line = 0;
	if (options->replace)
		gtk_widget_show (options->replace_box);
	/*gtk_menu_item_select (GTK_MENU_ITEM (options->text_item));	*/
	gtk_option_menu_set_history (GTK_OPTION_MENU (options->search_for), 0);
	gtk_check_menu_item_set_state (GTK_CHECK_MENU_ITEM (options->text_item), 1);
	gtk_editable_delete_text (GTK_EDITABLE (options->search_entry), 0, -1);
}

void search_for_line (GtkWidget *w, gE_search *options)
{
	gtk_widget_hide (options->start_at_cursor);
	gtk_widget_hide (options->start_at_beginning);
	gtk_widget_hide (options->case_sensitive);
	options->line = 1;
	if (options->replace) {
		gtk_widget_hide (options->replace_box);
	}
	gtk_option_menu_set_history (GTK_OPTION_MENU (options->search_for), 1);
	gtk_editable_delete_text (GTK_EDITABLE (options->search_entry), 0, -1);

}

void search_goto_line_callback (GtkWidget *w, gpointer cbwindow)
{
	gE_window *window = (gE_window *)cbwindow;

	if (!window->search->window)
		search_create (window, window->search, 0);
	gtk_check_menu_item_set_state (GTK_CHECK_MENU_ITEM (window->search->line_item), 1);
	gtk_option_menu_set_history (GTK_OPTION_MENU (window->search->search_for), 1);
	search_for_line (NULL, window->search);
	gtk_widget_show (window->search->window);

}


static void
search_create (gE_window *window, gE_search *options, gint replace)
{
	GtkWidget *search_label, *replace_label, *ok, *cancel, *search_hbox, *replace_hbox, *hbox;
	GtkWidget *search_for_menu_items, *search_for_label;
	gE_data *data;
	options->window = gtk_dialog_new();
	gtk_window_set_policy (GTK_WINDOW(options->window), TRUE, TRUE, TRUE);

	if (!replace) {
		options->replace = 0;
		gtk_window_set_title (GTK_WINDOW (options->window), ("Search"));
	}
	else {
		options->replace = 1;
		gtk_window_set_title (GTK_WINDOW (options->window), ("Search and Replace"));
	}

	hbox = gtk_hbox_new (FALSE, 1);
	search_for_label = gtk_label_new ("Search For:");
	gtk_widget_show (search_for_label);
	
	search_for_menu_items = gtk_menu_new ();
	options->text_item = gtk_radio_menu_item_new_with_label (NULL, "Text");
	gtk_menu_append (GTK_MENU (search_for_menu_items), options->text_item);
	gtk_widget_show (options->text_item);
	options->line_item = gtk_radio_menu_item_new_with_label (
		gtk_radio_menu_item_group (GTK_RADIO_MENU_ITEM (options->text_item)),
		"Line Number");
	gtk_menu_append (GTK_MENU (search_for_menu_items), options->line_item);
	gtk_widget_show (options->line_item);
	
	options->search_for = gtk_option_menu_new ();
	gtk_option_menu_set_menu (GTK_OPTION_MENU (options->search_for), search_for_menu_items);
	gtk_widget_show (options->search_for);
	
	gtk_widget_show (hbox);
	
	gtk_signal_connect (GTK_OBJECT (options->text_item), "activate", 
	                             GTK_SIGNAL_FUNC (search_for_text), options);
	gtk_signal_connect (GTK_OBJECT (options->line_item), "activate",
	                             GTK_SIGNAL_FUNC (search_for_line), options);
	 
	search_hbox = gtk_hbox_new(FALSE, 1);
	options->search_entry = gtk_entry_new();

	search_label = gtk_label_new (("Search:"));
	options->start_at_cursor = gtk_radio_button_new_with_label (NULL, ("Start searching at cursor position"));
	options->start_at_beginning = gtk_radio_button_new_with_label(
	                                           gtk_radio_button_group(GTK_RADIO_BUTTON(options->start_at_cursor)),
	                                           ("Start searching at beginning of the document"));
	options->case_sensitive = gtk_check_button_new_with_label (("Case sensitive"));
	
	options->replace_box = gtk_vbox_new(FALSE, 1);
	replace_hbox = gtk_hbox_new(FALSE, 1);
	replace_label = gtk_label_new (("Replace:"));
	options->replace_entry = gtk_entry_new();
	options->prompt_before_replacing = gtk_check_button_new_with_label (("Prompt before replacing"));

#ifdef WITHOUT_GNOME
	ok = gtk_button_new_with_label ("OK");
	cancel = gtk_button_new_with_label ("Cancel");
#else
	ok = gnome_stock_button (GNOME_STOCK_BUTTON_OK);
	cancel = gnome_stock_button (GNOME_STOCK_BUTTON_CANCEL);
#endif
	gtk_box_pack_start (GTK_BOX(GTK_DIALOG(options->window)->vbox), hbox, TRUE, TRUE, 0);
	
	gtk_box_pack_start (GTK_BOX(hbox), search_for_label, FALSE, FALSE, 2);
	gtk_box_pack_start (GTK_BOX(hbox), options->search_for, TRUE, TRUE, 0);
	
	gtk_box_pack_start (GTK_BOX(GTK_DIALOG(options->window)->vbox), search_hbox, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX(search_hbox), search_label, FALSE, FALSE, 2);
	gtk_box_pack_start (GTK_BOX(search_hbox), options->search_entry, TRUE, TRUE, 2);
	gtk_box_pack_start (GTK_BOX(GTK_DIALOG(options->window)->vbox), options->start_at_cursor, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX(GTK_DIALOG(options->window)->vbox), options->start_at_beginning, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX(GTK_DIALOG(options->window)->vbox), options->case_sensitive, TRUE, TRUE, 2);

	gtk_box_pack_start (GTK_BOX(GTK_DIALOG(options->window)->vbox), options->replace_box, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX(options->replace_box), replace_hbox, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX(replace_hbox), replace_label, FALSE, FALSE, 2);
	gtk_box_pack_start (GTK_BOX(replace_hbox), options->replace_entry, TRUE, TRUE, 0);
	gtk_box_pack_start (GTK_BOX(options->replace_box), options->prompt_before_replacing, TRUE, TRUE, 0);

	gtk_box_pack_start (GTK_BOX(GTK_DIALOG(options->window)->action_area), ok, TRUE, TRUE, 2);
	gtk_box_pack_start (GTK_BOX(GTK_DIALOG(options->window)->action_area), cancel, TRUE, TRUE, 2);

	gtk_widget_show (search_hbox);
	gtk_widget_show (options->search_entry);
	gtk_widget_show (search_label);
	gtk_widget_show (options->start_at_cursor);
	gtk_widget_show (options->start_at_beginning);
	gtk_widget_show (options->case_sensitive);

	if (replace)
		gtk_widget_show (options->replace_box);

	gtk_widget_show (replace_hbox);
	gtk_widget_show (replace_label);
	gtk_widget_show (options->replace_entry);
	gtk_widget_show (options->prompt_before_replacing);
	gtk_widget_show (ok);
	gtk_widget_show (cancel);
	/*gtk_widget_show (options->window);*/
	
	data = g_malloc0 (sizeof (gE_data));
	data->window = window;
	data->temp2 = options;
	gtk_signal_connect (GTK_OBJECT(ok), "clicked", GTK_SIGNAL_FUNC(search_start), data);
	gtk_signal_connect_object (GTK_OBJECT(ok), "clicked", 
	                                       GTK_SIGNAL_FUNC(gtk_widget_hide), (gpointer) options->window);
	/*GTK_WIDGET_SET_FLAGS (ok, GTK_CAN_DEFAULT);
	gtk_widget_grab_default (ok);*/
	gtk_signal_connect_object (GTK_OBJECT(cancel), "clicked", 
	                                       GTK_SIGNAL_FUNC(gtk_widget_hide), (gpointer) options->window);
}



void search_search_cmd_callback (GtkWidget *w, gpointer cbdata)
{
	gE_data *data = (gE_data *)cbdata;

	if (!data->window->search->window)
		search_create (data->window, data->window->search, 0);

	data->window->search->replace = 0;
	data->window->search->again = 0;
	search_for_text (NULL, data->window->search);
	gtk_window_set_title (GTK_WINDOW (data->window->search->window), ("Search"));
	gtk_widget_hide (data->window->search->replace_box);
	gtk_widget_show (data->window->search->window);

}

void search_replace_cmd_callback (GtkWidget *w, gpointer cbdata)
{
	gE_data *data = (gE_data *)cbdata;

	if (!data->window->search->window)
		search_create (data->window, data->window->search, 1);

	data->window->search->replace = 1;
	data->window->search->again = 0;
	search_for_text (NULL, data->window->search);
	gtk_window_set_title (GTK_WINDOW (data->window->search->window), ("Search and Replace"));
	gtk_widget_show (data->window->search->replace_box);
	gtk_widget_show (data->window->search->window);

	
}

void search_again_cmd_callback (GtkWidget *w, gpointer cbdata)
{
	gE_data *data = (gE_data *)cbdata;

	if (data->window->search->window) {
		data->temp2 = data->window->search;
		data->window->search->replace = 0;
		data->window->search->again = 1;
		search_start (w, data);
	}
}

void search_replace_yes_sel (GtkWidget *w, gE_data *data)
{
	gchar *replace_with;
	gE_document *doc;
	gint i;
	doc = gE_document_current (data->window);
	replace_with = gtk_entry_get_text (GTK_ENTRY(data->window->search->replace_entry));
	i = gtk_text_get_point (GTK_TEXT (doc->text)) - 2;
	gtk_editable_delete_selection (GTK_EDITABLE(doc->text));
	gtk_editable_insert_text (GTK_EDITABLE(doc->text), replace_with, strlen(replace_with), &i);
	gtk_text_set_point (GTK_TEXT(doc->text), i+strlen(replace_with));
	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON(data->window->search->start_at_cursor), TRUE);
	data->temp2 = data->window->search;
	search_start (w, data);
}

void search_replace_no_sel (GtkWidget *w, gE_data *data)
{
	/*gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON(data->window->search->start_at_cursor), TRUE);*/
	data->window->search->again = 1;
	data->temp2 = data->window->search;
	search_start (w, data);
}

void popup_replace_window(gE_data *data)
{
	GtkWidget *window, *yes, *no, *cancel, *label;
	window = gtk_dialog_new();
	gtk_window_set_title (GTK_WINDOW(window), ("Replace?"));
	gtk_widget_set_usize (GTK_WIDGET (window), 280, 90);
	label = gtk_label_new (("Are you sure you want to replace this?"));
#ifdef WITHOUT_GNOME
	yes = gtk_button_new_with_label ("Yes");
	no = gtk_button_new_with_label ("No");
	cancel = gtk_button_new_with_label ("Cancel");
#else
	/* This whole code could be enhanced by using gnome_messagebox */
	yes    = gnome_stock_button (GNOME_STOCK_BUTTON_YES);
	no     = gnome_stock_button (GNOME_STOCK_BUTTON_NO);
	cancel = gnome_stock_button (GNOME_STOCK_BUTTON_CANCEL);
#endif
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), label, TRUE, FALSE, 10);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), yes, TRUE, TRUE, 2);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), no, TRUE, TRUE, 2);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->action_area), cancel, TRUE, TRUE, 2);
	gtk_widget_show (label);
	gtk_widget_show (yes);
	gtk_widget_show (no);
	gtk_widget_show (cancel);
	gtk_widget_show (window);
	gtk_signal_connect (GTK_OBJECT(yes), "clicked", GTK_SIGNAL_FUNC(search_replace_yes_sel), data);
	gtk_signal_connect (GTK_OBJECT(no), "clicked", GTK_SIGNAL_FUNC(search_replace_no_sel), data);
	gtk_signal_connect_object (GTK_OBJECT(yes), "clicked", GTK_SIGNAL_FUNC(gtk_widget_destroy), (gpointer) window);
	gtk_signal_connect_object (GTK_OBJECT(no), "clicked", GTK_SIGNAL_FUNC(gtk_widget_destroy), (gpointer) window);
	gtk_signal_connect_object (GTK_OBJECT(cancel), "clicked", GTK_SIGNAL_FUNC(gtk_widget_destroy), (gpointer) window);
	gtk_grab_add (window);
}

/* Add a file to the Recent-used list... */

void recent_add (char *filename)
{
	#ifndef WITHOUT_GNOME
	gnome_history_recently_used (filename, "text/plain", "gEdit", "");
	#endif
	
}

/*
 * gnome_app_remove_menu_range(app, path, start, num) removes num items from the existing app's menu structure
 * begining with item described by path plus the number specified by start
 */
void
gnome_app_remove_menu_range (GnomeApp *app,
		       gchar *path,
		       gint start,
		       gint items)
{
  GtkWidget *parent, *child;
  GList *children;
  gint pos;

  g_return_if_fail(app != NULL);
  g_return_if_fail(GNOME_IS_APP(app));

  /* find the first item (which is actually at position pos-1) to remove */
  parent = gnome_app_find_menu_pos(app->menubar, path, &pos);

  /* in case of path ".../" remove the first item */
  if(pos == 0)
    pos = 1;

  pos += start;
  
  if( parent == NULL ) {
    g_warning("gnome_app_remove_menus: couldn't find first item to remove!");
    return;
  }

  /* remove items */
  children = g_list_nth(GTK_MENU_SHELL(parent)->children, pos - 1);
  while(children && items > 0) {
    child = GTK_WIDGET(children->data);
    /* children = g_list_next(children); */
    gtk_container_remove(GTK_CONTAINER(parent), child);
    children = g_list_nth(GTK_MENU_SHELL(parent)->children, pos - 1);
    items--;
  }

  gtk_widget_queue_resize(parent);
}

/* Grabs the recent used list, then updates the menus via a call to recent_update_menus 
 * Should be called after each addition to the list 
 */

void recent_update (gE_window *window)
{
	GList *filelist;
	
	#ifndef WITHOUT_GNOME
	GList *gnome_recent_list;
	GnomeHistoryEntry histentry;
	char *filename;
	int i;
	
	filelist = NULL;
	gnome_recent_list = gnome_history_get_recently_used ();
	
	if (g_list_length (gnome_recent_list) > 0)
	{
		for (i = g_list_length (gnome_recent_list) - 1; i >= 0; i--)
		{
			histentry = g_list_nth_data (gnome_recent_list, i);
			if (strcmp ("gEdit", histentry->creator) == 0)
			{
				filename = g_malloc0 (strlen (histentry->filename) + 1);
				strcpy (filename, histentry->filename);
				filelist = g_list_append (filelist, filename);
				if (g_list_length (filelist) == MAX_RECENT)
					break;
			}
		}
	}
	gnome_history_free_recently_used_list (gnome_recent_list);
	#endif /* Using GNOME */
	
	recent_update_menus (window, filelist);
}

/* Actually updates the recent-used menu... */

void recent_update_menus (gE_window *window, GList *recent_files)
{
	#ifndef WITHOUT_GNOME
	GnomeUIInfo *menu;
	gE_data *data;
	gchar *path;
	int i;
	
	if (window->num_recent > 0)
		gnome_app_remove_menu_range (GNOME_APP (window->window), 
		                                         "File/", 8, window->num_recent + 1);

	if (recent_files == NULL)
		return;


	/* insert a separator at the beginning */
	
	menu = g_malloc0 (2 * sizeof (GnomeUIInfo));
	path = g_new (gchar, strlen (_("File")) + strlen ("<Separator>") + 3 );
	sprintf (path, "%s/%s", _("File"), "<Separator>");
	menu->type = GNOME_APP_UI_SEPARATOR;

	(menu + 1)->type = GNOME_APP_UI_ENDOFINFO;
	gnome_app_insert_menus (GNOME_APP(window->window), path, menu);


	for (i = g_list_length (recent_files) - 1; i >= 0;  i--)
	{
		menu = g_malloc0 (2 * sizeof (GnomeUIInfo));
	
		data = g_malloc0 (sizeof (gE_data));
		data->temp1 = g_strdup (g_list_nth_data (recent_files, i));
		data->window = window;
		menu->label = g_new (gchar, strlen (g_list_nth_data (recent_files, i)) + 5);
		sprintf (menu->label, "%i. %s", i+1, g_list_nth_data (recent_files, i));
		menu->type = GNOME_APP_UI_ITEM;
		menu->hint = NULL;
		menu->moreinfo = recent_callback;
		menu->user_data = data;
		menu->unused_data = NULL;
		menu->pixmap_type = 0;
		menu->pixmap_info = NULL;
		menu->accelerator_key = 0;

		(menu + 1)->type = GNOME_APP_UI_ENDOFINFO;
	
		gnome_app_insert_menus_with_data (GNOME_APP(window->window), path, menu, data);
	}
	window->num_recent = g_list_length (recent_files);
	

	#endif /* Using GNOME */
}

void recent_callback (GtkWidget *w, gE_data *data)
{
	gE_document *doc = gE_document_current (data->window);
	if (doc->filename != NULL || doc->changed != 0)
		gE_document_new (data->window);
	gE_file_open (data->window, gE_document_current (data->window), data->temp1);
}


