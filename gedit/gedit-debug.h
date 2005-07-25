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


/* G_GNUC_FUNCTION works only with gcc < 3 at the moment... */
#if defined (__GNUC__) && (__GNUC__ < 3)
#define GEDIT_FUNCTION         __FUNCTION__
#elif defined (__GNUC__)
#define GEDIT_FUNCTION         __func__
#else   /* !__GNUC__ */
#define GEDIT_FUNCTION         ""
#endif  /* !__GNUC__ */

/*
 * Set an environmental var of the same name to turn on
 * debugging output. Setting GEDIT_DEBUG will turn on all
 * sections.
 */
typedef enum {
	GEDIT_NO_DEBUG       = 0,
	GEDIT_DEBUG_VIEW     = 1 << 0,
	GEDIT_DEBUG_UNDO     = 1 << 1,
	GEDIT_DEBUG_SEARCH   = 1 << 2,
	GEDIT_DEBUG_PRINT    = 1 << 3,
	GEDIT_DEBUG_PREFS    = 1 << 4,
	GEDIT_DEBUG_PLUGINS  = 1 << 5,
	GEDIT_DEBUG_FILE     = 1 << 6,
	GEDIT_DEBUG_DOCUMENT = 1 << 7,
	GEDIT_DEBUG_RECENT   = 1 << 8,
	GEDIT_DEBUG_COMMANDS = 1 << 9,
	GEDIT_DEBUG_MDI      = 1 << 10,
	GEDIT_DEBUG_SESSION  = 1 << 11,
	GEDIT_DEBUG_UTILS    = 1 << 12,
	GEDIT_DEBUG_METADATA = 1 << 13
} GeditDebugSection;


#define	DEBUG_VIEW	GEDIT_DEBUG_VIEW,    __FILE__, __LINE__, GEDIT_FUNCTION
#define	DEBUG_UNDO	GEDIT_DEBUG_UNDO,    __FILE__, __LINE__, GEDIT_FUNCTION
#define	DEBUG_SEARCH	GEDIT_DEBUG_SEARCH,  __FILE__, __LINE__, GEDIT_FUNCTION
#define	DEBUG_PRINT	GEDIT_DEBUG_PRINT,   __FILE__, __LINE__, GEDIT_FUNCTION
#define	DEBUG_PREFS	GEDIT_DEBUG_PREFS,   __FILE__, __LINE__, GEDIT_FUNCTION
#define	DEBUG_PLUGINS	GEDIT_DEBUG_PLUGINS, __FILE__, __LINE__, GEDIT_FUNCTION
#define	DEBUG_FILE	GEDIT_DEBUG_FILE,    __FILE__, __LINE__, GEDIT_FUNCTION
#define	DEBUG_DOCUMENT	GEDIT_DEBUG_DOCUMENT,__FILE__, __LINE__, GEDIT_FUNCTION
#define	DEBUG_RECENT	GEDIT_DEBUG_RECENT,  __FILE__, __LINE__, GEDIT_FUNCTION
#define	DEBUG_COMMANDS	GEDIT_DEBUG_COMMANDS,__FILE__, __LINE__, GEDIT_FUNCTION
#define	DEBUG_MDI	GEDIT_DEBUG_MDI,     __FILE__, __LINE__, GEDIT_FUNCTION
#define	DEBUG_SESSION	GEDIT_DEBUG_SESSION, __FILE__, __LINE__, GEDIT_FUNCTION
#define	DEBUG_UTILS	GEDIT_DEBUG_UTILS,   __FILE__, __LINE__, GEDIT_FUNCTION
#define	DEBUG_METADATA	GEDIT_DEBUG_METADATA,__FILE__, __LINE__, GEDIT_FUNCTION

void gedit_debug_init (void);

void gedit_debug (GeditDebugSection section, const gchar *file,
		  gint line, const gchar* function, const gchar* format, ...);

#endif /* __GEDIT_DEBUG_H__ */
