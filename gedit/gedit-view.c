/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-view.c
 * This file is part of gedit
 *
 * Copyright (C) 1998, 1999 Alex Roberts, Evan Lawrence
 * Copyright (C) 2000, 2002 Chema Celorio, Paolo Maggi 
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
 * Modified by the gedit Team, 1998-2002. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include <glib/gi18n.h>
#include <gdk/gdkkeysyms.h>
#include <gtksourceview/gtksourceview.h>

#include "gedit-view.h"
#include "gedit-debug.h"
#include "gedit-menus.h"
#include "gedit-prefs-manager-app.h"
#include "gedit-marshal.h"

#define MIN_NUMBER_WINDOW_WIDTH 20

struct _GeditViewPrivate
{
	GtkSourceView *text_view;	

	GeditDocument *document;

	gboolean line_numbers_visible;

	GtkWidget *cursor_position_statusbar;
	GtkWidget *overwrite_mode_statusbar;

	gboolean overwrite_mode;
};

enum
{
 	POPULATE_POPUP,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

static void gedit_view_class_init 	(GeditViewClass	*klass);
static void gedit_view_init 		(GeditView 	*view);
static void gedit_view_finalize 	(GObject 	*object);

static void gedit_view_update_cursor_position_statusbar 
					(GtkTextBuffer *buffer, 
					 GeditView* view);
static void gedit_view_cursor_moved 	(GtkTextBuffer     *buffer,
					 const GtkTextIter *new_location,
					 GtkTextMark       *mark,
					 GeditView         *view);
static void gedit_view_update_overwrite_mode_statusbar (GtkTextView* w, GeditView* view);
static void gedit_view_doc_readonly_changed_handler (GeditDocument *document, 
						     gboolean readonly, 
						     GeditView *view);
static void gedit_view_populate_popup (GtkTextView *textview, GtkMenu *menu, gpointer data);

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
}

void 
gedit_view_set_editable (GeditView *view, gboolean editable)
{
	gedit_debug (DEBUG_VIEW, "");

	g_return_if_fail (GEDIT_IS_VIEW (view));

	gtk_text_view_set_editable (GTK_TEXT_VIEW (view->priv->text_view), 
				    editable);
}

static void
gedit_view_doc_readonly_changed_handler (GeditDocument *document, gboolean readonly,
		GeditView *view)
{
	gedit_debug (DEBUG_VIEW, "");

	g_return_if_fail (GEDIT_IS_VIEW (view));

	gedit_view_set_editable (view, !readonly);	
}

static void
gedit_view_class_init (GeditViewClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

  	parent_class = g_type_class_peek_parent (klass);

  	object_class->finalize = gedit_view_finalize;

	GTK_WIDGET_CLASS (klass)->grab_focus = gedit_view_grab_focus;

	signals[POPULATE_POPUP] =
		g_signal_new ("populate_popup",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GeditViewClass, populate_popup),
			      NULL, NULL,
			      gedit_marshal_VOID__OBJECT,
			      G_TYPE_NONE,
			      1,
			      GTK_TYPE_MENU);
}

static void
move_cursor (GtkTextView       *text_view,
	     const GtkTextIter *new_location,
	     gboolean           extend_selection)
{
	GtkTextBuffer *buffer = text_view->buffer;

	if (extend_selection)
		gtk_text_buffer_move_mark_by_name (buffer, "insert",
						   new_location);
	else
		gtk_text_buffer_place_cursor (buffer, new_location);

	gtk_text_view_scroll_mark_onscreen (text_view,
					    gtk_text_buffer_get_insert (buffer));
}

/*
 * This feature is implemented in gedit and not in gtksourceview since the latter
 * has a similar feature called smart home/end that it is non-capatible with this 
 * one and is more "invasive". May be in the future we will move this feature in 
 * gtksourceview.
 */
static void
gedit_view_move_cursor (GtkTextView    *text_view,
			GtkMovementStep step,
			gint            count,
			gboolean        extend_selection,
			gpointer	data)
{
	GtkTextBuffer *buffer = text_view->buffer;
	GtkTextMark *mark;
	GtkTextIter cur, iter;

	if (step != GTK_MOVEMENT_DISPLAY_LINE_ENDS)
		return;

	g_return_if_fail (!gtk_source_view_get_smart_home_end (GTK_SOURCE_VIEW (text_view)));	
			
	mark = gtk_text_buffer_get_insert (buffer);
	gtk_text_buffer_get_iter_at_mark (buffer, &cur, mark);
	iter = cur;

	if ((count == -1) && gtk_text_iter_starts_line (&iter))
	{
		/* Find the iter of the first character on the line. */
		while (!gtk_text_iter_ends_line (&cur))
		{
			gunichar c = gtk_text_iter_get_char (&cur);
			if (g_unichar_isspace (c))
				gtk_text_iter_forward_char (&cur);
			else
				break;
		}

		if (!gtk_text_iter_equal (&cur, &iter))
		{
			move_cursor (text_view, &cur, extend_selection);
			g_signal_stop_emission_by_name (text_view, "move-cursor");
		}

		return;
	}

	if ((count == 1) && gtk_text_iter_ends_line (&iter))
	{
		/* Find the iter of the last character on the line. */
		while (!gtk_text_iter_starts_line (&cur))
		{
			gunichar c;
			gtk_text_iter_backward_char (&cur);
			c = gtk_text_iter_get_char (&cur);
			if (!g_unichar_isspace (c))
			{
				/* We've gone one character too far. */
				gtk_text_iter_forward_char (&cur);
				break;
			}
		}

		if (!gtk_text_iter_equal (&cur, &iter))
		{
			move_cursor (text_view, &cur, extend_selection);
			g_signal_stop_emission_by_name (text_view, "move-cursor");
		}
	}
}

static void 
gedit_view_init (GeditView  *view)
{
	GtkSourceView *text_view;
	GtkWidget *sw; /* the scrolled window */
	GdkColor background, text, selection, sel_text;
	
	gedit_debug (DEBUG_VIEW, "");

	view->priv = g_new0 (GeditViewPrivate, 1);

	view->priv->document = NULL;	
	view->priv->line_numbers_visible = FALSE;

	/* Create the scrolled window */
	sw = gtk_scrolled_window_new (NULL, NULL);
	g_return_if_fail (sw != NULL);
	
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);

	text_view = GTK_SOURCE_VIEW (gtk_source_view_new ());
	g_return_if_fail (text_view != NULL);
	view->priv->text_view = text_view;

	/*
	 *  Set tab, fonts, wrap mode, colors, etc. according
	 *  to preferences 
	 */
	if (!gedit_prefs_manager_get_use_default_font ())
	{
		gchar *editor_font = gedit_prefs_manager_get_editor_font ();
		
		gedit_view_set_font (view, FALSE, editor_font);

		g_free (editor_font);
	}

	if (!gedit_prefs_manager_get_use_default_colors ())
	{
		background = gedit_prefs_manager_get_background_color ();
		text = gedit_prefs_manager_get_text_color ();
		selection = gedit_prefs_manager_get_selection_color ();
		sel_text = gedit_prefs_manager_get_selected_text_color ();

		gedit_view_set_colors (view, FALSE,
				&background, &text, &selection, &sel_text);
	}	

	gedit_view_set_wrap_mode (view, gedit_prefs_manager_get_wrap_mode ());

	gedit_view_set_auto_indent (view, gedit_prefs_manager_get_auto_indent ());

	gedit_view_set_tab_size (view, gedit_prefs_manager_get_tabs_size ());
	gedit_view_set_insert_spaces_instead_of_tabs (view, 
			gedit_prefs_manager_get_insert_spaces ());
	
	g_object_set (G_OBJECT (view->priv->text_view), 
		      "show_margin", gedit_prefs_manager_get_display_right_margin (), 
		      "margin", gedit_prefs_manager_get_right_margin_position (),
		      "highlight_current_line", gedit_prefs_manager_get_highlight_current_line (), 
		      "smart_home_end", FALSE, /* Never changes this */
		      NULL);

	gtk_box_pack_start (GTK_BOX (view), sw, TRUE, TRUE, 0);
	gtk_container_add (GTK_CONTAINER (sw), GTK_WIDGET (view->priv->text_view));
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw),
                                             GTK_SHADOW_IN);

	g_signal_connect (G_OBJECT (view->priv->text_view), "populate-popup",
			  G_CALLBACK (gedit_view_populate_popup), view);

	g_signal_connect (G_OBJECT (view->priv->text_view), "move-cursor",
			  G_CALLBACK (gedit_view_move_cursor), view);

	
}

static void 
gedit_view_finalize (GObject *object)
{
	GeditView *view;

	gedit_debug (DEBUG_VIEW, "%d", object->ref_count);

	g_return_if_fail (object != NULL);
	g_return_if_fail (GEDIT_IS_VIEW (object));

   	view = GEDIT_VIEW (object);

	g_return_if_fail (GEDIT_IS_VIEW (view));
	g_return_if_fail (view->priv != NULL);

	g_return_if_fail (view->priv->document != NULL);

	g_signal_handlers_disconnect_matched (G_OBJECT (view->priv->document),
			(GSignalMatchType)G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, view);

	g_object_unref (view->priv->document);
	view->priv->document = NULL;

	g_free (view->priv);

	G_OBJECT_CLASS (parent_class)->finalize (object);

	gedit_debug (DEBUG_VIEW, "END");
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
	
	gtk_text_view_set_buffer (GTK_TEXT_VIEW (view->priv->text_view), 
				  GTK_TEXT_BUFFER (doc));

	view->priv->document = doc;
	g_object_ref (view->priv->document);

	gtk_text_view_scroll_to_mark (GTK_TEXT_VIEW (view->priv->text_view),
				      gtk_text_buffer_get_mark (GTK_TEXT_BUFFER (doc), "insert"),
				      0, TRUE, 0.0, 1.0);

	if (gedit_prefs_manager_get_display_line_numbers ())
		gedit_view_show_line_numbers (view, TRUE);
			
	gtk_widget_show_all (GTK_WIDGET (view));

	g_signal_connect (GTK_TEXT_BUFFER (doc), 
			  "changed",
			  G_CALLBACK (gedit_view_update_cursor_position_statusbar),
			  view);

	g_signal_connect (GTK_TEXT_BUFFER (doc),
			  "mark_set",/* cursor moved */
			  G_CALLBACK (gedit_view_cursor_moved),
			  view);

	g_signal_connect (GTK_TEXT_VIEW (view->priv->text_view),
			  "toggle_overwrite",/* cursor moved */
			  G_CALLBACK (gedit_view_update_overwrite_mode_statusbar),
			  view);

	g_signal_connect (doc,
			  "readonly_changed",
			  G_CALLBACK (gedit_view_doc_readonly_changed_handler),
			  view);

	gtk_text_view_set_editable (GTK_TEXT_VIEW (view->priv->text_view), 
				    !gedit_document_is_readonly (doc));	
	
	gedit_debug (DEBUG_VIEW, "END: %d", G_OBJECT (view)->ref_count);

	return view;
}

void
gedit_view_cut_clipboard (GeditView *view)
{
	GtkTextBuffer* buffer = NULL;

	gedit_debug (DEBUG_VIEW, "");

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view->priv->text_view));
	g_return_if_fail (buffer != NULL);
	g_return_if_fail (view->priv->document != NULL);

	/* FIXME: what is default editability of a buffer? */
  	gtk_text_buffer_cut_clipboard (buffer,
				gtk_clipboard_get (GDK_NONE),
				!gedit_document_is_readonly (view->priv->document));
  	
	gtk_text_view_scroll_mark_onscreen (GTK_TEXT_VIEW (view->priv->text_view),
				gtk_text_buffer_get_mark (buffer,
				"insert"));
}

void
gedit_view_copy_clipboard (GeditView *view)
{
	GtkTextBuffer* buffer = NULL;

	gedit_debug (DEBUG_VIEW, "");

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view->priv->text_view));
	g_return_if_fail (buffer != NULL);
	
  	gtk_text_buffer_copy_clipboard (buffer,
				gtk_clipboard_get (GDK_NONE));
  	
	gtk_text_view_scroll_mark_onscreen (GTK_TEXT_VIEW (view->priv->text_view),
				gtk_text_buffer_get_mark (buffer,
				"insert"));
}

void
gedit_view_paste_clipboard (GeditView *view)
{
  	GtkTextBuffer* buffer = NULL;

	gedit_debug (DEBUG_VIEW, "");

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view->priv->text_view));
	g_return_if_fail (buffer != NULL);
	g_return_if_fail (view->priv->document != NULL);

	/* FIXME: what is default editability of a buffer? */
  	gtk_text_buffer_paste_clipboard (buffer,
				gtk_clipboard_get (GDK_NONE),
				NULL,
				!gedit_document_is_readonly (view->priv->document));
  	
	gtk_text_view_scroll_mark_onscreen (GTK_TEXT_VIEW (view->priv->text_view),
				gtk_text_buffer_get_mark (buffer,
				"insert"));
}

void
gedit_view_delete_selection (GeditView *view)
{
  	GtkTextBuffer* buffer = NULL;

	gedit_debug (DEBUG_VIEW, "");

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view->priv->text_view));
	g_return_if_fail (buffer != NULL);
	g_return_if_fail (view->priv->document != NULL);

	/* FIXME: what is default editability of a buffer? */
	gtk_text_buffer_delete_selection (buffer, 
				TRUE,
				!gedit_document_is_readonly (view->priv->document));
  	
	gtk_text_view_scroll_mark_onscreen (GTK_TEXT_VIEW (view->priv->text_view),
				gtk_text_buffer_get_mark (buffer,
				"insert"));
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
	
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view->priv->text_view));
	g_return_if_fail (buffer != NULL);

	gtk_text_view_scroll_mark_onscreen (GTK_TEXT_VIEW (view->priv->text_view),
				gtk_text_buffer_get_mark (buffer,
				"insert"));
}

/* assign a unique name */
static G_CONST_RETURN gchar *
get_widget_name (GtkWidget *w)
{
	const gchar *name;	

	name = gtk_widget_get_name (w);
	g_return_val_if_fail (name != NULL, NULL);

	if (strcmp (name, g_type_name (GTK_WIDGET_TYPE (w))) == 0)
	{
		static guint d = 0;
		gchar *n;

		n = g_strdup_printf ("%s_%u_%u", name, d, (guint) g_random_int);
		d++;

		gtk_widget_set_name (w, n);
		g_free (n);

		name = gtk_widget_get_name (w);
	}

	return name;
}

/* There is no clean way to set the cursor-color, so we are stuck
 * with the following hack: set the name of each widget and parse
 * a gtkrc string.
 */
static void 
modify_cursor_color (GtkWidget *textview, 
		     GdkColor  *color)
{
	static const char cursor_color_rc[] =
		"style \"svs-cc\"\n"
		"{\n"
			"GtkSourceView::cursor-color=\"#%04x%04x%04x\"\n"
		"}\n"
		"widget \"*.%s\" style : application \"svs-cc\"\n";

	const gchar *name;
	gchar *rc_temp;

	gedit_debug (DEBUG_VIEW, "");

	name = get_widget_name (textview);
	g_return_if_fail (name != NULL);

	if (color != NULL)
	{
		rc_temp = g_strdup_printf (cursor_color_rc,
					   color->red, 
					   color->green, 
					   color->blue,
					   name);
	}
	else
	{
		GtkRcStyle *rc_style;

 		rc_style = gtk_widget_get_modifier_style (textview);

		rc_temp = g_strdup_printf (cursor_color_rc,
					   rc_style->text [GTK_STATE_NORMAL].red,
					   rc_style->text [GTK_STATE_NORMAL].green,
					   rc_style->text [GTK_STATE_NORMAL].blue,
					   name);
	}

	gtk_rc_parse_string (rc_temp);
	gtk_widget_reset_rc_styles (textview);

	g_free (rc_temp);
}

void
gedit_view_set_colors (GeditView *view,
		       gboolean   def,
		       GdkColor  *backgroud,
		       GdkColor  *text,
		       GdkColor  *selection,
		       GdkColor  *sel_text)
{
	gedit_debug (DEBUG_VIEW, "");

	g_return_if_fail (GEDIT_IS_VIEW (view));

	/* just a bit of paranoia */
	gtk_widget_ensure_style (GTK_WIDGET (view->priv->text_view));

	if (!def)
	{
		if (backgroud != NULL)
			gtk_widget_modify_base (GTK_WIDGET (view->priv->text_view), 
						GTK_STATE_NORMAL, backgroud);

		if (selection != NULL)
		{
			gtk_widget_modify_base (GTK_WIDGET (view->priv->text_view), 
						GTK_STATE_SELECTED, selection);

			gtk_widget_modify_base (GTK_WIDGET (view->priv->text_view), 
						GTK_STATE_ACTIVE, selection);
		}

		if (sel_text != NULL)
		{
			gtk_widget_modify_text (GTK_WIDGET (view->priv->text_view), 
						GTK_STATE_SELECTED, sel_text);		

			gtk_widget_modify_text (GTK_WIDGET (view->priv->text_view), 
						GTK_STATE_ACTIVE, sel_text);		
		}

		if (text != NULL)
		{
			gtk_widget_modify_text (GTK_WIDGET (view->priv->text_view), 
						GTK_STATE_NORMAL, text);
			modify_cursor_color (GTK_WIDGET (view->priv->text_view), text);
		}
	}
	else
	{
		GtkRcStyle *rc_style;

		rc_style = gtk_widget_get_modifier_style (GTK_WIDGET (view->priv->text_view));

		rc_style->color_flags [GTK_STATE_NORMAL] = 0;
		rc_style->color_flags [GTK_STATE_SELECTED] = 0;
		rc_style->color_flags [GTK_STATE_ACTIVE] = 0;

		gtk_widget_modify_style (GTK_WIDGET (view->priv->text_view), rc_style);

		/* It must be called after the text color has been modified */
		modify_cursor_color (GTK_WIDGET (view->priv->text_view), NULL);
	}
}

void
gedit_view_set_font (GeditView* view, gboolean def, const gchar* font_name)
{
	gedit_debug (DEBUG_VIEW, "");

	g_return_if_fail (GEDIT_IS_VIEW (view));

	if (!def)
	{
		PangoFontDescription *font_desc = NULL;

		g_return_if_fail (font_name != NULL);
		
		font_desc = pango_font_description_from_string (font_name);
		g_return_if_fail (font_desc != NULL);

		gtk_widget_modify_font (GTK_WIDGET (view->priv->text_view), font_desc);
		
		pango_font_description_free (font_desc);		
	}
	else
	{
		GtkRcStyle *rc_style;

		rc_style = gtk_widget_get_modifier_style (GTK_WIDGET (view->priv->text_view));

		if (rc_style->font_desc)
			pango_font_description_free (rc_style->font_desc);

		rc_style->font_desc = NULL;
		
		gtk_widget_modify_style (GTK_WIDGET (view->priv->text_view), rc_style);
	}
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
	gedit_debug (DEBUG_VIEW, "Tab size: %d", tab_size);

	g_return_if_fail (GEDIT_IS_VIEW (view));

	g_object_set (G_OBJECT (view->priv->text_view), 
		      "tabs_width", tab_size,
		      NULL);
}

void
gedit_view_show_line_numbers (GeditView* view, gboolean visible)
{
	gedit_debug (DEBUG_VIEW, "");

	g_object_set (G_OBJECT (view->priv->text_view), 
		      "show_line_numbers", visible,
		      NULL);
}

void
gedit_view_set_cursor_position_statusbar (GeditView *view, GtkWidget* status)
{
	gedit_debug (DEBUG_VIEW, "");

	g_return_if_fail (GEDIT_IS_VIEW (view));
			
	view->priv->cursor_position_statusbar = status;

	if ((status != NULL) && (view->priv->document != NULL))
		gedit_view_update_cursor_position_statusbar
			(GTK_TEXT_BUFFER (view->priv->document),		
			 view);
}

void
gedit_view_set_overwrite_mode_statusbar (GeditView *view, GtkWidget* status)
{
	gedit_debug (DEBUG_VIEW, "");

	g_return_if_fail (GEDIT_IS_VIEW (view));
	
	view->priv->overwrite_mode_statusbar = status;

	view->priv->overwrite_mode = !GTK_TEXT_VIEW (view->priv->text_view)->overwrite_mode;

	if (status != NULL)
		gedit_view_update_overwrite_mode_statusbar (GTK_TEXT_VIEW (view->priv->text_view), 
							    view);
}
			
static void
gedit_view_update_cursor_position_statusbar (GtkTextBuffer *buffer, GeditView* view)
{
	gchar *msg;
	guint row, col/*, chars*/;
	GtkTextIter iter;
	GtkTextIter start;
	guint tab_size;

	gedit_debug (DEBUG_VIEW, "");
  
	if (view->priv->cursor_position_statusbar == NULL)
		return;
	
	/* clear any previous message, underflow is allowed */
	gtk_statusbar_pop (GTK_STATUSBAR (view->priv->cursor_position_statusbar), 0); 
	
	gtk_text_buffer_get_iter_at_mark (buffer,
					  &iter,
					  gtk_text_buffer_get_insert (buffer));
	
	row = gtk_text_iter_get_line (&iter);
	
	/*
	chars = gtk_text_iter_get_line_offset (&iter);
	*/
	
	start = iter;
	gtk_text_iter_set_line_offset (&start, 0);
	col = 0;

	tab_size = gtk_source_view_get_tabs_width (
					GTK_SOURCE_VIEW (view->priv->text_view));

	while (!gtk_text_iter_equal (&start, &iter))
	{
		if (gtk_text_iter_get_char (&start) == '\t')
					
			col += (tab_size - (col  % tab_size));
		else
			++col;

		gtk_text_iter_forward_char (&start);
	}
	
	/*
	if (col == chars)
		msg = g_strdup_printf (_("  Ln %d, Col %d"), row + 1, col + 1);
	else
		msg = g_strdup_printf (_("  Ln %d, Col %d-%d"), row + 1, chars + 1, col + 1);
	*/

	/* Translators: "Ln" is an abbreviation for "Line", Col is an abbreviation for "Column". Please,
	use abbreviations if possible to avoid space problems. */
	msg = g_strdup_printf (_("  Ln %d, Col %d"), row + 1, col + 1);
	
	gtk_statusbar_push (GTK_STATUSBAR (view->priv->cursor_position_statusbar), 
			    0, msg);

      	g_free (msg);
}
	
static void
gedit_view_cursor_moved (GtkTextBuffer     *buffer,
			 const GtkTextIter *new_location,
			 GtkTextMark       *mark,
			 GeditView         *view)
{
	gedit_debug (DEBUG_VIEW, "");

	if (mark == gtk_text_buffer_get_insert (buffer))
		gedit_view_update_cursor_position_statusbar (buffer, view);
}

static void
gedit_view_update_overwrite_mode_statusbar (GtkTextView* w, GeditView* view)
{
	gchar *msg;
	
	gedit_debug (DEBUG_VIEW, "");
  
	view->priv->overwrite_mode = !view->priv->overwrite_mode;
	
	if (view->priv->overwrite_mode_statusbar == NULL)
		return;
	
	/* clear any previous message, underflow is allowed */
	gtk_statusbar_pop (GTK_STATUSBAR (view->priv->overwrite_mode_statusbar), 0); 
	
	if (view->priv->overwrite_mode)
		msg = g_strdup (_("  OVR"));
	else
		msg = g_strdup (_("  INS"));

	gtk_statusbar_push (GTK_STATUSBAR (view->priv->overwrite_mode_statusbar), 
			    0, msg);

      	g_free (msg);
}

static void
gedit_view_populate_popup (GtkTextView *textview, GtkMenu *menu, gpointer view) 
{
	g_signal_emit (G_OBJECT (view), signals[POPULATE_POPUP], 0, menu);
}

GtkTextView *
gedit_view_get_gtk_text_view (const GeditView *view)
{
	gedit_debug (DEBUG_VIEW, "");

	g_return_val_if_fail (GEDIT_IS_VIEW (view), NULL);

	return GTK_TEXT_VIEW (view->priv->text_view);
}

void
gedit_view_set_auto_indent (GeditView *view, gboolean enable)
{
	gedit_debug (DEBUG_VIEW, "");

	g_return_if_fail (GEDIT_IS_VIEW (view));

	gtk_source_view_set_auto_indent (view->priv->text_view,
					 enable);
}

void
gedit_view_set_insert_spaces_instead_of_tabs (GeditView *view, gboolean enable)
{
	gedit_debug (DEBUG_VIEW, "");

	g_return_if_fail (GEDIT_IS_VIEW (view));

	gtk_source_view_set_insert_spaces_instead_of_tabs (view->priv->text_view,
							   enable);
}
