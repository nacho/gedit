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

#include <libgnome/gnome-i18n.h>
#include "gedit-view.h"
#include "gedit-debug.h"
#include "gedit-menus.h"
#include "gedit-prefs.h"

#define MIN_NUMBER_WINDOW_WIDTH 20

struct _GeditViewPrivate
{
	GtkTextView *text_view;	

	GeditDocument *document;

	gboolean line_numbers_visible;

	GtkWidget *cursor_position_statusbar;
	GtkWidget *overwrite_mode_statusbar;

	gboolean overwrite_mode;
};


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

	return TRUE;
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
	if (!gedit_settings->use_default_font)
		gedit_view_set_font (view, gedit_settings->editor_font);

	if (!gedit_settings->use_default_colors)
	{
		background = gedit_settings->background_color;
		text = gedit_settings->text_color;
		selection = gedit_settings->selection_color;
		sel_text = gedit_settings->selected_text_color;

		gedit_view_set_colors (view, 
				&background, &text, &selection, &sel_text);
	}	

	gedit_view_set_wrap_mode (view, gedit_settings->wrap_mode);

	/* FIXME: uncomment when gedit_view_set_tab_size will work
	gedit_view_set_tab_size (view, 8);
	*/
	g_object_set (G_OBJECT (view->priv->text_view), "cursor_visible", TRUE, NULL);
	
	gtk_box_pack_start (GTK_BOX (view), sw, TRUE, TRUE, 0);
	gtk_container_add (GTK_CONTAINER (sw), GTK_WIDGET (view->priv->text_view));
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw),
                                             GTK_SHADOW_IN);

	gtk_text_view_set_left_margin (GTK_TEXT_VIEW (view->priv->text_view), 2);
	gtk_text_view_set_right_margin (GTK_TEXT_VIEW (view->priv->text_view), 2);

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

	if (view->priv->cursor_position_statusbar != NULL)
		gtk_statusbar_pop (GTK_STATUSBAR (
				   view->priv->cursor_position_statusbar), 0); 

	if (view->priv->overwrite_mode_statusbar != NULL)
		gtk_statusbar_pop (GTK_STATUSBAR (
				   view->priv->overwrite_mode_statusbar), 0); 

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

	if (gedit_settings->show_line_numbers)
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

	gtk_text_view_set_editable (view->priv->text_view, !gedit_document_is_readonly (doc));	

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

			gtk_signal_connect (GTK_OBJECT (view->priv->text_view),
                      		"expose_event",
                      		GTK_SIGNAL_FUNC (gedit_view_line_numbers_expose),
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

			gtk_signal_disconnect_by_func (GTK_OBJECT (view->priv->text_view),
				GTK_SIGNAL_FUNC (gedit_view_line_numbers_expose),
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
	gint row, col;
	GtkTextIter iter;

	gedit_debug (DEBUG_VIEW, "");
  
	if (view->priv->cursor_position_statusbar == NULL)
		return;
	
	/* clear any previous message, underflow is allowed */
	gtk_statusbar_pop (GTK_STATUSBAR (view->priv->cursor_position_statusbar), 0); 
	
	gtk_text_buffer_get_iter_at_mark (buffer,
					  &iter,
					  gtk_text_buffer_get_insert (buffer));
	
	row = gtk_text_iter_get_line (&iter);
	col = gtk_text_iter_get_line_offset (&iter);
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
