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
#include <sys/types.h>
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
#include "gE_view.h"
#include "commands.h"
#include "gE_mdi.h"
#include "gE_window.h"
#include "gE_prefs.h"
#include "gE_files.h"
#include "gE_plugin_api.h"
/*#include "dialog.h"*/
#include "search.h"

static void close_file_save_yes_sel (GtkWidget *w, gE_data *data);
static void close_file_save_cancel_sel(GtkWidget *w, gE_data *data);
static void close_file_save_no_sel(GtkWidget *w, gE_data *data);
static void close_doc_common(gE_data *data);
static void close_window_common(gE_window *w);
static gint file_saveas_destroy(GtkWidget *w, GtkWidget **sel);
static gint file_cancel_sel (GtkWidget *w, GtkFileSelection *fs);
static void file_sel_destroy (GtkWidget *w, GtkFileSelection *fs);
static void recent_update_menus (GnomeApp *app, GList *recent_files);
static void recent_cb(GtkWidget *w, gE_data *data);

static GtkWidget *open_fs, *save_fs;
GtkWidget *ssel = NULL;
GtkWidget *osel = NULL;
GtkWidget *col_label;
gchar *oname = NULL;

/*
 * file save callback : user selected "No"
 */
static void
close_file_save_no_sel(GtkWidget *w, gE_data *data)
{

	g_assert(data != NULL);
	
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
 	    
	} else {
		
	 int error;

	 error = gE_file_save(doc, doc->filename);
	 if (!error) {
			
	   data->temp1 = NULL;
	   file_close_cb (w, data);
	   data->temp2 = NULL;
	   data->flag = TRUE;
	   
	 } else {
	  
	  data->flag = FALSE;
	 
	 }
	 
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
			printf("popup_close_verify: returned %d\n", ret);
			exit(-1);
		
	} /* switch */
	
} /* popup_close_verify */


/* popup to handle new file creation from nothing. */
int
popup_create_new_file (GtkWidget *w, gchar *title)
{
	GtkWidget *msgbox;
	gE_document *doc;
	int ret;
	char msg[100];
	
	sprintf(msg  , "The file %s does not exist. Would you like to create it?", title);


	msgbox = gnome_message_box_new (msg, GNOME_MESSAGE_BOX_QUESTION,
								GNOME_STOCK_BUTTON_YES, GNOME_STOCK_BUTTON_NO,
								NULL);
	
	ret = gnome_dialog_run_and_close (GNOME_DIALOG (msgbox));

	switch (ret) {
	/* yes */
	case 0 : 
			doc = gE_document_new_with_title (title);
			gnome_mdi_add_child (mdi, GNOME_MDI_CHILD (doc));
	        	gnome_mdi_add_view (mdi, GNOME_MDI_CHILD (doc));
		
                	if ((gE_file_save(doc, title)) != 0) {
                	
		  	  gnome_app_flash (mdi->active_window, _("Could not save file!"));
		  	  return FALSE;
			
			}
			return TRUE;
	/* no */
	case 1 : 
			return FALSE;

	/* error */
	default: 
			g_print ("and error occured %d\n", ret);
			exit(-1);
			
	} 
}



/* --- Notebook Tab Stuff --- */

void tab_pos (GtkPositionType pos)
{
	
	gint i;
	GnomeApp *app;
	GtkWidget *book;
	
	
	for (i = 0; i < g_list_length (mdi->windows); i++) {
	
	  app = g_list_nth_data (mdi->windows, i);
	  
	  book = app->contents;
	  
	  gtk_notebook_set_tab_pos (GTK_NOTEBOOK(book), pos);
	  
	}
	
}
   
void
tab_top_cb(GtkWidget *widget, gpointer cbwindow)
{

	mdi->tab_pos = GTK_POS_TOP;
	settings->tab_pos = GTK_POS_TOP;
	
	tab_pos (GTK_POS_TOP);
	
}


void
tab_bot_cb(GtkWidget *widget, gpointer cbwindow)
{

	mdi->tab_pos =  GTK_POS_BOTTOM;
	settings->tab_pos = GTK_POS_BOTTOM;
	
	tab_pos (GTK_POS_BOTTOM);
}

void
tab_lef_cb(GtkWidget *widget, gpointer cbwindow)
{

	mdi->tab_pos =  GTK_POS_LEFT;
	settings->tab_pos = GTK_POS_LEFT;
	
	tab_pos (GTK_POS_LEFT);
	
}

void
tab_rgt_cb(GtkWidget *widget, gpointer cbwindow)
{

	mdi->tab_pos =  GTK_POS_RIGHT;
	settings->tab_pos = GTK_POS_RIGHT;
	
	tab_pos (GTK_POS_RIGHT);
	
}
/*
void
tab_toggle_cb(GtkWidget *widget, gpointer cbwindow)
{

	w->show_tabs = !w->show_tabs;
	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(w->notebook), w->show_tabs);
}
*/

/* Scrollbar Options */
void
scrollbar_none_cb (GtkWidget *widget, gpointer cbdata)
{
	int c, w;

	for (c = 0; c < g_list_length (mdi->children); c++) {
	
	     gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (
	        ((gE_view *) g_list_nth_data (mdi->children, c))->scrwindow[0]),
				      GTK_POLICY_NEVER,
				      GTK_POLICY_NEVER);

	     gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (
	        ((gE_view *) g_list_nth_data (mdi->children, c))->scrwindow[1]),
				      GTK_POLICY_NEVER,
				      GTK_POLICY_NEVER);
        
        }
         
        settings->scrollbar = GTK_POLICY_NEVER;
        
}

void
scrollbar_always_cb (GtkWidget *widget, gpointer cbdata)
{
	int c, w;

	for (c = 0; c < g_list_length (mdi->children); c++) {
	
	     gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (
	        ((gE_view *) g_list_nth_data (mdi->children, c))->scrwindow[0]),
				      GTK_POLICY_NEVER,
				      GTK_POLICY_ALWAYS);

	     gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (
	        ((gE_view *) g_list_nth_data (mdi->children, c))->scrwindow[1]),
				      GTK_POLICY_NEVER,
				      GTK_POLICY_ALWAYS);
        }
        
        settings->scrollbar = GTK_POLICY_ALWAYS;

}

void
scrollbar_auto_cb (GtkWidget *widget, gpointer cbdata)
{
	int c, w;

	for (c = 0; c < g_list_length (mdi->children); c++) {
	
	     gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (
	        ((gE_view *) g_list_nth_data (mdi->children, c))->scrwindow[0]),
				      GTK_POLICY_NEVER,
				      GTK_POLICY_AUTOMATIC);

	     gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (
	        ((gE_view *) g_list_nth_data (mdi->children, c))->scrwindow[1]),
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
		gnome_mdi_add_child (mdi, GNOME_MDI_CHILD (doc));
	        gnome_mdi_add_view (mdi, GNOME_MDI_CHILD (doc));
	        
		tmp_list = tmp_list->next;
		
	}
}


/* ---- File Menu Callbacks ---- */

void file_new_cb (GtkWidget *widget, gpointer cbdata)
{
	gE_document *doc;

	gnome_app_flash (mdi->active_window, MSGBAR_FILE_NEW);
	doc = gE_document_new();
	
	gnome_mdi_add_child (mdi, GNOME_MDI_CHILD (doc));
	gnome_mdi_add_view (mdi, GNOME_MDI_CHILD (doc));
	
}


void window_new_cb(GtkWidget *widget, gpointer cbdata)
{

	/* I'm not sure about this.. it appears correct */
	gnome_mdi_open_toplevel(mdi);
	
}


/*
 * file open callback : user selects "Ok"
 */
static void file_open_ok_sel (GtkWidget *widget, GtkFileSelection *files)
{
	gchar *filename;
	gchar *nfile;
	gchar *flash;
	struct stat sb;
	gE_document *doc;

	filename = g_malloc (strlen (gtk_file_selection_get_filename (GTK_FILE_SELECTION(osel))) + 1);
	filename = g_strdup (gtk_file_selection_get_filename(GTK_FILE_SELECTION(osel)));
	gtk_file_selection_set_filename(GTK_FILE_SELECTION(osel),"");
	
	if (filename != NULL) {
		
	   if (stat(filename, &sb) == -1)
	     return;

	   if (S_ISDIR(sb.st_mode)) {
	   
	      nfile = g_malloc0(strlen (filename) + 3);
	      sprintf(nfile, "%s/.", filename);
	      
	      gtk_file_selection_set_filename(GTK_FILE_SELECTION(osel), nfile);
	      
	      g_free(nfile);
	      
	      return;
	   }

	   if (gE_document_current ()) {
	   
	     doc = gE_document_current();
		     
	     if (doc->filename || doc->changed) {
	     
	       doc = gE_document_new_with_file (filename);
	       gnome_mdi_add_child (mdi, GNOME_MDI_CHILD (doc));
	       gnome_mdi_add_view (mdi, GNOME_MDI_CHILD (doc));
	        		 
	     } else {
	      
	      gE_file_open (GE_DOCUMENT(doc), filename);
	     
	     }
		     
	     gtk_widget_hide (GTK_WIDGET(osel));
	     
	     return;
	     
	   } else {
		     
	    doc = gE_document_new_with_file (filename);
	    gnome_mdi_add_child (mdi, GNOME_MDI_CHILD (doc));
	    gnome_mdi_add_view (mdi, GNOME_MDI_CHILD (doc));
		     
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
static void
file_sel_destroy (GtkWidget *w, GtkFileSelection *fs)
{

	gtk_widget_hide (w);
	
		
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
	
	if (osel == NULL) {
	
	  osel = gtk_file_selection_new(_("Open File..."));
		

	  gtk_file_selection_hide_fileop_buttons(GTK_FILE_SELECTION(osel));
	
	  gtk_signal_connect(GTK_OBJECT(osel), "delete_event",
				     (GtkSignalFunc)file_sel_destroy,
				     osel);
			
	  gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(osel)->ok_button), 
				     "clicked", (GtkSignalFunc)file_open_ok_sel,
				     osel);
				      
	  gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(osel)->cancel_button),
				     "clicked", (GtkSignalFunc)file_cancel_sel,
			         osel);
	
	}


/*	if (GTK_WIDGET_VISIBLE(open_fs))
		return;
*/
	gtk_widget_show(osel);
	return;
	
}


void file_save_cb(GtkWidget *widget, gpointer cbdata)
{

	gchar *fname;
	gE_document *doc;
	gE_view *view;
	gchar *title;

	if (gE_document_current()) {
	
	  doc = gE_document_current();
	  view = GE_VIEW (mdi->active_view);
	    
	  if (doc->changed) {
	  
 	    fname = doc->filename;

 	    if (fname == NULL) {
 	    
		title = g_strdup_printf ("%s", GNOME_MDI_CHILD(doc)->name);
		file_save_as_cb(widget, title);
	    } else
	      if ((gE_file_save(doc, doc->filename)) != 0) {
	      
		gnome_app_flash (mdi->active_window, _("Read only file!"));
		file_save_as_cb(widget, NULL);
       	      
       	      }
          
          }
        
        }
        
}

/*
 * file save-all callback : user selects "Save All"
 *
 * saves all open and changed files
 */
void 
file_save_all_cb(GtkWidget *widget, gpointer cbdata)
{

	int i;
	gchar *fname, *title;
	gE_document *doc;

        for (i = 0; i < g_list_length (mdi->children); i++) {

          doc = (gE_document *)g_list_nth_data (mdi->children, i);
          if (doc->changed) {
          
            fname = doc->filename;

            if (fname == NULL) {
            
	      title = g_strdup_printf ("%s", GNOME_MDI_CHILD(doc)->name);
	      /*gtk_label_get((GtkLabel *)doc->tab_label, &title);*/

              file_save_all_as_cb(widget, title);
            
            } else {
              
             if ((gE_file_save(doc, doc->filename)) != 0) {
             
               gnome_app_flash (mdi->active_window, _("Read only file!"));
               file_save_all_as_cb(widget, NULL);
               
             }
             
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
	
	if (fname) {
	
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

	gchar *title;
	
	title = g_strdup_printf (_("Save %s As..."), (gchar*) cbdata);

	ssel = gtk_file_selection_new(title);

	gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(ssel)->ok_button),
				   "clicked", (GtkSignalFunc)file_saveas_ok_sel, 
				   NULL);

	gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(ssel)->cancel_button),
				"clicked", (GtkSignalFunc)file_saveas_destroy, 
				&ssel);


	gtk_widget_show(ssel);

}

/*
 * file save-all-as callback : user selects "Ok"
 *
 * data->temp1 must be the file saveas dialog box
 */
static void file_save_all_as_ok_sel(GtkWidget *w, GtkFileSelection *fs)
{

	gchar *fname = gtk_file_selection_get_filename (GTK_FILE_SELECTION(fs));
	gE_document *doc;

	if (mdi->active_child == NULL)
	  return;
	
	doc = gE_document_current();
	
	if (fname) {
	
	  if (gE_file_save(doc, fname) != 0) 
	    gnome_app_flash (mdi->active_window, _("Error saving file!"));
	    
	}

	gtk_widget_destroy (GTK_WIDGET (fs)); 
	fs = NULL;
	
} 

/*
 * destroy the "save all as" dialog box
 */
static gint file_save_all_as_destroy(GtkWidget *w, GtkFileSelection *fs)
{

	gtk_widget_destroy (GTK_WIDGET (fs)); 
	fs = NULL;
	
	return TRUE;
	
}

/*
 * save all as callback
 */
void 
file_save_all_as_cb(GtkWidget *widget, gpointer cbdata)
{

	GtkWidget *fs = NULL; 
	gchar *title;

	title = g_strdup_printf(_("Save %s As ..."), (gchar *)cbdata);
	fs = gtk_file_selection_new(title);

	
	gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(fs)->ok_button),
						"clicked", (GtkSignalFunc)file_save_all_as_ok_sel, fs);
						
	gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(fs)->cancel_button),
						"clicked", (GtkSignalFunc)file_save_all_as_destroy, fs);


	gtk_widget_show(fs);
	
}



/*
 * file close callback (used from menus.c)
 */
void
file_close_cb(GtkWidget *widget, gpointer cbdata)
{

	gE_document *doc;
	
	if (mdi->active_child == NULL)
	  return;
	
	if (gnome_mdi_remove_child (mdi, mdi->active_child, FALSE)) {
	
	  if (mdi->active_child == NULL) {
	  
	    if (!settings->close_doc) {
	    
	      doc = gE_document_new ();
	      gnome_mdi_add_child (mdi, GNOME_MDI_CHILD (doc));
	      gnome_mdi_add_view (mdi, GNOME_MDI_CHILD (doc));
	    
	    } else {
	    
	     g_print ("Er.. Unimplemented alternative!\n");
	    
	    }
	     
	  }
	  
	}
	
}

/*
 * file close all callback (used from menus.c)
 */
void
file_close_all_cb(GtkWidget *widget, gpointer cbdata)
{

	gE_document *doc;

	if (gnome_mdi_remove_all (mdi, FALSE)) {
	
          if (mdi->active_child == NULL) {
          
	    /* if there are no open documents create a blank one */
	    if (g_list_length(mdi->children) == 0) {

		doc = gE_document_new ();
		gnome_mdi_add_child (mdi, GNOME_MDI_CHILD (doc));
		gnome_mdi_add_view  (mdi, GNOME_MDI_CHILD (doc));
	    }

	  }
	  
	}

}

gint file_revert_do (gE_document *doc)
{

	GnomeMessageBox *msgbox;
	gchar msg[80];
	gint ret;

	sprintf(msg, _("Are you sure you wish to revert all changes?\n(%s)"),
		doc->filename);
	

	msgbox = GNOME_MESSAGE_BOX (gnome_message_box_new (
	    							  msg,
	    							  GNOME_MESSAGE_BOX_QUESTION,
	    							  GNOME_STOCK_BUTTON_YES,
	    							  GNOME_STOCK_BUTTON_NO,
	    							  NULL));
	    							  
	    							  
	gnome_dialog_set_default (GNOME_DIALOG (msgbox), 2);
	ret = gnome_dialog_run_and_close (GNOME_DIALOG (msgbox));
	    	    
	switch (ret) {
	        
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
	
	if (gE_document_current ()) {
	
	  doc = gE_document_current ();
	    
	  if (doc->filename) {
	  
	    if (doc->changed) {
	    
	      if ((file_revert_do (doc)) == 0)
	        return;
	    
	    } else
	     gnome_app_flash (mdi->active_window, _("Document Unchanged..."));
	  
	  } else {
	      
	   gnome_app_flash (mdi->active_window, _("Document Unsaved..."));
	    
	  }
	  
	}
	
}


/*
 * quits gEdit by closing all windows.  only quits if all windows closed.
 */
void
file_quit_cb(GtkWidget *widget, gpointer cbdata)
{

	plugin_save_list();

	gE_save_settings ();

	if (gnome_mdi_remove_all (mdi, FALSE)) {
	
	  gtk_object_destroy (GTK_OBJECT (mdi));
	  
	} else {
	
	  return;
	
	}
	
	gtk_exit(0);	/* should not reach here */
	
}


/* ---- Clipboard Callbacks ---- */

void
edit_cut_cb(GtkWidget *widget, gpointer cbdata)
{

	gE_data *data = (gE_data *)cbdata;

	gtk_editable_cut_clipboard(GTK_EDITABLE(
		GE_VIEW (mdi->active_view)->text));

	gnome_app_flash (mdi->active_window, MSGBAR_CUT);
	
}

void
edit_copy_cb(GtkWidget *widget, gpointer cbdata)
{

	gE_data *data = (gE_data *)cbdata;

	gtk_editable_copy_clipboard(
		GTK_EDITABLE(GE_VIEW (mdi->active_view)->text));

	gnome_app_flash (mdi->active_window, MSGBAR_COPY);
	
}
	
void
edit_paste_cb(GtkWidget *widget, gpointer cbdata)
{

	gE_data *data = (gE_data *)cbdata;

	gtk_editable_paste_clipboard(
		GTK_EDITABLE(GE_VIEW (mdi->active_view)->text));

	gnome_app_flash (mdi->active_window, MSGBAR_PASTE);
	
}

void
edit_selall_cb(GtkWidget *widget, gpointer cbdata)
{

	gE_data *data = (gE_data *)cbdata;

	gtk_editable_select_region(
		GTK_EDITABLE(GE_VIEW (mdi->active_view)->text), 0,
		gtk_text_get_length( GTK_TEXT(GE_VIEW (mdi->active_view)->text)));

	gnome_app_flash (mdi->active_window, MSGBAR_SELECT_ALL);
	
}


/* Add a file to the Recent-used list... */

void recent_add (char *filename)
{

	gnome_history_recently_used (filename, "text/plain", "gEdit", "");
	
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
	
	if (g_list_length (gnome_recent_list) > 0) {
	
	   for (i = g_list_length (gnome_recent_list) - 1; i >= 0; i--) {
	   
	     histentry = g_list_nth_data (gnome_recent_list, i);
	     
	     if (strcmp ("gEdit", histentry->creator) == 0) {
	     
	       /* This is to make sure you don't have more than one
		  file of the same name in the recent list
	       */
	      
	       if (g_list_length (filelist) > 0) {
	      
		 for (j = g_list_length (filelist) - 1; j >= 0; j--) {
		
		  if (strcmp (histentry->filename, g_list_nth_data (filelist, j)) == 0) {
		 
		    filelist = g_list_remove (filelist, g_list_nth_data (filelist, j));
		  }
		
		 }
		
	       }

	       filename = g_malloc0 (strlen (histentry->filename) + 1);
	       strcpy (filename, histentry->filename);
	       filelist = g_list_append (filelist, filename);

	       /* For recent-directories, not yet fully implemented...
		   
	       end_path = strrchr (histentry->filename, '/');
	       if (end_path) {
		   
		 for (i = 0; i < strlen (histentry->filename); i++)
		  if ((histentry->filename + i) == end_path)
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
	  gnome_app_remove_menu_range (app, _("_File/"), 6, settings->num_recent + 1);

	if (recent_files == NULL)
	  return;


	/* insert a separator at the beginning */
	
	menu = g_malloc0 (2 * sizeof (GnomeUIInfo));
	path = g_new (gchar, strlen (_("_File")) + strlen ("<Separator>") + 3 );
	sprintf (path, "%s/%s", _("_File"), "<Separator>");
	menu->type = GNOME_APP_UI_SEPARATOR;

	(menu + 1)->type = GNOME_APP_UI_ENDOFINFO;
	gnome_app_insert_menus (GNOME_APP(app), path, menu);


	for (i = g_list_length (recent_files) - 1; i >= 0;  i--) {
	
	 menu = g_malloc0 (2 * sizeof (GnomeUIInfo));
	
	 data = g_malloc0 (sizeof (gE_data));
	 data->temp1 = g_strdup (g_list_nth_data (recent_files, i));
	
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
	
	 gnome_app_insert_menus (GNOME_APP(app), path, menu);
	 g_free (g_list_nth_data (recent_files, i));
	 
	}
	
	settings->num_recent = g_list_length (recent_files);
	g_list_free (recent_files);

}

static void
recent_cb(GtkWidget *w, gE_data *data)
{

	gE_document *doc;
	
	if ((doc = gE_document_current ())) {
	
	  if (doc->filename || GE_VIEW(mdi->active_view)->changed) {
	  
	    doc = gE_document_new_with_file (data->temp1);
	    gnome_mdi_add_child (mdi, GNOME_MDI_CHILD (doc));
	    gnome_mdi_add_view (mdi, GNOME_MDI_CHILD (doc));
	    
	  } else {
	  
	   /*doc = gE_document_new_with_file (data->temp1);
	   gnome_mdi_add_child (mdi, GNOME_MDI_CHILD (doc));
	   gnome_mdi_add_view (mdi, GNOME_MDI_CHILD (doc));*/
	   gE_file_open (doc, data->temp1);
	   
	  }
	
	}
	
}


void options_toggle_split_screen_cb (GtkWidget *widget, gpointer data)
{

	gE_view *view = GE_VIEW (mdi->active_view);
	gint visible;

	if (!view->split_parent)
	  return;

	gE_view_set_split_screen
		(view, !GTK_WIDGET_VISIBLE (view->split_parent));
		
}


void options_toggle_read_only_cb (GtkWidget *widget, gpointer data)
{

	gE_view *view = GE_VIEW (mdi->active_view);
	
	gE_view_set_read_only (view, !view->read_only);
	
}

void options_toggle_word_wrap_cb (GtkWidget *widget, gpointer data)
{

	gE_view *view = GE_VIEW (mdi->active_view);
	
	gE_view_set_word_wrap (view, !view->word_wrap);

}

void options_toggle_line_wrap_cb (GtkWidget *widget, gpointer data)
{

	gE_view *view = GE_VIEW (mdi->active_view);

	gE_view_set_line_wrap (view, !view->line_wrap);

}

void options_toggle_status_bar_cb (GtkWidget *w, gpointer data)
{

	gE_window_set_status_bar (!settings->show_status);

}
