/*
 * gedit-search-dialog.c
 * This file is part of gedit
 *
 * Copyright (C) 2005 Paolo Maggi
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
 * Modified by the gedit Team, 2005. See the AUTHORS file for a 
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
#include <libgnomeui/gnome-entry.h>

#include "gedit-search-dialog.h"
#include "gedit-utils.h"

#define GEDIT_SEARCH_DIALOG_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), \
						GEDIT_TYPE_SEARCH_DIALOG,              \
						GeditSearchDialogPrivate))

struct _GeditSearchDialogPrivate 
{
	gboolean   show_replace;

	GtkWidget *table;
	GtkWidget *search_entry;
	GtkWidget *replace_label;
	GtkWidget *replace_entry;
	GtkWidget *search_list;
	GtkWidget *replace_list;
	GtkWidget *match_case_checkbutton;
	GtkWidget *entire_word_checkbutton;
	GtkWidget *backwards_checkbutton;
	GtkWidget *wrap_around_checkbutton;
	GtkWidget *find_button;
	GtkWidget *replace_button;
	GtkWidget *replace_all_button;

	gboolean   glade_error;
};

G_DEFINE_TYPE(GeditSearchDialog, gedit_search_dialog, GTK_TYPE_DIALOG)

enum
{
	PROP_0,
	PROP_SHOW_REPLACE
};

static void
gedit_search_dialog_set_property (GObject      *object,
				  guint         prop_id,
				  const GValue *value,
				  GParamSpec   *pspec)
{
	GeditSearchDialog *dlg = GEDIT_SEARCH_DIALOG (object);

	switch (prop_id)
	{
		case PROP_SHOW_REPLACE:
			gedit_search_dialog_set_show_replace (dlg,
							      g_value_get_boolean (value));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_search_dialog_get_property (GObject    *object,
				  guint       prop_id,
				  GValue     *value,
				  GParamSpec *pspec)
{
	GeditSearchDialog *dlg = GEDIT_SEARCH_DIALOG (object);

	switch (prop_id)
	{
		case PROP_SHOW_REPLACE:
			g_value_set_boolean (value, dlg->priv->show_replace);
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void 
gedit_search_dialog_class_init (GeditSearchDialogClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->set_property = gedit_search_dialog_set_property;
	object_class->get_property = gedit_search_dialog_get_property;

	g_object_class_install_property (object_class, PROP_SHOW_REPLACE,
					 g_param_spec_boolean ("show-replace",
					 		       "Show Replace",
					 		       "Whether the dialog is used for Search&Replace",
					 		       FALSE,
					 		       G_PARAM_READWRITE));

	g_type_class_add_private (object_class, sizeof (GeditSearchDialogPrivate));
}

static void
insert_text_handler (GtkEditable *editable,
		     const gchar *text,
		     gint         length,
		     gint        *position)
{
	static gboolean insert_text = FALSE;
	gchar *escaped_text;
	gint new_len;

	/* To avoid recursive behavior */
	if (insert_text)
		return;

	escaped_text = gedit_utils_escape_search_text (text);

	new_len = strlen (escaped_text);

	if (new_len == length)
	{
		g_free (escaped_text);
		return;
	}

	insert_text = TRUE;

	g_signal_stop_emission_by_name (editable, "insert_text");

	gtk_editable_insert_text (editable, escaped_text, new_len, position);

	insert_text = FALSE;

	g_free (escaped_text);
}

static void
search_entry_changed (GtkEditable       *editable,
		      GeditSearchDialog *dialog)
{
	const gchar *search_string;

	search_string = gtk_entry_get_text (GTK_ENTRY (dialog->priv->search_entry));
	g_return_if_fail (search_string != NULL);

	if (*search_string != '\0')
	{
		gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog), 
			GEDIT_SEARCH_DIALOG_FIND_RESPONSE, TRUE);
		gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog), 
			GEDIT_SEARCH_DIALOG_REPLACE_ALL_RESPONSE, TRUE);
	}
	else
	{
		gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog), 
			GEDIT_SEARCH_DIALOG_FIND_RESPONSE, FALSE);
		gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog), 
			GEDIT_SEARCH_DIALOG_REPLACE_RESPONSE, FALSE);
		gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog), 
			GEDIT_SEARCH_DIALOG_REPLACE_ALL_RESPONSE, FALSE);
	}
}

static void
search_options_changed (GtkToggleButton   *button,
			GeditSearchDialog *dialog)
{
	/* make sure Replace All is reactivated when the options
	 * become less restrictive */
	if (!gtk_toggle_button_get_active (button))
	{
		gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog),
						   GEDIT_SEARCH_DIALOG_REPLACE_ALL_RESPONSE,
						   TRUE);
	}
}

static void
response_handler (GeditSearchDialog *dialog,
		  gint               response_id,
		  gpointer           data)
{
	const gchar *str;

	switch (response_id)
	{
		case GEDIT_SEARCH_DIALOG_REPLACE_RESPONSE:
		case GEDIT_SEARCH_DIALOG_REPLACE_ALL_RESPONSE:
			str = gtk_entry_get_text (GTK_ENTRY (dialog->priv->replace_entry));
			if (*str != '\0')
				gnome_entry_prepend_history (GNOME_ENTRY (dialog->priv->replace_list),
							     TRUE,
							     str);
			/* fall through, so that we also save the find entry */
		case GEDIT_SEARCH_DIALOG_FIND_RESPONSE:
			str = gtk_entry_get_text (GTK_ENTRY (dialog->priv->search_entry));
			if (*str != '\0')
				gnome_entry_prepend_history (GNOME_ENTRY (dialog->priv->search_list),
							     TRUE,
							     str);
	}
}

static void
show_replace_widgets (GeditSearchDialog *dlg,
		      gboolean           show_replace)
{
	if (show_replace)
	{
		gtk_widget_show (dlg->priv->replace_label);
		gtk_widget_show (dlg->priv->replace_entry);
		gtk_widget_show (dlg->priv->replace_list);
		gtk_widget_show (dlg->priv->replace_all_button);
		gtk_widget_show (dlg->priv->replace_button);

		gtk_table_set_row_spacings (GTK_TABLE (dlg->priv->table), 12);

		gtk_window_set_title (GTK_WINDOW (dlg), _("Replace"));
	}
	else
	{
		gtk_widget_hide (dlg->priv->replace_label);
		gtk_widget_hide (dlg->priv->replace_entry);
		gtk_widget_hide (dlg->priv->replace_list);
		gtk_widget_hide (dlg->priv->replace_all_button);
		gtk_widget_hide (dlg->priv->replace_button);

		gtk_table_set_row_spacings (GTK_TABLE (dlg->priv->table), 0);

		gtk_window_set_title (GTK_WINDOW (dlg), _("Find"));
	}

	gtk_widget_show (dlg->priv->find_button);
}

static void
gedit_search_dialog_init (GeditSearchDialog *dlg)
{
	GtkWidget *content;
	GtkWidget *error_widget;
	gboolean ret;

	dlg->priv = GEDIT_SEARCH_DIALOG_GET_PRIVATE (dlg);

	gtk_window_set_resizable (GTK_WINDOW (dlg), FALSE);
	gtk_dialog_set_has_separator (GTK_DIALOG (dlg), FALSE);
	gtk_window_set_destroy_with_parent (GTK_WINDOW (dlg), TRUE);

	gtk_dialog_add_buttons (GTK_DIALOG (dlg),
				GTK_STOCK_CLOSE, GTK_RESPONSE_CANCEL,
				NULL);

	ret = gedit_utils_get_glade_widgets (GEDIT_GLADEDIR "gedit-search-dialog.glade",
					     "search_dialog_content",
					     &error_widget,
					     "search_dialog_content", &content,
					     "table", &dlg->priv->table,
					     "search_for_text_entry", &dlg->priv->search_entry,
					     "replace_with_text_entry", &dlg->priv->replace_entry,
					     "search_for_text_entry_list", &dlg->priv->search_list,
					     "replace_with_text_entry_list", &dlg->priv->replace_list,
					     "replace_with_label", &dlg->priv->replace_label,
					     "match_case_checkbutton", &dlg->priv->match_case_checkbutton,
					     "entire_word_checkbutton", &dlg->priv->entire_word_checkbutton,
					     "search_backwards_checkbutton", &dlg->priv->backwards_checkbutton,
					     "wrap_around_checkbutton", &dlg->priv->wrap_around_checkbutton,
					     NULL);

	if (!ret)
	{
		gtk_widget_show (error_widget);

		gtk_box_pack_start_defaults (GTK_BOX (GTK_DIALOG (dlg)->vbox),
					     error_widget);

		dlg->priv->glade_error = TRUE;

		return;
	}

	dlg->priv->find_button = gtk_button_new_from_stock (GTK_STOCK_FIND);
	dlg->priv->replace_all_button = gtk_button_new_with_mnemonic (_("Replace _All"));
	dlg->priv->replace_button = gedit_gtk_button_new_with_stock_icon (_("_Replace"),
									  GTK_STOCK_FIND_AND_REPLACE);

	gtk_dialog_add_action_widget (GTK_DIALOG (dlg),
				      dlg->priv->replace_all_button, GEDIT_SEARCH_DIALOG_REPLACE_ALL_RESPONSE);
	gtk_dialog_add_action_widget (GTK_DIALOG (dlg),
				      dlg->priv->replace_button, GEDIT_SEARCH_DIALOG_REPLACE_RESPONSE);
	gtk_dialog_add_action_widget (GTK_DIALOG (dlg),
				      dlg->priv->find_button, GEDIT_SEARCH_DIALOG_FIND_RESPONSE);
	g_object_set (G_OBJECT (dlg->priv->find_button), "can-default", TRUE, NULL);

	gtk_dialog_set_default_response (GTK_DIALOG (dlg),
					 GEDIT_SEARCH_DIALOG_FIND_RESPONSE);

	/* insensitive by default */
	gtk_dialog_set_response_sensitive (GTK_DIALOG (dlg),
					   GEDIT_SEARCH_DIALOG_FIND_RESPONSE,
					   FALSE);
	gtk_dialog_set_response_sensitive (GTK_DIALOG (dlg),
					   GEDIT_SEARCH_DIALOG_REPLACE_RESPONSE,
					   FALSE);
	gtk_dialog_set_response_sensitive (GTK_DIALOG (dlg),
					   GEDIT_SEARCH_DIALOG_REPLACE_ALL_RESPONSE,
					   FALSE);

	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox),
			    content, FALSE, FALSE, 0);

	g_signal_connect (dlg->priv->search_entry,
			  "insert_text",
			  G_CALLBACK (insert_text_handler),
			  NULL);
	g_signal_connect (dlg->priv->replace_entry,
			  "insert_text",
			  G_CALLBACK (insert_text_handler),
			  NULL);
	g_signal_connect (dlg->priv->search_list,
			  "changed",
			  G_CALLBACK (search_entry_changed),
			  dlg);

	g_signal_connect (dlg->priv->match_case_checkbutton,
			  "toggled",
			  G_CALLBACK (search_options_changed),
			  dlg);
	g_signal_connect (dlg->priv->entire_word_checkbutton,
			  "toggled",
			  G_CALLBACK (search_options_changed),
			  dlg);

	g_signal_connect (dlg,
			  "response",
			  G_CALLBACK (response_handler),
			  NULL);
}

GtkWidget *
gedit_search_dialog_new (GtkWindow *parent,
			 gboolean   show_replace)
{
	GeditSearchDialog *dlg;

	dlg = g_object_new (GEDIT_TYPE_SEARCH_DIALOG,
			    "show-replace", show_replace,
			    NULL);

	if (parent != NULL)
		gtk_window_set_transient_for (GTK_WINDOW (dlg),
					      parent);

	return GTK_WIDGET (dlg);
}

gboolean
gedit_search_dialog_get_show_replace (GeditSearchDialog *dialog)
{
	g_return_val_if_fail (GEDIT_IS_SEARCH_DIALOG (dialog), FALSE);

	return dialog->priv->show_replace;
}

void
gedit_search_dialog_set_show_replace (GeditSearchDialog *dialog,
				      gboolean           show_replace)
{
	g_return_if_fail (GEDIT_IS_SEARCH_DIALOG (dialog));

	if (dialog->priv->glade_error)
		return;

	dialog->priv->show_replace = show_replace != FALSE;
	show_replace_widgets (dialog, dialog->priv->show_replace);

	g_object_notify (G_OBJECT (dialog), "show-replace");
}

void
gedit_search_dialog_set_search_text (GeditSearchDialog *dialog,
				     const gchar       *text)
{
	g_return_if_fail (GEDIT_IS_SEARCH_DIALOG (dialog));
	g_return_if_fail (text != NULL);

	gtk_entry_set_text (GTK_ENTRY (dialog->priv->search_entry), text);

	gtk_dialog_set_response_sensitive (GTK_DIALOG (dialog),
					   GEDIT_SEARCH_DIALOG_FIND_RESPONSE,
					   (text != '\0'));
}

/*
 * The text must be unescaped before searching.
 */
const gchar *
gedit_search_dialog_get_search_text (GeditSearchDialog *dialog)
{
	g_return_val_if_fail (GEDIT_IS_SEARCH_DIALOG (dialog), NULL);

	return gtk_entry_get_text (GTK_ENTRY (dialog->priv->search_entry));
}

void
gedit_search_dialog_set_replace_text (GeditSearchDialog *dialog,
				      const gchar       *text)
{
	g_return_if_fail (GEDIT_IS_SEARCH_DIALOG (dialog));
	g_return_if_fail (text != NULL);

	gtk_entry_set_text (GTK_ENTRY (dialog->priv->replace_entry), text);
}

const gchar *
gedit_search_dialog_get_replace_text (GeditSearchDialog *dialog)
{
	g_return_val_if_fail (GEDIT_IS_SEARCH_DIALOG (dialog), NULL);

	return gtk_entry_get_text (GTK_ENTRY (dialog->priv->replace_entry));
}

void
gedit_search_dialog_set_match_case (GeditSearchDialog *dialog,
				    gboolean           match_case)
{
	g_return_if_fail (GEDIT_IS_SEARCH_DIALOG (dialog));

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->priv->match_case_checkbutton),
				      match_case);
}

gboolean
gedit_search_dialog_get_match_case (GeditSearchDialog *dialog)
{
	g_return_val_if_fail (GEDIT_IS_SEARCH_DIALOG (dialog), FALSE);

	return gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->priv->match_case_checkbutton));
}

void
gedit_search_dialog_set_entire_word (GeditSearchDialog *dialog,
				     gboolean           entire_word)
{
	g_return_if_fail (GEDIT_IS_SEARCH_DIALOG (dialog));

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->priv->entire_word_checkbutton),
				      entire_word);
}

gboolean
gedit_search_dialog_get_entire_word (GeditSearchDialog *dialog)
{
	g_return_val_if_fail (GEDIT_IS_SEARCH_DIALOG (dialog), FALSE);

	return gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->priv->entire_word_checkbutton));
}

void
gedit_search_dialog_set_backwards (GeditSearchDialog *dialog,
				  gboolean           backwards)
{
	g_return_if_fail (GEDIT_IS_SEARCH_DIALOG (dialog));

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->priv->backwards_checkbutton),
				      backwards);
}

gboolean
gedit_search_dialog_get_backwards (GeditSearchDialog *dialog)
{
	g_return_val_if_fail (GEDIT_IS_SEARCH_DIALOG (dialog), FALSE);

	return gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->priv->backwards_checkbutton));
}

void
gedit_search_dialog_set_wrap_around (GeditSearchDialog *dialog,
				     gboolean           wrap_around)
{
	g_return_if_fail (GEDIT_IS_SEARCH_DIALOG (dialog));

	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->priv->wrap_around_checkbutton),
				      wrap_around);
}

gboolean
gedit_search_dialog_get_wrap_around (GeditSearchDialog *dialog)
{
	g_return_val_if_fail (GEDIT_IS_SEARCH_DIALOG (dialog), FALSE);

	return gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (dialog->priv->wrap_around_checkbutton));
}
