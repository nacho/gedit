/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */

#include <config.h>
#include <glib.h>
#include <string.h> /* For strlen and strcmp */
#include <gnome.h>
#include <glade/glade.h>

#include "file.h"
#include "plugin.h"
#include "document.h"
#include "utils.h"
#include "window.h"

#include "dialogs/dialogs.h"
#include "gtkchecklist.h"

#define GEDIT_PLUGIN_MANAGER_GLADE_FILE "plugin-manager2.glade"

GtkWidget *list_of_plugins;
GtkWidget *description_label;
GtkWidget *authors_label;
GtkWidget *file_label;

/**
 * gedit_plugin_email_sendmail_location_dialog:
 * @void: 
 * 
 * it displays and pop-up a dialog so that the user can specify a
 * location for an auxiliary program for a plugin (sendmail,diff,lynx)
 * 
 * Return Value: a string with the path specified by the user
 **/
gchar *
gedit_plugin_program_location_dialog (gchar *program_name, gchar *plugin_name)
{
	GladeXML *gui;
	GtkWidget *program_location_dialog = NULL;
	GtkWidget *program_location_entry = NULL;
	GtkWidget *program_location_combo_entry = NULL;
	GtkWidget *label = NULL;
	gchar *str_label;
	
	gchar * location = NULL;

	gedit_debug (DEBUG_PLUGINS, "");

	gui = glade_xml_new (GEDIT_GLADEDIR "/program.glade", NULL);

	if (!gui)
	{
		g_warning ("Could not find program.glade");
		return NULL;
	}

	program_location_dialog = glade_xml_get_widget (gui, "program_location_dialog");
	program_location_entry  = glade_xml_get_widget (gui, "program_location_file_entry");
	program_location_combo_entry  = glade_xml_get_widget (gui, "combo-entry1");
	label = glade_xml_get_widget (gui, "label");

	if (!program_location_dialog || !program_location_entry || 
	    !label || !program_location_combo_entry)
	{
		g_warning ("Could not get the program location dialog from program.glade");
		return NULL;
	}

	str_label = g_strdup_printf(_("To use the %s plugin, you need to specify a location for \"%s\"."),
			plugin_name, program_name);

	gtk_label_set_text (GTK_LABEL (label), str_label);
	g_free(str_label);

	gnome_dialog_set_default     (GNOME_DIALOG (program_location_dialog), 0);
	gnome_dialog_editable_enters (GNOME_DIALOG (program_location_dialog), 
				      GTK_EDITABLE (program_location_combo_entry));
	
	/* If we do this the main plugin dialog goes below the program window. Chema.
	*/
	switch (gnome_dialog_run (GNOME_DIALOG (program_location_dialog)))
	{
	case 0:
		location = gnome_file_entry_get_full_path(GNOME_FILE_ENTRY(program_location_entry), FALSE);
		gnome_dialog_close (GNOME_DIALOG(program_location_dialog));
		break;
	case 1:
		gnome_dialog_close (GNOME_DIALOG(program_location_dialog));
		break;
	default:
		location = NULL;
		break;
	}

	gtk_object_unref (GTK_OBJECT (gui));

	return location;
}

static void
gedit_plugin_manager_ok_clicked (GtkWidget *widget, GtkWidget* data)
{
	gint i;
	GnomeApp *nth_app;
	PluginData *plugin_data;

	for (i = 0; i < GTK_CHECK_LIST (list_of_plugins)->n_rows; ++i)
	{
		plugin_data = (PluginData*)gtk_check_list_get_row_data (
						GTK_CHECK_LIST (list_of_plugins), i);

		if (gtk_check_list_get_toggled (GTK_CHECK_LIST (list_of_plugins), i))
		{
			plugin_data->installed = TRUE;
		}
		else
		{
			plugin_data->installed = FALSE;
		}
	}

	gnome_dialog_close (GNOME_DIALOG (data));
	
	for (i = 0; i < g_list_length (mdi->windows); ++i)
	{
		nth_app = g_list_nth_data (mdi->windows, i);
		gedit_plugins_menu_add (nth_app);	
	}
	
	if(gedit_document_current () == NULL) 
		gedit_window_set_plugins_menu_sensitivity (FALSE);
		
	return;
}

static void
gedit_plugin_manager_close (GtkWidget *widget, GtkWidget* data)
{
	gedit_debug (DEBUG_PLUGINS, "");
	gnome_dialog_close (GNOME_DIALOG (data));
}

static void
gedit_plugin_manager_help_clicked (GtkWidget *widget, GtkWidget* data)
{
	static GnomeHelpMenuEntry help_entry = { "gedit", "plugins.html" };
	gnome_help_display (NULL, &help_entry);	
		
	return;
}

static void
gedit_plugin_manager_remove_all_clicked (GtkWidget *widget, GtkWidget* data)
{
	gint i;

	gtk_clist_freeze (GTK_CLIST (list_of_plugins));

	for (i=0; i < GTK_CHECK_LIST (list_of_plugins)->n_rows; ++i)
	{
		gtk_check_list_set_toggled (GTK_CHECK_LIST (list_of_plugins), i, FALSE);
	}

	gtk_clist_thaw (GTK_CLIST (list_of_plugins));	
}

static void
gedit_plugin_manager_select_all_clicked (GtkWidget *widget, GtkWidget* data)
{
	gint i;

	gtk_clist_freeze (GTK_CLIST (list_of_plugins));

	for (i=0; i < GTK_CHECK_LIST (list_of_plugins)->n_rows; ++i)
	{
		gtk_check_list_set_toggled (GTK_CHECK_LIST (list_of_plugins), i, TRUE);
	}

	gtk_clist_thaw (GTK_CLIST (list_of_plugins));	
}

static gint 
gedit_plugin_manager_clist_compare_function (GtkCList *clist, GtkCListRow *row_1, GtkCListRow *row_2)
{
	gchar *string_1 = ((GtkCellText*)&(row_1->cell[clist->sort_column]))->text;
	gchar *string_2 = ((GtkCellText*)&(row_2->cell[clist->sort_column]))->text;

	gedit_debug (DEBUG_PLUGINS, "");

	g_return_val_if_fail (string_1 != NULL, -1);
	g_return_val_if_fail (string_2 != NULL, -1);
	
	return strcmp (string_1, string_2);
}

static void
gedit_plugin_manager_item_load ()
{
	gint n;
	PluginData *nth_plugin_data;
	
	gtk_clist_freeze (GTK_CLIST (list_of_plugins));

	for (n=0; n < g_list_length (plugins_list); n++)
	{
		nth_plugin_data = g_list_nth_data (plugins_list, n);
		
		gtk_check_list_append_row (GTK_CHECK_LIST (list_of_plugins),
			nth_plugin_data->name, nth_plugin_data->installed, 
			(gpointer) nth_plugin_data);
	}

	gtk_clist_set_compare_func (GTK_CLIST (list_of_plugins),
				    (GtkCListCompareFunc) gedit_plugin_manager_clist_compare_function);
	
	gtk_clist_set_sort_column (GTK_CLIST (list_of_plugins), 1);
	gtk_clist_set_auto_sort (GTK_CLIST (list_of_plugins), TRUE);
	
	gtk_clist_thaw (GTK_CLIST (list_of_plugins));
}

static void
gedit_plugin_manager_select_row (GtkCList *clist,
                                 gint row,
                                 gint column,
                                 GdkEventButton *event,
                                 gpointer user_data)
{
	PluginData *plugin_data;
	plugin_data = (PluginData*)gtk_check_list_get_row_data (GTK_CHECK_LIST (list_of_plugins), row);

	gtk_label_set_text (GTK_LABEL (description_label), plugin_data->long_desc);
	gtk_label_set_text (GTK_LABEL (authors_label), plugin_data->author);
	gtk_label_set_text (GTK_LABEL (file_label), plugin_data->file);
}

void
gedit_plugin_manager_create (GtkWidget *widget, gpointer data)
{
	GladeXML *gui;
	GtkWidget *dialog;
	
	GtkWidget *ok_button;
	GtkWidget *cancel_button;
	GtkWidget *help_button;
	GtkWidget *scrolled_window;
	
	GtkWidget *select_all_button;
	GtkWidget *remove_all_button;

	gedit_debug (DEBUG_PLUGINS, "");
	
	gui = glade_xml_new (GEDIT_GLADEDIR GEDIT_PLUGIN_MANAGER_GLADE_FILE, NULL);

	if (!gui)
	{
		g_warning ("Could not find %s", GEDIT_GLADEDIR GEDIT_PLUGIN_MANAGER_GLADE_FILE);
		return;
	}

	dialog             = glade_xml_get_widget (gui, "plugin_manager");
	ok_button          = glade_xml_get_widget (gui, "ok_button");
	cancel_button      = glade_xml_get_widget (gui, "cancel_button");
	help_button        = glade_xml_get_widget (gui, "help_button");
	scrolled_window    = glade_xml_get_widget (gui, "scrolled_window"); 
	select_all_button  = glade_xml_get_widget (gui, "select_all_button");
	remove_all_button  = glade_xml_get_widget (gui, "remove_all_button");
	description_label  = glade_xml_get_widget (gui, "description_label");
	authors_label      = glade_xml_get_widget (gui, "authors_label");
        file_label         = glade_xml_get_widget (gui, "file_label");

	g_return_if_fail (dialog             != NULL);
	g_return_if_fail (ok_button          != NULL);
	g_return_if_fail (cancel_button      != NULL);
	g_return_if_fail (help_button        != NULL);
	g_return_if_fail (scrolled_window    != NULL);
	g_return_if_fail (select_all_button  != NULL);
	g_return_if_fail (remove_all_button  != NULL);
	g_return_if_fail (description_label  != NULL);
	g_return_if_fail (authors_label      != NULL);
	g_return_if_fail (file_label         != NULL);

    	/* Create the GtkCheckList. */
	list_of_plugins = gtk_check_list_new ();
	gtk_container_add(GTK_CONTAINER(scrolled_window), list_of_plugins);

	/* connect the signals */
	gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
			    GTK_SIGNAL_FUNC (gedit_plugin_manager_close), dialog);

	gtk_signal_connect (GTK_OBJECT (ok_button), "clicked",
			    GTK_SIGNAL_FUNC (gedit_plugin_manager_ok_clicked), dialog);
	gtk_signal_connect (GTK_OBJECT (cancel_button), "clicked",
			    GTK_SIGNAL_FUNC (gedit_plugin_manager_close), dialog);
	gtk_signal_connect (GTK_OBJECT (help_button), "clicked",
			    GTK_SIGNAL_FUNC (gedit_plugin_manager_help_clicked), NULL);

	gtk_signal_connect (GTK_OBJECT (remove_all_button), "clicked",
			    GTK_SIGNAL_FUNC (gedit_plugin_manager_remove_all_clicked), NULL);
	gtk_signal_connect (GTK_OBJECT (select_all_button), "clicked",
			    GTK_SIGNAL_FUNC (gedit_plugin_manager_select_all_clicked), NULL);
	
	/* Set the dialog parent and modal type */
	gnome_dialog_set_parent  (GNOME_DIALOG (dialog), gedit_window_active());
	gtk_window_set_modal     (GTK_WINDOW (dialog), TRUE);
	gnome_dialog_set_default (GNOME_DIALOG (dialog), 0);

	/* Show everything and free */
	gtk_widget_show_all (dialog);
	gtk_object_unref (GTK_OBJECT (gui));

	gedit_plugin_manager_item_load ();

	gtk_signal_connect (GTK_OBJECT (list_of_plugins), "select_row",
		            GTK_SIGNAL_FUNC (gedit_plugin_manager_select_row), NULL);

	gtk_clist_select_row (GTK_CLIST (list_of_plugins), 0, 1);

}
