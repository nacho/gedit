/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit
 *
 * Copyright (C) 2000 Jose M Celorio
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
#include <gnome.h>
#include <glade/glade.h>

#include "file.h"
#include "plugin.h"
#include "document.h"
#include "utils.h"
#include "window.h"

#include "dialogs/dialogs.h"

#define GEDIT_PLUGIN_MANAGER_GLADE_FILE "plugin-manager.glade"

GtkWidget *installed_list;
GtkWidget *available_list;

GtkEditable *text_box;
gint text_length = 0;


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
gedit_plugin_program_location_dialog (void)
{
	GladeXML *gui;
	GtkWidget *program_location_dialog;
	GtkWidget *program_location_entry;
	gchar * location = NULL;

	gedit_debug (DEBUG_WINDOW, "");

	gui = glade_xml_new (GEDIT_GLADEDIR "/program.glade", NULL);

	if (!gui)
	{
		g_warning ("Could not find program.glade");
		return NULL;
	}

	program_location_dialog = glade_xml_get_widget (gui, "program_location_dialog");
	program_location_entry  = glade_xml_get_widget (gui, "program_location_file_entry");

	if (!program_location_dialog || !program_location_entry)
	{
		g_warning ("Could not get the program location dialog from program.glade");
		return NULL;
	}

	/* If we do this the main plugin dialog goes below the program window. Chema.
	  gnome_dialog_set_parent (GNOME_DIALOG(program_location_dialog), gedit_window_active());
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
gedit_plugin_manager_info_clear (void)
{
	if (text_length > 0)
		gtk_editable_delete_text (text_box, 0, text_length);
	text_length = 0;
}

static void
gedit_plugin_manager_lists_thaw (void)
{
	gtk_clist_unselect_all (GTK_CLIST (installed_list));
	gtk_clist_unselect_all (GTK_CLIST (available_list));

#if 0
	g_print ("A:%i I:%i\n",
		 GTK_CLIST (available_list)->rows,
		 GTK_CLIST (installed_list)->rows);
#endif	

	/*
	if (GTK_CLIST (available_list)->rows > 0)
		gtk_clist_thaw (GTK_CLIST (available_list));
	if (GTK_CLIST (installed_list)->rows > 0)
	    gtk_clist_thaw (GTK_CLIST (installed_list));
	*/

	gedit_plugin_manager_info_clear ();
}

static void
gedit_plugin_manager_lists_freeze (void)
{
	/*
	gtk_clist_freeze (GTK_CLIST (installed_list));
	gtk_clist_freeze (GTK_CLIST (available_list));
	*/
}

static gint 
gedit_plugin_manager_clist_compare_function (GtkCList *clist, GtkCListRow *row_1, GtkCListRow *row_2)
{
	gchar *string_1 = ((GtkCellText*)&(row_1->cell[clist->sort_column]))->text;
	gchar *string_2 = ((GtkCellText*)&(row_2->cell[clist->sort_column]))->text;

	return strcmp (string_1, string_2);
}

static void
gedit_plugin_manager_item_load (void)
{
	gint n;
	gint row;
	PluginData *nth_plugin_data;
	gchar * name[1];

	gedit_plugin_manager_lists_freeze();
	
	for (n=0; n < g_list_length (plugins_list); n++)
	{
		nth_plugin_data = g_list_nth_data (plugins_list, n);
		name[0] = nth_plugin_data->name;
		if (nth_plugin_data->installed)
		{
			row = gtk_clist_append (GTK_CLIST(installed_list),
						name);
			gtk_clist_set_row_data (GTK_CLIST(installed_list),
						row, nth_plugin_data);
		}
		else
		{
			row = gtk_clist_append (GTK_CLIST(available_list),
						name);
			gtk_clist_set_row_data (GTK_CLIST(available_list),
						row, nth_plugin_data);
		}
	}

	gtk_clist_set_compare_func (GTK_CLIST (installed_list),
				    (GtkCListCompareFunc) gedit_plugin_manager_clist_compare_function);
	gtk_clist_set_compare_func (GTK_CLIST (available_list),
				    (GtkCListCompareFunc) gedit_plugin_manager_clist_compare_function);
	
	gtk_clist_set_sort_column (GTK_CLIST (installed_list), 0);
	gtk_clist_set_sort_column (GTK_CLIST (available_list), 0);

	gtk_clist_set_auto_sort (GTK_CLIST (installed_list), TRUE);
	gtk_clist_set_auto_sort (GTK_CLIST (available_list), TRUE);
	
	gedit_plugin_manager_lists_thaw();
	

}

static void
gedit_plugin_manager_item_add (GtkWidget *widget, gpointer data)
{
	gint n;
	gint row;
	gint selection_length;
	gchar *name;
	gchar *name_array[1];
	gpointer item_data;

	selection_length = g_list_length (GTK_CLIST (available_list)->selection);

	gedit_plugin_manager_lists_freeze();

	for (n=0; n < selection_length; n++)
	{
		row = (gint) g_list_nth_data (GTK_CLIST (available_list)->selection, n);
		gtk_clist_get_text (GTK_CLIST (available_list), row, 0, &name);
		item_data = gtk_clist_get_row_data (GTK_CLIST(available_list), row);

		name_array[0] = g_strdup(name);
		gtk_clist_remove (GTK_CLIST (available_list), row);
		row = gtk_clist_append (GTK_CLIST (installed_list), name_array);
		gtk_clist_set_row_data (GTK_CLIST (installed_list), row, item_data);
	}

	gedit_plugin_manager_lists_thaw();
}

static void
gedit_plugin_manager_item_remove (GtkWidget *widget, gpointer data)
{
	gint n;
	gint row;
	gint selection_length;
	gchar *name;
	gchar *name_array[1];
	gpointer item_data;

	selection_length = g_list_length (GTK_CLIST (installed_list)->selection);

	gedit_plugin_manager_lists_freeze();

	for (n=0; n < selection_length; n++)
	{
		row = (gint) g_list_nth_data (GTK_CLIST (installed_list)->selection, n);
		gtk_clist_get_text (GTK_CLIST (installed_list), row, 0, &name);
		item_data = gtk_clist_get_row_data (GTK_CLIST(installed_list), row);

		name_array[0] = g_strdup(name);
		gtk_clist_remove (GTK_CLIST (installed_list), row);
		row = gtk_clist_append (GTK_CLIST (available_list), name_array);
		gtk_clist_set_row_data (GTK_CLIST (available_list), row, item_data);
	}

	gedit_plugin_manager_lists_thaw();
}

static void
gedit_plugin_manager_item_add_all (GtkWidget *widget, gpointer data)
{
	gint row;
	gint row_inserted;
	gint rows;
	gchar *name;
	gchar *name_array[1];
	gpointer item_data;

	rows = GTK_CLIST (available_list)->rows;

	gedit_plugin_manager_lists_freeze();

	for (row=rows-1; row >= 0; row--)
	{
		gtk_clist_get_text (GTK_CLIST (available_list), row, 0, &name);
		item_data = gtk_clist_get_row_data (GTK_CLIST(available_list), row);

		name_array[0] = g_strdup(name);
		
		gtk_clist_remove (GTK_CLIST (available_list), row);
		row_inserted = gtk_clist_append (GTK_CLIST (installed_list), name_array);
		gtk_clist_set_row_data (GTK_CLIST (installed_list), row_inserted, item_data);
	}
	
	gedit_plugin_manager_lists_thaw();
}

static void
gedit_plugin_manager_item_remove_all (GtkWidget *widget, gpointer data)
{
	gint row;
	gint row_inserted;
	gint rows;
	gchar *name;
	gchar *name_array[1];
	gpointer item_data;

	rows = GTK_CLIST (installed_list)->rows;

	gedit_plugin_manager_lists_freeze();

	for (row=rows-1; row >= 0; row--)
	{
		gtk_clist_get_text (GTK_CLIST (installed_list), row, 0, &name);
		item_data = gtk_clist_get_row_data (GTK_CLIST(installed_list), row);

		name_array[0] = g_strdup(name);

		gtk_clist_remove (GTK_CLIST (installed_list), row);
		
		row_inserted = gtk_clist_append (GTK_CLIST (available_list), name_array);
		gtk_clist_set_row_data (GTK_CLIST (available_list), row_inserted, item_data);
	}

	gedit_plugin_manager_lists_thaw();
}

static void
gedit_plugin_manager_destroy (GtkWidget *widget, gpointer data)
{
	gnome_dialog_close (GNOME_DIALOG (widget));
}

/*
static gboolean
gtk_clist_row_selected (GtkCList *clist, gint row)
{
	gint selection_length;
	gint n;
	 
	selection_length = g_list_length (clist->selection);

	for (n=0; n < selection_length; n++)
		if ((gint) g_list_nth_data (clist->selection, n) == row)
			return TRUE;
	return FALSE;
}
*/

static void
gedit_plugin_manager_item_clicked (GtkCList *clist, GdkEventButton *event, gpointer data)
{
	gint row;
	gint column;
	PluginData *plugin;
	gint dummy_pos;
	gchar *plugin_info;
	
	if (event->button != 1)
		return;

	/* if this is a double click, do nothing */
	if (event->type != GDK_BUTTON_PRESS)
		return;

	if (!gtk_clist_get_selection_info (clist, event->x, event->y, &row, &column))
		return;

	text_box = GTK_EDITABLE(data);
	
	gedit_plugin_manager_info_clear();
	
	plugin = gtk_clist_get_row_data (clist, row);
	plugin_info  = g_strdup_printf (_("Plugin Name : %s\n"
					  "Author : %s\n"
					  "Description : %s\n"),
					plugin->name,
					plugin->author,
					plugin->desc);
	text_length = strlen (plugin_info);
	dummy_pos = 0;
	gtk_editable_insert_text (text_box, plugin_info, text_length, &dummy_pos);
	g_free (plugin_info);
}

static void
gedit_plugin_manager_clicked (GtkWidget *widget, gpointer button)
{
	gint n;
	gint row;
	gint rows;
	GnomeApp *nth_app;
	PluginData *plugin_data;

	if (button != 0)
	{
		gnome_dialog_close (GNOME_DIALOG(widget));
		return;
	}

	rows = GTK_CLIST (available_list)->rows;
	for (row=rows-1; row >= 0; row--)
	{
		plugin_data = gtk_clist_get_row_data (GTK_CLIST(available_list), row);
		plugin_data->installed = FALSE;
	}

	rows = GTK_CLIST (installed_list)->rows;
	for (row=rows-1; row >= 0; row--)
	{
		plugin_data = gtk_clist_get_row_data (GTK_CLIST(installed_list), row);
		plugin_data->installed = TRUE;
	}

	gnome_dialog_close (GNOME_DIALOG(widget));

	for (n=0; n<g_list_length (mdi->windows); n++)
	{
		nth_app = g_list_nth_data (mdi->windows, n);
		gedit_plugins_menu_add (nth_app);
	}
	
	return;

}

void
gedit_plugin_manager_create (GtkWidget *widget, gpointer data)
{
	GladeXML *gui;
	GtkWidget *dialog;
	GtkWidget *add_button;
	GtkWidget *remove_button;
	GtkWidget *add_all_button;
	GtkWidget *remove_all_button;
	GtkWidget *plugin_info_text;

	if (!g_file_exists (GEDIT_GLADEDIR GEDIT_PLUGIN_MANAGER_GLADE_FILE))
	{
		g_warning ("Could not find %s", GEDIT_GLADEDIR GEDIT_PLUGIN_MANAGER_GLADE_FILE);
		return;
	}

	gui = glade_xml_new (GEDIT_GLADEDIR GEDIT_PLUGIN_MANAGER_GLADE_FILE, NULL);

	if (!gui)
	{
		g_warning ("Could not find %s", GEDIT_GLADEDIR GEDIT_PLUGIN_MANAGER_GLADE_FILE);
		return;
	}

	dialog = glade_xml_get_widget (gui, "dialog");
	add_button        = glade_xml_get_widget (gui, "add_button");
	remove_button     = glade_xml_get_widget (gui, "remove_button");
	add_all_button    = glade_xml_get_widget (gui, "add_all_button");
	remove_all_button = glade_xml_get_widget (gui, "remove_all_button");
	available_list    = glade_xml_get_widget (gui, "available_list");
	installed_list    = glade_xml_get_widget (gui, "installed_list");
	plugin_info_text  = glade_xml_get_widget (gui, "plugin_info_text");

	g_return_if_fail (dialog != NULL);
	g_return_if_fail (add_button != NULL);
	g_return_if_fail (remove_button != NULL);
	g_return_if_fail (add_all_button != NULL);
	g_return_if_fail (remove_all_button != NULL);
	g_return_if_fail (available_list != NULL);
	g_return_if_fail (installed_list != NULL);
	g_return_if_fail (plugin_info_text != NULL);

	/* connect the signals */
	gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
			    GTK_SIGNAL_FUNC (gedit_plugin_manager_destroy), NULL);
	gtk_signal_connect (GTK_OBJECT (dialog), "clicked",
			    GTK_SIGNAL_FUNC (gedit_plugin_manager_clicked), NULL);
	
	/* now the buttons */
	gtk_signal_connect (GTK_OBJECT (add_button), "clicked",
			    GTK_SIGNAL_FUNC (gedit_plugin_manager_item_add), NULL);
	gtk_signal_connect (GTK_OBJECT (remove_button), "clicked",
			    GTK_SIGNAL_FUNC (gedit_plugin_manager_item_remove), NULL);
	gtk_signal_connect (GTK_OBJECT (add_all_button), "clicked",
			    GTK_SIGNAL_FUNC (gedit_plugin_manager_item_add_all), NULL);
	gtk_signal_connect (GTK_OBJECT (remove_all_button), "clicked",
			    GTK_SIGNAL_FUNC (gedit_plugin_manager_item_remove_all), NULL);
	gtk_signal_connect (GTK_OBJECT (available_list), "button_press_event",
			    GTK_SIGNAL_FUNC (gedit_plugin_manager_item_clicked),
			    plugin_info_text);
	gtk_signal_connect (GTK_OBJECT (installed_list), "button_press_event",
			    GTK_SIGNAL_FUNC (gedit_plugin_manager_item_clicked),
			    plugin_info_text);
	gedit_plugin_manager_item_load ();
	
	/* Set the dialog parent and modal type */
	gnome_dialog_set_parent (GNOME_DIALOG (dialog),
				 gedit_window_active());
	gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);

	/* Show everything and free */
	gtk_widget_show_all (dialog);
	gtk_object_unref (GTK_OBJECT (gui));
}
