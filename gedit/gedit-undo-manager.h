/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-undo-manager.h
 * This file is part of gedit
 *
 * Copyright (C) 1998, 1999 Alex Roberts, Evan Lawrence
 * Copyright (C) 2000, 2001 Chema Celorio, Paolo Maggi
 * Copyright (C) 2002, 2003 Paolo Maggi
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef __GEDIT_UNDO_MANAGER_H__
#define __GEDIT_UNDO_MANAGER_H__

#include <glib-object.h>
#include <gtksourceview/gtksourcebuffer.h>

G_BEGIN_DECLS

#define GEDIT_TYPE_UNDO_MANAGER			(gedit_undo_manager_get_type ())
#define GEDIT_UNDO_MANAGER(obj)			(G_TYPE_CHECK_INSTANCE_CAST((obj), GEDIT_TYPE_UNDO_MANAGER, GeditUndoManager))
#define GEDIT_UNDO_MANAGER_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), GEDIT_TYPE_UNDO_MANAGER, GeditUndoManagerClass))
#define GEDIT_IS_UNDO_MANAGER(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEDIT_TYPE_UNDO_MANAGER))
#define GEDIT_IS_UNDO_MANAGER_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEDIT_TYPE_UNDO_MANAGER))
#define GEDIT_UNDO_MANAGER_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GEDIT_TYPE_UNDO_MANAGER, GeditUndoManagerClass))

typedef struct _GeditUndoManager        	GeditUndoManager;
typedef struct _GeditUndoManagerClass 		GeditUndoManagerClass;
typedef struct _GeditUndoManagerPrivate 	GeditUndoManagerPrivate;

struct _GeditUndoManager
{
	GObject parent;

	GeditUndoManagerPrivate *priv;
};

struct _GeditUndoManagerClass
{
	GObjectClass parent_class;
};

GType gedit_undo_manager_get_type (void) G_GNUC_CONST;

GtkSourceUndoManager * gedit_undo_manager_new (GtkSourceBuffer *buffer);

void gedit_undo_manager_set_max_undo_levels (GeditUndoManager *manager,
                                             gint              max_undo_levels);

G_END_DECLS

#endif /* __GEDIT_UNDO_MANAGER_H__ */


