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
#define SAVE_SETTINGS		7
#define TABS_SETTINGS		3
#define UNDO_SETTINGS		4
#define WRAP_MODE_SETTINGS	8
#define PRINT_SETTINGS		9
#define LINE_NUMBERS_SETTINGS	10

/*
#define DEBUG_MDI_PREFS
*/

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
	GtkWidget	*statusbar_cursor_position_checkbutton;
	GtkWidget	*statusbar_overwrite_mode_checkbutton;

#ifdef DEBUG_MDI_PREFS	
	/*MDI page */
	GtkWidget	*mdi_mode_optionmenu;
	GtkWidget	*mdi_tab_pos_optionmenu;
	GtkWidget	*mdi_tab_pos_label;
#endif
	/* Font & Colors page */
	GtkWidget	*default_font_checkbutton;
	GtkWidget	*default_colors_checkbutton;
	GtkWidget	*fontpicker;
	GtkWidget	*text_colorpicker;
	GtkWidget	*background_colorpicker;
	GtkWidget	*sel_text_colorpicker;
	GtkWidget	*selection_colorpicker;
	GtkWidget	*colors_table;
	GtkWidget	*font_hbox;

	/* Undo page */
	GtkWidget	*undo_checkbutton;
	GtkWidget	*undo_levels_spinbutton;
	GtkWidget	*undo_levels_label;

	/* Tabs page */
	GtkWidget	*tabs_width_spinbutton;

	/* Wrap mode page */
	GtkWidget	*wrap_never_radiobutton;
	GtkWidget	*wrap_word_radiobutton;
	GtkWidget	*wrap_char_radiobutton;

	/* Save page */
	GtkWidget	*backup_copy_checkbutton;
	GtkWidget	*auto_save_checkbutton;
	GtkWidget	*auto_save_spinbutton;
	GtkWidget	*utf8_radiobutton;
	GtkWidget	*locale_if_possible_radiobutton;
	GtkWidget	*locale_if_previous_radiobutton;

	/* Print/page page */
	GtkWidget	*add_header_checkbutton;
	GtkWidget	*wrap_lines_checkbutton;
	GtkWidget	*line_numbers_checkbutton;
	GtkWidget	*line_numbers_spinbutton;

	/* Line numbers page */
	GtkWidget	*display_line_numbers_checkbutton;

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

#ifdef DEBUG_MDI_PREFS
static gboolean gedit_preferences_dialog_setup_mdi_page (GeditPreferencesDialog *dlg, GladeXML *gui);
static void gedit_preferences_dialog_mdi_mode_selection_done (GtkOptionMenu *option_menu,
	       						GeditPreferencesDialog *dlg);
#endif

static gboolean gedit_preferences_dialog_setup_font_and_colors_page (GeditPreferencesDialog *dlg, 
							GladeXML *gui);
static gboolean gedit_preferences_dialog_setup_undo_page (GeditPreferencesDialog *dlg, GladeXML *gui);
static void gedit_preferences_dialog_undo_checkbutton_toggled (GtkToggleButton *button,
	       						GeditPreferencesDialog *dlg);
static gboolean gedit_preferences_dialog_setup_tabs_page (GeditPreferencesDialog *dlg, GladeXML *gui);
static gboolean gedit_preferences_dialog_setup_logo_page (GeditPreferencesDialog *dlg, GladeXML *gui);
static gboolean gedit_preferences_dialog_setup_wrap_mode_page (GeditPreferencesDialog *dlg, GladeXML *gui);
static gboolean gedit_preferences_dialog_setup_save_page (GeditPreferencesDialog *dlg, GladeXML *gui);
static void gedit_preferences_dialog_line_numbers_checkbutton_toggled (GtkToggleButton *button,
							 GeditPreferencesDialog *dlg);
static gboolean gedit_preferences_dialog_setup_page_page (GeditPreferencesDialog *dlg, GladeXML *gui);
static gboolean gedit_preferences_dialog_setup_line_numbers_page (GeditPreferencesDialog *dlg, 
							          GladeXML *gui);


static GtkDialogClass* parent_class = NULL;

static CategoriesTreeItem user_interface [] =
{
	{_("Toolbar"), NULL, TOOLBAR_SETTINGS},
	{_("Status bar"), NULL, STATUS_BAR_SETTINGS},

#ifdef DEBUG_MDI_PREFS	
	{_("MDI"), NULL, MDI_SETTINGS},
#endif

	{ NULL }
};

static CategoriesTreeItem editor_behavior [] =
{
	{_("Font & Colors"), NULL, FONT_COLORS_SETTINGS},

	{_("Tabs"), NULL, TABS_SETTINGS},
	{_("Wrap mode"), NULL, WRAP_MODE_SETTINGS},
	{_("Line numbers"), NULL , LINE_NUMBERS_SETTINGS},
	
 	{_("Save"), NULL, SAVE_SETTINGS },
	{_("Undo"), NULL, UNDO_SETTINGS},


	{ NULL }
};

static CategoriesTreeItem print [] =
{
	{_("Page"), NULL, PRINT_SETTINGS},

	{ NULL }
};

static CategoriesTreeItem toplevel [] =
{
	{_("Editor"), editor_behavior, LOGO},
	{_("Print"), print, LOGO},
	{_("User interface"), user_interface, LOGO},
	{ NULL }
};

GType
gedit_preferences_dialog_get_type (void)
{
	static GType dialog_type = 0;

	if (!dialog_type)
    	{
      		static const GTypeInfo dialog_info =
      		{
			sizeof (GeditPreferencesDialogClass),
        		NULL,		/* base_init */
        		NULL,		/* base_finalize */
        		(GClassInitFunc) gedit_preferences_dialog_class_init,
        		NULL,           /* class_finalize */
        		NULL,           /* class_data */
        		sizeof (GeditPreferencesDialog),
        		0,              /* n_preallocs */
        		(GInstanceInitFunc) gedit_preferences_dialog_init
      		};

     		dialog_type = g_type_register_static (GTK_TYPE_DIALOG,
						      "GeditPreferencesDialog",
						      &dialog_info, 
						      0);
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

	gtk_window_set_resizable (GTK_WINDOW (dlg), FALSE);
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

	/* FIXME: Should be GTK_RESPONSE_OK ? */
	gtk_dialog_set_default_response (GTK_DIALOG (dlg),
				GTK_RESPONSE_CANCEL);

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

	g_signal_connect (selection, "changed", 
			G_CALLBACK (gedit_preferences_dialog_categories_tree_selection_cb), dlg);

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
#ifdef DEBUG_MDI_PREFS
	gedit_preferences_dialog_setup_mdi_page (dlg, gui);
#endif
	gedit_preferences_dialog_setup_font_and_colors_page (dlg, gui);
	gedit_preferences_dialog_setup_undo_page (dlg, gui);
	gedit_preferences_dialog_setup_tabs_page (dlg, gui);
	gedit_preferences_dialog_setup_logo_page (dlg, gui);
	gedit_preferences_dialog_setup_wrap_mode_page (dlg, gui);
	gedit_preferences_dialog_setup_save_page (dlg, gui);
	gedit_preferences_dialog_setup_page_page (dlg, gui);
	gedit_preferences_dialog_setup_line_numbers_page (dlg, gui);

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


	if (gedit_settings->toolbar_visible)
		gtk_toggle_button_set_active (
			GTK_TOGGLE_BUTTON (dlg->priv->toolbar_show_radiobutton), TRUE);
	else
		gtk_toggle_button_set_active (
			GTK_TOGGLE_BUTTON (dlg->priv->toolbar_hide_radiobutton), TRUE);

	switch (gedit_settings->toolbar_buttons_style)
	{
		case GEDIT_TOOLBAR_SYSTEM:
			gtk_toggle_button_set_active (
				GTK_TOGGLE_BUTTON (dlg->priv->toolbar_system_radiobutton), TRUE);
			break;
		case GEDIT_TOOLBAR_ICONS:
			gtk_toggle_button_set_active (
				GTK_TOGGLE_BUTTON (dlg->priv->toolbar_icon_radiobutton), TRUE);
			break;
		case GEDIT_TOOLBAR_ICONS_AND_TEXT:
			gtk_toggle_button_set_active (
				GTK_TOGGLE_BUTTON (dlg->priv->toolbar_icon_text_radiobutton), TRUE);
			break;

		default:
			g_return_val_if_fail (FALSE, FALSE);
	}
	
	if (gedit_settings->toolbar_view_tooltips)
		gtk_toggle_button_set_active (
			GTK_TOGGLE_BUTTON (dlg->priv->toolbar_tooltips_checkbutton), TRUE);
	else
		gtk_toggle_button_set_active (
			GTK_TOGGLE_BUTTON (dlg->priv->toolbar_tooltips_checkbutton), FALSE);
		
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
	dlg->priv->statusbar_cursor_position_checkbutton = glade_xml_get_widget (gui, 
								"statusbar_cursor_position_checkbutton");
	dlg->priv->statusbar_overwrite_mode_checkbutton = glade_xml_get_widget (gui, 
								"statusbar_overwrite_mode_checkbutton");

	g_return_val_if_fail (dlg->priv->statusbar_show_radiobutton, FALSE);
	g_return_val_if_fail (dlg->priv->statusbar_hide_radiobutton, FALSE);
	g_return_val_if_fail (dlg->priv->statusbar_cursor_position_checkbutton, FALSE);
	g_return_val_if_fail (dlg->priv->statusbar_overwrite_mode_checkbutton, FALSE);
	
	if (gedit_settings->statusbar_visible)
		gtk_toggle_button_set_active (
				GTK_TOGGLE_BUTTON (dlg->priv->statusbar_show_radiobutton), TRUE);
	else
		gtk_toggle_button_set_active (
				GTK_TOGGLE_BUTTON (dlg->priv->statusbar_hide_radiobutton), TRUE);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (
				      dlg->priv->statusbar_cursor_position_checkbutton),	
				      gedit_settings->statusbar_view_cursor_position);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (
				      dlg->priv->statusbar_overwrite_mode_checkbutton),
				      gedit_settings->statusbar_view_overwrite_mode);
	return TRUE;
}

#ifdef DEBUG_MDI_PREFS

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
				     gedit_settings->mdi_mode);
	gtk_option_menu_set_history (GTK_OPTION_MENU (dlg->priv->mdi_tab_pos_optionmenu),
				     gedit_settings->mdi_tabs_position);

}

#endif

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
	GtkWidget *font_label;
	
	gedit_debug (DEBUG_PREFS, "");
	
	dlg->priv->default_font_checkbutton = glade_xml_get_widget (gui, "default_font_checkbutton");
	dlg->priv->default_colors_checkbutton = glade_xml_get_widget (gui, "default_colors_checkbutton");
	
	dlg->priv->text_colorpicker = glade_xml_get_widget (gui, "text_colorpicker");
	dlg->priv->background_colorpicker = glade_xml_get_widget (gui, "background_colorpicker");
	dlg->priv->sel_text_colorpicker = glade_xml_get_widget (gui, "sel_text_colorpicker");
	dlg->priv->selection_colorpicker = glade_xml_get_widget (gui, "selection_colorpicker");

	dlg->priv->colors_table = glade_xml_get_widget (gui, "colors_table");
	dlg->priv->font_hbox = glade_xml_get_widget (gui, "font_hbox");	

	font_label = glade_xml_get_widget (gui, "font_label");

	dlg->priv->fontpicker = gnome_font_picker_new ();
	g_return_val_if_fail (dlg->priv->fontpicker, FALSE);

	gnome_font_picker_set_mode (GNOME_FONT_PICKER (dlg->priv->fontpicker), 
			GNOME_FONT_PICKER_MODE_FONT_INFO);
	gnome_font_picker_fi_set_use_font_in_label (GNOME_FONT_PICKER (dlg->priv->fontpicker),
                                                       TRUE, 14);
	gnome_font_picker_fi_set_show_size (GNOME_FONT_PICKER (dlg->priv->fontpicker), TRUE);

	g_return_val_if_fail (dlg->priv->default_font_checkbutton, FALSE);
	g_return_val_if_fail (dlg->priv->default_colors_checkbutton, FALSE);

	g_return_val_if_fail (dlg->priv->text_colorpicker, FALSE);
	g_return_val_if_fail (dlg->priv->background_colorpicker, FALSE);
	g_return_val_if_fail (dlg->priv->sel_text_colorpicker, FALSE);
	g_return_val_if_fail (dlg->priv->selection_colorpicker, FALSE);

	g_return_val_if_fail (dlg->priv->colors_table, FALSE);
	g_return_val_if_fail (dlg->priv->font_hbox, FALSE);

	g_return_val_if_fail (font_label, FALSE);

	gtk_label_set_mnemonic_widget (GTK_LABEL (font_label), dlg->priv->fontpicker);
	
	gtk_box_pack_start (GTK_BOX (dlg->priv->font_hbox), dlg->priv->fontpicker, TRUE, TRUE, 0);
	
	/* setup the initial states */ 
	gnome_color_picker_set_i16 (GNOME_COLOR_PICKER (dlg->priv->background_colorpicker),
				    gedit_settings->background_color.red,
				    gedit_settings->background_color.green,  
				    gedit_settings->background_color.blue, 0);

	gnome_color_picker_set_i16 (GNOME_COLOR_PICKER (dlg->priv->text_colorpicker),
				    gedit_settings->text_color.red,
				    gedit_settings->text_color.green,
				    gedit_settings->text_color.blue, 0);

	gnome_color_picker_set_i16 (GNOME_COLOR_PICKER (dlg->priv->selection_colorpicker),
				    gedit_settings->selection_color.red,
				    gedit_settings->selection_color.green,
				    gedit_settings->selection_color.blue, 0);

	gnome_color_picker_set_i16 (GNOME_COLOR_PICKER (dlg->priv->sel_text_colorpicker),
				    gedit_settings->selected_text_color.red,
				    gedit_settings->selected_text_color.green,
				    gedit_settings->selected_text_color.blue, 0);

	if (gedit_settings->editor_font)
		gnome_font_picker_set_font_name (GNOME_FONT_PICKER (dlg->priv->fontpicker),
						 gedit_settings->editor_font);

	g_signal_connect (G_OBJECT (dlg->priv->default_font_checkbutton), "toggled", 
			G_CALLBACK (gedit_preferences_dialog_default_font_colors_checkbutton_toggled), 
			dlg);

	g_signal_connect (G_OBJECT (dlg->priv->default_colors_checkbutton), "toggled", 
			G_CALLBACK (gedit_preferences_dialog_default_font_colors_checkbutton_toggled), 
			dlg);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON 
			(dlg->priv->default_font_checkbutton), gedit_settings->use_default_font);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON 
			(dlg->priv->default_colors_checkbutton), gedit_settings->use_default_colors);
}

static void
gedit_preferences_dialog_undo_checkbutton_toggled (GtkToggleButton *button,
							 GeditPreferencesDialog *dlg)
{
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dlg->priv->undo_checkbutton)))
	{
		gtk_widget_set_sensitive (dlg->priv->undo_levels_spinbutton, TRUE);
		gtk_widget_grab_focus (dlg->priv->undo_levels_spinbutton);
	}
	else	
		gtk_widget_set_sensitive (dlg->priv->undo_levels_spinbutton, FALSE);
}
	
static gboolean 
gedit_preferences_dialog_setup_undo_page (GeditPreferencesDialog *dlg, GladeXML *gui)
{
	gedit_debug (DEBUG_PREFS, "");
	
	dlg->priv->undo_checkbutton = glade_xml_get_widget (gui, "undo_checkbutton");
	dlg->priv->undo_levels_spinbutton = glade_xml_get_widget (gui, "undo_levels_spinbutton");
	dlg->priv->undo_levels_label = glade_xml_get_widget (gui, "undo_levels_label");
		
	g_return_val_if_fail (dlg->priv->undo_checkbutton, FALSE);
	g_return_val_if_fail (dlg->priv->undo_levels_spinbutton, FALSE);
	g_return_val_if_fail (dlg->priv->undo_levels_label, FALSE);

	g_signal_connect (G_OBJECT (dlg->priv->undo_checkbutton), "toggled", 
			G_CALLBACK (gedit_preferences_dialog_undo_checkbutton_toggled), dlg);

	if (gedit_settings->undo_levels > 0)
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (dlg->priv->undo_levels_spinbutton),
					   (guint) gedit_settings->undo_levels);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dlg->priv->undo_checkbutton), 
			gedit_settings->undo_levels > 0);

	return TRUE;
}


static gboolean 
gedit_preferences_dialog_setup_tabs_page (GeditPreferencesDialog *dlg, GladeXML *gui)
{
	gedit_debug (DEBUG_PREFS, "");
	
	dlg->priv->tabs_width_spinbutton = glade_xml_get_widget (gui, "tabs_width_spinbutton");

	g_return_val_if_fail (dlg->priv->undo_levels_spinbutton, FALSE);

	gtk_spin_button_set_value (GTK_SPIN_BUTTON (dlg->priv->tabs_width_spinbutton),
					   (guint) gedit_settings->tab_size);

	return TRUE;
}

static gboolean 
gedit_preferences_dialog_setup_logo_page (GeditPreferencesDialog *dlg, GladeXML *gui)
{
	GtkWidget *logo;
	GValue value = { 0, };

	static const char* logo_file = GNOME_ICONDIR "/gedit-logo.png";

	gedit_debug (DEBUG_PREFS, "");

	logo = 	glade_xml_get_widget (gui, "logo_pixmap");

	g_return_val_if_fail (logo, FALSE);

	g_value_init (&value, G_TYPE_STRING);
	
	g_value_set_static_string (&value, logo_file);
	g_object_set_property (G_OBJECT (logo), "file" , &value);

	g_value_unset (&value);

	return TRUE;
}

static gboolean 
gedit_preferences_dialog_setup_wrap_mode_page (GeditPreferencesDialog *dlg, GladeXML *gui)
{
	gedit_debug (DEBUG_PREFS, "");

	dlg->priv->wrap_never_radiobutton = glade_xml_get_widget (gui, "wrap_never_radiobutton");
	dlg->priv->wrap_word_radiobutton = glade_xml_get_widget (gui, "wrap_word_radiobutton");
	dlg->priv->wrap_char_radiobutton = glade_xml_get_widget (gui, "wrap_char_radiobutton");

	g_return_val_if_fail (dlg->priv->wrap_never_radiobutton, FALSE);
	g_return_val_if_fail (dlg->priv->wrap_word_radiobutton, FALSE);
	g_return_val_if_fail (dlg->priv->wrap_char_radiobutton, FALSE);

	switch (gedit_settings->wrap_mode)
	{
		case GTK_WRAP_WORD:
			gtk_toggle_button_set_active (
				GTK_TOGGLE_BUTTON (dlg->priv->wrap_word_radiobutton), TRUE);
			break;
		case GTK_WRAP_CHAR:
			gtk_toggle_button_set_active (
				GTK_TOGGLE_BUTTON (dlg->priv->wrap_char_radiobutton), TRUE);
			break;
		default:
			gtk_toggle_button_set_active (
				GTK_TOGGLE_BUTTON (dlg->priv->wrap_never_radiobutton), TRUE);
	}

	return TRUE;
}

static gboolean 
gedit_preferences_dialog_setup_save_page (GeditPreferencesDialog *dlg, GladeXML *gui)
{
	gedit_debug (DEBUG_PREFS, "");
	
	dlg->priv->backup_copy_checkbutton = glade_xml_get_widget (gui, 
			"backup_copy_checkbutton");
	
	dlg->priv->auto_save_checkbutton = glade_xml_get_widget (gui, 
			"auto_save_checkbutton");

	dlg->priv->auto_save_spinbutton = glade_xml_get_widget (gui, 
			"auto_save_spinbutton");

	dlg->priv->utf8_radiobutton= glade_xml_get_widget (gui, 
			"utf8_radiobutton"); 
	dlg->priv->locale_if_possible_radiobutton= glade_xml_get_widget (gui, 
			"locale_if_possible_radiobutton"); 
	dlg->priv->locale_if_previous_radiobutton= glade_xml_get_widget (gui, 
			"locale_if_previous_radiobutton");

	g_return_val_if_fail (dlg->priv->backup_copy_checkbutton, FALSE);
	
	g_return_val_if_fail (dlg->priv->utf8_radiobutton, FALSE);
	g_return_val_if_fail (dlg->priv->locale_if_possible_radiobutton, FALSE);
	g_return_val_if_fail (dlg->priv->locale_if_previous_radiobutton, FALSE);

	/* FIXME */
	gtk_widget_set_sensitive (dlg->priv->locale_if_previous_radiobutton, FALSE);
	
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dlg->priv->backup_copy_checkbutton),
				      gedit_settings->create_backup_copy);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dlg->priv->auto_save_checkbutton),
				      gedit_settings->auto_save);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (dlg->priv->auto_save_spinbutton),
				    gedit_settings->auto_save_interval);


	switch (gedit_settings->save_encoding)
	{
		case GEDIT_SAVE_ALWAYS_UTF8:
			gtk_toggle_button_set_active (
				GTK_TOGGLE_BUTTON (dlg->priv->utf8_radiobutton), TRUE);
			break;
		case GEDIT_SAVE_CURRENT_LOCALE_WHEN_POSSIBLE:
			gtk_toggle_button_set_active (
				GTK_TOGGLE_BUTTON (dlg->priv->locale_if_possible_radiobutton),
				TRUE);
			break;
		case GEDIT_SAVE_CURRENT_LOCALE_IF_USED:
			gtk_toggle_button_set_active (
				GTK_TOGGLE_BUTTON (dlg->priv->locale_if_previous_radiobutton),
				TRUE);
			break;
		default:
			/* Not possible */
			g_return_val_if_fail (FALSE, FALSE);
	}
	
	return TRUE;
}

static void
gedit_preferences_dialog_line_numbers_checkbutton_toggled (GtkToggleButton *button,
							 GeditPreferencesDialog *dlg)
{
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dlg->priv->line_numbers_checkbutton)))
	{
		gtk_widget_set_sensitive (dlg->priv->line_numbers_spinbutton, TRUE);
		gtk_widget_grab_focus (dlg->priv->line_numbers_spinbutton);
	}
	else	
		gtk_widget_set_sensitive (dlg->priv->line_numbers_spinbutton, FALSE);
}

static gboolean 
gedit_preferences_dialog_setup_page_page (GeditPreferencesDialog *dlg, GladeXML *gui)
{
	gedit_debug (DEBUG_PREFS, "");
	
	dlg->priv->add_header_checkbutton = glade_xml_get_widget (gui, 
				"add_header_checkbutton");
	dlg->priv->wrap_lines_checkbutton= glade_xml_get_widget (gui, 
				"wrap_lines_checkbutton");
	dlg->priv->line_numbers_checkbutton = glade_xml_get_widget (gui, 
				"line_numbers_checkbutton");
	dlg->priv->line_numbers_spinbutton = glade_xml_get_widget (gui, 
				"line_numbers_spinbutton");

	g_return_val_if_fail (dlg->priv->add_header_checkbutton, FALSE);
	g_return_val_if_fail (dlg->priv->wrap_lines_checkbutton, FALSE);
	g_return_val_if_fail (dlg->priv->line_numbers_checkbutton, FALSE);
	g_return_val_if_fail (dlg->priv->line_numbers_spinbutton, FALSE);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dlg->priv->add_header_checkbutton),
				gedit_settings->print_header);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dlg->priv->wrap_lines_checkbutton),
				gedit_settings->print_wrap_lines);

	g_signal_connect (G_OBJECT (dlg->priv->line_numbers_checkbutton), "toggled", 
			G_CALLBACK (gedit_preferences_dialog_line_numbers_checkbutton_toggled), dlg);

	if (gedit_settings->print_line_numbers > 0)
	{
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (dlg->priv->line_numbers_spinbutton),
					   (guint) gedit_settings->print_line_numbers);
		gtk_widget_set_sensitive (dlg->priv->line_numbers_spinbutton, TRUE);
	}
	else
	{
		gtk_widget_set_sensitive (dlg->priv->line_numbers_spinbutton, FALSE);
	}
	
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dlg->priv->line_numbers_checkbutton),
				gedit_settings->print_line_numbers > 0);
}

static gboolean 
gedit_preferences_dialog_setup_line_numbers_page (GeditPreferencesDialog *dlg, GladeXML *gui)
{
	gedit_debug (DEBUG_PREFS, "");
	
	dlg->priv->display_line_numbers_checkbutton = glade_xml_get_widget (gui, 
				"display_line_numbers_checkbutton");

	g_return_val_if_fail (dlg->priv->display_line_numbers_checkbutton != NULL, FALSE);
	
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dlg->priv->display_line_numbers_checkbutton),
				gedit_settings->show_line_numbers);
}

gboolean 
gedit_preferences_dialog_update_settings (GeditPreferencesDialog *dlg)
{
	GeditPreferences old_prefs;
	const gchar* font;
	guint16 dummy;

	gedit_debug (DEBUG_PREFS, "");
	
	old_prefs = *gedit_settings;
	
	/* Get data from toolbar page */
	if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dlg->priv->toolbar_show_radiobutton)))
		gedit_settings->toolbar_visible = FALSE;
	else
	{
		gedit_settings->toolbar_visible = TRUE;
		
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dlg->priv->toolbar_system_radiobutton)))
			gedit_settings->toolbar_buttons_style = GEDIT_TOOLBAR_SYSTEM;
		else
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dlg->priv->toolbar_icon_radiobutton)))
			gedit_settings->toolbar_buttons_style = GEDIT_TOOLBAR_ICONS;
		else
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dlg->priv->toolbar_icon_text_radiobutton)))
			gedit_settings->toolbar_buttons_style = GEDIT_TOOLBAR_ICONS_AND_TEXT;
		else
			goto error;

		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dlg->priv->toolbar_tooltips_checkbutton)))
			gedit_settings->toolbar_view_tooltips = TRUE;
		else
			gedit_settings->toolbar_view_tooltips = FALSE;
	}

	/* Get data from status bar page */
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dlg->priv->statusbar_show_radiobutton)))
		gedit_settings->statusbar_visible = TRUE;
	else
		gedit_settings->statusbar_visible = FALSE;

	gedit_settings->statusbar_view_cursor_position = 
		gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (
					dlg->priv->statusbar_cursor_position_checkbutton));

	gedit_settings->statusbar_view_overwrite_mode = 
		gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (
					dlg->priv->statusbar_overwrite_mode_checkbutton));
		
	/* Get data from undo page */
	if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dlg->priv->undo_checkbutton)))
		gedit_settings->undo_levels = 0;
	else
	{
		gint undo_levels;

		undo_levels = gtk_spin_button_get_value_as_int (
				GTK_SPIN_BUTTON (dlg->priv->undo_levels_spinbutton));
	
		if (undo_levels < 1)
			undo_levels = 1;

		gedit_settings->undo_levels = undo_levels;
	}
	
#ifdef DEBUG_MDI_PREFS
     	/* BONOBO_MDI has to be fixed before MDI mode can be used */
	/* Get Data from MDI page */
	index = gtk_option_menu_get_history (GTK_OPTION_MENU (dlg->priv->mdi_mode_optionmenu));
	if (index > 2) index = BONOBO_MDI_NOTEBOOK;
	gedit_settings->mdi_mode = index;
	
	if (index == BONOBO_MDI_NOTEBOOK)
	{
		index = gtk_option_menu_get_history (GTK_OPTION_MENU (dlg->priv->mdi_tab_pos_optionmenu));
		if (index > 3) index = 2; /* 2 = Top */
		gedit_settings->mdi_tabs_position = index;
	}
#endif		

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dlg->priv->default_colors_checkbutton)))
		gedit_settings->use_default_colors = TRUE;
	else
		gedit_settings->use_default_colors = FALSE;

	gnome_color_picker_get_i16 (GNOME_COLOR_PICKER (dlg->priv->text_colorpicker), 
				&gedit_settings->text_color.red,
				&gedit_settings->text_color.green,
		       		&gedit_settings->text_color.blue, 	       
				&dummy);

	gnome_color_picker_get_i16 (GNOME_COLOR_PICKER (dlg->priv->background_colorpicker),
		       		&gedit_settings->background_color.red,
				&gedit_settings->background_color.green,
		       		&gedit_settings->background_color.blue, 	       
				&dummy);

	gnome_color_picker_get_i16 (GNOME_COLOR_PICKER (dlg->priv->sel_text_colorpicker), 
				&gedit_settings->selected_text_color.red,
				&gedit_settings->selected_text_color.green,
		       		&gedit_settings->selected_text_color.blue, 	       
				&dummy);

	gnome_color_picker_get_i16 (GNOME_COLOR_PICKER (dlg->priv->selection_colorpicker),
		       		&gedit_settings->selection_color.red,
				&gedit_settings->selection_color.green,
		       		&gedit_settings->selection_color.blue, 	       
				&dummy);


	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dlg->priv->default_font_checkbutton)))
		gedit_settings->use_default_font = TRUE;
	else
		gedit_settings->use_default_font = FALSE;
		
	font = gnome_font_picker_get_font_name (GNOME_FONT_PICKER (dlg->priv->fontpicker));		
	if (font != NULL)
	{
		g_free (gedit_settings->editor_font);
		gedit_settings->editor_font = g_strdup (font);
	}

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dlg->priv->wrap_word_radiobutton)))
		gedit_settings->wrap_mode = GTK_WRAP_WORD;
	else
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dlg->priv->wrap_char_radiobutton)))
		gedit_settings->wrap_mode = GTK_WRAP_CHAR;
	else
		gedit_settings->wrap_mode = GTK_WRAP_NONE;
			
	/* Save page */
	gedit_settings->create_backup_copy = 
		gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dlg->priv->backup_copy_checkbutton));

	gedit_settings->auto_save = 
		gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dlg->priv->auto_save_checkbutton));
	gedit_settings->auto_save_interval =
		gtk_spin_button_get_value (GTK_SPIN_BUTTON (dlg->priv->auto_save_spinbutton));

	

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dlg->priv->utf8_radiobutton)))
		gedit_settings->save_encoding = GEDIT_SAVE_ALWAYS_UTF8;
	else
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dlg->priv->locale_if_possible_radiobutton)))
		gedit_settings->save_encoding = GEDIT_SAVE_CURRENT_LOCALE_WHEN_POSSIBLE;
	else
		gedit_settings->save_encoding = GEDIT_SAVE_CURRENT_LOCALE_IF_USED;
	
	/* Print page / page */
	gedit_settings->print_header = 
		gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dlg->priv->add_header_checkbutton));	

	gedit_settings->print_wrap_lines = 
		gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dlg->priv->wrap_lines_checkbutton));	

	if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dlg->priv->line_numbers_checkbutton)))
		gedit_settings->print_line_numbers = 0;
	else
	{
		gedit_settings->print_line_numbers = MAX (1, gtk_spin_button_get_value_as_int (
				GTK_SPIN_BUTTON (dlg->priv->line_numbers_spinbutton)));
	}

	gedit_settings->show_line_numbers = 
		gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (
					      dlg->priv->display_line_numbers_checkbutton));

	/* Tabs page */	
	gedit_settings->tab_size =
		gtk_spin_button_get_value (GTK_SPIN_BUTTON (dlg->priv->tabs_width_spinbutton));

	return TRUE;
	
error:
	*gedit_settings = old_prefs;
	g_return_val_if_fail (FALSE, FALSE);
}

	
