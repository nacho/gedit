/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-view.c
 * This file is part of gedit
 *
 * Copyright (C) 1998, 1999 Alex Roberts, Evan Lawrence
 * Copyright (C) 2000, 2001 Chema Celorio, Paolo Maggi 
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
 * Foundation, Inc., 59 Temple Place, Suite 330, 
 * Boston, MA 02111-1307, USA. 
 */
 
/*
 * Modified by the gedit Team, 1998-2001. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 */

#include "gedit-view.h"
#include "gedit-debug.h"
#include "gedit-menus.h"
#include "gedit-prefs.h"

struct _GeditViewPrivate
{
	GtkTextView *text_view;	

	GeditDocument *document;
};


static void gedit_view_class_init 	(GeditViewClass	*klass);
static void gedit_view_init 		(GeditView 	*view);
static void gedit_view_finalize 	(GObject 	*object);


static GtkVBoxClass *parent_class = NULL;


GType
gedit_view_get_type (void)
{
	static GType view_type = 0;

  	if (view_type == 0)
    	{
      		static const GTypeInfo our_info =
      		{
        		sizeof (GeditViewClass),
        		NULL,		/* base_init */
        		NULL,		/* base_finalize */
        		(GClassInitFunc) gedit_view_class_init,
        		NULL,           /* class_finalize */
        		NULL,           /* class_data */
        		sizeof (GeditView),
        		0,              /* n_preallocs */
        		(GInstanceInitFunc) gedit_view_init
      		};

      		view_type = g_type_register_static (GTK_TYPE_VBOX,
                				    "GeditView",
                                       	 	    &our_info,
                                       		    0);
    	}

	return view_type;
}

static void 
gedit_view_grab_focus (GtkWidget *widget)
{
	GeditView *view;
	
	gedit_debug (DEBUG_VIEW, "");
	
	view = GEDIT_VIEW (widget);
	
	gtk_widget_grab_focus (GTK_WIDGET (view->priv->text_view));
	
	/* Try to have a visible cursor */

	/* FIXME: Remove this dirty hack to have the cursor visible when we will 
	 * understand why it is not visible */
	GTK_WIDGET_SET_FLAGS (GTK_WIDGET (view->priv->text_view), GTK_HAS_FOCUS);
	g_object_set (G_OBJECT (view->priv->text_view), "cursor_visible", FALSE, NULL);
	g_object_set (G_OBJECT (view->priv->text_view), "cursor_visible", TRUE, NULL);
}

static void
gedit_view_class_init (GeditViewClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

  	parent_class = g_type_class_peek_parent (klass);

  	object_class->finalize = gedit_view_finalize;
	
	GTK_WIDGET_CLASS (klass)->grab_focus = gedit_view_grab_focus;
	
}

static void 
gedit_view_init (GeditView  *view)
{
	GtkTextView *text_view;
	GtkWidget *sw; /* the scrolled window */
	GdkColor background, text, selection, sel_text;

	/* FIXME
	static GtkWidget *popup_menu = NULL;
	*/
	
	gedit_debug (DEBUG_VIEW, "");

	view->priv = g_new0 (GeditViewPrivate, 1);

	view->priv->document = NULL;	

	/* Create the scrolled window */
	sw = gtk_scrolled_window_new (NULL, NULL);
	g_return_if_fail (sw != NULL);
	
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);

	text_view = GTK_TEXT_VIEW (gtk_text_view_new ());
	g_return_if_fail (text_view != NULL);
	view->priv->text_view = text_view;

	/*
	 *  Set tab, fonts, wrap mode, colors, etc. according
	 *  to preferences 
	 */
	if (!settings->use_default_font)
		gedit_view_set_font (view, settings->font);

	if (!settings->use_default_colors)
	{
		background.red = settings->bg [0];
		background.green = settings->bg [1];
		background.blue = settings->bg [2];

		text.red = settings->fg [0];
		text.green = settings->fg [1];
		text.blue = settings->fg [2];

		selection.red = settings->sel [0];
		selection.green = settings->sel [1];
		selection.blue = settings->sel [2];

		sel_text.red = settings->st [0];
		sel_text.green = settings->st [1];
		sel_text.blue = settings->st [2];

		gedit_view_set_colors (view, 
				&background, &text, &selection, &sel_text);
	}	

	gedit_view_set_wrap_mode (view, settings->wrap_mode);

	/* FIXME: uncomment when gedit_view_set_tab_size will work
	gedit_view_set_tab_size (view, 8);
	*/
	g_object_set (G_OBJECT (view->priv->text_view), "cursor_visible", TRUE, NULL);
	
	gtk_box_pack_start (GTK_BOX (view), sw, TRUE, TRUE, 0);
	gtk_container_add (GTK_CONTAINER (sw), GTK_WIDGET (view->priv->text_view));
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw),
                                             GTK_SHADOW_IN);
/*
	GTK_WIDGET_SET_FLAGS (GTK_WIDGET (view->priv->text_view), GTK_CAN_FOCUS);
*/
#if 0
	/* FIXME */
	/* Popup Menu */
	if (popup_menu == NULL) 
	{
		popup_menu = gnome_popup_menu_new (gedit_popup_menu);

		/* popup menu will not be destroyed when all the view are closed
		 * FIXME: destroy popup before exiting the program*/
		gtk_widget_ref (popup_menu);
	}

	/* The same popup menu is attached to all views */
	gnome_popup_menu_attach (popup_menu, GTK_WIDGET (view->priv->text_view), NULL);
#endif 
}

static void 
gedit_view_finalize (GObject *object)
{
	GeditView *view;

	gedit_debug (DEBUG_VIEW, "");

	g_return_if_fail (object != NULL);
	
   	view = GEDIT_VIEW (object);

	g_return_if_fail (GEDIT_IS_VIEW (view));
	g_return_if_fail (view->priv != NULL);

	g_return_if_fail (view->priv->document != NULL);
	g_object_unref (view->priv->document);
	view->priv->document = NULL;

	G_OBJECT_CLASS (parent_class)->finalize (object);

	g_free (view->priv);
}


/**
 * gedit_view_new:
 * @doc: a #GeditDocument
 * 
 * Creates a new #GeditView object displaying the @doc document. 
 * One document can be shared among many views. @doc cannot be NULL.
 * The view adds its own reference count to the document; 
 * it does not take over an existing reference.
 *
 * Return value: a new #GeditView
 **/
GeditView*
gedit_view_new (GeditDocument *doc)
{
	GeditView *view;
	
	gedit_debug (DEBUG_VIEW, "START");

	g_return_val_if_fail (doc != NULL, NULL);
	
	view = GEDIT_VIEW (g_object_new (GEDIT_TYPE_VIEW, NULL));
  	g_return_val_if_fail (view != NULL, NULL);
	
	gtk_text_view_set_buffer (view->priv->text_view, 
				  GTK_TEXT_BUFFER (doc));

	view->priv->document = doc;
	g_object_ref (view->priv->document);

	gtk_text_view_scroll_to_mark (view->priv->text_view,
				      gtk_text_buffer_get_mark (GTK_TEXT_BUFFER (doc), "insert"),
				      0, TRUE, 0.0, 1.0);

	gtk_widget_show_all (GTK_WIDGET (view));

	gedit_debug (DEBUG_VIEW, "END");

	return view;
}

void
gedit_view_cut_clipboard (GeditView *view)
{
	GtkTextBuffer* buffer = NULL;

	gedit_debug (DEBUG_VIEW, "");

	buffer = gtk_text_view_get_buffer (view->priv->text_view);
	g_return_if_fail (buffer != NULL);
	g_return_if_fail (view->priv->document != NULL);

	/* FIXME: what is default editability of a buffer? */
  	gtk_text_buffer_cut_clipboard (buffer,
				gtk_clipboard_get (GDK_NONE),
				!gedit_document_is_readonly (view->priv->document));
  	
	gtk_text_view_scroll_mark_onscreen (view->priv->text_view,
				gtk_text_buffer_get_mark (buffer,
				"insert"));
}

void
gedit_view_copy_clipboard (GeditView *view)
{
	GtkTextBuffer* buffer = NULL;

	gedit_debug (DEBUG_VIEW, "");

	buffer = gtk_text_view_get_buffer (view->priv->text_view);
	g_return_if_fail (buffer != NULL);
	
  	gtk_text_buffer_copy_clipboard (buffer,
				gtk_clipboard_get (GDK_NONE));
  	
	gtk_text_view_scroll_mark_onscreen (view->priv->text_view,
				gtk_text_buffer_get_mark (buffer,
				"insert"));
}

void
gedit_view_paste_clipboard (GeditView *view)
{
  	GtkTextBuffer* buffer = NULL;

	gedit_debug (DEBUG_VIEW, "");

	buffer = gtk_text_view_get_buffer (view->priv->text_view);
	g_return_if_fail (buffer != NULL);
	g_return_if_fail (view->priv->document != NULL);

	/* FIXME: what is default editability of a buffer? */
  	gtk_text_buffer_paste_clipboard (buffer,
				gtk_clipboard_get (GDK_NONE),
				NULL,
				!gedit_document_is_readonly (view->priv->document));
  	
	gtk_text_view_scroll_mark_onscreen (view->priv->text_view,
				gtk_text_buffer_get_mark (buffer,
				"insert"));
}

void
gedit_view_delete_selection (GeditView *view)
{
  	GtkTextBuffer* buffer = NULL;

	gedit_debug (DEBUG_VIEW, "");

	buffer = gtk_text_view_get_buffer (view->priv->text_view);
	g_return_if_fail (buffer != NULL);
	g_return_if_fail (view->priv->document != NULL);

	/* FIXME: what is default editability of a buffer? */
	gtk_text_buffer_delete_selection (buffer, 
				TRUE,
				!gedit_document_is_readonly (view->priv->document));
  	
	gtk_text_view_scroll_mark_onscreen (view->priv->text_view,
				gtk_text_buffer_get_mark (buffer,
				"insert"));
}

void
gedit_view_select_all (GeditView *view)
{
	/* FIXME: it does not select the last char of the buffer */
  	GtkTextBuffer* buffer = NULL;
	GtkTextIter start_iter, end_iter;
	
	gedit_debug (DEBUG_VIEW, "");

	buffer = gtk_text_view_get_buffer (view->priv->text_view);
	g_return_if_fail (buffer != NULL);

	gtk_text_buffer_get_start_iter (buffer, &start_iter);
	gtk_text_buffer_get_end_iter (buffer, &end_iter);

	gtk_text_buffer_place_cursor (buffer, &end_iter);

	gtk_text_buffer_move_mark (buffer,
                               gtk_text_buffer_get_mark (buffer, "selection_bound"),
                               &start_iter);
}

GeditDocument*	
gedit_view_get_document	(const GeditView *view)
{
	gedit_debug (DEBUG_VIEW, "");
	
	g_return_val_if_fail (GEDIT_IS_VIEW (view), NULL);
	g_return_val_if_fail (view->priv != NULL, NULL);

	return view->priv->document;
}

void
gedit_view_scroll_to_cursor (GeditView *view)
{
	GtkTextBuffer* buffer = NULL;

	gedit_debug (DEBUG_VIEW, "");

	g_return_if_fail (GEDIT_IS_VIEW (view));
	g_return_if_fail (view->priv != NULL);
	
	buffer = gtk_text_view_get_buffer (view->priv->text_view);
	g_return_if_fail (buffer != NULL);

	gtk_text_view_scroll_mark_onscreen (view->priv->text_view,
				gtk_text_buffer_get_mark (buffer,
				"insert"));
}

void 
gedit_view_set_colors (GeditView* view, GdkColor* backgroud, GdkColor* text,
		GdkColor* selection, GdkColor* sel_text)
{
	gedit_debug (DEBUG_VIEW, "");

	g_return_if_fail (GEDIT_IS_VIEW (view));

	gtk_widget_modify_base (GTK_WIDGET (view->priv->text_view), 
				GTK_STATE_NORMAL, backgroud);

	gtk_widget_modify_text (GTK_WIDGET (view->priv->text_view), 
				GTK_STATE_NORMAL, text);
	
	gtk_widget_modify_base (GTK_WIDGET (view->priv->text_view), 
				GTK_STATE_SELECTED, selection);

	gtk_widget_modify_text (GTK_WIDGET (view->priv->text_view), 
				GTK_STATE_SELECTED, sel_text);		

	gtk_widget_modify_base (GTK_WIDGET (view->priv->text_view), 
				GTK_STATE_ACTIVE, selection);

	gtk_widget_modify_text (GTK_WIDGET (view->priv->text_view), 
				GTK_STATE_ACTIVE, sel_text);		
}

void
gedit_view_set_font (GeditView* view, const gchar* font_name)
{
	PangoFontDescription *font_desc = NULL;

	gedit_debug (DEBUG_VIEW, "");

	g_return_if_fail (GEDIT_IS_VIEW (view));

	font_desc = pango_font_description_from_string (font_name);
	g_return_if_fail (font_desc != NULL);
	
	gtk_widget_modify_font (GTK_WIDGET (view->priv->text_view), font_desc);

	pango_font_description_free (font_desc);
}

void
gedit_view_set_wrap_mode (GeditView* view, GtkWrapMode wrap_mode)
{
	gedit_debug (DEBUG_VIEW, "");

	g_return_if_fail (GEDIT_IS_VIEW (view));
		
	gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (view->priv->text_view), wrap_mode);
}

void
gedit_view_set_tab_size (GeditView* view, gint tab_size)
{
	/* FIXME: it is broken */

	PangoTabArray *tab_array;

	gedit_debug (DEBUG_VIEW, "");

	g_return_if_fail (GEDIT_IS_VIEW (view));

	tab_array = pango_tab_array_new_with_positions (1,
                                             TRUE,
                                             PANGO_TAB_LEFT, tab_size*10);

	gtk_text_view_set_tabs (GTK_TEXT_VIEW (view->priv->text_view), tab_array);
	pango_tab_array_free (tab_array);

}




	


