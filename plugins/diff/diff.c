/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * diff.c
 * This file is part of gedit
 *
 * Copyright (C) 2000 Chema Celorio
 * Copyright (C) 2001 Chema Celorio and Paolo Maggi
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
 * Modified by the gedit Team, 2000-2002. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glade/glade-xml.h>
#include <libgnome/gnome-i18n.h>
#include <libgnomeui/gnome-file-entry.h>
#include <gconf/gconf-client.h>

#include <unistd.h> /* getpid and unlink */
#include <stdlib.h> /* rand   */
#include <time.h>   /* time   */
#include <string.h> /* strcmp   */

#include <sys/types.h>
#include <sys/wait.h>

#include <gedit-menus.h>
#include <gedit-plugin.h>
#include <gedit-utils.h>
#include <gedit-debug.h>
#include <gedit-file.h>
#include <dialogs/gedit-dialogs.h>

#define DIFF_BASE_KEY 		"/apps/gedit2/plugins/diff"
#define DIFF_LOCATION_KEY	"/diff-program-location"
#define UNIFIED_FORMAT_KEY	"/use-unified-format"
#define IGNORE_BLANKS		"/ignore-blanks"

#define MENU_ITEM_LABEL		N_("_Compare files...")
#define MENU_ITEM_PATH		"/menu/File/FileOps/"
#define MENU_ITEM_NAME		"Diff"	
#define MENU_ITEM_TIP		N_("Makes a diff file from two documents or files")

#define PLUGIN_NAME 		_("Compare files")
#define DIFF_PROGRAM_NAME	"diff"

typedef struct _DiffDialog DiffDialog;

struct _DiffDialog {
	GtkWidget *dialog;

	GtkWidget *from_document_1;
	GtkWidget *from_document_2;
	GtkWidget *document_list_1;
	GtkWidget *document_list_2;

	GtkWidget *from_file_1;
	GtkWidget *from_file_2;
	GtkWidget *file_entry_1;
	GtkWidget *file_entry_2;

	GtkWidget *unified_checkbutton;
	GtkWidget *ignore_blanks_checkbutton;

	GtkWidget *file_selector_combo_1;
	GtkWidget *file_selector_combo_2;

	gint document_selected_1;
	gint document_selected_2;

	GList *open_docs;
};


G_MODULE_EXPORT GeditPluginState update_ui (GeditPlugin *plugin, BonoboWindow *window);
G_MODULE_EXPORT GeditPluginState destroy (GeditPlugin *pd);
G_MODULE_EXPORT GeditPluginState activate (GeditPlugin *pd);
G_MODULE_EXPORT GeditPluginState deactivate (GeditPlugin *pd);
G_MODULE_EXPORT GeditPluginState init (GeditPlugin *pd);
G_MODULE_EXPORT GeditPluginState configure (GeditPlugin *p, GtkWidget *parent);
G_MODULE_EXPORT GeditPluginState save_settings (GeditPlugin *pd);

static void dialog_destroyed (GtkObject *obj,  void **dialog_pointer);
static void diff_file_selected (GtkWidget *widget, gpointer data);
static void diff_file_selected_event (GtkWidget *widget, GdkEvent *event, gpointer data);
static gboolean diff_document_selected (GtkWidget *widget, GdkEvent *event, gpointer data);
static void diff_load_documents (DiffDialog* dialog, GtkWidget **options_menu);

static void diff_cb (BonoboUIComponent *uic, gpointer user_data, const gchar* verbname);
static void diff_real (void);
static void error_dialog (const gchar* str, GtkWindow *parent);
static gboolean diff_execute (DiffDialog *dialog);
static gchar *temp_file_name_new (char* prefix);

static gboolean configure_real (GtkWindow *parent);

static gchar* diff_program_location = NULL;

static gboolean use_unified_format;
static gboolean ignore_blanks;

static GConfClient 	*diff_gconf_client 	= NULL;	

static void
dialog_destroyed (GtkObject *obj,  void **dialog_pointer)
{
	gedit_debug (DEBUG_PLUGINS, "");

	if (dialog_pointer != NULL)
	{
		if (((DiffDialog*)*dialog_pointer)->open_docs != NULL)
			g_list_free (((DiffDialog*)*dialog_pointer)->open_docs);
		
		g_free (*dialog_pointer);
		*dialog_pointer = NULL;
	}

	gedit_debug (DEBUG_PLUGINS, "END");	
}

static void
diff_file_selected (GtkWidget *widget, gpointer data)
{
	GtkToggleButton *toggle_button = data;
	g_return_if_fail (toggle_button!=NULL);

	gedit_debug (DEBUG_PLUGINS, "");

	gtk_toggle_button_set_active (toggle_button, TRUE);
}

static void
diff_file_selected_event (GtkWidget *widget, GdkEvent *event, gpointer data)
{
	gedit_debug (DEBUG_PLUGINS, "");

	diff_file_selected (widget, data);
}

static gboolean
diff_document_selected (GtkWidget *widget, GdkEvent *event, gpointer data)
{
	gedit_debug (DEBUG_PLUGINS, "");

	diff_file_selected (widget, data);

	return FALSE;
}

static void
diff_update_document (GtkWidget *widget, gpointer data)
{
	gint item;
	DiffDialog* dialog = (DiffDialog*)data;
	
	gedit_debug (DEBUG_PLUGINS, "");

	item = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (widget), "pos"));

	if (item >= 10000)
		dialog->document_selected_2 = item - 10000;
	else
		dialog->document_selected_1 = item;
}

static void
diff_load_documents (DiffDialog* dialog, GtkWidget **options_menu)
{
	GtkWidget *menu;
	GtkWidget *menu_item;
	gint n = 0;
	GeditDocument *nth_doc;
	gchar * document_name;
	GList* docs;
	
	gedit_debug (DEBUG_PLUGINS, "");

	menu = gtk_menu_new();
     
	docs = dialog->open_docs;
	
	while (docs != NULL)
	{
		nth_doc = GEDIT_DOCUMENT (docs->data);
		document_name = gedit_document_get_short_name (nth_doc);
		
		gedit_debug (DEBUG_PLUGINS, "Doc: %s", document_name);

		menu_item = gtk_menu_item_new_with_label (document_name);
	
		g_object_set_data (G_OBJECT (menu_item), "pos", 
			GINT_TO_POINTER (n + 
				         ((*options_menu == dialog->document_list_2) ? 10000 : 0)));
		
		g_signal_connect (G_OBJECT (menu_item), "activate",
				  G_CALLBACK (diff_update_document),
			    	  dialog);
		
		gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
		gtk_widget_show (menu_item);

		g_free (document_name);
		
		++n;

		docs = g_list_next (docs);
	}
     
	gtk_option_menu_set_menu (GTK_OPTION_MENU (*options_menu), menu);
}


static DiffDialog *
get_diff_dialog (GtkWindow* parent)
{
	static DiffDialog *dialog = NULL;

	GladeXML *gui;
	GtkWidget *content;

	gedit_debug (DEBUG_PLUGINS, "");

	if (dialog != NULL)
	{
		gtk_window_present (GTK_WINDOW (dialog->dialog));
		
		gtk_window_set_transient_for (GTK_WINDOW (dialog->dialog),
				parent);
		
		return dialog;
	}

	gui = glade_xml_new (GEDIT_GLADEDIR "diff.glade2",
			     "dialog_content", NULL);

	if (!gui) {
		g_warning
		    ("Could not find diff.glade2, reinstall gedit.\n");
		return NULL;
	}

	dialog = g_new0 (DiffDialog, 1);

	/* Create the dialog */
	dialog->dialog = gtk_dialog_new_with_buttons (_("Compare two files..."),
						      parent,
						      GTK_DIALOG_DESTROY_WITH_PARENT |
						      GTK_DIALOG_MODAL,
						      GTK_STOCK_CANCEL,
						      GTK_RESPONSE_CANCEL,
						      GTK_STOCK_HELP,
						      GTK_RESPONSE_HELP,
						      NULL);

	g_return_val_if_fail (dialog->dialog != NULL, NULL);

	/* Add the update compare */
	gedit_dialog_add_button (GTK_DIALOG (dialog->dialog), 
				 _("C_ompare"), GTK_STOCK_EXECUTE, GTK_RESPONSE_OK);

	/* Load widgets */
	content				= glade_xml_get_widget (gui, "dialog_content");
	dialog->from_document_1        	= glade_xml_get_widget (gui, "from_document_1");
	dialog->document_list_1        	= glade_xml_get_widget (gui, "document_list_1");
	
	dialog->from_file_1            	= glade_xml_get_widget (gui, "from_file_1");
	dialog->file_entry_1           	= glade_xml_get_widget (gui, "file_entry_1");
	dialog->file_selector_combo_1  	= glade_xml_get_widget (gui, "file_selector_combo_1");

	dialog->from_document_2        	= glade_xml_get_widget (gui, "from_document_2");
	dialog->document_list_2        	= glade_xml_get_widget (gui, "document_list_2");

	dialog->from_file_2            	= glade_xml_get_widget (gui, "from_file_2");
	dialog->file_entry_2           	= glade_xml_get_widget (gui, "file_entry_2");
	dialog->file_selector_combo_2  	= glade_xml_get_widget (gui, "file_selector_combo_2");

	dialog->unified_checkbutton    	= glade_xml_get_widget (gui, "unified_checkbutton");
	dialog->ignore_blanks_checkbutton = glade_xml_get_widget (gui, "blanks_checkbutton");
	
	g_return_val_if_fail (content != NULL, NULL);
	

	g_return_val_if_fail (dialog->from_document_1       != NULL, NULL);
	g_return_val_if_fail (dialog->document_list_1       != NULL, NULL);

	g_return_val_if_fail (dialog->from_file_1           != NULL, NULL);
	g_return_val_if_fail (dialog->file_entry_1          != NULL, NULL);
	g_return_val_if_fail (dialog->file_selector_combo_1 != NULL, NULL);

	g_return_val_if_fail (dialog->from_document_2       != NULL, NULL);
	g_return_val_if_fail (dialog->document_list_2       != NULL, NULL);

	g_return_val_if_fail (dialog->from_file_2           != NULL, NULL);
	g_return_val_if_fail (dialog->file_entry_2          != NULL, NULL);
	g_return_val_if_fail (dialog->file_selector_combo_2 != NULL, NULL);

	g_return_val_if_fail (dialog->unified_checkbutton 	!= NULL, NULL);
	g_return_val_if_fail (dialog->ignore_blanks_checkbutton != NULL, NULL);

	/* Insert the content in the dialog */
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog->dialog)->vbox),
			    content, FALSE, FALSE, 0);

	/* Set default response */
	gtk_dialog_set_default_response (GTK_DIALOG (dialog->dialog),
					 GTK_RESPONSE_OK);

	/* Set check buttons state */
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->unified_checkbutton), 
			              use_unified_format); 
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->ignore_blanks_checkbutton), 
				      ignore_blanks); 

	/* Load and connect the document list */	
	g_signal_connect (G_OBJECT (dialog->document_list_1), "button_press_event",
			  G_CALLBACK (diff_document_selected),
			  dialog->from_document_1);	
	g_signal_connect (G_OBJECT (dialog->document_list_2), "button_press_event",
			  G_CALLBACK (diff_document_selected),
			  dialog->from_document_2);
	
	dialog->document_selected_1 = 0;
	dialog->document_selected_2 = 0;

	dialog->open_docs = gedit_get_open_documents ();
	
	if ((dialog->open_docs == NULL) || (g_list_length (dialog->open_docs) == 0))
	{
		gtk_widget_set_sensitive (dialog->from_document_1, FALSE);
		gtk_widget_set_sensitive (dialog->from_document_2, FALSE);
		gtk_widget_set_sensitive (dialog->document_list_1, FALSE);
		gtk_widget_set_sensitive (dialog->document_list_2, FALSE);

		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->from_file_1), TRUE);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->from_file_2), TRUE);
	}
	else
	{
		diff_load_documents (dialog, &dialog->document_list_1);
 		diff_load_documents (dialog, &dialog->document_list_2);		
	}

	g_signal_connect (G_OBJECT (dialog->file_entry_1), "browse_clicked",
			  G_CALLBACK (diff_file_selected),
			  dialog->from_file_1);
	g_signal_connect (G_OBJECT (dialog->file_selector_combo_1), "focus_in_event",
			  G_CALLBACK (diff_file_selected_event),
			  dialog->from_file_1);
	g_signal_connect (G_OBJECT (dialog->file_entry_2), "browse_clicked",
			  G_CALLBACK (diff_file_selected),
			  dialog->from_file_2);
	g_signal_connect (G_OBJECT (dialog->file_selector_combo_2), "focus_in_event",
			  G_CALLBACK (diff_file_selected_event),
			  dialog->from_file_2);

	/* Connect destroy signal */
	g_signal_connect (G_OBJECT (dialog->dialog), "destroy",
			  G_CALLBACK (dialog_destroyed), &dialog);
	
	g_object_unref (gui);

	gtk_window_set_resizable (GTK_WINDOW (dialog->dialog), FALSE);

	return dialog;

}

static void
diff_cb (BonoboUIComponent *uic, gpointer user_data, const gchar* verbname)
{
	gedit_debug (DEBUG_PLUGINS, "");

	diff_real ();	
}


static void
diff_real (void)
{
	GtkWindow *parent;
	DiffDialog *dialog = NULL;
	gint ret;

	gedit_debug (DEBUG_PLUGINS, "");

	parent = GTK_WINDOW (gedit_get_active_window ());
	
	if (diff_program_location  == NULL)
		if (!configure_real (parent))
			return;	

	dialog = get_diff_dialog (parent);
	if (dialog == NULL) 
	{
		g_warning ("Could not create the Compare files dialog");
		return;
	}

	do 
	{
		ret = gtk_dialog_run (GTK_DIALOG (dialog->dialog));

		switch (ret) {
			case GTK_RESPONSE_OK:

				if (diff_execute (dialog))
					gtk_widget_hide (dialog->dialog);

				break;

			case GTK_RESPONSE_HELP:
				
				break;

			default:
				gtk_widget_hide (dialog->dialog);
		}
	} while (GTK_WIDGET_VISIBLE (dialog->dialog));

	gtk_widget_destroy (dialog->dialog);
}

static gboolean
diff_execute (DiffDialog *dialog)
{
	gint state_1;
	gint state_2;
	gchar * file_name_1;
	gchar * file_name_2;
	
	GeditDocument *document;
	
	gboolean uf; /* use unified format */
	gboolean ib; /* ignore blanks */

	gchar *command_line = NULL;
	gchar *output = NULL;
	gint exit_status = 0;
	gint output_size = 0;

	gboolean ret = FALSE;

	gedit_debug (DEBUG_PLUGINS, "");

	uf = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->unified_checkbutton)); 
	ib = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->ignore_blanks_checkbutton)); 

	state_1 = !gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->from_document_1));
	state_2 = !gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->from_document_2));

	file_name_1 = gnome_file_entry_get_full_path (GNOME_FILE_ENTRY (dialog->file_entry_1), FALSE);
	file_name_2 = gnome_file_entry_get_full_path (GNOME_FILE_ENTRY (dialog->file_entry_2), FALSE);

	/* We need to:
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

	if (!state_1 && !state_2 && 
	    (dialog->document_selected_1 == dialog->document_selected_2))
	{
		error_dialog (_("The two documents you selected are the same."), 
			      GTK_WINDOW (dialog->dialog));
		return FALSE;
	}

	if (state_1 && ((file_name_1 == NULL ) || 
	    !g_file_test (file_name_1, G_FILE_TEST_EXISTS)))
	{
		error_dialog (_("The \"first\" file you selected does not exist.\n\n"
			        "Please provide a valid file."), 
			      GTK_WINDOW (dialog->dialog));
		return FALSE;
	}

	if (state_2 && ((file_name_2 == NULL ) || 
	    !g_file_test (file_name_2, G_FILE_TEST_EXISTS)))
	{
		error_dialog (_("The \"second\" file you selected does not exist.\n\n"
			        "Please provide a valid file."), 
			      GTK_WINDOW (dialog->dialog));
		return FALSE;
	}

	if (state_1 && state_2 && (strcmp (file_name_1, file_name_2) == 0))
	{	
		error_dialog (_("The two files you selected are the same."), 
			      GTK_WINDOW (dialog->dialog));
		return FALSE;
	}

	if (!state_1)
	{
		gchar *sn;
		
		if (file_name_1 != NULL)
			g_free (file_name_1);

		document = (GeditDocument *)g_list_nth_data (dialog->open_docs, 
				dialog->document_selected_1);
		
		if (gedit_document_get_char_count (document) < 1)
		{
			error_dialog (_("The \"first\" document contains no text."), 
			              GTK_WINDOW (dialog->dialog));
			return FALSE;
		}
		
		sn = gedit_document_get_short_name (document);
		file_name_1 = temp_file_name_new (sn);
		gedit_debug (DEBUG_PLUGINS, "file_name_1: %s", file_name_1);
		g_free (sn);

		if (!gedit_document_save_a_copy_as (document, file_name_1, NULL))
		{
			g_free (file_name_1);
			file_name_1 = NULL;
		}
	}

	if (!state_2)
	{
		gchar *sn;

		if (file_name_2 != NULL)
			g_free (file_name_2);
		
		document = (GeditDocument *)g_list_nth_data (dialog->open_docs, 
				dialog->document_selected_2);
		
		if (gedit_document_get_char_count (document) < 1)
		{
			error_dialog (_("The \"second\" document contains no text."),
				      GTK_WINDOW (dialog->dialog));

			return FALSE;
		}

		sn = gedit_document_get_short_name (document);
		file_name_2 = temp_file_name_new (sn);
		gedit_debug (DEBUG_PLUGINS, "file_name_2: %s", file_name_2);
		g_free (sn);
		
		if (!gedit_document_save_a_copy_as (document, file_name_2, NULL))
		{
			g_free (file_name_2);
			file_name_2 = NULL;
		}
	}

	if (file_name_1 == NULL || file_name_2 == NULL)
	{
		/* FIXME: do better error reporting ... . Chema */
		error_dialog (_("Impossible to compare the selected documents.\n\n"
				"gedit could not create a temporary file."),
			      GTK_WINDOW (dialog->dialog));
		ret = FALSE;
		goto finally;
	}

	command_line = g_strdup_printf ("%s %s %s \"%s\" \"%s\"",
					diff_program_location, 
					uf ? "-u" : "", 
					ib ? "-i" : "", 
					file_name_1, 
					file_name_2);
	
	gedit_debug (DEBUG_PLUGINS, "Command line: %s", command_line);

	if (!g_spawn_command_line_sync (command_line, &output, NULL, &exit_status, NULL))
	{
		error_dialog (_("Impossible to compare the selected documents.\n\n"
				"Error executing the diff command."),
			      GTK_WINDOW (dialog->dialog));
	
		ret = FALSE;
		goto finally;

	}

	gedit_debug (DEBUG_PLUGINS, "Exit status : %d", WEXITSTATUS (exit_status));

	if (WEXITSTATUS (exit_status) > 1)
	{				
		error_dialog (_("Impossible to compare the selected documents.\n\n"
				"Error executing the diff command."),
			      GTK_WINDOW (dialog->dialog));
	
		if (output != NULL)
			g_free (output);

		ret = FALSE;
		goto finally;
	}

	if (WEXITSTATUS (exit_status) == 0)
	{
		GtkWidget *message_dlg;

		gedit_debug (DEBUG_PLUGINS, "");

		message_dlg = gtk_message_dialog_new (
				GTK_WINDOW (dialog->dialog),
				GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_MESSAGE_INFO,
				GTK_BUTTONS_OK, 
				_("No differences were found between the selected documents."));
		
		gtk_dialog_set_default_response (GTK_DIALOG (message_dlg), GTK_RESPONSE_OK);

		gtk_window_set_resizable (GTK_WINDOW (message_dlg), FALSE);
	
		gtk_dialog_run (GTK_DIALOG (message_dlg));
	  	gtk_widget_destroy (message_dlg);

		if (output != NULL)
			g_free (output);

		ret = TRUE;
		goto finally;

	}

	output_size = strlen (output);
	
	if (output_size > 0)
	{
		if (!g_utf8_validate (output, output_size, NULL))
		{
			/* output contains invalid UTF8 data */
			/* Try to convert it to UTF-8 from currence locale */
			gchar* converted_file_contents = NULL;
			gsize bytes_written;
			GError *conv_error = NULL;

			converted_file_contents = g_locale_to_utf8 (output, output_size,
					NULL, &bytes_written, &conv_error); 
			
			g_free (output);

			if ((conv_error != NULL) || 
			    !g_utf8_validate (converted_file_contents, bytes_written, NULL))		
			{
				/* Coversion failed */
				if (conv_error != NULL)
					g_error_free (conv_error);
	
				error_dialog (_("Impossible to compare the selected documents.\n\n"
						"The result contains invalid UTF-8 data."),
					      GTK_WINDOW (dialog->dialog));
	
				ret = FALSE;
				goto finally;

				if (converted_file_contents != NULL)
					g_free (converted_file_contents);
			

				goto finally;
			}
		
			output = converted_file_contents;
			output_size = bytes_written;
		}

		gedit_file_new ();
		
		document = gedit_get_active_document ();
		g_return_val_if_fail (document != NULL, FALSE);

		gedit_document_begin_not_undoable_action (document);
		
		/* Insert text in the buffer */
		gedit_document_insert_text (document, 0, output, output_size);
		gedit_document_set_cursor (document, 0);
		
		gedit_document_end_not_undoable_action (document);
	}
		
	g_free (output);

	ret = TRUE;	
	
finally:
	
	if (!state_1)
		unlink (file_name_1);
	if (!state_2)
		unlink (file_name_2);

	if (file_name_1 != NULL)
		g_free (file_name_1);

	if (file_name_2 != NULL)
		g_free (file_name_2);

	if (command_line != NULL)
		g_free (command_line);

	if (ret)
	{
		ignore_blanks = ib;
		use_unified_format = uf;
	}
	
	return ret;
}

static gchar *
temp_file_name_new (char* prefix)
{
	gedit_debug (DEBUG_PLUGINS, "");

	return g_strdup_printf ("%s/%s (%x%x)", 
				g_get_tmp_dir (), 
				prefix, 
				(gint) time (NULL), 
				rand ());
}

static void 
error_dialog (const gchar* str, GtkWindow *parent)
{
	GtkWidget *message_dlg;

	gedit_debug (DEBUG_PLUGINS, "");

	message_dlg = gtk_message_dialog_new (
			parent,
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_MESSAGE_WARNING,
			GTK_BUTTONS_OK, 
			str);
		
	gtk_dialog_set_default_response (GTK_DIALOG (message_dlg), GTK_RESPONSE_OK);

	gtk_window_set_resizable (GTK_WINDOW (message_dlg), FALSE);
	
	gtk_dialog_run (GTK_DIALOG (message_dlg));
  	gtk_widget_destroy (message_dlg);
}

static gboolean
configure_real (GtkWindow *parent)
{
	gchar *temp;
	
	gedit_debug (DEBUG_PLUGINS, "");
	
	temp = gedit_plugin_program_location_dialog (DIFF_PROGRAM_NAME, 
					      	     PLUGIN_NAME, 
					      	     parent);

	if (temp != NULL)
	{
		if (diff_program_location != NULL)
			g_free (diff_program_location);
		
		diff_program_location = temp;
	}
	
	return (diff_program_location != NULL);
}

G_MODULE_EXPORT GeditPluginState
update_ui (GeditPlugin *plugin, BonoboWindow *window)
{
	BonoboUIComponent *uic;
	
	gedit_debug (DEBUG_PLUGINS, "");
	
	g_return_val_if_fail (window != NULL, PLUGIN_ERROR);

	uic = gedit_get_ui_component_from_window (window);

	gedit_menus_set_verb_sensitive (uic, "/commands/" MENU_ITEM_NAME, TRUE);

	return PLUGIN_OK;
}

G_MODULE_EXPORT GeditPluginState
configure (GeditPlugin *p, GtkWidget *parent)
{
	if (configure_real (GTK_WINDOW (parent)))
		return PLUGIN_OK;
	else
		return PLUGIN_ERROR;	
}

G_MODULE_EXPORT GeditPluginState
destroy (GeditPlugin *plugin)
{
	gedit_debug (DEBUG_PLUGINS, "");

	plugin->deactivate (plugin);

	g_object_unref (G_OBJECT (diff_gconf_client));

	return PLUGIN_OK;
}

G_MODULE_EXPORT GeditPluginState
save_settings (GeditPlugin *pd)
{
	gedit_debug (DEBUG_PLUGINS, "");

	g_return_val_if_fail (diff_gconf_client != NULL, PLUGIN_ERROR);

	if (diff_program_location != NULL)
		gconf_client_set_string (
				diff_gconf_client,
				DIFF_BASE_KEY DIFF_LOCATION_KEY,
				diff_program_location,
		      		NULL);

	gconf_client_set_bool (
			diff_gconf_client,
			DIFF_BASE_KEY UNIFIED_FORMAT_KEY,
			use_unified_format,
	      		NULL);

	gconf_client_set_bool (
			diff_gconf_client,
			DIFF_BASE_KEY IGNORE_BLANKS,
			ignore_blanks,
	      		NULL);

	gconf_client_suggest_sync (diff_gconf_client, NULL);

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
				     MENU_ITEM_LABEL, MENU_ITEM_TIP, NULL,
				     diff_cb);

                pd->update_ui (pd, BONOBO_WINDOW (top_windows->data));

                top_windows = g_list_next (top_windows);
        }

        return PLUGIN_OK;
}

G_MODULE_EXPORT GeditPluginState
deactivate (GeditPlugin *pd)
{
	gedit_menus_remove_menu_item_all (MENU_ITEM_PATH, MENU_ITEM_NAME);

	return PLUGIN_OK;
}

G_MODULE_EXPORT GeditPluginState
init (GeditPlugin *pd)
{
	/* initialize */
	gedit_debug (DEBUG_PLUGINS, "");
     
	pd->name = PLUGIN_NAME;
	pd->desc = _("Makes a diff file from two documents or files on disk.\n\n"
		     "For more info on \"diff\" program, type \"man diff\" in a shell prompt.\n");
	pd->author = "Chema Celorio <chema@celorio.com> and Paolo Maggi <maggi@athena.polito.it>";
	pd->copyright = _("Copyright (C) 2000 - 2001 Chema Celorio \n"
			  "Copyright (C) 2002 Paolo Maggi");
	
	pd->private_data = NULL;

	diff_gconf_client = gconf_client_get_default ();
	g_return_val_if_fail (diff_gconf_client != NULL, PLUGIN_ERROR);

	gconf_client_add_dir (diff_gconf_client,
			      DIFF_BASE_KEY,
			      GCONF_CLIENT_PRELOAD_ONELEVEL,
			      NULL);
	
	diff_program_location = gconf_client_get_string (
				diff_gconf_client,
				DIFF_BASE_KEY DIFF_LOCATION_KEY,
			      	NULL);

	use_unified_format = gconf_client_get_bool (
			 	diff_gconf_client,
				DIFF_BASE_KEY UNIFIED_FORMAT_KEY,
			      	NULL);

	ignore_blanks = gconf_client_get_bool (
			 	diff_gconf_client,
				DIFF_BASE_KEY IGNORE_BLANKS,
			      	NULL);
		
	return PLUGIN_OK;
}




