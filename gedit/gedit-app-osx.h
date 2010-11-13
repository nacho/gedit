/*
 * gedit-app-osx.h
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

#ifndef __GEDIT_APP_OSX_H__
#define __GEDIT_APP_OSX_H__

#include "gedit-app.h"
#include <ige-mac-integration.h>

G_BEGIN_DECLS

#define GEDIT_TYPE_APP_OSX		(gedit_app_osx_get_type ())
#define GEDIT_APP_OSX(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_APP_OSX, GeditAppOSX))
#define GEDIT_APP_OSX_CONST(obj)	(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_APP_OSX, GeditAppOSX const))
#define GEDIT_APP_OSX_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GEDIT_TYPE_APP_OSX, GeditAppOSXClass))
#define GEDIT_IS_APP_OSX(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEDIT_TYPE_APP_OSX))
#define GEDIT_IS_APP_OSX_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEDIT_TYPE_APP_OSX))
#define GEDIT_APP_OSX_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GEDIT_TYPE_APP_OSX, GeditAppOSXClass))

typedef struct _GeditAppOSX		GeditAppOSX;
typedef struct _GeditAppOSXClass	GeditAppOSXClass;
typedef struct _GeditAppOSXPrivate	GeditAppOSXPrivate;

struct _GeditAppOSX
{
	GeditApp parent;
};

struct _GeditAppOSXClass
{
	GeditAppClass parent_class;
};

GType		 gedit_app_osx_get_type		(void) G_GNUC_CONST;
void		 gedit_app_osx_set_window_title	(GeditAppOSX   *app,
						 GeditWindow   *window,
						 const gchar   *title,
						 GeditDocument *document);
gboolean	 gedit_app_osx_show_url		(GeditAppOSX   *app,
						 const gchar   *url);
gboolean	 gedit_app_osx_show_help	(GeditAppOSX   *app,
						 const gchar   *link_id);

G_END_DECLS

#endif /* __GEDIT_APP_OSX_H__ */

/* ex:set ts=8 noet: */
