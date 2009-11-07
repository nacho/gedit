/*
 * gedit-documents-panel.c
 * This file is part of gedit
 *
 * Copyright (C) 2005 - Paolo Maggi 
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
 * Modified by the gedit Team, 2005. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 *
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "gedit-documents-panel.h"
#include "gedit-utils.h"
#include "gedit-notebook.h"

#include <glib/gi18n.h>

#define GEDIT_DOCUMENTS_PANEL_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), \
						  GEDIT_TYPE_DOCUMENTS_PANEL,            \
						  GeditDocumentsPanelPrivate))

struct _GeditDocumentsPanelPrivate
{
	GeditWindow  *window;

	GtkWidget    *treeview;
	GtkTreeModel *model;
	
	gboolean      adding_page;
	gboolean      is_reodering;
};

G_DEFINE_TYPE(GeditDocumentsPanel, gedit_documents_panel, GTK_TYPE_VBOX)

enum
{
	PROP_0,
	PROP_WINDOW,
};

#define MAX_DOC_NAME_LENGTH 60

static gchar *
page_get_name (GeditPage *page)
{
	GeditViewContainer *container;
	GeditDocument *doc;
	gchar *name;
	gchar *docname;
	gchar *page_name;

	g_return_val_if_fail (GEDIT_IS_PAGE (page), NULL);

	container = gedit_page_get_active_view_container (page);
	
	/* FIXME: check container == NULL? */
	doc = gedit_view_container_get_document (container);

	name = gedit_document_get_short_name_for_display (doc);

	/* Truncate the name so it doesn't get insanely wide. */
	docname = gedit_utils_str_middle_truncate (name, MAX_DOC_NAME_LENGTH);

	if (gedit_document_get_modified (doc))
	{
		if (gedit_document_get_readonly (doc))
		{
			page_name = g_markup_printf_escaped ("<i>%s</i> [<i>%s</i>]",
							     docname,
							     _("Read Only"));
		}
		else
		{
			page_name = g_markup_printf_escaped ("<i>%s</i>", 
							     docname);
		}
	}
	else
	{
		if (gedit_document_get_readonly (doc))
		{
			page_name = g_markup_printf_escaped ("%s [<i>%s</i>]",
							     docname,
							     _("Read Only"));
		}
		else
		{
			page_name = g_markup_escape_text (docname, -1);
		}
	}

	g_free (docname);
	g_free (name);

	return page_name;
}

static void
get_iter_from_page (GeditDocumentsPanel *panel, GeditPage *page, GtkTreeIter *iter)
{
	gint num;
	GtkWidget *nb;
	GtkTreePath *path;

	nb = _gedit_window_get_notebook (panel->priv->window);
	num = gtk_notebook_page_num (GTK_NOTEBOOK (nb),
				     GTK_WIDGET (page));

	path = gtk_tree_path_new_from_indices (num, -1);
	gtk_tree_model_get_iter (panel->priv->model,
        	                 iter,
        	                 path);
	gtk_tree_path_free (path);
}

static void
window_active_page_changed (GeditWindow         *window,
			    GeditPage           *page,
			    GeditDocumentsPanel *panel)
{	
	g_return_if_fail (page != NULL);

	if (!_gedit_window_is_removing_pages (window))
	{
		GtkTreeIter iter;
		GtkTreeSelection *selection;

		get_iter_from_page (panel, page, &iter);

		if (gtk_list_store_iter_is_valid (GTK_LIST_STORE (panel->priv->model),
						  &iter))
		{
			selection = gtk_tree_view_get_selection (
					GTK_TREE_VIEW (panel->priv->treeview));

			gtk_tree_selection_select_iter (selection, &iter);
		}
	}
}

static void
refresh_list (GeditDocumentsPanel *panel)
{
	/* TODO: refresh the list only if the panel is visible */

	GList *pages;
	GList *l;
	GtkWidget *nb;
	GtkListStore *list_store;
	GeditPage *active_page;

	/* g_debug ("refresh_list"); */
	
	list_store = GTK_LIST_STORE (panel->priv->model);

	gtk_list_store_clear (list_store);

	active_page = gedit_window_get_active_page (panel->priv->window);

	nb = _gedit_window_get_notebook (panel->priv->window);

	pages = gtk_container_get_children (GTK_CONTAINER (nb));
	l = pages;

	panel->priv->adding_page = TRUE;
	
	while (l != NULL)
	{	
		GdkPixbuf *pixbuf;
		gchar *name;
		GtkTreeIter iter;
		GeditViewContainer *container;

		container = gedit_page_get_active_view_container (GEDIT_PAGE (l->data));

		name = page_get_name (GEDIT_PAGE (l->data));
		pixbuf = _gedit_view_container_get_icon (container);

		/* Add a new row to the model */
		gtk_list_store_append (list_store, &iter);
		gtk_list_store_set (list_store, 
				    &iter,
				    0, pixbuf,
				    1, name,
				    2, l->data,
				    -1);

		g_free (name);
		if (pixbuf != NULL)
			g_object_unref (pixbuf);

		if (l->data == active_page)
		{
			GtkTreeSelection *selection;

			selection = gtk_tree_view_get_selection (
					GTK_TREE_VIEW (panel->priv->treeview));

			gtk_tree_selection_select_iter (selection, &iter);
		}

		l = g_list_next (l);
	}
	
	panel->priv->adding_page = FALSE;

	g_list_free (pages);
}

static void
sync_name_and_icon (GeditPage           *page,
		    GeditViewContainer  *container,
		    GeditDocumentsPanel *panel)
{
	GdkPixbuf *pixbuf;
	gchar *name;
	GtkTreeIter iter;

	get_iter_from_page (panel, page, &iter);
	
	name = page_get_name (page);
	pixbuf = _gedit_view_container_get_icon (container);

	gtk_list_store_set (GTK_LIST_STORE (panel->priv->model),
			    &iter,
			    0, pixbuf,
			    1, name,
			    2, page,
			    -1);

	g_free (name);
	if (pixbuf != NULL)
		g_object_unref (pixbuf);
}

static void
on_notify_name_changed (GeditViewContainer  *container,
			GParamSpec          *pspec,
			GeditDocumentsPanel *panel)
{
	GeditPage *page;

	page = gedit_page_get_from_container (container);

	sync_name_and_icon (page, container, panel);
}

static void
window_page_removed (GeditWindow         *window,
		     GeditPage           *page,
		     GeditDocumentsPanel *panel)
{
	g_signal_handlers_disconnect_by_func (page,
					      G_CALLBACK (sync_name_and_icon),
					      panel);

	if (_gedit_window_is_removing_pages (window))
		gtk_list_store_clear (GTK_LIST_STORE (panel->priv->model));
	else
		refresh_list (panel);
}

static void
window_page_added (GeditWindow         *window,
		   GeditPage           *page,
		   GeditDocumentsPanel *panel)
{
	GtkTreeIter iter;
	GtkTreeIter sibling;
	GdkPixbuf *pixbuf;
	gchar *name;
	GList *containers, *l;
	GeditViewContainer *container;
	
	/* FIXME: split-added instead ? */
	
	containers = gedit_page_get_view_containers (page);

	for (l = containers; l != NULL; l = g_list_next (l))
	{
		g_signal_connect (l->data,
				 "notify::name",
				  G_CALLBACK (on_notify_name_changed),
				  panel);

		g_signal_connect (l->data, 
				 "notify::state",
				  G_CALLBACK (on_notify_name_changed),
				  panel);
	}
	
	g_signal_connect (page,
			  "active-container-changed",
			  G_CALLBACK (sync_name_and_icon),
			  panel);

	get_iter_from_page (panel, page, &sibling);

	panel->priv->adding_page = TRUE;
	
	if (gtk_list_store_iter_is_valid (GTK_LIST_STORE (panel->priv->model), 
					  &sibling)) 
	{
		gtk_list_store_insert_after (GTK_LIST_STORE (panel->priv->model),
					     &iter,
					     &sibling);
	}
	else
	{
		GeditPage *active_page;

		gtk_list_store_append (GTK_LIST_STORE (panel->priv->model), 
				       &iter);

		active_page = gedit_window_get_active_page (panel->priv->window);

		if (page == active_page)
		{
			GtkTreeSelection *selection;

			selection = gtk_tree_view_get_selection (
						GTK_TREE_VIEW (panel->priv->treeview));

			gtk_tree_selection_select_iter (selection, &iter);
		}
	}

	container = gedit_page_get_active_view_container (page);

	name = page_get_name (page);
	pixbuf = _gedit_view_container_get_icon (container);

	gtk_list_store_set (GTK_LIST_STORE (panel->priv->model),
			    &iter,
			    0, pixbuf,
			    1, name,
			    2, page,
			    -1);

	g_free (name);
	if (pixbuf != NULL)
		g_object_unref (pixbuf);

	panel->priv->adding_page = FALSE;
}

static void
window_pages_reordered (GeditWindow         *window,
			GeditDocumentsPanel *panel)
{
	if (panel->priv->is_reodering)
		return;

	refresh_list (panel);
}

static void
set_window (GeditDocumentsPanel *panel,
	    GeditWindow         *window)
{
	g_return_if_fail (panel->priv->window == NULL);
	g_return_if_fail (GEDIT_IS_WINDOW (window));

	panel->priv->window = g_object_ref (window);

	g_signal_connect (window,
			  "page_added",
			  G_CALLBACK (window_page_added),
			  panel);
	g_signal_connect (window,
			  "page_removed",
			  G_CALLBACK (window_page_removed),
			  panel);
	g_signal_connect (window,
			  "pages_reordered",
			  G_CALLBACK (window_pages_reordered),
			  panel);
	g_signal_connect (window,
			  "active_page_changed",
			  G_CALLBACK (window_active_page_changed),
			  panel);
}

static void
treeview_cursor_changed (GtkTreeView         *view,
			 GeditDocumentsPanel *panel)
{
	GtkTreeIter iter;
	GtkTreeSelection *selection;
	gpointer page;

	selection = gtk_tree_view_get_selection (
				GTK_TREE_VIEW (panel->priv->treeview));

	if (gtk_tree_selection_get_selected (selection, NULL, &iter))
	{
		gtk_tree_model_get (panel->priv->model,
				    &iter,
				    2,
				    &page,
				    -1);

		if (gedit_window_get_active_page (panel->priv->window) != page)
		{
			gedit_window_set_active_page (panel->priv->window,
						      GEDIT_PAGE (page));
		}
	}
}

static void
gedit_documents_panel_set_property (GObject      *object,
				    guint         prop_id,
				    const GValue *value,
				    GParamSpec   *pspec)
{
	GeditDocumentsPanel *panel = GEDIT_DOCUMENTS_PANEL (object);

	switch (prop_id)
	{
		case PROP_WINDOW:
			set_window (panel, g_value_get_object (value));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_documents_panel_get_property (GObject    *object,
				    guint       prop_id,
				    GValue     *value,
				    GParamSpec *pspec)
{
	GeditDocumentsPanel *panel = GEDIT_DOCUMENTS_PANEL (object);

	switch (prop_id)
	{
		case PROP_WINDOW:
			g_value_set_object (value,
					    GEDIT_DOCUMENTS_PANEL_GET_PRIVATE (panel)->window);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_documents_panel_finalize (GObject *object)
{
	/* GeditDocumentsPanel *tab = GEDIT_DOCUMENTS_PANEL (object); */
	
	// TODO: disconnect signal with window

	G_OBJECT_CLASS (gedit_documents_panel_parent_class)->finalize (object);
}

static void
gedit_documents_panel_dispose (GObject *object)
{
	GeditDocumentsPanel *panel = GEDIT_DOCUMENTS_PANEL (object);

	if (panel->priv->window != NULL) {
		g_object_unref (panel->priv->window);
		panel->priv->window = NULL;
	}

	G_OBJECT_CLASS (gedit_documents_panel_parent_class)->dispose (object);
}

static void 
gedit_documents_panel_class_init (GeditDocumentsPanelClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = gedit_documents_panel_finalize;
	object_class->dispose = gedit_documents_panel_dispose;
	object_class->get_property = gedit_documents_panel_get_property;
	object_class->set_property = gedit_documents_panel_set_property;

	g_object_class_install_property (object_class,
					 PROP_WINDOW,
					 g_param_spec_object ("window",
							 "Window",
							 "The GeditWindow this GeditDocumentsPanel is associated with",
							 GEDIT_TYPE_WINDOW,
							 G_PARAM_READWRITE |
							 G_PARAM_CONSTRUCT_ONLY |
							 G_PARAM_STATIC_STRINGS));

	g_type_class_add_private (object_class, sizeof(GeditDocumentsPanelPrivate));
}

static GtkTreePath *
get_current_path (GeditDocumentsPanel *panel)
{
	gint num;
	GtkWidget *nb;
	GtkTreePath *path;

	nb = _gedit_window_get_notebook (panel->priv->window);
	num = gtk_notebook_get_current_page (GTK_NOTEBOOK (nb));

	path = gtk_tree_path_new_from_indices (num, -1);

	return path;
}

static void
menu_position (GtkMenu             *menu,
	       gint                *x,
	       gint                *y,
	       gboolean            *push_in,
	       GeditDocumentsPanel *panel)
{
	GtkTreePath *path;
	GdkRectangle rect;
	gint wx, wy;
	GtkRequisition requisition;
	GtkWidget *w;

	w = panel->priv->treeview;

	path = get_current_path (panel);

	gtk_tree_view_get_cell_area (GTK_TREE_VIEW (w),
				     path,
				     NULL,
				     &rect);

	wx = rect.x;
	wy = rect.y;

	gdk_window_get_origin (w->window, x, y);
	
	gtk_widget_size_request (GTK_WIDGET (menu), &requisition);

	if (gtk_widget_get_direction (w) == GTK_TEXT_DIR_RTL)
	{
		*x += w->allocation.x + w->allocation.width - requisition.width - 10;
	}
	else
	{
		*x += w->allocation.x + 10 ;
	}

	wy = MAX (*y + 5, *y + wy + 5);
	wy = MIN (wy, *y + w->allocation.height - requisition.height - 5);
	
	*y = wy;

	*push_in = TRUE;
}

static gboolean
show_popup_menu (GeditDocumentsPanel *panel,
		 GdkEventButton      *event)
{
	GtkWidget *menu;

	menu = gtk_ui_manager_get_widget (gedit_window_get_ui_manager (panel->priv->window),
					 "/NotebookPopup");
	g_return_val_if_fail (menu != NULL, FALSE);

	if (event != NULL)
	{
		gtk_menu_popup (GTK_MENU (menu), 
				NULL, 
				NULL,
				NULL, 
				NULL,
				event->button, 
				event->time);
	}
	else
	{
		gtk_menu_popup (GTK_MENU (menu), 
				NULL, 
				NULL,
				(GtkMenuPositionFunc) menu_position, 
				panel,
				0, 
				gtk_get_current_event_time ());

		gtk_menu_shell_select_first (GTK_MENU_SHELL (menu), FALSE);
	}

	return TRUE;
}

static gboolean
panel_button_press_event (GtkTreeView         *treeview,
			  GdkEventButton      *event,
			  GeditDocumentsPanel *panel)
{
	if ((GDK_BUTTON_PRESS == event->type) && (3 == event->button))
	{
		GtkTreePath* path = NULL;
		
		if (event->window == gtk_tree_view_get_bin_window (treeview))
		{
			/* Change the cursor position */
			if (gtk_tree_view_get_path_at_pos (treeview,
							   event->x,
							   event->y,
							   &path,
							   NULL,
							   NULL,
							   NULL))
			{				
			
				gtk_tree_view_set_cursor (treeview,
							  path,
							  NULL,
							  FALSE);
					
				gtk_tree_path_free (path);
							   
				/* A row exists at mouse position */
				return show_popup_menu (panel, event);
			}
		}
	}
	
	return FALSE;
}

static gboolean
panel_popup_menu (GtkWidget           *treeview,
		  GeditDocumentsPanel *panel)
{
	/* Only respond if the treeview is the actual focus */
	if (gtk_window_get_focus (GTK_WINDOW (panel->priv->window)) == treeview)
	{
		return show_popup_menu (panel, NULL);
	}

	return FALSE;
}

static gboolean
treeview_query_tooltip (GtkWidget  *widget,
			gint        x,
			gint        y,
			gboolean    keyboard_tip,
			GtkTooltip *tooltip,
			gpointer    data)
{
	GtkTreeIter iter;
	GtkTreeView *tree_view = GTK_TREE_VIEW (widget);
	GtkTreeModel *model = gtk_tree_view_get_model (tree_view);
	GtkTreePath *path = NULL;
	gpointer *page;
	GeditViewContainer *container;
	gchar *tip;

	if (keyboard_tip)
	{
		gtk_tree_view_get_cursor (tree_view, &path, NULL);

		if (path == NULL)
		{
			return FALSE;
		}
	}
	else
	{
		gint bin_x, bin_y;

		gtk_tree_view_convert_widget_to_bin_window_coords (tree_view,
								   x, y,
								   &bin_x, &bin_y);
      
		if (!gtk_tree_view_get_path_at_pos (tree_view,
						    bin_x, bin_y,
						    &path,
						    NULL, NULL, NULL))
		{
			return FALSE;
		}
	}

	gtk_tree_model_get_iter (model, &iter, path);
	gtk_tree_model_get (model, 
			    &iter, 
			    2, 
			    &page, 
			    -1);

	container = gedit_page_get_active_view_container (GEDIT_PAGE (page));
	tip = _gedit_view_container_get_tooltips (container);
	gtk_tooltip_set_markup (tooltip, tip);

	g_free (tip);
	gtk_tree_path_free (path);

	return TRUE;
}

static void
treeview_row_inserted (GtkTreeModel        *tree_model,
		       GtkTreePath         *path,
		       GtkTreeIter         *iter,
		       GeditDocumentsPanel *panel)
{
	GeditPage *page;
	gint *indeces;
	GtkWidget *nb;
	gint old_position;
	gint new_position;
	
	if (panel->priv->adding_page)
		return;
		
	page = gedit_window_get_active_page (panel->priv->window);
	g_return_if_fail (page != NULL);

	panel->priv->is_reodering = TRUE;
	
	indeces = gtk_tree_path_get_indices (path);
	
	/* g_debug ("New Index: %d (path: %s)", indeces[0], gtk_tree_path_to_string (path));*/
	
	nb = _gedit_window_get_notebook (panel->priv->window);

	new_position = indeces[0];
	old_position = gtk_notebook_page_num (GTK_NOTEBOOK (nb), 
				    	      GTK_WIDGET (page));
	if (new_position > old_position)
		new_position = MAX (0, new_position - 1);
		
	gedit_notebook_reorder_page (GEDIT_NOTEBOOK (nb),
				     page,
				     new_position);

	panel->priv->is_reodering = FALSE;
}

static void
gedit_documents_panel_init (GeditDocumentsPanel *panel)
{
	GtkWidget 		*sw;
	GtkTreeViewColumn	*column;
	GtkCellRenderer 	*cell;
	GtkTreeSelection 	*selection;

	panel->priv = GEDIT_DOCUMENTS_PANEL_GET_PRIVATE (panel);
	
	panel->priv->adding_page = FALSE;
	panel->priv->is_reodering = FALSE;
	
	/* Create the scrolled window */
	sw = gtk_scrolled_window_new (NULL, NULL);
	g_return_if_fail (sw != NULL);
	
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw),
                                             GTK_SHADOW_IN);
	gtk_widget_show (sw);
	gtk_box_pack_start (GTK_BOX (panel), sw, TRUE, TRUE, 0);
	
	/* Create the empty model */
	panel->priv->model = GTK_TREE_MODEL (gtk_list_store_new (3, 
								 GDK_TYPE_PIXBUF, 
								 G_TYPE_STRING, 
								 G_TYPE_POINTER));

	/* Create the treeview */
	panel->priv->treeview = gtk_tree_view_new_with_model (panel->priv->model);
	g_object_unref (G_OBJECT (panel->priv->model));
  	gtk_container_add (GTK_CONTAINER (sw), panel->priv->treeview);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (panel->priv->treeview), FALSE);
	gtk_tree_view_set_reorderable (GTK_TREE_VIEW (panel->priv->treeview), TRUE);

	g_object_set (panel->priv->treeview, "has-tooltip", TRUE, NULL);

	gtk_widget_show (panel->priv->treeview);
	
	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (column, _("Documents"));

	cell = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (column, cell, FALSE);
	gtk_tree_view_column_add_attribute (column, cell, "pixbuf", 0);
	cell = gtk_cell_renderer_text_new ();
      	gtk_tree_view_column_pack_start (column, cell, TRUE);	
	gtk_tree_view_column_add_attribute (column, cell, "markup", 1);

	gtk_tree_view_append_column (GTK_TREE_VIEW (panel->priv->treeview),
				     column);
				     
     	selection = gtk_tree_view_get_selection (
			GTK_TREE_VIEW (panel->priv->treeview));

	gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
	
	g_signal_connect (panel->priv->treeview, 
			  "cursor_changed",
			  G_CALLBACK (treeview_cursor_changed),
			  panel);
	g_signal_connect (panel->priv->treeview,
			  "button-press-event",
			  G_CALLBACK (panel_button_press_event),
			  panel);
	g_signal_connect (panel->priv->treeview, 
			  "popup-menu",
			  G_CALLBACK (panel_popup_menu),
			  panel);
  	g_signal_connect (panel->priv->treeview,
			  "query-tooltip",
			  G_CALLBACK (treeview_query_tooltip),
			  NULL);

	g_signal_connect (panel->priv->model, 
			  "row-inserted",
			  G_CALLBACK (treeview_row_inserted),
			  panel);
}

GtkWidget *
gedit_documents_panel_new (GeditWindow *window)
{
	g_return_val_if_fail (GEDIT_IS_WINDOW (window), NULL);

	return GTK_WIDGET (g_object_new (GEDIT_TYPE_DOCUMENTS_PANEL, 
					 "window", window,
					 NULL));
}
