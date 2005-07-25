/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-debug.c
 * This file is part of gedit
 *
 * Copyright (C) 1998, 1999 Alex Roberts, Evan Lawrence
 * Copyright (C) 2000, 2001 Chema Celorio, Paolo Maggi 
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
 * Boston, MA 02111-1307, USA. * *
 */
 
/*
 * Modified by the gedit Team, 1998-2001. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <stdio.h>
#include "gedit-debug.h"

static GeditDebugSection debug = GEDIT_NO_DEBUG;

void
gedit_debug_init ()
{
	if (g_getenv ("GEDIT_DEBUG") != NULL)
	{
		/* enable all debugging */
		debug = ~GEDIT_NO_DEBUG;
		return;
	}

	if (g_getenv ("GEDIT_DEBUG_VIEW") != NULL)
		debug = debug | GEDIT_DEBUG_VIEW;
	if (g_getenv ("GEDIT_DEBUG_UNDO") != NULL)
		debug = debug | GEDIT_DEBUG_UNDO;
	if (g_getenv ("GEDIT_DEBUG_SEARCH") != NULL)
		debug = debug | GEDIT_DEBUG_SEARCH;
	if (g_getenv ("GEDIT_DEBUG_PREFS") != NULL)
		debug = debug | GEDIT_DEBUG_PREFS;
	if (g_getenv ("GEDIT_DEBUG_PRINT") != NULL)
		debug = debug | GEDIT_DEBUG_PRINT;
	if (g_getenv ("GEDIT_DEBUG_PLUGINS") != NULL)
		debug = debug | GEDIT_DEBUG_PLUGINS;
	if (g_getenv ("GEDIT_DEBUG_FILE") != NULL)
		debug = debug | GEDIT_DEBUG_FILE;
	if (g_getenv ("GEDIT_DEBUG_DOCUMENT") != NULL)
		debug = debug | GEDIT_DEBUG_DOCUMENT;
	if (g_getenv ("GEDIT_DEBUG_COMMANDS") != NULL)
		debug = debug | GEDIT_DEBUG_COMMANDS;
	if (g_getenv ("GEDIT_DEBUG_RECENT") != NULL)
		debug = debug | GEDIT_DEBUG_RECENT;
	if (g_getenv ("GEDIT_DEBUG_MDI") != NULL)
		debug = debug | GEDIT_DEBUG_MDI;
	if (g_getenv ("GEDIT_DEBUG_SESSION") != NULL)
		debug = debug | GEDIT_DEBUG_SESSION;
	if (g_getenv ("GEDIT_DEBUG_UTILS") != NULL)
		debug = debug | GEDIT_DEBUG_UTILS;
	if (g_getenv ("GEDIT_DEBUG_METADATA") != NULL)
		debug = debug | GEDIT_DEBUG_METADATA;
}

void
gedit_debug (GeditDebugSection section, const gchar *file,
	     gint line, const gchar* function, const gchar* format, ...)
{
	if  (debug & section)
	{
		va_list args;
		gchar *msg;

		g_return_if_fail (format != NULL);

		va_start (args, format);
		msg = g_strdup_vprintf (format, args);
		va_end (args);

		g_print ("%s:%d (%s) %s\n", file, line, function, msg);
		g_free (msg);

		fflush (stdout);
	}
}

