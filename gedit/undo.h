/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit
 *
 * Copyright (C) 1998, 1999 Alex Roberts, Evan Lawrence
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

#ifndef __UNDO_H__
#define __UNDO_H__

#include "document.h"
#include "view.h"

/* Actions */
#define	GEDIT_UNDO_INSERT	0
#define	GEDIT_UNDO_DELETE	1

typedef struct _gedit_undo gedit_undo;

struct _gedit_undo
{
	gchar *text;	/* The text data */
	gint start_pos;	/* The position in the document */
	gint end_pos;
	gint action;	/* whether the user has inserted or deleted */
	gint status;	/* the changed status of the document used with this node */
	gfloat window_position;
	gint mergeable;
};

extern void gedit_undo_add      (gchar *text, gint start_pos, gint end_pos, gint action, Document *doc, View *view);
extern void gedit_undo_reset	(GList*);
extern void gedit_undo_do	(GtkWidget*, gpointer);
extern void gedit_undo_redo	(GtkWidget*, gpointer);
extern void gedit_undo_history	(GtkWidget*, gpointer*);

#endif /* __UNDO_H__ */
