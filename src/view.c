/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* gedit - New Document interface  
 *
 * gedit
 * Copyright (C) 1999 Alex Roberts and Evan Lawrence
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
#include "gedit.h"
#include "undo.h"
#include "print.h"
#include "utils.h"
#include "file.h"

#include "menus.h" /* We need this because some functions
		      modify menu entries. Chema */
#include "view.h"
#include "document.h"
#include "commands.h"
#include "prefs.h"

#define GE_DATA		1

enum {
	CURSOR_MOVED_SIGNAL,
	LAST_SIGNAL
};

void line_pos_cb (GtkWidget *w, gedit_data *data);
void doc_insert_text_cb (GtkWidget *editable, const guchar *insertion_text, int length, int *pos, View *view);

static gint gedit_view_signals[LAST_SIGNAL] = { 0 };
GtkVBoxClass *parent_class = NULL;

void view_changed_cb (GtkWidget *w, gpointer cbdata);
void gedit_view_list_insert (View *view, gedit_data *data);
void view_list_erase (View *view, gedit_data *data);
gint insert_into_buffer (Document *doc, gchar *buffer, gint position);


/* Callback to the "changed" signal in the text widget */
void
view_changed_cb (GtkWidget *w, gpointer cbdata)
{
	View *view;
	
	gedit_debug_mess ("F:view_changed_cb\n", DEBUG_VIEW);
	g_return_if_fail (cbdata != NULL);

	view = (View *) cbdata;

	if (view->document->changed)
		return;
	
	view->document->changed = TRUE;
	gtk_signal_disconnect (GTK_OBJECT(view->text), (gint) view->changed_id);
	view->changed_id = FALSE;

	/* Set the title */
	/*gedit_set_title (view->document);*/ 
	gedit_view_set_read_only ( view, view->document->readonly);
}

/* 
 * Text insertion and deletion callbacks - used for Undo/Redo (not yet
 * implemented) and split screening
 */
void
gedit_view_list_insert (View *view, gedit_data *data)
{
	gint p1;
	gchar *buffer = (guchar *)data->temp2;
	gint position = (gint)data->temp1;
	gint length = strlen (buffer);

	gedit_debug_mess ("F:gedit_view_list_insert\n", DEBUG_VIEW);
	
	if (view != VIEW(mdi->active_view))
	{
		gtk_text_freeze (GTK_TEXT (view->text));
		p1 = gtk_text_get_point (GTK_TEXT (view->text));
		gtk_text_set_point (GTK_TEXT(view->text), position);
		gtk_text_insert (GTK_TEXT (view->text), NULL,
				 NULL, NULL, buffer, length);
		gtk_text_set_point (GTK_TEXT (view->text), p1);
		gtk_text_thaw (GTK_TEXT (view->text));
		
		gtk_text_freeze (GTK_TEXT (view->split_screen));
		p1 = gtk_text_get_point (GTK_TEXT (view->split_screen));
		gtk_text_set_point (GTK_TEXT(view->split_screen), (position));
		gtk_text_insert (GTK_TEXT (view->split_screen), NULL,
				 NULL, NULL, buffer, length);
		gtk_text_set_point (GTK_TEXT (view->text), p1);
		gtk_text_thaw (GTK_TEXT (view->split_screen));
	}	
}

void
view_list_erase (View *view, gedit_data *data)
{
	gedit_debug_mess ("F:gedit_view_list_erase. FIXME: I am empty \n", DEBUG_VIEW);
}

gint
insert_into_buffer (Document *doc, gchar *buffer, gint position)
{
	gedit_debug_mess ("F:insert_into_buffer\n", DEBUG_VIEW);

	if ((doc->buf->len > 0) && (position < doc->buf->len) && (position))
	{
		doc->buf = g_string_insert (doc->buf, position, buffer);
	}
	else if (position == 0)
	{
		gedit_debug_mess ("  pos==0, prepend\n", DEBUG_VIEW_DEEP);
		doc->buf = g_string_prepend (doc->buf, buffer);
	}
	else
	{
		gedit_debug_mess ("  pos!=0, append\n", DEBUG_VIEW_DEEP);
		doc->buf = g_string_append (doc->buf, buffer);
	}
	return 0;		
}

void
doc_insert_text_cb (GtkWidget *editable, const guchar *insertion_text,
		    int length, int *pos, View *view)
{
	GtkWidget *significant_other;
	guchar *buffer;
	gint position = *pos;
	Document *doc;
	gedit_data *data;

	gedit_debug_mess ("F:doc_insert_text_cb\n", DEBUG_VIEW);
	
	if (!view->split_screen)
		return;
	
	if (view->flag == editable)
	{
		view->flag = NULL;
		return;
	}
	
	if (editable == view->text)
		significant_other = view->split_screen;
	else if (editable == view->split_screen)
		significant_other = view->text;
	else
		return;
	
	view->flag = significant_other;	

	buffer = g_new0 (guchar, length + 1);
	strncpy (buffer, insertion_text, length);
	doc = view->document;
	gedit_undo_add (buffer, position, (position + length), INSERT, doc);
	data = g_malloc0 (sizeof (gedit_data));
	data->temp1 = (gint*) position;
	data->temp2 = (guchar*) buffer;
	g_list_foreach (doc->views, (GFunc) gedit_view_list_insert, data);
	gtk_text_freeze (GTK_TEXT (significant_other));
	gtk_editable_insert_text (GTK_EDITABLE (significant_other), buffer, length, &position);
	gtk_text_thaw (GTK_TEXT (significant_other));
	gtk_text_set_point (GTK_TEXT (significant_other), position);
	g_free (data);
	g_free (buffer);
}

void
doc_delete_text_cb (GtkWidget *editable, int start_pos, int end_pos,
		    View *view)
{
	GtkWidget *significant_other;
	Document *doc;
	View *nth_view = NULL;
	gchar *buffer;
	gint n;

	gedit_debug_mess ("F:doc_delete_text_cb\n", DEBUG_VIEW);

	if (!view->split_screen)
		return;

	if (view->flag == editable)
	{
		view->flag = NULL;
		return;
	}
	
	if (editable == view->text)
		significant_other = view->split_screen;
	else if (editable == view->split_screen)
		significant_other = view->text;
	else
		return;
	
	doc = view->document;
	buffer = gtk_editable_get_chars (GTK_EDITABLE(editable), start_pos, end_pos);
	gedit_undo_add (buffer, start_pos, end_pos, DELETE, doc);
	view->flag = significant_other;
	gtk_text_freeze (GTK_TEXT (significant_other));
	gtk_editable_delete_text (GTK_EDITABLE (significant_other), start_pos, end_pos);
	gtk_text_thaw (GTK_TEXT (significant_other));

	for (n = 0; n < g_list_length (doc->views); n++)
	{
		nth_view = g_list_nth_data (doc->views, n);
		if (nth_view != VIEW(mdi->active_view))
		{
			/* Disconnect the signals so we can safely update the other views */
			gtk_signal_disconnect (GTK_OBJECT (nth_view->text), 
					       (gint)nth_view->delete);
			gtk_signal_disconnect (GTK_OBJECT (nth_view->split_screen),
					       (gint)nth_view->s_delete);
			gtk_text_freeze (GTK_TEXT (nth_view->text));
			gtk_editable_delete_text (GTK_EDITABLE (nth_view->text), start_pos, end_pos);
			gtk_text_thaw (GTK_TEXT (nth_view->text));
			gtk_text_freeze (GTK_TEXT (nth_view->split_screen));
			gtk_editable_delete_text (GTK_EDITABLE (nth_view->split_screen),
						  start_pos, end_pos);
			gtk_text_thaw (GTK_TEXT (nth_view->split_screen));
			/* Reconnect the signals, now the views have been updated */
			nth_view->delete = gtk_signal_connect (GTK_OBJECT (nth_view->text),
							       "delete_text",
							       GTK_SIGNAL_FUNC(doc_delete_text_cb),
							       view);
			nth_view->s_delete = gtk_signal_connect (GTK_OBJECT (nth_view->split_screen),
								 "delete_text",
								 GTK_SIGNAL_FUNC(doc_delete_text_cb),
								 view);	
		}
	}
}

gboolean
auto_indent_cb (GtkWidget *text, char *insertion_text, int length,
		int *pos, gpointer data)
{
	int i, newlines, newline_1;
	gchar *buffer, *whitespace;
	View *view;
	Document *doc;

	g_return_val_if_fail (data != NULL, FALSE);

	view = (View *)data;
	newline_1 = 0;

	if ((length != 1) || (insertion_text[0] != '\n'))
		return FALSE;
	if (gtk_text_get_length (GTK_TEXT (text)) <=1)
		return FALSE;
	if (!settings->auto_indent)
		return FALSE;

	gedit_debug_mess ("F:auto_indent_cb\n", DEBUG_VIEW);

	doc = view->document;

	newlines = 0;
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

void
line_pos_cb (GtkWidget *widget, gedit_data *data)
{
	static char col [32];

	gedit_debug_mess ("F:line_pos_cb\n", DEBUG_VIEW);

	sprintf (col, "Column: %d",
		 GTK_TEXT(VIEW(mdi->active_view)->text)->cursor_pos_x/6);

	if (settings->show_status)
	{
		GnomeApp *app = gnome_mdi_get_active_window (mdi);
		gnome_appbar_set_status (GNOME_APPBAR(app->statusbar), col);
	}
}

gint
gedit_event_button_press (GtkWidget *widget, GdkEventButton *event)
{
	gedit_data *data;
	data = g_malloc0 (sizeof (gedit_data));

	gedit_debug_mess ("F:gedit_event_button_press\n", DEBUG_VIEW);
	
	line_pos_cb (NULL, data);

	return FALSE;
}

gint
gedit_event_key_press (GtkWidget *w, GdkEventKey *event)
{
	gint mask;

	gedit_debug_mess ("F:gedit_event_key_press\n", DEBUG_VIEW);

	line_pos_cb (NULL, NULL);
	
	mask = GDK_CONTROL_MASK | GDK_MOD1_MASK | GDK_MOD2_MASK |
	       GDK_MOD3_MASK | GDK_MOD4_MASK | GDK_MOD5_MASK;
	
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
			file_save_cb (w);
	    		break;
		case 'p':
	    		file_print_cb (w, NULL);
	    		break;
		case 'n':
			gtk_signal_emit_stop_by_name (GTK_OBJECT (w), "key_press_event");
			return FALSE;
			break;
		case 'w':
	    		file_close_cb (w, NULL);
	    		break;
		case 'z':
	    		gedit_undo_do (w, NULL);
	    		break;
		case 'r':
	    		gedit_undo_redo (w, NULL);
	    		break;
		case 'k':
	    		gedit_undo_redo (w, NULL);
	    		break;
		default:
	    		return TRUE;
	    		break;
		}
	}
	return TRUE;
}

/* The Widget Stuff */

static void
gedit_view_class_init (ViewClass *klass)
{
	GtkObjectClass *object_class;
	/*GtkWidgetClass *widget_class;
	GtkFixedClass *fixed_class;*/
	
	object_class = (GtkObjectClass *)klass;
	/*widget_class = (GtkWidgetClass *)klass;*/

	gedit_debug_mess ("F:gedit_view_class_init.\n", DEBUG_VIEW);
	
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
	/*widget_class->size_allocate = gedit_view_size_allocate;
	widget_class->size_request = gedit_view_size_request;
	widget_class->expose_event = gedit_view_expose;
	widget_class->realize = gedit_view_realize;
	object_class->finalize = gedit_view_finalize;
	parent_class = gtk_type_class (gtk_vbox_get_type ());*/
}

static void
gedit_view_init (View *view)
{
/*	GtkWidget *vpaned; */
	GtkWidget *menu;
	GtkStyle *style;
	GdkColor *bg;
	GdkColor *fg;

	gedit_debug_mess ("F:gedit_view_init\n", DEBUG_VIEW);

	view->vbox = gtk_vbox_new (TRUE, TRUE);
	gtk_container_add (GTK_CONTAINER (view), view->vbox);
	gtk_widget_show (view->vbox);
	
	/* create our paned window */
	view->pane = gtk_vpaned_new ();
	gtk_box_pack_start (GTK_BOX (view->vbox), view->pane, TRUE, TRUE, 0);
	gtk_paned_set_handle_size (GTK_PANED (view->pane), 10);
	gtk_paned_set_gutter_size (GTK_PANED (view->pane), 10);
	gtk_widget_show (view->pane);
	
	/* Create the upper split screen */
	view->scrwindow[0] = gtk_scrolled_window_new (NULL, NULL);
	gtk_paned_pack1 (GTK_PANED (view->pane), view->scrwindow[0], TRUE, TRUE);
	
	/*gtk_box_pack_start (GTK_BOX (view->vbox), view->scrwindow[0], TRUE, TRUE, 0);*/
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (view->scrwindow[0]),
					GTK_POLICY_NEVER,
					GTK_POLICY_AUTOMATIC);
      	gtk_widget_show (view->scrwindow[0]);

	view->line_wrap = 1;

	view->text = gtk_text_new (NULL, NULL);
	gtk_text_set_editable (GTK_TEXT(view->text), !view->read_only);
	gtk_text_set_word_wrap (GTK_TEXT(view->text), settings->word_wrap);
	gtk_text_set_line_wrap (GTK_TEXT(view->text), view->line_wrap);
	
	/* - Signals - */
	gtk_signal_connect_after(GTK_OBJECT (view->text), "button_press_event",
				 GTK_SIGNAL_FUNC (gedit_event_button_press), NULL);
	gtk_signal_connect_after (GTK_OBJECT (view->text), "key_press_event",
				  GTK_SIGNAL_FUNC (gedit_event_key_press), 0);


	/* Handle Auto Indent */
	view->indent = gtk_signal_connect_after (GTK_OBJECT (view->text), "insert_text",
						 GTK_SIGNAL_FUNC (auto_indent_cb), view);

	/*	
	I'm not even sure why these are here.. i'm sure there are much easier ways
	of implementing undo/redo... 
	*/
	view->insert = gtk_signal_connect (GTK_OBJECT (view->text), "insert_text",
		                           GTK_SIGNAL_FUNC (doc_insert_text_cb), view);
	view->delete = gtk_signal_connect (GTK_OBJECT (view->text), "delete_text",
		                           GTK_SIGNAL_FUNC (doc_delete_text_cb),
		                           (gpointer) view);

/*	gtk_signal_connect_after (GTK_OBJECT(view->text), "key_press_event",
		GTK_SIGNAL_FUNC(gedit_event_button_press), NULL);*/
	gtk_container_add (GTK_CONTAINER (view->scrwindow[0]), view->text);

/*	style = gtk_style_new();
	gtk_widget_set_style(GTK_WIDGET(view->text), style);

	gtk_widget_set_rc_style(GTK_WIDGET(view->text));
	gtk_widget_ensure_style(GTK_WIDGET(view->text));
	g_free (style);
	*/	
/*	doc->changed = FALSE; */
	view->changed_id = gtk_signal_connect (GTK_OBJECT(view->text), "changed",
					       GTK_SIGNAL_FUNC (view_changed_cb), view);

	gtk_widget_show (view->text);
	gtk_text_set_point (GTK_TEXT(view->text), 0);
	
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
	gtk_text_set_editable (GTK_TEXT (view->split_screen), !view->read_only);
	gtk_text_set_word_wrap (GTK_TEXT (view->split_screen), settings->word_wrap);
	gtk_text_set_line_wrap (GTK_TEXT (view->split_screen), view->line_wrap);
	
	/* - Signals - */
	gtk_signal_connect_after (GTK_OBJECT (view->split_screen), "button_press_event",
				  GTK_SIGNAL_FUNC (gedit_event_button_press), NULL);

	gtk_signal_connect_after (GTK_OBJECT (view->split_screen), "key_press_event",
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

	style = gtk_style_copy (gtk_widget_get_style (GTK_WIDGET (mdi->active_window)));
  	gdk_font_unref (style->font);

	bg = &style->base[0];
	bg->red = settings->bg[0];
	bg->green = settings->bg[1];
	bg->blue = settings->bg[2];

	fg = &style->text[0];
	fg->red = settings->fg[0];
	fg->green = settings->fg[1];
	fg->blue = settings->fg[2];

	if (use_fontset)
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
	
 	gtk_widget_push_style (style);
	gtk_widget_set_style (GTK_WIDGET(view->split_screen), style);
   	gtk_widget_set_style (GTK_WIDGET(view->text), style);
   	gtk_widget_pop_style ();

	gtk_widget_show (view->split_screen);
	gtk_text_set_point (GTK_TEXT(view->split_screen), 0);

	/* Popup Menu */
	menu = gnome_popup_menu_new (popup_menu);
	gnome_popup_menu_attach (menu, view->text, view);
	gnome_popup_menu_attach (menu, view->split_screen, view);
	
        gnome_config_push_prefix ("/gedit/Global/");
	view->splitscreen = gnome_config_get_int ("splitscreen");
	gnome_config_pop_prefix ();
	gnome_config_sync ();
	
	/*if (!view->splitscreen)
	  gtk_widget_hide (GTK_WIDGET (view->split_screen)->parent);*/
	gtk_paned_set_position (GTK_PANED (view->pane), 1000);
	/*gtk_fixed_put (GTK_FIXED (view), view->vbox, 0, 0);
	gtk_widget_show (view->vbox);*/

	gtk_widget_grab_focus (view->text);
}

guint
gedit_view_get_type (void)
{
	static guint gedit_view_type = 0;

	gedit_debug_mess ("F:gedit_view_get_type.\n", DEBUG_VIEW);

	if (!gedit_view_type)
	{
		GtkTypeInfo gedit_view_info =
		{
	  		"gedit_view",
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

	if (doc == NULL)
		return NULL;

	view = gtk_type_new (gedit_view_get_type ());
	view->document = doc;

	gedit_debug_mess ("F:gedit_view_new\n", DEBUG_VIEW);
	
	if (view->document->buf)
	{
	  	gtk_text_freeze (GTK_TEXT (view->text));
	  	gtk_text_insert (GTK_TEXT (view->text), NULL,
				 NULL, NULL,
				 view->document->buf->str,
				 view->document->buf->len);
	  	gtk_text_insert (GTK_TEXT (view->split_screen), NULL,
				 NULL, NULL,
				 view->document->buf->str,
				 view->document->buf->len);
		gedit_view_set_position (view, 0);
		gtk_text_thaw (GTK_TEXT (view->text));
	}
	
	view->document->views = g_list_append (doc->views, view);
	
	return GTK_WIDGET (view);
}



/* Public Functions */
void
gedit_view_set_group_type (View *view, guint type)
{
	gedit_debug_mess ("F:gedit_view_set_group_type\n", DEBUG_VIEW);

	view->group_type = type;
	gtk_widget_queue_resize (GTK_WIDGET (view));
}

void
gedit_view_set_split_screen (View *view, gint split_screen)
{
	gedit_debug_mess ("F:gedit_view_set_split_screen\n", DEBUG_VIEW);

	if (!view->split_parent)
		return;

	if (split_screen)
	   	gtk_widget_show (view->split_parent);
	else
		gtk_widget_hide (view->split_parent);
 
   	view->splitscreen = split_screen;
}

void
gedit_view_set_word_wrap (View *view, gint word_wrap)
{
	gedit_debug_mess ("F:gedit_view_set_word_wrap\n", DEBUG_VIEW);

	view->word_wrap = word_wrap;

	gtk_text_set_word_wrap (GTK_TEXT (view->text), word_wrap);
	gtk_text_set_word_wrap (GTK_TEXT (view->split_screen), word_wrap);
}

void
gedit_view_set_line_wrap (View *view, gint line_wrap)
{
	gedit_debug_mess ("F:gedit_view_set_line_wrap\n", DEBUG_VIEW);

	view->line_wrap = line_wrap;
	gtk_text_set_line_wrap (GTK_TEXT (view->text), view->line_wrap);
}

void
gedit_view_set_read_only (View *view, gint read_only)
{
	gchar RO_label[255];

	gedit_debug_mess ("F:gedit_view_set_read_only\n", DEBUG_VIEW);
	
	view->read_only = read_only;
	gtk_text_set_editable (GTK_TEXT (view->text), !view->read_only);
	
	if (read_only)
	{
		sprintf (RO_label, "RO - %s", GNOME_MDI_CHILD(view->document)->name);
		gnome_mdi_child_set_name (GNOME_MDI_CHILD (view->document), RO_label);
	}
	else
	{
		if (view->document->filename)
			gnome_mdi_child_set_name (GNOME_MDI_CHILD(view->document),
						  g_basename(view->document->filename));
		else
			gnome_mdi_child_set_name (GNOME_MDI_CHILD(view->document),
						  gedit_get_document_tab_name());
	}
	 
	if (view->split_screen)
		gtk_text_set_editable (GTK_TEXT (view->split_screen),
				       !view->read_only);

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

	gedit_debug_mess ("F:gedit_view_set_font\n", DEBUG_VIEW);
	
	style = gtk_style_copy (gtk_widget_get_style (GTK_WIDGET((view)->text)));
  	
  	if (use_fontset)
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

  	gtk_widget_set_style (GTK_WIDGET((view)->split_screen), style);
  	gtk_widget_set_style (GTK_WIDGET((view)->text), style);
}

void
gedit_view_set_position (View *view, gint pos)
{
	gedit_debug_mess ("F:gedit_view_set_position\n", DEBUG_VIEW);

	gtk_text_set_point (GTK_TEXT (view->text), pos);
	gtk_text_set_point (GTK_TEXT (view->split_screen), pos);
}

guint
gedit_view_get_position (View *view)
{
	gedit_debug_mess ("F:gedit_view_get_position\n", DEBUG_VIEW);
	return gtk_text_get_point (GTK_TEXT (view->text));
}

guint
gedit_view_get_length (View *view)
{
	gedit_debug_mess ("F:gedit_view_get_length\n", DEBUG_VIEW);
	return gtk_text_get_length (GTK_TEXT (view->text));
}

void
gedit_view_set_selection (View *view, gint start, gint end)
{
	gedit_debug_mess ("F:gedit_view_set_seletion\n", DEBUG_VIEW);
	
	gtk_editable_select_region (GTK_EDITABLE (view->text), start, end);
	gtk_editable_select_region (GTK_EDITABLE (view->text), start, end);
}

/* Sync the itnernal document buffer with what is visible in the text box */
void
gedit_view_buffer_sync (View *view) 
{
	gchar *buf;
	Document *doc = view->document;

	gedit_debug_mess ("F:gedit_view_buffer_sync\n", DEBUG_VIEW);
	
	buf = g_strdup (gtk_editable_get_chars (GTK_EDITABLE (view->text),
						0, gedit_view_get_length (view)));

	doc->buf = g_string_new (buf);
	
	g_free (buf);				   									  
}

void
gedit_view_refresh (View *view)
{
	gint i = gedit_view_get_length (view);

	gedit_debug_mess ("F:gedit_view_refresh<------------------------\n", DEBUG_VIEW);

	if (i > 0)
	{
		gedit_view_set_position (view, i);
		gtk_text_backward_delete (GTK_TEXT(view->text), i);
		gtk_text_backward_delete (GTK_TEXT(view->split_screen), i);
	}

	if (view->document->buf)
	{
		gedit_debug_mess ("F:gedit_view_refresh - Insering buffer \n", DEBUG_VIEW_DEEP);

	  	gtk_text_freeze (GTK_TEXT (view->text));
	  	gtk_text_insert (GTK_TEXT (view->text), NULL,
				 NULL,
				 NULL, view->document->buf->str,
				 view->document->buf->len);

	  	gtk_text_insert (GTK_TEXT (view->split_screen), NULL,
				 NULL,
				 NULL, view->document->buf->str,
				 view->document->buf->len);
		gtk_text_thaw (GTK_TEXT (view->text));
		
		gedit_view_set_position (view, 0);
	}
}
