/* vi:set ts=8 sts=0 sw=8:
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

#include <unistd.h>
#define __need_sigset_t
#include <signal.h>
#define __need_timespec
#include <time.h>
/*#include <signal.h>*/
#include <sys/stat.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <config.h>
#ifndef WITHOUT_GNOME
#include <gnome.h>
#include <libgnome/gnome-history.h>
#endif

#include "main.h"
#include "commands.h"
#include "gE_document.h"
#include "gE_prefs.h"
#include "gE_files.h"
#include "gE_plugin_api.h"
#include "msgbox.h"
#include "dialog.h"

static void close_file_save_yes_sel (GtkWidget *w, gE_data *data);
static void close_file_save_cancel_sel(GtkWidget *w, gE_data *data);
static void close_file_save_no_sel(GtkWidget *w, gE_data *data);
static void close_doc_common(gE_data *data);
static void close_window_common(gE_window *w);
static void file_saveas_destroy(GtkWidget *w, gpointer cbdata);
static void file_cancel_sel (GtkWidget *w, GtkFileSelection *fs);
static void file_sel_destroy (GtkWidget *w, GtkFileSelection *fs);
static void line_pos_cb(GtkWidget *w, gE_data *data);
static void recent_update_menus (gE_window *window, GList *recent_files);
static void recent_cb(GtkWidget *w, gE_data *data);


/* handles changes in the text widget... */
void
doc_changed_cb(GtkWidget *w, gpointer cbdata)
{
gchar MOD_label[255];

	gE_document *doc = (gE_document *) cbdata;

	doc->changed = TRUE;
	gtk_signal_disconnect (GTK_OBJECT(doc->text), (gint) doc->changed_id);
	doc->changed_id = FALSE;
	
	sprintf(MOD_label, "*%s", GTK_LABEL(doc->tab_label)->label);
	gtk_label_set(GTK_LABEL(doc->tab_label), MOD_label);
}


/*
 * file save callback : user selected "No"
 */
static void
close_file_save_no_sel(GtkWidget *w, gE_data *data)
{
	g_assert(data != NULL);
	close_doc_execute(NULL, data);
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

	g_assert(data != NULL);
	doc = data->document;

	if (doc->filename == NULL) {
		data->temp1 = NULL;
		file_save_as_cb(w, data);
		if (data->flag == TRUE) /* close document if successful */
			close_doc_execute(NULL, data);
	} else {
		int error;

		error = gE_file_save(data->window, doc, doc->filename);
		if (!error) {
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
	g_assert(data != NULL);
	data->temp1 = NULL;
	data->temp2 = NULL;
	data->flag = FALSE;
} /* close_file_save_cancel_sel */


/*
 * creates file save (yes/no/cancel) dialog box
 */
#define CLOSE_TITLE	"Save File"
#define CLOSE_MSG	"has been modified.  Do you wish to save it?"
void
popup_close_verify(gE_document *doc, gE_data *data)
{
	int ret;
	char *fname, *title, *msg;
	char *buttons[] = { GE_BUTTON_YES, GE_BUTTON_NO, GE_BUTTON_CANCEL } ;

#ifdef GTK_HAVE_FEATURES_1_1_0	
	fname = (doc->filename) ? g_basename(doc->filename) : UNTITLED;
#else
	fname = doc->filename;
#endif
	
	title = (char *)g_malloc(strlen(CLOSE_TITLE) + strlen(fname) + 5);
	msg =   (char *)g_malloc(strlen(CLOSE_MSG) + strlen(fname) + 6);

	sprintf(title, "%s '%s'?", CLOSE_TITLE, fname);
	sprintf(msg  , " '%s' %s ", fname, CLOSE_MSG);

	ret = ge_dialog(title, msg, 3, buttons, 3, NULL, NULL, TRUE);

	g_free(title);
	g_free(msg);

	/* use data->flag to indicate whether or not to quit */
	data->flag = FALSE;
	data->document = doc;

	switch (ret) {
	case 1 :
		close_file_save_yes_sel(NULL, data);
		break;
	case 2 :
		close_file_save_no_sel(NULL, data);
		break;
	case 3 :
		close_file_save_cancel_sel(NULL, data);
		break;
	default:
		printf("popup_close_verify: ge_dialog returned %d\n", ret);
		exit(-1);
	} /* switch */
	
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


	g_assert(data != NULL);
	w = data->window;
	g_assert(w != NULL);
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

	g_assert(data != NULL);
	safs = (GtkWidget *)(data->temp1);
	g_assert(safs != NULL);

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


/* --- Notebook Tab Stuff --- */

void
tab_top_cb(GtkWidget *widget, gpointer cbwindow)
{
	gE_window *w = (gE_window *)cbwindow;

	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(w->notebook), GTK_POS_TOP);
	w->tab_pos = GTK_POS_TOP;
}


void
tab_bot_cb(GtkWidget *widget, gpointer cbwindow)
{
	gE_window *w = (gE_window *)cbwindow;

	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(w->notebook), GTK_POS_BOTTOM);
	w->tab_pos = GTK_POS_BOTTOM;
}

void
tab_lef_cb(GtkWidget *widget, gpointer cbwindow)
{
	gE_window *w = (gE_window *)cbwindow;

	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(w->notebook), GTK_POS_LEFT);
	w->tab_pos = GTK_POS_LEFT;
}

void
tab_rgt_cb(GtkWidget *widget, gpointer cbwindow)
{
	gE_window *w = (gE_window *)cbwindow;

	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(w->notebook), GTK_POS_RIGHT);
	w->tab_pos = GTK_POS_RIGHT;
}

void
tab_toggle_cb(GtkWidget *widget, gpointer cbwindow)
{
	gE_window *w = (gE_window *)cbwindow;

	w->show_tabs = !w->show_tabs;
	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(w->notebook), w->show_tabs);
}



/* ---- Auto-indent Callback(s) --- */

void
auto_indent_toggle_cb(GtkWidget *w, gpointer cbdata)
{
	gE_data *data = (gE_data *)cbdata;

	gE_window_set_auto_indent (data->window, !data->window->auto_indent);
}

void
auto_indent_cb(GtkWidget *text, GdkEventKey *event, gE_window *window)
{
	int i, newlines, newline_1 = 0;
	gchar *buffer, *whitespace;
	gE_data *data;
	
	data = g_malloc0 (sizeof (gE_data));
	data->temp2 = text;
	data->window = window;
	line_pos_cb(NULL, data);

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
	line_pos_cb(NULL, data); /* <-- this is so the statusbar updates when it auto-indents */
}


static void
line_pos_cb(GtkWidget *w, gE_data *data)
{
	static char line [32];
	static char col [32];
	GtkWidget *text = data->temp2;
	int x;
	
	/*x = GTK_TEXT(text)->current_line->data;*/
	
	sprintf (line,"%d", GTK_TEXT(text)->cursor_pos_y/13);
	/*sprintf(line,"%d", x);*/
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
	line_pos_cb(NULL, data);
}


/* ---- File Menu Callbacks ---- */

void file_new_cb (GtkWidget *widget, gpointer cbdata)
{
	gE_data *data = (gE_data *)cbdata;
	gE_window *w;
	gE_document *doc;

	g_assert(data != NULL);
	w = data->window;
	g_assert(w != NULL);
	gE_msgbar_set(w, MSGBAR_FILE_NEW);
	gE_document_new(w);
	doc = gE_document_current(w);

	if (w->files_list_window)
		flw_append_entry(w, doc,
			g_list_length(GTK_NOTEBOOK(w->notebook)->children) - 1,
			doc->filename);
}


void window_new_cb(GtkWidget *widget, gpointer cbdata)
{
	gE_window *window;
	gE_data *data = (gE_data *)cbdata;

	window = gE_window_new();
	window->auto_indent = data->window->auto_indent;
	window->show_tabs = data->window->show_tabs;
	window->show_status = data->window->show_status;
	window->tab_pos = data->window->tab_pos;
	window->have_toolbar = data->window->have_toolbar;

	gE_get_settings (window);
}


void file_open_cb(GtkWidget *widget, gpointer cbdata)
{
	gE_data *data = (gE_data *)cbdata;
	gE_window *w;

	g_assert(data != NULL);
	w = data->window;
	g_assert(w != NULL);

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


/*
 * XXX - by using "buffer = gtk_editable_get_chars()", we're effectively making
 * a second copy of the file to be opened in a new window.  if this file is
 * huge, we have some serious memory usage and this can be really expensive.  i
 * think it would be better to remove document from the doc list of the
 * original window and directly put it into the new window.  however, we still
 * have to "close" the document in the original window without destroying the
 * actual contents.
 */
void
file_open_in_new_win_cb(GtkWidget *widget, gpointer cbdata)
{
	gE_document *src_doc, *dest_doc;
	gE_window *win;
	gchar *buffer;
	gE_data *data = (gE_data *)cbdata;
	int pos = 0;
	#ifdef GTK_HAVE_FEATURES_1_1_0	
	gchar *base;
	#endif
	
	src_doc = gE_document_current (data->window);
	buffer = gtk_editable_get_chars (GTK_EDITABLE (src_doc->text), 0, -1);
	win = gE_window_new ();
	dest_doc = gE_document_current (win);
	dest_doc->filename = g_strdup (src_doc->filename);
	#ifdef GTK_HAVE_FEATURES_1_1_0	
	gtk_label_set (GTK_LABEL (dest_doc->tab_label), (const char *)g_basename (dest_doc->filename));
	#else
	gtk_label_set (GTK_LABEL (dest_doc->tab_label), GTK_LABEL (src_doc->tab_label)->label);
	#endif
	gtk_editable_insert_text (GTK_EDITABLE (dest_doc->text), buffer, strlen (buffer), &pos);
	dest_doc->changed = src_doc->changed;
	close_doc_execute (src_doc, data);
	g_free (buffer);
}


void
file_save_cb(GtkWidget *widget, gpointer cbdata)
{
	gchar *fname;
	gE_data *data = (gE_data *)cbdata;

	g_assert(data != NULL);
	g_assert(data->window != NULL);
 	fname = gE_document_current(data->window)->filename;
	if (fname == NULL)
		file_save_as_cb(NULL, data);
	else
	 if ((gE_file_save(data->window, gE_document_current(data->window),
	               gE_document_current(data->window)->filename)) != 0)
	   {
	gE_msgbar_set(data->window, "Read only file!");
	file_save_as_cb(NULL, data);
        }

}

void
file_save_as_cb(GtkWidget *widget, gpointer cbdata)
{
	GtkWidget *safs;
	gE_data *data = (gE_data *)cbdata;

	g_assert(data != NULL);

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
file_close_cb(GtkWidget *widget, gpointer cbdata)
{
	gE_data *data = (gE_data *)cbdata;

	g_assert(data != NULL);
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

	g_assert(data != NULL);
	g_assert(data->window != NULL);
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
void
close_doc_execute(gE_document *opt_doc, gpointer cbdata)
{
	int num, numdoc;
	GtkNotebook *nb;
	gE_window *w;
	gE_document *doc;
	gE_data *data = (gE_data *)cbdata;

	g_assert(data != NULL);
	w = data->window;
	g_assert(w != NULL);
	nb = GTK_NOTEBOOK(w->notebook);
	g_assert(nb != NULL);
	/*
	 * Provide the option to pass the specific document as an argument
	 * instead of always going with the current document - needed for
	 * the plugins api...
	 */
	doc = (opt_doc == NULL) ? gE_document_current(w) : opt_doc;
	g_assert(doc != NULL);

	/* if all we have is a blank, Untitled doc, then return immediately */
	if (!doc->changed && g_list_length(nb->children) == 1 &&
		doc->filename == NULL)
		return;

	/* Remove document from our hash tables */
	g_free( g_hash_table_lookup(doc_pointer_to_int, doc) );
	g_hash_table_remove(doc_int_to_pointer, g_hash_table_lookup(doc_pointer_to_int, doc));
	g_hash_table_remove(doc_pointer_to_int, doc);

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
	g_assert(num == numdoc);

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
file_close_all_cb(GtkWidget *widget, gpointer cbdata)
{
	gE_data *data = (gE_data *)cbdata;
	GtkNotebook *nb;
	int num, i;
	gboolean allclosed = TRUE;

	g_assert(data != NULL);
	g_assert(data->window != NULL);
	nb = GTK_NOTEBOOK(data->window->notebook);
	g_assert(nb != NULL);
	num = g_list_length(nb->children);
	g_assert(num > 0);
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
		g_assert(allclosed == TRUE);
		gE_msgbar_set(data->window, MSGBAR_FILE_CLOSED_ALL);
		mbprintf("closed all documents");
	}
} /* file_close_all_cb */


/*
 * closes gEdit window by closing all documents.  only if all documents are
 * closed will the window actually go away.
 */
void
window_close_cb(GtkWidget *widget, gpointer cbdata)
{
	gE_data *data = (gE_data *)cbdata;

	g_assert(data != NULL);
	g_assert(data->window != NULL);
	gtk_widget_hide(data->window->window);
	flw_destroy(NULL, data);

	data->flag = FALSE;	/* use flag to indicate all files closed */
	file_close_all_cb(widget, cbdata);

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
	g_assert(w != NULL);
	window_list = g_list_remove(window_list, (gpointer)w);

	g_free( g_hash_table_lookup(win_pointer_to_int, w) );
	g_hash_table_remove(win_int_to_pointer, g_hash_table_lookup(win_pointer_to_int, w));
	g_hash_table_remove(win_pointer_to_int, w);

	if (w->files_list_window)
		gtk_widget_destroy(w->files_list_window);
	gtk_widget_destroy(w->window);
	gE_save_settings (w, w);

	g_free(w->search);
	g_free(w);

	if (window_list == NULL)
	{
		#ifdef WITHOUT_GNOME
		gE_prefs_close ();
		#endif
		gtk_exit(0);
	}
}


/*
 * quits gEdit by closing all windows.  only quits if all windows closed.
 */
void
file_quit_cb(GtkWidget *widget, gpointer cbdata)
{
	gE_data *data = (gE_data *)cbdata;

	g_assert(data != NULL);

	msgbox_close();
	while (window_list) {
		data->window = g_list_nth_data(window_list, 0);
		gtk_widget_hide(data->window->window);
		window_close_cb(widget, data);
		if (data->flag == FALSE)	/* cancelled by user */
			return;
	}
	gtk_exit(0);	/* should not reach here */
}


/* ---- Clipboard Callbacks ---- */

void
edit_cut_cb(GtkWidget *widget, gpointer cbdata)
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

void
edit_copy_cb(GtkWidget *widget, gpointer cbdata)
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
	
void
edit_paste_cb(GtkWidget *widget, gpointer cbdata)
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

void
edit_selall_cb(GtkWidget *widget, gpointer cbdata)
{
	gE_data *data = (gE_data *)cbdata;

	gtk_editable_select_region(
		GTK_EDITABLE(gE_document_current(data->window)->text), 0,
		gtk_text_get_length(
			GTK_TEXT(gE_document_current(data->window)->text)));
	gE_msgbar_set(data->window, MSGBAR_SELECT_ALL);
}


/* Add a file to the Recent-used list... */

void recent_add (char *filename)
{
#ifndef WITHOUT_GNOME
	gnome_history_recently_used (filename, "text/plain", "gEdit", "");
#endif
	
}

#ifndef WITHOUT_GNOME
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
#endif

/* Grabs the recent used list, then updates the menus via a call to recent_update_menus 
 * Should be called after each addition to the list 
 */

void recent_update (gE_window *window)
{
	GList *filelist = NULL;
	GList *dirlist = NULL;
#ifndef WITHOUT_GNOME
	
	GList *gnome_recent_list;
	GnomeHistoryEntry histentry;
	char *filename;
	int i, j;
	gboolean duplicate = FALSE;
	
	filelist = NULL;
	gnome_recent_list = gnome_history_get_recently_used ();
	
	if (g_list_length (gnome_recent_list) > 0)
	{
		for (i = g_list_length (gnome_recent_list) - 1; i >= 0; i--)
		{
			histentry = g_list_nth_data (gnome_recent_list, i);
			if (strcmp ("gEdit", histentry->creator) == 0)
			{
				/* This is to make sure you don't have more than one
				   file of the same name in the recent list
				*/
				if (g_list_length (filelist) > 0)
					for (j = g_list_length (filelist) - 1; j >= 0; j--)
						if (strcmp (histentry->filename, g_list_nth_data (filelist, j)) == 0)
							filelist = g_list_remove (filelist, g_list_nth_data (filelist, j));

				filename = g_malloc0 (strlen (histentry->filename) + 1);
				strcpy (filename, histentry->filename);
				filelist = g_list_append (filelist, filename);

/* For recent-directories, not yet fully implemented...
				end_path = strrchr (histentry->filename, '/');
				if (end_path)
				{
					for (i = 0; i < strlen (histentry->filename); i++)
						if ( (histentry->filename + i) == end_path)
							break;
					directory = g_malloc0 (i + 2);
					strcat (directory, histentry->filename, i);
				}
*/
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

static void
recent_update_menus (gE_window *window, GList *recent_files)
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
		sprintf (menu->label, "_%i. %s", i+1, (gchar*)g_list_nth_data (recent_files, i));
		menu->type = GNOME_APP_UI_ITEM;
		menu->hint = NULL;
		menu->moreinfo = (gpointer) recent_cb;
		menu->user_data = data;
		menu->unused_data = NULL;
		menu->pixmap_type = 0;
		menu->pixmap_info = NULL;
		menu->accelerator_key = 0;

		(menu + 1)->type = GNOME_APP_UI_ENDOFINFO;
	
		gnome_app_insert_menus_with_data (GNOME_APP(window->window), path, menu, data);
		g_free (g_list_nth_data (recent_files, i));
	}
	window->num_recent = g_list_length (recent_files);
	g_list_free (recent_files);

#endif /* Using GNOME */
}


#ifdef GTK_HAVE_FEATURES_1_1_0

static void
recent_cb(GtkWidget *w, gE_data *data)
{
	gE_document *doc = gE_document_current (data->window);
	if (doc->filename != NULL || doc->changed != 0)
		gE_document_new (data->window);
	gE_file_open (data->window, gE_document_current (data->window), data->temp1);
}


/* 
 * Text insertion and deletion callbacks - used for Undo/Redo (not yet implemented) and split screening
 */

void
doc_insert_text_cb(GtkWidget *editable, char *insertion_text, int length,
	int *pos, gE_document *doc)
{
	GtkWidget *significant_other;
	gchar *buffer;
	gint position = *pos;
	
	if (!doc->split_screen)
		return;
	
	if (doc->flag == editable)
	{
		doc->flag = NULL;
		return;
	}
	
	if (editable == doc->text)
		significant_other = doc->split_screen;
	else if (editable == doc->split_screen)
		significant_other = doc->text;
	else
		return;
	
	doc->flag = significant_other;	
	buffer = g_strdup (insertion_text);
	gtk_text_freeze (GTK_TEXT (significant_other));
	gtk_editable_insert_text (GTK_EDITABLE (significant_other), buffer, length, &position);
	gtk_text_thaw (GTK_TEXT (significant_other));
	g_free (buffer);

}

void
doc_delete_text_cb(GtkWidget *editable, int start_pos, int end_pos,
	gE_document *doc)
{
	GtkWidget *significant_other;
	
	if (!doc->split_screen)
		return;

	if (doc->flag == editable)
	{
		doc->flag = NULL;
		return;
	}
	
	if (editable == doc->text)
		significant_other = doc->split_screen;
	else if (editable == doc->split_screen)
		significant_other = doc->text;
	else
		return;
	
	doc->flag = significant_other;
	gtk_text_freeze (GTK_TEXT (significant_other));
	gtk_editable_delete_text (GTK_EDITABLE (significant_other), start_pos, end_pos);
	gtk_text_thaw (GTK_TEXT (significant_other));
}

void options_toggle_split_screen_cb (GtkWidget *widget, gE_window *window)
{
	gE_document *doc = gE_document_current (window);
	gint visible;

	if (!doc->split_parent)
		return;

	gE_document_set_split_screen
		(doc, !GTK_WIDGET_VISIBLE (doc->split_parent));
}

#endif	/* GTK_HAVE_FEATURES_1_1_0 */

#ifndef WITHOUT_GNOME

void options_toggle_scroll_ball_cb (GtkWidget *w, gE_window *window)
{
	gE_document *doc = gE_document_current (window);
	gint visible = GTK_WIDGET_VISIBLE
		(GTK_WIDGET (doc->scrollball));
	gE_document_set_scroll_ball (doc, !visible);
}

#endif /* WITHOUT_GNOME */

void options_toggle_read_only_cb (GtkWidget *widget, gE_window *window)
{
	gE_document *doc = gE_document_current (window);
	gE_document_set_read_only (doc, !doc->read_only);
}

void options_toggle_word_wrap_cb (GtkWidget *widget, gE_window *window)
{
	gE_document *doc = gE_document_current (window);
	gE_document_set_word_wrap (doc, !doc->word_wrap);
}

#ifdef GTK_HAVE_FEATURES_1_1_0	
void options_toggle_line_wrap_cb (GtkWidget *widget, gE_window *window)
{
	gE_document *doc = gE_document_current (window);
	gE_document_set_line_wrap (doc, !doc->line_wrap);
}
#endif

void options_toggle_status_bar_cb (GtkWidget *w, gE_window *window)
{
	gE_window_set_status_bar (window, !window->show_status);
}
