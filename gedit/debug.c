/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit
 *
 * Copyright (C) 1998-2001 Alex Roberts, Evan Lawrence, Jason Leach, Jose Celorio
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

#include <glib.h>

#include "debug.h"

/* External debug options, used here and in gedit.c */
gint debug = 0;
gint debug_view = 0;
gint debug_undo = 0;
gint debug_search = 0;
gint debug_prefs = 0;
gint debug_print = 0;
gint debug_plugins = 0;
gint debug_file = 0;
gint debug_document = 0;
gint debug_commands = 0;
gint debug_recent = 0;
gint debug_window = 0;

void
gedit_debug (gint section, gchar *file, gint line, gchar* function, gchar* format, ...)
{
	va_list args;
	gchar *msg;

	g_return_if_fail (format != NULL);

	va_start (args, format);
	msg = g_strdup_vprintf (format, args);
	va_end (args);

	if (debug ||
	    (debug_view     && section == GEDIT_DEBUG_VIEW)     ||
	    (debug_undo     && section == GEDIT_DEBUG_UNDO)     ||
	    (debug_search   && section == GEDIT_DEBUG_SEARCH)   ||
	    (debug_print    && section == GEDIT_DEBUG_PRINT)    ||
	    (debug_prefs    && section == GEDIT_DEBUG_PREFS)    ||
	    (debug_plugins  && section == GEDIT_DEBUG_PLUGINS)  ||
	    (debug_file     && section == GEDIT_DEBUG_FILE)     ||
	    (debug_document && section == GEDIT_DEBUG_DOCUMENT) ||
	    (debug_commands && section == GEDIT_DEBUG_COMMANDS) ||
	    (debug_recent   && section == GEDIT_DEBUG_RECENT)   ||
	    (debug_window   && section == GEDIT_DEBUG_WINDOW) )
		g_print ("%s:%d (%s) %s\n", file, line, function, msg);
	
	g_free (msg);
}
