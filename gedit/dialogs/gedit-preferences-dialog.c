/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-preferences-dialog.c
 * This file is part of gedit
 *
 * Copyright (C) 2001 Paolo Maggi 
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

#include <libgnome/libgnome.h>
#include <libgnomeui/libgnomeui.h>

#include <glade/glade-xml.h>

#include "bonobo-mdi.h"
#include "gedit-preferences-dialog.h"
#include "gedit-debug.h"
#include "gedit-prefs.h"
#include "gedit-view.h"
#include "gedit2.h"

/* To be syncronized with gedit-preferences.glade2 */
#define LOGO			6
#define	TOOLBAR_SETTINGS  	0
#define	STATUS_BAR_SETTINGS 	1
#define	FONT_COLORS_SETTINGS 	5
#define MDI_SETTINGS		2
#define SAVE_SETTINGS		3
#define TABS_SETTINGS		3
#define UNDO_SETTINGS		4

enum
{
	CATEGORY_COLUMN = 0,
	PAGE_NUM_COLUMN,
	NUM_COLUMNS
};

struct _GeditPreferencesDialogPrivate
{
	GtkWidget* categories_tree;

	GtkWidget* notebook;

	GtkTreeModel *categories_tree_model;

	/* Toolbar page */
	GtkWidget	*toolbar_show_radiobutton;
	GtkWidget	*toolbar_hide_radiobutton;
	
	GtkWidget	*toolbar_button_frame;
	GtkWidget	*toolbar_system_radiobutton;
	GtkWidget	*toolbar_icon_radiobutton;
	GtkWidget	*toolbar_icon_text_radiobutton;
	
	GtkWidget	*toolbar_tooltips_checkbutton;

	/* Statusbar page */
	GtkWidget	*statusbar_show_radiobutton;
	GtkWidget	*statusbar_hide_radiobutton;

	/*MDI page */
	GtkWidget	*mdi_mode_optionmenu;
	GtkWidget	*mdi_tab_pos_optionmenu;
	GtkWidget	*mdi_tab_pos_label;

	/* Font & Colors page */
	GtkWidget	*default_font_checkbutton;
	GtkWidget	*default_colors_checkbutton;
	GtkWidget	*fontpicker;
	GtkWidget	*text_colorpicker;
	GtkWidget	*background_colorpicker;
	GtkWidget	*colors_table;
	GtkWidget	*font_hbox;

};

typedef struct _CategoriesTreeItem	CategoriesTreeItem;

struct _CategoriesTreeItem
{
	gchar			*category;
	
	CategoriesTreeItem 	*children;

	gint			notebook_page;
};

static void gedit_preferences_dialog_class_init 	(GeditPreferencesDialogClass *klass);
static void gedit_preferences_dialog_init 		(GeditPreferencesDialog *dlg);
static void gedit_preferences_dialog_finalize 		(GObject *object);
		
static void gedit_preferences_dialog_add_buttons 	(GeditPreferencesDialog *dlg);

static GtkWidget* gedit_preferences_dialog_create_categories_tree 
							(GeditPreferencesDialog *dlg);
static GtkWidget* gedit_preferences_dialog_create_notebook 
							(GeditPreferencesDialog *dlg);
static GtkTreeModel* gedit_preferences_dialog_create_categories_tree_model ();

static void gedit_preferences_dialog_categories_tree_selection_cb (GtkTreeSelection *selection, 
							GeditPreferencesDialog *dlg);

static gboolean gedit_preferences_dialog_setup_toolbar_page (GeditPreferencesDialog *dlg, GladeXML *gui);
static void gedit_preferences_dialog_toolbar_show_radiobutton_toggled (GtkToggleButton *show_button,
							 GeditPreferencesDialog *dlg);
static gboolean gedit_preferences_dialog_setup_statusbar_page (GeditPreferencesDialog *dlg, GladeXML *gui);
static gboolean gedit_preferences_dialog_setup_mdi_page (GeditPreferencesDialog *dlg, GladeXML *gui);
static void gedit_preferences_dialog_mdi_mode_selection_done (GtkOptionMenu *option_menu,
	       						GeditPreferencesDialog *dlg);
static gboolean gedit_preferences_dialog_setup_font_and_colors_page (GeditPreferencesDialog *dlg, 
							GladeXML *gui);


static GtkDialogClass* parent_class = NULL;

static CategoriesTreeItem user_interface [] =
{
	{_("Toolbar"), NULL, TOOLBAR_SETTINGS},
	{_("Status bar"), NULL, STATUS_BAR_SETTINGS},
	{_("Font & Colors"), NULL, FONT_COLORS_SETTINGS},
	{_("MDI"), NULL, MDI_SETTINGS},
	NULL
};

static CategoriesTreeItem editor_behavior [] =
{
	{_("Save"), NULL, SAVE_SETTINGS },
	{_("Tabs"), NULL, TABS_SETTINGS},
	{_("Undo"), NULL, UNDO_SETTINGS},
	NULL
};

static CategoriesTreeItem toplevel [] =
{
	{_("Editor behavior"), editor_behavior, LOGO},
	{_("User interface"), user_interface, LOGO},
	NULL
};

GtkType
gedit_preferences_dialog_get_type (void)
{
	static GtkType dialog_type = 0;

	if (!dialog_type)
    	{
      		static const GtkTypeInfo dialog_info =
      		{
			"GeditPreferencesDialog",
			sizeof (GeditPreferencesDialog),
			sizeof (GeditPreferencesDialogClass),
			(GtkClassInitFunc) gedit_preferences_dialog_class_init,
			(GtkObjectInitFunc) gedit_preferences_dialog_init,
			/* reserved_1 */ NULL,
        		/* reserved_2 */ NULL,
        		(GtkClassInitFunc) NULL,
      		};

     		dialog_type = gtk_type_unique (GTK_TYPE_DIALOG, &dialog_info);
    	}

	return dialog_type;
}

static void
gedit_preferences_dialog_class_init (GeditPreferencesDialogClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	gedit_debug (DEBUG_PREFS, "");
	
  	parent_class = g_type_class_peek_parent (klass);

  	object_class->finalize = gedit_preferences_dialog_finalize;  	
}

static void
gedit_preferences_dialog_init (GeditPreferencesDialog *dlg)
{
	GtkWidget *hbox;
	GtkWidget *r;
       	GtkWidget *l;
	
	gedit_debug (DEBUG_PREFS, "");

	dlg->priv = g_new0 (GeditPreferencesDialogPrivate, 1);

	hbox = gtk_hbox_new (FALSE, 12);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);
	r = gedit_preferences_dialog_create_categories_tree (dlg);
	l = gedit_preferences_dialog_create_notebook (dlg);

	gtk_box_pack_start (GTK_BOX (hbox), r, FALSE, FALSE, 0);

	gtk_box_pack_start (GTK_BOX (hbox), l, TRUE, TRUE, 0);

	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), hbox,
			FALSE, FALSE, 0);

	gtk_widget_show_all (hbox);

	gedit_preferences_dialog_add_buttons (dlg);	

	gtk_window_set_modal (GTK_WINDOW (dlg), TRUE);

	gtk_window_set_title (GTK_WINDOW (dlg), _("Preferences"));
}

static void 
gedit_preferences_dialog_finalize (GObject *object)
{
	GeditPreferencesDialog* dlg;
	
	gedit_debug (DEBUG_PREFS, "");

	g_return_if_fail (object != NULL);
	
   	dlg = GEDIT_PREFERENCES_DIALOG (object);

	g_return_if_fail (GEDIT_IS_PREFERENCES_DIALOG (dlg));
	g_return_if_fail (dlg->priv != NULL);

	G_OBJECT_CLASS (parent_class)->finalize (object);

	g_free (dlg->priv);
}

static void
gedit_preferences_dialog_add_buttons (GeditPreferencesDialog *dlg)
{
	gedit_debug (DEBUG_PREFS, "");
	
	g_return_if_fail (GEDIT_IS_PREFERENCES_DIALOG (dlg));
			
	gtk_dialog_add_button (GTK_DIALOG (dlg),
                             	GTK_STOCK_CANCEL,
                             	GTK_RESPONSE_CANCEL);

	gtk_dialog_add_button (GTK_DIALOG (dlg),
                             	GTK_STOCK_OK,
                             	GTK_RESPONSE_OK);

	gtk_dialog_add_button (GTK_DIALOG (dlg),
                             	GTK_STOCK_HELP,
                             	GTK_RESPONSE_HELP);
}

static GtkTreeModel*
gedit_preferences_dialog_create_categories_tree_model ()
{
	GtkTreeStore *model;
	GtkTreeIter iter;
	CategoriesTreeItem *category = toplevel;

	gedit_debug (DEBUG_PREFS, "");

      	/* create tree store */
	model = gtk_tree_store_new (NUM_COLUMNS,
			      G_TYPE_STRING,
			      G_TYPE_INT);
  
	/* add data to the tree store */		
	while (category->category)
    	{
      		CategoriesTreeItem *sub_category = category->children;
		
		gtk_tree_store_append (model, &iter, NULL);
		
		gtk_tree_store_set (model, &iter,
			  CATEGORY_COLUMN, category->category,
			  PAGE_NUM_COLUMN, category->notebook_page,
			  -1);
		
		/* add children */
		while (sub_category->category)
		{
	  		GtkTreeIter child_iter;
	  
	  		gtk_tree_store_append (model, &child_iter, &iter);
	  
			gtk_tree_store_set (model, &child_iter,
				CATEGORY_COLUMN, sub_category->category,
				PAGE_NUM_COLUMN, sub_category->notebook_page,
			      -1);
			      
	  		sub_category++;
		}
      
		category++;
	}

	gedit_debug (DEBUG_PREFS, "Done");

	return GTK_TREE_MODEL (model);
	
}

static void
gedit_preferences_dialog_categories_tree_selection_cb (GtkTreeSelection *selection, GeditPreferencesDialog *dlg)
{
 	GtkTreeIter iter;
	GValue value = {0, };
	gint page_num;

	gedit_debug (DEBUG_PREFS, "");

	if (! gtk_tree_selection_get_selected (selection, NULL, &iter))
		return;

	gtk_tree_model_get_value (dlg->priv->categories_tree_model, &iter,
			    PAGE_NUM_COLUMN,
			    &value);

	page_num = g_value_get_int (&value);

	if (dlg->priv->notebook != NULL)
		gtk_notebook_set_current_page (GTK_NOTEBOOK (dlg->priv->notebook), page_num);
      	
	g_value_unset (&value);
}


static GtkWidget*
gedit_preferences_dialog_create_categories_tree (GeditPreferencesDialog *dlg)
{
	GtkWidget *sw;
	GtkTreeModel *model;
	GtkWidget *treeview;
	GtkCellRenderer *renderer;
	GtkTreeSelection *selection;
	GtkTreeViewColumn *column;
 	gint col_offset;
	
	gedit_debug (DEBUG_PREFS, "");

	sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw),
					   GTK_SHADOW_ETCHED_IN);
      	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
				      GTK_POLICY_AUTOMATIC,
				      GTK_POLICY_AUTOMATIC);
	
	gtk_widget_set_size_request (sw, 160, 240);
	
	model = gedit_preferences_dialog_create_categories_tree_model ();
	
	treeview = gtk_tree_view_new_with_model (model);
/*
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (treeview), TRUE);
*/
	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
	gtk_tree_selection_set_mode (selection,
				   GTK_SELECTION_SINGLE);

	 /* add column for category */
	renderer = gtk_cell_renderer_text_new ();
  	g_object_set (G_OBJECT (renderer), "xalign", 0.0, NULL);
  
	col_offset = gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (treeview),
							    -1, _("Categories"),
							    renderer, "text",
							    CATEGORY_COLUMN,
							    NULL);
	
  	column = gtk_tree_view_get_column (GTK_TREE_VIEW (treeview), col_offset - 1);
  	gtk_tree_view_column_set_clickable (GTK_TREE_VIEW_COLUMN (column), FALSE);

	g_signal_connect (selection, "changed", G_CALLBACK (gedit_preferences_dialog_categories_tree_selection_cb), dlg);

	gtk_container_add (GTK_CONTAINER (sw), treeview);

      	g_signal_connect (G_OBJECT (treeview), "realize", 
			G_CALLBACK (gtk_tree_view_expand_all), NULL);

	dlg->priv->categories_tree = treeview;
	dlg->priv->categories_tree_model = model;
	
	return sw;
}

static GtkWidget*
gedit_preferences_dialog_create_notebook (GeditPreferencesDialog *dlg)
{
	GladeXML *gui;

	gedit_debug (DEBUG_PREFS, "");

	gui = glade_xml_new (GEDIT_GLADEDIR "gedit-preferences.glade2",
			     "prefs_notebook", NULL);

	if (!gui) {
		g_warning
		    ("Could not find gedit-preferences.glade2, reinstall gedit.\n");
		return NULL;
	}
	
	dlg->priv->notebook = glade_xml_get_widget (gui, "prefs_notebook");

	if (!dlg->priv->notebook) {
		g_print
		    ("Could not find the required widgets inside gedit-preferences.glade2.\n");
		return NULL;
	}

	gedit_preferences_dialog_setup_toolbar_page (dlg, gui);
	gedit_preferences_dialog_setup_statusbar_page (dlg, gui);
	gedit_preferences_dialog_setup_mdi_page (dlg, gui);
	gedit_preferences_dialog_setup_font_and_colors_page (dlg, gui);

	gtk_notebook_set_current_page (GTK_NOTEBOOK (dlg->priv->notebook), LOGO);
	
	g_object_unref (G_OBJECT (gui));
	
	return dlg->priv->notebook;
}

GtkWidget*
gedit_preferences_dialog_new (GtkWindow *parent)
{
	GtkWidget *dlg;

	gedit_debug (DEBUG_PREFS, "");

	dlg = GTK_WIDGET (g_object_new (GEDIT_TYPE_PREFERENCES_DIALOG, NULL));

	if (parent)
		gtk_window_set_transient_for (GTK_WINDOW (dlg), parent);
	
	return dlg;
}

static gboolean 
gedit_preferences_dialog_setup_toolbar_page (GeditPreferencesDialog *dlg, GladeXML *gui)
{
	gedit_debug (DEBUG_PREFS, "");

	dlg->priv->toolbar_show_radiobutton = glade_xml_get_widget (gui, "toolbar_show_radiobutton");
	dlg->priv->toolbar_hide_radiobutton = glade_xml_get_widget (gui, "toolbar_hide_radiobutton");
	
	dlg->priv->toolbar_button_frame = glade_xml_get_widget (gui, "toolbar_button_frame");
	dlg->priv->toolbar_system_radiobutton = glade_xml_get_widget (gui, "toolbar_system_radiobutton");
	dlg->priv->toolbar_icon_radiobutton = glade_xml_get_widget (gui, "toolbar_icon_radiobutton");
	dlg->priv->toolbar_icon_text_radiobutton = glade_xml_get_widget (gui, "toolbar_icon_text_radiobutton");
	
	dlg->priv->toolbar_tooltips_checkbutton = glade_xml_get_widget (gui, "toolbar_tooltips_checkbutton");

	g_return_val_if_fail (dlg->priv->toolbar_show_radiobutton != NULL, FALSE);
	g_return_val_if_fail (dlg->priv->toolbar_hide_radiobutton != NULL, FALSE);
	
	g_return_val_if_fail (dlg->priv->toolbar_button_frame != NULL, FALSE);
	g_return_val_if_fail (dlg->priv->toolbar_system_radiobutton != NULL, FALSE);
	g_return_val_if_fail (dlg->priv->toolbar_icon_radiobutton != NULL, FALSE);
	g_return_val_if_fail (dlg->priv->toolbar_icon_text_radiobutton != NULL, FALSE);
	
	g_return_val_if_fail (dlg->priv->toolbar_tooltips_checkbutton != NULL, FALSE);

	g_signal_connect (G_OBJECT (dlg->priv->toolbar_show_radiobutton), "toggled", 
			G_CALLBACK (gedit_preferences_dialog_toolbar_show_radiobutton_toggled), dlg);


	if (settings->have_toolbar)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dlg->priv->toolbar_show_radiobutton), TRUE);
	else
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dlg->priv->toolbar_hide_radiobutton), TRUE);

	if (settings->have_tb_pix)
	
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dlg->priv->toolbar_icon_radiobutton), TRUE);
	
	else 
		if (settings->have_tb_text)
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dlg->priv->toolbar_icon_text_radiobutton), TRUE);
		else
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dlg->priv->toolbar_system_radiobutton), TRUE);
	
	if (settings->show_tooltips)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dlg->priv->toolbar_tooltips_checkbutton), TRUE);
	else
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dlg->priv->toolbar_tooltips_checkbutton), FALSE);
		
	return TRUE;
}

static void 
gedit_preferences_dialog_toolbar_show_radiobutton_toggled (GtkToggleButton *show_button,
							 GeditPreferencesDialog *dlg)
{
	gedit_debug (DEBUG_PREFS, "");

	if (gtk_toggle_button_get_active (show_button))
	{
		gtk_widget_set_sensitive (dlg->priv->toolbar_button_frame, TRUE);
		gtk_widget_set_sensitive (dlg->priv->toolbar_tooltips_checkbutton, TRUE);
	}
	else
	{
		gtk_widget_set_sensitive (dlg->priv->toolbar_button_frame, FALSE);
		gtk_widget_set_sensitive (dlg->priv->toolbar_tooltips_checkbutton, FALSE);
	}
}

static gboolean 
gedit_preferences_dialog_setup_statusbar_page (GeditPreferencesDialog *dlg, GladeXML *gui)
{
	gedit_debug (DEBUG_PREFS, "");

	dlg->priv->statusbar_show_radiobutton = glade_xml_get_widget (gui, "statusbar_show_radiobutton");
	dlg->priv->statusbar_hide_radiobutton = glade_xml_get_widget (gui, "statusbar_hide_radiobutton");

	g_return_val_if_fail (dlg->priv->statusbar_show_radiobutton, FALSE);
	g_return_val_if_fail (dlg->priv->statusbar_hide_radiobutton, FALSE);

	if (settings->show_status)
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dlg->priv->statusbar_show_radiobutton), TRUE);
	else
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dlg->priv->statusbar_hide_radiobutton), TRUE);

	return TRUE;
}

static void
gedit_preferences_dialog_mdi_mode_changed (GtkOptionMenu *option_menu,
	       						GeditPreferencesDialog *dlg)
{
	gedit_debug (DEBUG_PREFS, "");

	if (gtk_option_menu_get_history (GTK_OPTION_MENU (option_menu)) != BONOBO_MDI_NOTEBOOK)
	{
		gtk_widget_set_sensitive (dlg->priv->mdi_tab_pos_optionmenu, FALSE);
		gtk_widget_set_sensitive (dlg->priv->mdi_tab_pos_label, FALSE);
	}
	else
	{
		gtk_widget_set_sensitive (dlg->priv->mdi_tab_pos_optionmenu, TRUE);
		gtk_widget_set_sensitive (dlg->priv->mdi_tab_pos_label, TRUE);
	}
}	

static gboolean 
gedit_preferences_dialog_setup_mdi_page (GeditPreferencesDialog *dlg, GladeXML *gui)
{
	gedit_debug (DEBUG_PREFS, "");

	dlg->priv->mdi_mode_optionmenu = glade_xml_get_widget (gui, "mdi_mode_optionmenu");
	dlg->priv->mdi_tab_pos_optionmenu = glade_xml_get_widget (gui, "mdi_tab_pos_optionmenu");
	dlg->priv->mdi_tab_pos_label = glade_xml_get_widget (gui, "mdi_tab_pos_label");

	g_return_val_if_fail (dlg->priv->mdi_mode_optionmenu, FALSE);
	g_return_val_if_fail (dlg->priv->mdi_tab_pos_optionmenu, FALSE);
	g_return_val_if_fail (dlg->priv->mdi_tab_pos_label, FALSE);

	gtk_signal_connect (GTK_OBJECT (dlg->priv->mdi_mode_optionmenu), "changed",
			    GTK_SIGNAL_FUNC (gedit_preferences_dialog_mdi_mode_changed), dlg);

	gtk_option_menu_set_history (GTK_OPTION_MENU (dlg->priv->mdi_mode_optionmenu),
				     settings->mdi_mode);
	gtk_option_menu_set_history (GTK_OPTION_MENU (dlg->priv->mdi_tab_pos_optionmenu),
				     settings->tab_pos);

}

static void
gedit_preferences_dialog_default_font_colors_checkbutton_toggled (GtkToggleButton *button,
							 GeditPreferencesDialog *dlg)
{
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dlg->priv->default_font_checkbutton)))
	{
		gtk_widget_set_sensitive (dlg->priv->font_hbox, FALSE);
	}
	else
	{
		gtk_widget_set_sensitive (dlg->priv->font_hbox, TRUE);
	}

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dlg->priv->default_colors_checkbutton)))
	{
		gtk_widget_set_sensitive (dlg->priv->colors_table, FALSE);
	}
	else
	{
		gtk_widget_set_sensitive (dlg->priv->colors_table, TRUE);
	}


}

static gboolean 
gedit_preferences_dialog_setup_font_and_colors_page (GeditPreferencesDialog *dlg, GladeXML *gui)
{
	GtkStyle *style;
	GeditView* active_view = NULL;
	GdkColor *c;

	gedit_debug (DEBUG_PREFS, "");
	
	dlg->priv->default_font_checkbutton = glade_xml_get_widget (gui, "default_font_checkbutton");
	dlg->priv->default_colors_checkbutton = glade_xml_get_widget (gui, "default_colors_checkbutton");
/*
	dlg->priv->fontpicker = glade_xml_get_widget (gui, "fontpicker");
*/
	dlg->priv->text_colorpicker = glade_xml_get_widget (gui, "text_colorpicker");
	dlg->priv->background_colorpicker = glade_xml_get_widget (gui, "background_colorpicker");
	
	dlg->priv->colors_table = glade_xml_get_widget (gui, "colors_table");
	dlg->priv->font_hbox = glade_xml_get_widget (gui, "font_hbox");	

	g_return_val_if_fail (dlg->priv->default_font_checkbutton, FALSE);
	g_return_val_if_fail (dlg->priv->default_colors_checkbutton, FALSE);
/*
	g_return_val_if_fail (dlg->priv->fontpicker, FALSE);
*/
	g_return_val_if_fail (dlg->priv->text_colorpicker, FALSE);
	g_return_val_if_fail (dlg->priv->background_colorpicker, FALSE);
	g_return_val_if_fail (dlg->priv->colors_table, FALSE);
	g_return_val_if_fail (dlg->priv->font_hbox, FALSE);

	/* setup the initial states */
	active_view = GEDIT_VIEW (bonobo_mdi_get_active_view (BONOBO_MDI (gedit_mdi)));
	if (active_view)
		style = gtk_style_copy (gtk_widget_get_style (GTK_WIDGET (active_view)));
	else
		style = gtk_style_new ();

	c = &style->base[0];
	gnome_color_picker_set_i16 (GNOME_COLOR_PICKER (dlg->priv->background_colorpicker),
				    c->red, c->green, c->blue, 0);

	c = &style->text[0];
	gnome_color_picker_set_i16 (GNOME_COLOR_PICKER (dlg->priv->text_colorpicker),
				    c->red, c->green, c->blue, 0);

	gtk_style_unref (style);
/*
	if (settings->font)
		gnome_font_picker_set_font_name (GNOME_FONT_PICKER (dlg->priv->fontpicker),
						 settings->font);
*/
	g_signal_connect (G_OBJECT (dlg->priv->default_font_checkbutton), "toggled", 
			G_CALLBACK (gedit_preferences_dialog_default_font_colors_checkbutton_toggled), dlg);

	g_signal_connect (G_OBJECT (dlg->priv->default_colors_checkbutton), "toggled", 
			G_CALLBACK (gedit_preferences_dialog_default_font_colors_checkbutton_toggled), dlg);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dlg->priv->default_font_checkbutton), settings->use_default_font);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dlg->priv->default_colors_checkbutton), settings->use_default_colors);
}
	

