/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-debug.h
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

#ifndef __GEDIT_DEBUG_H__
#define __GEDIT_DEBUG_H__

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
	GEDIT_DEBUG_MDI,
	GEDIT_DEBUG_SESSION
} GeditDebugSection;

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
extern gint debug_mdi;
extern gint debug_session;

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
#define	DEBUG_MDI	GEDIT_DEBUG_MDI,     __FILE__, __LINE__, __FUNCTION__
#define	DEBUG_SESSION	GEDIT_DEBUG_SESSION, __FILE__, __LINE__, __FUNCTION__

void gedit_debug (gint section, gchar *file,
		  gint line, gchar* function, gchar* format, ...);

#endif /* __GEDIT_DEBUG_H__ */
