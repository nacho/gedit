/*
 * gedit-app-x11.h
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

#include "gedit-app-x11.h"
#include "eggdesktopfile.h"

#define GEDIT_APP_X11_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE((object), GEDIT_TYPE_APP_X11, GeditAppX11Private))

G_DEFINE_TYPE (GeditAppX11, gedit_app_x11, GEDIT_TYPE_APP)

static void
gedit_app_x11_finalize (GObject *object)
{
	G_OBJECT_CLASS (gedit_app_x11_parent_class)->finalize (object);
}

static void
gedit_app_x11_class_init (GeditAppX11Class *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = gedit_app_x11_finalize;
}

static void
gedit_app_x11_init (GeditAppX11 *self)
{
	egg_set_desktop_file (DATADIR "/applications/gedit.desktop");
}

/* ex:ts=8:noet: */
