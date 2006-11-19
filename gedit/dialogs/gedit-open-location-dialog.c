/*
 * gedit-open-location-dialog.c
 * This file is part of gedit
 *
 * Copyright (C) 2001-2005 Paolo Maggi 
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
 * Modified by the gedit Team, 2001-2005. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 *
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n.h>
#include <libgnomevfs/gnome-vfs-utils.h>

#include "gedit-open-location-dialog.h"
#include "gedit-history-entry.h"
#include "gedit-encodings-option-menu.h"
#include "gedit-utils.h"
#include "gedit-help.h"

#define GEDIT_OPEN_LOCATION_DIALOG_GET_PRIVATE(object) (G_TYPE_INSTANCE_GET_PRIVATE ((object), \
							GEDIT_TYPE_OPEN_LOCATION_DIALOG, \
							GeditOpenLocationDialogPrivate))

struct _GeditOpenLocationDialogPrivate 
{
	GtkWidget *uri_entry;
	GtkWidget *uri_text_entry;
	GtkWidget *encoding_menu;
};

G_DEFINE_TYPE(GeditOpenLocationDialog, gedit_open_location_dialog, GTK_TYPE_DIALOG)

static void 
gedit_open_location_dialog_class_init (GeditOpenLocationDialogClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
					      								      
	g_type_class_add_private (object_class, sizeof (GeditOpenLocationDialogPrivate));
}

static void 
entry_changed (GtkComboBox             *combo, 
	       GeditOpenLocationDialog *dlg)
{
	const gchar *str;

	str = gtk_entry_get_text (GTK_ENTRY (dlg->priv->uri_text_entry));
	g_return_if_fail (str != NULL);

	gtk_dialog_set_response_sensitive (GTK_DIALOG (dlg), 
					   GTK_RESPONSE_OK,
					   (str[0] != '\0'));
}

static void
response_handler (GeditOpenLocationDialog *dlg,
                  gint                     response_id,
                  gpointer                 data)
{
	gchar *uri;

	switch (response_id)
	{
		case GTK_RESPONSE_HELP:
			gedit_help_display (GTK_WINDOW (dlg),
					    NULL,
					    NULL);

			g_signal_stop_emission_by_name (dlg, "response");
			break;

		case GTK_RESPONSE_OK:
			uri = gedit_open_location_dialog_get_uri (dlg);
			if (uri != NULL)
			{
				const gchar *text;

				text = gtk_entry_get_text
						(GTK_ENTRY (dlg->priv->uri_text_entry));
				gedit_history_entry_prepend_text
						 (GEDIT_HISTORY_ENTRY (dlg->priv->uri_entry),
						  text);

				g_free (uri);
			}
			break;
	}
}

static void
gedit_open_location_dialog_init (GeditOpenLocationDialog *dlg)
{	
	GtkWidget *content;
	GtkWidget *vbox;
	GtkWidget *location_label;
	GtkWidget *encoding_label;
	GtkWidget *encoding_hbox;
	GtkWidget *error_widget;
	gboolean   ret;

	dlg->priv = GEDIT_OPEN_LOCATION_DIALOG_GET_PRIVATE (dlg);

	gtk_dialog_add_buttons (GTK_DIALOG (dlg),
				GTK_STOCK_CANCEL, 
				GTK_RESPONSE_CANCEL,
				GTK_STOCK_OPEN,
				GTK_RESPONSE_OK,
				GTK_STOCK_HELP,
				GTK_RESPONSE_HELP,
				NULL);

	gtk_window_set_title (GTK_WINDOW (dlg), _("Open Location"));
	gtk_window_set_resizable (GTK_WINDOW (dlg), FALSE);
	gtk_dialog_set_has_separator (GTK_DIALOG (dlg), FALSE);
	gtk_window_set_destroy_with_parent (GTK_WINDOW (dlg), TRUE);

	gtk_dialog_set_default_response (GTK_DIALOG (dlg),
					 GTK_RESPONSE_OK);
	gtk_dialog_set_response_sensitive (GTK_DIALOG (dlg), 
					   GTK_RESPONSE_OK, FALSE);

	g_signal_connect (G_OBJECT (dlg), 
			  "response",
			  G_CALLBACK (response_handler),
			  NULL);

	ret = gedit_utils_get_glade_widgets (GEDIT_GLADEDIR "gedit-open-location-dialog.glade",
					     "open_uri_dialog_content",
					     &error_widget,
					     "open_uri_dialog_content", &content,
					     "main_vbox", &vbox,
					     "location_label", &location_label,
					     "encoding_label", &encoding_label,
					     "encoding_hbox", &encoding_hbox,
					     NULL);

	if (!ret)
	{
		gtk_widget_show (error_widget);
			
		gtk_box_pack_start_defaults (GTK_BOX (GTK_DIALOG (dlg)->vbox),
					     error_widget);

		return;
	}

	dlg->priv->uri_entry = gedit_history_entry_new ("gedit2_uri_list");
	dlg->priv->uri_text_entry = gedit_history_entry_get_entry (GEDIT_HISTORY_ENTRY (dlg->priv->uri_entry));
	gtk_entry_set_activates_default (GTK_ENTRY (dlg->priv->uri_text_entry), TRUE);
	gtk_widget_show (dlg->priv->uri_entry);
	gtk_box_pack_start (GTK_BOX (vbox),
			    dlg->priv->uri_entry,
			    FALSE,
			    FALSE,
			    0);

	gtk_label_set_mnemonic_widget (GTK_LABEL (location_label),
				       dlg->priv->uri_entry);

	dlg->priv->encoding_menu = gedit_encodings_option_menu_new (FALSE);

	gtk_label_set_mnemonic_widget (GTK_LABEL (encoding_label),
				       dlg->priv->encoding_menu);

	gtk_box_pack_end (GTK_BOX (encoding_hbox), 
			  dlg->priv->encoding_menu,
			  TRUE,
			  TRUE,
			  0);

	gtk_widget_show (dlg->priv->encoding_menu);

	g_signal_connect (dlg->priv->uri_entry,
			  "changed",
			  G_CALLBACK (entry_changed), 
			  dlg);

	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox),
			    content, FALSE, FALSE, 0);
}

GtkWidget *
gedit_open_location_dialog_new (GtkWindow *parent)
{
	GtkWidget *dlg;
	
	dlg = GTK_WIDGET (g_object_new (GEDIT_TYPE_OPEN_LOCATION_DIALOG, NULL));
	
	if (parent != NULL)
		gtk_window_set_transient_for (GTK_WINDOW (dlg),
					      parent);

	return dlg;
}

/* Always return a valid gnome vfs uri or NULL */
gchar *
gedit_open_location_dialog_get_uri (GeditOpenLocationDialog *dlg)
{
	const gchar *str;
	gchar *uri;
	gchar *canonical_uri;
	GnomeVFSURI *vfs_uri;

	g_return_val_if_fail (GEDIT_IS_OPEN_LOCATION_DIALOG (dlg), NULL);

	str = gtk_entry_get_text (GTK_ENTRY (dlg->priv->uri_text_entry));
	g_return_val_if_fail (str != NULL, NULL);

	if (str[0] == '\0')
		return NULL;

	uri = gnome_vfs_make_uri_from_input_with_dirs (str,
						       GNOME_VFS_MAKE_URI_DIR_CURRENT);
	g_return_val_if_fail (uri != NULL, NULL);

	canonical_uri = gnome_vfs_make_uri_canonical (uri);
	g_free (uri);

	g_return_val_if_fail (canonical_uri != NULL, NULL);

	vfs_uri = gnome_vfs_uri_new (canonical_uri);
	if (vfs_uri == NULL)
	{
		g_free (canonical_uri);
		return NULL;
	}

	gnome_vfs_uri_unref (vfs_uri);

	return canonical_uri;
}

const GeditEncoding *
gedit_open_location_dialog_get_encoding	(GeditOpenLocationDialog *dlg)
{
	g_return_val_if_fail (GEDIT_IS_OPEN_LOCATION_DIALOG (dlg), NULL);
	
	return gedit_encodings_option_menu_get_selected_encoding (
				GEDIT_ENCODINGS_OPTION_MENU (dlg->priv->encoding_menu));
}
