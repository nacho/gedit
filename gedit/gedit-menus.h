/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-menus.h
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

#ifndef __GEDIT_MENU_H__
#define __GEDIT_MENU_H__

#include <bonobo/bonobo-ui-component.h>
#include <bonobo/bonobo-ui-engine.h>
#include <bonobo/bonobo-window.h>

extern BonoboUIVerb gedit_verbs [];
extern gchar* gedit_menus_ro_sensible_verbs [];
extern gchar* gedit_menus_no_docs_sensible_verbs []; 
extern gchar* gedit_menus_untitled_doc_sensible_verbs [];
extern gchar* gedit_menus_not_modified_doc_sensible_verbs []; 

#define gedit_menus_all_sensible_verbs gedit_menus_no_docs_sensible_verbs

void gedit_menus_set_verb_sensitive 		(BonoboUIComponent *ui_component,
					 	 gchar             *cname,
						 gboolean           sensitive);
void gedit_menus_set_verb_list_sensitive 	(BonoboUIComponent *ui_component, 
						 gchar            **vlist,
						 gboolean           sensitive);
void gedit_menus_set_verb_state 		(BonoboUIComponent *ui_component, 
						 gchar* cname, 
						 gboolean state);

/* convenience functions for plugins */

void gedit_menus_add_menu_item (BonoboWindow *window,
		     const gchar *path,
		     const gchar *name,
		     const gchar *label,
		     const gchar *tooltip,
		     const gchar *stock_pixmap,
		     BonoboUIVerbFn cb);

void gedit_menus_remove_menu_item (BonoboWindow *window,
		     const gchar *path,
		     const gchar *name);


void gedit_menus_add_menu_item_all (const gchar *path,
		     const gchar *name,
		     const gchar *label,
		     const gchar *tooltip,
		     const gchar *stock_pixmap,
		     BonoboUIVerbFn cb);

void gedit_menus_remove_menu_item_all (const gchar *path,
		     const gchar *name);

void gedit_menus_add_menu_item_toggle (BonoboWindow *window, 
		     const gchar *path,
		     const gchar *name, 
		     const gchar *label, 
		     const gchar *tooltip, 
		     BonoboUIListenerFn lt, 
		     gpointer data);

void gedit_menus_add_menu_item_toggle_all (const gchar *path,
		     const gchar *name, 
		     const gchar *label, 
		     const gchar *tooltip, 
		     BonoboUIListenerFn lt, 
		     gpointer data);

void gedit_menus_add_menu_item_radio (BonoboWindow *window, 
		     const gchar *path,
		     const gchar *name, 
		     const gchar *group, 
		     const gchar *label, 
		     const gchar *tooltip, 
		     BonoboUIListenerFn lt, 
		     gpointer data);

void gedit_menus_add_menu_item_radio_all (const gchar *path,
		     const gchar *name, 
		     const gchar *group, 
		     const gchar *label, 
		     const gchar *tooltip, 
		     BonoboUIListenerFn lt, 
		     gpointer data);

#define gedit_menus_remove_menu_item_toggle 	gedit_menus_remove_menu_item
#define gedit_menus_remove_menu_item_toggle_all	gedit_menus_remove_menu_item_all
#define gedit_menus_remove_menu_item_radio 	gedit_menus_remove_menu_item
#define gedit_menus_remove_menu_item_radio_all	gedit_menus_remove_menu_item_all


void gedit_menus_add_submenu (BonoboWindow *window, 
		     const gchar *path,
		     const gchar *name, 
		     const gchar *label);

void gedit_menus_add_submenu_all (const gchar *path, 
		     const gchar *name, 
		     const gchar *label);

void gedit_menus_remove_submenu (BonoboWindow *window, 
		     const gchar *path,
		     const gchar *name);

void gedit_menus_remove_submenu_all (const gchar *path,
		     const gchar *name);

#endif /* __GEDIT_MENU_H__ */
