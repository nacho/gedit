/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit
 *
 * Copyright (C) 1998, 1999, 2000 Alex Roberts, Evan Lawrence, Jason Leach, Jose M Celorio
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
 *
 * Authors: 
 *   Jason Leach <leach@wam.umd.edu>
 *   Chema Celorio <chema@celorio.com>
 */

#ifndef __UTILS_H__
#define __UTILS_H__

typedef enum {
	DEBUG_VIEW,
	DEBUG_UNDO,
	DEBUG_SEARCH,
	DEBUG_PRINT,
	DEBUG_PREFS,
	DEBUG_PLUGINS,
	DEBUG_FILE,
	DEBUG_DOCUMENT,
	DEBUG_RECENT,	
	DEBUG_COMMANDS,
	DEBUG_WINDOW
}DebugSection;

extern gint debug;
extern gint debug_view;
extern gint debug_undo;
extern gint debug_search;
extern gint debug_print;
extern gint debug_prefs;
extern gint debug_plugins;
extern gint debug_file;
extern gint debug_document;
extern gint debug_commands;
extern gint debug_recent;
extern gint debug_window;

#define gedit_debug(str, section) \
    if (debug) \
	printf ("%s:%d (%s) %s\n", __FILE__, __LINE__, __FUNCTION__, str); \
    else { \
    if (debug_view && section == DEBUG_VIEW) \
	printf ("%s:%d (%s) %s\n", __FILE__, __LINE__, __FUNCTION__, str); \
    if (debug_undo && section == DEBUG_UNDO) \
	printf ("%s:%d (%s) %s\n", __FILE__, __LINE__, __FUNCTION__, str); \
    if (debug_search && section == DEBUG_SEARCH) \
	printf ("%s:%d (%s) %s\n", __FILE__, __LINE__, __FUNCTION__, str); \
    if (debug_print && section == DEBUG_PRINT) \
	printf ("%s:%d (%s) %s\n", __FILE__, __LINE__, __FUNCTION__, str); \
    if (debug_prefs && section == DEBUG_PREFS) \
	printf ("%s:%d (%s) %s\n", __FILE__, __LINE__, __FUNCTION__, str); \
    if (debug_plugins && section == DEBUG_PLUGINS) \
	printf ("%s:%d (%s) %s\n", __FILE__, __LINE__, __FUNCTION__, str); \
    if (debug_file && section == DEBUG_FILE) \
	printf ("%s:%d (%s) %s\n", __FILE__, __LINE__, __FUNCTION__, str); \
    if (debug_document && section == DEBUG_DOCUMENT) \
	printf ("%s:%d (%s) %s\n", __FILE__, __LINE__, __FUNCTION__, str); \
    if (debug_commands && section == DEBUG_COMMANDS) \
	printf ("%s:%d (%s) %s\n", __FILE__, __LINE__, __FUNCTION__, str); \
    if (debug_recent && section == DEBUG_RECENT) \
	printf ("%s:%d (%s) %s\n", __FILE__, __LINE__, __FUNCTION__, str); \
    if (debug_window && section == DEBUG_WINDOW) \
	printf ("%s:%d (%s) %s\n", __FILE__, __LINE__, __FUNCTION__, str); \
    }

static const struct poptOption options[] =
{
	{ "debug-window", '\0', 0, &debug_commands, 0,
	  N_("Show window debugging messages."), NULL },

	{ "debug-commands", '\0', 0, &debug_commands, 0,
	  N_("Show commands debugging messages."), NULL },

	{ "debug-document", '\0', 0, &debug_document, 0,
	  N_("Show document debugging messages."), NULL },

	{ "debug-file", '\0', 0, &debug_file, 0,
	  N_("Show file debugging messages."), NULL },

	{ "debug-plugins", '\0', 0, &debug_plugins, 0,
	  N_("Show plugin debugging messages."), NULL },

	{ "debug-prefs", '\0', 0, &debug_prefs, 0,
	  N_("Show prefs debugging messages."), NULL },

	{ "debug-print", '\0', 0, &debug_print, 0,
	  N_("Show printing debugging messages."), NULL },

	{ "debug-search", '\0', 0, &debug_search, 0,
	  N_("Show search debugging messages."), NULL },

	{ "debug-undo", '\0', 0, &debug_undo, 0,
	  N_("Show undo debugging messages."), NULL },

	{ "debug-view", '\0', 0, &debug_view, 0,
	  N_("Show view debugging messages."), NULL },

	{ "debug-recent", '\0', 0, &debug_recent, 0,
	  N_("Show recent debugging messages."), NULL },

	{ "debug", '\0', 0, &debug, 0,
	  N_("Turn on all debugging messages."), NULL },

	{NULL, '\0', 0, NULL, 0}
};

#define gedit_editable_active() GTK_EDITABLE(VIEW (gedit_view_active())->text)

void	gedit_flash     (gchar *msg);
void	gedit_flash_va  (gchar *format, ...);
void	gedit_debug_mess (gchar *message, DebugSection type);

/* Radio buttons utility functions */
gint	gtk_radio_group_get_selected (GSList *radio_group);
void	gtk_radio_button_select (GSList *group, int n);

#endif /* __UTILS_H__ */
