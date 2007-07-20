/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-style-scheme-dialog.c
 * This file is part of gedit
 *
 * Copyright (C) 2007 Paolo Borelli
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
 * Modified by the gedit Team, 2001-2003. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 *
 * $Id: gedit-preferences-dialog.c 5645 2007-06-24 19:42:53Z pborelli $
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n.h>

#include "gedit-style-scheme-dialog.h"
#include "gedit-utils.h"
#include "gedit-debug.h"
#include "gedit-help.h"

#define GEDIT_STYLE_SCHEME_DIALOG_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), \
						      GEDIT_TYPE_STYLE_SCHEME_DIALOG, \
						      GeditStyleSchemeDialogPrivate))

struct _GeditStyleSchemeDialogPrivate
{
	GeditStyleSchemeGenerator *generator;
	
	GtkWidget                 *main_vbox;
};


G_DEFINE_TYPE(GeditStyleSchemeDialog, gedit_style_scheme_dialog, GTK_TYPE_DIALOG)


static void
gedit_style_scheme_dialog_finalize (GObject *object)
{
	GeditStyleSchemeDialog *dlg = GEDIT_STYLE_SCHEME_DIALOG (object);
	
	g_object_unref (dlg->priv->generator); 
	
	G_OBJECT_CLASS (gedit_style_scheme_dialog_parent_class)->finalize (object);
}

static void 
gedit_style_scheme_dialog_class_init (GeditStyleSchemeDialogClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = gedit_style_scheme_dialog_finalize;

	g_type_class_add_private (object_class, sizeof(GeditStyleSchemeDialogPrivate));
}

static void
dialog_response_handler (GtkDialog *dlg,
			 gint       res_id)
{
	gedit_debug (DEBUG_PREFS);

	switch (res_id)
	{
		case GTK_RESPONSE_HELP:
			gedit_help_display (GTK_WINDOW (dlg),
					    "gedit.xml",
					    "gedit-prefs"); // FIXME

			g_signal_stop_emission_by_name (dlg, "response");

			break;

		default:
			gtk_widget_destroy (GTK_WIDGET(dlg));
	}
}

static void
gedit_style_scheme_dialog_init (GeditStyleSchemeDialog *dlg)
{
	GtkWidget *error_widget;
	gboolean ret;

	gedit_debug (DEBUG_PREFS);

	dlg->priv = GEDIT_STYLE_SCHEME_DIALOG_GET_PRIVATE (dlg);

	gtk_dialog_add_buttons (GTK_DIALOG (dlg),
				GTK_STOCK_CLOSE,
				GTK_RESPONSE_CLOSE,
				GTK_STOCK_HELP,
				GTK_RESPONSE_HELP,
				NULL);

	gtk_window_set_title (GTK_WINDOW (dlg), _("gedit Style Scheme editor"));
	gtk_window_set_resizable (GTK_WINDOW (dlg), FALSE);
	gtk_dialog_set_has_separator (GTK_DIALOG (dlg), FALSE);
	gtk_window_set_destroy_with_parent (GTK_WINDOW (dlg), TRUE);

	g_signal_connect (dlg,
			  "response",
			  G_CALLBACK (dialog_response_handler),
			  NULL);

	ret = gedit_utils_get_glade_widgets (GEDIT_GLADEDIR "gedit-style-scheme-dialog.glade",
		"contents",
		&error_widget,

		"contents", &dlg->priv->main_vbox,

		NULL);

	if (!ret)
	{
		gtk_widget_show (error_widget);
			
		gtk_box_pack_start_defaults (GTK_BOX (GTK_DIALOG (dlg)->vbox),
					     error_widget);

		return;
	}

	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox),
			    dlg->priv->main_vbox,
			    FALSE, FALSE, 0);
}

GtkWidget *
gedit_style_scheme_dialog_new (GeditStyleSchemeGenerator *generator)
{
	GeditStyleSchemeDialog *dlg;
	
	dlg = GEDIT_STYLE_SCHEME_DIALOG (g_object_new (GEDIT_TYPE_STYLE_SCHEME_DIALOG, NULL));
	
	dlg->priv->generator = g_object_ref (generator);
	
	
	return GTK_WIDGET (dlg);
}
