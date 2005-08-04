/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * docinfo.c
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

#include <string.h> /* For strlen (...) */

#include <glib/gi18n.h>
#include <glade/glade-xml.h>
#include <pango/pango-break.h>

#include <gedit/gedit-plugin.h>
#include <gedit/gedit-debug.h>
#include <gedit/gedit-menus.h>
#include <gedit/gedit-utils.h>

#define MENU_ITEM_LABEL		N_("_Document Statistics")
#define MENU_ITEM_PATH		"/menu/Tools/ToolsOps_2/"
#define MENU_ITEM_NAME		"PluginWordCount"	
#define MENU_ITEM_TIP		N_("Get statistic info on current document")

typedef struct _DocInfoDialog DocInfoDialog;

struct _DocInfoDialog {
	GtkWidget *dialog;

	GtkWidget *file_name_label;
	GtkWidget *lines_label;
	GtkWidget *words_label;
	GtkWidget *chars_label;
	GtkWidget *chars_ns_label;
	GtkWidget *bytes_label;
};

static void dialog_destroyed (GtkObject *obj,  void **dialog_pointer);
static DocInfoDialog *get_dialog ();
static void dialog_response_handler (GtkDialog *dlg, gint res_id,  DocInfoDialog *dialog);

static void	word_count_real (void);
static void	word_count_cb (BonoboUIComponent *uic, gpointer user_data, 
			       const gchar* verbname);

G_MODULE_EXPORT GeditPluginState update_ui (GeditPlugin *plugin, BonoboWindow *window);
G_MODULE_EXPORT GeditPluginState activate (GeditPlugin *pd);
G_MODULE_EXPORT GeditPluginState deactivate (GeditPlugin *pd);
G_MODULE_EXPORT GeditPluginState init (GeditPlugin *pd);


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
dialog_response_handler (GtkDialog *dlg, gint res_id,  DocInfoDialog *dialog)
{
	gedit_debug (DEBUG_PLUGINS, "");

	switch (res_id) {
		case GTK_RESPONSE_OK:
			word_count_real ();
			break;
			
		default:
			gtk_widget_destroy (dialog->dialog);
	}
}

static DocInfoDialog*
get_dialog ()
{
	static DocInfoDialog *dialog = NULL;

	GladeXML *gui;
	GtkWindow *window;
	GtkWidget *content;

	gedit_debug (DEBUG_PLUGINS, "");

	window = GTK_WINDOW (gedit_get_active_window ());

	if (dialog != NULL)
	{
		gtk_window_set_transient_for (GTK_WINDOW (dialog->dialog),
				window);
		gtk_window_present (GTK_WINDOW (dialog->dialog));
		gtk_widget_grab_focus (dialog->dialog);

		return dialog;
	}

	gui = glade_xml_new (GEDIT_GLADEDIR "docinfo.glade2",
			     "docinfo_dialog_content", NULL);
	if (!gui)
	{
		gedit_warning (window,
			       MISSING_FILE,	
			       GEDIT_GLADEDIR "docinfo.glade2");
		return NULL;
	}

	dialog = g_new0 (DocInfoDialog, 1);

	dialog->dialog = gtk_dialog_new_with_buttons (_("Document Statistics"),
						      window,
						      GTK_DIALOG_DESTROY_WITH_PARENT,
						      GTK_STOCK_CLOSE,
						      GTK_RESPONSE_CLOSE,
						      NULL);

	g_return_val_if_fail (dialog->dialog != NULL, NULL);

	gtk_window_set_resizable (GTK_WINDOW (dialog->dialog), FALSE);
	gtk_dialog_set_has_separator (GTK_DIALOG (dialog->dialog), FALSE);

	/* Add the update button */
	gedit_dialog_add_button (GTK_DIALOG (dialog->dialog), 
				 _("_Update"), GTK_STOCK_REFRESH, GTK_RESPONSE_OK);

	content	= glade_xml_get_widget (gui, "docinfo_dialog_content");
	dialog->file_name_label	= glade_xml_get_widget (gui, "file_name_label");
	dialog->words_label	= glade_xml_get_widget (gui, "words_label");
	dialog->bytes_label	= glade_xml_get_widget (gui, "bytes_label");
	dialog->lines_label	= glade_xml_get_widget (gui, "lines_label");
	dialog->chars_label	= glade_xml_get_widget (gui, "chars_label");
	dialog->chars_ns_label	= glade_xml_get_widget (gui, "chars_ns_label");

	if (!content ||
	    !dialog->file_name_label ||
	    !dialog->words_label     ||
	    !dialog->bytes_label     ||
	    !dialog->lines_label     ||
	    !dialog->chars_label     ||
	    !dialog->chars_ns_label)
	{
		gedit_warning (window,
			       MISSING_WIDGETS,
			       GEDIT_GLADEDIR "docinfo.glade2");
		return NULL;
	}

	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog->dialog)->vbox),
			    content, FALSE, FALSE, 0);

	gtk_dialog_set_default_response (GTK_DIALOG (dialog->dialog),
					 GTK_RESPONSE_OK);

	g_signal_connect (G_OBJECT (dialog->dialog), "destroy",
			  G_CALLBACK (dialog_destroyed), &dialog);

	g_signal_connect (G_OBJECT (dialog->dialog), "response",
			  G_CALLBACK (dialog_response_handler), dialog);

	g_object_unref (gui);

	gtk_widget_show (dialog->dialog);

	return dialog;

}

static void
word_count_cb (BonoboUIComponent *uic, gpointer user_data, const gchar* verbname)
{
	gedit_debug (DEBUG_PLUGINS, "");

	word_count_real ();
}

#define MAX_DOC_NAME_LENGTH 40

static void
word_count_real (void)
{
	DocInfoDialog *dialog;

	GeditDocument 	*doc;
	GtkTextIter	 start, end;
	gchar		*text;
	PangoLogAttr 	*attrs;
	gint		 words = 0;
	gint		 chars = 0;
	gint		 white_chars = 0;
	gint		 lines = 0;
	gint 		 bytes = 0;
	gint		 i;
	gchar		*tmp_str;
	gchar 		*file_name;
	gchar		*trunc_name;

	gedit_debug (DEBUG_PLUGINS, "");

	dialog = get_dialog ();
	if (!dialog)
		return;

	doc = gedit_get_active_document ();

	if (doc == NULL)
	{
		gtk_widget_destroy (dialog->dialog);
		return;
	}

	gtk_text_buffer_get_bounds (GTK_TEXT_BUFFER (doc), &start, &end);
	text = gtk_text_buffer_get_slice (GTK_TEXT_BUFFER (doc),
					   &start, &end, TRUE);

	g_return_if_fail (g_utf8_validate (text, -1, NULL));

	lines = gtk_text_buffer_get_line_count (GTK_TEXT_BUFFER (doc));

	chars = g_utf8_strlen (text, -1);
 	attrs = g_new0 (PangoLogAttr, chars + 1);

	pango_get_log_attrs (text,
                       -1,
                       0,
                       pango_language_from_string ("C"),
                       attrs,
                       chars + 1);

	for (i = 0; i < chars; i++)
	{
		if (attrs [i].is_white)
			++white_chars;

		if (attrs [i].is_word_start)
			++words;
	}

	if (chars == 0)
		lines = 0;

	bytes = strlen (text);
	gedit_debug (DEBUG_PLUGINS, "Chars: %d", chars);
	gedit_debug (DEBUG_PLUGINS, "Lines: %d", lines);
	gedit_debug (DEBUG_PLUGINS, "Words: %d", words);
	gedit_debug (DEBUG_PLUGINS, "Chars non-space: %d", chars - white_chars);
	
	g_free (attrs);
	g_free (text);

	file_name = gedit_document_get_short_name (doc);
	/* Truncate the name so it doesn't get insanely wide. */
	trunc_name = gedit_utils_str_middle_truncate (file_name, 
						      MAX_DOC_NAME_LENGTH);
	g_free (file_name);

	tmp_str = g_markup_printf_escaped ("<span weight=\"bold\">%s</span>", trunc_name);
	
	gtk_label_set_markup (GTK_LABEL (dialog->file_name_label), tmp_str);
	
	g_free (trunc_name);
	g_free (tmp_str);

	tmp_str = g_strdup_printf("%d", lines);
	gtk_label_set_text (GTK_LABEL (dialog->lines_label), tmp_str);
	g_free (tmp_str);
	
	tmp_str = g_strdup_printf("%d", words);
	gtk_label_set_text (GTK_LABEL (dialog->words_label), tmp_str);
	g_free (tmp_str);

	tmp_str = g_strdup_printf("%d", chars);
	gtk_label_set_text (GTK_LABEL (dialog->chars_label), tmp_str);
	g_free (tmp_str);

	tmp_str = g_strdup_printf("%d", chars - white_chars);
	gtk_label_set_text (GTK_LABEL (dialog->chars_ns_label), tmp_str);
	g_free (tmp_str);

	tmp_str = g_strdup_printf("%d", bytes);
	gtk_label_set_text (GTK_LABEL (dialog->bytes_label), tmp_str);
	g_free (tmp_str);
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

	if (doc == NULL)		
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
				     MENU_ITEM_LABEL, MENU_ITEM_TIP, NULL,
				     word_count_cb);

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
     
	pd->private_data = NULL;
		
	return PLUGIN_OK;
}

