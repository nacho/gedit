/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * shell_output.c
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

#include <glade/glade-xml.h>
#include <libgnome/gnome-i18n.h>
#include <libgnome/gnome-help.h>

#include <string.h>

#include <gedit-plugin.h>
#include <gedit-debug.h>
#include <gedit-utils.h>
#include <gedit-menus.h>

#define MENU_ITEM_LABEL		N_("Insert Shell _Output")
#define MENU_ITEM_PATH		"/menu/Edit/EditOps_4/"
#define MENU_ITEM_NAME		"PluginShellOutput"	
#define MENU_ITEM_TIP		N_("Insert the shell output in the current document")

#define SHELL_OUTPUT_LOGO "/shell-output-logo.png"

typedef struct _ShellOutputDialog ShellOutputDialog;

struct _ShellOutputDialog {
	GtkWidget *dialog;

	GtkWidget *logo;
	GtkWidget *command;
	GtkWidget *directory;
};

static void dialog_destroyed (GtkObject *obj,  void **dialog_pointer);
static ShellOutputDialog *get_dialog ();
static void dialog_response_handler (GtkDialog *dlg, gint res_id,  ShellOutputDialog *dialog);

static void	run_command_cb (BonoboUIComponent *uic, gpointer user_data, 
			       const gchar* verbname);
static gboolean	run_command_real (ShellOutputDialog *dialog);

G_MODULE_EXPORT GeditPluginState update_ui (GeditPlugin *plugin, BonoboWindow *window);
G_MODULE_EXPORT GeditPluginState destroy (GeditPlugin *pd);
G_MODULE_EXPORT GeditPluginState activate (GeditPlugin *pd);
G_MODULE_EXPORT GeditPluginState deactivate (GeditPlugin *pd);
G_MODULE_EXPORT GeditPluginState init (GeditPlugin *pd);

gchar *current_directory = NULL;

static void
dialog_destroyed (GtkObject *obj,  void **dialog_pointer)
{
	gedit_debug (DEBUG_PLUGINS, "");

	if (dialog_pointer != NULL)
	{
		g_free (*dialog_pointer);
		*dialog_pointer = NULL;
	}	
}

static void
dialog_response_handler (GtkDialog *dlg, gint res_id,  ShellOutputDialog *dialog)
{
	GError *error = NULL;
	
	gedit_debug (DEBUG_PLUGINS, "");

	switch (res_id) {
		case GTK_RESPONSE_OK:
			if (run_command_real (dialog))
				gtk_widget_destroy (dialog->dialog);
			break;
			
		case GTK_RESPONSE_HELP:
			/* FIXME: choose a better link id */
			gnome_help_display ("gedit.xml", "gedit-pipe-output", &error);
	
			if (error != NULL)
			{
				g_warning (error->message);

				g_error_free (error);
			}

			break;
	
		default:
			gtk_widget_destroy (dialog->dialog);
	}
}


static ShellOutputDialog*
get_dialog ()
{
	static ShellOutputDialog *dialog = NULL;

	GladeXML *gui;
	GtkWindow *window;
	GtkWidget *content;

	gedit_debug (DEBUG_PLUGINS, "");

	window = GTK_WINDOW (gedit_get_active_window ());

	if (dialog != NULL)
	{
		gdk_window_show (dialog->dialog->window);
		gdk_window_raise (dialog->dialog->window);
		gtk_widget_grab_focus (dialog->dialog);

		gtk_window_set_transient_for (GTK_WINDOW (dialog->dialog),
				window);
	
		gtk_widget_grab_focus (dialog->command);

		if (!GTK_WIDGET_VISIBLE (dialog->dialog))
			gtk_widget_show (dialog->dialog);

		return dialog;
	}

	gui = glade_xml_new (GEDIT_GLADEDIR "shell_output.glade2",
			     "shell_output_dialog_content", NULL);

	if (!gui) {
		g_warning
		    ("Could not find shell_output.glade2, reinstall gedit.\n");
		return NULL;
	}

	dialog = g_new0 (ShellOutputDialog, 1);

	dialog->dialog = gtk_dialog_new_with_buttons (_("Shell output"),
						      window,
						      GTK_DIALOG_DESTROY_WITH_PARENT,
						      GTK_STOCK_CLOSE,
						      GTK_RESPONSE_CLOSE,
						      GTK_STOCK_HELP,
						      GTK_RESPONSE_HELP,
						      NULL);

	g_return_val_if_fail (dialog->dialog != NULL, NULL);

	/* Add the run button */
	gedit_dialog_add_button (GTK_DIALOG (dialog->dialog), 
				 _("_Run"), GTK_STOCK_EXECUTE, GTK_RESPONSE_OK);

	content			= glade_xml_get_widget (gui, "shell_output_dialog_content");

	g_return_val_if_fail (content != NULL, NULL);

	dialog->logo		= glade_xml_get_widget (gui, "logo");
 
	dialog->command    	= glade_xml_get_widget (gui, "command_entry");
	dialog->directory  	= glade_xml_get_widget (gui, "directory_entry");

	g_return_val_if_fail (dialog->logo      != NULL, NULL);
	g_return_val_if_fail (dialog->command   != NULL, NULL);
	g_return_val_if_fail (dialog->directory != NULL, NULL);

	/* stick the shell_output logo in there */
	gtk_image_set_from_file (GTK_IMAGE (dialog->logo), GNOME_ICONDIR SHELL_OUTPUT_LOGO);

	gtk_entry_set_text (GTK_ENTRY (dialog->directory), current_directory);

	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog->dialog)->vbox),
			    content, FALSE, FALSE, 0);

	gtk_dialog_set_default_response (GTK_DIALOG (dialog->dialog),
					 GTK_RESPONSE_OK);

	g_signal_connect (G_OBJECT (dialog->dialog), "destroy",
			   G_CALLBACK (dialog_destroyed), &dialog);

	g_signal_connect (G_OBJECT (dialog->dialog), "response",
			   G_CALLBACK (dialog_response_handler), dialog);

	g_object_unref (gui);

	gtk_window_set_resizable (GTK_WINDOW (dialog->dialog), FALSE);

	gtk_widget_grab_focus (dialog->command);

	gtk_widget_show (dialog->dialog);

	return dialog;

}

static void
run_command_cb (BonoboUIComponent *uic, gpointer user_data, const gchar* verbname)
{
	ShellOutputDialog *dialog;

	gedit_debug (DEBUG_PLUGINS, "");

	dialog = get_dialog ();
	if (dialog == NULL) {
		g_warning ("Could not create the Shell Output dialog");
		return;
	}
}

static gboolean
run_command_real (ShellOutputDialog *dialog)
{
	const gchar    *command_string   = NULL ;
	const gchar    *directory_string = NULL ;
	gboolean  retval;
	gchar 	**argv = 0;
	gchar    *standard_output = NULL;
	GeditDocument	*doc;
	
	gedit_debug (DEBUG_PLUGINS, "");

	g_return_val_if_fail (dialog != NULL, FALSE);

	doc = gedit_get_active_document ();

	if (doc == NULL)
		return TRUE;

	command_string = gtk_entry_get_text (GTK_ENTRY (dialog->command));

	if (command_string == NULL || (strlen (command_string) == 0))
	{
		GtkWidget *err_dialog;
		
		err_dialog = gtk_message_dialog_new (
				GTK_WINDOW (dialog->dialog),
				GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			   	GTK_MESSAGE_ERROR,
			   	GTK_BUTTONS_OK,
				_("The shell command entry is empty.\n\n"
				  "Please, insert a valid shell command."));
			
		gtk_dialog_set_default_response (GTK_DIALOG (err_dialog), GTK_RESPONSE_OK);

		gtk_window_set_resizable (GTK_WINDOW (err_dialog), FALSE);

		gtk_dialog_run (GTK_DIALOG (err_dialog));
  		gtk_widget_destroy (err_dialog);

		return FALSE;
	}

	directory_string = gtk_entry_get_text (GTK_ENTRY (dialog->directory));
	
	if (directory_string == NULL || (strlen (directory_string) == 0)) 
		directory_string = current_directory;
	
	if (!g_shell_parse_argv (command_string,
                           NULL, &argv,
                           NULL))
	{
		GtkWidget *err_dialog;
		
		err_dialog = gtk_message_dialog_new (
				GTK_WINDOW (dialog->dialog),
				GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			   	GTK_MESSAGE_ERROR,
			   	GTK_BUTTONS_OK,
				_("Error parsing the shell command.\n\n"
				  "Please, insert a valid shell command."));
			
		gtk_dialog_set_default_response (GTK_DIALOG (err_dialog), GTK_RESPONSE_OK);

		gtk_window_set_resizable (GTK_WINDOW (err_dialog), FALSE);

		gtk_dialog_run (GTK_DIALOG (err_dialog));
  		gtk_widget_destroy (err_dialog);

		return FALSE;
	}

		
	retval = g_spawn_sync (directory_string,
                         argv,
                         NULL,
                         G_SPAWN_SEARCH_PATH,
                         NULL,
                         NULL,
                         &standard_output,
                         NULL,
                         NULL,
                         NULL);
	g_strfreev (argv);

	if (retval)
	{
		gedit_document_begin_user_action (doc);

		gedit_document_insert_text_at_cursor (doc, standard_output, -1);

		gedit_document_end_user_action (doc);

		g_free (standard_output);

		if (directory_string != current_directory)
		{
			g_free (current_directory);
			current_directory = g_strdup (directory_string);
		}

		return TRUE;
	}
	else
	{
		GtkWidget *err_dialog;
		
		err_dialog = gtk_message_dialog_new (
				GTK_WINDOW (dialog->dialog),
				GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			   	GTK_MESSAGE_ERROR,
			   	GTK_BUTTONS_OK,
				_("An error occurs while running the selected command."));
			
		gtk_dialog_set_default_response (GTK_DIALOG (err_dialog), GTK_RESPONSE_OK);

		gtk_window_set_resizable (GTK_WINDOW (err_dialog), FALSE);

		gtk_dialog_run (GTK_DIALOG (err_dialog));
  		gtk_widget_destroy (err_dialog);

		return FALSE;
	}
}

G_MODULE_EXPORT GeditPluginState
destroy (GeditPlugin *plugin)
{
	gedit_debug (DEBUG_PLUGINS, "");

	plugin->deactivate (plugin);

	g_free (current_directory);

	return PLUGIN_OK;
}
	
G_MODULE_EXPORT GeditPluginState
update_ui (GeditPlugin *plugin, BonoboWindow *window)
{
	BonoboUIComponent *uic;
	GeditDocument *doc;
	
	gedit_debug (DEBUG_PLUGINS, "");	
	g_return_val_if_fail (window != NULL, PLUGIN_ERROR);

	uic = gedit_get_ui_component_from_window (window);

	doc = gedit_get_active_document ();

	if ((doc == NULL) || gedit_document_is_readonly (doc))		
		gedit_menus_set_verb_sensitive (uic, "/commands/" MENU_ITEM_NAME, FALSE);
	else
		gedit_menus_set_verb_sensitive (uic, "/commands/" MENU_ITEM_NAME, TRUE);

	return PLUGIN_OK;
}
	
G_MODULE_EXPORT GeditPluginState
activate (GeditPlugin *pd)
{
	GList *top_windows;
        gedit_debug (DEBUG_PLUGINS, "");

        top_windows = gedit_get_top_windows ();
        g_return_val_if_fail (top_windows != NULL, PLUGIN_ERROR);

        while (top_windows)
        {
		gedit_menus_add_menu_item (BONOBO_WINDOW (top_windows->data),
				     MENU_ITEM_PATH, MENU_ITEM_NAME,
				     MENU_ITEM_LABEL, MENU_ITEM_TIP,
				     GTK_STOCK_EXECUTE, run_command_cb);

                pd->update_ui (pd, BONOBO_WINDOW (top_windows->data));

                top_windows = g_list_next (top_windows);
        }

        return PLUGIN_OK;
}

G_MODULE_EXPORT GeditPluginState
deactivate (GeditPlugin *pd)
{
	gedit_debug (DEBUG_PLUGINS, "");
	
	gedit_menus_remove_menu_item_all (MENU_ITEM_PATH, MENU_ITEM_NAME);

	return PLUGIN_OK;
}

G_MODULE_EXPORT GeditPluginState
init (GeditPlugin *pd)
{
	/* initialize */
	gedit_debug (DEBUG_PLUGINS, "");
     
	pd->name = _("Shell output");
	pd->desc = _("Execute a program and insert its output in the current document at the current "
		     "cursor position");
	pd->author = "Paolo Maggi <maggi@athena.polito.it>";
	pd->copyright = _("Copyright (C) 2002 - Paolo Maggi");
	
	pd->private_data = NULL;

	current_directory = g_get_current_dir ();
		
	return PLUGIN_OK;
}




