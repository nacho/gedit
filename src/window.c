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

#include "window.h"
#include "view.h"
#include "commands.h"
#include "document.h"
#include "print.h"
#include "menus.h"
#include "prefs.h"
#include "search.h"
#include "utils.h"
#include "plugin.h"
#include "recent.h"
#include "file.h"
#include "undo.h"
#include "../pixmaps/gedit-icon.xpm"

/*
extern GtkWidget *col_label;
GList *window_list = NULL; dis by chema*/
/* Window *window; dis by chema*/

GtkWindow *	gedit_window_active (void);
GnomeApp *	gedit_window_active_app (void);

void	gedit_window_new (GnomeMDI *mdi, GnomeApp *app);
void	gedit_window_set_auto_indent (gint auto_indent);
void	gedit_window_set_status_bar (void);
void	gedit_window_refresh_toolbar (void);
void	gedit_window_set_toolbar_labels (void);
void	gedit_window_load_toolbar_widgets (void);

static void	gedit_window_set_icon (GtkWidget *window, char *icon);

GeditToolbar *gedit_toolbar = NULL;

GtkWindow *
gedit_window_active (void)
{
	gedit_debug ("", DEBUG_WINDOW);
	if (GTK_IS_WIDGET (mdi->active_window))
		return GTK_WINDOW (mdi->active_window);
	else
		return NULL;
}

GnomeApp *
gedit_window_active_app (void)
{
	gedit_debug ("", DEBUG_WINDOW);
	return mdi->active_window;
}

void
gedit_window_new (GnomeMDI *mdi, GnomeApp *app)
{
	static GtkTargetEntry drag_types[] =
	{
		{ "text/uri-list", 0, 0 },
	};

	static gint n_drag_types = sizeof (drag_types) / sizeof (drag_types [0]);

	gedit_debug ("", DEBUG_WINDOW);

	g_print ("1. A\n");
	gtk_drag_dest_set (GTK_WIDGET(app),
			   GTK_DEST_DEFAULT_MOTION |
			   GTK_DEST_DEFAULT_HIGHLIGHT |
			   GTK_DEST_DEFAULT_DROP,
			   drag_types, n_drag_types,
			   GDK_ACTION_COPY);
		
	g_print ("2. A\n");
	gtk_signal_connect (GTK_OBJECT (app), "drag_data_received",
			    GTK_SIGNAL_FUNC (filenames_dropped), NULL);

	g_print ("3. A\n");
	gedit_window_set_icon (GTK_WIDGET (app), "gedit_icon");

	g_print ("4. A\n");
	gtk_window_set_default_size (GTK_WINDOW(app), settings->width, settings->height);

	g_print ("5. A\n");
	gtk_window_set_policy (GTK_WINDOW (app), TRUE, TRUE, FALSE);

	/*gedit_load_settings ();*/
	/*
	*/
	
	settings->num_recent = 0;

	g_print ("6. A\n");
	gedit_recent_update (GNOME_APP (app));
	g_print ("7. A\n");
	

}

void
gedit_window_set_auto_indent (gint auto_indent)
{
	gedit_debug ("", DEBUG_WINDOW);
	settings->auto_indent = auto_indent;
}

/* set the a window icon */
static void
gedit_window_set_icon (GtkWidget *window, char *icon)
{
	GdkPixmap *pixmap;
        GdkBitmap *mask;

	gedit_debug ("", DEBUG_WINDOW);

	gtk_widget_realize (window);
	
	pixmap = gdk_pixmap_create_from_xpm_d (window->window, &mask,
					       &window->style->bg[GTK_STATE_NORMAL],
					       (char **)gedit_icon_xpm);
	
	gdk_window_set_icon (window->window, NULL, pixmap, mask);
	
	/* Not sure about this.. need to test in E 
	gtk_widget_unrealize (window);*/
}

void
gedit_window_set_status_bar (void)
{
	GtkWidget *statusbar;
	gedit_debug ("", DEBUG_WINDOW);

	if (!mdi->active_window->statusbar)
	{
		statusbar = gnome_appbar_new (FALSE, TRUE, GNOME_PREFERENCES_USER);
		gnome_app_set_statusbar (GNOME_APP (mdi->active_window),
					 GTK_WIDGET (statusbar));
		gnome_app_install_menu_hints (GNOME_APP (mdi->active_window),
					      gnome_mdi_get_menubar_info (mdi->active_window));
		mdi->active_window->statusbar = statusbar;
	}

	if (mdi->active_window->statusbar->parent)
	{
		if (settings->show_status)
			return;
		gtk_widget_ref (mdi->active_window->statusbar);
		gtk_container_remove (GTK_CONTAINER (mdi->active_window->statusbar->parent),
				      mdi->active_window->statusbar);
	}
	else 
	{
		if (!settings->show_status)
			return;
		gtk_box_pack_start (GTK_BOX (mdi->active_window->vbox),
				    mdi->active_window->statusbar,
				    FALSE, FALSE, 0);
		gtk_widget_unref (GNOME_APP (mdi->active_window)->statusbar);
	}
}

/* WHY is this in gnome-mdi.c and not gnome-mdi.h ????????? Ask Jaka.
   Chema */
#define GNOME_MDI_TOOLBAR_INFO_KEY		"MDIToolbarUIInfo"
/* FIXME : This is not nice. But we can't use the toolbar labels since
 */
void
gedit_window_load_toolbar_widgets (void)
{
	GnomeUIInfo *toolbar_ui_info;
	gint count;
	
	gedit_debug ("", DEBUG_WINDOW);

	toolbar_ui_info = gtk_object_get_data (GTK_OBJECT (gedit_window_active()), GNOME_MDI_TOOLBAR_INFO_KEY);

	if (!gedit_toolbar)
		gedit_toolbar = g_malloc (sizeof (GeditToolbar));

	count = 0;
	while (toolbar_ui_info[count].type != GNOME_APP_UI_ENDOFINFO)
	{
		if (toolbar_ui_info [count].moreinfo == gedit_undo_undo)
			gedit_toolbar->undo_button = toolbar_ui_info[count].widget;
		if (toolbar_ui_info [count].moreinfo == gedit_undo_redo)
			gedit_toolbar->redo_button = toolbar_ui_info[count].widget;
		count++;
	}

	g_return_if_fail (gedit_toolbar->undo_button != NULL);
	g_return_if_fail (gedit_toolbar->redo_button != NULL);

	/* initialy both undo and redo are unsensitive */
	gtk_widget_set_sensitive (gedit_toolbar->undo_button, FALSE);
	gtk_widget_set_sensitive (gedit_toolbar->redo_button, FALSE);
	gedit_toolbar->undo = FALSE;
	gedit_toolbar->redo = FALSE;

}

#if 0
void
gedit_window_refresh_toolbar (void)
{
	GnomeApp *app;
	GnomeDockItem *dock_item;

	GtkToolbar *toolbar;

	GList * toolbar_item;
	GList *tooltip_item;

	GtkToolbarChild *toolbar_child;
	GtkTooltipsData *tooltip_data;

	gchar *label;
	
	gedit_debug ("", DEBUG_WINDOW);

	app = gedit_window_active_app();
	g_return_if_fail (app != NULL);

	dock_item = gnome_app_get_dock_item_by_name (app, GNOME_APP_TOOLBAR_NAME);
	toolbar = GTK_TOOLBAR (gnome_dock_item_get_child (dock_item));
	
	toolbar_item = GTK_TOOLBAR (toolbar)->children;
	tooltip_item = GTK_TOOLBAR (toolbar)->tooltips->tips_data_list;
	
	for ( ; toolbar_item; toolbar_item = toolbar_item->next)
	{
		toolbar_child = toolbar_item->data;
		if (toolbar_child->type == GTK_TOOLBAR_CHILD_BUTTON)
		{
			gtk_label_get (GTK_LABEL(toolbar_child->label), &label);
			g_print ("label: %s \n", label);
		}
		if (toolbar_child->type != GTK_TOOLBAR_CHILD_SPACE)
		{
			tooltip_data = tooltip_item->data;
			g_print("t: %s\n", tooltip_data->tip_text);
			tooltip_item = tooltip_item->next;
		}
	}
}
#endif

void
gedit_window_set_toolbar_labels (void)
{
	GnomeApp *app;
	GnomeDockItem *dock_item;
	GtkToolbar *toolbar;

	gedit_debug ("", DEBUG_WINDOW);

	app = mdi->active_window;
	g_return_if_fail (app != NULL);
	
	dock_item = gnome_app_get_dock_item_by_name (app, GNOME_APP_TOOLBAR_NAME);
	toolbar = GTK_TOOLBAR (gnome_dock_item_get_child (dock_item));
	if (settings->toolbar_labels)
		gtk_toolbar_set_style (toolbar, GTK_TOOLBAR_BOTH);
	else
		gtk_toolbar_set_style (toolbar, GTK_TOOLBAR_ICONS);

}

