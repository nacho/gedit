/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * time.c
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

#include <string.h>

#include <glade/glade-xml.h>
#include <libgnome/gnome-i18n.h>
#include <gconf/gconf-client.h>
#include <libgnome/gnome-help.h>
#include <libgnome/gnome-config.h>

#include <time.h>

#include <gedit/gedit-plugin.h>
#include <gedit/gedit-debug.h>
#include <gedit/gedit-menus.h>
#include <gedit/gedit-utils.h>


#define MENU_ITEM_LABEL		N_("In_sert Date and Time...")
#define MENU_ITEM_PATH		"/menu/Edit/EditOps_4/"
#define MENU_ITEM_NAME		"PluginInsertDateAndTime"	
#define MENU_ITEM_TIP		N_("Insert current date and time at the cursor position")

#define TIME_BASE_KEY 	"/apps/gedit-2/plugins/time"
#define SELECTED_FORMAT_KEY "/selected_format"
#define PROMPT_TYPE_KEY "/prompt_type"
#define CUSTOM_FORMAT_KEY "/custom_format"

#define DIALOG_PROMPT_TYPE_KEY "gedit-2/time_plugin/prompt_type"

#define DEFAULT_CUSTOM_FORMAT "%d/%m/%Y %H:%M:%S"

enum
{
  COLUMN_FORMATS = 0,
  COLUMN_INDEX,
  NUM_COLUMNS
};

typedef struct _TimeConfigureDialog TimeConfigureDialog;

struct _TimeConfigureDialog {
	GtkWidget *dialog;

	GtkWidget *list;
        /* Radio buttons to indicate what should be done */
        GtkWidget *prompt;
        GtkWidget *use_list;
        GtkWidget *custom;
        /* The Entry for putting a custom format in */
        GtkWidget *custom_entry;
	GtkWidget *custom_format_example;
};

typedef struct _ChoseFormatDialog ChoseFormatDialog;

struct _ChoseFormatDialog {
	GtkWidget *dialog;

	GtkWidget *list;
        /* Radio buttons to indicate what should be done */
        GtkWidget *use_list;
        GtkWidget *custom;
        /* The Entry for putting a custom format in */
        GtkWidget *custom_entry;
	GtkWidget *custom_format_example;
};


static gchar *formats[] =
{
	"%c",
	"%x",
	"%X",
	"%x %X",
	"%Y-%m-%d %H:%M:%S",
	"%a %b %d %H:%M:%S %Z %Y",
	"%a %b %d %H:%M:%S %Y",
	"%a %d %b %Y %H:%M:%S %Z",
	"%a %d %b %Y %H:%M:%S",
	"%d/%m/%Y",
	"%d/%m/%y",
	"%D",
	"%A %d %B %Y",
	"%A %B %d %Y",
	"%Y-%m-%d",
	"%d %B %Y",
	"%B %d, %Y",
	"%A %b %d",
	"%H:%M:%S",
	"%H:%M",
	"%I:%M:%S %p",
	"%I:%M %p",
	"%H.%M.%S",
	"%H.%M",
	"%I.%M.%S %p",
	"%I.%M %p",
	"%d/%m/%Y %H:%M:%S",
	"%d/%m/%y %H:%M:%S",
#if __GLIBC__ >= 2
	"%a, %d %b %Y %H:%M:%S %z",
#endif
	NULL
};


typedef enum 
{
	PROMPT_FOR_FORMAT = 0,
	USE_SELECTED_FORMAT,
	USE_CUSTOM_FORMAT
} GeditTimePluginPromptType;

static GConfClient 		*time_gconf_client 	= NULL;

static char 			*get_time (const gchar* format);
static void			 dialog_destroyed (GtkObject *obj,  void **dialog_pointer);
static GtkTreeModel		*create_model (GtkWidget *listview,  const gchar *sel_format);
static void 			 scroll_to_selected (GtkTreeView *tree_view);
static void			 create_formats_list (GtkWidget *listview, const gchar *sel_format);
static TimeConfigureDialog 	*get_configure_dialog (GtkWindow* parent);
static void			 time_world_cb (BonoboUIComponent *uic, gpointer user_data, 
						const gchar* verbname);
static void			 ok_button_pressed (TimeConfigureDialog *dialog);

static void			 help_button_pressed (TimeConfigureDialog *dialog);
static gint 			 get_format_from_list(GtkWidget *listview);

G_MODULE_EXPORT GeditPluginState update_ui (GeditPlugin *plugin, BonoboWindow *window);
G_MODULE_EXPORT GeditPluginState destroy (GeditPlugin *pd);
G_MODULE_EXPORT GeditPluginState activate (GeditPlugin *pd);
G_MODULE_EXPORT GeditPluginState deactivate (GeditPlugin *pd);
G_MODULE_EXPORT GeditPluginState init (GeditPlugin *pd);
G_MODULE_EXPORT GeditPluginState configure (GeditPlugin *p, GtkWidget *parent);

static gchar *dialog_custom_format = NULL;
static gchar *dialog_selected_format = NULL;
static GeditTimePluginPromptType dialog_prompt_type;

static GeditTimePluginPromptType
get_prompt_type ()
{
	gchar *prompt_type;
	GeditTimePluginPromptType res;
	
	g_return_val_if_fail (time_gconf_client != NULL, PROMPT_FOR_FORMAT);
       	
	prompt_type = gconf_client_get_string (time_gconf_client,
			        TIME_BASE_KEY PROMPT_TYPE_KEY,
				NULL);

	if (prompt_type == NULL)
		return PROMPT_FOR_FORMAT;

	if (strcmp (prompt_type, "USE_SELECTED_FORMAT") == 0)
		res = USE_SELECTED_FORMAT;
	else 
	{
		if  (strcmp (prompt_type, "USE_CUSTOM_FORMAT") == 0)
			res = USE_CUSTOM_FORMAT;
		else
			res = PROMPT_FOR_FORMAT;
	}

	g_free (prompt_type);

	return res;
}

static void
set_prompt_type (GeditTimePluginPromptType prompt_type)
{
	const gchar * str;

	g_return_if_fail (time_gconf_client != NULL);
	g_return_if_fail (gconf_client_key_is_writable (time_gconf_client,
				TIME_BASE_KEY PROMPT_TYPE_KEY, NULL));

	switch (prompt_type)
	{
		case USE_SELECTED_FORMAT:
			str = "USE_SELECTED_FORMAT";
			break;
		case USE_CUSTOM_FORMAT:
			str = "USE_CUSTOM_FORMAT";
			break;
		default:
			str = "PROMPT_FOR_FORMAT";
	}
	
	gconf_client_set_string (time_gconf_client, 
				 TIME_BASE_KEY PROMPT_TYPE_KEY,
		       		 str, NULL);
}
	
static gchar *
get_selected_format ()
{
	gchar *sel_format;
		
	g_return_val_if_fail (time_gconf_client != NULL, g_strdup (formats [0]));
       	
	sel_format = gconf_client_get_string (time_gconf_client,
			        TIME_BASE_KEY SELECTED_FORMAT_KEY,
				NULL);
	if (sel_format == NULL)
		return g_strdup (formats [0]);

	return sel_format;
}

static void
set_selected_format (const gchar *format)
{
	g_return_if_fail (format != NULL);
	g_return_if_fail (time_gconf_client != NULL);
	
	if (!gconf_client_key_is_writable (time_gconf_client, 
				TIME_BASE_KEY SELECTED_FORMAT_KEY, NULL))
		return;
		
	gconf_client_set_string (time_gconf_client, 
				 TIME_BASE_KEY SELECTED_FORMAT_KEY,
		       		 format, NULL);
}

static gchar *
get_custom_format ()
{
	gchar *format;
	
	g_return_val_if_fail (time_gconf_client != NULL, g_strdup (DEFAULT_CUSTOM_FORMAT));
       	
	format = gconf_client_get_string (time_gconf_client,
			        TIME_BASE_KEY CUSTOM_FORMAT_KEY,
				NULL);
	if (format == NULL)
		return g_strdup (DEFAULT_CUSTOM_FORMAT);

	return format;
}

static void
set_custom_format (const gchar *format)
{
	g_return_if_fail (format != NULL);
	g_return_if_fail (time_gconf_client != NULL);
	if (!gconf_client_key_is_writable (time_gconf_client,
				TIME_BASE_KEY CUSTOM_FORMAT_KEY, NULL))
		return;

	gconf_client_set_string (time_gconf_client, 
				 TIME_BASE_KEY CUSTOM_FORMAT_KEY,
		       		 format, NULL);
}


static char *
get_time (const gchar* format)
{
  	gchar *out = NULL;
	gchar *out_utf8 = NULL;
  	time_t clock;
  	struct tm *now;
  	size_t out_length = 0;
	gchar *locale_format;
  	
	gedit_debug (DEBUG_PLUGINS, "");

	if (strlen (format) == 0)
		return g_strdup (" ");

	locale_format = g_locale_from_utf8 (format, -1, NULL, NULL, NULL);
	
	if (locale_format == NULL)
		return g_strdup (" ");
		
  	clock = time (NULL);
  	now = localtime (&clock);
	  	
	do
	{
		out_length += 255;
		out = g_realloc (out, out_length);
	}
  	while (strftime (out, out_length, locale_format, now) == 0);
  	
	g_free (locale_format);

	if (g_utf8_validate (out, -1, NULL))
		out_utf8 = out;
	else
	{
		out_utf8 = g_locale_to_utf8 (out, -1, NULL, NULL, NULL);
		g_free (out);

		if (out_utf8 == NULL)
			out_utf8 = g_strdup (" ");
	}
	
  	return out_utf8;
}

static void
dialog_destroyed (GtkObject *obj,  void **dialog_pointer)
{
	gedit_debug (DEBUG_PLUGINS, "");

	if (dialog_pointer != NULL)
	{
		g_free (*dialog_pointer);
		*dialog_pointer = NULL;
	}

	gedit_debug (DEBUG_PLUGINS, "END");
	
}

static GtkTreeModel*
create_model (GtkWidget *listview, const gchar *sel_format)
{
	gint i = 0;
	GtkListStore *store;
	GtkTreeIter iter;
	gchar *sf;
	
	gedit_debug (DEBUG_PLUGINS, "");

	/* create list store */
	store = gtk_list_store_new (NUM_COLUMNS, G_TYPE_STRING, G_TYPE_INT);

	/* Set tree view model*/
	gtk_tree_view_set_model (GTK_TREE_VIEW (listview), 
				 GTK_TREE_MODEL (store));

	g_object_unref (G_OBJECT (store));

	if (sel_format == NULL)
		sf = get_selected_format ();
	else
		sf = g_strdup (sel_format);
	
	/* add data to the list store */
	while (formats[i] != NULL)
	{
		gchar *str;

		str = get_time (formats[i]);
		
		gedit_debug (DEBUG_PLUGINS, "%d : %s", i, str);
		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter,
				    COLUMN_FORMATS, str,
				    COLUMN_INDEX, i,
				    -1);
		g_free (str);	
		
		if (strncmp (formats[i], sf, strlen (sf)) == 0)
		{
			GtkTreeSelection *selection;
						
			selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (listview));
			g_return_val_if_fail (selection != NULL, GTK_TREE_MODEL (store));
			gtk_tree_selection_select_iter (selection, &iter);
		}	

		++i;
	}
	
	g_free (sf);	

	return GTK_TREE_MODEL (store);
}

static void 
scroll_to_selected (GtkTreeView *tree_view)
{
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreeIter iter;

	gedit_debug (DEBUG_PLUGINS, "");

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
create_formats_list (GtkWidget *listview, const gchar* sel_format)
{
	GtkTreeViewColumn *column;
	GtkCellRenderer *cell;
	
	gedit_debug (DEBUG_PLUGINS, "");

	g_return_if_fail (listview != NULL);

	/* Create model, it also add model to the tree view */
	create_model (listview, sel_format);
	
	/* the Available formats column */
	cell = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Available formats"), cell, 
			"text", COLUMN_FORMATS, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (listview), column);

	g_signal_connect (G_OBJECT (listview), "realize", 
			G_CALLBACK (scroll_to_selected), NULL);

	gtk_widget_show (listview);
}

static void 
updated_custom_format_example (GtkEntry *format_entry, GtkLabel *format_example)
{
	const gchar *format;
	gchar *time;
	gchar *str;
	gchar *escaped_time;
	
	gedit_debug (DEBUG_PLUGINS, "");

	g_return_if_fail (GTK_IS_ENTRY (format_entry));
	g_return_if_fail (GTK_IS_LABEL (format_example));

	format = gtk_entry_get_text (format_entry);

	time = get_time (format);
	escaped_time = g_markup_escape_text (time, -1);
	
	str = g_strdup_printf ("<span size=\"small\">%s</span>", escaped_time);
	
	gtk_label_set_markup (format_example, str);

	g_free (escaped_time);
	g_free (time);
	g_free (str);
}

static void
configure_dialog_button_toggled (GtkToggleButton *button, TimeConfigureDialog *dialog)
{
	gedit_debug (DEBUG_PLUGINS, "");

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->custom)))
	{
		gtk_widget_set_sensitive (dialog->list, FALSE);
		gtk_widget_set_sensitive (dialog->custom_entry, TRUE);
		gtk_widget_set_sensitive (dialog->custom_format_example, TRUE);

		return;
	}

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->use_list)))
	{
		gtk_widget_set_sensitive (dialog->list, TRUE);
		gtk_widget_set_sensitive (dialog->custom_entry, FALSE);
		gtk_widget_set_sensitive (dialog->custom_format_example, FALSE);

		return;
	}

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->prompt)))
	{
		gtk_widget_set_sensitive (dialog->list, FALSE);
		gtk_widget_set_sensitive (dialog->custom_entry, FALSE);
		gtk_widget_set_sensitive (dialog->custom_format_example, FALSE);

		return;
	}
}

static TimeConfigureDialog*
get_configure_dialog (GtkWindow* parent)
{
	static TimeConfigureDialog *dialog = NULL;

	GladeXML *gui;
	GtkWidget *content;
	GtkWidget *viewport;
	GeditTimePluginPromptType prompt_type;
	gchar *sf, *cf;

	gedit_debug (DEBUG_PLUGINS, "");

	if (dialog != NULL)
	{
		gtk_window_set_transient_for (GTK_WINDOW (dialog->dialog),
				parent);
	
		gtk_window_present (GTK_WINDOW (dialog->dialog));

		return dialog;
	}

	gui = glade_xml_new (GEDIT_GLADEDIR "time.glade2",
			     "time_dialog_content", NULL);   
	if (!gui) 
        {
		g_warning
		    ("Could not find time.glade2, reinstall gedit.\n");
		return NULL;
	}

	dialog = g_new0 (TimeConfigureDialog, 1);

	dialog->dialog = gtk_dialog_new_with_buttons (_("Configure insert date/time plugin..."),
						      parent,
						      GTK_DIALOG_DESTROY_WITH_PARENT |
						      GTK_DIALOG_MODAL,
						      GTK_STOCK_CANCEL,
						      GTK_RESPONSE_CANCEL,
						      GTK_STOCK_OK,
						      GTK_RESPONSE_OK,
						      GTK_STOCK_HELP,
						      GTK_RESPONSE_HELP,
						      NULL);

	g_return_val_if_fail (dialog->dialog != NULL, NULL);

	content		= glade_xml_get_widget (gui, "time_dialog_content");
	viewport 	= glade_xml_get_widget (gui, "formats_viewport");
	dialog->list 	= glade_xml_get_widget (gui, "formats_tree");
        dialog->prompt  = glade_xml_get_widget (gui, "always_prompt");
        dialog->use_list= glade_xml_get_widget (gui, "never_prompt");
        dialog->custom  = glade_xml_get_widget (gui, "use_custom");
        dialog->custom_entry = glade_xml_get_widget (gui, "custom_entry");
	dialog->custom_format_example = glade_xml_get_widget (gui, "custom_format_example");

	g_return_val_if_fail (content != NULL, NULL);
	g_return_val_if_fail (viewport != NULL, NULL);
	g_return_val_if_fail (dialog->list != NULL, NULL);
	g_return_val_if_fail (dialog->prompt != NULL, NULL);
	g_return_val_if_fail (dialog->use_list != NULL, NULL);
	g_return_val_if_fail (dialog->custom != NULL, NULL);
	g_return_val_if_fail (dialog->custom_entry != NULL, NULL);
	g_return_val_if_fail (dialog->custom_format_example != NULL, NULL);

	sf = get_selected_format ();
	create_formats_list (dialog->list, sf);
	g_free (sf);
	
	prompt_type = get_prompt_type ();

	cf = get_custom_format ();
     	gtk_entry_set_text (GTK_ENTRY(dialog->custom_entry), cf);
       	g_free (cf);

        if (prompt_type == USE_CUSTOM_FORMAT)
        {
	        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->custom), TRUE);

		gtk_widget_set_sensitive (dialog->list, FALSE);
		gtk_widget_set_sensitive (dialog->custom_entry, TRUE);
		gtk_widget_set_sensitive (dialog->custom_format_example, TRUE);
        }
        else if (prompt_type == USE_SELECTED_FORMAT)
        {
	        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->use_list), TRUE);

		gtk_widget_set_sensitive (dialog->list, TRUE);
		gtk_widget_set_sensitive (dialog->custom_entry, FALSE);
		gtk_widget_set_sensitive (dialog->custom_format_example, FALSE);
        }
        else
        {
	        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->prompt), TRUE);

		gtk_widget_set_sensitive (dialog->list, FALSE);
		gtk_widget_set_sensitive (dialog->custom_entry, FALSE);
		gtk_widget_set_sensitive (dialog->custom_format_example, FALSE);
        }

	updated_custom_format_example (GTK_ENTRY (dialog->custom_entry), 
			GTK_LABEL (dialog->custom_format_example));
	
	/* setup a window of a sane size. */
	gtk_widget_set_size_request (GTK_WIDGET (viewport), 10, 200);

	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog->dialog)->vbox),
			    content, FALSE, FALSE, 0);

	gtk_dialog_set_default_response (GTK_DIALOG (dialog->dialog),
					 GTK_RESPONSE_OK);

	g_signal_connect (G_OBJECT (dialog->custom), "toggled",
			  G_CALLBACK (configure_dialog_button_toggled), 
			  dialog);

   	g_signal_connect (G_OBJECT (dialog->prompt), "toggled",
			  G_CALLBACK (configure_dialog_button_toggled), 
			  dialog);

	g_signal_connect (G_OBJECT (dialog->use_list), "toggled",
			  G_CALLBACK (configure_dialog_button_toggled), 
			  dialog);

	g_signal_connect (G_OBJECT (dialog->dialog), "destroy",
			  G_CALLBACK (dialog_destroyed), &dialog);
	
	g_signal_connect (G_OBJECT (dialog->custom_entry), "changed",
			  G_CALLBACK (updated_custom_format_example), 
			  dialog->custom_format_example);

	g_object_unref (gui);

	gtk_window_set_resizable (GTK_WINDOW (dialog->dialog), FALSE);

	gtk_window_set_transient_for (GTK_WINDOW (dialog->dialog),
				parent);

	return dialog;
}

static void
chose_format_dialog_button_toggled (GtkToggleButton *button, ChoseFormatDialog *dialog)
{
	gedit_debug (DEBUG_PLUGINS, "");

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->custom)))
	{
		gtk_widget_set_sensitive (dialog->list, FALSE);
		gtk_widget_set_sensitive (dialog->custom_entry, TRUE);
		gtk_widget_set_sensitive (dialog->custom_format_example, TRUE);

		return;
	}

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->use_list)))
	{
		gtk_widget_set_sensitive (dialog->list, TRUE);
		gtk_widget_set_sensitive (dialog->custom_entry, FALSE);
		gtk_widget_set_sensitive (dialog->custom_format_example, FALSE);

		return;
	}
}

static ChoseFormatDialog *
get_chose_format_dialog (GtkWindow *parent)
{
	static ChoseFormatDialog *dialog = NULL;
	GladeXML *gui;

	g_return_val_if_fail (dialog == NULL, NULL);

	gui = glade_xml_new (GEDIT_GLADEDIR "time.glade2",
			     "chose_format_dialog", NULL);
	if (!gui) 
        {
		g_warning
		    ("Could not find time.glade2, reinstall gedit.\n");
		return NULL;
	}

	dialog = g_new0 (ChoseFormatDialog, 1);

	dialog->dialog 		= glade_xml_get_widget (gui, "chose_format_dialog");
	dialog->list 		= glade_xml_get_widget (gui, "choice_list");
	dialog->use_list 	= glade_xml_get_widget (gui, "use_sel_format_radiobutton");
	dialog->custom		= glade_xml_get_widget (gui, "use_custom_radiobutton");
	dialog->custom_entry	= glade_xml_get_widget (gui, "custom_entry");
	dialog->custom_format_example = glade_xml_get_widget (gui, "custom_format_example");

	g_return_val_if_fail (dialog->dialog != NULL, NULL);
	g_return_val_if_fail (dialog->list != NULL, NULL);
	g_return_val_if_fail (dialog->use_list != NULL, NULL);
	g_return_val_if_fail (dialog->custom != NULL, NULL);
	g_return_val_if_fail (dialog->custom_entry != NULL, NULL);
	g_return_val_if_fail (dialog->custom_format_example != NULL, NULL);

	if (dialog_selected_format == NULL)
		dialog_selected_format = get_selected_format ();

	create_formats_list (dialog->list, dialog_selected_format);

	if (dialog_custom_format == NULL)
		dialog_custom_format = get_custom_format ();
	
     	gtk_entry_set_text (GTK_ENTRY(dialog->custom_entry), dialog_custom_format);
	updated_custom_format_example (GTK_ENTRY (dialog->custom_entry), 
			GTK_LABEL (dialog->custom_format_example));

	if (dialog_prompt_type == USE_CUSTOM_FORMAT)
	{
        	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->custom), TRUE);
		
		gtk_widget_set_sensitive (dialog->list, FALSE);
		gtk_widget_set_sensitive (dialog->custom_entry, TRUE);
		gtk_widget_set_sensitive (dialog->custom_format_example, TRUE);
	}
	else
	{
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->use_list), TRUE);

		gtk_widget_set_sensitive (dialog->list, TRUE);
		gtk_widget_set_sensitive (dialog->custom_entry, FALSE);
		gtk_widget_set_sensitive (dialog->custom_format_example, FALSE);
	}

	/* setup a window of a sane size. */
	gtk_widget_set_size_request (dialog->list, 10, 200);

	gtk_dialog_set_default_response (GTK_DIALOG (dialog->dialog),
					 GTK_RESPONSE_OK);

	g_signal_connect (G_OBJECT (dialog->custom), "toggled",
			  G_CALLBACK (chose_format_dialog_button_toggled), 
			  dialog);

	g_signal_connect (G_OBJECT (dialog->use_list), "toggled",
			  G_CALLBACK (chose_format_dialog_button_toggled), 
			  dialog);

	g_signal_connect (G_OBJECT (dialog->dialog), "destroy",
			  G_CALLBACK (dialog_destroyed), &dialog);
	
	g_signal_connect (G_OBJECT (dialog->custom_entry), "changed",
			  G_CALLBACK (updated_custom_format_example), 
			  dialog->custom_format_example);

	g_object_unref (gui);

	gtk_window_set_resizable (GTK_WINDOW (dialog->dialog), FALSE);

	gtk_window_set_transient_for (GTK_WINDOW (dialog->dialog), parent);

	return dialog;
}
	
static void
time_world_cb (BonoboUIComponent *uic, gpointer user_data, const gchar* verbname)
{
	GeditDocument *doc;
        gint prompt_response;
	GeditView *view;
	gchar *the_time = NULL;
	GeditTimePluginPromptType prompt_type;
	
	gedit_debug (DEBUG_PLUGINS, "");

	view = gedit_get_active_view ();
	g_return_if_fail (view != NULL);
	
	doc = gedit_view_get_document (view);
	g_return_if_fail (doc != NULL);

	prompt_type = get_prompt_type ();
	
        if (prompt_type == USE_CUSTOM_FORMAT)
        {
		gchar *cf = get_custom_format ();
	        the_time = get_time (cf);
		g_free (cf);

	}
        else if (prompt_type == USE_SELECTED_FORMAT)
        {
		gchar *sf = get_selected_format ();
	        the_time = get_time (sf);
		g_free (sf);
	}
        else
        {
		ChoseFormatDialog *dialog;
		
		BonoboWindow *aw = gedit_get_active_window ();
		g_return_if_fail (aw != NULL);

		dialog = get_chose_format_dialog (GTK_WINDOW (aw));
		g_return_if_fail (dialog != NULL);

		prompt_response = gtk_dialog_run (GTK_DIALOG (dialog->dialog));

		switch (prompt_response)
		{
			case GTK_RESPONSE_OK:
				/* Get the user's chosen format */
				if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (
							dialog->use_list)))
				{
					gint sel_format;
					
					sel_format = get_format_from_list (dialog->list);
					
					the_time = get_time (formats [sel_format]);
					
					g_free (dialog_selected_format);		
					dialog_selected_format =
					       g_strdup	(formats [sel_format]);

					dialog_prompt_type = USE_SELECTED_FORMAT;
					set_selected_format (dialog_selected_format);
				}
				else
				{
					const gchar *format;
					
					format = gtk_entry_get_text (
								GTK_ENTRY (dialog->custom_entry));
					the_time = get_time (format);
					
					g_free (dialog_custom_format);					
					dialog_custom_format = g_strdup (format);

					dialog_prompt_type = USE_CUSTOM_FORMAT;
					set_custom_format (dialog_custom_format);
				}
						
				gtk_widget_destroy (dialog->dialog);
				break;
				
			case GTK_RESPONSE_CANCEL:
				gtk_widget_destroy (dialog->dialog);
				return;
	           
			case GTK_RESPONSE_HELP:
				/* FIXME: help hooks go in here! */
				gtk_widget_destroy (dialog->dialog);
		     		return;
		}
	}
   
	g_return_if_fail (the_time != NULL);

	gedit_document_begin_user_action (doc);
	
	gedit_document_insert_text_at_cursor (doc, the_time, -1);
	gedit_document_insert_text_at_cursor (doc, " ", -1);

	gedit_document_end_user_action (doc);

	g_free (the_time);
}

static 
gint get_format_from_list(GtkWidget *listview)
{
	GtkTreeModel *model;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
        gint selected_value;

	gedit_debug (DEBUG_PLUGINS, "");

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (listview));
	g_return_val_if_fail (model != NULL, 0);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (listview));
	g_return_val_if_fail (selection != NULL, 0);

	if (gtk_tree_selection_get_selected (selection, NULL, &iter))
	{
		gtk_tree_model_get (model, &iter, COLUMN_INDEX, &selected_value, -1);
	}
	gedit_debug (DEBUG_PLUGINS, "");

        return selected_value;
}


static void
ok_button_pressed (TimeConfigureDialog *dialog)
{
	gint sel_format;
	const gchar *custom_format;

	gedit_debug (DEBUG_PLUGINS, "");
   
	sel_format = get_format_from_list (dialog->list);
   
	custom_format = gtk_entry_get_text (GTK_ENTRY (dialog->custom_entry));
	
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->custom)))
	{
		set_prompt_type (USE_CUSTOM_FORMAT);
		set_custom_format (custom_format);
	}
	else 
	{
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->use_list)))
		{
			set_prompt_type (USE_SELECTED_FORMAT);

			set_selected_format (formats [sel_format]);
		}
		else
     			/* Default to always prompt the user */
			set_prompt_type (PROMPT_FOR_FORMAT);
	}

	gedit_debug (DEBUG_PLUGINS, "Sel: %d", sel_format);
}


static void
help_button_pressed (TimeConfigureDialog *dialog)
{
	GError *error = NULL;
	
	gedit_debug (DEBUG_PLUGINS, "");

	gnome_help_display ("gedit.xml", "gedit-date-time-configure", &error);

	if (error != NULL)
	{
		gedit_warning (GTK_WINDOW (dialog->dialog), error->message);

		g_error_free (error);
	}
}


G_MODULE_EXPORT GeditPluginState
configure (GeditPlugin *p, GtkWidget *parent)
{
	TimeConfigureDialog *dialog;
	gint ret;
	
	gedit_debug (DEBUG_PLUGINS, "");
	
	dialog = get_configure_dialog (GTK_WINDOW (parent));

	if (dialog == NULL) 
	{
		g_warning
		    ("Could not create the configure dialog.\n");
		return PLUGIN_ERROR;
	}

	do
	{
		ret = gtk_dialog_run (GTK_DIALOG (dialog->dialog));

		switch (ret)
		{
			case GTK_RESPONSE_OK:
				gedit_debug (DEBUG_PLUGINS, "Ok button pressed");
				ok_button_pressed (dialog);
				break;
			
			case GTK_RESPONSE_HELP:
				gedit_debug (DEBUG_PLUGINS, "Help button pressed");
				help_button_pressed (dialog);
				break;
			
			default:
				gedit_debug (DEBUG_PLUGINS, "Default");
		}

	} while (ret == GTK_RESPONSE_HELP);

	gedit_debug (DEBUG_PLUGINS, "Destroying dialog");

	gtk_widget_destroy (dialog->dialog);

	gedit_debug (DEBUG_PLUGINS, "Done");

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

	if ((doc == NULL) || (gedit_document_is_readonly (doc)))		
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
				     NULL, time_world_cb);

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
destroy (GeditPlugin *pd)
{
	gedit_debug (DEBUG_PLUGINS, "");

	g_return_val_if_fail (time_gconf_client != NULL, PLUGIN_ERROR);

	gconf_client_suggest_sync (time_gconf_client, NULL);

	g_object_unref (G_OBJECT (time_gconf_client));
	time_gconf_client = NULL;

	g_free (dialog_selected_format);
	g_free (dialog_custom_format);

	gnome_config_set_int (DIALOG_PROMPT_TYPE_KEY, dialog_prompt_type);
	gnome_config_sync ();

	return PLUGIN_OK;
}

G_MODULE_EXPORT GeditPluginState
init (GeditPlugin *pd)
{
	/* initialize */
	gedit_debug (DEBUG_PLUGINS, "");

	pd->private_data = NULL;
		
	time_gconf_client = gconf_client_get_default ();
	g_return_val_if_fail (time_gconf_client != NULL, PLUGIN_ERROR);

	gconf_client_add_dir (time_gconf_client,
			      TIME_BASE_KEY,
			      GCONF_CLIENT_PRELOAD_ONELEVEL,
			      NULL);

	dialog_prompt_type = gnome_config_get_int (DIALOG_PROMPT_TYPE_KEY "=1");

	return PLUGIN_OK;
}




