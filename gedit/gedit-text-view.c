/*
 * gedit-view.c
 * This file is part of gedit
 *
 * Copyright (C) 1998, 1999 Alex Roberts, Evan Lawrence
 * Copyright (C) 2000, 2002 Chema Celorio, Paolo Maggi 
 * Copyright (C) 2003-2005 Paolo Maggi  
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
 * Modified by the gedit Team, 1998-2005. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes.
 *
 * $Id$ 
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <stdlib.h>

#include <gdk/gdkkeysyms.h>

#include <glib/gi18n.h>

#include "gedit-view-interface.h"
#include "gedit-text-view.h"
#include "gedit-text-buffer.h"
#include "gedit-debug.h"
#include "gedit-prefs-manager.h"
#include "gedit-prefs-manager-app.h"
#include "gedit-marshal.h"
#include "gedit-utils.h"


#define GEDIT_TEXT_VIEW_SCROLL_MARGIN 0.02
#define GEDIT_TEXT_VIEW_SEARCH_DIALOG_TIMEOUT (30*1000) /* 30 seconds */

#define MIN_SEARCH_COMPLETION_KEY_LEN	3
#define GEDIT_TEXT_VIEW_NAME _("Text Editor")

#define GEDIT_TEXT_VIEW_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), GEDIT_TYPE_TEXT_VIEW, GeditTextViewPrivate))

typedef enum
{
	GOTO_LINE,
	SEARCH
} SearchMode;

enum
{
	TARGET_URI_LIST = 100
};

struct _GeditTextViewPrivate
{
	SearchMode   search_mode;
	
	GtkTextIter  start_search_iter;

	/* used to restore the search state if an
	 * incremental search is cancelled
	 */
 	gchar       *old_search_text;
	guint        old_search_flags;

	/* used to remeber the state of the last
	 * incremental search (the document search
	 * state may be changed by the search dialog)
	 */
	guint        search_flags;
	gboolean     wrap_around;

	GtkWidget   *search_window;
	GtkWidget   *search_entry;

	guint        typeselect_flush_timeout;
	guint        search_entry_changed_id;
	
	gboolean     disable_popdown;
	
	GtkTextBuffer *current_buffer;
};

/* The search entry completion is shared among all the views */
GtkListStore *search_completion_model = NULL;

static void	gedit_text_view_destroy		(GtkObject        *object);
static void	gedit_text_view_finalize		(GObject          *object);
static gint     gedit_text_view_focus_out		(GtkWidget        *widget,
						 GdkEventFocus    *event);
static gboolean gedit_text_view_drag_motion		(GtkWidget        *widget,
						 GdkDragContext   *context,
						 gint              x,
						 gint              y,
						 guint             timestamp);
static void     gedit_text_view_drag_data_received   (GtkWidget        *widget,
						 GdkDragContext   *context,
						 gint              x,
						 gint              y,
						 GtkSelectionData *selection_data,
						 guint             info,
						 guint             timestamp);
static gboolean gedit_text_view_drag_drop	(GtkWidget        *widget,
	      					 GdkDragContext   *context,
	      					 gint              x,
	      					 gint              y,
	      					 guint             timestamp);
static gboolean	gedit_text_view_button_press_event	(GtkWidget        *widget,
						 GdkEventButton   *event);

static gboolean start_interactive_search	(GeditTextView        *view);
static gboolean start_interactive_goto_line	(GeditTextView        *view);
static gboolean reset_searched_text		(GeditTextView        *view);

static void	hide_search_window 		(GeditTextView        *view,
						 gboolean          cancel);


static gint	gedit_text_view_expose	 	(GtkWidget        *widget,
						 GdkEventExpose   *event);
static void 	search_highlight_updated_cb	(GeditDocument    *doc,
						 GtkTextIter      *start,
						 GtkTextIter      *end,
						 GeditTextView        *view);

static void	gedit_text_view_delete_from_cursor 	(GtkTextView     *text_view,
						 GtkDeleteType    type,
						 gint             count);

static void	gedit_text_view_iface_init	(GeditViewIface  *iface);

G_DEFINE_TYPE_WITH_CODE (GeditTextView, 
			 gedit_text_view, 
			 GTK_TYPE_SOURCE_VIEW,
			 G_IMPLEMENT_INTERFACE (GEDIT_TYPE_VIEW,
			 			gedit_text_view_iface_init))

/* Signals */
enum
{
	START_INTERACTIVE_SEARCH,
	START_INTERACTIVE_GOTO_LINE,
	RESET_SEARCHED_TEXT,
	LAST_SIGNAL
};

static guint view_signals [LAST_SIGNAL] = { 0 };

typedef enum
{
	GEDIT_SEARCH_ENTRY_NORMAL,
	GEDIT_SEARCH_ENTRY_NOT_FOUND
} GeditSearchEntryBgColor;

static void
document_read_only_notify_handler (GeditDocument *document, 
			           GParamSpec    *pspec,
				   GeditTextView     *view)
{
	gedit_debug (DEBUG_VIEW);

	gtk_text_view_set_editable (GTK_TEXT_VIEW (view), 
				    !gedit_document_get_readonly (document));
}

static void
gedit_text_view_class_init (GeditTextViewClass *klass)
{
	GObjectClass     *object_class = G_OBJECT_CLASS (klass);
	GtkObjectClass   *gtkobject_class = GTK_OBJECT_CLASS (klass);
	GtkWidgetClass   *widget_class = GTK_WIDGET_CLASS (klass);
	GtkTextViewClass *text_view_class = GTK_TEXT_VIEW_CLASS (klass);
	
	GtkBindingSet    *binding_set;

	gtkobject_class->destroy = gedit_text_view_destroy;
	object_class->finalize = gedit_text_view_finalize;

	widget_class->focus_out_event = gedit_text_view_focus_out;
	widget_class->expose_event = gedit_text_view_expose;
	
	/*
	 * Override the gtk_text_view_drag_motion and drag_drop
	 * functions to get URIs
	 *
	 * If the mime type is text/uri-list, then we will accept
	 * the potential drop, or request the data (depending on the
	 * function).
	 *
	 * If the drag context has any other mime type, then pass the
	 * information onto the GtkTextView's standard handlers.
	 * (widget_class->function_name).
	 *
	 * See bug #89881 for details
	 */
	widget_class->drag_motion = gedit_text_view_drag_motion;
	widget_class->drag_data_received = gedit_text_view_drag_data_received;
	widget_class->drag_drop = gedit_text_view_drag_drop;
	widget_class->button_press_event = gedit_text_view_button_press_event;
	klass->start_interactive_search = start_interactive_search;
	klass->start_interactive_goto_line = start_interactive_goto_line;
	klass->reset_searched_text = reset_searched_text;	

	text_view_class->delete_from_cursor = gedit_text_view_delete_from_cursor;
	
	view_signals[START_INTERACTIVE_SEARCH] =
    		g_signal_new ("start_interactive_search",
		  	      G_TYPE_FROM_CLASS (object_class),
		  	      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  	      G_STRUCT_OFFSET (GeditTextViewClass, start_interactive_search),
			      NULL, NULL,
			      gedit_marshal_BOOLEAN__NONE,
			      G_TYPE_BOOLEAN, 0);	

	view_signals[START_INTERACTIVE_GOTO_LINE] =
    		g_signal_new ("start_interactive_goto_line",
		  	      G_TYPE_FROM_CLASS (object_class),
		  	      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  	      G_STRUCT_OFFSET (GeditTextViewClass, start_interactive_goto_line),
			      NULL, NULL,
			      gedit_marshal_BOOLEAN__NONE,
			      G_TYPE_BOOLEAN, 0);

	view_signals[RESET_SEARCHED_TEXT] =
    		g_signal_new ("reset_searched_text",
		  	      G_TYPE_FROM_CLASS (object_class),
		  	      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  	      G_STRUCT_OFFSET (GeditTextViewClass, reset_searched_text),
			      NULL, NULL,
			      gedit_marshal_BOOLEAN__NONE,
			      G_TYPE_BOOLEAN, 0);
			      
	g_type_class_add_private (klass, sizeof (GeditTextViewPrivate));
	
	binding_set = gtk_binding_set_by_class (klass);
	
	gtk_binding_entry_add_signal (binding_set,
				      GDK_k,
				      GDK_CONTROL_MASK,
				      "start_interactive_search", 0);
		
	gtk_binding_entry_add_signal (binding_set,
				      GDK_i,
				      GDK_CONTROL_MASK,
				      "start_interactive_goto_line", 0);
	
	gtk_binding_entry_add_signal (binding_set,
				      GDK_k,
				      GDK_CONTROL_MASK | GDK_SHIFT_MASK,
				      "reset_searched_text", 0);

	gtk_binding_entry_add_signal (binding_set, 
				      GDK_d, 
				      GDK_CONTROL_MASK,
				      "delete_from_cursor", 2,
				      G_TYPE_ENUM, GTK_DELETE_PARAGRAPHS,
				      G_TYPE_INT, 1);
}

static const gchar *
gedit_view_get_name_impl (GeditView *view)
{
	return GEDIT_TEXT_VIEW_NAME;
}

static GeditDocument *
gedit_view_get_document_impl (GeditView *view)
{
	return GEDIT_DOCUMENT (gtk_text_view_get_buffer (GTK_TEXT_VIEW (view)));
}

static void
gedit_view_set_editable_impl (GeditView *view,
			      gboolean setting)
{
	gtk_text_view_set_editable (GTK_TEXT_VIEW (view), setting);
}

static gboolean
gedit_view_get_editable_impl (GeditView *view)
{
	return gtk_text_view_get_editable (GTK_TEXT_VIEW (view));
}

static gboolean
gedit_view_get_overwrite_impl (GeditView *view)
{
	return gtk_text_view_get_overwrite (GTK_TEXT_VIEW (view));
}

static void
gedit_view_cut_clipboard_impl (GeditView *view)
{
	GtkTextBuffer *buffer;
	GtkClipboard *clipboard;

	gedit_debug (DEBUG_VIEW);

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
	g_return_if_fail (buffer != NULL);

	clipboard = gtk_widget_get_clipboard (GTK_WIDGET (view),
					      GDK_SELECTION_CLIPBOARD);

	/* FIXME: what is default editability of a buffer? */
	gtk_text_buffer_cut_clipboard (buffer,
				       clipboard,
				       !gedit_document_get_readonly (
				       		GEDIT_DOCUMENT (buffer)));
	
	gtk_text_view_scroll_to_mark (GTK_TEXT_VIEW (view),
				      gtk_text_buffer_get_insert (buffer),
				      GEDIT_TEXT_VIEW_SCROLL_MARGIN,
				      FALSE,
				      0.0,
				      0.0);
}

static void
gedit_view_copy_clipboard_impl (GeditView *view)
{
	GtkTextBuffer *buffer;
	GtkClipboard *clipboard;

	gedit_debug (DEBUG_VIEW);

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
	g_return_if_fail (buffer != NULL);

	clipboard = gtk_widget_get_clipboard (GTK_WIDGET (view),
					      GDK_SELECTION_CLIPBOARD);

	gtk_text_buffer_copy_clipboard (buffer, clipboard);

	/* on copy do not scroll, we are already on screen */
}

static void
gedit_view_paste_clipboard_impl (GeditView *view)
{
	GtkTextBuffer *buffer;
	GtkClipboard *clipboard;

	gedit_debug (DEBUG_VIEW);

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
	g_return_if_fail (buffer != NULL);

	clipboard = gtk_widget_get_clipboard (GTK_WIDGET (view),
					      GDK_SELECTION_CLIPBOARD);

	/* FIXME: what is default editability of a buffer? */
	gtk_text_buffer_paste_clipboard (buffer,
					 clipboard,
					 NULL,
					 !gedit_document_get_readonly (
						GEDIT_DOCUMENT (buffer)));

	gtk_text_view_scroll_to_mark (GTK_TEXT_VIEW (view),
				      gtk_text_buffer_get_insert (buffer),
				      GEDIT_TEXT_VIEW_SCROLL_MARGIN,
				      FALSE,
				      0.0,
				      0.0);
}

static void
gedit_view_delete_selection_impl (GeditView *view)
{
	GtkTextBuffer *buffer = NULL;

	gedit_debug (DEBUG_VIEW);

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
	g_return_if_fail (buffer != NULL);

	/* FIXME: what is default editability of a buffer? */
	gtk_text_buffer_delete_selection (buffer,
					  TRUE,
					  !gedit_document_get_readonly (
						GEDIT_DOCUMENT (buffer)));
						
	gtk_text_view_scroll_to_mark (GTK_TEXT_VIEW (view),
				      gtk_text_buffer_get_insert (buffer),
				      GEDIT_TEXT_VIEW_SCROLL_MARGIN,
				      FALSE,
				      0.0,
				      0.0);
}

static void
gedit_view_select_all_impl (GeditView *view)
{
	GtkTextBuffer *buffer = NULL;
	GtkTextIter start, end;

	gedit_debug (DEBUG_VIEW);

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
	g_return_if_fail (buffer != NULL);

	gtk_text_buffer_get_bounds (buffer, &start, &end);
	gtk_text_buffer_select_range (buffer, &start, &end);
}

static void
gedit_view_scroll_to_cursor_impl (GeditView *view)
{
	GtkTextBuffer* buffer = NULL;

	gedit_debug (DEBUG_VIEW);
	
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
	g_return_if_fail (buffer != NULL);

	gtk_text_view_scroll_to_mark (GTK_TEXT_VIEW (view),
				      gtk_text_buffer_get_insert (buffer),
				      0.25,
				      FALSE,
				      0.0,
				      0.0);
}

static void
gedit_view_set_font_impl (GeditView   *view, 
			  gboolean     def, 
			  const gchar *font_name)
{
	PangoFontDescription *font_desc = NULL;

	gedit_debug (DEBUG_VIEW);

	if (def)
	{
		gchar *font;

		font = gedit_prefs_manager_get_system_font ();
		font_desc = pango_font_description_from_string (font);
		g_free (font);
	}
	else
	{
		g_return_if_fail (font_name != NULL);

		font_desc = pango_font_description_from_string (font_name);
	}

	g_return_if_fail (font_desc != NULL);

	gtk_widget_modify_font (GTK_WIDGET (view), font_desc);

	pango_font_description_free (font_desc);
}

static void
gedit_text_view_iface_init (GeditViewIface *iface)
{
	iface->get_name = gedit_view_get_name_impl;
	iface->get_document = gedit_view_get_document_impl;
	iface->set_editable = gedit_view_set_editable_impl;
	iface->get_editable = gedit_view_get_editable_impl;
	iface->get_overwrite = gedit_view_get_overwrite_impl;
	iface->cut_clipboard = gedit_view_cut_clipboard_impl;
	iface->copy_clipboard = gedit_view_copy_clipboard_impl;
	iface->paste_clipboard = gedit_view_paste_clipboard_impl;
	iface->delete_selection = gedit_view_delete_selection_impl;
	iface->select_all = gedit_view_select_all_impl;
	iface->scroll_to_cursor = gedit_view_scroll_to_cursor_impl;
	iface->set_font = gedit_view_set_font_impl;
}

static void
current_buffer_removed (GeditTextView *view)
{
	if (view->priv->current_buffer)
	{
		g_signal_handlers_disconnect_by_func (view->priv->current_buffer,
						      document_read_only_notify_handler,
						      view);
		g_signal_handlers_disconnect_by_func (view->priv->current_buffer,
						      search_highlight_updated_cb,
						      view);
				     
		g_object_unref (view->priv->current_buffer);
		view->priv->current_buffer = NULL;
	}
}

static void
on_notify_buffer_cb (GeditTextView  *view,
		     GParamSpec *arg1,
		     gpointer    userdata)
{
	GtkTextBuffer *buffer;
	
	current_buffer_removed (view);
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
	
	if (buffer == NULL || !GEDIT_IS_DOCUMENT (buffer))
		return;

	view->priv->current_buffer = g_object_ref (buffer);
	g_signal_connect (buffer,
			  "notify::read-only",
			  G_CALLBACK (document_read_only_notify_handler),
			  view);

	gtk_text_view_set_editable (GTK_TEXT_VIEW (view), 
				    !gedit_document_get_readonly (GEDIT_DOCUMENT (buffer)));

	g_signal_connect (buffer,
			  "search_highlight_updated",
			  G_CALLBACK (search_highlight_updated_cb),
			  view);
}

static void 
gedit_text_view_init (GeditTextView *view)
{
	GtkTargetList *tl;
	
	gedit_debug (DEBUG_VIEW);
	
	view->priv = GEDIT_TEXT_VIEW_GET_PRIVATE (view);

	/*
	 *  Set tab, fonts, wrap mode, colors, etc. according
	 *  to preferences 
	 */
	if (!gedit_prefs_manager_get_use_default_font ())
	{
		gchar *editor_font;

		editor_font = gedit_prefs_manager_get_editor_font ();

		gedit_view_set_font (GEDIT_VIEW (view), FALSE, editor_font);

		g_free (editor_font);
	}
	else
	{
		gedit_view_set_font (GEDIT_VIEW (view), TRUE, NULL);
	}

	g_object_set (G_OBJECT (view), 
		      "wrap_mode", gedit_prefs_manager_get_wrap_mode (),
		      "show_line_numbers", gedit_prefs_manager_get_display_line_numbers (),
		      "auto_indent", gedit_prefs_manager_get_auto_indent (),
		      "tab_width", gedit_prefs_manager_get_tabs_size (),
		      "insert_spaces_instead_of_tabs", gedit_prefs_manager_get_insert_spaces (),
		      "show_right_margin", gedit_prefs_manager_get_display_right_margin (), 
		      "right_margin_position", gedit_prefs_manager_get_right_margin_position (),
		      "highlight_current_line", gedit_prefs_manager_get_highlight_current_line (),
		      "smart_home_end", gedit_prefs_manager_get_smart_home_end (),
		      "indent_on_tab", TRUE,
		      NULL);

	view->priv->typeselect_flush_timeout = 0;
	view->priv->wrap_around = TRUE;

	/* Drag and drop support */	
	tl = gtk_drag_dest_get_target_list (GTK_WIDGET (view));

	if (tl != NULL)
		gtk_target_list_add_uri_targets (tl, TARGET_URI_LIST);
		
	/* Act on buffer change */
	g_signal_connect (view, 
			  "notify::buffer", 
			  G_CALLBACK (on_notify_buffer_cb),
			  NULL);
}

static void
gedit_text_view_destroy (GtkObject *object)
{
	GeditTextView *view;

	view = GEDIT_TEXT_VIEW (object);

	if (view->priv->search_window != NULL)
	{
		gtk_widget_destroy (view->priv->search_window);
		view->priv->search_window = NULL;
		view->priv->search_entry = NULL;
		
		if (view->priv->typeselect_flush_timeout != 0)
		{
			g_source_remove (view->priv->typeselect_flush_timeout);
			view->priv->typeselect_flush_timeout = 0;
		}
	}
	
	/* Disconnect notify buffer because the destroy of the textview will
	   set the buffer to NULL, and we call get_buffer in the notify which
	   would reinstate a GtkTextBuffer which we don't want */
	current_buffer_removed (view);
	g_signal_handlers_disconnect_by_func (view, on_notify_buffer_cb, NULL);
	
	(* GTK_OBJECT_CLASS (gedit_text_view_parent_class)->destroy) (object);
}

static void
gedit_text_view_finalize (GObject *object)
{
	GeditTextView *view;

	view = GEDIT_TEXT_VIEW (object);

	current_buffer_removed (view);

	g_free (view->priv->old_search_text);

	(* G_OBJECT_CLASS (gedit_text_view_parent_class)->finalize) (object);
}

static gint
gedit_text_view_focus_out (GtkWidget *widget, GdkEventFocus *event)
{
	GeditTextView *view = GEDIT_TEXT_VIEW (widget);
	
	gtk_widget_queue_draw (widget);
	
	/* hide interactive search dialog */
	if (view->priv->search_window != NULL)
		hide_search_window (view, FALSE);
	
	(* GTK_WIDGET_CLASS (gedit_text_view_parent_class)->focus_out_event) (widget, event);
	
	return FALSE;
}

/**
 * gedit_text_view_new:
 * @doc: a #GeditDocument
 * 
 * Creates a new #GeditTextView object displaying the @doc document. 
 * @doc cannot be NULL.
 *
 * Return value: a new #GeditTextView
 **/
GtkWidget *
gedit_text_view_new (GeditTextBuffer *doc)
{
	GtkWidget *view;

	gedit_debug_message (DEBUG_VIEW, "START");
	
	g_return_val_if_fail (GEDIT_IS_TEXT_BUFFER (doc), NULL);

	view = GTK_WIDGET (g_object_new (GEDIT_TYPE_TEXT_VIEW, "buffer", doc, NULL));

	gedit_debug_message (DEBUG_VIEW, "END: %d", G_OBJECT (view)->ref_count);

	gtk_widget_show_all (view);

	return view;
}

static void
add_search_completion_entry (const gchar *str)
{
	gchar        *text;
	gboolean      valid;
	GtkTreeModel *model;
	GtkTreeIter   iter;

	if (str == NULL)
		return;

	text = gedit_utils_unescape_search_text (str);

	if (g_utf8_strlen (text, -1) < MIN_SEARCH_COMPLETION_KEY_LEN)
	{
		g_free (text);
		return;
	}

	g_return_if_fail (GTK_IS_TREE_MODEL (search_completion_model));

	model = GTK_TREE_MODEL (search_completion_model);

	/* Get the first iter in the list */
	valid = gtk_tree_model_get_iter_first (model, &iter);

	while (valid)
	{
		/* Walk through the list, reading each row */
     		gchar *str_data;

		gtk_tree_model_get (model, 
				    &iter, 
                          	    0, 
                          	    &str_data,
                          	    -1);

		if (strcmp (text, str_data) == 0)
		{
			g_free (text);
			g_free (str_data);
			gtk_list_store_move_after (GTK_LIST_STORE (model),
						   &iter,
						   NULL);

			return;
		}

		g_free (str_data);

		valid = gtk_tree_model_iter_next (model, &iter);
    	}

	gtk_list_store_prepend (GTK_LIST_STORE (model), &iter);
	gtk_list_store_set (GTK_LIST_STORE (model),
			    &iter,
			    0,
			    text,
			    -1);

	g_free (text);
}

static void
set_entry_background (GtkWidget               *entry,
		      GeditSearchEntryBgColor  col)
{
	if (col == GEDIT_SEARCH_ENTRY_NOT_FOUND)
	{
		GdkColor red;
		GdkColor white;

		/* FIXME: a11y and theme */

		gdk_color_parse ("#FF6666", &red);
		gdk_color_parse ("white", &white);

		gtk_widget_modify_base (entry,
				        GTK_STATE_NORMAL,
				        &red);
		gtk_widget_modify_text (entry,
				        GTK_STATE_NORMAL,
				        &white);
	}
	else /* reset */
	{
		gtk_widget_modify_base (entry,
				        GTK_STATE_NORMAL,
				        NULL);
		gtk_widget_modify_text (entry,
				        GTK_STATE_NORMAL,
				        NULL);
	}
}

static gboolean
run_search (GeditTextView        *view,
            const gchar      *entry_text,
	    gboolean          search_backward,
	    gboolean          wrap_around,
            gboolean          typing)
{
	GtkTextIter    start_iter;
	GtkTextIter    match_start;
	GtkTextIter    match_end;	
	gboolean       found = FALSE;
	GeditTextBuffer *doc;

	g_return_val_if_fail (view->priv->search_mode == SEARCH, FALSE);

	doc = GEDIT_TEXT_BUFFER (gtk_text_view_get_buffer (GTK_TEXT_VIEW (view)));
	
	start_iter = view->priv->start_search_iter;
	
	if (*entry_text != '\0')
	{	
		if (!search_backward)
		{
			if (!typing)
			{
				/* forward and _NOT_ typing */
				gtk_text_buffer_get_selection_bounds (GTK_TEXT_BUFFER (doc),
							      &start_iter,
							      &match_end);
		
				gtk_text_iter_order (&match_end, &start_iter);
			}
		
			/* run search */
			found = gedit_text_buffer_search_forward (doc,
								  &start_iter,
								  NULL,
								  &match_start,
								  &match_end);
		}
		else if (!typing)
		{
			/* backward and not typing */
			gtk_text_buffer_get_selection_bounds (GTK_TEXT_BUFFER (doc),
							      &start_iter,
							      &match_end);
			
			/* run search */
			found = gedit_text_buffer_search_backward (doc,
								   NULL,
								   &start_iter,
								   &match_start,
								   &match_end);
		} 
		else
		{
			/* backward (while typing) */
			g_return_val_if_reached (FALSE);

		}
		
		if (!found && wrap_around)
		{
			if (!search_backward)
				found = gedit_text_buffer_search_forward (doc,
									  NULL,
									  NULL, /* FIXME: set the end_inter */
									  &match_start,
									  &match_end);
			else
				found = gedit_text_buffer_search_backward (doc,
									   NULL, /* FIXME: set the start_inter */
									   NULL, 
									   &match_start,
									   &match_end);
		}
	}
	else
	{
		gtk_text_buffer_get_selection_bounds (GTK_TEXT_BUFFER (doc),
						      &start_iter, 
						      NULL);	
	}
	
	if (found)
	{
		gtk_text_buffer_place_cursor (GTK_TEXT_BUFFER (doc),
					&match_start);

		gtk_text_buffer_move_mark_by_name (GTK_TEXT_BUFFER (doc),
					"selection_bound", &match_end);
	}
	else
	{
		if (typing)
		{
			gtk_text_buffer_place_cursor (GTK_TEXT_BUFFER (doc),
						      &view->priv->start_search_iter);
		}
	}

	if (found || (*entry_text == '\0'))
	{				   
		gedit_view_scroll_to_cursor (GEDIT_VIEW (view));

		set_entry_background (view->priv->search_entry,
				      GEDIT_SEARCH_ENTRY_NORMAL);
	}
	else
	{
		set_entry_background (view->priv->search_entry,
				      GEDIT_SEARCH_ENTRY_NOT_FOUND);
	}

	return found;
}

/* Cut and paste from gtkwindow.c */
static void
send_focus_change (GtkWidget *widget,
		   gboolean   in)
{
	GdkEvent *fevent = gdk_event_new (GDK_FOCUS_CHANGE);

	g_object_ref (widget);

	if (in)
		GTK_WIDGET_SET_FLAGS (widget, GTK_HAS_FOCUS);
	else
		GTK_WIDGET_UNSET_FLAGS (widget, GTK_HAS_FOCUS);

	fevent->focus_change.type = GDK_FOCUS_CHANGE;
	fevent->focus_change.window = g_object_ref (widget->window);
	fevent->focus_change.in = in;

	gtk_widget_event (widget, fevent);

	g_object_notify (G_OBJECT (widget), "has-focus");

	g_object_unref (widget);
	gdk_event_free (fevent);
}

static void
hide_search_window (GeditTextView *view, gboolean cancel)
{
	if (view->priv->disable_popdown)
		return;

	if (view->priv->search_entry_changed_id != 0)
	{
		g_signal_handler_disconnect (view->priv->search_entry,
					     view->priv->search_entry_changed_id);
		view->priv->search_entry_changed_id = 0;
	}

	if (view->priv->typeselect_flush_timeout != 0)
	{
		g_source_remove (view->priv->typeselect_flush_timeout);
		view->priv->typeselect_flush_timeout = 0;
	}

	/* send focus-in event */
	send_focus_change (GTK_WIDGET (view->priv->search_entry), FALSE);
	gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (view), TRUE);
	gtk_widget_hide (view->priv->search_window);
	
	if (cancel)
	{
		GtkTextBuffer *buffer;
		
		buffer = GTK_TEXT_BUFFER (gtk_text_view_get_buffer (GTK_TEXT_VIEW (view)));
		gtk_text_buffer_place_cursor (buffer, &view->priv->start_search_iter);
		
		gedit_view_scroll_to_cursor (GEDIT_VIEW (view));
	}

	/* make sure a focus event is sent for the edit area */
	send_focus_change (GTK_WIDGET (view), TRUE);
}

static gboolean
search_entry_flush_timeout (GeditTextView *view)
{
	GDK_THREADS_ENTER ();

  	view->priv->typeselect_flush_timeout = 0;
	hide_search_window (view, FALSE);

	GDK_THREADS_LEAVE ();

	return FALSE;
}

static void
update_search_window_position (GeditTextView *view)
{
	gint x, y;
	gint view_x, view_y;
	GdkWindow *view_window = GTK_WIDGET (view)->window;

	gtk_widget_realize (view->priv->search_window);

	gdk_window_get_origin (view_window, &view_x, &view_y);

	x = MAX (12, view_x + 12);
	y = MAX (12, view_y - 12);
	
	gtk_window_move (GTK_WINDOW (view->priv->search_window), x, y);
}

static gboolean
search_window_delete_event (GtkWidget   *widget,
			    GdkEventAny *event,
			    GeditTextView   *view)
{
	hide_search_window (view, FALSE);

	return TRUE;
}

static gboolean
search_window_button_press_event (GtkWidget      *widget,
				  GdkEventButton *event,
				  GeditTextView      *view)
{
	hide_search_window (view, FALSE);
	
	gtk_propagate_event (GTK_WIDGET (view), (GdkEvent *)event);

	return FALSE;
}

static void
search_again (GeditTextView *view,
	      gboolean   search_backward)
{
	const gchar *entry_text;

	g_return_if_fail (view->priv->search_mode == SEARCH);
		
	/* SEARCH mode */	
	/* renew the flush timeout */
	if (view->priv->typeselect_flush_timeout != 0)
	{
		g_source_remove (view->priv->typeselect_flush_timeout);
		view->priv->typeselect_flush_timeout =
			g_timeout_add (GEDIT_TEXT_VIEW_SEARCH_DIALOG_TIMEOUT,
		       		       (GSourceFunc)search_entry_flush_timeout,
		       		       view);
	}
	
	entry_text = gtk_entry_get_text (GTK_ENTRY (view->priv->search_entry));
	
	add_search_completion_entry (entry_text);
		
	run_search (view,
		    entry_text,
		    search_backward,
		    view->priv->wrap_around,
		    FALSE);
}

static gboolean
search_window_scroll_event (GtkWidget      *widget,
			    GdkEventScroll *event,
			    GeditTextView      *view)
{
	gboolean retval = FALSE;

	if (view->priv->search_mode == GOTO_LINE)
		return retval;
		
	/* SEARCH mode */	
	if (event->direction == GDK_SCROLL_UP)
	{
		search_again (view, TRUE);
		retval = TRUE;
	}
	else if (event->direction == GDK_SCROLL_DOWN)
	{
      		search_again (view, FALSE);
      		retval = TRUE;
	}

	return retval;
}

static gboolean
search_window_key_press_event (GtkWidget   *widget,
			       GdkEventKey *event,
			       GeditTextView   *view)
{
	gboolean retval = FALSE;
	guint modifiers;

	modifiers = gtk_accelerator_get_default_mod_mask ();

	/* Close window */
	if (event->keyval == GDK_Tab)
	{
		hide_search_window (view, FALSE);
		retval = TRUE;
	}

	/* Close window and cancel the search */
	if (event->keyval == GDK_Escape)
	{
		if (view->priv->search_mode == SEARCH)
		{
			GeditTextBuffer *doc;

			/* restore document search so that Find Next does the right thing */
			doc = GEDIT_TEXT_BUFFER (gtk_text_view_get_buffer (GTK_TEXT_VIEW (view)));
			gedit_text_buffer_set_search_text (doc,
							   view->priv->old_search_text,
							   view->priv->old_search_flags);
						
		}
		
		hide_search_window (view, TRUE);
		retval = TRUE;
	}
	
	if (view->priv->search_mode == GOTO_LINE)
		return retval;
		
	/* SEARCH mode */

	/* select previous matching iter */
	if (event->keyval == GDK_Up || event->keyval == GDK_KP_Up)
	{
		search_again (view, TRUE);
		retval = TRUE;
	}

	if (((event->state & modifiers) == (GDK_CONTROL_MASK | GDK_SHIFT_MASK)) && 
	    (event->keyval == GDK_g || event->keyval == GDK_G))
	{
		search_again (view, TRUE);
		retval = TRUE;
	}

	/* select next matching iter */
	if (event->keyval == GDK_Down || event->keyval == GDK_KP_Down)
	{
		search_again (view, FALSE);
		retval = TRUE;
	}

	if (((event->state & modifiers) == GDK_CONTROL_MASK) && 
	    (event->keyval == GDK_g || event->keyval == GDK_G))
	{
		search_again (view, FALSE);
		retval = TRUE;
	}

	return retval;
}

static void
search_entry_activate (GtkEntry  *entry,
		       GeditTextView *view)
{
	hide_search_window (view, FALSE);
}

static void
wrap_around_menu_item_toggled (GtkCheckMenuItem *checkmenuitem,
			       GeditTextView        *view)
{	
	view->priv->wrap_around = gtk_check_menu_item_get_active (checkmenuitem);
}

static void
match_entire_word_menu_item_toggled (GtkCheckMenuItem *checkmenuitem,
				     GeditTextView        *view)
{
	GEDIT_SEARCH_SET_ENTIRE_WORD (view->priv->search_flags,
				      gtk_check_menu_item_get_active (checkmenuitem));
}

static void
match_case_menu_item_toggled (GtkCheckMenuItem *checkmenuitem,
			      GeditTextView        *view)
{
	GEDIT_SEARCH_SET_CASE_SENSITIVE (view->priv->search_flags,
					 gtk_check_menu_item_get_active (checkmenuitem));
}

static gboolean
real_search_enable_popdown (gpointer data)
{
	GeditTextView *view = (GeditTextView *)data;

	GDK_THREADS_ENTER ();

	view->priv->disable_popdown = FALSE;

	GDK_THREADS_LEAVE ();

	return FALSE;
}

static void
search_enable_popdown (GtkWidget *widget,
		       GeditTextView *view)
{
	g_timeout_add (200, real_search_enable_popdown, view);
	
	/* renew the flush timeout */
	if (view->priv->typeselect_flush_timeout != 0)
		g_source_remove (view->priv->typeselect_flush_timeout);

	view->priv->typeselect_flush_timeout =
		g_timeout_add (GEDIT_TEXT_VIEW_SEARCH_DIALOG_TIMEOUT,
	       		       (GSourceFunc)search_entry_flush_timeout,
	       		       view);
}

static void
search_entry_populate_popup (GtkEntry  *entry,
			     GtkMenu   *menu,
			     GeditTextView *view)
{
	GtkWidget *menu_item;

	view->priv->disable_popdown = TRUE;
	g_signal_connect (menu, "hide",
		    	  G_CALLBACK (search_enable_popdown), view);

	if (view->priv->search_mode == GOTO_LINE)
		return;

	/* separator */
	menu_item = gtk_menu_item_new ();
	gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), menu_item);
	gtk_widget_show (menu_item);

	/* create "Wrap Around" menu item. */
	menu_item = gtk_check_menu_item_new_with_mnemonic (_("_Wrap Around"));
	g_signal_connect (G_OBJECT (menu_item), "toggled",
			  G_CALLBACK (wrap_around_menu_item_toggled), 
			  view);
	gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), menu_item);
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menu_item),
					view->priv->wrap_around);
	gtk_widget_show (menu_item);

	/* create "Match Entire Word Only" menu item. */
	menu_item = gtk_check_menu_item_new_with_mnemonic (_("Match _Entire Word Only"));
	g_signal_connect (G_OBJECT (menu_item), "toggled",
			  G_CALLBACK (match_entire_word_menu_item_toggled), 
			  view);
	gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), menu_item);
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menu_item),
					GEDIT_SEARCH_IS_ENTIRE_WORD (view->priv->search_flags));
	gtk_widget_show (menu_item);

	/* create "Match Case" menu item. */
	menu_item = gtk_check_menu_item_new_with_mnemonic (_("_Match Case"));
	g_signal_connect (G_OBJECT (menu_item), "toggled",
			  G_CALLBACK (match_case_menu_item_toggled), 
			  view);
	gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), menu_item);
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menu_item),
					GEDIT_SEARCH_IS_CASE_SENSITIVE (view->priv->search_flags));
	gtk_widget_show (menu_item);
}

static void
search_entry_insert_text (GtkEditable *editable, 
			  const gchar *text, 
			  gint         length, 
			  gint        *position,
			  GeditTextView   *view)
{
	if (view->priv->search_mode == GOTO_LINE)
	{
		gunichar c;
		const gchar *p;
	 	const gchar *end;
	 	const gchar *next;

		p = text;
		end = text + length;

		if (p == end)
			return;

		c = g_utf8_get_char (p);
		
		if (((c == '-' || c == '+') && *position == 0) ||
		    (c == ':' && *position != 0))
		{
			gchar *s = NULL;
		
			if (c == ':')
			{
				s = gtk_editable_get_chars (editable, 0, -1);
				s = g_utf8_strchr (s, -1, ':');
			}
			
			if (s == NULL || s == p)
			{
				next = g_utf8_next_char (p);
				p = next;
			}
			
			g_free (s);
		}

		while (p != end)
		{
			next = g_utf8_next_char (p);

			c = g_utf8_get_char (p);

			if (!g_unichar_isdigit (c)) {
				g_signal_stop_emission_by_name (editable, "insert_text");
				gtk_widget_error_bell (view->priv->search_entry);
				break;
			}

			p = next;
		}
	}
	else
	{
		/* SEARCH mode */
		static gboolean  insert_text = FALSE;
		gchar           *escaped_text;
		gint             new_len;

		gedit_debug_message (DEBUG_SEARCH, "Text: %s", text);

		/* To avoid recursive behavior */
		if (insert_text)
			return;

		escaped_text = gedit_utils_escape_search_text (text);

		gedit_debug_message (DEBUG_SEARCH, "Escaped Text: %s", escaped_text);

		new_len = strlen (escaped_text);

		if (new_len == length)
		{
			g_free (escaped_text);
			return;
		}

		insert_text = TRUE;

		g_signal_stop_emission_by_name (editable, "insert_text");
		
		gtk_editable_insert_text (editable, escaped_text, new_len, position);

		insert_text = FALSE;

		g_free (escaped_text);
	}
}

static void
customize_for_search_mode (GeditTextView *view)
{
	if (view->priv->search_mode == SEARCH)
	{
		gtk_entry_set_icon_from_stock (GTK_ENTRY (view->priv->search_entry),
					       GTK_ENTRY_ICON_PRIMARY,
					       GTK_STOCK_FIND);
		
		gtk_widget_set_tooltip_text (view->priv->search_entry,
					     _("String you want to search for"));
	}
	else
	{
		gtk_entry_set_icon_from_stock (GTK_ENTRY (view->priv->search_entry),
					       GTK_ENTRY_ICON_PRIMARY,
					       GTK_STOCK_JUMP_TO);
		
		gtk_widget_set_tooltip_text (view->priv->search_entry,
					     _("Line you want to move the cursor to"));
	}
}

static gboolean
completion_func (GtkEntryCompletion *completion,
                 const char         *key,
		 GtkTreeIter        *iter,
		 gpointer            data)
{
	gchar *item = NULL;
	gboolean ret = FALSE;
	GtkTreeModel *model;
	GeditTextViewPrivate *priv = (GeditTextViewPrivate *)data;
	const gchar *real_key;
		
	if (priv->search_mode == GOTO_LINE)
		return FALSE;
		
	real_key = gtk_entry_get_text (GTK_ENTRY (gtk_entry_completion_get_entry (completion)));
	
	if (g_utf8_strlen (real_key, -1) <= MIN_SEARCH_COMPLETION_KEY_LEN)
		return FALSE;
		
	model = gtk_entry_completion_get_model (completion);
	g_return_val_if_fail (gtk_tree_model_get_column_type (model, 0) == G_TYPE_STRING, 
			      FALSE);
			      
	gtk_tree_model_get (model, 
			    iter,
			    0, 
			    &item,
			    -1);
	
	if (item == NULL)
		return FALSE;
		
	if (GEDIT_SEARCH_IS_CASE_SENSITIVE (priv->search_flags))
	{		
		if (!strncmp (real_key, item, strlen (real_key)))
			ret = TRUE;
	}
	else
	{
		gchar *normalized_string;
		gchar *case_normalized_string;
		
		normalized_string = g_utf8_normalize (item, -1, G_NORMALIZE_ALL);
		case_normalized_string = g_utf8_casefold (normalized_string, -1);
      
      		if (!strncmp (key, case_normalized_string, strlen (key)))
			ret = TRUE;
 		
		g_free (normalized_string);
		g_free (case_normalized_string);
		
	}
	
	g_free (item);
	
	return ret;	
}

static void
ensure_search_window (GeditTextView *view)
{  
	GtkWidget          *frame;
	GtkWidget          *vbox;
	GtkWidget          *toplevel;
	GtkEntryCompletion *completion;
	
	toplevel = gtk_widget_get_toplevel (GTK_WIDGET (view));

	if (view->priv->search_window != NULL)
	{
		if (GTK_WINDOW (toplevel)->group)
			gtk_window_group_add_window (GTK_WINDOW (toplevel)->group,
						     GTK_WINDOW (view->priv->search_window));
		else if (GTK_WINDOW (view->priv->search_window)->group)
	 		gtk_window_group_remove_window (GTK_WINDOW (view->priv->search_window)->group,
					 		GTK_WINDOW (view->priv->search_window));
					 		
		customize_for_search_mode (view);
		
		return;
	}
   
	view->priv->search_window = gtk_window_new (GTK_WINDOW_POPUP);

	if (GTK_WINDOW (toplevel)->group)
		gtk_window_group_add_window (GTK_WINDOW (toplevel)->group,
					     GTK_WINDOW (view->priv->search_window));
					     
	gtk_window_set_modal (GTK_WINDOW (view->priv->search_window), TRUE);
	
	g_signal_connect (view->priv->search_window, "delete_event",
			  G_CALLBACK (search_window_delete_event),
			  view);
	g_signal_connect (view->priv->search_window, "key_press_event",
			  G_CALLBACK (search_window_key_press_event),
			  view);
	g_signal_connect (view->priv->search_window, "button_press_event",
			  G_CALLBACK (search_window_button_press_event),
			  view);
	g_signal_connect (view->priv->search_window, "scroll_event",
			  G_CALLBACK (search_window_scroll_event),
			  view);

	frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
	gtk_widget_show (frame);
	gtk_container_add (GTK_CONTAINER (view->priv->search_window), frame);

	vbox = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox);
	gtk_container_add (GTK_CONTAINER (frame), vbox);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 3);

	/* add entry */
	view->priv->search_entry = gtk_entry_new ();
	gtk_widget_show (view->priv->search_entry);
	
	g_signal_connect (view->priv->search_entry, "populate_popup",
			  G_CALLBACK (search_entry_populate_popup),
			  view);
	g_signal_connect (view->priv->search_entry, "activate", 
			  G_CALLBACK (search_entry_activate),
			  view);
	/* CHECK: do we really need to connect to preedit too? -- Paolo
	g_signal_connect (GTK_ENTRY (view->priv->search_entry)->im_context, "preedit-changed",
			  G_CALLBACK (gtk_view_search_preedit_changed),
			  view);
	*/		
	g_signal_connect (view->priv->search_entry, 
			  "insert_text",
			  G_CALLBACK (search_entry_insert_text), 
			  view);	  
			  
	gtk_container_add (GTK_CONTAINER (vbox),
			   view->priv->search_entry);

	if (search_completion_model == NULL)
	{
		/* Create a tree model and use it as the completion model */
		search_completion_model = gtk_list_store_new (1, G_TYPE_STRING);
	}
	
	/* Create the completion object for the search entry */
	completion = gtk_entry_completion_new ();
	gtk_entry_completion_set_model (completion, 
					GTK_TREE_MODEL (search_completion_model));
		
	/* Use model column 0 as the text column */
	gtk_entry_completion_set_text_column (completion, 0);

	gtk_entry_completion_set_minimum_key_length (completion,
						     MIN_SEARCH_COMPLETION_KEY_LEN);

	gtk_entry_completion_set_popup_completion (completion, FALSE);
	gtk_entry_completion_set_inline_completion (completion, TRUE);
	
	gtk_entry_completion_set_match_func (completion, 
					     completion_func,
					     view->priv,
					     NULL);

	/* Assign the completion to the entry */
	gtk_entry_set_completion (GTK_ENTRY (view->priv->search_entry), 
				  completion);
	g_object_unref (completion);

	gtk_widget_realize (view->priv->search_entry);

	customize_for_search_mode (view);	
}

static gboolean
get_selected_text (GtkTextBuffer *doc, gchar **selected_text, gint *len)
{
	GtkTextIter start, end;

	g_return_val_if_fail (selected_text != NULL, FALSE);
	g_return_val_if_fail (*selected_text == NULL, FALSE);

	if (!gtk_text_buffer_get_selection_bounds (doc, &start, &end))
	{
		if (len != NULL)
			len = 0;

		return FALSE;
	}

	*selected_text = gtk_text_buffer_get_slice (doc, &start, &end, TRUE);

	if (len != NULL)
		*len = g_utf8_strlen (*selected_text, -1);

	return TRUE;
}

static void
init_search_entry (GeditTextView *view)
{
	GtkTextBuffer *buffer;
				
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
	
	if (view->priv->search_mode == GOTO_LINE)
	{	
		gint   line;
		gchar *line_str;
		
		line = gtk_text_iter_get_line (&view->priv->start_search_iter);
		
		line_str = g_strdup_printf ("%d", line + 1);
		
		gtk_entry_set_text (GTK_ENTRY (view->priv->search_entry), 
				    line_str);
				    
		g_free (line_str);
		
		return;
	}
	else
	{
		/* SEARCH mode */
		gboolean  selection_exists;
		gchar    *find_text = NULL;
		gchar    *old_find_text;
		guint     old_find_flags = 0;
		gint      sel_len = 0;

		g_free (view->priv->old_search_text);
		
		old_find_text = gedit_text_buffer_get_search_text (GEDIT_TEXT_BUFFER (buffer), 
								   &old_find_flags);
		if (old_find_text != NULL)
		{
			view->priv->old_search_text = old_find_text;
			add_search_completion_entry (old_find_text);
		}

		if (old_find_flags != 0)
		{
			view->priv->old_search_flags = old_find_flags;
		}

		selection_exists = get_selected_text (buffer, 
						      &find_text, 
						      &sel_len);

		if (selection_exists  && (find_text != NULL) && (sel_len <= 160))
		{
			gtk_entry_set_text (GTK_ENTRY (view->priv->search_entry), 
					    find_text);	
		}
		else
		{
			gtk_entry_set_text (GTK_ENTRY (view->priv->search_entry), 
					    "");
		}
		
		g_free (find_text);
	}
}

static void
search_init (GtkWidget *entry,
	     GeditTextView *view)
{
	GeditTextBuffer *doc;
	const gchar *entry_text;

	/* renew the flush timeout */
	if (view->priv->typeselect_flush_timeout != 0)
	{
		g_source_remove (view->priv->typeselect_flush_timeout);
		view->priv->typeselect_flush_timeout =
			g_timeout_add (GEDIT_TEXT_VIEW_SEARCH_DIALOG_TIMEOUT,
		       		       (GSourceFunc)search_entry_flush_timeout,
		       		       view);
	}
	
	doc = GEDIT_TEXT_BUFFER (gtk_text_view_get_buffer (GTK_TEXT_VIEW (view)));
			
	entry_text = gtk_entry_get_text (GTK_ENTRY (entry));
	
	if (view->priv->search_mode == SEARCH)
	{
		gchar *search_text;
		guint  search_flags;

		search_text = gedit_text_buffer_get_search_text (doc, &search_flags);

		if ((search_text == NULL) ||
		    (strcmp (search_text, entry_text) != 0) ||
		     search_flags != view->priv->search_flags)
		{
			gedit_text_buffer_set_search_text (doc, 
							   entry_text,
							   view->priv->search_flags);
		}

		g_free (search_text);

		run_search (view,
			    entry_text,
			    FALSE,
			    view->priv->wrap_around,
			    TRUE);
	}
	else
	{
		if (*entry_text != '\0')
		{
			gboolean moved, moved_offset;
			gint line;
			gint offset_line = 0;
			gint line_offset = 0;
			gchar **split_text = NULL;
			const gchar *text;
			
			split_text = g_strsplit (entry_text, ":", -1);
			
			if (g_strv_length (split_text) > 1)
			{
				text = split_text[0];
			}
			else
			{
				text = entry_text;
			}
			
			if (*text == '-')
			{
				gint cur_line = gtk_text_iter_get_line (&view->priv->start_search_iter);
			
				if (*(text + 1) != '\0')
					offset_line = MAX (atoi (text + 1), 0);
				
				line = MAX (cur_line - offset_line, 0);
			}
			else if (*entry_text == '+')
			{
				gint cur_line = gtk_text_iter_get_line (&view->priv->start_search_iter);
			
				if (*(text + 1) != '\0')
					offset_line = MAX (atoi (text + 1), 0);
				
				line = cur_line + offset_line;
			}
			else
			{
				line = MAX (atoi (text) - 1, 0);
			}
			
			if (split_text[1] != NULL)
			{
				line_offset = atoi (split_text[1]);
			}
			
			g_strfreev (split_text);
			
			moved = gedit_document_goto_line (GEDIT_DOCUMENT (doc), line);
			moved_offset = gedit_document_goto_line_offset (GEDIT_DOCUMENT (doc),
									line,
									line_offset);
			
			gedit_view_scroll_to_cursor (GEDIT_VIEW (view));

			if (!moved || !moved_offset)
				set_entry_background (view->priv->search_entry,
						      GEDIT_SEARCH_ENTRY_NOT_FOUND);
			else
				set_entry_background (view->priv->search_entry,
						      GEDIT_SEARCH_ENTRY_NORMAL);
		}
	}
}

static gboolean
start_interactive_search_real (GeditTextView *view)
{	
	GtkTextBuffer *buffer;
	
	if ((view->priv->search_window != NULL) &&
	    GTK_WIDGET_VISIBLE (view->priv->search_window))
		return TRUE;

	if (!GTK_WIDGET_HAS_FOCUS (view))
		return FALSE;

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));

	if (view->priv->search_mode == SEARCH)
		gtk_text_buffer_get_selection_bounds (buffer, &view->priv->start_search_iter, NULL);
	else
		gtk_text_buffer_get_iter_at_mark (buffer,
						  &view->priv->start_search_iter,
						  gtk_text_buffer_get_insert (buffer));

	ensure_search_window (view);

	/* done, show it */
	update_search_window_position (view);
	gtk_widget_show (view->priv->search_window);

	if (view->priv->search_entry_changed_id == 0)
	{
      		view->priv->search_entry_changed_id =
			g_signal_connect (view->priv->search_entry,
					  "changed",
					  G_CALLBACK (search_init),
					  view);
	}

	init_search_entry (view);

	view->priv->typeselect_flush_timeout =  
		g_timeout_add (GEDIT_TEXT_VIEW_SEARCH_DIALOG_TIMEOUT,
		   	       (GSourceFunc) search_entry_flush_timeout,
		   	       view);

	gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (view), FALSE);
	gtk_widget_grab_focus (view->priv->search_entry);
	
	send_focus_change (view->priv->search_entry, TRUE);
	
	return TRUE;
}

static gboolean
reset_searched_text (GeditTextView *view)
{		
	GeditTextBuffer *doc;

	doc = GEDIT_TEXT_BUFFER (gtk_text_view_get_buffer (GTK_TEXT_VIEW (view)));
	
	gedit_text_buffer_set_search_text (doc, "", GEDIT_SEARCH_DONT_SET_FLAGS);
	
	return TRUE;
}

static gboolean
start_interactive_search (GeditTextView *view)
{		
	view->priv->search_mode = SEARCH;
	
	return start_interactive_search_real (view);
}

static gboolean 
start_interactive_goto_line (GeditTextView *view)
{
	view->priv->search_mode = GOTO_LINE;
	
	return start_interactive_search_real (view);
}

static gint
gedit_text_view_expose (GtkWidget      *widget,
                   GdkEventExpose *event)
{
	GtkTextView *text_view;
	GeditTextBuffer *doc;
	
	text_view = GTK_TEXT_VIEW (widget);
	
	doc = GEDIT_TEXT_BUFFER (gtk_text_view_get_buffer (text_view));
	
	if ((event->window == gtk_text_view_get_window (text_view, GTK_TEXT_WINDOW_TEXT)) &&
	    gedit_text_buffer_get_enable_search_highlighting (doc))
	{
		GdkRectangle visible_rect;
		GtkTextIter iter1, iter2;
		
		gtk_text_view_get_visible_rect (text_view, &visible_rect);
		gtk_text_view_get_line_at_y (text_view, &iter1,
					     visible_rect.y, NULL);
		gtk_text_view_get_line_at_y (text_view, &iter2,
					     visible_rect.y
					     + visible_rect.height, NULL);
		gtk_text_iter_forward_line (&iter2);
				     
		_gedit_text_buffer_search_region (doc,
						  &iter1,
						  &iter2);
	}

	return (* GTK_WIDGET_CLASS (gedit_text_view_parent_class)->expose_event)(widget, event);
}

static GdkAtom
drag_get_uri_target (GtkWidget      *widget,
		     GdkDragContext *context)
{
	GdkAtom target;
	GtkTargetList *tl;
	
	tl = gtk_target_list_new (NULL, 0);
	gtk_target_list_add_uri_targets (tl, 0);
	
	target = gtk_drag_dest_find_target (widget, context, tl);
	gtk_target_list_unref (tl);
	
	return target;
}

static gboolean
gedit_text_view_drag_motion (GtkWidget      *widget,
			GdkDragContext *context,
			gint            x,
			gint            y,
			guint           timestamp)
{
	gboolean result;

	/* Chain up to allow textview to scroll and position dnd mark, note 
	 * that this needs to be checked if gtksourceview or gtktextview
	 * changes drag_motion behaviour */
	result = GTK_WIDGET_CLASS (gedit_text_view_parent_class)->drag_motion (widget, context, x, y, timestamp);

	/* If this is a URL, deal with it here */
	if (drag_get_uri_target (widget, context) != GDK_NONE) 
	{
		gdk_drag_status (context, context->suggested_action, timestamp);
		result = TRUE;
	}

	return result;
}

static void
gedit_text_view_drag_data_received (GtkWidget        *widget,
		       	       GdkDragContext   *context,
			       gint              x,
			       gint              y,
			       GtkSelectionData *selection_data,
			       guint             info,
			       guint             timestamp)
{
	gchar **uri_list;
	
	/* If this is an URL emit DROP_URIS, otherwise chain up the signal */
	if (info == TARGET_URI_LIST)
	{
		uri_list = gedit_utils_drop_get_uris (selection_data);
		
		if (uri_list != NULL)
		{
			g_signal_emit_by_name (widget, "drop-uris", uri_list);
			g_strfreev (uri_list);
			
			gtk_drag_finish (context, TRUE, FALSE, timestamp);
		}
	}
	else
	{
		GTK_WIDGET_CLASS (gedit_text_view_parent_class)->drag_data_received (widget, context, x, y, selection_data, info, timestamp);
	}
}

static gboolean
gedit_text_view_drag_drop (GtkWidget      *widget,
		      GdkDragContext *context,
		      gint            x,
		      gint            y,
		      guint           timestamp)
{
	gboolean result;
	GdkAtom target;

	/* If this is a URL, just get the drag data */
	target = drag_get_uri_target (widget, context);

	if (target != GDK_NONE)
	{
		gtk_drag_get_data (widget, context, target, timestamp);
		result = TRUE;
	}
	else
	{
		/* Chain up */
		result = GTK_WIDGET_CLASS (gedit_text_view_parent_class)->drag_drop (widget, context, x, y, timestamp);
	}

	return result;
}

static void
show_line_numbers_toggled (GtkMenu   *menu,
			   GeditTextView *view)
{
	gboolean show;

	show = gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menu));

	gedit_prefs_manager_set_display_line_numbers (show);
}

static GtkWidget *
create_line_numbers_menu (GtkWidget *view)
{
	GtkWidget *menu;
	GtkWidget *item;

	menu = gtk_menu_new ();

	item = gtk_check_menu_item_new_with_mnemonic (_("_Display line numbers"));
	gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item),
					gtk_source_view_get_show_line_numbers (GTK_SOURCE_VIEW (view)));
	g_signal_connect (item, "toggled",
			  G_CALLBACK (show_line_numbers_toggled), view);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
	
	gtk_widget_show_all (menu);
	
	return menu;
}

static void
show_line_numbers_menu (GtkWidget      *view,
			GdkEventButton *event)
{
	GtkWidget *menu;

	menu = create_line_numbers_menu (view);

	gtk_menu_popup (GTK_MENU (menu), 
			NULL, 
			NULL,
			NULL, 
			NULL,
			event->button, 
			event->time);
}

static gboolean
gedit_text_view_button_press_event (GtkWidget *widget, GdkEventButton *event)
{
	if ((event->type == GDK_BUTTON_PRESS) && 
	    (event->button == 3) &&
	    (event->window == gtk_text_view_get_window (GTK_TEXT_VIEW (widget),
						        GTK_TEXT_WINDOW_LEFT)))
	{
		show_line_numbers_menu (widget, event);

		return TRUE;
	}

	return GTK_WIDGET_CLASS (gedit_text_view_parent_class)->button_press_event (widget, event);
}

static void 	
search_highlight_updated_cb (GeditDocument *doc,
			     GtkTextIter   *start,
			     GtkTextIter   *end,
			     GeditTextView     *view)
{
	GdkRectangle visible_rect;
	GdkRectangle updated_rect;
	GdkRectangle redraw_rect;
	gint y;
	gint height;
	GtkTextView *text_view;
	
	text_view = GTK_TEXT_VIEW (view);

	g_return_if_fail (gedit_text_buffer_get_enable_search_highlighting (
				GEDIT_TEXT_BUFFER (gtk_text_view_get_buffer (text_view))));
	
	/* get visible area */
	gtk_text_view_get_visible_rect (text_view, &visible_rect);
	
	/* get updated rectangle */
	gtk_text_view_get_line_yrange (text_view, start, &y, &height);
	updated_rect.y = y;
	gtk_text_view_get_line_yrange (text_view, end, &y, &height);
	updated_rect.height = y + height - updated_rect.y;
	updated_rect.x = visible_rect.x;
	updated_rect.width = visible_rect.width;

	/* intersect both rectangles to see whether we need to queue a redraw */
	if (gdk_rectangle_intersect (&updated_rect, &visible_rect, &redraw_rect)) 
	{
		GdkRectangle widget_rect;
		
		gtk_text_view_buffer_to_window_coords (text_view,
						       GTK_TEXT_WINDOW_WIDGET,
						       redraw_rect.x,
						       redraw_rect.y,
						       &widget_rect.x,
						       &widget_rect.y);
		
		widget_rect.width = redraw_rect.width;
		widget_rect.height = redraw_rect.height;
		
		gtk_widget_queue_draw_area (GTK_WIDGET (text_view),
					    widget_rect.x,
					    widget_rect.y,
					    widget_rect.width,
					    widget_rect.height);
	}
}

/* There is no "official" way to reset the im context in GtkTextView */
static void
reset_im_context (GtkTextView *text_view)
{
	if (text_view->need_im_reset)
	{
		text_view->need_im_reset = FALSE;
		gtk_im_context_reset (text_view->im_context);
	}
}

static void
delete_line (GtkTextView *text_view,
	     gint         count)
{
	GtkTextIter start;
	GtkTextIter end;
	GtkTextBuffer *buffer;

	buffer = gtk_text_view_get_buffer (text_view);

	reset_im_context (text_view);

	/* If there is a selection delete the selected lines and
	 * ignore count */
	if (gtk_text_buffer_get_selection_bounds (buffer, &start, &end))
	{
		gtk_text_iter_order (&start, &end);
		
		if (gtk_text_iter_starts_line (&end))
		{
			/* Do no delete the line with the cursor if the cursor
			 * is at the beginning of the line */
			count = 0;
		}
		else	
			count = 1;
	}
	
	gtk_text_iter_set_line_offset (&start, 0);

	if (count > 0)
	{		
		gtk_text_iter_forward_lines (&end, count);

		if (gtk_text_iter_is_end (&end))
		{		
			if (gtk_text_iter_backward_line (&start) && !gtk_text_iter_ends_line (&start))
				gtk_text_iter_forward_to_line_end (&start);
		}
	}
	else if (count < 0) 
	{
		if (!gtk_text_iter_ends_line (&end))
			gtk_text_iter_forward_to_line_end (&end);

		while (count < 0) 
		{
			if (!gtk_text_iter_backward_line (&start))
				break;
				
			++count;
		}

		if (count == 0)
		{
			if (!gtk_text_iter_ends_line (&start))
				gtk_text_iter_forward_to_line_end (&start);
		}
		else
			gtk_text_iter_forward_line (&end);
	}

	if (!gtk_text_iter_equal (&start, &end))
	{
		GtkTextIter cur = start;
		gtk_text_iter_set_line_offset (&cur, 0);
		
		gtk_text_buffer_begin_user_action (buffer);

		gtk_text_buffer_place_cursor (buffer, &cur);

		gtk_text_buffer_delete_interactive (buffer, 
						    &start,
						    &end,
						    gtk_text_view_get_editable (text_view));

		gtk_text_buffer_end_user_action (buffer);

		gtk_text_view_scroll_mark_onscreen (text_view,
						    gtk_text_buffer_get_insert (buffer));
	}
	else
	{
		gtk_widget_error_bell (GTK_WIDGET (text_view));
	}
}

static void
gedit_text_view_delete_from_cursor (GtkTextView   *text_view,
			       GtkDeleteType  type,
			       gint           count)
{
	/* We override the standard handler for delete_from_cursor since
	   the GTK_DELETE_PARAGRAPHS case is not implemented as we like (i.e. it
	   does not remove the carriage return in the previous line)
	 */
	switch (type)
	{
		case GTK_DELETE_PARAGRAPHS:
			delete_line (text_view, count);
			break;
		default:
			GTK_TEXT_VIEW_CLASS (gedit_text_view_parent_class)->delete_from_cursor(text_view, type, count);
			break;
	}
}
