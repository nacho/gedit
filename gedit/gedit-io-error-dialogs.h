/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-io-error-dialogs.h
 * This file is part of gedit
 *
 * Copyright (C) 1998, 1999 Alex Roberts, Evan Lawrence
 * Copyright (C) 2000, 2001 Chema Celorio, Paolo Maggi
 * Copyright (C) 2002, 2004 Paolo Maggi
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

#ifndef __GEDIT_IO_ERROR_DIALOGS_H__
#define __GEDIT_IO_ERROR_DIALOGS_H__

#include <glib.h>
#include <gtk/gtkwindow.h>
#include <gedit/gedit-encodings.h>


void gedit_error_reporting_loading_file (const gchar *uri, 
					 const GeditEncoding *encoding,
					 GError *error,
					 GtkWindow *parent);

void gedit_error_reporting_reverting_file (const gchar *uri, 
					   const GError *error,
					   GtkWindow *parent);

void gedit_error_reporting_saving_file (const gchar *uri, 
					GError *error,
					GtkWindow *parent);

void gedit_error_reporting_creating_file (const gchar *uri,
					  gint error_code,
					  GtkWindow *parent);

#endif /* __GEDIT_IO_ERROR_DIALOGS_H__ */
