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
	GEDIT_DEBUG_VIEW,
	GEDIT_DEBUG_UNDO,
	GEDIT_DEBUG_SEARCH,
	GEDIT_DEBUG_PRINT,
	GEDIT_DEBUG_PREFS,
	GEDIT_DEBUG_PLUGINS,
	GEDIT_DEBUG_FILE,
	GEDIT_DEBUG_DOCUMENT,
	GEDIT_DEBUG_RECENT,
	GEDIT_DEBUG_COMMANDS,
	GEDIT_DEBUG_WINDOW
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

/* __FUNCTION_ is not defined in Irix according to David Kaelbling <drk@sgi.com>*/
#ifndef __GNUC__
#define __FUNCTION__   ""
#endif

#define	DEBUG_VIEW	GEDIT_DEBUG_VIEW,    __FILE__, __LINE__, __FUNCTION__
#define	DEBUG_UNDO	GEDIT_DEBUG_UNDO,    __FILE__, __LINE__, __FUNCTION__
#define	DEBUG_SEARCH	GEDIT_DEBUG_SEARCH,  __FILE__, __LINE__, __FUNCTION__
#define	DEBUG_PRINT	GEDIT_DEBUG_PRINT,   __FILE__, __LINE__, __FUNCTION__
#define	DEBUG_PREFS	GEDIT_DEBUG_PREFS,   __FILE__, __LINE__, __FUNCTION__
#define	DEBUG_PLUGINS	GEDIT_DEBUG_PLUGINS, __FILE__, __LINE__, __FUNCTION__
#define	DEBUG_FILE	GEDIT_DEBUG_FILE,    __FILE__, __LINE__, __FUNCTION__
#define	DEBUG_DOCUMENT	GEDIT_DEBUG_DOCUMENT,__FILE__, __LINE__, __FUNCTION__
#define	DEBUG_RECENT	GEDIT_DEBUG_RECENT,  __FILE__, __LINE__, __FUNCTION__
#define	DEBUG_COMMANDS	GEDIT_DEBUG_COMMANDS,__FILE__, __LINE__, __FUNCTION__
#define	DEBUG_WINDOW	GEDIT_DEBUG_WINDOW,  __FILE__, __LINE__, __FUNCTION__

#define gedit_editable_active() GTK_EDITABLE(GEDIT_VIEW (gedit_view_active())->text)

void	gedit_flash     (gchar *msg);
void	gedit_flash_va  (gchar *format, ...);
void	gedit_debug_mess (gchar *message, DebugSection type);

/* Radio buttons utility functions */
gint	gtk_radio_group_get_selected (GSList *radio_group);
void	gtk_radio_button_select (GSList *group, int n);

typedef enum {
	GEDIT_PROGRAM_OK,
	GEDIT_PROGRAM_NOT_EXISTS,
	GEDIT_PROGRAM_IS_DIRECTORY,
	GEDIT_PROGRAM_IS_INSIDE_DIRECTORY,
	GEDIT_PROGRAM_NOT_EXECUTABLE
} GeditProgramErrors;

gint	gedit_utils_is_program (gchar * program, gchar* default_name);

void	gedit_debug (gint section, gchar *file, gint line, gchar* function, gchar* format, ...);
void 	gedit_utils_delete_temp (gchar* file_name);
gchar*	gedit_utils_create_temp_from_doc (Document *doc, gint number);
void	gedit_utils_error_dialog (gchar *error_message, GtkWidget *widget);

#endif /* __UTILS_H__ */
