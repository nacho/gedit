/*
 * gedit-file-chooser-dialog.c
 * This file is part of gedit
 *
 * Copyright (C) 2005-2007 - Paolo Maggi
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
 * Modified by the gedit Team, 2005-2007. See the AUTHORS file for a
 * list of people on the gedit Team.
 * See the ChangeLog files for a list of changes.
 *
 * $Id$
 */

/* TODO: Override set_extra_widget */
/* TODO: add encoding property */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <libgnomevfs/gnome-vfs-mime-utils.h>

#include "gedit-file-chooser-dialog.h"
#include "gedit-encodings-option-menu.h"
#include "gedit-language-manager.h"
#include "gedit-prefs-manager-app.h"
#include "gedit-debug.h"

#define GEDIT_FILE_CHOOSER_DIALOG_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), GEDIT_TYPE_FILE_CHOOSER_DIALOG, GeditFileChooserDialogPrivate))

#define ALL_FILES		_("All Files")
#define ALL_TEXT_FILES		_("All Text Files")

struct _GeditFileChooserDialogPrivate
{
	GtkWidget *option_menu;
};

G_DEFINE_TYPE(GeditFileChooserDialog, gedit_file_chooser_dialog, GTK_TYPE_FILE_CHOOSER_DIALOG)

static void
gedit_file_chooser_dialog_class_init (GeditFileChooserDialogClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (object_class, sizeof(GeditFileChooserDialogPrivate));
}

static void
create_option_menu (GeditFileChooserDialog *dialog)
{
	GtkWidget *hbox;
	GtkWidget *label;
	GtkWidget *menu;

	hbox = gtk_hbox_new (FALSE, 6);

	label = gtk_label_new_with_mnemonic (_("C_haracter Coding:"));
	menu = gedit_encodings_option_menu_new (
		gtk_file_chooser_get_action (GTK_FILE_CHOOSER (dialog)) == GTK_FILE_CHOOSER_ACTION_SAVE);

	gtk_label_set_mnemonic_widget (GTK_LABEL (label), menu);
	gtk_box_pack_start (GTK_BOX (hbox),
			    label,
			    FALSE,
			    FALSE,
			    0);

	gtk_box_pack_end (GTK_BOX (hbox),
			  menu,
			  TRUE,
			  TRUE,
			  0);

	gtk_file_chooser_set_extra_widget (GTK_FILE_CHOOSER (dialog),
					   hbox);
	gtk_widget_show_all (hbox);

	dialog->priv->option_menu = menu;
}

static void
action_changed (GeditFileChooserDialog *dialog,
		GParamSpec	       *pspec,
		gpointer		data)
{
	GtkFileChooserAction action;

	action = gtk_file_chooser_get_action (GTK_FILE_CHOOSER (dialog));

	switch (action)
	{
		case GTK_FILE_CHOOSER_ACTION_OPEN:
			g_object_set (dialog->priv->option_menu,
				      "save_mode", FALSE,
				      NULL);
			gtk_widget_show (dialog->priv->option_menu);
			break;
		case GTK_FILE_CHOOSER_ACTION_SAVE:
			g_object_set (dialog->priv->option_menu,
				      "save_mode", TRUE,
				      NULL);
			gtk_widget_show (dialog->priv->option_menu);
			break;
		default:
			gtk_widget_hide (dialog->priv->option_menu);
	}
}

static void
filter_changed (GeditFileChooserDialog *dialog,
		GParamSpec	       *pspec,
		gpointer		data)
{
	GtkFileFilter *filter;
	const gchar *name;
	gint id = 0;

	if (!gedit_prefs_manager_active_file_filter_can_set ())
		return;

	filter = gtk_file_chooser_get_filter (GTK_FILE_CHOOSER (dialog));
	name = gtk_file_filter_get_name (filter);
	g_return_if_fail (name != NULL);

	if (strcmp (name, ALL_TEXT_FILES) == 0)
		id = 1;

	gedit_debug_message (DEBUG_COMMANDS, "Active filter: %s (%d)", name, id);

	gedit_prefs_manager_set_active_file_filter (id);
}

/* FIXME: use globs too - Paolo (Aug. 27, 2007) */
static gboolean
all_text_files_filter (const GtkFileFilterInfo *filter_info,
		       gpointer                 data)
{
	static GSList *known_mime_types = NULL;
	GSList *mime_types;

	if (known_mime_types == NULL)
	{
		GtkSourceLanguageManager *lm;
		const gchar * const *languages;

		lm = gedit_get_language_manager ();
		languages = gtk_source_language_manager_get_language_ids (lm);

		while (*languages != NULL)
		{
			gchar **mime_types;
			gint i;
			GtkSourceLanguage *lang;

			lang = gtk_source_language_manager_get_language (lm, *languages);
			g_return_val_if_fail (GTK_IS_SOURCE_LANGUAGE (lang), FALSE);
			++languages;

			mime_types = gtk_source_language_get_mime_types (lang);
			if (mime_types == NULL)
				continue;

			for (i = 0; mime_types[i] != NULL; i++)
			{
				GnomeVFSMimeEquivalence res;

				res = gnome_vfs_mime_type_get_equivalence (mime_types[i],
									   "text/plain");

				if (res == GNOME_VFS_MIME_UNRELATED)
				{
					gedit_debug_message (DEBUG_COMMANDS,
							     "Mime-type %s is not related to text/plain",
							     mime_types[i]);

					known_mime_types = g_slist_prepend (known_mime_types,
									    g_strdup (mime_types[i]));
				}
			}

			g_strfreev (mime_types);
		}

		/* known_mime_types always has "text/plain" as first item" */
		known_mime_types = g_slist_prepend (known_mime_types, g_strdup ("text/plain"));
	}

	/* known mime_types contains "text/plain" and then the list of mime-types unrelated to "text/plain"
	 * that gedit recognizes */

	if (filter_info->mime_type == NULL)
		return FALSE;

	/*
	 * The filter is matching:
	 * - the mime-types beginning with "text/"
	 * - the mime-types inheriting from a known mime-type (note the text/plain is
	 *   the first known mime-type)
	 */

	if (strncmp (filter_info->mime_type, "text/", 5) == 0)
		return TRUE;

	mime_types = known_mime_types;
	while (mime_types != NULL)
	{
		GnomeVFSMimeEquivalence res;
		res = gnome_vfs_mime_type_get_equivalence (filter_info->mime_type,
							   (const gchar*)mime_types->data);

		if (res != GNOME_VFS_MIME_UNRELATED)
			return TRUE;

		mime_types = g_slist_next (mime_types);
	}

	return FALSE;
}

static void
gedit_file_chooser_dialog_init (GeditFileChooserDialog *dialog)
{
	dialog->priv = GEDIT_FILE_CHOOSER_DIALOG_GET_PRIVATE (dialog);
}

static GtkWidget *
gedit_file_chooser_dialog_new_valist (const gchar          *title,
				      GtkWindow            *parent,
				      GtkFileChooserAction  action,
				      const GeditEncoding  *encoding,
				      const gchar          *first_button_text,
				      va_list               varargs)
{
	GtkWidget *result;
	const char *button_text = first_button_text;
	gint response_id;
	GtkFileFilter *filter;
	gint active_filter;

	g_return_val_if_fail (parent != NULL, NULL);

	result = g_object_new (GEDIT_TYPE_FILE_CHOOSER_DIALOG,
			       "title", title,
			       "file-system-backend", NULL,
			       "local-only", FALSE,
			       "action", action,
			       "select-multiple", action == GTK_FILE_CHOOSER_ACTION_OPEN,
			       NULL);

	create_option_menu (GEDIT_FILE_CHOOSER_DIALOG (result));

	g_signal_connect (result,
			  "notify::action",
			  G_CALLBACK (action_changed),
			  NULL);

	if (encoding != NULL)
		gedit_encodings_option_menu_set_selected_encoding (
				GEDIT_ENCODINGS_OPTION_MENU (GEDIT_FILE_CHOOSER_DIALOG (result)->priv->option_menu),
				encoding);

	active_filter = gedit_prefs_manager_get_active_file_filter ();
	gedit_debug_message (DEBUG_COMMANDS, "Active filter: %d", active_filter);

	/* Filters */
	filter = gtk_file_filter_new ();

	gtk_file_filter_set_name (filter, ALL_FILES);
	gtk_file_filter_add_pattern (filter, "*");
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (result), filter);

	if (active_filter != 1)
	{
		/* Make this filter the default */
		gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (result), filter);
	}

	filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, ALL_TEXT_FILES);
	gtk_file_filter_add_custom (filter,
				    GTK_FILE_FILTER_MIME_TYPE,
				    all_text_files_filter,
				    NULL,
				    NULL);
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (result), filter);

	if (active_filter == 1)
	{
		/* Make this filter the default */
		gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (result), filter);
	}

	g_signal_connect (result,
			  "notify::filter",
			  G_CALLBACK (filter_changed),
			  NULL);

	gtk_window_set_transient_for (GTK_WINDOW (result), parent);
	gtk_window_set_destroy_with_parent (GTK_WINDOW (result), TRUE);

	while (button_text)
	{
		response_id = va_arg (varargs, gint);

		gtk_dialog_add_button (GTK_DIALOG (result), button_text, response_id);
		if ((response_id == GTK_RESPONSE_OK) ||
		    (response_id == GTK_RESPONSE_ACCEPT) ||
		    (response_id == GTK_RESPONSE_YES) ||
		    (response_id == GTK_RESPONSE_APPLY))
			gtk_dialog_set_default_response (GTK_DIALOG (result), response_id);

		button_text = va_arg (varargs, const gchar *);
	}

	return result;
}

/**
 * gedit_file_chooser_dialog_new:
 * @title: Title of the dialog, or %NULL
 * @parent: Transient parent of the dialog, or %NULL
 * @action: Open or save mode for the dialog
 * @first_button_text: stock ID or text to go in the first button, or %NULL
 * @Varargs: response ID for the first button, then additional (button, id) pairs, ending with %NULL
 *
 * Creates a new #GeditFileChooserDialog.  This function is analogous to
 * gtk_dialog_new_with_buttons().
 *
 * Return value: a new #GeditFileChooserDialog
 *
 **/
GtkWidget *
gedit_file_chooser_dialog_new (const gchar          *title,
			       GtkWindow            *parent,
			       GtkFileChooserAction  action,
			       const GeditEncoding  *encoding,
			       const gchar          *first_button_text,
			       ...)
{
	GtkWidget *result;
	va_list varargs;

	va_start (varargs, first_button_text);
	result = gedit_file_chooser_dialog_new_valist (title, parent, action,
						       encoding, first_button_text,
						       varargs);
	va_end (varargs);

	return result;
}

void
gedit_file_chooser_dialog_set_encoding (GeditFileChooserDialog *dialog,
					const GeditEncoding    *encoding)
{
	g_return_if_fail (GEDIT_IS_FILE_CHOOSER_DIALOG (dialog));
	g_return_if_fail (GEDIT_IS_ENCODINGS_OPTION_MENU (dialog->priv->option_menu));

	gedit_encodings_option_menu_set_selected_encoding (
				GEDIT_ENCODINGS_OPTION_MENU (dialog->priv->option_menu),
				encoding);
}

const GeditEncoding *
gedit_file_chooser_dialog_get_encoding (GeditFileChooserDialog *dialog)
{
	g_return_val_if_fail (GEDIT_IS_FILE_CHOOSER_DIALOG (dialog), NULL);
	g_return_val_if_fail (GEDIT_IS_ENCODINGS_OPTION_MENU (dialog->priv->option_menu), NULL);
	g_return_val_if_fail ((gtk_file_chooser_get_action (GTK_FILE_CHOOSER (dialog)) == GTK_FILE_CHOOSER_ACTION_OPEN ||
			       gtk_file_chooser_get_action (GTK_FILE_CHOOSER (dialog)) == GTK_FILE_CHOOSER_ACTION_SAVE), NULL);

	return gedit_encodings_option_menu_get_selected_encoding (
				GEDIT_ENCODINGS_OPTION_MENU (dialog->priv->option_menu));
}
