/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* Cleaned 10-00 by Chema */
/*
 * gedit
 *
 * Copyright (C) 2000 Chema Celorio
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __GEDIT_UNDO_H__
#define __GEDIT_UNDO_H__

#include "document.h"
#include "view.h"

typedef enum {
	GEDIT_UNDO_ACTION_INSERT,
	GEDIT_UNDO_ACTION_DELETE,
	GEDIT_UNDO_ACTION_REPLACE_INSERT,
	GEDIT_UNDO_ACTION_REPLACE_DELETE
} GeditUndoAction;

typedef enum {
	GEDIT_UNDO_STATE_TRUE,
	GEDIT_UNDO_STATE_FALSE,
	GEDIT_UNDO_STATE_UNCHANGED,
	GEDIT_UNDO_STATE_REFRESH,
} GeditUndoState;

void gedit_undo_add       (const gchar *text, gint start_pos, gint end_pos, GeditUndoAction action, GeditDocument *doc, GeditView *view);
void gedit_undo_undo	  (GtkWidget*, gpointer);
void gedit_undo_redo	  (GtkWidget*, gpointer);
void gedit_undo_free_list (GList **list_pointer);

#endif /* __GEDIT_UNDO_H__ */
