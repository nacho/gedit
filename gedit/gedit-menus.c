/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-menus.c
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
 * Boston, MA 02111-1307, USA. * *
 */
 
/*
 * Modified by the gedit Team, 1998-2001. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "gedit-menus.h"
#include "gedit-commands.h"


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
	BONOBO_UI_VERB ("SearchFindAgain", gedit_cmd_search_find_again),
	BONOBO_UI_VERB ("SearchReplace", gedit_cmd_search_replace),
	BONOBO_UI_VERB ("SearchGoToLine", gedit_cmd_search_goto_line),
	BONOBO_UI_VERB ("SettingsPreferences", gedit_cmd_settings_preferences),
	BONOBO_UI_VERB ("About", gedit_cmd_help_about),
	BONOBO_UI_VERB ("PluginsManager", gedit_cmd_tools_plugin_manager),

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
	"/commands/SearchFindAgain",
	"/commands/SearchReplace",
	"/commands/SearchGoToLine",

	NULL
};

gchar* gedit_menus_untitled_doc_sensible_verbs [] = {
	"/commands/FileRevert",

	NULL
};

gchar* gedit_menus_not_modified_doc_sensible_verbs [] = {
	"/commands/FileSave",
	"/commands/FileRevert",

	NULL
};


void
gedit_menus_set_verb_sensitive (BonoboUIEngine* ui_engine, gchar* cname, gboolean sensitive)
{
	g_return_if_fail (BONOBO_IS_UI_ENGINE (ui_engine));
	g_return_if_fail (cname != NULL);

	bonobo_ui_engine_xml_set_prop (ui_engine, cname, "sensitive", sensitive ? "1" : "0", "1");
}

void
gedit_menus_set_verb_list_sensitive (BonoboUIEngine* ui_engine, gchar** vlist, gboolean sensitive)
{
	g_return_if_fail (BONOBO_IS_UI_ENGINE (ui_engine));
	g_return_if_fail (vlist != NULL);

	for ( ; *vlist; ++vlist)
	{
		bonobo_ui_engine_xml_set_prop (ui_engine, *vlist, "sensitive", sensitive ? "1" : "0", "1");
	}
}
