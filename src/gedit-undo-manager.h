/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-undo-manager.h
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

#ifndef __GEDIT_UNDO_MANAGER_H__
#define __GEDIT_UNDO_MANAGER_H__

#include "gedit-document.h"

#define GEDIT_TYPE_UNDO_MANAGER             	(gedit_undo_manager_get_type ())
#define GEDIT_UNDO_MANAGER(obj)			(GTK_CHECK_CAST ((obj), GEDIT_TYPE_UNDO_MANAGER, GeditUndoManager))
#define GEDIT_UNDO_MANAGER_CLASS(klass)		(GTK_CHECK_CLASS_CAST ((klass), GEDIT_TYPE_UNDO_MANAGER, GeditUndoManagerClass))
#define GEDIT_IS_UNDO_MANAGER(obj)		(GTK_CHECK_TYPE ((obj), GEDIT_TYPE_UNDO_MANAGER))
#define GEDIT_IS_UNDO_MANAGER_CLASS(klass)  	(GTK_CHECK_CLASS_TYPE ((klass), GEDIT_TYPE_UNDO_MANAGER))
#define GEDIT_UNDO_MANAGER_GET_CLASS(obj)  	(GTK_CHECK_GET_CLASS ((obj), GEDIT_TYPE_UNDO_MANAGER, GeditUndoManagerClass))


typedef struct _GeditUndoManager        	GeditUndoManager;
typedef struct _GeditUndoManagerClass 		GeditUndoManagerClass;

typedef struct _GeditUndoManagerPrivate 	GeditUndoManagerPrivate;

struct _GeditUndoManager
{
	GObject base;
	
	GeditUndoManagerPrivate *priv;
};

struct _GeditUndoManagerClass
{
	GObjectClass parent_class;

	/* Signals */
	void (*can_undo)( GeditUndoManager *um, gboolean can_undo);
    	void (*can_redo)( GeditUndoManager *um, gboolean can_redo);
};

GType        		gedit_undo_manager_get_type	(void) G_GNUC_CONST;

GeditUndoManager* 	gedit_undo_manager_new 		(GeditDocument *document);

gboolean		gedit_undo_manager_can_undo	(const GeditUndoManager *um);
gboolean		gedit_undo_manager_can_redo 	(const GeditUndoManager *um);

void			gedit_undo_manager_undo 	(GeditUndoManager *um);
void			gedit_undo_manager_redo 	(GeditUndoManager *um);

void	gedit_undo_manager_begin_not_undoable_action 	(GeditUndoManager *um);
void	gedit_undo_manager_end_not_undoable_action 	(GeditUndoManager *um);

#endif /* __GEDIT_UNDO_MANAGER_H__ */


