/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-commands.h
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

#ifndef __GEDIT_COMMANDS_H__
#define __GEDIT_COMMANDS_H__

#include <bonobo/bonobo-ui-component.h>

void gedit_cmd_file_new 	(BonoboUIComponent *uic, gpointer user_data, const gchar* verbname);
void gedit_cmd_file_open 	(BonoboUIComponent *uic, gpointer user_data, const gchar* verbname);
void gedit_cmd_file_save 	(BonoboUIComponent *uic, gpointer user_data, const gchar* verbname);
void gedit_cmd_file_save_as     (BonoboUIComponent *uic, gpointer user_data, const gchar* verbname);
void gedit_cmd_file_save_all    (BonoboUIComponent *uic, gpointer user_data, const gchar* verbname);
void gedit_cmd_file_revert      (BonoboUIComponent *uic, gpointer user_data, const gchar* verbname);
void gedit_cmd_file_open_uri    (BonoboUIComponent *uic, gpointer user_data, const gchar* verbname);
void gedit_cmd_file_page_setup  (BonoboUIComponent *uic, gpointer user_data, const gchar* verbname);
void gedit_cmd_file_print       (BonoboUIComponent *uic, gpointer user_data, const gchar* verbname);
void gedit_cmd_file_print_preview 
				(BonoboUIComponent *uic, gpointer user_data, const gchar* verbname);
void gedit_cmd_file_close       (BonoboUIComponent *uic, gpointer user_data, const gchar* verbname);
void gedit_cmd_file_close_all   (BonoboUIComponent *uic, gpointer user_data, const gchar* verbname);
void gedit_cmd_file_exit        (BonoboUIComponent *uic, gpointer user_data, const gchar* verbname);

void gedit_cmd_edit_undo 	(BonoboUIComponent *uic, gpointer user_data, const gchar* verbname);
void gedit_cmd_edit_redo 	(BonoboUIComponent *uic, gpointer user_data, const gchar* verbname);
void gedit_cmd_edit_cut 	(BonoboUIComponent *uic, gpointer user_data, const gchar* verbname);
void gedit_cmd_edit_copy 	(BonoboUIComponent *uic, gpointer user_data, const gchar* verbname);
void gedit_cmd_edit_paste 	(BonoboUIComponent *uic, gpointer user_data, const gchar* verbname);
void gedit_cmd_edit_clear 	(BonoboUIComponent *uic, gpointer user_data, const gchar* verbname);
void gedit_cmd_edit_select_all 	(BonoboUIComponent *uic, gpointer user_data, const gchar* verbname);

void gedit_cmd_search_find      (BonoboUIComponent *uic, gpointer user_data, const gchar* verbname);
void gedit_cmd_search_find_next	(BonoboUIComponent *uic, gpointer user_data, const gchar* verbname);
void gedit_cmd_search_find_prev	(BonoboUIComponent *uic, gpointer user_data, const gchar* verbname);
void gedit_cmd_search_replace   (BonoboUIComponent *uic, gpointer user_data, const gchar* verbname);
void gedit_cmd_search_goto_line (BonoboUIComponent *uic, gpointer user_data, const gchar* verbname);

void gedit_cmd_settings_preferences 
				(BonoboUIComponent *uic, gpointer user_data, const gchar* verbname);

void gedit_cmd_documents_move_to_new_window 
				(BonoboUIComponent *uic, gpointer user_data, const gchar* verbname);

void gedit_cmd_help_contents 	(BonoboUIComponent *uic, gpointer user_data, const gchar* verbname);
void gedit_cmd_help_about 	(BonoboUIComponent *uic, gpointer user_data, const gchar* verbname);

#endif /* __GEDIT_COMMANDS_H__ */ 
