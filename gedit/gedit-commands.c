/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-commands.c
 * This file is part of gedit
 *
 * Copyright (C) 1998, 1999 Alex Roberts, Evan Lawrence
 * Copyright (C) 2000, 2001 Chema Celorio, Paolo Maggi 
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
 * Modified by the gedit Team, 1998-2001. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <libgnome/libgnome.h>
#include <libgnomeui/libgnomeui.h>

#include "gedit-commands.h"
#include "gedit2.h"
#include "gedit-mdi-child.h"
#include "gedit-debug.h"
#include "gedit-view.h"
#include "gedit-file.h"
#include "gedit-print.h"
#include "dialogs/gedit-dialogs.h"
#include "dialogs/gedit-preferences-dialog.h"

void 
gedit_cmd_file_new (BonoboUIComponent *uic, gpointer user_data, const gchar* verbname)
{
	gedit_debug (DEBUG_COMMANDS, "verbname: %s", verbname);
	
	gedit_file_new ();
}

void 
gedit_cmd_file_open (BonoboUIComponent *uic, gpointer user_data, const gchar* verbname)
{
	BonoboMDIChild *active_child;
	
	gedit_debug (DEBUG_COMMANDS, "");

	active_child = bonobo_mdi_get_active_child (BONOBO_MDI (gedit_mdi));
	
	gedit_file_open ((GeditMDIChild*) active_child);
}

void 
gedit_cmd_file_save (BonoboUIComponent *uic, gpointer user_data, const gchar* verbname)
{
	GeditMDIChild *active_child;
	
	gedit_debug (DEBUG_COMMANDS, "");

	active_child = GEDIT_MDI_CHILD (bonobo_mdi_get_active_child (BONOBO_MDI (gedit_mdi)));
	if (active_child == NULL)
		return;
	
	gedit_file_save (active_child);
}

void 
gedit_cmd_file_save_as (BonoboUIComponent *uic, gpointer user_data, const gchar* verbname)
{
	GeditMDIChild *active_child;
	
	gedit_debug (DEBUG_COMMANDS, "");

	active_child = GEDIT_MDI_CHILD (bonobo_mdi_get_active_child (BONOBO_MDI (gedit_mdi)));
	if (active_child == NULL)
		return;
	
	gedit_file_save_as (active_child);
}

void 
gedit_cmd_file_save_all (BonoboUIComponent *uic, gpointer user_data, const gchar* verbname)
{
	gedit_debug (DEBUG_COMMANDS, "");

	gedit_file_save_all ();
}

void 
gedit_cmd_file_revert (BonoboUIComponent *uic, gpointer user_data, const gchar* verbname)
{
	GeditMDIChild *active_child;
	
	gedit_debug (DEBUG_COMMANDS, "");

	active_child = GEDIT_MDI_CHILD (bonobo_mdi_get_active_child (BONOBO_MDI (gedit_mdi)));
	if (active_child == NULL)
		return;
	
	gedit_file_revert (active_child);
}

void 
gedit_cmd_file_open_uri (BonoboUIComponent *uic, gpointer user_data, const gchar* verbname)
{
	gedit_debug (DEBUG_COMMANDS, "");

	gedit_dialog_open_uri ();
}

void
gedit_cmd_file_print (BonoboUIComponent *uic, gpointer user_data, const gchar* verbname)
{
	GeditMDIChild *active_child;
	
	gedit_debug (DEBUG_COMMANDS, "");

	active_child = GEDIT_MDI_CHILD (bonobo_mdi_get_active_child (BONOBO_MDI (gedit_mdi)));
	if (active_child == NULL)
		return;
	
	gedit_print (active_child);
}

void
gedit_cmd_file_print_preview (BonoboUIComponent *uic, gpointer user_data, const gchar* verbname)
{
	GeditMDIChild *active_child;
	
	gedit_debug (DEBUG_COMMANDS, "");

	active_child = GEDIT_MDI_CHILD (bonobo_mdi_get_active_child (BONOBO_MDI (gedit_mdi)));
	if (active_child == NULL)
		return;
	
	gedit_print_preview (active_child);
}

void 
gedit_cmd_file_close (BonoboUIComponent *uic, gpointer user_data, const gchar* verbname)
{
	GtkWidget *active_view;
	
	gedit_debug (DEBUG_COMMANDS, "");

	active_view = bonobo_mdi_get_active_view (BONOBO_MDI (gedit_mdi));
	
	if (active_view == NULL)
		return;

	gedit_file_close (active_view);
}

void 
gedit_cmd_file_close_all (BonoboUIComponent *uic, gpointer user_data, const gchar* verbname)
{
	gedit_debug (DEBUG_COMMANDS, "");

	gedit_file_close_all ();
}

void 
gedit_cmd_file_exit (BonoboUIComponent *uic, gpointer user_data, const gchar* verbname)
{
	gedit_debug (DEBUG_COMMANDS, "");

	gedit_file_exit ();	
}

void 
gedit_cmd_edit_undo (BonoboUIComponent *uic, gpointer user_data, const gchar* verbname)
{
	GeditView* active_view;
	GeditDocument* active_document;

	active_view = GEDIT_VIEW (bonobo_mdi_get_active_view (BONOBO_MDI (gedit_mdi)));
	g_return_if_fail (active_view);

	active_document = gedit_view_get_document (active_view);
	g_return_if_fail (active_document);

	gedit_document_undo (active_document);

	/* TODO: Move the cursor */
}

void 
gedit_cmd_edit_redo (BonoboUIComponent *uic, gpointer user_data, const gchar* verbname)
{
	GeditView* active_view;
	GeditDocument* active_document;

	active_view = GEDIT_VIEW (bonobo_mdi_get_active_view (BONOBO_MDI (gedit_mdi)));
	g_return_if_fail (active_view);

	active_document = gedit_view_get_document (active_view);
	g_return_if_fail (active_document);

	gedit_document_redo (active_document);

	/* TODO: Move the cursor */
}

void 
gedit_cmd_edit_cut (BonoboUIComponent *uic, gpointer user_data, const gchar* verbname)
{
	GeditView* active_view;

	active_view = GEDIT_VIEW (bonobo_mdi_get_active_view (BONOBO_MDI (gedit_mdi)));
	g_return_if_fail (active_view);
	
	gedit_view_cut_clipboard (active_view); 
}

void 
gedit_cmd_edit_copy (BonoboUIComponent *uic, gpointer user_data, const gchar* verbname)
{
	GeditView* active_view;

	active_view = GEDIT_VIEW (bonobo_mdi_get_active_view (BONOBO_MDI (gedit_mdi)));
	g_return_if_fail (active_view);
	
	gedit_view_copy_clipboard (active_view); 
}

void 
gedit_cmd_edit_paste (BonoboUIComponent *uic, gpointer user_data, const gchar* verbname)
{
	GeditView* active_view;

	active_view = GEDIT_VIEW (bonobo_mdi_get_active_view (BONOBO_MDI (gedit_mdi)));
	g_return_if_fail (active_view);
	
	gedit_view_paste_clipboard (active_view); 
}

void 
gedit_cmd_edit_clear (BonoboUIComponent *uic, gpointer user_data, const gchar* verbname)
{
	GeditView* active_view;

	active_view = GEDIT_VIEW (bonobo_mdi_get_active_view (BONOBO_MDI (gedit_mdi)));
	g_return_if_fail (active_view);
	
	gedit_view_delete_selection (active_view); 
}

void
gedit_cmd_edit_select_all (BonoboUIComponent *uic, gpointer user_data, const gchar* verbname)
{
	GeditView* active_view;

	active_view = GEDIT_VIEW (bonobo_mdi_get_active_view (BONOBO_MDI (gedit_mdi)));
	g_return_if_fail (active_view);
	
	gedit_view_select_all (active_view); 
}

void 
gedit_cmd_search_find (BonoboUIComponent *uic, gpointer user_data, const gchar* verbname)
{
	gedit_debug (DEBUG_COMMANDS, "");

	gedit_dialog_find ();
}

void 
gedit_cmd_search_replace (BonoboUIComponent *uic, gpointer user_data, const gchar* verbname)
{
	gedit_debug (DEBUG_COMMANDS, "");

	gedit_dialog_replace ();
}

void 
gedit_cmd_search_goto_line (BonoboUIComponent *uic, gpointer user_data, const gchar* verbname)
{
	gedit_debug (DEBUG_COMMANDS, "");

	gedit_dialog_goto_line ();
}

void
gedit_cmd_settings_preferences (BonoboUIComponent *uic, gpointer user_data, const gchar* verbname)
{
	GtkWidget *dlg;

	dlg = gedit_preferences_dialog_new (
			GTK_WINDOW (bonobo_mdi_get_active_window (BONOBO_MDI (gedit_mdi))));

	gtk_dialog_run (GTK_DIALOG (dlg));

	gtk_widget_destroy (dlg);
}

void 
gedit_cmd_help_about (BonoboUIComponent *uic, gpointer user_data, const gchar* verbname)
{
	/* FIXME */
	static const char *authors[] = { "Chema Celorio", 
					 "Paolo Maggi", 
					  NULL };
	
	static const char *documentors[] = { "Eric Baudais", NULL };
	
	static GtkWidget *about_box = NULL;
	
	gedit_debug (DEBUG_COMMANDS, "");
	
	if (about_box == NULL)
	{
		GdkPixbuf *pixbuf = NULL;

		/*
		pixbuf = gdk_pixbuf_new_from_file ("../pixmaps/gedit-logo.png", NULL);
		*/
	
		about_box = gnome_about_new ("gedit",
					VERSION,
					"(c) 1998-2000 Alex Robert and Ewan Lawrence\n"
					"(c) 2000-2001 Chema Celorio and Paolo Maggi",
					"gedit is a small and lightweight text editor for GNOME",
					authors,
					documentors,
					NULL /*"Translation credits"*/,
					NULL /* logo pixbuf */);

		/*
		g_object_unref (G_OBJECT (pixbuf));
		*/
	}

	gtk_window_set_transient_for (GTK_WINDOW (about_box),
			      GTK_WINDOW (bonobo_mdi_get_active_window (BONOBO_MDI (gedit_mdi))));

	gtk_dialog_run (about_box);

	gtk_widget_hide (about_box);
}

