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
#include <gnome.h>
#include <libgnome/gnome-history.h>


#include "main.h"
#include "commands.h"
#include "gE_mdi.h"
#include "gE_document.h"
#include "gE_prefs.h"
#include "gE_files.h"
#include "gE_plugin_api.h"
#include "dialog.h"

static void close_file_save_yes_sel (GtkWidget *w, gE_data *data);
static void close_file_save_cancel_sel(GtkWidget *w, gE_data *data);
static void close_file_save_no_sel(GtkWidget *w, gE_data *data);
static void close_doc_common(gE_data *data);
static void close_window_common(gE_window *w);
static gint file_saveas_destroy(GtkWidget *w, GtkWidget **sel);
static gint file_cancel_sel (GtkWidget *w, GtkFileSelection *fs);
static gint file_sel_destroy (GtkWidget *w, GtkFileSelection *fs);
static void line_pos_cb(GtkWidget *w, gE_data *data);
static void recent_update_menus (GnomeApp *app, GList *recent_files);
static void recent_cb(GtkWidget *w, gE_data *data);

static GtkWidget *open_fs, *save_fs;
GtkWidget *ssel = NULL;
GtkWidget *osel = NULL;
GtkWidget *col_label;
gchar *oname = NULL;

/* handles changes in the text widget... */
void
doc_changed_cb(GtkWidget *w, gpointer cbdata)
{
gchar MOD_label[255];

	gE_document *doc = (gE_document *) cbdata;

	doc->changed = TRUE;
	gtk_signal_disconnect (GTK_OBJECT(doc->text), (gint) doc->changed_id);
	doc->changed_id = FALSE;
	
	sprintf(MOD_label, "*%s", GNOME_MDI_CHILD(doc)->name);
	/*gtk_label_set(GTK_LABEL(doc->tab_label), MOD_label);*/
	gnome_mdi_child_set_name (GNOME_MDI_CHILD (doc), MOD_label);
}


/*
 * file save callback : user selected "No"
 */
static void
close_file_save_no_sel(GtkWidget *w, gE_data *data)
{
	g_assert(data != NULL);
	/*close_doc_execute(NULL, data);*/
	file_close_cb (w, data);
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
		     file_close_cb (w, data);
			/*close_doc_execute(NULL, data);*/
	} else {
		int error;

		error = gE_file_save(doc, doc->filename);
		if (!error) {
			data->temp1 = NULL;
			/*close_doc_execute(NULL, data);*/
			file_close_cb (w, data);
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
#define CLOSE_MSG	"has been modified.  Do you wish to save it?"
void
popup_close_verify(gE_document *doc, gE_data *data)
{
	GtkWidget *msgbox;
	int ret;
	char *fname, *msg;
	char *buttons[] = { GE_BUTTON_YES, GE_BUTTON_NO, GE_BUTTON_CANCEL} ;

	fname = (doc->filename) ? g_basename(doc->filename) : _(UNTITLED);

	msg =   (char *)g_malloc(strlen(_(CLOSE_MSG)) + strlen(fname) + 6);

	sprintf(msg  , " '%s' %s ", fname, _(CLOSE_MSG));


	/* use data->flag to indicate whether or not to quit */
	data->flag = FALSE;
	data->document = doc;

	msgbox = gnome_message_box_new (msg, GNOME_MESSAGE_BOX_QUESTION,
		GNOME_STOCK_BUTTON_YES, GNOME_STOCK_BUTTON_NO,
		GNOME_STOCK_BUTTON_CANCEL, NULL);

	ret = gnome_dialog_run_and_close (GNOME_DIALOG (msgbox));

	g_free(msg);

	switch (ret) {
	case 0 :
		close_file_save_yes_sel(NULL, data);
		break;
	case 1 :
		close_file_save_no_sel(NULL, data);
		break;
	case 2 :
		close_file_save_cancel_sel(NULL, data);
		break;
	default:
		printf("popup_close_verify: ge_dialog returned %d\n", ret);
		exit(-1);
	} /* switch */
	
} /* popup_close_verify */


/* --- Notebook Tab Stuff --- */

   
void
tab_top_cb(GtkWidget *widget, gpointer cbwindow)
{
	gE_window *w = (gE_window *)cbwindow;

	mdi->tab_pos = GTK_POS_TOP;
	settings->tab_pos = GTK_POS_TOP;
}


void
tab_bot_cb(GtkWidget *widget, gpointer cbwindow)
{
	gE_window *w = (gE_window *)cbwindow;

	mdi->tab_pos =  GTK_POS_BOTTOM;
	settings->tab_pos = GTK_POS_BOTTOM;
}

void
tab_lef_cb(GtkWidget *widget, gpointer cbwindow)
{
	gE_window *w = (gE_window *)cbwindow;

	mdi->tab_pos =  GTK_POS_LEFT;
	settings->tab_pos = GTK_POS_LEFT;
}

void
tab_rgt_cb(GtkWidget *widget, gpointer cbwindow)
{
	gE_window *w = (gE_window *)cbwindow;

	mdi->tab_pos =  GTK_POS_RIGHT;
	settings->tab_pos = GTK_POS_RIGHT;
}
/*
void
tab_toggle_cb(GtkWidget *widget, gpointer cbwindow)
{
	gE_window *w = (gE_window *)cbwindow;

	w->show_tabs = !w->show_tabs;
	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(w->notebook), w->show_tabs);
}
*/

/* Scrollbar Options */
void
scrollbar_none_cb (GtkWidget *widget, gpointer cbdata)
{
int c, w;

	for (c = 0; c < g_list_length (mdi->children); c++)
  	   {
	     gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (
	        ((gE_document *) g_list_nth_data (mdi->children, c))->scrwindow[0]),
				      GTK_POLICY_NEVER,
				      GTK_POLICY_NEVER);

	     gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (
	        ((gE_document *) g_list_nth_data (mdi->children, c))->scrwindow[1]),
				      GTK_POLICY_NEVER,
				      GTK_POLICY_NEVER);
           }
         settings->scrollbar = GTK_POLICY_NEVER;
}

void
scrollbar_always_cb (GtkWidget *widget, gpointer cbdata)
{
int c, w;

	for (c = 0; c < g_list_length (mdi->children); c++)
  	   {
	     gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (
	        ((gE_document *) g_list_nth_data (mdi->children, c))->scrwindow[0]),
				      GTK_POLICY_NEVER,
				      GTK_POLICY_ALWAYS);

	     gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (
	        ((gE_document *) g_list_nth_data (mdi->children, c))->scrwindow[1]),
				      GTK_POLICY_NEVER,
				      GTK_POLICY_ALWAYS);
           }
         settings->scrollbar = GTK_POLICY_ALWAYS;

}

void
scrollbar_auto_cb (GtkWidget *widget, gpointer cbdata)
{
int c, w;

	for (c = 0; c < g_list_length (mdi->children); c++)
  	   {
	     gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (
	        ((gE_document *) g_list_nth_data (mdi->children, c))->scrwindow[0]),
				      GTK_POLICY_NEVER,
				      GTK_POLICY_AUTOMATIC);

	     gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (
	        ((gE_document *) g_list_nth_data (mdi->children, c))->scrwindow[1]),
				      GTK_POLICY_NEVER,
				      GTK_POLICY_AUTOMATIC);
           }
         settings->scrollbar = GTK_POLICY_NEVER;
}

/* ---- Auto-indent Callback(s) --- */

void
auto_indent_toggle_cb(GtkWidget *w, gpointer cbdata)
{
	gE_data *data = (gE_data *)cbdata;

	gE_window_set_auto_indent (!settings->auto_indent);
}

gint
auto_indent_cb(GtkWidget *text, char *insertion_text, int length,
	int *pos, gpointer cbdata)
{
	int i, newlines, newline_1 = 0;
	gchar *buffer, *whitespace;
	gE_data *data;
	
	data = g_malloc0 (sizeof (gE_data));
	data->temp2 = text;
	/*data->window = window;
	line_pos_cb(NULL, data);*/

	if ((length != 1) || (insertion_text[0] != '\n'))
		return FALSE;
	if (gtk_text_get_length (GTK_TEXT (text)) <=1)
		return FALSE;
	if (!settings->auto_indent)
		return FALSE;

	newlines = 0;
	for (i = *pos; i > 0; i--)
	{
		buffer = gtk_editable_get_chars (GTK_EDITABLE (text), i-1, i);
		if (buffer == NULL)
			continue;
		if (buffer[0] == 10)
		{
			if (newlines > 0)
			{
				g_free (buffer);
				buffer = NULL;
				break;
			}
			else {
				newlines++;
				newline_1 = i;
				g_free (buffer);
				buffer = NULL;
			}
		}
		g_free (buffer);
		buffer = NULL;
	}

	whitespace = g_malloc0 (newline_1 - i + 2);

	for (i = i; i <= newline_1; i++)
	{
		buffer = gtk_editable_get_chars (GTK_EDITABLE (text), i, i+1);
		if ((buffer[0] != 32) & (buffer[0] != 9))
		{
			g_free (buffer);
			buffer = NULL;
			break;
		}
		strncat (whitespace, buffer, 1);
		g_free (buffer);
	}

	if (strlen(whitespace) > 0)
	{
		i = *pos;
		gtk_text_set_point (GTK_TEXT (text), i);
		gtk_text_insert (GTK_TEXT (text), NULL, NULL, NULL,
			whitespace, strlen (whitespace));
	}
	
	g_free (whitespace);
	data->temp2 = text;
	/*line_pos_cb(NULL, data);*/
	return TRUE;
}


static void
line_pos_cb(GtkWidget *w, gE_data *data)
{
	static char col [32];
	/*GtkWidget *text = data->temp2;*/
	int x;

	sprintf (col, "%d", GTK_TEXT(gE_document_current()->text)->cursor_pos_x/7);
	
	gtk_label_set (GTK_LABEL(col_label), col);

}


gint gE_event_button_press (GtkWidget *w, GdkEventButton *event)
{
	gE_data *data;
	data = g_malloc0 (sizeof (gE_data));
/*	data->temp2 = w;
	data->window = window;*/
	line_pos_cb(NULL, data);
#ifdef DEBUG
	g_print ("you pressed a button\n");
#endif
	return FALSE;
}

/* --- Drag and Drop Callback(s) --- */

void
filenames_dropped (GtkWidget * widget,
                   GdkDragContext   *context,
                   gint              x,
                   gint              y,
                   GtkSelectionData *selection_data,
                   guint             info,
                   guint             time)
{
	GList *names, *tmp_list;
	gE_document *doc;

	names = gnome_uri_list_extract_filenames ((char *)selection_data->data);
	tmp_list = names;

	while (tmp_list) {
		doc = gE_document_new_with_file ((gchar *)tmp_list->data);
		
		tmp_list = tmp_list->next;
	}
}


/* ---- File Menu Callbacks ---- */

void file_new_cb (GtkWidget *widget, gpointer cbdata)
{
	gE_document *doc;

	gnome_app_flash (mdi->active_window, MSGBAR_FILE_NEW);
	doc = gE_document_new();
}


void window_new_cb(GtkWidget *widget, gpointer cbdata)
{
/*	gE_window *window;
	gE_data *data = (gE_data *)cbdata;

	window = gE_window_new();
	window->auto_indent = data->window->auto_indent;
	window->show_tabs = data->window->show_tabs;
	window->show_status = data->window->show_status;
	window->tab_pos = data->window->tab_pos;
	window->have_toolbar = data->window->have_toolbar;

	gE_get_settings (window);*/
	gnome_mdi_open_toplevel(mdi);
	
}


/*
 * file open callback : user selects "Ok"
 */
static void file_open_ok_sel(GtkWidget *widget, GtkFileSelection *files)
{
	gchar *filename;
	gchar *nfile;
	gchar *flash;
	struct stat sb;
	gE_document *doc;


	filename = g_malloc (strlen (gtk_file_selection_get_filename (GTK_FILE_SELECTION(osel))) + 1);
	filename = g_strdup (gtk_file_selection_get_filename(GTK_FILE_SELECTION(osel)));
	gtk_file_selection_set_filename(GTK_FILE_SELECTION(osel),"");
	
	if (filename != NULL) 
	  {
		if (stat(filename, &sb) == -1)
			return;

		if (S_ISDIR(sb.st_mode)) 
		  {
			nfile = g_malloc0(strlen (filename) + 3);
			sprintf(nfile, "%s/.", filename);
			gtk_file_selection_set_filename(GTK_FILE_SELECTION(osel), nfile);
			g_free(nfile);
			return;
		  }

		 if (gE_document_current ())
		   {
		     doc = gE_document_current();
		     
		     if (doc->filename || doc->changed)
		       doc = gE_document_new_with_file (filename);
		     else
		       gE_file_open (GE_DOCUMENT(doc), filename);
		     
		     gtk_widget_hide (GTK_WIDGET(osel));
		     return;
		   }
		 else
		   {
		     doc = gE_document_new_with_file (filename);
		     
		     gtk_widget_hide (GTK_WIDGET(osel));
		     return;
		   }
	
		

	  }
	
	gtk_widget_hide (GTK_WIDGET(osel));
	return;
	
} /* file_open_ok_sel */

/*
 * file selection dialog callback
 */
static gint
file_sel_destroy (GtkWidget *w, GtkFileSelection *fs)
{
	gtk_widget_destroy (GTK_WIDGET(fs));
	fs = NULL;
	
	return TRUE;
}

/*
 * file open callback : user selects "Cancel"
 */
static gint
file_cancel_sel (GtkWidget *w, GtkFileSelection *fs)
{
  if (GTK_WIDGET_VISIBLE(fs))
    gtk_widget_hide (GTK_WIDGET(fs));
  return TRUE;
}

void file_open_cb(GtkWidget *widget, gpointer cbdata)
{
	
	/*static GtkWidget *open_fileselector;*/
	
	if (osel == NULL)
	  osel = gtk_file_selection_new(_("Open File..."));
		

	gtk_file_selection_hide_fileop_buttons(GTK_FILE_SELECTION(osel));
	
/*	gtk_signal_connect(GTK_OBJECT(open_fs), "destroy",
			(GtkSignalFunc)file_sel_destroy, osel);*/
			
	gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(osel)->ok_button), 
				   "clicked", (GtkSignalFunc)file_open_ok_sel,
				   osel);
	gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(osel)->cancel_button),
				   "clicked", (GtkSignalFunc)file_cancel_sel,
			       osel);


/*	if (GTK_WIDGET_VISIBLE(open_fs))
		return;
*/
	gtk_widget_show(osel);
	
	return;
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
/*void		This function is defunct..
file_open_in_new_win_cb(GtkWidget *widget, gpointer cbdata)
{
	gE_document *src_doc, *dest_doc;
	gE_window *win;
	gchar *buffer;
	gE_data *data = (gE_data *)cbdata;
	int pos = 0;
	gchar *base;
	
	src_doc = gE_document_current ();
	buffer = gtk_editable_get_chars (GTK_EDITABLE (src_doc->text), 0, -1);
	win = gE_window_new ();
	dest_doc = gE_document_current ();
	dest_doc->filename = g_strdup (src_doc->filename);

	gtk_label_set (GTK_LABEL (dest_doc->tab_label), (const char *)g_basename (dest_doc->filename));

	gtk_editable_insert_text (GTK_EDITABLE (dest_doc->text), buffer, strlen (buffer), &pos);
	dest_doc->changed = src_doc->changed;
	close_doc_execute (src_doc, data);
	g_free (buffer);
}
*/

void file_save_cb(GtkWidget *widget, gpointer cbdata)
{
	gchar *fname;
	gE_document *doc;

	if (gE_document_current())
	  {
	    doc = gE_document_current();
	    if (doc->changed)
	      {
 	        fname = doc->filename;

 	    
	        if (fname == NULL)
	           file_save_as_cb(widget, NULL);
	        else
	           if ((gE_file_save(doc, doc->filename)) != 0)
	             {
		       gnome_app_flash (mdi->active_window, _("Read only file!"));
		       file_save_as_cb(widget, NULL);
        	     }
              }
          }
}


/*
 * file save-as callback : user selects "Ok"
 *
 * data->temp1 must be the file saveas dialog box
 */
static void file_saveas_ok_sel(GtkWidget *w, gE_data *data)
{
	gchar *fname = gtk_file_selection_get_filename (GTK_FILE_SELECTION(ssel));
	gE_document *doc;

	if (mdi->active_child == NULL)
	  return;
	
	doc = gE_document_current();
	
	if (fname) 
	  {
	    if (gE_file_save(doc, fname) != 0) 
	      gnome_app_flash (mdi->active_window, _("Error saving file!"));
	  }

	gtk_widget_destroy (GTK_WIDGET (ssel));
	ssel = NULL;
} /* file_saveas_ok_sel */

/*
 * destroy the "save as" dialog box
 */
static gint
file_saveas_destroy(GtkWidget *w, GtkWidget **sel)
{
	gtk_widget_destroy(*sel);
	*sel = NULL;
	
	return TRUE;
}

void
file_save_as_cb(GtkWidget *widget, gpointer cbdata)
{

	ssel = gtk_file_selection_new(_("Save As..."));

/*	gtk_signal_connect(GTK_OBJECT(safs), "destroy",
		(GtkSignalFunc)file_saveas_destroy, safs);*/
		
	gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(ssel)->ok_button),
				   "clicked", (GtkSignalFunc)file_saveas_ok_sel, 
				   NULL);
	gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(ssel)->cancel_button),
				"clicked", (GtkSignalFunc)file_saveas_destroy, 
				&ssel);

	gtk_widget_show(ssel);
}




/*
 * file close callback (used from menus.c)
 */
void
file_close_cb(GtkWidget *widget, gpointer cbdata)
{
/*	gE_data *data = (gE_data *)cbdata;

	g_assert(data != NULL);
	close_doc_common(data);*/
	
	if (mdi->active_child == NULL)
	  return;
	
	gnome_mdi_remove_child (mdi, mdi->active_child, FALSE);
}


gint file_revert_do (gE_document *doc)
{
	GnomeMessageBox *msgbox;
	gchar msg[80];
	gint ret;

	sprintf(msg, _("Are you sure you wish to revert all changes?"));
	

	msgbox = GNOME_MESSAGE_BOX (gnome_message_box_new (
	    							  msg,
	    							  GNOME_MESSAGE_BOX_QUESTION,
	    							  GNOME_STOCK_BUTTON_YES,
	    							  GNOME_STOCK_BUTTON_NO,
	    							  NULL));
	gnome_dialog_set_default (GNOME_DIALOG (msgbox), 2);
	ret = gnome_dialog_run_and_close (GNOME_DIALOG (msgbox));
	    	    
	switch (ret)
	      {
	        case 0:
	                 gE_file_open (doc, doc->filename);
	                 return TRUE;
	        case 1:
	                 return FALSE;
	        default:
	                 return FALSE;
	      }
	      

}

/* File revertion callback */
void file_revert_cb (GtkWidget *widget, gpointer cbdata)
{
	gE_document *doc;
	
	if (gE_document_current ())
	  {
	    doc = gE_document_current ();
	    
	    if (doc->filename)
	      if (doc->changed)
	        {
	          if ((file_revert_do (doc)) == 0)
	            return;
	        }
	      else
	        gnome_app_flash (mdi->active_window, _("Document Unchanged..."));
	    else
	      gnome_app_flash (mdi->active_window, _("Document Unsaved..."));
	  }
	
}

/*
 * common routine for closing a file.  will prompt for saving if the
 * file was changed.
 *
 * data->flag is set to TRUE if file closed
 *//*
static void
close_doc_common(gE_data *data)
{
	gE_document *doc;

	g_assert(data != NULL);
	g_assert(data->window != NULL);
	doc = gE_document_current();
	if (doc->changed)
		popup_close_verify(doc, data);
	else {
		close_doc_execute(NULL, data);
		data->flag = TRUE;	// indicate closed 
	}
} *//* close_doc_common */


/*
 * actually close the document.
 *//*
void
close_doc_execute(gE_document *opt_doc, gpointer cbdata)
{
	int num, numdoc;
	int *ptr;
	gchar *title;
	GtkNotebook *nb;
	gE_window *w;
	gE_document *doc;
	gE_data *data = (gE_data *)cbdata;

	g_assert(data != NULL);
	w = data->window;
	g_assert(w != NULL);
	nb = GTK_NOTEBOOK(w->notebook);
	g_assert(nb != NULL);
	//
	// Provide the option to pass the specific document as an argument
	// instead of always going with the current document - needed for
	// the plugins api...
	//
	doc = (opt_doc == NULL) ? gE_document_current() : opt_doc;
	g_assert(doc != NULL);

	// if all we have is a blank, Untitled doc, then return immediately 
	if (!doc->changed && g_list_length(nb->children) == 1 &&
		doc->filename == NULL)
		return;

	// Remove document from our hash tables 
	ptr = g_hash_table_lookup(doc_pointer_to_int, doc);
	g_hash_table_remove(doc_int_to_pointer, ptr);
	g_hash_table_remove(doc_pointer_to_int, doc);
	g_free (ptr);

	// remove notebook entry and item from document list //
	num = gtk_notebook_current_page(nb);
	gtk_notebook_remove_page(nb, num);
	w->documents = g_list_remove(w->documents, doc);
	if (doc->filename)
		g_free(doc->filename);
	if (doc->sb)
		g_free(doc->sb);
	g_free(doc);

	// if files list window present, remove corresponding entry 
	flw_remove_entry(w, num);

	// echo message to user 
	gE_msgbar_set(w, MSGBAR_FILE_CLOSED);

	num = g_list_length(nb->children);
	numdoc = g_list_length(w->documents);
	g_assert(num == numdoc);

	//
	// we always have at least one document (e.g., "Untitled").
	// so if we just closed the last document, create "Untitled".
	//
	if (num < 1) {
		g_list_free(w->documents);
		w->documents = NULL;
		gE_document_new(w);
		if (w->files_list_window)
			flw_append_entry(w, doc,
				g_list_length(nb->children) - 1, NULL);
	}
	// Set the title of the window to Current Document - GEDIT_ID 
	
	title = g_malloc0(strlen(GEDIT_ID) +
		strlen(GTK_LABEL(gE_document_current()->tab_label)->label) + 4);
	sprintf(title, "%s - %s",
	 GTK_LABEL(gE_document_current()->tab_label)->label,
	 GEDIT_ID);
	gtk_window_set_title(GTK_WINDOW(w->window), title);
	g_free(title);

} *//* close_doc_execute */




/*
 * quits gEdit by closing all windows.  only quits if all windows closed.
 */
void
file_quit_cb(GtkWidget *widget, gpointer cbdata)
{
	plugin_save_list();

	if (gnome_mdi_remove_all (mdi, FALSE))
	  gtk_object_destroy (GTK_OBJECT (mdi));
	else
	  return;
	
	gtk_exit(0);	/* should not reach here */
}


/* ---- Clipboard Callbacks ---- */

void
edit_cut_cb(GtkWidget *widget, gpointer cbdata)
{
	gE_data *data = (gE_data *)cbdata;

	gtk_editable_cut_clipboard(GTK_EDITABLE(
		gE_document_current()->text));

	/*gE_msgbar_set(data->window, MSGBAR_CUT);*/
	gnome_app_flash (mdi->active_window, MSGBAR_CUT);
}

void
edit_copy_cb(GtkWidget *widget, gpointer cbdata)
{
	gE_data *data = (gE_data *)cbdata;

	gtk_editable_copy_clipboard(
		GTK_EDITABLE(gE_document_current()->text));
/*	gE_msgbar_set(data->window, MSGBAR_COPY);*/
	gnome_app_flash (mdi->active_window, MSGBAR_COPY);
}
	
void
edit_paste_cb(GtkWidget *widget, gpointer cbdata)
{
	gE_data *data = (gE_data *)cbdata;

	gtk_editable_paste_clipboard(
		GTK_EDITABLE(gE_document_current()->text));

/*	gE_msgbar_set(data->window, MSGBAR_PASTE);*/
	gnome_app_flash (mdi->active_window, MSGBAR_PASTE);
}

void
edit_selall_cb(GtkWidget *widget, gpointer cbdata)
{
	gE_data *data = (gE_data *)cbdata;

	gtk_editable_select_region(
		GTK_EDITABLE(gE_document_current()->text), 0,
		gtk_text_get_length(
			GTK_TEXT(gE_document_current()->text)));
/*	gE_msgbar_set(data->window, MSGBAR_SELECT_ALL);*/
	gnome_app_flash (mdi->active_window, MSGBAR_SELECT_ALL);
}


/* Add a file to the Recent-used list... */

void recent_add (char *filename)
{
	gnome_history_recently_used (filename, "text/plain", "gEdit", "");
	
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

void recent_update (GnomeApp *app)
{
	GList *filelist = NULL;
	GList *dirlist = NULL;
	
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
	
	recent_update_menus (app, filelist);
}

/* Actually updates the recent-used menu... */

static void
recent_update_menus (GnomeApp *app, GList *recent_files)
{
	GnomeUIInfo *menu;
	gE_data *data;
	gchar *path;
	int i;

	if (settings->num_recent)
	  gnome_app_remove_menu_range (app, "_File/", 6, settings->num_recent + 1);

	if (recent_files == NULL)
		return;


	/* insert a separator at the beginning */
	
	menu = g_malloc0 (2 * sizeof (GnomeUIInfo));
	path = g_new (gchar, strlen (_("_File")) + strlen ("<Separator>") + 3 );
	sprintf (path, "%s/%s", _("_File"), "<Separator>");
	menu->type = GNOME_APP_UI_SEPARATOR;

	(menu + 1)->type = GNOME_APP_UI_ENDOFINFO;
	gnome_app_insert_menus (GNOME_APP(app), path, menu);


	for (i = g_list_length (recent_files) - 1; i >= 0;  i--)
	{
		menu = g_malloc0 (2 * sizeof (GnomeUIInfo));
	
		data = g_malloc0 (sizeof (gE_data));
		data->temp1 = g_strdup (g_list_nth_data (recent_files, i));
		/*data->window = window;*/
		menu->label = g_new (gchar, 
			strlen (g_list_nth_data (recent_files, i)) + 5);
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
	
		gnome_app_insert_menus (GNOME_APP(app), path, menu);
		g_free (g_list_nth_data (recent_files, i));
	}
	settings->num_recent = g_list_length (recent_files);
	g_list_free (recent_files);


}

static void
recent_cb(GtkWidget *w, gE_data *data)
{
	gE_document *doc = gE_document_current ();
	
	if (doc->filename || doc->changed)
	  doc = gE_document_new_with_file (data->temp1);
	else		
	  gE_file_open (doc, data->temp1);
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
	gint n;
	gE_document *temp;
	GnomeMDIChild *child;
	/*GnomeMDIChild *temp = NULL;*/
	gE_data *data;
	GtkWidget *text;
	GList *view;

	data = g_malloc0 (sizeof (gE_data));
	line_pos_cb(NULL, data);
	
	g_free (data);
	
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

/*	FIXME : This looks correct, but it isn't... I can't figure why it isnt working properly
		( * - denotes comment, cant have comemts within comments )
	child = GNOME_MDI_CHILD (doc);
	view = child->views;
	
	while (view)
	   {
	     g_print ("doc_insert_text: n = %d views = %d\n",n, g_list_length (GNOME_MDI_CHILD (doc)->views));
	     temp = (gE_document *)view->data;
g_warning ("temp* has been set..");

	     
	     *gtk_text_freeze (GTK_TEXT(GE_DOCUMENT(temp)->text)); *
	     gtk_text_freeze (GTK_TEXT(temp->text));
g_warning ("temp->text has been frozen..");	     
	    * gtk_editable_insert_text (GTK_EDITABLE(GE_DOCUMENT(temp)->text), buffer, length, &position);*
	    gtk_editable_insert_text (GTK_EDITABLE(temp->text), buffer, length, &position);
	     
	    * gtk_text_thaw (GTK_TEXT(GE_DOCUMENT(temp)->text));*
	    gtk_text_thaw (GTK_TEXT(temp->text));
	    
	    view = g_list_next (view);
	   }
*/	   
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
	gE_data *data;

	data = g_malloc0 (sizeof (gE_data));
	line_pos_cb(NULL, data);
	
	g_free (data);
		
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

void options_toggle_split_screen_cb (GtkWidget *widget, gpointer data)
{
	gE_document *doc = gE_document_current ();
	gint visible;

	if (!doc->split_parent)
		return;

	gE_document_set_split_screen
		(doc, !GTK_WIDGET_VISIBLE (doc->split_parent));
}


void options_toggle_read_only_cb (GtkWidget *widget, gpointer data)
{
	gE_document *doc = gE_document_current ();
	gE_document_set_read_only (doc, !doc->read_only);
}

void options_toggle_word_wrap_cb (GtkWidget *widget, gpointer data)
{
	gE_document *doc = gE_document_current ();
	gE_document_set_word_wrap (doc, !doc->word_wrap);
}

void options_toggle_line_wrap_cb (GtkWidget *widget, gpointer data)
{
	gE_document *doc = gE_document_current ();
	gE_document_set_line_wrap (doc, !doc->line_wrap);
}

void options_toggle_status_bar_cb (GtkWidget *w, gpointer data)
{
	gE_window_set_status_bar (!settings->show_status);
}
