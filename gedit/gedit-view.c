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

#include <libgnome/gnome-i18n.h>
#include <gdk/gdkkeysyms.h>

#include "gedit-view.h"
#include "gedit-debug.h"
#include "gedit-menus.h"
#include "gedit-prefs-manager-app.h"

#include "gedit-marshal.h"

#define MIN_NUMBER_WINDOW_WIDTH 20

struct _GeditViewPrivate
{
	GtkTextView *text_view;	

	GeditDocument *document;

	gboolean line_numbers_visible;

	GtkWidget *cursor_position_statusbar;
	GtkWidget *overwrite_mode_statusbar;

	gboolean overwrite_mode;

	gint tab_size;

	gint old_lines;
};


/* Implement DnD for application/x-color drops */
typedef enum {
	TARGET_COLOR = 200
} GeditViewDropTypes;

static GtkTargetEntry drop_types[] = {
	{"application/x-color", 0, TARGET_COLOR}
};

static gint n_drop_types = sizeof (drop_types) / sizeof (drop_types[0]);

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
					 gpointer           data);
static void gedit_view_update_overwrite_mode_statusbar (GtkTextView* w, GeditView* view);
static void gedit_view_doc_readonly_changed_handler (GeditDocument *document, 
						     gboolean readonly, 
						     GeditView *view);
static gint gedit_view_calculate_real_tab_width (GeditView *view, gint tab_size);

static void gedit_view_populate_popup (GtkTextView *textview, GtkMenu *menu, gpointer data);
static void gedit_view_undo_activate_callback (GtkWidget *menu_item, GeditView *view);
static void gedit_view_redo_activate_callback (GtkWidget *menu_item, GeditView *view);

static void gedit_view_dnd_drop (GtkTextView *view, 
			     GdkDragContext *context,
			     gint x,
			     gint y,
			     GtkSelectionData *selection_data,
			     guint info,
			     guint time,
			     gpointer data);
static gint gedit_view_key_press_cb (GtkWidget *widget, GdkEventKey *event, GeditView *view);

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

static void
gedit_view_doc_readonly_changed_handler (GeditDocument *document, gboolean readonly,
		GeditView *view)
{
	gedit_debug (DEBUG_VIEW, "");

	g_return_if_fail (GEDIT_IS_VIEW (view));

	gtk_text_view_set_editable (view->priv->text_view, !readonly);	
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

/* This function is taken from gtk+/tests/testtext.c */
static void
gedit_view_get_lines (GtkTextView  *text_view,
		      gint          first_y,
		      gint          last_y,
		      GArray       *buffer_coords,
		      GArray       *numbers,
		      gint         *countp)
{
	GtkTextIter iter;
	gint count;
	gint size;
      	gint last_line_num;	

	gedit_debug (DEBUG_VIEW, "");

	g_array_set_size (buffer_coords, 0);
	g_array_set_size (numbers, 0);
  
	/* Get iter at first y */
	gtk_text_view_get_line_at_y (text_view, &iter, first_y, NULL);

	/* For each iter, get its location and add it to the arrays.
	 * Stop when we pass last_y
	*/
	count = 0;
  	size = 0;

  	while (!gtk_text_iter_is_end (&iter))
    	{
		gint y, height;
      
		gtk_text_view_get_line_yrange (text_view, &iter, &y, &height);

		g_array_append_val (buffer_coords, y);
		last_line_num = gtk_text_iter_get_line (&iter);
		g_array_append_val (numbers, last_line_num);
      	
		++count;

		if ((y + height) > last_y)
			break;
      
		gtk_text_iter_forward_line (&iter);
	}

	if (gtk_text_iter_is_end (&iter))
    	{
		gint y, height;
		gint line_num;
      
		gtk_text_view_get_line_yrange (text_view, &iter, &y, &height);

		line_num = gtk_text_iter_get_line (&iter);

		if (line_num != last_line_num)
		{
			g_array_append_val (buffer_coords, y);
			g_array_append_val (numbers, line_num);
			++count;
		}
	}

	*countp = count;
}

/* The original version of this function 
 * is taken from gtk+/tests/testtext.c 
 */
static gint
gedit_view_line_numbers_expose (GtkWidget      *widget,
			        GdkEventExpose *event,
			        GeditDocument  *doc)
{
	gint count;
	GArray *numbers;
	GArray *pixels;
	gint first_y;
	gint last_y;
	gint i;
	GdkWindow *left_win;
	GdkWindow *right_win;
	PangoLayout *layout;
	GtkTextView *text_view;
	GtkTextWindowType type;
	GdkDrawable *target;
  	gint layout_width;
	gchar *str;

	/* It is annoying -- Paolo
	 * gedit_debug (DEBUG_VIEW, "");
	 */
	text_view = GTK_TEXT_VIEW (widget);
  
	/* See if this expose is on the line numbers window */
	left_win = gtk_text_view_get_window (text_view,
        				     GTK_TEXT_WINDOW_LEFT);
	right_win = gtk_text_view_get_window (text_view,
					      GTK_TEXT_WINDOW_RIGHT);

	if (event->window == left_win)
	{
		type = GTK_TEXT_WINDOW_LEFT;
		target = left_win;
  	}
  	else if (event->window == right_win)
    	{
      		type = GTK_TEXT_WINDOW_RIGHT;
      		target = right_win;
    	}
  	else
		return FALSE;
  
	first_y = event->area.y;
	last_y = first_y + event->area.height;

	gtk_text_view_window_to_buffer_coords (text_view,
					       type,
					       0,
					       first_y,
					       NULL,
					       &first_y);

	gtk_text_view_window_to_buffer_coords (text_view,
					       type,
					       0,
					       last_y,
					       NULL,
					       &last_y);

	numbers = g_array_new (FALSE, FALSE, sizeof (gint));
	pixels = g_array_new (FALSE, FALSE, sizeof (gint));
  
	gedit_view_get_lines (text_view,
			      first_y,
			      last_y,
			      pixels,
			      numbers,
			      &count);
	
	/* a zero-lined document should display a "1"; we don't need to worry about
	scrolling effects of the text widget in this special case */
	
	if (count == 0)
	{
		gint y = 0;
		gint n = 0;
		count = 1;
		g_array_append_val (pixels, y);
		g_array_append_val (numbers, n);
	}
  
	layout = gtk_widget_create_pango_layout (widget, "");
  
	/* Set size */
	str = g_strdup_printf ("%d", MAX (99, gedit_document_get_line_count (doc)));
	pango_layout_set_text (layout, str, -1);
	g_free (str);

	pango_layout_get_pixel_size (layout, &layout_width, NULL);

	gtk_text_view_set_border_window_size (GTK_TEXT_VIEW (text_view),
                                        GTK_TEXT_WINDOW_LEFT,
        				layout_width + 4);
	
	/* Draw fully internationalized numbers! */	

	i = 0;
	while (i < count)
	{
		gint pos;
      
		gtk_text_view_buffer_to_window_coords (text_view,
						       type,
						       0,
						       g_array_index (pixels, gint, i),
						       NULL,
						       &pos);

		str = g_strdup_printf ("%d", g_array_index (numbers, gint, i) + 1);
		
		pango_layout_set_text (layout, str, -1);

		gtk_paint_layout (widget->style,
				  target,
				  GTK_WIDGET_STATE (widget),
				  FALSE,
				  NULL,
				  widget,
				  NULL,
				  2, pos,
				  layout);

		g_free (str);
      
		++i;
	}

	g_array_free (pixels, TRUE);
	g_array_free (numbers, TRUE);
  
	g_object_unref (G_OBJECT (layout));

	/* don't stop emission, need to draw children */
	return FALSE;
}

/* when the user right-clicks on a word, we move the cursor to the 
 * location of the clicked-upon word.
 */
static gboolean
button_press_event (GtkTextView *view, GdkEventButton *event, gpointer data) 
{
	if (event->button == 3) 
	{
		gint x, y;
		GtkTextIter iter;
		GtkTextIter start, end;

		gtk_text_view_window_to_buffer_coords (view, 
				GTK_TEXT_WINDOW_TEXT, 
				event->x, event->y,
				&x, &y);
		
		gtk_text_view_get_iter_at_location(view, &iter, x, y);

		if (!(gtk_text_buffer_get_selection_bounds (
					gtk_text_view_get_buffer (view), &start, &end) &&          
		    gtk_text_iter_in_range (&iter, &start, &end)))
			gtk_text_buffer_place_cursor (gtk_text_view_get_buffer (view), &iter);
	}
	return FALSE; /* false: let gtk process this event, too.
			 we don't want to eat any events. */
}

static void 
gedit_view_init (GeditView  *view)
{
	GtkTextView *text_view;
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

	text_view = GTK_TEXT_VIEW (gtk_text_view_new ());
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
		
	g_object_set (G_OBJECT (view->priv->text_view), "cursor_visible", TRUE, NULL);
	
	gtk_box_pack_start (GTK_BOX (view), sw, TRUE, TRUE, 0);
	gtk_container_add (GTK_CONTAINER (sw), GTK_WIDGET (view->priv->text_view));
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw),
                                             GTK_SHADOW_IN);

	gtk_text_view_set_left_margin (GTK_TEXT_VIEW (view->priv->text_view), 2);
	gtk_text_view_set_right_margin (GTK_TEXT_VIEW (view->priv->text_view), 2);

	/*
	GTK_WIDGET_SET_FLAGS (GTK_WIDGET (view), GTK_CAN_FOCUS);
	GTK_WIDGET_SET_FLAGS (GTK_WIDGET (view->priv->text_view), GTK_CAN_FOCUS);
	*/

	g_signal_connect (G_OBJECT (view->priv->text_view), "populate-popup",
			  G_CALLBACK (gedit_view_populate_popup), view);

	g_signal_connect (G_OBJECT (view->priv->text_view), "button-press-event",
			  G_CALLBACK (button_press_event), NULL);
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

	G_OBJECT_CLASS (parent_class)->finalize (object);

	g_free (view->priv);

	gedit_debug (DEBUG_VIEW, "END");
}

static void
gedit_view_realize_cb (GtkWidget *widget, GeditView *view)
{
	gedit_debug (DEBUG_VIEW, "");
			
	g_return_if_fail (GEDIT_IS_VIEW (view));
			
	/* Set tab size: this function must be called after the widget is
	 * realized */
	gedit_view_set_tab_size (view, 
				 gedit_prefs_manager_get_tabs_size ());
}

static void
gedit_view_doc_changed_handler (GtkTextBuffer *buffer, GeditView* view)
{
	gint lines;

	lines = gtk_text_buffer_get_line_count (buffer);

	if (view->priv->old_lines != lines)
	{
		GdkWindow *w;
		view->priv->old_lines = lines;

		w = gtk_text_view_get_window (view->priv->text_view, GTK_TEXT_WINDOW_LEFT);

		if (w != NULL)
			gdk_window_invalidate_rect (w, NULL, FALSE);
	}
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
	GtkTargetList *tl;
	
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

	if (gedit_prefs_manager_get_display_line_numbers ())
		gedit_view_show_line_numbers (view, TRUE);
			
	gtk_widget_show_all (GTK_WIDGET (view));

	tl = gtk_drag_dest_get_target_list (GTK_WIDGET (view->priv->text_view));
	g_return_val_if_fail (tl != NULL, view);

	gtk_target_list_add_table (tl, drop_types, n_drop_types);
	
	g_signal_connect (G_OBJECT (view->priv->text_view), 
			  "drag_data_received", 
			  G_CALLBACK (gedit_view_dnd_drop), 
			  doc);

	g_signal_connect (GTK_TEXT_BUFFER (doc), 
			  "changed",
			  G_CALLBACK (gedit_view_update_cursor_position_statusbar),
			  view);

	g_signal_connect (GTK_TEXT_BUFFER (doc), 
			  "changed",
			  G_CALLBACK (gedit_view_doc_changed_handler),
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

	g_signal_connect (G_OBJECT (view->priv->text_view),
			  "key_press_event",
			  G_CALLBACK (gedit_view_key_press_cb),
			  view);

	g_signal_connect (G_OBJECT (view->priv->text_view),
			  "realize",
			  G_CALLBACK (gedit_view_realize_cb),
			  view);

	gtk_text_view_set_editable (view->priv->text_view, 
				    !gedit_document_is_readonly (doc));	
	
	gedit_debug (DEBUG_VIEW, "END: %d", G_OBJECT (view)->ref_count);

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
gedit_view_set_colors (GeditView* view, gboolean def, GdkColor* backgroud, GdkColor* text,
		GdkColor* selection, GdkColor* sel_text)
{
	gedit_debug (DEBUG_VIEW, "");

	g_return_if_fail (GEDIT_IS_VIEW (view));

	if (!def)
	{	
		if (backgroud != NULL)
			gtk_widget_modify_base (GTK_WIDGET (view->priv->text_view), 
						GTK_STATE_NORMAL, backgroud);

		if (text != NULL)			
			gtk_widget_modify_text (GTK_WIDGET (view->priv->text_view), 
						GTK_STATE_NORMAL, text);
	
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
	}
	else
	{
		GtkRcStyle *rc_style;

		rc_style = gtk_widget_get_modifier_style (GTK_WIDGET (view->priv->text_view));

		rc_style->color_flags [GTK_STATE_NORMAL] = 0;
		rc_style->color_flags [GTK_STATE_SELECTED] = 0;
		rc_style->color_flags [GTK_STATE_ACTIVE] = 0;

		gtk_widget_modify_style (GTK_WIDGET (view->priv->text_view), rc_style);
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

/* This function is taken from gtksourceview
 * 
 * Copyright (C) 2001
 * Mikael Hermansson<tyan@linux.se>
 * Chris Phelps <chicane@reninet.com>
 */

static gint
gedit_view_calculate_real_tab_width (GeditView *view, gint tab_size)
{
	PangoLayout *layout;
	gchar *tab_string;
	gint counter = 0;
	gint tab_width = 0;

	gedit_debug (DEBUG_VIEW, "");

	g_return_val_if_fail (GEDIT_IS_VIEW (view), -1);

	if (tab_size == 0)
		return -1;

	tab_string = g_malloc (tab_size + 1);

	while (counter < tab_size) {
		tab_string [counter] = ' ';
		counter++;
	}

	tab_string [tab_size] = 0;

	layout = gtk_widget_create_pango_layout (
			GTK_WIDGET (view->priv->text_view), 
			tab_string);
	g_free (tab_string);

	if (layout != NULL) {
		pango_layout_get_pixel_size (layout, &tab_width, NULL);
		g_object_unref (G_OBJECT (layout));
	} else
		tab_width = -1;

	gedit_debug (DEBUG_VIEW, "Tab width: %d", tab_width);

	return tab_width;
}

/* This function must be called after the widget is
 * realized 
 */
void
gedit_view_set_tab_size (GeditView* view, gint tab_size)
{
	PangoTabArray *tab_array;
	gint real_tab_width;

	gedit_debug (DEBUG_VIEW, "Tab size: %d", tab_size);

	g_return_if_fail (GEDIT_IS_VIEW (view));

	if (view->priv->tab_size == tab_size)
		return;

	real_tab_width = gedit_view_calculate_real_tab_width (
					 GEDIT_VIEW (view),
					 tab_size);

	if (real_tab_width < 0)
	{
		g_warning ("Impossible to set tab width.");
		return;
	}
	
	tab_array = pango_tab_array_new (1, TRUE);
	pango_tab_array_set_tab (tab_array, 0, PANGO_TAB_LEFT, real_tab_width);

	gtk_text_view_set_tabs (GTK_TEXT_VIEW (view->priv->text_view), 
				tab_array);

	pango_tab_array_free (tab_array);

	view->priv->tab_size = tab_size;
	gedit_view_update_cursor_position_statusbar (
			GTK_TEXT_BUFFER (gedit_view_get_document (view)),
			view);	
}

void
gedit_view_show_line_numbers (GeditView* view, gboolean visible)
{
	gedit_debug (DEBUG_VIEW, "");

	if (visible)
	{
		if (!view->priv->line_numbers_visible)
		{
			gedit_debug (DEBUG_VIEW, "Show line numbers");

			gtk_text_view_set_border_window_size (
					GTK_TEXT_VIEW (view->priv->text_view),
                                        GTK_TEXT_WINDOW_LEFT,
        				MIN_NUMBER_WINDOW_WIDTH);

			g_signal_connect (
				G_OBJECT (view->priv->text_view),
                      		"expose_event",
                      		G_CALLBACK (gedit_view_line_numbers_expose),
                      		view->priv->document);

			view->priv->line_numbers_visible = visible;
		}
	}
	else
		if (view->priv->line_numbers_visible)
		{
			gedit_debug (DEBUG_VIEW, "Hide line numbers");

			gtk_text_view_set_border_window_size (
					GTK_TEXT_VIEW (view->priv->text_view),
                                        GTK_TEXT_WINDOW_LEFT,
        				0);

			g_signal_handlers_disconnect_by_func (
				G_OBJECT (view->priv->text_view),
				G_CALLBACK (gedit_view_line_numbers_expose),
                      		view->priv->document);

			view->priv->line_numbers_visible = visible;
		}
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
		gedit_view_update_overwrite_mode_statusbar (view->priv->text_view, view);
}
			
static void
gedit_view_update_cursor_position_statusbar (GtkTextBuffer *buffer, GeditView* view)
{
	gchar *msg;
	gint row, col/*, chars*/;
	GtkTextIter iter;
	GtkTextIter start;
	
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

	while (!gtk_text_iter_equal (&start, &iter))
	{
		if (gtk_text_iter_get_char (&start) == '\t')
		{
			col += (view->priv->tab_size - (col  % view->priv->tab_size));
		}
		else
			++col;

		gtk_text_iter_forward_char (&start);
	}
	
	/*
	if (col == chars)
		msg = g_strdup_printf (_("  Ln %d, Col. %d"), row + 1, col + 1);
	else
		msg = g_strdup_printf (_("  Ln %d, Col. %d-%d"), row + 1, chars + 1, col + 1);
	*/

	msg = g_strdup_printf (_("  Ln %d, Col. %d"), row + 1, col + 1);
	
	gtk_statusbar_push (GTK_STATUSBAR (view->priv->cursor_position_statusbar), 
			    0, msg);

      	g_free (msg);
}
	
static void
gedit_view_cursor_moved (GtkTextBuffer     *buffer,
			 const GtkTextIter *new_location,
			 GtkTextMark       *mark,
			 gpointer           data)
{
	GeditView* view;

	gedit_debug (DEBUG_VIEW, "");

	view = GEDIT_VIEW (data);
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
gedit_view_undo_activate_callback (GtkWidget *menu_item, GeditView *view)
{
	GeditDocument *doc;
	
	g_return_if_fail (view != NULL);
	g_return_if_fail (GEDIT_IS_VIEW (view));

	doc = gedit_view_get_document (view);

	g_return_if_fail (doc != NULL);
	g_return_if_fail (GEDIT_IS_DOCUMENT (doc));

	if (gedit_document_can_undo (doc))
	{
		gedit_document_undo (doc);
		gedit_view_scroll_to_cursor (view);
	}

	gtk_widget_grab_focus (GTK_WIDGET (view));
}

static void
gedit_view_redo_activate_callback (GtkWidget *menu_item, GeditView *view)
{
	GeditDocument *doc;

	g_return_if_fail (view != NULL);
	g_return_if_fail (GEDIT_IS_VIEW (view));

	doc = gedit_view_get_document (view);
	
	g_return_if_fail (doc != NULL);
	g_return_if_fail (GEDIT_IS_DOCUMENT (doc));

	if (gedit_document_can_redo (doc))
	{
		gedit_document_redo (doc);
		gedit_view_scroll_to_cursor (view);
	}

	gtk_widget_grab_focus (GTK_WIDGET (view));
}

static void
gedit_view_populate_popup (GtkTextView *textview, GtkMenu *menu, gpointer data) 
{
	GeditView *view;
	GeditDocument *doc;
	GtkWidget *menu_item;

	g_return_if_fail (menu != NULL);
	g_return_if_fail (GTK_IS_MENU_SHELL (menu));
	g_return_if_fail (data != NULL);
	g_return_if_fail (GEDIT_IS_VIEW (data));

	view = GEDIT_VIEW (data);
	
	doc = gedit_view_get_document (view);
	g_return_if_fail (doc != NULL);

	/* Add the separator */
	menu_item = gtk_separator_menu_item_new ();
	gtk_widget_show (menu_item);
	gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), menu_item);

	/* Add the redo button */
	menu_item = gtk_image_menu_item_new_from_stock (GTK_STOCK_REDO, NULL);
	gtk_widget_set_sensitive (menu_item, gedit_document_can_redo (doc));
	gtk_widget_show (menu_item);
	g_signal_connect (G_OBJECT (menu_item), "activate",
		      	  G_CALLBACK (gedit_view_redo_activate_callback), view);
	gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), menu_item);

	/* Add the undo button */
	menu_item = gtk_image_menu_item_new_from_stock (GTK_STOCK_UNDO, NULL);
	gtk_widget_set_sensitive (menu_item, gedit_document_can_undo (doc));
	g_signal_connect (G_OBJECT (menu_item), "activate",
		      	  G_CALLBACK (gedit_view_undo_activate_callback), view);
	gtk_widget_show (menu_item);
	gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), menu_item);

	g_signal_emit (G_OBJECT (view), signals[POPULATE_POPUP], 0, menu);
}

static void 
gedit_view_dnd_drop (GtkTextView *view, 
		     GdkDragContext *context,
		     gint x,
		     gint y,
		     GtkSelectionData *selection_data,
		     guint info,
		     guint time,
		     gpointer data)
{

	GtkTextIter iter;

	gedit_debug (DEBUG_VIEW, "");

	if (info == TARGET_COLOR) 
	{
		guint16 *vals;
		gchar string[] = "#000000";
		
		if (selection_data->length < 0)
			return;

		if ((selection_data->format != 16) || (selection_data->length != 8)) 
		{
			g_warning ("Received invalid color data\n");
			return;
		}

		vals = (guint16 *) selection_data->data;

		vals[0] /= 256;
	        vals[1] /= 256;
		vals[2] /= 256;
		
		g_snprintf (string, sizeof (string), "#%02X%02X%02X", vals[0], vals[1], vals[2]);
		
		gtk_text_view_get_iter_at_location (view, &iter, x, y);

		if (!gedit_document_is_readonly (GEDIT_DOCUMENT (data)))
			gtk_text_buffer_insert (GTK_TEXT_BUFFER (view->buffer), &iter, string, strlen (string));

		/*
		 * FIXME: Check if the iter is inside a selection
		 * If it is, remove the selection and then insert at
		 * the cursor position
		 */

		return;
	}
}

GtkTextView *
gedit_view_get_gtk_text_view (const GeditView *view)
{
	gedit_debug (DEBUG_VIEW, "");

	g_return_val_if_fail (GEDIT_IS_VIEW (view), NULL);

	return view->priv->text_view;
}

static gchar*
compute_indentation (GeditView *view, gint line)
{
	GtkTextIter start;
	GtkTextIter end;
	gunichar ch;

	gedit_debug (DEBUG_VIEW, "");

	gtk_text_buffer_get_iter_at_line (GTK_TEXT_BUFFER (view->priv->document), 
			&start, line);

	end = start;

	ch = gtk_text_iter_get_char (&end);
	while (g_unichar_isspace (ch) && ch != '\n')
	{
		if (!gtk_text_iter_forward_char (&end))
			break;

		ch = gtk_text_iter_get_char (&end);
	}

	if (gtk_text_iter_equal (&start, &end))
		return NULL;

	return gtk_text_iter_get_slice (&start, &end);
}

static gint
gedit_view_key_press_cb (GtkWidget *widget, GdkEventKey *event, GeditView *view)
{
	GtkTextBuffer *buf;
	GtkTextIter cur;
	GtkTextMark *mark;
	gint key;

	buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget));

	key = event->keyval;

	mark = gtk_text_buffer_get_mark (buf, "insert");
	gtk_text_buffer_get_iter_at_mark (buf, &cur, mark);
	
	if ((key == GDK_Return) && gtk_text_iter_ends_line (&cur) && 
			gedit_prefs_manager_get_auto_indent ()) 
	{
		/* Auto-indent means that when you press ENTER at the end of a
		 * line, the new line is automatically indented at the same
		 * level as the previous line.
		 */
		gchar *indent = NULL;

		/* Calculate line indentation and create indent string. */
		indent = compute_indentation (view, gtk_text_iter_get_line (&cur));

		if (indent != NULL)
		{
			/* Insert new line and auto-indent. */
			gtk_text_buffer_begin_user_action (buf);
			gtk_text_buffer_insert (buf, &cur, "\n", 1);
			gtk_text_buffer_insert (buf, &cur, indent, strlen (indent));
			g_free (indent);
			gtk_text_buffer_end_user_action (buf);
			gtk_text_view_scroll_mark_onscreen (GTK_TEXT_VIEW (widget),
							    gtk_text_buffer_get_insert (buf));
			return TRUE;
		}
	}

	if ((key == GDK_Tab) && gedit_prefs_manager_get_insert_spaces ())
	{
		gint cur_pos;
		gint num_of_equivalent_spaces;
		gint tabs_size;
		gint i;
		gchar *spaces;

		tabs_size = gedit_prefs_manager_get_tabs_size (); 
		
		cur_pos = gtk_text_iter_get_line_offset (&cur);

		num_of_equivalent_spaces = tabs_size - (cur_pos % tabs_size); 

		spaces = g_malloc (num_of_equivalent_spaces + 1);
		
		for (i = 0; i < num_of_equivalent_spaces; i++)
			spaces [i] = ' ';
		
		spaces [num_of_equivalent_spaces] = '\0';
		
		gtk_text_buffer_begin_user_action (buf);
		gtk_text_buffer_insert (buf,  &cur, spaces, num_of_equivalent_spaces);
		gtk_text_buffer_end_user_action (buf);

		gtk_text_view_scroll_mark_onscreen (GTK_TEXT_VIEW (widget),
						    gtk_text_buffer_get_insert (buf));
		
		g_free (spaces);

		return TRUE;
	}
		
	return FALSE;
}


