/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * gedit
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


static void 	  gedit_document_class_init (DocumentClass *);
static void 	  gedit_document_init (Document *);
static GtkWidget* gedit_document_create_view (GnomeMDIChild *);
static void	  gedit_document_destroy (GtkObject *);
/*static void	  gedit_document_real_changed (Document *, gpointer); */
static gchar* gedit_document_get_config_string (GnomeMDIChild *child);

gchar* gedit_get_document_tab_name (void);

GnomeMDI *mdi;

typedef void (*gedit_document_signal) (GtkObject *, gpointer, gpointer);

static GnomeMDIChildClass *parent_class = NULL;


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
	  
		doc_type = gtk_type_unique (gnome_mdi_child_get_type (), &doc_info);
	}
	  
	return doc_type;
}

static GtkWidget *
gedit_document_create_view (GnomeMDIChild *child)
{
	View  *new_view;
	
	new_view = VIEW (gedit_view_new (DOCUMENT (child)));

	gedit_debug_mess ("f:gedit_document_create_view\n", DEBUG_FILE);
	
	gedit_view_set_font (new_view, settings->font);
	gedit_view_set_read_only ( new_view, DOCUMENT (child)->readonly);
	gtk_widget_queue_resize (GTK_WIDGET (new_view));
	
	return GTK_WIDGET(new_view);
}

static void
gedit_document_destroy (GtkObject *obj)
{
	Document *doc = DOCUMENT (obj);

	gedit_debug_mess ("f:gedit_document_destroy\n", DEBUG_DOCUMENT);
	
	g_free (doc->filename);
	g_string_free (doc->buf, TRUE);

	g_list_free (doc->undo);
	g_list_free (doc->redo);
	
	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy)(GTK_OBJECT (doc));
}

static void
gedit_document_class_init (DocumentClass *class)
{
	GtkObjectClass  	*object_class;
	GnomeMDIChildClass	*child_class;
	
	object_class = (GtkObjectClass*)class;
	child_class = GNOME_MDI_CHILD_CLASS (class);

	gedit_debug_mess ("f:gedit_document_class_init\n", DEBUG_DOCUMENT);
	
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
gedit_document_init (Document *doc)
{
	/* FIXME: This prolly needs work.. */
	gedit_debug_mess ("f:gedit_document_init\n", DEBUG_DOCUMENT);
	
	doc->filename = NULL;
	doc->changed = FALSE;
	doc->views = NULL;
		
	gnome_mdi_child_set_menu_template (GNOME_MDI_CHILD (doc), doc_menu);
}

gchar*
gedit_get_document_tab_name (void)
{
	Document *doc;	
	int counter = 0;
	int i;
	const char *UNTITLED = N_("Untitled");

	gedit_debug_mess ("f:gedit_document_tab_name\n", DEBUG_DOCUMENT);
	
        for (i = 0; i < g_list_length (mdi->children); i++)
	{
		doc = (Document *)g_list_nth_data (mdi->children, i);
	  
		if (!doc->filename)
			counter++;
	}
	
        if (counter == 0)
		return _(UNTITLED);
        else
		return _(g_strdup_printf ("%s %d", UNTITLED, counter));
	   
	return NULL;
}

Document *
gedit_document_new (void)
{
	Document *doc;

	gedit_debug_mess ("f:gedit_document_new\n", DEBUG_DOCUMENT);

	doc = gtk_type_new (gedit_document_get_type ());
	if (doc)
	{
		gnome_mdi_child_set_name (GNOME_MDI_CHILD (doc),
					  gedit_get_document_tab_name());

		doc->buf = g_string_sized_new (256);
		return doc;
	}

	g_assert_not_reached ();
	gtk_object_destroy (GTK_OBJECT (doc));
	
	return NULL;
}

Document *
gedit_document_new_with_title (gchar *title)
{
	Document *doc;
	
	gedit_debug_mess ("f:gedit_document_new_with_title\n", DEBUG_DOCUMENT);

	g_return_val_if_fail (title != NULL, NULL);

	doc = gtk_type_new (gedit_document_get_type ());
	if (doc)
	{
		gnome_mdi_child_set_name (GNOME_MDI_CHILD(doc),
					  title);

		doc->buf = g_string_sized_new (256);
	
		return doc;
        }

	g_assert_not_reached ();
	gtk_object_destroy (GTK_OBJECT(doc));
	
	return NULL;
}

Document *
gedit_document_new_with_file (gchar *filename)
{
	Document *doc;

	gedit_debug_mess ("f:gedit_document_new_with_file\n", DEBUG_DOCUMENT);

	doc = gtk_type_new (gedit_document_get_type ());

	if (doc)
	{
		if (!gedit_file_open (doc, filename))
			return doc;
		else
			return NULL;
	}

	g_assert_not_reached ();
	gtk_object_destroy (GTK_OBJECT(doc));

	return NULL;
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
	gedit_debug_mess ("f:gedit_document_get_config_string\n", DEBUG_DOCUMENT);
	/* FIXME: Is this correct? */
	/*return g_strdup (GE_DOCUMENT(child)->filename);*/
	return g_strdup_printf ("%d", GPOINTER_TO_INT (gtk_object_get_user_data (GTK_OBJECT (child))));
}

void
gedit_add_view (GtkWidget *w, gpointer data)
{
	GnomeMDIChild *child;
	View *view;

	gedit_debug_mess ("f:gedit_add_view\n", DEBUG_DOCUMENT);

	if (mdi->active_view)
	{
		view = VIEW (mdi->active_view);
/*		g_print ("contents: %d\n", GTK_TEXT(VIEW(mdi->active_view)->text)->first_line_start_index);*/
	   
		/* sync the buffer before adding the view */
		gedit_view_buffer_sync (view);
	   
		child = gnome_mdi_get_child_from_view (mdi->active_view);
	   
		gnome_mdi_add_view (mdi, child);
	}
}

void
gedit_remove_view (GtkWidget *w, gpointer data)
{
	Document *doc = DOCUMENT (data);

	gedit_debug_mess ("f:gedti_remove_view\n", DEBUG_DOCUMENT);
	
	if (mdi->active_view == NULL)
		return;

	/* First, we remove the view from the document's list */
	doc->views = g_list_remove (doc->views, VIEW (mdi->active_view));
	  
	/* Now, we can remove the view proper */
	gnome_mdi_remove_view (mdi, mdi->active_view, FALSE);
}

/* Callback for the "child_changed" signal */
static void
child_changed_cb (GnomeMDI *mdi, Document *doc)
{
	Document *doc2 = gedit_document_current();

	gedit_debug_mess ("f:child_changed_cb\n", DEBUG_DOCUMENT);

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

	gedit_debug_mess ("f:remove_child_cb\n", DEBUG_DOCUMENT);
	
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
			file_save_cb (NULL);
			/* FIXME: why is this NULL ?? chema */
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
	gedit_debug_mess ("f:gedit_mdi_init\n", DEBUG_DOCUMENT);
	
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

	gedit_debug_mess ("f:mdi_view_changed_cb\n", DEBUG_DOCUMENT);

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
	gedit_debug_mess ("f:add_view\n", DEBUG_DOCUMENT);
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
	gedit_debug_mess ("f:gedit_add_child\n", DEBUG_DOCUMENT);

	return TRUE;
}
#endif


#if 0  
GnomeMDIChild *
gedit_document_new_from_config (gchar *file)
{
	Document *doc;

	gedit_debug_mess ("f:gedit_document_new_from_config\n", DEBUG_DOCUMENT);
	
	doc = gedit_document_new_with_file (file);
	gnome_mdi_add_child (mdi, GNOME_MDI_CHILD (doc));
        gnome_mdi_add_view (mdi, GNOME_MDI_CHILD (doc));        
        
	return GNOME_MDI_CHILD (doc);
}
#endif
