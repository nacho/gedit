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

#include <string.h>

#include "gedit-mdi.h"
#include "gedit-mdi-child.h"
#include "gedit2.h"
#include "gedit-menus.h"
#include "gedit-debug.h"
#include "gedit-prefs.h"
#include "gedit-recent.h" 
#include "gedit-file.h"
#include "gedit-view.h"
#include "gedit-utils.h"
#include "gedit-plugins-engine.h"

#include <bonobo/bonobo-ui-util.h>
#include <bonobo/bonobo-control.h>

#include <gconf/gconf-client.h>

struct _GeditMDIPrivate
{
	gint untitled_number;
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
static gint gedit_mdi_remove_view_handler (BonoboMDI *mdi, GtkWidget *view);

static void gedit_mdi_view_changed_handler (BonoboMDI *mdi, GtkWidget *old_view);
static void gedit_mdi_child_changed_handler (BonoboMDI *mdi, BonoboMDIChild *old_child);
static void gedit_mdi_child_state_changed_handler (GeditMDIChild *child);

static void gedit_mdi_set_active_window_undo_redo_verbs_sensitivity (BonoboMDI *mdi);

static void gedit_mdi_view_menu_item_toggled_handler (
			BonoboUIComponent           *ui_component,
			const char                  *path,
			Bonobo_UIComponent_EventType type,
			const char                  *state,
			BonoboWindow                *win);

static BonoboMDIClass *parent_class = NULL;

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
}

static void 
gedit_mdi_init (GeditMDI  *mdi)
{
	gedit_debug (DEBUG_MDI, "START");

	bonobo_mdi_construct (BONOBO_MDI (mdi), "gedit-2", "gedit", 
			      gedit_settings->mdi_tabs_position);
	
	mdi->priv = g_new0 (GeditMDIPrivate, 1);

	mdi->priv->untitled_number = 0;	

	bonobo_mdi_set_ui_template_file (BONOBO_MDI (mdi), GEDIT_UI_DIR "gedit-ui.xml", gedit_verbs);
	
	bonobo_mdi_set_child_list_path (BONOBO_MDI (mdi), "/menu/Documents/");

	bonobo_mdi_set_mode (BONOBO_MDI (mdi), gedit_settings->mdi_mode);

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

	g_signal_connect (G_OBJECT (mdi), "child_changed",
			  G_CALLBACK (gedit_mdi_child_changed_handler), NULL);
	g_signal_connect (G_OBJECT (mdi), "view_changed",
			  G_CALLBACK (gedit_mdi_view_changed_handler), NULL);

	g_signal_connect (G_OBJECT (mdi), "destroy",
			  G_CALLBACK (gedit_file_exit), NULL);

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

	G_OBJECT_CLASS (parent_class)->finalize (object);

	g_free (mdi->priv);
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

static void
gedit_mdi_app_created_handler (BonoboMDI *mdi, BonoboWindow *win)
{
	GtkWidget *widget;
	BonoboControl *control;
	BonoboUIComponent *ui_component;

	static GtkTargetEntry drag_types[] =
	{
		{ "text/uri-list", 0, 0 },
	};

	static gint n_drag_types = sizeof (drag_types) / sizeof (drag_types [0]);

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
	
	gtk_widget_set_size_request (widget, 150, 10);	
	gtk_statusbar_set_has_resize_grip (GTK_STATUSBAR (widget), FALSE);
	
	bonobo_ui_component_object_set (ui_component,
		       			"/status/CursorPosition",
					BONOBO_OBJREF (control),
					NULL);

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

	bonobo_object_unref (BONOBO_OBJECT (control));

	g_object_set_data (G_OBJECT (win), "OverwriteMode", widget);

	/* Set the statusbar style according to prefs */
	gedit_mdi_set_app_statusbar_style (win);
	
	/* Set the toolbar style according to prefs */
	gedit_mdi_set_app_toolbar_style (win);
		
	/* Add listener fo the view menu */
	bonobo_ui_component_add_listener (ui_component, "ViewToolbar", 
			(BonoboUIListenerFn)gedit_mdi_view_menu_item_toggled_handler, 
			(gpointer)win);
	bonobo_ui_component_add_listener (ui_component, "ViewStatusbar", 
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
	bonobo_ui_component_add_listener (ui_component, "ToolbarTooltips", 
			(BonoboUIListenerFn)gedit_mdi_view_menu_item_toggled_handler, 
			(gpointer)win);

	bonobo_ui_component_add_listener (ui_component, "StatusBarCursorPosition", 
			(BonoboUIListenerFn)gedit_mdi_view_menu_item_toggled_handler, 
			(gpointer)win);
	bonobo_ui_component_add_listener (ui_component, "StatusBarOverwriteMode", 
			(BonoboUIListenerFn)gedit_mdi_view_menu_item_toggled_handler, 
			(gpointer)win);


	/* Set the window prefs. */
	gtk_window_set_default_size (GTK_WINDOW (win), 
			gedit_settings->window_width, 
			gedit_settings->window_height);

	g_signal_connect (G_OBJECT (win), "configure_event",
	                  G_CALLBACK (gedit_prefs_configure_event_handler), 
			  NULL);
	
	/* Add the recent files */
	gedit_recent_init (win);

	/* Add the plugins menus */
	gedit_plugins_engine_update_plugins_ui (win, TRUE);
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

	gedit_debug (DEBUG_MDI, "%s toggled to '%s'", path, state);

	s = (strcmp (state, "1") == 0);

	if ((strcmp (path, "ViewToolbar") == 0) &&
	    (s != gedit_settings->toolbar_visible))
	{
		gedit_settings->toolbar_visible = s;
		gedit_mdi_set_app_toolbar_style (win);

		return;
	}

	if ((strcmp (path, "ViewStatusbar") == 0) &&
	    (s != gedit_settings->statusbar_visible))
	{
		gedit_settings->statusbar_visible = s;
		gedit_mdi_set_app_statusbar_style (win);
	}

	if (s && (strcmp (path, "ToolbarSystem") == 0) &&
	    (gedit_settings->toolbar_buttons_style != GEDIT_TOOLBAR_SYSTEM))
	{		
		gedit_settings->toolbar_buttons_style = GEDIT_TOOLBAR_SYSTEM;
		gedit_mdi_set_app_toolbar_style (win);

		return;
	}

	if (s && (strcmp (path, "ToolbarIcon") == 0) &&
	    (gedit_settings->toolbar_buttons_style != GEDIT_TOOLBAR_ICONS))
	{		
		gedit_settings->toolbar_buttons_style = GEDIT_TOOLBAR_ICONS;
		gedit_mdi_set_app_toolbar_style (win);

		return;
	}

	if (s && (strcmp (path, "ToolbarIconText") == 0) &&
	    (gedit_settings->toolbar_buttons_style != GEDIT_TOOLBAR_ICONS_AND_TEXT))
	{		
		gedit_settings->toolbar_buttons_style = GEDIT_TOOLBAR_ICONS_AND_TEXT;
		gedit_mdi_set_app_toolbar_style (win);

		return;
	}

	if ((strcmp (path, "ToolbarTooltips") == 0) &&
	    (s != gedit_settings->toolbar_view_tooltips))
	{
		gedit_settings->toolbar_view_tooltips = s;
		gedit_mdi_set_app_toolbar_style (win);

		return;
	}

	if ((strcmp (path, "StatusBarCursorPosition") == 0) &&
	    (s != gedit_settings->statusbar_view_cursor_position))
	{
		gedit_settings->statusbar_view_cursor_position = s;
		gedit_mdi_set_app_statusbar_style (win);
	}

	if ((strcmp (path, "StatusBarOverwriteMode") == 0) &&
	    (s != gedit_settings->statusbar_view_overwrite_mode))
	{
		gedit_settings->statusbar_view_overwrite_mode = s;
		gedit_mdi_set_app_statusbar_style (win);
	}
}

static void 
gedit_mdi_drag_data_received_handler (GtkWidget *widget, GdkDragContext *context, 
		                      gint x, gint y, GtkSelectionData *selection_data, 
				      guint info, guint time)
{
	GList *list = NULL;
	GList *file_list = NULL;
	GList *p = NULL;
	
	gedit_debug (DEBUG_MDI, "");

	list = gnome_vfs_uri_list_parse (selection_data->data);
	p = list;

	while (p != NULL)
	{
		file_list = g_list_append (file_list, 
				gnome_vfs_uri_to_string ((const GnomeVFSURI*)(p->data), 
				GNOME_VFS_URI_HIDE_NONE));
		p = p->next;
	}
	
	gnome_vfs_uri_list_free (list);

	gedit_file_open_uri_list (file_list, 0);	
	
	if (file_list == NULL)
		return;

	for (p = file_list; p != NULL; p = p->next) {
		g_free (p->data);
	}
	
	g_list_free (file_list);
}

static void
gedit_mdi_set_app_toolbar_style (BonoboWindow *win)
{
	BonoboUIComponent *ui_component;
	GConfClient *client;
	gboolean labels;

	gedit_debug (DEBUG_MDI, "");
	
	g_return_if_fail (BONOBO_IS_WINDOW (win));
			
	ui_component = bonobo_mdi_get_ui_component_from_window (win);
	g_return_if_fail (ui_component != NULL);
			
	bonobo_ui_component_freeze (ui_component, NULL);

	/* Updated view menu */
	gedit_menus_set_verb_state (ui_component, 
				    "/commands/ViewToolbar",
				    gedit_settings->toolbar_visible);

	gedit_menus_set_verb_sensitive (ui_component, 
				        "/commands/ToolbarSystem",
				        gedit_settings->toolbar_visible);
	gedit_menus_set_verb_sensitive (ui_component, 
				        "/commands/ToolbarIcon",
				        gedit_settings->toolbar_visible);
	gedit_menus_set_verb_sensitive (ui_component, 
				        "/commands/ToolbarIconText",
				        gedit_settings->toolbar_visible);
	gedit_menus_set_verb_sensitive (ui_component, 
				        "/commands/ToolbarTooltips",
				        gedit_settings->toolbar_visible);

	gedit_menus_set_verb_state (ui_component, 
				    "/commands/ToolbarSystem",
				    gedit_settings->toolbar_buttons_style == GEDIT_TOOLBAR_SYSTEM);

	gedit_menus_set_verb_state (ui_component, 
				    "/commands/ToolbarIcon",
				    gedit_settings->toolbar_buttons_style == GEDIT_TOOLBAR_ICONS);

	gedit_menus_set_verb_state (ui_component, 
				    "/commands/ToolbarIconText",
				    gedit_settings->toolbar_buttons_style == GEDIT_TOOLBAR_ICONS_AND_TEXT);

	gedit_menus_set_verb_state (ui_component, 
				    "/commands/ToolbarTooltips",
				    gedit_settings->toolbar_view_tooltips);

	
	/* Actually update toolbar style */
	bonobo_ui_component_set_prop (
		ui_component, "/Toolbar",
		"tips", gedit_settings->toolbar_view_tooltips ? "1" : "0",
		NULL);
	
	switch (gedit_settings->toolbar_buttons_style)
	{
		case GEDIT_TOOLBAR_SYSTEM:
						
			client = gconf_client_get_default ();
			if (client == NULL) 
				goto error;

			labels = gconf_client_get_bool (client, 
					"/desktop/gnome/interface/toolbar-labels", NULL);

			g_object_unref (G_OBJECT (client));
			
			if (labels)
			{			
				gedit_debug (DEBUG_MDI, "SYSTEM: BOTH");
				bonobo_ui_component_set_prop (
					ui_component, "/Toolbar", "look", "both", NULL);
			
			}
			else
			{
				gedit_debug (DEBUG_MDI, "SYSTEM: ICONS");
				bonobo_ui_component_set_prop (
					ui_component, "/Toolbar", "look", "icons", NULL);
			}
	
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
		default:
			goto error;
			break;
	}
	
	bonobo_ui_component_set_prop (
			ui_component, "/Toolbar",
			"hidden", gedit_settings->toolbar_visible ? "0":"1", NULL);

 error:
	bonobo_ui_component_thaw (ui_component, NULL);
}

static void
gedit_mdi_set_app_statusbar_style (BonoboWindow *win)
{
	BonoboUIComponent *ui_component;
	GtkWidget *cp, *om;
	
	gedit_debug (DEBUG_MDI, "");
	
	g_return_if_fail (BONOBO_IS_WINDOW (win));
			
	ui_component = bonobo_mdi_get_ui_component_from_window (win);
	g_return_if_fail (ui_component != NULL);

	bonobo_ui_component_freeze (ui_component, NULL);

	/* Update menu */
	gedit_menus_set_verb_state (ui_component, 
				    "/commands/ViewStatusbar",
				    gedit_settings->statusbar_visible);

	gedit_menus_set_verb_sensitive (ui_component, 
				    "/commands/StatusBarCursorPosition",
				    gedit_settings->statusbar_visible);

	gedit_menus_set_verb_sensitive (ui_component, 
				    "/commands/StatusBarOverwriteMode",
				    gedit_settings->statusbar_visible);

	gedit_menus_set_verb_state (ui_component, 
				    "/commands/StatusBarCursorPosition",
				    gedit_settings->statusbar_view_cursor_position);

	gedit_menus_set_verb_state (ui_component, 
				    "/commands/StatusBarOverwriteMode",
				    gedit_settings->statusbar_view_overwrite_mode);

	
	/* Actually update status bar style */
	bonobo_ui_component_set_prop (
		ui_component, "/status",
		"hidden", gedit_settings->statusbar_visible ? "0" : "1",
		NULL);

	cp = GTK_WIDGET (g_object_get_data (G_OBJECT (win), "CursorPosition"));
	if (cp == NULL)
		goto error;

	if (gedit_settings->statusbar_view_cursor_position)
		gtk_widget_show (cp);
	else
		gtk_widget_hide (cp);

	om = GTK_WIDGET (g_object_get_data (G_OBJECT (win), "OverwriteMode"));
	if (om == NULL)
		goto error;

	if (gedit_settings->statusbar_view_overwrite_mode)
		gtk_widget_show (om);
	else
		gtk_widget_hide (om);

	if (!gedit_settings->statusbar_view_overwrite_mode)
		gtk_statusbar_set_has_resize_grip (GTK_STATUSBAR (cp), TRUE);
	else
		gtk_statusbar_set_has_resize_grip (GTK_STATUSBAR (cp), FALSE);

	if (!gedit_settings->statusbar_view_cursor_position &&
	    !gedit_settings->statusbar_view_overwrite_mode)
		bonobo_ui_component_set_prop (
			ui_component, "/status/item", "resize_grip", "1", NULL);
	else
		bonobo_ui_component_set_prop (
			ui_component, "/status/item", "resize_grip", "0", NULL);

error:
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
}

static void 
gedit_mdi_child_undo_redo_state_changed_handler (GeditMDIChild *child)
{
	gedit_debug (DEBUG_MDI, "");

	if (bonobo_mdi_get_active_child (BONOBO_MDI (gedit_mdi)) != BONOBO_MDI_CHILD (child))
		return;
	
	gedit_mdi_set_active_window_undo_redo_verbs_sensitivity (BONOBO_MDI (gedit_mdi));
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

	return TRUE;
}

static gint 
gedit_mdi_add_view_handler (BonoboMDI *mdi, GtkWidget *view)
{
	static GtkTargetEntry drag_types[] =
	{
		{ "text/uri-list", 0, 0 },
	};

	static gint n_drag_types = sizeof (drag_types) / sizeof (drag_types [0]);

	gedit_debug (DEBUG_MDI, "");
	
	/* FIXME */
	/* Drag and drop support */
	gtk_drag_dest_set (view,
			   GTK_DEST_DEFAULT_MOTION |
			   GTK_DEST_DEFAULT_HIGHLIGHT |
			   GTK_DEST_DEFAULT_DROP,
			   drag_types, n_drag_types,
			   GDK_ACTION_COPY);
		
	g_signal_connect (GTK_OBJECT (view), "drag_data_received",
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

	doc = GEDIT_MDI_CHILD (child)->document;

	if (gedit_document_get_modified (doc))
	{
		GtkWidget *msgbox, *w;
		gchar *fname = NULL, *msg = NULL;
		gint ret;
		gboolean exiting;

		w = GTK_WIDGET (g_list_nth_data (bonobo_mdi_child_get_views (child), 0));
			
		if(w != NULL)
			bonobo_mdi_set_active_view (mdi, w);

		fname = gedit_document_get_short_name (doc);

		msgbox = gtk_message_dialog_new (GTK_WINDOW (bonobo_mdi_get_active_window (mdi)),
				GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_MESSAGE_QUESTION,
				GTK_BUTTONS_NONE,
				_("Do you want to save the changes you made to the document \"%s\"? \n\n"
				  "Your changes will be lost if you don't save them."),
				fname);

		gedit_dialog_add_button (GTK_DIALOG (msgbox),
				_("Do_n't save"), GTK_STOCK_NO,
				GTK_RESPONSE_NO);

		if (gedit_close_x_button_pressed)
			exiting = FALSE;
		else if (gedit_exit_button_pressed)
			exiting = TRUE;
		else
		{
			/* Delete event generated */
			if (g_list_length (bonobo_mdi_get_windows (BONOBO_MDI (gedit_mdi))) == 1)
				exiting = TRUE;
			else
				exiting = FALSE;
		}

#if 0		
		if (exiting)
			gedit_dialog_add_button (GTK_DIALOG (msgbox),
					_("_Don't quit"), GTK_STOCK_CANCEL,
                	             	GTK_RESPONSE_CANCEL);
		else
			gedit_dialog_add_button (GTK_DIALOG (msgbox),
					_("_Don't close"), GTK_STOCK_CANCEL,
                	             	GTK_RESPONSE_CANCEL);
#endif
		
		gtk_dialog_add_button (GTK_DIALOG (msgbox), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);

		gtk_dialog_add_button (GTK_DIALOG (msgbox),
			       	GTK_STOCK_SAVE,
				GTK_RESPONSE_YES);

		gtk_dialog_set_default_response	(GTK_DIALOG (msgbox), GTK_RESPONSE_YES);

		gtk_window_set_resizable (GTK_WINDOW (msgbox), FALSE);

		ret = gtk_dialog_run (GTK_DIALOG (msgbox));
		
		gtk_widget_destroy (msgbox);

		g_free (fname);
		g_free (msg);
		
		switch (ret)
		{
			case GTK_RESPONSE_YES:
				close = gedit_file_save (GEDIT_MDI_CHILD (child));
				break;
			case GTK_RESPONSE_NO:
				close = TRUE;
				break;
			default:
				close = FALSE;
		}

		gedit_debug (DEBUG_MDI, "CLOSE: %s", close ? "TRUE" : "FALSE");
	}
	
	/* FIXME: there is a bug if you "close all" >1 docs, don't save the document
	 * and then don't close the last one.
	 */
	/* Disable to avoid the bug */
	/*
	if (close)
	{
		gtk_signal_disconnect_by_func (GTK_OBJECT (child), 
				       G_CALLBACK (gedit_mdi_child_state_changed_handler),
				       NULL);
		gtk_signal_disconnect_by_func (GTK_OBJECT (child), 
				       G_CALLBACK (gedit_mdi_child_undo_redo_state_changed_handler),
				       NULL);
	}
	*/
	
	return close;
}

static gint 
gedit_mdi_remove_view_handler (BonoboMDI *mdi,  GtkWidget *view)
{
	gedit_debug (DEBUG_MDI, "");

	return TRUE;
}

void 
gedit_mdi_set_active_window_title (BonoboMDI *mdi)
{
	BonoboMDIChild* active_child = NULL;
	GeditDocument* doc = NULL;
	gchar* docname = NULL;
	gchar* title = NULL;
	
	gedit_debug (DEBUG_MDI, "");

	
	active_child = bonobo_mdi_get_active_child (mdi);
	if (active_child == NULL)
		return;

	doc = GEDIT_MDI_CHILD (active_child)->document;
	g_return_if_fail (doc != NULL);
	
	/* Set active window title */
	docname = gedit_document_get_uri (doc);
	g_return_if_fail (docname != NULL);

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

	gtk_window_set_title (GTK_WINDOW (bonobo_mdi_get_active_window (mdi)), title);
	
	g_free (docname);
	g_free (title);
}

static 
void gedit_mdi_child_changed_handler (BonoboMDI *mdi, BonoboMDIChild *old_child)
{
	gedit_debug (DEBUG_MDI, "");

	gedit_mdi_set_active_window_title (mdi);	
}

static 
void gedit_mdi_view_changed_handler (BonoboMDI *mdi, GtkWidget *old_view)
{
	BonoboWindow *win;
	GtkWidget* status;
	GeditView *active_view;
	
	gedit_debug (DEBUG_MDI, "");

	gedit_mdi_set_active_window_verbs_sensitivity (mdi);

	active_view = GEDIT_VIEW (bonobo_mdi_get_active_view (mdi));
		
	win = bonobo_mdi_get_active_window (mdi);
	g_return_if_fail (win != NULL);

	if (old_view != NULL)
	{
		gedit_view_set_cursor_position_statusbar (GEDIT_VIEW (old_view), NULL);
		gedit_view_set_overwrite_mode_statusbar (GEDIT_VIEW (old_view), NULL);
	}

	gtk_widget_grab_focus (GTK_WIDGET (active_view));

	status = g_object_get_data (G_OBJECT (win), "CursorPosition");	
	gedit_view_set_cursor_position_statusbar (active_view, status);

	status = g_object_get_data (G_OBJECT (win), "OverwriteMode");	
	gedit_view_set_overwrite_mode_statusbar (active_view, status);
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

	if (active_child == NULL)
	{
		gedit_menus_set_verb_list_sensitive (ui_component, 
				gedit_menus_no_docs_sensible_verbs, FALSE);
		goto end;
	}
	else
	{
		gedit_menus_set_verb_list_sensitive (ui_component, 
				gedit_menus_all_sensible_verbs, TRUE);
	}

	doc = GEDIT_MDI_CHILD (active_child)->document;
	g_return_if_fail (doc != NULL);
	
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
		goto end;
	}

	if (gedit_document_is_untitled (doc))
	{
		gedit_menus_set_verb_list_sensitive (ui_component, 
				gedit_menus_untitled_doc_sensible_verbs, FALSE);
	}

end:
	bonobo_ui_component_thaw (ui_component, NULL);
}


static void 
gedit_mdi_set_active_window_undo_redo_verbs_sensitivity (BonoboMDI *mdi)
{
	BonoboWindow* active_window = NULL;
	BonoboMDIChild* active_child = NULL;
	GeditDocument* doc = NULL;
	BonoboUIComponent *ui_component;
	
	gedit_debug (DEBUG_MDI, "");
	
	active_window = bonobo_mdi_get_active_window (mdi);
	g_return_if_fail (active_window != NULL);
	
	ui_component = bonobo_mdi_get_ui_component_from_window (active_window);
	g_return_if_fail (ui_component != NULL);
	
	active_child = bonobo_mdi_get_active_child (mdi);
	doc = GEDIT_MDI_CHILD (active_child)->document;
	g_return_if_fail (doc != NULL);

	bonobo_ui_component_freeze (ui_component, NULL);

	gedit_menus_set_verb_sensitive (ui_component, "/commands/EditUndo", 
			gedit_document_can_undo (doc));	

	gedit_menus_set_verb_sensitive (ui_component, "/commands/EditRedo", 
			gedit_document_can_redo (doc));	

	bonobo_ui_component_thaw (ui_component, NULL);
}

void
gedit_mdi_update_ui_according_to_preferences (GeditMDI *mdi)
{
#if 0
	GList *windows;		
#endif
	GList *children;
	GdkColor background;
	GdkColor text;
	GdkColor selection;
	GdkColor sel_text;
	gchar* font;

	gedit_debug (DEBUG_MDI, "");

#if 0
	windows = bonobo_mdi_get_windows (BONOBO_MDI (mdi));

	while (windows != NULL)
	{
		BonoboUIComponent *ui_component;
		BonoboWindow *active_window = BONOBO_WINDOW (windows->data);
		g_return_if_fail (active_window != NULL);
		
		ui_component = bonobo_mdi_get_ui_component_from_window (active_window);
		g_return_if_fail (ui_component != NULL);

		bonobo_ui_component_freeze (ui_component, NULL);

		gedit_mdi_set_app_statusbar_style (active_window);
		gedit_mdi_set_app_toolbar_style (active_window);

		bonobo_ui_component_thaw (ui_component, NULL);

		windows = windows->next;
	}
#endif
	children = bonobo_mdi_get_children (BONOBO_MDI (mdi));

	if (gedit_settings->use_default_colors)
	{
		GtkStyle *style;
	       
		style = gtk_style_new ();

		background = style->base [GTK_STATE_NORMAL];
		text = style->text [GTK_STATE_NORMAL];
		sel_text = style->text [GTK_STATE_SELECTED];
		selection = style->base [GTK_STATE_SELECTED];

		g_object_unref (G_OBJECT (style));
	}
	else
	{
		background = gedit_settings->background_color;
		text = gedit_settings->text_color;
		selection = gedit_settings->selection_color;
		sel_text = gedit_settings->selected_text_color;
	}

	if (gedit_settings->use_default_font)
	{
		GtkStyle *style;
		
		style = gtk_style_new ();

		font = pango_font_description_to_string (style->font_desc);
		
		if (font == NULL)
			/* Fallback */
			font = g_strdup (gedit_settings->editor_font);

		g_object_unref (G_OBJECT (style));

	}
	else
		font = g_strdup (gedit_settings->editor_font);

	while (children != NULL)
	{
		GList *views = bonobo_mdi_child_get_views (BONOBO_MDI_CHILD (children->data));

		while (views != NULL)
		{
			GeditView *v =	GEDIT_VIEW (views->data);
			
			gedit_view_set_colors (v, &background, &text, &selection, &sel_text);
			gedit_view_set_font (v, font);
			gedit_view_set_wrap_mode (v, gedit_settings->wrap_mode);
			gedit_view_show_line_numbers (v, gedit_settings->show_line_numbers);
			gedit_view_set_tab_size (v, gedit_settings->tab_size);
			views = views->next;
		}
		
		children = children->next;
	}

	g_free (font);

/*
	bonobo_mdi_set_mode (BONOBO_MDI (mdi), gedit_settings->mdi_mode);
*/
}
