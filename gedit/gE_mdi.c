/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* vi:set ts=8 sts=0 sw=8:
 *
 * gEdit
 * Copyright (C) 1998, 1999 Alex Roberts and Evan Lawrence
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
#include <sys/stat.h>
#include "main.h"
#include "gE_undo.h"
#include "gE_prefs.h"
#include "gE_window.h"
#include "gE_view.h"
#include "gE_files.h"
#include "commands.h"
#include "search.h"
#include "gE_mdi.h"
#include "gE_print.h"
#include "menus.h"

/*
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <gtk/gtk.h>
#include <glib.h>
*/

static void 	  gedit_document_class_init (gedit_document_class *);
static void 	  gedit_document_init (gedit_document *);
static GtkWidget *gedit_document_create_view (GnomeMDIChild *);
static void	  gedit_document_destroy (GtkObject *);
/*static void	  gedit_document_real_changed (gedit_document *, gpointer); */
static gchar *gedit_document_get_config_string (GnomeMDIChild *child);
       gchar *get_untitled_as_string ();


/* MDI Menus Stuff */

#define GE_DATA		1
#define GE_WINDOW	2

GnomeUIInfo gedit_edit_menu [] = {


	GNOMEUIINFO_MENU_UNDO_ITEM (gedit_undo_do, NULL),
	GNOMEUIINFO_MENU_REDO_ITEM (gedit_undo_redo, NULL),

	GNOMEUIINFO_SEPARATOR,

        GNOMEUIINFO_MENU_CUT_ITEM(edit_cut_cb, NULL),

        GNOMEUIINFO_MENU_COPY_ITEM(edit_copy_cb, NULL),

	GNOMEUIINFO_MENU_PASTE_ITEM(edit_paste_cb, NULL),

	GNOMEUIINFO_MENU_SELECT_ALL_ITEM(edit_selall_cb, NULL),


	GNOMEUIINFO_SEPARATOR,

	{ GNOME_APP_UI_ITEM, N_("Find _Line..."),
	  N_("Search for a line"),

	  goto_line_cb, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_SEARCH },

	{ GNOME_APP_UI_ITEM, N_("Find _In Files..."),
	  N_("Find text in all open files"),
	  find_in_files_cb, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_SEARCH
	},

	GNOMEUIINFO_MENU_FIND_ITEM(search_cb, NULL),

	GNOMEUIINFO_MENU_REPLACE_ITEM(replace_cb, NULL),
	
	GNOMEUIINFO_END
	
};

GnomeUIInfo view_menu[] = {

	GNOMEUIINFO_ITEM_NONE (N_("_Add View"),
					   N_("Add a new view of the document"), gedit_add_view),
	GNOMEUIINFO_ITEM_NONE (N_("_Remove View"),
					   N_("Remove view of the document"), gedit_remove_view),
	GNOMEUIINFO_END
	
};

GnomeUIInfo doc_menu[] = {

	GNOMEUIINFO_MENU_EDIT_TREE(gedit_edit_menu),
	GNOMEUIINFO_MENU_VIEW_TREE(view_menu),
	GNOMEUIINFO_END
	
};



enum {
	LAST_SIGNAL
};

typedef void (*gedit_document_signal) (GtkObject *, gpointer, gpointer);

static GnomeMDIChildClass *parent_class = NULL;

GtkType
gedit_document_get_type (void)
{
	static GtkType doc_type = 0;
	
	if (!doc_type) {
	  	
	  static const GtkTypeInfo doc_info = {
	  	"gedit_document",
	  	sizeof (gedit_document),
	  	sizeof (gedit_document_class),
	  	(GtkClassInitFunc) gedit_document_class_init,
	  	(GtkObjectInitFunc) gedit_document_init,
	  	(GtkArgSetFunc) NULL,
	  	(GtkArgGetFunc) NULL,
	  
	  };
	  
	  doc_type = gtk_type_unique (gnome_mdi_child_get_type (), &doc_info);
	  
	}
	  
	return doc_type;
}

static GtkWidget *
gedit_document_create_view (GnomeMDIChild *child)
{

	GtkWidget *new_view;
	
	new_view = gedit_view_new (GE_DOCUMENT (child));


	gedit_view_set_font (GE_VIEW(new_view), settings->font);
	gtk_widget_queue_resize (GTK_WIDGET (new_view));
	
	return new_view;

}

static void gedit_document_destroy (GtkObject *obj)
{

	gedit_document *doc;
	
	doc = GE_DOCUMENT(obj);
	
	if (doc->filename)
	  g_free (doc->filename);
	
	g_free (doc->buf->str);
	doc->buf = NULL;
	
	if (doc->undo)
	  g_list_free (doc->undo);
	
	if (doc->redo)
	  g_list_free (doc->redo);
	
	if (GTK_OBJECT_CLASS (parent_class)->destroy)
	  (* GTK_OBJECT_CLASS (parent_class)->destroy)(GTK_OBJECT (doc));

}

static void gedit_document_class_init (gedit_document_class *class)
{

	GtkObjectClass 		*object_class;
	GnomeMDIChildClass	*child_class;
	
	object_class = (GtkObjectClass*)class;
	child_class = GNOME_MDI_CHILD_CLASS (class);
	
	/* blarg.. signals stuff.. doc_changed? FIXME 
	
	gtk_object_class_add_signals (object_class, gedit_document_signals, LAST_SIGNAL);
	*/
	
	object_class->destroy = gedit_document_destroy;
	
	child_class->create_view = (GnomeMDIChildViewCreator)(gedit_document_create_view);
	child_class->get_config_string = (GnomeMDIChildConfigFunc)(gedit_document_get_config_string);
	
	/*class->document_changed = gedit_document_real_changed;*/
	
	parent_class = gtk_type_class (gnome_mdi_child_get_type ());
	
}

void
gedit_document_init (gedit_document *doc)
{
	/* FIXME: This prolly needs work.. */
/*	gint *ptr; */
	
	doc->filename = NULL;
	doc->changed = FALSE;
	doc->views = NULL;
		
	gnome_mdi_child_set_menu_template (GNOME_MDI_CHILD (doc), doc_menu);
}

gedit_document *
gedit_document_new (void)
{
	gedit_document *doc;
/*	int i; */

	/* FIXME: Blarg!! */
	if ((doc = gtk_type_new (gedit_document_get_type ()))) {
	
	  gnome_mdi_child_set_name(GNOME_MDI_CHILD(doc), get_untitled_as_string());
	  
	  doc->buf = g_string_sized_new (64);
	  return doc;
	}
	
	g_print ("Eeek.. bork!\n");
	gtk_object_destroy (GTK_OBJECT(doc));
	
	return NULL;
}

gchar*
get_untitled_as_string (void)
{

	gedit_document *doc;	
	int counter = 0;
	int i;
	
        for (i = 0; i < g_list_length (mdi->children); i++) {

	  doc = (gedit_document *)g_list_nth_data (mdi->children, i);
	  
	  if (!doc->filename)
	    counter++;

	}
	
        if (counter == 0)
          return _(UNTITLED);
        else
	   return (g_strdup_printf("%s %d", _(UNTITLED), counter));        
	   
	return NULL;

}

gedit_document *
gedit_document_new_with_title (gchar *title)
{
	gedit_document *doc;
	
	/* FIXME: Blarg!! */
	
	if ((doc = gtk_type_new (gedit_document_get_type ()))) {
	
	  gnome_mdi_child_set_name(GNOME_MDI_CHILD(doc), title);

	  doc->buf = g_string_sized_new (64);
	
	  return doc;
	  
        }
	
	g_print ("An error occured!\n");
	gtk_object_destroy (GTK_OBJECT(doc));
	
	return NULL;
}


gedit_document *
gedit_document_new_with_file (gchar *filename)
{
	gedit_document *doc;
	struct stat stats;
/*	gchar *tmp_buf; */
/*	FILE *fp; */

	if (!stat(filename, &stats) && S_ISREG(stats.st_mode)) {
	
	  if ((doc = gtk_type_new (gedit_document_get_type ()))) {
	  

	    if (!gedit_file_open (doc, filename)) {
	    	/*g_free (filename);*/
	    	return doc;
	    }
	
	  }
	
	  g_print ("Eeek.. bork!\n");
	  gtk_object_destroy (GTK_OBJECT(doc));
	
	}
	return NULL;
}

gedit_document *
gedit_document_current (void)
{
/*	gint cur; */
	gedit_document *current_document = NULL;

	if (mdi->active_child)
	  current_document = GE_DOCUMENT(mdi->active_child);
 
	return current_document;
}

static gchar *gedit_document_get_config_string (GnomeMDIChild *child)
{
	/* FIXME: Is this correct? */
	/*return g_strdup (GE_DOCUMENT(child)->filename);*/
	return g_strdup_printf ("%d", GPOINTER_TO_INT (gtk_object_get_user_data (GTK_OBJECT (child))));
}

GnomeMDIChild *gedit_document_new_from_config (gchar *file)
{
	gedit_document *doc;
	
	doc = gedit_document_new_with_file (file);
	gnome_mdi_add_child (mdi, GNOME_MDI_CHILD (doc));
        gnome_mdi_add_view (mdi, GNOME_MDI_CHILD (doc));        
        
	return GNOME_MDI_CHILD (doc);
}

void gedit_add_view (GtkWidget *w, gpointer data)
{
	GnomeMDIChild *child;
	gedit_view *view;
/*	gedit_document *doc = GE_DOCUMENT (data); */
/*	gint v; */
/*	gchar *buf; */
/*	GtkText *text; */

	if (mdi->active_view) {
	
	  view = GE_VIEW (mdi->active_view);
	  g_print ("contents: %d\n",
		    GTK_TEXT(GE_VIEW(mdi->active_view)->text)->first_line_start_index);
	   
	  /* sync the buffer before adding the view */
	  	  gedit_view_buffer_sync (view);
	   
	  child = gnome_mdi_get_child_from_view (mdi->active_view);
	   
  
	  gnome_mdi_add_view (mdi, child);
   
	}
	
}

void gedit_remove_view (GtkWidget *w, gpointer data)
{
	gedit_document *doc = GE_DOCUMENT (data);
	
	if (mdi->active_view == NULL)
	  return;

	/* First, we remove the view from the document's list */
	doc->views = g_list_remove (doc->views, GE_VIEW (mdi->active_view));
	  
	/* Now, we can remove the view proper */
	gnome_mdi_remove_view (mdi, mdi->active_view, FALSE);
}

/* Various MDI Callbacks */

gint
remove_doc_cb (GnomeMDI *mdi, gedit_document *doc)
{
	GnomeMessageBox *msgbox;
	int ret;
	char *fname, *msg;
	gedit_data *data = g_malloc (sizeof(gedit_data));
/*	int *ptr; */

	fname = GNOME_MDI_CHILD (doc)->name;
	msg =   (char *)g_malloc(strlen(fname) + 52);
	sprintf(msg, _(" '%s' has been modified. Do you wish to save it?"), fname);
	
	if (mdi->active_view) {
	
	  if (doc->changed) {
	  
	    msgbox = GNOME_MESSAGE_BOX (gnome_message_box_new (
	    							msg,
	    							GNOME_MESSAGE_BOX_QUESTION,
	    							GNOME_STOCK_BUTTON_YES,
	    							GNOME_STOCK_BUTTON_NO,
	    							GNOME_STOCK_BUTTON_CANCEL,
	    							NULL));
	    							
	    gnome_dialog_set_default (GNOME_DIALOG (msgbox), 2);
	    ret = gnome_dialog_run_and_close (GNOME_DIALOG (msgbox));
	    	    
	    switch (ret) {
	    
	    case 0:
	    		file_save_cb (NULL, data);
	    		g_print("blargh\n");
	    
	    case 1:
	    		return TRUE;
	    
	    default:
	    		return FALSE;
	    		
	    }

          }
          
        }
	
	return TRUE;
	
}

void
mdi_view_changed_cb (GnomeMDI *mdi, GtkWidget *old_view)
{
	GnomeApp *app;
/*	GnomeUIInfo *uiinfo; */
/*	gedit_document *doc; */
/*	GtkWidget *shell, *item; */
/*	gint group_item, pos, i; */
/*	gchar *p, *label; */

	if (mdi->active_view == NULL)
	  return;
	  
	app = mdi->active_window;
	
	/*gedit_set_menu_toggle_states();
	*/
/*	label = NULL;
	p = g_strconcat (GNOME_MENU_VIEW_PATH, label, NULL);
	shell = gnome_app_find_menu_pos (app->menubar, p, &pos);
	if (shell) {
	    
	  item = g_list_nth_data (GTK_MENU_SHELL (shell)->children, pos -1);
	    
	  if (item)
	    gtk_menu_shell_activate_item (GTK_MENU_SHELL (shell), item, TRUE);
   
	}
	g_free (p);

*/	
}

void
add_view_cb (GnomeMDI *mdi, gedit_document *doc)
{
/*      if (doc != NULL)
	gtk_object_set_data (GTK_OBJECT(GE_DOCUMENT(mdi->active_view)),
	                     "TEST",
	                     doc);
	
	return;*/
}

gint
add_child_cb (GnomeMDI *mdi, gedit_document *doc)
{
	/* Add child stuff.. we need to make sure that it is safe to add a child,
	   or something, i'm not quite sure about the syntax for this function */
	
	return TRUE;
}

#if 0 /* These are defined but not used */
static gint gedit_document_signals [LAST_SIGNAL];

static void
gedit_document_marshal (GtkObject     *object,
		     GtkSignalFunc	func,
		     gpointer	func_data,
		     GtkArg        *args)
{
	gedit_document_signal rfunc;
	
	rfunc = (gedit_document_signal) func;
	
	(* rfunc)(object, GTK_VALUE_POINTER(args[0]), func_data);
}

static void
gedit_document_real_changed (gedit_document *doc, gpointer change_data)
{
	/* FIXME! */
	
	g_print ("blarg\n");
}
#endif /* #if 0 */
