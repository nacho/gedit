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
 *
 * Authors: 
 *   Jason Leach <leach@wam.umd.edu>
 *   Chema Celorio <chema@celorio.com>
 */

#ifndef __UTILS_H__
#define __UTILS_H__

typedef enum {
	DEBUG_UNDO,
	DEBUG_VIEW,
	DEBUG_PLUGINS,
	DEBUG_FILE,
	DEBUG_SEARCH,
	DEBUG_PREFS,
	DEBUG_PRINT,
	DEBUG_DOCUMENT,
	DEBUG_COMMANDS
} DebugSection;

extern gint debug;
extern gint debug_print;
extern gint debug_file;

#define gedit_debug(str, num) \
    if (debug) \
	printf ("%s:%d (%s) %s", __FILE__, __LINE__, __FUNCTION__, str); \
    else { \
    if (debug_print && num == DEBUG_PRINT) \
	printf ("%s:%d (%s) %s", __FILE__, __LINE__, __FUNCTION__, str); \
    if (debug_file && num == DEBUG_FILE) \
	printf ("%s:%d (%s) %s", __FILE__, __LINE__, __FUNCTION__, str); \
    }

void gedit_set_title (Document *doc);
void gedit_flash     (gchar *msg);
void gedit_flash_va  (gchar *format, ...);
int  gtk_radio_group_get_selected (GSList *radio_group);
void gtk_radio_button_select (GSList *group, int n);
void gedit_debug_mess (gchar *message, DebugSection type);

#endif /* __UTILS_H__ */
