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
#include <libgnomevfs/gnome-vfs.h>

#include "window.h"
#include "undo.h"
#include "file.h"
#include "utils.h"
#include "document.h"
#include "prefs.h"
#include "menus.h"
#include "commands.h"

GnomeMDI *mdi;
typedef void (*gedit_document_signal) (GtkObject *, gpointer, gpointer);
static GnomeMDIChildClass *parent_class = NULL;

static gchar*		gedit_document_get_config_string (GnomeMDIChild *child);
static gint		remove_child_cb (GnomeMDI *mdi, GeditDocument *doc);

static	GtkWidget*	gedit_document_create_view (GnomeMDIChild *child);
static	void		gedit_document_destroy (GtkObject *obj);
static	void		gedit_document_class_init (GeditDocumentClass *class);
static	void		gedit_document_init (GeditDocument *doc);
	GtkType		gedit_document_get_type (void);

/* ----- Local structs ------------ */

struct _GeditDocumentClass
{
	GnomeMDIChildClass parent_class;

	void (*document_changed)(GeditDocument *, gpointer);
	
};











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
gedit_document_insert_text (GeditDocument *doc, const guchar *text, guint position, gint undoable)
{
	GeditView *view;
	GtkText *text_widget;
	GtkWidget *editable;
	gint length;
	gint exclude_this_view;

	if (doc->readonly)
	     return;
	g_return_if_fail (text!=NULL);
	view = g_list_nth_data (doc->views, 0);
	g_return_if_fail (view!=NULL);
	text_widget = GTK_TEXT (view->text);
	g_return_if_fail (text_widget!=NULL);
	editable = GTK_WIDGET (text_widget);
	g_return_if_fail (editable!=NULL);
	length = strlen (text);
	exclude_this_view = FALSE;

	doc_insert_text_real_cb (editable, text, length, &position, view, exclude_this_view, undoable);
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
gedit_document_delete_text (GeditDocument *doc, guint position, gint length, gint undoable)
{
	GeditView *view;
	GtkText *text_widget;
	GtkWidget *editable;
	gint exclude_this_view;

	if (doc->readonly)
	     return;

	view = g_list_nth_data (doc->views, 0);
	g_return_if_fail (view!=NULL);
	text_widget = GTK_TEXT (view->text);
	g_return_if_fail (text_widget!=NULL);
	editable = GTK_WIDGET (text_widget);
	g_return_if_fail (editable!=NULL);
	exclude_this_view = FALSE;

	doc_delete_text_real_cb (editable, position, position+length, view, exclude_this_view, undoable);
}

/**
 * gedit_document_replace_text:
 * @doc: 
 * @text: 
 * @position: 
 * @length: 
 * @undoable: 
 * 
 * is equivalent to delete_text + insert_text
 **/
void
gedit_document_replace_text (GeditDocument *doc, const guchar * text_to_insert,
			     gint length, guint position, gint undoable)
{
	gchar *text_to_delete;

	gedit_debug (DEBUG_DOCUMENT, "");

	g_return_if_fail (doc != NULL);
	g_return_if_fail (text_to_insert != NULL);

	text_to_delete = gedit_document_get_chars (doc, position, position + length);

	g_return_if_fail (text_to_delete != NULL);

	if (undoable)
	{
		gint   text_to_delete_length;
		gint   text_to_insert_length;

		text_to_delete_length = strlen (text_to_delete);
		text_to_insert_length = strlen (text_to_insert);

		gedit_undo_add (text_to_delete,
				position,
				position + length,
				GEDIT_UNDO_ACTION_REPLACE_DELETE,
				doc,
				NULL);

		gedit_undo_add (text_to_insert,
				position,
				position + text_to_insert_length,
				GEDIT_UNDO_ACTION_REPLACE_INSERT,
				doc,
				NULL);
	}

	gedit_document_delete_text (doc, position, length, FALSE);
	gedit_document_insert_text (doc, text_to_insert, position, FALSE);

	
	g_free (text_to_delete);
}


void
gedit_document_set_readonly (GeditDocument *doc, gint readonly)
{
	GeditView * nth_view;
	gint n;
	
	gedit_debug (DEBUG_DOCUMENT, "");

	g_return_if_fail(doc != NULL);
	
	doc->readonly = readonly;
	doc->changed = FALSE;
		
	for (n = 0; n < g_list_length (doc->views); n++)
	{
		nth_view = g_list_nth_data (doc->views, n);
		gedit_view_set_readonly (nth_view, doc->readonly);
	}

}

void
gedit_document_text_changed_signal_connect (GeditDocument *doc)
{
	GeditView *nth_view;
	gint n;
	
	for (n = 0; n < g_list_length (doc->views); n++)
	{
		nth_view = g_list_nth_data (doc->views, n);

		g_return_if_fail (nth_view != NULL);

		nth_view->view_text_changed_signal =
			gtk_signal_connect (GTK_OBJECT(nth_view->text), "changed",
					    GTK_SIGNAL_FUNC (gedit_view_text_changed_cb), nth_view);
	}  

}



/**
 * gedit_document_get_tab_name:
 * @doc: 
 * 
 * determines the correct tab name for doc and assings a untitled
 * number if necessary
 *
 * Return Value: a potiner to a newly allocated string 
 **/
gchar*
gedit_document_get_tab_name (GeditDocument *doc, gboolean star)
{
	GeditDocument *nth_doc;
	int max_number = 0;
	int i;

	gedit_debug (DEBUG_DOCUMENT, "");
	
	g_return_val_if_fail (doc != NULL, g_strdup ("?"));
	
	if (doc->filename != NULL)
	{
		gchar * tab_name = NULL;
		gchar * unescaped_str = gnome_vfs_unescape_string_for_display (doc->filename);

		if(unescaped_str == NULL) 
		{
			g_warning ("unescaped_str == NULL in gedit_document_get_tab_name ()");
			unescaped_str = g_strdup (doc->filename);
		}
		
		if (!doc->changed || !star)
		{
			tab_name = g_strdup_printf ("%s%s", doc->readonly?_("RO - "):"", g_basename(unescaped_str));
		}
		else
		{			
			tab_name = g_strdup_printf ("%s%s*", doc->readonly?_("RO - "):"", g_basename(unescaped_str));
		}

		g_free (unescaped_str);
		
		return tab_name;
	}
	else
	{
	        if (doc->untitled_number == 0)
		{
			for (i = 0; i < g_list_length (mdi->children); i++)
			{
				nth_doc = (GeditDocument *)g_list_nth_data (mdi->children, i);
				
				if (nth_doc->untitled_number > max_number)
				{
					max_number = nth_doc->untitled_number;
				}
			}
			doc->untitled_number = max_number + 1;
		}
		if (!doc->changed || !star)
		{
			return _(g_strdup_printf ("%s %d", _("Untitled"), doc->untitled_number));
		}
		else
		{
			return _(g_strdup_printf ("%s %d*", _("Untitled"), doc->untitled_number));
		}
	}
}

guchar *
gedit_document_get_chars (GeditDocument *doc, guint start_pos, guint end_pos)
{
	guchar * buffer;
	GtkText * text;
	GeditView * view;

	gedit_debug (DEBUG_DOCUMENT, "");

	g_return_val_if_fail (doc!=NULL, NULL);
	g_return_val_if_fail (end_pos > start_pos, NULL);
	view = g_list_nth_data (doc->views, 0);
	g_return_val_if_fail (view!=NULL, NULL);

	text = GTK_TEXT (view->text);

	buffer = gtk_editable_get_chars ( GTK_EDITABLE ( text ),
					  start_pos,
					  end_pos);
	return buffer;
}

/**
 * gedit_document_get_buffer:
 * @doc: 
 * 
 * returns a newly allocated buffer containg the text inside doc
 * 
 * Return Value: the content of the document as a null-terminated string. 
 * The result must be freed with g_free() when the application is finished with it.
 **/
guchar *
gedit_document_get_buffer (GeditDocument *doc)
{
	guchar * buffer;
	GeditView * view;

	gedit_debug (DEBUG_DOCUMENT, "");

	g_return_val_if_fail (doc!=NULL, NULL);

	view = g_list_nth_data (doc->views, 0);
	g_return_val_if_fail (view!=NULL, NULL);
	
	buffer = gtk_editable_get_chars ( GTK_EDITABLE ( view->text ),
					  0,
					  -1);

	return buffer;
}

guint
gedit_document_get_buffer_length (GeditDocument *doc)
{
	guint length;
	GeditView * view;

	gedit_debug (DEBUG_DOCUMENT, "");

	g_return_val_if_fail (doc!=NULL, 0);
	view = g_list_nth_data (doc->views, 0);
	g_return_val_if_fail (view!=NULL, 0);

	length = gtk_text_get_length (GTK_TEXT (view->text));
	return length;
}


GeditDocument *
gedit_document_new (void)
{
	GeditDocument *doc;

	gedit_debug (DEBUG_DOCUMENT, "");

	doc = gtk_type_new (gedit_document_get_type ());

	g_return_val_if_fail (doc != NULL, NULL);

	gnome_mdi_add_child (mdi, GNOME_MDI_CHILD (doc));
	gnome_mdi_add_view (mdi, GNOME_MDI_CHILD (doc));

	gedit_window_set_widgets_sensitivity (TRUE);
	gedit_window_set_toolbar_labels (GEDIT_VIEW(doc->views->data)->app);

	return doc;
}

GeditDocument *
gedit_document_new_with_title (const gchar *title)
{
	GeditDocument *doc;
	
	gedit_debug (DEBUG_DOCUMENT, "");

	g_return_val_if_fail (title != NULL, NULL);
	doc = gtk_type_new (gedit_document_get_type ());
	g_return_val_if_fail (doc != NULL, NULL);

	doc->filename = g_strdup (title);
	
	gnome_mdi_add_child (mdi, GNOME_MDI_CHILD (doc));
	gnome_mdi_add_view (mdi, GNOME_MDI_CHILD (doc));

	gedit_window_set_widgets_sensitivity (TRUE);

	return doc;
}

gint
gedit_document_new_with_file (const gchar *file_name)
{
	GeditDocument *doc;

	gedit_debug (DEBUG_DOCUMENT, "");

	doc = gedit_document_current();

	if (doc!=NULL) 
		if (doc->changed || doc->filename)
			doc = NULL;

	if (gedit_file_open (doc, file_name) == 1)
		return FALSE;
	
	gedit_window_set_widgets_sensitivity (TRUE);

	return TRUE;

}

GeditDocument *
gedit_document_current (void)
{
	GeditDocument *current_document = NULL;

	if (mdi->active_child)
		current_document = GEDIT_DOCUMENT (mdi->active_child);
 
	return current_document;
}

static gchar *
gedit_document_get_config_string (GnomeMDIChild *child)
{
	gedit_debug (DEBUG_DOCUMENT, "");
	return g_strdup_printf ("%d", GPOINTER_TO_INT (gtk_object_get_user_data (GTK_OBJECT (child))));
}

static gint
remove_child_cb (GnomeMDI *mdi, GeditDocument *doc)
{
	gedit_debug (DEBUG_DOCUMENT, "start");
	
	if (!gedit_view_active())
	{
		gedit_debug (DEBUG_DOCUMENT, "returning since views are NULL");
		return TRUE;
	}

	if (doc->changed)
	{
		GtkWidget *msgbox, *w;
		gchar *fname = NULL, *msg = NULL;
		gint ret;

		w = GTK_WIDGET (g_list_nth_data(doc->views, 0));
			
		if(w != NULL)
			gnome_mdi_set_active_view (mdi, w);

		fname = gedit_document_get_tab_name (doc, FALSE);

		msg = g_strdup_printf (_("``%s'' has been modified.  Do you wish to save it?"),
				       fname);
		
		msgbox = gnome_message_box_new (msg,
						GNOME_MESSAGE_BOX_QUESTION,
						GNOME_STOCK_BUTTON_YES,
						GNOME_STOCK_BUTTON_NO,
						GNOME_STOCK_BUTTON_CANCEL, 
						NULL);

		gnome_dialog_set_default (GNOME_DIALOG (msgbox), 2);	
		gnome_dialog_set_parent (GNOME_DIALOG (msgbox),
					 GTK_WINDOW (mdi->active_window));

		ret = gnome_dialog_run_and_close (GNOME_DIALOG (msgbox));

		g_free (fname);
		g_free (msg);
		
		switch (ret)
		{
		case 0:
			return file_save_document (doc);
		case 1:
			return TRUE;
		default:
			gedit_close_all_flag_clear();
			return FALSE;
		}
	}
	
	gedit_debug (DEBUG_DOCUMENT, "end");
	
	return TRUE;
}

static GtkWidget *
gedit_document_create_view (GnomeMDIChild *child)
{
	GeditView  *new_view;

	gedit_debug (DEBUG_DOCUMENT, "");

	g_return_val_if_fail (child != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_MDI_CHILD (child), NULL);

	new_view = GEDIT_VIEW (gedit_view_new (GEDIT_DOCUMENT (child)));

	gedit_view_set_font (new_view, settings->font);
	gedit_view_set_readonly (new_view, GEDIT_DOCUMENT (child)->readonly);

	return GTK_WIDGET (new_view);
}

static void
gedit_document_destroy (GtkObject *obj)
{
	GeditDocument *doc = GEDIT_DOCUMENT (obj);

	gedit_debug (DEBUG_DOCUMENT, "");

	g_list_free (doc->views);
	g_free (doc->filename);
	gedit_undo_free_list (&doc->undo);
	gedit_undo_free_list (&doc->redo);
	
	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy)(GTK_OBJECT (doc));

}

static void
gedit_document_class_init (GeditDocumentClass *class)
{
	GtkObjectClass  	*object_class;
	GnomeMDIChildClass	*child_class;
	
	object_class = (GtkObjectClass*)class;
	child_class = GNOME_MDI_CHILD_CLASS (class);

	gedit_debug (DEBUG_DOCUMENT, "");
	
	object_class->destroy = gedit_document_destroy;
	
	child_class->create_view = (GnomeMDIChildViewCreator)(gedit_document_create_view);
	child_class->get_config_string = (GnomeMDIChildConfigFunc)(gedit_document_get_config_string);

	parent_class = gtk_type_class (gnome_mdi_child_get_type ());

}

static void
gedit_document_init (GeditDocument *doc)
{
	gedit_debug (DEBUG_DOCUMENT, "");

	doc->filename = NULL;
	doc->changed = FALSE;
	doc->views = NULL;
	doc->undo = NULL;
	doc->redo = NULL;
		
	gnome_mdi_child_set_menu_template (GNOME_MDI_CHILD (doc), doc_menu);
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
			sizeof (GeditDocument),
			sizeof (GeditDocumentClass),
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


void
gedit_mdi_init (void)
{
	gedit_debug (DEBUG_DOCUMENT, "start");

	/*
	mdi = GNOME_MDI (gnome_mdi_new ("gedit", "gedit "VESION));
	*/
	mdi = GNOME_MDI (gnome_mdi_new ("gedit", "gedit "));

	mdi->tab_pos = settings->tab_pos;

	gnome_mdi_set_menubar_template (mdi, gedit_menu);
	gnome_mdi_set_toolbar_template (mdi, toolbar_data);
	
	gnome_mdi_set_child_menu_path (mdi, GNOME_MENU_FILE_STRING);
	gnome_mdi_set_child_list_path (mdi, D_("_Documents/"));

	/* connect signals */
	gtk_signal_connect (GTK_OBJECT (mdi), "remove_child",
			    GTK_SIGNAL_FUNC (remove_child_cb), NULL);
	gedit_mdi_destroy_signal =
		gtk_signal_connect (GTK_OBJECT (mdi), "destroy",
				    GTK_SIGNAL_FUNC (file_quit_cb), NULL);
	gtk_signal_connect (GTK_OBJECT (mdi), "view_changed",
			    GTK_SIGNAL_FUNC (gedit_view_changed_cb), NULL);
        gtk_signal_connect (GTK_OBJECT (mdi), "app_created",
			    GTK_SIGNAL_FUNC (gedit_window_new), NULL);

	gnome_mdi_set_mode (mdi, settings->mdi_mode);
	gnome_mdi_open_toplevel (mdi);

	/* Loads the structure gedit_toolbar with the widgets */
	gedit_window_set_toolbar_labels (mdi->active_window);
	gedit_window_set_view_menu_sensitivity (mdi->active_window);

	gedit_debug (DEBUG_DOCUMENT, "end");
}



/**
 * gedit_set_title:
 * @docname: Document name in a string, the new title
 *
 * Set the title to "$docname - $gedit_ver" and if the document has
 * changed, lets show that it has. 
 **/
void
gedit_document_set_title (GeditDocument *doc)
{
	gchar *title;
	gchar *docname;

	gedit_debug (DEBUG_DOCUMENT, "");

	
	if (doc == NULL)
		return;
	
	if (doc->filename == NULL) {
		docname = g_strdup_printf (_("Untitled %i"), doc->untitled_number);
	} else {
#if 0
		docname = g_strdup (g_basename (doc->filename));
#endif
		docname = gnome_vfs_unescape_string_for_display (doc->filename);		
	}

#if 0
	if (doc->changed) {
		title = g_strdup_printf ("gedit: %s %s", docname, _("(modified)"));
	} else if (doc->readonly) {
		title = g_strdup_printf ("gedit: %s %s", docname, _("(readonly)"));
	} else {
		title = g_strdup_printf ("gedit: %s", docname);
	}
#endif
	if (doc->changed) {
		title = g_strdup_printf ("gedit - [%s] %s", docname, _("(modified)"));
	} else if (doc->readonly) {
		title = g_strdup_printf ("gedit - [%s] %s", docname, _("(readonly)"));
	} else {
		title = g_strdup_printf ("gedit - [%s]", docname);
	}


	gtk_window_set_title (gedit_window_active(), title);
	
	g_free (docname);
	
	g_free (title);

	gedit_debug (DEBUG_DOCUMENT, "end");
}


/**
 * gedit_document_swap_hc_cb:
 * 
 * if .c file is open, then open the related .h file and v.v.
 *
 * TODO: if a .h file is open, do we swap to a .c or a .cpp?  we
 * should put a check in there.  if both exist, then probably open
 * both files.
 **/
void
gedit_document_swap_hc_cb (GtkWidget *widget, gpointer data)
{
	size_t len;
	gchar *new_file_name;
	GeditDocument *doc, *nth_doc;
	gint n;

	gedit_debug (DEBUG_DOCUMENT, "");
	
	doc = gedit_document_current();
	if (!doc || !doc->filename)
		return;

	new_file_name = NULL;
	len = strlen (doc->filename);

	while (len)
	{
		if (doc->filename[len] == '.')
			break;
		len--;
	}

	if (len == 0)
	{
		gchar *errstr = g_strdup (_("This file does not end with a valid extension\n"));
		gnome_app_error (gedit_window_active_app(), errstr);
		g_free (errstr);
		return;
	}

	len++;
	if (doc->filename[len] == 'h')
	{
		new_file_name = g_strdup (doc->filename);
		new_file_name[len] = 'c';
	}
	else if (doc->filename[len] == 'H')
	{
		new_file_name = g_strdup (doc->filename);
		new_file_name[len] = 'C';
	}
	else if (doc->filename[len] == 'c')
	{
		new_file_name = g_strdup (doc->filename);
		new_file_name[len] = 'h';

		if (len < strlen(doc->filename) && strcmp(doc->filename + len, "cpp") == 0)
			new_file_name[len+1] = '\0';
	}
	else if (doc->filename[len] == 'C')
	{
		new_file_name = g_strdup (doc->filename);

		if (len < strlen(doc->filename) && strcmp(doc->filename + len, "CPP") == 0)
		{
			new_file_name[len] = 'H';
			new_file_name[len+1] = '\0';
		}
		else
			new_file_name[len] = 'H';
	}

	if (!new_file_name)
	{
		gchar *errstr = g_strdup (_("This file does not end with a valid extension\n"));
		gnome_app_error (gedit_window_active_app(), errstr);
		g_free (errstr);
		return;
	}

	if (!g_file_exists (new_file_name))
	{
		gchar *errstr = g_strdup_printf (_("The file %s was not found."), new_file_name);
		gnome_app_error (gedit_window_active_app(), errstr);
		g_free (errstr);
		return;
	}

	/* Scan the documents to see if the file we are looking for is allready open */
	for (n = 0; n < g_list_length (mdi->children); n++)
	{
		nth_doc = (GeditDocument *)g_list_nth_data (mdi->children, n);
		
		if (strcmp(nth_doc->filename, new_file_name) == 0)
		{
			GeditView *view;
			view = g_list_nth_data (nth_doc->views, 0);
			g_return_if_fail (view != NULL);
			gnome_mdi_set_active_view (mdi, GTK_WIDGET (view));
			return;
		}
		
	}
	
	gedit_document_new_with_file (new_file_name);
}

void
gedit_document_set_undo (GeditDocument *doc, gint undo_state, gint redo_state)
{
	GeditView *nth_view;
	gint n;
	
	gedit_debug (DEBUG_DOCUMENT, "");
	
	g_return_if_fail (GEDIT_IS_DOCUMENT(doc));

	for ( n=0; n < g_list_length (doc->views); n++)
	{
		nth_view = GEDIT_VIEW (g_list_nth_data (doc->views, n));
		gedit_view_set_undo (nth_view, undo_state, redo_state);
	}
	
}

