/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-undo-manager.c
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
 * Boston, MA 02111-1307, USA.
 */
 
/*
 * Modified by the gedit Team, 1998-2001. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <stdlib.h>
#include <string.h>

#include "gedit-undo-manager.h"
#include "gedit-debug.h"
#include "gedit-prefs-manager.h"
#include "gedit-marshal.h"

typedef struct _GeditUndoAction  		GeditUndoAction;
typedef struct _GeditUndoInsertAction		GeditUndoInsertAction;
typedef struct _GeditUndoDeleteAction		GeditUndoDeleteAction;

typedef enum {
	GEDIT_UNDO_ACTION_INSERT,
	GEDIT_UNDO_ACTION_DELETE
} GeditUndoActionType;

/* 
 * We use offsets instead of GtkTextIters because the last ones
 * require to much memory in this context without giving us any advantage.
 */ 

struct _GeditUndoInsertAction
{
	gint   pos; 
	gchar *text;
	gint   length;
	gint   chars;
};

struct _GeditUndoDeleteAction
{
	gint   start;
	gint   end;
	gchar *text;
};

struct _GeditUndoAction
{
	GeditUndoActionType action_type;
	
	union {
		GeditUndoInsertAction	insert;
		GeditUndoDeleteAction   delete;
	} action;

	gboolean mergeable;

	gint order_in_group;
};

struct _GeditUndoManagerPrivate
{
	GeditDocument*  document;
	
	GList*		actions;
	gint 		next_redo;	

	gint 		actions_in_current_group;
	
	gboolean 	can_undo;
	gboolean	can_redo;

	gint		running_not_undoable_actions;

	gint		num_of_groups;
};

enum {
	CAN_UNDO,
	CAN_REDO,
	LAST_SIGNAL
};

static void gedit_undo_manager_class_init 		(GeditUndoManagerClass 	*klass);
static void gedit_undo_manager_init 			(GeditUndoManager 	*um);
static void gedit_undo_manager_finalize 		(GObject 		*object);

static void gedit_undo_manager_insert_text_handler 	(GtkTextBuffer *buffer, GtkTextIter *pos,
		                             		 const gchar *text, gint length, 
							 GeditUndoManager *um);
static void gedit_undo_manager_delete_range_handler 	(GtkTextBuffer *buffer, GtkTextIter *start,
                        		      		 GtkTextIter *end, GeditUndoManager *um);
static void gedit_undo_manager_begin_user_action_handler (GtkTextBuffer *buffer, GeditUndoManager *um);
static void gedit_undo_manager_end_user_action_handler   (GtkTextBuffer *buffer, GeditUndoManager *um);

static void gedit_undo_manager_free_action_list 	(GeditUndoManager *um);

static void gedit_undo_manager_add_action 		(GeditUndoManager *um, 
		                                         GeditUndoAction undo_action);
static void gedit_undo_manager_free_first_n_actions 	(GeditUndoManager *um, gint n);
static void gedit_undo_manager_check_list_size 		(GeditUndoManager *um);

static gboolean gedit_undo_manager_merge_action 	(GeditUndoManager *um, 
		                                         GeditUndoAction *undo_action);

static GObjectClass 	*parent_class 				= NULL;
static guint 		undo_manager_signals [LAST_SIGNAL] 	= { 0 };

GType
gedit_undo_manager_get_type (void)
{
	static GType undo_manager_type = 0;

  	if (undo_manager_type == 0)
    	{
      		static const GTypeInfo our_info =
      		{
        		sizeof (GeditDocumentClass),
        		NULL,		/* base_init */
        		NULL,		/* base_finalize */
        		(GClassInitFunc) gedit_undo_manager_class_init,
        		NULL,           /* class_finalize */
        		NULL,           /* class_data */
        		sizeof (GeditDocument),
        		0,              /* n_preallocs */
        		(GInstanceInitFunc) gedit_undo_manager_init
      		};

      		undo_manager_type = g_type_register_static (G_TYPE_OBJECT,
                					"GeditUndoManager",
                                           	 	&our_info,
                                           		0);
    	}

	return undo_manager_type;
}

static void
gedit_undo_manager_class_init (GeditUndoManagerClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

  	parent_class = g_type_class_peek_parent (klass);

  	object_class->finalize = gedit_undo_manager_finalize;

        klass->can_undo 	= NULL;
	klass->can_redo 	= NULL;
	
	undo_manager_signals[CAN_UNDO] =
   		g_signal_new ("can_undo",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GeditUndoManagerClass, can_undo),
			      NULL, NULL,
			      gedit_marshal_VOID__BOOLEAN,
			      G_TYPE_NONE,
			      1,
			      G_TYPE_BOOLEAN);

	undo_manager_signals[CAN_REDO] =
   		g_signal_new ("can_redo",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GeditUndoManagerClass, can_redo),
			      NULL, NULL,
			      gedit_marshal_VOID__BOOLEAN,
			      G_TYPE_NONE,
			      1,
			      G_TYPE_BOOLEAN);

}

static void
gedit_undo_manager_init (GeditUndoManager *um)
{
	gedit_debug (DEBUG_UNDO, "");

	um->priv = g_new0 (GeditUndoManagerPrivate, 1);

	um->priv->actions = NULL;
	um->priv->next_redo = 0;

	um->priv->can_undo = FALSE;
	um->priv->can_redo = FALSE;

	um->priv->running_not_undoable_actions = 0;

	um->priv->num_of_groups = 0;
}

static void
gedit_undo_manager_finalize (GObject *object)
{
	GeditUndoManager *um;

	gedit_debug (DEBUG_UNDO, "");

	g_return_if_fail (object != NULL);
	g_return_if_fail (GEDIT_IS_UNDO_MANAGER (object));
	
   	um = GEDIT_UNDO_MANAGER (object);

	g_return_if_fail (um->priv != NULL);

	if (um->priv->actions != NULL)
	{
		gedit_undo_manager_free_action_list (um);
	}

	g_signal_handlers_disconnect_by_func (G_OBJECT (um->priv->document),
			  G_CALLBACK (gedit_undo_manager_delete_range_handler), 
			  um);

	g_signal_handlers_disconnect_by_func (G_OBJECT (um->priv->document),
			  G_CALLBACK (gedit_undo_manager_insert_text_handler), 
			  um);
	
	g_signal_handlers_disconnect_by_func (G_OBJECT (um->priv->document),
			  G_CALLBACK (gedit_undo_manager_begin_user_action_handler), 
			  um);

	g_signal_handlers_disconnect_by_func (G_OBJECT (um->priv->document),
			  G_CALLBACK (gedit_undo_manager_end_user_action_handler), 
			  um);

	g_free (um->priv);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

GeditUndoManager*
gedit_undo_manager_new (GeditDocument* document)
{
 	GeditUndoManager *um;

	gedit_debug (DEBUG_UNDO, "");

	um = GEDIT_UNDO_MANAGER (g_object_new (GEDIT_TYPE_UNDO_MANAGER, NULL));

	g_return_val_if_fail (um->priv != NULL, NULL);
  	um->priv->document = document;

	g_signal_connect (G_OBJECT (document), "insert_text",
			  G_CALLBACK (gedit_undo_manager_insert_text_handler), 
			  um);

	g_signal_connect (G_OBJECT (document), "delete_range",
			  G_CALLBACK (gedit_undo_manager_delete_range_handler), 
			  um);

	g_signal_connect (G_OBJECT (document), "begin_user_action",
			  G_CALLBACK (gedit_undo_manager_begin_user_action_handler), 
			  um);

	g_signal_connect (G_OBJECT (document), "end_user_action",
			  G_CALLBACK (gedit_undo_manager_end_user_action_handler), 
			  um);

	return um;
}

void 
gedit_undo_manager_begin_not_undoable_action (GeditUndoManager *um)
{
	gedit_debug (DEBUG_UNDO, "");

	g_return_if_fail (GEDIT_IS_UNDO_MANAGER (um));
	g_return_if_fail (um->priv != NULL);

	++um->priv->running_not_undoable_actions;
}

static void 
gedit_undo_manager_end_not_undoable_action_internal (GeditUndoManager *um)
{
	gedit_debug (DEBUG_UNDO, "");

	g_return_if_fail (GEDIT_IS_UNDO_MANAGER (um));
	g_return_if_fail (um->priv != NULL);

	g_return_if_fail (um->priv->running_not_undoable_actions > 0);
	
	--um->priv->running_not_undoable_actions;
}

void 
gedit_undo_manager_end_not_undoable_action (GeditUndoManager *um)
{
	gedit_debug (DEBUG_UNDO, "");

	g_return_if_fail (GEDIT_IS_UNDO_MANAGER (um));
	g_return_if_fail (um->priv != NULL);

	gedit_undo_manager_end_not_undoable_action_internal (um);

	if (um->priv->running_not_undoable_actions == 0)
	{	
		gedit_undo_manager_free_action_list (um);
	
		um->priv->next_redo = -1;	

		if (um->priv->can_undo)
		{
			um->priv->can_undo = FALSE;
			g_signal_emit (G_OBJECT (um), undo_manager_signals [CAN_UNDO], 0, FALSE);
		}

		if (um->priv->can_redo)
		{
			um->priv->can_redo = FALSE;
			g_signal_emit (G_OBJECT (um), undo_manager_signals [CAN_REDO], 0, FALSE);
		}
	}
}


gboolean
gedit_undo_manager_can_undo (const GeditUndoManager *um)
{
	gedit_debug (DEBUG_UNDO, "");

	g_return_val_if_fail (GEDIT_IS_UNDO_MANAGER (um), FALSE);
	g_return_val_if_fail (um->priv != NULL, FALSE);

	return um->priv->can_undo;
}

gboolean 
gedit_undo_manager_can_redo (const GeditUndoManager *um)
{
	gedit_debug (DEBUG_UNDO, "");

	g_return_val_if_fail (GEDIT_IS_UNDO_MANAGER (um), FALSE);
	g_return_val_if_fail (um->priv != NULL, FALSE);

	return um->priv->can_redo;
}

void 
gedit_undo_manager_undo (GeditUndoManager *um)
{
	GeditUndoAction *undo_action;

	gedit_debug (DEBUG_UNDO, "");

	g_return_if_fail (GEDIT_IS_UNDO_MANAGER (um));
	g_return_if_fail (um->priv != NULL);
	g_return_if_fail (um->priv->can_undo);
	
	gedit_undo_manager_begin_not_undoable_action (um);

	do
	{
		++um->priv->next_redo;
		
		undo_action = g_list_nth_data (um->priv->actions, um->priv->next_redo);
		g_return_if_fail (undo_action != NULL);

		switch (undo_action->action_type)
		{
			case GEDIT_UNDO_ACTION_DELETE:
				gedit_document_set_cursor (
					um->priv->document, 
					undo_action->action.delete.start);

				gedit_document_insert_text (
					um->priv->document, 
					undo_action->action.delete.start, 
					undo_action->action.delete.text,
					strlen (undo_action->action.delete.text));

								break;
			case GEDIT_UNDO_ACTION_INSERT:
				gedit_document_delete_text (
					um->priv->document, 
					undo_action->action.insert.pos, 
					undo_action->action.insert.pos + 
						undo_action->action.insert.chars); 

				gedit_document_set_cursor (
					um->priv->document, 
					undo_action->action.insert.pos);
				break;

			default:
				g_warning ("This should not happen.");
				return;
		}

	} while (undo_action->order_in_group > 1);

	gedit_undo_manager_end_not_undoable_action_internal (um);
	
	if (!um->priv->can_redo)
	{
		um->priv->can_redo = TRUE;
		g_signal_emit (G_OBJECT (um), undo_manager_signals [CAN_REDO], 0, TRUE);
	}

	if (um->priv->next_redo >= (gint)(g_list_length (um->priv->actions) - 1))
	{
		um->priv->can_undo = FALSE;
		g_signal_emit (G_OBJECT (um), undo_manager_signals [CAN_UNDO], 0, FALSE);
	}
}

void 
gedit_undo_manager_redo (GeditUndoManager *um)
{
	GeditUndoAction *undo_action;

	gedit_debug (DEBUG_UNDO, "");

	g_return_if_fail (GEDIT_IS_UNDO_MANAGER (um));
	g_return_if_fail (um->priv != NULL);
	g_return_if_fail (um->priv->can_redo);
	
	undo_action = g_list_nth_data (um->priv->actions, um->priv->next_redo);
	g_return_if_fail (undo_action != NULL);

	gedit_undo_manager_begin_not_undoable_action (um);

	do
	{
		switch (undo_action->action_type)
		{
			case GEDIT_UNDO_ACTION_DELETE:
				gedit_document_delete_text (
					um->priv->document, 
					undo_action->action.delete.start, 
					undo_action->action.delete.end); 

				gedit_document_set_cursor (
					um->priv->document,
					undo_action->action.delete.start);

				break;
				
			case GEDIT_UNDO_ACTION_INSERT:
				gedit_document_set_cursor (
					um->priv->document,
					undo_action->action.insert.pos);

				gedit_document_insert_text (
					um->priv->document, 
					undo_action->action.insert.pos, 
					undo_action->action.insert.text,
					undo_action->action.insert.length);

				break;

			default:
				g_warning ("This should not happen.");
				return;
		}

		--um->priv->next_redo;

		if (um->priv->next_redo < 0)
			undo_action = NULL;
		else
			undo_action = g_list_nth_data (um->priv->actions, um->priv->next_redo);
		
	} while ((undo_action != NULL) && (undo_action->order_in_group > 1));

	gedit_undo_manager_end_not_undoable_action_internal (um);

	if (um->priv->next_redo < 0)
	{
		um->priv->can_redo = FALSE;
		g_signal_emit (G_OBJECT (um), undo_manager_signals [CAN_REDO], 0, FALSE);
	}

	if (!um->priv->can_undo)
	{
		um->priv->can_undo = TRUE;
		g_signal_emit (G_OBJECT (um), undo_manager_signals [CAN_UNDO], 0, TRUE);
	}

}

static void 
gedit_undo_manager_free_action_list (GeditUndoManager *um)
{
	gint n, len;
	
	gedit_debug (DEBUG_UNDO, "");

	g_return_if_fail (GEDIT_IS_UNDO_MANAGER (um));
	g_return_if_fail (um->priv != NULL);

	if (um->priv->actions == NULL)
	{
		return;
	}
	len = g_list_length (um->priv->actions);
	
	for (n = 0; n < len; n++)
	{
		GeditUndoAction *undo_action = 
			(GeditUndoAction *)(g_list_nth_data (um->priv->actions, n));

		gedit_debug (DEBUG_UNDO, "Free action (type %s) %d/%d", 
				(undo_action->action_type == GEDIT_UNDO_ACTION_INSERT) ? "insert":
				"delete", n, len);

		if (undo_action->action_type == GEDIT_UNDO_ACTION_INSERT)
			g_free (undo_action->action.insert.text);
		else if (undo_action->action_type == GEDIT_UNDO_ACTION_DELETE)
			g_free (undo_action->action.delete.text);
		else
			g_return_if_fail (FALSE);

		if (undo_action->order_in_group == 1)
			--um->priv->num_of_groups;

		g_free (undo_action);
	}

	g_list_free (um->priv->actions);
	um->priv->actions = NULL;	
}

static void 
gedit_undo_manager_insert_text_handler (GtkTextBuffer *buffer, GtkTextIter *pos,
		                             const gchar *text, gint length, GeditUndoManager *um)
{
	GeditUndoAction undo_action;
	
	gedit_debug (DEBUG_UNDO, "");	

	if (um->priv->running_not_undoable_actions > 0)
		return;

	g_return_if_fail (strlen (text) == (guint)length);
	
	undo_action.action_type = GEDIT_UNDO_ACTION_INSERT;

	undo_action.action.insert.pos    = gtk_text_iter_get_offset (pos);
	undo_action.action.insert.text   = (gchar*) text;
	undo_action.action.insert.length = length;
	undo_action.action.insert.chars  = g_utf8_strlen (text, length);

	if ((undo_action.action.insert.chars > 1) || (g_utf8_get_char (text) == '\n'))

	       	undo_action.mergeable = FALSE;
	else
		undo_action.mergeable = TRUE;

	gedit_undo_manager_add_action (um, undo_action);
}

static void 
gedit_undo_manager_delete_range_handler (GtkTextBuffer *buffer, GtkTextIter *start,
                        		      GtkTextIter *end, GeditUndoManager *um)
{
	GeditUndoAction undo_action;

	gedit_debug (DEBUG_UNDO, "");

	if (um->priv->running_not_undoable_actions > 0)
		return;

	undo_action.action_type = GEDIT_UNDO_ACTION_DELETE;

	gtk_text_iter_order (start, end);

	undo_action.action.delete.start  = gtk_text_iter_get_offset (start);
	undo_action.action.delete.end    = gtk_text_iter_get_offset (end);

	undo_action.action.delete.text   = gedit_document_get_chars (
						GEDIT_DOCUMENT (buffer),
						undo_action.action.delete.start,
						undo_action.action.delete.end);

	if (((undo_action.action.delete.end - undo_action.action.delete.start) > 1) ||
	     (g_utf8_get_char (undo_action.action.delete.text  ) == '\n'))
	       	undo_action.mergeable = FALSE;
	else
		undo_action.mergeable = TRUE;

	gedit_debug (DEBUG_UNDO, "START: %d", undo_action.action.delete.start);
	gedit_debug (DEBUG_UNDO, "END: %d", undo_action.action.delete.end);
	gedit_debug (DEBUG_UNDO, "TEXT: %s", undo_action.action.delete.text);

	gedit_undo_manager_add_action (um, undo_action);

	g_free (undo_action.action.delete.text);

	gedit_debug (DEBUG_UNDO, "DONE");
}

static void 
gedit_undo_manager_begin_user_action_handler (GtkTextBuffer *buffer, GeditUndoManager *um)
{
	gedit_debug (DEBUG_UNDO, "");

	g_return_if_fail (GEDIT_IS_UNDO_MANAGER (um));
	g_return_if_fail (um->priv != NULL);

	if (um->priv->running_not_undoable_actions > 0)
		return;

	um->priv->actions_in_current_group = 0;
}

static void
gedit_undo_manager_end_user_action_handler (GtkTextBuffer *buffer, GeditUndoManager *um)
{
	gedit_debug (DEBUG_UNDO, "");

	if (um->priv->running_not_undoable_actions > 0)
		return;

	/* TODO: is it needed ? */
}

/* FIXME: change prototype to use GeditUndoAction *undo_action : Paolo */
static void
gedit_undo_manager_add_action (GeditUndoManager *um, GeditUndoAction undo_action)
{
	GeditUndoAction* action;
	
	gedit_debug (DEBUG_UNDO, "");

	if (um->priv->next_redo >= 0)
	{
		gedit_undo_manager_free_first_n_actions	(um, um->priv->next_redo + 1);
	}

	um->priv->next_redo = -1;

	if (!gedit_undo_manager_merge_action (um, &undo_action))
	{
		action = g_new (GeditUndoAction, 1);
		*action = undo_action;

		if (action->action_type == GEDIT_UNDO_ACTION_INSERT)
			action->action.insert.text = g_strdup (undo_action.action.insert.text);
		else if (action->action_type == GEDIT_UNDO_ACTION_DELETE)
			action->action.delete.text = g_strdup (undo_action.action.delete.text); 
		else
		{
			g_free (action);
			g_return_if_fail (FALSE);
		}
		
		++um->priv->actions_in_current_group;
		action->order_in_group = um->priv->actions_in_current_group;

		if (action->order_in_group == 1)
			++um->priv->num_of_groups;
	
		um->priv->actions = g_list_prepend (um->priv->actions, action);
	}
	
	gedit_undo_manager_check_list_size (um);

	if (!um->priv->can_undo)
	{
		um->priv->can_undo = TRUE;
		g_signal_emit (G_OBJECT (um), undo_manager_signals [CAN_UNDO], 0, TRUE);
	}

	if (um->priv->can_redo)
	{
		um->priv->can_redo = FALSE;
		g_signal_emit (G_OBJECT (um), undo_manager_signals [CAN_REDO], 0, FALSE);
	}

	gedit_debug (DEBUG_UNDO, "DONE");
}

static void 
gedit_undo_manager_free_first_n_actions (GeditUndoManager *um, gint n)
{
	gint i;
	
	gedit_debug (DEBUG_UNDO, "");

	g_return_if_fail (GEDIT_IS_UNDO_MANAGER (um));
	g_return_if_fail (um->priv != NULL);

	if (um->priv->actions == NULL)
		return;
	
	for (i = 0; i < n; i++)
	{
		GeditUndoAction *undo_action = 
			(GeditUndoAction *)(g_list_first (um->priv->actions)->data);
	
		if (undo_action->action_type == GEDIT_UNDO_ACTION_INSERT)
			g_free (undo_action->action.insert.text);
		else if (undo_action->action_type == GEDIT_UNDO_ACTION_DELETE)
			g_free (undo_action->action.delete.text);
		else
			g_return_if_fail (FALSE);

		if (undo_action->order_in_group == 1)
			--um->priv->num_of_groups;

		g_free (undo_action);

		um->priv->actions = g_list_delete_link (um->priv->actions, um->priv->actions);

		if (um->priv->actions == NULL) 
			return;
	}
}

static void 
gedit_undo_manager_check_list_size (GeditUndoManager *um)
{
	gint undo_levels;
	
	gedit_debug (DEBUG_UNDO, "TODO");

	g_return_if_fail (GEDIT_IS_UNDO_MANAGER (um));
	g_return_if_fail (um->priv != NULL);
	
	undo_levels = gedit_prefs_manager_get_undo_actions_limit ();
	
	if (undo_levels < 1)
		return;

	if (um->priv->num_of_groups > undo_levels)
	{
		GeditUndoAction *undo_action;
		GList* last;
		
		last = g_list_last (um->priv->actions);
		undo_action = (GeditUndoAction*) last->data;
			
		do
		{
			if (undo_action->action_type == GEDIT_UNDO_ACTION_INSERT)
				g_free (undo_action->action.insert.text);
			else if (undo_action->action_type == GEDIT_UNDO_ACTION_DELETE)
				g_free (undo_action->action.delete.text);
			else
				g_return_if_fail (FALSE);

			if (undo_action->order_in_group == 1)
				--um->priv->num_of_groups;

			g_free (undo_action);

			um->priv->actions = g_list_delete_link (um->priv->actions, last);
			g_return_if_fail (um->priv->actions != NULL); 

			last = g_list_last (um->priv->actions);
			undo_action = (GeditUndoAction*) last->data;

		} while ((undo_action->order_in_group > 1) || 
			 (um->priv->num_of_groups > undo_levels));
	}	
}

/**
 * gedit_undo_manager_merge_action:
 * @um: a #GeditUndoManager 
 * @undo_action: 
 * 
 * This function tries to merge the undo action at the top of
 * the stack with a new undo action. So when we undo for example
 * typing, we can undo the whole word and not each letter by itself
 * 
 * Return Value: TRUE is merge was sucessful, FALSE otherwise
 **/
static gboolean 
gedit_undo_manager_merge_action (GeditUndoManager *um, GeditUndoAction *undo_action)
{
	GeditUndoAction *last_action;
	
	gedit_debug (DEBUG_UNDO, "");

	g_return_val_if_fail (GEDIT_IS_UNDO_MANAGER (um), FALSE);
	g_return_val_if_fail (um->priv != NULL, FALSE);

	if (um->priv->actions == NULL)
		return FALSE;

	last_action = (GeditUndoAction*) g_list_nth_data (um->priv->actions, 0);

	if (!last_action->mergeable)
		return FALSE;

	if ((!undo_action->mergeable) ||
	    (undo_action->action_type != last_action->action_type))
	{
		last_action->mergeable = FALSE;
		return FALSE;
	}

	if (undo_action->action_type == GEDIT_UNDO_ACTION_DELETE)
	{				
		if ((last_action->action.delete.start != undo_action->action.delete.start) &&
		    (last_action->action.delete.start != undo_action->action.delete.end))
		{
			last_action->mergeable = FALSE;
			return FALSE;
		}
		
		if (last_action->action.delete.start == undo_action->action.delete.start)
		{
			gchar *str;
			
#define L  (last_action->action.delete.end - last_action->action.delete.start - 1)
#define g_utf8_get_char_at(p,i) g_utf8_get_char(g_utf8_offset_to_pointer((p),(i)))
		
			gedit_debug (DEBUG_UNDO, "L = %d", L);

			/* Deleted with the delete key */
			if ((g_utf8_get_char (undo_action->action.delete.text) != ' ') &&
			    (g_utf8_get_char (undo_action->action.delete.text) != '\t') &&
                            ((g_utf8_get_char_at (last_action->action.delete.text, L) == ' ') ||
			     (g_utf8_get_char_at (last_action->action.delete.text, L)  == '\t')))
			{
				last_action->mergeable = FALSE;
				return FALSE;
			}
			
			str = g_strdup_printf ("%s%s", last_action->action.delete.text, 
				undo_action->action.delete.text);
			
			g_free (last_action->action.delete.text);
			last_action->action.delete.end += (undo_action->action.delete.end - undo_action->action.delete.start);
			last_action->action.delete.text = str;
		}
		else
		{
			gchar *str;
			
			/* Deleted with the backspace key */
			if ((g_utf8_get_char (undo_action->action.delete.text) != ' ') &&
			    (g_utf8_get_char (undo_action->action.delete.text) != '\t') &&
                            ((g_utf8_get_char (last_action->action.delete.text) == ' ') ||
			     (g_utf8_get_char (last_action->action.delete.text) == '\t')))
			{
				last_action->mergeable = FALSE;
				return FALSE;
			}

			str = g_strdup_printf ("%s%s", undo_action->action.delete.text, 
				last_action->action.delete.text);
			
			g_free (last_action->action.delete.text);
			last_action->action.delete.start = undo_action->action.delete.start;
			last_action->action.delete.text = str;
		}
	}
	else if (undo_action->action_type == GEDIT_UNDO_ACTION_INSERT)
	{
		gchar* str;
		
#define I (last_action->action.insert.chars - 1)
		
		if ((undo_action->action.insert.pos != 
		     	(last_action->action.insert.pos + last_action->action.insert.chars)) ||
		    ((g_utf8_get_char (undo_action->action.insert.text) != ' ') &&
		      (g_utf8_get_char (undo_action->action.insert.text) != '\t') &&
		     ((g_utf8_get_char_at (last_action->action.insert.text, I) == ' ') ||
		      (g_utf8_get_char_at (last_action->action.insert.text, I) == '\t')))
		   )
		{
			last_action->mergeable = FALSE;
			return FALSE;
		}

		str = g_strdup_printf ("%s%s", last_action->action.insert.text, 
				undo_action->action.insert.text);
		
		g_free (last_action->action.insert.text);
		last_action->action.insert.length += undo_action->action.insert.length;
		last_action->action.insert.text = str;
		last_action->action.insert.chars += undo_action->action.insert.chars;

	}
	else
		g_warning ("Unknown action inside undo merge encountered");	
		
	return TRUE;
}


