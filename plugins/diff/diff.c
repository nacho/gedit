/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* diff.c - diff plugin.
 *
 * Copyright (C) 2000 Chema Celorio
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <config.h>
#include <gnome.h>
#include <glade/glade.h>

#include "window.h"
#include "document.h"
#include "plugin.h"
#include "view.h"
#include "utils.h"

#define GEDIT_PLUGIN_PROGRAM "diff"
/* xgettext translators : !!!!!!!!!!!---------> the name of the plugin only.
   it is used to display "you can not use the [name] plugin without this program... */
#define GEDIT_PLUGIN_NAME  _("diff")
#define GEDIT_PLUGIN_GLADE_FILE "/diff.glade"

static GtkWidget *from_document_1;
static GtkWidget *from_document_2;
static GtkWidget *document_list_1;
static GtkWidget *document_list_2;
static GtkWidget *file_entry_1;
static GtkWidget *file_entry_2;

static gint document_selected_1;
static gint document_selected_2;

static void
gedit_plugin_destroy (PluginData *pd)
{
	g_free (pd->name);
}

static void
gedit_plugin_finish (GtkWidget *widget, gpointer data)
{
	gnome_dialog_close (GNOME_DIALOG(widget));
}

static void
gedit_plugin_execute (GtkWidget *widget, gint button, gpointer data)
{
	gint state_1;
	gint state_2;
	gchar * file_name_1;
	gchar * file_name_2;
	Document *document;

	int pid;
	char buff[1025];
	guint length, pos; 
	Document *doc;
	int fdpipe[2];
	gchar * program_location;

	GtkLabel *label;


	label  = gtk_object_get_data (GTK_OBJECT (widget), "location_label");
	g_return_if_fail (label!=NULL);

	program_location = g_strdup (GTK_LABEL(label)->label);
	
	if (button != 0)
	{
		gnome_dialog_close (GNOME_DIALOG (widget));
		return;
	}
	
	state_1 = gtk_radio_group_get_selected (GTK_RADIO_BUTTON(from_document_1)->group);
	state_2 = gtk_radio_group_get_selected (GTK_RADIO_BUTTON(from_document_2)->group);

	file_name_1 = gnome_file_entry_get_full_path (GNOME_FILE_ENTRY(file_entry_1), FALSE);
	file_name_2 = gnome_file_entry_get_full_path (GNOME_FILE_ENTRY(file_entry_2), FALSE);

	/* We need to :
	   - if !state_1 & !state_2. Verify that the doc numbers are
	   not the same. If they are display an err msg and return;
	   - if state_x verify that the file exists and that we can read from
	   it. If not, display err messg and return;
	   -  if !state_? get the buffer with g_list_nth (mdi->child) and
	   copy the buffer to a temp file in /temp. Verify that we have a temp
	   place to copy the buffer to.
	   - compose the diff command from the files or the temp files
	   - execute the command and create a new document.
	*/
	if (!state_1 && !state_2 && (document_selected_1 == document_selected_2))
	{
		gedit_utils_error_dialog (_("The two documents selected are the same"), widget);
		return;
	}

	if ((state_1 && ((file_name_1 == NULL ) || (!g_file_exists (file_name_1)))) ||
	    (state_2 && ((file_name_2 == NULL ) || (!g_file_exists (file_name_2)))) )
	{
		gedit_utils_error_dialog (_("The file selected does not exists\n\n"
					    "Please provide a valid file"), widget);
		return;
	}

	if (!state_1)
	{
		g_free (file_name_1);
		document = (Document *)g_list_nth_data (mdi->children, document_selected_1);
		if (gedit_document_get_buffer_length (document)<1)
		{
			gedit_utils_error_dialog (_("The document contains no text"), widget);
			return;
		}
		file_name_1 = gedit_utils_create_temp_from_doc (document, 1);

	}
	if (!state_2)
	{
		g_free (file_name_2);
		document = (Document *)g_list_nth_data (mdi->children, document_selected_2);
		if (gedit_document_get_buffer_length (document)<1)
		{
			gedit_utils_error_dialog (_("The document contains no text"), widget);
			return;
		}
		file_name_2 = gedit_utils_create_temp_from_doc (document, 2);
	}

	if (file_name_1 == NULL || file_name_2 == NULL)
	{
		/* FIXME: do better error reporting ... . Chema */
		gedit_utils_error_dialog (_("gedit could not create a temp file.\n\n"), widget);
		return;
	}
	
	document = NULL;

	if (pipe (fdpipe) == -1)
	{
		_exit (1);
	}
  
	pid = fork();
	if (pid == 0)
	{
		/* New process. */
		char *argv[5];

		close (1);
		dup (fdpipe[1]);
		close (fdpipe[0]);
		close (fdpipe[1]);
      
		argv[0] = "diff";
		argv[1] = "-u";
		argv[2] = file_name_1;
		argv[3] = file_name_2;
		argv[4] = NULL;
		execv (program_location, argv);
		/* This is only reached if something goes wrong. */
		_exit (1);
	}
	close (fdpipe[1]);

	doc = gedit_document_new_with_title ("diff");

	length = 1;
	pos = 0;
	while (length > 0)
	{
		buff [ length = read (fdpipe[0], buff, 1024) ] = 0;
		if (length > 0)
		{
		     	gedit_document_insert_text (doc, buff, pos, FALSE);
			pos += length;
		}
	}

	gedit_view_set_position (gedit_view_active (), 0);

	if (!state_1)
		gedit_utils_delete_temp (file_name_1);
	if (!state_2)
		gedit_utils_delete_temp (file_name_2);

	g_free (file_name_1);
	g_free (file_name_2);

	gnome_dialog_close (GNOME_DIALOG (widget));
}

static void
gedit_plugin_change_location (GtkWidget *button, gpointer userdata)
{
	GtkWidget *dialog;
	GtkWidget *label;
	gchar * new_location;

	gedit_debug (DEBUG_PLUGINS, "");
	dialog = userdata;

	new_location = gedit_plugin_program_location_change (GEDIT_PLUGIN_PROGRAM,
							     GEDIT_PLUGIN_NAME);
	if ( new_location == NULL)
	{
		gdk_window_raise (dialog->window);
		return;
	}

	/* We need to update the label */
	label  = gtk_object_get_data (GTK_OBJECT (dialog), "location_label");
	g_return_if_fail (label!=NULL);
	gtk_label_set_text (GTK_LABEL (label),
			    new_location);
	g_free (new_location);

	gdk_window_raise (dialog->window);	

}

static void
gedit_plugin_diff_update_document (GtkWidget *widget, gpointer data)
{
	gint item = GPOINTER_TO_INT (data);

	if (item>999)
		document_selected_2 = item-1000;
	else
		document_selected_1 = item;
}
static void
gedit_plugin_diff_load_documents (GtkWidget ** options_menu, gint second_menu)
{
	GtkWidget *menu;
	GtkWidget *menu_item;
	gint n;
	Document *nth_doc;
	gchar * document_name;
	
	menu = gtk_menu_new();
     
	for (n = 0; n < g_list_length (mdi->children); n++)
	{
		nth_doc = (Document *)g_list_nth_data (mdi->children, n);
		if (nth_doc->filename == NULL)
			document_name = g_strdup_printf ("%s %d", _("Untitled"), nth_doc->untitled_number);
		else
			document_name = g_strdup (g_basename(nth_doc->filename));
		
		menu_item = gtk_menu_item_new_with_label (document_name);
		gtk_signal_connect (GTK_OBJECT (menu_item), "activate",
				    GTK_SIGNAL_FUNC (gedit_plugin_diff_update_document),
				    GINT_TO_POINTER (n+(second_menu?1000:0)));
		gtk_menu_append (GTK_MENU (menu), menu_item);
	}
     
	gtk_option_menu_set_menu (GTK_OPTION_MENU(*options_menu), menu);

}

static void
gedit_plugin_diff_document_selected (GtkWidget *widget, GdkEvent *event, gpointer data)
{
	GtkRadioButton *radio_button = data;

	g_return_if_fail (radio_button!=NULL);
	
	gtk_radio_button_select (radio_button->group, 0);
}

static void
gedit_plugin_diff_file_selected (GtkWidget *widget, gpointer data)
{
	GtkRadioButton *radio_button = data;

	g_return_if_fail (radio_button!=NULL);
	
	gtk_radio_button_select (radio_button->group, 1);
}

static void
gedit_plugin_diff_file_selected_event (GtkWidget *widget, GdkEvent *event, gpointer data)
{
	gedit_plugin_diff_file_selected (widget, data);
}

static void
gedit_plugin_create_dialog (void)
{
	GladeXML *gui;
	GtkWidget *dialog;
	GtkWidget *change_button;
	GtkWidget *location_label;

	GtkWidget *file_selector_combo_1;
	GtkWidget *file_selector_combo_2;
	
	gchar * program_location;

	program_location = gedit_plugin_program_location_get (GEDIT_PLUGIN_PROGRAM,
							      GEDIT_PLUGIN_NAME,
							      FALSE);
	if (program_location == NULL)
		return;

	if (!g_file_exists (GEDIT_GLADEDIR GEDIT_PLUGIN_GLADE_FILE))
	{
		g_warning ("Could not find %s",
			   GEDIT_GLADEDIR
			   GEDIT_PLUGIN_GLADE_FILE);
		return;
	}

	gui = glade_xml_new (GEDIT_GLADEDIR
			     GEDIT_PLUGIN_GLADE_FILE,
			     NULL);

	if (!gui)
	{
		g_warning ("Could not find %s",
			   GEDIT_GLADEDIR
			   GEDIT_PLUGIN_GLADE_FILE);
		return;
	}

	dialog          = glade_xml_get_widget (gui, "dialog");

	from_document_1 = glade_xml_get_widget (gui, "from_document_1");
	document_list_1 = glade_xml_get_widget (gui, "document_list_1");
	file_entry_1    = glade_xml_get_widget (gui, "file_entry_1");
	file_selector_combo_1 = glade_xml_get_widget (gui, "file_selector_combo_1");

	from_document_2 = glade_xml_get_widget (gui, "from_document_2");
	document_list_2 = glade_xml_get_widget (gui, "document_list_2");
	file_entry_2    = glade_xml_get_widget (gui, "file_entry_2");
	file_selector_combo_2 = glade_xml_get_widget (gui, "file_selector_combo_2");

	location_label  = glade_xml_get_widget (gui, "location_label");
	change_button   = glade_xml_get_widget (gui, "change_button");

	g_return_if_fail (dialog          != NULL);
	g_return_if_fail (from_document_1 != NULL);
	g_return_if_fail (document_list_1 != NULL);
	g_return_if_fail (file_entry_1    != NULL);
	g_return_if_fail (from_document_2 != NULL);
	g_return_if_fail (document_list_2 != NULL);
	g_return_if_fail (file_entry_2    != NULL);
	g_return_if_fail (location_label  != NULL);
	g_return_if_fail (change_button   != NULL);

        /* Set the location label */
	gtk_object_set_data (GTK_OBJECT (dialog), "location_label", location_label);
	gtk_label_set_text (GTK_LABEL (glade_xml_get_widget (gui, "location_label")),
			    program_location);
	
        /* Connect the signals */
	gtk_signal_connect (GTK_OBJECT (dialog), "clicked",
			    GTK_SIGNAL_FUNC (gedit_plugin_execute), NULL);
	gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
			    GTK_SIGNAL_FUNC (gedit_plugin_finish), NULL);
	gtk_signal_connect (GTK_OBJECT (change_button), "clicked",
			    GTK_SIGNAL_FUNC (gedit_plugin_change_location), dialog);

	/* Load and connect the document list */
	gtk_signal_connect (GTK_OBJECT (document_list_1), "button_press_event",
			    GTK_SIGNAL_FUNC (gedit_plugin_diff_document_selected),
			    from_document_1);
	gtk_signal_connect (GTK_OBJECT (document_list_2), "button_press_event",
			    GTK_SIGNAL_FUNC (gedit_plugin_diff_document_selected),
			    from_document_2);
	
	document_selected_1 = 0;
	document_selected_2 = 0;
 	gedit_plugin_diff_load_documents (&document_list_1, FALSE);
 	gedit_plugin_diff_load_documents (&document_list_2, TRUE);

	gtk_signal_connect (GTK_OBJECT (file_entry_1), "browse_clicked",
			    GTK_SIGNAL_FUNC (gedit_plugin_diff_file_selected),
			    from_document_1);
	gtk_signal_connect (GTK_OBJECT (file_selector_combo_1), "focus_in_event",
			    GTK_SIGNAL_FUNC (gedit_plugin_diff_file_selected_event),
			    from_document_1);
	gtk_signal_connect (GTK_OBJECT (file_entry_2), "browse_clicked",
			    GTK_SIGNAL_FUNC (gedit_plugin_diff_file_selected),
			    from_document_2);
	gtk_signal_connect (GTK_OBJECT (file_selector_combo_2), "focus_in_event",
			    GTK_SIGNAL_FUNC (gedit_plugin_diff_file_selected_event),
			    from_document_2);

	/* Set the dialog parent and modal type */ 
	gnome_dialog_set_parent (GNOME_DIALOG (dialog),
				 gedit_window_active());
	gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);

	/* Show everything then free the GladeXML memmory */
	gtk_widget_show_all (dialog);
	gtk_object_unref (GTK_OBJECT (gui));
}

gint
init_plugin (PluginData *pd)
{
	/* initialize */
	pd->destroy_plugin = gedit_plugin_destroy;
	pd->name = _("Diff");
	pd->desc = _("Makes a diff file from 2 documents or files on disk\n"
		     "For more info on diff, type \"man diff\" in a shell prompt\n"
		     "\n\n\n\n\nAsta la vista.\n");
	pd->author = "Chema Celorio";
	pd->needs_a_document = FALSE;

	pd->private_data = (gpointer)gedit_plugin_create_dialog;
	pd->installed_by_default = TRUE;
	
	return PLUGIN_OK;
}
