/*
 * gedit-settings.h
 * This file is part of gedit
 *
 * Copyright (C) 2009 - Ignacio Casal Quinteiro
 *               2002 - Paolo Maggi
 *
 * gedit is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * gedit is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with gedit; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */


#ifndef __GEDIT_SETTINGS_H__
#define __GEDIT_SETTINGS_H__

#include <glib-object.h>
#include <glib.h>
#include "gedit-app.h"

G_BEGIN_DECLS

#define GEDIT_TYPE_SETTINGS		(gedit_settings_get_type ())
#define GEDIT_SETTINGS(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_SETTINGS, GeditSettings))
#define GEDIT_SETTINGS_CONST(obj)	(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_SETTINGS, GeditSettings const))
#define GEDIT_SETTINGS_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GEDIT_TYPE_SETTINGS, GeditSettingsClass))
#define GEDIT_IS_SETTINGS(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEDIT_TYPE_SETTINGS))
#define GEDIT_IS_SETTINGS_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEDIT_TYPE_SETTINGS))
#define GEDIT_SETTINGS_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GEDIT_TYPE_SETTINGS, GeditSettingsClass))

typedef struct _GeditSettings		GeditSettings;
typedef struct _GeditSettingsClass	GeditSettingsClass;
typedef struct _GeditSettingsPrivate	GeditSettingsPrivate;

struct _GeditSettings
{
	GObject parent;

	GeditSettingsPrivate *priv;
};

struct _GeditSettingsClass
{
	GObjectClass parent_class;
};

GType			 gedit_settings_get_type			(void) G_GNUC_CONST;

GObject			*gedit_settings_new				(void);

GeditLockdownMask	 gedit_settings_get_lockdown			(GeditSettings *gs);

gchar			*gedit_settings_get_system_font			(GeditSettings *gs);

/* Utility functions */
GSList			*gedit_settings_get_list			(GSettings     *settings,
									 const gchar   *key);

void			 gedit_settings_set_list			(GSettings     *settings,
									 const gchar   *key,
									 const GSList  *list);

/* key constants */
#define GEDIT_SETTINGS_USE_DEFAULT_FONT			"use-default-font"
#define GEDIT_SETTINGS_EDITOR_FONT			"editor-font"
#define GEDIT_SETTINGS_SCHEME				"scheme"
#define GEDIT_SETTINGS_CREATE_BACKUP_COPY		"create-backup-copy"
#define GEDIT_SETTINGS_AUTO_SAVE			"auto-save"
#define GEDIT_SETTINGS_AUTO_SAVE_INTERVAL		"auto-save-interval"
#define GEDIT_SETTINGS_UNDO_ACTIONS_LIMIT		"undo-actions-limit"
#define GEDIT_SETTINGS_MAX_UNDO_ACTIONS			"max-undo-actions"
#define GEDIT_SETTINGS_WRAP_MODE			"wrap-mode"
#define GEDIT_SETTINGS_TABS_SIZE			"tabs-size"
#define GEDIT_SETTINGS_INSERT_SPACES			"insert-spaces"
#define GEDIT_SETTINGS_AUTO_INDENT			"auto-indent"
#define GEDIT_SETTINGS_DISPLAY_LINE_NUMBERS		"display-line-numbers"
#define GEDIT_SETTINGS_HIGHLIGHT_CURRENT_LINE		"highlight-current-line"
#define GEDIT_SETTINGS_BRACKET_MATCHING			"bracket-matching"
#define GEDIT_SETTINGS_DISPLAY_RIGHT_MARGIN		"display-right-margin"
#define GEDIT_SETTINGS_RIGHT_MARGIN_POSITION		"right-margin-position"
#define GEDIT_SETTINGS_SMART_HOME_END			"smart-home-end"
#define GEDIT_SETTINGS_WRITABLE_VFS_SCHEMES		"writable-vfs-schemes"
#define GEDIT_SETTINGS_RESTORE_CURSOR_POSITION		"restore-cursor-position"
#define GEDIT_SETTINGS_SYNTAX_HIGHLIGHTING		"syntax-highlighting"
#define GEDIT_SETTINGS_SEARCH_HIGHLIGHTING		"search-highlighting"
#define GEDIT_SETTINGS_TOOLBAR_VISIBLE			"toolbar-visible"
#define GEDIT_SETTINGS_TOOLBAR_BUTTONS_STYLE		"toolbar-buttons-style"
#define GEDIT_SETTINGS_STATUSBAR_VISIBLE		"statusbar-visible"
#define GEDIT_SETTINGS_SIDE_PANEL_VISIBLE		"side-panel-visible"
#define GEDIT_SETTINGS_BOTTOM_PANEL_VISIBLE		"bottom-panel-visible"
#define GEDIT_SETTINGS_MAX_RECENTS			"max-recents"
#define GEDIT_SETTINGS_PRINT_SYNTAX_HIGHLIGHTING	"print-syntax-highlighting"
#define GEDIT_SETTINGS_PRINT_HEADER			"print-header"
#define GEDIT_SETTINGS_PRINT_WRAP_MODE			"print-wrap-mode"
#define GEDIT_SETTINGS_PRINT_LINE_NUMBERS		"print-line-numbers"
#define GEDIT_SETTINGS_PRINT_FONT_BODY_PANGO		"print-font-body-pango"
#define GEDIT_SETTINGS_PRINT_FONT_HEADER_PANGO		"print-font-header-pango"
#define GEDIT_SETTINGS_PRINT_FONT_NUMBERS_PANGO		"print-font-numbers-pango"
#define GEDIT_SETTINGS_ENCODING_AUTO_DETECTED		"auto-detected"
#define GEDIT_SETTINGS_ENCODING_SHOWN_IN_MENU		"shown-in-menu"
#define GEDIT_SETTINGS_ACTIVE_PLUGINS			"active-plugins"

/* window state keys */
#define GEDIT_SETTINGS_WINDOW_STATE			"state"
#define GEDIT_SETTINGS_WINDOW_SIZE			"size"
#define GEDIT_SETTINGS_SHOW_TABS_MODE			"notebook-show-tabs-mode"
#define GEDIT_SETTINGS_SIDE_PANEL_SIZE			"side-panel-size"
#define GEDIT_SETTINGS_SIDE_PANEL_ACTIVE_PAGE		"side-panel-active-page"
#define GEDIT_SETTINGS_BOTTOM_PANEL_SIZE		"bottom-panel-size"
#define GEDIT_SETTINGS_BOTTOM_PANEL_ACTIVE_PAGE		"bottom-panel-active-page"
#define GEDIT_SETTINGS_ACTIVE_FILE_FILTER		"filter-id"

G_END_DECLS

#endif /* __GEDIT_SETTINGS_H__ */

/* ex:ts=8:noet: */
