/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-utils.h
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

#ifndef __GEDIT_UTILS_H__
#define __GEDIT_UTILS_H__

#include <glib.h>
#include <gtk/gtkwidget.h>
#include <gtk/gtkdialog.h>
#include <gtk/gtktextiter.h>
#include <atk/atk.h>
#include <gedit/gedit-encodings.h>


/* some common error strings, %s must be a file path */
#define MISSING_FILE    N_("Could not find \"%s\". Please, reinstall gedit.")
#define MISSING_WIDGETS N_("Could not find the required widgets inside\"%s\". Please, reinstall gedit.")


void	gedit_utils_flash     (const gchar *msg);
void	gedit_utils_flash_va  (gchar *format, ...);

void	gedit_utils_set_status    (const gchar *msg);
void	gedit_utils_set_status_va (gchar *format, ...);


gboolean gedit_utils_is_uri_read_only (const gchar* uri);
gboolean gedit_utils_uri_has_file_scheme (const gchar *uri);

GtkWidget *gedit_dialog_add_button (GtkDialog *dialog, 
				    const gchar* text, 
				    const gchar* stock_id, 
				    gint response_id);

gchar *gedit_utils_str_middle_truncate (const gchar *string, 
					guint truncate_length);

void gedit_utils_error_reporting_loading_file (const gchar *uri, 
					       const GeditEncoding *encoding,
					       GError *error,
					       GtkWindow *parent);
void gedit_utils_error_reporting_saving_file  (const gchar *uri, 
					       GError *error,
					       GtkWindow *parent);
void gedit_utils_error_reporting_reverting_file (const gchar *uri, 
					       const GError *error,
					       GtkWindow *parent);
void gedit_utils_error_reporting_creating_file (const gchar *uri,
						gint error_code,
						GtkWindow *parent);

gboolean g_utf8_caselessnmatch (const char *s1, const char *s2, gssize n1, gssize n2);

void gedit_utils_set_atk_name_description (GtkWidget *widget, const gchar *name,
						const gchar *description);
void gedit_utils_set_atk_relation (GtkWidget *obj1, GtkWidget *obj2,
						AtkRelationType rel_type);

gboolean gedit_utils_uri_exists (const gchar* text_uri);

gchar *gedit_utils_convert_search_text (const gchar *text);

gchar *gedit_utils_get_stdin (void);

void gedit_warning (GtkWindow *parent, gchar *format, ...);

gchar *gedit_utils_str_middle_truncate (const char *string,
					guint truncate_length);

#endif /* __GEDIT_UTILS_H__ */


