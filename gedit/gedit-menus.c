/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-menus.c
 * This file is part of gedit
 *
 * Copyright (C) 1998, 1999 Alex Roberts, Evan Lawrence
 * Copyright (C) 2000, 2001 Chema Celorio, Paolo Maggi 
 * Copyright (C) 2002  Paolo Maggi 
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
 * Modified by the gedit Team, 1998-2002. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "gedit-menus.h"
#include "gedit-commands.h"
#include "gedit2.h"

BonoboUIVerb gedit_verbs [] = {
	BONOBO_UI_VERB ("FileNew", gedit_cmd_file_new),
	BONOBO_UI_VERB ("FileOpen", gedit_cmd_file_open),
	BONOBO_UI_VERB ("FileSave", gedit_cmd_file_save),
	BONOBO_UI_VERB ("FileSaveAs", gedit_cmd_file_save_as),
	BONOBO_UI_VERB ("FileSaveAll", gedit_cmd_file_save_all),
	BONOBO_UI_VERB ("FileRevert", gedit_cmd_file_revert),
	BONOBO_UI_VERB ("FileOpenURI", gedit_cmd_file_open_uri),
	BONOBO_UI_VERB ("FilePrint", gedit_cmd_file_print),
	BONOBO_UI_VERB ("FilePrintPreview", gedit_cmd_file_print_preview),
	BONOBO_UI_VERB ("FileClose", gedit_cmd_file_close),
	BONOBO_UI_VERB ("FileCloseAll", gedit_cmd_file_close_all),
	BONOBO_UI_VERB ("FileExit", gedit_cmd_file_exit),
	BONOBO_UI_VERB ("EditUndo", gedit_cmd_edit_undo),
	BONOBO_UI_VERB ("EditRedo", gedit_cmd_edit_redo),
	BONOBO_UI_VERB ("EditCut", gedit_cmd_edit_cut),
	BONOBO_UI_VERB ("EditCopy", gedit_cmd_edit_copy),
	BONOBO_UI_VERB ("EditPaste", gedit_cmd_edit_paste),
	BONOBO_UI_VERB ("EditClear", gedit_cmd_edit_clear),
	BONOBO_UI_VERB ("EditSelectAll", gedit_cmd_edit_select_all),
	BONOBO_UI_VERB ("SearchFind", gedit_cmd_search_find),
	BONOBO_UI_VERB ("SearchFindNext", gedit_cmd_search_find_next),
	BONOBO_UI_VERB ("SearchFindPrev", gedit_cmd_search_find_prev),
	BONOBO_UI_VERB ("SearchReplace", gedit_cmd_search_replace),
	BONOBO_UI_VERB ("SearchGoToLine", gedit_cmd_search_goto_line),
	BONOBO_UI_VERB ("SettingsPreferences", gedit_cmd_settings_preferences),
	BONOBO_UI_VERB ("DocumentsMoveToNewWindow", gedit_cmd_documents_move_to_new_window),
	BONOBO_UI_VERB ("HelpContents", gedit_cmd_help_contents),
	BONOBO_UI_VERB ("About", gedit_cmd_help_about),

	BONOBO_UI_VERB_END
};

gchar* gedit_menus_ro_sensible_verbs [] = {
	"/commands/FileSave",
	"/commands/FileRevert",
	"/commands/EditUndo",
	"/commands/EditRedo",
	"/commands/EditCut",
	"/commands/EditPaste",
	"/commands/EditClear",
	"/commands/SearchReplace",

	NULL
};

gchar* gedit_menus_no_docs_sensible_verbs [] = {
	"/commands/FileSave",
	"/commands/FileSaveAll",
	"/commands/FileSaveAs",
	"/commands/FileRevert",
	"/commands/FilePrint",
	"/commands/FilePrintPreview",
	"/commands/FileClose",
	"/commands/FileCloseAll",
	"/commands/EditUndo",
	"/commands/EditRedo",
	"/commands/EditCut",
	"/commands/EditCopy",
	"/commands/EditPaste",
	"/commands/EditClear",
	"/commands/EditSelectAll",
	"/commands/SearchFind",
	"/commands/SearchFindNext",
	"/commands/SearchFindPrev",
	"/commands/SearchReplace",
	"/commands/SearchGoToLine",
	"/commands/DocumentsMoveToNewWindow",
	NULL
};

gchar* gedit_menus_untitled_doc_sensible_verbs [] = {
	"/commands/FileRevert",

	NULL
};

gchar* gedit_menus_not_modified_doc_sensible_verbs [] = {
	/*
	"/commands/FileSave",
	*/
	"/commands/FileRevert",

	NULL
};


void
gedit_menus_set_verb_sensitive (BonoboUIComponent *ui_component, gchar* cname, gboolean sensitive)
{
	g_return_if_fail (cname != NULL);
	g_return_if_fail (BONOBO_IS_UI_COMPONENT (ui_component));

	bonobo_ui_component_set_prop (
		ui_component, cname, "sensitive", sensitive ? "1" : "0", NULL);
}

void
gedit_menus_set_verb_list_sensitive (BonoboUIComponent *ui_component, gchar** vlist, gboolean sensitive)
{
	g_return_if_fail (vlist != NULL);
	g_return_if_fail (BONOBO_IS_UI_COMPONENT (ui_component));

	for ( ; *vlist; ++vlist)
	{
		bonobo_ui_component_set_prop (
			ui_component, *vlist, "sensitive", sensitive ? "1" : "0", NULL);
	}
}

void
gedit_menus_set_verb_state (BonoboUIComponent *ui_component, gchar* cname, gboolean state)
{
	g_return_if_fail (cname != NULL);
	g_return_if_fail (BONOBO_IS_UI_COMPONENT (ui_component));

	bonobo_ui_component_set_prop (
		ui_component, cname, "state", state ? "1" : "0", NULL);
}

void
gedit_menus_add_menu_item (BonoboWindow *window, const gchar *path,
		     const gchar *name, const gchar *label,
		     const gchar *tooltip, const gchar *stock_pixmap,
		     BonoboUIVerbFn cb)
{
	BonoboUIComponent *ui_component;
	gchar *item_path;
	gchar *cmd;

	g_return_if_fail (window != NULL);
	g_return_if_fail (path != NULL);
	g_return_if_fail (label != NULL);
	g_return_if_fail (cb != NULL);
	
	item_path = g_strconcat (path, name, NULL);
	ui_component = gedit_get_ui_component_from_window (BONOBO_WINDOW (window));
	if (!bonobo_ui_component_path_exists (ui_component, item_path, NULL)) {
		gchar *xml;

		xml = g_strdup_printf ("<menuitem name=\"%s\" verb=\"\""
				       " _label=\"%s\""
				       " _tip=\"%s\" hidden=\"0\" />", name,
				       label, tooltip);


		if (stock_pixmap != NULL) {
			cmd = g_strdup_printf ("<cmd name=\"%s\""
				" pixtype=\"stock\" pixname=\"%s\" />",
				name, stock_pixmap);
		}
		else {
			cmd = g_strdup_printf ("<cmd name=\"%s\" />", name);
		}


		bonobo_ui_component_set_translate (ui_component, path,
						   xml, NULL);

		bonobo_ui_component_set_translate (ui_component, "/commands/",
						   cmd, NULL);
						   
		bonobo_ui_component_add_verb (ui_component, name, cb, NULL);

		g_free (xml);
		g_free (cmd);
	}

	g_free (item_path);
}

void
gedit_menus_remove_menu_item (BonoboWindow *window, const gchar *path,
			const gchar *name)
{
	BonoboUIComponent *ui_component;
	gchar *item_path;

	g_return_if_fail (window != NULL);
	g_return_if_fail (path != NULL);
	g_return_if_fail (name != NULL);

	item_path = g_strconcat (path, name, NULL);
	ui_component = gedit_get_ui_component_from_window (BONOBO_WINDOW (window));

	if (bonobo_ui_component_path_exists (ui_component, item_path, NULL)) {
		gchar *cmd;

		cmd = g_strdup_printf ("/commands/%s", name);
		
		bonobo_ui_component_rm (ui_component, item_path, NULL);
		bonobo_ui_component_rm (ui_component, cmd, NULL);
		
		g_free (cmd);
	}

	g_free (item_path);
}

void
gedit_menus_add_menu_item_all (const gchar *path, const gchar *name,
			 const gchar *label, const gchar *tooltip,
			 const gchar *stock_pixmap,
			 BonoboUIVerbFn cb)
{
	GList* top_windows;
	
	top_windows = gedit_get_top_windows ();
	g_return_if_fail (top_windows != NULL);
       
	while (top_windows)
	{
		BonoboWindow* window = BONOBO_WINDOW (top_windows->data);


		gedit_menus_add_menu_item (window, path, name, label, tooltip,
				     stock_pixmap, cb);
		
		top_windows = g_list_next (top_windows);
	}
}

void
gedit_menus_remove_menu_item_all (const gchar *path, const gchar *name)
{
	GList* top_windows;
	
	top_windows = gedit_get_top_windows ();
	g_return_if_fail (top_windows != NULL);
       
	while (top_windows)
	{
		BonoboWindow* window = BONOBO_WINDOW (top_windows->data);

		gedit_menus_remove_menu_item (window, path, name);
		
		top_windows = g_list_next (top_windows);
	}
}


void
gedit_menus_add_menu_item_toggle (BonoboWindow *window, const gchar *path,
		     const gchar *name, const gchar *label, const gchar *tooltip, 
		     BonoboUIListenerFn lt, gpointer data)
{
	BonoboUIComponent *ui_component;
	gchar *item_path;
	gchar *cmd;

	g_return_if_fail (window != NULL);
	g_return_if_fail (path != NULL);
	g_return_if_fail (label != NULL);
	g_return_if_fail (lt != NULL);
	
	item_path = g_strconcat (path, name, NULL);
	ui_component = gedit_get_ui_component_from_window (BONOBO_WINDOW (window));
	if (!bonobo_ui_component_path_exists (ui_component, item_path, NULL)) 
	{
		gchar *xml;

		xml = g_strdup_printf ("<menuitem name=\"%s\" id=\"%s\" verb=\"\" />",
				       name, name);

		cmd = g_strdup_printf ("<cmd name=\"%s\" _label=\"%s\" type=\"toggle\" _tip=\"%s\" state=\"0\"/>", 
				name, label, tooltip);
		
		bonobo_ui_component_set_translate (ui_component, "/commands/",
						   cmd, NULL);

		bonobo_ui_component_set_translate (ui_component, path,
						   xml, NULL);

		bonobo_ui_component_add_listener (ui_component, name, lt, data);

		g_free (xml);
		g_free (cmd);
	}

	g_free (item_path);
}


void
gedit_menus_add_menu_item_toggle_all (const gchar *path,
		     const gchar *name, const gchar *label, const gchar *tooltip, 
		     BonoboUIListenerFn lt, gpointer data)
{
	GList* top_windows;
	
	top_windows = gedit_get_top_windows ();
	g_return_if_fail (top_windows != NULL);
       
	while (top_windows)
	{
		BonoboWindow* window = BONOBO_WINDOW (top_windows->data);


		gedit_menus_add_menu_item_toggle (window, path, name, label, tooltip,
				     lt, data);
		
		top_windows = g_list_next (top_windows);
	}
}

void
gedit_menus_add_menu_item_radio (BonoboWindow *window, const gchar *path,
		     const gchar *name, const gchar *group, const gchar *label, const gchar *tooltip, 
		     BonoboUIListenerFn lt, gpointer data)
{
	BonoboUIComponent *ui_component;
	gchar *item_path;
	gchar *cmd;

	g_return_if_fail (window != NULL);
	g_return_if_fail (path != NULL);
	g_return_if_fail (label != NULL);
	g_return_if_fail (tooltip != NULL);
	g_return_if_fail (group != NULL);
	g_return_if_fail (lt != NULL);
	
	item_path = g_strconcat (path, name, NULL);
	ui_component = gedit_get_ui_component_from_window (BONOBO_WINDOW (window));
	if (!bonobo_ui_component_path_exists (ui_component, item_path, NULL)) 
	{
		gchar *xml;

		xml = g_strdup_printf ("<menuitem name=\"%s\" id=\"%s\" verb=\"\" />",
				       name, name);

		cmd = g_strdup_printf ("<cmd name=\"%s\" _label=\"%s\" type=\"radio\" group =\"%s\" "
				       "_tip=\"%s\" state=\"0\"/>", 
				       name, label, group, tooltip);
		
		bonobo_ui_component_set_translate (ui_component, "/commands/",
						   cmd, NULL);

		bonobo_ui_component_set_translate (ui_component, path,
						   xml, NULL);

		bonobo_ui_component_add_listener (ui_component, name, lt, data);

		g_free (xml);
		g_free (cmd);
	}

	g_free (item_path);
}


void
gedit_menus_add_menu_item_radio_all (const gchar *path,
		     const gchar *name, const gchar *group,const gchar *label, const gchar *tooltip, 
		     BonoboUIListenerFn lt, gpointer data)
{
	GList* top_windows;
	
	top_windows = gedit_get_top_windows ();
	g_return_if_fail (top_windows != NULL);
       
	while (top_windows)
	{
		BonoboWindow* window = BONOBO_WINDOW (top_windows->data);


		gedit_menus_add_menu_item_radio (window, path, name, group, label, tooltip,
				     lt, data);
		
		top_windows = g_list_next (top_windows);
	}
}

void
gedit_menus_add_submenu (BonoboWindow *window, const gchar *path,
		     const gchar *name, const gchar *label)
{
	BonoboUIComponent *ui_component;
	gchar *item_path;

	g_return_if_fail (window != NULL);
	g_return_if_fail (path != NULL);
	g_return_if_fail (label != NULL);
	
	item_path = g_strconcat (path, name, NULL);
	ui_component = gedit_get_ui_component_from_window (BONOBO_WINDOW (window));
	if (!bonobo_ui_component_path_exists (ui_component, item_path, NULL)) {
		gchar *xml;

		xml = g_strdup_printf ("<submenu name=\"%s\""
				       " _label=\"%s\" />", 
				       name, label);

		bonobo_ui_component_set_translate (ui_component, path,
						   xml, NULL);

		g_free (xml);
	}

	g_free (item_path);
}

void
gedit_menus_add_submenu_all (const gchar *path, const gchar *name, const gchar *label)
{
	GList* top_windows;
	
	top_windows = gedit_get_top_windows ();
	g_return_if_fail (top_windows != NULL);
       
	while (top_windows)
	{
		BonoboWindow* window = BONOBO_WINDOW (top_windows->data);

		gedit_menus_add_submenu (window, path, name, label);
		
		top_windows = g_list_next (top_windows);
	}

}

void
gedit_menus_remove_submenu (BonoboWindow *window, const gchar *path,  const gchar *name)
{
	BonoboUIComponent *ui_component;
	gchar *item_path;

	g_return_if_fail (window != NULL);
	g_return_if_fail (path != NULL);
	g_return_if_fail (name != NULL);

	item_path = g_strconcat (path, name, NULL);
	ui_component = gedit_get_ui_component_from_window (BONOBO_WINDOW (window));

	if (bonobo_ui_component_path_exists (ui_component, item_path, NULL)) 
		bonobo_ui_component_rm (ui_component, item_path, NULL);

	g_free (item_path);
}

void
gedit_menus_remove_submenu_all (const gchar *path, const gchar *name)
{
	GList* top_windows;
	
	top_windows = gedit_get_top_windows ();
	g_return_if_fail (top_windows != NULL);
       
	while (top_windows)
	{
		BonoboWindow* window = BONOBO_WINDOW (top_windows->data);

		gedit_menus_remove_submenu (window, path, name);
		
		top_windows = g_list_next (top_windows);
	}
}

