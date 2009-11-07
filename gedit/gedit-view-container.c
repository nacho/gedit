/*
 * gedit-view-container.c
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
#include <gio/gio.h>

#include "gedit-app.h"
#include "gedit-notebook.h"
#include "gedit-view-container.h"
#include "gedit-utils.h"
#include "gedit-io-error-message-area.h"
#include "gedit-print-job.h"
#include "gedit-print-preview.h"
#include "gedit-progress-message-area.h"
#include "gedit-debug.h"
#include "gedit-prefs-manager-app.h"
#include "gedit-convert.h"
#include "gedit-enum-types.h"
#include "gedit-text-view.h"
#include "gedit-web-view.h"

#if !GTK_CHECK_VERSION (2, 17, 1)
#include "gedit-message-area.h"
#endif

#define GEDIT_VIEW_CONTAINER_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), GEDIT_TYPE_VIEW_CONTAINER,\
						 GeditViewContainerPrivate))

#define GEDIT_VIEW_CONTAINER_KEY "GEDIT_VIEW_CONTAINER_KEY"

struct _GeditViewContainerPrivate
{
	GeditViewContainerState state;
	GeditViewContainerMode  mode;
	
	GeditDocument	       *doc;
	
	GtkWidget	       *notebook;
	GList		       *views;
	GtkWidget	       *active_view;
	GtkWidget	       *active_view_scrolled_window;

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

	gint                    ask_if_externally_modified : 1;
};

/* Signals */
enum
{
	VIEW_ADDED,
	VIEW_REMOVED,
	ACTIVE_VIEW_CHANGED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE(GeditViewContainer, gedit_view_container, GTK_TYPE_VBOX)

enum
{
	PROP_0,
	PROP_DOCUMENT,
	PROP_MODE,
	PROP_NAME,
	PROP_STATE,
	PROP_AUTO_SAVE,
	PROP_AUTO_SAVE_INTERVAL
};

static gboolean gedit_view_container_auto_save (GeditViewContainer *container);

static void
install_auto_save_timeout (GeditViewContainer *container)
{
	gint timeout;

	gedit_debug (DEBUG_VIEW_CONTAINER);

	g_return_if_fail (container->priv->auto_save_timeout <= 0);
	g_return_if_fail (container->priv->auto_save);
	g_return_if_fail (container->priv->auto_save_interval > 0);
	
	g_return_if_fail (container->priv->state != GEDIT_VIEW_CONTAINER_STATE_LOADING);
	g_return_if_fail (container->priv->state != GEDIT_VIEW_CONTAINER_STATE_SAVING);
	g_return_if_fail (container->priv->state != GEDIT_VIEW_CONTAINER_STATE_REVERTING);
	g_return_if_fail (container->priv->state != GEDIT_VIEW_CONTAINER_STATE_LOADING_ERROR);
	g_return_if_fail (container->priv->state != GEDIT_VIEW_CONTAINER_STATE_SAVING_ERROR);
	g_return_if_fail (container->priv->state != GEDIT_VIEW_CONTAINER_STATE_SAVING_ERROR);
	g_return_if_fail (container->priv->state != GEDIT_VIEW_CONTAINER_STATE_REVERTING_ERROR);

	/* Add a new timeout */
	timeout = g_timeout_add_seconds (container->priv->auto_save_interval * 60,
					 (GSourceFunc) gedit_view_container_auto_save,
					 container);

	container->priv->auto_save_timeout = timeout;
}

static gboolean
install_auto_save_timeout_if_needed (GeditViewContainer *container)
{
	GeditDocument *doc;

	gedit_debug (DEBUG_VIEW_CONTAINER);
	
	g_return_val_if_fail (container->priv->auto_save_timeout <= 0, FALSE);
	g_return_val_if_fail ((container->priv->state == GEDIT_VIEW_CONTAINER_STATE_NORMAL) ||
			      (container->priv->state == GEDIT_VIEW_CONTAINER_STATE_SHOWING_PRINT_PREVIEW) ||
			      (container->priv->state == GEDIT_VIEW_CONTAINER_STATE_CLOSING), FALSE);

	if (container->priv->state == GEDIT_VIEW_CONTAINER_STATE_CLOSING)
		return FALSE;

	doc = gedit_view_container_get_document (container);

 	if (container->priv->auto_save && 
 	    !gedit_document_is_untitled (doc) &&
 	    !gedit_document_get_readonly (doc))
 	{ 
 		install_auto_save_timeout (container);
 		
 		return TRUE;
 	}
 	
 	return FALSE;
}

static void
remove_auto_save_timeout (GeditViewContainer *container)
{
	gedit_debug (DEBUG_VIEW_CONTAINER);

	/* FIXME: check sugli stati */
	
	g_return_if_fail (container->priv->auto_save_timeout > 0);
	
	g_source_remove (container->priv->auto_save_timeout);
	container->priv->auto_save_timeout = 0;
}

static void
gedit_view_container_get_property (GObject    *object,
				   guint       prop_id,
				   GValue     *value,
				   GParamSpec *pspec)
{
	GeditViewContainer *container = GEDIT_VIEW_CONTAINER (object);

	switch (prop_id)
	{
		case PROP_DOCUMENT:
			g_value_take_object (value,
					     gedit_view_container_get_document (container));
			break;
		case PROP_MODE:
			g_value_set_enum (value,
					  gedit_view_container_get_mode (container));
			break;
		case PROP_NAME:
			g_value_take_string (value,
					     _gedit_view_container_get_name (container));
			break;
		case PROP_STATE:
			g_value_set_enum (value,
					  gedit_view_container_get_state (container));
			break;
		case PROP_AUTO_SAVE:
			g_value_set_boolean (value,
					     gedit_view_container_get_auto_save_enabled (container));
			break;
		case PROP_AUTO_SAVE_INTERVAL:
			g_value_set_int (value,
					 gedit_view_container_get_auto_save_interval (container));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_view_container_set_property (GObject      *object,
				   guint         prop_id,
				   const GValue *value,
				   GParamSpec   *pspec)
{
	GeditViewContainer *container = GEDIT_VIEW_CONTAINER (object);

	switch (prop_id)
	{
		case PROP_AUTO_SAVE:
			gedit_view_container_set_auto_save_enabled (container,
								    g_value_get_boolean (value));
			break;
		case PROP_AUTO_SAVE_INTERVAL:
			gedit_view_container_set_auto_save_interval (container,
								     g_value_get_int (value));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_view_container_finalize (GObject *object)
{
	GeditViewContainer *container = GEDIT_VIEW_CONTAINER (object);

	if (container->priv->timer != NULL)
		g_timer_destroy (container->priv->timer);

	g_free (container->priv->tmp_save_uri);

	if (container->priv->auto_save_timeout > 0)
		remove_auto_save_timeout (container);

	g_list_free (container->priv->views);

	G_OBJECT_CLASS (gedit_view_container_parent_class)->finalize (object);
}

static void 
gedit_view_container_class_init (GeditViewContainerClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = gedit_view_container_finalize;
	object_class->get_property = gedit_view_container_get_property;
	object_class->set_property = gedit_view_container_set_property;
	
	g_object_class_install_property (object_class,
					 PROP_DOCUMENT,
					 g_param_spec_object ("document",
							      "Document",
							      "The container's document",
							      GEDIT_TYPE_DOCUMENT,
							      G_PARAM_READABLE));
	
	g_object_class_install_property (object_class,
					 PROP_MODE,
					 g_param_spec_enum ("mode",
							    "Mode",
							    "The container's mode",
							    GEDIT_TYPE_VIEW_CONTAINER_MODE,
							    GEDIT_VIEW_CONTAINER_MODE_TEXT,
							    G_PARAM_READABLE |
							    G_PARAM_STATIC_STRINGS));
	
	g_object_class_install_property (object_class,
					 PROP_NAME,
					 g_param_spec_string ("name",
							      "Name",
							      "The container's name",
							      NULL,
							      G_PARAM_READABLE |
							      G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (object_class,
					 PROP_STATE,
					 g_param_spec_enum ("state",
							    "State",
							    "The container's state",
							    GEDIT_TYPE_VIEW_CONTAINER_STATE,
							    GEDIT_VIEW_CONTAINER_STATE_NORMAL,
							    G_PARAM_READABLE |
							    G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (object_class,
					 PROP_AUTO_SAVE,
					 g_param_spec_boolean ("autosave",
							       "Autosave",
							       "Autosave feature",
							       TRUE,
							       G_PARAM_READWRITE |
							       G_PARAM_STATIC_STRINGS));

	g_object_class_install_property (object_class,
					 PROP_AUTO_SAVE_INTERVAL,
					 g_param_spec_int ("autosave-interval",
							   "AutosaveInterval",
							   "Time between two autosaves",
							   0,
							   G_MAXINT,
							   0,
							   G_PARAM_READWRITE |
							   G_PARAM_STATIC_STRINGS));

	signals[VIEW_ADDED] =
		g_signal_new ("view-added",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (GeditViewContainerClass, view_added),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE,
			      1,
			      GEDIT_TYPE_VIEW);

	signals[VIEW_ADDED] =
		g_signal_new ("view-removed",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (GeditViewContainerClass, view_removed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE,
			      1,
			      GEDIT_TYPE_VIEW);

	signals[ACTIVE_VIEW_CHANGED] =
		g_signal_new ("active-view-changed",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (GeditViewContainerClass, active_view_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE,
			      1,
			      GEDIT_TYPE_VIEW);

	g_type_class_add_private (object_class, sizeof (GeditViewContainerPrivate));
}

/**
 * gedit_view_container_get_state:
 * @container: a #GeditViewContainer
 *
 * Gets the #GeditViewContainerState of @container.
 *
 * Returns: the #GeditViewContainerState of @container
 */
GeditViewContainerState
gedit_view_container_get_state (GeditViewContainer *container)
{
	g_return_val_if_fail (GEDIT_IS_VIEW_CONTAINER (container), GEDIT_VIEW_CONTAINER_STATE_NORMAL);
	
	return container->priv->state;
}

GeditViewContainerMode
gedit_view_container_get_mode (GeditViewContainer *container)
{
	g_return_val_if_fail (GEDIT_IS_VIEW_CONTAINER (container), GEDIT_VIEW_CONTAINER_MODE_TEXT);
	
	return container->priv->mode;
}

static void
set_cursor_according_to_state (GtkTextView   *view,
			       GeditViewContainerState  state)
{
	GdkCursor *cursor;
	GdkWindow *text_window;
	GdkWindow *left_window;

	text_window = gtk_text_view_get_window (view, GTK_TEXT_WINDOW_TEXT);
	left_window = gtk_text_view_get_window (view, GTK_TEXT_WINDOW_LEFT);

	if ((state == GEDIT_VIEW_CONTAINER_STATE_LOADING)          ||
	    (state == GEDIT_VIEW_CONTAINER_STATE_REVERTING)        ||
	    (state == GEDIT_VIEW_CONTAINER_STATE_SAVING)           ||
	    (state == GEDIT_VIEW_CONTAINER_STATE_PRINTING)         ||
	    (state == GEDIT_VIEW_CONTAINER_STATE_PRINT_PREVIEWING) ||
	    (state == GEDIT_VIEW_CONTAINER_STATE_CLOSING))
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
view_realized (GtkWidget *view,
	       GeditViewContainer  *container)
{
	/* FIXME: Maybe we should take care of all views? */
	if (GTK_IS_TEXT_VIEW (view))
		set_cursor_according_to_state (GTK_TEXT_VIEW (view), container->priv->state);
}

static void
set_view_properties_according_to_state (GeditViewContainer      *container,
					GeditViewContainerState  state)
{
	gboolean val;

	val = ((state == GEDIT_VIEW_CONTAINER_STATE_NORMAL) &&
	       (container->priv->print_preview == NULL) &&
	       !container->priv->not_editable);
	gedit_view_set_editable (GEDIT_VIEW (container->priv->active_view), val);

	if (GEDIT_IS_TEXT_VIEW (GEDIT_TEXT_VIEW (container->priv->active_view)))
	{
		val = ((state != GEDIT_VIEW_CONTAINER_STATE_LOADING) &&
		       (state != GEDIT_VIEW_CONTAINER_STATE_CLOSING));
		gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (container->priv->active_view), val);


		val = ((state != GEDIT_VIEW_CONTAINER_STATE_LOADING) &&
		       (state != GEDIT_VIEW_CONTAINER_STATE_CLOSING) &&
		       (gedit_prefs_manager_get_highlight_current_line ()));
		gtk_source_view_set_highlight_current_line (GTK_SOURCE_VIEW (container->priv->active_view), val);
	}
}

static void
gedit_view_container_set_state (GeditViewContainer      *container,
				GeditViewContainerState  state)
{
	g_return_if_fail (GEDIT_IS_VIEW_CONTAINER (container));
	g_return_if_fail ((state >= 0) && (state < GEDIT_VIEW_CONTAINER_NUM_OF_STATES));

	if (container->priv->state == state)
		return;

	container->priv->state = state;

	set_view_properties_according_to_state (container, state);

	if ((state == GEDIT_VIEW_CONTAINER_STATE_LOADING_ERROR) || /* FIXME: add other states if needed */
	    (state == GEDIT_VIEW_CONTAINER_STATE_SHOWING_PRINT_PREVIEW))
	{
		gtk_widget_hide (container->priv->active_view_scrolled_window);
	}
	else
	{
		if (container->priv->print_preview == NULL)
			gtk_widget_show (container->priv->active_view_scrolled_window);
	}

	/* FIXME: Should we take care of all views? */
	if (GTK_IS_TEXT_VIEW (container->priv->active_view))
		set_cursor_according_to_state (GTK_TEXT_VIEW (container->priv->active_view),
					       state);

	g_object_notify (G_OBJECT (container), "state");
}

static void 
document_uri_notify_handler (GeditDocument      *document,
			     GParamSpec         *pspec,
			     GeditViewContainer *container)
{
	gedit_debug (DEBUG_VIEW_CONTAINER);
	
	/* Notify the change in the URI */
	g_object_notify (G_OBJECT (container), "name");
}

static void
document_modified_changed (GtkTextBuffer      *document,
			   GeditViewContainer *container)
{
	g_object_notify (G_OBJECT (container), "name");
}

static void
set_message_area (GeditViewContainer  *container,
		  GtkWidget *message_area)
{
	if (container->priv->message_area == message_area)
		return;

	if (container->priv->message_area != NULL)
		gtk_widget_destroy (container->priv->message_area);

	container->priv->message_area = message_area;

	if (message_area == NULL)
		return;

	gtk_box_pack_start (GTK_BOX (container),
			    container->priv->message_area,
			    FALSE,
			    FALSE,
			    0);

	g_object_add_weak_pointer (G_OBJECT (container->priv->message_area), 
				   (gpointer *)&container->priv->message_area);
}

/* FIXME: This can't be here when we add GeditPage */
static void
remove_container (GeditViewContainer *container)
{
	/*GeditNotebook *notebook;

	notebook = GEDIT_NOTEBOOK (gtk_widget_get_parent (GTK_WIDGET (container)));

	gedit_notebook_remove_page (notebook, container);*/
}

static void 
io_loading_error_message_area_response (GtkWidget          *message_area,
					gint                response_id,
					GeditViewContainer *container)
{
	if (response_id == GTK_RESPONSE_OK)
	{
		GeditDocument *doc;
		gchar *uri;

		doc = gedit_view_container_get_document (container);
		g_return_if_fail (GEDIT_IS_DOCUMENT (doc));

		uri = gedit_document_get_uri (doc);
		g_return_if_fail (uri != NULL);

		set_message_area (container, NULL);
		gedit_view_container_set_state (container, GEDIT_VIEW_CONTAINER_STATE_LOADING);

		g_return_if_fail (container->priv->auto_save_timeout <= 0);

		gedit_document_load (doc,
				     uri,
				     container->priv->tmp_encoding,
				     container->priv->tmp_line_pos,
				     FALSE);
	}
	else
	{
		remove_container (container);
	}
}

static void 
conversion_loading_error_message_area_response (GtkWidget          *message_area,
						gint                response_id,
						GeditViewContainer *container)
{
	GeditDocument *doc;
	gchar *uri;

	doc = gedit_view_container_get_document (container);
	g_return_if_fail (GEDIT_IS_DOCUMENT (doc));

	uri = gedit_document_get_uri (doc);
	g_return_if_fail (uri != NULL);

	if (response_id == GTK_RESPONSE_OK)
	{
		const GeditEncoding *encoding;

		encoding = gedit_conversion_error_message_area_get_encoding (
				GTK_WIDGET (message_area));

		g_return_if_fail (encoding != NULL);

		set_message_area (container, NULL);
		gedit_view_container_set_state (container, GEDIT_VIEW_CONTAINER_STATE_LOADING);

		container->priv->tmp_encoding = encoding;

		g_return_if_fail (container->priv->auto_save_timeout <= 0);

		gedit_document_load (doc,
				     uri,
				     encoding,
				     container->priv->tmp_line_pos,
				     FALSE);
	}
	else
	{
		_gedit_recent_remove (GEDIT_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (container))), uri);

		remove_container (container);
	}

	g_free (uri);
}

static void 
file_already_open_warning_message_area_response (GtkWidget          *message_area,
						 gint                response_id,
						 GeditViewContainer *container)
{
	GeditView *view;
	
	view = gedit_view_container_get_view (container);
	
	if (response_id == GTK_RESPONSE_YES)
	{
		container->priv->not_editable = FALSE;
		
		gedit_view_set_editable (GEDIT_VIEW (view),
					 TRUE);
	}

	gtk_widget_destroy (message_area);

	gtk_widget_grab_focus (GTK_WIDGET (view));
}

static void
load_cancelled (GtkWidget          *area,
                gint                response_id,
                GeditViewContainer *container)
{
	g_return_if_fail (GEDIT_IS_PROGRESS_MESSAGE_AREA (container->priv->message_area));
	
	g_object_ref (container);
	gedit_document_load_cancel (gedit_view_container_get_document (container));
	g_object_unref (container);
}

static void 
unrecoverable_reverting_error_message_area_response (GtkWidget          *message_area,
						     gint                response_id,
						     GeditViewContainer *container)
{
	GeditView *view;
	
	gedit_view_container_set_state (container,
			     GEDIT_VIEW_CONTAINER_STATE_NORMAL);

	set_message_area (container, NULL);

	view = gedit_view_container_get_view (container);

	gtk_widget_grab_focus (GTK_WIDGET (view));
	
	install_auto_save_timeout_if_needed (container);
}

#define MAX_MSG_LENGTH 100

static void
show_loading_message_area (GeditViewContainer *container)
{
	GtkWidget *area;
	GeditDocument *doc = NULL;
	gchar *name;
	gchar *dirname = NULL;
	gchar *msg = NULL;
	gchar *name_markup;
	gchar *dirname_markup;
	gint len;

	if (container->priv->message_area != NULL)
		return;

	gedit_debug (DEBUG_VIEW_CONTAINER);
		
	doc = gedit_view_container_get_document (container);
	g_return_if_fail (doc != NULL);

	name = gedit_document_get_short_name_for_display (doc);
	len = g_utf8_strlen (name, -1);

	/* if the name is awfully long, truncate it and be done with it,
	 * otherwise also show the directory (ellipsized if needed)
	 */
	if (len > MAX_MSG_LENGTH)
	{
		gchar *str;

		str = gedit_utils_str_middle_truncate (name, MAX_MSG_LENGTH);
		g_free (name);
		name = str;
	}
	else
	{
		GFile *file;

		file = gedit_document_get_location (doc);
		if (file != NULL)
		{
			gchar *str;

			str = gedit_utils_location_get_dirname_for_display (file);
			g_object_unref (file);

			/* use the remaining space for the dir, but use a min of 20 chars
			 * so that we do not end up with a dirname like "(a...b)".
			 * This means that in the worst case when the filename is long 99
			 * we have a title long 99 + 20, but I think it's a rare enough
			 * case to be accepcontainerle. It's justa darn title afterall :)
			 */
			dirname = gedit_utils_str_middle_truncate (str, 
								   MAX (20, MAX_MSG_LENGTH - len));
			g_free (str);
		}
	}

	name_markup = g_markup_printf_escaped ("<b>%s</b>", name);

	if (container->priv->state == GEDIT_VIEW_CONTAINER_STATE_REVERTING)
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
			  container);

	gtk_widget_show (area);

	set_message_area (container, area);

	g_free (msg);
	g_free (name);
	g_free (name_markup);
	g_free (dirname);
}

static void
show_saving_message_area (GeditViewContainer *container)
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

	g_return_if_fail (container->priv->tmp_save_uri != NULL);
	
	if (container->priv->message_area != NULL)
		return;
	
	gedit_debug (DEBUG_VIEW_CONTAINER);
	
	doc = gedit_view_container_get_document (container);
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

		to = gedit_utils_uri_for_display (container->priv->tmp_save_uri);

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

	set_message_area (container, area);

	g_free (msg);
	g_free (to);
	g_free (from);
	g_free (from_markup);
}

static void
message_area_set_progress (GeditViewContainer *container,
			   goffset   size,
			   goffset   total_size)
{
	if (container->priv->message_area == NULL)
		return;

	gedit_debug_message (DEBUG_VIEW_CONTAINER, "%" G_GUINT64_FORMAT "/%" G_GUINT64_FORMAT, size, total_size);

	g_return_if_fail (GEDIT_IS_PROGRESS_MESSAGE_AREA (container->priv->message_area));
	
	if (total_size == 0)
	{
		if (size != 0)
			gedit_progress_message_area_pulse (
					GEDIT_PROGRESS_MESSAGE_AREA (container->priv->message_area));
		else
			gedit_progress_message_area_set_fraction (
				GEDIT_PROGRESS_MESSAGE_AREA (container->priv->message_area),
				0);
	}
	else
	{
		gdouble frac;

		frac = (gdouble)size / (gdouble)total_size;

		gedit_progress_message_area_set_fraction (
				GEDIT_PROGRESS_MESSAGE_AREA (container->priv->message_area),
				frac);
	}
}

static void
document_loading (GeditDocument      *document,
		  goffset             size,
		  goffset             total_size,
		  GeditViewContainer *container)
{
	double et;

	g_return_if_fail ((container->priv->state == GEDIT_VIEW_CONTAINER_STATE_LOADING) ||
		 	  (container->priv->state == GEDIT_VIEW_CONTAINER_STATE_REVERTING));

	gedit_debug_message (DEBUG_VIEW_CONTAINER, "%" G_GUINT64_FORMAT "/%" G_GUINT64_FORMAT, size, total_size);

	if (container->priv->timer == NULL)
	{
		g_return_if_fail (container->priv->times_called == 0);
		container->priv->timer = g_timer_new ();
	}

	et = g_timer_elapsed (container->priv->timer, NULL);

	if (container->priv->times_called == 1)
	{
		if ((total_size == 0) || (total_size > 51200UL) /* 50 KB */)
		{
			show_loading_message_area (container);
		}
	}
	else
	{
		if ((container->priv->times_called == 3) && (total_size != 0))
		{
			gdouble total_time;

			/* et : total_time = size : total_size */
			total_time = (et * total_size)/size;

			if ((total_time - et) > 3.0)
			{
				show_loading_message_area (container);
			}
		}
		else
		{
			if (et > 3.0)
			{
				show_loading_message_area (container);
			}
		}
	}
	
	message_area_set_progress (container, size, total_size);

	container->priv->times_called++;
}

static gboolean
remove_container_idle (GeditViewContainer *container)
{
	remove_container (container);

	return FALSE;
}

static void
document_loaded (GeditDocument      *document,
		 const GError       *error,
		 GeditViewContainer *container)
{
	GtkWidget *emsg;
	GFile *location;
	gchar *uri;
	const GeditEncoding *encoding;

	g_return_if_fail ((container->priv->state == GEDIT_VIEW_CONTAINER_STATE_LOADING) ||
			  (container->priv->state == GEDIT_VIEW_CONTAINER_STATE_REVERTING));
	g_return_if_fail (container->priv->auto_save_timeout <= 0);

	if (container->priv->timer != NULL)
	{
		g_timer_destroy (container->priv->timer);
		container->priv->timer = NULL;
	}
	container->priv->times_called = 0;

	set_message_area (container, NULL);

	location = gedit_document_get_location (document);
	uri = gedit_document_get_uri (document);

	if (error != NULL)
	{
		if (container->priv->state == GEDIT_VIEW_CONTAINER_STATE_LOADING)
			gedit_view_container_set_state (container, GEDIT_VIEW_CONTAINER_STATE_LOADING_ERROR);
		else
			gedit_view_container_set_state (container, GEDIT_VIEW_CONTAINER_STATE_REVERTING_ERROR);

		encoding = gedit_document_get_encoding (document);

		if (error->domain == G_IO_ERROR && 
		    error->code == G_IO_ERROR_CANCELLED)
		{
			/* remove the container, but in an idle handler, since
			 * we are in the handler of doc loaded and we 
			 * don't want doc and container to be finalized now.
			 */
			g_idle_add ((GSourceFunc) remove_container_idle, container);

			goto end;
		}
		else if (error->domain == G_IO_ERROR || 
			 error->domain == GEDIT_DOCUMENT_ERROR)
		{
			_gedit_recent_remove (GEDIT_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (container))), uri);

			if (container->priv->state == GEDIT_VIEW_CONTAINER_STATE_LOADING_ERROR)
			{
				emsg = gedit_io_loading_error_message_area_new (uri, 
										error);
				g_signal_connect (emsg,
						  "response",
						  G_CALLBACK (io_loading_error_message_area_response),
						  container);
			}
			else
			{
				g_return_if_fail (container->priv->state == GEDIT_VIEW_CONTAINER_STATE_REVERTING_ERROR);
				
				emsg = gedit_unrecoverable_reverting_error_message_area_new (uri, 
											     error);

				g_signal_connect (emsg,
						  "response",
						  G_CALLBACK (unrecoverable_reverting_error_message_area_response),
						  container);
			}

			set_message_area (container, emsg);
		}
		else
		{
			g_return_if_fail ((error->domain == G_CONVERT_ERROR) ||
			      		  (error->domain == GEDIT_CONVERT_ERROR));

			// TODO: different error messages if container->priv->state == GEDIT_VIEW_CONTAINER_STATE_REVERTING?
			// note that while reverting encoding should be ok, so this is unlikely to happen
			emsg = gedit_conversion_error_while_loading_message_area_new (
									uri,
									container->priv->tmp_encoding,
									error);

			set_message_area (container, emsg);

			g_signal_connect (emsg,
					  "response",
					  G_CALLBACK (conversion_loading_error_message_area_response),
					  container);
		}

#if !GTK_CHECK_VERSION (2, 17, 1)
		gedit_message_area_set_default_response (GEDIT_MESSAGE_AREA (emsg),
							 GTK_RESPONSE_CANCEL);
#else
		gtk_info_bar_set_default_response (GTK_INFO_BAR (emsg),
						   GTK_RESPONSE_CANCEL);
#endif

		gtk_widget_show (emsg);

		g_object_unref (location);
		g_free (uri);

		return;
	}
	else
	{
		gchar *mime;
		GList *all_documents;
		GList *l;

		g_return_if_fail (uri != NULL);

		mime = gedit_document_get_mime_type (document);
		_gedit_recent_add (GEDIT_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (container))),
				   uri,
				   mime);
		g_free (mime);
		
		/* Scroll to the cursor when the document is loaded */
		for (l = container->priv->views; l != NULL; l = g_list_next (l))
		{
			gedit_view_scroll_to_cursor (GEDIT_VIEW (l->data));
		}

		all_documents = gedit_app_get_documents (gedit_app_get_default ());

		for (l = all_documents; l != NULL; l = g_list_next (l))
		{
			GeditDocument *d = GEDIT_DOCUMENT (l->data);
			
			if (d != document)
			{
				GFile *loc;

				loc = gedit_document_get_location (d);

				if ((loc != NULL) &&
			    	    g_file_equal (location, loc))
			    	{
			    		GtkWidget *w;
			    		GeditView *view;

			    		view = gedit_view_container_get_view (container);

			    		container->priv->not_editable = TRUE;

			    		w = gedit_file_already_open_warning_message_area_new (uri);

					set_message_area (container, w);

#if !GTK_CHECK_VERSION (2, 17, 1)
					gedit_message_area_set_default_response (GEDIT_MESSAGE_AREA (w),
										 GTK_RESPONSE_CANCEL);
#else
					gtk_info_bar_set_default_response (GTK_INFO_BAR (w),
									   GTK_RESPONSE_CANCEL);
#endif

					gtk_widget_show (w);

					g_signal_connect (w,
							  "response",
							  G_CALLBACK (file_already_open_warning_message_area_response),
							  container);

			    		g_object_unref (loc);
			    		break;
			    	}
			    	
			    	if (loc != NULL)
					g_object_unref (loc);
			}
		}

		g_list_free (all_documents);

		gedit_view_container_set_state (container, GEDIT_VIEW_CONTAINER_STATE_NORMAL);
		
		install_auto_save_timeout_if_needed (container);

		container->priv->ask_if_externally_modified = TRUE;
	}

 end:
	g_object_unref (location);
	g_free (uri);

	container->priv->tmp_line_pos = 0;
	container->priv->tmp_encoding = NULL;
}

static void
document_saving (GeditDocument      *document,
		 goffset             size,
		 goffset             total_size,
		 GeditViewContainer *container)
{
	double et;

	g_return_if_fail (container->priv->state == GEDIT_VIEW_CONTAINER_STATE_SAVING);

	gedit_debug_message (DEBUG_VIEW_CONTAINER, "%" G_GUINT64_FORMAT "/%" G_GUINT64_FORMAT, size, total_size);


	if (container->priv->timer == NULL)
	{
		g_return_if_fail (container->priv->times_called == 0);
		container->priv->timer = g_timer_new ();
	}

	et = g_timer_elapsed (container->priv->timer, NULL);

	if (container->priv->times_called == 1)
	{
		if ((total_size == 0) || (total_size > 51200UL) /* 50 KB */)
		{
			show_saving_message_area (container);
		}
	}
	else
	{
		if ((container->priv->times_called == 3) && (total_size != 0))
		{
			gdouble total_time;

			/* et : total_time = size : total_size */
			total_time = (et * total_size)/size;

			if ((total_time - et) > 3.0)
			{
				show_saving_message_area (container);
			}
		}
		else
		{
			if (et > 3.0)
			{
				show_saving_message_area (container);
			}
		}
	}
	
	message_area_set_progress (container, size, total_size);

	container->priv->times_called++;
}

static void
end_saving (GeditViewContainer *container)
{
	/* Reset tmp data for saving */
	g_free (container->priv->tmp_save_uri);
	container->priv->tmp_save_uri = NULL;
	container->priv->tmp_encoding = NULL;
	
	install_auto_save_timeout_if_needed (container);
}

static void 
unrecoverable_saving_error_message_area_response (GtkWidget          *message_area,
						  gint                response_id,
						  GeditViewContainer *container)
{
	GeditView *view;
	
	if (container->priv->print_preview != NULL)
		gedit_view_container_set_state (container, GEDIT_VIEW_CONTAINER_STATE_SHOWING_PRINT_PREVIEW);
	else
		gedit_view_container_set_state (container, GEDIT_VIEW_CONTAINER_STATE_NORMAL);

	end_saving (container);
	
	set_message_area (container, NULL);

	view = gedit_view_container_get_view (container);

	gtk_widget_grab_focus (GTK_WIDGET (view));
}

static void 
no_backup_error_message_area_response (GtkWidget          *message_area,
				       gint                response_id,
				       GeditViewContainer *container)
{
	if (response_id == GTK_RESPONSE_YES)
	{
		GeditDocument *doc;

		doc = gedit_view_container_get_document (container);
		g_return_if_fail (GEDIT_IS_DOCUMENT (doc));

		set_message_area (container, NULL);

		g_return_if_fail (container->priv->tmp_save_uri != NULL);
		g_return_if_fail (container->priv->tmp_encoding != NULL);

		gedit_view_container_set_state (container, GEDIT_VIEW_CONTAINER_STATE_SAVING);

		/* don't bug the user again with this... */
		container->priv->save_flags |= GEDIT_DOCUMENT_SAVE_IGNORE_BACKUP;

		g_return_if_fail (container->priv->auto_save_timeout <= 0);
		
		/* Force saving */
		gedit_document_save (doc, container->priv->save_flags);
	}
	else
	{
		unrecoverable_saving_error_message_area_response (message_area,
								  response_id,
								  container);
	}
}

static void
externally_modified_error_message_area_response (GtkWidget          *message_area,
						 gint                response_id,
						 GeditViewContainer *container)
{
	if (response_id == GTK_RESPONSE_YES)
	{
		GeditDocument *doc;

		doc = gedit_view_container_get_document (container);
		g_return_if_fail (GEDIT_IS_DOCUMENT (doc));
		
		set_message_area (container, NULL);

		g_return_if_fail (container->priv->tmp_save_uri != NULL);
		g_return_if_fail (container->priv->tmp_encoding != NULL);

		gedit_view_container_set_state (container, GEDIT_VIEW_CONTAINER_STATE_SAVING);

		g_return_if_fail (container->priv->auto_save_timeout <= 0);
		
		/* ignore mtime should not be persisted in save flags across saves */

		/* Force saving */
		gedit_document_save (doc, container->priv->save_flags | GEDIT_DOCUMENT_SAVE_IGNORE_MTIME);
	}
	else
	{		
		unrecoverable_saving_error_message_area_response (message_area,
								  response_id,
								  container);
	}
}

static void 
recoverable_saving_error_message_area_response (GtkWidget          *message_area,
						gint                response_id,
						GeditViewContainer *container)
{
	GeditDocument *doc;

	doc = gedit_view_container_get_document (container);
	g_return_if_fail (GEDIT_IS_DOCUMENT (doc));
	
	if (response_id == GTK_RESPONSE_OK)
	{
		const GeditEncoding *encoding;
		
		encoding = gedit_conversion_error_message_area_get_encoding (
									GTK_WIDGET (message_area));

		g_return_if_fail (encoding != NULL);

		set_message_area (container, NULL);

		g_return_if_fail (container->priv->tmp_save_uri != NULL);
				
		gedit_view_container_set_state (container, GEDIT_VIEW_CONTAINER_STATE_SAVING);
			
		container->priv->tmp_encoding = encoding;

		gedit_debug_message (DEBUG_VIEW_CONTAINER, "Force saving with URI '%s'", container->priv->tmp_save_uri);
			 
		g_return_if_fail (container->priv->auto_save_timeout <= 0);
		
		gedit_document_save_as (doc,
					container->priv->tmp_save_uri,
					container->priv->tmp_encoding,
					container->priv->save_flags);
	}
	else
	{		
		unrecoverable_saving_error_message_area_response (message_area,
								  response_id,
								  container);
	}
}

static void
document_saved (GeditDocument      *document,
		const GError       *error,
		GeditViewContainer *container)
{
	GtkWidget *emsg;

	g_return_if_fail (container->priv->state == GEDIT_VIEW_CONTAINER_STATE_SAVING);

	g_return_if_fail (container->priv->tmp_save_uri != NULL);
	g_return_if_fail (container->priv->tmp_encoding != NULL);
	g_return_if_fail (container->priv->auto_save_timeout <= 0);
	
	g_timer_destroy (container->priv->timer);
	container->priv->timer = NULL;
	container->priv->times_called = 0;
	
	set_message_area (container, NULL);
	
	if (error != NULL)
	{
		gedit_view_container_set_state (container, GEDIT_VIEW_CONTAINER_STATE_SAVING_ERROR);
		
		if (error->domain == GEDIT_DOCUMENT_ERROR &&
		    error->code == GEDIT_DOCUMENT_ERROR_EXTERNALLY_MODIFIED)
		{
			/* This error is recoverable */
			emsg = gedit_externally_modified_saving_error_message_area_new (
							container->priv->tmp_save_uri,
							error);
			g_return_if_fail (emsg != NULL);

			set_message_area (container, emsg);

			g_signal_connect (emsg,
					  "response",
					  G_CALLBACK (externally_modified_error_message_area_response),
					  container);
		}
		else if ((error->domain == GEDIT_DOCUMENT_ERROR &&
			  error->code == GEDIT_DOCUMENT_ERROR_CANT_CREATE_BACKUP) ||
		         (error->domain == G_IO_ERROR &&
			  error->code == G_IO_ERROR_CANT_CREATE_BACKUP))
		{
			/* This error is recoverable */
			emsg = gedit_no_backup_saving_error_message_area_new (
							container->priv->tmp_save_uri,
							error);
			g_return_if_fail (emsg != NULL);

			set_message_area (container, emsg);

			g_signal_connect (emsg,
					  "response",
					  G_CALLBACK (no_backup_error_message_area_response),
					  container);
		}
		else if (error->domain == GEDIT_DOCUMENT_ERROR || 
			 error->domain == G_IO_ERROR)
		{
			/* These errors are _NOT_ recoverable */
			_gedit_recent_remove  (GEDIT_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (container))),
					       container->priv->tmp_save_uri);

			emsg = gedit_unrecoverable_saving_error_message_area_new (container->priv->tmp_save_uri,
										  error);
			g_return_if_fail (emsg != NULL);
	
			set_message_area (container, emsg);

			g_signal_connect (emsg,
					  "response",
					  G_CALLBACK (unrecoverable_saving_error_message_area_response),
					  container);
		}
		else
		{
			/* This error is recoverable */
			g_return_if_fail (error->domain == G_CONVERT_ERROR);

			emsg = gedit_conversion_error_while_saving_message_area_new (
									container->priv->tmp_save_uri,
									container->priv->tmp_encoding,
									error);

			set_message_area (container, emsg);

			g_signal_connect (emsg,
					  "response",
					  G_CALLBACK (recoverable_saving_error_message_area_response),
					  container);
		}

#if !GTK_CHECK_VERSION (2, 17, 1)
		gedit_message_area_set_default_response (GEDIT_MESSAGE_AREA (emsg),
							 GTK_RESPONSE_CANCEL);
#else
		gtk_info_bar_set_default_response (GTK_INFO_BAR (emsg),
						   GTK_RESPONSE_CANCEL);
#endif

		gtk_widget_show (emsg);
	}
	else
	{
		gchar *mime = gedit_document_get_mime_type (document);

		_gedit_recent_add (GEDIT_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (container))),
				   container->priv->tmp_save_uri,
				   mime);
		g_free (mime);

		if (container->priv->print_preview != NULL)
			gedit_view_container_set_state (container, GEDIT_VIEW_CONTAINER_STATE_SHOWING_PRINT_PREVIEW);
		else
			gedit_view_container_set_state (container, GEDIT_VIEW_CONTAINER_STATE_NORMAL);

		container->priv->ask_if_externally_modified = TRUE;
			
		end_saving (container);
	}
}

static void 
externally_modified_notification_message_area_response (GtkWidget          *message_area,
							gint                response_id,
							GeditViewContainer *container)
{
	GeditView *view;

	set_message_area (container, NULL);
	view = gedit_view_container_get_view (container);

	if (response_id == GTK_RESPONSE_OK)
	{
		_gedit_view_container_revert (container);
	}
	else
	{
		container->priv->ask_if_externally_modified = FALSE;

		/* go back to normal state */
		gedit_view_container_set_state (container, GEDIT_VIEW_CONTAINER_STATE_NORMAL);
	}

	gtk_widget_grab_focus (GTK_WIDGET (view));
}

static void
display_externally_modified_notification (GeditViewContainer *container)
{
	GtkWidget *message_area;
	GeditDocument *doc;
	gchar *uri;
	gboolean document_modified;

	doc = gedit_view_container_get_document (container);
	g_return_if_fail (GEDIT_IS_DOCUMENT (doc));

	/* uri cannot be NULL, we're here because
	 * the file we're editing changed on disk */
	uri = gedit_document_get_uri (doc);
	g_return_if_fail (uri != NULL);

	document_modified = gedit_document_get_modified (doc);
	message_area = gedit_externally_modified_message_area_new (uri, document_modified);
	g_free (uri);

	container->priv->message_area = NULL;
	set_message_area (container, message_area);
	gtk_widget_show (message_area);

	g_signal_connect (message_area,
			  "response",
			  G_CALLBACK (externally_modified_notification_message_area_response),
			  container);
}

static gboolean
view_focused_in (GtkWidget          *widget,
                 GdkEventFocus      *event,
                 GeditViewContainer *container)
{
	GeditDocument *doc;

	g_return_val_if_fail (GEDIT_IS_VIEW_CONTAINER (container), FALSE);

	/* we try to detect file changes only in the normal state */
	if (container->priv->state != GEDIT_VIEW_CONTAINER_STATE_NORMAL)
	{
		return FALSE;
	}

	/* we already asked, don't bug the user again */
	if (!container->priv->ask_if_externally_modified)
	{
		return FALSE;
	}

	doc = gedit_view_container_get_document (container);

	/* If file was never saved or is remote we do not check */
	if (!gedit_document_is_local (doc))
	{
		return FALSE;
	}

	if (gedit_document_check_externally_modified (doc))
	{
		gedit_view_container_set_state (container, GEDIT_VIEW_CONTAINER_STATE_EXTERNALLY_MODIFIED_NOTIFICATION);

		display_externally_modified_notification (container);

		return FALSE;
	}

	return FALSE;
}
/*
static GMountOperation *
container_mount_operation_factory (GeditDocument *doc,
			     gpointer userdata)
{
	GeditViewContainer *container = GEDIT_VIEW_CONTAINER (userdata);
	GtkWidget *window;

	window = gtk_widget_get_toplevel (GTK_WIDGET (container));
	return gtk_mount_operation_new (GTK_WINDOW (window));
}*/

static void
install_view (GeditViewContainer *container,
	      GtkWidget          *view)
{
	GtkWidget *sw;

	container->priv->views = g_list_append (container->priv->views, view);
	if (!GTK_WIDGET_VISIBLE (view))
		gtk_widget_show (view);
	
	g_object_set_data (G_OBJECT (view), GEDIT_VIEW_CONTAINER_KEY, container);

	/* FIXME: We need to know if we need viewport */
	/* Create the scrolled window */
	sw = gtk_scrolled_window_new (NULL, NULL);

	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	
	gtk_container_add (GTK_CONTAINER (sw), view);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw),
					     GTK_SHADOW_IN);
	gtk_widget_show (sw);
	
	/* Add view to notebook */
	gtk_notebook_append_page (GTK_NOTEBOOK (container->priv->notebook),
				  sw, NULL);

	g_signal_connect_after (view,
				"focus-in-event",
				G_CALLBACK (view_focused_in),
				container);

	g_signal_connect_after (view,
				"realize",
				G_CALLBACK (view_realized),
				container);

	g_signal_emit (G_OBJECT (container),
		       signals[VIEW_ADDED],
		       0,
		       view);
}

static void
uninstall_view (GeditViewContainer *container,
		GtkWidget          *view)
{
	GtkWidget *sw;
	gint page_num;
	
	/* FIXME: Check that the parent is indeed a scrolled window */
	sw = gtk_widget_get_parent (view);
	
	page_num = gtk_notebook_page_num (GTK_NOTEBOOK (container->priv->notebook),
					  sw);

	g_object_ref (view);
	
	gtk_notebook_remove_page (GTK_NOTEBOOK (container->priv->notebook),
				  page_num);

	g_signal_emit (G_OBJECT (container),
		       signals[VIEW_REMOVED],
		       0,
		       view);
	
	g_object_unref (view);
}

static void
on_switch_page_cb (GtkNotebook        *notebook,
		   GtkNotebookPage    *page,
		   guint               page_num,
		   GeditViewContainer *container)
{
	GtkWidget *sw;
	GtkWidget *view;

	sw = gtk_notebook_get_nth_page (notebook, page_num);
	
	if (sw == container->priv->active_view_scrolled_window)
		return;

	view = gtk_bin_get_child (GTK_BIN (sw));

	container->priv->active_view = view;
	container->priv->active_view_scrolled_window = sw;

	g_signal_emit (G_OBJECT (container),
		       signals[ACTIVE_VIEW_CHANGED],
		       0,
		       container->priv->active_view);
}

static void
gedit_view_container_init (GeditViewContainer *container)
{
	GeditLockdownMask lockdown;
	GtkWidget *view;

	container->priv = GEDIT_VIEW_CONTAINER_GET_PRIVATE (container);

	container->priv->active_view = NULL;
	container->priv->state = GEDIT_VIEW_CONTAINER_STATE_NORMAL;
	container->priv->mode = GEDIT_VIEW_CONTAINER_MODE_TEXT;

	container->priv->not_editable = FALSE;

	container->priv->save_flags = 0;

	container->priv->ask_if_externally_modified = TRUE;

	/* Manage auto save data */
	lockdown = gedit_app_get_lockdown (gedit_app_get_default ());
	container->priv->auto_save = gedit_prefs_manager_get_auto_save () &&
			       !(lockdown & GEDIT_LOCKDOWN_SAVE_TO_DISK);
	container->priv->auto_save = (container->priv->auto_save != FALSE);

	container->priv->auto_save_interval = gedit_prefs_manager_get_auto_save_interval ();
	if (container->priv->auto_save_interval <= 0)
		container->priv->auto_save_interval = GPM_DEFAULT_AUTO_SAVE_INTERVAL;

	/*FIXME
	_gedit_document_set_mount_operation_factory (doc,
						     container_mount_operation_factory,
						     container);*/

	/* Create the notebook */
	container->priv->notebook = gtk_notebook_new ();
	gtk_widget_show (container->priv->notebook);
	gtk_box_pack_end (GTK_BOX (container),
			  container->priv->notebook,
			  TRUE, TRUE, 0);

	g_signal_connect (container->priv->notebook,
			  "switch-page",
			  G_CALLBACK (on_switch_page_cb),
			  container);

	/* Create the main document */
	container->priv->doc = gedit_text_buffer_new ();
	
	g_object_set_data (G_OBJECT (container->priv->doc),
			   GEDIT_VIEW_CONTAINER_KEY, container);
	
	g_signal_connect (container->priv->doc,
			  "notify::uri",
			  G_CALLBACK (document_uri_notify_handler),
			  container);
	g_signal_connect (container->priv->doc,
			  "modified_changed",
			  G_CALLBACK (document_modified_changed),
			  container);
	g_signal_connect (container->priv->doc,
			  "loading",
			  G_CALLBACK (document_loading),
			  container);
	g_signal_connect (container->priv->doc,
			  "loaded",
			  G_CALLBACK (document_loaded),
			  container);
	g_signal_connect (container->priv->doc,
			  "saving",
			  G_CALLBACK (document_saving),
			  container);
	g_signal_connect (container->priv->doc,
			  "saved",
			  G_CALLBACK (document_saved),
			  container);

	/* Add the text view */
	view = gedit_text_view_new (GEDIT_TEXT_BUFFER (container->priv->doc));
	install_view (container, view);
	
	/* FIXME: This is only for testing */
	view = GTK_WIDGET (gedit_web_view_new ());
	install_view (container, view);
}

GtkWidget *
_gedit_view_container_new ()
{
	return GTK_WIDGET (g_object_new (GEDIT_TYPE_VIEW_CONTAINER, NULL));
}

/* Whether create is TRUE, creates a new empty document if location does 
   not refer to an existing file */
GtkWidget *
_gedit_view_container_new_from_uri (const gchar         *uri,
				    const GeditEncoding *encoding,
				    gint                 line_pos,
				    gboolean             create)
{
	GeditViewContainer *container;

	g_return_val_if_fail (uri != NULL, NULL);

	container = GEDIT_VIEW_CONTAINER (_gedit_view_container_new ());

	_gedit_view_container_load (container,
				    uri,
				    encoding,
				    line_pos,
				    create);

	return GTK_WIDGET (container);
}

/**
 * gedit_view_container_get_view:
 * @container: a #GeditViewContainer
 *
 * Gets the #GeditView inside @container.
 *
 * Returns: the #GeditView inside @container
 */
GeditView *
gedit_view_container_get_view (GeditViewContainer *container)
{
	g_return_val_if_fail (GEDIT_IS_VIEW_CONTAINER (container), NULL);

	return GEDIT_VIEW (container->priv->active_view);
}

GList *
gedit_view_container_get_views (GeditViewContainer *container)
{
	g_return_val_if_fail (GEDIT_IS_VIEW_CONTAINER (container), NULL);
	
	return container->priv->views;
}

/**
 * gedit_view_container_get_document:
 * @container: a #GeditViewContainer
 *
 * Gets the #GeditDocument associated to @container.
 *
 * Returns: the #GeditDocument associated to @container
 */
GeditDocument *
gedit_view_container_get_document (GeditViewContainer *container)
{
	g_return_val_if_fail (GEDIT_IS_VIEW_CONTAINER (container), NULL);

	return container->priv->doc;
}

#define MAX_DOC_NAME_LENGTH 40

gchar *
_gedit_view_container_get_name (GeditViewContainer *container)
{
	GeditDocument *doc;
	gchar *name;
	gchar *docname;
	gchar *container_name;

	g_return_val_if_fail (GEDIT_IS_VIEW_CONTAINER (container), NULL);

	doc = gedit_view_container_get_document (container);

	name = gedit_document_get_short_name_for_display (doc);

	/* Truncate the name so it doesn't get insanely wide. */
	docname = gedit_utils_str_middle_truncate (name, MAX_DOC_NAME_LENGTH);

	if (gedit_document_get_modified (doc))
	{
		container_name = g_strdup_printf ("*%s", docname);
	} 
	else 
	{
 #if 0		
		if (gedit_document_get_readonly (doc)) 
		{
			container_name = g_strdup_printf ("%s [%s]", docname, 
						/*Read only*/ _("RO"));
		} 
		else 
		{
			container_name = g_strdup_printf ("%s", docname);
		}
#endif
		container_name = g_strdup (docname);
	}
	
	g_free (docname);
	g_free (name);

	return container_name;
}

gchar *
_gedit_view_container_get_tooltips (GeditViewContainer *container)
{
	GeditDocument *doc;
	gchar *tip;
	gchar *uri;
	gchar *ruri;
	gchar *ruri_markup;

	g_return_val_if_fail (GEDIT_IS_VIEW_CONTAINER (container), NULL);

	doc = gedit_view_container_get_document (container);

	uri = gedit_document_get_uri_for_display (doc);
	g_return_val_if_fail (uri != NULL, NULL);

	ruri = 	gedit_utils_replace_home_dir_with_tilde (uri);
	g_free (uri);

	ruri_markup = g_markup_printf_escaped ("<i>%s</i>", ruri);

	switch (container->priv->state)
	{
		gchar *content_type;
		gchar *mime_type;
		gchar *content_description;
		gchar *content_full_description; 
		gchar *encoding;
		const GeditEncoding *enc;

		case GEDIT_VIEW_CONTAINER_STATE_LOADING_ERROR:
			tip = g_strdup_printf (_("Error opening file %s"),
					       ruri_markup);
			break;

		case GEDIT_VIEW_CONTAINER_STATE_REVERTING_ERROR:
			tip = g_strdup_printf (_("Error reverting file %s"),
					       ruri_markup);
			break;			

		case GEDIT_VIEW_CONTAINER_STATE_SAVING_ERROR:
			tip =  g_strdup_printf (_("Error saving file %s"),
						ruri_markup);
			break;			
		default:
			content_type = gedit_document_get_content_type (doc);
			mime_type = gedit_document_get_mime_type (doc);
			content_description = g_content_type_get_description (content_type);

			if (content_description == NULL)
				content_full_description = g_strdup (mime_type);
			else
				content_full_description = g_strdup_printf ("%s (%s)",
						content_description, mime_type);

			g_free (content_type);
			g_free (mime_type);
			g_free (content_description);

			enc = gedit_document_get_encoding (doc);

			if (enc == NULL)
				encoding = g_strdup (_("Unicode (UTF-8)"));
			else
				encoding = gedit_encoding_to_string (enc);

			tip =  g_markup_printf_escaped ("<b>%s</b> %s\n\n"
						        "<b>%s</b> %s\n"
						        "<b>%s</b> %s",
						        _("Name:"), ruri,
						        _("MIME Type:"), content_full_description,
						        _("Encoding:"), encoding);

			g_free (encoding);
			g_free (content_full_description);

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
	gint width, height;

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

static GdkPixbuf *
get_icon (GtkIconTheme *theme,
	  GFile        *location,
	  gint          size)
{
	GdkPixbuf *pixbuf;
	GtkIconInfo *icon_info;
	GFileInfo *info;
	GIcon *gicon;

	if (location == NULL)
		return get_stock_icon (theme, GTK_STOCK_FILE, size);

	/* FIXME: Doing a sync stat is bad, this should be fixed */
	info = g_file_query_info (location, 
	                          G_FILE_ATTRIBUTE_STANDARD_ICON, 
	                          G_FILE_QUERY_INFO_NONE, 
	                          NULL, 
	                          NULL);
	if (info == NULL)
		return get_stock_icon (theme, GTK_STOCK_FILE, size);

	gicon = g_file_info_get_icon (info);

	if (gicon == NULL)
	{
		g_object_unref (info);
		return get_stock_icon (theme, GTK_STOCK_FILE, size);
	}

	icon_info = gtk_icon_theme_lookup_by_gicon (theme, gicon, size, 0);
	g_object_unref (info);
	
	if (icon_info == NULL)
		return get_stock_icon (theme, GTK_STOCK_FILE, size);
	
	pixbuf = gtk_icon_info_load_icon (icon_info, NULL);
	gtk_icon_info_free (icon_info);
	
	if (pixbuf == NULL)
		return get_stock_icon (theme, GTK_STOCK_FILE, size);
		
	return resize_icon (pixbuf, size);
}

/* FIXME: add support for theme changed. I think it should be as easy as
   call g_object_notify (container, "name") when the icon theme changes */
GdkPixbuf *
_gedit_view_container_get_icon (GeditViewContainer *container)
{
	GdkPixbuf *pixbuf;
	GtkIconTheme *theme;
	GdkScreen *screen;
	gint icon_size;

	g_return_val_if_fail (GEDIT_IS_VIEW_CONTAINER (container), NULL);

	screen = gtk_widget_get_screen (GTK_WIDGET (container));

	theme = gtk_icon_theme_get_for_screen (screen);
	g_return_val_if_fail (theme != NULL, NULL);

	gtk_icon_size_lookup_for_settings (gtk_widget_get_settings (GTK_WIDGET (container)),
					   GTK_ICON_SIZE_MENU,
					   NULL,
					   &icon_size);

	switch (container->priv->state)
	{
		case GEDIT_VIEW_CONTAINER_STATE_LOADING:
			pixbuf = get_stock_icon (theme,
						 GTK_STOCK_OPEN,
						 icon_size);
			break;

		case GEDIT_VIEW_CONTAINER_STATE_REVERTING:
			pixbuf = get_stock_icon (theme,
						 GTK_STOCK_REVERT_TO_SAVED,
						 icon_size);
			break;

		case GEDIT_VIEW_CONTAINER_STATE_SAVING:
			pixbuf = get_stock_icon (theme,
						 GTK_STOCK_SAVE,
						 icon_size);
			break;

		case GEDIT_VIEW_CONTAINER_STATE_PRINTING:
			pixbuf = get_stock_icon (theme,
						 GTK_STOCK_PRINT,
						 icon_size);
			break;

		case GEDIT_VIEW_CONTAINER_STATE_PRINT_PREVIEWING:
		case GEDIT_VIEW_CONTAINER_STATE_SHOWING_PRINT_PREVIEW:
			pixbuf = get_stock_icon (theme,
						 GTK_STOCK_PRINT_PREVIEW,
						 icon_size);
			break;

		case GEDIT_VIEW_CONTAINER_STATE_LOADING_ERROR:
		case GEDIT_VIEW_CONTAINER_STATE_REVERTING_ERROR:
		case GEDIT_VIEW_CONTAINER_STATE_SAVING_ERROR:
		case GEDIT_VIEW_CONTAINER_STATE_GENERIC_ERROR:
			pixbuf = get_stock_icon (theme,
						 GTK_STOCK_DIALOG_ERROR,
						 icon_size);
			break;

		case GEDIT_VIEW_CONTAINER_STATE_EXTERNALLY_MODIFIED_NOTIFICATION:
			pixbuf = get_stock_icon (theme,
						 GTK_STOCK_DIALOG_WARNING,
						 icon_size);
			break;

		default:
		{
			GFile *location;
			GeditDocument *doc;

			doc = gedit_view_container_get_document (container);

			location = gedit_document_get_location (doc);
			pixbuf = get_icon (theme, location, icon_size);

			if (location)
				g_object_unref (location);
		}
	}

	return pixbuf;
}

/**
 * gedit_view_container_get_from_document:
 * @doc: a #GeditDocument
 *
 * Gets the #GeditViewContainer associated with @doc.
 *
 * Returns: the #GeditViewContainer associated with @doc
 */
GeditViewContainer *
gedit_view_container_get_from_document (GeditDocument *doc)
{
	gpointer res;
	
	g_return_val_if_fail (GEDIT_IS_DOCUMENT (doc), NULL);
	
	res = g_object_get_data (G_OBJECT (doc), GEDIT_VIEW_CONTAINER_KEY);
	
	return (res != NULL) ? GEDIT_VIEW_CONTAINER (res) : NULL;
}

GeditViewContainer *
gedit_view_container_get_from_view (GeditView *view)
{
	gpointer res;
	
	g_return_val_if_fail (GEDIT_IS_VIEW (view), NULL);
	
	res = g_object_get_data (G_OBJECT (view), GEDIT_VIEW_CONTAINER_KEY);
	
	return (res != NULL) ? GEDIT_VIEW_CONTAINER (res) : NULL;
}

void
_gedit_view_container_load (GeditViewContainer *container,
			    const gchar         *uri,
			    const GeditEncoding *encoding,
			    gint                 line_pos,
			    gboolean             create)
{
	g_return_if_fail (GEDIT_IS_VIEW_CONTAINER (container));
	g_return_if_fail (container->priv->state == GEDIT_VIEW_CONTAINER_STATE_NORMAL);

	gedit_view_container_set_state (container, GEDIT_VIEW_CONTAINER_STATE_LOADING);

	container->priv->tmp_line_pos = line_pos;
	container->priv->tmp_encoding = encoding;

	if (container->priv->auto_save_timeout > 0)
		remove_auto_save_timeout (container);

	gedit_document_load (container->priv->doc,
			     uri,
			     encoding,
			     line_pos,
			     create);
}

void
_gedit_view_container_revert (GeditViewContainer *container)
{
	GeditDocument *doc;
	gchar *uri;

	g_return_if_fail (GEDIT_IS_VIEW_CONTAINER (container));
	g_return_if_fail ((container->priv->state == GEDIT_VIEW_CONTAINER_STATE_NORMAL) ||
			  (container->priv->state == GEDIT_VIEW_CONTAINER_STATE_EXTERNALLY_MODIFIED_NOTIFICATION));

	if (container->priv->state == GEDIT_VIEW_CONTAINER_STATE_EXTERNALLY_MODIFIED_NOTIFICATION)
	{
		set_message_area (container, NULL);
	}

	doc = gedit_view_container_get_document (container);
	g_return_if_fail (GEDIT_IS_DOCUMENT (doc));

	gedit_view_container_set_state (container, GEDIT_VIEW_CONTAINER_STATE_REVERTING);

	uri = gedit_document_get_uri (doc);
	g_return_if_fail (uri != NULL);

	container->priv->tmp_line_pos = 0;
	container->priv->tmp_encoding = gedit_document_get_encoding (doc);

	if (container->priv->auto_save_timeout > 0)
		remove_auto_save_timeout (container);

	gedit_document_load (doc,
			     uri,
			     container->priv->tmp_encoding,
			     0,
			     FALSE);

	g_free (uri);
}

void
_gedit_view_container_save (GeditViewContainer *container)
{
	GeditDocument *doc;
	GeditDocumentSaveFlags save_flags;

	g_return_if_fail (GEDIT_IS_VIEW_CONTAINER (container));
	g_return_if_fail ((container->priv->state == GEDIT_VIEW_CONTAINER_STATE_NORMAL) ||
			  (container->priv->state == GEDIT_VIEW_CONTAINER_STATE_EXTERNALLY_MODIFIED_NOTIFICATION) ||
			  (container->priv->state == GEDIT_VIEW_CONTAINER_STATE_SHOWING_PRINT_PREVIEW));
	g_return_if_fail (container->priv->tmp_save_uri == NULL);
	g_return_if_fail (container->priv->tmp_encoding == NULL);

	doc = gedit_view_container_get_document (container);
	g_return_if_fail (GEDIT_IS_DOCUMENT (doc));
	g_return_if_fail (!gedit_document_is_untitled (doc));

	if (container->priv->state == GEDIT_VIEW_CONTAINER_STATE_EXTERNALLY_MODIFIED_NOTIFICATION)
	{
		/* We already told the user about the external
		 * modification: hide the message area and set
		 * the save flag.
		 */

		set_message_area (container, NULL);
		save_flags = container->priv->save_flags | GEDIT_DOCUMENT_SAVE_IGNORE_MTIME;
	}
	else
	{
		save_flags = container->priv->save_flags;
	}

	gedit_view_container_set_state (container, GEDIT_VIEW_CONTAINER_STATE_SAVING);

	/* uri used in error messages, will be freed in document_saved */
	container->priv->tmp_save_uri = gedit_document_get_uri (doc);
	container->priv->tmp_encoding = gedit_document_get_encoding (doc);

	if (container->priv->auto_save_timeout > 0)
		remove_auto_save_timeout (container);
		
	gedit_document_save (doc, save_flags);
}

static gboolean
gedit_view_container_auto_save (GeditViewContainer *container)
{
	GeditDocument *doc;

	gedit_debug (DEBUG_VIEW_CONTAINER);
	
	g_return_val_if_fail (container->priv->tmp_save_uri == NULL, FALSE);
	g_return_val_if_fail (container->priv->tmp_encoding == NULL, FALSE);
	
	doc = gedit_view_container_get_document (container);
	
	g_return_val_if_fail (!gedit_document_is_untitled (doc), FALSE);
	g_return_val_if_fail (!gedit_document_get_readonly (doc), FALSE);

	g_return_val_if_fail (container->priv->auto_save_timeout > 0, FALSE);
	g_return_val_if_fail (container->priv->auto_save, FALSE);
	g_return_val_if_fail (container->priv->auto_save_interval > 0, FALSE);

	if (!gedit_document_get_modified (doc))
	{
		gedit_debug_message (DEBUG_VIEW_CONTAINER, "Document not modified");

		return TRUE;
	}
			
	if ((container->priv->state != GEDIT_VIEW_CONTAINER_STATE_NORMAL) &&
	    (container->priv->state != GEDIT_VIEW_CONTAINER_STATE_SHOWING_PRINT_PREVIEW))
	{
		/* Retry after 30 seconds */
		guint timeout;

		gedit_debug_message (DEBUG_VIEW_CONTAINER, "Retry after 30 seconds");

		/* Add a new timeout */
		timeout = g_timeout_add_seconds (30,
						 (GSourceFunc) gedit_view_container_auto_save,
						 container);

		container->priv->auto_save_timeout = timeout;

	    	/* Returns FALSE so the old timeout is "destroyed" */
		return FALSE;
	}
	
	gedit_view_container_set_state (container, GEDIT_VIEW_CONTAINER_STATE_SAVING);

	/* uri used in error messages, will be freed in document_saved */
	container->priv->tmp_save_uri = gedit_document_get_uri (doc);
	container->priv->tmp_encoding = gedit_document_get_encoding (doc);

	/* Set auto_save_timeout to 0 since the timeout is going to be destroyed */
	container->priv->auto_save_timeout = 0;

	/* Since we are autosaving, we need to preserve the backup that was produced
	   the last time the user "manually" saved the file. In the case a recoverable
	   error happens while saving, the last backup is not preserved since the user
	   expressed his willing of saving the file */
	gedit_document_save (doc, container->priv->save_flags | GEDIT_DOCUMENT_SAVE_PRESERVE_BACKUP);
	
	gedit_debug_message (DEBUG_VIEW_CONTAINER, "Done");
	
	/* Returns FALSE so the old timeout is "destroyed" */
	return FALSE;
}

void
_gedit_view_container_save_as (GeditViewContainer *container,
			       const gchar         *uri,
			       const GeditEncoding *encoding)
{
	GeditDocument *doc;
	GeditDocumentSaveFlags save_flags;

	g_return_if_fail (GEDIT_IS_VIEW_CONTAINER (container));
	g_return_if_fail ((container->priv->state == GEDIT_VIEW_CONTAINER_STATE_NORMAL) ||
			  (container->priv->state == GEDIT_VIEW_CONTAINER_STATE_EXTERNALLY_MODIFIED_NOTIFICATION) ||
			  (container->priv->state == GEDIT_VIEW_CONTAINER_STATE_SHOWING_PRINT_PREVIEW));
	g_return_if_fail (encoding != NULL);

	g_return_if_fail (container->priv->tmp_save_uri == NULL);
	g_return_if_fail (container->priv->tmp_encoding == NULL);

	doc = gedit_view_container_get_document (container);
	g_return_if_fail (GEDIT_IS_DOCUMENT (doc));

	/* reset the save flags, when saving as */
	container->priv->save_flags = 0;

	if (container->priv->state == GEDIT_VIEW_CONTAINER_STATE_EXTERNALLY_MODIFIED_NOTIFICATION)
	{
		/* We already told the user about the external
		 * modification: hide the message area and set
		 * the save flag.
		 */

		set_message_area (container, NULL);
		save_flags = container->priv->save_flags | GEDIT_DOCUMENT_SAVE_IGNORE_MTIME;
	}
	else
	{
		save_flags = container->priv->save_flags;
	}

	gedit_view_container_set_state (container, GEDIT_VIEW_CONTAINER_STATE_SAVING);

	/* uri used in error messages... strdup because errors are async
	 * and the string can go away, will be freed in document_saved */
	container->priv->tmp_save_uri = g_strdup (uri);
	container->priv->tmp_encoding = encoding;

	if (container->priv->auto_save_timeout > 0)
		remove_auto_save_timeout (container);

	gedit_document_save_as (doc, uri, encoding, container->priv->save_flags);
}

#define GEDIT_PAGE_SETUP_KEY "gedit-page-setup-key"
#define GEDIT_PRINT_SETTINGS_KEY "gedit-print-settings-key"

static GtkPageSetup *
get_page_setup (GeditViewContainer *container)
{
	gpointer data;
	GeditDocument *doc;

	doc = gedit_view_container_get_document (container);

	data = g_object_get_data (G_OBJECT (doc),
				  GEDIT_PAGE_SETUP_KEY);

	if (data == NULL)
	{
		return _gedit_app_get_default_page_setup (gedit_app_get_default());
	}
	else
	{
		return gtk_page_setup_copy (GTK_PAGE_SETUP (data));
	}
}

static GtkPrintSettings *
get_print_settings (GeditViewContainer *container)
{
	gpointer data;
	GeditDocument *doc;

	doc = gedit_view_container_get_document (container);

	data = g_object_get_data (G_OBJECT (doc),
				  GEDIT_PRINT_SETTINGS_KEY);

	if (data == NULL)
	{
		return _gedit_app_get_default_print_settings (gedit_app_get_default());
	}
	else
	{
		return gtk_print_settings_copy (GTK_PRINT_SETTINGS (data));
	}
}

/* FIXME: show the message area only if the operation will be "long" */
static void
printing_cb (GeditPrintJob       *job,
	     GeditPrintJobStatus  status,
	     GeditViewContainer  *container)
{
	g_return_if_fail (GEDIT_IS_PROGRESS_MESSAGE_AREA (container->priv->message_area));

	gtk_widget_show (container->priv->message_area);

	gedit_progress_message_area_set_text (GEDIT_PROGRESS_MESSAGE_AREA (container->priv->message_area),
					      gedit_print_job_get_status_string (job));

	gedit_progress_message_area_set_fraction (GEDIT_PROGRESS_MESSAGE_AREA (container->priv->message_area),
						  gedit_print_job_get_progress (job));
}

static void
done_printing_cb (GeditPrintJob       *job,
		  GeditPrintJobResult  result,
		  const GError        *error,
		  GeditViewContainer  *container)
{
	GeditView *view;

	g_return_if_fail (container->priv->state == GEDIT_VIEW_CONTAINER_STATE_PRINT_PREVIEWING ||
			  container->priv->state == GEDIT_VIEW_CONTAINER_STATE_SHOWING_PRINT_PREVIEW ||
			  container->priv->state == GEDIT_VIEW_CONTAINER_STATE_PRINTING);

	if (container->priv->state == GEDIT_VIEW_CONTAINER_STATE_SHOWING_PRINT_PREVIEW)
	{
		/* print preview has been destroyed... */
		container->priv->print_preview = NULL;
	}
	else
	{
		g_return_if_fail (GEDIT_IS_PROGRESS_MESSAGE_AREA (container->priv->message_area));

		set_message_area (container, NULL); /* destroy the message area */
	}

	// TODO: check status and error

	/* Save the print settings and the page setup */ 
	if (result ==  GEDIT_PRINT_JOB_RESULT_OK)
	{
		GeditDocument *doc;
		GtkPrintSettings *settings;
		GtkPageSetup *page_setup;

		doc = gedit_view_container_get_document (container);

		settings = gedit_print_job_get_print_settings (job);

		/* remember settings for this document */
		g_object_set_data_full (G_OBJECT (doc),
					GEDIT_PRINT_SETTINGS_KEY,
					g_object_ref (settings),
					(GDestroyNotify)g_object_unref);

		/* make them the default */
		_gedit_app_set_default_print_settings (gedit_app_get_default (),
						       settings);

		page_setup = gedit_print_job_get_page_setup (job);

		/* remember page setup for this document */
		g_object_set_data_full (G_OBJECT (doc),
					GEDIT_PAGE_SETUP_KEY,
					g_object_ref (page_setup),
					(GDestroyNotify)g_object_unref);

		/* make it the default */
		_gedit_app_set_default_page_setup (gedit_app_get_default (),
						   page_setup);
	}

#if 0
	if (container->priv->print_preview != NULL)
	{
		/* If we were printing while showing the print preview,
		   see bug #352658 */
		gtk_widget_destroy (container->priv->print_preview);
		g_return_if_fail (container->priv->state == GEDIT_VIEW_CONTAINER_STATE_PRINTING);
	}
#endif

	gedit_view_container_set_state (container, GEDIT_VIEW_CONTAINER_STATE_NORMAL);

	view = gedit_view_container_get_view (container);
	gtk_widget_grab_focus (GTK_WIDGET (view));

 	g_object_unref (container->priv->print_job);
	container->priv->print_job = NULL;
}

#if 0
static void
print_preview_destroyed (GtkWidget *preview,
			 GeditViewContainer  *container)
{
	container->priv->print_preview = NULL;

	if (container->priv->state == GEDIT_VIEW_CONTAINER_STATE_SHOWING_PRINT_PREVIEW)
	{
		GeditView *view;

		gedit_view_container_set_state (container, GEDIT_VIEW_CONTAINER_STATE_NORMAL);

		view = gedit_view_container_get_view (container);
		gtk_widget_grab_focus (GTK_WIDGET (view));
	}
	else
	{
		/* This should happen only when printing while showing the print
		 * preview. In this case let us continue whithout changing
		 * the state and show the document. See bug #352658 */
		gtk_widget_show (container->priv->view_scrolled_window);
		
		g_return_if_fail (container->priv->state == GEDIT_VIEW_CONTAINER_STATE_PRINTING);
	}	
}
#endif

static void
show_preview_cb (GeditPrintJob       *job,
		 GeditPrintPreview   *preview,
		 GeditViewContainer            *container)
{
//	g_return_if_fail (container->priv->state == GEDIT_VIEW_CONTAINER_STATE_PRINT_PREVIEWING);
	g_return_if_fail (container->priv->print_preview == NULL);

	set_message_area (container, NULL); /* destroy the message area */

	container->priv->print_preview = GTK_WIDGET (preview);
	gtk_box_pack_end (GTK_BOX (container),
			  container->priv->print_preview,
			  TRUE,
			  TRUE,
			  0);
	gtk_widget_show (container->priv->print_preview);
	gtk_widget_grab_focus (container->priv->print_preview);

/* when the preview gets destroyed we get "done" signal
	g_signal_connect (container->priv->print_preview,
			  "destroy",
			  G_CALLBACK (print_preview_destroyed),
			  container);
*/
	gedit_view_container_set_state (container, GEDIT_VIEW_CONTAINER_STATE_SHOWING_PRINT_PREVIEW);
}

#if 0

static void
set_print_preview (GeditViewContainer  *container,
		   GtkWidget *print_preview)
{
	if (container->priv->print_preview == print_preview)
		return;
		
	if (container->priv->print_preview != NULL)
		gtk_widget_destroy (container->priv->print_preview);

	container->priv->print_preview = print_preview;

	gtk_box_pack_end (GTK_BOX (container),
			  container->priv->print_preview,
			  TRUE,
			  TRUE,
			  0);		

	gtk_widget_grab_focus (container->priv->print_preview);

	g_signal_connect (container->priv->print_preview,
			  "destroy",
			  G_CALLBACK (print_preview_destroyed),
			  container);
}

static void
preview_finished_cb (GtkSourcePrintJob *pjob, GeditViewContainer *container)
{
	GnomePrintJob *gjob;
	GtkWidget *preview = NULL;

	g_return_if_fail (GEDIT_IS_PROGRESS_MESSAGE_AREA (container->priv->message_area));
	set_message_area (container, NULL); /* destroy the message area */
	
	gjob = gtk_source_print_job_get_print_job (pjob);

	preview = gedit_print_job_preview_new (gjob);	
 	g_object_unref (gjob);
	
	set_print_preview (container, preview);
	
	gtk_widget_show (preview);
	g_object_unref (pjob);
	
	gedit_view_container_set_state (container, GEDIT_VIEW_CONTAINER_STATE_SHOWING_PRINT_PREVIEW);
}


#endif

static void
print_cancelled (GtkWidget          *area,
                 gint                response_id,
                 GeditViewContainer *container)
{
	g_return_if_fail (GEDIT_IS_PROGRESS_MESSAGE_AREA (container->priv->message_area));

	gedit_print_job_cancel (container->priv->print_job);

	g_debug ("print_cancelled");
}

static void
show_printing_message_area (GeditViewContainer *container,
			    gboolean preview)
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
			  container);
	
	set_message_area (container, area);
}

#if !GTK_CHECK_VERSION (2, 17, 4)

static void
page_setup_done_cb (GtkPageSetup       *setup,
		    GeditViewContainer *container)
{
	if (setup != NULL)
	{
		GeditDocument *doc;

		doc = gedit_view_container_get_document (container);

		/* remember it for this document */
		g_object_set_data_full (G_OBJECT (doc),
					GEDIT_PAGE_SETUP_KEY,
					g_object_ref (setup),
					(GDestroyNotify)g_object_unref);

		/* make it the default */
		_gedit_app_set_default_page_setup (gedit_app_get_default(),
						   setup);
	}
}

void 
_gedit_view_container_page_setup (GeditViewContainer *container)
{
	GtkPageSetup *setup;
	GtkPrintSettings *settings;

	g_return_if_fail (GEDIT_IS_VIEW_CONTAINER (container));

	setup = get_page_setup (container);
	settings = get_print_settings (container);

	gtk_print_run_page_setup_dialog_async (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (container))),
					       setup,
					       settings,
					       (GtkPageSetupDoneFunc) page_setup_done_cb,
					       container);

	/* CHECK: should we unref setup and settings? */
}

#endif

static void
gedit_view_container_print_or_print_preview (GeditViewContainer      *container,
					     GtkPrintOperationAction  print_action)
{
	GeditView *view;
	gboolean is_preview;
	GtkPageSetup *setup;
	GtkPrintSettings *settings;
	GtkPrintOperationResult res;
	GError *error = NULL;

	g_return_if_fail (container->priv->print_job == NULL);
	g_return_if_fail (container->priv->state == GEDIT_VIEW_CONTAINER_STATE_NORMAL);

	view = gedit_view_container_get_view (container);

	is_preview = (print_action == GTK_PRINT_OPERATION_ACTION_PREVIEW);

	container->priv->print_job = gedit_print_job_new (view);
	g_object_add_weak_pointer (G_OBJECT (container->priv->print_job),
				   (gpointer *) &container->priv->print_job);

	show_printing_message_area (container, is_preview);

	g_signal_connect (container->priv->print_job,
			  "printing",
			  G_CALLBACK (printing_cb),
			  container);
	g_signal_connect (container->priv->print_job,
			  "show-preview",
			  G_CALLBACK (show_preview_cb),
			  container);
	g_signal_connect (container->priv->print_job,
			  "done",
			  G_CALLBACK (done_printing_cb),
			  container);

	if (is_preview)
		gedit_view_container_set_state (container, GEDIT_VIEW_CONTAINER_STATE_PRINT_PREVIEWING);
	else
		gedit_view_container_set_state (container, GEDIT_VIEW_CONTAINER_STATE_PRINTING);

	setup = get_page_setup (container);
	settings = get_print_settings (container);

	res = gedit_print_job_print (container->priv->print_job,
				     print_action,
				     setup,
				     settings,
				     GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (container))),
				     &error);

	// TODO: manage res in the correct way
	if (res == GTK_PRINT_OPERATION_RESULT_ERROR)
	{
		/* FIXME: go in error state */
		gedit_view_container_set_state (container, GEDIT_VIEW_CONTAINER_STATE_NORMAL);
		g_warning ("Async print preview failed (%s)", error->message);
		g_object_unref (container->priv->print_job);
		g_error_free (error);
	}
}

void 
_gedit_view_container_print (GeditViewContainer *container)
{
	g_return_if_fail (GEDIT_IS_VIEW_CONTAINER (container));

	/* FIXME: currently we can have just one printoperation going on
	 * at a given time, so before starting the print we close the preview.
	 * Would be nice to handle it properly though */
	if (container->priv->state == GEDIT_VIEW_CONTAINER_STATE_SHOWING_PRINT_PREVIEW)
	{
		gtk_widget_destroy (container->priv->print_preview);
	}

	gedit_view_container_print_or_print_preview (container,
						     GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG);
}

void
_gedit_view_container_print_preview (GeditViewContainer *container)
{
	g_return_if_fail (GEDIT_IS_VIEW_CONTAINER (container));

	gedit_view_container_print_or_print_preview (container,
						     GTK_PRINT_OPERATION_ACTION_PREVIEW);
}

void 
_gedit_view_container_mark_for_closing (GeditViewContainer *container)
{
	g_return_if_fail (GEDIT_IS_VIEW_CONTAINER (container));
	g_return_if_fail (container->priv->state == GEDIT_VIEW_CONTAINER_STATE_NORMAL);
	
	gedit_view_container_set_state (container, GEDIT_VIEW_CONTAINER_STATE_CLOSING);
}

gboolean
_gedit_view_container_can_close (GeditViewContainer *container)
{
	GeditDocument *doc;
	GeditViewContainerState ts;

	g_return_val_if_fail (GEDIT_IS_VIEW_CONTAINER (container), FALSE);

	ts = gedit_view_container_get_state (container);

	/* if we are loading or reverting, the container can be closed */
	if ((ts == GEDIT_VIEW_CONTAINER_STATE_LOADING)       ||
	    (ts == GEDIT_VIEW_CONTAINER_STATE_LOADING_ERROR) ||
	    (ts == GEDIT_VIEW_CONTAINER_STATE_REVERTING)     ||
	    (ts == GEDIT_VIEW_CONTAINER_STATE_REVERTING_ERROR)) /* CHECK: I'm not sure this is the right behavior for REVERTING ERROR */
		return TRUE;

	/* Do not close container with saving errors */
	if (ts == GEDIT_VIEW_CONTAINER_STATE_SAVING_ERROR)
		return FALSE;
		
	doc = gedit_view_container_get_document (container);

	/* TODO: we need to save the file also if it has been externally
	   modified - Paolo (Oct 10, 2005) */

	return (!gedit_document_get_modified (doc) &&
		!gedit_document_get_deleted (doc));
}

/**
 * gedit_view_container_get_auto_save_enabled:
 * @container: a #GeditViewContainer
 * 
 * Gets the current state for the autosave feature
 * 
 * Return value: %TRUE if the autosave is enabled, else %FALSE
 **/
gboolean
gedit_view_container_get_auto_save_enabled (GeditViewContainer *container)
{
	gedit_debug (DEBUG_VIEW_CONTAINER);

	g_return_val_if_fail (GEDIT_IS_VIEW_CONTAINER (container), FALSE);

	return container->priv->auto_save;
}

/**
 * gedit_view_container_set_auto_save_enabled:
 * @container: a #GeditViewContainer
 * @enable: enable (%TRUE) or disable (%FALSE) auto save
 * 
 * Enables or disables the autosave feature. It does not install an
 * autosave timeout if the document is new or is read-only
 **/
void
gedit_view_container_set_auto_save_enabled (GeditViewContainer *container, 
					    gboolean enable)
{
	GeditDocument *doc = NULL;
	GeditLockdownMask lockdown;
	
	gedit_debug (DEBUG_VIEW_CONTAINER);

	g_return_if_fail (GEDIT_IS_VIEW_CONTAINER (container));

	/* Force disabling when lockdown is active */
	lockdown = gedit_app_get_lockdown (gedit_app_get_default());
	if (lockdown & GEDIT_LOCKDOWN_SAVE_TO_DISK)
		enable = FALSE;
	
	doc = gedit_view_container_get_document (container);

	if (container->priv->auto_save == enable)
		return;

	container->priv->auto_save = enable;

 	if (enable && 
 	    (container->priv->auto_save_timeout <=0) &&
 	    !gedit_document_is_untitled (doc) &&
 	    !gedit_document_get_readonly (doc))
 	{
 		if ((container->priv->state != GEDIT_VIEW_CONTAINER_STATE_LOADING) &&
		    (container->priv->state != GEDIT_VIEW_CONTAINER_STATE_SAVING) &&
		    (container->priv->state != GEDIT_VIEW_CONTAINER_STATE_REVERTING) &&
		    (container->priv->state != GEDIT_VIEW_CONTAINER_STATE_LOADING_ERROR) &&
		    (container->priv->state != GEDIT_VIEW_CONTAINER_STATE_SAVING_ERROR) &&
		    (container->priv->state != GEDIT_VIEW_CONTAINER_STATE_REVERTING_ERROR))
		{
			install_auto_save_timeout (container);
		}
		/* else: the timeout will be installed when loading/saving/reverting
		         will terminate */
		
		return;
	}
 		
 	if (!enable && (container->priv->auto_save_timeout > 0))
 	{
		remove_auto_save_timeout (container);
		
 		return; 
 	} 

 	g_return_if_fail ((!enable && (container->priv->auto_save_timeout <= 0)) ||  
 			  gedit_document_is_untitled (doc) || gedit_document_get_readonly (doc)); 
}

/**
 * gedit_view_container_get_auto_save_interval:
 * @container: a #GeditViewContainer
 * 
 * Gets the current interval for the autosaves
 * 
 * Return value: the value of the autosave
 **/
gint 
gedit_view_container_get_auto_save_interval (GeditViewContainer *container)
{
	gedit_debug (DEBUG_VIEW_CONTAINER);

	g_return_val_if_fail (GEDIT_IS_VIEW_CONTAINER (container), 0);

	return container->priv->auto_save_interval;
}

/**
 * gedit_view_container_set_auto_save_interval:
 * @container: a #GeditViewContainer
 * @interval: the new interval
 * 
 * Sets the interval for the autosave feature. It does nothing if the
 * interval is the same as the one already present. It removes the old
 * interval timeout and adds a new one with the autosave passed as
 * argument.
 **/
void 
gedit_view_container_set_auto_save_interval (GeditViewContainer *container, 
					     gint interval)
{
	GeditDocument *doc = NULL;
	
	gedit_debug (DEBUG_VIEW_CONTAINER);
	
	g_return_if_fail (GEDIT_IS_VIEW_CONTAINER (container));

	doc = gedit_view_container_get_document (container);

	g_return_if_fail (GEDIT_IS_DOCUMENT (doc));
	g_return_if_fail (interval > 0);

	if (container->priv->auto_save_interval == interval)
		return;

	container->priv->auto_save_interval = interval;
		
	if (!container->priv->auto_save)
		return;

	if (container->priv->auto_save_timeout > 0)
	{
		g_return_if_fail (!gedit_document_is_untitled (doc));
		g_return_if_fail (!gedit_document_get_readonly (doc));

		remove_auto_save_timeout (container);

		install_auto_save_timeout (container);
	}
}
