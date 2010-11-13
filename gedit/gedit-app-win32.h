/*
 * gedit-app-win32.h
 * This file is part of gedit
 *
 * Copyright (C) 2010 - Jesse van den Kieboom
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

#ifndef __GEDIT_APP_WIN32_H__
#define __GEDIT_APP_WIN32_H__

#include "gedit-app.h"

G_BEGIN_DECLS

#define GEDIT_TYPE_APP_WIN32		(gedit_app_win32_get_type ())
#define GEDIT_APP_WIN32(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_APP_WIN32, GeditAppWin32))
#define GEDIT_APP_WIN32_CONST(obj)	(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_APP_WIN32, GeditAppWin32 const))
#define GEDIT_APP_WIN32_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GEDIT_TYPE_APP_WIN32, GeditAppWin32Class))
#define GEDIT_IS_APP_WIN32(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEDIT_TYPE_APP_WIN32))
#define GEDIT_IS_APP_WIN32_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEDIT_TYPE_APP_WIN32))
#define GEDIT_APP_WIN32_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GEDIT_TYPE_APP_WIN32, GeditAppWin32Class))

typedef struct _GeditAppWin32		GeditAppWin32;
typedef struct _GeditAppWin32Class	GeditAppWin32Class;
typedef struct _GeditAppWin32Private	GeditAppWin32Private;

struct _GeditAppWin32
{
	GeditApp parent;
};

struct _GeditAppWin32Class
{
	GeditAppClass parent_class;
};

GType	 gedit_app_win32_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __GEDIT_APP_WIN32_H__ */

/* ex:set ts=8 noet: */
