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

#include <gnome.h>
#include <config.h>
#include "main.h"
#include "gE_undo.h"
#include "gE_mdi.h"
#include "gE_window.h"


void gE_undo_add (gchar *text, gint start_pos, gint end_pos, gint action, gE_document *doc)
{

	gE_undo *undo;
	
	undo = g_new(gE_undo, 1);
	
	undo->text = text;
	undo->start_pos = start_pos;
	undo->end_pos = end_pos;
	undo->action = action;
	undo->status = doc->changed;
	
	/* nuke the redo list, if its available */
	if (doc->redo) {
	
		g_list_free (doc->redo);
	
	}

	g_message ("gE_undo_add: Adding to Undo list..");
	
	doc->undo = g_list_prepend (doc->undo, undo);

}

