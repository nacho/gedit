/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit - Utility functions
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Utility functions by Jason Leach <leach@wam.umd.edu>
 */

#ifndef __UTILS_H__
#define __UTILS_H__

typedef enum {
	DEBUG_UNDO,
	DEBUG_UNDO_DEEP,
	DEBUG_VIEW,
	DEBUG_VIEW_DEEP,
	DEBUG_PLUGINS,
	DEBUG_PLUGINS_DEEP,
	DEBUG_FILE,
	DEBUG_FILE_DEEP,
	DEBUG_SEARCH,
	DEBUG_SEARCH_DEEP,
	DEBUG_PREFS,
	DEBUG_PREFS_DEEP,
	DEBUG_PRINT,
	DEBUG_PRINT_DEEP,
	DEBUG_DOCUMENT,
	DEBUG_DOCUMENT_DEEP,
	DEBUG_COMMANDS
} DebugSection;


void gedit_set_title (Document *doc);
void gedit_flash     (gchar *msg);
void gedit_flash_va  (gchar *format, ...);
int  gtk_radio_group_get_selected (GSList *radio_group);
void gtk_radio_button_select (GSList *group, int n);
void gedit_debug_mess (gchar *message, DebugSection type);

#endif /* __UTILS_H__ */
