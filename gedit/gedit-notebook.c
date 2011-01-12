/*
 * gedit-notebook.c
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
 */

/* This file is a modified version of the epiphany file ephy-notebook.c
 * Here the relevant copyright:
 *
 *  Copyright (C) 2002 Christophe Fergeau
 *  Copyright (C) 2003 Marco Pesenti Gritti
 *  Copyright (C) 2003, 2004 Christian Persch
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib-object.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "gedit-notebook.h"
#include "gedit-tab.h"
#include "gedit-tab-label.h"
#include "gedit-window.h"
#include "gedit-enum-types.h"
#include "gedit-settings.h"

#define GEDIT_NOTEBOOK_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), GEDIT_TYPE_NOTEBOOK, GeditNotebookPrivate))

#define GEDIT_NOTEBOOK_GROUP_NAME "GeditNotebookGroup"

struct _GeditNotebookPrivate
{
	GSettings     *ui_settings;

	GList         *focused_pages;

	GeditNotebookShowTabsModeType show_tabs_mode;

	GtkCssProvider *css;

	guint          close_buttons_sensitive : 1;
};

G_DEFINE_TYPE(GeditNotebook, gedit_notebook, GTK_TYPE_NOTEBOOK)

/* Properties */
enum
{
	PROP_0,
	PROP_SHOW_TABS_MODE
};

/* Signals */
enum
{
	TAB_CLOSE_REQUEST,
	LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

static void
update_tabs_visibility (GeditNotebook *notebook,
			gboolean       before_inserting)
{
	gboolean show_tabs;

	switch (notebook->priv->show_tabs_mode)
	{
		case GEDIT_NOTEBOOK_SHOW_TABS_NEVER:
			show_tabs = FALSE;
			break;
		case GEDIT_NOTEBOOK_SHOW_TABS_AUTO:
			{
				guint num;

				num = gtk_notebook_get_n_pages (GTK_NOTEBOOK (notebook));

				if (before_inserting)
					++num;

				show_tabs = num > 1;
			}
			break;
		case GEDIT_NOTEBOOK_SHOW_TABS_ALWAYS:
		default:
			show_tabs = TRUE;
			break;
	}

	gtk_notebook_set_show_tabs (GTK_NOTEBOOK (notebook), show_tabs);
}

static void
gedit_notebook_get_property (GObject    *object,
			     guint       prop_id,
			     GValue     *value,
			     GParamSpec *pspec)
{
	GeditNotebook *notebook = GEDIT_NOTEBOOK (object);

	switch (prop_id)
	{
		case PROP_SHOW_TABS_MODE:
			g_value_set_enum (value,
					  notebook->priv->show_tabs_mode);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_notebook_set_property (GObject      *object,
			     guint         prop_id,
			     const GValue *value,
			     GParamSpec   *pspec)
{
	GeditNotebook *notebook = GEDIT_NOTEBOOK (object);

	switch (prop_id)
	{
		case PROP_SHOW_TABS_MODE:
			notebook->priv->show_tabs_mode = g_value_get_enum (value);
			update_tabs_visibility (notebook, FALSE);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_notebook_dispose (GObject *object)
{
	GeditNotebook *notebook = GEDIT_NOTEBOOK (object);

	if (notebook->priv->ui_settings != NULL)
	{
		g_object_unref (notebook->priv->ui_settings);
		notebook->priv->ui_settings = NULL;
	}

	if (notebook->priv->css != NULL)
	{
		g_object_unref (notebook->priv->css);
		notebook->priv->css = NULL;
	}

	G_OBJECT_CLASS (gedit_notebook_parent_class)->dispose (object);
}

static void
gedit_notebook_finalize (GObject *object)
{
	GeditNotebook *notebook = GEDIT_NOTEBOOK (object);

	g_list_free (notebook->priv->focused_pages);

	G_OBJECT_CLASS (gedit_notebook_parent_class)->finalize (object);
}

static void
gedit_notebook_grab_focus (GtkWidget *widget)
{
	GtkNotebook *nb = GTK_NOTEBOOK (widget);
	GtkWidget *tab;
	gint current_page;

	current_page = gtk_notebook_get_current_page (nb);
	tab = gtk_notebook_get_nth_page (nb, current_page);

	gtk_widget_grab_focus (tab);
}

/*
 * We need to override this because when we don't show the tabs, like in
 * fullscreen we need to have wrap around too
 */
static gboolean
gedit_notebook_change_current_page (GtkNotebook *notebook,
				    gint         offset)
{
	gboolean wrap_around;
	gint current;

	current = gtk_notebook_get_current_page (notebook);

	if (current != -1)
	{
		current = current + offset;

		g_object_get (gtk_widget_get_settings (GTK_WIDGET (notebook)),
			      "gtk-keynav-wrap-around", &wrap_around,
			      NULL);

		if (wrap_around)
		{
			if (current < 0)
			{
				current = gtk_notebook_get_n_pages (notebook) - 1;
			}
			else if (current >= gtk_notebook_get_n_pages (notebook))
			{
				current = 0;
			}
		}

		gtk_notebook_set_current_page (notebook, current);
	}
	else
	{
		gtk_widget_error_bell (GTK_WIDGET (notebook));
	}

	return TRUE;
}

static void
gedit_notebook_switch_page (GtkNotebook *notebook,
                            GtkWidget   *page,
                            guint        page_num)
{
	GeditNotebook *nb = GEDIT_NOTEBOOK (notebook);

	GTK_NOTEBOOK_CLASS (gedit_notebook_parent_class)->switch_page (notebook, page, page_num);

	/* Remove the old page, we dont want to grow unnecessarily
	 * the list */
	if (nb->priv->focused_pages)
	{
		nb->priv->focused_pages =
			g_list_remove (nb->priv->focused_pages, page);
	}

	nb->priv->focused_pages = g_list_append (nb->priv->focused_pages,
	                                         page);

	/* give focus to the tab */
	gtk_widget_grab_focus (page);
}

static void
on_tab_label_destroyed (GtkWidget *tab_label,
                        GeditTab  *tab)
{
	g_object_set_data (G_OBJECT (tab), "tab-label", NULL);
}

static void
close_button_clicked_cb (GeditTabLabel *tab_label, GeditNotebook *notebook)
{
	GeditTab *tab;

	tab = gedit_tab_label_get_tab (tab_label);
	g_signal_emit (notebook, signals[TAB_CLOSE_REQUEST], 0, tab);
}

static void
smart_tab_switching_on_closure (GeditNotebook *nb,
				GeditTab      *tab)
{
	gboolean jump_to;

	jump_to = GPOINTER_TO_INT (g_object_get_data
	                           (G_OBJECT (tab), "jump_to"));

	if (!jump_to || !nb->priv->focused_pages)
	{
		gtk_notebook_next_page (GTK_NOTEBOOK (nb));
	}
	else
	{
		GList *l;
		GtkWidget *child;
		int page_num;

		/* activate the last focused tab */
		l = g_list_last (nb->priv->focused_pages);
		child = GTK_WIDGET (l->data);
		page_num = gtk_notebook_page_num (GTK_NOTEBOOK (nb),
		                                  child);
		gtk_notebook_set_current_page (GTK_NOTEBOOK (nb),
		                               page_num);
	}
}

static GtkWidget *
get_tab_label (GeditTab *tab)
{
	GObject *tab_label;

	tab_label = g_object_get_data (G_OBJECT (tab), "tab-label");

	return (tab_label != NULL) ? GTK_WIDGET (tab_label) : NULL;
}

static void
gedit_notebook_page_removed (GtkNotebook *notebook,
                             GtkWidget   *page,
                             guint        page_num)
{
	GeditNotebook *nb = GEDIT_NOTEBOOK (notebook);
	gint num_pages;
	gint curr;
	GtkWidget *tab_label;

	tab_label = get_tab_label (GEDIT_TAB (page));

	if (tab_label != NULL)
	{
		g_signal_handlers_disconnect_by_func (tab_label,
		                                      G_CALLBACK (on_tab_label_destroyed),
		                                      page);
		g_signal_handlers_disconnect_by_func (tab_label,
		                                      G_CALLBACK (close_button_clicked_cb),
		                                      nb);
	}

	/* Remove the page from the focused pages list */
	nb->priv->focused_pages =  g_list_remove (nb->priv->focused_pages,
	                                          page);

	curr = gtk_notebook_get_current_page (notebook);

	if (page_num == curr)
	{
		smart_tab_switching_on_closure (nb, GEDIT_TAB (page));
	}

	num_pages = gtk_notebook_get_n_pages (GTK_NOTEBOOK (nb));

	/* If there is no tabs, calling this is pointless */
	if (num_pages > 0)
	{
		update_tabs_visibility (nb, FALSE);
	}
}

static void
gedit_notebook_page_added (GtkNotebook *notebook,
                           GtkWidget   *page,
                           guint        page_num)
{
	GeditNotebook *nb = GEDIT_NOTEBOOK (notebook);
	GtkWidget *tab_label;

	tab_label = get_tab_label (GEDIT_TAB (page));

	g_signal_connect (tab_label,
	                  "destroy",
	                  G_CALLBACK (on_tab_label_destroyed),
	                  page);

	g_signal_connect (tab_label,
	                  "close-clicked",
	                  G_CALLBACK (close_button_clicked_cb),
	                  nb);

	update_tabs_visibility (GEDIT_NOTEBOOK (notebook), FALSE);
}

static void
gedit_notebook_class_init (GeditNotebookClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkWidgetClass *gtkwidget_class = GTK_WIDGET_CLASS (klass);
	GtkNotebookClass *notebook_class = GTK_NOTEBOOK_CLASS (klass);

	object_class->dispose = gedit_notebook_dispose;
	object_class->finalize = gedit_notebook_finalize;
	object_class->get_property = gedit_notebook_get_property;
	object_class->set_property = gedit_notebook_set_property;

	gtkwidget_class->grab_focus = gedit_notebook_grab_focus;

	notebook_class->change_current_page = gedit_notebook_change_current_page;
	notebook_class->switch_page = gedit_notebook_switch_page;
	notebook_class->page_removed = gedit_notebook_page_removed;
	notebook_class->page_added = gedit_notebook_page_added;

	g_object_class_install_property (object_class, PROP_SHOW_TABS_MODE,
					 g_param_spec_enum ("show-tabs-mode",
							    "Show Tabs Mode",
							    "When tabs should be shown",
							    GEDIT_TYPE_NOTEBOOK_SHOW_TABS_MODE_TYPE,
							    GEDIT_NOTEBOOK_SHOW_TABS_ALWAYS,
							    G_PARAM_READWRITE));

	signals[TAB_CLOSE_REQUEST] =
		g_signal_new ("tab-close-request",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (GeditNotebookClass, tab_close_request),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE,
			      1,
			      GEDIT_TYPE_TAB);

	g_type_class_add_private (object_class, sizeof (GeditNotebookPrivate));
}

/**
 * gedit_notebook_new:
 *
 * Creates a new #GeditNotebook object.
 *
 * Returns: a new #GeditNotebook
 */
GtkWidget *
gedit_notebook_new (void)
{
	return GTK_WIDGET (g_object_new (GEDIT_TYPE_NOTEBOOK, NULL));
}

static void
gedit_notebook_init (GeditNotebook *notebook)
{
	notebook->priv = GEDIT_NOTEBOOK_GET_PRIVATE (notebook);

	notebook->priv->ui_settings = g_settings_new ("org.gnome.gedit.preferences.ui");

	notebook->priv->show_tabs_mode = GEDIT_NOTEBOOK_SHOW_TABS_ALWAYS;
	notebook->priv->close_buttons_sensitive = TRUE;
	
	gtk_notebook_set_scrollable (GTK_NOTEBOOK (notebook), TRUE);
	gtk_notebook_set_show_border (GTK_NOTEBOOK (notebook), FALSE);
	gtk_notebook_set_show_tabs (GTK_NOTEBOOK (notebook), TRUE);
	gtk_notebook_set_group_name (GTK_NOTEBOOK (notebook),
	                             GEDIT_NOTEBOOK_GROUP_NAME);
	gtk_container_set_border_width (GTK_CONTAINER (notebook), 0);

	g_settings_bind (notebook->priv->ui_settings,
			 GEDIT_SETTINGS_SHOW_TABS_MODE,
			 notebook,
			 "show-tabs-mode",
			 G_SETTINGS_BIND_GET | G_SETTINGS_BIND_SET);
}

static GtkWidget *
create_tab_label (GeditNotebook *nb,
		  GeditTab      *tab)
{
	GtkWidget *tab_label;

	tab_label = gedit_tab_label_new (tab);

	g_object_set_data (G_OBJECT (tab), "tab-label", tab_label);

	return tab_label;
}

/**
 * gedit_notebook_add_tab:
 * @nb: a #GeditNotebook
 * @tab: a #GeditTab
 * @position: the position where the @tab should be added
 * @jump_to: %TRUE to set the @tab as active
 *
 * Adds the specified @tab to the @nb.
 */
void
gedit_notebook_add_tab (GeditNotebook *nb,
		        GeditTab      *tab,
		        gint           position,
		        gboolean       jump_to)
{
	GtkWidget *tab_label;

	g_return_if_fail (GEDIT_IS_NOTEBOOK (nb));
	g_return_if_fail (GEDIT_IS_TAB (tab));

	tab_label = create_tab_label (nb, tab);

	gtk_notebook_insert_page (GTK_NOTEBOOK (nb), 
				  GTK_WIDGET (tab),
				  tab_label,
				  position);
	gtk_notebook_set_tab_reorderable (GTK_NOTEBOOK (nb),
	                                  GTK_WIDGET (tab),
	                                  TRUE);
	gtk_notebook_set_tab_detachable (GTK_NOTEBOOK (nb),
	                                 GTK_WIDGET (tab),
	                                 TRUE);

	/* The signal handler may have reordered the tabs */
	position = gtk_notebook_page_num (GTK_NOTEBOOK (nb),
					  GTK_WIDGET (tab));

	if (jump_to)
	{
		gtk_notebook_set_current_page (GTK_NOTEBOOK (nb), position);
		g_object_set_data (G_OBJECT (tab), 
				   "jump_to",
				   GINT_TO_POINTER (jump_to));
		
		gtk_widget_grab_focus (GTK_WIDGET (tab));
	}
}

static void
remove_tab (GeditTab      *tab,
            GeditNotebook *nb)
{
	gint position;

	position = gtk_notebook_page_num (GTK_NOTEBOOK (nb), GTK_WIDGET (tab));

	gtk_notebook_remove_page (GTK_NOTEBOOK (nb), position);
}

/**
 * gedit_notebook_move_tab:
 * @src: a #GeditNotebook
 * @dest: a #GeditNotebook
 * @tab: a #GeditTab
 * @dest_position: the position for @tab
 *
 * Moves @tab from @src to @dest.
 * If dest_position is greater than or equal to the number of tabs
 * of the destination nootebook or negative, tab will be moved to the
 * end of the tabs.
 */
void
gedit_notebook_move_tab (GeditNotebook *src,
                         GeditNotebook *dest,
                         GeditTab      *tab,
                         gint           dest_position)
{
	g_return_if_fail (GEDIT_IS_NOTEBOOK (src));
	g_return_if_fail (GEDIT_IS_NOTEBOOK (dest));
	g_return_if_fail (src != dest);
	g_return_if_fail (GEDIT_IS_TAB (tab));

	/* make sure the tab isn't destroyed while we move it */
	g_object_ref (tab);
	remove_tab (tab, src);
	gedit_notebook_add_tab (dest, tab, dest_position, TRUE);
	g_object_unref (tab);
}

/**
 * gedit_notebook_remove_tab:
 * @nb: a #GeditNotebook
 * @tab: a #GeditTab
 *
 * Removes @tab from @nb.
 */
void
gedit_notebook_remove_tab (GeditNotebook *nb,
			   GeditTab      *tab)
{
	g_return_if_fail (GEDIT_IS_NOTEBOOK (nb));
	g_return_if_fail (GEDIT_IS_TAB (tab));

	remove_tab (tab, nb);
}

/**
 * gedit_notebook_remove_all_tabs:
 * @nb: a #GeditNotebook
 *
 * Removes all #GeditTab from @nb.
 */
void
gedit_notebook_remove_all_tabs (GeditNotebook *nb)
{	
	g_return_if_fail (GEDIT_IS_NOTEBOOK (nb));
	
	g_list_free (nb->priv->focused_pages);
	nb->priv->focused_pages = NULL;

	gtk_container_foreach (GTK_CONTAINER (nb),
			       (GtkCallback)remove_tab,
			       nb);
}

static void
set_close_buttons_sensitivity (GeditTab      *tab,
                               GeditNotebook *nb)
{
	GtkWidget *tab_label;

	tab_label = get_tab_label (tab);

	gedit_tab_label_set_close_button_sensitive (GEDIT_TAB_LABEL (tab_label),
						    nb->priv->close_buttons_sensitive);
}

/**
 * gedit_notebook_set_close_buttons_sensitive:
 * @nb: a #GeditNotebook
 * @sensitive: %TRUE to make the buttons sensitive
 *
 * Sets whether the close buttons in the tabs of @nb are sensitive.
 */
void
gedit_notebook_set_close_buttons_sensitive (GeditNotebook *nb,
					    gboolean       sensitive)
{
	g_return_if_fail (GEDIT_IS_NOTEBOOK (nb));

	sensitive = (sensitive != FALSE);

	if (sensitive == nb->priv->close_buttons_sensitive)
		return;

	nb->priv->close_buttons_sensitive = sensitive;

	gtk_container_foreach (GTK_CONTAINER (nb),
			       (GtkCallback)set_close_buttons_sensitivity,
			       nb);
}

/**
 * gedit_notebook_get_close_buttons_sensitive:
 * @nb: a #GeditNotebook
 *
 * Whether the close buttons are sensitive.
 *
 * Returns: %TRUE if the close buttons are sensitive
 */
gboolean
gedit_notebook_get_close_buttons_sensitive (GeditNotebook *nb)
{
	g_return_val_if_fail (GEDIT_IS_NOTEBOOK (nb), TRUE);

	return nb->priv->close_buttons_sensitive;
}

void
gedit_notebook_collapse_border (GeditNotebook *nb,
                                gboolean       collapse)
{
	GtkStyleContext *context;
	GError *error = NULL;
	const gchar style[] =
		"* {\n"
		"	padding-left: 0;\n"
		"	padding-right: 0;\n"
		"}";

	g_return_if_fail (GEDIT_IS_NOTEBOOK (nb));

	context = gtk_widget_get_style_context (GTK_WIDGET (nb));

	if (collapse)
	{
		/* make it as small as possible */
		if (nb->priv->css == NULL)
		{
			nb->priv->css = gtk_css_provider_new ();
		}

		if (gtk_css_provider_load_from_data (nb->priv->css, style, -1, &error))
		{
			gtk_style_context_add_provider (context,
			                                GTK_STYLE_PROVIDER (nb->priv->css),
			                                GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
		}
		else
		{
			g_warning ("%s", error->message);
			g_error_free (error);
		}
	}
	else if (nb->priv->css != NULL)
	{
		gtk_style_context_remove_provider (context,
		                                   GTK_STYLE_PROVIDER (nb->priv->css));
	}
}

/* ex:set ts=8 noet: */
