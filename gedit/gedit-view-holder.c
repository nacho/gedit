/*
 * gedit-view-holder.c
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

#include "gedit-view-holder.h"
#include "gedit-undo-manager.h"
#include "gedit-marshal.h"


#define GEDIT_VIEW_HOLDER_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE((object), GEDIT_TYPE_VIEW_HOLDER, GeditViewHolderPrivate))

/* Properties to bind between views and documents */
enum
{
	AUTO_INDENT,
	DRAW_SPACES,
	HIGHLIGHT_CURRENT_LINE,
	INDENT_ON_TAB,
	INDENT_WIDTH,
	INSERT_SPACES_INSTEAD_OF_TABS,
	RIGHT_MARGIN_POSITION,
	SHOW_LINE_MARKS,
	SHOW_LINE_NUMBERS,
	SHOW_RIGHT_MARGIN,
	SMART_HOME_END,
	TAB_WIDTH,
	HIGHLIGHT_MATCHING_BRACKETS,
	HIGHLIGHT_SYNTAX,
	LANGUAGE,
	MAX_UNDO_LEVELS,
	STYLE_SCHEME,
	LOCATION,
	SHORTNAME,
	CONTENT_TYPE,
	LAST_PROPERTY
};

enum
{
	REAL,
	VIRTUAL,
	N_VIEW_TYPES
};

typedef struct _Binding
{
	GeditViewFrame *frames[N_VIEW_TYPES];

	gulong          insert_text_id[N_VIEW_TYPES];
	gulong          delete_range_id[N_VIEW_TYPES];

	GBinding       *bindings[LAST_PROPERTY];
} Binding;

struct _GeditViewHolderPrivate
{
	GSList               *frames;
	GeditViewFrame       *active_frame;
	GSList               *bindings;
	GtkSourceUndoManager *undo_manager;

	guint                 dispose_has_run : 1;
};

/* Signals */
enum
{
	FRAME_ADDED,
	FRAME_REMOVED,
	ACTIVE_FRAME_CHANGED,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (GeditViewHolder, gedit_view_holder, GTK_TYPE_VBOX)

static void
unbind (Binding *binding)
{
	gint i;

	for (i = 0; i < LAST_PROPERTY; i++)
	{
		g_object_unref (binding->bindings[i]);
	}

	g_signal_handler_disconnect (gedit_view_frame_get_document (binding->frames[REAL]),
				     binding->insert_text_id[REAL]);
	g_signal_handler_disconnect (gedit_view_frame_get_document (binding->frames[VIRTUAL]),
				     binding->insert_text_id[VIRTUAL]);
	g_signal_handler_disconnect (gedit_view_frame_get_document (binding->frames[REAL]),
				     binding->delete_range_id[REAL]);
	g_signal_handler_disconnect (gedit_view_frame_get_document (binding->frames[VIRTUAL]),
				     binding->delete_range_id[VIRTUAL]);
}

static void
insert_text_on_virtual_doc_cb (GtkTextBuffer *textbuffer,
			       GtkTextIter   *location,
			       gchar         *text,
			       gint           len,
			       Binding       *binding)
{
	GtkTextBuffer *doc;
	GtkTextIter iter;
	gint offset;
	gulong id;

	if (textbuffer == GTK_TEXT_BUFFER (gedit_view_frame_get_document (binding->frames[REAL])))
	{
		doc = GTK_TEXT_BUFFER (gedit_view_frame_get_document (binding->frames[VIRTUAL]));
		id = binding->insert_text_id[VIRTUAL];
	}
	else
	{
		doc = GTK_TEXT_BUFFER (gedit_view_frame_get_document (binding->frames[REAL]));
		id = binding->insert_text_id[REAL];
	}

	offset = gtk_text_iter_get_offset (location);
	gtk_text_buffer_get_iter_at_offset (doc, &iter, offset);

	g_signal_handler_block (doc, id);
	gtk_text_buffer_insert (doc, &iter, text, len);
	g_signal_handler_unblock (doc, id);
}

static void
delete_range_on_virtual_doc_cb (GtkTextBuffer *textbuffer,
				GtkTextIter   *start,
				GtkTextIter   *end,
				gpointer       user_data)
{
	Binding *binding = (Binding *)user_data;
	GtkTextBuffer *doc;
	GtkTextIter start_iter, end_iter;
	gint offset;
	gulong id;

	if (textbuffer == GTK_TEXT_BUFFER (gedit_view_frame_get_document (binding->frames[REAL])))
	{
		doc = GTK_TEXT_BUFFER (gedit_view_frame_get_document (binding->frames[VIRTUAL]));
		id = binding->delete_range_id[VIRTUAL];
	}
	else
	{
		doc = GTK_TEXT_BUFFER (gedit_view_frame_get_document (binding->frames[REAL]));
		id = binding->delete_range_id[REAL];
	}

	offset = gtk_text_iter_get_offset (start);
	gtk_text_buffer_get_iter_at_offset (doc, &start_iter, offset);

	offset = gtk_text_iter_get_offset (end);
	gtk_text_buffer_get_iter_at_offset (doc, &end_iter, offset);

	g_signal_handler_block (doc, id);
	gtk_text_buffer_delete (doc, &start_iter, &end_iter);
	g_signal_handler_unblock (doc, id);
}

static void
bind (Binding *binding)
{
	GeditDocument *real_doc, *virtual_doc;
	GeditView *real_view, *virtual_view;

	real_doc = gedit_view_frame_get_document (binding->frames[REAL]);
	virtual_doc = gedit_view_frame_get_document (binding->frames[VIRTUAL]);
	real_view = gedit_view_frame_get_view (binding->frames[REAL]);
	virtual_view = gedit_view_frame_get_view (binding->frames[VIRTUAL]);

	/* Signals to sync the text inserted or deleted */
	binding->insert_text_id[REAL] =
		g_signal_connect (real_doc,
				  "insert-text",
				  G_CALLBACK (insert_text_on_virtual_doc_cb),
				  binding);

	binding->delete_range_id[REAL] =
		g_signal_connect (real_doc,
				  "delete-range",
				  G_CALLBACK (delete_range_on_virtual_doc_cb),
				  binding);

	binding->insert_text_id[VIRTUAL] =
		g_signal_connect (virtual_doc,
				  "insert-text",
				  G_CALLBACK (insert_text_on_virtual_doc_cb),
				  binding);

	binding->delete_range_id[VIRTUAL] =
		g_signal_connect (virtual_doc,
				  "delete-range",
				  G_CALLBACK (delete_range_on_virtual_doc_cb),
				  binding);

	/* Bind all properties */
	binding->bindings[AUTO_INDENT] =
		g_object_bind_property (real_view,
					"auto-indent",
					virtual_view,
					"auto-indent",
					G_BINDING_SYNC_CREATE);
	binding->bindings[DRAW_SPACES] =
		g_object_bind_property (real_view,
					"draw-spaces",
					virtual_view,
					"draw-spaces",
					G_BINDING_SYNC_CREATE);
	binding->bindings[HIGHLIGHT_CURRENT_LINE] =
		g_object_bind_property (real_view,
					"highlight-current-line",
					virtual_view,
					"highlight-current-line",
					G_BINDING_SYNC_CREATE);
	binding->bindings[INDENT_ON_TAB] =
		g_object_bind_property (real_view,
					"indent-on-tab",
					virtual_view,
					"indent-on-tab",
					G_BINDING_SYNC_CREATE);
	binding->bindings[INDENT_WIDTH] =
		g_object_bind_property (real_view,
					"indent-width",
					virtual_view,
					"indent-width",
					G_BINDING_SYNC_CREATE);
	binding->bindings[INSERT_SPACES_INSTEAD_OF_TABS] =
		g_object_bind_property (real_view,
					"insert-spaces-instead-of-tabs",
					virtual_view,
					"insert-spaces-instead-of-tabs",
					G_BINDING_SYNC_CREATE);
	binding->bindings[RIGHT_MARGIN_POSITION] =
		g_object_bind_property (real_view,
					"right-margin-position",
					virtual_view,
					"right-margin-position",
					G_BINDING_SYNC_CREATE);
	binding->bindings[SHOW_LINE_MARKS] =
		g_object_bind_property (real_view,
					"show-line-marks",
					virtual_view,
					"show-line-marks",
					G_BINDING_SYNC_CREATE);
	binding->bindings[SHOW_LINE_NUMBERS] =
		g_object_bind_property (real_view,
					"show-line-numbers",
					virtual_view,
					"show-line-numbers",
					G_BINDING_SYNC_CREATE);
	binding->bindings[SHOW_RIGHT_MARGIN] =
		g_object_bind_property (real_view,
					"show-right-margin",
					virtual_view,
					"show-right-margin",
					G_BINDING_SYNC_CREATE);
	binding->bindings[SMART_HOME_END] =
		g_object_bind_property (real_view,
					"smart-home-end",
					virtual_view,
					"smart-home-end",
					G_BINDING_SYNC_CREATE);
	binding->bindings[TAB_WIDTH] =
		g_object_bind_property (real_view,
					"tab-width",
					virtual_view,
					"tab-width",
					G_BINDING_SYNC_CREATE);

	binding->bindings[HIGHLIGHT_MATCHING_BRACKETS] =
		g_object_bind_property (real_doc,
					"highlight-matching-brackets",
					virtual_doc,
					"highlight-matching-brackets",
					G_BINDING_SYNC_CREATE);
	binding->bindings[HIGHLIGHT_SYNTAX] =
		g_object_bind_property (real_doc,
					"highlight-syntax",
					virtual_doc,
					"highlight-syntax",
					G_BINDING_SYNC_CREATE);
	binding->bindings[LANGUAGE] =
		g_object_bind_property (real_doc,
					"language",
					virtual_doc,
					"language",
					G_BINDING_SYNC_CREATE);
	binding->bindings[MAX_UNDO_LEVELS] =
		g_object_bind_property (real_doc,
					"max-undo-levels",
					virtual_doc,
					"max-undo-levels",
					G_BINDING_SYNC_CREATE);
	binding->bindings[STYLE_SCHEME] =
		g_object_bind_property (real_doc,
					"style-scheme",
					virtual_doc,
					"style-scheme",
					G_BINDING_SYNC_CREATE);
	binding->bindings[LOCATION] =
		g_object_bind_property (real_doc,
					"location",
					virtual_doc,
					"location",
					G_BINDING_SYNC_CREATE);
	binding->bindings[SHORTNAME] =
		g_object_bind_property (real_doc,
					"shortname",
					virtual_doc,
					"shortname",
					G_BINDING_SYNC_CREATE);
	binding->bindings[CONTENT_TYPE] =
		g_object_bind_property (real_doc,
					"content-type",
					virtual_doc,
					"content-type",
					G_BINDING_SYNC_CREATE);
}

static void
unbind_frame (GeditViewHolder *holder,
              GeditViewFrame  *frame)
{
	GSList *l;
	GSList *unbind_list = NULL;

	for (l = holder->priv->bindings; l != NULL; l = g_slist_next (l))
	{
		Binding *binding = (Binding *)l->data;

		if (frame == binding->frames[REAL] ||
		    frame == binding->frames[VIRTUAL])
		{
			unbind_list = g_slist_prepend (unbind_list, binding);
		}
	}

	for (l = unbind_list; l != NULL; l = g_slist_next (l))
	{
		Binding *b1 = (Binding *)l->data;

		unbind (b1);

		if (l->next != NULL)
		{
			Binding *b2 = (Binding *)l->next->data;
			Binding *new_binding;
			gint i, j;

			i = (frame == b1->frames[REAL]) ? VIRTUAL : REAL;
			j = (frame == b2->frames[REAL]) ? VIRTUAL : REAL;

			new_binding = g_slice_new (Binding);
			new_binding->frames[REAL] = b1->frames[i];
			new_binding->frames[VIRTUAL] = b2->frames[j];

			bind (new_binding);

			holder->priv->bindings = g_slist_append (holder->priv->bindings,
								 new_binding);
		}

		/* We remove the old binding */
		holder->priv->bindings = g_slist_remove (holder->priv->bindings,
							 b1);
		g_slice_free (Binding, b1);
	}

	g_slist_free (unbind_list);
}

static void
document_sync_documents (GeditDocument   *doc,
			 GeditViewHolder *holder)
{
	GSList *l;

	for (l = holder->priv->frames; l != NULL; l = g_slist_next (l))
	{
		GeditViewFrame *frame = GEDIT_VIEW_FRAME (l->data);

		if (frame != holder->priv->active_frame)
		{
			GeditDocument *vdoc;

			vdoc = gedit_view_frame_get_document (frame);

			_gedit_document_sync_documents (doc, vdoc);
		}
	}
}

static void
on_document_loaded (GeditDocument   *document,
		    const GError    *error,
		    GeditViewHolder *holder)
{
	if (error == NULL)
	{
		GeditView *view;

		view = gedit_view_frame_get_view (holder->priv->active_frame);

		/* Scroll to the cursor when the document is loaded */
		gedit_view_scroll_to_cursor (view);
	}
}

static void
on_grab_focus (GtkWidget       *widget,
	       GeditViewHolder *holder)
{
	GtkWidget *active_view;

	active_view = GTK_WIDGET (gedit_view_frame_get_view (holder->priv->active_frame));

	/* Update the active view */
	if (active_view != widget)
	{
		GSList *l;

		for (l = holder->priv->frames; l != NULL; l = g_slist_next (l))
		{
			GeditViewFrame *frame = GEDIT_VIEW_FRAME (l->data);
			GtkWidget *view;

			view = GTK_WIDGET (gedit_view_frame_get_view (frame));

			if (view == widget)
			{
				GeditViewFrame *old_frame;

				old_frame = holder->priv->active_frame;
				holder->priv->active_frame = frame;

				g_object_set (G_OBJECT (holder->priv->undo_manager),
					      "buffer",
					      gedit_view_frame_get_document (frame),
					      NULL);

				/* Mark the old view as non editable and the new
				   one with the editability of the old one */
				gedit_view_set_editable (GEDIT_VIEW (view),
							 gedit_view_get_editable (GEDIT_VIEW (active_view)));
				gedit_view_set_editable (GEDIT_VIEW (active_view),
							 FALSE);

				g_signal_emit (G_OBJECT (holder), signals[ACTIVE_FRAME_CHANGED],
				               0, old_frame, frame);
				break;
			}
		}
	}
}

static void
add_view_frame (GeditViewHolder *holder,
		GeditViewFrame  *frame,
		gboolean         main_container,
		GtkOrientation   orientation)
{
	if (main_container)
	{
		gtk_box_pack_start (GTK_BOX (holder), GTK_WIDGET (frame),
				    TRUE, TRUE, 0);
	}
	else
	{
		GeditViewFrame *active_frame = holder->priv->active_frame;
		GtkWidget *paned;
		GtkWidget *parent;
		GtkAllocation allocation;
		gint pos;

		gtk_widget_get_allocation (GTK_WIDGET (active_frame),
					   &allocation);

		if (orientation == GTK_ORIENTATION_HORIZONTAL)
		{
			paned = gtk_hpaned_new ();
			pos = allocation.width / 2;
		}
		else
		{
			paned = gtk_vpaned_new ();
			pos = allocation.height / 2;
		}

		gtk_widget_show (paned);

		/* First we remove the active view from its parent, to do
		   this we add a ref to it*/
		g_object_ref (active_frame);
		parent = gtk_widget_get_parent (GTK_WIDGET (active_frame));

		gtk_container_remove (GTK_CONTAINER (parent),
				      GTK_WIDGET (active_frame));

		if (GTK_IS_VBOX (parent))
		{
			gtk_box_pack_end (GTK_BOX (parent), paned, TRUE, TRUE, 0);
		}
		else
		{
			gtk_container_add (GTK_CONTAINER (parent), paned);
		}

		gtk_paned_pack1 (GTK_PANED (paned), GTK_WIDGET (active_frame),
				 FALSE, TRUE);
		g_object_unref (active_frame);

		gtk_paned_pack2 (GTK_PANED (paned), GTK_WIDGET (frame),
				 FALSE, FALSE);

		/* We need to set the new paned in the right place */
		gtk_paned_set_position (GTK_PANED (paned), pos);
	}

	holder->priv->frames = g_slist_append (holder->priv->frames, frame);

	g_signal_emit (G_OBJECT (holder), signals[FRAME_ADDED], 0, frame);
}

static void
connect_frame_signals (GeditViewHolder *holder,
		       GeditViewFrame  *frame)
{
	GeditDocument *doc;
	GeditView *view;

	doc = gedit_view_frame_get_document (frame);
	view = gedit_view_frame_get_view (frame);

	g_signal_connect (doc,
			  "sync-documents",
			  G_CALLBACK (document_sync_documents),
			  holder);
	g_signal_connect_after (doc,
				"loaded",
				G_CALLBACK (on_document_loaded),
				holder);

	g_signal_connect_after (view,
				"grab-focus",
				G_CALLBACK (on_grab_focus),
				holder);
}

static void
split_document (GeditViewHolder *holder,
		GtkOrientation   orientation)
{
	GeditViewFrame *frame;
	Binding *binding;
	GtkTextIter start, end;
	gchar *content;
	GeditDocument *real_doc, *virtual_doc;

	/* Create the holder */
	frame = gedit_view_frame_new ();
	connect_frame_signals (holder, frame);
	gtk_widget_show (GTK_WIDGET (frame));

	real_doc = gedit_view_frame_get_document (holder->priv->active_frame);
	virtual_doc = gedit_view_frame_get_document (frame);

	gtk_source_buffer_set_undo_manager (GTK_SOURCE_BUFFER (virtual_doc),
					    holder->priv->undo_manager);

	add_view_frame (holder, frame, FALSE, orientation);

	/* Create the binding for the frames */
	binding = g_slice_new (Binding);
	binding->frames[REAL] = holder->priv->active_frame;
	binding->frames[VIRTUAL] = frame;

	holder->priv->bindings = g_slist_append (holder->priv->bindings, binding);

	/* Copy the content from the real doc to the virtual doc */
	gtk_text_buffer_get_bounds (GTK_TEXT_BUFFER (real_doc), &start, &end);

	content = gtk_text_buffer_get_text (GTK_TEXT_BUFFER (real_doc),
					    &start, &end, FALSE);

	gtk_text_buffer_set_text (GTK_TEXT_BUFFER (virtual_doc),
				  content, -1);

	g_free (content);

	/* Bind the views */
	bind (binding);
	_gedit_document_sync_documents (real_doc, virtual_doc);

	/* When grabbing the focus the active changes, so be careful here
	   to grab the focus after creating the binding */
	gtk_widget_grab_focus (GTK_WIDGET (gedit_view_frame_get_view (frame)));
}

static void
remove_view_frame (GeditViewHolder *holder,
		   GeditViewFrame  *frame)
{
	GtkWidget *parent;
	GtkWidget *grandpa;
	GList *children;
	GSList *next_item;
	GeditViewFrame *next_frame;
	GeditView *next_view;

	parent = gtk_widget_get_parent (GTK_WIDGET (frame));
	if (GTK_IS_VBOX (parent))
	{
		g_warning ("We can't have an empty page");
		return;
	}

	/* Get the next holder */
	next_item = g_slist_find (holder->priv->frames, frame);
	if (next_item->next != NULL)
	{
		next_item = next_item->next;
	}
	else
	{
		next_item = holder->priv->frames;
	}

	next_frame = GEDIT_VIEW_FRAME (next_item->data);

	/* Now we destroy the widget, we get the children of parent and we destroy
	   parent too as the parent is a useless paned. Finally we add the child
	   into the grand parent */
	g_object_ref (frame);
	unbind_frame (holder, frame);

	holder->priv->frames = g_slist_remove (holder->priv->frames,
					       frame);

	next_view = gedit_view_frame_get_view (next_frame);

	/* It is important to grab the focus before destroying the view and
	   after unbind the view, in this way we don't have to make weird
	   checks in the grab focus handler */
	gtk_widget_grab_focus (GTK_WIDGET (next_view));

	gtk_widget_destroy (GTK_WIDGET (frame));

	children = gtk_container_get_children (GTK_CONTAINER (parent));
	if (g_list_length (children) > 1)
	{
		g_warning ("The parent is not a paned");
		return;
	}
	grandpa = gtk_widget_get_parent (parent);

	g_object_ref (children->data);
	gtk_container_remove (GTK_CONTAINER (parent),
			      GTK_WIDGET (children->data));
	gtk_widget_destroy (parent);
	gtk_container_add (GTK_CONTAINER (grandpa),
			   GTK_WIDGET (children->data));
	g_object_unref (children->data);

	g_list_free (children);
	g_object_unref (frame);

	/* FIXME: this is a hack, the first grab focus is the one that emits
	   the signal (ACTIVE_FRAME_CHANGED) and the second one makes the view
	   get the focus as it is steal when destroying the active view */
	gtk_widget_grab_focus (GTK_WIDGET (next_view));
}

static void
gedit_view_holder_dispose (GObject *object)
{
	GeditViewHolder *holder = GEDIT_VIEW_HOLDER (object);

	if (!holder->priv->dispose_has_run)
	{
		GSList *l;

		for (l = holder->priv->bindings; l != NULL; l = g_slist_next (l))
		{
			Binding *b = (Binding *)l->data;

			unbind (b);
			g_slice_free (Binding, b);
		}

		holder->priv->dispose_has_run = TRUE;
	}

	G_OBJECT_CLASS (gedit_view_holder_parent_class)->dispose (object);
}

static void
gedit_view_holder_finalize (GObject *object)
{
	GeditViewHolder *holder = GEDIT_VIEW_HOLDER (object);

	g_slist_free (holder->priv->bindings);
	g_slist_free (holder->priv->frames);

	G_OBJECT_CLASS (gedit_view_holder_parent_class)->finalize (object);
}

static void
gedit_view_holder_class_init (GeditViewHolderClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	
	object_class->dispose = gedit_view_holder_dispose;
	object_class->finalize = gedit_view_holder_finalize;

	signals[FRAME_ADDED] =
		g_signal_new ("frame-added",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (GeditViewHolderClass, frame_added),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE,
			      1,
			      GEDIT_TYPE_VIEW_FRAME);

	signals[FRAME_REMOVED] =
		g_signal_new ("frame-removed",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (GeditViewHolderClass, frame_removed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE,
			      1,
			      GEDIT_TYPE_VIEW_FRAME);

	signals[ACTIVE_FRAME_CHANGED] =
		g_signal_new ("active-frame-changed",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (GeditViewHolderClass, active_frame_changed),
			      NULL, NULL,
			      gedit_marshal_VOID__OBJECT_OBJECT,
			      G_TYPE_NONE,
			      2,
			      GEDIT_TYPE_VIEW_FRAME,
			      GEDIT_TYPE_VIEW_FRAME);

	g_type_class_add_private (object_class, sizeof (GeditViewHolderPrivate));
}

static void
gedit_view_holder_init (GeditViewHolder *holder)
{
	GeditDocument *doc;

	holder->priv = GEDIT_VIEW_HOLDER_GET_PRIVATE (holder);

	holder->priv->active_frame = gedit_view_frame_new ();
	connect_frame_signals (holder, holder->priv->active_frame);
	gtk_widget_show (GTK_WIDGET (holder->priv->active_frame));

	add_view_frame (holder, holder->priv->active_frame, TRUE,
			GTK_ORIENTATION_HORIZONTAL);

	doc = gedit_view_frame_get_document (holder->priv->active_frame);

	/* Create the default undo manager and assign it to the active view */
	holder->priv->undo_manager = gedit_undo_manager_new (GTK_SOURCE_BUFFER (doc));

	gtk_source_buffer_set_undo_manager (GTK_SOURCE_BUFFER (doc),
					    holder->priv->undo_manager);
}

GeditViewHolder *
gedit_view_holder_new ()
{
	return g_object_new (GEDIT_TYPE_VIEW_HOLDER, NULL);
}

GeditViewFrame *
gedit_view_holder_get_active_frame (GeditViewHolder *holder)
{
	g_return_val_if_fail (GEDIT_IS_VIEW_HOLDER (holder), NULL);

	return holder->priv->active_frame;
}

GSList *
gedit_view_holder_get_frames (GeditViewHolder *holder)
{
	g_return_val_if_fail (GEDIT_IS_VIEW_HOLDER (holder), NULL);

	return holder->priv->frames;
}

void
gedit_view_holder_split (GeditViewHolder *holder,
			 GtkOrientation   orientation)
{
	g_return_if_fail (GEDIT_IS_VIEW_HOLDER (holder));

	split_document (holder, orientation);
}

void
gedit_view_holder_unsplit (GeditViewHolder *holder)
{
	g_return_if_fail (GEDIT_VIEW_HOLDER (holder));

	remove_view_frame (holder, holder->priv->active_frame);
}

gboolean
gedit_view_holder_can_split (GeditViewHolder *holder)
{
	g_return_val_if_fail (GEDIT_IS_VIEW_HOLDER (holder), FALSE);

	/*TODO: for future expansion */
	return TRUE;
}

gboolean
gedit_view_holder_can_unsplit (GeditViewHolder *holder)
{
	g_return_val_if_fail (GEDIT_IS_VIEW_HOLDER (holder), FALSE);

	return (holder->priv->frames->next != NULL);
}

void
gedit_view_holder_set_cursor (GeditViewHolder *holder,
			      GdkCursorType    cursor_type)
{
	GSList *l;

	g_return_if_fail (GEDIT_IS_VIEW_HOLDER (holder));

	for (l = holder->priv->frames; l != NULL; l = g_slist_next (l))
	{
		GeditViewFrame *frame = GEDIT_VIEW_FRAME (l->data);
		GeditView *view;
		GdkCursor *cursor;
		GdkWindow *text_window;
		GdkWindow *left_window;

		view = gedit_view_frame_get_view (frame);

		/* TODO: Maybe set the the cursor for other views too ? */
		if (!GTK_IS_TEXT_VIEW (view))
			continue;

		text_window = gtk_text_view_get_window (GTK_TEXT_VIEW (view),
							GTK_TEXT_WINDOW_TEXT);
		left_window = gtk_text_view_get_window (GTK_TEXT_VIEW (view),
							GTK_TEXT_WINDOW_LEFT);

		cursor = gdk_cursor_new_for_display (
				gtk_widget_get_display (GTK_WIDGET (view)),
				cursor_type);

		if (text_window != NULL)
			gdk_window_set_cursor (text_window, cursor);
		if (left_window != NULL)
			gdk_window_set_cursor (left_window, NULL);

		gdk_cursor_unref (cursor);
	}
}
