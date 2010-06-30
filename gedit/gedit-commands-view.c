/*
 * gedit-view-commands.c
 * This file is part of gedit
 *
 * Copyright (C) 1998, 1999 Alex Roberts, Evan Lawrence
 * Copyright (C) 2000, 2001 Chema Celorio, Paolo Maggi
 * Copyright (C) 2002-2005 Paolo Maggi
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
 * Modified by the gedit Team, 1998-2005. See the AUTHORS file for a
 * list of people on the gedit Team.
 * See the ChangeLog files for a list of changes.
 *
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>

#include "gedit-commands.h"
#include "gedit-debug.h"
#include "gedit-window.h"
#include "gedit-window-private.h"


void
_gedit_cmd_view_show_toolbar (GtkAction   *action,
			      GeditWindow *window)
{
	gboolean visible;

	gedit_debug (DEBUG_COMMANDS);

	visible = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

	gtk_widget_set_visible (window->priv->toolbar, visible);
}

void
_gedit_cmd_view_show_statusbar (GtkAction   *action,
			        GeditWindow *window)
{
	gboolean visible;

	gedit_debug (DEBUG_COMMANDS);

	visible = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

	gtk_widget_set_visible (window->priv->statusbar, visible);
}

void
_gedit_cmd_view_show_side_panel (GtkAction   *action,
			         GeditWindow *window)
{
	gboolean visible;
	GeditPanel *panel;

	gedit_debug (DEBUG_COMMANDS);

	visible = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
	panel = gedit_window_get_side_panel (window);
	
	gtk_widget_set_visible (GTK_WIDGET (panel), visible);

	if (visible)
	{
		gtk_widget_grab_focus (GTK_WIDGET (panel));
	}
}

void
_gedit_cmd_view_show_bottom_panel (GtkAction   *action,
				   GeditWindow *window)
{
	gboolean visible;
	GeditPanel *panel;

	gedit_debug (DEBUG_COMMANDS);

	visible = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));
	panel = gedit_window_get_bottom_panel (window);

	gtk_widget_set_visible (GTK_WIDGET (panel), visible);

	if (visible)
	{
		gtk_widget_grab_focus (GTK_WIDGET (panel));
	}
}

void
_gedit_cmd_view_toggle_fullscreen_mode (GtkAction   *action,
					GeditWindow *window)
{
	gedit_debug (DEBUG_COMMANDS);

	if (_gedit_window_is_fullscreen (window))
		_gedit_window_unfullscreen (window);
	else
		_gedit_window_fullscreen (window);
}

void
_gedit_cmd_view_leave_fullscreen_mode (GtkAction   *action,
				       GeditWindow *window)
{
	GtkAction *view_action;

	view_action = gtk_action_group_get_action (window->priv->always_sensitive_action_group,
						   "ViewFullscreen");
	g_signal_handlers_block_by_func
		(view_action, G_CALLBACK (_gedit_cmd_view_toggle_fullscreen_mode),
		 window);
	gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (view_action),
				      FALSE);
	_gedit_window_unfullscreen (window);
	g_signal_handlers_unblock_by_func (view_action,
					   G_CALLBACK (_gedit_cmd_view_toggle_fullscreen_mode),
					   window);
}

void
_gedit_cmd_view_split_horizontally (GtkAction   *action,
				    GeditWindow *window)
{
	GeditTab *tab;

	tab = gedit_window_get_active_tab (window);

	_gedit_tab_split (tab, GTK_ORIENTATION_HORIZONTAL);
}

void
_gedit_cmd_view_split_vertically (GtkAction   *action,
				  GeditWindow *window)
{
	GeditTab *tab;

	tab = gedit_window_get_active_tab (window);

	_gedit_tab_split (tab, GTK_ORIENTATION_VERTICAL);
}

void
_gedit_cmd_view_unsplit (GtkAction   *action,
			 GeditWindow *window)
{
	GeditTab *tab;

	tab = gedit_window_get_active_tab (window);

	_gedit_tab_unsplit (tab);
}

/* ex:ts=8:noet: */
