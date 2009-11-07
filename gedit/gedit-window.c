/*
 * gedit-window.c
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
 *
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <time.h>
#include <sys/types.h>
#include <string.h>

#include <glib/gi18n.h>
#include <gio/gio.h>

#include "gedit-ui.h"
#include "gedit-window.h"
#include "gedit-window-private.h"
#include "gedit-app.h"
#include "gedit-notebook.h"
#include "gedit-statusbar.h"
#include "gedit-utils.h"
#include "gedit-commands.h"
#include "gedit-debug.h"
#include "gedit-language-manager.h"
#include "gedit-prefs-manager-app.h"
#include "gedit-panel.h"
#include "gedit-documents-panel.h"
#include "gedit-plugins-engine.h"
#include "gedit-enum-types.h"
#include "gedit-dirs.h"
#include "gedit-status-combo-box.h"
#include "gedit-text-buffer.h"
#include "gedit-text-view.h"

#define LANGUAGE_NONE (const gchar *)"LangNone"
#define GEDIT_UIFILE "gedit-ui.xml"
#define TAB_WIDTH_DATA "GeditWindowTabWidthData"
#define LANGUAGE_DATA "GeditWindowLanguageData"
#define FULLSCREEN_ANIMATION_SPEED 4

#define GEDIT_WINDOW_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object),\
					 GEDIT_TYPE_WINDOW,                    \
					 GeditWindowPrivate))

/* Signals */
enum
{
	PAGE_ADDED,
	PAGE_REMOVED,
	PAGES_REORDERED,
	ACTIVE_PAGE_CHANGED,
	ACTIVE_PAGE_STATE_CHANGED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

enum
{
	PROP_0,
	PROP_STATE
};

enum
{
	TARGET_URI_LIST = 100
};

G_DEFINE_TYPE(GeditWindow, gedit_window, GTK_TYPE_WINDOW)

static void	recent_manager_changed	(GtkRecentManager *manager,
					 GeditWindow *window);

static void
gedit_window_get_property (GObject    *object,
			   guint       prop_id,
			   GValue     *value,
			   GParamSpec *pspec)
{
	GeditWindow *window = GEDIT_WINDOW (object);

	switch (prop_id)
	{
		case PROP_STATE:
			g_value_set_enum (value,
					  gedit_window_get_state (window));
			break;			
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;			
	}
}

static void
save_panes_state (GeditWindow *window)
{
	gint pane_page;

	gedit_debug (DEBUG_WINDOW);

	if (gedit_prefs_manager_window_size_can_set ())
		gedit_prefs_manager_set_window_size (window->priv->width,
						     window->priv->height);

	if (gedit_prefs_manager_window_state_can_set ())
		gedit_prefs_manager_set_window_state (window->priv->window_state);

	if ((window->priv->side_panel_size > 0) &&
	    gedit_prefs_manager_side_panel_size_can_set ())
		gedit_prefs_manager_set_side_panel_size	(
					window->priv->side_panel_size);

	pane_page = _gedit_panel_get_active_item_id (GEDIT_PANEL (window->priv->side_panel));
	if (pane_page != 0 &&
	    gedit_prefs_manager_side_panel_active_page_can_set ())
		gedit_prefs_manager_set_side_panel_active_page (pane_page);

	if ((window->priv->bottom_panel_size > 0) && 
	    gedit_prefs_manager_bottom_panel_size_can_set ())
		gedit_prefs_manager_set_bottom_panel_size (
					window->priv->bottom_panel_size);

	pane_page = _gedit_panel_get_active_item_id (GEDIT_PANEL (window->priv->bottom_panel));
	if (pane_page != 0 &&
	    gedit_prefs_manager_bottom_panel_active_page_can_set ())
		gedit_prefs_manager_set_bottom_panel_active_page (pane_page);
}

static void
gedit_window_dispose (GObject *object)
{
	GeditWindow *window;

	gedit_debug (DEBUG_WINDOW);

	window = GEDIT_WINDOW (object);

	/* Stop tracking removal of panes otherwise we always
	 * end up with thinking we had no pane active, since they
	 * should all be removed below */
	if (window->priv->bottom_panel_item_removed_handler_id != 0)
	{
		g_signal_handler_disconnect (window->priv->bottom_panel,
					     window->priv->bottom_panel_item_removed_handler_id);
		window->priv->bottom_panel_item_removed_handler_id = 0;
	}

	/* First of all, force collection so that plugins
	 * really drop some of the references.
	 */
	gedit_plugins_engine_garbage_collect (gedit_plugins_engine_get_default ());

	/* save the panes position and make sure to deactivate plugins
	 * for this window, but only once */
	if (!window->priv->dispose_has_run)
	{
		save_panes_state (window);

		gedit_plugins_engine_deactivate_plugins (gedit_plugins_engine_get_default (),
					                  window);
		window->priv->dispose_has_run = TRUE;
	}

	if (window->priv->fullscreen_animation_timeout_id != 0)
	{
		g_source_remove (window->priv->fullscreen_animation_timeout_id);
		window->priv->fullscreen_animation_timeout_id = 0;
	}

	if (window->priv->fullscreen_controls != NULL)
	{
		gtk_widget_destroy (window->priv->fullscreen_controls);
		
		window->priv->fullscreen_controls = NULL;
	}

	if (window->priv->recents_handler_id != 0)
	{
		GtkRecentManager *recent_manager;

		recent_manager =  gtk_recent_manager_get_default ();
		g_signal_handler_disconnect (recent_manager,
					     window->priv->recents_handler_id);
		window->priv->recents_handler_id = 0;
	}

	if (window->priv->manager != NULL)
	{
		g_object_unref (window->priv->manager);
		window->priv->manager = NULL;
	}

	if (window->priv->message_bus != NULL)
	{
		g_object_unref (window->priv->message_bus);
		window->priv->message_bus = NULL;
	}

	if (window->priv->window_group != NULL)
	{
		g_object_unref (window->priv->window_group);
		window->priv->window_group = NULL;
	}
	
	/* Now that there have broken some reference loops,
	 * force collection again.
	 */
	gedit_plugins_engine_garbage_collect (gedit_plugins_engine_get_default ());

	G_OBJECT_CLASS (gedit_window_parent_class)->dispose (object);
}

static void
gedit_window_finalize (GObject *object)
{
	GeditWindow *window; 

	gedit_debug (DEBUG_WINDOW);

	window = GEDIT_WINDOW (object);

	if (window->priv->default_location != NULL)
		g_object_unref (window->priv->default_location);

	G_OBJECT_CLASS (gedit_window_parent_class)->finalize (object);
}

static gboolean
gedit_window_window_state_event (GtkWidget           *widget,
				 GdkEventWindowState *event)
{
	GeditWindow *window = GEDIT_WINDOW (widget);

	window->priv->window_state = event->new_window_state;

	if (event->changed_mask &
	    (GDK_WINDOW_STATE_MAXIMIZED | GDK_WINDOW_STATE_FULLSCREEN))
	{
		gboolean show;

		show = !(event->new_window_state &
			(GDK_WINDOW_STATE_MAXIMIZED | GDK_WINDOW_STATE_FULLSCREEN));

		_gedit_statusbar_set_has_resize_grip (GEDIT_STATUSBAR (window->priv->statusbar),
						      show);
	}

	return FALSE;
}

static gboolean 
gedit_window_configure_event (GtkWidget         *widget,
			      GdkEventConfigure *event)
{
	GeditWindow *window = GEDIT_WINDOW (widget);

	window->priv->width = event->width;
	window->priv->height = event->height;

	return GTK_WIDGET_CLASS (gedit_window_parent_class)->configure_event (widget, event);
}

/*
 * GtkWindow catches keybindings for the menu items _before_ passing them to
 * the focused widget. This is unfortunate and means that pressing ctrl+V
 * in an entry on a panel ends up pasting text in the TextView.
 * Here we override GtkWindow's handler to do the same things that it
 * does, but in the opposite order and then we chain up to the grand
 * parent handler, skipping gtk_window_key_press_event.
 */
static gboolean
gedit_window_key_press_event (GtkWidget   *widget,
			      GdkEventKey *event)
{
	static gpointer grand_parent_class = NULL;
	GtkWindow *window = GTK_WINDOW (widget);
	gboolean handled = FALSE;

	if (grand_parent_class == NULL)
		grand_parent_class = g_type_class_peek_parent (gedit_window_parent_class);

	/* handle focus widget key events */
	if (!handled)
		handled = gtk_window_propagate_key_event (window, event);

	/* handle mnemonics and accelerators */
	if (!handled)
		handled = gtk_window_activate_key (window, event);

	/* Chain up, invokes binding set */
	if (!handled)
		handled = GTK_WIDGET_CLASS (grand_parent_class)->key_press_event (widget, event);

	return handled;
}

static void
gedit_window_page_removed (GeditWindow *window,
			   GeditPage   *page)
{
	gedit_plugins_engine_garbage_collect (gedit_plugins_engine_get_default ());
}

static void
gedit_window_class_init (GeditWindowClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

	klass->page_removed = gedit_window_page_removed;

	object_class->dispose = gedit_window_dispose;
	object_class->finalize = gedit_window_finalize;
	object_class->get_property = gedit_window_get_property;

	widget_class->window_state_event = gedit_window_window_state_event;
	widget_class->configure_event = gedit_window_configure_event;
	widget_class->key_press_event = gedit_window_key_press_event;

	signals[PAGE_ADDED] =
		g_signal_new ("page_added",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (GeditWindowClass, page_added),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE,
			      1,
			      GEDIT_TYPE_PAGE);
	signals[PAGE_REMOVED] =
		g_signal_new ("page_removed",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (GeditWindowClass, page_removed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE,
			      1,
			      GEDIT_TYPE_PAGE);
	signals[PAGES_REORDERED] =
		g_signal_new ("pages_reordered",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (GeditWindowClass, pages_reordered),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);
	signals[ACTIVE_PAGE_CHANGED] =
		g_signal_new ("active_page_changed",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (GeditWindowClass, active_page_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE,
			      1,
			      GEDIT_TYPE_PAGE);
	signals[ACTIVE_PAGE_STATE_CHANGED] =
		g_signal_new ("active_page_state_changed",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (GeditWindowClass, active_page_state_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE,
			      0);

	g_object_class_install_property (object_class,
					 PROP_STATE,
					 g_param_spec_flags ("state",
							     "State",
							     "The window's state",
							     GEDIT_TYPE_WINDOW_STATE,
							     GEDIT_WINDOW_STATE_NORMAL,
							     G_PARAM_READABLE |
							     G_PARAM_STATIC_STRINGS));

	g_type_class_add_private (object_class, sizeof(GeditWindowPrivate));
}

static void
menu_item_select_cb (GtkMenuItem *proxy,
		     GeditWindow *window)
{
	GtkAction *action;
	char *message;

	action = gtk_widget_get_action (GTK_WIDGET (proxy));
	g_return_if_fail (action != NULL);

	g_object_get (G_OBJECT (action), "tooltip", &message, NULL);
	if (message)
	{
		gtk_statusbar_push (GTK_STATUSBAR (window->priv->statusbar),
				    window->priv->tip_message_cid, message);
		g_free (message);
	}
}

static void
menu_item_deselect_cb (GtkMenuItem *proxy,
                       GeditWindow *window)
{
	gtk_statusbar_pop (GTK_STATUSBAR (window->priv->statusbar),
			   window->priv->tip_message_cid);
}

static void
connect_proxy_cb (GtkUIManager *manager,
                  GtkAction *action,
                  GtkWidget *proxy,
                  GeditWindow *window)
{
	if (GTK_IS_MENU_ITEM (proxy))
	{
		g_signal_connect (proxy, "select",
				  G_CALLBACK (menu_item_select_cb), window);
		g_signal_connect (proxy, "deselect",
				  G_CALLBACK (menu_item_deselect_cb), window);
	}
}

static void
disconnect_proxy_cb (GtkUIManager *manager,
                     GtkAction *action,
                     GtkWidget *proxy,
                     GeditWindow *window)
{
	if (GTK_IS_MENU_ITEM (proxy))
	{
		g_signal_handlers_disconnect_by_func
			(proxy, G_CALLBACK (menu_item_select_cb), window);
		g_signal_handlers_disconnect_by_func
			(proxy, G_CALLBACK (menu_item_deselect_cb), window);
	}
}

static void
apply_toolbar_style (GeditWindow *window,
		     GtkWidget *toolbar)
{
	switch (window->priv->toolbar_style)
	{
		case GEDIT_TOOLBAR_SYSTEM:
			gedit_debug_message (DEBUG_WINDOW, "GEDIT: SYSTEM");
			gtk_toolbar_unset_style (
					GTK_TOOLBAR (toolbar));
			break;

		case GEDIT_TOOLBAR_ICONS:
			gedit_debug_message (DEBUG_WINDOW, "GEDIT: ICONS");
			gtk_toolbar_set_style (
					GTK_TOOLBAR (toolbar),
					GTK_TOOLBAR_ICONS);
			break;

		case GEDIT_TOOLBAR_ICONS_AND_TEXT:
			gedit_debug_message (DEBUG_WINDOW, "GEDIT: ICONS_AND_TEXT");
			gtk_toolbar_set_style (
					GTK_TOOLBAR (toolbar),
					GTK_TOOLBAR_BOTH);
			break;

		case GEDIT_TOOLBAR_ICONS_BOTH_HORIZ:
			gedit_debug_message (DEBUG_WINDOW, "GEDIT: ICONS_BOTH_HORIZ");
			gtk_toolbar_set_style (
					GTK_TOOLBAR (toolbar),
					GTK_TOOLBAR_BOTH_HORIZ);
			break;
	}
}

/* Returns TRUE if toolbar is visible */
static gboolean
set_toolbar_style (GeditWindow *window,
		   GeditWindow *origin)
{
	gboolean visible;
	GeditToolbarSetting style;
	GtkAction *action;
	
	if (origin == NULL)
		visible = gedit_prefs_manager_get_toolbar_visible ();
	else
		visible = GTK_WIDGET_VISIBLE (origin->priv->toolbar);
	
	/* Set visibility */
	if (visible)
		gtk_widget_show (window->priv->toolbar);
	else
		gtk_widget_hide (window->priv->toolbar);

	action = gtk_action_group_get_action (window->priv->always_sensitive_action_group,
					      "ViewToolbar");

	if (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)) != visible)
		gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), visible);

	/* Set style */
	if (origin == NULL)
		style = gedit_prefs_manager_get_toolbar_buttons_style ();
	else
		style = origin->priv->toolbar_style;
	
	window->priv->toolbar_style = style;
	
	apply_toolbar_style (window, window->priv->toolbar);
	
	return visible;
}

static void
update_next_prev_doc_sensitivity (GeditWindow *window,
				  GeditPage   *page)
{
	gint	     page_number;
	GtkNotebook *notebook;
	GtkAction   *action;
	
	gedit_debug (DEBUG_WINDOW);
	
	notebook = GTK_NOTEBOOK (_gedit_window_get_notebook (window));
	
	page_number = gtk_notebook_page_num (notebook, GTK_WIDGET (page));
	g_return_if_fail (page_number >= 0);
	
	action = gtk_action_group_get_action (window->priv->action_group,
					      "DocumentsPreviousDocument");
	gtk_action_set_sensitive (action, page_number != 0);
	
	action = gtk_action_group_get_action (window->priv->action_group,
					      "DocumentsNextDocument");
	gtk_action_set_sensitive (action, 
				  page_number < gtk_notebook_get_n_pages (notebook) - 1);
}

static void
update_next_prev_doc_sensitivity_per_window (GeditWindow *window)
{
	GeditPage *page;
	GtkAction *action;
	
	gedit_debug (DEBUG_WINDOW);
	
	page = gedit_window_get_active_page (window);
	if (page != NULL)
	{
		update_next_prev_doc_sensitivity (window, page);
		
		return;
	}
	
	action = gtk_action_group_get_action (window->priv->action_group,
					      "DocumentsPreviousDocument");
	gtk_action_set_sensitive (action, FALSE);
	
	action = gtk_action_group_get_action (window->priv->action_group,
					      "DocumentsNextDocument");
	gtk_action_set_sensitive (action, FALSE);
	
}

static void
received_clipboard_contents (GtkClipboard     *clipboard,
			     GtkSelectionData *selection_data,
			     GeditWindow      *window)
{
	gboolean sens;
	GtkAction *action;

	/* getting clipboard contents is async, so we need to
	 * get the current page and its state */

	if (window->priv->active_page != NULL)
	{
		GeditViewContainer *container;
		GeditViewContainerState state;
		gboolean state_normal;

		container = gedit_page_get_active_view_container (window->priv->active_page);
		state = gedit_view_container_get_state (container);
		state_normal = (state == GEDIT_VIEW_CONTAINER_STATE_NORMAL);

		sens = state_normal &&
		       gtk_selection_data_targets_include_text (selection_data);
	}
	else
	{
		sens = FALSE;
	}

	action = gtk_action_group_get_action (window->priv->action_group,
					      "EditPaste");

	gtk_action_set_sensitive (action, sens);

	g_object_unref (window);
}

static void
set_paste_sensitivity_according_to_clipboard (GeditWindow  *window,
					      GtkClipboard *clipboard)
{
	GdkDisplay *display;

	display = gtk_clipboard_get_display (clipboard);

	if (gdk_display_supports_selection_notification (display))
	{
		gtk_clipboard_request_contents (clipboard,
						gdk_atom_intern_static_string ("TARGETS"),
						(GtkClipboardReceivedFunc) received_clipboard_contents,
						g_object_ref (window));
	}
	else
	{
		GtkAction *action;

		action = gtk_action_group_get_action (window->priv->action_group,
						      "EditPaste");

		/* XFIXES extension not availbale, make
		 * Paste always sensitive */
		gtk_action_set_sensitive (action, TRUE);
	}
}

static void
set_sensitivity_for_search_items (GeditWindow   *window,
				  GeditDocument *doc,
				  GeditViewContainerState  state,
				  gboolean       state_normal,
				  gboolean       editable)
{
	GtkAction *action;
	gboolean   b;
	
	/* FIXME: quick hack */
	if (!GEDIT_IS_TEXT_BUFFER (doc))
		state_normal = FALSE;
	else
		b = gedit_text_buffer_get_can_search_again (GEDIT_TEXT_BUFFER (doc));

	action = gtk_action_group_get_action (window->priv->action_group,
					      "SearchFind");
	gtk_action_set_sensitive (action,
				  (state_normal ||
				   state == GEDIT_VIEW_CONTAINER_STATE_EXTERNALLY_MODIFIED_NOTIFICATION));

	action = gtk_action_group_get_action (window->priv->action_group,
					      "SearchIncrementalSearch");
	gtk_action_set_sensitive (action,
				  (state_normal ||
				   state == GEDIT_VIEW_CONTAINER_STATE_EXTERNALLY_MODIFIED_NOTIFICATION));

	action = gtk_action_group_get_action (window->priv->action_group,
					      "SearchReplace");
	gtk_action_set_sensitive (action,
				  state_normal &&
				  editable);

	action = gtk_action_group_get_action (window->priv->action_group,
					      "SearchFindNext");
	gtk_action_set_sensitive (action,
				  (state_normal ||
				   state == GEDIT_VIEW_CONTAINER_STATE_EXTERNALLY_MODIFIED_NOTIFICATION) && b);

	action = gtk_action_group_get_action (window->priv->action_group,
					      "SearchFindPrevious");
	gtk_action_set_sensitive (action,
				  (state_normal ||
				   state == GEDIT_VIEW_CONTAINER_STATE_EXTERNALLY_MODIFIED_NOTIFICATION) && b);

	action = gtk_action_group_get_action (window->priv->action_group,
					      "SearchClearHighlight");
	gtk_action_set_sensitive (action,
				  (state_normal ||
				   state == GEDIT_VIEW_CONTAINER_STATE_EXTERNALLY_MODIFIED_NOTIFICATION) && b);

	action = gtk_action_group_get_action (window->priv->action_group,
					      "SearchGoToLine");
	gtk_action_set_sensitive (action,
				  (state_normal ||
				   state == GEDIT_VIEW_CONTAINER_STATE_EXTERNALLY_MODIFIED_NOTIFICATION));
}

static void
set_sensitivity_according_to_page (GeditWindow *window,
				   GeditPage   *page)
{
	GeditViewContainer *container;
	GeditDocument *doc;
	GeditView     *view;
	GtkAction     *action;
	gboolean       state_normal;
	gboolean       editable;
	GeditViewContainerState  state;
	GtkClipboard  *clipboard;
	GeditLockdownMask lockdown;

	g_return_if_fail (GEDIT_PAGE (page));

	gedit_debug (DEBUG_WINDOW);

	container = gedit_page_get_active_view_container (page);

	lockdown = gedit_app_get_lockdown (gedit_app_get_default ());

	state = gedit_view_container_get_state (container);
	state_normal = (state == GEDIT_VIEW_CONTAINER_STATE_NORMAL);

	view = gedit_view_container_get_view (container);
	editable = gedit_view_get_editable (view);

	doc = gedit_view_container_get_document (container);

	clipboard = gtk_widget_get_clipboard (GTK_WIDGET (window),
					      GDK_SELECTION_CLIPBOARD);

	action = gtk_action_group_get_action (window->priv->action_group,
					      "FileSave");
	gtk_action_set_sensitive (action,
				  (state_normal ||
				   (state == GEDIT_VIEW_CONTAINER_STATE_EXTERNALLY_MODIFIED_NOTIFICATION) ||
				   (state == GEDIT_VIEW_CONTAINER_STATE_SHOWING_PRINT_PREVIEW)) &&
				  !gedit_document_get_readonly (doc) &&
				  !(lockdown & GEDIT_LOCKDOWN_SAVE_TO_DISK));

	action = gtk_action_group_get_action (window->priv->action_group,
					      "FileSaveAs");
	gtk_action_set_sensitive (action,
				  (state_normal ||
				   (state == GEDIT_VIEW_CONTAINER_STATE_SAVING_ERROR) ||
				   (state == GEDIT_VIEW_CONTAINER_STATE_EXTERNALLY_MODIFIED_NOTIFICATION) ||
				   (state == GEDIT_VIEW_CONTAINER_STATE_SHOWING_PRINT_PREVIEW)) &&
				  !(lockdown & GEDIT_LOCKDOWN_SAVE_TO_DISK));

	action = gtk_action_group_get_action (window->priv->action_group,
					      "FileRevert");
	gtk_action_set_sensitive (action,
				  (state_normal ||
				   (state == GEDIT_VIEW_CONTAINER_STATE_EXTERNALLY_MODIFIED_NOTIFICATION)) &&
				  !gedit_document_is_untitled (doc));

	action = gtk_action_group_get_action (window->priv->action_group,
					      "FilePrintPreview");
	gtk_action_set_sensitive (action,
				  state_normal &&
				  !(lockdown & GEDIT_LOCKDOWN_PRINTING));

	action = gtk_action_group_get_action (window->priv->action_group,
					      "FilePrint");
	gtk_action_set_sensitive (action,
				  (state_normal ||
				  (state == GEDIT_VIEW_CONTAINER_STATE_SHOWING_PRINT_PREVIEW)) &&
				  !(lockdown & GEDIT_LOCKDOWN_PRINTING));
				  
	action = gtk_action_group_get_action (window->priv->action_group,
					      "FileClose");

	gtk_action_set_sensitive (action,
				  (state != GEDIT_VIEW_CONTAINER_STATE_CLOSING) &&
				  (state != GEDIT_VIEW_CONTAINER_STATE_SAVING) &&
				  (state != GEDIT_VIEW_CONTAINER_STATE_SHOWING_PRINT_PREVIEW) &&
				  (state != GEDIT_VIEW_CONTAINER_STATE_PRINTING) &&
				  (state != GEDIT_VIEW_CONTAINER_STATE_PRINT_PREVIEWING) &&
				  (state != GEDIT_VIEW_CONTAINER_STATE_SAVING_ERROR));

	action = gtk_action_group_get_action (window->priv->action_group,
					      "EditUndo");
	gtk_action_set_sensitive (action, 
				  state_normal &&
				  gedit_document_can_undo (doc));

	action = gtk_action_group_get_action (window->priv->action_group,
					      "EditRedo");
	gtk_action_set_sensitive (action, 
				  state_normal &&
				  gedit_document_can_redo (doc));

	action = gtk_action_group_get_action (window->priv->action_group,
					      "EditCut");
	gtk_action_set_sensitive (action,
				  state_normal &&
				  editable &&
				  gedit_document_get_has_selection (doc));

	action = gtk_action_group_get_action (window->priv->action_group,
					      "EditCopy");
	gtk_action_set_sensitive (action,
				  (state_normal ||
				   state == GEDIT_VIEW_CONTAINER_STATE_EXTERNALLY_MODIFIED_NOTIFICATION) &&
				  gedit_document_get_has_selection (doc));
				  
	action = gtk_action_group_get_action (window->priv->action_group,
					      "EditPaste");
	if (state_normal && editable)
	{
		set_paste_sensitivity_according_to_clipboard (window,
							      clipboard);
	}
	else
	{
		gtk_action_set_sensitive (action, FALSE);
	}

	action = gtk_action_group_get_action (window->priv->action_group,
					      "EditDelete");
	gtk_action_set_sensitive (action,
				  state_normal &&
				  editable &&
				  gedit_document_get_has_selection (doc));
	
	action = gtk_action_group_get_action (window->priv->action_group,
					      "ViewHighlightMode");
	gtk_action_set_sensitive (action, 
				  (state != GEDIT_VIEW_CONTAINER_STATE_CLOSING) &&
				  gedit_prefs_manager_get_enable_syntax_highlighting ());

	set_sensitivity_for_search_items (window, doc, state, state_normal, editable);

	update_next_prev_doc_sensitivity (window, page);

	gedit_plugins_engine_update_plugins_ui (gedit_plugins_engine_get_default (),
						window);
}

static void
language_toggled (GtkToggleAction *action,
		  GeditWindow     *window)
{
	GeditDocument *doc;
	GtkSourceLanguage *lang;
	const gchar *lang_id;

	if (gtk_toggle_action_get_active (action) == FALSE)
		return;

	doc = gedit_window_get_active_document (window);
	if (doc == NULL && !GEDIT_IS_TEXT_BUFFER (doc))
		return;

	lang_id = gtk_action_get_name (GTK_ACTION (action));
	
	if (strcmp (lang_id, LANGUAGE_NONE) == 0)
	{
		/* Normal (no highlighting) */
		lang = NULL;
	}
	else
	{
		lang = gtk_source_language_manager_get_language (
				gedit_get_language_manager (),
				lang_id);
		if (lang == NULL)
		{
			g_warning ("Could not get language %s\n", lang_id);
		}
	}

	gedit_text_buffer_set_language (GEDIT_TEXT_BUFFER (doc), lang);
}

static gchar *
escape_section_name (const gchar *name)
{
	gchar *ret;

	ret = g_markup_escape_text (name, -1);

	/* Replace '/' with '-' to avoid problems in xml paths */
	g_strdelimit (ret, "/", '-');

	return ret;
}

static void
create_language_menu_item (GtkSourceLanguage *lang,
			   gint               index,
			   guint              ui_id,
			   GeditWindow       *window)
{
	GtkAction *section_action;
	GtkRadioAction *action;
	GtkAction *normal_action;
	GSList *group;
	const gchar *section;
	gchar *escaped_section;
	const gchar *lang_id;
	const gchar *lang_name;
	gchar *escaped_lang_name;
	gchar *tip;
	gchar *path;

	section = gtk_source_language_get_section (lang);
	escaped_section = escape_section_name (section);

	/* check if the section submenu exists or create it */
	section_action = gtk_action_group_get_action (window->priv->languages_action_group,
						      escaped_section);

	if (section_action == NULL)
	{
		gchar *section_name;
		
		section_name = gedit_utils_escape_underscores (section, -1);
		
		section_action = gtk_action_new (escaped_section,
						 section_name,
						 NULL,
						 NULL);
						 
		g_free (section_name);

		gtk_action_group_add_action (window->priv->languages_action_group,
					     section_action);
		g_object_unref (section_action);

		gtk_ui_manager_add_ui (window->priv->manager,
				       ui_id,
				       "/MenuBar/ViewMenu/ViewHighlightModeMenu/LanguagesMenuPlaceholder",
				       escaped_section,
				       escaped_section,
				       GTK_UI_MANAGER_MENU,
				       FALSE);
	}

	/* now add the language item to the section */
	lang_name = gtk_source_language_get_name (lang);
	lang_id = gtk_source_language_get_id (lang);
	
	escaped_lang_name = gedit_utils_escape_underscores (lang_name, -1);
	
	tip = g_strdup_printf (_("Use %s highlight mode"), lang_name);
	path = g_strdup_printf ("/MenuBar/ViewMenu/ViewHighlightModeMenu/LanguagesMenuPlaceholder/%s",
				escaped_section);

	action = gtk_radio_action_new (lang_id,
				       escaped_lang_name,
				       tip,
				       NULL,
				       index);

	g_free (escaped_lang_name);

	/* Action is added with a NULL accel to make the accel overridable */
	gtk_action_group_add_action_with_accel (window->priv->languages_action_group,
						GTK_ACTION (action),
						NULL);
	g_object_unref (action);

	/* add the action to the same radio group of the "Normal" action */
	normal_action = gtk_action_group_get_action (window->priv->languages_action_group,
						     LANGUAGE_NONE);
	group = gtk_radio_action_get_group (GTK_RADIO_ACTION (normal_action));
	gtk_radio_action_set_group (action, group);

	g_signal_connect (action,
			  "activate",
			  G_CALLBACK (language_toggled),
			  window);

	gtk_ui_manager_add_ui (window->priv->manager,
			       ui_id,
			       path,
			       lang_id, 
			       lang_id,
			       GTK_UI_MANAGER_MENUITEM,
			       FALSE);

	g_free (path);
	g_free (tip);
	g_free (escaped_section);
}

static void
create_languages_menu (GeditWindow *window)
{
	GtkRadioAction *action_none;
	GSList *languages;
	GSList *l;
	guint id;
	gint i;

	gedit_debug (DEBUG_WINDOW);

	/* add the "Plain Text" item before all the others */
	
	/* Translators: "Plain Text" means that no highlight mode is selected in the 
	 * "View->Highlight Mode" submenu and so syntax highlighting is disabled */
	action_none = gtk_radio_action_new (LANGUAGE_NONE, _("Plain Text"),
					    _("Disable syntax highlighting"),
					    NULL,
					    -1);

	gtk_action_group_add_action (window->priv->languages_action_group,
				     GTK_ACTION (action_none));
	g_object_unref (action_none);

	g_signal_connect (action_none,
			  "activate",
			  G_CALLBACK (language_toggled),
			  window);

	id = gtk_ui_manager_new_merge_id (window->priv->manager);

	gtk_ui_manager_add_ui (window->priv->manager,
			       id,
			       "/MenuBar/ViewMenu/ViewHighlightModeMenu/LanguagesMenuPlaceholder",
			       LANGUAGE_NONE, 
			       LANGUAGE_NONE,
			       GTK_UI_MANAGER_MENUITEM,
			       TRUE);

	gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action_none), TRUE);

	/* now add all the known languages */
	languages = gedit_language_manager_list_languages_sorted (
						gedit_get_language_manager (),
						FALSE);

	for (l = languages, i = 0; l != NULL; l = l->next, ++i)
	{
		create_language_menu_item (l->data,
					   i,
					   id,
					   window);
	}

	g_slist_free (languages);
}

static void
update_languages_menu (GeditWindow *window)
{
	GeditDocument *doc;
	GList *actions;
	GList *l;
	GtkAction *action;
	GtkSourceLanguage *lang;
	const gchar *lang_id;

	doc = gedit_window_get_active_document (window);
	if (doc == NULL && !GEDIT_TEXT_BUFFER (doc))
		return;

	lang = gedit_text_buffer_get_language (GEDIT_TEXT_BUFFER (doc));
	if (lang != NULL)
		lang_id = gtk_source_language_get_id (lang);
	else
		lang_id = LANGUAGE_NONE;

	actions = gtk_action_group_list_actions (window->priv->languages_action_group);

	/* prevent recursion */
	for (l = actions; l != NULL; l = l->next)
	{
		g_signal_handlers_block_by_func (GTK_ACTION (l->data),
						 G_CALLBACK (language_toggled),
						 window);
	}

	action = gtk_action_group_get_action (window->priv->languages_action_group,
					      lang_id);

	gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), TRUE);

	for (l = actions; l != NULL; l = l->next)
	{
		g_signal_handlers_unblock_by_func (GTK_ACTION (l->data),
						   G_CALLBACK (language_toggled),
						   window);
	}

	g_list_free (actions);
}

void
_gedit_recent_add (GeditWindow *window,
		   const gchar *uri,
		   const gchar *mime)
{
	GtkRecentManager *recent_manager;
	GtkRecentData *recent_data;

	static gchar *groups[2] = {
		"gedit",
		NULL
	};

	recent_manager =  gtk_recent_manager_get_default ();

	recent_data = g_slice_new (GtkRecentData);

	recent_data->display_name = NULL;
	recent_data->description = NULL;
	recent_data->mime_type = (gchar *) mime;
	recent_data->app_name = (gchar *) g_get_application_name ();
	recent_data->app_exec = g_strjoin (" ", g_get_prgname (), "%u", NULL);
	recent_data->groups = groups;
	recent_data->is_private = FALSE;

	gtk_recent_manager_add_full (recent_manager,
				     uri,
				     recent_data);

	g_free (recent_data->app_exec);

	g_slice_free (GtkRecentData, recent_data);
}

void
_gedit_recent_remove (GeditWindow *window,
		      const gchar *uri)
{
	GtkRecentManager *recent_manager;

	recent_manager =  gtk_recent_manager_get_default ();

	gtk_recent_manager_remove_item (recent_manager, uri, NULL);
}

static void
open_recent_file (const gchar *uri,
		  GeditWindow *window)
{
	GSList *uris = NULL;

	uris = g_slist_prepend (uris, (gpointer) uri);

	if (gedit_commands_load_uris (window, uris, NULL, 0) != 1)
	{
		_gedit_recent_remove (window, uri);
	}

	g_slist_free (uris);
}

static void
recent_chooser_item_activated (GtkRecentChooser *chooser,
			       GeditWindow      *window)
{
	gchar *uri;

	uri = gtk_recent_chooser_get_current_uri (chooser);

	open_recent_file (uri, window);

	g_free (uri);
}

static void
recents_menu_activate (GtkAction   *action,
		       GeditWindow *window)
{
	GtkRecentInfo *info;
	const gchar *uri;

	info = g_object_get_data (G_OBJECT (action), "gtk-recent-info");
	g_return_if_fail (info != NULL);

	uri = gtk_recent_info_get_uri (info);

	open_recent_file (uri, window);
}

static gint
sort_recents_mru (GtkRecentInfo *a, GtkRecentInfo *b)
{
	return (gtk_recent_info_get_modified (b) - gtk_recent_info_get_modified (a));
}

static void	update_recent_files_menu (GeditWindow *window);

static void
recent_manager_changed (GtkRecentManager *manager,
			GeditWindow      *window)
{
	/* regenerate the menu when the model changes */
	update_recent_files_menu (window);
}

/*
 * Manually construct the inline recents list in the File menu.
 * Hopefully gtk 2.12 will add support for it.
 */
static void
update_recent_files_menu (GeditWindow *window)
{
	GeditWindowPrivate *p = window->priv;
	GtkRecentManager *recent_manager;
	gint max_recents;
	GList *actions, *l, *items;
	GList *filtered_items = NULL;
	gint i;

	gedit_debug (DEBUG_WINDOW);

	max_recents = gedit_prefs_manager_get_max_recents ();

	g_return_if_fail (p->recents_action_group != NULL);

	if (p->recents_menu_ui_id != 0)
		gtk_ui_manager_remove_ui (p->manager,
					  p->recents_menu_ui_id);

	actions = gtk_action_group_list_actions (p->recents_action_group);
	for (l = actions; l != NULL; l = l->next)
	{
		g_signal_handlers_disconnect_by_func (GTK_ACTION (l->data),
						      G_CALLBACK (recents_menu_activate),
						      window);
 		gtk_action_group_remove_action (p->recents_action_group,
						GTK_ACTION (l->data));
	}
	g_list_free (actions);

	p->recents_menu_ui_id = gtk_ui_manager_new_merge_id (p->manager);

	recent_manager =  gtk_recent_manager_get_default ();
	items = gtk_recent_manager_get_items (recent_manager);

	/* filter */
	for (l = items; l != NULL; l = l->next)
	{
		GtkRecentInfo *info = l->data;

		if (!gtk_recent_info_has_group (info, "gedit"))
			continue;

		filtered_items = g_list_prepend (filtered_items, info);
	}

	/* sort */
	filtered_items = g_list_sort (filtered_items,
				      (GCompareFunc) sort_recents_mru);

	i = 0;
	for (l = filtered_items; l != NULL; l = l->next)
	{
		gchar *action_name;
		const gchar *display_name;
		gchar *escaped;
		gchar *label;
		gchar *uri;
		gchar *ruri;
		gchar *tip;
		GtkAction *action;
		GtkRecentInfo *info = l->data;

		/* clamp */
		if (i >= max_recents)
			break;

		i++;

		action_name = g_strdup_printf ("recent-info-%d", i);

		display_name = gtk_recent_info_get_display_name (info);
		escaped = gedit_utils_escape_underscores (display_name, -1);
		if (i >= 10)
			label = g_strdup_printf ("%d.  %s",
						 i, 
						 escaped);
		else
			label = g_strdup_printf ("_%d.  %s",
						 i, 
						 escaped);
		g_free (escaped);

		/* gtk_recent_info_get_uri_display (info) is buggy and
		 * works only for local files */
		uri = gedit_utils_uri_for_display (gtk_recent_info_get_uri (info));
		ruri = gedit_utils_replace_home_dir_with_tilde (uri);
		g_free (uri);

		/* Translators: %s is a URI */
		tip = g_strdup_printf (_("Open '%s'"), ruri);
		g_free (ruri);

		action = gtk_action_new (action_name,
					 label,
					 tip,
					 NULL);

		g_object_set_data_full (G_OBJECT (action),
					"gtk-recent-info",
					gtk_recent_info_ref (info),
					(GDestroyNotify) gtk_recent_info_unref);

		g_signal_connect (action,
				  "activate",
				  G_CALLBACK (recents_menu_activate),
				  window);

		gtk_action_group_add_action (p->recents_action_group,
					     action);
		g_object_unref (action);

		gtk_ui_manager_add_ui (p->manager,
				       p->recents_menu_ui_id,
				       "/MenuBar/FileMenu/FileRecentsPlaceholder",
				       action_name,
				       action_name,
				       GTK_UI_MANAGER_MENUITEM,
				       FALSE);

		g_free (action_name);
		g_free (label);
		g_free (tip);
	}

	g_list_free (filtered_items);

	g_list_foreach (items, (GFunc) gtk_recent_info_unref, NULL);
	g_list_free (items);
}

static void
set_non_homogeneus (GtkWidget *widget, gpointer data)
{
	gtk_tool_item_set_homogeneous (GTK_TOOL_ITEM (widget), FALSE);
}

static void
toolbar_visibility_changed (GtkWidget   *toolbar,
			    GeditWindow *window)
{
	gboolean visible;
	GtkAction *action;

	visible = GTK_WIDGET_VISIBLE (toolbar);

	if (gedit_prefs_manager_toolbar_visible_can_set ())
		gedit_prefs_manager_set_toolbar_visible (visible);

	action = gtk_action_group_get_action (window->priv->always_sensitive_action_group,
					      "ViewToolbar");

	if (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)) != visible)
		gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), visible);
}

static GtkWidget *
setup_toolbar_open_button (GeditWindow *window,
			   GtkWidget *toolbar)
{
	GtkRecentManager *recent_manager;
	GtkRecentFilter *filter;
	GtkWidget *toolbar_recent_menu;
	GtkToolItem *open_button;
	GtkAction *action;

	recent_manager = gtk_recent_manager_get_default ();

	/* recent files menu tool button */
	toolbar_recent_menu = gtk_recent_chooser_menu_new_for_manager (recent_manager);

	gtk_recent_chooser_set_local_only (GTK_RECENT_CHOOSER (toolbar_recent_menu),
					   FALSE);
	gtk_recent_chooser_set_sort_type (GTK_RECENT_CHOOSER (toolbar_recent_menu),
					  GTK_RECENT_SORT_MRU);
	gtk_recent_chooser_set_limit (GTK_RECENT_CHOOSER (toolbar_recent_menu),
				      gedit_prefs_manager_get_max_recents ());

	filter = gtk_recent_filter_new ();
	gtk_recent_filter_add_group (filter, "gedit");
	gtk_recent_chooser_set_filter (GTK_RECENT_CHOOSER (toolbar_recent_menu),
				       filter);

	g_signal_connect (toolbar_recent_menu,
			  "item_activated",
			  G_CALLBACK (recent_chooser_item_activated),
			  window);
	
	/* add the custom Open button to the toolbar */
	open_button = gtk_menu_tool_button_new_from_stock (GTK_STOCK_OPEN);
	gtk_menu_tool_button_set_menu (GTK_MENU_TOOL_BUTTON (open_button),
				       toolbar_recent_menu);

	gtk_tool_item_set_tooltip_text (open_button, _("Open a file"));
	gtk_menu_tool_button_set_arrow_tooltip_text (GTK_MENU_TOOL_BUTTON (open_button),
						     _("Open a recently used file"));

	action = gtk_action_group_get_action (window->priv->always_sensitive_action_group,
					      "FileOpen");
	g_object_set (action,
		      "is_important", TRUE,
		      "short_label", _("Open"),
		      NULL);
	gtk_action_connect_proxy (action, GTK_WIDGET (open_button));

	gtk_toolbar_insert (GTK_TOOLBAR (toolbar),
			    open_button,
			    1);
	
	return toolbar_recent_menu;
}

static void
create_menu_bar_and_toolbar (GeditWindow *window, 
			     GtkWidget   *main_box)
{
	GtkActionGroup *action_group;
	GtkAction *action;
	GtkUIManager *manager;
	GtkWidget *menubar;
	GtkRecentManager *recent_manager;
	GError *error = NULL;
	gchar *ui_file;

	gedit_debug (DEBUG_WINDOW);

	manager = gtk_ui_manager_new ();
	window->priv->manager = manager;

	gtk_window_add_accel_group (GTK_WINDOW (window),
				    gtk_ui_manager_get_accel_group (manager));

	action_group = gtk_action_group_new ("GeditWindowAlwaysSensitiveActions");
	gtk_action_group_set_translation_domain (action_group, NULL);
	gtk_action_group_add_actions (action_group,
				      gedit_always_sensitive_menu_entries,
				      G_N_ELEMENTS (gedit_always_sensitive_menu_entries),
				      window);
	gtk_action_group_add_toggle_actions (action_group,
					     gedit_always_sensitive_toggle_menu_entries,
					     G_N_ELEMENTS (gedit_always_sensitive_toggle_menu_entries),
					     window);

	gtk_ui_manager_insert_action_group (manager, action_group, 0);
	g_object_unref (action_group);
	window->priv->always_sensitive_action_group = action_group;

	action_group = gtk_action_group_new ("GeditWindowActions");
	gtk_action_group_set_translation_domain (action_group, NULL);
	gtk_action_group_add_actions (action_group,
				      gedit_menu_entries,
				      G_N_ELEMENTS (gedit_menu_entries),
				      window);
	gtk_ui_manager_insert_action_group (manager, action_group, 0);
	g_object_unref (action_group);
	window->priv->action_group = action_group;

	/* set short labels to use in the toolbar */
	action = gtk_action_group_get_action (action_group, "FileSave");
	g_object_set (action, "short_label", _("Save"), NULL);
	action = gtk_action_group_get_action (action_group, "FilePrint");
	g_object_set (action, "short_label", _("Print"), NULL);
	action = gtk_action_group_get_action (action_group, "SearchFind");
	g_object_set (action, "short_label", _("Find"), NULL);
	action = gtk_action_group_get_action (action_group, "SearchReplace");
	g_object_set (action, "short_label", _("Replace"), NULL);

	/* set which actions should have priority on the toolbar */
	action = gtk_action_group_get_action (action_group, "FileSave");
	g_object_set (action, "is_important", TRUE, NULL);
	action = gtk_action_group_get_action (action_group, "EditUndo");
	g_object_set (action, "is_important", TRUE, NULL);

	action_group = gtk_action_group_new ("GeditQuitWindowActions");
	gtk_action_group_set_translation_domain (action_group, NULL);
	gtk_action_group_add_actions (action_group,
				      gedit_quit_menu_entries,
				      G_N_ELEMENTS (gedit_quit_menu_entries),
				      window);

	gtk_ui_manager_insert_action_group (manager, action_group, 0);
	g_object_unref (action_group);
	window->priv->quit_action_group = action_group;

	action_group = gtk_action_group_new ("GeditWindowPanesActions");
	gtk_action_group_set_translation_domain (action_group, NULL);
	gtk_action_group_add_toggle_actions (action_group,
					     gedit_panes_toggle_menu_entries,
					     G_N_ELEMENTS (gedit_panes_toggle_menu_entries),
					     window);

	gtk_ui_manager_insert_action_group (manager, action_group, 0);
	g_object_unref (action_group);
	window->priv->panes_action_group = action_group;

	/* now load the UI definition */
	ui_file = gedit_dirs_get_ui_file (GEDIT_UIFILE);
	gtk_ui_manager_add_ui_from_file (manager, ui_file, &error);
	if (error != NULL)
	{
		g_warning ("Could not merge %s: %s", ui_file, error->message);
		g_error_free (error);
	}
	g_free (ui_file);

#if !GTK_CHECK_VERSION (2, 17, 4)
	/* merge page setup menu manually since we cannot have conditional
	 * sections in gedit-ui.xml */
	{
		guint merge_id;
		GeditLockdownMask lockdown;

		merge_id = gtk_ui_manager_new_merge_id (manager);
		gtk_ui_manager_add_ui (manager,
				       merge_id,
				       "/MenuBar/FileMenu/FileOps_5",
				       "FilePageSetupMenu",
				       "FilePageSetup",
				       GTK_UI_MANAGER_MENUITEM,
				       FALSE);

		lockdown = gedit_app_get_lockdown (gedit_app_get_default ());
		action = gtk_action_group_get_action (window->priv->action_group,
						      "FilePageSetup");
		gtk_action_set_sensitive (action, 
					  !(lockdown & GEDIT_LOCKDOWN_PRINT_SETUP));
	}
#endif

	/* show tooltips in the statusbar */
	g_signal_connect (manager,
			  "connect_proxy",
			  G_CALLBACK (connect_proxy_cb),
			  window);
	g_signal_connect (manager,
			  "disconnect_proxy",
			  G_CALLBACK (disconnect_proxy_cb),
			  window);

	/* recent files menu */
	action_group = gtk_action_group_new ("RecentFilesActions");
	gtk_action_group_set_translation_domain (action_group, NULL);
	window->priv->recents_action_group = action_group;
	gtk_ui_manager_insert_action_group (manager, action_group, 0);
	g_object_unref (action_group);

	recent_manager = gtk_recent_manager_get_default ();
	window->priv->recents_handler_id = g_signal_connect (recent_manager,
							     "changed",
							     G_CALLBACK (recent_manager_changed),
							     window);
	update_recent_files_menu (window);

	/* languages menu */
	action_group = gtk_action_group_new ("LanguagesActions");
	gtk_action_group_set_translation_domain (action_group, NULL);
	window->priv->languages_action_group = action_group;
	gtk_ui_manager_insert_action_group (manager, action_group, 0);
	g_object_unref (action_group);
	create_languages_menu (window);

	/* list of open documents menu */
	action_group = gtk_action_group_new ("DocumentsListActions");
	gtk_action_group_set_translation_domain (action_group, NULL);
	window->priv->documents_list_action_group = action_group;
	gtk_ui_manager_insert_action_group (manager, action_group, 0);
	g_object_unref (action_group);

	menubar = gtk_ui_manager_get_widget (manager, "/MenuBar");
	gtk_box_pack_start (GTK_BOX (main_box), 
			    menubar, 
			    FALSE, 
			    FALSE, 
			    0);

	window->priv->toolbar = gtk_ui_manager_get_widget (manager, "/ToolBar");
	gtk_box_pack_start (GTK_BOX (main_box),
			    window->priv->toolbar,
			    FALSE,
			    FALSE,
			    0);

	set_toolbar_style (window, NULL);
	
	window->priv->toolbar_recent_menu = setup_toolbar_open_button (window,
								       window->priv->toolbar);

	gtk_container_foreach (GTK_CONTAINER (window->priv->toolbar),
			       (GtkCallback)set_non_homogeneus,
			       NULL);

	g_signal_connect_after (G_OBJECT (window->priv->toolbar),
				"show",
				G_CALLBACK (toolbar_visibility_changed),
				window);
	g_signal_connect_after (G_OBJECT (window->priv->toolbar),
				"hide",
				G_CALLBACK (toolbar_visibility_changed),
				window);
}

static void
documents_list_menu_activate (GtkToggleAction *action,
			      GeditWindow     *window)
{
	gint n;

	if (gtk_toggle_action_get_active (action) == FALSE)
		return;

	n = gtk_radio_action_get_current_value (GTK_RADIO_ACTION (action));
	gtk_notebook_set_current_page (GTK_NOTEBOOK (window->priv->notebook), n);
}

static gchar *
get_menu_tip_for_page (GeditPage *page)
{
	GeditViewContainer *container;
	GeditDocument *doc;
	gchar *uri;
	gchar *ruri;
	gchar *tip;

	container = gedit_page_get_active_view_container (page);
	doc = gedit_view_container_get_document (container);

	uri = gedit_document_get_uri_for_display (doc);
	ruri = gedit_utils_replace_home_dir_with_tilde (uri);
	g_free (uri);

	/* Translators: %s is a URI */
	tip =  g_strdup_printf (_("Activate '%s'"), ruri);
	g_free (ruri);
	
	return tip;
}

static void
update_documents_list_menu (GeditWindow *window)
{
	GeditWindowPrivate *p = window->priv;
	GList *actions, *l;
	gint n, i;
	guint id;
	GSList *group = NULL;

	gedit_debug (DEBUG_WINDOW);

	g_return_if_fail (p->documents_list_action_group != NULL);

	if (p->documents_list_menu_ui_id != 0)
		gtk_ui_manager_remove_ui (p->manager,
					  p->documents_list_menu_ui_id);

	actions = gtk_action_group_list_actions (p->documents_list_action_group);
	for (l = actions; l != NULL; l = l->next)
	{
		g_signal_handlers_disconnect_by_func (GTK_ACTION (l->data),
						      G_CALLBACK (documents_list_menu_activate),
						      window);
 		gtk_action_group_remove_action (p->documents_list_action_group,
						GTK_ACTION (l->data));
	}
	g_list_free (actions);

	n = gtk_notebook_get_n_pages (GTK_NOTEBOOK (p->notebook));

	id = (n > 0) ? gtk_ui_manager_new_merge_id (p->manager) : 0;

	for (i = 0; i < n; i++)
	{
		GtkWidget *page;
		GeditViewContainer *container;
		GtkRadioAction *action;
		gchar *action_name;
		gchar *page_name;
		gchar *name;
		gchar *tip;
		gchar *accel;

		page = gtk_notebook_get_nth_page (GTK_NOTEBOOK (p->notebook), i);
		container = gedit_page_get_active_view_container (GEDIT_PAGE (page));

		/* NOTE: the action is associated to the position of the tab in
		 * the notebook not to the tab itself! This is needed to work
		 * around the gtk+ bug #170727: gtk leaves around the accels
		 * of the action. Since the accel depends on the tab position
		 * the problem is worked around, action with the same name always
		 * get the same accel.
		 */
		action_name = g_strdup_printf ("Tab_%d", i);
		page_name = _gedit_view_container_get_name (container);
		name = gedit_utils_escape_underscores (page_name, -1);
		tip =  get_menu_tip_for_page (GEDIT_PAGE (page));

		/* alt + 1, 2, 3... 0 to switch to the first ten tabs */
		accel = (i < 10) ? g_strdup_printf ("<alt>%d", (i + 1) % 10) : NULL;

		action = gtk_radio_action_new (action_name,
					       name,
					       tip,
					       NULL,
					       i);

		if (group != NULL)
			gtk_radio_action_set_group (action, group);

		/* note that group changes each time we add an action, so it must be updated */
		group = gtk_radio_action_get_group (action);

		gtk_action_group_add_action_with_accel (p->documents_list_action_group,
							GTK_ACTION (action),
							accel);

		g_signal_connect (action,
				  "activate",
				  G_CALLBACK (documents_list_menu_activate),
				  window);

		gtk_ui_manager_add_ui (p->manager,
				       id,
				       "/MenuBar/DocumentsMenu/DocumentsListPlaceholder",
				       action_name, action_name,
				       GTK_UI_MANAGER_MENUITEM,
				       FALSE);

		if (GEDIT_PAGE (page) == p->active_page)
			gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), TRUE);

		g_object_unref (action);

		g_free (action_name);
		g_free (page_name);
		g_free (name);
		g_free (tip);
		g_free (accel);
	}

	p->documents_list_menu_ui_id = id;
}

/* Returns TRUE if status bar is visible */
static gboolean
set_statusbar_style (GeditWindow *window,
		     GeditWindow *origin)
{
	GtkAction *action;
	
	gboolean visible;

	if (origin == NULL)
		visible = gedit_prefs_manager_get_statusbar_visible ();
	else
		visible = GTK_WIDGET_VISIBLE (origin->priv->statusbar);

	if (visible)
		gtk_widget_show (window->priv->statusbar);
	else
		gtk_widget_hide (window->priv->statusbar);

	action = gtk_action_group_get_action (window->priv->always_sensitive_action_group,
					      "ViewStatusbar");
					      
	if (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)) != visible)
		gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), visible);
		
	return visible;
}

static void
statusbar_visibility_changed (GtkWidget   *statusbar,
			      GeditWindow *window)
{
	gboolean visible;
	GtkAction *action;

	visible = GTK_WIDGET_VISIBLE (statusbar);

	if (gedit_prefs_manager_statusbar_visible_can_set ())
		gedit_prefs_manager_set_statusbar_visible (visible);

	action = gtk_action_group_get_action (window->priv->always_sensitive_action_group,
					      "ViewStatusbar");

	if (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)) != visible)
		gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), visible);
}

static void
tab_width_combo_changed (GeditStatusComboBox *combo,
			 GtkMenuItem         *item,
			 GeditWindow         *window)
{
	GeditView *view;
	guint width_data = 0;
	
	view = gedit_window_get_active_view (window);
	
	if (!view)
		return;
	
	width_data = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (item), TAB_WIDTH_DATA));
	
	if (width_data == 0)
		return;
	
	g_signal_handler_block (view, window->priv->tab_width_id);
	gtk_source_view_set_tab_width (GTK_SOURCE_VIEW (view), width_data);
	g_signal_handler_unblock (view, window->priv->tab_width_id);
}

static void
use_spaces_toggled (GtkCheckMenuItem *item,
		    GeditWindow      *window)
{
	GeditView *view;
	
	view = gedit_window_get_active_view (window);
	
	g_signal_handler_block (view, window->priv->spaces_instead_of_tabs_id);
	gtk_source_view_set_insert_spaces_instead_of_tabs (
			GTK_SOURCE_VIEW (view),
			gtk_check_menu_item_get_active (item));
	g_signal_handler_unblock (view, window->priv->spaces_instead_of_tabs_id);
}

static void
language_combo_changed (GeditStatusComboBox *combo,
			GtkMenuItem         *item,
			GeditWindow         *window)
{
	GeditDocument *doc;
	GtkSourceLanguage *language;
	
	doc = gedit_window_get_active_document (window);
	
	if (!doc && !GEDIT_TEXT_BUFFER (doc))
		return;
	
	language = GTK_SOURCE_LANGUAGE (g_object_get_data (G_OBJECT (item), LANGUAGE_DATA));
	
	g_signal_handler_block (doc, window->priv->language_changed_id);
	gedit_text_buffer_set_language (GEDIT_TEXT_BUFFER (doc), language);
	g_signal_handler_unblock (doc, window->priv->language_changed_id);
}

typedef struct
{
	const gchar *label;
	guint width;
} TabWidthDefinition;
	 
static void
fill_tab_width_combo (GeditWindow *window)
{
	static TabWidthDefinition defs[] = {
		{"2", 2},
		{"4", 4},
		{"8", 8},
		{"", 0}, /* custom size */
		{NULL, 0}
	};
	
	GeditStatusComboBox *combo = GEDIT_STATUS_COMBO_BOX (window->priv->tab_width_combo);
	guint i = 0;
	GtkWidget *item;
	
	while (defs[i].label != NULL)
	{
		item = gtk_menu_item_new_with_label (defs[i].label);
		g_object_set_data (G_OBJECT (item), TAB_WIDTH_DATA, GINT_TO_POINTER (defs[i].width));
		
		gedit_status_combo_box_add_item (combo,
						 GTK_MENU_ITEM (item),
						 defs[i].label);

		if (defs[i].width != 0)
			gtk_widget_show (item);

		++i;
	}
	
	item = gtk_separator_menu_item_new ();
	gedit_status_combo_box_add_item (combo, GTK_MENU_ITEM (item), NULL);
	gtk_widget_show (item);
	
	item = gtk_check_menu_item_new_with_label (_("Use Spaces"));
	gedit_status_combo_box_add_item (combo, GTK_MENU_ITEM (item), NULL);
	gtk_widget_show (item);
	
	g_signal_connect (item, 
			  "toggled", 
			  G_CALLBACK (use_spaces_toggled), 
			  window);
}

static void
fill_language_combo (GeditWindow *window)
{
	GtkSourceLanguageManager *manager;
	GSList *languages;
	GSList *item;
	GtkWidget *menu_item;
	const gchar *name;
	
	manager = gedit_get_language_manager ();
	languages = gedit_language_manager_list_languages_sorted (manager, FALSE);

	name = _("Plain Text");
	menu_item = gtk_menu_item_new_with_label (name);
	gtk_widget_show (menu_item);
	
	g_object_set_data (G_OBJECT (menu_item), LANGUAGE_DATA, NULL);
	gedit_status_combo_box_add_item (GEDIT_STATUS_COMBO_BOX (window->priv->language_combo),
					 GTK_MENU_ITEM (menu_item),
					 name);

	for (item = languages; item; item = item->next)
	{
		GtkSourceLanguage *lang = GTK_SOURCE_LANGUAGE (item->data);
		
		name = gtk_source_language_get_name (lang);
		menu_item = gtk_menu_item_new_with_label (name);
		gtk_widget_show (menu_item);
		
		g_object_set_data_full (G_OBJECT (menu_item),
				        LANGUAGE_DATA,		
					g_object_ref (lang),
					(GDestroyNotify)g_object_unref);

		gedit_status_combo_box_add_item (GEDIT_STATUS_COMBO_BOX (window->priv->language_combo),
						 GTK_MENU_ITEM (menu_item),
						 name);
	}
	
	g_slist_free (languages);
}

static void
create_statusbar (GeditWindow *window, 
		  GtkWidget   *main_box)
{
	gedit_debug (DEBUG_WINDOW);

	window->priv->statusbar = gedit_statusbar_new ();

	window->priv->generic_message_cid = gtk_statusbar_get_context_id
		(GTK_STATUSBAR (window->priv->statusbar), "generic_message");
	window->priv->tip_message_cid = gtk_statusbar_get_context_id
		(GTK_STATUSBAR (window->priv->statusbar), "tip_message");

	gtk_box_pack_end (GTK_BOX (main_box),
			  window->priv->statusbar,
			  FALSE, 
			  TRUE, 
			  0);

	window->priv->tab_width_combo = gedit_status_combo_box_new (_("Tab Width"));
	gtk_widget_show (window->priv->tab_width_combo);
	gtk_box_pack_end (GTK_BOX (window->priv->statusbar),
			  window->priv->tab_width_combo,
			  FALSE,
			  TRUE,
			  0);

	fill_tab_width_combo (window);

	g_signal_connect (G_OBJECT (window->priv->tab_width_combo),
			  "changed",
			  G_CALLBACK (tab_width_combo_changed),
			  window);
			  
	window->priv->language_combo = gedit_status_combo_box_new (NULL);
	gtk_widget_show (window->priv->language_combo);
	gtk_box_pack_end (GTK_BOX (window->priv->statusbar),
			  window->priv->language_combo,
			  FALSE,
			  TRUE,
			  0);

	fill_language_combo (window);

	g_signal_connect (G_OBJECT (window->priv->language_combo),
			  "changed",
			  G_CALLBACK (language_combo_changed),
			  window);

	g_signal_connect_after (G_OBJECT (window->priv->statusbar),
				"show",
				G_CALLBACK (statusbar_visibility_changed),
				window);
	g_signal_connect_after (G_OBJECT (window->priv->statusbar),
				"hide",
				G_CALLBACK (statusbar_visibility_changed),
				window);

	set_statusbar_style (window, NULL);
}

static GeditWindow *
clone_window (GeditWindow *origin)
{
	GeditWindow *window;
	GdkScreen *screen;
	GeditApp  *app;
	gint panel_page;

	gedit_debug (DEBUG_WINDOW);	

	app = gedit_app_get_default ();

	screen = gtk_window_get_screen (GTK_WINDOW (origin));
	window = gedit_app_create_window (app, screen);

	if ((origin->priv->window_state & GDK_WINDOW_STATE_MAXIMIZED) != 0)
	{
		gint w, h;

		gedit_prefs_manager_get_default_window_size (&w, &h);
		gtk_window_set_default_size (GTK_WINDOW (window), w, h);
		gtk_window_maximize (GTK_WINDOW (window));
	}
	else
	{
		gtk_window_set_default_size (GTK_WINDOW (window),
					     origin->priv->width,
					     origin->priv->height);

		gtk_window_unmaximize (GTK_WINDOW (window));
	}		

	if ((origin->priv->window_state & GDK_WINDOW_STATE_STICKY ) != 0)
		gtk_window_stick (GTK_WINDOW (window));
	else
		gtk_window_unstick (GTK_WINDOW (window));

	/* set the panes size, the paned position will be set when
	 * they are mapped */ 
	window->priv->side_panel_size = origin->priv->side_panel_size;
	window->priv->bottom_panel_size = origin->priv->bottom_panel_size;

	panel_page = _gedit_panel_get_active_item_id (GEDIT_PANEL (origin->priv->side_panel));
	_gedit_panel_set_active_item_by_id (GEDIT_PANEL (window->priv->side_panel),
					    panel_page);

	panel_page = _gedit_panel_get_active_item_id (GEDIT_PANEL (origin->priv->bottom_panel));
	_gedit_panel_set_active_item_by_id (GEDIT_PANEL (window->priv->bottom_panel),
					    panel_page);

	if (GTK_WIDGET_VISIBLE (origin->priv->side_panel))
		gtk_widget_show (window->priv->side_panel);
	else
		gtk_widget_hide (window->priv->side_panel);

	if (GTK_WIDGET_VISIBLE (origin->priv->bottom_panel))
		gtk_widget_show (window->priv->bottom_panel);
	else
		gtk_widget_hide (window->priv->bottom_panel);

	set_statusbar_style (window, origin);
	set_toolbar_style (window, origin);

	return window;
}

static void
update_cursor_position_statusbar (GeditDocument *doc, 
				  GeditWindow   *window)
{
	gint row, col;
	guint tab_size;
	GeditView *view;

	gedit_debug (DEBUG_WINDOW);

	if (doc != gedit_window_get_active_document (window)
	    || !GEDIT_IS_TEXT_BUFFER (doc))
		return;
	
	view = gedit_window_get_active_view (window);
	tab_size = gtk_source_view_get_tab_width (GTK_SOURCE_VIEW (view));
	
	gedit_text_buffer_get_cursor_position (GEDIT_TEXT_BUFFER (doc),
					       tab_size, &row, &col);
	
	gedit_statusbar_set_cursor_position (
				GEDIT_STATUSBAR (window->priv->statusbar),
				row + 1,
				col + 1);
}

static void
update_overwrite_mode_statusbar (GeditView *view, 
				 GeditWindow *window)
{
	if (view != gedit_window_get_active_view (window))
		return;
		
	/* Note that we have to use !gedit_view_get_overwrite since we
	   are in the in the signal handler of "toggle overwrite" that is
	   G_SIGNAL_RUN_LAST
	*/
	gedit_statusbar_set_overwrite (
			GEDIT_STATUSBAR (window->priv->statusbar),
			!gedit_view_get_overwrite (view));
}

#define MAX_TITLE_LENGTH 100

static void 
set_title (GeditWindow *window)
{
	GeditViewContainer *container;
	GeditDocument *doc = NULL;
	gchar *name;
	gchar *dirname = NULL;
	gchar *title = NULL;
	gint len;

	if (window->priv->active_page == NULL)
	{
		gtk_window_set_title (GTK_WINDOW (window), "gedit");
		return;
	}

	container = gedit_page_get_active_view_container (window->priv->active_page);
	doc = gedit_view_container_get_document (container);
	g_return_if_fail (doc != NULL);

	name = gedit_document_get_short_name_for_display (doc);

	len = g_utf8_strlen (name, -1);

	/* if the name is awfully long, truncate it and be done with it,
	 * otherwise also show the directory (ellipsized if needed)
	 */
	if (len > MAX_TITLE_LENGTH)
	{
		gchar *tmp;

		tmp = gedit_utils_str_middle_truncate (name,
						       MAX_TITLE_LENGTH);
		g_free (name);
		name = tmp;
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
			 * case to be acceptable. It's justa darn title afterall :)
			 */
			dirname = gedit_utils_str_middle_truncate (str, 
								   MAX (20, MAX_TITLE_LENGTH - len));
			g_free (str);
		}
	}

	if (gedit_document_get_modified (doc))
	{
		gchar *tmp_name;
		
		tmp_name = g_strdup_printf ("*%s", name);
		g_free (name);
		
		name = tmp_name;
	}		

	if (gedit_document_get_readonly (doc)) 
	{
		if (dirname != NULL)
			title = g_strdup_printf ("%s [%s] (%s) - gedit", 
						 name, 
						 _("Read Only"), 
						 dirname);
		else
			title = g_strdup_printf ("%s [%s] - gedit", 
						 name, 
						 _("Read Only"));
	} 
	else 
	{
		if (dirname != NULL)
			title = g_strdup_printf ("%s (%s) - gedit", 
						 name, 
						 dirname);
		else
			title = g_strdup_printf ("%s - gedit", 
						 name);
	}

	gtk_window_set_title (GTK_WINDOW (window), title);

	g_free (dirname);
	g_free (name);
	g_free (title);
}

#undef MAX_TITLE_LENGTH

static void
set_tab_width_item_blocked (GeditWindow *window,
			    GtkMenuItem *item)
{
	g_signal_handlers_block_by_func (window->priv->tab_width_combo, 
					 tab_width_combo_changed, 
					 window);
	
	gedit_status_combo_box_set_item (GEDIT_STATUS_COMBO_BOX (window->priv->tab_width_combo),
					 item);
	
	g_signal_handlers_unblock_by_func (window->priv->tab_width_combo, 
					   tab_width_combo_changed, 
					   window);
}

static void
spaces_instead_of_tabs_changed (GObject     *object,
		   		GParamSpec  *pspec,
		 		GeditWindow *window)
{
	GeditView *view = GEDIT_VIEW (object);
	gboolean active = gtk_source_view_get_insert_spaces_instead_of_tabs (
			GTK_SOURCE_VIEW (view));
	GList *children = gedit_status_combo_box_get_items (
			GEDIT_STATUS_COMBO_BOX (window->priv->tab_width_combo));
	GtkCheckMenuItem *item;
	
	item = GTK_CHECK_MENU_ITEM (g_list_last (children)->data);
	
	gtk_check_menu_item_set_active (item, active);
	
	g_list_free (children);
}

static void
tab_width_changed (GObject     *object,
		   GParamSpec  *pspec,
		   GeditWindow *window)
{
	GList *items;
	GList *item;
	GeditStatusComboBox *combo = GEDIT_STATUS_COMBO_BOX (window->priv->tab_width_combo);
	guint new_tab_width;
	gboolean found = FALSE;

	items = gedit_status_combo_box_get_items (combo);

	new_tab_width = gtk_source_view_get_tab_width (GTK_SOURCE_VIEW (object));
	
	for (item = items; item; item = item->next)
	{
		guint tab_width = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (item->data), TAB_WIDTH_DATA));
		
		if (tab_width == new_tab_width)
		{
			set_tab_width_item_blocked (window, GTK_MENU_ITEM (item->data));
			found = TRUE;
		}
		
		if (GTK_IS_SEPARATOR_MENU_ITEM (item->next->data))
		{
			if (!found)
			{
				/* Set for the last item the custom thing */
				gchar *text;
			
				text = g_strdup_printf ("%u", new_tab_width);
				gedit_status_combo_box_set_item_text (combo, 
								      GTK_MENU_ITEM (item->data),
								      text);

				gtk_label_set_text (GTK_LABEL (gtk_bin_get_child (GTK_BIN (item->data))),
						    text);
			
				set_tab_width_item_blocked (window, GTK_MENU_ITEM (item->data));
				gtk_widget_show (GTK_WIDGET (item->data));
			}
			else
			{
				gtk_widget_hide (GTK_WIDGET (item->data));
			}
			
			break;
		}
	}
	
	g_list_free (items);
}

static void
language_changed (GObject     *object,
		  GParamSpec  *pspec,
		  GeditWindow *window)
{
	GList *items;
	GList *item;
	GeditStatusComboBox *combo = GEDIT_STATUS_COMBO_BOX (window->priv->language_combo);
	GtkSourceLanguage *new_language;
	const gchar *new_id;
	
	items = gedit_status_combo_box_get_items (combo);

	new_language = gtk_source_buffer_get_language (GTK_SOURCE_BUFFER (object));
	
	if (new_language)
		new_id = gtk_source_language_get_id (new_language);
	else
		new_id = NULL;
	
	for (item = items; item; item = item->next)
	{
		GtkSourceLanguage *lang = g_object_get_data (G_OBJECT (item->data), LANGUAGE_DATA);
		
		if ((new_id == NULL && lang == NULL) || 
		    (new_id != NULL && lang != NULL && strcmp (gtk_source_language_get_id (lang),
		    					       new_id) == 0))
		{
			g_signal_handlers_block_by_func (window->priv->language_combo, 
							 language_combo_changed, 
					 		 window);
			
			gedit_status_combo_box_set_item (GEDIT_STATUS_COMBO_BOX (window->priv->language_combo),
					 		 GTK_MENU_ITEM (item->data));

			g_signal_handlers_unblock_by_func (window->priv->language_combo, 
							   language_combo_changed, 
					 		   window);
		}
	}

	g_list_free (items);
}

static void
connect_per_container_properties (GeditWindow        *window,
				  GeditViewContainer *container)
{
	GeditView *view;
	
	view = gedit_view_container_get_view (container);
	
	if (GEDIT_IS_TEXT_VIEW (view))
	{
		GeditDocument *doc;
	
		/* update the syntax menu */
		update_languages_menu (window);

		doc = gedit_view_get_document (view);

		/* sync the statusbar */
		update_cursor_position_statusbar (doc,
						  window);
		gedit_statusbar_set_overwrite (GEDIT_STATUSBAR (window->priv->statusbar),
					       gedit_view_get_overwrite (view));

		gtk_widget_show (window->priv->tab_width_combo);
		gtk_widget_show (window->priv->language_combo);

		window->priv->tab_width_id =
			g_signal_connect (view, 
					  "notify::tab-width", 
					  G_CALLBACK (tab_width_changed),
					  window);
		window->priv->spaces_instead_of_tabs_id =
			g_signal_connect (view, 
					  "notify::insert-spaces-instead-of-tabs", 
					  G_CALLBACK (spaces_instead_of_tabs_changed),
					  window);

		window->priv->language_changed_id =
			g_signal_connect (doc,
					  "notify::language",
					  G_CALLBACK (language_changed),
					  window);

		/* call it for the first time */
		tab_width_changed (G_OBJECT (view), NULL, window);
		spaces_instead_of_tabs_changed (G_OBJECT (view), NULL, window);
		language_changed (G_OBJECT (doc),
				  NULL, window);
	}
	else
	{
		/* Remove line and col info */
		gedit_statusbar_set_cursor_position (
				GEDIT_STATUSBAR (window->priv->statusbar),
				-1,
				-1);
				
		gedit_statusbar_clear_overwrite (
				GEDIT_STATUSBAR (window->priv->statusbar));
		
		/* We don't need to show the combo boxes */
		gtk_widget_hide (window->priv->tab_width_combo);
		gtk_widget_hide (window->priv->language_combo);
	}
}

static void
disconnect_per_container_properties (GeditWindow        *window,
				     GeditViewContainer *container)
{
	if (container)
	{
		GeditView *view;
		
		view = gedit_view_container_get_view (container);
		
		if (GEDIT_IS_TEXT_VIEW (view))
		{
			if (window->priv->tab_width_id)
			{
				g_signal_handler_disconnect (view,
							     window->priv->tab_width_id);
		
				window->priv->tab_width_id = 0;
			}
		
			if (window->priv->spaces_instead_of_tabs_id)
			{
				g_signal_handler_disconnect (view,
							     window->priv->spaces_instead_of_tabs_id);
		
				window->priv->spaces_instead_of_tabs_id = 0;
			}
		}
	}
}

static void 
notebook_switch_page (GtkNotebook     *book, 
		      GtkNotebookPage *pg,
		      gint             page_num, 
		      GeditWindow     *window)
{
	GeditPage *page;
	GtkAction *action;
	gchar *action_name;
	GeditViewContainer *container;
	
	/* CHECK: I don't know why but it seems notebook_switch_page is called
	two times every time the user change the active tab */
	
	page = GEDIT_PAGE (gtk_notebook_get_nth_page (book, page_num));
	if (page == window->priv->active_page)
		return;
	
	if (window->priv->active_page)
	{
		disconnect_per_container_properties (window,
						     window->priv->active_container);
	}
	
	/* set the active tab and container */
	window->priv->active_page = page;
	container = gedit_page_get_active_view_container (page);
	window->priv->active_container = container;

	set_title (window);
	set_sensitivity_according_to_page (window, page);

	/* activate the right item in the documents menu */
	action_name = g_strdup_printf ("Tab_%d", page_num);
	action = gtk_action_group_get_action (window->priv->documents_list_action_group,
					      action_name);

	/* sometimes the action doesn't exist yet, and the proper action
	 * is set active during the documents list menu creation
	 * CHECK: would it be nicer if active_tab was a property and we monitored the notify signal?
	 */
	if (action != NULL)
		gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), TRUE);

	g_free (action_name);
	
	connect_per_container_properties (window, container);
	
	g_signal_emit (G_OBJECT (window),
		       signals[ACTIVE_PAGE_CHANGED],
		       0,
		       window->priv->active_page);
}

static void
set_sensitivity_according_to_window_state (GeditWindow *window)
{
	GtkAction *action;
	GeditLockdownMask lockdown;

	lockdown = gedit_app_get_lockdown (gedit_app_get_default ());

	/* We disable File->Quit/SaveAll/CloseAll while printing to avoid to have two
	   operations (save and print/print preview) that uses the message area at
	   the same time (may be we can remove this limitation in the future) */
	/* We disable File->Quit/CloseAll if state is saving since saving cannot be
	   cancelled (may be we can remove this limitation in the future) */
	gtk_action_group_set_sensitive (window->priv->quit_action_group,
				  !(window->priv->state & GEDIT_WINDOW_STATE_SAVING) &&
				  !(window->priv->state & GEDIT_WINDOW_STATE_PRINTING));

	action = gtk_action_group_get_action (window->priv->action_group,
				              "FileCloseAll");
	gtk_action_set_sensitive (action, 
				  !(window->priv->state & GEDIT_WINDOW_STATE_SAVING) &&
				  !(window->priv->state & GEDIT_WINDOW_STATE_PRINTING));

	action = gtk_action_group_get_action (window->priv->action_group,
				              "FileSaveAll");
	gtk_action_set_sensitive (action, 
				  !(window->priv->state & GEDIT_WINDOW_STATE_PRINTING) &&
				  !(lockdown & GEDIT_LOCKDOWN_SAVE_TO_DISK));
			
	action = gtk_action_group_get_action (window->priv->always_sensitive_action_group,
					      "FileNew");
	gtk_action_set_sensitive (action, 
				  !(window->priv->state & GEDIT_WINDOW_STATE_SAVING_SESSION));
				  
	action = gtk_action_group_get_action (window->priv->always_sensitive_action_group,
					      "FileOpen");
	gtk_action_set_sensitive (action, 
				  !(window->priv->state & GEDIT_WINDOW_STATE_SAVING_SESSION));

	gtk_action_group_set_sensitive (window->priv->recents_action_group,
					!(window->priv->state & GEDIT_WINDOW_STATE_SAVING_SESSION));

	gedit_notebook_set_close_buttons_sensitive (GEDIT_NOTEBOOK (window->priv->notebook),
						    !(window->priv->state & GEDIT_WINDOW_STATE_SAVING_SESSION));
						    
	gedit_notebook_set_page_drag_and_drop_enabled (GEDIT_NOTEBOOK (window->priv->notebook),
						       !(window->priv->state & GEDIT_WINDOW_STATE_SAVING_SESSION));

	if ((window->priv->state & GEDIT_WINDOW_STATE_SAVING_SESSION) != 0)
	{
		/* TODO: If we really care, Find could be active
		 * when in SAVING_SESSION state */

		if (gtk_action_group_get_sensitive (window->priv->action_group))
			gtk_action_group_set_sensitive (window->priv->action_group,
							FALSE);
		if (gtk_action_group_get_sensitive (window->priv->quit_action_group))
			gtk_action_group_set_sensitive (window->priv->quit_action_group,
							FALSE);
	}
	else
	{
		if (!gtk_action_group_get_sensitive (window->priv->action_group))
			gtk_action_group_set_sensitive (window->priv->action_group,
							window->priv->num_pages > 0);
		if (!gtk_action_group_get_sensitive (window->priv->quit_action_group))
			gtk_action_group_set_sensitive (window->priv->quit_action_group,
							window->priv->num_pages > 0);
	}
}

static void
update_tab_autosave (GtkWidget *widget,
		     gpointer   data)
{
	GeditViewContainer *tab = GEDIT_VIEW_CONTAINER (widget);
	gboolean *enabled = (gboolean *) data;

	gedit_view_container_set_auto_save_enabled (tab, *enabled);
}

void
_gedit_window_set_lockdown (GeditWindow       *window,
			    GeditLockdownMask  lockdown)
{
	GeditPage *page;
	GtkAction *action;
	gboolean autosave;

	/* start/stop autosave in each existing tab */
	autosave = gedit_prefs_manager_get_auto_save ();
	gtk_container_foreach (GTK_CONTAINER (window->priv->notebook),
			       update_tab_autosave,
			       &autosave);

	/* update menues wrt the current active tab */
	page = gedit_window_get_active_page (window);

	set_sensitivity_according_to_page (window, page);

	action = gtk_action_group_get_action (window->priv->action_group,
				              "FileSaveAll");
	gtk_action_set_sensitive (action, 
				  !(window->priv->state & GEDIT_WINDOW_STATE_PRINTING) &&
				  !(lockdown & GEDIT_LOCKDOWN_SAVE_TO_DISK));

#if !GTK_CHECK_VERSION (2, 17, 4)
	action = gtk_action_group_get_action (window->priv->action_group,
				              "FilePageSetup");
	gtk_action_set_sensitive (action, 
				  !(lockdown & GEDIT_LOCKDOWN_PRINT_SETUP));
#endif
}

static void
analyze_page_state (GeditViewContainer    *tab, 
		   GeditWindow *window)
{
	GeditViewContainerState ts;
#if 0
	ts = gedit_view_container_get_state (tab);
	
	switch (ts)
	{
		case GEDIT_VIEW_CONTAINER_STATE_LOADING:
		case GEDIT_VIEW_CONTAINER_STATE_REVERTING:
			window->priv->state |= GEDIT_WINDOW_STATE_LOADING;
			break;
		
		case GEDIT_VIEW_CONTAINER_STATE_SAVING:
			window->priv->state |= GEDIT_WINDOW_STATE_SAVING;
			break;
			
		case GEDIT_VIEW_CONTAINER_STATE_PRINTING:
		case GEDIT_VIEW_CONTAINER_STATE_PRINT_PREVIEWING:
			window->priv->state |= GEDIT_WINDOW_STATE_PRINTING;
			break;
	
		case GEDIT_VIEW_CONTAINER_STATE_LOADING_ERROR:
		case GEDIT_VIEW_CONTAINER_STATE_REVERTING_ERROR:
		case GEDIT_VIEW_CONTAINER_STATE_SAVING_ERROR:
		case GEDIT_VIEW_CONTAINER_STATE_GENERIC_ERROR:
			window->priv->state |= GEDIT_WINDOW_STATE_ERROR;
			++window->priv->num_pages_with_error;
		default:
			/* NOP */
			break;
	}
#endif
}

static void
update_window_state (GeditWindow *window)
{
	GeditWindowState old_ws;
	gint old_num_of_errors;
	
	gedit_debug_message (DEBUG_WINDOW, "Old state: %x", window->priv->state);
	
	old_ws = window->priv->state;
	old_num_of_errors = window->priv->num_pages_with_error;
	
	window->priv->state = old_ws & GEDIT_WINDOW_STATE_SAVING_SESSION;
	
	window->priv->num_pages_with_error = 0;

	gtk_container_foreach (GTK_CONTAINER (window->priv->notebook),
	       		       (GtkCallback)analyze_page_state,
	       		       window);
		
	gedit_debug_message (DEBUG_WINDOW, "New state: %x", window->priv->state);
		
	if (old_ws != window->priv->state)
	{
		set_sensitivity_according_to_window_state (window);

		gedit_statusbar_set_window_state (GEDIT_STATUSBAR (window->priv->statusbar),
						  window->priv->state,
						  window->priv->num_pages_with_error);
						  
		g_object_notify (G_OBJECT (window), "state");
	}
	else if (old_num_of_errors != window->priv->num_pages_with_error)
	{
		gedit_statusbar_set_window_state (GEDIT_STATUSBAR (window->priv->statusbar),
						  window->priv->state,
						  window->priv->num_pages_with_error);
	}
}

static void
sync_state (GeditViewContainer *container,
	    GParamSpec         *pspec,
	    GeditWindow        *window)
{
	GeditPage *page;

	gedit_debug (DEBUG_WINDOW);
	
	update_window_state (window);
	page = gedit_page_get_from_container (container);
	
	if (page != window->priv->active_page)
		return;

	set_sensitivity_according_to_page (window, page);

	g_signal_emit (G_OBJECT (window), signals[ACTIVE_PAGE_STATE_CHANGED], 0);
}

static void
sync_name (GeditViewContainer *container,
	   GParamSpec         *pspec,
	   GeditWindow        *window)
{
	GtkAction *action;
	gchar *action_name;
	gchar *page_name;
	gchar *escaped_name;
	gchar *tip;
	gint n;
	GeditDocument *doc;
	GeditPage *page;

	page = gedit_page_get_from_container (container);

	if (page == window->priv->active_page)
	{
		set_title (window);

		doc = gedit_view_container_get_document (container);
		action = gtk_action_group_get_action (window->priv->action_group,
						      "FileRevert");
		gtk_action_set_sensitive (action,
					  !gedit_document_is_untitled (doc));
	}

	/* sync the item in the documents list menu */
	n = gtk_notebook_page_num (GTK_NOTEBOOK (window->priv->notebook),
				   GTK_WIDGET (page));
	action_name = g_strdup_printf ("Tab_%d", n);
	action = gtk_action_group_get_action (window->priv->documents_list_action_group,
					      action_name);
	g_return_if_fail (action != NULL);

	page_name = _gedit_view_container_get_name (container);
	escaped_name = gedit_utils_escape_underscores (page_name, -1);
	tip =  get_menu_tip_for_page (page);

	g_object_set (action, "label", escaped_name, NULL);
	g_object_set (action, "tooltip", tip, NULL);

	g_free (action_name);
	g_free (page_name);
	g_free (escaped_name);
	g_free (tip);

	gedit_plugins_engine_update_plugins_ui (gedit_plugins_engine_get_default (),
						window);
}

static GeditWindow *
get_drop_window (GtkWidget *widget)
{
	GtkWidget *target_window;

	target_window = gtk_widget_get_toplevel (widget);
	g_return_val_if_fail (GEDIT_IS_WINDOW (target_window), NULL);

	if ((GEDIT_WINDOW(target_window)->priv->state & GEDIT_WINDOW_STATE_SAVING_SESSION) != 0)
		return NULL;
	
	return GEDIT_WINDOW (target_window);
}

static void
load_uris_from_drop (GeditWindow  *window,
		     gchar       **uri_list)
{
	GSList *uris = NULL;
	gint i;
	
	if (uri_list == NULL)
		return;
	
	for (i = 0; uri_list[i] != NULL; ++i)
	{
		uris = g_slist_prepend (uris, uri_list[i]);
	}

	uris = g_slist_reverse (uris);
	gedit_commands_load_uris (window,
				  uris,
				  NULL,
				  0);

	g_slist_free (uris);
}

/* Handle drops on the GeditWindow */
static void
drag_data_received_cb (GtkWidget        *widget,
		       GdkDragContext   *context,
		       gint              x,
		       gint              y,
		       GtkSelectionData *selection_data,
		       guint             info,
		       guint             timestamp,
		       gpointer          data)
{
	GeditWindow *window;
	gchar **uri_list;

	window = get_drop_window (widget);
	
	if (window == NULL)
		return;

	if (info == TARGET_URI_LIST)
	{
		uri_list = gedit_utils_drop_get_uris(selection_data);
		load_uris_from_drop (window, uri_list);
		g_strfreev (uri_list);
	}
}

/* Handle drops on the GeditView */
static void
drop_uris_cb (GtkWidget    *widget,
	      gchar       **uri_list)
{
	GeditWindow *window;

	window = get_drop_window (widget);
	
	if (window == NULL)
		return;

	load_uris_from_drop (window, uri_list);
}

static void
fullscreen_controls_show (GeditWindow *window)
{
	GdkScreen *screen;
	GdkRectangle fs_rect;
	gint w, h;

	screen = gtk_window_get_screen (GTK_WINDOW (window));
	gdk_screen_get_monitor_geometry (screen,
					 gdk_screen_get_monitor_at_window (screen,
									   gtk_widget_get_window (GTK_WIDGET (window))),
					 &fs_rect);

	gtk_window_get_size (GTK_WINDOW (window->priv->fullscreen_controls), &w, &h);

	gtk_window_resize (GTK_WINDOW (window->priv->fullscreen_controls),
			   fs_rect.width, h);

	gtk_window_move (GTK_WINDOW (window->priv->fullscreen_controls),
			 fs_rect.x, fs_rect.y - h + 1);

	gtk_widget_show_all (window->priv->fullscreen_controls);
}

static gboolean
run_fullscreen_animation (gpointer data)
{
	GeditWindow *window = GEDIT_WINDOW (data);
	GdkScreen *screen;
	GdkRectangle fs_rect;
	gint x, y;
	
	screen = gtk_window_get_screen (GTK_WINDOW (window));
	gdk_screen_get_monitor_geometry (screen,
					 gdk_screen_get_monitor_at_window (screen,
									   gtk_widget_get_window (GTK_WIDGET (window))),
					 &fs_rect);
					 
	gtk_window_get_position (GTK_WINDOW (window->priv->fullscreen_controls),
				 &x, &y);
	
	if (window->priv->fullscreen_animation_enter)
	{
		if (y == fs_rect.y)
		{
			window->priv->fullscreen_animation_timeout_id = 0;
			return FALSE;
		}
		else
		{
			gtk_window_move (GTK_WINDOW (window->priv->fullscreen_controls),
					 x, y + 1);
			return TRUE;
		}
	}
	else
	{
		gint w, h;
	
		gtk_window_get_size (GTK_WINDOW (window->priv->fullscreen_controls),
				     &w, &h);
	
		if (y == fs_rect.y - h + 1)
		{
			window->priv->fullscreen_animation_timeout_id = 0;
			return FALSE;
		}
		else
		{
			gtk_window_move (GTK_WINDOW (window->priv->fullscreen_controls),
					 x, y - 1);
			return TRUE;
		}
	}
}

static void
show_hide_fullscreen_toolbar (GeditWindow *window,
			      gboolean     show,
			      gint         height)
{
	GtkSettings *settings;
	gboolean enable_animations;

	settings = gtk_widget_get_settings (GTK_WIDGET (window));
	g_object_get (G_OBJECT (settings),
		      "gtk-enable-animations",
		      &enable_animations,
		      NULL);

	if (enable_animations)
	{
		window->priv->fullscreen_animation_enter = show;

		if (window->priv->fullscreen_animation_timeout_id == 0)
		{
			window->priv->fullscreen_animation_timeout_id =
				g_timeout_add (FULLSCREEN_ANIMATION_SPEED,
					       (GSourceFunc) run_fullscreen_animation,
					       window);
		}
	}
	else
	{
		GdkRectangle fs_rect;
		GdkScreen *screen;

		screen = gtk_window_get_screen (GTK_WINDOW (window));
		gdk_screen_get_monitor_geometry (screen,
						 gdk_screen_get_monitor_at_window (screen,
										   gtk_widget_get_window (GTK_WIDGET (window))),
						 &fs_rect);

		if (show)
			gtk_window_move (GTK_WINDOW (window->priv->fullscreen_controls),
				 fs_rect.x, fs_rect.y);
		else
			gtk_window_move (GTK_WINDOW (window->priv->fullscreen_controls),
					 fs_rect.x, fs_rect.y - height + 1);
	}

}

static gboolean
on_fullscreen_controls_enter_notify_event (GtkWidget        *widget,
					   GdkEventCrossing *event,
					   GeditWindow      *window)
{
	show_hide_fullscreen_toolbar (window, TRUE, 0);

	return FALSE;
}

static gboolean
on_fullscreen_controls_leave_notify_event (GtkWidget        *widget,
					   GdkEventCrossing *event,
					   GeditWindow      *window)
{
	GdkDisplay *display;
	GdkScreen *screen;
	gint w, h;
	gint x, y;

	display = gdk_display_get_default ();
	screen = gtk_window_get_screen (GTK_WINDOW (window));

	gtk_window_get_size (GTK_WINDOW (window->priv->fullscreen_controls), &w, &h);
	gdk_display_get_pointer (display, &screen, &x, &y, NULL);
	
	/* gtk seems to emit leave notify when clicking on tool items,
	 * work around it by checking the coordinates
	 */
	if (y >= h)
	{
		show_hide_fullscreen_toolbar (window, FALSE, h);
	}

	return FALSE;
}

static void
fullscreen_controls_build (GeditWindow *window)
{
	GeditWindowPrivate *priv = window->priv;
	GtkWidget *toolbar;
	GtkWidget *toolbar_recent_menu;
	GtkAction *action;

	if (priv->fullscreen_controls != NULL)
		return;
	
	priv->fullscreen_controls = gtk_window_new (GTK_WINDOW_POPUP);

	gtk_window_set_transient_for (GTK_WINDOW (priv->fullscreen_controls),
				      &window->window);
	
	/* popup toolbar */
	toolbar = gtk_ui_manager_get_widget (priv->manager, "/FullscreenToolBar");
	gtk_container_add (GTK_CONTAINER (priv->fullscreen_controls),
			   toolbar);
	
	action = gtk_action_group_get_action (priv->always_sensitive_action_group,
					      "LeaveFullscreen");
	g_object_set (action, "is-important", TRUE, NULL);
	
	toolbar_recent_menu = setup_toolbar_open_button (window, toolbar);

	gtk_container_foreach (GTK_CONTAINER (toolbar),
			       (GtkCallback)set_non_homogeneus,
			       NULL);

	/* Set the toolbar style */
	gtk_toolbar_set_style (GTK_TOOLBAR (toolbar),
			       GTK_TOOLBAR_BOTH_HORIZ);

	g_signal_connect (priv->fullscreen_controls, "enter-notify-event",
			  G_CALLBACK (on_fullscreen_controls_enter_notify_event),
			  window);
	g_signal_connect (priv->fullscreen_controls, "leave-notify-event",
			  G_CALLBACK (on_fullscreen_controls_leave_notify_event),
			  window);
}

static void
can_search_again (GeditDocument *doc,
		  GParamSpec    *pspec,
		  GeditWindow   *window)
{
	gboolean sensitive;
	GtkAction *action;

	if (doc != gedit_window_get_active_document (window) ||
	    !GEDIT_TEXT_BUFFER (doc))
		return;

	sensitive = gedit_text_buffer_get_can_search_again (GEDIT_TEXT_BUFFER (doc));

	action = gtk_action_group_get_action (window->priv->action_group,
					      "SearchFindNext");
	gtk_action_set_sensitive (action, sensitive);

	action = gtk_action_group_get_action (window->priv->action_group,
					      "SearchFindPrevious");
	gtk_action_set_sensitive (action, sensitive);

	action = gtk_action_group_get_action (window->priv->action_group,
					      "SearchClearHighlight");
	gtk_action_set_sensitive (action, sensitive);
}

static void
can_undo (GeditDocument *doc,
	  GParamSpec    *pspec,
	  GeditWindow   *window)
{
	GtkAction *action;
	gboolean sensitive;

	sensitive = gedit_document_can_undo (doc);

	if (doc != gedit_window_get_active_document (window))
		return;

	action = gtk_action_group_get_action (window->priv->action_group,
					     "EditUndo");
	gtk_action_set_sensitive (action, sensitive);
}

static void
can_redo (GeditDocument *doc,
	  GParamSpec    *pspec,
	  GeditWindow   *window)
{
	GtkAction *action;
	gboolean sensitive;

	sensitive = gedit_document_can_redo (doc);

	if (doc != gedit_window_get_active_document (window))
		return;

	action = gtk_action_group_get_action (window->priv->action_group,
					     "EditRedo");
	gtk_action_set_sensitive (action, sensitive);
}

static void
selection_changed (GeditDocument *doc,
		   GParamSpec    *pspec,
		   GeditWindow   *window)
{
	GeditViewContainer *container;
	GeditView *view;
	GtkAction *action;
	GeditViewContainerState state;
	gboolean state_normal;
	gboolean editable;

	gedit_debug (DEBUG_WINDOW);

	if (doc != gedit_window_get_active_document (window))
		return;

	container = gedit_view_container_get_from_document (doc);
	state = gedit_view_container_get_state (container);
	state_normal = (state == GEDIT_VIEW_CONTAINER_STATE_NORMAL);

	view = gedit_view_container_get_view (container);
	editable = gedit_view_get_editable (view);

	action = gtk_action_group_get_action (window->priv->action_group,
					      "EditCut");
	gtk_action_set_sensitive (action,
				  state_normal &&
				  editable &&
				  gedit_document_get_has_selection (doc));

	action = gtk_action_group_get_action (window->priv->action_group,
					      "EditCopy");
	gtk_action_set_sensitive (action,
				  (state_normal ||
				   state == GEDIT_VIEW_CONTAINER_STATE_EXTERNALLY_MODIFIED_NOTIFICATION) &&
				  gedit_document_get_has_selection (doc));

	action = gtk_action_group_get_action (window->priv->action_group,
					      "EditDelete");
	gtk_action_set_sensitive (action,
				  state_normal &&
				  editable &&
				  gedit_document_get_has_selection (doc));

	gedit_plugins_engine_update_plugins_ui (gedit_plugins_engine_get_default (),
						window);
}

static void
sync_languages_menu (GeditDocument *doc,
		     GParamSpec    *pspec,
		     GeditWindow   *window)
{
	update_languages_menu (window);
	gedit_plugins_engine_update_plugins_ui (gedit_plugins_engine_get_default (),
						 window);
}

static void
editable_changed (GeditView  *view,
                  GParamSpec  *arg1,
                  GeditWindow *window)
{
	gedit_plugins_engine_update_plugins_ui (gedit_plugins_engine_get_default (),
						window);
}

static void
connect_per_container_signals (GeditWindow        *window,
			       GeditViewContainer *container)
{
	GList *views, *l;
	GeditDocument *doc;

	views = gedit_view_container_get_views (container);
	doc = gedit_view_container_get_document (container);

	g_signal_connect (container,
			 "notify::name",
			  G_CALLBACK (sync_name),
			  window);
	g_signal_connect (container, 
			 "notify::state",
			  G_CALLBACK (sync_state),
			  window);

	if (GEDIT_IS_TEXT_BUFFER (doc))
	{
		g_signal_connect (doc,
				  "cursor-moved",
				  G_CALLBACK (update_cursor_position_statusbar),
				  window);
		g_signal_connect (doc,
				  "notify::language",
				  G_CALLBACK (sync_languages_menu),
				  window);
		g_signal_connect (doc,
				  "notify::can-search-again",
				  G_CALLBACK (can_search_again),
				  window);
	}
	
	g_signal_connect (doc,
			  "notify::can-undo",
			  G_CALLBACK (can_undo),
			  window);
	g_signal_connect (doc,
			  "notify::can-redo",
			  G_CALLBACK (can_redo),
			  window);
	g_signal_connect (doc,
			  "notify::has-selection",
			  G_CALLBACK (selection_changed),
			  window);

	for (l = views; l != NULL; l = g_list_next (l))
	{
		GeditView *view = GEDIT_VIEW (l->data);
		
		if (GEDIT_IS_TEXT_VIEW (view))
		{
			g_signal_connect (view,
					  "toggle_overwrite",
					  G_CALLBACK (update_overwrite_mode_statusbar),
					  window);
		}
		g_signal_connect (view,
				  "notify::editable",
				  G_CALLBACK (editable_changed),
				  window);

		g_signal_connect (view,
				  "drop_uris",
				  G_CALLBACK (drop_uris_cb),
				  NULL);
	}
}

static void
disconnect_per_container_signals (GeditWindow        *window,
				  GeditViewContainer *container)
{
	GList *views, *l;
	GeditDocument *doc;
	
	views = gedit_view_container_get_views (container);
	doc = gedit_view_container_get_document (container);

	g_signal_handlers_disconnect_by_func (container,
					      G_CALLBACK (sync_name),
					      window);
	g_signal_handlers_disconnect_by_func (container,
					      G_CALLBACK (sync_state),
					      window);

	if (GEDIT_IS_TEXT_BUFFER (doc))
	{
		g_signal_handlers_disconnect_by_func (doc,
						      G_CALLBACK (update_cursor_position_statusbar), 
						      window);
		g_signal_handlers_disconnect_by_func (doc, 
						      G_CALLBACK (can_search_again),
						      window);
		g_signal_handlers_disconnect_by_func (doc,
						      G_CALLBACK (sync_languages_menu),
						      window);
	}
	g_signal_handlers_disconnect_by_func (doc, 
					      G_CALLBACK (can_undo),
					      window);
	g_signal_handlers_disconnect_by_func (doc, 
					      G_CALLBACK (can_redo),
					      window);
	g_signal_handlers_disconnect_by_func (doc,
					      G_CALLBACK (selection_changed),
					      window);
	
	for (l = views; l != NULL; l = g_list_next (l))
	{
		GeditView *view = GEDIT_VIEW (l->data);
		
		if (GEDIT_IS_TEXT_VIEW (view))
		{
			g_signal_handlers_disconnect_by_func (view,
							      G_CALLBACK (update_overwrite_mode_statusbar),
							      window);
		}
		g_signal_handlers_disconnect_by_func (view,
						      G_CALLBACK (editable_changed),
						      window);
		g_signal_handlers_disconnect_by_func (view,
						      G_CALLBACK (drop_uris_cb),
						      NULL);
	}
}

static void
on_container_added (GeditPage          *page,
		    GeditViewContainer *container,
		    GeditWindow        *window)
{
	connect_per_container_signals (window, container);
}

static void
on_container_removed (GeditPage          *page,
		      GeditViewContainer *container,
		      GeditWindow        *window)
{
	if (window->priv->active_container == container)
	{
		window->priv->active_container = NULL;
	}
}

static void
on_active_container_changed (GeditPage          *page,
			     GeditViewContainer *container,
			     GeditWindow        *window)
{
	disconnect_per_container_properties (window, window->priv->active_container);
	
	window->priv->active_container = container;
	
	connect_per_container_properties (window, container);
}

static void
notebook_page_added (GeditNotebook *notebook,
		     GeditPage     *page,
		     GeditWindow   *window)
{
	GList *containers, *l;
	GtkAction *action;

	gedit_debug (DEBUG_WINDOW);

	g_return_if_fail ((window->priv->state & GEDIT_WINDOW_STATE_SAVING_SESSION) == 0);
	
	++window->priv->num_pages;

	/* Set sensitivity */
	if (!gtk_action_group_get_sensitive (window->priv->action_group))
		gtk_action_group_set_sensitive (window->priv->action_group,
						TRUE);

	action = gtk_action_group_get_action (window->priv->action_group,
					      "DocumentsMoveToNewWindow");
	gtk_action_set_sensitive (action,
				  window->priv->num_pages > 1);

	containers = gedit_page_get_view_containers (page);

	/* IMPORTANT: remember to disconnect the signal in notebook_page_removed
	 * if a new signal is connected here */

	/* By default we have at least one container then we have to listen
	 * container-added to catch the new ones */
	for (l = containers; l != NULL; l = g_list_next (l))
	{
		connect_per_container_signals (window, l->data);
	}
	
	g_signal_connect (page, "container-added",
			  G_CALLBACK (on_container_added),
			  window);
	g_signal_connect (page, "container-removed",
			  G_CALLBACK (on_container_removed),
			  window);
	g_signal_connect (page, "active-container-changed",
			  G_CALLBACK (on_active_container_changed),
			  window);

	update_documents_list_menu (window);

	update_window_state (window);

	g_signal_emit (G_OBJECT (window), signals[PAGE_ADDED], 0, page);
}

static void
notebook_page_removed (GeditNotebook *notebook,
		       GeditPage     *page,
		       GeditWindow   *window)
{
	GtkAction     *action;
	GeditViewContainer *container;
	GList *containers, *l;
	GeditDocument *doc;
	GeditView *view;

	gedit_debug (DEBUG_WINDOW);

	g_return_if_fail ((window->priv->state & GEDIT_WINDOW_STATE_SAVING_SESSION) == 0);

	--window->priv->num_pages;

	/* FIXME: This sucks: One reason is that it produces warnings when disconnecting */
	containers = gedit_page_get_view_containers (page);
	container = gedit_page_get_active_view_container (page);
	doc = gedit_view_container_get_document (container);
	view = gedit_view_container_get_view (container);

	for (l = containers; l != NULL; l = g_list_next (l))
	{
		disconnect_per_container_signals (window, l->data);
	}

	if (window->priv->tab_width_id && page == gedit_window_get_active_page (window))
	{
		g_signal_handler_disconnect (view, window->priv->tab_width_id);
		window->priv->tab_width_id = 0;
	}
	
	if (window->priv->spaces_instead_of_tabs_id && page == gedit_window_get_active_page (window))
	{
		g_signal_handler_disconnect (view, window->priv->spaces_instead_of_tabs_id);
		window->priv->spaces_instead_of_tabs_id = 0;
	}
	
	if (window->priv->language_changed_id && page == gedit_window_get_active_page (window))
	{
		g_signal_handler_disconnect (doc, window->priv->language_changed_id);
		window->priv->language_changed_id = 0;
	}

	g_return_if_fail (window->priv->num_pages >= 0);
	if (window->priv->num_pages == 0)
	{
		window->priv->active_page = NULL;
		window->priv->active_container = NULL;
		
		set_title (window);
		
		/* Remove line and col info */
		gedit_statusbar_set_cursor_position (
				GEDIT_STATUSBAR (window->priv->statusbar),
				-1,
				-1);
				
		gedit_statusbar_clear_overwrite (
				GEDIT_STATUSBAR (window->priv->statusbar));

		/* hide the combos */
		gtk_widget_hide (window->priv->tab_width_combo);
		gtk_widget_hide (window->priv->language_combo);
	}

	if (!window->priv->removing_pages)
	{
		update_documents_list_menu (window);
		update_next_prev_doc_sensitivity_per_window (window);
	}
	else
	{
		if (window->priv->num_pages == 0)
		{
			update_documents_list_menu (window);
			update_next_prev_doc_sensitivity_per_window (window);
		}
	}

	/* Set sensitivity */
	if (window->priv->num_pages == 0)
	{
		if (gtk_action_group_get_sensitive (window->priv->action_group))
			gtk_action_group_set_sensitive (window->priv->action_group,
							FALSE);

		action = gtk_action_group_get_action (window->priv->action_group,
						      "ViewHighlightMode");
		gtk_action_set_sensitive (action, FALSE);

		gedit_plugins_engine_update_plugins_ui (gedit_plugins_engine_get_default (),
							window);
	}

	if (window->priv->num_pages <= 1)
	{
		action = gtk_action_group_get_action (window->priv->action_group,
						     "DocumentsMoveToNewWindow");
		gtk_action_set_sensitive (action,
					  FALSE);
	}

	update_window_state (window);

	g_signal_emit (G_OBJECT (window), signals[PAGE_REMOVED], 0, page);
}

static void
notebook_pages_reordered (GeditNotebook *notebook,
			  GeditWindow   *window)
{
	update_documents_list_menu (window);
	update_next_prev_doc_sensitivity_per_window (window);
	
	g_signal_emit (G_OBJECT (window), signals[PAGES_REORDERED], 0);
}

static void
notebook_page_detached (GeditNotebook *notebook,
			GeditPage     *page,
			GeditWindow   *window)
{
	GeditWindow *new_window;
	
	new_window = clone_window (window);

	gedit_notebook_move_page (notebook,
				  GEDIT_NOTEBOOK (_gedit_window_get_notebook (new_window)),
				  page, 0);

	gtk_window_set_position (GTK_WINDOW (new_window), 
				 GTK_WIN_POS_MOUSE);

	gtk_widget_show (GTK_WIDGET (new_window));
}

static void 
notebook_page_close_request (GeditNotebook *notebook,
			     GeditPage     *page,
			     GtkWindow     *window)
{
	/* Note: we are destroying the tab before the default handler
	 * seems to be ok, but we need to keep an eye on this. */
	_gedit_cmd_file_close_page (page, GEDIT_WINDOW (window));
}

static gboolean
show_notebook_popup_menu (GtkNotebook    *notebook,
			  GeditWindow    *window,
			  GdkEventButton *event)
{
	GtkWidget *menu;
//	GtkAction *action;

	menu = gtk_ui_manager_get_widget (window->priv->manager, "/NotebookPopup");
	g_return_val_if_fail (menu != NULL, FALSE);

// CHECK do we need this?
#if 0
	/* allow extensions to sync when showing the popup */
	action = gtk_action_group_get_action (window->priv->action_group,
					      "NotebookPopupAction");
	g_return_val_if_fail (action != NULL, FALSE);
	gtk_action_activate (action);
#endif
	if (event != NULL)
	{
		gtk_menu_popup (GTK_MENU (menu), NULL, NULL,
				NULL, NULL,
				event->button, event->time);
	}
	else
	{
		GtkWidget *page;
		GtkWidget *tab_label;

		page = GTK_WIDGET (gedit_window_get_active_page (window));
		g_return_val_if_fail (page != NULL, FALSE);

		tab_label = gtk_notebook_get_tab_label (notebook, page);

		gtk_menu_popup (GTK_MENU (menu), NULL, NULL,
				gedit_utils_menu_position_under_widget, tab_label,
				0, gtk_get_current_event_time ());

		gtk_menu_shell_select_first (GTK_MENU_SHELL (menu), FALSE);
	}

	return TRUE;
}

static gboolean
notebook_button_press_event (GtkNotebook    *notebook,
			     GdkEventButton *event,
			     GeditWindow    *window)
{
	if (GDK_BUTTON_PRESS == event->type && 3 == event->button)
	{
		return show_notebook_popup_menu (notebook, window, event);
	}

	return FALSE;
}

static gboolean
notebook_popup_menu (GtkNotebook *notebook,
		     GeditWindow *window)
{
	/* Only respond if the notebook is the actual focus */
	if (GEDIT_IS_NOTEBOOK (gtk_window_get_focus (GTK_WINDOW (window))))
	{
		return show_notebook_popup_menu (notebook, window, NULL);
	}

	return FALSE;
}

static void
side_panel_size_allocate (GtkWidget     *widget,
			  GtkAllocation *allocation,
			  GeditWindow   *window)
{
	window->priv->side_panel_size = allocation->width;
}

static void
bottom_panel_size_allocate (GtkWidget     *widget,
			    GtkAllocation *allocation,
			    GeditWindow   *window)
{
	window->priv->bottom_panel_size = allocation->height;
}

static void
hpaned_restore_position (GtkWidget   *widget,
			 GeditWindow *window)
{
	gint pos;

	gedit_debug_message (DEBUG_WINDOW,
			     "Restoring hpaned position: side panel size %d",
			     window->priv->side_panel_size);

	pos = MAX (100, window->priv->side_panel_size);
	gtk_paned_set_position (GTK_PANED (window->priv->hpaned), pos);

	/* start monitoring the size */
	g_signal_connect (window->priv->side_panel,
			  "size-allocate",
			  G_CALLBACK (side_panel_size_allocate),
			  window);

	/* run this only once */
	g_signal_handlers_disconnect_by_func (widget, hpaned_restore_position, window);
}

static void
vpaned_restore_position (GtkWidget   *widget,
			 GeditWindow *window)
{
	gint pos;

	gedit_debug_message (DEBUG_WINDOW,
			     "Restoring vpaned position: bottom panel size %d",
			     window->priv->bottom_panel_size);

	pos = widget->allocation.height -
	      MAX (50, window->priv->bottom_panel_size);
	gtk_paned_set_position (GTK_PANED (window->priv->vpaned), pos);

	/* start monitoring the size */
	g_signal_connect (window->priv->bottom_panel,
			  "size-allocate",
			  G_CALLBACK (bottom_panel_size_allocate),
			  window);

	/* run this only once */
	g_signal_handlers_disconnect_by_func (widget, vpaned_restore_position, window);
}

static void
side_panel_visibility_changed (GtkWidget   *side_panel,
			       GeditWindow *window)
{
	gboolean visible;
	GtkAction *action;

	visible = GTK_WIDGET_VISIBLE (side_panel);

	if (gedit_prefs_manager_side_pane_visible_can_set ())
		gedit_prefs_manager_set_side_pane_visible (visible);

	action = gtk_action_group_get_action (window->priv->panes_action_group,
	                                      "ViewSidePane");

	if (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)) != visible)
		gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), visible);

	/* focus the document */
	if (!visible && window->priv->active_page != NULL)
	{
		GeditViewContainer *container;
		
		container = gedit_page_get_active_view_container (window->priv->active_page);
		gtk_widget_grab_focus (GTK_WIDGET (
				gedit_view_container_get_view (container)));
	}
}

static void
create_side_panel (GeditWindow *window)
{
	GtkWidget *documents_panel;

	gedit_debug (DEBUG_WINDOW);

	window->priv->side_panel = gedit_panel_new (GTK_ORIENTATION_VERTICAL);

	gtk_paned_pack1 (GTK_PANED (window->priv->hpaned), 
			 window->priv->side_panel, 
			 FALSE, 
			 FALSE);

	g_signal_connect_after (window->priv->side_panel,
				"show",
				G_CALLBACK (side_panel_visibility_changed),
				window);
	g_signal_connect_after (window->priv->side_panel,
				"hide",
				G_CALLBACK (side_panel_visibility_changed),
				window);

	documents_panel = gedit_documents_panel_new (window);
	gedit_panel_add_item_with_stock_icon (GEDIT_PANEL (window->priv->side_panel),
					      documents_panel,
					      _("Documents"),
					      GTK_STOCK_FILE);
}

static void
bottom_panel_visibility_changed (GeditPanel  *bottom_panel,
				 GeditWindow *window)
{
	gboolean visible;
	GtkAction *action;

	visible = GTK_WIDGET_VISIBLE (bottom_panel);

	if (gedit_prefs_manager_bottom_panel_visible_can_set ())
		gedit_prefs_manager_set_bottom_panel_visible (visible);

	action = gtk_action_group_get_action (window->priv->panes_action_group,
					      "ViewBottomPane");

	if (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)) != visible)
		gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), visible);

	/* focus the document */
	if (!visible && window->priv->active_page != NULL)
	{
		GeditViewContainer *container;
		
		container = gedit_page_get_active_view_container (window->priv->active_page);
		gtk_widget_grab_focus (GTK_WIDGET (
				gedit_view_container_get_view (container)));
	}
}

static void
bottom_panel_item_removed (GeditPanel  *panel,
			   GtkWidget   *item,
			   GeditWindow *window)
{
	if (gedit_panel_get_n_items (panel) == 0)
	{
		GtkAction *action;

		gtk_widget_hide (GTK_WIDGET (panel));

		action = gtk_action_group_get_action (window->priv->panes_action_group,
						      "ViewBottomPane");
		gtk_action_set_sensitive (action, FALSE);
	}
}

static void
bottom_panel_item_added (GeditPanel  *panel,
			 GtkWidget   *item,
			 GeditWindow *window)
{
	/* if it's the first item added, set the menu item
	 * sensitive and if needed show the panel */
	if (gedit_panel_get_n_items (panel) == 1)
	{
		GtkAction *action;
		gboolean show;

		action = gtk_action_group_get_action (window->priv->panes_action_group,
						      "ViewBottomPane");
		gtk_action_set_sensitive (action, TRUE);

		show = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
		if (show)
			gtk_widget_show (GTK_WIDGET (panel));
	}
}

static void
create_bottom_panel (GeditWindow *window) 
{
	gedit_debug (DEBUG_WINDOW);

	window->priv->bottom_panel = gedit_panel_new (GTK_ORIENTATION_HORIZONTAL);

	gtk_paned_pack2 (GTK_PANED (window->priv->vpaned), 
			 window->priv->bottom_panel, 
			 FALSE,
			 FALSE);

	g_signal_connect_after (window->priv->bottom_panel,
				"show",
				G_CALLBACK (bottom_panel_visibility_changed),
				window);
	g_signal_connect_after (window->priv->bottom_panel,
				"hide",
				G_CALLBACK (bottom_panel_visibility_changed),
				window);
}

static void
init_panels_visibility (GeditWindow *window)
{
	gint active_page;

	gedit_debug (DEBUG_WINDOW);

	/* side pane */
	active_page = gedit_prefs_manager_get_side_panel_active_page ();
	_gedit_panel_set_active_item_by_id (GEDIT_PANEL (window->priv->side_panel),
					    active_page);

	if (gedit_prefs_manager_get_side_pane_visible ())
	{
		gtk_widget_show (window->priv->side_panel);
	}

	/* bottom pane, it can be empty */
	if (gedit_panel_get_n_items (GEDIT_PANEL (window->priv->bottom_panel)) > 0)
	{
		active_page = gedit_prefs_manager_get_bottom_panel_active_page ();
		_gedit_panel_set_active_item_by_id (GEDIT_PANEL (window->priv->bottom_panel),
						    active_page);

		if (gedit_prefs_manager_get_bottom_panel_visible ())
		{
			gtk_widget_show (window->priv->bottom_panel);
		}
	}
	else
	{
		GtkAction *action;
		action = gtk_action_group_get_action (window->priv->panes_action_group,
						      "ViewBottomPane");
		gtk_action_set_sensitive (action, FALSE);
	}

	/* start track sensitivity after the initial state is set */
	window->priv->bottom_panel_item_removed_handler_id =
		g_signal_connect (window->priv->bottom_panel,
				  "item_removed",
				  G_CALLBACK (bottom_panel_item_removed),
				  window);

	g_signal_connect (window->priv->bottom_panel,
			  "item_added",
			  G_CALLBACK (bottom_panel_item_added),
			  window);
}

static void
clipboard_owner_change (GtkClipboard        *clipboard,
			GdkEventOwnerChange *event,
			GeditWindow         *window)
{
	set_paste_sensitivity_according_to_clipboard (window,
						      clipboard);
}

static void
window_realized (GtkWidget *window,
		 gpointer  *data)
{
	GtkClipboard *clipboard;

	clipboard = gtk_widget_get_clipboard (window,
					      GDK_SELECTION_CLIPBOARD);

	g_signal_connect (clipboard,
			  "owner_change",
			  G_CALLBACK (clipboard_owner_change),
			  window);
}

static void
window_unrealized (GtkWidget *window,
		   gpointer  *data)
{
	GtkClipboard *clipboard;

	clipboard = gtk_widget_get_clipboard (window,
					      GDK_SELECTION_CLIPBOARD);

	g_signal_handlers_disconnect_by_func (clipboard,
					      G_CALLBACK (clipboard_owner_change),
					      window);
}

static void
check_window_is_active (GeditWindow *window,
			GParamSpec *property,
			gpointer useless)
{
	if (window->priv->window_state & GDK_WINDOW_STATE_FULLSCREEN)
	{
		if (gtk_window_is_active (GTK_WINDOW (window)))
		{
			gtk_widget_show (window->priv->fullscreen_controls);
		}
		else
		{
			gtk_widget_hide (window->priv->fullscreen_controls);
		}
	}
}

static void
gedit_window_init (GeditWindow *window)
{
	GtkWidget *main_box;
	GtkTargetList *tl;

	gedit_debug (DEBUG_WINDOW);

	window->priv = GEDIT_WINDOW_GET_PRIVATE (window);
	window->priv->active_page = NULL;
	window->priv->active_container = NULL;
	window->priv->num_pages = 0;
	window->priv->removing_pages = FALSE;
	window->priv->state = GEDIT_WINDOW_STATE_NORMAL;
	window->priv->dispose_has_run = FALSE;
	window->priv->fullscreen_controls = NULL;
	window->priv->fullscreen_animation_timeout_id = 0;

	window->priv->message_bus = gedit_message_bus_new ();

	window->priv->window_group = gtk_window_group_new ();
	gtk_window_group_add_window (window->priv->window_group, GTK_WINDOW (window));

	main_box = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (window), main_box);
	gtk_widget_show (main_box);

	/* Add menu bar and toolbar bar */
	create_menu_bar_and_toolbar (window, main_box);

	/* Add status bar */
	create_statusbar (window, main_box);

	/* Add the main area */
	gedit_debug_message (DEBUG_WINDOW, "Add main area");
	window->priv->hpaned = gtk_hpaned_new ();
  	gtk_box_pack_start (GTK_BOX (main_box), 
  			    window->priv->hpaned, 
  			    TRUE, 
  			    TRUE, 
  			    0);

	window->priv->vpaned = gtk_vpaned_new ();
  	gtk_paned_pack2 (GTK_PANED (window->priv->hpaned), 
  			 window->priv->vpaned, 
  			 TRUE, 
  			 FALSE);
  	
	gedit_debug_message (DEBUG_WINDOW, "Create gedit notebook");
	window->priv->notebook = gedit_notebook_new ();
  	gtk_paned_pack1 (GTK_PANED (window->priv->vpaned),
  			 window->priv->notebook,
  			 TRUE, 
  			 TRUE);
  	gtk_widget_show (window->priv->notebook);

	/* side and bottom panels */
  	create_side_panel (window);
	create_bottom_panel (window);

	/* panes' state must be restored after panels have been mapped,
	 * since the bottom pane position depends on the size of the vpaned. */
	window->priv->side_panel_size = gedit_prefs_manager_get_side_panel_size ();
	window->priv->bottom_panel_size = gedit_prefs_manager_get_bottom_panel_size ();

	g_signal_connect_after (window->priv->hpaned,
				"map",
				G_CALLBACK (hpaned_restore_position),
				window);
	g_signal_connect_after (window->priv->vpaned,
				"map",
				G_CALLBACK (vpaned_restore_position),
				window);

	gtk_widget_show (window->priv->hpaned);
	gtk_widget_show (window->priv->vpaned);

	/* Drag and drop support, set targets to NULL because we add the
	   default uri_targets below */
	gtk_drag_dest_set (GTK_WIDGET (window),
			   GTK_DEST_DEFAULT_MOTION |
			   GTK_DEST_DEFAULT_HIGHLIGHT |
			   GTK_DEST_DEFAULT_DROP,
			   NULL,
			   0,
			   GDK_ACTION_COPY);

	/* Add uri targets */
	tl = gtk_drag_dest_get_target_list (GTK_WIDGET (window));
	
	if (tl == NULL)
	{
		tl = gtk_target_list_new (NULL, 0);
		gtk_drag_dest_set_target_list (GTK_WIDGET (window), tl);
		gtk_target_list_unref (tl);
	}
	
	gtk_target_list_add_uri_targets (tl, TARGET_URI_LIST);

	/* Connect signals */
	g_signal_connect (window->priv->notebook,
			  "switch_page",
			  G_CALLBACK (notebook_switch_page),
			  window);
	g_signal_connect (window->priv->notebook,
			  "gedit_page_added",
			  G_CALLBACK (notebook_page_added),
			  window);
	g_signal_connect (window->priv->notebook,
			  "gedit_page_removed",
			  G_CALLBACK (notebook_page_removed),
			  window);
	g_signal_connect (window->priv->notebook,
			  "pages_reordered",
			  G_CALLBACK (notebook_pages_reordered),
			  window);
	g_signal_connect (window->priv->notebook,
			  "page_detached",
			  G_CALLBACK (notebook_page_detached),
			  window);
	g_signal_connect (window->priv->notebook,
			  "page_close_request",
			  G_CALLBACK (notebook_page_close_request),
			  window);
	g_signal_connect (window->priv->notebook,
			  "button-press-event",
			  G_CALLBACK (notebook_button_press_event),
			  window);
	g_signal_connect (window->priv->notebook,
			  "popup-menu",
			  G_CALLBACK (notebook_popup_menu),
			  window);

	/* connect instead of override, so that we can
	 * share the cb code with the view */
	g_signal_connect (window,
			  "drag_data_received",
	                  G_CALLBACK (drag_data_received_cb), 
	                  NULL);

	/* we can get the clipboard only after the widget
	 * is realized */
	g_signal_connect (window,
			  "realize",
			  G_CALLBACK (window_realized),
			  NULL);
	g_signal_connect (window,
			  "unrealize",
			  G_CALLBACK (window_unrealized),
			  NULL);

	/* Check if the window is active for fullscreen */
	g_signal_connect (window,
			  "notify::is-active",
			  G_CALLBACK (check_window_is_active),
			  NULL);

	gedit_debug_message (DEBUG_WINDOW, "Update plugins ui");
	
	gedit_plugins_engine_activate_plugins (gedit_plugins_engine_get_default (),
					        window);

	/* set visibility of panes.
	 * This needs to be done after plugins activatation */
	init_panels_visibility (window);

	gedit_debug_message (DEBUG_WINDOW, "END");
}

GeditViewContainer *
gedit_window_get_active_view_container (GeditWindow *window)
{
	GeditViewContainer *container;

	g_return_val_if_fail (GEDIT_IS_WINDOW (window), NULL);
	
	if (window->priv->active_page == NULL)
		return NULL;

	container = gedit_page_get_active_view_container (window->priv->active_page);
	
	return container;
}

/**
 * gedit_window_get_active_view:
 * @window: a #GeditWindow
 *
 * Gets the active #GeditView.
 *
 * Returns: the active #GeditView
 */
GeditView *
gedit_window_get_active_view (GeditWindow *window)
{
	GeditView *view;
	GeditViewContainer *container;

	g_return_val_if_fail (GEDIT_IS_WINDOW (window), NULL);

	container = gedit_window_get_active_view_container (window);
	if (container == NULL)
		return NULL;
	
	view = gedit_view_container_get_view (container);

	return view;
}

/**
 * gedit_window_get_active_document:
 * @window: a #GeditWindow
 *
 * Gets the active #GeditDocument.
 * 
 * Returns: the active #GeditDocument
 */
GeditDocument *
gedit_window_get_active_document (GeditWindow *window)
{
	GeditViewContainer *container;

	g_return_val_if_fail (GEDIT_IS_WINDOW (window), NULL);

	container = gedit_window_get_active_view_container (window);
	if (container == NULL)
		return NULL;

	return gedit_view_container_get_document (container);
}

GtkWidget *
_gedit_window_get_notebook (GeditWindow *window)
{
	g_return_val_if_fail (GEDIT_IS_WINDOW (window), NULL);

	return window->priv->notebook;
}

/**
 * gedit_window_create_page:
 * @window: a #GeditWindow
 * @jump_to: %TRUE to set the new #GeditPage as active
 *
 * Creates a new #GeditPage and adds the new page to the #GeditNotebook.
 * In case @jump_to is %TRUE the #GeditNotebook switches to that new #GeditPage.
 *
 * Returns: a new #GeditPage
 */
GeditPage *
gedit_window_create_page (GeditWindow *window,
			  gboolean     jump_to)
{
	GeditPage *page;

	g_return_val_if_fail (GEDIT_IS_WINDOW (window), NULL);

	page = GEDIT_PAGE (_gedit_page_new (NULL));
	gtk_widget_show (GTK_WIDGET (page));

	gedit_notebook_add_page (GEDIT_NOTEBOOK (window->priv->notebook),
				 page,
				 -1,
				 jump_to);

	return page;
}

/**
 * gedit_window_create_page_from_uri:
 * @window: a #GeditWindow
 * @uri: the uri of the document
 * @encoding: a #GeditEncoding
 * @line_pos: the line position to visualize
 * @create: %TRUE to create a new document in case @uri does exist
 * @jump_to: %TRUE to set the new #GeditPage as active
 *
 * Creates a new #GeditPage loading the document specified by @uri.
 * In case @jump_to is %TRUE the #GeditNotebook swithes to that new #GeditPage.
 * Whether @create is %TRUE, creates a new empty document if location does 
 * not refer to an existing file
 *
 * Returns: a new #GeditPage
 */
GeditPage *
gedit_window_create_page_from_uri (GeditWindow         *window,
				   const gchar         *uri,
				   const GeditEncoding *encoding,
				   gint                 line_pos,
				   gboolean             create,
				   gboolean             jump_to)
{
	GtkWidget *container;
	GtkWidget *page;

	g_return_val_if_fail (GEDIT_IS_WINDOW (window), NULL);
	g_return_val_if_fail (uri != NULL, NULL);

	container = _gedit_view_container_new_from_uri (uri,
							encoding,
							line_pos,
							create);
	if (container == NULL)
		return NULL;

	page = _gedit_page_new (GEDIT_VIEW_CONTAINER (container));
	gtk_widget_show (page);

	gedit_notebook_add_page (GEDIT_NOTEBOOK (window->priv->notebook),
				 GEDIT_PAGE (page),
				 -1,
				 jump_to);

	return GEDIT_PAGE (page);
}

/**
 * gedit_window_get_active_page:
 * @window: a GeditWindow
 *
 * Gets the active #GeditPage in the @window.
 *
 * Returns: the active #GeditPage in the @window.
 */
GeditPage *
gedit_window_get_active_page (GeditWindow *window)
{
	g_return_val_if_fail (GEDIT_IS_WINDOW (window), NULL);
	
	return (window->priv->active_page == NULL) ? 
				NULL : GEDIT_PAGE (window->priv->active_page);
}

static void
add_document (GeditPage *page, GList **res)
{
	GeditDocument *doc;
	GeditViewContainer *container;
	
	container = gedit_page_get_active_view_container (page);
	doc = gedit_view_container_get_document (container);
	
	*res = g_list_prepend (*res, doc);
}

/**
 * gedit_window_get_documents:
 * @window: a #GeditWindow
 *
 * Gets a newly allocated list with all the documents in the window.
 * This list must be freed.
 *
 * Returns: a newly allocated list with all the documents in the window
 */
GList *
gedit_window_get_documents (GeditWindow *window)
{
	GList *res = NULL;

	g_return_val_if_fail (GEDIT_IS_WINDOW (window), NULL);
	
	gtk_container_foreach (GTK_CONTAINER (window->priv->notebook),
			       (GtkCallback)add_document,
			       &res);
			       
	res = g_list_reverse (res);
	
	return res;
}

static void
add_view (GeditPage *page, GList **res)
{
	GeditView *view;
	GeditViewContainer *container;
	
	container = gedit_page_get_active_view_container (page);
	view = gedit_view_container_get_view (container);
	
	*res = g_list_prepend (*res, view);
}

/**
 * gedit_window_get_views:
 * @window: a #GeditWindow
 *
 * Gets a list with all the views in the window. This list must be freed.
 *
 * Returns: a newly allocated list with all the views in the window
 */
GList *
gedit_window_get_views (GeditWindow *window)
{
	GList *res = NULL;

	g_return_val_if_fail (GEDIT_IS_WINDOW (window), NULL);
	
	gtk_container_foreach (GTK_CONTAINER (window->priv->notebook),
			       (GtkCallback)add_view,
			       &res);
			       
	res = g_list_reverse (res);
	
	return res;
}

/**
 * gedit_window_close_tab:
 * @window: a #GeditWindow
 * @page: the #GeditPage to close
 *
 * Closes the @page.
 */
void
gedit_window_close_page (GeditWindow *window,
			 GeditPage   *page)
{
	GList *containers, *l;

	g_return_if_fail (GEDIT_IS_WINDOW (window));
	g_return_if_fail (GEDIT_IS_PAGE (page));
	
	/* FIXME: This should go in split-removed ? */
	containers = gedit_page_get_view_containers (page);
	for (l = containers; l != NULL; l = g_list_next (l))
	{
		GeditViewContainer *c = GEDIT_VIEW_CONTAINER (l->data);
	
		g_return_if_fail ((gedit_view_container_get_state (c) != GEDIT_VIEW_CONTAINER_STATE_SAVING) &&
				  (gedit_view_container_get_state (c) != GEDIT_VIEW_CONTAINER_STATE_SHOWING_PRINT_PREVIEW));
	}
	
	gedit_notebook_remove_page (GEDIT_NOTEBOOK (window->priv->notebook),
				    page);
}

/**
 * gedit_window_close_all_pages:
 * @window: a #GeditWindow
 *
 * Closes all opened pages.
 */
void
gedit_window_close_all_pages (GeditWindow *window)
{
	g_return_if_fail (GEDIT_IS_WINDOW (window));
	g_return_if_fail (!(window->priv->state & GEDIT_WINDOW_STATE_SAVING) &&
			  !(window->priv->state & GEDIT_WINDOW_STATE_SAVING_SESSION));

	window->priv->removing_pages = TRUE;

	gedit_notebook_remove_all_pages (GEDIT_NOTEBOOK (window->priv->notebook));

	window->priv->removing_pages = FALSE;
}

/**
 * gedit_window_close_pages:
 * @window: a #GeditWindow
 * @pages: a list of #GeditPage
 *
 * Closes all pages specified by @pages.
 */
void
gedit_window_close_pages (GeditWindow *window,
			  const GList *pages)
{
	g_return_if_fail (GEDIT_IS_WINDOW (window));
	g_return_if_fail (!(window->priv->state & GEDIT_WINDOW_STATE_SAVING) &&
			  !(window->priv->state & GEDIT_WINDOW_STATE_SAVING_SESSION));

	if (pages == NULL)
		return;

	window->priv->removing_pages = TRUE;

	while (pages != NULL)
	{
		if (pages->next == NULL)
			window->priv->removing_pages = FALSE;

		gedit_notebook_remove_page (GEDIT_NOTEBOOK (window->priv->notebook),
					    GEDIT_PAGE (pages->data));

		pages = g_list_next (pages);
	}

	g_return_if_fail (window->priv->removing_pages == FALSE);
}

GeditWindow *
_gedit_window_move_page_to_new_window (GeditWindow *window,
				       GeditPage   *page)
{
	GeditWindow *new_window;

	g_return_val_if_fail (GEDIT_IS_WINDOW (window), NULL);
	g_return_val_if_fail (GEDIT_IS_PAGE (page), NULL);
	g_return_val_if_fail (gtk_notebook_get_n_pages (
			      GTK_NOTEBOOK (window->priv->notebook)) > 1, 
			      NULL);

	new_window = clone_window (window);

	gedit_notebook_move_page (GEDIT_NOTEBOOK (window->priv->notebook),
				  GEDIT_NOTEBOOK (new_window->priv->notebook),
				  page,
				  -1);

	gtk_widget_show (GTK_WIDGET (new_window));
	
	return new_window;
}

/**
 * gedit_window_set_active_page:
 * @window: a #GeditWindow
 * @page: a #GeditPage
 *
 * Switches to the page that matches with @page.
 */
void
gedit_window_set_active_page (GeditWindow *window,
			      GeditPage   *page)
{
	gint page_num;
	
	g_return_if_fail (GEDIT_IS_WINDOW (window));
	g_return_if_fail (GEDIT_IS_PAGE (page));
	
	page_num = gtk_notebook_page_num (GTK_NOTEBOOK (window->priv->notebook),
					  GTK_WIDGET (page));
	g_return_if_fail (page_num != -1);
	
	gtk_notebook_set_current_page (GTK_NOTEBOOK (window->priv->notebook),
				       page_num);
}

/**
 * gedit_window_get_group:
 * @window: a #GeditWindow
 *
 * Gets the #GtkWindowGroup in which @window resides.
 *
 * Returns: the #GtkWindowGroup
 */
GtkWindowGroup *
gedit_window_get_group (GeditWindow *window)
{
	g_return_val_if_fail (GEDIT_IS_WINDOW (window), NULL);
	
	return window->priv->window_group;
}

gboolean
_gedit_window_is_removing_pages (GeditWindow *window)
{
	g_return_val_if_fail (GEDIT_IS_WINDOW (window), FALSE);
	
	return window->priv->removing_pages;
}

/**
 * gedit_window_get_ui_manager:
 * @window: a #GeditWindow
 *
 * Gets the #GtkUIManager associated with the @window.
 *
 * Returns: the #GtkUIManager of the @window.
 */
GtkUIManager *
gedit_window_get_ui_manager (GeditWindow *window)
{
	g_return_val_if_fail (GEDIT_IS_WINDOW (window), NULL);

	return window->priv->manager;
}

/**
 * gedit_window_get_side_panel:
 * @window: a #GeditWindow
 *
 * Gets the side #GeditPanel of the @window.
 *
 * Returns: the side #GeditPanel.
 */
GeditPanel *
gedit_window_get_side_panel (GeditWindow *window)
{
	g_return_val_if_fail (GEDIT_IS_WINDOW (window), NULL);

	return GEDIT_PANEL (window->priv->side_panel);
}

/**
 * gedit_window_get_bottom_panel:
 * @window: a #GeditWindow
 *
 * Gets the bottom #GeditPanel of the @window.
 *
 * Returns: the bottom #GeditPanel.
 */
GeditPanel *
gedit_window_get_bottom_panel (GeditWindow *window)
{
	g_return_val_if_fail (GEDIT_IS_WINDOW (window), NULL);

	return GEDIT_PANEL (window->priv->bottom_panel);
}

/**
 * gedit_window_get_statusbar:
 * @window: a #GeditWindow
 *
 * Gets the #GeditStatusbar of the @window.
 *
 * Returns: the #GeditStatusbar of the @window.
 */
GtkWidget *
gedit_window_get_statusbar (GeditWindow *window)
{
	g_return_val_if_fail (GEDIT_IS_WINDOW (window), 0);

	return window->priv->statusbar;
}

/**
 * gedit_window_get_state:
 * @window: a #GeditWindow
 *
 * Retrieves the state of the @window.
 *
 * Returns: the current #GeditWindowState of the @window.
 */
GeditWindowState
gedit_window_get_state (GeditWindow *window)
{
	g_return_val_if_fail (GEDIT_IS_WINDOW (window), GEDIT_WINDOW_STATE_NORMAL);

	return window->priv->state;
}

GFile *
_gedit_window_get_default_location (GeditWindow *window)
{
	g_return_val_if_fail (GEDIT_IS_WINDOW (window), NULL);

	return window->priv->default_location != NULL ?
		g_object_ref (window->priv->default_location) : NULL;
}

void
_gedit_window_set_default_location (GeditWindow *window,
				    GFile       *location)
{
	GFile *dir;

	g_return_if_fail (GEDIT_IS_WINDOW (window));
	g_return_if_fail (G_IS_FILE (location));

	dir = g_file_get_parent (location);
	g_return_if_fail (dir != NULL);

	if (window->priv->default_location != NULL)
		g_object_unref (window->priv->default_location);

	window->priv->default_location = dir;
}

/**
 * gedit_window_get_unsaved_documents:
 * @window: a #GeditWindow
 *
 * Gets the list of documents that need to be saved before closing the window.
 *
 * Returns: a list of #GeditDocument that need to be saved before closing the window
 */
GList *
gedit_window_get_unsaved_documents (GeditWindow *window)
{
	GList *unsaved_docs = NULL;
	GList *pages;
	GList *l;

	g_return_val_if_fail (GEDIT_IS_WINDOW (window), NULL);
	
	pages = gtk_container_get_children (GTK_CONTAINER (window->priv->notebook));
	
	l = pages;
	while (l != NULL)
	{
		GeditViewContainer *container;

		container = gedit_page_get_active_view_container (GEDIT_PAGE (l->data));
		
		if (!_gedit_view_container_can_close (container))
		{
			GeditDocument *doc;
			
			doc = gedit_view_container_get_document (container);
			unsaved_docs = g_list_prepend (unsaved_docs, doc);
		}	
		
		l = g_list_next (l);
	}
	
	g_list_free (pages);

	return g_list_reverse (unsaved_docs);
}

void 
_gedit_window_set_saving_session_state (GeditWindow *window,
					gboolean     saving_session)
{
	GeditWindowState old_state;

	g_return_if_fail (GEDIT_IS_WINDOW (window));
	
	old_state = window->priv->state;

	if (saving_session)
		window->priv->state |= GEDIT_WINDOW_STATE_SAVING_SESSION;
	else
		window->priv->state &= ~GEDIT_WINDOW_STATE_SAVING_SESSION;

	if (old_state != window->priv->state)
	{
		set_sensitivity_according_to_window_state (window);

		g_object_notify (G_OBJECT (window), "state");
	}
}

static void
hide_notebook_tabs_on_fullscreen (GtkNotebook	*notebook, 
				  GParamSpec	*pspec,
				  GeditWindow	*window)
{
	gtk_notebook_set_show_tabs (notebook, FALSE);
}

void
_gedit_window_fullscreen (GeditWindow *window)
{
	g_return_if_fail (GEDIT_IS_WINDOW (window));

	if (_gedit_window_is_fullscreen (window))
		return;

	/* Go to fullscreen mode and hide bars */
	gtk_window_fullscreen (&window->window);
	gtk_notebook_set_show_tabs (GTK_NOTEBOOK (window->priv->notebook), FALSE);
	g_signal_connect (window->priv->notebook, "notify::show-tabs",
			  G_CALLBACK (hide_notebook_tabs_on_fullscreen), window);
	
	gtk_widget_hide (gtk_ui_manager_get_widget (window->priv->manager,
			 "/MenuBar"));
	
	g_signal_handlers_block_by_func (window->priv->toolbar,
					 toolbar_visibility_changed,
					 window);
	gtk_widget_hide (window->priv->toolbar);
	
	g_signal_handlers_block_by_func (window->priv->statusbar,
					 statusbar_visibility_changed,
					 window);
	gtk_widget_hide (window->priv->statusbar);

	fullscreen_controls_build (window);
	fullscreen_controls_show (window);
}

void
_gedit_window_unfullscreen (GeditWindow *window)
{
	gboolean visible;
	GtkAction *action;

	g_return_if_fail (GEDIT_IS_WINDOW (window));

	if (!_gedit_window_is_fullscreen (window))
		return;

	/* Unfullscreen and show bars */
	gtk_window_unfullscreen (&window->window);
	g_signal_handlers_disconnect_by_func (window->priv->notebook,
					      hide_notebook_tabs_on_fullscreen,
					      window);
	gtk_notebook_set_show_tabs (GTK_NOTEBOOK (window->priv->notebook), TRUE);
	gtk_widget_show (gtk_ui_manager_get_widget (window->priv->manager, "/MenuBar"));
	
	action = gtk_action_group_get_action (window->priv->always_sensitive_action_group,
					      "ViewToolbar");
	visible = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
	if (visible)
		gtk_widget_show (window->priv->toolbar);
	g_signal_handlers_unblock_by_func (window->priv->toolbar,
					   toolbar_visibility_changed,
					   window);
	
	action = gtk_action_group_get_action (window->priv->always_sensitive_action_group,
					      "ViewStatusbar");
	visible = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
	if (visible)
		gtk_widget_show (window->priv->statusbar);
	g_signal_handlers_unblock_by_func (window->priv->statusbar,
					   statusbar_visibility_changed,
					   window);

	gtk_widget_hide (window->priv->fullscreen_controls);
}

gboolean
_gedit_window_is_fullscreen (GeditWindow *window)
{
	g_return_val_if_fail (GEDIT_IS_WINDOW (window), FALSE);

	return window->priv->window_state & GDK_WINDOW_STATE_FULLSCREEN;
}

/**
 * gedit_window_get_tab_from_location:
 * @window: a #GeditWindow
 * @location: a #GFile
 *
 * Gets the #GeditViewContainer that matches with the given @location.
 *
 * Returns: the #GeditViewContainer that matches with the given @location.
 */
GeditViewContainer *
gedit_window_get_view_container_from_location (GeditWindow *window,
					       GFile       *location)
{
	GList *tabs;
	GList *l;
	GeditViewContainer *ret = NULL;

	g_return_val_if_fail (GEDIT_IS_WINDOW (window), NULL);
	g_return_val_if_fail (G_IS_FILE (location), NULL);

	tabs = gtk_container_get_children (GTK_CONTAINER (window->priv->notebook));
	
	for (l = tabs; l != NULL; l = g_list_next (l))
	{
		GeditDocument *d;
		GeditViewContainer *t;
		GFile *f;

		t = GEDIT_VIEW_CONTAINER (l->data);
		d = gedit_view_container_get_document (t);

		f = gedit_document_get_location (d);

		if (f != NULL)
		{
			gboolean found = g_file_equal (location, f);

			g_object_unref (f);

			if (found)
			{
				ret = t;
				break;
			}
		}
	}
	
	g_list_free (tabs);
	
	return ret;
}

/**
 * gedit_window_get_message_bus:
 * @window: a #GeditWindow
 *
 * Gets the #GeditMessageBus associated with @window. The returned reference
 * is owned by the window and should not be unreffed.
 *
 * Return value: the #GeditMessageBus associated with @window
 */
GeditMessageBus	*
gedit_window_get_message_bus (GeditWindow *window)
{
	g_return_val_if_fail (GEDIT_IS_WINDOW (window), NULL);
	
	return window->priv->message_bus;
}

