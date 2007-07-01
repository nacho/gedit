/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-spell-language-dialog.c
 * This file is part of gedit
 *
 * Copyright (C) 2002 Paolo Maggi 
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
 * Modified by the gedit Team, 2002. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n.h>
#include <glade/glade-xml.h>
#include <gtk/gtk.h>
#include <gedit/gedit-utils.h>
#include <gedit/gedit-help.h>

#include "gedit-spell-language-dialog.h"
#include "gedit-spell-checker-language.h"

enum
{
	COLUMN_LANGUAGE_NAME = 0,
	COLUMN_LANGUAGE_POINTER,
	ENCODING_NUM_COLS
};


typedef struct _GeditSpellLanguageDialog GeditSpellLanguageDialog;

struct _GeditSpellLanguageDialog 
{
	GtkWidget *dialog;
	
	GtkWidget *languages_treeview;
	GtkTreeModel *model;

	GeditSpellChecker *spell_checker;
};

static void ok_button_pressed (GeditSpellLanguageDialog *dialog);
static GeditSpellLanguageDialog *get_languages_dialog (GeditSpellChecker *spell_checker);

static void
dialog_destroyed (GtkObject *obj,  void **dialog_pointer)
{
	if (dialog_pointer != NULL)
	{
		g_free (*dialog_pointer);
		*dialog_pointer = NULL;
	}	
}

static void
dialog_response_handler (GtkDialog                *dlg,
			 gint                      res_id,
			 GeditSpellLanguageDialog *dialog)
{
	switch (res_id)
	{
		case GTK_RESPONSE_OK:
			ok_button_pressed (dialog);
			gtk_widget_destroy (dialog->dialog);
			break;

		case GTK_RESPONSE_HELP:
			gedit_help_display (GTK_WINDOW (dlg),
					    "gedit.xml",
					    "gedit-spell-checker-plugin");
			break;

		default:
			gtk_widget_destroy (dialog->dialog);
	}
}

static void 
ok_button_pressed (GeditSpellLanguageDialog *dialog)
{
	GValue value = {0, };
	const GeditSpellCheckerLanguage* lang;

	GtkTreeIter iter;
	GtkTreeSelection *selection;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (dialog->languages_treeview));
	g_return_if_fail (selection != NULL);

	if (!gtk_tree_selection_get_selected (selection, NULL, &iter))
			return;

	gtk_tree_model_get_value (dialog->model, &iter,
			    COLUMN_LANGUAGE_POINTER, &value);

	lang = (const GeditSpellCheckerLanguage* ) g_value_get_pointer (&value);
	g_return_if_fail (lang != NULL);
	
	gedit_spell_checker_set_language (dialog->spell_checker, lang);
}

static GtkTreeModel *
init_languages_treeview_model (GeditSpellLanguageDialog *dlg)
{
	GtkListStore *store;
	GtkTreeIter iter;

	const GSList* langs;

	/* create list store */
	store = GTK_LIST_STORE (dlg->model);

	langs = gedit_spell_checker_get_available_languages ();

	while (langs)
	{
		const gchar *name;

		name = gedit_spell_checker_language_to_string ((const GeditSpellCheckerLanguage*)langs->data);
	       	
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter,
				    COLUMN_LANGUAGE_NAME, name,
				    COLUMN_LANGUAGE_POINTER, langs->data,
				    -1);
				    
		if (langs->data == gedit_spell_checker_get_language (dlg->spell_checker))
		{
			GtkTreeSelection *selection;
						
			selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (dlg->languages_treeview));
			g_return_val_if_fail (selection != NULL, GTK_TREE_MODEL (store));
			gtk_tree_selection_select_iter (selection, &iter);
		}

		langs = g_slist_next (langs);
	}
	
	return GTK_TREE_MODEL (store);
}

static void 
scroll_to_selected (GtkTreeView *tree_view)
{
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreeIter iter;

	model = gtk_tree_view_get_model (tree_view);
	g_return_if_fail (model != NULL);

	/* Scroll to selected */
	selection = gtk_tree_view_get_selection (tree_view);
	g_return_if_fail (selection != NULL);

	if (gtk_tree_selection_get_selected (selection, NULL, &iter))
	{
		GtkTreePath* path;

		path = gtk_tree_model_get_path (model, &iter);
		g_return_if_fail (path != NULL);

		gtk_tree_view_scroll_to_cell (tree_view,
					      path, NULL, TRUE, 1.0, 0.0);
		gtk_tree_path_free (path);
	}
}

static void
language_row_activated (GtkTreeView *tree_view,
			GtkTreePath *path,
			GtkTreeViewColumn *column,
			GeditSpellLanguageDialog *dialog)
{
	gtk_dialog_response (GTK_DIALOG (dialog->dialog), GTK_RESPONSE_OK);
}

static GeditSpellLanguageDialog *
get_languages_dialog (GeditSpellChecker *spell_checker)
{
	GladeXML *gui;
	static GeditSpellLanguageDialog *dialog = NULL;	
	GtkTreeViewColumn *column;
	GtkCellRenderer *cell;

	if (dialog != NULL)
	{
		gtk_window_present (GTK_WINDOW (dialog->dialog));
	}

	gui = glade_xml_new (GEDIT_GLADEDIR "languages-dialog.glade2",
			     "dialog", NULL);

	if (!gui) 
	{
		g_warning
		    ("Could not find languages-dialog.glade2, reinstall gedit.\n");
		return NULL;
	}

	dialog = g_new0 (GeditSpellLanguageDialog, 1);

	dialog->spell_checker = spell_checker;

	dialog->dialog = glade_xml_get_widget (gui, "dialog");
	
	dialog->languages_treeview = glade_xml_get_widget (gui, "languages_treeview");

	if (!dialog->dialog || 
	    !dialog->languages_treeview) 
	{
		g_warning (
			_("Could not find the required widgets inside %s."), "languages-dialog.glade2.");
		g_object_unref (gui);
		return NULL;
	}
	
	g_signal_connect (dialog->dialog,
			  "destroy",
			  G_CALLBACK (dialog_destroyed),
			  &dialog);
	g_signal_connect (dialog->dialog,
			  "response",
			  G_CALLBACK (dialog_response_handler),
			  dialog);

	dialog->model = GTK_TREE_MODEL (gtk_list_store_new (ENCODING_NUM_COLS, G_TYPE_STRING, G_TYPE_POINTER));
	g_return_val_if_fail (dialog->model != NULL, FALSE);

	gtk_tree_view_set_model (GTK_TREE_VIEW (dialog->languages_treeview), dialog->model);
	
	/* Add the encoding column */
	cell = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Languages"), cell, 
			"text", COLUMN_LANGUAGE_NAME, NULL);
	
	gtk_tree_view_append_column (GTK_TREE_VIEW (dialog->languages_treeview), column);

	gtk_tree_view_set_search_column (GTK_TREE_VIEW (dialog->languages_treeview),
			COLUMN_LANGUAGE_NAME);

	init_languages_treeview_model (dialog);

	g_signal_connect (dialog->languages_treeview,
			  "realize",
			  G_CALLBACK (scroll_to_selected),
			  dialog);
	g_signal_connect (dialog->languages_treeview,
			  "row-activated", 
			  G_CALLBACK (language_row_activated),
			  dialog);

	g_object_unref (gui);

	return dialog;
}

void 
gedit_spell_language_dialog_run (GeditSpellChecker *spell_checker, GtkWindow *parent)
{
	GeditSpellLanguageDialog *dialog;

	g_return_if_fail (GTK_IS_WINDOW (parent));
	g_return_if_fail (spell_checker != NULL);

	dialog = get_languages_dialog (spell_checker);

	gtk_window_set_transient_for (GTK_WINDOW (dialog->dialog), parent);

	gtk_widget_grab_focus (dialog->languages_treeview);

	if (!GTK_WIDGET_VISIBLE (dialog->dialog))
		gtk_widget_show (dialog->dialog);
}
