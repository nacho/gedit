/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 	- Undo/Redo interface
 *
 * gEdit
 * Copyright (C) 1999 Alex Roberts and Evan Lawrence
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __GE_UNDO_H__
#define __GE_UNDO_H__

/* Actions */
#define	INSERT	0
#define	DELETE	1

typedef struct _gedit_undo {
	gchar *text;		/* The text data */
	/* The position in the document */
	gint start_pos;
	gint end_pos;
		
	gint action;	/* whether the user has inserted or deleted */
	gint status;	/* the changed status of the document used with this node */

}gedit_undo;


/* add an undo node */
extern void gedit_undo_add (gchar*, gint, gint, gint, gedit_document*);
/* clear the list */
extern void gedit_undo_reset	(GList*);
/* callbacks to do the undo or the redo */
extern void gedit_undo_do		(GtkWidget*, gpointer);
extern void gedit_undo_redo	(GtkWidget*, gpointer);
/* let the user see the undo history of the docu,emt */
extern void gedit_undo_history	(GtkWidget*, gpointer*);

#endif /* __GE_UNDO_H__ */
