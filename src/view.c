/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* Clean part of this 10-00, by Chema. divided the ugly code and the nice one */
/*
 * gedit
 *
 * Copyright (C) 1998, 1999 Alex Roberts, Evan Lawrence
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

#include "auto.h"
#include "undo.h"
#include "print.h"
#include "utils.h"
#include "file.h"
#include "menus.h"
#include "view.h"
#include "document.h"
#include "commands.h"
#include "prefs.h"
#include "window.h"

static GtkVBoxClass * parent_class;

struct _GeditViewClass
{
	GtkVBoxClass parent_class;
};

static void gedit_view_class_init (GeditViewClass *klass);
static void gedit_view_init (GeditView *view);

/**
 * gedit_view_get_type:
 * @void: 
 * 
 * Std. get type function
 * 
 * Return Value: 
 **/
/* ------------------  Cleaned stuff -------------------------*/
guint
gedit_view_get_type (void)
{
	static GtkType gedit_view_type = 0;

	if (!gedit_view_type)
	{
		GtkTypeInfo gedit_view_info =
		{
	  		"GeditView",
	  		sizeof (GeditView),
	  		sizeof (GeditViewClass),
	  		(GtkClassInitFunc) gedit_view_class_init,
	  		(GtkObjectInitFunc) gedit_view_init,
	  		(GtkArgSetFunc) NULL,
	  		(GtkArgGetFunc) NULL,
		};
	    
		gedit_view_type = gtk_type_unique (gtk_vbox_get_type (),
						   &gedit_view_info);
	  
	}
	 
	return gedit_view_type;
}

/**
 * gedit_view_finalize:
 * @object: 
 * 
 * View finalize function.
 **/
static void 
gedit_view_destroy (GtkObject *object)
{
	GeditView *view;

	gedit_debug (DEBUG_VIEW, "");

	view = GEDIT_VIEW (object);

	g_return_if_fail (GEDIT_IS_VIEW (view));

	g_free (view->toolbar);

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

/**
 * gedit_view_class_init:
 * @klass: 
 * 
 * Std GTk class init
 **/
static void
gedit_view_class_init (GeditViewClass *klass)
{
	GtkObjectClass *object_class;
	
	gedit_debug (DEBUG_VIEW, "");

	object_class = (GtkObjectClass *)klass;

	parent_class = gtk_type_class (gtk_vbox_get_type ());

	object_class->destroy = gedit_view_destroy;
}

/**
 * gedit_view_active:
 * @void: 
 * 
 * Get the active view
 * 
 * Return Value: The active view, NULL if thera aren't any views
 **/
GeditView *
gedit_view_active (void)
{
	GeditView *current_view = NULL;

	gedit_debug (DEBUG_VIEW, "");

	if (mdi->active_view)
		current_view = GEDIT_VIEW (mdi->active_view);

	return current_view;
}

/**
 * gedit_view_get_window_position:
 * @view: View to get the position from
 * 
 * Get the visible position of a view
 *
 * Return Value: the current visible position of the view
 **/
gfloat
gedit_view_get_window_position (GeditView *view)
{
	gedit_debug (DEBUG_VIEW, "");

	g_return_val_if_fail (GEDIT_IS_VIEW (view), 0.0);
	
	return GTK_ADJUSTMENT(view->text->vadj)->value;
}

/**
 * gedit_view_set_window_position:
 * @view: View to set 
 * @position: Position.
 * 
 * Set the visible view position
 **/
void
gedit_view_set_window_position (GeditView *view, gfloat position)
{
	gedit_debug (DEBUG_VIEW, "");

	g_return_if_fail (GEDIT_IS_VIEW (view));

	gtk_adjustment_set_value (GTK_ADJUSTMENT(view->text->vadj),
				  position);
}

/**
 * gedit_view_set_window_position_from_lines:
 * @view: 
 * @line: line to move to 
 * @lines: total number of line in the view (document)
 * 
 * Given a line number and the toltal number of lines, move to visible part of
 * the view so that line is visible. This is usefull so that the screen does not
 * scroll and we can move rapidly to a different visible section of the view
 **/
void
gedit_view_set_window_position_from_lines (GeditView *view, guint line, guint lines)
{
	gfloat upper;
	gfloat value;
	gfloat page_size;
	gfloat gt;
	gfloat min;
	gfloat max;
	
	gedit_debug (DEBUG_VIEW, "");

	g_return_if_fail (GEDIT_IS_VIEW (view));
	g_return_if_fail (line <= lines);
	
	upper     = GTK_ADJUSTMENT (view->text->vadj)->upper;
	value     = GTK_ADJUSTMENT (view->text->vadj)->value;
	page_size = GTK_ADJUSTMENT (view->text->vadj)->page_size;

	min = value;
	max = value + (0.9 * page_size); /* We don't want to be in the lower 10%*/
	
	gt = upper * (float)(line-1) / (float)(lines-1);

	/* No need to move */
	if (gt > min && gt < max)
		return;

	gedit_view_set_window_position (view, gt);
	
}

/**
 * gedit_view_set_selection:
 * @view: View to set the selection 
 * @start: start of selection to be set
 * @end: end of selection to be set
 * 
 * Sets the selection from start to end. If start & end are 0, it clears the
 * selection
 **/
void
gedit_view_set_selection (GeditView *view, guint start, guint end)
{
	gedit_debug (DEBUG_VIEW, "");

	g_return_if_fail (GEDIT_IS_VIEW (view));

	if (start == 0 && end == 0)
		gtk_text_set_point (view->text, GTK_EDITABLE(view->text)->selection_end_pos);

	gtk_editable_select_region (GTK_EDITABLE (view->text), start, end);

	if (start == 0 && end == 0)
	{
		gtk_text_insert (view->text, NULL, NULL, NULL, " ", 1);
		gtk_text_backward_delete (view->text, 1);
	}

}

/**
 * gedit_view_get_selection:
 * @view: View to get the selection from 
 * @start: return here the start position of the selection 
 * @end: return here the end position of the selection
 * 
 * Gets the current selection for View
 * 
 * Return Value: TRUE if there is a selection active, FALSE if not
 **/
gint
gedit_view_get_selection (GeditView *view, guint *start, guint *end)
{
	guint start_pos, end_pos;

	gedit_debug (DEBUG_VIEW, "");

	g_return_val_if_fail (GEDIT_IS_VIEW (view), FALSE);

	start_pos = GTK_EDITABLE(view->text)->selection_start_pos;
        end_pos   = GTK_EDITABLE(view->text)->selection_end_pos;

	/* The user can select from end to start too. If so, swap it*/
	if (end_pos < start_pos)
	{
		guint swap_pos;
		swap_pos  = end_pos;
		end_pos   = start_pos;
		start_pos = swap_pos;
	}

	if (start != NULL)
		*start = start_pos;
	if (end != NULL)
		*end = end_pos;

	if ((start_pos > 0 || end_pos > 0) && (start_pos != end_pos))
		return TRUE;
	else 
		return FALSE;
}


/**
 * gedit_view_set_position:
 * @view: View to set the cusor
 * @pos: position to set the curor at
 * 
 * set the cursor position position for this view
 **/
void
gedit_view_set_position (GeditView *view, gint pos)
{
	gedit_debug (DEBUG_VIEW, "");

	g_return_if_fail (GEDIT_IS_VIEW (view));

	gtk_text_set_point       (view->text, pos);
	gtk_text_insert          (view->text, NULL, NULL, NULL, " ", 1);
	gtk_text_backward_delete (view->text, 1);

}

/**
 * gedit_view_get_position:
 * @view: View to get the cursor position
 * 
 * get the cursor position in this view
 * 
 * Return Value: the current cursor position
 **/
guint
gedit_view_get_position (GeditView *view)
{
	guint start_pos, end_pos;
	
	gedit_debug (DEBUG_VIEW, "");

	g_return_val_if_fail (GEDIT_IS_VIEW (view), 0);

	if (gedit_view_get_selection (view, &start_pos, &end_pos))
	{
		return end_pos;
	}

#if 0 /* I don't think we need this, but I am going to leave it here
	 just in case it is to work around yet another gtktext bug*/
	gtk_text_freeze (view->text);
	gtk_text_thaw   (view->text);
#endif	

	return gtk_editable_get_position (GTK_EDITABLE(view->text));
}





/**
 * gedit_view_set_readonly:
 * @view: View to set/unset readonly
 * @readonly: TRUE to turn on the readonly flag, FALSE otherwise
 * 
 * Change the Readonly atribute of a view. Update the title accordingly
 **/
void
gedit_view_set_readonly (GeditView *view, gint readonly)
{
	gchar * doc_name = NULL;

	gedit_debug (DEBUG_VIEW, "");

	g_return_if_fail (GEDIT_IS_VIEW (view));
		
	view->readonly = readonly;
	gtk_text_set_editable (view->text, !view->readonly);
	
	doc_name = gedit_document_get_tab_name (view->doc, TRUE);
	gnome_mdi_child_set_name (GNOME_MDI_CHILD (view->doc), doc_name);
	gedit_document_set_title (view->doc);
	g_free (doc_name);
}

/**
 * gedit_view_get_document:
 * @view: 
 * 
 * get the document from a view
 * 
 * Return Value: document for the @view, NULL on error
 **/
GeditDocument *
gedit_view_get_document (GeditView *view)
{
        if (view == NULL)
                return NULL;

        return view->doc;
}




























































static void
gedit_view_update_line_indicator (void)
{
	static char col [32];

	return;
	
	/* FIXME: Disable by chema for 0.7.0 . this hack is not working correctly */
	gedit_debug (DEBUG_VIEW, "");

	sprintf (col, "Column: %d",
		 GEDIT_VIEW(gedit_view_active())->text->cursor_pos_x/7);

	if (settings->show_status)
	{
		GnomeApp *app = gedit_window_active_app();
		gnome_appbar_set_status (GNOME_APPBAR(app->statusbar), col);
	}
}

static gint
gedit_event_button_press (GtkWidget *widget, GdkEventButton *event)
{
	gedit_debug (DEBUG_VIEW, "");

	gedit_view_update_line_indicator ();
	
	return FALSE;
}

static gint
gedit_event_key_press (GtkWidget *w, GdkEventKey *event)
{
	gedit_debug (DEBUG_VIEW, "");

	gedit_view_update_line_indicator ();

	if (event->state & GDK_MOD1_MASK)
	{
		gtk_signal_emit_stop_by_name (GTK_OBJECT (w), "key_press_event");
		return FALSE;
	}

	/* Control key related */
	if (event->state & GDK_CONTROL_MASK)
	{
		switch (event->keyval)
		{
			/* This is nasty, NASTY NASTY !!!!!!!
			   since it is dependent on the current languaje */
		case 's':
			file_save_cb (w, NULL);
	    		break;
		case 'p':
	    		gedit_print_cb (w, NULL);
	    		break;
		case 'n':
			return FALSE;
			break;
		case 'w':
	    		file_close_cb (w, NULL);
			/* If the user cancels the file_close action, the cut still happens*/
			gtk_signal_emit_stop_by_name (GTK_OBJECT (w), "key_press_event");
			return TRUE;
		case 'z':
			/* Undo is getting called twice, 1 thru this function
			   and 1 time thru the aceleratior. Chema 
	    		gedit_undo_do (w, NULL);
			*/
	    		break;
		case 'r':
			/* Same as undo_do 
	    		gedit_undo_redo (w, NULL);
			*/
	    		break;
		case 'q':
			file_quit_cb (w, NULL);
	    		break;
		default:
	    		return TRUE;
	    		break;
		}
	}

	return TRUE;
}

static void
gedit_view_insert (GeditView *view, guint position, const gchar * text, gint length)
{
	gint p1;
	
	g_return_if_fail (length > 0);
	g_return_if_fail (text != NULL);
	
	gedit_debug (DEBUG_VIEW, "");

	if (!GTK_WIDGET_MAPPED (GTK_WIDGET (view->text)))
		g_warning ("Inserting text into an unmapped text widget (%i)", length);

	p1 = gtk_text_get_point (view->text);
	gtk_text_set_point (view->text, position);
	gtk_text_insert    (view->text, NULL,
			    NULL, NULL, text,
			    length);
}

static void
gedit_views_insert (GeditDocument *doc, guint position, gchar * text,
		    gint length, GeditView * view_exclude)
{
	gint i;
	GeditView *nth_view;

	gedit_debug (DEBUG_VIEW, "");

	g_return_if_fail (doc!=NULL);

	if (length < 1)
		return;

	/* Why do we have to call view_text_changed_cb ? why is the "chaged" signal not calling it ?. Chema */
	if (!doc->changed)
		if (g_list_length (doc->views))
			gedit_view_text_changed_cb (NULL,
						    (gpointer) g_list_nth_data (doc->views, 0));
	
	for (i = 0; i < g_list_length (doc->views); i++)
	{
		nth_view = g_list_nth_data (doc->views, i);

		if (nth_view == view_exclude)
			continue;

		gedit_view_insert (nth_view, position, text, length);
	}  
}

void
doc_insert_text_real_cb (GtkWidget *editable, const guchar *insertion_text,int length,
			 int *pos, GeditView *view, gint exclude_this_view, gint undo)
{
	gint position = *pos;
	GeditDocument *doc;
	guchar *text_to_insert;

	gedit_debug (DEBUG_VIEW, "");

	doc = view->doc;

	/* This strings are not be terminated with a null cero */
	text_to_insert = g_strndup (insertion_text, length);

	if (undo)
		gedit_undo_add (text_to_insert, position, (position + length), GEDIT_UNDO_ACTION_INSERT, doc, view);

	if (!exclude_this_view)
		gedit_views_insert (doc, position, text_to_insert, length, NULL);
	else
		gedit_views_insert (doc, position, text_to_insert, length, view);

	g_free (text_to_insert);
}

static void
doc_insert_text_cb (GtkWidget *editable, const guchar *insertion_text,int length,
		    int *pos, GeditView *view)
{
	gedit_debug (DEBUG_VIEW, "");

	doc_insert_text_real_cb (editable, insertion_text, length, pos, view, TRUE, TRUE);
}

/* Work arround for a gtktext bug*/
static gint
gedit_view_refresh_line_hack (GeditView *view)
{
	gtk_text_insert          (view->text, NULL, NULL, NULL, " ", 1);
	gtk_text_backward_delete (view->text, 1);

	return FALSE;
}

static void
gedit_view_delete (GeditView *view, guint position, gint length, gboolean exclude_this_view)
{
	gfloat upper;
	gfloat value;
	gfloat page_size;
	guint p1;

	gedit_debug (DEBUG_VIEW, "");

	if (!exclude_this_view) {
		p1 = gtk_text_get_point (view->text);
		gtk_text_set_point      (view->text, position);
		gtk_text_forward_delete (view->text, length);
	}
	
	upper     = GTK_ADJUSTMENT (view->text->vadj)->upper;
	value     = GTK_ADJUSTMENT (view->text->vadj)->value;
	page_size = GTK_ADJUSTMENT (view->text->vadj)->page_size;

	/* Only add the hack when we need it, if we add it all the time
	   the reaplace all is ugly. This problem only happens when we are
	   at the bottom of the file and there is a scroolbar*/
	if ((page_size != upper) && (value + page_size >= upper))
		gtk_idle_add ((GtkFunction) gedit_view_refresh_line_hack, view);
	
}


static void
gedit_views_delete (GeditDocument *doc, guint start_pos, guint end_pos, GeditView *view_exclude)
{
	GeditView *nth_view;
	gint n;

	gedit_debug (DEBUG_VIEW, "");

	g_return_if_fail (doc!=NULL);
	g_return_if_fail (end_pos > start_pos);

	for (n = 0; n < g_list_length (doc->views); n++)
	{
		nth_view = g_list_nth_data (doc->views, n);

		gedit_view_delete (nth_view, start_pos, end_pos-start_pos, nth_view == view_exclude);
	}
}

void
doc_delete_text_real_cb (GtkWidget *editable, int start_pos, int end_pos,
			 GeditView *view, gint exclude_this_view, gint undo)
{
	GeditDocument *doc;
	guchar *text_to_delete;

	gedit_debug (DEBUG_VIEW, "");

	g_return_if_fail (view != NULL);
	doc = view->doc;
	g_return_if_fail (doc != NULL);

	if (start_pos == end_pos )
	     return;

	text_to_delete = gtk_editable_get_chars (GTK_EDITABLE(editable), start_pos, end_pos);
	if (undo)
		gedit_undo_add (text_to_delete, start_pos, end_pos, GEDIT_UNDO_ACTION_DELETE, doc, view);
	g_free (text_to_delete);

	if (!exclude_this_view)
		gedit_views_delete (doc, start_pos, end_pos, NULL);
	else
		gedit_views_delete (doc, start_pos, end_pos, view);

}


static void
doc_delete_text_cb (GtkWidget *editable, int start_pos, int end_pos, GeditView *view)
{
	gedit_debug (DEBUG_VIEW, "");

	doc_delete_text_real_cb (editable, start_pos, end_pos, view, TRUE, TRUE);
}


static void
gedit_view_init (GeditView *view)
{
	GtkWidget *window;
	static GtkWidget *menu = NULL;
	GtkStyle *style;
	GdkColor *bg;
	GdkColor *fg;
	
	gedit_debug (DEBUG_VIEW, "");

	/* Vbox */
	/*
	view->vbox = gtk_vbox_new (TRUE, TRUE);
	gtk_container_add (GTK_CONTAINER (view), view->vbox);
	*/
	
	/* Create the upper split screen */
	window = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (window),
					GTK_POLICY_NEVER,
					GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start (GTK_BOX (view), window, TRUE, TRUE, 0);
	
	/* Create and configure the GtkText Widget */
	view->text = GTK_TEXT (gtk_text_new (NULL, NULL));
	gedit_view_set_tab_size (view, settings->tab_size);
	gtk_text_set_editable  (view->text, !view->readonly);
	gtk_text_set_word_wrap (view->text, settings->word_wrap);
	
	/* Toolbar */
	view->toolbar = NULL;

        /* Hook the button & key pressed events */
	gtk_signal_connect (GTK_OBJECT (view->text), "button_press_event",
			    GTK_SIGNAL_FUNC (gedit_event_button_press), NULL);
	gtk_signal_connect (GTK_OBJECT (view->text), "key_press_event",
			    GTK_SIGNAL_FUNC (gedit_event_key_press), NULL);

	/* Handle Auto Indent */
	gtk_signal_connect_after (GTK_OBJECT (view->text), "insert_text",
				  GTK_SIGNAL_FUNC (auto_indent_cb), view);

	/* Connect the insert & delete text callbacks */
	gtk_signal_connect (GTK_OBJECT (view->text), "insert_text",
			    GTK_SIGNAL_FUNC (doc_insert_text_cb), (gpointer) view);
	gtk_signal_connect (GTK_OBJECT (view->text), "delete_text",
			    GTK_SIGNAL_FUNC (doc_delete_text_cb), (gpointer) view);
	gtk_container_add (GTK_CONTAINER (window), GTK_WIDGET (view->text));
	
	/* View changed signal */
	view->view_text_changed_signal =
		gtk_signal_connect (GTK_OBJECT(view->text), "changed",
				    GTK_SIGNAL_FUNC (gedit_view_text_changed_cb), view);

	gtk_text_set_point (view->text, 0);

	/* Set colors */
	style = gtk_style_copy (gtk_widget_get_style (GTK_WIDGET (view->text)));

	bg = &style->base[0];
	bg->red   = settings->bg[0];
	bg->green = settings->bg[1];
	bg->blue  = settings->bg[2];

	fg = &style->text[0];
	fg->red   = settings->fg[0];
	fg->green = settings->fg[1];
	fg->blue  = settings->fg[2];
	
   	gtk_widget_set_style (GTK_WIDGET(view->text), style);
	
	gtk_style_unref (style);

	/* Popup Menu */
	if (menu == NULL) 
	{
		menu = gnome_popup_menu_new (popup_menu);
		/* popup menu will not be destroyed when all the view are closed
		 * FIXME: destroy popup before exiting the program*/
		gtk_widget_ref(menu);
	}

	/* The same popup menu is attached to all views */
	gnome_popup_menu_attach (menu, GTK_WIDGET (view->text), view);

	gtk_widget_show_all   (GTK_WIDGET (view));
	gtk_widget_grab_focus (GTK_WIDGET (view->text));
}


void
gedit_view_changed_cb (GnomeMDI *mdi, GtkWidget *old_view)
{
	GeditView *view = gedit_view_active();
	GeditDocument *doc = view->doc;
	gint undo_state, redo_state;

	gedit_debug (DEBUG_VIEW, "start");

	g_return_if_fail (view!=NULL);
		
	gtk_widget_grab_focus (GTK_WIDGET (view->text));
	gedit_document_set_title (view->doc);

	view->app = gedit_window_active_app();
	
	gedit_view_load_widgets (view);
	
	if (g_list_length(doc->undo) == 0)
		undo_state = GEDIT_UNDO_STATE_FALSE;
	else
		undo_state = GEDIT_UNDO_STATE_TRUE;
	if (g_list_length(doc->redo) == 0)
		redo_state = GEDIT_UNDO_STATE_FALSE;
	else
		redo_state = GEDIT_UNDO_STATE_TRUE;
	gedit_view_set_undo (view, undo_state, redo_state);
	gedit_view_set_undo (view, GEDIT_UNDO_STATE_REFRESH, GEDIT_UNDO_STATE_REFRESH);
	gnome_app_install_menu_hints(view->app, gnome_mdi_get_child_menu_info(view->app));

	gedit_window_set_view_menu_sensitivity (gedit_window_active_app());
	gedit_window_set_widgets_sensitivity_ro (gedit_window_active_app(), doc->readonly);

	gedit_debug (DEBUG_VIEW, "end");
}

void
gedit_view_text_changed_cb (GtkWidget *w, gpointer cbdata)
{
	GeditView *view;
	gchar* doc_name;
	
	gedit_debug (DEBUG_VIEW, "");

	view = GEDIT_VIEW (cbdata);
	g_return_if_fail (view != NULL);

	if (view->doc->changed)
		return;
	
	view->doc->changed = TRUE;

	/* Disconect this signal */
	gtk_signal_disconnect (GTK_OBJECT(view->text), (gint) view->view_text_changed_signal);

	/* Set the title ( so that we add the "modified" string to it )*/
	gedit_document_set_title (view->doc);
	
	doc_name = gedit_document_get_tab_name (view->doc, TRUE);
	gnome_mdi_child_set_name (GNOME_MDI_CHILD (view->doc), doc_name);
	g_free(doc_name);
}















GtkWidget *
gedit_view_new (GeditDocument *doc)
{
	GeditView *view;
	guchar *document_buffer;

	gedit_debug (DEBUG_VIEW, "");

	if (doc == NULL)
		return NULL;

	view = gtk_type_new (gedit_view_get_type ());

	view->app = NULL;
	view->doc = doc;
	doc->views = g_list_append (doc->views, view);

	if (g_list_length (doc->views) > 1) {
		document_buffer = gedit_document_get_buffer (view->doc);
		gedit_view_insert (view, 0, document_buffer, strlen (document_buffer));
		g_free (document_buffer);
	}

	return GTK_WIDGET (view);
}



/**
 * gedit_view_set_font:
 * @view: View that we're going to change the font for.
 * @fontname: String name "-b&h-lucida-blah-..." of the font to load
 *
 *
 **/
void
gedit_view_set_font (GeditView *view, gchar *fontname)
{
	GtkStyle *style;
	GdkFont *font = NULL;

	gedit_debug (DEBUG_VIEW, "");
	
	style = gtk_style_copy (gtk_widget_get_style (GTK_WIDGET((view)->text)));
  	
  	if (settings->use_fontset)
		font = gdk_fontset_load (fontname);
  	else
		font = gdk_font_load (fontname);

	if (font != NULL)
	{
		gdk_font_unref (style->font);
		style->font = font;
	}
	else
		g_warning ("Unable to load font ``%s''", fontname);

  	gtk_widget_set_style (GTK_WIDGET((view)->text), style);

	gtk_style_unref (style);
}



void
gedit_view_add_cb (GtkWidget *widget, gpointer data)
{
	GnomeMDIChild *child;
	GeditView *view;
	guchar * buffer;

	gedit_debug (DEBUG_VIEW, "");

	view = gedit_view_active();

	if (view)
	{
		view = gedit_view_active();
		buffer = gedit_document_get_buffer (view->doc);
		child = gnome_mdi_get_child_from_view (GTK_WIDGET(view));
		gnome_mdi_add_view (mdi, child);
		view = gedit_view_active();
		gedit_view_insert ( view, 0, buffer, strlen (buffer));
		/* Move the window to the top after inserting */
		gedit_view_set_window_position (view, 0);
		g_free (buffer);
		gedit_window_set_view_menu_sensitivity (gedit_window_active_app());
		gedit_window_set_widgets_sensitivity_ro (gedit_window_active_app(), view->doc->readonly);
	}

}

void
gedit_view_remove (GeditView *view)
{
	GeditDocument *doc;
	
	doc = view->doc;
	
	g_return_if_fail (doc != NULL);

	if (g_list_length (doc->views) < 2)
	{
		gnome_app_error (gedit_window_active_app(), _("You can't remove the last view of a document."));
		return;
	}

	/* First, we remove the view from the document's list */
	doc->views = g_list_remove (doc->views, view);

	/* Now, we can remove the view proper */
	gnome_mdi_remove_view (mdi, GTK_WIDGET(view), FALSE);
	
	gedit_document_set_title (doc);

	gedit_window_set_view_menu_sensitivity (gedit_window_active_app());
	gedit_window_set_widgets_sensitivity_ro (gedit_window_active_app(), doc->readonly);
}

void
gedit_view_remove_cb (GtkWidget *widget, gpointer data)
{
	GeditView *view;

	gedit_debug (DEBUG_VIEW, "");

	view = gedit_view_active();
	
	if (view == NULL)
		return;

	gedit_view_remove (view);
}



void
gedit_view_load_widgets (GeditView *view)
{
	GnomeUIInfo *toolbar_ui_info = NULL;
	GnomeUIInfo *menu_ui_info = NULL;
	gint count, sub_count;

	gedit_debug (DEBUG_VIEW, "");
	
	g_return_if_fail (view->app != NULL);

	toolbar_ui_info = gnome_mdi_get_toolbar_info (view->app);
	menu_ui_info	= gnome_mdi_get_menubar_info (view->app);
/*
	menu_ui_info    = gnome_mdi_get_child_menu_info (view->app);
*/
	g_return_if_fail (toolbar_ui_info != NULL);
	g_return_if_fail (menu_ui_info != NULL);

	if (!view->toolbar)
		view->toolbar = g_malloc (sizeof (GeditToolbar));
	
	view->toolbar->undo_button    = NULL;
	view->toolbar->redo_button    = NULL;
	view->toolbar->undo_menu_item = NULL;
	view->toolbar->redo_menu_item = NULL;
	
	count = 0;
	while (toolbar_ui_info[count].type != GNOME_APP_UI_ENDOFINFO)
	{
		if (toolbar_ui_info [count].moreinfo == gedit_undo_undo)
		{
			view->toolbar->undo_button = toolbar_ui_info[count].widget;
#if 0			
			g_print ("undo button found ..   count %i widget 0x%x  view 0x%x\n",
				 count,
				 (gint) toolbar_ui_info[count].widget,
				 (gint) view);
#endif			
		}
		if (toolbar_ui_info [count].moreinfo == gedit_undo_redo)
		{
			view->toolbar->redo_button = toolbar_ui_info[count].widget;
#if 0			
			g_print ("redo button found ..   count %i widget 0x%x view 0x%x\n",
				 count,
				 (gint) toolbar_ui_info[count].widget,
				 (gint) view);
#endif			
		}
		count++;
	}
	count = 0;
	while (menu_ui_info[count].type != GNOME_APP_UI_ENDOFINFO)
	{
		/* FIXME: The correct way to do this is to do recurse the find function
		   Chema.*/
		if (menu_ui_info[count].type == GNOME_APP_UI_SUBTREE_STOCK ||
		    menu_ui_info[count].type == GNOME_APP_UI_SUBTREE)
		{
			GnomeUIInfo *sub_menu_ui_info = menu_ui_info[count].moreinfo;
			sub_count = 0;
			while (sub_menu_ui_info[sub_count].type != GNOME_APP_UI_ENDOFINFO)
			{
				if (sub_menu_ui_info [sub_count].moreinfo == gedit_undo_undo)
				{
					view->toolbar->undo_menu_item = sub_menu_ui_info[sub_count].widget;
					g_return_if_fail (GTK_IS_WIDGET(view->toolbar->undo_menu_item));
#if 0					
					g_print ("<-undo menu item Widget 0x%x      view 0x%x toolbar: 0x%x\n",
						 (gint) view->toolbar->undo_menu_item,
						 (gint) view,
						 (gint) view->toolbar);
#endif					
				}
				if (sub_menu_ui_info [sub_count].moreinfo == gedit_undo_redo)
				{
					view->toolbar->redo_menu_item = sub_menu_ui_info[sub_count].widget;
					g_return_if_fail (GTK_IS_WIDGET(view->toolbar->redo_menu_item));
#if 0					
					g_print ("<-redo menu item found count %i widget 0x%x view 0x%x toolbar: 0x%x\n",
						 sub_count,
						 (gint) view->toolbar->redo_menu_item,
						 (gint) view,
						 (gint) view->toolbar);
#endif					
				}
				sub_count++;
			}
		}
		else
		{
			g_warning ("BORK !\n");
			if (menu_ui_info [count].moreinfo == gedit_undo_undo)
				view->toolbar->undo_menu_item = menu_ui_info[count].widget;
			if (menu_ui_info [count].moreinfo == gedit_undo_redo)
				view->toolbar->redo_menu_item = menu_ui_info[count].widget;
		}
		count++;
	}

	g_return_if_fail (view->toolbar->undo_button != NULL);
	g_return_if_fail (view->toolbar->redo_button != NULL);
	g_return_if_fail (view->toolbar->undo_menu_item != NULL);
	g_return_if_fail (view->toolbar->redo_menu_item != NULL);
}

void
gedit_view_set_undo (GeditView *view, gint undo_state, gint redo_state)
{
	gedit_debug (DEBUG_VIEW, "");

#if 0
	g_print ("\nSet_undo. view:0x%x Widget: 0x%x  UNDO:%i  REDO:%i\n",
		 (gint) view,
		 (gint) view->toolbar->undo_menu_item,
		 undo_state,
		 redo_state);
#endif	
	g_return_if_fail (view != NULL);
	g_return_if_fail (view->toolbar != NULL);
	g_return_if_fail (view->toolbar->undo_button != NULL);
	g_return_if_fail (view->toolbar->redo_button != NULL);
	g_return_if_fail (view->toolbar->undo_menu_item != NULL);
	g_return_if_fail (view->toolbar->redo_menu_item != NULL);
	g_return_if_fail (GTK_IS_WIDGET (view->toolbar->undo_button));
	g_return_if_fail (GTK_IS_WIDGET (view->toolbar->redo_button));
	g_return_if_fail (GTK_IS_WIDGET (view->toolbar->undo_menu_item));
	g_return_if_fail (GTK_IS_WIDGET (view->toolbar->redo_menu_item));
		 
/*
  g_print ("Undo->button %i Redo->menu_button %i\n",
		 (guint) view->toolbar->undo_button,
		 (guint) view->toolbar->redo_button);
	g_print ("Undo->menu_item %i Redo->menu_item %i\n",
		 (guint) view->toolbar->undo_menu_item,
		 (guint) view->toolbar->redo_menu_item);
	g_print ("undo state %i redo state %i\n", undo_state, redo_state);
	g_print ("view->toolbar->undo:%i view->toolbar->redo:%i FALSE:%i TRUE:%i\n\n\n",
		 view->toolbar->undo,
		 view->toolbar->redo,
		 FALSE, TRUE);
*/

	/* Set undo */
	switch (undo_state)
	{
	case GEDIT_UNDO_STATE_TRUE:
		if (!view->toolbar->undo)
		{
			view->toolbar->undo = TRUE;
			gtk_widget_set_sensitive (view->toolbar->undo_button, TRUE);
			gtk_widget_set_sensitive (view->toolbar->undo_menu_item, TRUE);
		}
		break;
	case GEDIT_UNDO_STATE_FALSE:
		if (view->toolbar->undo)
		{
			view->toolbar->undo = FALSE;
			gtk_widget_set_sensitive (view->toolbar->undo_button, FALSE);
			gtk_widget_set_sensitive (view->toolbar->undo_menu_item, FALSE);
		}
		break;
	case GEDIT_UNDO_STATE_UNCHANGED:
		break;
	case GEDIT_UNDO_STATE_REFRESH:
		gtk_widget_set_sensitive (view->toolbar->undo_button, view->toolbar->undo);
		gtk_widget_set_sensitive (view->toolbar->undo_menu_item, view->toolbar->undo);
		break;
	default:
		g_warning ("Undo state not recognized");
	}
	
	/* Set redo*/
	switch (redo_state)
	{
	case GEDIT_UNDO_STATE_TRUE:
		if (!view->toolbar->redo)
		{
			view->toolbar->redo = TRUE;
			gtk_widget_set_sensitive (view->toolbar->redo_button, TRUE);
			gtk_widget_set_sensitive (view->toolbar->redo_menu_item, TRUE);
		}
		break;
	case GEDIT_UNDO_STATE_FALSE:
		if (view->toolbar->redo)
		{
			view->toolbar->redo = FALSE;
			gtk_widget_set_sensitive (view->toolbar->redo_button, FALSE);
			gtk_widget_set_sensitive (view->toolbar->redo_menu_item, FALSE);
		}
		break;
	case GEDIT_UNDO_STATE_UNCHANGED:
		break;
	case GEDIT_UNDO_STATE_REFRESH:
		gtk_widget_set_sensitive (view->toolbar->redo_button, view->toolbar->redo);
		gtk_widget_set_sensitive (view->toolbar->redo_menu_item, view->toolbar->redo);
		break;
	default:
		g_warning ("Redo state not recognized");
	}
}
		

/**
 * gedit_view_set_tab_size:
 * @view: 
 * @tab_size: 
 * 
 * Set the gtk_text tab size
 **/
void
gedit_view_set_tab_size (GeditView *view, gint tab_size)
{
	GList *l;
	gint i;

	g_return_if_fail (GEDIT_IS_VIEW (view));

	l = GTK_TEXT (view->text)->tab_stops;
	g_list_free (l);
		
	l = NULL;
	for (i = 0; i < 50; i++)
		l = g_list_prepend (l, GINT_TO_POINTER (tab_size));
	GTK_TEXT (view->text)->tab_stops = l;

}



