/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
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

enum {
	CURSOR_MOVED_SIGNAL,
	LAST_SIGNAL
};

static gint gedit_view_signals[LAST_SIGNAL] = { 0 };
GtkVBoxClass *parent_class = NULL;

void	gedit_view_text_changed_cb (GtkWidget *w, gpointer cbdata);
void	gedit_view_changed_cb (GnomeMDI *mdi, GtkWidget *old_view);

gfloat	gedit_view_get_window_position (View *view);
void	gedit_view_set_window_position (View *view, gfloat position);
void	gedit_view_set_window_position_from_lines (View *view, guint line, guint lines);

static void	gedit_views_insert (Document *doc, guint position, gchar * text, gint lenth, View * view_exclude);
static void	gedit_views_delete (Document *doc, guint start_pos, guint end_pos, View * view_exclude);

void		gedit_view_insert (View  *view, guint position, gchar * text, gint length);
void		gedit_view_delete (View *view, guint position, gint length);

void		doc_insert_text_cb (GtkWidget *editable, const guchar *insertion_text, int length, int *pos, View *view, gint exclude_this_view, gint undo);
void		doc_delete_text_cb (GtkWidget *editable, int start_pos, int end_pos, View *view, gint exclude_this_view, gint undo);
gboolean	auto_indent_cb (GtkWidget *text, char *insertion_text, int length, int *pos, gpointer data);

guint		gedit_view_get_type (void);
GtkWidget *	gedit_view_new (Document *doc);

void	gedit_view_set_word_wrap (View *view, gint word_wrap);
void	gedit_view_set_readonly (View *view, gint readonly);
void	gedit_view_set_font (View *view, gchar *fontname);
void	gedit_view_set_position (View *view, gint pos);
guint	gedit_view_get_position (View *view);
void	gedit_view_set_selection (View *view, guint  start, guint  end);
gint	gedit_view_get_selection (View *view, guint *start, guint *end);
void	gedit_view_add_cb (GtkWidget *widget, gpointer data);
void	gedit_view_remove_cb (GtkWidget *widget, gpointer data);
void	gedit_view_load_widgets (View *view);
void	gedit_view_set_undo (View *view, gint undo_state, gint redo_state);

static void gedit_view_update_line_indicator (void);
static gint gedit_event_button_press (GtkWidget *widget, GdkEventButton *event);
static gint gedit_event_key_press (GtkWidget *w, GdkEventKey *event);
static void gedit_view_class_init (ViewClass *klass);
static void gedit_view_init (View *view);

View *
gedit_view_active (void)
{
	View *current_view = NULL;

	gedit_debug ("", DEBUG_VIEW);

	if (mdi->active_view)
		current_view = VIEW (mdi->active_view);
 
	return current_view;
}

void
gedit_view_changed_cb (GnomeMDI *mdi, GtkWidget *old_view)
{
	View *view = gedit_view_active();
	Document *doc = view->doc;
	gint undo_state, redo_state;

	gedit_debug ("start", DEBUG_DOCUMENT);

	g_return_if_fail (view!=NULL);
		
	gtk_widget_grab_focus (view->text);
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
	gedit_debug ("end", DEBUG_DOCUMENT);
}


void
gedit_view_text_changed_cb (GtkWidget *w, gpointer cbdata)
{
	View *view;
	
	gedit_debug ("---------", DEBUG_VIEW);

	view = (View *) cbdata;
	g_return_if_fail (view != NULL);

	if (view->doc->changed)
		return;
	
	view->doc->changed = TRUE;

	/* Disconect this signal */
	gtk_signal_disconnect (GTK_OBJECT(view->text), (gint) view->view_text_changed_signal);

	/* Set the title ( so that we add the "modified" string to it )*/
	gedit_document_set_title (view->doc);
	
}


gfloat
gedit_view_get_window_position (View *view)
{
	gedit_debug ("", DEBUG_VIEW);
	return GTK_ADJUSTMENT(GTK_TEXT(view->text)->vadj)->value;
}

void
gedit_view_set_window_position (View *view, gfloat position)
{
	gedit_debug ("", DEBUG_VIEW);
	gtk_adjustment_set_value (GTK_ADJUSTMENT(GTK_TEXT(view->text)->vadj),
				  position);
}

void
gedit_view_set_window_position_from_lines (View *view, guint line, guint lines)
{
	float position;
	float upper;
	float page_increment;

	gedit_debug ("", DEBUG_VIEW);

	g_return_if_fail (line <= lines);
	
	upper = GTK_ADJUSTMENT(GTK_TEXT(view->text)->vadj)->upper;
	page_increment = GTK_ADJUSTMENT(GTK_TEXT(view->text)->vadj)->page_increment; 
	position = ( line *  upper / lines - page_increment);

	gedit_view_set_window_position (view, position);
	
}

void
gedit_views_insert (Document *doc, guint position, gchar * text, gint length, View * view_exclude)
{
	gint i;
	View *nth_view;

	gedit_debug ("", DEBUG_VIEW);

	g_return_if_fail (doc!=NULL);

	/* Why do we have to call view_text_changed_cb ? why is the "chaged" signal not calling it ?. Chema */
	if (!doc->changed)
		if (g_list_length (doc->views))
			gedit_view_text_changed_cb (NULL, (gpointer) g_list_nth_data (doc->views, 0));
	
	for (i = 0; i < g_list_length (doc->views); i++)
	{
		nth_view = g_list_nth_data (doc->views, i);
		
		if (nth_view == view_exclude)
			continue;

		gedit_view_insert (nth_view, position, text, length);
	}  
}

void
gedit_view_insert (View  *view, guint position, gchar * text, gint length)
{
	gedit_debug ("", DEBUG_VIEW);

	gtk_text_set_point (GTK_TEXT(view->text), position);
	gtk_text_insert (GTK_TEXT (view->text), NULL,
			 NULL, NULL, text,
			 length);
}

static void
gedit_views_delete (Document *doc, guint start_pos, guint end_pos, View *view_exclude)
{
	View *nth_view;
	gint n;

	gedit_debug ("", DEBUG_VIEW);

	g_return_if_fail (doc!=NULL);
	g_return_if_fail (end_pos > start_pos);

	for (n = 0; n < g_list_length (doc->views); n++)
	{
		nth_view = g_list_nth_data (doc->views, n);

		if (nth_view == view_exclude)
			continue;
		
		g_return_if_fail (nth_view!=NULL);

		gedit_view_delete (nth_view, start_pos, end_pos-start_pos);
	}
}

void
gedit_view_delete (View *view, guint position, gint length)
{
	guint p1;

	gedit_debug ("", DEBUG_VIEW);

	p1 = gtk_text_get_point (GTK_TEXT (view->text));
	gtk_text_set_point (GTK_TEXT(view->text), position);
	gtk_text_forward_delete (GTK_TEXT (view->text), length);
}

void
doc_insert_text_cb (GtkWidget *editable, const guchar *insertion_text,int length,
		    int *pos, View *view, gint exclude_this_view, gint undo)
{
	gint position = *pos;
	Document *doc;
	guchar *text_to_insert;

	gedit_debug ("start", DEBUG_VIEW);

	doc = view->doc;

	/* This string might not be terminated with a null cero */
	text_to_insert = g_new0 (guchar, length+1);
	strncpy (text_to_insert, insertion_text, length);

	if (undo)
		gedit_undo_add (text_to_insert, position, (position + length), GEDIT_UNDO_ACTION_INSERT, doc, view);
	
	if (!exclude_this_view)
		gedit_views_insert (doc, position, text_to_insert, length, NULL);
	else
		gedit_views_insert (doc, position, text_to_insert, length, view);

	g_free (text_to_insert);

	gedit_debug ("end", DEBUG_VIEW);
}


void
doc_delete_text_cb (GtkWidget *editable, int start_pos, int end_pos,
		    View *view, gint exclude_this_view, gint undo)
{
	Document *doc;
	guchar *text_to_delete;

	gedit_debug ("start", DEBUG_VIEW);

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

	gedit_debug ("end", DEBUG_VIEW);
}


/* FIXME: rewrite this function. It's a bit dirty and it is dependant on the
   text widget. Chema */
gboolean
auto_indent_cb (GtkWidget *text, char *insertion_text, int length,
		int *pos, gpointer data)
{
	int i, newlines, newline_1;
	gchar *buffer, *whitespace;
	View *view;
	Document *doc;

	gedit_debug ("", DEBUG_VIEW);

	view = (View *)data;
	g_return_val_if_fail (view != NULL, FALSE);

	if (!settings->auto_indent)
		return FALSE;
	if ((length != 1) || (insertion_text[0] != '\n'))
		return FALSE;
	if (gtk_text_get_length (GTK_TEXT (text)) <=1)
		return FALSE;

	doc = view->doc;

	newlines = 0;
	newline_1 = 0;
	
	for (i = *pos; i > 0; i--)
	{
		buffer = gtk_editable_get_chars (GTK_EDITABLE (text), i-1, i);
		
		if (buffer == NULL)
			continue;
		if (buffer[0] == 10)
		{
			if (newlines > 0)
			{
				g_free (buffer);
				buffer = NULL;
				break;
			}
			else
			{
				newlines++;
				newline_1 = i;
				g_free (buffer);
				buffer = NULL;
			}
		}
		
		g_free (buffer);
		buffer = NULL;
	}

	whitespace = g_malloc0 (newline_1 - i + 2);

	for (i = i; i <= newline_1; i++)
	{
		buffer = gtk_editable_get_chars (GTK_EDITABLE (text), i, i+1);
		
		if ((buffer[0] != 32) & (buffer[0] != 9))
		{
			g_free (buffer);
			buffer = NULL;
			break;
		}
		
		strncat (whitespace, buffer, 1);
		g_free (buffer);
		
	}

	if (strlen(whitespace) > 0)
	{
		i = *pos;
		gtk_text_set_point (GTK_TEXT (text), i);
		gtk_text_insert (GTK_TEXT (text), NULL, NULL, NULL,
				 whitespace, strlen (whitespace));
	}
	g_free (whitespace);

	return TRUE;
}

static void
gedit_view_class_init (ViewClass *klass)
{
	GtkObjectClass *object_class;
	
	object_class = (GtkObjectClass *)klass;

	gedit_debug ("", DEBUG_VIEW);
	
	gedit_view_signals[CURSOR_MOVED_SIGNAL] =
		gtk_signal_new ("cursor_moved",
				GTK_RUN_FIRST,
				object_class->type,
				GTK_SIGNAL_OFFSET (ViewClass, cursor_moved),
				gtk_signal_default_marshaller,
				GTK_TYPE_NONE,
				0);

	gtk_object_class_add_signals (object_class, gedit_view_signals, LAST_SIGNAL);
	klass->cursor_moved = NULL;

	/*
	widget_class->size_allocate = gedit_view_size_allocate;
	widget_class->size_request = gedit_view_size_request;
	widget_class->expose_event = gedit_view_expose;
	widget_class->realize = gedit_view_realize;
	object_class->finalize = gedit_view_finalize;*/
	parent_class = gtk_type_class (gtk_vbox_get_type ());
}

static void
gedit_view_init (View *view)
{
	GtkWidget *menu;
	GtkStyle *style;
	GdkColor *bg;
	GdkColor *fg;

	gedit_debug ("", DEBUG_VIEW);

	view->vbox = gtk_vbox_new (TRUE, TRUE);
	gtk_container_add (GTK_CONTAINER (view), view->vbox);
	
	/* create our paned window */
#ifdef ENABLE_SPLIT_SCREEN
	view->pane = gtk_vpaned_new ();
	gtk_box_pack_start (GTK_BOX (view->vbox), view->pane, TRUE, TRUE, 0);
	gtk_paned_set_handle_size (GTK_PANED (view->pane), 10);
	gtk_paned_set_gutter_size (GTK_PANED (view->pane), 10);

	gtk_paned_pack1 (GTK_PANED (view->pane), view->window, TRUE, TRUE);
#else
	/* Create the upper split screen */
	view->window = gtk_scrolled_window_new (NULL, NULL);
	gtk_box_pack_start (GTK_BOX (view->vbox), view->window, TRUE, TRUE, 0);
#endif	
	
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (view->window),
					GTK_POLICY_NEVER,
					GTK_POLICY_AUTOMATIC);

        /* FIXME use settings->line_wrap and add horz. scroll bars. Chema*/

	view->line_wrap = 1;

	view->text = gtk_text_new (NULL, NULL);
	gtk_text_set_editable (GTK_TEXT(view->text), !view->readonly);
	gtk_text_set_word_wrap (GTK_TEXT(view->text), settings->word_wrap);
	gtk_text_set_line_wrap (GTK_TEXT(view->text), view->line_wrap);

	/* Toolbar */
	view->toolbar = NULL;

        /* Hook the button & key pressed events */
	gtk_signal_connect (GTK_OBJECT (view->text), "button_press_event",
			    GTK_SIGNAL_FUNC (gedit_event_button_press), NULL);
	gtk_signal_connect (GTK_OBJECT (view->text), "key_press_event",
			    GTK_SIGNAL_FUNC (gedit_event_key_press), 0);


	/* Handle Auto Indent */
	gtk_signal_connect_after (GTK_OBJECT (view->text), "insert_text",
				  GTK_SIGNAL_FUNC (auto_indent_cb), view);

	/* Connect the insert & delete text callbacks */
	gtk_signal_connect (GTK_OBJECT (view->text), "insert_text",
			    GTK_SIGNAL_FUNC (doc_insert_text_cb), view);
	gtk_signal_connect (GTK_OBJECT (view->text), "delete_text",
			    GTK_SIGNAL_FUNC (doc_delete_text_cb),
			    (gpointer) view);

	gtk_container_add (GTK_CONTAINER (view->window), view->text);

	view->view_text_changed_signal =
		gtk_signal_connect (GTK_OBJECT(view->text), "changed",
				    GTK_SIGNAL_FUNC (gedit_view_text_changed_cb), view);

	gtk_text_set_point (GTK_TEXT(view->text), 0);
	
#ifdef ENABLE_SPLIT_SCREEN    
	/* Create the bottom split screen */
	view->scrwindow[1] = gtk_scrolled_window_new (NULL, NULL);
	gtk_paned_pack2 (GTK_PANED (view->pane), view->scrwindow[1], TRUE, TRUE);
	/*gtk_box_pack_start (GTK_BOX (view->vbox), view->scrwindow[1], TRUE, TRUE, 1);*/
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (view->scrwindow[1]),
					GTK_POLICY_NEVER,
					GTK_POLICY_AUTOMATIC);

	/*gtk_fixed_put (GTK_FIXED(view), view->scrwindow[1], 0, 0);*/
      	gtk_widget_show (view->scrwindow[1]);

	view->split_screen = gtk_text_new (NULL, NULL);
	gtk_text_set_editable (GTK_TEXT (view->split_screen), !view->readonly);
	gtk_text_set_word_wrap (GTK_TEXT (view->split_screen), settings->word_wrap);
	gtk_text_set_line_wrap (GTK_TEXT (view->split_screen), view->line_wrap);
	
	/* - Signals - */
	gtk_signal_connect (GTK_OBJECT (view->split_screen), "button_press_event",
			   GTK_SIGNAL_FUNC (gedit_event_button_press), NULL);
	gtk_signal_connect (GTK_OBJECT (view->split_screen), "key_press_event",
			    GTK_SIGNAL_FUNC (gedit_event_key_press), NULL);
	
	view->s_insert = gtk_signal_connect (GTK_OBJECT (view->split_screen), "insert_text",
					     GTK_SIGNAL_FUNC (doc_insert_text_cb),
					     (gpointer) view);
		                             
	view->s_delete = gtk_signal_connect (GTK_OBJECT (view->split_screen), "delete_text",
					     GTK_SIGNAL_FUNC (doc_delete_text_cb),
					     (gpointer) view);

	view->s_indent = gtk_signal_connect_after (GTK_OBJECT (view->split_screen), "insert_text",
						   GTK_SIGNAL_FUNC (auto_indent_cb),
						   (gpointer) view);
	gtk_container_add (GTK_CONTAINER (view->scrwindow[1]), view->split_screen);

	view->split_parent = GTK_WIDGET (view->split_screen)->parent;
#endif
	style = gtk_style_copy (gtk_widget_get_style (GTK_WIDGET (gedit_window_active())));
  	gdk_font_unref (style->font);

	bg = &style->base[0];
	bg->red = settings->bg[0];
	bg->green = settings->bg[1];
	bg->blue = settings->bg[2];

	fg = &style->text[0];
	fg->red = settings->fg[0];
	fg->green = settings->fg[1];
	fg->blue = settings->fg[2];

	if (settings->use_fontset)
	{
		style->font = view->font ? gdk_fontset_load (view->font) : NULL;
		if (style->font == NULL)
		{
			style->font = gdk_fontset_load (DEFAULT_FONTSET);
			view->font = DEFAULT_FONTSET;
		}
	}
	else
	{
		style->font = view->font ? gdk_font_load (view->font) : NULL;
		
		if (style->font == NULL)
		{
			style->font = gdk_font_load (DEFAULT_FONT);
			view->font = DEFAULT_FONT;
		} 

	}
	
   	gtk_widget_set_style (GTK_WIDGET(view->text), style);
	gtk_style_unref (style);

	/* Popup Menu */
	menu = gnome_popup_menu_new (popup_menu);
	gnome_popup_menu_attach (menu, view->text, view);
	
        gnome_config_push_prefix ("/gedit/Global/");
	gnome_config_pop_prefix ();
	gnome_config_sync ();

	/*
	gtk_paned_set_position (GTK_PANED (view->pane), 1000);
	*/

	gtk_widget_show_all (view->vbox);
	gtk_widget_grab_focus (view->text);

#ifdef ENABLE_SPLIT_SCREEN    
	gtk_widget_set_style (GTK_WIDGET(view->split_screen), style);
	gtk_widget_show (view->split_screen);
	gtk_text_set_point (GTK_TEXT(view->split_screen), 0);
	gnome_popup_menu_attach (menu, view->split_screen, view);
	view->splitscreen = gnome_config_get_int ("splitscreen");
#endif

}

guint
gedit_view_get_type (void)
{
	static guint gedit_view_type = 0;

	if (!gedit_view_type)
	{
		GtkTypeInfo gedit_view_info =
		{
	  		"gedit_view1",
	  		sizeof (View),
	  		sizeof (ViewClass),
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

GtkWidget *
gedit_view_new (Document *doc)
{
	View *view;
	guchar *document_buffer;

	gedit_debug ("start", DEBUG_VIEW);

	if (doc == NULL)
		return NULL;

	view = gtk_type_new (gedit_view_get_type ());
	view->doc = doc;
	view->doc->views = g_list_append (doc->views, view);
	view->app = NULL;

	document_buffer = gedit_document_get_buffer (view->doc);
	gedit_view_insert (view, 0, document_buffer, FALSE);
	g_free (document_buffer);

	gedit_debug ("end", DEBUG_VIEW);

	return GTK_WIDGET (view);
}

#ifdef ENABLE_SPLIT_SCREEN    
void
gedit_view_set_split_screen (View *view, gint split_screen)
{
	gedit_debug ("", DEBUG_VIEW);

	if (!view->split_parent)
		return;

	if (split_screen)
	   	gtk_widget_show (view->split_parent);
	else
		gtk_widget_hide (view->split_parent);
 
   	view->splitscreen = split_screen;
}
#endif

void
gedit_view_set_word_wrap (View *view, gint word_wrap)
{
	gedit_debug ("", DEBUG_VIEW);

	view->word_wrap = word_wrap;

	gtk_text_set_word_wrap (GTK_TEXT (view->text), word_wrap);
#ifdef ENABLE_SPLIT_SCREEN    
	gtk_text_set_word_wrap (GTK_TEXT (view->split_screen), word_wrap);
#endif	
}

void
gedit_view_set_readonly (View *view, gint readonly)
{
	gchar * doc_name;

	gedit_debug ("", DEBUG_VIEW);
	
	view->readonly = readonly;
	gtk_text_set_editable (GTK_TEXT (view->text), !view->readonly);
#ifdef ENABLE_SPLIT_SCREEN
	if (view->split_screen)
		gtk_text_set_editable (GTK_TEXT (view->split_screen),
				       !view->readonly);
#endif
	
	doc_name = gedit_document_get_tab_name (view->doc);
	gnome_mdi_child_set_name (GNOME_MDI_CHILD (view->doc), doc_name);
	gedit_document_set_title(view->doc);
	g_free (doc_name);
}

/**
 * gedit_view_set_font:
 * @view: View that we're going to change the font for.
 * @fontname: String name "-b&h-lucida-blah-..." of the font to load
 *
 *
 **/
void
gedit_view_set_font (View *view, gchar *fontname)
{
	GtkStyle *style;
	GdkFont *font = NULL;

	gedit_debug ("", DEBUG_VIEW);
	
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

#ifdef ENABLE_SPLIT_SCREEN
  	gtk_widget_set_style (GTK_WIDGET((view)->split_screen), style);
#endif
  	gtk_widget_set_style (GTK_WIDGET((view)->text), style);
}

void
gedit_view_set_position (View *view, gint pos)
{
	gedit_debug ("", DEBUG_VIEW);

	gtk_text_set_point (GTK_TEXT (view->text), pos);
	gtk_text_insert (GTK_TEXT(view->text), NULL, NULL, NULL, " ", 1);
	gtk_text_backward_delete (GTK_TEXT(view->text), 1);

#ifdef ENABLE_SPLIT_SCREEN    	
	gtk_text_set_point (GTK_TEXT (view->split_screen), pos);
#endif
}

guint
gedit_view_get_position (View *view)
{
	guint start_pos, end_pos;
	
	gedit_debug ("", DEBUG_VIEW);

	g_return_val_if_fail (view!=NULL, 0);

	if (gedit_view_get_selection (view, &start_pos, &end_pos))
		return end_pos;
	return gtk_text_get_point (GTK_TEXT (view->text));
}

#if 0 /* Commented out by chema to kill warning. We should implement this */
static void
gedit_view_set_line_wrap (View *view, gint line_wrap)
{
	gedit_debug ("", DEBUG_VIEW);

	view->line_wrap = line_wrap;
	gtk_text_set_line_wrap (GTK_TEXT (view->text), view->line_wrap);
}
#endif

/**
 * gedit_view_set_selection:
 * @view: 
 * @start: 
 * @end: 
 * 
 * if start && end = 0 this is a unselect request
 **/
void
gedit_view_set_selection (View *view, guint start, guint end)
{
	gedit_debug ("", DEBUG_VIEW);

	if (start == 0 && end == 0)
		gtk_text_set_point (GTK_TEXT(view->text), GTK_EDITABLE(GTK_TEXT(view->text))->selection_end_pos);

	gtk_editable_select_region (GTK_EDITABLE (view->text), start, end);

	if (start == 0 && end == 0)
	{
		gtk_text_insert (GTK_TEXT(view->text), NULL, NULL, NULL, " ", 1);
		gtk_text_backward_delete (GTK_TEXT(view->text), 1);
	}

}

/**
 * gedit_view_get_selection:
 * @view: 
 * @start: 
 * @end: 
 * 
 * 
 * 
 * Return Value: TRUE if there is a text slected
 **/
gint
gedit_view_get_selection (View *view, guint *start, guint *end)
{
	guint start_pos, end_pos;

	gedit_debug ("", DEBUG_VIEW);

	start_pos = GTK_EDITABLE(view->text)->selection_start_pos;
        end_pos   = GTK_EDITABLE(view->text)->selection_end_pos;

	/* The user can select from end to start also */
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
	{
		return TRUE;
	}
	else
	{
		start_pos = 0;
		end_pos = 0;
		return FALSE;
	}
}


void
gedit_view_add_cb (GtkWidget *widget, gpointer data)
{
	GnomeMDIChild *child;
	View *view;
	guchar * buffer;

	gedit_debug ("start", DEBUG_DOCUMENT);

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
	}

	gedit_debug ("end", DEBUG_DOCUMENT);
}

void
gedit_view_remove_cb (GtkWidget *widget, gpointer data)
{
	Document *doc = DOCUMENT (data);
	View *view;

	gedit_debug ("", DEBUG_DOCUMENT);

	view = gedit_view_active();
	
	if (view == NULL)
		return;

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
}

static void
gedit_view_update_line_indicator (void)
{
	static char col [32];

	return;
	/*
	*/
	
	/* FIXME: Disable by chema for 0.7.0 . this hack is not working correctly */
	gedit_debug ("", DEBUG_VIEW);

	sprintf (col, "Column: %d", GTK_TEXT(VIEW(gedit_view_active())->text)->cursor_pos_x/7);

	if (settings->show_status)
	{
		GnomeApp *app = gedit_window_active_app();
		gnome_appbar_set_status (GNOME_APPBAR(app->statusbar), col);
	}
}

static gint
gedit_event_button_press (GtkWidget *widget, GdkEventButton *event)
{
	gedit_debug ("", DEBUG_VIEW);
	
	gedit_view_update_line_indicator ();

	return FALSE;
}

static gint
gedit_event_key_press (GtkWidget *w, GdkEventKey *event)
{
	gedit_debug ("", DEBUG_VIEW);

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
		case 's':
			file_save_cb (w, NULL);
	    		break;
		case 'p':
	    		file_print_cb (w, NULL, FALSE);
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
			   and 1 time thru the aceleratior (I guess). Chema 
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


void
gedit_view_load_widgets (View *view)
{
	GnomeUIInfo *toolbar_ui_info;
	GnomeUIInfo *menu_ui_info;
	gint count, sub_count;
	gint temp_undo, temp_redo;

	gedit_debug ("", DEBUG_VIEW);
	
	g_return_if_fail (view->app != NULL);

	toolbar_ui_info = gtk_object_get_data (GTK_OBJECT (view->app),
					       GNOME_MDI_TOOLBAR_INFO_KEY);
	menu_ui_info = gtk_object_get_data (GTK_OBJECT (view->app),
					    GNOME_MDI_CHILD_MENU_INFO_KEY);
	
	g_return_if_fail (toolbar_ui_info != NULL);
	g_return_if_fail (menu_ui_info != NULL);

	if (view->toolbar)
	{
		temp_undo = view->toolbar->undo;
		temp_redo = view->toolbar->redo;
		g_free (view->toolbar);
		view->toolbar->undo = temp_undo;
		view->toolbar->redo = temp_redo;
	}
	
	view->toolbar = g_malloc (sizeof (GeditToolbar));
	view->toolbar->undo_button = NULL;
	view->toolbar->redo_button = NULL;
	view->toolbar->undo_menu_item = NULL;
	view->toolbar->redo_menu_item = NULL;
	
	count = 0;
	while (toolbar_ui_info[count].type != GNOME_APP_UI_ENDOFINFO)
	{
		if (toolbar_ui_info [count].moreinfo == gedit_undo_undo)
			view->toolbar->undo_button = toolbar_ui_info[count].widget;
		if (toolbar_ui_info [count].moreinfo == gedit_undo_redo)
			view->toolbar->redo_button = toolbar_ui_info[count].widget;
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
					view->toolbar->undo_menu_item = sub_menu_ui_info[sub_count].widget;
				if (sub_menu_ui_info [sub_count].moreinfo == gedit_undo_redo)
					view->toolbar->redo_menu_item = sub_menu_ui_info[sub_count].widget;
				sub_count++;
			}
		}
		else
		{
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
gedit_view_set_undo (View *view, gint undo_state, gint redo_state)
{
	gedit_debug ("", DEBUG_VIEW);
	
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
	g_print ("view->toolbar->undo :%i view->toolbar->redo:%i FALSE:%i TRUE:%i\n\n\n",
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
		
