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
#include <libpeas/peas-extension-set.h>

#include <glib/gi18n.h>

#include "gedit-view.h"
#include "gedit-view-activatable.h"
#include "gedit-plugins-engine.h"
#include "gedit-debug.h"
#include "gedit-marshal.h"
#include "gedit-utils.h"
#include "gedit-settings.h"
#include "gedit-app.h"
#include "gedit-notebook.h"

#define GEDIT_VIEW_SCROLL_MARGIN 0.02

#define GEDIT_VIEW_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), GEDIT_TYPE_VIEW, GeditViewPrivate))

typedef enum
{
	GOTO_LINE,
	SEARCH
} SearchMode;

enum
{
	TARGET_URI_LIST = 100,
	TARGET_TAB
};

struct _GeditViewPrivate
{
	GSettings *editor_settings;
	GtkTextBuffer *current_buffer;
	PeasExtensionSet *extensions;
};

static void	gedit_view_finalize		(GObject          *object);
static void	gedit_view_destroy		(GtkWidget        *widget);
static gint	gedit_view_focus_out		(GtkWidget        *widget,
						 GdkEventFocus    *event);
static gboolean gedit_view_drag_motion		(GtkWidget        *widget,
						 GdkDragContext   *context,
						 gint              x,
						 gint              y,
						 guint             timestamp);
static void     gedit_view_drag_data_received   (GtkWidget        *widget,
						 GdkDragContext   *context,
						 gint              x,
						 gint              y,
						 GtkSelectionData *selection_data,
						 guint             info,
						 guint             timestamp);
static gboolean gedit_view_drag_drop		(GtkWidget        *widget,
	      					 GdkDragContext   *context,
	      					 gint              x,
	      					 gint              y,
	      					 guint             timestamp);
static gboolean	gedit_view_button_press_event	(GtkWidget        *widget,
						 GdkEventButton   *event);
static void	gedit_view_realize		(GtkWidget        *widget);

static gboolean reset_searched_text		(GeditView        *view);


static gboolean	gedit_view_draw			(GtkWidget        *widget,
						 cairo_t          *cr);
static void 	search_highlight_updated_cb	(GeditDocument    *doc,
						 GtkTextIter      *start,
						 GtkTextIter      *end,
						 GeditView        *view);

static void	gedit_view_delete_from_cursor 	(GtkTextView      *text_view,
						 GtkDeleteType     type,
						 gint              count);

G_DEFINE_TYPE(GeditView, gedit_view, GTK_TYPE_SOURCE_VIEW)

/* Signals */
enum
{
	START_INTERACTIVE_SEARCH,
	START_INTERACTIVE_GOTO_LINE,
	RESET_SEARCHED_TEXT,
	DROP_URIS,
	LAST_SIGNAL
};

static guint view_signals [LAST_SIGNAL] = { 0 };

static void
document_read_only_notify_handler (GeditDocument *document, 
			           GParamSpec    *pspec,
				   GeditView     *view)
{
	gedit_debug (DEBUG_VIEW);

	gtk_text_view_set_editable (GTK_TEXT_VIEW (view), 
				    !gedit_document_get_readonly (document));
}

static void
gedit_view_class_init (GeditViewClass *klass)
{
	GObjectClass     *object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass   *widget_class = GTK_WIDGET_CLASS (klass);
	GtkTextViewClass *text_view_class = GTK_TEXT_VIEW_CLASS (klass);
	GtkBindingSet    *binding_set;

	object_class->finalize = gedit_view_finalize;

	widget_class->destroy = gedit_view_destroy;
	widget_class->focus_out_event = gedit_view_focus_out;
	widget_class->draw = gedit_view_draw;

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
	widget_class->drag_motion = gedit_view_drag_motion;
	widget_class->drag_data_received = gedit_view_drag_data_received;
	widget_class->drag_drop = gedit_view_drag_drop;
	widget_class->button_press_event = gedit_view_button_press_event;
	widget_class->realize = gedit_view_realize;
	klass->reset_searched_text = reset_searched_text;

	text_view_class->delete_from_cursor = gedit_view_delete_from_cursor;

	view_signals[START_INTERACTIVE_SEARCH] =
		g_signal_new ("start-interactive-search",
		              G_TYPE_FROM_CLASS (object_class),
		              G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		              G_STRUCT_OFFSET (GeditViewClass, start_interactive_search),
		              NULL, NULL,
		              gedit_marshal_BOOLEAN__NONE,
		              G_TYPE_BOOLEAN, 0);

	view_signals[START_INTERACTIVE_GOTO_LINE] =
		g_signal_new ("start-interactive-goto-line",
		              G_TYPE_FROM_CLASS (object_class),
		              G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		              G_STRUCT_OFFSET (GeditViewClass, start_interactive_goto_line),
		              NULL, NULL,
		              gedit_marshal_BOOLEAN__NONE,
		              G_TYPE_BOOLEAN, 0);

	view_signals[RESET_SEARCHED_TEXT] =
		g_signal_new ("reset-searched-text",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
			      G_STRUCT_OFFSET (GeditViewClass, reset_searched_text),
			      NULL, NULL,
			      gedit_marshal_BOOLEAN__NONE,
			      G_TYPE_BOOLEAN, 0);

	/* A new signal DROP_URIS has been added to allow plugins to intercept
	 * the default dnd behaviour of 'text/uri-list'. GeditView now handles
	 * dnd in the default handlers of drag_drop, drag_motion and 
	 * drag_data_received. The view emits drop_uris from drag_data_received
	 * if valid uris have been dropped. Plugins should connect to 
	 * drag_motion, drag_drop and drag_data_received to change this 
	 * default behaviour. They should _NOT_ use this signal because this
	 * will not prevent gedit from loading the uri
	 */
	view_signals[DROP_URIS] =
    		g_signal_new ("drop_uris",
		  	      G_TYPE_FROM_CLASS (object_class),
		  	      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
		  	      G_STRUCT_OFFSET (GeditViewClass, drop_uris),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__BOXED,
			      G_TYPE_NONE, 1, G_TYPE_STRV);
			      
	g_type_class_add_private (klass, sizeof (GeditViewPrivate));
	
	binding_set = gtk_binding_set_by_class (klass);

	gtk_binding_entry_add_signal (binding_set,
	                              GDK_KEY_f,
	                              GDK_CONTROL_MASK,
	                              "start-interactive-search", 0);

	gtk_binding_entry_add_signal (binding_set,
	                              GDK_KEY_i,
	                              GDK_CONTROL_MASK,
	                              "start-interactive-goto-line", 0);

	gtk_binding_entry_add_signal (binding_set,
				      GDK_KEY_k,
				      GDK_CONTROL_MASK | GDK_SHIFT_MASK,
				      "reset_searched_text", 0);

	gtk_binding_entry_add_signal (binding_set, 
				      GDK_KEY_d,
				      GDK_CONTROL_MASK,
				      "delete_from_cursor", 2,
				      G_TYPE_ENUM, GTK_DELETE_PARAGRAPHS,
				      G_TYPE_INT, 1);
}

static void
current_buffer_removed (GeditView *view)
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
extension_added (PeasExtensionSet *extensions,
		 PeasPluginInfo   *info,
		 PeasExtension    *exten,
		 GeditView        *view)
{
	peas_extension_call (exten, "activate");
}

static void
extension_removed (PeasExtensionSet *extensions,
		   PeasPluginInfo   *info,
		   PeasExtension    *exten,
		   GeditView        *view)
{
	peas_extension_call (exten, "deactivate");
}

static void
on_notify_buffer_cb (GeditView  *view,
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
gedit_view_init (GeditView *view)
{
	GObject *gs;
	GtkTargetList *tl;
	gboolean use_default_font;
	gboolean display_line_numbers;
	gboolean auto_indent;
	gboolean insert_spaces;
	gboolean display_right_margin;
	gboolean hl_current_line;
	guint tabs_size;
	guint right_margin_position;
	GtkWrapMode wrap_mode;
	GtkSourceSmartHomeEndType smart_home_end;
	
	gedit_debug (DEBUG_VIEW);
	
	view->priv = GEDIT_VIEW_GET_PRIVATE (view);

	gs = _gedit_app_get_settings (gedit_app_get_default ());
	view->priv->editor_settings = g_settings_new ("org.gnome.gedit.preferences.editor");

	/* Get setting values */
	use_default_font = g_settings_get_boolean (view->priv->editor_settings,
						   GEDIT_SETTINGS_USE_DEFAULT_FONT);

	/*
	 *  Set tab, fonts, wrap mode, colors, etc. according
	 *  to preferences 
	 */
	if (!use_default_font)
	{
		gchar *editor_font;

		editor_font = g_settings_get_string (view->priv->editor_settings,
						     GEDIT_SETTINGS_EDITOR_FONT);

		gedit_view_set_font (view, FALSE, editor_font);

		g_free (editor_font);
	}
	else
	{
		gedit_view_set_font (view, TRUE, NULL);
	}
 
	display_line_numbers = g_settings_get_boolean (view->priv->editor_settings,
						       GEDIT_SETTINGS_DISPLAY_LINE_NUMBERS);
	auto_indent = g_settings_get_boolean (view->priv->editor_settings,
					      GEDIT_SETTINGS_AUTO_INDENT);
	g_settings_get (view->priv->editor_settings, GEDIT_SETTINGS_TABS_SIZE,
			"u", &tabs_size);
	insert_spaces = g_settings_get_boolean (view->priv->editor_settings,
						GEDIT_SETTINGS_INSERT_SPACES);
	display_right_margin = g_settings_get_boolean (view->priv->editor_settings,
						       GEDIT_SETTINGS_DISPLAY_RIGHT_MARGIN);
	g_settings_get (view->priv->editor_settings, GEDIT_SETTINGS_RIGHT_MARGIN_POSITION,
			"u", &right_margin_position);
	hl_current_line = g_settings_get_boolean (view->priv->editor_settings,
						  GEDIT_SETTINGS_HIGHLIGHT_CURRENT_LINE);

	wrap_mode = g_settings_get_enum (view->priv->editor_settings,
					 GEDIT_SETTINGS_WRAP_MODE);
	
	smart_home_end = g_settings_get_enum (view->priv->editor_settings,
					      GEDIT_SETTINGS_SMART_HOME_END);

	g_object_set (G_OBJECT (view), 
		      "wrap_mode", wrap_mode,
		      "show_line_numbers", display_line_numbers,
		      "auto_indent", auto_indent,
		      "tab_width", tabs_size,
		      "insert_spaces_instead_of_tabs", insert_spaces,
		      "show_right_margin", display_right_margin,
		      "right_margin_position", right_margin_position,
		      "highlight_current_line", hl_current_line,
		      "smart_home_end", smart_home_end,
		      "indent_on_tab", TRUE,
		      NULL);

	/* Drag and drop support */
	tl = gtk_drag_dest_get_target_list (GTK_WIDGET (view));

	if (tl != NULL)
	{
		gtk_target_list_add_uri_targets (tl, TARGET_URI_LIST);
		gtk_target_list_add (tl,
		                     gdk_atom_intern_static_string ("GTK_NOTEBOOK_TAB"),
		                     GTK_TARGET_SAME_APP,
		                     TARGET_TAB);
	}

	view->priv->extensions = peas_extension_set_new (PEAS_ENGINE (gedit_plugins_engine_get_default ()),
							 GEDIT_TYPE_VIEW_ACTIVATABLE,
							 "view", view,
							 NULL);
	g_signal_connect (view->priv->extensions,
			  "extension-added",
			  G_CALLBACK (extension_added),
			  view);
	g_signal_connect (view->priv->extensions,
			  "extension-removed",
			  G_CALLBACK (extension_removed),
			  view);

	/* Act on buffer change */
	g_signal_connect (view, 
			  "notify::buffer", 
			  G_CALLBACK (on_notify_buffer_cb),
			  NULL);
}

static void
gedit_view_destroy (GtkWidget *widget)
{
	GeditView *view;

	view = GEDIT_VIEW (widget);

	if (view->priv->extensions != NULL)
	{
		peas_extension_set_call (view->priv->extensions, "deactivate");
		g_object_unref (view->priv->extensions);
		view->priv->extensions = NULL;
	}

	/* Disconnect notify buffer because the destroy of the textview will
	   set the buffer to NULL, and we call get_buffer in the notify which
	   would reinstate a GtkTextBuffer which we don't want */
	current_buffer_removed (view);
	g_signal_handlers_disconnect_by_func (view, on_notify_buffer_cb, NULL);

	if (view->priv->editor_settings != NULL)
	{
		g_object_unref (view->priv->editor_settings);
		view->priv->editor_settings = NULL;
	}

	GTK_WIDGET_CLASS (gedit_view_parent_class)->destroy (widget);
}

static void
gedit_view_finalize (GObject *object)
{
	GeditView *view;

	view = GEDIT_VIEW (object);

	current_buffer_removed (view);

	G_OBJECT_CLASS (gedit_view_parent_class)->finalize (object);
}

static gint
gedit_view_focus_out (GtkWidget *widget, GdkEventFocus *event)
{
	gtk_widget_queue_draw (widget);

	GTK_WIDGET_CLASS (gedit_view_parent_class)->focus_out_event (widget, event);

	return FALSE;
}

/**
 * gedit_view_new:
 * @doc: a #GeditDocument
 * 
 * Creates a new #GeditView object displaying the @doc document. 
 * @doc cannot be %NULL.
 *
 * Return value: a new #GeditView
 **/
GtkWidget *
gedit_view_new (GeditDocument *doc)
{
	GtkWidget *view;

	gedit_debug_message (DEBUG_VIEW, "START");

	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), NULL);

	view = GTK_WIDGET (g_object_new (GEDIT_TYPE_VIEW, "buffer", doc, NULL));

	gedit_debug_message (DEBUG_VIEW, "END: %d", G_OBJECT (view)->ref_count);

	gtk_widget_show_all (view);

	return view;
}

void
gedit_view_cut_clipboard (GeditView *view)
{
	GtkTextBuffer *buffer;
	GtkClipboard *clipboard;

	gedit_debug (DEBUG_VIEW);

	g_return_if_fail (GEDIT_IS_VIEW (view));

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
				      GEDIT_VIEW_SCROLL_MARGIN,
				      FALSE,
				      0.0,
				      0.0);
}

void
gedit_view_copy_clipboard (GeditView *view)
{
	GtkTextBuffer *buffer;
	GtkClipboard *clipboard;

	gedit_debug (DEBUG_VIEW);

	g_return_if_fail (GEDIT_IS_VIEW (view));

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
	g_return_if_fail (buffer != NULL);

	clipboard = gtk_widget_get_clipboard (GTK_WIDGET (view),
					      GDK_SELECTION_CLIPBOARD);

  	gtk_text_buffer_copy_clipboard (buffer, clipboard);

	/* on copy do not scroll, we are already on screen */
}

void
gedit_view_paste_clipboard (GeditView *view)
{
  	GtkTextBuffer *buffer;
	GtkClipboard *clipboard;

	gedit_debug (DEBUG_VIEW);

	g_return_if_fail (GEDIT_IS_VIEW (view));

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
				      GEDIT_VIEW_SCROLL_MARGIN,
				      FALSE,
				      0.0,
				      0.0);
}

/**
 * gedit_view_delete_selection:
 * @view: a #GeditView
 * 
 * Deletes the text currently selected in the #GtkTextBuffer associated
 * to the view and scroll to the cursor position.
 **/
void
gedit_view_delete_selection (GeditView *view)
{
  	GtkTextBuffer *buffer = NULL;

	gedit_debug (DEBUG_VIEW);

	g_return_if_fail (GEDIT_IS_VIEW (view));

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
	g_return_if_fail (buffer != NULL);

	/* FIXME: what is default editability of a buffer? */
	gtk_text_buffer_delete_selection (buffer,
					  TRUE,
					  !gedit_document_get_readonly (
						GEDIT_DOCUMENT (buffer)));
						
	gtk_text_view_scroll_to_mark (GTK_TEXT_VIEW (view),
				      gtk_text_buffer_get_insert (buffer),
				      GEDIT_VIEW_SCROLL_MARGIN,
				      FALSE,
				      0.0,
				      0.0);
}

/**
 * gedit_view_select_all:
 * @view: a #GeditView
 * 
 * Selects all the text displayed in the @view.
 **/
void
gedit_view_select_all (GeditView *view)
{
	GtkTextBuffer *buffer = NULL;
	GtkTextIter start, end;

	gedit_debug (DEBUG_VIEW);

	g_return_if_fail (GEDIT_IS_VIEW (view));

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
	g_return_if_fail (buffer != NULL);

	gtk_text_buffer_get_bounds (buffer, &start, &end);
	gtk_text_buffer_select_range (buffer, &start, &end);
}

/**
 * gedit_view_scroll_to_cursor:
 * @view: a #GeditView
 * 
 * Scrolls the @view to the cursor position.
 **/
void
gedit_view_scroll_to_cursor (GeditView *view)
{
	GtkTextBuffer* buffer = NULL;

	gedit_debug (DEBUG_VIEW);

	g_return_if_fail (GEDIT_IS_VIEW (view));
	
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
	g_return_if_fail (buffer != NULL);

	gtk_text_view_scroll_to_mark (GTK_TEXT_VIEW (view),
				      gtk_text_buffer_get_insert (buffer),
				      0.25,
				      FALSE,
				      0.0,
				      0.0);
}

/* FIXME this is an issue for introspection */
/**
 * gedit_view_set_font:
 * @view: a #GeditView
 * @def: whether to reset the default font
 * @font_name: the name of the font to use
 * 
 * If @def is #TRUE, resets the font of the @view to the default font
 * otherwise sets it to @font_name.
 **/
void
gedit_view_set_font (GeditView   *view, 
		     gboolean     def, 
		     const gchar *font_name)
{
	PangoFontDescription *font_desc = NULL;

	gedit_debug (DEBUG_VIEW);

	g_return_if_fail (GEDIT_IS_VIEW (view));

	if (def)
	{
		GObject *settings;
		gchar *font;

		settings = _gedit_app_get_settings (gedit_app_get_default ());
		font = gedit_settings_get_system_font (GEDIT_SETTINGS (settings));
		
		font_desc = pango_font_description_from_string (font);
		g_free (font);
	}
	else
	{
		g_return_if_fail (font_name != NULL);

		font_desc = pango_font_description_from_string (font_name);
	}

	g_return_if_fail (font_desc != NULL);

	gtk_widget_override_font (GTK_WIDGET (view), font_desc);

	pango_font_description_free (font_desc);
}

static gboolean
reset_searched_text (GeditView *view)
{		
	GeditDocument *doc;

	doc = GEDIT_DOCUMENT (gtk_text_view_get_buffer (GTK_TEXT_VIEW (view)));
	
	gedit_document_set_search_text (doc, "", GEDIT_SEARCH_DONT_SET_FLAGS);
	
	return TRUE;
}

static gboolean
gedit_view_draw (GtkWidget *widget,
                 cairo_t   *cr)
{
	GtkTextView *text_view;
	GeditDocument *doc;
	GdkWindow *window;

	text_view = GTK_TEXT_VIEW (widget);

	doc = GEDIT_DOCUMENT (gtk_text_view_get_buffer (text_view));
	window = gtk_text_view_get_window (text_view, GTK_TEXT_WINDOW_TEXT);

	if (gtk_cairo_should_draw_window (cr, window) &&
	    gedit_document_get_enable_search_highlighting (doc))
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
				     
		_gedit_document_search_region (doc,
					       &iter1,
					       &iter2);
	}

	return GTK_WIDGET_CLASS (gedit_view_parent_class)->draw (widget, cr);
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
gedit_view_drag_motion (GtkWidget      *widget,
			GdkDragContext *context,
			gint            x,
			gint            y,
			guint           timestamp)
{
	gboolean result;

	/* Chain up to allow textview to scroll and position dnd mark, note 
	 * that this needs to be checked if gtksourceview or gtktextview
	 * changes drag_motion behaviour */
	result = GTK_WIDGET_CLASS (gedit_view_parent_class)->drag_motion (widget, context, x, y, timestamp);

	/* If this is a URL, deal with it here */
	if (drag_get_uri_target (widget, context) != GDK_NONE) 
	{
		gdk_drag_status (context,
				 gdk_drag_context_get_suggested_action (context),
				 timestamp);
		result = TRUE;
	}

	return result;
}

static GtkWidget *
get_notebook_from_view (GtkWidget *view)
{
	GtkWidget *widget;

	widget = view;

	do
	{
		widget = gtk_widget_get_parent (widget);
	}
	while (!GEDIT_IS_NOTEBOOK (widget));

	return widget;
}

static void
gedit_view_drag_data_received (GtkWidget        *widget,
		       	       GdkDragContext   *context,
			       gint              x,
			       gint              y,
			       GtkSelectionData *selection_data,
			       guint             info,
			       guint             timestamp)
{
	/* If this is an URL emit DROP_URIS, otherwise chain up the signal */
	if (info == TARGET_URI_LIST)
	{
		gchar **uri_list;

		uri_list = gedit_utils_drop_get_uris (selection_data);
		
		if (uri_list != NULL)
		{
			g_signal_emit (widget, view_signals[DROP_URIS], 0, uri_list);
			g_strfreev (uri_list);
			
			gtk_drag_finish (context, TRUE, FALSE, timestamp);
		}
	}
	else if (info == TARGET_TAB)
	{
		GtkWidget *notebook;
		GtkWidget *new_notebook;
		GtkWidget *page;

		notebook = gtk_drag_get_source_widget (context);

		if (!GTK_IS_WIDGET (notebook))
		{
			return;
		}

		page = *(GtkWidget **) gtk_selection_data_get_data (selection_data);
		g_return_if_fail (page != NULL);

		/* We need to iterate and get the notebook of the target view
		   because we can have several notebooks per window */
		new_notebook = get_notebook_from_view (widget);

		if (notebook != new_notebook)
		{
			gedit_notebook_move_tab (GEDIT_NOTEBOOK (notebook),
				                 GEDIT_NOTEBOOK (new_notebook),
				                 GEDIT_TAB (page),
				                 0);
		}

		gtk_drag_finish (context, TRUE, TRUE, timestamp);
	}
	else
	{
		GTK_WIDGET_CLASS (gedit_view_parent_class)->drag_data_received (widget,
										context,
										x, y,
										selection_data,
										info,
										timestamp);
	}
}

static gboolean
gedit_view_drag_drop (GtkWidget      *widget,
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
		result = GTK_WIDGET_CLASS (gedit_view_parent_class)->drag_drop (widget,
										context,
										x, y,
										timestamp);
	}

	return result;
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

	g_settings_bind (GEDIT_VIEW (view)->priv->editor_settings,
			 GEDIT_SETTINGS_DISPLAY_LINE_NUMBERS,
			 item,
			 "active",
			 G_SETTINGS_BIND_SET);
	
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
gedit_view_button_press_event (GtkWidget *widget, GdkEventButton *event)
{
	if ((event->type == GDK_BUTTON_PRESS) && 
	    (event->button == 3) &&
	    (event->window == gtk_text_view_get_window (GTK_TEXT_VIEW (widget),
						        GTK_TEXT_WINDOW_LEFT)))
	{
		show_line_numbers_menu (widget, event);

		return TRUE;
	}

	return GTK_WIDGET_CLASS (gedit_view_parent_class)->button_press_event (widget, event);
}

static void
gedit_view_realize (GtkWidget *widget)
{
	GeditView *view = GEDIT_VIEW (widget);

	/* We only activate the extensions when the view is realized,
	 * because most plugins will expect this behaviour, and we won't
	 * change the buffer later anyway. */
	peas_extension_set_call (view->priv->extensions, "activate");

	GTK_WIDGET_CLASS (gedit_view_parent_class)->realize (widget);
}

static void 	
search_highlight_updated_cb (GeditDocument *doc,
			     GtkTextIter   *start,
			     GtkTextIter   *end,
			     GeditView     *view)
{
	GdkRectangle visible_rect;
	GdkRectangle updated_rect;	
	GdkRectangle redraw_rect;
	gint y;
	gint height;
	GtkTextView *text_view;
	
	text_view = GTK_TEXT_VIEW (view);

	g_return_if_fail (gedit_document_get_enable_search_highlighting (
				GEDIT_DOCUMENT (gtk_text_view_get_buffer (text_view))));
	
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

static void
delete_line (GtkTextView *text_view,
	     gint         count)
{
	GtkTextIter start;
	GtkTextIter end;
	GtkTextBuffer *buffer;

	buffer = gtk_text_view_get_buffer (text_view);

	gtk_text_view_reset_im_context (text_view);

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
		{
			count = 1;
		}
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
		{
			gtk_text_iter_forward_line (&end);
		}
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
gedit_view_delete_from_cursor (GtkTextView   *text_view,
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
			GTK_TEXT_VIEW_CLASS (gedit_view_parent_class)->delete_from_cursor(text_view, type, count);
			break;
	}
}

/* ex:set ts=8 noet: */
