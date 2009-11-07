/*
 * gedit-page.c
 * This file is part of gedit
 *
 * Copyright (C) 2009 - Ignacio Casal Quinteiro
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


#include "gedit-page.h"
#include "gedit-data-binding.h"
#include "gedit-view-container.h"
#include "gedit-text-view.h"

#define GEDIT_PAGE_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE((object), GEDIT_TYPE_PAGE, GeditPagePrivate))

#define DESTROY_DATA_KEY     "gedit-page-destroy-data-key"
#define VIRTUAL_DOCUMENT_KEY "gedit-page-virtual-document-key"

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
	LAST_PROPERTY
};

enum
{
	REAL,
	VIRTUAL,
	N_STATES
};

typedef struct _Binding
{
	GeditDocument	       *docs[N_STATES];
	GtkWidget	       *views[N_STATES];
	
	gulong		        insert_text_id[N_STATES];
	gulong		        delete_range_id[N_STATES];
	
	GeditDataBinding       *bindings[LAST_PROPERTY];
} Binding;

typedef struct _VirtualDocument
{
	GList		       *bindings; /* List of Binding */
} VirtualDocument;

typedef struct _DestroyData
{
	GtkWidget *view;
	VirtualDocument *vdoc;
} DestroyData;

struct _GeditPagePrivate
{
	GList *containers;
	GeditViewContainer *active_container;
};

/* Signals */
enum
{
	CONTAINER_ADDED,
	CONTAINER_REMOVED,
	ACTIVE_CONTAINER_CHANGED,
	LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (GeditPage, gedit_page, GTK_TYPE_VBOX)

static Binding *
get_binding_from_view (DestroyData *data)
{
	GList *l;
	
	for (l = data->vdoc->bindings; l != NULL; l = g_list_next (l))
	{
		Binding *binding = (Binding *)l->data;
		
		if (data->view == binding->views[REAL] ||
		    data->view == binding->views[VIRTUAL])
			return binding;
	}

	return NULL;
}

static void
unbind (Binding *binding)
{
	gint i;

	for (i = 0; i < LAST_PROPERTY; i++)
	{
		gedit_data_binding_free (binding->bindings[i]);
	}
	
	g_signal_handler_disconnect (binding->docs[REAL],
				     binding->insert_text_id[REAL]);
	g_signal_handler_disconnect (binding->docs[VIRTUAL],
				     binding->insert_text_id[VIRTUAL]);
	g_signal_handler_disconnect (binding->docs[REAL],
				     binding->delete_range_id[REAL]);
	g_signal_handler_disconnect (binding->docs[VIRTUAL],
				     binding->delete_range_id[VIRTUAL]);
}

static void
insert_text_on_virtual_doc_cb (GtkTextBuffer *textbuffer,
			       GtkTextIter   *location,
			       gchar         *text,
			       gint           len,
			       gpointer       user_data)
{
	Binding *binding = (Binding *)user_data;
	GtkTextBuffer *doc;
	GtkTextIter iter;
	gint line;
	gint line_offset;
	gulong id;
	
	if (textbuffer == GTK_TEXT_BUFFER (binding->docs[REAL]))
	{
		doc = GTK_TEXT_BUFFER (binding->docs[VIRTUAL]);
		id = binding->insert_text_id[VIRTUAL];
	}
	else
	{
		doc = GTK_TEXT_BUFFER (binding->docs[REAL]);
		id = binding->insert_text_id[REAL];
	}
	
	line = gtk_text_iter_get_line (location);
	line_offset = gtk_text_iter_get_line_offset (location);
	
	gtk_text_buffer_get_iter_at_line_index (doc,
						&iter,
						line,
						line_offset);
	
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
	gint line;
	gint line_offset;
	gulong id;
	
	if (textbuffer == GTK_TEXT_BUFFER (binding->docs[REAL]))
	{
		doc = GTK_TEXT_BUFFER (binding->docs[VIRTUAL]);
		id = binding->delete_range_id[VIRTUAL];
	}
	else
	{
		doc = GTK_TEXT_BUFFER (binding->docs[REAL]);
		id = binding->delete_range_id[REAL];
	}
	
	line = gtk_text_iter_get_line (start);
	line_offset = gtk_text_iter_get_line_offset (start);
	
	gtk_text_buffer_get_iter_at_line_index (doc,
						&start_iter,
						line,
						line_offset);
	
	line = gtk_text_iter_get_line (end);
	line_offset = gtk_text_iter_get_line_offset (end);
	
	gtk_text_buffer_get_iter_at_line_index (doc,
						&end_iter,
						line,
						line_offset);
	
	g_signal_handler_block (doc, id);
	gtk_text_buffer_delete (doc, &start_iter, &end_iter);
	g_signal_handler_unblock (doc, id);
}

static void
bind (Binding *binding,
      gboolean loaded)
{
	binding->insert_text_id[REAL] =
		g_signal_connect (binding->docs[REAL], "insert-text",
				  G_CALLBACK (insert_text_on_virtual_doc_cb),
				  binding);
	
	binding->delete_range_id[REAL] =
		g_signal_connect (binding->docs[REAL], "delete-range",
				  G_CALLBACK (delete_range_on_virtual_doc_cb),
				  binding);

	if (loaded)
	{
		binding->insert_text_id[VIRTUAL] =
		g_signal_connect (binding->docs[VIRTUAL], "insert-text",
				  G_CALLBACK (insert_text_on_virtual_doc_cb),
				  binding);

		binding->delete_range_id[VIRTUAL] =
			g_signal_connect (binding->docs[VIRTUAL], "delete-range",
					  G_CALLBACK (delete_range_on_virtual_doc_cb),
					  binding);
	}

	/* Bind all properties */
	binding->bindings[AUTO_INDENT] =
		gedit_data_binding_new_mutual (binding->views[REAL], "auto-indent",
					       binding->views[VIRTUAL], "auto-indent");
	binding->bindings[DRAW_SPACES] =
		gedit_data_binding_new_mutual (binding->views[REAL], "draw-spaces",
					       binding->views[VIRTUAL], "draw-spaces");
	binding->bindings[HIGHLIGHT_CURRENT_LINE] =
		gedit_data_binding_new_mutual (binding->views[REAL], "highlight-current-line",
					       binding->views[VIRTUAL], "highlight-current-line");
	binding->bindings[INDENT_ON_TAB] =
		gedit_data_binding_new_mutual (binding->views[REAL], "indent-on-tab",
					       binding->views[VIRTUAL], "indent-on-tab");
	binding->bindings[INDENT_WIDTH] =
		gedit_data_binding_new_mutual (binding->views[REAL], "indent-width",
					       binding->views[VIRTUAL], "indent-width");
	binding->bindings[INSERT_SPACES_INSTEAD_OF_TABS] =
		gedit_data_binding_new_mutual (binding->views[REAL], "insert-spaces-instead-of-tabs",
					       binding->views[VIRTUAL], "insert-spaces-instead-of-tabs");
	binding->bindings[RIGHT_MARGIN_POSITION] =
		gedit_data_binding_new_mutual (binding->views[REAL], "right-margin-position",
					       binding->views[VIRTUAL], "right-margin-position");
	binding->bindings[SHOW_LINE_MARKS] =
		gedit_data_binding_new_mutual (binding->views[REAL], "show-line-marks",
					       binding->views[VIRTUAL], "show-line-marks");
	binding->bindings[SHOW_LINE_NUMBERS] =
		gedit_data_binding_new_mutual (binding->views[REAL], "show-line-numbers",
					       binding->views[VIRTUAL], "show-line-numbers");
	binding->bindings[SHOW_RIGHT_MARGIN] =
		gedit_data_binding_new_mutual (binding->views[REAL], "show-right-margin",
					       binding->views[VIRTUAL], "show-right-margin");
	binding->bindings[SMART_HOME_END] =
		gedit_data_binding_new_mutual (binding->views[REAL], "smart-home-end",
					       binding->views[VIRTUAL], "smart-home-end");
	binding->bindings[TAB_WIDTH] =
		gedit_data_binding_new_mutual (binding->views[REAL], "tab-width",
					       binding->views[VIRTUAL], "tab-width");

	binding->bindings[HIGHLIGHT_MATCHING_BRACKETS] =
		gedit_data_binding_new_mutual (binding->docs[REAL], "highlight-matching-brackets",
					       binding->docs[VIRTUAL], "highlight-matching-brackets");
	binding->bindings[HIGHLIGHT_SYNTAX] =
		gedit_data_binding_new_mutual (binding->docs[REAL], "highlight-syntax",
					       binding->docs[VIRTUAL], "highlight-syntax");
	binding->bindings[LANGUAGE] =
		gedit_data_binding_new_mutual (binding->docs[REAL], "language",
					       binding->docs[VIRTUAL], "language");
	binding->bindings[MAX_UNDO_LEVELS] =
		gedit_data_binding_new_mutual (binding->docs[REAL], "max-undo-levels",
					       binding->docs[VIRTUAL], "max-undo-levels");
	binding->bindings[STYLE_SCHEME] =
		gedit_data_binding_new_mutual (binding->docs[REAL], "style-scheme",
					       binding->docs[VIRTUAL], "style-scheme");
}

static void
on_container_destroy (GeditViewContainer *container,
		      GeditPage          *page)
{
	DestroyData *destroy_data;
	Binding *binding;
	VirtualDocument *vdoc;
	GList *l, *aux_bind = NULL;
	
	destroy_data = g_object_get_data (G_OBJECT (container), DESTROY_DATA_KEY);
	
	if (destroy_data == NULL)
		return;

	vdoc = destroy_data->vdoc;
	
	binding = get_binding_from_view (destroy_data);
	
	/* FIXME: There are appends inside loops */
	while (binding != NULL)
	{
		aux_bind = g_list_append (aux_bind, binding);
	
		vdoc->bindings = g_list_remove (vdoc->bindings, binding);
		
		unbind (binding);
		
		binding = get_binding_from_view (destroy_data);
	}
	
	for (l = aux_bind; l != NULL; l = g_list_next (l))
	{
		binding = (Binding *)l->data;
		
		if (l->next != NULL)
		{
			Binding *b = (Binding *)l->next->data;
			Binding *b2;
			gint i, j;
			
			/* We need the opposite */
			if (destroy_data->view == b->views[REAL])
				i = VIRTUAL;
			else
				i = REAL;
			if (destroy_data->view == binding->views[REAL])
				j = VIRTUAL;
			else
				j = REAL;
		
			b2 = g_slice_new (Binding);
			b2->views[REAL] = b->views[i];
			b2->views[VIRTUAL] = binding->views[j];
			b2->docs[REAL] = b->docs[i];
			b2->docs[VIRTUAL] = binding->docs[j];
		
			bind (b2, TRUE);
		
			vdoc->bindings = g_list_append (vdoc->bindings, b2);
		}
		
		g_slice_free (Binding, binding);
	}
	
	g_signal_emit (G_OBJECT (page),
		       signals[CONTAINER_REMOVED],
		       0,
		       container);
}

static void
on_grab_focus (GeditView *view,
	       GeditPage *page)
{
	GeditViewContainer *container;
	
	container = gedit_view_container_get_from_view (view);

	if (page->priv->active_container == container)
		return;
	
	page->priv->active_container = container;
	
	g_signal_emit (G_OBJECT (page),
		       signals[ACTIVE_CONTAINER_CHANGED],
		       0,
		       container);
}

static void
add_container (GeditPage *page,
	       GtkWidget *container,
	       gboolean   main_container,
	       gboolean   horizontal)
{
	GList *views, *l;
	
	if (main_container)
	{
		gtk_box_pack_start (GTK_BOX (page), GTK_WIDGET (container),
				    TRUE, TRUE, 0);
	}
	else
	{
		GtkWidget *active_container = GTK_WIDGET (page->priv->active_container);
		GtkWidget *paned;
		GtkWidget *parent;
		
		if (horizontal)
			paned = gtk_hpaned_new ();
		else
			paned = gtk_vpaned_new ();
	
		gtk_widget_show (paned);
	
		/* First we remove the active container from its parent to make
		   this we add a ref to it*/
		g_object_ref (active_container);
		parent = gtk_widget_get_parent (active_container);
	
		gtk_container_remove (GTK_CONTAINER (parent), active_container);
		gtk_container_add (GTK_CONTAINER (parent), paned);
	
		gtk_paned_pack1 (GTK_PANED (paned), active_container, FALSE, TRUE);
		g_object_unref (active_container);
	
		gtk_paned_pack2 (GTK_PANED (paned), container, FALSE, TRUE);
	}
	
	views = gedit_view_container_get_views (GEDIT_VIEW_CONTAINER (container));
	for (l = views; l != NULL; l = g_list_next (l))
	{
		g_signal_connect (l->data,
				  "grab-focus",
				  G_CALLBACK (on_grab_focus),
				  page);
	}
	
	page->priv->containers = g_list_append (page->priv->containers, container);
	
	g_signal_emit (G_OBJECT (page),
		       signals[CONTAINER_ADDED],
		       0,
		       container);
}

static void
remove_container (GeditPage *page,
		  GtkWidget *container)
{
	GtkWidget *parent;
	GtkWidget *grandpa;
	GList *children;
	
	parent = gtk_widget_get_parent (container);
	if (GEDIT_IS_PAGE (parent))
	{
		g_warning ("We can't have an empty page");
		return;
	}
	
	/* Now we destroy the widget, we get the children of parent and we destroy
	  parent too as the parent is an useless paned. Finally we add the child
	  into the grand parent */
	g_object_ref (container);
	gtk_widget_destroy (container);
	
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
	
	if (page->priv->active_container == GEDIT_VIEW_CONTAINER (container))
	{
		_gedit_page_switch_between_splits (page);
		page->priv->containers = g_list_remove (page->priv->containers,
							container);
	}
	
	g_list_free (children);
	g_object_unref (container);
}

static void
on_virtual_document_loaded_cb (GeditDocument *document,
			       const GError  *error,
			       gpointer       user_data)
{
	Binding *binding = (Binding *)user_data;

	binding->insert_text_id[VIRTUAL] =
		g_signal_connect (document, "insert-text",
				  G_CALLBACK (insert_text_on_virtual_doc_cb),
				  binding);

	binding->delete_range_id[VIRTUAL] =
		g_signal_connect (document, "delete-range",
				  G_CALLBACK (delete_range_on_virtual_doc_cb),
				  binding);
}

static void
split_page (GeditPage *page,
	    gboolean   horizontally)
{
	DestroyData *destroy_data;
	VirtualDocument *vdoc;
	Binding *binding;
	GtkWidget *container;
	GeditView *view;
	gchar *uri;
	
	g_return_if_fail (GEDIT_IS_PAGE (page));
	
	view = gedit_view_container_get_view (page->priv->active_container);
	
	g_return_if_fail (GEDIT_IS_TEXT_VIEW (view));
	
	container = _gedit_view_container_new ();
	gtk_widget_show (container);
	add_container (page, container, FALSE, horizontally);
	
	vdoc = g_object_get_data (G_OBJECT (page->priv->active_container),
				  VIRTUAL_DOCUMENT_KEY);
	
	if (vdoc == NULL)
	{
		vdoc = g_slice_new (VirtualDocument);
		vdoc->bindings = NULL;
		
		g_object_set_data (G_OBJECT (page->priv->active_container),
				   VIRTUAL_DOCUMENT_KEY,
				   vdoc);
	}
	
	g_object_set_data (G_OBJECT (container),
			   VIRTUAL_DOCUMENT_KEY,
			   vdoc);
	
	destroy_data = g_object_get_data (G_OBJECT (page->priv->active_container),
					  DESTROY_DATA_KEY);
	
	if (destroy_data == NULL)
	{
		destroy_data = g_slice_new (DestroyData);
		destroy_data->vdoc = vdoc;
		destroy_data->view = GTK_WIDGET (view);
		
		g_object_set_data (G_OBJECT (page->priv->active_container),
				   DESTROY_DATA_KEY,
				   destroy_data);
		
		g_signal_connect (page->priv->active_container,
				  "destroy",
				  G_CALLBACK (on_container_destroy),
				  page);
	}
	
	g_object_set_data (G_OBJECT (container),
			   DESTROY_DATA_KEY,
			   destroy_data);
	
	g_signal_connect (container,
			  "destroy",
			  G_CALLBACK (on_container_destroy),
			  page);
	
	binding = g_slice_new (Binding);
	binding->views[REAL] = GTK_WIDGET (view);
	binding->views[VIRTUAL] = GTK_WIDGET (gedit_view_container_get_view (GEDIT_VIEW_CONTAINER (container)));
	binding->docs[REAL] = gedit_view_container_get_document (page->priv->active_container);
	binding->docs[VIRTUAL] = gedit_view_container_get_document (GEDIT_VIEW_CONTAINER (container));
	
	vdoc->bindings = g_list_append (vdoc->bindings, binding);
	
	uri = gedit_document_get_uri (binding->docs[REAL]);
	if (uri != NULL)
	{
		GtkTextMark *insert_mark;
		GtkTextIter iter;
		
		/* Load the document */
		insert_mark = gtk_text_buffer_get_insert (GTK_TEXT_BUFFER (binding->docs[REAL]));
		gtk_text_buffer_get_iter_at_mark (GTK_TEXT_BUFFER (binding->docs[REAL]),
						  &iter, insert_mark);
		
		gedit_document_load (binding->docs[VIRTUAL],
				     uri,
				     NULL,
				     gtk_text_iter_get_line (&iter) + 1,
				     FALSE);

		g_signal_connect (binding->docs[VIRTUAL], "loaded",
				  G_CALLBACK (on_virtual_document_loaded_cb),
				  binding);
		
		g_free (uri);
		
		bind (binding, FALSE);
	}
	else
	{
		GtkTextIter start, end;
		gchar *content;
		
		gtk_text_buffer_get_bounds (GTK_TEXT_BUFFER (binding->docs[REAL]),
					    &start, &end);
		
		content = gtk_text_buffer_get_text (GTK_TEXT_BUFFER (binding->docs[REAL]),
						    &start, &end, FALSE);
		
		gtk_text_buffer_set_text (GTK_TEXT_BUFFER (binding->docs[VIRTUAL]),
					  content, -1);
		
		g_free (content);
		
		bind (binding, TRUE);
	}
}

static void
gedit_page_finalize (GObject *object)
{
	GeditPage *page = GEDIT_PAGE (object);
	
	g_list_free (page->priv->containers);

	G_OBJECT_CLASS (gedit_page_parent_class)->finalize (object);
}

static void
gedit_page_class_init (GeditPageClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	
	object_class->finalize = gedit_page_finalize;
	
	signals[CONTAINER_ADDED] =
		g_signal_new ("container-added",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (GeditPageClass, container_added),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE,
			      1,
			      GEDIT_TYPE_VIEW_CONTAINER);

	signals[CONTAINER_REMOVED] =
		g_signal_new ("container-removed",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (GeditPageClass, container_removed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE,
			      1,
			      GEDIT_TYPE_VIEW_CONTAINER);

	signals[ACTIVE_CONTAINER_CHANGED] =
		g_signal_new ("active-container-changed",
			      G_TYPE_FROM_CLASS (object_class),
			      G_SIGNAL_RUN_FIRST,
			      G_STRUCT_OFFSET (GeditPageClass, active_container_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE,
			      1,
			      GEDIT_TYPE_VIEW_CONTAINER);

	g_type_class_add_private (object_class, sizeof (GeditPagePrivate));
}

static void
gedit_page_init (GeditPage *page)
{
	page->priv = GEDIT_PAGE_GET_PRIVATE (page);
}

/**
 * gedit_page_new:
 * @container: a #GeditViewContainer or %NULL
 *
 * Creates a new #GeditPage. If @container is %NULL a new default
 * #GeditViewContainer is created.
 *
 * Returns: a new #GeditPage object.
 */
GtkWidget *
_gedit_page_new (GeditViewContainer *container)
{
	GeditPage *page;
	
	page = g_object_new (GEDIT_TYPE_PAGE, NULL);
	
	if (container == NULL)
	{
		container = GEDIT_VIEW_CONTAINER (_gedit_view_container_new ());
	}
	
	gtk_widget_show (GTK_WIDGET (container));
	add_container (page, GTK_WIDGET (container), TRUE, FALSE);
	page->priv->active_container = container;
	
	return GTK_WIDGET (page);
}

GeditViewContainer *
gedit_page_get_active_view_container (GeditPage *page)
{
	g_return_val_if_fail (GEDIT_IS_PAGE (page), NULL);
	
	return page->priv->active_container;
}

GList *
gedit_page_get_view_containers (GeditPage *page)
{
	g_return_val_if_fail (GEDIT_IS_PAGE (page), NULL);
	
	return page->priv->containers;
}

GeditPage *
gedit_page_get_from_container (GeditViewContainer *container)
{
	GtkWidget *parent;
	
	g_return_val_if_fail (GEDIT_IS_VIEW_CONTAINER (container), NULL);
	
	parent = gtk_widget_get_parent (GTK_WIDGET (container));
	
	/* We are going to have X paned widgets and then the page */
	while (!GTK_IS_VBOX (parent))
	{
		parent = gtk_widget_get_parent (parent);
	}
	
	return GEDIT_PAGE (parent);
}

void
_gedit_page_switch_between_splits (GeditPage *page)
{
	GeditView *view;
	GList *active;
	
	active = g_list_find (page->priv->containers,
			      page->priv->active_container);
	
	if (active->next)
		active = active->next;
	else
		active = g_list_first (active);
	
	/* Maybe there are no more views in the page */
	if (active == NULL)
		return;
	
	view = gedit_view_container_get_view (GEDIT_VIEW_CONTAINER (active->data));
	gtk_widget_grab_focus (GTK_WIDGET (view));
}

void
_gedit_page_unsplit (GeditPage *page)
{
	remove_container (page, GTK_WIDGET (page->priv->active_container));
}

gboolean
_gedit_page_can_unsplit (GeditPage *page)
{
	GtkWidget *parent;
	
	g_return_val_if_fail (GEDIT_IS_PAGE (page), FALSE);
	
	parent = gtk_widget_get_parent (GTK_WIDGET (page->priv->active_container));

	if (parent == GTK_WIDGET (page))
		return FALSE;
	
	return TRUE;
}

gboolean
_gedit_page_can_split (GeditPage *page)
{
	GeditView *view;
	
	g_return_val_if_fail (GEDIT_IS_PAGE (page), FALSE);
	
	view = gedit_view_container_get_view (page->priv->active_container);
	
	if (GEDIT_IS_TEXT_VIEW (view))
		return TRUE;
	
	return FALSE;
}

void
_gedit_page_split_horizontally (GeditPage *page)
{
	split_page (page, TRUE);
}

void
_gedit_page_split_vertically (GeditPage *page)
{
	split_page (page, FALSE);
}
