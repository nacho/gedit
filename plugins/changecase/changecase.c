/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * changecase.c
 * This file is part of gedit
 *
 * Copyright (C) 2004 Paolo Borelli 
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

#include <libgnome/gnome-i18n.h>
#include <libgnome/gnome-help.h>

#include <gedit/gedit-plugin.h>
#include <gedit/gedit-debug.h>
#include <gedit/gedit-menus.h>


#define SUBMENU_LABEL	N_("C_hange Case")
#define SUBMENU_NAME	"ChangeCase"	
#define SUBMENU_PATH	"/menu/Edit/EditOps_6/"

#define MENU_ITEMS_PATH	"/menu/Edit/EditOps_6/ChangeCase/"

#define UPPER_ITEM_LABEL	N_("All _Upper Case")
#define LOWER_ITEM_LABEL	N_("All _Lower Case")
#define INVERT_ITEM_LABEL	N_("_Invert Case")
#define TITLE_ITEM_LABEL	N_("_Title Case")

#define UPPER_ITEM_TIP		N_("Change selected text to upper case")
#define LOWER_ITEM_TIP		N_("Change selected text to lower case")
#define INVERT_ITEM_TIP		N_("Invert the case of selected text")
#define TITLE_ITEM_TIP		N_("Capitalize the first letter of each selected word")


G_MODULE_EXPORT GeditPluginState update_ui (GeditPlugin *plugin, BonoboWindow *window);
G_MODULE_EXPORT GeditPluginState init (GeditPlugin *pd);
G_MODULE_EXPORT GeditPluginState destroy (GeditPlugin *pd);
G_MODULE_EXPORT GeditPluginState activate (GeditPlugin *pd);
G_MODULE_EXPORT GeditPluginState deactivate (GeditPlugin *pd);


typedef enum {
	TO_UPPER_CASE,
	TO_LOWER_CASE,
	INVERT_CASE,
	TITLE_CASE,
} ChangeCaseChoice;


static void
do_upper_case (GtkTextBuffer *buffer, GtkTextIter *start, GtkTextIter *end)
{
	GString *s = g_string_new (NULL);

	while (!gtk_text_iter_is_end (start) &&
	       !gtk_text_iter_equal (start, end))
	{
		gunichar c, nc;

		c = gtk_text_iter_get_char (start);
		nc = g_unichar_toupper (c);
		g_string_append_unichar (s, nc);

		gtk_text_iter_forward_char (start);
	}

	gtk_text_buffer_delete_selection (buffer, TRUE, TRUE);
	gtk_text_buffer_insert_at_cursor (buffer, s->str, s->len);

	g_string_free (s, TRUE);
}

static void
do_lower_case (GtkTextBuffer *buffer, GtkTextIter *start, GtkTextIter *end)
{
	GString *s = g_string_new (NULL);

	while (!gtk_text_iter_is_end (start) &&
	       !gtk_text_iter_equal (start, end))
	{
		gunichar c, nc;

		c = gtk_text_iter_get_char (start);
		nc = g_unichar_tolower (c);
		g_string_append_unichar (s, nc);

		gtk_text_iter_forward_char (start);
	}

	gtk_text_buffer_delete_selection (buffer, TRUE, TRUE);
	gtk_text_buffer_insert_at_cursor (buffer, s->str, s->len);

	g_string_free (s, TRUE);
}

static void
do_invert_case (GtkTextBuffer *buffer, GtkTextIter *start, GtkTextIter *end)
{
	GString *s = g_string_new (NULL);

	while (!gtk_text_iter_is_end (start) &&
	       !gtk_text_iter_equal (start, end))
	{
		gunichar c, nc;

		c = gtk_text_iter_get_char (start);
		if (g_unichar_islower (c))
			nc = g_unichar_toupper (c);
		else
			nc = g_unichar_tolower (c);
		g_string_append_unichar (s, nc);

		gtk_text_iter_forward_char (start);
	}

	gtk_text_buffer_delete_selection (buffer, TRUE, TRUE);
	gtk_text_buffer_insert_at_cursor (buffer, s->str, s->len);

	g_string_free (s, TRUE);
}

static void
do_title_case (GtkTextBuffer *buffer, GtkTextIter *start, GtkTextIter *end)
{
	GString *s = g_string_new (NULL);

	while (!gtk_text_iter_is_end (start) &&
	       !gtk_text_iter_equal (start, end))
	{
		gunichar c, nc;

		c = gtk_text_iter_get_char (start);
		if (gtk_text_iter_starts_word (start))
			nc = g_unichar_totitle (c);
		else
			nc = g_unichar_tolower (c);
		g_string_append_unichar (s, nc);

		gtk_text_iter_forward_char (start);
	}

	gtk_text_buffer_delete_selection (buffer, TRUE, TRUE);
	gtk_text_buffer_insert_at_cursor (buffer, s->str, s->len);

	g_string_free (s, TRUE);
}

static void
change_case (ChangeCaseChoice choice)
{
	GeditDocument *doc;
	GeditView *view;
	GtkTextIter start, end;

	gedit_debug (DEBUG_PLUGINS, "");

	view = gedit_get_active_view ();
	g_return_if_fail (view != NULL);

	doc = gedit_view_get_document (view);
	g_return_if_fail (doc != NULL);

	if (!gtk_text_buffer_get_selection_bounds (GTK_TEXT_BUFFER (doc),
						   &start, &end))
	{
		return;
	}

	gedit_document_begin_user_action (doc);

	switch (choice)
	{
	case TO_UPPER_CASE:
		do_upper_case (GTK_TEXT_BUFFER (doc), &start, &end);
		break;
	case TO_LOWER_CASE:
		do_lower_case (GTK_TEXT_BUFFER (doc), &start, &end);
		break;
	case INVERT_CASE:
		do_invert_case (GTK_TEXT_BUFFER (doc), &start, &end);
		break;
	case TITLE_CASE:
		do_title_case (GTK_TEXT_BUFFER (doc), &start, &end);
		break;
	default:
		g_return_if_reached ();
	}

	gedit_document_end_user_action (doc);
}

static void
upper_case_cb (BonoboUIComponent *uic, gpointer user_data, const gchar* verbname)
{
	change_case (TO_UPPER_CASE);
}

static void
lower_case_cb (BonoboUIComponent *uic, gpointer user_data, const gchar* verbname)
{
	change_case (TO_LOWER_CASE);
}

static void
invert_case_cb (BonoboUIComponent *uic, gpointer user_data, const gchar* verbname)
{
	change_case (INVERT_CASE);
}

static void
title_case_cb (BonoboUIComponent *uic, gpointer user_data, const gchar* verbname)
{
	change_case (TITLE_CASE);
}

gchar *changecase_sensible_verbs[] = {
	"/commands/AllUpperCase",
	"/commands/AllLowerCase",
	"/commands/InvertCase",
	"/commands/TitleCase",
	NULL
};

G_MODULE_EXPORT GeditPluginState
update_ui (GeditPlugin *plugin, BonoboWindow *window)
{
	BonoboUIComponent *uic;
	GeditMDI *mdi;
	GeditDocument *doc;

	gedit_debug (DEBUG_PLUGINS, "");
	g_return_val_if_fail (window != NULL, PLUGIN_ERROR);

	mdi = gedit_get_mdi ();
	g_return_val_if_fail (window != NULL, PLUGIN_ERROR);

	uic = gedit_get_ui_component_from_window (window);
	doc = gedit_get_active_document ();

	if (doc == NULL ||
	    gedit_document_is_readonly (doc) ||
	    gedit_mdi_get_state (mdi) != GEDIT_STATE_NORMAL)
	{
		gedit_menus_set_verb_list_sensitive (uic, changecase_sensible_verbs, FALSE);
	}
	else
	{
		gedit_menus_set_verb_list_sensitive (uic, changecase_sensible_verbs, TRUE);
	}

	return PLUGIN_OK;
}

G_MODULE_EXPORT GeditPluginState
activate (GeditPlugin *plugin)
{
	GList *top_windows;
	gedit_debug (DEBUG_PLUGINS, "");

	top_windows = gedit_get_top_windows ();
	g_return_val_if_fail (top_windows != NULL, PLUGIN_ERROR);

	while (top_windows)
	{
		gedit_menus_add_submenu (BONOBO_WINDOW (top_windows->data),
		                         SUBMENU_PATH, SUBMENU_NAME,
		                         SUBMENU_LABEL);

		gedit_menus_add_menu_item (BONOBO_WINDOW (top_windows->data),
		                           MENU_ITEMS_PATH, "AllUpperCase",
		                           UPPER_ITEM_LABEL, UPPER_ITEM_TIP,
		                           NULL, upper_case_cb);
		gedit_menus_add_menu_item (BONOBO_WINDOW (top_windows->data),
		                           MENU_ITEMS_PATH, "AllLowerCase",
		                           LOWER_ITEM_LABEL, LOWER_ITEM_TIP,
		                           NULL, lower_case_cb);
		gedit_menus_add_menu_item (BONOBO_WINDOW (top_windows->data),
		                           MENU_ITEMS_PATH, "InvertCase",
		                           INVERT_ITEM_LABEL, INVERT_ITEM_TIP,
		                           NULL, invert_case_cb);
		gedit_menus_add_menu_item (BONOBO_WINDOW (top_windows->data),
		                           MENU_ITEMS_PATH, "TitleCase",
		                           TITLE_ITEM_LABEL, TITLE_ITEM_TIP,
		                           NULL, title_case_cb);

		plugin->update_ui (plugin, BONOBO_WINDOW (top_windows->data));

		top_windows = g_list_next (top_windows);
        }

	return PLUGIN_OK;
}

G_MODULE_EXPORT GeditPluginState
deactivate (GeditPlugin *plugin)
{
	gedit_menus_remove_submenu_all (SUBMENU_PATH, SUBMENU_NAME);

	return PLUGIN_OK;
}

G_MODULE_EXPORT GeditPluginState
init (GeditPlugin *plugin)
{
	gedit_debug (DEBUG_PLUGINS, "");

	return PLUGIN_OK;
}

G_MODULE_EXPORT GeditPluginState
destroy (GeditPlugin *plugin)
{
	gedit_debug (DEBUG_PLUGINS, "");

	return PLUGIN_OK;
}

