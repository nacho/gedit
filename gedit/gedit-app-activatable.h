/*
 * gedit-app-activatable.h
 * This file is part of gedit
 *
 * Copyright (C) 2010 - Steve Frécinaux
 * Copyright (C) 2010 - Jesse van den Kieboom
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Library General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __GEDIT_APP_ACTIVATABLE_H__
#define __GEDIT_APP_ACTIVATABLE_H__

#include <glib-object.h>

G_BEGIN_DECLS

/*
 * Type checking and casting macros
 */
#define GEDIT_TYPE_APP_ACTIVATABLE		(gedit_app_activatable_get_type ())
#define GEDIT_APP_ACTIVATABLE(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_APP_ACTIVATABLE, GeditAppActivatable))
#define GEDIT_APP_ACTIVATABLE_IFACE(obj)	(G_TYPE_CHECK_CLASS_CAST ((obj), GEDIT_TYPE_APP_ACTIVATABLE, GeditAppActivatableInterface))
#define GEDIT_IS_APP_ACTIVATABLE(obj)	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEDIT_TYPE_APP_ACTIVATABLE))
#define GEDIT_APP_ACTIVATABLE_GET_IFACE(obj)	(G_TYPE_INSTANCE_GET_INTERFACE ((obj), GEDIT_TYPE_APP_ACTIVATABLE, GeditAppActivatableInterface))

typedef struct _GeditAppActivatable           GeditAppActivatable; /* dummy typedef */
typedef struct _GeditAppActivatableInterface  GeditAppActivatableInterface;

struct _GeditAppActivatableInterface
{
	GTypeInterface g_iface;

	/* Virtual public methods */
	void	(*activate)		(GeditAppActivatable *activatable);
	void	(*deactivate)		(GeditAppActivatable *activatable);
};

/*
 * Public methods
 */
GType	 gedit_app_activatable_get_type	(void)  G_GNUC_CONST;

void	 gedit_app_activatable_activate	(GeditAppActivatable *activatable);
void	 gedit_app_activatable_deactivate	(GeditAppActivatable *activatable);

G_END_DECLS

#endif /* __GEDIT_APP_ACTIVATABLE_H__ */
