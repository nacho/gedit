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

#include <libgnome/gnome-defs.h>
#include <libgnomeui/gnome-preferences.h>

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

/*
extern GtkWidget *col_label;
GList *window_list = NULL; dis by chema*/
/* Window *window; dis by chema*/

GtkWindow *	gedit_window_active (void);
GnomeApp *	gedit_window_active_app (void);

void	gedit_window_new (GnomeMDI *mdi, GnomeApp *app);
void	gedit_window_set_auto_indent (gint auto_indent);
void	gedit_window_set_status_bar (GnomeApp *app);
void	gedit_window_refresh_all (gint mdi_mode_changed);
void	gedit_window_set_toolbar_labels (GnomeApp *app);
void	gedit_window_set_widgets_sensitivity (gint sensitive);

GeditToolbar *gedit_toolbar = NULL;

GtkWindow *
gedit_window_active (void)
{
	gedit_debug (DEBUG_WINDOW, "");
	if (GTK_IS_WIDGET (mdi->active_window))
		return GTK_WINDOW (mdi->active_window);
	else
		return NULL;
}

GnomeApp *
gedit_window_active_app (void)
{
	gedit_debug (DEBUG_WINDOW, "");
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
	GtkWidget *statusbar;

	gedit_debug (DEBUG_WINDOW, "");

	/* Drag and drop support */
	gtk_drag_dest_set (GTK_WIDGET(app),
			   GTK_DEST_DEFAULT_MOTION |
			   GTK_DEST_DEFAULT_HIGHLIGHT |
			   GTK_DEST_DEFAULT_DROP,
			   drag_types, n_drag_types,
			   GDK_ACTION_COPY);
		
	gtk_signal_connect (GTK_OBJECT (app), "drag_data_received",
			    GTK_SIGNAL_FUNC (filenames_dropped), NULL);


	/* Set the status bar */
	statusbar = gnome_appbar_new (FALSE, TRUE, GNOME_PREFERENCES_USER);
	gnome_app_set_statusbar (app,
				 GTK_WIDGET (statusbar));
	gedit_window_set_status_bar (app);
	gnome_app_install_menu_hints (app,
				      gnome_mdi_get_menubar_info (mdi->active_window));

	/* Set the window prefs. */
	gtk_window_set_default_size (GTK_WINDOW(app), settings->width, settings->height);
	gtk_window_set_policy (GTK_WINDOW (app), TRUE, TRUE, FALSE);

	/* Add the recent files */
	gedit_recent_update (app);

	/* Add the plugins to the menus */
	gedit_plugins_menu_add (app);

}

void
gedit_window_set_auto_indent (gint auto_indent)
{
	gedit_debug (DEBUG_WINDOW, "");
	settings->auto_indent = auto_indent;
}

/* set the a window icon */
#if 0
static void
gedit_window_set_icon (GtkWidget *window, char *icon)
{
	GdkPixmap *pixmap;
        GdkBitmap *mask;

	gedit_debug (DEBUG_WINDOW, "");

	gtk_widget_realize (window);
	
	pixmap = gdk_pixmap_create_from_xpm_d (window->window, &mask,
					       &window->style->bg[GTK_STATE_NORMAL],
					       (char **)gedit_icon_xpm);
	
	gdk_window_set_icon (window->window, NULL, pixmap, mask);
	
	/* Not sure about this.. need to test in E 
	gtk_widget_unrealize (window);*/
}
#endif

void
gedit_window_set_status_bar (GnomeApp *app)
{
	gedit_debug (DEBUG_WINDOW, "");

	
	if (app->statusbar->parent)
	{
		if (settings->show_status)
			return;
		gtk_widget_ref (app->statusbar);
		gtk_container_remove (GTK_CONTAINER (app->statusbar->parent),
				      app->statusbar);
	}
	else 
	{
		if (!settings->show_status)
			return;
		gtk_box_pack_start (GTK_BOX (app->vbox),
				    app->statusbar,
				    FALSE, FALSE, 0);
		gtk_widget_unref (app->statusbar);
	}
}


void
gedit_window_set_toolbar_labels (GnomeApp *app)
{
	GnomeDockItem *dock_item;
	GtkToolbar *toolbar;
	
	gedit_debug (DEBUG_WINDOW, "");

	g_return_if_fail (app != NULL);
	
	dock_item = gnome_app_get_dock_item_by_name (app, GNOME_APP_TOOLBAR_NAME);
	g_return_if_fail (dock_item != NULL);
	
	toolbar = GTK_TOOLBAR (gnome_dock_item_get_child (dock_item));
	g_return_if_fail (toolbar != NULL);

	switch (settings->toolbar_labels)
	{
	case GEDIT_TOOLBAR_SYSTEM:
		if (gnome_preferences_get_toolbar_labels())
		{
			gtk_toolbar_set_style (toolbar, GTK_TOOLBAR_BOTH);
		}
		else
		{
			gtk_toolbar_set_style (toolbar, GTK_TOOLBAR_ICONS);
			gtk_widget_queue_resize (GTK_WIDGET (dock_item)->parent);
		}
		break;
	case 1:
		gtk_toolbar_set_style (toolbar, GTK_TOOLBAR_ICONS);
		gtk_widget_queue_resize (GTK_WIDGET (dock_item)->parent);
		break;
	case 2:
		gtk_toolbar_set_style (toolbar, GTK_TOOLBAR_BOTH);
		break;
	default:
		g_return_if_fail (FALSE);
		break;
	}
}

void
gedit_window_set_view_menu_sensitivity (GnomeApp *app)
{
	GnomeUIInfo *ui_info;
	GnomeUIInfo *sub_ui_info;
	GtkWidget *widget;
	gint sensitivity = FALSE;
	gint count, sub_count;
	
	gedit_debug (DEBUG_WINDOW, "");

	g_return_if_fail (GNOME_IS_APP (app));
	
	switch (settings->mdi_mode)
	{
	case GNOME_MDI_NOTEBOOK:
	case GNOME_MDI_TOPLEVEL:
		sensitivity = TRUE;
		break;
	case GNOME_MDI_MODAL:
		sensitivity = FALSE;
		break;
	case GNOME_MDI_DEFAULT_MODE:
		if (gnome_preferences_get_mdi_mode() == GNOME_MDI_MODAL)
			sensitivity = FALSE;
		else
			sensitivity = TRUE;
		break;
	default:
		g_warning ("Should not happen.\n");
		return;
	}

	/* get the UI_info structures */
	ui_info = gnome_mdi_get_menubar_info (app);

	g_return_if_fail (ui_info != NULL);

	/* Set the menus and submenus */
	count = 0;
	while (ui_info[count].type != GNOME_APP_UI_ENDOFINFO)
	{
		if (ui_info[count].type == GNOME_APP_UI_SUBTREE_STOCK ||
		    ui_info[count].type == GNOME_APP_UI_SUBTREE)
		{
			sub_count = 0;
			sub_ui_info = ui_info [count].moreinfo;
			while (sub_ui_info[sub_count].type != GNOME_APP_UI_ENDOFINFO)
			{
				if (sub_ui_info [sub_count].moreinfo == gedit_view_add_cb)
				{
					widget =  sub_ui_info [sub_count].widget;
					if (GTK_IS_WIDGET (widget))
						gtk_widget_set_sensitive (widget, sensitivity);
				}
				if (sub_ui_info [sub_count].moreinfo == gedit_view_remove_cb)
				{
					/* We need to check if there are more than 2 views opened */
					/* The only info we have is *app, and we can't use view_active. */
					GeditDocument *doc;
					doc = gedit_document_current();
					if (doc!=NULL)
						if (g_list_length(doc->views)<2)
							sensitivity = FALSE;
					widget =  sub_ui_info [sub_count].widget;
					if (GTK_IS_WIDGET (widget))
						gtk_widget_set_sensitive (widget, sensitivity);
				}
				if (sub_ui_info [sub_count].moreinfo == file_revert_cb)
				{
					/* We need to check if there are more than 2 views opened */
					/* The only info we have is *app, and we can't use view_active. */
					GeditDocument *doc;
					doc = gedit_document_current();

					widget =  sub_ui_info [sub_count].widget;
					if (GTK_IS_WIDGET (widget) && doc != NULL)
						gtk_widget_set_sensitive (widget, doc->filename!=NULL);
				}
				sub_count++;
			}
		}
		count++;
	}

}

void
gedit_window_refresh_all (gint mdi_mode_changed)
{
	gint n, m;

	GeditDocument *nth_doc;
	GnomeApp *nth_app;
	GeditView     *mth_view;

	GtkStyle *style;
	GdkColor *bg, *fg;

	
	gedit_debug (DEBUG_WINDOW, "");

	/* Set mdi mode */
	if (mdi_mode_changed)
		gnome_mdi_set_mode (mdi, settings->mdi_mode);

	tab_pos (settings->tab_pos);

	/* Set style and font for each children */

	/* We  need to change the toolbar style even if there aren't any documents open !!!
	   the toolbar is loaded for every view that is open, because when mdi_mode = toplevel
	   each view will have it's own undo & redo buttons that need to get shaded/unshaded
	   Chema */
	/* Set the toolbar and the status bar for each window. (mdi_mode = toplevel); */
	for (n = 0; n < g_list_length (mdi->windows); n++)
	{
		nth_app = GNOME_APP (g_list_nth_data (mdi->windows, n));
		gedit_window_set_status_bar (nth_app);
		gedit_window_set_toolbar_labels (nth_app);
		if (mdi_mode_changed)
			gedit_window_set_view_menu_sensitivity (nth_app);
	}

	if (gedit_document_current()==NULL)
		return;
		
	style = gtk_style_copy (gtk_widget_get_style (GTK_WIDGET (GEDIT_VIEW(mdi->active_view)->text)));

	bg = &style->base[0];
	bg->red = settings->bg[0];
	bg->green = settings->bg[1];
	bg->blue = settings->bg[2];

	fg = &style->text[0];
	fg->red = settings->fg[0];
	fg->green = settings->fg[1];
	fg->blue = settings->fg[2];

	for (n = 0; n < g_list_length (mdi->children); n++)
	{
		nth_doc = GEDIT_DOCUMENT (g_list_nth_data (mdi->children, n));
		for (m = 0; m < g_list_length (nth_doc->views); m++)
		{
			mth_view = GEDIT_VIEW (g_list_nth_data (nth_doc->views, m));
			if (mdi_mode_changed)
			{
				gtk_widget_grab_focus (GTK_WIDGET (mth_view->text));
				mth_view->app = gedit_window_active_app();
				gedit_view_load_widgets (mth_view);
			}
			gedit_view_set_undo (mth_view, GEDIT_UNDO_STATE_REFRESH, GEDIT_UNDO_STATE_REFRESH);
			gtk_widget_set_style (GTK_WIDGET (mth_view->text),
					      style);
			gedit_view_set_font (mth_view, settings->font);
		}
	}
	
}


void
gedit_window_set_widgets_sensitivity_ro (GnomeApp *app, gint unsensitive)
{
	GnomeUIInfo *ui_info;
	GnomeUIInfo *sub_ui_info;
	GtkWidget *widget;
	gint count, sub_count;
	
	PluginData  *pd;
	GnomeDockItem *dock_item;
	GtkWidget *menu_bar;

	GList* children;
	gchar* path;
	gint menu_pos;

	gedit_debug (DEBUG_WINDOW, "");

	g_return_if_fail (GNOME_IS_APP (app));
	
	ui_info = gnome_mdi_get_toolbar_info (app);

	g_return_if_fail (ui_info != NULL);

	/* Set the toolbar tems */
	count = 0;
	while (ui_info[count].type != GNOME_APP_UI_ENDOFINFO)
	{
		if (ui_info [count].moreinfo == file_save_cb    ||
		    ui_info [count].moreinfo == edit_cut_cb     ||
		    ui_info [count].moreinfo == edit_paste_cb	||
		    ui_info [count].moreinfo == gedit_replace_cb)
		{
			widget =  ui_info [count].widget;
			if (GTK_IS_WIDGET (widget))
				gtk_widget_set_sensitive (widget, !unsensitive);
		}
		count++;
	}

	/* get the UI_info structures */
	ui_info = gnome_mdi_get_menubar_info (app);

	/* Set the menus and submenus */
	count = 0;
	while (ui_info[count].type != GNOME_APP_UI_ENDOFINFO)
	{
		if (ui_info[count].type == GNOME_APP_UI_SUBTREE_STOCK ||
		    ui_info[count].type == GNOME_APP_UI_SUBTREE)
		{
			sub_count = 0;
			sub_ui_info = ui_info [count].moreinfo;
			while (sub_ui_info[sub_count].type != GNOME_APP_UI_ENDOFINFO)
			{
				if (sub_ui_info [sub_count].moreinfo == file_save_cb    ||
				    sub_ui_info [sub_count].moreinfo == edit_cut_cb     ||
				    sub_ui_info [sub_count].moreinfo == edit_paste_cb   ||
				    sub_ui_info [sub_count].moreinfo == gedit_replace_cb)
				{
					widget =  sub_ui_info [sub_count].widget;
					if (GTK_IS_WIDGET (widget))
						gtk_widget_set_sensitive (widget, !unsensitive);
				}

				if (sub_ui_info [sub_count].moreinfo == file_revert_cb)
				{
					/* We need to check if there are more than 2 views opened */
					/* The only info we have is *app, and we can't use view_active. */
					GeditDocument *doc;
					doc = gedit_document_current();

					widget =  sub_ui_info [sub_count].widget;
					if (GTK_IS_WIDGET (widget) && doc != NULL)
						gtk_widget_set_sensitive (widget, (doc->filename!=NULL) && !unsensitive);
				}

				sub_count++;

			}

		}
		count++;
	}

	
	/* Set popup menu sensitivity*/
	count = 0;
	while (popup_menu[count].type != GNOME_APP_UI_ENDOFINFO)
	{
		if (popup_menu [count].moreinfo == file_save_cb    ||
		    popup_menu [count].moreinfo == edit_cut_cb     ||
		    popup_menu [count].moreinfo == edit_paste_cb   ||
		    popup_menu [count].moreinfo == gedit_replace_cb)
		{
			widget =  popup_menu [count].widget;
	
			if (GTK_IS_WIDGET (widget) && (GTK_OBJECT(widget)->ref_count > 0))
			{
				gtk_widget_set_sensitive (widget, !unsensitive);			
		/*		g_print("%d %s\n", count, !unsensitive ? "sensitive" : "not sensitive");*/
			}
		}
		
		count++;

		g_assert(count <= 10); /* Note: change this line if popup menu changes */
	}

	g_assert(count != 0);

	
	/* Set plugins menu sensitivity*/
	dock_item = gnome_app_get_dock_item_by_name (app, GNOME_APP_MENUBAR_NAME);	
	g_return_if_fail (GNOME_IS_DOCK_ITEM (dock_item));
	
	menu_bar = gnome_dock_item_get_child (dock_item);
			
	for (count = 0; count < g_list_length (plugins_list); count++)
	{
		pd = g_list_nth_data (plugins_list, count);

		if (pd->installed && pd->needs_a_document)
		{			       
			path = g_strdup_printf ("%s/%s", _("_Plugins"), pd->name);
			
			children = gtk_container_children (GTK_CONTAINER ( 
				gnome_app_find_menu_pos (menu_bar, path, &menu_pos)));
	
			widget = GTK_WIDGET (g_list_nth_data (children, menu_pos - 1));

			if (GTK_IS_WIDGET (widget))
			{
				if(pd->modifies_an_existing_doc)
				{
					gtk_widget_set_sensitive (widget, !unsensitive);
				}
				else
				{
					gtk_widget_set_sensitive (widget, TRUE);
				}
			}

			g_free (path);
		}
	}
	
	return;
	
}

/**
 * gedit_window_set_widgets_sensitivity:
 * @sensitive: 
 *
 * for sensitive = FALSE
 * this rutine is called when a document is closed and it there aren't any documents
 * opened it sets the menu items and toolbar items sensitivity.
 *
 * for sensitive = TRUE
 * is called whenever a new document is created, and will set menu/toolbar items
 * sensitivity to SENSITIVE
 *
 **/
void
gedit_window_set_widgets_sensitivity (gint sensitive)
{
	GnomeApp *app = NULL;
	GnomeUIInfo *ui_info;
	GnomeUIInfo *sub_ui_info;
	GtkWidget *widget;
		
	gint count = 0, sub_count = 0;
	
	gedit_debug (DEBUG_WINDOW, "");

	if (!sensitive && g_list_length (mdi->children) > 0)
		return;
	if (sensitive && g_list_length (mdi->children) == 0)
		return;

	app = GNOME_APP (g_list_nth_data (mdi->windows, 0));

	g_return_if_fail (GNOME_IS_APP (app));

	/* get the UI_info structures */
	ui_info = gnome_mdi_get_toolbar_info (app);

	g_return_if_fail (ui_info != NULL);

	/* Set the toolbar tems */
	while (ui_info[count].type != GNOME_APP_UI_ENDOFINFO)
	{
		if (ui_info [count].moreinfo == file_save_as_cb ||
		    ui_info [count].moreinfo == file_close_cb   ||
		    ui_info [count].moreinfo == file_print_cb   ||
		    ui_info [count].moreinfo == file_save_cb    ||
		    (ui_info [count].moreinfo == gedit_undo_undo && !sensitive)||
		    (ui_info [count].moreinfo == gedit_undo_redo && !sensitive) ||
		    ui_info [count].moreinfo == edit_cut_cb     ||
		    ui_info [count].moreinfo == edit_copy_cb    ||
		    ui_info [count].moreinfo == edit_paste_cb   ||
		    ui_info [count].moreinfo == gedit_find_cb   ||
		    ui_info [count].moreinfo == gedit_file_info_cb  )
		{
			widget =  ui_info [count].widget;
			if (GTK_IS_WIDGET (widget))
				gtk_widget_set_sensitive (widget, sensitive);
		}
		count++;
	}


	ui_info = gnome_mdi_get_menubar_info (app);
	
	g_return_if_fail (ui_info != NULL);

	/* Set the menus and submenus */
	count = 0;
	while (ui_info[count].type != GNOME_APP_UI_ENDOFINFO)
	{
		if (ui_info[count].type == GNOME_APP_UI_SUBTREE_STOCK ||
		    ui_info[count].type == GNOME_APP_UI_SUBTREE)
		{
			sub_count = 0;
			sub_ui_info = ui_info [count].moreinfo;
			while (sub_ui_info[sub_count].type != GNOME_APP_UI_ENDOFINFO)
			{
				if (sub_ui_info [sub_count].moreinfo == file_save_as_cb ||
				    sub_ui_info [sub_count].moreinfo == file_save_cb    ||
				    sub_ui_info [sub_count].moreinfo == file_save_all_cb   ||
				    sub_ui_info [sub_count].moreinfo == file_close_cb   ||
				    sub_ui_info [sub_count].moreinfo == file_close_all_cb   ||
				    sub_ui_info [sub_count].moreinfo == file_revert_cb  ||
				    sub_ui_info [sub_count].moreinfo == file_print_cb   ||
				    sub_ui_info [sub_count].moreinfo == file_print_preview_cb   ||
				    (sub_ui_info [sub_count].moreinfo == gedit_undo_undo && !sensitive)||
				    (sub_ui_info [sub_count].moreinfo == gedit_undo_redo && !sensitive)||
				    sub_ui_info [sub_count].moreinfo == edit_cut_cb     ||
				    sub_ui_info [sub_count].moreinfo == edit_copy_cb    ||
				    sub_ui_info [sub_count].moreinfo == edit_paste_cb   ||
				    sub_ui_info [sub_count].moreinfo == gedit_find_cb    ||
				    sub_ui_info [sub_count].moreinfo == edit_select_all_cb ||
				    sub_ui_info [sub_count].moreinfo == gedit_find_again_cb ||
				    sub_ui_info [sub_count].moreinfo == gedit_replace_cb ||
				    sub_ui_info [sub_count].moreinfo == gedit_goto_line_cb ||
				    sub_ui_info [sub_count].moreinfo == gedit_view_add_cb||
				    sub_ui_info [sub_count].moreinfo == gedit_view_remove_cb ||
				    sub_ui_info [sub_count].moreinfo == gedit_file_info_cb  )
				{
					widget =  sub_ui_info [sub_count].widget;
					if (GTK_IS_WIDGET (widget))
						gtk_widget_set_sensitive (widget, sensitive);
				}
				sub_count++;
			}
		}
		count++;
	}

	if (sensitive)
		gedit_window_set_view_menu_sensitivity (app);

	gedit_window_set_plugins_menu_sensitivity (sensitive);

	return;
}

/**
 * gedit_window_set_plugins_menu_sensitivity:
 * @sensitive: 
 *
 * Set plugins menu sensitivity
 **/
void
gedit_window_set_plugins_menu_sensitivity (gint sensitive)
{
	GnomeApp *app;
	PluginData  *pd;
	GnomeDockItem *dock_item;
	GtkWidget *menu_bar, *widget;
	gint count;

	GList* children;
	gchar* path;
	gint menu_pos;
	
	app = GNOME_APP (g_list_nth_data (mdi->windows, 0));
	g_return_if_fail (GNOME_IS_APP (app));

	dock_item = gnome_app_get_dock_item_by_name (app, GNOME_APP_MENUBAR_NAME);	
	g_return_if_fail (GNOME_IS_DOCK_ITEM (dock_item));
	
	menu_bar = gnome_dock_item_get_child (dock_item);
			
	for (count = 0; count < g_list_length (plugins_list); count++)
	{
		pd = g_list_nth_data (plugins_list, count);

		if (pd->installed && pd->needs_a_document)
		{			       
			path = g_strdup_printf ("%s/%s", _("_Plugins"), pd->name);
			
			children = gtk_container_children (GTK_CONTAINER ( 
				gnome_app_find_menu_pos (menu_bar, path, &menu_pos)));
	
			widget = GTK_WIDGET (g_list_nth_data (children, menu_pos - 1));

			if (GTK_IS_WIDGET (widget))
				gtk_widget_set_sensitive (widget, sensitive);

			g_free (path);
		}
	}
	
	return;
			
}

