/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-mdi.c
 * This file is part of gedit
 *
 * Copyright (C) 1998, 1999 Alex Roberts, Evan Lawrence
 * Copyright (C) 2000, 2001 Chema Celorio, Paolo Maggi 
 * Copyright (C) 2002  Paolo Maggi 
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

#include <libgnome/libgnome.h>
#include <libgnomeui/libgnomeui.h>
#include <libgnomevfs/gnome-vfs.h>
#include <eel/eel-string.h>

#include <gtksourceview/gtksourcelanguage.h>
#include <gtksourceview/gtksourcelanguagesmanager.h>

#include <string.h>

#include "gedit-mdi.h"
#include "gedit-mdi-child.h"
#include "gedit2.h"
#include "gedit-menus.h"
#include "gedit-debug.h"
#include "gedit-prefs-manager-app.h"
#include "gedit-recent.h" 
#include "gedit-file.h"
#include "gedit-view.h"
#include "gedit-utils.h"
#include "gedit-plugins-engine.h"
#include "gedit-output-window.h"
#include "recent-files/egg-recent-view-bonobo.h"
#include "recent-files/egg-recent-view-gtk.h"
#include "recent-files/egg-recent-model.h"
#include "gedit-languages-manager.h"
#include "dialogs/gedit-close-confirmation-dialog.h"

#include <eel/eel-alert-dialog.h>

#include <bonobo/bonobo-ui-util.h>
#include <bonobo/bonobo-control.h>

#include <gconf/gconf-client.h>

#define RECENT_KEY 		"GeditRecent"
#define OUTPUT_WINDOW_KEY	"GeditOutputWindow"

struct _GeditMDIPrivate
{
	GeditState state;
};

typedef struct _GeditWindowPrefs	GeditWindowPrefs;

struct _GeditWindowPrefs
{
	gboolean toolbar_visible;
	GeditToolbarSetting toolbar_buttons_style;

	gboolean statusbar_visible;

	gboolean output_window_visible;
};

static void gedit_mdi_class_init 	(GeditMDIClass	*klass);
static void gedit_mdi_init 		(GeditMDI 	*mdi);
static void gedit_mdi_finalize 		(GObject 	*object);

static void gedit_mdi_app_created_handler	(BonoboMDI *mdi, BonoboWindow *win);
static void gedit_mdi_drag_data_received_handler (GtkWidget *widget, GdkDragContext *context, 
		                                  gint x, gint y, 
						  GtkSelectionData *selection_data, 
				                  guint info, guint time);
static void gedit_mdi_set_app_toolbar_style 	(BonoboWindow *win);
static void gedit_mdi_set_app_statusbar_style 	(BonoboWindow *win);

static gint gedit_mdi_add_child_handler (BonoboMDI *mdi, BonoboMDIChild *child);
static gint gedit_mdi_add_view_handler (BonoboMDI *mdi, GtkWidget *view);
static gint gedit_mdi_remove_child_handler (BonoboMDI *mdi, BonoboMDIChild *child);

static gboolean gedit_mdi_remove_view_handler  (BonoboMDI *mdi, GtkWidget *view);
static gboolean gedit_mdi_remove_views_handler (BonoboMDI *mdi, BonoboWindow *window);

static void gedit_mdi_view_changed_handler (BonoboMDI *mdi, GtkWidget *old_view);
static void gedit_mdi_child_changed_handler (BonoboMDI *mdi, BonoboMDIChild *old_child);
static void gedit_mdi_child_state_changed_handler (GeditMDIChild *child);

static void gedit_mdi_set_active_window_undo_redo_verbs_sensitivity (BonoboMDI *mdi);
static void gedit_mdi_set_active_window_find_verbs_sensitivity (BonoboMDI *mdi);

static void gedit_mdi_app_destroy_handler (BonoboMDI *mdi, BonoboWindow *window);

static void gedit_mdi_view_menu_item_toggled_handler (
			BonoboUIComponent           *ui_component,
			const char                  *path,
			Bonobo_UIComponent_EventType type,
			const char                  *state,
			BonoboWindow                *win);
static void add_languages_menu (BonoboMDI *mdi, BonoboWindow *win);
static void gedit_mdi_update_languages_menu (BonoboMDI *mdi);

static GQuark window_prefs_id = 0;

static GeditWindowPrefs *gedit_window_prefs_new 		(void);
static void		 gedit_window_prefs_attach_to_window 	(GeditWindowPrefs *prefs,
								 BonoboWindow 	  *win);
static GeditWindowPrefs	*gedit_window_prefs_get_from_window 	(BonoboWindow     *win);
static void		 gedit_window_prefs_save 		(GeditWindowPrefs *prefs);

enum 
{
	STATE_CHANGED = 0,
	LAST_SIGNAL
};

static BonoboMDIClass *parent_class = NULL;
static guint mdi_signals [LAST_SIGNAL] = { 0 };

enum
{
	TARGET_URI_LIST = 100
};

static GtkTargetEntry drag_types[] =
{
	{ "text/uri-list", 0, TARGET_URI_LIST },
};

static gint n_drag_types = sizeof (drag_types) / sizeof (drag_types [0]);


GType
gedit_mdi_get_type (void)
{
	static GType mdi_type = 0;

  	if (mdi_type == 0)
    	{
      		static const GTypeInfo our_info =
      		{
        		sizeof (GeditMDIClass),
        		NULL,		/* base_init */
        		NULL,		/* base_finalize */
        		(GClassInitFunc) gedit_mdi_class_init,
        		NULL,           /* class_finalize */
        		NULL,           /* class_data */
        		sizeof (GeditMDI),
        		0,              /* n_preallocs */
        		(GInstanceInitFunc) gedit_mdi_init
      		};

      		mdi_type = g_type_register_static (BONOBO_TYPE_MDI,
                				    "GeditMDI",
                                       	 	    &our_info,
                                       		    0);
    	}

	return mdi_type;
}

static void
gedit_mdi_class_init (GeditMDIClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

  	parent_class = g_type_class_peek_parent (klass);

  	object_class->finalize = gedit_mdi_finalize;

	mdi_signals[STATE_CHANGED] = 
		g_signal_new ("state_changed",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GeditMDIClass, state_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__INT,
			      G_TYPE_NONE, 
			      1, 
			      G_TYPE_INT);
}

static void
menu_position_under_widget (GtkMenu *menu, int *x, int *y,
			    gboolean *push_in, gpointer user_data)
{
	GtkWidget *w;
	int screen_width, screen_height;
	GtkRequisition requisition;

	w = GTK_WIDGET (user_data);
	
	gdk_window_get_origin (w->window, x, y);
	*x += w->allocation.x;
	*y += w->allocation.y + w->allocation.height;

	gtk_widget_size_request (GTK_WIDGET (menu), &requisition);

	screen_width = gdk_screen_width ();
	screen_height = gdk_screen_height ();

	*x = CLAMP (*x, 0, MAX (0, screen_width - requisition.width));
	*y = CLAMP (*y, 0, MAX (0, screen_height - requisition.height));
}

static gboolean
open_button_pressed_cb (GtkWidget *widget,
			      GdkEventButton *event,
			      gpointer *user_data)
{
	GtkWidget *menu;
	GeditMDI *mdi;

	g_return_val_if_fail (GTK_IS_BUTTON (widget), FALSE);
	g_return_val_if_fail (GEDIT_IS_MDI (user_data), FALSE);

	mdi = GEDIT_MDI (user_data);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), TRUE);

	menu = g_object_get_data (G_OBJECT (widget), "recent-menu");
	gnome_popup_menu_do_popup_modal (menu,
				menu_position_under_widget, widget,
				event, widget, widget);
	
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), FALSE);
	
	return TRUE;
}

static gboolean
open_button_key_pressed_cb (GtkWidget *widget,
				  GdkEventKey *event,
				  gpointer *user_data)
{
	if (event->keyval == GDK_space ||
	    event->keyval == GDK_KP_Space ||
	    event->keyval == GDK_Return ||
	    event->keyval == GDK_KP_Enter) {
		open_button_pressed_cb (widget, NULL, user_data);
	}

	return FALSE;
}

static void
tooltip_func (GtkTooltips   *tooltips,
	      GtkWidget     *menu,
	      EggRecentItem *item,
	      gpointer       user_data)
{
	gchar *tip;
	gchar *uri_for_display;

	uri_for_display = egg_recent_item_get_uri_for_display (item);
	g_return_if_fail (uri_for_display != NULL);
	
	/* Translators: %s is a URI */
	tip = g_strdup_printf (_("Open '%s'"), uri_for_display);

	g_free (uri_for_display);

	gtk_tooltips_set_tip (tooltips, GTK_WIDGET (menu), tip, NULL);

	g_free (tip);
}

static GtkWidget *
gedit_mdi_add_open_button (GeditMDI *mdi, BonoboUIComponent *ui_component,
			 const gchar *path, const gchar *tooltip)
{
	GtkWidget *menu;
	EggRecentViewGtk *view;
	EggRecentModel *model;
	GtkWidget *button;
	
	static GtkTooltips *button_tooltip = NULL;

	if (button_tooltip == NULL)
		button_tooltip = gtk_tooltips_new ();

	button = gtk_toggle_button_new ();
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);

	gtk_tooltips_set_tip (button_tooltip, GTK_WIDGET (button), tooltip, NULL);

	gtk_container_add (GTK_CONTAINER (button),
			   gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_OUT));

	gtk_widget_show_all (GTK_WIDGET (button));

	model = gedit_recent_get_model ();

	menu = gtk_menu_new ();
	gtk_widget_show (menu);
	view = egg_recent_view_gtk_new (menu, NULL);
	g_signal_connect (view, "activate",
			  G_CALLBACK (gedit_file_open_recent), NULL);
	egg_recent_view_gtk_show_icons (view, TRUE);
	egg_recent_view_gtk_show_numbers (view, FALSE);
	egg_recent_view_gtk_set_tooltip_func (view, tooltip_func, NULL);
	
	egg_recent_view_set_model (EGG_RECENT_VIEW (view), model);
	g_object_set_data (G_OBJECT (button), "recent-menu", menu);
	
	g_signal_connect_object (button, "key_press_event",
				 G_CALLBACK (open_button_key_pressed_cb),
				 mdi, 0);
	g_signal_connect_object (button, "button_press_event",
				 G_CALLBACK (open_button_pressed_cb),
				 mdi, 0);

	bonobo_ui_component_widget_set (ui_component, path, GTK_WIDGET (button),
					NULL);

	return button;
}

static void 
gedit_mdi_init (GeditMDI  *mdi)
{
	gedit_debug (DEBUG_MDI, "START");

	bonobo_mdi_construct (BONOBO_MDI (mdi), 
			      "gedit-2", 
			      "gedit",
			      gedit_prefs_manager_get_default_window_width (),
			      gedit_prefs_manager_get_default_window_height ());
	
	mdi->priv = g_new0 (GeditMDIPrivate, 1);

	mdi->priv->state = GEDIT_STATE_NORMAL;

	bonobo_mdi_set_ui_template_file (BONOBO_MDI (mdi), GEDIT_UI_DIR "gedit-ui.xml", gedit_verbs);
	
	bonobo_mdi_set_child_list_path (BONOBO_MDI (mdi), "/menu/Documents/OpenDocuments/");

	/* Connect signals */
	g_signal_connect (G_OBJECT (mdi), "top_window_created",
			  G_CALLBACK (gedit_mdi_app_created_handler), NULL);
	
	g_signal_connect (G_OBJECT (mdi), "add_child",
			  G_CALLBACK (gedit_mdi_add_child_handler), NULL);
	g_signal_connect (G_OBJECT (mdi), "add_view",
			  G_CALLBACK (gedit_mdi_add_view_handler), NULL);
	
	g_signal_connect (G_OBJECT (mdi), "remove_child",
			  G_CALLBACK (gedit_mdi_remove_child_handler), NULL);
	g_signal_connect (G_OBJECT (mdi), "remove_view",
			  G_CALLBACK (gedit_mdi_remove_view_handler), NULL);
	g_signal_connect (G_OBJECT (mdi), "remove_views",
			  G_CALLBACK (gedit_mdi_remove_views_handler), NULL);

	g_signal_connect (G_OBJECT (mdi), "child_changed",
			  G_CALLBACK (gedit_mdi_child_changed_handler), NULL);
	g_signal_connect (G_OBJECT (mdi), "view_changed",
			  G_CALLBACK (gedit_mdi_view_changed_handler), NULL);
	
	g_signal_connect (G_OBJECT (mdi), "all_windows_destroyed",
			  G_CALLBACK (gedit_file_exit), NULL);

	g_signal_connect (G_OBJECT (mdi), "top_window_destroy",
			  G_CALLBACK (gedit_mdi_app_destroy_handler), NULL);

			  
	gedit_debug (DEBUG_MDI, "END");
}

static void
gedit_mdi_finalize (GObject *object)
{
	GeditMDI *mdi;

	gedit_debug (DEBUG_MDI, "");

	g_return_if_fail (object != NULL);
	
   	mdi = GEDIT_MDI (object);

	g_return_if_fail (GEDIT_IS_MDI (mdi));
	g_return_if_fail (mdi->priv != NULL);

	g_free (mdi->priv);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}


/**
 * gedit_mdi_new:
 * 
 * Creates a new #GeditMDI object.
 *
 * Return value: a new #GeditMDI
 **/
GeditMDI*
gedit_mdi_new (void)
{
	GeditMDI *mdi;

	gedit_debug (DEBUG_MDI, "");

	mdi = GEDIT_MDI (g_object_new (GEDIT_TYPE_MDI, NULL));
  	g_return_val_if_fail (mdi != NULL, NULL);
	
	return mdi;
}

static gchar* 
tip_func (EggRecentItem *item, gpointer user_data)
{
	gchar *tip;
	gchar *uri_for_display;

	uri_for_display = egg_recent_item_get_uri_for_display (item);
	g_return_val_if_fail (uri_for_display != NULL, NULL);
	
	/* Translators: %s is a URI */
	tip = g_strdup_printf (_("Open '%s'"), uri_for_display);

	g_free (uri_for_display);

	return tip;
}

static void
gedit_mdi_app_created_handler (BonoboMDI *mdi, BonoboWindow *win)
{
	GtkWidget *widget;

	BonoboControl *control;
	BonoboUIComponent *ui_component;
	EggRecentView *view;
	EggRecentModel *model;
	GeditWindowPrefs *prefs;
	GdkWindowState state;
	
	gedit_debug (DEBUG_MDI, "");
	
	ui_component = bonobo_mdi_get_ui_component_from_window (win);
	g_return_if_fail (ui_component != NULL);
	
	/* Drag and drop support */
	gtk_drag_dest_set (GTK_WIDGET (win),
			   GTK_DEST_DEFAULT_MOTION |
			   GTK_DEST_DEFAULT_HIGHLIGHT |
			   GTK_DEST_DEFAULT_DROP,
			   drag_types, n_drag_types,
			   GDK_ACTION_COPY);
		
	g_signal_connect (G_OBJECT (win), "drag_data_received",
			  G_CALLBACK (gedit_mdi_drag_data_received_handler), 
			  NULL);
		
	/* Add cursor position status bar */
	widget = gtk_statusbar_new ();
	control = bonobo_control_new (widget);
	
	gtk_widget_set_size_request (widget, 160, 10);	
	gtk_statusbar_set_has_resize_grip (GTK_STATUSBAR (widget), FALSE);
	
	bonobo_ui_component_object_set (ui_component,
		       			"/status/CursorPosition",
					BONOBO_OBJREF (control),
					NULL);

	gtk_widget_show (widget);

	bonobo_object_unref (BONOBO_OBJECT (control));

	g_object_set_data (G_OBJECT (win), "CursorPosition", widget);

	/* Add overwrite mode status bar */
	widget = gtk_statusbar_new ();
	control = bonobo_control_new (widget);
	
	gtk_widget_set_size_request (widget, 80, 10);
	
	bonobo_ui_component_object_set (ui_component,
		       			"/status/OverwriteMode",
					BONOBO_OBJREF (control),
					NULL);

	gtk_widget_show (widget);

	bonobo_object_unref (BONOBO_OBJECT (control));

	g_object_set_data (G_OBJECT (win), "OverwriteMode", widget);
	
	/* Add custom Open button to toolbar */
	widget = gedit_mdi_add_open_button (GEDIT_MDI (mdi),
					    ui_component, 
					    "/Toolbar/FileOpenMenu",
					    _("Open a recently used file"));
	g_object_set_data (G_OBJECT (win), "recent-menu-button", widget);

	prefs = gedit_window_prefs_new ();
	gedit_window_prefs_attach_to_window (prefs, win);

	/* Set the statusbar style according to prefs */
	gedit_mdi_set_app_statusbar_style (win);
	
	/* Set the toolbar style according to prefs */
	gedit_mdi_set_app_toolbar_style (win);

	gedit_menus_set_verb_state (ui_component, 
				    "/commands/ViewOutputWindow",
				    prefs->output_window_visible);
		
	/* Add listener fo the view menu */
	bonobo_ui_component_add_listener (ui_component, "ViewToolbar", 
			(BonoboUIListenerFn)gedit_mdi_view_menu_item_toggled_handler, 
			(gpointer)win);
	bonobo_ui_component_add_listener (ui_component, "ViewStatusbar", 
			(BonoboUIListenerFn)gedit_mdi_view_menu_item_toggled_handler, 
			(gpointer)win);
	bonobo_ui_component_add_listener (ui_component, "ViewOutputWindow", 
			(BonoboUIListenerFn)gedit_mdi_view_menu_item_toggled_handler, 
			(gpointer)win);

	bonobo_ui_component_add_listener (ui_component, "ToolbarSystem", 
			(BonoboUIListenerFn)gedit_mdi_view_menu_item_toggled_handler, 
			(gpointer)win);
	bonobo_ui_component_add_listener (ui_component, "ToolbarIcon", 
			(BonoboUIListenerFn)gedit_mdi_view_menu_item_toggled_handler, 
			(gpointer)win);
	bonobo_ui_component_add_listener (ui_component, "ToolbarIconText", 
			(BonoboUIListenerFn)gedit_mdi_view_menu_item_toggled_handler, 
			(gpointer)win);
	bonobo_ui_component_add_listener (ui_component, "ToolbarIconBothHoriz", 
			(BonoboUIListenerFn)gedit_mdi_view_menu_item_toggled_handler, 
			(gpointer)win);
	
	/* add a GeditRecentView object */
	model = gedit_recent_get_model ();
	view = EGG_RECENT_VIEW (egg_recent_view_bonobo_new (
					ui_component, "/menu/File/Recents"));
	egg_recent_view_bonobo_show_icons (EGG_RECENT_VIEW_BONOBO (view), FALSE);
	egg_recent_view_bonobo_set_tooltip_func (EGG_RECENT_VIEW_BONOBO (view), tip_func, NULL);
	
	egg_recent_view_set_model (view, model);

	g_signal_connect (G_OBJECT (view), "activate",
			  G_CALLBACK (gedit_file_open_recent), NULL);
	
	g_object_set_data_full (G_OBJECT (win), RECENT_KEY, view,
				g_object_unref);

		
	/* Set window state and size, but only if the session is not being restored */
	if (!bonobo_mdi_get_restoring_state (mdi))
	{
		state = gedit_prefs_manager_get_window_state ();

		if ((state & GDK_WINDOW_STATE_MAXIMIZED) != 0)
		{
			gtk_window_set_default_size (GTK_WINDOW (win),
						     gedit_prefs_manager_get_default_window_width (),
						     gedit_prefs_manager_get_default_window_height ());

			gtk_window_maximize (GTK_WINDOW (win));
		}
		else
		{
			gtk_window_set_default_size (GTK_WINDOW (win), 
						     gedit_prefs_manager_get_window_width (),
						     gedit_prefs_manager_get_window_height ());

			gtk_window_unmaximize (GTK_WINDOW (win));
		}

		if ((state & GDK_WINDOW_STATE_STICKY ) != 0)
			gtk_window_stick (GTK_WINDOW (win));
		else
			gtk_window_unstick (GTK_WINDOW (win));
	}

	add_languages_menu (mdi, win);
	
	/* Add the plugins menus */
	gedit_plugins_engine_update_plugins_ui (win, TRUE);
}

static void
gedit_mdi_app_destroy_handler (BonoboMDI *mdi, BonoboWindow *window)
{
	gedit_debug (DEBUG_MDI, "");
	
	g_return_if_fail (window != NULL);
	g_return_if_fail (BONOBO_IS_WINDOW (window));

	gedit_prefs_manager_save_window_size_and_state (window);
}

static void
gedit_mdi_view_menu_item_toggled_handler (
			BonoboUIComponent           *ui_component,
			const char                  *path,
			Bonobo_UIComponent_EventType type,
			const char                  *state,
			BonoboWindow                *win)
{
	gboolean s;
	GeditWindowPrefs *prefs;

	gedit_debug (DEBUG_MDI, "%s toggled to '%s'", path, state);

	prefs = gedit_window_prefs_get_from_window (win);
	g_return_if_fail (prefs != NULL);

	s = (strcmp (state, "1") == 0);

	if (strcmp (path, "ViewToolbar") == 0)
	{
		if (s != prefs->toolbar_visible)
		{
			prefs->toolbar_visible = s;
			gedit_mdi_set_app_toolbar_style (win);
		}

		goto save_prefs;
	}

	if (strcmp (path, "ViewStatusbar") == 0)
	{
		if (s != prefs->statusbar_visible)
		{
			prefs->statusbar_visible = s;
			gedit_mdi_set_app_statusbar_style (win);
		}

		goto save_prefs;
	}

	if (strcmp (path, "ViewOutputWindow") == 0)
	{
		if (s != prefs->output_window_visible)
		{
			GtkWidget *ow;

			prefs->output_window_visible = s;
			gedit_mdi_set_app_statusbar_style (win);

			ow = gedit_mdi_get_output_window_from_window (win);

			if (prefs->output_window_visible)
				gtk_widget_show (ow);
			else
				gtk_widget_hide (ow);
		}	

		return;
	}


	if (s && (strcmp (path, "ToolbarSystem") == 0))
	{
		if (prefs->toolbar_buttons_style  != GEDIT_TOOLBAR_SYSTEM)
		{
			prefs->toolbar_buttons_style  = GEDIT_TOOLBAR_SYSTEM;
			gedit_mdi_set_app_toolbar_style (win);
		}

		goto save_prefs;
	}

	if (s && (strcmp (path, "ToolbarIcon") == 0))
	{
		if (prefs->toolbar_buttons_style != GEDIT_TOOLBAR_ICONS)
		{
			prefs->toolbar_buttons_style = GEDIT_TOOLBAR_ICONS;
			gedit_mdi_set_app_toolbar_style (win);
		}
		
		goto save_prefs;
	}

	if (s && (strcmp (path, "ToolbarIconText") == 0))
	{
		if (prefs->toolbar_buttons_style != GEDIT_TOOLBAR_ICONS_AND_TEXT)
		{
			prefs->toolbar_buttons_style = GEDIT_TOOLBAR_ICONS_AND_TEXT;
			gedit_mdi_set_app_toolbar_style (win);
		}

		goto save_prefs;
	}

	if (s && (strcmp (path, "ToolbarIconBothHoriz") == 0))
	{
		if (prefs->toolbar_buttons_style != GEDIT_TOOLBAR_ICONS_BOTH_HORIZ)
		{
			prefs->toolbar_buttons_style = GEDIT_TOOLBAR_ICONS_BOTH_HORIZ;
			gedit_mdi_set_app_toolbar_style (win);
		}

		goto save_prefs;
	}

save_prefs:
	gedit_window_prefs_save (prefs);
}

static void 
gedit_mdi_drag_data_received_handler (GtkWidget *widget, 
				      GdkDragContext *context, 
		                      gint x, 
				      gint y, 
				      GtkSelectionData *selection_data, 
				      guint info, 
				      guint time)
{
	GList *list = NULL;
	GSList *file_list = NULL;
	GList *p = NULL;
	GtkWidget *target_window;

	gedit_debug (DEBUG_MDI, "");

	if (info != TARGET_URI_LIST)
		return;

	g_return_if_fail (widget != NULL);

	target_window = gtk_widget_get_toplevel (widget);
	g_return_if_fail (BONOBO_IS_WINDOW (target_window));

	/* Make sure to activate the target window so to load the
	 * dropped files in the right window */
	if (target_window != GTK_WIDGET(gedit_get_active_window ()))
	{
		GtkWidget *view;
		view = bonobo_mdi_get_view_from_window (BONOBO_MDI (gedit_mdi),
							BONOBO_WINDOW (target_window));
		bonobo_mdi_set_active_view (BONOBO_MDI (gedit_mdi),
					    view);
	}
					
	list = gnome_vfs_uri_list_parse (selection_data->data);
	p = list;

	while (p != NULL)
	{
		file_list = g_slist_prepend (file_list, 
				gnome_vfs_uri_to_string ((const GnomeVFSURI*)(p->data), 
				GNOME_VFS_URI_HIDE_NONE));

		p = g_list_next (p);
	}
	
	gnome_vfs_uri_list_free (list);

	file_list = g_slist_reverse (file_list);

	if (file_list == NULL)
		return;

	gedit_file_open_uri_list (file_list, NULL, 0, FALSE);	
	
	g_slist_foreach (file_list, (GFunc)g_free, NULL);	
	g_slist_free (file_list);
}

static void
gedit_mdi_set_app_toolbar_style (BonoboWindow *win)
{
	BonoboUIComponent *ui_component;
	GeditWindowPrefs *prefs = NULL;

	gedit_debug (DEBUG_MDI, "");
	
	g_return_if_fail (BONOBO_IS_WINDOW (win));
	
	prefs = gedit_window_prefs_get_from_window (win);
	g_return_if_fail (prefs != NULL);
	
	ui_component = bonobo_mdi_get_ui_component_from_window (win);
	g_return_if_fail (ui_component != NULL);
			
	bonobo_ui_component_freeze (ui_component, NULL);

	/* Updated view menu */
	gedit_menus_set_verb_state (ui_component, 
				    "/commands/ViewToolbar",
				    prefs->toolbar_visible);

	gedit_menus_set_verb_sensitive (ui_component, 
				        "/commands/ToolbarSystem",
				        prefs->toolbar_visible);
	gedit_menus_set_verb_sensitive (ui_component, 
				        "/commands/ToolbarIcon",
				        prefs->toolbar_visible);
	gedit_menus_set_verb_sensitive (ui_component, 
				        "/commands/ToolbarIconText",
				        prefs->toolbar_visible);
	gedit_menus_set_verb_sensitive (ui_component, 
				        "/commands/ToolbarIconBothHoriz",
				        prefs->toolbar_visible);
	gedit_menus_set_verb_sensitive (ui_component, 
				        "/commands/ToolbarTooltips",
				        prefs->toolbar_visible);

	gedit_menus_set_verb_state (ui_component, 
				    "/commands/ToolbarSystem",
				    prefs->toolbar_buttons_style == GEDIT_TOOLBAR_SYSTEM);

	gedit_menus_set_verb_state (ui_component, 
				    "/commands/ToolbarIcon",
				    prefs->toolbar_buttons_style == GEDIT_TOOLBAR_ICONS);

	gedit_menus_set_verb_state (ui_component, 
				    "/commands/ToolbarIconText",
				    prefs->toolbar_buttons_style == GEDIT_TOOLBAR_ICONS_AND_TEXT);

	gedit_menus_set_verb_state (ui_component, 
				    "/commands/ToolbarIconBothHoriz",
				    prefs->toolbar_buttons_style == GEDIT_TOOLBAR_ICONS_BOTH_HORIZ);
	
	switch (prefs->toolbar_buttons_style)
	{
		case GEDIT_TOOLBAR_SYSTEM:
			gedit_debug (DEBUG_MDI, "GEDIT: SYSTEM");
			bonobo_ui_component_set_prop (
				ui_component, "/Toolbar", "look", "system", NULL);

			break;
			
		case GEDIT_TOOLBAR_ICONS:
			gedit_debug (DEBUG_MDI, "GEDIT: ICONS");
			bonobo_ui_component_set_prop (
				ui_component, "/Toolbar", "look", "icon", NULL);
			
			break;
			
		case GEDIT_TOOLBAR_ICONS_AND_TEXT:
			gedit_debug (DEBUG_MDI, "GEDIT: ICONS_AND_TEXT");
			bonobo_ui_component_set_prop (
				ui_component, "/Toolbar", "look", "both", NULL);
			
			break;
			
		case GEDIT_TOOLBAR_ICONS_BOTH_HORIZ:
			gedit_debug (DEBUG_MDI, "GEDIT: ICONS_BOTH_HORIZ");
			bonobo_ui_component_set_prop (
				ui_component, "/Toolbar", "look", "both_horiz", NULL);
			
			break;       
		default:
			goto error;
			break;
	}
	
	bonobo_ui_component_set_prop (
			ui_component, "/Toolbar",
			"hidden", prefs->toolbar_visible ? "0":"1", NULL);

 error:
	bonobo_ui_component_thaw (ui_component, NULL);
}

static void
gedit_mdi_set_app_statusbar_style (BonoboWindow *win)
{
	GeditWindowPrefs *prefs = NULL;
	BonoboUIComponent *ui_component;
	
	gedit_debug (DEBUG_MDI, "");
	
	g_return_if_fail (BONOBO_IS_WINDOW (win));

	prefs = gedit_window_prefs_get_from_window (win);
	g_return_if_fail (prefs != NULL);

	ui_component = bonobo_mdi_get_ui_component_from_window (win);
	g_return_if_fail (ui_component != NULL);

	bonobo_ui_component_freeze (ui_component, NULL);
	
	/* Update menu */
	gedit_menus_set_verb_state (ui_component, 
				    "/commands/ViewStatusbar",
				    prefs->statusbar_visible);
	
	/* Actually update status bar style */
	bonobo_ui_component_set_prop (
		ui_component, "/status",
		"hidden", prefs->statusbar_visible ? "0" : "1",
		NULL);

	bonobo_ui_component_thaw (ui_component, NULL);
}

static void 
gedit_mdi_child_state_changed_handler (GeditMDIChild *child)
{
	gedit_debug (DEBUG_MDI, "");

	if (bonobo_mdi_get_active_child (BONOBO_MDI (gedit_mdi)) != BONOBO_MDI_CHILD (child))
		return;
	
	gedit_mdi_set_active_window_title (BONOBO_MDI (gedit_mdi));
	gedit_mdi_set_active_window_verbs_sensitivity (BONOBO_MDI (gedit_mdi));
	gedit_mdi_update_languages_menu (BONOBO_MDI (gedit_mdi));
}

static void 
gedit_mdi_child_undo_redo_state_changed_handler (GeditMDIChild *child)
{
	gedit_debug (DEBUG_MDI, "");

	if (bonobo_mdi_get_active_child (BONOBO_MDI (gedit_mdi)) != BONOBO_MDI_CHILD (child))
		return;
	
	gedit_mdi_set_active_window_undo_redo_verbs_sensitivity (BONOBO_MDI (gedit_mdi));
}

static void 
gedit_mdi_child_find_state_changed_handler (GeditMDIChild *child)
{
	gedit_debug (DEBUG_MDI, "");

	if (bonobo_mdi_get_active_child (BONOBO_MDI (gedit_mdi)) != BONOBO_MDI_CHILD (child))
		return;
	
	gedit_mdi_set_active_window_find_verbs_sensitivity (BONOBO_MDI (gedit_mdi));
}

static gint 
gedit_mdi_add_child_handler (BonoboMDI *mdi, BonoboMDIChild *child)
{
	gedit_debug (DEBUG_MDI, "");

	g_signal_connect (G_OBJECT (child), "state_changed",
			  G_CALLBACK (gedit_mdi_child_state_changed_handler), 
			  NULL);
	g_signal_connect (G_OBJECT (child), "undo_redo_state_changed",
			  G_CALLBACK (gedit_mdi_child_undo_redo_state_changed_handler), 
			  NULL);
	g_signal_connect (G_OBJECT (child), "find_state_changed",
			  G_CALLBACK (gedit_mdi_child_find_state_changed_handler), 
			  NULL);

	return TRUE;
}

static gint 
gedit_mdi_add_view_handler (BonoboMDI *mdi, GtkWidget *view)
{
	GtkTextView *text_view;
	GtkTargetList *tl;

	gedit_debug (DEBUG_MDI, "");

	g_return_val_if_fail (view != NULL, TRUE);

	text_view = gedit_view_get_gtk_text_view (GEDIT_VIEW (view));
	g_return_val_if_fail (text_view != NULL, TRUE);
	
	/* Drag and drop support */
	tl = gtk_drag_dest_get_target_list (GTK_WIDGET (text_view));
	g_return_val_if_fail (tl != NULL, TRUE);

	gtk_target_list_add_table (tl, drag_types, n_drag_types);

	g_signal_connect (G_OBJECT (text_view), "drag_data_received",
			  G_CALLBACK (gedit_mdi_drag_data_received_handler), 
			  NULL);

	return TRUE;
}


static gint 
gedit_mdi_remove_child_handler (BonoboMDI *mdi, BonoboMDIChild *child)
{
	GeditDocument* doc;
	gboolean close = TRUE;
	
	gedit_debug (DEBUG_MDI, "");

	g_return_val_if_fail (child != NULL, FALSE);
	g_return_val_if_fail (GEDIT_MDI_CHILD (child)->document != NULL, FALSE);

	if (gedit_mdi_child_get_closing (GEDIT_MDI_CHILD (child)))
	{
		return TRUE;
	}

	doc = GEDIT_MDI_CHILD (child)->document;

	if (gedit_document_get_modified (doc) || gedit_document_get_deleted (doc))
	{
		GtkWidget *view;
		GtkWindow *window;
		gboolean   save;
		GtkWidget *dlg;
		
		window = NULL;
		view = GTK_WIDGET (g_list_nth_data (bonobo_mdi_child_get_views (child), 0));
			
		if (view != NULL)
		{
			window = GTK_WINDOW (bonobo_mdi_get_window_from_view (view));
			gtk_window_present (window);
			
			bonobo_mdi_set_active_view (mdi, view);
		}

		dlg = gedit_close_confirmation_dialog_new_single (window, doc);
		
		save = gedit_close_confirmation_dialog_run (GEDIT_CLOSE_CONFIRMATION_DIALOG (dlg));
		
		gtk_widget_hide (dlg);

		if (save)
		{
			GSList *sel_docs;

			sel_docs = gedit_close_confirmation_dialog_get_selected_documents (
						GEDIT_CLOSE_CONFIRMATION_DIALOG (dlg));

			if (sel_docs == NULL)
			{
				close = TRUE;
			}
			else
			{
				close = gedit_file_save (GEDIT_MDI_CHILD (child), TRUE);

				g_slist_free (sel_docs);
			}
		}
		else
		{
			close = FALSE;
		}

		gtk_widget_destroy (dlg);

		gedit_debug (DEBUG_MDI, "CLOSE: %s", close ? "TRUE" : "FALSE");
	}
		
	return close;
}

static gboolean
gedit_mdi_remove_view_handler (BonoboMDI *mdi, GtkWidget *view)
{
	gedit_debug (DEBUG_MDI, "");

	return TRUE;
}

static gboolean
gedit_mdi_can_remove_views (GList *views, BonoboWindow *window)
{
	GList *l;
	GSList *unsaved_docs;
	GtkWidget *dlg;
	gboolean close;
	
	gedit_debug (DEBUG_MDI, "");

	if (window == NULL)
	{
		window = gedit_get_active_window ();
	}

	unsaved_docs = NULL;

	l = views;
	while (l != NULL)
	{
		GeditView *view;
		GeditDocument *doc;

		view = GEDIT_VIEW (l->data);
		
		doc = gedit_view_get_document (view);
		g_return_val_if_fail (doc != NULL, FALSE);

		if (gedit_document_get_modified (doc) ||
		    gedit_document_get_deleted (doc))
		{
			unsaved_docs = g_slist_prepend (unsaved_docs, doc);
		}

		l = g_list_next (l);
	}

	if (unsaved_docs == NULL)
	{
		l = views;
		while (l != NULL)
		{
			GeditView *view;
			BonoboMDIChild *child;
			
			view = GEDIT_VIEW (l->data);

			child =	bonobo_mdi_get_child_from_view (GTK_WIDGET (view));
			g_return_val_if_fail (child != NULL, FALSE);
			
			gedit_mdi_child_set_closing (GEDIT_MDI_CHILD (child),
						     TRUE);
						     
			l = g_list_next (l);
		}

		return TRUE;
	}

	unsaved_docs = g_slist_reverse (unsaved_docs);

	if (g_slist_length (unsaved_docs) == 1)
	{
		GtkWidget *view;
		GeditMDIChild *child;

		child = gedit_mdi_child_get_from_document (GEDIT_DOCUMENT (unsaved_docs->data));

		view = GTK_WIDGET (g_list_nth_data (
					bonobo_mdi_child_get_views (BONOBO_MDI_CHILD (child)), 0));
			
		if (view != NULL)
		{
			window = bonobo_mdi_get_window_from_view (view);
			gtk_window_present (GTK_WINDOW (window));
			
			bonobo_mdi_set_active_view (BONOBO_MDI (gedit_mdi), view);
		}
	}	
	
	dlg = gedit_close_confirmation_dialog_new (GTK_WINDOW (window), unsaved_docs);

	close = gedit_close_confirmation_dialog_run (GEDIT_CLOSE_CONFIRMATION_DIALOG (dlg));
		
	gtk_widget_hide (dlg);

	if (close)
	{
		GSList *sel_docs;

		sel_docs = gedit_close_confirmation_dialog_get_selected_documents
				(GEDIT_CLOSE_CONFIRMATION_DIALOG (dlg));

		if (sel_docs != NULL)
		{
			GSList *sl;

			sl = sel_docs;
			while (sl != NULL)
			{
				GeditMDIChild *child;
				GeditDocument *doc;

				doc = GEDIT_DOCUMENT (sl->data);
				child = gedit_mdi_child_get_from_document (doc);
				g_return_val_if_fail (child != NULL, FALSE);
				
				close &= gedit_file_save (child, FALSE);
				
				sl = g_slist_next (sl);
			}

			g_slist_free (sel_docs);
		}
	}

	gtk_widget_destroy (dlg);

	if (close)
	{
		l = views;
		while (l != NULL)
		{
			GeditView *view;
			BonoboMDIChild *child;
			
			view = GEDIT_VIEW (l->data);

			child =	bonobo_mdi_get_child_from_view (GTK_WIDGET (view));
			g_return_val_if_fail (child != NULL, FALSE);
			
			gedit_mdi_child_set_closing (GEDIT_MDI_CHILD (child),
						     TRUE);
						     
			l = g_list_next (l);
		}
	
	}

	g_slist_free (unsaved_docs);

	return close;
}

static gboolean
gedit_mdi_remove_views_handler (BonoboMDI *mdi, BonoboWindow *window)
{
	GList *views;
	gboolean ret;
	
	gedit_debug (DEBUG_MDI, "");

	if (gedit_mdi_get_state (GEDIT_MDI (mdi)) != GEDIT_STATE_NORMAL)
		return FALSE;

	views = bonobo_mdi_get_views_from_window (mdi, window);

	ret = gedit_mdi_can_remove_views (views, window);

	g_list_free (views);

	return ret;
}

gboolean 
gedit_mdi_remove_all (GeditMDI *mdi)
{
	GList *views;
	gboolean ret;
	
	gedit_debug (DEBUG_MDI, "");

	g_return_val_if_fail (GEDIT_IS_MDI (mdi), FALSE);

	views = bonobo_mdi_get_views (BONOBO_MDI (mdi));

	ret = gedit_mdi_can_remove_views (views, NULL);

	g_list_free (views);

	if (ret)
	{
		ret = bonobo_mdi_remove_all (BONOBO_MDI (mdi),
					     TRUE);
	}

	return ret;
}
	
#define MAX_URI_IN_TITLE_LENGTH 75

void 
gedit_mdi_set_active_window_title (BonoboMDI *mdi)
{
	BonoboMDIChild *active_child = NULL;
	GeditDocument *doc = NULL;
	gchar *docname = NULL;
	gchar *title = NULL;
	gchar *uri;
	GtkWidget *active_window;
	
	gedit_debug (DEBUG_MDI, "");

	active_child = bonobo_mdi_get_active_child (mdi);
	if (active_child == NULL)
		return;

	doc = GEDIT_MDI_CHILD (active_child)->document;
	g_return_if_fail (doc != NULL);
	
	/* Set active window title */
	uri = gedit_document_get_uri (doc);
	g_return_if_fail (uri != NULL);

	/* Truncate the URI so it doesn't get insanely wide. */
	docname = eel_str_middle_truncate (uri, MAX_URI_IN_TITLE_LENGTH);
	g_free (uri);

	if (gedit_document_get_modified (doc))
	{
		title = g_strdup_printf ("%s %s - gedit", docname, _("(modified)"));
	} 
	else 
	{
		if (gedit_document_is_readonly (doc)) 
		{
			title = g_strdup_printf ("%s %s - gedit", docname, _("(readonly)"));
		} 
		else 
		{
			title = g_strdup_printf ("%s - gedit", docname);
		}

	}

	active_window = GTK_WIDGET (gedit_get_active_window ());

	if (GTK_WIDGET_REALIZED (active_window))
	{	
		gchar *short_name;
		
		short_name = gedit_document_get_short_name (doc);
		
		gdk_window_set_icon_name (active_window->window, short_name);
		g_free (short_name);
	}

	gtk_window_set_title (GTK_WINDOW (active_window), title);
	
	g_free (docname);
	g_free (title);
}

static 
void gedit_mdi_child_changed_handler (BonoboMDI *mdi, BonoboMDIChild *old_child)
{
	gedit_debug (DEBUG_MDI, "");
		
	gedit_mdi_set_active_window_title (mdi);	
	gedit_mdi_update_languages_menu (mdi);
}

static 
void gedit_mdi_view_changed_handler (BonoboMDI *mdi, GtkWidget *old_view)
{
	BonoboWindow *win;
	GtkWidget *status;
	GtkWidget *active_view;
	
	gedit_debug (DEBUG_MDI, "");

	gedit_mdi_set_active_window_verbs_sensitivity (mdi);

	active_view = bonobo_mdi_get_active_view (mdi);
		
	win = bonobo_mdi_get_active_window (mdi);
	g_return_if_fail (win != NULL);

	if (old_view != NULL)
	{
		gedit_view_set_cursor_position_statusbar (GEDIT_VIEW (old_view), NULL);
		gedit_view_set_overwrite_mode_statusbar (GEDIT_VIEW (old_view), NULL);
	}

	if (active_view == NULL)
		return;

	/*
	gtk_widget_grab_focus (active_view);
	*/

	status = g_object_get_data (G_OBJECT (win), "CursorPosition");	
	gedit_view_set_cursor_position_statusbar (GEDIT_VIEW (active_view), status);

	status = g_object_get_data (G_OBJECT (win), "OverwriteMode");	
	gedit_view_set_overwrite_mode_statusbar (GEDIT_VIEW (active_view), status);
}

void 
gedit_mdi_clear_active_window_statusbar (GeditMDI *mdi)
{
	gpointer status;
	BonoboWindow *win;

	win = bonobo_mdi_get_active_window (BONOBO_MDI (mdi));
	if (win == NULL)
		return;

	status = g_object_get_data (G_OBJECT (win), "CursorPosition");	
	g_return_if_fail (status != NULL);
	g_return_if_fail (GTK_IS_STATUSBAR (status));

	/* clear any previous message, underflow is allowed */
	gtk_statusbar_pop (GTK_STATUSBAR (status), 0); 

	status = g_object_get_data (G_OBJECT (win), "OverwriteMode");	
	g_return_if_fail (status != NULL);
	g_return_if_fail (GTK_IS_STATUSBAR (status));

	/* clear any previous message, underflow is allowed */
	gtk_statusbar_pop (GTK_STATUSBAR (status), 0); 
}

void 
gedit_mdi_set_active_window_verbs_sensitivity (BonoboMDI *mdi)
{
	/* FIXME: it is too slooooooow! - Paolo */

	BonoboWindow* active_window = NULL;
	BonoboMDIChild* active_child = NULL;
	GeditDocument* doc = NULL;
	BonoboUIComponent *ui_component;
	
	gedit_debug (DEBUG_MDI, "");
	
	active_window = bonobo_mdi_get_active_window (mdi);

	if (active_window == NULL)
		return;
	
	ui_component = bonobo_mdi_get_ui_component_from_window (active_window);
	g_return_if_fail (ui_component != NULL);
	
	active_child = bonobo_mdi_get_active_child (mdi);
	
	bonobo_ui_component_freeze (ui_component, NULL);
	
	gedit_plugins_engine_update_plugins_ui (active_window, FALSE);
	
	gedit_menus_set_verb_list_sensitive (ui_component, 
				gedit_menus_all_sensible_verbs, TRUE);

	if (active_child == NULL)
	{
		gedit_menus_set_verb_list_sensitive (ui_component, 
				gedit_menus_no_docs_sensible_verbs, FALSE);

		bonobo_ui_component_set_prop (
			ui_component, "/menu/View/HighlightMode", "sensitive", "0", NULL);

		goto end;
	}

	if (gedit_prefs_manager_get_enable_syntax_highlighting ())
		bonobo_ui_component_set_prop (ui_component, "/menu/View/HighlightMode",
					      "sensitive", "1", NULL);
	else
		bonobo_ui_component_set_prop (ui_component, "/menu/View/HighlightMode",
					      "sensitive", "0", NULL);
	
	doc = GEDIT_MDI_CHILD (active_child)->document;
	g_return_if_fail (doc != NULL);
	
	if (!gedit_document_can_find_again (doc))
	{
		gedit_menus_set_verb_sensitive (ui_component, "/commands/SearchFindNext", FALSE);
		gedit_menus_set_verb_sensitive (ui_component, "/commands/SearchFindPrev", FALSE);
	}
		
	if (gedit_document_is_readonly (doc))
	{
		gedit_menus_set_verb_list_sensitive (ui_component, 
				gedit_menus_ro_sensible_verbs, FALSE);
		goto end;
	}

	if (!gedit_document_can_undo (doc))
		gedit_menus_set_verb_sensitive (ui_component, "/commands/EditUndo", FALSE);	

	if (!gedit_document_can_redo (doc))
		gedit_menus_set_verb_sensitive (ui_component, "/commands/EditRedo", FALSE);		

	if (!gedit_document_get_modified (doc))
	{
		gedit_menus_set_verb_list_sensitive (ui_component, 
				gedit_menus_not_modified_doc_sensible_verbs, FALSE);
	}

	if (gedit_document_is_untitled (doc))
	{
		gedit_menus_set_verb_list_sensitive (ui_component, 
				gedit_menus_untitled_doc_sensible_verbs, FALSE);
	}

end:
	switch (gedit_mdi_get_state (GEDIT_MDI (mdi)))
	{
		case GEDIT_STATE_LOADING:
		case GEDIT_STATE_SAVING:
		case GEDIT_STATE_REVERTING:
			gedit_menus_set_verb_list_sensitive (ui_component,
				gedit_menus_loading_sensible_verbs, FALSE);
			
			bonobo_ui_component_set_prop (ui_component, 
					"/menu/View/HighlightMode", "sensitive", "0", NULL);

			break;

		case GEDIT_STATE_PRINTING:
			gedit_menus_set_verb_list_sensitive (ui_component,
				gedit_menus_printing_sensible_verbs, FALSE);

			bonobo_ui_component_set_prop (ui_component, 
					"/menu/View/HighlightMode", "sensitive", "0", NULL);

			break;
		default:
			/* Do nothing */
			break;
	}

	gedit_menus_set_verb_sensitive (ui_component, "/commands/DocumentsMoveToNewWindow",
				(bonobo_mdi_n_views_for_window (active_window) > 1) ? TRUE : FALSE);

	bonobo_ui_component_thaw (ui_component, NULL);
}

static void 
gedit_mdi_set_active_window_undo_redo_verbs_sensitivity (BonoboMDI *mdi)
{
	BonoboWindow* active_window = NULL;
	GeditDocument* doc = NULL;
	BonoboUIComponent *ui_component;
	
	gedit_debug (DEBUG_MDI, "");
	
	active_window = gedit_get_active_window ();
	g_return_if_fail (active_window != NULL);
	
	ui_component = gedit_get_ui_component_from_window (active_window);
	g_return_if_fail (ui_component != NULL);
	
	doc = gedit_get_active_document ();
	g_return_if_fail (doc != NULL);

	bonobo_ui_component_freeze (ui_component, NULL);

	gedit_menus_set_verb_sensitive (ui_component, "/commands/EditUndo", 
			gedit_document_can_undo (doc));	

	gedit_menus_set_verb_sensitive (ui_component, "/commands/EditRedo", 
			gedit_document_can_redo (doc));	

	bonobo_ui_component_thaw (ui_component, NULL);
}

static void 
gedit_mdi_set_active_window_find_verbs_sensitivity (BonoboMDI *mdi)
{
	BonoboWindow* active_window = NULL;
	GeditDocument* doc = NULL;
	BonoboUIComponent *ui_component;
	gboolean sensitive;
	
	gedit_debug (DEBUG_MDI, "");
	
	active_window = gedit_get_active_window ();
	g_return_if_fail (active_window != NULL);
	
	ui_component = gedit_get_ui_component_from_window (active_window);
	g_return_if_fail (ui_component != NULL);
	
	doc = gedit_get_active_document ();
	g_return_if_fail (doc != NULL);

	sensitive = gedit_document_can_find_again (doc);

	bonobo_ui_component_freeze (ui_component, NULL);

	gedit_menus_set_verb_sensitive (ui_component, "/commands/SearchFindNext",
					sensitive);

	gedit_menus_set_verb_sensitive (ui_component, "/commands/SearchFindPrev",
					sensitive);

	bonobo_ui_component_thaw (ui_component, NULL);
}


EggRecentView *
gedit_mdi_get_recent_view_from_window (BonoboWindow *win)
{
	gpointer r;
	gedit_debug (DEBUG_MDI, "");

	r = g_object_get_data (G_OBJECT (win), RECENT_KEY);
	
	return (r != NULL) ? EGG_RECENT_VIEW (r) : NULL;
}

static void
gedit_mdi_close_output_window_cb (GtkWidget *widget, gpointer user_data)
{
	BonoboWindow *bw;
	
	bw = BONOBO_WINDOW (user_data);

	gtk_widget_hide (widget);
}

static void
gedit_mdi_show_output_window_cb (GtkWidget *widget, gpointer user_data)
{
	BonoboWindow *bw;
	GeditWindowPrefs *prefs;
	BonoboUIComponent *ui_component;
	
	gedit_debug (DEBUG_MDI, "");
	
	bw = BONOBO_WINDOW (user_data);
	g_return_if_fail (bw != NULL);
	
	ui_component = bonobo_mdi_get_ui_component_from_window (bw);
	g_return_if_fail (ui_component != NULL);
	
	prefs = gedit_window_prefs_get_from_window (bw);
	g_return_if_fail (prefs != NULL);
	
	prefs->output_window_visible = TRUE;
	
	gedit_menus_set_verb_state (ui_component, 
				    "/commands/ViewOutputWindow",
				    prefs->output_window_visible);
}

static void
gedit_mdi_hide_output_window_cb (GtkWidget *widget, gpointer user_data)
{
	BonoboWindow *bw;
	GeditWindowPrefs *prefs;
	BonoboUIComponent *ui_component;
	
	gedit_debug (DEBUG_MDI, "");
	
	bw = BONOBO_WINDOW (user_data);
	g_return_if_fail (bw != NULL);
	
	ui_component = bonobo_mdi_get_ui_component_from_window (bw);
	g_return_if_fail (ui_component != NULL);

	prefs = gedit_window_prefs_get_from_window (bw);
	g_return_if_fail (prefs != NULL);

	prefs->output_window_visible = FALSE;

	gedit_menus_set_verb_state (ui_component, 
				    "/commands/ViewOutputWindow",
				    prefs->output_window_visible);
}

GtkWidget *
gedit_mdi_get_output_window_from_window (BonoboWindow *win)
{
	gpointer r;
	gedit_debug (DEBUG_MDI, "");

	r = g_object_get_data (G_OBJECT (win), OUTPUT_WINDOW_KEY);

	if (r == NULL)
	{
		GtkWidget *ow;

		/* Add output window */
		ow = gedit_output_window_new ();
		bonobo_mdi_set_bottom_pane_for_window (BONOBO_WINDOW (win), ow);
		gtk_widget_show_all (ow);
		g_signal_connect (G_OBJECT (ow), "close_requested",
			  G_CALLBACK (gedit_mdi_close_output_window_cb), win);
		g_signal_connect (G_OBJECT (ow), "show",
			  G_CALLBACK (gedit_mdi_show_output_window_cb), win);
		g_signal_connect (G_OBJECT (ow), "hide",
			  G_CALLBACK (gedit_mdi_hide_output_window_cb), win);
		g_object_set_data (G_OBJECT (win), OUTPUT_WINDOW_KEY, ow);

		return ow;
	}
	
	return (r != NULL) ? GTK_WIDGET (r) : NULL;
}

static GeditWindowPrefs *
gedit_window_prefs_new (void)
{
	GeditWindowPrefs *prefs;

	gedit_debug (DEBUG_MDI, "");

	prefs = g_new0 (GeditWindowPrefs, 1);

	prefs->toolbar_visible = gedit_prefs_manager_get_toolbar_visible ();
	prefs->toolbar_buttons_style = gedit_prefs_manager_get_toolbar_buttons_style ();

	prefs->statusbar_visible = gedit_prefs_manager_get_statusbar_visible ();

	prefs->output_window_visible = FALSE;

	return prefs;
}

static void
gedit_window_prefs_attach_to_window (GeditWindowPrefs *prefs, BonoboWindow *win)
{
	gedit_debug (DEBUG_MDI, "");

	g_return_if_fail (prefs != NULL);
	g_return_if_fail (win != NULL);
	g_return_if_fail (BONOBO_IS_WINDOW (win));

	if (!window_prefs_id)
		window_prefs_id = g_quark_from_static_string ("GeditWindowPrefsData");

	g_object_set_qdata_full (G_OBJECT (win), window_prefs_id, prefs, g_free);
}

static GeditWindowPrefs	*
gedit_window_prefs_get_from_window (BonoboWindow *win)
{
	GeditWindowPrefs *prefs;

	gedit_debug (DEBUG_MDI, "");

	g_return_val_if_fail (win != NULL, NULL);
	g_return_val_if_fail (BONOBO_IS_WINDOW (win), NULL);

	prefs = g_object_get_qdata (G_OBJECT (win), window_prefs_id);

	return (prefs != NULL) ? (GeditWindowPrefs*)prefs : NULL;
}

static void
gedit_window_prefs_save (GeditWindowPrefs *prefs)
{
	gedit_debug (DEBUG_MDI, "");

	g_return_if_fail (prefs != NULL);

	if ((prefs->toolbar_visible != gedit_prefs_manager_get_toolbar_visible ()) &&
	     gedit_prefs_manager_toolbar_visible_can_set ())
		gedit_prefs_manager_set_toolbar_visible (prefs->toolbar_visible);

	if ((prefs->toolbar_buttons_style != gedit_prefs_manager_get_toolbar_buttons_style ()) &&
	    gedit_prefs_manager_toolbar_buttons_style_can_set ())
		gedit_prefs_manager_set_toolbar_buttons_style (prefs->toolbar_buttons_style);

	if ((prefs->statusbar_visible != gedit_prefs_manager_get_statusbar_visible ()) &&
	    gedit_prefs_manager_statusbar_visible_can_set ())
		gedit_prefs_manager_set_statusbar_visible (prefs->statusbar_visible);

}

static gchar* 
escape_underscores (const gchar* text)
{
	GString *str;
	gint length;
	const gchar *p;
 	const gchar *end;

  	g_return_val_if_fail (text != NULL, NULL);

    	length = strlen (text);

	str = g_string_new ("");

  	p = text;
  	end = text + length;

  	while (p != end)
    	{
      		const gchar *next;
      		next = g_utf8_next_char (p);

		switch (*p)
        	{
       			case '_':
          			g_string_append (str, "__");
          			break;
        		default:
          			g_string_append_len (str, p, next - p);
          			break;
        	}

      		p = next;
    	}

	return g_string_free (str, FALSE);
}

static gchar* 
replace_spaces_with_underscores (const gchar* text)
{
	GString *str;
	gint length;
	const gchar *p;
 	const gchar *end;

  	g_return_val_if_fail (text != NULL, NULL);

    	length = strlen (text);

	str = g_string_new ("");

  	p = text;
  	end = text + length;

  	while (p != end)
    	{
      		const gchar *next;
      		next = g_utf8_next_char (p);

		switch (*p)
        	{
       			case ' ':
			case '\t':
			case '+':
          			g_string_append (str, "_");
          			break;
        		default:
          			g_string_append_len (str, p, next - p);
          			break;
        	}

      		p = next;
    	}

	return g_string_free (str, FALSE);
}

static void
slist_deep_free (GSList *list)
{
	g_slist_foreach (list, (GFunc) g_free, NULL);
	g_slist_free (list);
}
	
static gboolean
section_exists (GSList      *list,
		const gchar *section)
{
	while (list != NULL)
	{
      		if (strcmp (list->data, section) == 0)
			return TRUE;

		list = g_slist_next (list);
    	}

  	return FALSE;
}

static gchar *
get_base_path_for_language (GtkSourceLanguage *lang)
{
	gchar *safe_name;
	gchar *section;
	gchar *name;
	gchar *path;

	section = gtk_source_language_get_section (lang);
	
	safe_name = g_markup_escape_text (section, strlen (section));
	name = replace_spaces_with_underscores (safe_name);
		
	path = g_strdup_printf ("/menu/View/HighlightMode/%s/", name);

	g_free (safe_name);
	g_free (name);
	g_free (section);

	return path;
}
	
static void
language_toggled_handler (
			BonoboUIComponent           *ui_component,
			const char                  *path,
			Bonobo_UIComponent_EventType type,
			const char                  *state,
			GtkSourceLanguage           *lang)
{
	GeditDocument *doc;
	gboolean s;

       	s = (strcmp (state, "1") == 0);
	
	if (!s)
		return;
	
	doc = gedit_get_active_document ();
	g_return_if_fail (doc != NULL);

	gedit_document_set_language (doc, lang);
}


static gchar *
get_verb_name_for_language (GtkSourceLanguage *lang)
{
	gchar *lang_name;
	gchar *safe_name;
	gchar *name;
	gchar *verb_name;

	if (lang == NULL)
		return g_strdup ("/commands/gedit_lang__Normal");

	lang_name = gtk_source_language_get_name (lang);
	safe_name = g_markup_escape_text (lang_name, strlen (lang_name));
	
	name = replace_spaces_with_underscores (safe_name);

	verb_name = g_strdup_printf ("/commands/gedit_lang__%s", name);
		
	g_free (safe_name);
	g_free (lang_name);
	g_free (name);

	return verb_name;
}

static void
add_languages_menu (BonoboMDI *mdi, BonoboWindow *win)
{
	const GSList *languages;
	GSList *sections = NULL;
	const GSList *l;
	GSList *s;

	gedit_debug (DEBUG_MDI, "");

	gedit_menus_add_menu_item_radio (win,
					 "/menu/View/HighlightMode/",
					 "gedit_lang__Normal",
					 "Langs",
					 _("Normal"),
					 _("Use Normal highlight mode"),
					 (BonoboUIListenerFn)language_toggled_handler,
					 NULL);
					  
	languages = gtk_source_languages_manager_get_available_languages (
						gedit_get_languages_manager ());

	l = languages;

	while (l != NULL)
	{
		gchar *section;
				
		section = gtk_source_language_get_section (GTK_SOURCE_LANGUAGE (l->data));

		if (!section_exists (sections, section))
			sections = g_slist_prepend (sections, section);
		else
			g_free (section);

		l = g_slist_next (l);
	}

	/*
	sections = g_slist_sort (sections, (GCompareFunc)strcmp);
	*/
	sections = g_slist_reverse (sections);

	s = sections;

	while (s != NULL)
	{
		gchar *safe_name;
		gchar *label;
		gchar *name;

		safe_name = g_markup_escape_text (s->data, strlen (s->data));
		label = escape_underscores (safe_name);
		name = replace_spaces_with_underscores (safe_name);
		
		gedit_menus_add_submenu (win, 
					 "/menu/View/HighlightMode/",
					 name,
					 label);
	
		g_free (safe_name);
		g_free (label);
		g_free (name);
		
		s = g_slist_next (s);
	}

	slist_deep_free (sections);

	l = languages;
	
	while (l != NULL)
	{
		gchar *path;
		
		gchar *lang_name;
		gchar *safe_name;
		gchar *label;
		gchar *name;
		gchar *tip;
		gchar *tmp;

		path = get_base_path_for_language (l->data);

		lang_name = gtk_source_language_get_name (l->data);
		safe_name = g_markup_escape_text (lang_name, strlen (lang_name));
		label = escape_underscores (safe_name);
		tmp = replace_spaces_with_underscores (safe_name);
		name = g_strdup_printf ("gedit_lang__%s", tmp);
		g_free (tmp);
		
		tip = g_strdup_printf (N_("Use %s highlight mode"), safe_name);

		gedit_menus_add_menu_item_radio (win,
					  path,
					  name,
					  "Langs",
					  label,
					  tip,
					  (BonoboUIListenerFn)language_toggled_handler,
					  l->data);

		g_free (safe_name);
		g_free (label);
		g_free (name);
		g_free (tip);
		g_free (lang_name);
		g_free (path);

		l = g_slist_next (l);
	}		
}

static 
void gedit_mdi_update_languages_menu (BonoboMDI *mdi)
{
	GtkSourceLanguage *lang;
	BonoboMDIChild *active_child;
	GeditDocument *doc;
	gchar *verb_name;
	BonoboWindow* active_window = NULL;
	BonoboUIComponent *ui_component;
	
	gedit_debug (DEBUG_MDI, "");
		
	active_window = bonobo_mdi_get_active_window (mdi);

	if (active_window == NULL)
		return;
	
	ui_component = bonobo_mdi_get_ui_component_from_window (active_window);
	g_return_if_fail (ui_component != NULL);

	active_child = bonobo_mdi_get_active_child (mdi);
	if (active_child == NULL)
		return;

	doc = GEDIT_MDI_CHILD (active_child)->document;
	g_return_if_fail (doc != NULL);

	lang = gedit_document_get_language (doc);
	verb_name = get_verb_name_for_language (lang);
	
	gedit_menus_set_verb_state (ui_component, verb_name, TRUE);

	g_free (verb_name);
}

static void
update_ui_according_to_state (GeditMDI *mdi)
{
	GList *views;
	GdkCursor *cursor;
	GList *windows;

	gedit_debug (DEBUG_MDI, "");
	
	/* Upate menus and toolbars */
	gedit_mdi_set_active_window_verbs_sensitivity (BONOBO_MDI (gedit_mdi));

	/* Update views editability */
	views = bonobo_mdi_get_views (BONOBO_MDI (mdi));
	while (views != NULL)
	{
		GeditView *view;
		GeditDocument *doc;

		view = GEDIT_VIEW (views->data);
		doc = gedit_view_get_document (view);
		
		switch (gedit_mdi_get_state (mdi))
		{
			case GEDIT_STATE_NORMAL:
				if (!gedit_document_is_readonly (doc))
				{
					gedit_view_set_editable (view, TRUE);
				}

				break;

			case GEDIT_STATE_LOADING:
			case GEDIT_STATE_PRINTING:
			case GEDIT_STATE_SAVING:
			case GEDIT_STATE_REVERTING:
				gedit_view_set_editable (view, FALSE);
				break;
		}

		views = g_list_next (views);
	}

	g_list_free (views);

	/* Change cursor */
	if (gedit_mdi_get_state (mdi) == GEDIT_STATE_NORMAL)
	{
		cursor = NULL;
	}
	else
	{
		cursor =  gdk_cursor_new_for_display (
				gtk_widget_get_display (GTK_WIDGET (gedit_get_active_window ())),
				GDK_WATCH);
	}

	windows = bonobo_mdi_get_windows (BONOBO_MDI (mdi));
	while (windows != NULL)
	{
		GtkWidget *win;
		gpointer recent_menu_button;

		g_return_if_fail (windows->data != NULL);
		
		win = GTK_WIDGET (windows->data);
		
		gdk_window_set_cursor (win->window, cursor);

		recent_menu_button = g_object_get_data (G_OBJECT (win), "recent-menu-button");

		if (recent_menu_button != NULL)
		{
			gtk_widget_set_sensitive (GTK_WIDGET (recent_menu_button),
						  gedit_mdi_get_state (mdi) == GEDIT_STATE_NORMAL);
		}

		windows = g_list_next (windows);
	}

	if (cursor != NULL)
		gdk_cursor_unref (cursor);


	/* FIXME: Disable the recent items in the file menu - Paolo (Feb 24, 2004) */

	/* TODO: Update/Add state icon - Paolo (Jan 13, 2004) */
}

GeditState
gedit_mdi_get_state (GeditMDI *mdi)
{
	gedit_debug (DEBUG_MDI, "");

	g_return_val_if_fail (GEDIT_IS_MDI (mdi), GEDIT_STATE_NORMAL);

	return mdi->priv->state;
}

void
gedit_mdi_set_state (GeditMDI   *mdi,
		     GeditState  state)
{
	gedit_debug (DEBUG_MDI, "");

	g_return_if_fail (GEDIT_IS_MDI (mdi));
	g_return_if_fail ((state == GEDIT_STATE_NORMAL) ||
			  (state == GEDIT_STATE_LOADING) ||
			  (state == GEDIT_STATE_SAVING) ||
			  (state == GEDIT_STATE_PRINTING) ||
			  (state == GEDIT_STATE_REVERTING));

	if (state != mdi->priv->state)
	{
		mdi->priv->state = state;
		
		update_ui_according_to_state (mdi);

		g_signal_emit (G_OBJECT (mdi), 
			       mdi_signals [STATE_CHANGED],
			       0,
			       state);
	}
}

