/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit
 *
 * Copyright (C) 1998, 1999, 2000  Alex Roberts, Evan Lawrence, Jason Leach, Jose M Celorio
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
 */

#include <config.h>
#include <gnome.h>

#include "window.h"
#include "undo.h"
#include "file.h"
#include "search.h"
#include "print.h"
#include "menus.h"
#include "utils.h"

#include "prefs.h"
#include "view.h"
#include "commands.h"
#include "document.h"


GnomeMDI *mdi;
typedef void (*gedit_document_signal) (GtkObject *, gpointer, gpointer);
static GnomeMDIChildClass *parent_class = NULL;

void gedit_document_insert_text (Document *doc, guchar *text, guint position, gint undoable);
void gedit_document_delete_text (Document *doc, guint position, gint length, gint undoable);

GtkType gedit_document_get_type (void);
static GtkWidget * gedit_document_create_view (GnomeMDIChild *child);
static void gedit_document_destroy (GtkObject *obj);
static void gedit_document_class_init (DocumentClass *class);
void gedit_document_init (Document *doc);
gchar* gedit_get_document_tab_name (Document *doc);
guchar * gedit_document_get_buffer (Document * doc);
Document * gedit_document_new (void);
Document * gedit_document_new_with_title (gchar *title);
Document * gedit_document_new_with_file (gchar *filename);
Document * gedit_document_stdin (void);
Document * gedit_document_current (void);
static gchar * gedit_document_get_config_string (GnomeMDIChild *child);
void gedit_add_view (GtkWidget *widget, gpointer data);
void gedit_remove_view (GtkWidget *widget, gpointer data);
static void child_changed_cb (GnomeMDI *mdi, Document *doc);
static gint remove_child_cb (GnomeMDI *mdi, Document *doc);
void gedit_mdi_init (void);
void mdi_view_changed_cb (GnomeMDI *mdi, GtkWidget *old_view);
void gedit_document_load ( GList *file_list);

/**
 * gedit_document_insert_text: Inserts text to a document and hadles all the details like
 *                             adding the data to the undo stack and adding it to multiple views
 *                             Provides an abstraction layer between gedit and the text widget.
 * @doc: document to insert text
 * @text: null terminated text to insert
 * @position: position to insert the text at
 * @undoable: if this insertion gets added to the undo stack or not.
 * 
 *
 **/
void
gedit_document_insert_text (Document *doc, guchar *text, guint position, gint undoable)
{
	View *view;
	GtkText *text_widget;
	GtkWidget *editable;
	gint length;
	gint exclude_this_view;

	view = g_list_nth_data (doc->views, 0);
	g_return_if_fail (view!=NULL);
	text_widget = GTK_TEXT (view->text);
	g_return_if_fail (text_widget!=NULL);
	editable = GTK_WIDGET (text_widget);
	g_return_if_fail (editable!=NULL);
	length = strlen (text);
	exclude_this_view = FALSE;

	doc_insert_text_cb (editable, text, length, &position, view, exclude_this_view, undoable);
}

/**
 * gedit_document_delete_text: Deletes text from a document and hadles all the details like
 *                             adding the data to the undo stack and deleting it from multiple views
 *                             Provides an abstraction layer between gedit and the text widget.
 * @doc: document to delete text from
 * @position: position to delete the text from
 * @length: length of the text to delete
 * @undoable: if this deletion gets added to the undo stack or not.
 * 
 *
 **/
void
gedit_document_delete_text (Document *doc, guint position, gint length, gint undoable)
{
	View *view;
	GtkText *text_widget;
	GtkWidget *editable;
	gint exclude_this_view;

	view = g_list_nth_data (doc->views, 0);
	g_return_if_fail (view!=NULL);
	text_widget = GTK_TEXT (view->text);
	g_return_if_fail (text_widget!=NULL);
	editable = GTK_WIDGET (text_widget);
	g_return_if_fail (editable!=NULL);
	exclude_this_view = FALSE;

	doc_delete_text_cb (editable, position, position+length, view, exclude_this_view, undoable);
}

GtkType
gedit_document_get_type (void)
{
	static GtkType doc_type = 0;
	
	if (!doc_type)
	{
		static const GtkTypeInfo doc_info =
		{
			"Document",
			sizeof (Document),
			sizeof (DocumentClass),
			(GtkClassInitFunc) gedit_document_class_init,
			(GtkObjectInitFunc) gedit_document_init,
			(GtkArgSetFunc) NULL,
			(GtkArgGetFunc) NULL,
		};
	  
		doc_type = gtk_type_unique (gnome_mdi_child_get_type (),
					    &doc_info);
	}
	  
	return doc_type;
}

static GtkWidget *
gedit_document_create_view (GnomeMDIChild *child)
{
	View  *new_view;

	gedit_debug ("", DEBUG_FILE);

	g_return_val_if_fail (child != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_MDI_CHILD (child), NULL);

	new_view = VIEW (gedit_view_new (DOCUMENT (child)));

	gedit_view_set_font (new_view, settings->font);
	gedit_view_set_read_only (new_view, DOCUMENT (child)->readonly); 

	return GTK_WIDGET (new_view);
}

static void
gedit_document_destroy (GtkObject *obj)
{
	Document *doc = DOCUMENT (obj);

	gedit_debug ("", DEBUG_DOCUMENT);
	
	g_free (doc->filename);
	/* dont have a buffer anymore
	g_string_free (doc->buffer, TRUE);
	*/
	gedit_undo_free_list (&doc->undo);
	gedit_undo_free_list (&doc->redo);
	
	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy)(GTK_OBJECT (doc));

	gedit_debug ("end", DEBUG_DOCUMENT);
}

static void
gedit_document_class_init (DocumentClass *class)
{
	GtkObjectClass  	*object_class;
	GnomeMDIChildClass	*child_class;
	
	object_class = (GtkObjectClass*)class;
	child_class = GNOME_MDI_CHILD_CLASS (class);

	gedit_debug ("", DEBUG_DOCUMENT);
	
	/* blarg.. signals stuff.. doc_changed? FIXME 
	gtk_object_class_add_signals (object_class, gedit_document_signals, LAST_SIGNAL);
	*/
	
	object_class->destroy = gedit_document_destroy;
	
	child_class->create_view = (GnomeMDIChildViewCreator)(gedit_document_create_view);
	child_class->get_config_string = (GnomeMDIChildConfigFunc)(gedit_document_get_config_string);

	parent_class = gtk_type_class (gnome_mdi_child_get_type ());

	gedit_debug ("end", DEBUG_DOCUMENT);
}

void
gedit_document_init (Document *doc)
{
	gedit_debug ("", DEBUG_DOCUMENT);

	doc->filename = NULL;
	doc->changed = FALSE;
	doc->views = NULL;
	doc->undo = NULL;
	doc->redo = NULL;
		
	gnome_mdi_child_set_menu_template (GNOME_MDI_CHILD (doc), doc_menu);
}

gchar*
gedit_get_document_tab_name (Document *doc)
{
	Document *nth_doc;
	int max_number = 0;
	int i;
	const char *UNTITLED = N_("Untitled");

	gedit_debug ("", DEBUG_DOCUMENT);

	if (doc->untitled_number == 0)
	{
		for (i = 0; i < g_list_length (mdi->children); i++)
		{
			nth_doc = (Document *)g_list_nth_data (mdi->children, i);
			
			if ( nth_doc->untitled_number > max_number)
			{
				max_number = nth_doc->untitled_number;
			}
		}
		doc->untitled_number = max_number + 1;
	}
	
	return _(g_strdup_printf ("%s %d", UNTITLED, doc->untitled_number));
	   
}

/**
 * gedit_document_get_buffer:
 * @doc: 
 * 
 * returns a newly allocated buffer containg the text inside doc
 * 
 * Return Value: 
 **/
guchar *
gedit_document_get_buffer (Document * doc)
{
	guchar * buffer;
	guint length;
	GtkText * text;
	View * view;

	gedit_debug ("", DEBUG_DOCUMENT);

	g_return_val_if_fail (doc!=NULL, NULL);
	view = g_list_nth_data (doc->views, 0);
	g_return_val_if_fail (view!=NULL, NULL);

	text = GTK_TEXT (view->text);

	length = gtk_text_get_length (text);
	buffer = gtk_editable_get_chars ( GTK_EDITABLE ( text ),
					  0,
					  length);
	return buffer;
}

Document *
gedit_document_new (void)
{
	Document *doc;
	gchar *doc_name;

	gedit_debug ("", DEBUG_DOCUMENT);

	doc = gtk_type_new (gedit_document_get_type ());

	g_return_val_if_fail (doc != NULL, NULL);

	doc_name = gedit_get_document_tab_name(doc);
	gnome_mdi_child_set_name (GNOME_MDI_CHILD (doc),
				  doc_name);
	g_free (doc_name);
	/*
	doc->buffer = g_string_sized_new (256);
	*/

	gedit_debug ("end", DEBUG_DOCUMENT);

	return doc;
}

Document *
gedit_document_new_with_title (gchar *title)
{
	Document *doc;
	
	gedit_debug ("", DEBUG_DOCUMENT);

	g_return_val_if_fail (title != NULL, NULL);
	doc = gtk_type_new (gedit_document_get_type ());
	g_return_val_if_fail (doc != NULL, NULL);

	gnome_mdi_child_set_name (GNOME_MDI_CHILD (doc),
				  title);

	doc->filename = g_strdup (title);
	/*
	doc->buffer = g_string_sized_new (256);
	*/

	return doc;
}

Document *
gedit_document_new_with_file (gchar *filename)
{
	Document *doc;

	gedit_debug ("", DEBUG_DOCUMENT);

	doc = gedit_document_current();

	if (doc!=NULL)
		if (doc->changed || doc->filename)
			doc = NULL;

	gedit_file_open (doc, filename);

	return doc;
}

Document *
gedit_document_current (void)
{
	Document *current_document = NULL;

	if (mdi->active_child)
		current_document = DOCUMENT (mdi->active_child);
 
	return current_document;
}

static gchar *
gedit_document_get_config_string (GnomeMDIChild *child)
{
	gedit_debug ("", DEBUG_DOCUMENT);
	/* FIXME: Is this correct? */
	/*return g_strdup (GE_DOCUMENT(child)->filename);*/
	return g_strdup_printf ("%d", GPOINTER_TO_INT (gtk_object_get_user_data (GTK_OBJECT (child))));
}

/* Callback for the "child_changed" signal */
static void
child_changed_cb (GnomeMDI *mdi, Document *doc)
{
	Document *doc2 = gedit_document_current();

	gedit_debug ("", DEBUG_DOCUMENT);

	if (doc2)
	{
		gtk_widget_grab_focus (VIEW (mdi->active_view)->text);
		gedit_set_title (doc2);
	}
}

static gint
remove_child_cb (GnomeMDI *mdi, Document *doc)
{
	GtkWidget *msgbox;
	int ret;
	char *fname, *msg;
	/*gedit_data *data = g_malloc (sizeof(gedit_data));*/

	gedit_debug ("", DEBUG_DOCUMENT);
	
	fname = GNOME_MDI_CHILD(doc)->name;

	if (!mdi->active_view)
	{
		return TRUE;
	}

	if ((doc->changed)&&(!doc->readonly))
	{
		msg = g_strdup_printf (_("``%s'' has been modified.  Do you wish to save it?"), fname);
		msgbox = gnome_message_box_new (msg,
						GNOME_MESSAGE_BOX_QUESTION,
						GNOME_STOCK_BUTTON_YES,
						GNOME_STOCK_BUTTON_NO,
						GNOME_STOCK_BUTTON_CANCEL,
						NULL);
		gnome_dialog_set_default (GNOME_DIALOG (msgbox), 2);
		ret = gnome_dialog_run_and_close (GNOME_DIALOG (msgbox));
		g_free (msg);
		switch (ret)
		{
		case 0:
			/* FIXME : If the user selects YES
			   treat as if he had selected CANCEL
			   because we don't want him to loose his
			   data. This is anoying, I know. Chema*/
			file_save_cb (NULL);
			return FALSE;
		case 1:
			return TRUE;
		default:
			return FALSE;
		}
	}
	
	return TRUE;
}

/**
 * gedit_mdi_init:
 *
 * Initialize the GnomeMDI
 **/
void
gedit_mdi_init (void)
{
	gedit_debug ("", DEBUG_DOCUMENT);
	
	mdi = GNOME_MDI (gnome_mdi_new ("gedit", "gedit "VERSION));

	mdi->tab_pos = settings->tab_pos;

	gnome_mdi_set_menubar_template (mdi, gedit_menu);
	gnome_mdi_set_toolbar_template (mdi, toolbar_data);
	
	gnome_mdi_set_child_menu_path (mdi, GNOME_MENU_FILE_STRING);
	gnome_mdi_set_child_list_path (mdi, GNOME_MENU_FILES_PATH);

	/* connect signals */
	gtk_signal_connect (GTK_OBJECT (mdi), "remove_child",
			    GTK_SIGNAL_FUNC (remove_child_cb), NULL);
	gtk_signal_connect (GTK_OBJECT (mdi), "destroy",
			    GTK_SIGNAL_FUNC (file_quit_cb), NULL);
	gtk_signal_connect (GTK_OBJECT (mdi), "child_changed",
			    GTK_SIGNAL_FUNC (child_changed_cb), NULL);
        gtk_signal_connect (GTK_OBJECT (mdi), "app_created",
			    GTK_SIGNAL_FUNC (gedit_window_new), NULL);
/*	gtk_signal_connect(GTK_OBJECT(mdi), "view_changed", GTK_SIGNAL_FUNC(mdi_view_changed_cb), NULL);*/
/*	gtk_signal_connect(GTK_OBJECT(mdi), "add_view", GTK_SIGNAL_FUNC(add_view_cb), NULL);*/
/*	gtk_signal_connect(GTK_OBJECT(mdi), "add_child", GTK_SIGNAL_FUNC(add_child_cb), NULL);*/

	gnome_mdi_set_mode (mdi, settings->mdi_mode);
}

void
mdi_view_changed_cb (GnomeMDI *mdi, GtkWidget *old_view)
{
	GnomeApp *app;
/*	GnomeUIInfo *uiinfo; */
/*	Document *doc; */
/*	GtkWidget *shell, *item; */
/*	gint group_item, pos, i; */
/*	gchar *p, *label; */

	gedit_debug ("", DEBUG_DOCUMENT);

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

#if 0 /* Looks like this are not beeing used. Chema
	 there are also gedit_add_view */
void
add_view_cb (GnomeMDI *mdi, Document *doc)
{
	gedit_debug ("", DEBUG_DOCUMENT);
/*      if (doc != NULL)
	gtk_object_set_data (GTK_OBJECT(GE_DOCUMENT(mdi->active_view)),
	                     "TEST",
	                     doc);
	
	return;*/
}

gint
add_child_cb (GnomeMDI *mdi, Document *doc)
{
	/* Add child stuff.. we need to make sure that it is safe to
	   add a child, or something, i'm not quite sure about the
	   syntax for this function */
	gedit_debug ("", DEBUG_DOCUMENT);

	return TRUE;
}
#endif


#if 0  
GnomeMDIChild *
gedit_document_new_from_config (gchar *file)
{
	Document *doc;

	gedit_debug ("", DEBUG_DOCUMENT);
	
	doc = gedit_document_new_with_file (file);
	gnome_mdi_add_child (mdi, GNOME_MDI_CHILD (doc));
        gnome_mdi_add_view (mdi, GNOME_MDI_CHILD (doc));        
        
	return GNOME_MDI_CHILD (doc);
}
#endif

void
gedit_document_load ( GList *file_list)
{
	Document *doc;
	gint empty;

        /* create a file for each document in the parameter list */
	for (;file_list; file_list = file_list->next)
	{
		if (g_file_exists (file_list->data))
		{
			doc = gedit_document_new_with_title (file_list->data);
			if (doc != NULL)
			{
				gnome_mdi_add_child (mdi, GNOME_MDI_CHILD (doc));
				gnome_mdi_add_view (mdi, GNOME_MDI_CHILD (doc));
				gedit_file_open (doc, file_list->data);
			}
		}
		else
		{
			popup_create_new_file (NULL, file_list->data);
		}
	}

	/* Create document from pipes, if pipes are not used this document becomes
	   the Untitled default one. Chema */
	doc = gedit_document_new ();
	if (doc != NULL)
	{
		gnome_mdi_add_child (mdi, GNOME_MDI_CHILD (doc));
		gnome_mdi_add_view (mdi, GNOME_MDI_CHILD (doc));
		empty = gedit_file_stdin (doc);
		if (empty)
			gnome_mdi_remove_child (mdi, GNOME_MDI_CHILD (doc), FALSE);
	}

	if (gedit_document_current() == NULL)
	{
		doc = gedit_document_new ();
		if (doc != NULL)
		{
			gnome_mdi_add_child (mdi, GNOME_MDI_CHILD (doc));
			gnome_mdi_add_view (mdi, GNOME_MDI_CHILD (doc));
		}
	}
}


