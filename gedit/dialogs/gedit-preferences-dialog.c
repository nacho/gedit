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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libgnome/libgnome.h>
#include <libgnomeui/libgnomeui.h>

#include <glade/glade-xml.h>

#include "bonobo-mdi.h"
#include "gedit-preferences-dialog.h"
#include "gedit-debug.h"
#include "gedit-prefs-manager.h"
#include "gedit-view.h"
#include "gedit-utils.h"
#include "gedit2.h"
#include "gedit-plugin-manager.h"

#include "gedit-encodings-dialog.h"

#include "gnome-print-font-picker.h"

/* To be syncronized with gedit-preferences.glade2 */
#define LOGO			3
#define	FONT_COLORS_SETTINGS 	2
#define SAVE_SETTINGS		4
#define TABS_SETTINGS		0
#define UNDO_SETTINGS		1
#define WRAP_MODE_SETTINGS	5
#define PRINT_SETTINGS		6
#define LINE_NUMBERS_SETTINGS	7
#define PRINT_FONTS_SETTINGS	8
#define PLUGIN_MANAGER_SETTINGS 9
#define LOAD_SETTINGS		10

enum
{
	CATEGORY_COLUMN = 0,
	PAGE_NUM_COLUMN,
	NUM_COLUMNS
};

enum
{
	COLUMN_ENCODING_NAME = 0,
	COLUMN_ENCODING_POINTER,
	COLUMN_INDEX,
	ENCODING_NUM_COLS
};

struct _GeditPreferencesDialogPrivate
{
	GtkWidget* categories_tree;

	GtkWidget* notebook;

	GtkTreeModel *categories_tree_model;

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
	GtkWidget	*original_if_possible_radiobutton;
	GtkWidget	*create_frame;
	GtkWidget	*create_utf8_radiobutton;
	GtkWidget	*create_locale_if_possible_radiobutton;

	/* Print/page page */
	GtkWidget	*add_header_checkbutton;
	GtkWidget	*wrap_lines_checkbutton;
	GtkWidget	*line_numbers_checkbutton;
	GtkWidget	*line_numbers_spinbutton;

	/* Line numbers page */
	GtkWidget	*display_line_numbers_checkbutton;

	/* Print/Fonts page */
	GtkWidget	*body_fontpicker;
	GtkWidget	*headers_fontpicker;
	GtkWidget	*numbers_fontpicker;
	GtkWidget	*restore_default_fonts_button;

	/* Plugin/Manager */
	GtkWidget	*plugin_manager;

	/* Load page */
	GtkWidget	*encodings_treeview;
	GtkWidget	*add_enc_button;
	GtkWidget	*remove_enc_button;
	GtkWidget	*up_enc_button;
	GtkWidget	*down_enc_button;
	
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
static void gedit_preferences_dialog_response_handler 	(GtkDialog *dialog, gint res_id,  
							 GeditPreferencesDialog *dlg);
		
static void gedit_preferences_dialog_add_buttons 	(GeditPreferencesDialog *dlg);

static GtkWidget* gedit_preferences_dialog_create_categories_tree 
							(GeditPreferencesDialog *dlg);
static GtkWidget* gedit_preferences_dialog_create_notebook 
							(GeditPreferencesDialog *dlg);
static GtkTreeModel* gedit_preferences_dialog_create_categories_tree_model ();

static void gedit_preferences_dialog_categories_tree_selection_cb (GtkTreeSelection *selection, 
							GeditPreferencesDialog *dlg);
static gboolean gedit_preferences_dialog_setup_font_and_colors_page (GeditPreferencesDialog *dlg, 
							GladeXML *gui);
static gboolean gedit_preferences_dialog_setup_undo_page (GeditPreferencesDialog *dlg, GladeXML *gui);
static void gedit_preferences_dialog_undo_checkbutton_toggled (GtkToggleButton *button,
	       						GeditPreferencesDialog *dlg);
static gboolean gedit_preferences_dialog_setup_tabs_page (GeditPreferencesDialog *dlg, GladeXML *gui);
static gboolean gedit_preferences_dialog_setup_logo_page (GeditPreferencesDialog *dlg, GladeXML *gui);
static gboolean gedit_preferences_dialog_setup_wrap_mode_page (GeditPreferencesDialog *dlg, GladeXML *gui);
static void gedit_preferences_dialog_auto_save_checkbutton_toggled (GtkToggleButton *button,
							 GeditPreferencesDialog *dlg);
static gboolean gedit_preferences_dialog_setup_save_page (GeditPreferencesDialog *dlg, GladeXML *gui);
static void gedit_preferences_dialog_line_numbers_checkbutton_toggled (GtkToggleButton *button,
							 GeditPreferencesDialog *dlg);
static gboolean gedit_preferences_dialog_setup_page_page (GeditPreferencesDialog *dlg, GladeXML *gui);
static gboolean gedit_preferences_dialog_setup_line_numbers_page (GeditPreferencesDialog *dlg, 
							          GladeXML *gui);
static gboolean gedit_preferences_dialog_setup_print_fonts_page (GeditPreferencesDialog *dlg, 
								 GladeXML *gui);
static void gedit_preferences_dialog_print_font_restore_default_button_clicked (
								GtkButton *button, 
								GeditPreferencesDialog *dlg);
static gboolean gedit_preferences_dialog_selection_init (GtkTreeModel *model, 
							 GtkTreePath *path, 
							 GtkTreeIter *iter, 
							 GeditPreferencesDialog *dlg);
static void gedit_preferences_dialog_categories_tree_realize (GtkWidget		     *widget, 
						  	      GeditPreferencesDialog *dlg);
static void gedit_preferences_dialog_editor_font_picker_font_set (GnomeFontPicker *gfp, 
								  const gchar *font_name, 
								  GeditPreferencesDialog *dlg);
static void gedit_preferences_dialog_editor_color_picker_color_set (GnomeColorPicker *cp, 
								    guint r, 
								    guint g, 
								    guint b, 
								    guint a,  
								    GeditPreferencesDialog *dlg);
static void gedit_preferences_dialog_wrap_mode_radiobutton_toggled (GtkToggleButton *button,
								    GeditPreferencesDialog *dlg);
static void gedit_preferences_dialog_display_line_numbers_checkbutton_toggled (GtkToggleButton *button,
									       GeditPreferencesDialog *dlg);
static gboolean gedit_preferences_dialog_setup_plugin_manager_page (GeditPreferencesDialog *dlg, 
								    GladeXML *gui);
static gboolean gedit_preferences_dialog_setup_load_page (GeditPreferencesDialog *dlg, GladeXML *gui);

static gint last_selected_page_num = FONT_COLORS_SETTINGS;
static GtkDialogClass* parent_class = NULL;

static CategoriesTreeItem editor_behavior [] =
{
	{N_("Font & Colors"), NULL, FONT_COLORS_SETTINGS},

	{N_("Tabs"), NULL, TABS_SETTINGS},
	{N_("Wrap mode"), NULL, WRAP_MODE_SETTINGS},
	{N_("Line numbers"), NULL , LINE_NUMBERS_SETTINGS},
	
	{N_("Load"), NULL, LOAD_SETTINGS },
 	{N_("Save"), NULL, SAVE_SETTINGS },
	{N_("Undo"), NULL, UNDO_SETTINGS},


	{ NULL }
};

static CategoriesTreeItem print [] =
{
	{N_("Page"), NULL, PRINT_SETTINGS},

	{N_("Fonts"), NULL, PRINT_FONTS_SETTINGS},

	{ NULL }
};

static CategoriesTreeItem plugins [] =
{
	{N_("Manager"), NULL, PLUGIN_MANAGER_SETTINGS},

	{ NULL }
};


static CategoriesTreeItem toplevel [] =
{
	{N_("Editor"), editor_behavior, LOGO},
	{N_("Print"), print, LOGO},
	{N_("Plugins"), plugins, LOGO},	

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
	GtkWidget *ct;
	GtkWidget *label;
	
	gedit_debug (DEBUG_PREFS, "");

	dlg->priv = g_new0 (GeditPreferencesDialogPrivate, 1);

	gedit_preferences_dialog_add_buttons (dlg);	
	
	hbox = gtk_hbox_new (FALSE, 18);
	
	/*
	gtk_container_set_border_width (GTK_CONTAINER (dlg), 12);
	*/
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 10);
	/*
	gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (dlg)->vbox), 12);
	*/

	r = gtk_vbox_new (FALSE, 6);
	
	label = gtk_label_new_with_mnemonic (_("Cat_egories:"));
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
	g_object_set (G_OBJECT (label), "xalign", 0.0, NULL);

	ct = gedit_preferences_dialog_create_categories_tree (dlg);
		
	gtk_box_pack_start (GTK_BOX (r), label, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (r), ct, TRUE, TRUE, 0);

	l = gedit_preferences_dialog_create_notebook (dlg);

	gtk_box_pack_start (GTK_BOX (hbox), r, FALSE, FALSE, 0);

	gtk_box_pack_start (GTK_BOX (hbox), l, TRUE, TRUE, 0);

	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), hbox,
			FALSE, FALSE, 0);
	
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), dlg->priv->categories_tree);

	gtk_widget_show_all (GTK_DIALOG (dlg)->vbox);	
	
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

	g_object_unref (G_OBJECT (dlg->priv->categories_tree_model));

	G_OBJECT_CLASS (parent_class)->finalize (object);

	g_free (dlg->priv);
}

static void
gedit_preferences_dialog_response_handler (GtkDialog *dialog, gint res_id,  
		GeditPreferencesDialog *dlg)
{
	GError *error = NULL;

	gedit_debug (DEBUG_PREFS, "");

	switch (res_id) 
	{
		case GTK_RESPONSE_HELP:
			gnome_help_display ("gedit.xml", "gedit-prefs", &error);
	
			if (error != NULL)
			{
				g_warning (error->message);
				g_error_free (error);
			}

			break;
			
		default:
			gtk_widget_destroy (GTK_WIDGET(dialog));
	}
}

static void
gedit_preferences_dialog_add_buttons (GeditPreferencesDialog *dlg)
{
	gedit_debug (DEBUG_PREFS, "");
	
	g_return_if_fail (GEDIT_IS_PREFERENCES_DIALOG (dlg));
			
	gtk_dialog_add_button (GTK_DIALOG (dlg),
                             	GTK_STOCK_CLOSE,
                             	GTK_RESPONSE_CLOSE);

	gtk_dialog_add_button (GTK_DIALOG (dlg),
                             	GTK_STOCK_HELP,
                             	GTK_RESPONSE_HELP);

	gtk_dialog_set_default_response (GTK_DIALOG (dlg), GTK_RESPONSE_CLOSE);

	g_signal_connect(G_OBJECT (dlg), "response",
			 G_CALLBACK (gedit_preferences_dialog_response_handler), dlg);
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
			  CATEGORY_COLUMN, gettext (category->category),
			  PAGE_NUM_COLUMN, category->notebook_page,
			  -1);

		/* add children */
		while (sub_category->category)
		{
	  		GtkTreeIter child_iter;
	  
	  		gtk_tree_store_append (model, &child_iter, &iter);
	  
			gtk_tree_store_set (model, &child_iter,
				CATEGORY_COLUMN, gettext (sub_category->category),
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
gedit_preferences_dialog_categories_tree_selection_cb (GtkTreeSelection *selection, 
		GeditPreferencesDialog *dlg)
{
 	GtkTreeIter iter;
	GValue value = {0, };

	gedit_debug (DEBUG_PREFS, "");

	if (! gtk_tree_selection_get_selected (selection, NULL, &iter))
		return;

	gtk_tree_model_get_value (dlg->priv->categories_tree_model, &iter,
			    PAGE_NUM_COLUMN,
			    &value);

	last_selected_page_num = g_value_get_int (&value);

	if (dlg->priv->notebook != NULL)
		gtk_notebook_set_current_page (GTK_NOTEBOOK (dlg->priv->notebook), 
					       last_selected_page_num);
      	
	g_value_unset (&value);
}

static gboolean 
gedit_preferences_dialog_selection_init (
		GtkTreeModel *model, 
		GtkTreePath *path, 
		GtkTreeIter *iter, 
		GeditPreferencesDialog *dlg)
{
	GValue value = {0, };
	gint page_num;

	gedit_debug (DEBUG_PREFS, "");

	gtk_tree_model_get_value (dlg->priv->categories_tree_model, iter,
			    PAGE_NUM_COLUMN,
			    &value);

	page_num = g_value_get_int (&value);

	g_value_unset (&value);

	if (page_num == last_selected_page_num)
	{
		GtkTreeSelection *selection;

		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (dlg->priv->categories_tree));
		g_return_val_if_fail (selection != NULL, TRUE);

		gtk_tree_selection_select_iter (selection, iter);

		g_return_val_if_fail (dlg->priv->notebook != NULL, TRUE);
		gtk_notebook_set_current_page (GTK_NOTEBOOK (dlg->priv->notebook), page_num);
    	
		return TRUE;
	}

	return FALSE;
}

static void
gedit_preferences_dialog_categories_tree_realize (GtkWidget		 *widget, 
						  GeditPreferencesDialog *dlg)
{
	gedit_debug (DEBUG_PREFS, "");

	gtk_tree_view_expand_all (GTK_TREE_VIEW (widget));

	gtk_tree_model_foreach (dlg->priv->categories_tree_model, 
			(GtkTreeModelForeachFunc) gedit_preferences_dialog_selection_init,
			(gpointer)dlg);
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
	
	dlg->priv->categories_tree = treeview;
	dlg->priv->categories_tree_model = model;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
	g_return_val_if_fail (selection != NULL, NULL);

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
			  G_CALLBACK (gedit_preferences_dialog_categories_tree_selection_cb), 
			  dlg);

	gtk_container_add (GTK_CONTAINER (sw), treeview);

      	g_signal_connect (G_OBJECT (treeview), "realize", 
			  G_CALLBACK (gedit_preferences_dialog_categories_tree_realize), 
			  dlg);

	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (treeview), FALSE);
		
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
	
	gedit_preferences_dialog_setup_font_and_colors_page (dlg, gui);
	gedit_preferences_dialog_setup_undo_page (dlg, gui);
	gedit_preferences_dialog_setup_tabs_page (dlg, gui);
	gedit_preferences_dialog_setup_logo_page (dlg, gui);
	gedit_preferences_dialog_setup_wrap_mode_page (dlg, gui);
	gedit_preferences_dialog_setup_save_page (dlg, gui);
	gedit_preferences_dialog_setup_page_page (dlg, gui);
	gedit_preferences_dialog_setup_line_numbers_page (dlg, gui);
	gedit_preferences_dialog_setup_print_fonts_page (dlg, gui);
	gedit_preferences_dialog_setup_plugin_manager_page (dlg, gui);
	gedit_preferences_dialog_setup_load_page (dlg, gui);

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

	gtk_window_set_destroy_with_parent (GTK_WINDOW (dlg), TRUE);

	return dlg;
}

static void
gedit_preferences_dialog_default_font_colors_checkbutton_toggled (GtkToggleButton *button,
							 GeditPreferencesDialog *dlg)
{
	gedit_debug (DEBUG_PREFS, "");

	if (GTK_TOGGLE_BUTTON (dlg->priv->default_font_checkbutton) == button)
	{
		if (gtk_toggle_button_get_active (button))
		{
			gtk_widget_set_sensitive (dlg->priv->font_hbox, FALSE);
			gedit_prefs_manager_set_use_default_font (TRUE);
		}
		else
		{
			gtk_widget_set_sensitive (dlg->priv->font_hbox, 
						  gedit_prefs_manager_editor_font_can_set ());
			gedit_prefs_manager_set_use_default_font (FALSE);
		}

		return;
	}

	if (GTK_TOGGLE_BUTTON (dlg->priv->default_colors_checkbutton) == button)
	{
		if (gtk_toggle_button_get_active (button))
		{
			gtk_widget_set_sensitive (dlg->priv->colors_table, FALSE);
			gedit_prefs_manager_set_use_default_colors (TRUE);
		}
		else
		{
			gedit_prefs_manager_set_use_default_colors (FALSE);
			gtk_widget_set_sensitive (dlg->priv->colors_table, 
						  gedit_prefs_manager_background_color_can_set () &&
					  	  gedit_prefs_manager_text_color_can_set () &&
					  	  gedit_prefs_manager_selection_color_can_set () &&
					  	  gedit_prefs_manager_selected_text_color_can_set ());
		}

		return;
	}

	g_return_if_fail (FALSE);
}

static void
gedit_preferences_dialog_editor_font_picker_font_set (GnomeFontPicker *gfp, 
		const gchar *font_name, GeditPreferencesDialog *dlg)
{
	gedit_debug (DEBUG_PREFS, "");

	g_return_if_fail (gfp == GNOME_FONT_PICKER (dlg->priv->fontpicker));
	g_return_if_fail (font_name != NULL);

	gedit_prefs_manager_set_editor_font (font_name);
}

static void 
gedit_preferences_dialog_editor_color_picker_color_set (GnomeColorPicker *cp, 
		guint r, guint g, guint b, guint a,  GeditPreferencesDialog *dlg)
{
	GdkColor color;

	color.red = r;
	color.green = g;
	color.blue = b;

	if (cp == GNOME_COLOR_PICKER (dlg->priv->background_colorpicker))
	{
		gedit_prefs_manager_set_background_color (color);
		return;
	}

	if (cp == GNOME_COLOR_PICKER (dlg->priv->text_colorpicker))
	{
		gedit_prefs_manager_set_text_color (color);
		return;
	}

	if (cp == GNOME_COLOR_PICKER (dlg->priv->selection_colorpicker))
	{
		gedit_prefs_manager_set_selection_color (color);
		return;
	}

	if (cp == GNOME_COLOR_PICKER (dlg->priv->sel_text_colorpicker))
	{
		gedit_prefs_manager_set_selected_text_color (color);
		return;
	}

	g_return_if_fail (FALSE);	
}


static gboolean 
gedit_preferences_dialog_setup_font_and_colors_page (GeditPreferencesDialog *dlg, GladeXML *gui)
{
	GtkWidget *font_label;
	
	gboolean use_default_font;
	gboolean use_default_colors;

	GdkColor background_color;
	GdkColor text_color;
	GdkColor selection_color;
	GdkColor selected_text_color;
	
	gchar *editor_font = NULL;
	
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

	gtk_tooltips_set_tip (gtk_tooltips_new(), dlg->priv->fontpicker, 
			_("Push this button to select the font to be used by the editor"), NULL);

	gtk_tooltips_set_tip (gtk_tooltips_new(), dlg->priv->text_colorpicker, 
			_("Push this button to configure text color"), NULL);
	gtk_tooltips_set_tip (gtk_tooltips_new(), dlg->priv->background_colorpicker, 
			_("Push this button to configure background color"), NULL);
	gtk_tooltips_set_tip (gtk_tooltips_new(), dlg->priv->sel_text_colorpicker, 
			_("Push this button to configure the color in which the selected "
			  "text should appear"), NULL);
	gtk_tooltips_set_tip (gtk_tooltips_new(), dlg->priv->selection_colorpicker, 
			_("Push this button to configure the color in which the selected "
			  "text should be marked"), NULL);

	gtk_label_set_mnemonic_widget (GTK_LABEL (font_label), dlg->priv->fontpicker);
	gedit_utils_set_atk_relation (dlg->priv->fontpicker, font_label, ATK_RELATION_LABELLED_BY);
	gedit_utils_set_atk_relation (dlg->priv->fontpicker, dlg->priv->default_font_checkbutton, 
                                                          ATK_RELATION_CONTROLLED_BY);
	gedit_utils_set_atk_relation (dlg->priv->default_font_checkbutton, dlg->priv->fontpicker, 
                                                         ATK_RELATION_CONTROLLER_FOR);
	gtk_box_pack_start (GTK_BOX (dlg->priv->font_hbox), dlg->priv->fontpicker, TRUE, TRUE, 0);
	
	/* read config value */
	use_default_font = gedit_prefs_manager_get_use_default_font ();
	use_default_colors = gedit_prefs_manager_get_use_default_colors ();

	background_color = gedit_prefs_manager_get_background_color ();
	text_color = gedit_prefs_manager_get_text_color ();
	selection_color = gedit_prefs_manager_get_selection_color ();
	selected_text_color = gedit_prefs_manager_get_selected_text_color ();
	
	editor_font = gedit_prefs_manager_get_editor_font ();

	/* setup the initial states */ 
	gnome_color_picker_set_i16 (GNOME_COLOR_PICKER (dlg->priv->background_colorpicker),
				    background_color.red,
				    background_color.green,  
				    background_color.blue, 0);

	gnome_color_picker_set_i16 (GNOME_COLOR_PICKER (dlg->priv->text_colorpicker),
				    text_color.red,
				    text_color.green,
				    text_color.blue, 0);

	gnome_color_picker_set_i16 (GNOME_COLOR_PICKER (dlg->priv->selection_colorpicker),
				    selection_color.red,
				    selection_color.green,
				    selection_color.blue, 0);

	gnome_color_picker_set_i16 (GNOME_COLOR_PICKER (dlg->priv->sel_text_colorpicker),
				    selected_text_color.red,
				    selected_text_color.green,
				    selected_text_color.blue, 0);

	if (editor_font != NULL)
	{
		gnome_font_picker_set_font_name (GNOME_FONT_PICKER (dlg->priv->fontpicker),
						 editor_font);

		g_free (editor_font);
	}

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON 
			(dlg->priv->default_font_checkbutton), use_default_font);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON 
			(dlg->priv->default_colors_checkbutton), use_default_colors);

	/* Connect signals */
	
	g_signal_connect (G_OBJECT (dlg->priv->default_font_checkbutton), "toggled", 
			G_CALLBACK (gedit_preferences_dialog_default_font_colors_checkbutton_toggled), 
			dlg);

	g_signal_connect (G_OBJECT (dlg->priv->default_colors_checkbutton), "toggled", 
			G_CALLBACK (gedit_preferences_dialog_default_font_colors_checkbutton_toggled), 
			dlg);

	g_signal_connect (G_OBJECT (dlg->priv->fontpicker), "font_set", 
			G_CALLBACK (gedit_preferences_dialog_editor_font_picker_font_set), 
			dlg);

	g_signal_connect (G_OBJECT (dlg->priv->background_colorpicker), "color_set",
			G_CALLBACK (gedit_preferences_dialog_editor_color_picker_color_set),
			dlg);

	g_signal_connect (G_OBJECT (dlg->priv->text_colorpicker), "color_set",
			G_CALLBACK (gedit_preferences_dialog_editor_color_picker_color_set),
			dlg);

	g_signal_connect (G_OBJECT (dlg->priv->selection_colorpicker), "color_set",
			G_CALLBACK (gedit_preferences_dialog_editor_color_picker_color_set),
			dlg);

	g_signal_connect (G_OBJECT (dlg->priv->sel_text_colorpicker), "color_set",
			G_CALLBACK (gedit_preferences_dialog_editor_color_picker_color_set),
			dlg);

	/* Set initial widget sensitivity */
	gtk_widget_set_sensitive (dlg->priv->default_font_checkbutton, 
				  gedit_prefs_manager_use_default_font_can_set ());

	gtk_widget_set_sensitive (dlg->priv->default_colors_checkbutton, 
				  gedit_prefs_manager_use_default_colors_can_set ());

	if (use_default_font)
		gtk_widget_set_sensitive (dlg->priv->font_hbox, FALSE);
	else
		gtk_widget_set_sensitive (dlg->priv->font_hbox, 
					  gedit_prefs_manager_editor_font_can_set ());

	if (use_default_colors)
		gtk_widget_set_sensitive (dlg->priv->colors_table, FALSE);
	else
		gtk_widget_set_sensitive (dlg->priv->colors_table, 
					  gedit_prefs_manager_background_color_can_set () &&
				  	  gedit_prefs_manager_text_color_can_set () &&
				  	  gedit_prefs_manager_selection_color_can_set () &&
				  	  gedit_prefs_manager_selected_text_color_can_set ());
		
	return TRUE;
}

static void
gedit_preferences_dialog_undo_checkbutton_toggled (GtkToggleButton *button,
							 GeditPreferencesDialog *dlg)
{
	gedit_debug (DEBUG_PREFS, "");

	g_return_if_fail (button == GTK_TOGGLE_BUTTON (dlg->priv->undo_checkbutton));

	if (gtk_toggle_button_get_active (button))
	{
		gint undo_levels;
		
		gtk_widget_set_sensitive (dlg->priv->undo_levels_spinbutton, 
					  gedit_prefs_manager_undo_actions_limit_can_set());
		/*
		gtk_widget_grab_focus (dlg->priv->undo_levels_spinbutton);
		*/
		undo_levels = gtk_spin_button_get_value_as_int (
				GTK_SPIN_BUTTON (dlg->priv->undo_levels_spinbutton));
		g_return_if_fail (undo_levels >= 1);

		gedit_prefs_manager_set_undo_actions_limit (undo_levels);
	}
	else	
	{
		gtk_widget_set_sensitive (dlg->priv->undo_levels_spinbutton, FALSE);

		gedit_prefs_manager_set_undo_actions_limit (-1);
	}
}

static void
gedit_preferences_dialog_undo_levels_spinbutton_value_changed (GtkSpinButton *spin_button,
		GeditPreferencesDialog *dlg)
{
	gint undo_levels;

	gedit_debug (DEBUG_PREFS, "");

	g_return_if_fail (spin_button == GTK_SPIN_BUTTON (dlg->priv->undo_levels_spinbutton));

	undo_levels = gtk_spin_button_get_value_as_int (
				GTK_SPIN_BUTTON (dlg->priv->undo_levels_spinbutton));
	g_return_if_fail (undo_levels >= 1);

	gedit_prefs_manager_set_undo_actions_limit (undo_levels);
}
	
static gboolean 
gedit_preferences_dialog_setup_undo_page (GeditPreferencesDialog *dlg, GladeXML *gui)
{
	gint undo_levels;
	gboolean can_set;
	
	gedit_debug (DEBUG_PREFS, "");
	
	dlg->priv->undo_checkbutton = glade_xml_get_widget (gui, "undo_checkbutton");
	dlg->priv->undo_levels_spinbutton = glade_xml_get_widget (gui, "undo_levels_spinbutton");
	dlg->priv->undo_levels_label = glade_xml_get_widget (gui, "undo_levels_label");
		
	g_return_val_if_fail (dlg->priv->undo_checkbutton, FALSE);
	g_return_val_if_fail (dlg->priv->undo_levels_spinbutton, FALSE);
	g_return_val_if_fail (dlg->priv->undo_levels_label, FALSE);

	/* Set initial value */
	undo_levels = gedit_prefs_manager_get_undo_actions_limit ();
	
	if (undo_levels > 0)
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (dlg->priv->undo_levels_spinbutton),
					   (guint) undo_levels);

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dlg->priv->undo_checkbutton), 
			undo_levels > 0);

	/* Set widget sensitivity */
	can_set = gedit_prefs_manager_undo_actions_limit_can_set ();

	gtk_widget_set_sensitive (dlg->priv->undo_checkbutton, can_set);
	gtk_widget_set_sensitive (dlg->priv->undo_levels_spinbutton, can_set && (undo_levels > 0));
	gtk_widget_set_sensitive (dlg->priv->undo_levels_label, can_set);

	/* Connect signals */
	if (can_set)
	{
		/*
		if (undo_levels > 0)
			gtk_widget_grab_focus (dlg->priv->undo_levels_spinbutton);
		*/
	
		g_signal_connect (G_OBJECT (dlg->priv->undo_checkbutton), "toggled", 
			  	  G_CALLBACK (gedit_preferences_dialog_undo_checkbutton_toggled), 
				  dlg);

		g_signal_connect (G_OBJECT (dlg->priv->undo_levels_spinbutton), "value_changed",
			  	  G_CALLBACK (gedit_preferences_dialog_undo_levels_spinbutton_value_changed),
			  	  dlg);
	}
	
	return TRUE;
}

static void
gedit_preferences_dialog_tabs_width_spinbutton_value_changed (GtkSpinButton *spin_button,
		GeditPreferencesDialog *dlg)
{
	g_return_if_fail (spin_button == GTK_SPIN_BUTTON (dlg->priv->tabs_width_spinbutton));

	gedit_prefs_manager_set_tabs_size (gtk_spin_button_get_value_as_int (spin_button));
}
			
static gboolean 
gedit_preferences_dialog_setup_tabs_page (GeditPreferencesDialog *dlg, GladeXML *gui)
{
	GtkWidget *tabs_width_hbox;
	
	gedit_debug (DEBUG_PREFS, "");
	
	dlg->priv->tabs_width_spinbutton = glade_xml_get_widget (gui, "tabs_width_spinbutton");
	tabs_width_hbox = glade_xml_get_widget (gui, "tabs_width_hbox");
		
	g_return_val_if_fail (dlg->priv->undo_levels_spinbutton, FALSE);
	g_return_val_if_fail (tabs_width_hbox, FALSE);

	/* Set initial state */
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (dlg->priv->tabs_width_spinbutton),
					   (guint) gedit_prefs_manager_get_tabs_size ());

	/* Set widget sensitivity */
	gtk_widget_set_sensitive (tabs_width_hbox, 
				  gedit_prefs_manager_tabs_size_can_set ());

	/* Connect signal */
	g_signal_connect (G_OBJECT (dlg->priv->tabs_width_spinbutton), "value_changed",
			  G_CALLBACK (gedit_preferences_dialog_tabs_width_spinbutton_value_changed),
			  dlg);
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

static void
gedit_preferences_dialog_wrap_mode_radiobutton_toggled (GtkToggleButton *button,
		GeditPreferencesDialog *dlg)
{
	if (button == GTK_TOGGLE_BUTTON (dlg->priv->wrap_never_radiobutton))
	{
		if (gtk_toggle_button_get_active (button))
		{
			gedit_prefs_manager_set_wrap_mode (GTK_WRAP_NONE);
			return;
		}
	}

	if (button == GTK_TOGGLE_BUTTON (dlg->priv->wrap_char_radiobutton))
	{
		if (gtk_toggle_button_get_active (button))
		{
			gedit_prefs_manager_set_wrap_mode (GTK_WRAP_CHAR);
			return;
		}
	}

	if (button == GTK_TOGGLE_BUTTON (dlg->priv->wrap_word_radiobutton))
	{
		if (gtk_toggle_button_get_active (button))
		{
			gedit_prefs_manager_set_wrap_mode (GTK_WRAP_WORD);
			return;
		}
	}
}

static gboolean 
gedit_preferences_dialog_setup_wrap_mode_page (GeditPreferencesDialog *dlg, GladeXML *gui)
{
	GtkWidget *wrap_mode_frame;
	
	gedit_debug (DEBUG_PREFS, "");

	dlg->priv->wrap_never_radiobutton = glade_xml_get_widget (gui, "wrap_never_radiobutton");
	dlg->priv->wrap_word_radiobutton = glade_xml_get_widget (gui, "wrap_word_radiobutton");
	dlg->priv->wrap_char_radiobutton = glade_xml_get_widget (gui, "wrap_char_radiobutton");
	wrap_mode_frame = glade_xml_get_widget (gui, "wrap_mode_frame");
	
	g_return_val_if_fail (dlg->priv->wrap_never_radiobutton, FALSE);
	g_return_val_if_fail (dlg->priv->wrap_word_radiobutton, FALSE);
	g_return_val_if_fail (dlg->priv->wrap_char_radiobutton, FALSE);
	g_return_val_if_fail (wrap_mode_frame, FALSE);
	
	/* Set initial state */
	switch (gedit_prefs_manager_get_wrap_mode ())
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

	/* Set widget sensitivity */
	gtk_widget_set_sensitive (wrap_mode_frame, 
				  gedit_prefs_manager_wrap_mode_can_set ());

	/* Connect signals */
	g_signal_connect (G_OBJECT (dlg->priv->wrap_never_radiobutton), "toggled", 
			G_CALLBACK (gedit_preferences_dialog_wrap_mode_radiobutton_toggled), 
			dlg);
	g_signal_connect (G_OBJECT (dlg->priv->wrap_word_radiobutton), "toggled", 
			G_CALLBACK (gedit_preferences_dialog_wrap_mode_radiobutton_toggled), 
			dlg);
	g_signal_connect (G_OBJECT (dlg->priv->wrap_char_radiobutton), "toggled", 
			G_CALLBACK (gedit_preferences_dialog_wrap_mode_radiobutton_toggled), 
			dlg);

	
	return TRUE;
}

static void
gedit_preferences_dialog_auto_save_checkbutton_toggled (GtkToggleButton *button,
							GeditPreferencesDialog *dlg)
{
	gedit_debug (DEBUG_PREFS, "");

	g_return_if_fail (button == GTK_TOGGLE_BUTTON (dlg->priv->auto_save_checkbutton));
	
	if (gtk_toggle_button_get_active (button))
	{
		gtk_widget_set_sensitive (dlg->priv->auto_save_spinbutton, 
					  gedit_prefs_manager_auto_save_interval_can_set());
		/*
		gtk_widget_grab_focus (dlg->priv->auto_save_spinbutton);
		*/
		gedit_prefs_manager_set_auto_save (TRUE);
	}
	else	
	{
		gtk_widget_set_sensitive (dlg->priv->auto_save_spinbutton, FALSE);
		gedit_prefs_manager_set_auto_save (FALSE);
	}
}

static void
gedit_preferences_dialog_backup_copy_checkbutton_toggled (GtkToggleButton *button,
							  GeditPreferencesDialog *dlg)
{
	gedit_debug (DEBUG_PREFS, "");

	g_return_if_fail (button == GTK_TOGGLE_BUTTON (dlg->priv->backup_copy_checkbutton));
	
	gedit_prefs_manager_set_create_backup_copy (gtk_toggle_button_get_active (button));
}

static void
gedit_preferences_dialog_auto_save_spinbutton_value_changed (GtkSpinButton *spin_button,
		GeditPreferencesDialog *dlg)
{
	g_return_if_fail (spin_button == GTK_SPIN_BUTTON (dlg->priv->auto_save_spinbutton));

	gedit_prefs_manager_set_auto_save_interval (
			MAX (1, gtk_spin_button_get_value_as_int (spin_button)));
}

static void
gedit_preferences_dialog_save_radiobutton_toggled (GtkToggleButton *button,
		GeditPreferencesDialog *dlg)
{
	if (button == GTK_TOGGLE_BUTTON (dlg->priv->utf8_radiobutton))
	{
		if (gtk_toggle_button_get_active (button))
		{
			gedit_prefs_manager_set_save_encoding (
					GEDIT_SAVE_ALWAYS_UTF8);

			gtk_widget_set_sensitive (dlg->priv->create_frame, FALSE);
			
			return;
		}
	}

	if (button == GTK_TOGGLE_BUTTON (dlg->priv->locale_if_possible_radiobutton))
	{
		if (gtk_toggle_button_get_active (button))
		{
			gedit_prefs_manager_set_save_encoding (
					GEDIT_SAVE_CURRENT_LOCALE_IF_POSSIBLE);

			gtk_widget_set_sensitive (dlg->priv->create_frame, FALSE);

			return;
		}
	}

	if (button == GTK_TOGGLE_BUTTON (dlg->priv->original_if_possible_radiobutton))
	{
		if (gtk_toggle_button_get_active (button))
		{
			if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dlg->priv->create_utf8_radiobutton)))
				gedit_prefs_manager_set_save_encoding (
						GEDIT_SAVE_ORIGINAL_FILE_ENCODING_IF_POSSIBLE);
			else
			{
				g_return_if_fail (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (
								dlg->priv->create_locale_if_possible_radiobutton)));

				gedit_prefs_manager_set_save_encoding (
						GEDIT_SAVE_ORIGINAL_FILE_ENCODING_IF_POSSIBLE_NCL);

			}
			
			gtk_widget_set_sensitive (dlg->priv->create_frame, TRUE);

			return;
		}
	}

	if (button == GTK_TOGGLE_BUTTON (dlg->priv->create_utf8_radiobutton))
	{
		if (gtk_toggle_button_get_active (button))
		{
			g_return_if_fail (gtk_toggle_button_get_active (
						GTK_TOGGLE_BUTTON (dlg->priv->original_if_possible_radiobutton)));

			gedit_prefs_manager_set_save_encoding (
					GEDIT_SAVE_ORIGINAL_FILE_ENCODING_IF_POSSIBLE);

			return;
		}
	}

	if (button == GTK_TOGGLE_BUTTON (dlg->priv->create_locale_if_possible_radiobutton))
	{
		if (gtk_toggle_button_get_active (button))
		{
			g_return_if_fail (gtk_toggle_button_get_active (
						GTK_TOGGLE_BUTTON (dlg->priv->original_if_possible_radiobutton)));

			gedit_prefs_manager_set_save_encoding (
					GEDIT_SAVE_ORIGINAL_FILE_ENCODING_IF_POSSIBLE_NCL);

			return;
		}
	}
}

static gboolean 
gedit_preferences_dialog_setup_save_page (GeditPreferencesDialog *dlg, GladeXML *gui)
{
	GtkWidget *autosave_hbox;
	GtkWidget *save_frame;
	gboolean auto_save;
	GeditSaveEncodingSetting encoding;
	
	gedit_debug (DEBUG_PREFS, "");
	
	autosave_hbox = glade_xml_get_widget (gui, 
			"autosave_hbox");

	save_frame = glade_xml_get_widget (gui, 
			"save_frame");
		
	dlg->priv->backup_copy_checkbutton = glade_xml_get_widget (gui, 
			"backup_copy_checkbutton");
	
	dlg->priv->auto_save_checkbutton = glade_xml_get_widget (gui, 
			"auto_save_checkbutton");

	dlg->priv->auto_save_spinbutton = glade_xml_get_widget (gui, 
			"auto_save_spinbutton");

	dlg->priv->utf8_radiobutton = glade_xml_get_widget (gui, 
			"utf8_radiobutton"); 
	dlg->priv->locale_if_possible_radiobutton= glade_xml_get_widget (gui, 
			"locale_if_possible_radiobutton"); 
	dlg->priv->original_if_possible_radiobutton = glade_xml_get_widget (gui, 
			"original_if_possible_radiobutton");

	dlg->priv->create_frame = glade_xml_get_widget (gui, 
			"create_frame"); 
	dlg->priv->create_utf8_radiobutton = glade_xml_get_widget (gui, 
			"create_utf8_radiobutton"); 
	dlg->priv->create_locale_if_possible_radiobutton = glade_xml_get_widget (gui, 
			"create_locale_if_possible_radiobutton"); 

	g_return_val_if_fail (autosave_hbox, FALSE);
	g_return_val_if_fail (save_frame, FALSE);

	g_return_val_if_fail (dlg->priv->backup_copy_checkbutton, FALSE);
	
	g_return_val_if_fail (dlg->priv->utf8_radiobutton, FALSE);
	g_return_val_if_fail (dlg->priv->locale_if_possible_radiobutton, FALSE);
	g_return_val_if_fail (dlg->priv->original_if_possible_radiobutton, FALSE);

	g_return_val_if_fail (dlg->priv->create_frame, FALSE);
	g_return_val_if_fail (dlg->priv->create_utf8_radiobutton, FALSE);
	g_return_val_if_fail (dlg->priv->create_locale_if_possible_radiobutton, FALSE);

	/* FIXME */
	/*
	gtk_widget_set_sensitive (dlg->priv->locale_if_previous_radiobutton, FALSE);
	*/
	/* Set current values */
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dlg->priv->backup_copy_checkbutton),
				      gedit_prefs_manager_get_create_backup_copy ());

	auto_save = gedit_prefs_manager_get_auto_save ();
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dlg->priv->auto_save_checkbutton),
				      auto_save );
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (dlg->priv->auto_save_spinbutton),
				   gedit_prefs_manager_get_auto_save_interval ());
	
	encoding = gedit_prefs_manager_get_save_encoding ();

	switch (encoding)
	{
		case GEDIT_SAVE_ALWAYS_UTF8:
			gtk_toggle_button_set_active (
				GTK_TOGGLE_BUTTON (dlg->priv->utf8_radiobutton), TRUE);
			break;
		case GEDIT_SAVE_CURRENT_LOCALE_IF_POSSIBLE:
			gtk_toggle_button_set_active (
				GTK_TOGGLE_BUTTON (dlg->priv->locale_if_possible_radiobutton),
				TRUE);
			break;
		case GEDIT_SAVE_ORIGINAL_FILE_ENCODING_IF_POSSIBLE:
			gtk_toggle_button_set_active (
				GTK_TOGGLE_BUTTON (dlg->priv->original_if_possible_radiobutton),
				TRUE);
			gtk_toggle_button_set_active (
				GTK_TOGGLE_BUTTON (dlg->priv->create_utf8_radiobutton),
				TRUE);
			break;
		case GEDIT_SAVE_ORIGINAL_FILE_ENCODING_IF_POSSIBLE_NCL:
			gtk_toggle_button_set_active (
				GTK_TOGGLE_BUTTON (dlg->priv->original_if_possible_radiobutton),
				TRUE);
			gtk_toggle_button_set_active (
				GTK_TOGGLE_BUTTON (dlg->priv->create_locale_if_possible_radiobutton),
				TRUE);
			break;
			
		default:
			/* Not possible */
			g_return_val_if_fail (FALSE, FALSE);
	}
	
	/* Set sensitivity */
	gtk_widget_set_sensitive (dlg->priv->backup_copy_checkbutton,
				  gedit_prefs_manager_create_backup_copy_can_set ());

	gtk_widget_set_sensitive (autosave_hbox, 
				  gedit_prefs_manager_auto_save_can_set ()); 

	gtk_widget_set_sensitive (save_frame,
				  gedit_prefs_manager_save_encoding_can_set ());
	
	gtk_widget_set_sensitive (dlg->priv->create_frame,
				  gedit_prefs_manager_save_encoding_can_set () && 
				  ((encoding == GEDIT_SAVE_ORIGINAL_FILE_ENCODING_IF_POSSIBLE) ||
				   (encoding == GEDIT_SAVE_ORIGINAL_FILE_ENCODING_IF_POSSIBLE_NCL)));

	gtk_widget_set_sensitive (dlg->priv->auto_save_spinbutton, 
			          auto_save &&
				  gedit_prefs_manager_auto_save_interval_can_set ());

	/* Connect signals */
	g_signal_connect (G_OBJECT (dlg->priv->auto_save_checkbutton), "toggled", 
			  G_CALLBACK (gedit_preferences_dialog_auto_save_checkbutton_toggled), 
			  dlg);

	g_signal_connect (G_OBJECT (dlg->priv->backup_copy_checkbutton), "toggled", 
			  G_CALLBACK (gedit_preferences_dialog_backup_copy_checkbutton_toggled), 
			  dlg);

	g_signal_connect (G_OBJECT (dlg->priv->auto_save_spinbutton), "value_changed",
			  G_CALLBACK (gedit_preferences_dialog_auto_save_spinbutton_value_changed),
			  dlg);

	g_signal_connect (G_OBJECT (dlg->priv->utf8_radiobutton), "toggled", 
			  G_CALLBACK (gedit_preferences_dialog_save_radiobutton_toggled), 
			  dlg);

	g_signal_connect (G_OBJECT (dlg->priv->locale_if_possible_radiobutton), "toggled", 
			  G_CALLBACK (gedit_preferences_dialog_save_radiobutton_toggled), 
			  dlg);

	g_signal_connect (G_OBJECT (dlg->priv->original_if_possible_radiobutton), "toggled", 
			  G_CALLBACK (gedit_preferences_dialog_save_radiobutton_toggled), 
			  dlg);

	g_signal_connect (G_OBJECT (dlg->priv->create_utf8_radiobutton), "toggled", 
			  G_CALLBACK (gedit_preferences_dialog_save_radiobutton_toggled), 
			  dlg);

	g_signal_connect (G_OBJECT (dlg->priv->create_locale_if_possible_radiobutton), "toggled", 
			  G_CALLBACK (gedit_preferences_dialog_save_radiobutton_toggled), 
			  dlg);

	return TRUE;
}

static void
gedit_preferences_dialog_line_numbers_checkbutton_toggled (GtkToggleButton *button,
							 GeditPreferencesDialog *dlg)
{
	gedit_debug (DEBUG_PREFS, "");

	g_return_if_fail (button == GTK_TOGGLE_BUTTON (dlg->priv->line_numbers_checkbutton));
	
	if (gtk_toggle_button_get_active (button))
	{
		gtk_widget_set_sensitive (dlg->priv->line_numbers_spinbutton, 
					  gedit_prefs_manager_print_line_numbers_can_set ());
		/*
		gtk_widget_grab_focus (dlg->priv->line_numbers_spinbutton);
		*/

		gedit_prefs_manager_set_print_line_numbers (
			MAX (1, gtk_spin_button_get_value_as_int (
			   GTK_SPIN_BUTTON (dlg->priv->line_numbers_spinbutton))));
	}
	else	
	{
		gtk_widget_set_sensitive (dlg->priv->line_numbers_spinbutton, FALSE);
		gedit_prefs_manager_set_print_line_numbers (0);
	}
}

static void
gedit_preferences_dialog_add_header_checkbutton_toggled (GtkToggleButton *button,
							 GeditPreferencesDialog *dlg)
{
	gedit_debug (DEBUG_PREFS, "");

	g_return_if_fail (button == GTK_TOGGLE_BUTTON (dlg->priv->add_header_checkbutton));
	
	gedit_prefs_manager_set_print_header (gtk_toggle_button_get_active (button));
}

static void
gedit_preferences_dialog_wrap_lines_checkbutton_toggled (GtkToggleButton *button,
							 GeditPreferencesDialog *dlg)
{
	gedit_debug (DEBUG_PREFS, "");

	g_return_if_fail (button == GTK_TOGGLE_BUTTON (dlg->priv->wrap_lines_checkbutton));
	
	gedit_prefs_manager_set_print_wrap_mode (
			gtk_toggle_button_get_active (button) ? GTK_WRAP_CHAR : GTK_WRAP_NONE);
}

static void
gedit_preferences_dialog_line_numbers_spinbutton_value_changed (GtkSpinButton *spin_button,
		GeditPreferencesDialog *dlg)
{
	g_return_if_fail (spin_button == GTK_SPIN_BUTTON (dlg->priv->line_numbers_spinbutton));

	gedit_prefs_manager_set_print_line_numbers (
			MAX (1, gtk_spin_button_get_value_as_int (spin_button)));
}

static gboolean 
gedit_preferences_dialog_setup_page_page (GeditPreferencesDialog *dlg, GladeXML *gui)
{
	GtkWidget *line_numbers_hbox;
	gint print_line_numbers;

	gedit_debug (DEBUG_PREFS, "");
	
	line_numbers_hbox = glade_xml_get_widget (gui,
				"line_numbers_hbox");
	dlg->priv->add_header_checkbutton = glade_xml_get_widget (gui, 
				"add_header_checkbutton");
	dlg->priv->wrap_lines_checkbutton= glade_xml_get_widget (gui, 
				"wrap_lines_checkbutton");
	dlg->priv->line_numbers_checkbutton = glade_xml_get_widget (gui, 
				"line_numbers_checkbutton");
	dlg->priv->line_numbers_spinbutton = glade_xml_get_widget (gui, 
				"line_numbers_spinbutton");

	g_return_val_if_fail (line_numbers_hbox, FALSE);
	g_return_val_if_fail (dlg->priv->add_header_checkbutton, FALSE);
	g_return_val_if_fail (dlg->priv->wrap_lines_checkbutton, FALSE);
	g_return_val_if_fail (dlg->priv->line_numbers_checkbutton, FALSE);
	g_return_val_if_fail (dlg->priv->line_numbers_spinbutton, FALSE);

	/* Set initial values */
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dlg->priv->add_header_checkbutton),
			gedit_prefs_manager_get_print_header ());
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dlg->priv->wrap_lines_checkbutton),
			gedit_prefs_manager_get_print_wrap_mode () != GTK_WRAP_NONE);
	
	print_line_numbers = gedit_prefs_manager_get_print_line_numbers ();
		
	if (print_line_numbers > 0)
	{
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (dlg->priv->line_numbers_spinbutton),
					   (guint) print_line_numbers);
		gtk_widget_set_sensitive (dlg->priv->line_numbers_spinbutton, 
					  gedit_prefs_manager_print_line_numbers_can_set ());
	}
	else
		gtk_widget_set_sensitive (dlg->priv->line_numbers_spinbutton, FALSE);
	
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dlg->priv->line_numbers_checkbutton),
				      print_line_numbers > 0);

	/* Set widget sensitivity */
	gtk_widget_set_sensitive (dlg->priv->add_header_checkbutton, 
				  gedit_prefs_manager_print_header_can_set ());
	gtk_widget_set_sensitive (dlg->priv->wrap_lines_checkbutton,
				  gedit_prefs_manager_print_wrap_mode_can_set ());
	gtk_widget_set_sensitive (line_numbers_hbox,
				  gedit_prefs_manager_print_line_numbers_can_set ());

	/* Connect signals */
	g_signal_connect (G_OBJECT (dlg->priv->line_numbers_checkbutton), "toggled", 
			  G_CALLBACK (gedit_preferences_dialog_line_numbers_checkbutton_toggled), dlg);
	g_signal_connect (G_OBJECT (dlg->priv->add_header_checkbutton), "toggled", 
			  G_CALLBACK (gedit_preferences_dialog_add_header_checkbutton_toggled), dlg);
	g_signal_connect (G_OBJECT (dlg->priv->wrap_lines_checkbutton), "toggled", 
			  G_CALLBACK (gedit_preferences_dialog_wrap_lines_checkbutton_toggled), dlg);

	g_signal_connect (G_OBJECT (dlg->priv->line_numbers_spinbutton), "value_changed",
			  G_CALLBACK (gedit_preferences_dialog_line_numbers_spinbutton_value_changed),
			  dlg);
	return TRUE;
}

static void
gedit_preferences_dialog_print_font_restore_default_button_clicked (
		GtkButton *button, GeditPreferencesDialog *dlg)
{
	g_return_if_fail (dlg->priv->body_fontpicker != NULL);
	g_return_if_fail (dlg->priv->headers_fontpicker != NULL);
	g_return_if_fail (dlg->priv->numbers_fontpicker != NULL);

	if (gedit_prefs_manager_print_font_body_can_set ())
	{
		gnome_print_font_picker_set_font_name (
				GNOME_PRINT_FONT_PICKER (dlg->priv->body_fontpicker),
				gedit_prefs_manager_get_default_print_font_body ());

		gedit_prefs_manager_set_print_font_body (
				gedit_prefs_manager_get_default_print_font_body ());
	}
	
	if (gedit_prefs_manager_print_font_header_can_set ())
	{
		gnome_print_font_picker_set_font_name (
				GNOME_PRINT_FONT_PICKER (dlg->priv->headers_fontpicker),
				gedit_prefs_manager_get_default_print_font_header ());
		
		gedit_prefs_manager_set_print_font_header (
				gedit_prefs_manager_get_default_print_font_header ());
	}
		
	if (gedit_prefs_manager_print_font_numbers_can_set ())
	{
		gnome_print_font_picker_set_font_name (
				GNOME_PRINT_FONT_PICKER (dlg->priv->numbers_fontpicker),
				gedit_prefs_manager_get_default_print_font_numbers ());
		
		gedit_prefs_manager_set_print_font_numbers (
				gedit_prefs_manager_get_default_print_font_numbers ());
	}
}

static void
gedit_preferences_dialog_body_font_picker_font_set (GnomePrintFontPicker *gfp, 
		const gchar *font_name, GeditPreferencesDialog *dlg)
{
	gedit_debug (DEBUG_PREFS, "");

	g_return_if_fail (gfp == GNOME_PRINT_FONT_PICKER (dlg->priv->body_fontpicker));
	g_return_if_fail (font_name != NULL);

	gedit_prefs_manager_set_print_font_body (font_name);
}

static void
gedit_preferences_dialog_headers_font_picker_font_set (GnomePrintFontPicker *gfp, 
		const gchar *font_name, GeditPreferencesDialog *dlg)
{
	gedit_debug (DEBUG_PREFS, "");

	g_return_if_fail (gfp == GNOME_PRINT_FONT_PICKER (dlg->priv->headers_fontpicker));
	g_return_if_fail (font_name != NULL);

	gedit_prefs_manager_set_print_font_header (font_name);
}

static void
gedit_preferences_dialog_numbers_font_picker_font_set (GnomePrintFontPicker *gfp, 
		const gchar *font_name, GeditPreferencesDialog *dlg)
{
	gedit_debug (DEBUG_PREFS, "");

	g_return_if_fail (gfp == GNOME_PRINT_FONT_PICKER (dlg->priv->numbers_fontpicker));
	g_return_if_fail (font_name != NULL);

	gedit_prefs_manager_set_print_font_numbers (font_name);
}

static gboolean 
gedit_preferences_dialog_setup_print_fonts_page (GeditPreferencesDialog *dlg, GladeXML *gui)
{	
	GtkWidget *print_fonts_table;
	GtkWidget *body_font_label;
	GtkWidget *headers_font_label;
	GtkWidget *numbers_font_label;
	gboolean can_set;
	gchar* font;

	gedit_debug (DEBUG_PREFS, "");

	print_fonts_table = glade_xml_get_widget (gui, "print_fonts_table");
	body_font_label = glade_xml_get_widget (gui, "body_font_label");
	headers_font_label = glade_xml_get_widget (gui,	"headers_font_label");
	numbers_font_label = glade_xml_get_widget (gui,	"numbers_font_label");
	dlg->priv->restore_default_fonts_button = glade_xml_get_widget (gui, 
 			"restore_default_fonts_button");

	g_return_val_if_fail (print_fonts_table != NULL, FALSE);
	g_return_val_if_fail (body_font_label != NULL, FALSE);
	g_return_val_if_fail (headers_font_label != NULL, FALSE);
	g_return_val_if_fail (numbers_font_label != NULL, FALSE);
	g_return_val_if_fail (dlg->priv->restore_default_fonts_button != NULL, FALSE);
			
	/* Body font picker */
	dlg->priv->body_fontpicker = gnome_print_font_picker_new ();

	gnome_print_font_picker_set_mode (
			GNOME_PRINT_FONT_PICKER (dlg->priv->body_fontpicker), 
			GNOME_PRINT_FONT_PICKER_MODE_FONT_INFO);
	
	gnome_print_font_picker_fi_set_show_size (
			GNOME_PRINT_FONT_PICKER (dlg->priv->body_fontpicker), TRUE);

	gnome_print_font_picker_fi_set_use_font_in_label (
			GNOME_PRINT_FONT_PICKER (dlg->priv->body_fontpicker), TRUE, 14);

	gtk_table_attach_defaults (GTK_TABLE (print_fonts_table), 
			dlg->priv->body_fontpicker, 1, 2, 0, 1);
	
	/* Headers font picker */
	dlg->priv->headers_fontpicker = gnome_print_font_picker_new ();
	gnome_print_font_picker_set_mode (
			GNOME_PRINT_FONT_PICKER (dlg->priv->headers_fontpicker), 
			GNOME_PRINT_FONT_PICKER_MODE_FONT_INFO);
	
	gnome_print_font_picker_fi_set_show_size (
			GNOME_PRINT_FONT_PICKER (dlg->priv->headers_fontpicker), TRUE);
	
	gnome_print_font_picker_fi_set_use_font_in_label (
			GNOME_PRINT_FONT_PICKER (dlg->priv->headers_fontpicker), TRUE, 14);

	gtk_table_attach_defaults (GTK_TABLE (print_fonts_table), 
			dlg->priv->headers_fontpicker, 1, 2, 1, 2);

	/* Numbers font picker */
	dlg->priv->numbers_fontpicker = gnome_print_font_picker_new ();
	gnome_print_font_picker_set_mode (
			GNOME_PRINT_FONT_PICKER (dlg->priv->numbers_fontpicker), 
			GNOME_PRINT_FONT_PICKER_MODE_FONT_INFO);
	
	gnome_print_font_picker_fi_set_show_size (
			GNOME_PRINT_FONT_PICKER (dlg->priv->numbers_fontpicker), TRUE);
	
	gnome_print_font_picker_fi_set_use_font_in_label (
			GNOME_PRINT_FONT_PICKER (dlg->priv->numbers_fontpicker), TRUE, 14);

	gtk_table_attach_defaults (GTK_TABLE (print_fonts_table), 
			dlg->priv->numbers_fontpicker, 1, 2, 2, 3);

	gtk_label_set_mnemonic_widget (GTK_LABEL (body_font_label), 
				       dlg->priv->body_fontpicker);
	
	gtk_label_set_mnemonic_widget (GTK_LABEL (headers_font_label), 
				       dlg->priv->headers_fontpicker);

	gtk_label_set_mnemonic_widget (GTK_LABEL (numbers_font_label), 
				       dlg->priv->numbers_fontpicker);

	gtk_tooltips_set_tip (gtk_tooltips_new(), dlg->priv->body_fontpicker, 
		_("Push this button to select the font to be used to print the body"), NULL);
	gtk_tooltips_set_tip (gtk_tooltips_new(), dlg->priv->headers_fontpicker, 
		_("Push this button to select the font to be used to print the headers"), NULL);
	gtk_tooltips_set_tip (gtk_tooltips_new(), dlg->priv->numbers_fontpicker, 
		_("Push this button to select the font to be used to print line numbers"), NULL);

	gedit_utils_set_atk_relation (dlg->priv->body_fontpicker, body_font_label, 
							ATK_RELATION_LABELLED_BY);
	gedit_utils_set_atk_relation (dlg->priv->headers_fontpicker, headers_font_label, 
							ATK_RELATION_LABELLED_BY);
	gedit_utils_set_atk_relation (dlg->priv->numbers_fontpicker, numbers_font_label, 
							ATK_RELATION_LABELLED_BY);

	/* Set initial values */
	font = gedit_prefs_manager_get_print_font_body ();
	g_return_val_if_fail (font, FALSE);
	
	gnome_print_font_picker_set_font_name (
			GNOME_PRINT_FONT_PICKER (dlg->priv->body_fontpicker),
			font);

	g_free (font);

	font = gedit_prefs_manager_get_print_font_header ();
	g_return_val_if_fail (font, FALSE);

	gnome_print_font_picker_set_font_name (
			GNOME_PRINT_FONT_PICKER (dlg->priv->headers_fontpicker),
			font);

	g_free (font);

	font = gedit_prefs_manager_get_print_font_numbers ();
	g_return_val_if_fail (font, FALSE);
	
	gnome_print_font_picker_set_font_name (
			GNOME_PRINT_FONT_PICKER (dlg->priv->numbers_fontpicker),
			font);

	g_free (font);

	/* Set widgets sensitivity */
	can_set = gedit_prefs_manager_print_font_body_can_set ();
	gtk_widget_set_sensitive (dlg->priv->body_fontpicker, can_set);
	gtk_widget_set_sensitive (body_font_label, can_set);
		  
	can_set = gedit_prefs_manager_print_font_header_can_set ();
	gtk_widget_set_sensitive (dlg->priv->headers_fontpicker, can_set);
	gtk_widget_set_sensitive (headers_font_label, can_set);

	can_set = gedit_prefs_manager_print_font_numbers_can_set ();
	gtk_widget_set_sensitive (dlg->priv->numbers_fontpicker, can_set);
	gtk_widget_set_sensitive (numbers_font_label, can_set);

	/* Connect signals */
	g_signal_connect (G_OBJECT (dlg->priv->restore_default_fonts_button), "clicked",
			  G_CALLBACK (gedit_preferences_dialog_print_font_restore_default_button_clicked),
			  dlg);

	g_signal_connect (G_OBJECT (dlg->priv->body_fontpicker), "font_set", 
			  G_CALLBACK (gedit_preferences_dialog_body_font_picker_font_set), 
			  dlg);

	g_signal_connect (G_OBJECT (dlg->priv->headers_fontpicker), "font_set", 
			  G_CALLBACK (gedit_preferences_dialog_headers_font_picker_font_set), 
			  dlg);

	g_signal_connect (G_OBJECT (dlg->priv->numbers_fontpicker), "font_set", 
			  G_CALLBACK (gedit_preferences_dialog_numbers_font_picker_font_set), 
			  dlg);


	return TRUE;
}

static void
gedit_preferences_dialog_display_line_numbers_checkbutton_toggled (GtkToggleButton *button,
							 GeditPreferencesDialog *dlg)
{
	g_return_if_fail (button == 
			GTK_TOGGLE_BUTTON (dlg->priv->display_line_numbers_checkbutton));

	gedit_prefs_manager_set_display_line_numbers (gtk_toggle_button_get_active (button));
}

static gboolean 
gedit_preferences_dialog_setup_line_numbers_page (GeditPreferencesDialog *dlg, GladeXML *gui)
{
	gedit_debug (DEBUG_PREFS, "");
	
	dlg->priv->display_line_numbers_checkbutton = glade_xml_get_widget (gui, 
				"display_line_numbers_checkbutton");

	g_return_val_if_fail (dlg->priv->display_line_numbers_checkbutton != NULL, FALSE);
	
	/* Set initial state */
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dlg->priv->display_line_numbers_checkbutton),
				gedit_prefs_manager_get_display_line_numbers ());

	/* Set widget sensitivity */
	gtk_widget_set_sensitive (dlg->priv->display_line_numbers_checkbutton,
			gedit_prefs_manager_display_line_numbers_can_set ());

	/* Connect signal */
	g_signal_connect (G_OBJECT (dlg->priv->display_line_numbers_checkbutton), "toggled", 
			  G_CALLBACK (gedit_preferences_dialog_display_line_numbers_checkbutton_toggled), 
			  dlg);

	return TRUE;
}

static gboolean 
gedit_preferences_dialog_setup_plugin_manager_page (GeditPreferencesDialog *dlg, GladeXML *gui)
{
	GtkWidget *plugin_manager_page;
	GtkWidget *page_content;
	
	gedit_debug (DEBUG_PREFS, "");

	plugin_manager_page = glade_xml_get_widget (gui, "plugin_manager_page");
	g_return_val_if_fail (plugin_manager_page != NULL, FALSE);
	
	page_content = gedit_plugin_manager_get_page ();
	g_return_val_if_fail (page_content != NULL, FALSE);
	
	gtk_box_pack_start (GTK_BOX (plugin_manager_page), page_content, TRUE, TRUE, 0);
	
	return TRUE;
}

static GtkTreeModel*
create_encodings_treeview_model (void)
{
	GtkListStore *store;
	GtkTreeIter iter;
	GSList const *list;
	gint i = 0;
	
	gedit_debug (DEBUG_PREFS, "");

	/* create list store */
	store = gtk_list_store_new (ENCODING_NUM_COLS, G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_INT);

	/* add data to the list store */
	list = gedit_prefs_manager_get_encodings ();
	
	while (list != NULL)
	{
		const GeditEncoding* enc;
		gchar *name;

		enc = (const GeditEncoding*) list->data;
		name = gedit_encoding_to_string (enc);
		
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter,
				    COLUMN_ENCODING_NAME, name,
				    COLUMN_ENCODING_POINTER, enc,
				    COLUMN_INDEX, i,
				    -1);

		g_free (name);
		
		list = g_slist_next (list);
		++i;
	}
	
	return GTK_TREE_MODEL (store);
}

static void
gedit_preferences_dialog_add_enc_button_clicked (GtkButton *button, GeditPreferencesDialog *dlg)
{
	gedit_debug (DEBUG_PREFS, "");

	gedit_encodings_dialog_run (dlg);
}


static gboolean 
gedit_preferences_dialog_setup_load_page (GeditPreferencesDialog *dlg, GladeXML *gui)
{
	GtkTreeModel *model;
	GtkTreeViewColumn *column;
	GtkCellRenderer *cell;

	gedit_debug (DEBUG_PREFS, "");

	dlg->priv->encodings_treeview = glade_xml_get_widget (gui, "encodings_treeview");
	dlg->priv->add_enc_button = glade_xml_get_widget (gui, "add_enc_button");
	dlg->priv->remove_enc_button = glade_xml_get_widget (gui, "remove_enc_button") ;
	dlg->priv->up_enc_button = glade_xml_get_widget (gui, "up_enc_button");
	dlg->priv->down_enc_button = glade_xml_get_widget (gui, "down_enc_button");

	g_return_val_if_fail (dlg->priv->encodings_treeview != NULL, FALSE);
	g_return_val_if_fail (dlg->priv->add_enc_button != NULL, FALSE);
	g_return_val_if_fail (dlg->priv->remove_enc_button != NULL, FALSE);
	g_return_val_if_fail (dlg->priv->up_enc_button != NULL, FALSE);
	g_return_val_if_fail (dlg->priv->down_enc_button != NULL, FALSE);

	gtk_widget_set_sensitive (dlg->priv->remove_enc_button, FALSE);
	gtk_widget_set_sensitive (dlg->priv->up_enc_button, FALSE);
	gtk_widget_set_sensitive (dlg->priv->down_enc_button, FALSE);

	g_signal_connect (G_OBJECT (dlg->priv->add_enc_button), "clicked", 
			  G_CALLBACK (gedit_preferences_dialog_add_enc_button_clicked), 
			  dlg);
	
	model = create_encodings_treeview_model ();
	g_return_val_if_fail (model != NULL, FALSE);

	gtk_tree_view_set_model (GTK_TREE_VIEW (dlg->priv->encodings_treeview), model);

	/* Add the encoding column */
	cell = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Encodings"), cell, 
			"text", COLUMN_ENCODING_NAME, NULL);
	
	gtk_tree_view_append_column (GTK_TREE_VIEW (dlg->priv->encodings_treeview), column);

	gtk_tree_view_set_search_column (GTK_TREE_VIEW (dlg->priv->encodings_treeview),
			COLUMN_ENCODING_NAME);

	return TRUE;
}

gboolean
gedit_preferences_dialog_add_encoding (GeditPreferencesDialog *dlg, const GeditEncoding* enc)
{
	GtkTreeModel *model;

	gboolean found = FALSE;
	GtkTreeIter iter;
	gchar *name;
	GValue value = {0, };

	gedit_debug (DEBUG_PREFS, "");
	
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (dlg->priv->encodings_treeview));

	if (gtk_tree_model_get_iter_first (model, &iter))
	{
		gtk_tree_model_get_value (model, &iter,
			    COLUMN_ENCODING_POINTER, &value);

		if (enc == g_value_get_pointer (&value))
			found = TRUE;
			
		g_value_unset (&value);

		while (gtk_tree_model_iter_next (model, &iter) && !found)
		{
			gtk_tree_model_get_value (model, &iter,
				    COLUMN_ENCODING_POINTER, &value);

			if (enc == g_value_get_pointer (&value))
				found = TRUE;
			
				g_value_unset (&value);
		}
	}

	if (found)
		return FALSE;

	name = gedit_encoding_to_string (enc);
		
	gtk_list_store_append (GTK_LIST_STORE (model), &iter);

	gtk_list_store_set (GTK_LIST_STORE (model), &iter,
				    COLUMN_ENCODING_NAME, name,
				    COLUMN_ENCODING_POINTER, enc,
				    COLUMN_INDEX, gtk_tree_model_iter_n_children (model, NULL),
				    -1);
	g_free (name);

	return TRUE;
}



