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

#include <glade/glade-xml.h>
#include <libgnome/gnome-i18n.h>

#include <pango/pango-break.h>

#include <string.h> /* For strlen (...) */

#include <gedit-plugin.h>
#include <gedit-debug.h>
#include <gedit-menus.h>
#include <gedit-utils.h>

#define MENU_ITEM_LABEL		N_("_Word Count")
#define MENU_ITEM_PATH		"/menu/Tools/ToolsOps_2/"
#define MENU_ITEM_NAME		"PluginWordCount"	
#define MENU_ITEM_TIP		N_("Get info on current document")

typedef struct _DocInfoDialog DocInfoDialog;

struct _DocInfoDialog {
	GtkWidget *dialog;

	GtkWidget *frame;
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

	if (!gui) {
		g_warning
		    ("Could not find docinfo.glade2, reinstall gedit.\n");
		return NULL;
	}

	dialog = g_new0 (DocInfoDialog, 1);

	dialog->dialog = gtk_dialog_new_with_buttons (_("Word count"),
						      window,
						      GTK_DIALOG_DESTROY_WITH_PARENT,
						      GTK_STOCK_CLOSE,
						      GTK_RESPONSE_CLOSE,
						      NULL);

	g_return_val_if_fail (dialog->dialog != NULL, NULL);

	/* Add the update button */
	gedit_dialog_add_button (GTK_DIALOG (dialog->dialog), 
				 _("_Update"), GTK_STOCK_REFRESH, GTK_RESPONSE_OK);

	content			= glade_xml_get_widget (gui, "docinfo_dialog_content");

	g_return_val_if_fail (content != NULL, NULL);

	dialog->frame		= glade_xml_get_widget (gui, "frame");
	dialog->words_label	= glade_xml_get_widget (gui, "words_label");
	dialog->bytes_label	= glade_xml_get_widget (gui, "bytes_label");
	dialog->lines_label	= glade_xml_get_widget (gui, "lines_label");
	dialog->chars_label	= glade_xml_get_widget (gui, "chars_label");
	dialog->chars_ns_label	= glade_xml_get_widget (gui, "chars_ns_label");

	g_return_val_if_fail (dialog->frame              != NULL, NULL);
	g_return_val_if_fail (dialog->words_label        != NULL, NULL);
	g_return_val_if_fail (dialog->bytes_label        != NULL, NULL);
	g_return_val_if_fail (dialog->lines_label        != NULL, NULL);
	g_return_val_if_fail (dialog->chars_label        != NULL, NULL);
	g_return_val_if_fail (dialog->chars_ns_label     != NULL, NULL);

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

	gtk_widget_show (dialog->dialog);

	return dialog;

}

static void
word_count_cb (BonoboUIComponent *uic, gpointer user_data, const gchar* verbname)
{
	gedit_debug (DEBUG_PLUGINS, "");

	word_count_real ();
}

static void
word_count_real (void)
{
	DocInfoDialog *dialog;

	GeditDocument 	*doc;
	gchar		*text;
	PangoLogAttr 	*attrs;
	gint		 words = 0;
	gint		 chars = 0;
	gint		 white_chars = 0;
	gint		 lines = 0;
	gint 		 bytes = 0;
	gint		 i;
	gchar		*tmp_str;

	gedit_debug (DEBUG_PLUGINS, "");

	dialog = get_dialog ();
	if (dialog == NULL) {
		g_warning ("Could not create the Word Count dialog");
		return;
	}

	doc = gedit_get_active_document ();

	if (doc == NULL)
	{
		gtk_widget_destroy (dialog->dialog);
		return;
	}

	text = gedit_document_get_chars (doc, 0, -1);
	g_return_if_fail (g_utf8_validate (text, -1, NULL));

	lines = gedit_document_get_line_count (doc);
	
	chars = g_utf8_strlen (text, -1);
 	attrs = g_new0 (PangoLogAttr, chars + 1);

	pango_get_log_attrs (text,
                       -1,
                       0,
                       pango_language_from_string ("C"),
                       attrs,
                       chars + 1);

	i = 0;
	
	while (i < chars)
	{
		if (attrs [i].is_white)
			++white_chars;

		if (attrs [i].is_word_start)
			++words;

		++i;
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

	tmp_str = gedit_document_get_short_name (doc);
	gtk_frame_set_label (GTK_FRAME (dialog->frame), tmp_str);
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
     
	pd->name = _("Word count");
	pd->desc = _("The word count plugin analyzes the current document and determines the number "
		     "of words, lines, characters and non-space characters in it "
		     "and display the result.");
	pd->author = "Paolo Maggi <maggi@athena.polito.it>";
	pd->copyright = _("Copyright (C) 2002 - Paolo Maggi");
	
	pd->private_data = NULL;
		
	return PLUGIN_OK;
}




