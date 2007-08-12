/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-page-setup-dialog.c
 * This file is part of gedit
 *
 * Copyright (C) 2003-2005 Paolo Maggi 
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
 *
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <glade/glade-xml.h>
#include <gconf/gconf-client.h>

#include <gedit/gedit-prefs-manager.h>

#include "gedit-page-setup-dialog.h"
#include "gedit-utils.h"
#include "gedit-debug.h"
#include "gedit-help.h"

/*
 * gedit-page-setup dialog is a singleton since we don't
 * want two dialogs showing an inconsistent state of the
 * preferences.
 * When gedit_show_page_setup_dialog is called and there
 * is already a page setup dialog open, it is reparented
 * and shown.
 */

static GtkWidget *page_setup_dialog = NULL;


#define GEDIT_PAGE_SETUP_DIALOG_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), GEDIT_TYPE_PAGE_SETUP_DIALOG, GeditPageSetupDialogPrivate))


struct _GeditPageSetupDialogPrivate
{
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

	GtkWidget *body_fontbutton;
	GtkWidget *headers_fontbutton;
	GtkWidget *numbers_fontbutton;

	GtkWidget *restore_button;
};


G_DEFINE_TYPE(GeditPageSetupDialog, gedit_page_setup_dialog, GTK_TYPE_DIALOG)


/* these are used to keep a consistent init value
 * of preferences not currently stored in gconf
 * across multiple instances.
 */
static gboolean split_button_state = TRUE;
static gint old_line_numbers_value = 1;


static void 
gedit_page_setup_dialog_class_init (GeditPageSetupDialogClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
					      								      
	g_type_class_add_private (object_class, sizeof (GeditPageSetupDialogPrivate));
}

static void
dialog_response_handler (GeditPageSetupDialog *dlg,
			 gint                  res_id,
			 gpointer              data)
{
	switch (res_id)
	{
		case GTK_RESPONSE_HELP:
			gedit_help_display (GTK_WINDOW (dlg),
					    "gedit.xml",
					    "gedit-page-setup");

			g_signal_stop_emission_by_name (dlg, "response");

			break;

		default:
			gtk_widget_destroy (GTK_WIDGET(dlg));
	}
}

static void
syntax_checkbutton_toggled (GtkToggleButton      *button,
			    GeditPageSetupDialog *dlg)
{
	gedit_debug (DEBUG_PRINT);

	g_return_if_fail (button == GTK_TOGGLE_BUTTON (dlg->priv->syntax_checkbutton));

	gedit_prefs_manager_set_print_syntax_hl (gtk_toggle_button_get_active (button));
}

static void
page_header_checkbutton_toggled (GtkToggleButton      *button,
				 GeditPageSetupDialog *dlg)
{
	gedit_debug (DEBUG_PRINT);

	g_return_if_fail (button == GTK_TOGGLE_BUTTON (dlg->priv->page_header_checkbutton));
	
	gedit_prefs_manager_set_print_header (gtk_toggle_button_get_active (button));
}

static void
line_numbers_checkbutton_toggled (GtkToggleButton      *button,
				  GeditPageSetupDialog *dlg)
{
	gedit_debug (DEBUG_PRINT);

	g_return_if_fail (button == GTK_TOGGLE_BUTTON (dlg->priv->line_numbers_checkbutton));

	if (gtk_toggle_button_get_active (button))
	{
		gtk_widget_set_sensitive (dlg->priv->line_numbers_hbox, 
					  gedit_prefs_manager_print_line_numbers_can_set ());

		gedit_prefs_manager_set_print_line_numbers (
			MAX (1, gtk_spin_button_get_value_as_int (
			   GTK_SPIN_BUTTON (dlg->priv->line_numbers_spinbutton))));
	}
	else
	{
		gtk_widget_set_sensitive (dlg->priv->line_numbers_hbox, FALSE);
		gedit_prefs_manager_set_print_line_numbers (0);
	}
}

static void
line_numbers_spinbutton_value_changed (GtkSpinButton        *spin_button,
				       GeditPageSetupDialog *dlg)
{
	g_return_if_fail (spin_button == GTK_SPIN_BUTTON (dlg->priv->line_numbers_spinbutton));

	old_line_numbers_value = MAX (1, gtk_spin_button_get_value_as_int (spin_button));

	gedit_prefs_manager_set_print_line_numbers (old_line_numbers_value);
}

static void
wrap_mode_checkbutton_toggled (GtkToggleButton      *button,
			       GeditPageSetupDialog *dlg)
{
	if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dlg->priv->text_wrapping_checkbutton)))
	{
		gedit_prefs_manager_set_print_wrap_mode (GTK_WRAP_NONE);
		
		gtk_widget_set_sensitive (dlg->priv->do_not_split_checkbutton, 
					  FALSE);
		gtk_toggle_button_set_inconsistent (
			GTK_TOGGLE_BUTTON (dlg->priv->do_not_split_checkbutton), TRUE);
	}
	else
	{
		gtk_widget_set_sensitive (dlg->priv->do_not_split_checkbutton, TRUE);

		gtk_toggle_button_set_inconsistent (
			GTK_TOGGLE_BUTTON (dlg->priv->do_not_split_checkbutton), FALSE);

		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dlg->priv->do_not_split_checkbutton)))
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
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->priv->syntax_checkbutton),
				      gedit_prefs_manager_get_print_syntax_hl ());
	gtk_widget_set_sensitive (dialog->priv->syntax_checkbutton,
				  gedit_prefs_manager_print_syntax_hl_can_set ());
	 
	/* Print page headers */
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->priv->page_header_checkbutton),
				      gedit_prefs_manager_get_print_header ());
	gtk_widget_set_sensitive (dialog->priv->page_header_checkbutton,
				  gedit_prefs_manager_print_header_can_set ());
	
	/* Line numbers */
	line_numbers =  gedit_prefs_manager_get_print_line_numbers ();
	can_set = gedit_prefs_manager_print_line_numbers_can_set ();
	
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->priv->line_numbers_checkbutton),
				      line_numbers > 0);
	gtk_widget_set_sensitive (dialog->priv->line_numbers_checkbutton,
				  can_set);

	if (line_numbers > 0)
	{
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (dialog->priv->line_numbers_spinbutton),
					   (guint) line_numbers);
		gtk_widget_set_sensitive (dialog->priv->line_numbers_hbox, 
					  can_set);	
	}
	else
	{
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (dialog->priv->line_numbers_spinbutton),
					   (guint)old_line_numbers_value);
		gtk_widget_set_sensitive (dialog->priv->line_numbers_hbox, FALSE);
	}

	/* Text wrapping */
	wrap_mode = gedit_prefs_manager_get_print_wrap_mode ();

	switch (wrap_mode )
	{
	case GTK_WRAP_WORD:
		gtk_toggle_button_set_active (
			GTK_TOGGLE_BUTTON (dialog->priv->text_wrapping_checkbutton), TRUE);
		gtk_toggle_button_set_active (
			GTK_TOGGLE_BUTTON (dialog->priv->do_not_split_checkbutton), TRUE);
		break;
	case GTK_WRAP_CHAR:
		gtk_toggle_button_set_active (
			GTK_TOGGLE_BUTTON (dialog->priv->text_wrapping_checkbutton), TRUE);
		gtk_toggle_button_set_active (
			GTK_TOGGLE_BUTTON (dialog->priv->do_not_split_checkbutton), FALSE);
		break;
	default:
		gtk_toggle_button_set_active (
			GTK_TOGGLE_BUTTON (dialog->priv->text_wrapping_checkbutton), FALSE);
		gtk_toggle_button_set_active (
			GTK_TOGGLE_BUTTON (dialog->priv->do_not_split_checkbutton), 
			split_button_state);
		gtk_toggle_button_set_inconsistent (
			GTK_TOGGLE_BUTTON (dialog->priv->do_not_split_checkbutton), TRUE);
	}

	can_set = gedit_prefs_manager_print_wrap_mode_can_set ();

	gtk_widget_set_sensitive (dialog->priv->text_wrapping_checkbutton, 
				  can_set);

	gtk_widget_set_sensitive (dialog->priv->do_not_split_checkbutton, 
				  can_set && (wrap_mode != GTK_WRAP_NONE));

	g_signal_connect (dialog->priv->syntax_checkbutton,
			  "toggled",
			  G_CALLBACK (syntax_checkbutton_toggled),
			  dialog);
	g_signal_connect (dialog->priv->page_header_checkbutton,
			  "toggled",
			  G_CALLBACK (page_header_checkbutton_toggled),
			  dialog);
	g_signal_connect (dialog->priv->line_numbers_checkbutton,
			  "toggled",
			  G_CALLBACK (line_numbers_checkbutton_toggled),
			  dialog);
	g_signal_connect (dialog->priv->line_numbers_spinbutton,
			  "value_changed",
			  G_CALLBACK (line_numbers_spinbutton_value_changed),
			  dialog);
	g_signal_connect (dialog->priv->text_wrapping_checkbutton,
			  "toggled",
			  G_CALLBACK (wrap_mode_checkbutton_toggled),
			  dialog);
	g_signal_connect (dialog->priv->do_not_split_checkbutton,
			  "toggled",
			  G_CALLBACK (wrap_mode_checkbutton_toggled),
			  dialog);
}

static void
restore_button_clicked (GtkButton            *button,
			GeditPageSetupDialog *dlg)
{
	g_return_if_fail (dlg->priv->body_fontbutton != NULL);
	g_return_if_fail (dlg->priv->headers_fontbutton != NULL);
	g_return_if_fail (dlg->priv->numbers_fontbutton != NULL);

	if (gedit_prefs_manager_print_font_body_can_set ())
	{
		const gchar* font = gedit_prefs_manager_get_default_print_font_body ();

		gtk_font_button_set_font_name (
				GTK_FONT_BUTTON (dlg->priv->body_fontbutton),
				font);

		gedit_prefs_manager_set_print_font_body (font);
	}
	
	if (gedit_prefs_manager_print_font_header_can_set ())
	{
		const gchar *font;

		font = gedit_prefs_manager_get_default_print_font_header ();

		gtk_font_button_set_font_name (
				GTK_FONT_BUTTON (dlg->priv->headers_fontbutton),
				font);

		gedit_prefs_manager_set_print_font_header (font);
	}

	if (gedit_prefs_manager_print_font_numbers_can_set ())
	{
		const gchar *font;

		font = gedit_prefs_manager_get_default_print_font_numbers ();

		gtk_font_button_set_font_name (
				GTK_FONT_BUTTON (dlg->priv->numbers_fontbutton),
				font);

		gedit_prefs_manager_set_print_font_numbers (font);
	}
}

static void
body_font_button_font_set (GtkFontButton        *fb,
			   GeditPageSetupDialog *dlg)
{
	gedit_debug (DEBUG_PRINT);

	g_return_if_fail (fb == GTK_FONT_BUTTON (dlg->priv->body_fontbutton));

	gedit_prefs_manager_set_print_font_body (gtk_font_button_get_font_name (fb));
}

static void
headers_font_button_font_set (GtkFontButton        *fb,
			      GeditPageSetupDialog *dlg)
{
	gedit_debug (DEBUG_PRINT);

	g_return_if_fail (fb == GTK_FONT_BUTTON (dlg->priv->headers_fontbutton));

	gedit_prefs_manager_set_print_font_header (gtk_font_button_get_font_name (fb));
}

static void
numbers_font_button_font_set (GtkFontButton        *fb,
			      GeditPageSetupDialog *dlg)
{
	gedit_debug (DEBUG_PRINT);

	g_return_if_fail (fb == GTK_FONT_BUTTON (dlg->priv->numbers_fontbutton));

	gedit_prefs_manager_set_print_font_numbers (gtk_font_button_get_font_name (fb));
}

static GtkWidget *
font_button_new (void)
{
	GtkWidget *button;

	button = gtk_font_button_new ();

	gtk_font_button_set_use_font (GTK_FONT_BUTTON (button), TRUE);
	gtk_font_button_set_show_style (GTK_FONT_BUTTON (button), FALSE);
	gtk_font_button_set_show_size (GTK_FONT_BUTTON (button), TRUE);

	gtk_widget_show (button);

	return button;
}

static void
setup_font_page (GeditPageSetupDialog *dlg)
{
	gboolean can_set;
	gchar* font;

	/* Body font button */
	dlg->priv->body_fontbutton = font_button_new ();

	gtk_table_attach_defaults (GTK_TABLE (dlg->priv->fonts_table), 
			dlg->priv->body_fontbutton, 1, 2, 0, 1);
	
	/* Headers font button */
	dlg->priv->headers_fontbutton = font_button_new ();

	gtk_table_attach_defaults (GTK_TABLE (dlg->priv->fonts_table), 
			dlg->priv->headers_fontbutton, 1, 2, 2, 3);

	/* Numbers font button */
	dlg->priv->numbers_fontbutton = font_button_new ();

	gtk_table_attach_defaults (GTK_TABLE (dlg->priv->fonts_table), 
			dlg->priv->numbers_fontbutton, 1, 2, 1, 2);

	gtk_label_set_mnemonic_widget (GTK_LABEL (dlg->priv->body_font_label), 
				       dlg->priv->body_fontbutton);
	gtk_label_set_mnemonic_widget (GTK_LABEL (dlg->priv->headers_font_label), 
				       dlg->priv->headers_fontbutton);
	gtk_label_set_mnemonic_widget (GTK_LABEL (dlg->priv->numbers_font_label), 
				       dlg->priv->numbers_fontbutton);

	/* Set initial values */
	font = gedit_prefs_manager_get_print_font_body ();
	g_return_if_fail (font);

	gtk_font_button_set_font_name (
			GTK_FONT_BUTTON (dlg->priv->body_fontbutton),
			font);
	
	g_free (font);

	font = gedit_prefs_manager_get_print_font_header ();
	g_return_if_fail (font);

	gtk_font_button_set_font_name (
			GTK_FONT_BUTTON (dlg->priv->headers_fontbutton),
			font);

	g_free (font);

	font = gedit_prefs_manager_get_print_font_numbers ();
	g_return_if_fail (font);
	
	gtk_font_button_set_font_name (
			GTK_FONT_BUTTON (dlg->priv->numbers_fontbutton),
			font);

	g_free (font);

	can_set = gedit_prefs_manager_print_font_body_can_set ();
	gtk_widget_set_sensitive (dlg->priv->body_fontbutton, can_set);
	gtk_widget_set_sensitive (dlg->priv->body_font_label, can_set);

	can_set = gedit_prefs_manager_print_font_header_can_set ();
	gtk_widget_set_sensitive (dlg->priv->headers_fontbutton, can_set);
	gtk_widget_set_sensitive (dlg->priv->headers_font_label, can_set);

	can_set = gedit_prefs_manager_print_font_numbers_can_set ();
	gtk_widget_set_sensitive (dlg->priv->numbers_fontbutton, can_set);
	gtk_widget_set_sensitive (dlg->priv->numbers_font_label, can_set);

	g_signal_connect (dlg->priv->restore_button,
			  "clicked",
			  G_CALLBACK (restore_button_clicked),
			  dlg);
	g_signal_connect (dlg->priv->body_fontbutton,
			  "font_set", 
			  G_CALLBACK (body_font_button_font_set), 
			  dlg);
	g_signal_connect (dlg->priv->headers_fontbutton,
			  "font_set", 
			  G_CALLBACK (headers_font_button_font_set), 
			  dlg);
	g_signal_connect (dlg->priv->numbers_fontbutton,
			  "font_set", 
			  G_CALLBACK (numbers_font_button_font_set), 
			  dlg);
}

static void
gedit_page_setup_dialog_init (GeditPageSetupDialog *dlg)
{
	GtkWidget *content = NULL;
	GtkWidget *error_widget = NULL;
	gboolean ret = FALSE;

	dlg->priv = GEDIT_PAGE_SETUP_DIALOG_GET_PRIVATE (dlg);

	gtk_dialog_add_buttons (GTK_DIALOG (dlg),
				GTK_STOCK_CLOSE,
				GTK_RESPONSE_CLOSE,
				GTK_STOCK_HELP,
				GTK_RESPONSE_HELP,
				NULL);

	gtk_window_set_title (GTK_WINDOW (dlg), _("Page Setup"));
	gtk_window_set_resizable (GTK_WINDOW (dlg), FALSE);
	gtk_dialog_set_has_separator (GTK_DIALOG (dlg), FALSE);
	gtk_window_set_destroy_with_parent (GTK_WINDOW (dlg), TRUE);
	
	/* HIG defaults */
	gtk_container_set_border_width (GTK_CONTAINER (dlg), 5);
	gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (dlg)->vbox), 2); /* 2 * 5 + 2 = 12 */
	gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG (dlg)->action_area), 5);
	gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (dlg)->action_area), 6);		

	gtk_dialog_set_default_response (GTK_DIALOG (dlg),
					 GTK_RESPONSE_CLOSE);

	g_signal_connect (G_OBJECT (dlg),
			  "response",
 			  G_CALLBACK (dialog_response_handler),
			  NULL);

	ret = gedit_utils_get_glade_widgets (GEDIT_GLADEDIR "gedit-page-setup-dialog.glade",
					     "notebook1",
					     &error_widget,
					     "notebook1", &content,
					     "syntax_checkbutton", &dlg->priv->syntax_checkbutton,
					     "page_header_checkbutton", &dlg->priv->page_header_checkbutton,
					     "line_numbers_checkbutton", &dlg->priv->line_numbers_checkbutton,
					     "line_numbers_hbox", &dlg->priv->line_numbers_hbox,
					     "line_numbers_spinbutton", &dlg->priv->line_numbers_spinbutton,
					     "text_wrapping_checkbutton", &dlg->priv->text_wrapping_checkbutton,
					     "do_not_split_checkbutton", &dlg->priv->do_not_split_checkbutton,
					     "fonts_table", &dlg->priv->fonts_table,
					     "body_font_label", &dlg->priv->body_font_label,
					     "headers_font_label", &dlg->priv->headers_font_label,
					     "numbers_font_label", &dlg->priv->numbers_font_label,
					     "restore_button", &dlg->priv->restore_button,
					     NULL);

	if (!ret)
	{
		gtk_widget_show (error_widget);
			
		gtk_box_pack_start_defaults (GTK_BOX (GTK_DIALOG (dlg)->vbox),
					     error_widget);
		gtk_container_set_border_width (GTK_CONTAINER (error_widget), 5);			     

		return;
	}

	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox),
			    content, FALSE, FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER (content), 5);			     

	setup_general_page (dlg);
	setup_font_page (dlg);
}

void
gedit_show_page_setup_dialog (GtkWindow *parent)
{
	g_return_if_fail (GTK_IS_WINDOW (parent));

	if (page_setup_dialog == NULL)
	{
		page_setup_dialog = GTK_WIDGET (g_object_new (GEDIT_TYPE_PAGE_SETUP_DIALOG, NULL));
		g_signal_connect (page_setup_dialog,
				  "destroy",
				  G_CALLBACK (gtk_widget_destroyed),
				  &page_setup_dialog);
	}

	if (parent != gtk_window_get_transient_for (GTK_WINDOW (page_setup_dialog)))
	{
		gtk_window_set_transient_for (GTK_WINDOW (page_setup_dialog),
					      parent);
		gtk_window_set_destroy_with_parent (GTK_WINDOW (page_setup_dialog),
						    TRUE);
	}

	gtk_window_present (GTK_WINDOW (page_setup_dialog));
}
