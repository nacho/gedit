/*
 * gedit-view-holder.h
 * This file is part of gedit
 *
 * Copyright (C) 2010 - Ignacio Casal Quinteiro
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

#ifndef __GEDIT_VIEW_HOLDER_H__
#define __GEDIT_VIEW_HOLDER_H__

#include <gtk/gtk.h>
#include "gedit-document.h"
#include "gedit-view.h"

G_BEGIN_DECLS

#define GEDIT_TYPE_VIEW_HOLDER			(gedit_view_holder_get_type ())
#define GEDIT_VIEW_HOLDER(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_VIEW_HOLDER, GeditViewHolder))
#define GEDIT_VIEW_HOLDER_CONST(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_VIEW_HOLDER, GeditViewHolder const))
#define GEDIT_VIEW_HOLDER_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), GEDIT_TYPE_VIEW_HOLDER, GeditViewHolderClass))
#define GEDIT_IS_VIEW_HOLDER(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEDIT_TYPE_VIEW_HOLDER))
#define GEDIT_IS_VIEW_HOLDER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEDIT_TYPE_VIEW_HOLDER))
#define GEDIT_VIEW_HOLDER_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GEDIT_TYPE_VIEW_HOLDER, GeditViewHolderClass))

typedef struct _GeditViewHolder		GeditViewHolder;
typedef struct _GeditViewHolderClass	GeditViewHolderClass;
typedef struct _GeditViewHolderPrivate	GeditViewHolderPrivate;

struct _GeditViewHolder
{
	GtkVBox parent;
	
	GeditViewHolderPrivate *priv;
};

struct _GeditViewHolderClass
{
	GtkVBoxClass parent_class;
};

GType		 gedit_view_holder_get_type	(void) G_GNUC_CONST;

GeditViewHolder	*gedit_view_holder_new		(void);

GeditDocument	*gedit_view_holder_get_document	(GeditViewHolder *holder);

GeditView	*gedit_view_holder_get_view	(GeditViewHolder *holder);

G_END_DECLS

#endif /* __GEDIT_VIEW_HOLDER_H__ */
