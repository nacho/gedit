/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-page-setup-dialog.c
 * This file is part of gedit
 *
 * Copyright (C) 2003 Paolo Maggi 
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
 * Modified by the gedit Team, 2003. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include <glade/glade-xml.h>
#include <libgnome/libgnome.h>
#include <gconf/gconf-client.h>

#include <gedit/gedit-prefs-manager.h>

#include "gedit-page-setup-dialog.h"
#include "gedit2.h"
#include "gedit-utils.h"
#include "gedit-debug.h"
#include "gnome-print-font-picker.h"

typedef struct _GeditPageSetupDialog GeditPageSetupDialog;

struct _GeditPageSetupDialog {
	GtkWidget *dialog;

	GtkWidget *syntax_checkbutton;

	GtkWidget *page_header_checkbutton;
	
	GtkWidget *line_numbers_checkbutton;
	GtkWidget *line_numbers_hbox;
	GtkWidget *line_numbers_spinbutton;

	GtkWidget *text_wrapping_checkbutton;
	GtkWidget *do_not_split_checkbutton;

	GtkWidget *fonts_table;

	GtkWidget *body_font_label;
	GtkWidget *headers_font_label;
	GtkWidget *numbers_font_label;

	GtkWidget *body_fontpicker;
	GtkWidget *headers_fontpicker;
	GtkWidget *numbers_fontpicker;

	GtkWidget *restore_button;
};

static gboolean split_button_state = TRUE;
static gint old_line_numbers_value = 1;

static void dialog_destroyed (GtkObject *obj,  void **dialog_pointer);

static void
dialog_destroyed (GtkObject *obj,  void **dialog_pointer)
{
	gedit_debug (DEBUG_PRINT, "");

	if (dialog_pointer != NULL)
	{
		g_free (*dialog_pointer);
		*dialog_pointer = NULL;
	}	
}

static void
dialog_response_handler (GtkDialog *dlg, gint res_id,  GeditPageSetupDialog *dialog)
{
	gedit_debug (DEBUG_PRINT, "");

	switch (res_id) {
		case GTK_RESPONSE_HELP:
			/* TODO */;
			break;
			
		default:
			gtk_widget_destroy (dialog->dialog);
	}
}

static void
syntax_checkbutton_toggled (GtkToggleButton *button, GeditPageSetupDialog *dlg)
{
	gedit_debug (DEBUG_PRINT, "");

	g_return_if_fail (button == GTK_TOGGLE_BUTTON (dlg->syntax_checkbutton));
	
	gedit_prefs_manager_set_print_syntax_hl (gtk_toggle_button_get_active (button));
}

static void
page_header_checkbutton_toggled (GtkToggleButton *button, GeditPageSetupDialog *dlg)
{
	gedit_debug (DEBUG_PRINT, "");

	g_return_if_fail (button == GTK_TOGGLE_BUTTON (dlg->page_header_checkbutton));
	
	gedit_prefs_manager_set_print_header (gtk_toggle_button_get_active (button));
}

static void
line_numbers_checkbutton_toggled (GtkToggleButton *button, GeditPageSetupDialog *dlg)
{
	gedit_debug (DEBUG_PRINT, "");

	g_return_if_fail (button == GTK_TOGGLE_BUTTON (dlg->line_numbers_checkbutton));
	
	if (gtk_toggle_button_get_active (button))
	{
		gtk_widget_set_sensitive (dlg->line_numbers_hbox, 
					  gedit_prefs_manager_print_line_numbers_can_set ());
		
		gedit_prefs_manager_set_print_line_numbers (
			MAX (1, gtk_spin_button_get_value_as_int (
			   GTK_SPIN_BUTTON (dlg->line_numbers_spinbutton))));
	}
	else	
	{
		gtk_widget_set_sensitive (dlg->line_numbers_hbox, FALSE);
		gedit_prefs_manager_set_print_line_numbers (0);
	}
}

static void
line_numbers_spinbutton_value_changed (GtkSpinButton *spin_button, GeditPageSetupDialog *dlg)
{
	g_return_if_fail (spin_button == GTK_SPIN_BUTTON (dlg->line_numbers_spinbutton));

	old_line_numbers_value = MAX (1, gtk_spin_button_get_value_as_int (spin_button));

	gedit_prefs_manager_set_print_line_numbers (old_line_numbers_value);
}

static void
wrap_mode_checkbutton_toggled (GtkToggleButton *button, GeditPageSetupDialog *dlg)
{
	if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dlg->text_wrapping_checkbutton)))
	{
		gedit_prefs_manager_set_print_wrap_mode (GTK_WRAP_NONE);
		
		gtk_widget_set_sensitive (dlg->do_not_split_checkbutton, 
					  FALSE);
		gtk_toggle_button_set_inconsistent (
			GTK_TOGGLE_BUTTON (dlg->do_not_split_checkbutton), TRUE);

	}
	else
	{
		gtk_widget_set_sensitive (dlg->do_not_split_checkbutton, 
					  TRUE);

		gtk_toggle_button_set_inconsistent (
			GTK_TOGGLE_BUTTON (dlg->do_not_split_checkbutton), FALSE);


		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dlg->do_not_split_checkbutton)))
		{
			split_button_state = TRUE;
			
			gedit_prefs_manager_set_print_wrap_mode (GTK_WRAP_WORD);
		}
		else
		{
			split_button_state = FALSE;
			
			gedit_prefs_manager_set_print_wrap_mode (GTK_WRAP_CHAR);
		}
			
	}
}

static void
setup_general_page (GeditPageSetupDialog *dialog)
{
	gint line_numbers;
	gboolean can_set;
	GtkWrapMode wrap_mode;
	
	/* Print syntax */
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->syntax_checkbutton),
				      gedit_prefs_manager_get_print_syntax_hl ());
	gtk_widget_set_sensitive (dialog->syntax_checkbutton,
				  gedit_prefs_manager_print_syntax_hl_can_set ());
	 
	/* Print page headers */
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->page_header_checkbutton),
				      gedit_prefs_manager_get_print_header ());
	gtk_widget_set_sensitive (dialog->page_header_checkbutton,
				  gedit_prefs_manager_print_header_can_set ());
	
	/* Line numbers */
	line_numbers =  gedit_prefs_manager_get_print_line_numbers ();
	can_set = gedit_prefs_manager_print_line_numbers_can_set ();
	
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->line_numbers_checkbutton),
				      line_numbers > 0);
	gtk_widget_set_sensitive (dialog->line_numbers_checkbutton,
				  can_set);

	if (line_numbers > 0)
	{
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (dialog->line_numbers_spinbutton),
					   (guint) line_numbers);
		gtk_widget_set_sensitive (dialog->line_numbers_hbox, 
					  can_set);	
	}
	else
	{
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (dialog->line_numbers_spinbutton),
					   (guint)old_line_numbers_value);
		gtk_widget_set_sensitive (dialog->line_numbers_hbox, FALSE);
	}

	/* Text wrapping */
	wrap_mode = gedit_prefs_manager_get_print_wrap_mode ();

	switch (wrap_mode )
	{
		case GTK_WRAP_WORD:
			gtk_toggle_button_set_active (
				GTK_TOGGLE_BUTTON (dialog->text_wrapping_checkbutton), TRUE);
			gtk_toggle_button_set_active (
				GTK_TOGGLE_BUTTON (dialog->do_not_split_checkbutton), TRUE);
			break;
		case GTK_WRAP_CHAR:
			gtk_toggle_button_set_active (
				GTK_TOGGLE_BUTTON (dialog->text_wrapping_checkbutton), TRUE);
			gtk_toggle_button_set_active (
				GTK_TOGGLE_BUTTON (dialog->do_not_split_checkbutton), FALSE);
			break;
		default:
			gtk_toggle_button_set_active (
				GTK_TOGGLE_BUTTON (dialog->text_wrapping_checkbutton), FALSE);
			gtk_toggle_button_set_active (
				GTK_TOGGLE_BUTTON (dialog->do_not_split_checkbutton), 
				split_button_state);
			gtk_toggle_button_set_inconsistent (
				GTK_TOGGLE_BUTTON (dialog->do_not_split_checkbutton), TRUE);

	}

	can_set = gedit_prefs_manager_print_wrap_mode_can_set ();

	gtk_widget_set_sensitive (dialog->text_wrapping_checkbutton, 
				  can_set);

	gtk_widget_set_sensitive (dialog->do_not_split_checkbutton, 
				  can_set && (wrap_mode != GTK_WRAP_NONE));

	g_signal_connect (G_OBJECT (dialog->syntax_checkbutton), "toggled", 
			  G_CALLBACK (syntax_checkbutton_toggled), dialog);
	
	g_signal_connect (G_OBJECT (dialog->page_header_checkbutton), "toggled", 
			  G_CALLBACK (page_header_checkbutton_toggled), dialog);

	g_signal_connect (G_OBJECT (dialog->line_numbers_checkbutton), "toggled", 
			  G_CALLBACK (line_numbers_checkbutton_toggled), dialog);

	g_signal_connect (G_OBJECT (dialog->line_numbers_spinbutton), "value_changed",
			  G_CALLBACK (line_numbers_spinbutton_value_changed), dialog);

	g_signal_connect (G_OBJECT (dialog->text_wrapping_checkbutton), "toggled", 
			  G_CALLBACK (wrap_mode_checkbutton_toggled), dialog);
	
	g_signal_connect (G_OBJECT (dialog->do_not_split_checkbutton), "toggled", 
			  G_CALLBACK (wrap_mode_checkbutton_toggled), dialog);
	
}

#define DEFAULT_FONT_SIZE 10

/* TODO: monitor changes of the font size - Paolo */

static gint 
get_desktop_default_font_size ()
{
	GConfClient *gconf_client = NULL;
	gint res;
	gchar *font;
	
	PangoFontDescription *desc;

	gedit_debug (DEBUG_PRINT, "");

	gconf_client = gconf_client_get_default ();
	if (gconf_client == NULL)
	{
		return DEFAULT_FONT_SIZE;
	}

	font = gconf_client_get_string (gconf_client,
				   	"/desktop/gnome/interface/font_name",
				   	NULL);
	if (font == NULL)
		return DEFAULT_FONT_SIZE;
	
	gedit_debug (DEBUG_PRINT, "Font: %s", font);

	desc = pango_font_description_from_string (font);

	g_free (font);
	g_return_val_if_fail (desc != NULL, DEFAULT_FONT_SIZE);

	res = pango_font_description_get_size (desc) / PANGO_SCALE;

	pango_font_description_free (desc);

	g_object_unref (gconf_client);

	gedit_debug (DEBUG_PRINT, "Size: %d", res);

	return res;
}

static void
restore_button_clicked (GtkButton *button, GeditPageSetupDialog *dlg)
{
	g_return_if_fail (dlg->body_fontpicker != NULL);
	g_return_if_fail (dlg->headers_fontpicker != NULL);
	g_return_if_fail (dlg->numbers_fontpicker != NULL);

	if (gedit_prefs_manager_print_font_body_can_set ())
	{
		const gchar* font = gedit_prefs_manager_get_default_print_font_body ();

		gnome_print_font_picker_set_font_name (
				GNOME_PRINT_FONT_PICKER (dlg->body_fontpicker),
				font);
		
		gedit_prefs_manager_set_print_font_body (font);
	}
	
	if (gedit_prefs_manager_print_font_header_can_set ())
	{
		const gchar* font = gedit_prefs_manager_get_default_print_font_header ();

		gnome_print_font_picker_set_font_name (
				GNOME_PRINT_FONT_PICKER (dlg->headers_fontpicker),
				font);
		
		gedit_prefs_manager_set_print_font_header (font);
	}
		
	if (gedit_prefs_manager_print_font_numbers_can_set ())
	{
		const gchar* font = gedit_prefs_manager_get_default_print_font_numbers ();
		
		gnome_print_font_picker_set_font_name (
				GNOME_PRINT_FONT_PICKER (dlg->numbers_fontpicker),
				font);
		
		gedit_prefs_manager_set_print_font_numbers (font);
	}
}

static void
body_font_picker_font_set (GnomePrintFontPicker *gfp, const gchar *font_name, 
		GeditPageSetupDialog *dlg)
{
	gedit_debug (DEBUG_PRINT, "");

	g_return_if_fail (gfp == GNOME_PRINT_FONT_PICKER (dlg->body_fontpicker));
	g_return_if_fail (font_name != NULL);

	gedit_prefs_manager_set_print_font_body (font_name);
}

static void
headers_font_picker_font_set (GnomePrintFontPicker *gfp, const gchar *font_name, 
		GeditPageSetupDialog *dlg)
{
	gedit_debug (DEBUG_PRINT, "");

	g_return_if_fail (gfp == GNOME_PRINT_FONT_PICKER (dlg->headers_fontpicker));
	g_return_if_fail (font_name != NULL);

	gedit_prefs_manager_set_print_font_header (font_name);
}

static void
numbers_font_picker_font_set (GnomePrintFontPicker *gfp, const gchar *font_name, 
		GeditPageSetupDialog *dlg)
{
	gedit_debug (DEBUG_PRINT, "");

	g_return_if_fail (gfp == GNOME_PRINT_FONT_PICKER (dlg->numbers_fontpicker));
	g_return_if_fail (font_name != NULL);

	gedit_prefs_manager_set_print_font_numbers (font_name);
}

static void
setup_font_page (GeditPageSetupDialog *dlg)
{
	gboolean can_set;
	gchar* font;

	gedit_debug (DEBUG_PRINT, "");

	/* Body font picker */
	dlg->body_fontpicker = gnome_print_font_picker_new ();

	gnome_print_font_picker_set_mode (
			GNOME_PRINT_FONT_PICKER (dlg->body_fontpicker), 
			GNOME_PRINT_FONT_PICKER_MODE_FONT_INFO);
	
	gnome_print_font_picker_fi_set_show_size (
			GNOME_PRINT_FONT_PICKER (dlg->body_fontpicker), TRUE);

	gnome_print_font_picker_fi_set_use_font_in_label (
			GNOME_PRINT_FONT_PICKER (dlg->body_fontpicker), 
			TRUE, 
			get_desktop_default_font_size ());

	gtk_table_attach_defaults (GTK_TABLE (dlg->fonts_table), 
			dlg->body_fontpicker, 1, 2, 0, 1);
	
	/* Headers font picker */
	dlg->headers_fontpicker = gnome_print_font_picker_new ();
	gnome_print_font_picker_set_mode (
			GNOME_PRINT_FONT_PICKER (dlg->headers_fontpicker), 
			GNOME_PRINT_FONT_PICKER_MODE_FONT_INFO);
	
	gnome_print_font_picker_fi_set_show_size (
			GNOME_PRINT_FONT_PICKER (dlg->headers_fontpicker), TRUE);
	
	gnome_print_font_picker_fi_set_use_font_in_label (
			GNOME_PRINT_FONT_PICKER (dlg->headers_fontpicker), 
			TRUE, 
			get_desktop_default_font_size ());

	gtk_table_attach_defaults (GTK_TABLE (dlg->fonts_table), 
			dlg->headers_fontpicker, 1, 2, 2, 3);

	/* Numbers font picker */
	dlg->numbers_fontpicker = gnome_print_font_picker_new ();
	gnome_print_font_picker_set_mode (
			GNOME_PRINT_FONT_PICKER (dlg->numbers_fontpicker), 
			GNOME_PRINT_FONT_PICKER_MODE_FONT_INFO);
	
	gnome_print_font_picker_fi_set_show_size (
			GNOME_PRINT_FONT_PICKER (dlg->numbers_fontpicker), TRUE);
	
	gnome_print_font_picker_fi_set_use_font_in_label (
			GNOME_PRINT_FONT_PICKER (dlg->numbers_fontpicker), 
			TRUE, 
			get_desktop_default_font_size ());

	gtk_table_attach_defaults (GTK_TABLE (dlg->fonts_table), 
			dlg->numbers_fontpicker, 1, 2, 1, 2);

	gtk_label_set_mnemonic_widget (GTK_LABEL (dlg->body_font_label), 
				       dlg->body_fontpicker);
	
	gtk_label_set_mnemonic_widget (GTK_LABEL (dlg->headers_font_label), 
				       dlg->headers_fontpicker);

	gtk_label_set_mnemonic_widget (GTK_LABEL (dlg->numbers_font_label), 
				       dlg->numbers_fontpicker);

	gedit_utils_set_atk_name_description (dlg->body_fontpicker, _("Body font picker"),
		_("Push this button to select the font to be used to print the body of the document"));
	gedit_utils_set_atk_name_description (dlg->headers_fontpicker, _("Page headers font picker"),
		_("Push this button to select the font to be used to print the page headers"));
	gedit_utils_set_atk_name_description (dlg->numbers_fontpicker, _("Line numbers font picker"),
		_("Push this button to select the font to be used to print line numbers"));

	gedit_utils_set_atk_relation (dlg->body_fontpicker, dlg->body_font_label, 
							ATK_RELATION_LABELLED_BY);
	gedit_utils_set_atk_relation (dlg->headers_fontpicker, dlg->headers_font_label, 
							ATK_RELATION_LABELLED_BY);
	gedit_utils_set_atk_relation (dlg->numbers_fontpicker, dlg->numbers_font_label, 
							ATK_RELATION_LABELLED_BY);

	/* Set initial values */
	font = gedit_prefs_manager_get_print_font_body ();
	g_return_if_fail (font);;

	gnome_print_font_picker_set_font_name (
			GNOME_PRINT_FONT_PICKER (dlg->body_fontpicker),
			font);
	
	g_free (font);

	font = gedit_prefs_manager_get_print_font_header ();
	g_return_if_fail (font);

	gnome_print_font_picker_set_font_name (
			GNOME_PRINT_FONT_PICKER (dlg->headers_fontpicker),
			font);

	g_free (font);

	font = gedit_prefs_manager_get_print_font_numbers ();
	g_return_if_fail (font);
	
	gnome_print_font_picker_set_font_name (
			GNOME_PRINT_FONT_PICKER (dlg->numbers_fontpicker),
			font);

	g_free (font);

	/* Set widgets sensitivity */
	can_set = gedit_prefs_manager_print_font_body_can_set ();
	gtk_widget_set_sensitive (dlg->body_fontpicker, can_set);
	gtk_widget_set_sensitive (dlg->body_font_label, can_set);
		  
	can_set = gedit_prefs_manager_print_font_header_can_set ();
	gtk_widget_set_sensitive (dlg->headers_fontpicker, can_set);
	gtk_widget_set_sensitive (dlg->headers_font_label, can_set);

	can_set = gedit_prefs_manager_print_font_numbers_can_set ();
	gtk_widget_set_sensitive (dlg->numbers_fontpicker, can_set);
	gtk_widget_set_sensitive (dlg->numbers_font_label, can_set);
	
	/* Connect signals */
	g_signal_connect (G_OBJECT (dlg->restore_button), "clicked",
			  G_CALLBACK (restore_button_clicked),
			  dlg);

	g_signal_connect (G_OBJECT (dlg->body_fontpicker), "font_set", 
			  G_CALLBACK (body_font_picker_font_set), 
			  dlg);

	g_signal_connect (G_OBJECT (dlg->headers_fontpicker), "font_set", 
			  G_CALLBACK (headers_font_picker_font_set), 
			  dlg);

	g_signal_connect (G_OBJECT (dlg->numbers_fontpicker), "font_set", 
			  G_CALLBACK (numbers_font_picker_font_set), 
			  dlg);
}

static GeditPageSetupDialog *
page_setup_get_dialog (GtkWindow *parent)
{
	static GeditPageSetupDialog *dialog = NULL;
	GladeXML *gui;
	
	gedit_debug (DEBUG_PRINT, "");

	if (dialog != NULL)
	{
		gtk_window_set_transient_for (GTK_WINDOW (dialog->dialog),
					      parent);
		gtk_window_present (GTK_WINDOW (dialog->dialog));

		return dialog;
	}

	gui = glade_xml_new (GEDIT_GLADEDIR "page-setup-dialog.glade2",
			     "dialog", NULL);
	if (!gui)
	{
		gedit_warning (parent,
			       MISSING_FILE,
			       GEDIT_GLADEDIR "page-setup-dialog.glade2");
		return NULL;
	}

	dialog = g_new0 (GeditPageSetupDialog, 1);

	dialog->dialog = glade_xml_get_widget (gui, "dialog");
	
	dialog->syntax_checkbutton = glade_xml_get_widget (gui, "syntax_checkbutton");
	dialog->page_header_checkbutton = glade_xml_get_widget (gui, "page_header_checkbutton");
	
	dialog->line_numbers_checkbutton = glade_xml_get_widget (gui, "line_numbers_checkbutton");
	dialog->line_numbers_hbox = glade_xml_get_widget (gui, "line_numbers_hbox");
	dialog->line_numbers_spinbutton = glade_xml_get_widget (gui, "line_numbers_spinbutton");

	dialog->text_wrapping_checkbutton = glade_xml_get_widget (gui, "text_wrapping_checkbutton");
	dialog->do_not_split_checkbutton = glade_xml_get_widget (gui, "do_not_split_checkbutton");

	dialog->fonts_table = glade_xml_get_widget (gui, "fonts_table");
	
	dialog->body_font_label = glade_xml_get_widget (gui, "body_font_label");
	dialog->headers_font_label = glade_xml_get_widget (gui,	"headers_font_label");
	dialog->numbers_font_label = glade_xml_get_widget (gui,	"numbers_font_label");

	dialog->restore_button = glade_xml_get_widget (gui, "restore_button");	

	if (!dialog->dialog 			|| 
	    !dialog->syntax_checkbutton		||
	    !dialog->page_header_checkbutton 	||
	    !dialog->line_numbers_checkbutton 	||
	    !dialog->line_numbers_hbox 		||
	    !dialog->line_numbers_spinbutton 	||
	    !dialog->text_wrapping_checkbutton	||
	    !dialog->do_not_split_checkbutton 	||
	    !dialog->fonts_table 		||
	    !dialog->body_font_label		||
	    !dialog->headers_font_label		||
	    !dialog->numbers_font_label		||
	    !dialog->restore_button)
	{
		gedit_warning (parent,
			       MISSING_WIDGETS,
			       GEDIT_GLADEDIR "page-setup-dialog.glade2");

		if (!dialog->dialog)
			gtk_widget_destroy (dialog->dialog);
		
		g_object_unref (gui);
		g_free (dialog);
		dialog = NULL;

		return NULL;
	}

	setup_general_page (dialog);
	setup_font_page (dialog);
	
	gtk_window_set_transient_for (GTK_WINDOW (dialog->dialog),
				      parent);

	g_signal_connect (G_OBJECT (dialog->dialog), "destroy",
			  G_CALLBACK (dialog_destroyed), &dialog);

	g_signal_connect (G_OBJECT (dialog->dialog), "response",
			  G_CALLBACK (dialog_response_handler), dialog);

	g_object_unref (gui);

	gtk_window_set_resizable (GTK_WINDOW (dialog->dialog), FALSE);

	return dialog;

}

void
gedit_show_page_setup_dialog (GtkWindow *parent)
{
	GeditPageSetupDialog *dialog;

	gedit_debug (DEBUG_PRINT, "");

	g_return_if_fail (parent != NULL);

	dialog = page_setup_get_dialog (parent);
	if (!dialog)
		return;

	if (!GTK_WIDGET_VISIBLE (dialog->dialog))
		gtk_widget_show_all (dialog->dialog);
}

