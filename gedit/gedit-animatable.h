/*
 * gedit-animatable.h
 * This file is part of gedit
 *
 * Copyright (C) 2011 - Ignacio Casal Quinteiro
 *
 * gedit is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * gedit is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef __GEDIT_ANIMATABLE_H__
#define __GEDIT_ANIMATABLE_H__

#include <glib-object.h>

G_BEGIN_DECLS

/*
 * Type checking and casting macros
 */
#define GEDIT_TYPE_ANIMATABLE		(gedit_animatable_get_type ())
#define GEDIT_ANIMATABLE(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_ANIMATABLE, GeditAnimatable))
#define GEDIT_ANIMATABLE_IFACE(obj)	(G_TYPE_CHECK_CLASS_CAST ((obj), GEDIT_TYPE_ANIMATABLE, GeditAnimatableInterface))
#define GEDIT_IS_ANIMATABLE(obj)	(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEDIT_TYPE_ANIMATABLE))
#define GEDIT_ANIMATABLE_GET_IFACE(obj)	(G_TYPE_INSTANCE_GET_INTERFACE ((obj), GEDIT_TYPE_ANIMATABLE, GeditAnimatableInterface))

typedef struct _GeditAnimatable           GeditAnimatable; /* dummy typedef */
typedef struct _GeditAnimatableInterface  GeditAnimatableInterface;

struct _GeditAnimatableInterface
{
	GTypeInterface g_iface;

	/* Virtual public methods */
};

/*
 * Public methods
 */
GType	 gedit_animatable_get_type	(void)  G_GNUC_CONST;

G_END_DECLS

#endif /* __GEDIT_ANIMATABLE_H__ */

/* ex:set ts=8 noet: */
