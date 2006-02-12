/*
 * gedit-tab.c
 * This file is part of gedit
 *
 * Copyright (C) 2005 - Paolo Maggi 
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
 * Modified by the gedit Team, 2005. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n.h>

#include <libgnomeui/libgnomeui.h>
#include <libgnomevfs/gnome-vfs-mime-handlers.h>
#include <libgnomevfs/gnome-vfs-utils.h>

#include "gedit-app.h"
#include "gedit-notebook.h"
#include "gedit-tab.h"
#include "gedit-utils.h"
#include "gedit-message-area.h"
#include "gedit-io-error-message-area.h"
#include "gedit-print-job-preview.h"
#include "gedit-progress-message-area.h"
#include "gedit-debug.h"
#include "gedit-prefs-manager-app.h"
#include "gedit-recent.h"
#include "gedit-convert.h"

#define GEDIT_TAB_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), GEDIT_TYPE_TAB, GeditTabPrivate))

#define GEDIT_TAB_KEY "GEDIT_TAB_KEY"

struct _GeditTabPrivate
{
	GeditTabState	        state;
	
	GtkWidget	       *view;
	GtkWidget	       *view_scrolled_window;

	GtkWidget	       *message_area;
	GtkWidget	       *print_preview;

	GeditPrintJob          *print_job;

	/* tmp data for saving */
	gchar		       *tmp_save_uri;

	/* tmp data for loading */
	gint                    tmp_line_pos;
	const GeditEncoding    *tmp_encoding;
	
	GTimer 		       *timer;
	guint		        times_called;

	GeditDocumentSaveFlags	save_flags;

        gint                    auto_save_interval;
        guint                   auto_save_timeout;
        
	gint	                not_editable : 1;
	gint                    auto_save : 1;
};

G_DEFINE_TYPE(GeditTab, gedit_tab, GTK_TYPE_VBOX)

enum
{
	PROP_0,
	PROP_NAME,
	PROP_STATE,
	PROP_AUTO_SAVE,
	PROP_AUTO_SAVE_INTERVAL
};

static gboolean gedit_tab_auto_save (GeditTab *tab);

static void
install_auto_save_timeout (GeditTab *tab)
{
	gint timeout;

	gedit_debug (DEBUG_TAB);

	g_return_if_fail (tab->priv->auto_save_timeout <= 0);
	g_return_if_fail (tab->priv->auto_save);
	g_return_if_fail (tab->priv->auto_save_interval > 0);
	
	g_return_if_fail (tab->priv->state != GEDIT_TAB_STATE_LOADING);
	g_return_if_fail (tab->priv->state != GEDIT_TAB_STATE_SAVING);
	g_return_if_fail (tab->priv->state != GEDIT_TAB_STATE_REVERTING);
	g_return_if_fail (tab->priv->state != GEDIT_TAB_STATE_LOADING_ERROR);
	g_return_if_fail (tab->priv->state != GEDIT_TAB_STATE_SAVING_ERROR);
	g_return_if_fail (tab->priv->state != GEDIT_TAB_STATE_SAVING_ERROR);
	g_return_if_fail (tab->priv->state != GEDIT_TAB_STATE_REVERTING_ERROR);
	
	/* Add a new timeout */
	timeout = g_timeout_add (tab->priv->auto_save_interval * 1000 * 60,
				 (GSourceFunc) gedit_tab_auto_save,
				 tab);

	tab->priv->auto_save_timeout = timeout;
}

static gboolean
install_auto_save_timeout_if_needed (GeditTab *tab)
{
	GeditDocument *doc;
	
	gedit_debug (DEBUG_TAB);
	
	g_return_val_if_fail (tab->priv->auto_save_timeout <= 0, FALSE);
	g_return_val_if_fail ((tab->priv->state == GEDIT_TAB_STATE_NORMAL) ||
			      (tab->priv->state == GEDIT_TAB_STATE_SHOWING_PRINT_PREVIEW), FALSE);
			  
	doc = gedit_tab_get_document (tab);
	
 	if (tab->priv->auto_save && 
 	    !gedit_document_is_untitled (doc) &&
 	    !gedit_document_get_readonly (doc))
 	{ 
 		install_auto_save_timeout (tab);
 		
 		return TRUE;
 	}
 	
 	return FALSE;
}

static void
remove_auto_save_timeout (GeditTab *tab)
{
	gedit_debug (DEBUG_TAB);

	/* FIXME: check sugli stati */
	
	g_return_if_fail (tab->priv->auto_save_timeout > 0);
	
	g_source_remove (tab->priv->auto_save_timeout);
	tab->priv->auto_save_timeout = 0;
}


static void
gedit_tab_get_property (GObject    *object,
		        guint       prop_id,
		        GValue     *value,
		        GParamSpec *pspec)
{
	GeditTab *tab = GEDIT_TAB (object);

	switch (prop_id)
	{
		case PROP_NAME:
			g_value_take_string (value,
					     _gedit_tab_get_name (tab));
			break;
		case PROP_STATE:
			g_value_set_int (value,
					 gedit_tab_get_state (tab));
			break;			
		case PROP_AUTO_SAVE:
			g_value_set_boolean (value,
					     gedit_tab_get_auto_save_enabled (tab));
			break;
		case PROP_AUTO_SAVE_INTERVAL:
			g_value_set_int (value,
					 gedit_tab_get_auto_save_interval (tab));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;			
	}
}

static void
gedit_tab_set_property (GObject      *object,
		        guint         prop_id,
		        const GValue *value,
		        GParamSpec   *pspec)
{
	GeditTab *tab = GEDIT_TAB (object);

	switch (prop_id)
	{
		case PROP_AUTO_SAVE:
			gedit_tab_set_auto_save_enabled (tab,
							 g_value_get_boolean (value));
			break;
		case PROP_AUTO_SAVE_INTERVAL:
			gedit_tab_set_auto_save_interval (tab,
							  g_value_get_int (value));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;			
	}
}

static void
gedit_tab_finalize (GObject *object)
{
	GeditTab *tab = GEDIT_TAB (object);
	
	if (tab->priv->print_job != NULL)
	{
		gedit_debug_message (DEBUG_TAB, "Cancelling printing");
		
		gtk_source_print_job_cancel (GTK_SOURCE_PRINT_JOB (tab->priv->print_job));
		g_object_unref (tab->priv->print_job);
	}
	
	if (tab->priv->timer != NULL)
		g_timer_destroy (tab->priv->timer);

	g_free (tab->priv->tmp_save_uri);

	if (tab->priv->auto_save_timeout > 0)
		remove_auto_save_timeout (tab);

	G_OBJECT_CLASS (gedit_tab_parent_class)->finalize (object);
}

static void 
gedit_tab_class_init (GeditTabClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = gedit_tab_finalize;
	object_class->get_property = gedit_tab_get_property;
	object_class->set_property = gedit_tab_set_property;
	
	g_object_class_install_property (object_class,
					 PROP_NAME,
					 g_param_spec_string ("name",
							      "Name",
							      "The tab's name",
							      NULL,
							      G_PARAM_READABLE));
	g_object_class_install_property (object_class,
					 PROP_STATE,
					 g_param_spec_int ("state",
							   "State",
							   "The tab's state",
							   0, /* GEDIT_TAB_STATE_NORMAL */
							   GEDIT_TAB_NUM_OF_STATES - 1,
							   0, /* GEDIT_TAB_STATE_NORMAL */
							   G_PARAM_READABLE));							      	
							      
	g_object_class_install_property (object_class,
					 PROP_AUTO_SAVE,
					 g_param_spec_boolean ("autosave",
							       "Autosave",
							       "Autosave feature",
							       TRUE,
							       G_PARAM_READWRITE));

	g_object_class_install_property (object_class,
					 PROP_AUTO_SAVE_INTERVAL,
					 g_param_spec_int ("autosave_interval",
							   "AutosaveInterval",
							   "Time between two autosaves",
							   0,
							   G_MAXINT,
							   0,
							   G_PARAM_READWRITE));
							      
	g_type_class_add_private (object_class, sizeof (GeditTabPrivate));
}


GeditTabState
gedit_tab_get_state (GeditTab *tab)
{
	g_return_val_if_fail (GEDIT_IS_TAB (tab), GEDIT_TAB_STATE_NORMAL);
	
	return tab->priv->state;
}

static void
set_cursor_according_to_state (GtkTextView   *view,
			       GeditTabState  state)
{
	GdkCursor *cursor;
	GdkWindow *text_window;
	GdkWindow *left_window;

	text_window = gtk_text_view_get_window (view, GTK_TEXT_WINDOW_TEXT);
	left_window = gtk_text_view_get_window (view, GTK_TEXT_WINDOW_LEFT);

	if ((state == GEDIT_TAB_STATE_LOADING)          ||
	    (state == GEDIT_TAB_STATE_REVERTING)        ||
	    (state == GEDIT_TAB_STATE_SAVING)           ||
	    (state == GEDIT_TAB_STATE_PRINTING)         ||
	    (state == GEDIT_TAB_STATE_PRINT_PREVIEWING) ||
	    (state == GEDIT_TAB_STATE_CLOSING))
	{
		cursor = gdk_cursor_new_for_display (
				gtk_widget_get_display (GTK_WIDGET (view)),
				GDK_WATCH);

		if (text_window != NULL)
			gdk_window_set_cursor (text_window, cursor);
		if (left_window != NULL)
			gdk_window_set_cursor (left_window, cursor);

		gdk_cursor_unref (cursor);
	}
	else
	{
		cursor = gdk_cursor_new_for_display (
				gtk_widget_get_display (GTK_WIDGET (view)),
				GDK_XTERM);

		if (text_window != NULL)
			gdk_window_set_cursor (text_window, cursor);
		if (left_window != NULL)
			gdk_window_set_cursor (left_window, NULL);

		gdk_cursor_unref (cursor);
	}
}

static void
view_realized (GtkTextView *view,
	       GeditTab    *tab)
{
	set_cursor_according_to_state (view, tab->priv->state);
}

static void
set_view_properties_according_to_state (GeditTab      *tab,
					GeditTabState  state)
{
	gboolean val;

	val = ((state == GEDIT_TAB_STATE_NORMAL) &&
	       (tab->priv->print_preview == NULL) &&
	       !tab->priv->not_editable);
	gtk_text_view_set_editable (GTK_TEXT_VIEW (tab->priv->view), val);

	val = ((state != GEDIT_TAB_STATE_LOADING) &&
	       (state != GEDIT_TAB_STATE_CLOSING));
	gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (tab->priv->view), val);

	val = ((state != GEDIT_TAB_STATE_LOADING) &&
	       (state != GEDIT_TAB_STATE_CLOSING) &&
	       (gedit_prefs_manager_get_highlight_current_line ()));
	gtk_source_view_set_highlight_current_line (GTK_SOURCE_VIEW (tab->priv->view), val);
}

static void
gedit_tab_set_state (GeditTab      *tab,
		     GeditTabState  state)
{
	g_return_if_fail (GEDIT_IS_TAB (tab));
	g_return_if_fail ((state >= 0) && (state < GEDIT_TAB_NUM_OF_STATES));

	if (tab->priv->state == state)
		return;

	tab->priv->state = state;

	set_view_properties_according_to_state (tab, state);

	if ((state == GEDIT_TAB_STATE_LOADING_ERROR) || // FIXME: add other states if needed
	    (state == GEDIT_TAB_STATE_SHOWING_PRINT_PREVIEW))
	{
		gtk_widget_hide (tab->priv->view_scrolled_window);
	}
	else
	{
		if (tab->priv->print_preview == NULL)
			gtk_widget_show (tab->priv->view_scrolled_window);
	}

	set_cursor_according_to_state (GTK_TEXT_VIEW (tab->priv->view),
				       state);

	g_object_notify (G_OBJECT (tab), "state");		
}



static void 
document_uri_notify_handler (GeditDocument *document,
			     GParamSpec    *pspec,
			     GeditTab      *tab)
{
	gedit_debug (DEBUG_TAB);
	
	/* Notify the change in the URI */
	g_object_notify (G_OBJECT (tab), "name");
}

static void
document_modified_changed (GtkTextBuffer *document,
			   GeditTab      *tab)
{
	g_object_notify (G_OBJECT (tab), "name");
}

static void
set_message_area (GeditTab  *tab,
		  GtkWidget *message_area)
{
	if (tab->priv->message_area == message_area)
		return;

	if (tab->priv->message_area != NULL)
		gtk_widget_destroy (tab->priv->message_area);

	tab->priv->message_area = message_area;

	if (message_area == NULL)
		return;

	gtk_box_pack_start (GTK_BOX (tab),
			    tab->priv->message_area,
			    FALSE,
			    FALSE,
			    0);		

	g_object_add_weak_pointer (G_OBJECT (tab->priv->message_area), 
				   (gpointer *)&tab->priv->message_area);
}

static void
remove_tab (GeditTab *tab)
{
	GeditNotebook *notebook;

	notebook = GEDIT_NOTEBOOK (gtk_widget_get_parent (GTK_WIDGET (tab)));

	gedit_notebook_remove_tab (notebook, tab);
}

static void 
unrecoverable_loading_error_message_area_response (GeditMessageArea *message_area,
						   gint              response_id,
						   GeditTab         *tab)
{
	remove_tab (tab);
}

static void 
recoverable_loading_error_message_area_response (GeditMessageArea *message_area,
						 gint              response_id,
						 GeditTab         *tab)
{
	GeditDocument *doc;
	const gchar *uri;

	doc = gedit_tab_get_document (tab);
	g_return_if_fail (GEDIT_IS_DOCUMENT (doc));

	uri = gedit_document_get_uri (doc);
	g_return_if_fail (uri != NULL);

	if (response_id == GTK_RESPONSE_OK)
	{
		const GeditEncoding *encoding;
		
		encoding = gedit_conversion_error_message_area_get_encoding (
				GTK_WIDGET (message_area));

		g_return_if_fail (encoding != NULL);
			
		set_message_area (tab, NULL);
		gedit_tab_set_state (tab, GEDIT_TAB_STATE_LOADING);
	
		tab->priv->tmp_encoding = encoding;
		
		g_return_if_fail (tab->priv->auto_save_timeout <= 0);

		gedit_document_load (doc,
				     uri,
				     encoding,
				     tab->priv->tmp_line_pos,
				     FALSE);
	}
	else
	{
		gedit_recent_remove (uri);
		
		unrecoverable_loading_error_message_area_response (message_area,
								   response_id,
								   tab);
	}
}

static void 
file_already_open_warning_message_area_response (GtkWidget   *message_area,
						 gint         response_id,
						 GeditTab    *tab)
{
	GeditView *view;
	
	view = gedit_tab_get_view (tab);
	
	if (response_id == GTK_RESPONSE_YES)
	{
		tab->priv->not_editable = FALSE;
		
		gtk_text_view_set_editable (GTK_TEXT_VIEW (view),
					    TRUE);
	}

	gtk_widget_destroy (message_area);

	gtk_widget_grab_focus (GTK_WIDGET (view));	
}

static void
load_cancelled (GeditMessageArea *area,
                gint              response_id,
                GeditTab         *tab)
{
	g_return_if_fail (GEDIT_IS_PROGRESS_MESSAGE_AREA (tab->priv->message_area));
	
	g_object_ref (tab);
	gedit_document_load_cancel (gedit_tab_get_document (tab));
	g_object_unref (tab);	
}

static void 
unrecoverable_reverting_error_message_area_response (GeditMessageArea *message_area,
						     gint              response_id,
						     GeditTab         *tab)
{
	GeditView *view;
	
	gedit_tab_set_state (tab,
			     GEDIT_TAB_STATE_NORMAL);

	set_message_area (tab, NULL);

	view = gedit_tab_get_view (tab);

	gtk_widget_grab_focus (GTK_WIDGET (view));
	
	install_auto_save_timeout_if_needed (tab);	
}

#define MAX_MSG_LENGTH 100

static void
show_loading_message_area (GeditTab *tab)
{
	GtkWidget *area;
	GeditDocument *doc = NULL;
	const gchar *short_name;
	gchar *name;
	gchar *dirname = NULL;
	gchar *msg = NULL;
	gchar *name_markup;
	gchar *dirname_markup;
	gint len;
	
	if (tab->priv->message_area != NULL)
		return;
	
	gedit_debug (DEBUG_TAB);
		
	doc = gedit_tab_get_document (tab);
	g_return_if_fail (doc != NULL);

	short_name = gedit_document_get_short_name_for_display (doc);

	len = g_utf8_strlen (short_name, -1);

	/* if the name is awfully long, truncate it and be done with it,
	 * otherwise also show the directory (ellipsized if needed)
	 */
	if (len > MAX_MSG_LENGTH)
	{
		name = gedit_utils_str_middle_truncate (short_name, 
							MAX_MSG_LENGTH);
	}
	else
	{
		gchar *uri;
		gchar *str;

		name = g_strdup (short_name);

		uri = gedit_document_get_uri_for_display (doc);
		str = gedit_utils_uri_get_dirname (uri);
		g_free (uri);

		if (str != NULL)
		{
			/* use the remaining space for the dir, but use a min of 20 chars
			 * so that we do not end up with a dirname like "(a...b)".
			 * This means that in the worst case when the filename is long 99
			 * we have a title long 99 + 20, but I think it's a rare enough
			 * case to be acceptable. It's justa darn title afterall :)
			 */
			dirname = gedit_utils_str_middle_truncate (str, 
								   MAX (20, MAX_MSG_LENGTH - len));
			g_free (str);
		}
	}

	name_markup = g_markup_printf_escaped ("<b>%s</b>", name);

	if (tab->priv->state == GEDIT_TAB_STATE_REVERTING)
	{
		if (dirname != NULL)
		{
			dirname_markup = g_markup_printf_escaped ("<b>%s</b>", dirname);

			/* Translators: the first %s is a file name (e.g. test.txt) the second one
			   is a directory (e.g. ssh://master.gnome.org/home/users/paolo) */
			msg = g_strdup_printf (_("Reverting %s from %s"),
					       name_markup,
					       dirname_markup);
			g_free (dirname_markup);
		}
		else
		{
			msg = g_strdup_printf (_("Reverting %s"), 
					       name_markup);
		}
		
		area = gedit_progress_message_area_new (GTK_STOCK_REVERT_TO_SAVED,
							msg,
							TRUE);
	}
	else
	{
		if (dirname != NULL)
		{
			dirname_markup = g_markup_printf_escaped ("<b>%s</b>", dirname);

			/* Translators: the first %s is a file name (e.g. test.txt) the second one
			   is a directory (e.g. ssh://master.gnome.org/home/users/paolo) */
			msg = g_strdup_printf (_("Loading %s from %s"),
					       name_markup,
					       dirname_markup);
			g_free (dirname_markup);
		}
		else
		{
			msg = g_strdup_printf (_("Loading %s"), 
					       name_markup);
		}

		area = gedit_progress_message_area_new (GTK_STOCK_OPEN,
							msg,
							TRUE);
	}

	g_signal_connect (area,
			  "response",
			  G_CALLBACK (load_cancelled),
			  tab);
						 
	gtk_widget_show (area);

	set_message_area (tab, area);

	g_free (msg);
	g_free (name);
	g_free (name_markup);
	g_free (dirname);
}

static void
show_saving_message_area (GeditTab *tab)
{
	GtkWidget *area;
	GeditDocument *doc = NULL;
	gchar *short_name;
	gchar *from;
	gchar *to = NULL;
	gchar *from_markup;
	gchar *to_markup;
	gchar *msg = NULL;
	gint len;

	g_return_if_fail (tab->priv->tmp_save_uri != NULL);
	
	if (tab->priv->message_area != NULL)
		return;
	
	gedit_debug (DEBUG_TAB);
		
	doc = gedit_tab_get_document (tab);
	g_return_if_fail (doc != NULL);

	short_name = gedit_document_get_short_name_for_display (doc);

	len = g_utf8_strlen (short_name, -1);

	/* if the name is awfully long, truncate it and be done with it,
	 * otherwise also show the directory (ellipsized if needed)
	 */
	if (len > MAX_MSG_LENGTH)
	{
		from = gedit_utils_str_middle_truncate (short_name, 
							MAX_MSG_LENGTH);
		g_free (short_name);
	}
	else
	{
		gchar *str;

		from = short_name;

		to = gnome_vfs_format_uri_for_display (tab->priv->tmp_save_uri);

		str = gedit_utils_str_middle_truncate (to, 
						       MAX (20, MAX_MSG_LENGTH - len));
		g_free (to);
			
		to = str;
	}

	from_markup = g_markup_printf_escaped ("<b>%s</b>", from);

	if (to != NULL)
	{
		to_markup = g_markup_printf_escaped ("<b>%s</b>", to);

		/* Translators: the first %s is a file name (e.g. test.txt) the second one
		   is a directory (e.g. ssh://master.gnome.org/home/users/paolo) */
		msg = g_strdup_printf (_("Saving %s to %s"),
				       from_markup,
				       to_markup);
		g_free (to_markup);
	}
	else
	{
		msg = g_strdup_printf (_("Saving %s"), from_markup);
	}

	area = gedit_progress_message_area_new (GTK_STOCK_SAVE,
						msg,
						FALSE);

	gtk_widget_show (area);

	set_message_area (tab, area);

	g_free (msg);
	g_free (to);
	g_free (from);
	g_free (from_markup);
}

static void
message_area_set_progress (GeditTab	    *tab,
			   GnomeVFSFileSize  size,
			   GnomeVFSFileSize  total_size)
{
	if (tab->priv->message_area == NULL)
		return;
	
	gedit_debug_message (DEBUG_TAB, "%Ld/%Ld", size, total_size);
	
	g_return_if_fail (GEDIT_IS_PROGRESS_MESSAGE_AREA (tab->priv->message_area));
	
	if (total_size == 0)
	{
		if (size != 0)
			gedit_progress_message_area_pulse (
					GEDIT_PROGRESS_MESSAGE_AREA (tab->priv->message_area));	
		else
			gedit_progress_message_area_set_fraction (
				GEDIT_PROGRESS_MESSAGE_AREA (tab->priv->message_area),
				0);
	}
	else
	{
		gdouble frac;

		frac = (gdouble)size / (gdouble)total_size;

		gedit_progress_message_area_set_fraction (
				GEDIT_PROGRESS_MESSAGE_AREA (tab->priv->message_area),
				frac);		
	}
}

static void
document_loading (GeditDocument    *document,
		  GnomeVFSFileSize  size,
		  GnomeVFSFileSize  total_size,
		  GeditTab         *tab)
{
	double et;

	g_return_if_fail ((tab->priv->state == GEDIT_TAB_STATE_LOADING) ||
		 	  (tab->priv->state == GEDIT_TAB_STATE_REVERTING));

	gedit_debug_message (DEBUG_TAB, "%Ld/%Ld", size, total_size);
	
	if (tab->priv->timer == NULL)
	{
		g_return_if_fail (tab->priv->times_called == 0);
		tab->priv->timer = g_timer_new ();
	}

	et = g_timer_elapsed (tab->priv->timer, NULL);

	if (tab->priv->times_called == 1)
	{
		if ((total_size == 0) || (total_size > 51200UL) /* 50 KB */)
		{
			show_loading_message_area (tab);
		}
	}
	else
	{
		if ((tab->priv->times_called == 3) && (total_size != 0))
		{
			gdouble total_time;

			/* et : total_time = size : total_size */
			total_time = (et * total_size)/size;

			if ((total_time - et) > 3.0)
			{
				show_loading_message_area (tab);
			}
		}
		else
		{
			if (et > 3.0)
			{
				show_loading_message_area (tab);
			}
		}
	}
	
	message_area_set_progress (tab, size, total_size);

	tab->priv->times_called++;
}

static gboolean
remove_tab_idle (GeditTab *tab)
{
	remove_tab (tab);

	return FALSE;
}

static void
document_loaded (GeditDocument *document,
		 const GError  *error,
		 GeditTab      *tab)
{
	GtkWidget *emsg;
	gchar *uri;
	const GeditEncoding *encoding;

	g_return_if_fail ((tab->priv->state == GEDIT_TAB_STATE_LOADING) ||
			  (tab->priv->state == GEDIT_TAB_STATE_REVERTING));
	g_return_if_fail (tab->priv->auto_save_timeout <= 0);

	if (tab->priv->timer != NULL)
	{
		g_timer_destroy (tab->priv->timer);
		tab->priv->timer = NULL;
	}
	tab->priv->times_called = 0;

	set_message_area (tab, NULL);

	uri = gedit_document_get_uri (document);

	if (error != NULL)
	{
		if (tab->priv->state == GEDIT_TAB_STATE_LOADING)
			gedit_tab_set_state (tab, GEDIT_TAB_STATE_LOADING_ERROR);
		else
			gedit_tab_set_state (tab, GEDIT_TAB_STATE_REVERTING_ERROR);

		encoding = gedit_document_get_encoding (document);

		if (error->domain == GEDIT_DOCUMENT_ERROR)
		{
			if (error->code == GNOME_VFS_ERROR_CANCELLED)
			{
				/* remove the tab, but in an idle handler, since
				 * we are in the handler of doc loaded and we 
				 * don't want doc and tab to be finalized now.
				 */
				g_idle_add ((GSourceFunc) remove_tab_idle, tab);

				goto end;
			}
			else
			{
				gedit_recent_remove (uri);

				if (tab->priv->state == GEDIT_TAB_STATE_LOADING_ERROR)
				{
					emsg = gedit_unrecoverable_loading_error_message_area_new (uri, 
												   error);
					g_signal_connect (emsg,
							  "response",
							  G_CALLBACK (unrecoverable_loading_error_message_area_response),
							  tab);
				}
				else
				{
					g_return_if_fail (tab->priv->state == GEDIT_TAB_STATE_REVERTING_ERROR);
					
					emsg = gedit_unrecoverable_reverting_error_message_area_new (uri, 
												     error);

					g_signal_connect (emsg,
							  "response",
							  G_CALLBACK (unrecoverable_reverting_error_message_area_response),
							  tab);
				}

				set_message_area (tab, emsg);
			}
		}					  
		else
		{
			g_return_if_fail ((error->domain == G_CONVERT_ERROR) ||
			      		  (error->domain == GEDIT_CONVERT_ERROR));
			
			// TODO: different error messages if tab->priv->state == GEDIT_TAB_STATE_REVERTING?
			// note that while reverting encoding should be ok, so this is unlikely to happen
			emsg = gedit_conversion_error_while_loading_message_area_new (
									uri,
									tab->priv->tmp_encoding,
									error);

			set_message_area (tab, emsg);

			g_signal_connect (emsg,
					  "response",
					  G_CALLBACK (recoverable_loading_error_message_area_response),
					  tab);
		}

		gedit_message_area_set_default_response (GEDIT_MESSAGE_AREA (emsg),
							 GTK_RESPONSE_CANCEL);

		gtk_widget_show (emsg);

		return;
	}
	else
	{
		GList *all_documents;
		GList *l;

		g_return_if_fail (uri != NULL);
		
		gedit_recent_add (uri);

		all_documents = gedit_app_get_documents (gedit_app_get_default ());

		for (l = all_documents; l != NULL; l = g_list_next (l))
		{
			GeditDocument *d = GEDIT_DOCUMENT (l->data);
			
			if (d != document)
			{
				gchar *u;

				u = gedit_document_get_uri (d);

				if ((u != NULL) &&
			    	    gnome_vfs_uris_match (uri, u))
			    	{
			    		GtkWidget *w;
			    		GeditView *view;

			    		view = gedit_tab_get_view (tab);

			    		tab->priv->not_editable = TRUE;

			    		w = gedit_file_already_open_warning_message_area_new (uri);

					set_message_area (tab, w);

					gedit_message_area_set_default_response (GEDIT_MESSAGE_AREA (w),
							 GTK_RESPONSE_CANCEL);

					gtk_widget_show (w);

					g_signal_connect (w,
							  "response",
							  G_CALLBACK (file_already_open_warning_message_area_response),
							  tab);

					g_free (u);

			    		break;
			    	}

				g_free (u);
			}
		}

		g_list_free (all_documents);

		gedit_tab_set_state (tab, GEDIT_TAB_STATE_NORMAL);
		
		install_auto_save_timeout_if_needed (tab);
	}

 end:
	g_free (uri);

	tab->priv->tmp_line_pos = 0;
	tab->priv->tmp_encoding = NULL;
}

static void
document_saving (GeditDocument    *document,
		 GnomeVFSFileSize  size,
		 GnomeVFSFileSize  total_size,
		 GeditTab         *tab)
{
	double et;

	g_return_if_fail (tab->priv->state == GEDIT_TAB_STATE_SAVING);

	gedit_debug_message (DEBUG_TAB, "%Ld/%Ld", size, total_size);
	
	if (tab->priv->timer == NULL)
	{
		g_return_if_fail (tab->priv->times_called == 0);
		tab->priv->timer = g_timer_new ();
	}

	et = g_timer_elapsed (tab->priv->timer, NULL);

	if (tab->priv->times_called == 1)
	{
		if ((total_size == 0) || (total_size > 51200UL) /* 50 KB */)
		{
			show_saving_message_area (tab);
		}
	}
	else
	{
		if ((tab->priv->times_called == 3) && (total_size != 0))
		{
			gdouble total_time;

			/* et : total_time = size : total_size */
			total_time = (et * total_size)/size;

			if ((total_time - et) > 3.0)
			{
				show_saving_message_area (tab);
			}
		}
		else
		{
			if (et > 3.0)
			{
				show_saving_message_area (tab);
			}
		}
	}
	
	message_area_set_progress (tab, size, total_size);

	tab->priv->times_called++;
}

static void
end_saving (GeditTab *tab)
{
	/* Reset tmp data for saving */
	g_free (tab->priv->tmp_save_uri);
	tab->priv->tmp_save_uri = NULL;
	tab->priv->tmp_encoding = NULL;
	
	install_auto_save_timeout_if_needed (tab);

}

static void 
unrecoverable_saving_error_message_area_response (GeditMessageArea *message_area,
						  gint              response_id,
						  GeditTab         *tab)
{
	GeditView *view;
	
	if (tab->priv->print_preview != NULL)
		gedit_tab_set_state (tab, GEDIT_TAB_STATE_SHOWING_PRINT_PREVIEW);
	else
		gedit_tab_set_state (tab, GEDIT_TAB_STATE_NORMAL);

	end_saving (tab);
	
	set_message_area (tab, NULL);

	view = gedit_tab_get_view (tab);

	gtk_widget_grab_focus (GTK_WIDGET (view));	
}

static void 
no_backup_error_message_area_response (GeditMessageArea *message_area,
				       gint              response_id,
				       GeditTab         *tab)
{
	if (response_id == GTK_RESPONSE_YES)
	{
		GeditDocument *doc;

		doc = gedit_tab_get_document (tab);
		g_return_if_fail (GEDIT_IS_DOCUMENT (doc));

		set_message_area (tab, NULL);

		g_return_if_fail (tab->priv->tmp_save_uri != NULL);
		g_return_if_fail (tab->priv->tmp_encoding != NULL);

		gedit_tab_set_state (tab, GEDIT_TAB_STATE_SAVING);

		/* don't bug the user again with this... */
		tab->priv->save_flags |= GEDIT_DOCUMENT_SAVE_IGNORE_BACKUP;

		g_return_if_fail (tab->priv->auto_save_timeout <= 0);
		
		/* Force saving */
		gedit_document_save (doc, tab->priv->save_flags);
	}
	else
	{
		unrecoverable_saving_error_message_area_response (message_area,
								  response_id,
								  tab);
	}
}

static void
externally_modified_error_message_area_response (GeditMessageArea *message_area,
						 gint              response_id,
						 GeditTab         *tab)
{
	if (response_id == GTK_RESPONSE_YES)
	{
		GeditDocument *doc;

		doc = gedit_tab_get_document (tab);
		g_return_if_fail (GEDIT_IS_DOCUMENT (doc));
		
		set_message_area (tab, NULL);

		g_return_if_fail (tab->priv->tmp_save_uri != NULL);
		g_return_if_fail (tab->priv->tmp_encoding != NULL);

		gedit_tab_set_state (tab, GEDIT_TAB_STATE_SAVING);

		g_return_if_fail (tab->priv->auto_save_timeout <= 0);
		
		/* ignore mtime should not be persisted in save flags across saves */

		/* Force saving */
		gedit_document_save (doc, tab->priv->save_flags | GEDIT_DOCUMENT_SAVE_IGNORE_MTIME);
	}
	else
	{		
		unrecoverable_saving_error_message_area_response (message_area,
								  response_id,
								  tab);
	}
}

static void 
recoverable_saving_error_message_area_response (GeditMessageArea *message_area,
						gint              response_id,
						GeditTab         *tab)
{
	GeditDocument *doc;

	doc = gedit_tab_get_document (tab);
	g_return_if_fail (GEDIT_IS_DOCUMENT (doc));
	
	if (response_id == GTK_RESPONSE_OK)
	{
		const GeditEncoding *encoding;
		
		encoding = gedit_conversion_error_message_area_get_encoding (
									GTK_WIDGET (message_area));

		g_return_if_fail (encoding != NULL);

		set_message_area (tab, NULL);

		g_return_if_fail (tab->priv->tmp_save_uri != NULL);
				
		gedit_tab_set_state (tab, GEDIT_TAB_STATE_SAVING);
			
		tab->priv->tmp_encoding = encoding;

		gedit_debug_message (DEBUG_TAB, "Force saving with URI '%s'", tab->priv->tmp_save_uri);
			 
		g_return_if_fail (tab->priv->auto_save_timeout <= 0);
		
		gedit_document_save_as (doc,
					tab->priv->tmp_save_uri,
					tab->priv->tmp_encoding,
					tab->priv->save_flags);
	}
	else
	{		
		unrecoverable_saving_error_message_area_response (message_area,
								  response_id,
								  tab);
	}
}

static void
document_saved (GeditDocument *document,
		const GError  *error,
		GeditTab      *tab)
{
	GtkWidget *emsg;

	g_return_if_fail (tab->priv->state == GEDIT_TAB_STATE_SAVING);

	g_return_if_fail (tab->priv->tmp_save_uri != NULL);
	g_return_if_fail (tab->priv->tmp_encoding != NULL);	
	g_return_if_fail (tab->priv->auto_save_timeout <= 0);
	
	g_timer_destroy (tab->priv->timer);
	tab->priv->timer = NULL;
	tab->priv->times_called = 0;
	
	set_message_area (tab, NULL);
	
	if (error != NULL)
	{
		gedit_tab_set_state (tab, GEDIT_TAB_STATE_SAVING_ERROR);
		
		if (error->domain == GEDIT_DOCUMENT_ERROR)
		{
			if (error->code == GEDIT_DOCUMENT_ERROR_EXTERNALLY_MODIFIED)
			{
				/* This error is recoverable */
				emsg = gedit_externally_modified_saving_error_message_area_new (
								tab->priv->tmp_save_uri, 
								error);
				g_return_if_fail (emsg != NULL);

				set_message_area (tab, emsg);

				g_signal_connect (emsg,
						  "response",
						  G_CALLBACK (externally_modified_error_message_area_response),
						  tab);
			}
			else if (error->code == GEDIT_DOCUMENT_ERROR_CANT_CREATE_BACKUP)
			{
				/* This error is recoverable */
				emsg = gedit_no_backup_saving_error_message_area_new (
								tab->priv->tmp_save_uri, 
								error);
				g_return_if_fail (emsg != NULL);

				set_message_area (tab, emsg);

				g_signal_connect (emsg,
						  "response",
						  G_CALLBACK (no_backup_error_message_area_response),
						  tab);
			}
			else
			{
				/* These errors are _NOT_ recoverable */
				gedit_recent_remove (tab->priv->tmp_save_uri);
				
				emsg = gedit_unrecoverable_saving_error_message_area_new (tab->priv->tmp_save_uri, 
									  error);
				g_return_if_fail (emsg != NULL);
		
				set_message_area (tab, emsg);

				g_signal_connect (emsg,
						  "response",
						  G_CALLBACK (unrecoverable_saving_error_message_area_response),
						  tab);
			}			
		}
		else
		{
			/* This error is recoverable */
			g_return_if_fail (error->domain == G_CONVERT_ERROR);

			emsg = gedit_conversion_error_while_saving_message_area_new (
									tab->priv->tmp_save_uri,
									tab->priv->tmp_encoding,
									error);

			set_message_area (tab, emsg);

			g_signal_connect (emsg,
					  "response",
					  G_CALLBACK (recoverable_saving_error_message_area_response),
					  tab);
		}
		
		gedit_message_area_set_default_response (GEDIT_MESSAGE_AREA (emsg),
							 GTK_RESPONSE_CANCEL);

		gtk_widget_show (emsg);
		
	}
	else
	{
		gedit_recent_add (tab->priv->tmp_save_uri);
		
		if (tab->priv->print_preview != NULL)
			gedit_tab_set_state (tab, GEDIT_TAB_STATE_SHOWING_PRINT_PREVIEW);
		else
			gedit_tab_set_state (tab, GEDIT_TAB_STATE_NORMAL);
			
		end_saving (tab);
	}

}

static void
gedit_tab_init (GeditTab *tab)
{
	GtkWidget *sw;
	GeditDocument *doc;

	tab->priv = GEDIT_TAB_GET_PRIVATE (tab);

	tab->priv->state = GEDIT_TAB_STATE_NORMAL;

	tab->priv->not_editable = FALSE;

	tab->priv->save_flags = 0;
	
	/* Create the scrolled window */
	sw = gtk_scrolled_window_new (NULL, NULL);
	tab->priv->view_scrolled_window = sw;

	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);

	/* Manage auto save data */
	tab->priv->auto_save = gedit_prefs_manager_get_auto_save ();
	tab->priv->auto_save = (tab->priv->auto_save != FALSE);

	tab->priv->auto_save_interval = gedit_prefs_manager_get_auto_save_interval ();
	if (tab->priv->auto_save_interval <= 0)
		tab->priv->auto_save_interval = GPM_DEFAULT_AUTO_SAVE_INTERVAL;

	/* Create the view */
	doc = gedit_document_new ();
	g_object_set_data (G_OBJECT (doc), GEDIT_TAB_KEY, tab);

	tab->priv->view = gedit_view_new (doc);
	g_object_unref (doc);
	gtk_widget_show (tab->priv->view);
	g_object_set_data (G_OBJECT (tab->priv->view), GEDIT_TAB_KEY, tab);

	gtk_box_pack_end (GTK_BOX (tab), sw, TRUE, TRUE, 0);
	gtk_container_add (GTK_CONTAINER (sw), tab->priv->view);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw),
					     GTK_SHADOW_IN);	
	gtk_widget_show (sw);

	g_signal_connect (doc,
			  "notify::uri",
			  G_CALLBACK (document_uri_notify_handler),
			  tab);
	g_signal_connect (doc,
			  "modified_changed",
			  G_CALLBACK (document_modified_changed),
			  tab);
	g_signal_connect (doc,
			  "loading",
			  G_CALLBACK (document_loading),
			  tab);
	g_signal_connect (doc,
			  "loaded",
			  G_CALLBACK (document_loaded),
			  tab);
	g_signal_connect (doc,
			  "saving",
			  G_CALLBACK (document_saving),
			  tab);
	g_signal_connect (doc,
			  "saved",
			  G_CALLBACK (document_saved),
			  tab);
			  
	g_signal_connect_after(tab->priv->view,
			       "realize",
			       G_CALLBACK (view_realized),
			       tab);
}

GtkWidget *
_gedit_tab_new (void)
{
	return GTK_WIDGET (g_object_new (GEDIT_TYPE_TAB, NULL));
}

/* Whether create is TRUE, creates a new empty document if location does 
   not refer to an existing file */
GtkWidget *
_gedit_tab_new_from_uri (const gchar         *uri,
			 const GeditEncoding *encoding,
			 gint                 line_pos,			
			 gboolean             create)
{
	GeditTab *tab;

	gboolean ret;
	
	g_return_val_if_fail (uri != NULL, NULL);
	
	tab = GEDIT_TAB (_gedit_tab_new ());
		
	ret = _gedit_tab_load (tab,
			       uri,
			       encoding,
			       line_pos,
			       create);

	if (!ret)
	{
		g_object_unref (tab);
		return NULL;
	}

	return GTK_WIDGET (tab);
}		

GeditView *
gedit_tab_get_view (GeditTab *tab)
{
	return GEDIT_VIEW (tab->priv->view);
}

/* This is only an helper function */
GeditDocument *
gedit_tab_get_document (GeditTab *tab)
{
	return GEDIT_DOCUMENT (gtk_text_view_get_buffer (
					GTK_TEXT_VIEW (tab->priv->view)));
}

#define MAX_DOC_NAME_LENGTH 40

gchar *
_gedit_tab_get_name (GeditTab *tab)
{
	GeditDocument *doc;
	gchar *name;
	gchar *docname;
	gchar *tab_name;

	g_return_val_if_fail (GEDIT_IS_TAB (tab), NULL);

	doc = gedit_tab_get_document (tab);

	name = gedit_document_get_short_name_for_display (doc);

	/* Truncate the name so it doesn't get insanely wide. */
	docname = gedit_utils_str_middle_truncate (name, MAX_DOC_NAME_LENGTH);

	if (gtk_text_buffer_get_modified (GTK_TEXT_BUFFER (doc)))
	{
		tab_name = g_strdup_printf ("*%s", docname);
	} 
	else 
	{
 #if 0		
		if (gedit_document_get_readonly (doc)) 
		{
			tab_name = g_strdup_printf ("%s [%s]", docname, 
						/*Read only*/ _("RO"));
		} 
		else 
		{
			tab_name = g_strdup_printf ("%s", docname);
		}
#endif
		tab_name = g_strdup (docname);
	}
	
	g_free (docname);
	g_free (name);

	return tab_name;
}

gchar *
_gedit_tab_get_tooltips	(GeditTab *tab)
{
	GeditDocument *doc;
	gchar *tip;
	gchar *uri;
	gchar *ruri;
	gchar *ruri_markup;
	gchar *mime_type;
	const gchar *mime_description = NULL;
	gchar *mime_full_description; 
	gchar *encoding;
	const GeditEncoding *enc;

	g_return_val_if_fail (GEDIT_IS_TAB (tab), NULL);

	doc = gedit_tab_get_document (tab);

	uri = gedit_document_get_uri_for_display (doc);
	g_return_val_if_fail (uri != NULL, NULL);

	ruri = 	gedit_utils_replace_home_dir_with_tilde (uri);
	g_free (uri);

	ruri_markup = g_markup_printf_escaped ("<i>%s</i>", ruri);

	switch (tab->priv->state)
	{
		case GEDIT_TAB_STATE_LOADING_ERROR:
			tip = g_strdup_printf (_("Error opening file %s"),
					       ruri_markup);
			break;

		case GEDIT_TAB_STATE_REVERTING_ERROR:
			tip = g_strdup_printf (_("Error reverting file %s"),
					       ruri_markup);
			break;			

		case GEDIT_TAB_STATE_SAVING_ERROR:
			tip =  g_strdup_printf (_("Error saving file %s"),
						ruri_markup);
			break;			
		default:
			mime_type = gedit_document_get_mime_type (doc);
			mime_description = gnome_vfs_mime_get_description (mime_type);

			if (mime_description == NULL)
				mime_full_description = g_strdup (mime_type);
			else
				mime_full_description = g_strdup_printf ("%s (%s)", 
						mime_description, mime_type);

			g_free (mime_type);

			enc = gedit_document_get_encoding (doc);

			if (enc == NULL)
				encoding = g_strdup (_("Unicode (UTF-8)"));
			else
				encoding = gedit_encoding_to_string (enc);

			tip =  g_markup_printf_escaped ("<b>%s</b> %s\n\n"
						        "<b>%s</b> %s\n"
						        "<b>%s</b> %s",
						        _("Name:"), ruri,
						        _("MIME Type:"), mime_full_description,
						        _("Encoding:"), encoding);

			g_free (encoding);
			g_free (mime_full_description);
			
			break;
	}
	
	g_free (ruri);	
	g_free (ruri_markup);
	
	return tip;
}

static GdkPixbuf *
resize_icon (GdkPixbuf *pixbuf,
	     gint       size)
{
	guint width, height;
	
	width = gdk_pixbuf_get_width (pixbuf); 
	height = gdk_pixbuf_get_height (pixbuf);
	/* if the icon is larger than the nominal size, scale down */
	if (MAX (width, height) > size) 
	{
		GdkPixbuf *scaled_pixbuf;
		
		if (width > height) 
		{
			height = height * size / width;
			width = size;
		} 
		else 
		{
			width = width * size / height;
			height = size;
		}
		
		scaled_pixbuf = gdk_pixbuf_scale_simple	(pixbuf, 
							 width, 
							 height, 
							 GDK_INTERP_BILINEAR);
		g_object_unref (pixbuf);
		pixbuf = scaled_pixbuf;
	}
	
	return pixbuf;
}

static GdkPixbuf *
get_icon (GtkIconTheme *theme, 
	  const gchar  *uri,
	  const gchar  *mime_type, 
	  gint          size)
{
	gchar *icon;
	GdkPixbuf *pixbuf;
	
	icon = gnome_icon_lookup (theme, NULL, uri, NULL, NULL,
				  mime_type, 0, NULL);
	

	g_return_val_if_fail (icon != NULL, NULL);

	pixbuf = gtk_icon_theme_load_icon (theme, icon, size, 0, NULL);
	g_free (icon);
	if (pixbuf == NULL)
		return NULL;
		
	return resize_icon (pixbuf, size);
}

static GdkPixbuf *
get_stock_icon (GtkIconTheme *theme, 
		const gchar  *stock,
		gint          size)
{
	GdkPixbuf *pixbuf;
	
	pixbuf = gtk_icon_theme_load_icon (theme, stock, size, 0, NULL);
	if (pixbuf == NULL)
		return NULL;
		
	return resize_icon (pixbuf, size);
}

/* FIXME: add support for theme changed. I think it should be as easy as
   call g_object_notify (tab, "name") when the icon theme changes */
GdkPixbuf *
_gedit_tab_get_icon (GeditTab *tab)
{
	GdkPixbuf *pixbuf;
	GtkIconTheme *theme;
	GdkScreen *screen;
	gint icon_size;

	g_return_val_if_fail (GEDIT_IS_TAB (tab), NULL);

	screen = gtk_widget_get_screen (GTK_WIDGET (tab));

	theme = gtk_icon_theme_get_for_screen (screen);
	g_return_val_if_fail (theme != NULL, NULL);

	gtk_icon_size_lookup_for_settings (gtk_widget_get_settings (GTK_WIDGET (tab)),
					   GTK_ICON_SIZE_MENU, 
					   NULL,
					   &icon_size);

	switch (tab->priv->state)
	{
		case GEDIT_TAB_STATE_LOADING:
			pixbuf = get_stock_icon (theme, 
						 GTK_STOCK_OPEN, 
						 icon_size);
			break;
		case GEDIT_TAB_STATE_REVERTING:
			pixbuf = get_stock_icon (theme, 
						 GTK_STOCK_REVERT_TO_SAVED, 
						 icon_size);						 
			break;
		case GEDIT_TAB_STATE_SAVING:
			pixbuf = get_stock_icon (theme, 
						 GTK_STOCK_SAVE, 
						 icon_size);
			break;
		case GEDIT_TAB_STATE_PRINTING:
			pixbuf = get_stock_icon (theme, 
						 GTK_STOCK_PRINT, 
						 icon_size);
			break;
		case GEDIT_TAB_STATE_PRINT_PREVIEWING:
		case GEDIT_TAB_STATE_SHOWING_PRINT_PREVIEW:		
			pixbuf = get_stock_icon (theme, 
						 GTK_STOCK_PRINT_PREVIEW, 
						 icon_size);
			break;

		case GEDIT_TAB_STATE_LOADING_ERROR:
		case GEDIT_TAB_STATE_REVERTING_ERROR:		
		case GEDIT_TAB_STATE_SAVING_ERROR:
		case GEDIT_TAB_STATE_GENERIC_ERROR:
			pixbuf = get_stock_icon (theme, 
						 GTK_STOCK_DIALOG_ERROR, 
						 icon_size);
			break;
		default:
		{
			gchar *raw_uri;
			gchar *mime_type;
			GeditDocument *doc;

			doc = gedit_tab_get_document (tab);

			raw_uri = gedit_document_get_uri (doc);
			mime_type = gedit_document_get_mime_type (doc);

			pixbuf = get_icon (theme, raw_uri, mime_type, icon_size);

			g_free (raw_uri);
			g_free (mime_type);
		}
	}

	return pixbuf;
}

GeditTab *
gedit_tab_get_from_document (GeditDocument *doc)
{
	gpointer res;
	
	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), NULL);
	
	res = g_object_get_data (G_OBJECT (doc), GEDIT_TAB_KEY);
	
	return (res != NULL) ? GEDIT_TAB (res) : NULL;
}

gboolean
_gedit_tab_load (GeditTab            *tab,
		 const gchar         *uri,
		 const GeditEncoding *encoding,
		 gint                 line_pos,
		 gboolean             create)
{
	GeditDocument *doc;

	g_return_val_if_fail (GEDIT_IS_TAB (tab), FALSE);
	g_return_val_if_fail (tab->priv->state == GEDIT_TAB_STATE_NORMAL, FALSE);

	doc = gedit_tab_get_document (tab);
	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), FALSE);

	gedit_tab_set_state (tab, GEDIT_TAB_STATE_LOADING);

	tab->priv->tmp_line_pos = line_pos;
	tab->priv->tmp_encoding = encoding;
	
	if (tab->priv->auto_save_timeout > 0)
		remove_auto_save_timeout (tab);

	return gedit_document_load (doc,
				    uri,
				    encoding,
				    line_pos,
				    create);
}

void
_gedit_tab_revert (GeditTab *tab)
{
	GeditDocument *doc;
	gchar *uri;

	g_return_if_fail (GEDIT_IS_TAB (tab));
	g_return_if_fail (tab->priv->state == GEDIT_TAB_STATE_NORMAL);

	doc = gedit_tab_get_document (tab);
	g_return_if_fail (GEDIT_IS_DOCUMENT (doc));

	gedit_tab_set_state (tab, GEDIT_TAB_STATE_REVERTING);

	uri = gedit_document_get_uri (doc);
	g_return_if_fail (uri != NULL);

	tab->priv->tmp_line_pos = 0;
	tab->priv->tmp_encoding = gedit_document_get_encoding (doc);

	if (tab->priv->auto_save_timeout > 0)
		remove_auto_save_timeout (tab);

	gedit_document_load (doc,
			     uri,
			     tab->priv->tmp_encoding,
			     0,
			     FALSE);

	g_free (uri);
}

void
_gedit_tab_save (GeditTab *tab)
{
	GeditDocument *doc;

	g_return_if_fail (GEDIT_IS_TAB (tab));
	g_return_if_fail ((tab->priv->state == GEDIT_TAB_STATE_NORMAL) ||
			  (tab->priv->state == GEDIT_TAB_STATE_SHOWING_PRINT_PREVIEW));
	g_return_if_fail (tab->priv->tmp_save_uri == NULL);
	g_return_if_fail (tab->priv->tmp_encoding == NULL);

	doc = gedit_tab_get_document (tab);
	g_return_if_fail (GEDIT_IS_DOCUMENT (doc));
	g_return_if_fail (!gedit_document_is_untitled (doc));

	gedit_tab_set_state (tab, GEDIT_TAB_STATE_SAVING);

	/* uri used in error messages, will be freed in document_saved */
	tab->priv->tmp_save_uri = gedit_document_get_uri (doc);
	tab->priv->tmp_encoding = gedit_document_get_encoding (doc); 

	if (tab->priv->auto_save_timeout > 0)
		remove_auto_save_timeout (tab);
		
	gedit_document_save (doc, tab->priv->save_flags);
}

static gboolean
gedit_tab_auto_save (GeditTab *tab)
{
	GeditDocument *doc;

	gedit_debug (DEBUG_TAB);
	
	g_return_val_if_fail (tab->priv->tmp_save_uri == NULL, FALSE);
	g_return_val_if_fail (tab->priv->tmp_encoding == NULL, FALSE);
	
	doc = gedit_tab_get_document (tab);
	
	g_return_val_if_fail (!gedit_document_is_untitled (doc), FALSE);
	g_return_val_if_fail (!gedit_document_get_readonly (doc), FALSE);

	g_return_val_if_fail (tab->priv->auto_save_timeout > 0, FALSE);
	g_return_val_if_fail (tab->priv->auto_save, FALSE);
	g_return_val_if_fail (tab->priv->auto_save_interval > 0, FALSE);

	if (!gtk_text_buffer_get_modified (GTK_TEXT_BUFFER(doc)))
	{
		gedit_debug_message (DEBUG_TAB, "Document not modified");

		return TRUE;
	}
			
	if ((tab->priv->state != GEDIT_TAB_STATE_NORMAL) &&
	    (tab->priv->state != GEDIT_TAB_STATE_SHOWING_PRINT_PREVIEW))
	{
		/* Retry after 15 seconds */
		guint timeout;
		
		gedit_debug_message (DEBUG_TAB, "Retry after 15 seconds");
	
		/* Add a new timeout */
		timeout = g_timeout_add (30 * 1000,
					(GSourceFunc) gedit_tab_auto_save,
					tab);

		tab->priv->auto_save_timeout = timeout;
	    
	    	/* Returns FALSE so the old timeout is "destroyed" */
		return FALSE;
	}
	
	gedit_tab_set_state (tab, GEDIT_TAB_STATE_SAVING);

	/* uri used in error messages, will be freed in document_saved */
	tab->priv->tmp_save_uri = gedit_document_get_uri (doc);
	tab->priv->tmp_encoding = gedit_document_get_encoding (doc); 

	/* Set auto_save_timeout to 0 since the timeout is going to be destroyed */
	tab->priv->auto_save_timeout = 0;

	/* Since we are autosaving, we need to preserve the backup that was produced
	   the last time the user "manually" saved the file. In the case a recoverable
	   error happens while saving, the last backup is not preserved since the user
	   expressed his willing of saving the file */
	gedit_document_save (doc, tab->priv->save_flags | GEDIT_DOCUMENT_SAVE_PRESERVE_BACKUP);
	
	gedit_debug_message (DEBUG_TAB, "Done");
	
	/* Returns FALSE so the old timeout is "destroyed" */
	return FALSE;
}

void
_gedit_tab_save_as (GeditTab            *tab,
		    const gchar         *uri,
		    const GeditEncoding *encoding)
{
	GeditDocument *doc;

	g_return_if_fail (GEDIT_IS_TAB (tab));
	g_return_if_fail ((tab->priv->state == GEDIT_TAB_STATE_NORMAL) ||
			  (tab->priv->state == GEDIT_TAB_STATE_SHOWING_PRINT_PREVIEW));
	g_return_if_fail (encoding != NULL);
				  
	g_return_if_fail (tab->priv->tmp_save_uri == NULL);
	g_return_if_fail (tab->priv->tmp_encoding == NULL);
	
	doc = gedit_tab_get_document (tab);
	g_return_if_fail (GEDIT_IS_DOCUMENT (doc));

	gedit_tab_set_state (tab, GEDIT_TAB_STATE_SAVING);

	/* uri used in error messages... strdup because errors are async
	 * and the string can go away, will be freed in document_saved */
	tab->priv->tmp_save_uri = g_strdup (uri);
	tab->priv->tmp_encoding = encoding;

	/* reset the save flags, when saving as */
	tab->priv->save_flags = 0;
	
	if (tab->priv->auto_save_timeout > 0)
		remove_auto_save_timeout (tab);

	gedit_document_save_as (doc, uri, encoding, tab->priv->save_flags);
}

static void
print_preview_destroyed (GtkWidget *preview,
			 GeditTab  *tab)
{
	tab->priv->print_preview = NULL;
	
	if (tab->priv->state == GEDIT_TAB_STATE_SHOWING_PRINT_PREVIEW)
		gedit_tab_set_state (tab, GEDIT_TAB_STATE_NORMAL);
	else
		gtk_widget_show (tab->priv->view_scrolled_window);		
}

static void
set_print_preview (GeditTab  *tab, GtkWidget *print_preview)
{
	if (tab->priv->print_preview == print_preview)
		return;
		
	if (tab->priv->print_preview != NULL)
		gtk_widget_destroy (tab->priv->print_preview);
		
	
	tab->priv->print_preview = print_preview;

	gtk_box_pack_end (GTK_BOX (tab),
			  tab->priv->print_preview,
			  TRUE,
			  TRUE,
			  0);		

	gtk_widget_grab_focus (tab->priv->print_preview);

	g_signal_connect (tab->priv->print_preview,
			  "destroy",
			  G_CALLBACK (print_preview_destroyed),
			  tab);
}

#define MIN_PAGES 15

static void
print_page_cb (GtkSourcePrintJob *pjob, GeditTab *tab)
{
	gchar *str;
	gint page_num;
	gint total;

	g_return_if_fail (GEDIT_IS_PROGRESS_MESSAGE_AREA (tab->priv->message_area));	

	total = gtk_source_print_job_get_page_count (pjob);
	
	if (total < MIN_PAGES)
		return;

	page_num = gtk_source_print_job_get_page (pjob);
			
	g_return_if_fail (GEDIT_IS_PROGRESS_MESSAGE_AREA (tab->priv->message_area));	
	
	gtk_widget_show (tab->priv->message_area);
	
	str = g_strdup_printf (_("Rendering page %d of %d..."), page_num, total);

	gedit_progress_message_area_set_text (GEDIT_PROGRESS_MESSAGE_AREA (tab->priv->message_area),
					      str);
	g_free (str);
	
	gedit_progress_message_area_set_fraction (GEDIT_PROGRESS_MESSAGE_AREA (tab->priv->message_area),
						  1.0 * page_num / total);
}

static void
preview_finished_cb (GtkSourcePrintJob *pjob, GeditTab *tab)
{
	GnomePrintJob *gjob;
	GtkWidget *preview = NULL;

	g_return_if_fail (GEDIT_IS_PROGRESS_MESSAGE_AREA (tab->priv->message_area));
	set_message_area (tab, NULL); /* destroy the message area */
	
	gjob = gtk_source_print_job_get_print_job (pjob);

	preview = gedit_print_job_preview_new (gjob);	
 	g_object_unref (gjob);
	
	set_print_preview (tab, preview);
	
	gtk_widget_show (preview);
	g_object_unref (pjob);
	
	gedit_tab_set_state (tab, GEDIT_TAB_STATE_SHOWING_PRINT_PREVIEW);
}

static void
print_finished_cb (GtkSourcePrintJob *pjob, GeditTab *tab)
{
	GnomePrintJob *gjob;

	g_return_if_fail (GEDIT_IS_PROGRESS_MESSAGE_AREA (tab->priv->message_area));
	set_message_area (tab, NULL); /* destroy the message area */
	
	gjob = gtk_source_print_job_get_print_job (pjob);

	gnome_print_job_print (gjob);
 	g_object_unref (gjob);

	gedit_print_job_save_config (GEDIT_PRINT_JOB (pjob));
	
	g_object_unref (pjob);
	
	gedit_tab_set_state (tab, GEDIT_TAB_STATE_NORMAL);
}

static void
print_cancelled (GeditMessageArea *area,
                 gint              response_id,
                 GeditTab         *tab)
{
	g_return_if_fail (GEDIT_IS_PROGRESS_MESSAGE_AREA (tab->priv->message_area));
	
	gtk_source_print_job_cancel (GTK_SOURCE_PRINT_JOB (tab->priv->print_job));
	g_object_unref (tab->priv->print_job);

	set_message_area (tab, NULL); /* destroy the message area */
	
	gedit_tab_set_state (tab, GEDIT_TAB_STATE_NORMAL);
}

static void
show_printing_message_area (GeditTab      *tab,
			    gboolean       preview)
{
	GtkWidget *area;
	
	if (preview)
		area = gedit_progress_message_area_new (GTK_STOCK_PRINT_PREVIEW,
							"",
							TRUE);
	else
		area = gedit_progress_message_area_new (GTK_STOCK_PRINT,
							"",
							TRUE);
	

	g_signal_connect (area,
			  "response",
			  G_CALLBACK (print_cancelled),
			  tab);
	  
	set_message_area (tab, area);
}

void 
_gedit_tab_print (GeditTab      *tab,
		  GeditPrintJob *pjob,
		  GtkTextIter   *start, 
		  GtkTextIter   *end)
{
	GeditDocument *doc;
	
	g_return_if_fail (GEDIT_IS_TAB (tab));
	g_return_if_fail (GEDIT_IS_PRINT_JOB (pjob));
	g_return_if_fail (start != NULL);
	g_return_if_fail (end != NULL);	
		
	doc = GEDIT_DOCUMENT (gtk_source_print_job_get_buffer (GTK_SOURCE_PRINT_JOB (pjob)));
	g_return_if_fail (doc != NULL);
	g_return_if_fail (gedit_tab_get_document (tab) == doc);
	g_return_if_fail (gtk_text_iter_get_buffer (start) == GTK_TEXT_BUFFER (doc));
	g_return_if_fail (gtk_text_iter_get_buffer (end) == GTK_TEXT_BUFFER (doc));	
	
	g_return_if_fail (tab->priv->print_job == NULL);
	g_return_if_fail ((tab->priv->state == GEDIT_TAB_STATE_NORMAL) ||
			  (tab->priv->state == GEDIT_TAB_STATE_SHOWING_PRINT_PREVIEW));
	
	g_object_ref (pjob);
	tab->priv->print_job = pjob;
	g_object_add_weak_pointer (G_OBJECT (pjob), 
				   (gpointer *) &tab->priv->print_job);
	
	show_printing_message_area (tab, FALSE);

	g_signal_connect (pjob, "begin_page", (GCallback) print_page_cb, tab);
	g_signal_connect (pjob, "finished", (GCallback) print_finished_cb, tab);

	gedit_tab_set_state (tab, GEDIT_TAB_STATE_PRINTING);
	
	if (!gtk_source_print_job_print_range_async (GTK_SOURCE_PRINT_JOB (pjob), start, end))
	{
		/* FIXME: go in error state */
		gtk_text_view_set_editable (GTK_TEXT_VIEW (tab->priv->view), 
					    !tab->priv->not_editable);
		g_warning ("Async print preview failed");
		g_object_unref (pjob);
	}
}

void
_gedit_tab_print_preview (GeditTab      *tab,
			  GeditPrintJob *pjob,
			  GtkTextIter   *start, 
			  GtkTextIter   *end)		  
{
	GeditDocument *doc;
	
	g_return_if_fail (GEDIT_IS_TAB (tab));
	g_return_if_fail (GEDIT_IS_PRINT_JOB (pjob));
	g_return_if_fail (start != NULL);
	g_return_if_fail (end != NULL);	
		
	doc = GEDIT_DOCUMENT (gtk_source_print_job_get_buffer (GTK_SOURCE_PRINT_JOB (pjob)));
	g_return_if_fail (doc != NULL);
	g_return_if_fail (gedit_tab_get_document (tab) == doc);
	g_return_if_fail (gtk_text_iter_get_buffer (start) == GTK_TEXT_BUFFER (doc));
	g_return_if_fail (gtk_text_iter_get_buffer (end) == GTK_TEXT_BUFFER (doc));	
	
	g_return_if_fail (tab->priv->print_job == NULL);
	g_return_if_fail (tab->priv->state == GEDIT_TAB_STATE_NORMAL);
	
	g_object_ref (pjob);
	tab->priv->print_job = pjob;
	g_object_add_weak_pointer (G_OBJECT (pjob), 
				   (gpointer *) &tab->priv->print_job);
	
	show_printing_message_area (tab, TRUE);

	g_signal_connect (pjob, "begin_page", (GCallback) print_page_cb, tab);
	g_signal_connect (pjob, "finished", (GCallback) preview_finished_cb, tab);

	gedit_tab_set_state (tab, GEDIT_TAB_STATE_PRINT_PREVIEWING);
	
	if (!gtk_source_print_job_print_range_async (GTK_SOURCE_PRINT_JOB (pjob), start, end))
	{
		/* FIXME: go in error state */
		gedit_tab_set_state (tab, GEDIT_TAB_STATE_NORMAL);
		g_warning ("Async print preview failed");
		g_object_unref (pjob);
	}
}

void 
_gedit_tab_mark_for_closing (GeditTab *tab)
{
	g_return_if_fail (GEDIT_IS_TAB (tab));
	g_return_if_fail (tab->priv->state == GEDIT_TAB_STATE_NORMAL);
	
	gedit_tab_set_state (tab, GEDIT_TAB_STATE_CLOSING);
}

gboolean
_gedit_tab_can_close (GeditTab *tab)
{
	GeditDocument *doc;
	GeditTabState  ts;

	g_return_val_if_fail (GEDIT_IS_TAB (tab), FALSE);

	ts = gedit_tab_get_state (tab);

	/* if we are loading or reverting, the tab can be closed */
	if ((ts == GEDIT_TAB_STATE_LOADING)       ||
	    (ts == GEDIT_TAB_STATE_LOADING_ERROR) ||
	    (ts == GEDIT_TAB_STATE_REVERTING)     ||
	    (ts == GEDIT_TAB_STATE_REVERTING_ERROR)) /* CHECK: I'm not sure this is the right behavior for REVERTING ERROR */
		return TRUE;

	/* Do not close tab with saving errors */
	if (ts == GEDIT_TAB_STATE_SAVING_ERROR)
		return FALSE;
		
	doc = gedit_tab_get_document (tab);

	/* TODO: we need to save the file also if it has been externally
	   modified - Paolo (Oct 10, 2005) */

	return (!gtk_text_buffer_get_modified (GTK_TEXT_BUFFER (doc)) &&
		!gedit_document_get_deleted (doc));
}

/**
 * gedit_tab_get_auto_save_enabled:
 * @tab: 
 * 
 * Gets the current state for the autosave feature
 * 
 * Return value: TRUE if the autosave is enabled, else FALSE
 **/
gboolean
gedit_tab_get_auto_save_enabled	(GeditTab *tab)
{
	gedit_debug (DEBUG_TAB);

	g_return_val_if_fail (GEDIT_IS_TAB (tab), FALSE);

	return tab->priv->auto_save;
}

/**
 * gedit_tab_set_auto_save_enabled:
 * @tab: a #GeditTab
 * @enable: enable (TRUE) or disable (FALSE) auto save
 * 
 * Enables or disables the autosave feature. It does not install an
 * autosave timeout if the document is new or is read-only
 **/
void
gedit_tab_set_auto_save_enabled	(GeditTab *tab, 
				 gboolean enable)
{
	GeditDocument *doc = NULL;
	
	gedit_debug (DEBUG_TAB);

	g_return_if_fail (GEDIT_IS_TAB (tab));

	doc = gedit_tab_get_document (tab);
	
	if (tab->priv->auto_save == enable)
		return;

	tab->priv->auto_save = enable;

 	if (enable && 
 	    (tab->priv->auto_save_timeout <=0) &&
 	    !gedit_document_is_untitled (doc) &&
 	    !gedit_document_get_readonly (doc))
 	{
 		if ((tab->priv->state != GEDIT_TAB_STATE_LOADING) &&
		    (tab->priv->state != GEDIT_TAB_STATE_SAVING) &&
		    (tab->priv->state != GEDIT_TAB_STATE_REVERTING) &&
		    (tab->priv->state != GEDIT_TAB_STATE_LOADING_ERROR) &&
		    (tab->priv->state != GEDIT_TAB_STATE_SAVING_ERROR) &&
		    (tab->priv->state != GEDIT_TAB_STATE_REVERTING_ERROR))
		{
			install_auto_save_timeout (tab);
		}
		/* else: the timeout will be installed when loading/saving/reverting
		         will terminate */
		
		return;
	}
 		
 	if (!enable && (tab->priv->auto_save_timeout > 0))
 	{
		remove_auto_save_timeout (tab);
		
 		return; 
 	} 

 	g_return_if_fail ((!enable && (tab->priv->auto_save_timeout <= 0)) ||  
 			  gedit_document_is_untitled (doc) || gedit_document_get_readonly (doc)); 
}

/**
 * gedit_tab_get_auto_save_interval:
 * @tab: 
 * 
 * Gets the current interval for the autosaves
 * 
 * Return value: the value of the autosave
 **/
gint 
gedit_tab_get_auto_save_interval (GeditTab *tab)
{
	gedit_debug (DEBUG_TAB);

	g_return_val_if_fail (GEDIT_IS_TAB (tab), 0);

	return tab->priv->auto_save_interval;
}

/**
 * gedit_tab_set_auto_save_interval:
 * @tab: a #GeditTab
 * @interval: the new interval
 * 
 * Sets the interval for the autosave feature. It does nothing if the
 * interval is the same as the one already present. It removes the old
 * interval timeout and adds a new one with the autosave passed as
 * argument.
 **/
void 
gedit_tab_set_auto_save_interval (GeditTab *tab, 
				  gint interval)
{
	GeditDocument *doc = NULL;
	
	gedit_debug (DEBUG_TAB);
	
	g_return_if_fail (GEDIT_IS_TAB (tab));

	doc = gedit_tab_get_document(tab);

	g_return_if_fail (GEDIT_IS_DOCUMENT (doc));
	g_return_if_fail (interval > 0);

	if (tab->priv->auto_save_interval == interval)
		return;

	tab->priv->auto_save_interval = interval;
		
	if (!tab->priv->auto_save)
		return;

	if (tab->priv->auto_save_timeout > 0)
	{
		g_return_if_fail (!gedit_document_is_untitled (doc));
		g_return_if_fail (!gedit_document_get_readonly (doc));

		remove_auto_save_timeout (tab);

		install_auto_save_timeout (tab);
	}
}
