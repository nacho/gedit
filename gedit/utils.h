/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit
 *
 * Copyright (C) 1998, 1999, 2000 Alex Roberts, Evan Lawrence, Jason Leach, Jose M Celorio
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 * Authors: 
 *   Jason Leach <leach@wam.umd.edu>
 *   Chema Celorio <chema@celorio.com>
 */

#ifndef __GEDIT_UTILS_H__
#define __GEDIT_UTILS_H__

#include "debug.h" /* Include this here so all the files currently
                      doing #include "util.h" can use debug stuff */

#define gedit_editable_active() GTK_EDITABLE(GEDIT_VIEW (gedit_view_active())->text)

void	gedit_flash     (gchar *msg);
void	gedit_flash_va  (gchar *format, ...);

/* Radio buttons utility functions */
gint	gtk_radio_group_get_selected (GSList *radio_group);
void	gtk_radio_button_select (GSList *group, int n);

typedef enum {
	GEDIT_PROGRAM_OK,
	GEDIT_PROGRAM_NOT_EXISTS,
	GEDIT_PROGRAM_IS_DIRECTORY,
	GEDIT_PROGRAM_IS_INSIDE_DIRECTORY,
	GEDIT_PROGRAM_NOT_EXECUTABLE
} GeditProgramErrors;

gint	gedit_utils_is_program (gchar * program, gchar* default_name);

void 	gedit_utils_delete_temp (gchar* file_name);
gchar *	gedit_utils_create_temp_from_doc (GeditDocument *doc, gint number);
void	gedit_utils_error_dialog (gchar *error_message, GtkWidget *widget);

#endif /* __GEDIT_UTILS_H__ */
