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
#include "gedit-debug.h"
#include "gedit-utils.h"
#include "gedit-multi-notebook.h"
#include "gedit-notebook.h"

#include <glib/gi18n.h>

#define GEDIT_DOCUMENTS_PANEL_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), \
						  GEDIT_TYPE_DOCUMENTS_PANEL,            \
						  GeditDocumentsPanelPrivate))

struct _GeditDocumentsPanelPrivate
{
	GeditWindow        *window;
	GeditMultiNotebook *mnb;

	GtkWidget          *treeview;
	GtkTreeModel       *model;

	guint               adding_tab : 1;
	guint               is_reodering : 1;
	guint               setting_active_notebook : 1;
};

G_DEFINE_TYPE(GeditDocumentsPanel, gedit_documents_panel, GTK_TYPE_VBOX)

enum
{
	PROP_0,
	PROP_WINDOW
};

enum
{
	PIXBUF_COLUMN = 0,
	NAME_COLUMN,
	NOTEBOOK_COLUMN,
	TAB_COLUMN,
	N_COLUMNS
};

#define MAX_DOC_NAME_LENGTH	60
#define NB_NAME_DATA_KEY	"DocumentsPanelNotebookNameKey"
#define TAB_NB_DATA_KEY		"DocumentsPanelTabNotebookKey"


static gchar *
notebook_get_name (GeditMultiNotebook *mnb,
		   GeditNotebook      *notebook)
{
	guint num;

	num = gedit_multi_notebook_get_notebook_num (mnb, notebook);

	return g_markup_printf_escaped ("Tab Group %i", num + 1);
}

static gchar *
tab_get_name (GeditTab *tab)
{
	GeditDocument *doc;
	gchar *name;
	gchar *docname;
	gchar *tab_name;

	gedit_debug (DEBUG_PANEL);

	g_return_val_if_fail (GEDIT_IS_TAB (tab), NULL);

	doc = gedit_tab_get_document (tab);

	name = gedit_document_get_short_name_for_display (doc);

	/* Truncate the name so it doesn't get insanely wide. */
	docname = gedit_utils_str_middle_truncate (name, MAX_DOC_NAME_LENGTH);

	if (gtk_text_buffer_get_modified (GTK_TEXT_BUFFER (doc)))
	{
		if (gedit_document_get_readonly (doc))
		{
			tab_name = g_markup_printf_escaped ("<i>%s</i> [<i>%s</i>]",
							    docname,
							    _("Read-Only"));
		}
		else
		{
			tab_name = g_markup_printf_escaped ("<i>%s</i>",
							    docname);
		}
	}
	else
	{
		if (gedit_document_get_readonly (doc))
		{
			tab_name = g_markup_printf_escaped ("%s [<i>%s</i>]",
							    docname,
							    _("Read-Only"));
		}
		else
		{
			tab_name = g_markup_escape_text (docname, -1);
		}
	}

	g_free (docname);
	g_free (name);

	return tab_name;
}

static gboolean
get_iter_from_tab (GeditDocumentsPanel *panel,
		   GeditNotebook       *notebook,
		   GeditTab            *tab,
		   GtkTreeIter         *tab_iter)
{
	GtkTreeIter parent;
	gboolean success;
	gboolean search_notebook;

	/* Note: we cannot use the functions of the MultiNotebook
	 *       because in cases where the notebook or tab has been
	 *       removed already we will fail to get an iter.
	 */

	gedit_debug (DEBUG_PANEL);

	g_assert (notebook != NULL || tab != NULL);
	g_assert (tab_iter != NULL);
	g_assert ((tab == NULL && tab_iter == NULL) || (tab != NULL && tab_iter != NULL));

	success = FALSE;
	search_notebook = (gedit_multi_notebook_get_n_notebooks (panel->priv->mnb) > 1);

	/* We never ever ask for a notebook or tab if it does not already exist in the tree */
	g_assert (gtk_tree_model_get_iter_first (panel->priv->model, &parent));

	do
	{
		if (search_notebook)
		{
			GeditNotebook *current_notebook;
			gboolean is_cur;

			gtk_tree_model_get (panel->priv->model,
					    &parent,
					    NOTEBOOK_COLUMN, &current_notebook,
					    -1);

			is_cur = (current_notebook == notebook);

			g_object_unref (current_notebook);

			if (is_cur)
			{
				success = TRUE;

				if (tab == NULL)
				{
					break;
				}
			}
		}
		else if (tab == NULL)
		{
			success = TRUE;

			break;
		}

		if (success || !search_notebook)
		{
			GeditTab *current_tab;
			GtkTreeIter iter;

			if (search_notebook)
			{
				g_assert (gtk_tree_model_iter_children (panel->priv->model,
									&iter, &parent));
			}
			else
			{
				iter = parent;
			}

			do
			{
				gboolean is_cur;

				gtk_tree_model_get (panel->priv->model,
						    &iter,
						    TAB_COLUMN, &current_tab,
						    -1);
				g_assert (current_tab != NULL);

				is_cur = (current_tab == tab);

				g_object_unref (current_tab);

				if (is_cur)
				{
					*tab_iter = iter;
					success = TRUE;

					/* break 2; */
					goto out;
				}
			} while (gtk_tree_model_iter_next (panel->priv->model, &iter));

			/* We already found the notebook and the tab was not found */
			g_assert (!success);
		}
	} while (gtk_tree_model_iter_next (panel->priv->model, &parent));

out:

	/* In the current code if we are not successful then something is wrong */
	g_assert (success);

	if (tab_iter != NULL)
	{
		g_assert (gtk_tree_store_iter_is_valid (GTK_TREE_STORE (panel->priv->model),
							tab_iter));
	}

	return success;
}

static void
select_iter (GeditDocumentsPanel *panel,
	     GtkTreeIter         *iter)
{
	GtkTreeView *treeview = GTK_TREE_VIEW (panel->priv->treeview);
	GtkTreeSelection *selection;
	GtkTreePath *path;

	selection = gtk_tree_view_get_selection (treeview);

	gtk_tree_selection_select_iter (selection, iter);

	path = gtk_tree_model_get_path (panel->priv->model, iter);
	gtk_tree_view_scroll_to_cell (treeview,
				      path, NULL,
				      FALSE,
				      0, 0);
	gtk_tree_path_free (path);
}

static void
select_active_tab (GeditDocumentsPanel *panel)
{
	GeditNotebook *notebook;
	GeditTab *tab;
	gboolean have_tabs;

	notebook = gedit_multi_notebook_get_active_notebook (panel->priv->mnb);
	have_tabs = gtk_notebook_get_n_pages (GTK_NOTEBOOK (notebook)) > 0;
	tab = gedit_multi_notebook_get_active_tab (panel->priv->mnb);

	if (notebook != NULL && tab != NULL && have_tabs)
	{
		GtkTreeIter iter;

		get_iter_from_tab (panel, notebook, tab, &iter);
		select_iter (panel, &iter);
	}
}

static void
multi_notebook_tab_switched (GeditMultiNotebook  *mnb,
			     GeditNotebook       *old_notebook,
			     GeditTab            *old_tab,
			     GeditNotebook       *new_notebook,
			     GeditTab            *new_tab,
			     GeditDocumentsPanel *panel)
{
	gedit_debug (DEBUG_PANEL);

	if (!panel->priv->setting_active_notebook &&
	    !_gedit_window_is_removing_tabs (panel->priv->window))
	{
		GtkTreeIter iter;

		get_iter_from_tab (panel, new_notebook, new_tab, &iter);

		if (gtk_tree_store_iter_is_valid (GTK_TREE_STORE (panel->priv->model),
						  &iter))
		{
			select_iter (panel, &iter);
		}
	}
}

static void
refresh_notebook (GeditDocumentsPanel *panel,
		  GeditNotebook       *notebook,
		  GtkTreeIter         *parent)
{
	GList *tabs;
	GList *l;
	GtkTreeStore *tree_store;
	GeditTab *active_tab;

	gedit_debug (DEBUG_PANEL);

	tree_store = GTK_TREE_STORE (panel->priv->model);

	active_tab = gedit_window_get_active_tab (panel->priv->window);

	tabs = gtk_container_get_children (GTK_CONTAINER (notebook));

	for (l = tabs; l != NULL; l = g_list_next (l))
	{
		GdkPixbuf *pixbuf;
		gchar *name;
		GtkTreeIter iter;

		name = tab_get_name (GEDIT_TAB (l->data));
		pixbuf = _gedit_tab_get_icon (GEDIT_TAB (l->data));

		/* Add a new row to the model */
		gtk_tree_store_append (tree_store, &iter, parent);
		gtk_tree_store_set (tree_store,
				    &iter,
				    PIXBUF_COLUMN, pixbuf,
				    NAME_COLUMN, name,
				    NOTEBOOK_COLUMN, notebook,
				    TAB_COLUMN, l->data,
				    -1);

		g_free (name);
		if (pixbuf != NULL)
		{
			g_object_unref (pixbuf);
		}

		if (l->data == active_tab)
		{
			select_iter (panel, &iter);
		}
	}

	g_list_free (tabs);
}

static void
refresh_notebook_foreach (GeditNotebook       *notebook,
			  GeditDocumentsPanel *panel)
{
	gboolean add_notebook;

	/* If we have only one notebook we don't want to show the notebook
	   header */
	add_notebook = (gedit_multi_notebook_get_n_notebooks (panel->priv->mnb) > 1);

	if (add_notebook)
	{
		GtkTreeIter iter;
		gchar *name;

		name = notebook_get_name (panel->priv->mnb, notebook);

		gtk_tree_store_append (GTK_TREE_STORE (panel->priv->model),
				       &iter, NULL);

		gtk_tree_store_set (GTK_TREE_STORE (panel->priv->model),
				    &iter,
				    PIXBUF_COLUMN, NULL,
				    NAME_COLUMN, name,
				    NOTEBOOK_COLUMN, notebook,
				    TAB_COLUMN, NULL,
				    -1);

		refresh_notebook (panel, notebook, &iter);

		g_free (name);
	}
	else
	{
		refresh_notebook (panel, notebook, NULL);
	}
}

static void
refresh_list (GeditDocumentsPanel *panel)
{
	gedit_debug (DEBUG_PANEL);

	gtk_tree_store_clear (GTK_TREE_STORE (panel->priv->model));

	panel->priv->adding_tab = TRUE;
	gedit_multi_notebook_foreach_notebook (panel->priv->mnb,
					       (GtkCallback)refresh_notebook_foreach,
					       panel);
	panel->priv->adding_tab = FALSE;

	gtk_tree_view_expand_all (GTK_TREE_VIEW (panel->priv->treeview));

	select_active_tab (panel);
}

static void
document_changed (GtkTextBuffer       *buffer,
		  GeditDocumentsPanel *panel)
{
	gedit_debug (DEBUG_PANEL);

	select_active_tab (panel);

	g_signal_handlers_disconnect_by_func (buffer,
					      G_CALLBACK (document_changed),
					      panel);
}

static void
sync_name_and_icon (GeditTab            *tab,
		    GParamSpec          *pspec,
		    GeditDocumentsPanel *panel)
{
	GdkPixbuf *pixbuf;
	gchar *name;
	GtkTreeIter iter;

	gedit_debug (DEBUG_PANEL);

	get_iter_from_tab (panel,
			   gedit_multi_notebook_get_active_notebook (panel->priv->mnb),
			   tab, &iter);

	name = tab_get_name (tab);
	pixbuf = _gedit_tab_get_icon (tab);

	gtk_tree_store_set (GTK_TREE_STORE (panel->priv->model),
			    &iter,
			    PIXBUF_COLUMN, pixbuf,
			    NAME_COLUMN, name,
			    -1);

	g_free (name);
	if (pixbuf != NULL)
	{
		g_object_unref (pixbuf);
	}
}

static void
multi_notebook_tab_removed (GeditMultiNotebook  *mnb,
			    GeditNotebook       *notebook,
			    GeditTab            *tab,
			    GeditDocumentsPanel *panel)
{
	gedit_debug (DEBUG_PANEL);

	g_signal_handlers_disconnect_by_func (gedit_tab_get_document (tab),
					      G_CALLBACK (document_changed),
					      panel);

	g_signal_handlers_disconnect_by_func (tab,
					      G_CALLBACK (sync_name_and_icon),
					      panel);

	refresh_list (panel);
}

static void
multi_notebook_tab_added (GeditMultiNotebook  *mnb,
			  GeditNotebook       *notebook,
			  GeditTab            *tab,
			  GeditDocumentsPanel *panel)
{
	gedit_debug (DEBUG_PANEL);

	g_signal_connect (tab,
			 "notify::name",
			  G_CALLBACK (sync_name_and_icon),
			  panel);
	g_signal_connect (tab,
			 "notify::state",
			  G_CALLBACK (sync_name_and_icon),
			  panel);

	refresh_list (panel);
}

static void
multi_notebook_notebook_removed (GeditMultiNotebook  *mnb,
				 GeditNotebook       *notebook,
				 GeditDocumentsPanel *panel)
{
	refresh_list (panel);
}

static void
multi_notebook_tabs_reordered (GeditWindow         *window,
			       GeditDocumentsPanel *panel)
{
	gedit_debug (DEBUG_PANEL);

	if (panel->priv->is_reodering)
		return;

	refresh_list (panel);
}

static void
set_window (GeditDocumentsPanel *panel,
	    GeditWindow         *window)
{
	gedit_debug (DEBUG_PANEL);

	g_return_if_fail (panel->priv->window == NULL);
	g_return_if_fail (GEDIT_IS_WINDOW (window));

	panel->priv->window = g_object_ref (window);
	panel->priv->mnb = GEDIT_MULTI_NOTEBOOK (_gedit_window_get_multi_notebook (window));

	g_signal_connect (panel->priv->mnb,
			  "notebook-removed",
			  G_CALLBACK (multi_notebook_notebook_removed),
			  panel);
	g_signal_connect (panel->priv->mnb,
			  "tab-added",
			  G_CALLBACK (multi_notebook_tab_added),
			  panel);
	g_signal_connect (panel->priv->mnb,
			  "tab-removed",
			  G_CALLBACK (multi_notebook_tab_removed),
			  panel);
	g_signal_connect (panel->priv->mnb,
			  "tabs-reordered",
			  G_CALLBACK (multi_notebook_tabs_reordered),
			  panel);
	g_signal_connect (panel->priv->mnb,
			  "switch-tab",
			  G_CALLBACK (multi_notebook_tab_switched),
			  panel);

	refresh_list (panel);
}

static void
treeview_cursor_changed (GtkTreeView         *view,
			 GeditDocumentsPanel *panel)
{
	GtkTreeIter iter;
	GtkTreeSelection *selection;

	gedit_debug (DEBUG_PANEL);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (panel->priv->treeview));

	if (gtk_tree_selection_get_selected (selection, NULL, &iter))
	{
		GeditNotebook *notebook;
		GeditTab *tab;

		gtk_tree_model_get (panel->priv->model,
				    &iter,
				    NOTEBOOK_COLUMN, &notebook,
				    TAB_COLUMN, &tab,
				    -1);

		if (tab != NULL)
		{
			gedit_multi_notebook_set_active_tab (panel->priv->mnb,
							     tab);
			g_object_unref (tab);
		}
		else
		{
			panel->priv->setting_active_notebook = TRUE;
			gtk_widget_grab_focus (GTK_WIDGET (notebook));
			panel->priv->setting_active_notebook = FALSE;

			tab = gedit_multi_notebook_get_active_tab (panel->priv->mnb);
			if (tab != NULL)
			{
				g_signal_connect (gedit_tab_get_document (tab),
						  "changed",
						  G_CALLBACK (document_changed),
						  panel);
			}
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
			g_value_set_object (value, panel->priv->window);
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

	/* TODO disconnect signal with window */

	gedit_debug (DEBUG_PANEL);

	G_OBJECT_CLASS (gedit_documents_panel_parent_class)->finalize (object);
}

static void
gedit_documents_panel_dispose (GObject *object)
{
	GeditDocumentsPanel *panel = GEDIT_DOCUMENTS_PANEL (object);

	gedit_debug (DEBUG_PANEL);

	if (panel->priv->window != NULL)
	{
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

	g_type_class_add_private (object_class, sizeof (GeditDocumentsPanelPrivate));
}

static GtkTreePath *
get_current_path (GeditDocumentsPanel *panel)
{
	gint notebook_num;
	gint page_num;
	GtkWidget *notebook;

	gedit_debug (DEBUG_PANEL);

	notebook = _gedit_window_get_notebook (panel->priv->window);

	notebook_num = gedit_multi_notebook_get_notebook_num (panel->priv->mnb,
							      GEDIT_NOTEBOOK (notebook));
	page_num = gtk_notebook_get_current_page (GTK_NOTEBOOK (notebook));

	return gtk_tree_path_new_from_indices (notebook_num, page_num, -1);
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
	GtkAllocation allocation;

	gedit_debug (DEBUG_PANEL);

	w = panel->priv->treeview;

	path = get_current_path (panel);

	gtk_tree_view_get_cell_area (GTK_TREE_VIEW (w),
				     path,
				     NULL,
				     &rect);

	wx = rect.x;
	wy = rect.y;

	gdk_window_get_origin (gtk_widget_get_window (w), x, y);

	gtk_widget_size_request (GTK_WIDGET (menu), &requisition);

	gtk_widget_get_allocation (w, &allocation);

	if (gtk_widget_get_direction (w) == GTK_TEXT_DIR_RTL)
	{
		*x += allocation.x + allocation.width - requisition.width - 10;
	}
	else
	{
		*x += allocation.x + 10;
	}

	wy = MAX (*y + 5, *y + wy + 5);
	wy = MIN (wy, *y + allocation.height - requisition.height - 5);

	*y = wy;

	*push_in = TRUE;
}

static gboolean
show_tab_popup_menu (GeditDocumentsPanel *panel,
		     GdkEventButton      *event)
{
	GtkWidget *menu;

	gedit_debug (DEBUG_PANEL);

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
				(GtkMenuPositionFunc)menu_position,
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
	gboolean ret = FALSE;

	gedit_debug (DEBUG_PANEL);

	if (event->type == GDK_BUTTON_PRESS && event->button == 3 &&
	    event->window == gtk_tree_view_get_bin_window (treeview))
	{
		GtkTreePath *path = NULL;

		/* Change the cursor position */
		if (gtk_tree_view_get_path_at_pos (treeview,
						   event->x,
						   event->y,
						   &path,
						   NULL,
						   NULL,
						   NULL))
		{
			GtkTreeIter iter;
			gchar *path_string;

			path_string = gtk_tree_path_to_string (path);

			if (gtk_tree_model_get_iter_from_string (panel->priv->model,
								 &iter,
								 path_string))
			{
				GeditTab *tab;

				gtk_tree_model_get (panel->priv->model,
						    &iter,
						    TAB_COLUMN, &tab,
						    -1);

				if (tab != NULL)
				{
					gtk_tree_view_set_cursor (treeview,
								  path,
								  NULL,
								  FALSE);

					/* A row exists at the mouse position */
					ret = show_tab_popup_menu (panel, event);

					g_object_unref (tab);
				}
			}

			g_free (path_string);
			gtk_tree_path_free (path);
		}
	}

	return ret;
}

static gchar *
notebook_get_tooltip (GeditMultiNotebook *mnb,
		      GeditNotebook      *notebook)
{
	gchar *tooltip;
	gchar *notebook_name;
	gint num_pages;


	notebook_name = notebook_get_name (mnb, notebook);
	num_pages = gtk_notebook_get_n_pages (GTK_NOTEBOOK (notebook));

	tooltip = g_markup_printf_escaped ("<b>Name:</b> %s\n\n"
					   "<b>Number of Tabs:</b> %i",
					   notebook_name,
					   num_pages);

	g_free (notebook_name);

	return tooltip;
}

static gboolean
treeview_query_tooltip (GtkWidget           *widget,
			gint                 x,
			gint                 y,
			gboolean             keyboard_tip,
			GtkTooltip          *tooltip,
			GeditDocumentsPanel *panel)
{
	GtkTreeIter iter;
	GtkTreeView *tree_view = GTK_TREE_VIEW (widget);
	GtkTreeModel *model = gtk_tree_view_get_model (tree_view);
	GtkTreePath *path = NULL;
	GeditNotebook *notebook;
	GeditTab *tab;
	gchar *tip;

	gedit_debug (DEBUG_PANEL);

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
			    NOTEBOOK_COLUMN, &notebook,
			    TAB_COLUMN, &tab,
			    -1);

	if (tab != NULL)
	{
		tip = _gedit_tab_get_tooltip (tab);
		g_object_unref (tab);
	}
	else
	{
		tip = notebook_get_tooltip (panel->priv->mnb, notebook);
	}

	gtk_tooltip_set_markup (tooltip, tip);

	g_object_unref (notebook);
	g_free (tip);
	gtk_tree_path_free (path);

	return TRUE;
}

/* TODO
static void
treeview_row_inserted (GtkTreeModel        *tree_model,
		       GtkTreePath         *path,
		       GtkTreeIter         *iter,
		       GeditDocumentsPanel *panel)
{
	GeditTab *tab;
	gint *indeces;
	gchar *path_string;
	GeditNotebook *notebook;

	gedit_debug (DEBUG_PANEL);

	if (panel->priv->adding_tab)
		return;

	panel->priv->is_reodering = TRUE;

	path_string = gtk_tree_path_to_string (path);

	gedit_debug_message (DEBUG_PANEL, "New Path: %s", path_string);

	g_message ("%s", path_string);

	gtk_tree_model_get (panel->priv->model,
			    iter,
			    NOTEBOOK_COLUMN, &notebook,
			    TAB_COLUMN, &tab,
			    -1);

	panel->priv->is_reodering = FALSE;

	g_free (path_string);
}
*/

static void
pixbuf_data_func (GtkTreeViewColumn   *column,
		  GtkCellRenderer     *cell,
		  GtkTreeModel        *model,
		  GtkTreeIter         *iter,
		  GeditDocumentsPanel *panel)
{
	GeditTab *tab;

	gtk_tree_model_get (model,
			    iter,
			    TAB_COLUMN, &tab,
			    -1);

	gtk_cell_renderer_set_visible (cell, tab != NULL);

	if (tab != NULL)
	{
		g_object_unref (tab);
	}
}

static void
gedit_documents_panel_init (GeditDocumentsPanel *panel)
{
	GtkWidget *sw;
	GtkTreeViewColumn *column;
	GtkCellRenderer *cell;
	GtkTreeSelection *selection;

	gedit_debug (DEBUG_PANEL);

	panel->priv = GEDIT_DOCUMENTS_PANEL_GET_PRIVATE (panel);

	panel->priv->adding_tab = FALSE;
	panel->priv->is_reodering = FALSE;

	/* Create the scrolled window */
	sw = gtk_scrolled_window_new (NULL, NULL);

	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw),
	                                     GTK_SHADOW_IN);
	gtk_widget_show (sw);
	gtk_box_pack_start (GTK_BOX (panel), sw, TRUE, TRUE, 0);

	/* Create the empty model */
	panel->priv->model = GTK_TREE_MODEL (gtk_tree_store_new (N_COLUMNS,
								 GDK_TYPE_PIXBUF,
								 G_TYPE_STRING,
								 G_TYPE_OBJECT,
								 G_TYPE_OBJECT));

	/* Create the treeview */
	panel->priv->treeview = gtk_tree_view_new_with_model (panel->priv->model);
	g_object_unref (G_OBJECT (panel->priv->model));
	gtk_container_add (GTK_CONTAINER (sw), panel->priv->treeview);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (panel->priv->treeview), FALSE);
	gtk_tree_view_set_reorderable (GTK_TREE_VIEW (panel->priv->treeview), FALSE); /* TODO */
	gtk_tree_view_set_show_expanders (GTK_TREE_VIEW (panel->priv->treeview), FALSE);
	gtk_tree_view_set_level_indentation (GTK_TREE_VIEW (panel->priv->treeview), 18);

	/* Disable search because each time the selection is changed, the
	   active tab is changed which focuses the view, and thus would remove
	   the search entry, rendering it useless */
	gtk_tree_view_set_enable_search (GTK_TREE_VIEW (panel->priv->treeview), FALSE);

	/* Disable focus so it doesn't steal focus each time from the view */
	gtk_widget_set_can_focus (panel->priv->treeview, FALSE);

	gtk_widget_set_has_tooltip (panel->priv->treeview, TRUE);

	gtk_widget_show (panel->priv->treeview);

	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (column, _("Documents"));

	cell = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (column, cell, FALSE);
	gtk_tree_view_column_add_attribute (column, cell, "pixbuf", PIXBUF_COLUMN);
	gtk_tree_view_column_set_cell_data_func (column, cell,
						 (GtkTreeCellDataFunc)pixbuf_data_func,
						 panel, NULL);

	cell = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, cell, TRUE);
	gtk_tree_view_column_add_attribute (column, cell, "markup", NAME_COLUMN);
	gtk_tree_view_append_column (GTK_TREE_VIEW (panel->priv->treeview),
				     column);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (panel->priv->treeview));

	gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);

	g_signal_connect (panel->priv->treeview,
			  "cursor-changed",
			  G_CALLBACK (treeview_cursor_changed),
			  panel);
	g_signal_connect (panel->priv->treeview,
			  "button-press-event",
			  G_CALLBACK (panel_button_press_event),
			  panel);
	g_signal_connect (panel->priv->treeview,
			  "query-tooltip",
			  G_CALLBACK (treeview_query_tooltip),
			  panel);

	/*
	g_signal_connect (panel->priv->model,
			  "row-inserted",
			  G_CALLBACK (treeview_row_inserted),
			  panel);*/
}

GtkWidget *
gedit_documents_panel_new (GeditWindow *window)
{
	gedit_debug (DEBUG_PANEL);

	g_return_val_if_fail (GEDIT_IS_WINDOW (window), NULL);

	return g_object_new (GEDIT_TYPE_DOCUMENTS_PANEL,
			     "window", window,
			     NULL);
}

/* ex:ts=8:noet: */
