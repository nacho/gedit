/*
 * gedit-dbus.h
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

#ifndef __GEDIT_DBUS_H__
#define __GEDIT_DBUS_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define GEDIT_TYPE_DBUS			(gedit_dbus_get_type ())
#define GEDIT_DBUS(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_DBUS, GeditDBus))
#define GEDIT_DBUS_CONST(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_DBUS, GeditDBus const))
#define GEDIT_DBUS_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), GEDIT_TYPE_DBUS, GeditDBusClass))
#define GEDIT_IS_DBUS(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEDIT_TYPE_DBUS))
#define GEDIT_IS_DBUS_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEDIT_TYPE_DBUS))
#define GEDIT_DBUS_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GEDIT_TYPE_DBUS, GeditDBusClass))

typedef struct _GeditDBus		GeditDBus;
typedef struct _GeditDBusClass		GeditDBusClass;
typedef struct _GeditDBusPrivate	GeditDBusPrivate;

typedef enum
{
	GEDIT_DBUS_RESULT_SUCCESS,
	GEDIT_DBUS_RESULT_FAILED,
	GEDIT_DBUS_RESULT_PROCEED,
	GEDIT_DBUS_RESULT_PROCEED_SERVICE
} GeditDBusResult;

struct _GeditDBus {
	GObject parent;

	GeditDBusPrivate *priv;
};

struct _GeditDBusClass {
	GObjectClass parent_class;
};

GType gedit_dbus_get_type (void) G_GNUC_CONST;
GeditDBus *gedit_dbus_new (void);

GeditDBusResult gedit_dbus_run (GeditDBus *bus);

G_END_DECLS

#endif /* __GEDIT_DBUS_H__ */

/* ex:ts=8:noet: */
