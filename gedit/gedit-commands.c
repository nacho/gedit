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

#include <string.h>
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

#define TO_BE_IMPLEMENTED	{ GtkWidget *message_dlg; \
				  message_dlg = gtk_message_dialog_new (	\
			                          GTK_WINDOW (bonobo_mdi_get_active_window (BONOBO_MDI (gedit_mdi))),	\
			                          GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,	\
			                          GTK_MESSAGE_INFO,	\
			                          GTK_BUTTONS_OK,	\
                                   		  _("Not yet implemented."));	\
				  gtk_dialog_set_default_response (GTK_DIALOG (message_dlg), GTK_RESPONSE_OK); \
	                          gtk_dialog_run (GTK_DIALOG (message_dlg));	\
  	                          gtk_widget_destroy (message_dlg); }

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
	
	gedit_file_save (active_child, TRUE);
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
	
	gedit_close_x_button_pressed = TRUE;
	
	gedit_file_close (active_view);

	gedit_close_x_button_pressed = FALSE;
}

void 
gedit_cmd_file_close_all (BonoboUIComponent *uic, gpointer user_data, const gchar* verbname)
{
	gedit_debug (DEBUG_COMMANDS, "");

	gedit_close_x_button_pressed = TRUE;
	
	gedit_file_close_all ();

	gedit_close_x_button_pressed = FALSE;
}

void 
gedit_cmd_file_exit (BonoboUIComponent *uic, gpointer user_data, const gchar* verbname)
{
	gedit_debug (DEBUG_COMMANDS, "");

	gedit_exit_button_pressed = TRUE;
	
	gedit_file_exit ();	

	gedit_exit_button_pressed = FALSE;
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
	GeditDocument* active_doc;

	active_doc = gedit_get_active_document ();

	gedit_document_set_selection (active_doc, 0, -1); 
}

void 
gedit_cmd_search_find (BonoboUIComponent *uic, gpointer user_data, const gchar* verbname)
{
	gedit_debug (DEBUG_COMMANDS, "");

	gedit_dialog_find ();
}

void 
gedit_cmd_search_find_again (BonoboUIComponent *uic, gpointer user_data, const gchar* verbname)
{
	GeditMDIChild *active_child;
	GeditDocument *doc;
	GeditView *active_view;
	gchar* last_searched_text;
	
	gedit_debug (DEBUG_COMMANDS, "");

	active_child = GEDIT_MDI_CHILD (bonobo_mdi_get_active_child (BONOBO_MDI (gedit_mdi)));
	g_return_if_fail (active_child);

	active_view = GEDIT_VIEW (bonobo_mdi_get_active_view (BONOBO_MDI (gedit_mdi)));
	g_return_if_fail (active_view != NULL);

	doc = GEDIT_DOCUMENT (active_child->document);
	g_return_if_fail (doc);

	last_searched_text = gedit_document_get_last_searched_text (doc);
	if (last_searched_text != NULL)
	{
		if (!gedit_document_find_again (doc))
		{	
			GtkWidget *message_dlg;

			message_dlg = gtk_message_dialog_new (
				GTK_WINDOW (bonobo_mdi_get_active_window (BONOBO_MDI (gedit_mdi))),
				GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_MESSAGE_INFO,
				GTK_BUTTONS_OK,
				_("The string \"%s\" has not been found."), last_searched_text);

			gtk_dialog_set_default_response (GTK_DIALOG (message_dlg), GTK_RESPONSE_OK);

			gtk_window_set_resizable (GTK_WINDOW (message_dlg), FALSE);

			gtk_dialog_run (GTK_DIALOG (message_dlg));
  			gtk_widget_destroy (message_dlg);
		}
		else
			gedit_view_scroll_to_cursor (active_view);

	}
	else
	{
		g_free (last_searched_text);
		gedit_dialog_find ();
	}
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
	static GtkWidget *dlg = NULL;

	gedit_debug (DEBUG_COMMANDS, "");

	if (dlg != NULL)
	{
		gtk_window_present (GTK_WINDOW (dlg));
		gtk_window_set_transient_for (GTK_WINDOW (dlg),	
					      GTK_WINDOW (gedit_get_active_window ()));

		return;
	}
		
	dlg = gedit_preferences_dialog_new (GTK_WINDOW (gedit_get_active_window ()));

	g_signal_connect (G_OBJECT (dlg), "destroy",
			  G_CALLBACK (gtk_widget_destroyed), &dlg);
	
	gtk_widget_show (dlg);
}

void 
gedit_cmd_help_contents (BonoboUIComponent *uic, gpointer user_data, const gchar* verbname)
{
	GError *error = NULL;

	gedit_debug (DEBUG_COMMANDS, "");

	gnome_help_display ("gedit.xml", NULL, &error);
	
	if (error != NULL)
	{
		g_warning (error->message);

		g_error_free (error);
	}
}

void 
gedit_cmd_help_about (BonoboUIComponent *uic, gpointer user_data, const gchar* verbname)
{
	static GtkWidget *about = NULL;
	GdkPixbuf* pixbuf = NULL;
	
	gchar *authors[] = {
		"Paolo Maggi <maggi@athena.polito.it>",
		"Chema Celorio <chema@ximian.com>", 
		"James Willcox <jwillcox@cs.indiana.edu>",
		"Federico Mena Quintero <federico@ximian.com>",
		NULL
	};
	
	gchar *documenters[] = {
		"Sun GNOME Documentation Team <gdocteam@sun.com>",
		"Eric Baudais <baudais@okstate.edu>",
		NULL
	};
	
	gchar *translator_credits = _("translator_credits");

	gedit_debug (DEBUG_COMMANDS, "");

	if (about != NULL)
	{
		gdk_window_show (about->window);
		gdk_window_raise (about->window);
		return;
	}
	
	pixbuf = gdk_pixbuf_new_from_file ( GNOME_ICONDIR "/gedit-logo.png", NULL);
	if (pixbuf != NULL)
	{
		GdkPixbuf* temp_pixbuf = NULL;

		temp_pixbuf = gdk_pixbuf_scale_simple (pixbuf, 
					 gdk_pixbuf_get_width (pixbuf),
					 gdk_pixbuf_get_height (pixbuf),
					 GDK_INTERP_HYPER);
		g_object_unref (pixbuf);

		pixbuf = temp_pixbuf;
	}

	about = gnome_about_new (_("gedit"), VERSION,
				_("(C) 1998-2000 Evan Lawrence and Alex Robert\n"
				  "(C) 2000-2002 Chema Celorio and Paolo Maggi"),
				_("gedit is a small and lightweight text editor for Gnome"),
				(const char **)authors,
				(const char **)documenters,
				strcmp (translator_credits, "translator_credits") != 0 ? (const char *)translator_credits : NULL,
				pixbuf);

	gtk_window_set_transient_for (GTK_WINDOW (about),
			GTK_WINDOW (gedit_get_active_window ()));

	gtk_window_set_destroy_with_parent (GTK_WINDOW (about), TRUE);

	if (pixbuf != NULL)
		g_object_unref (pixbuf);
	
	g_signal_connect (G_OBJECT (about), "destroy",
			  G_CALLBACK (gtk_widget_destroyed), &about);
	
	gtk_widget_show (about);
}


