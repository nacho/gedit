/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-preferences-dialog.h
 * This file is part of gedit
 *
 * Copyright (C) 2001 Paolo Maggi 
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

#ifndef __GEDIT_PREFERENCES_DIALOG_H__
#define __GEDIT_PREFERENCES_DIALOG_H__

#include <gtk/gtk.h>

#define GEDIT_TYPE_PREFERENCES_DIALOG            (gedit_preferences_dialog_get_type ())
#define GEDIT_PREFERENCES_DIALOG(obj)            (GTK_CHECK_CAST ((obj), GEDIT_TYPE_PREFERENCES_DIALOG, GeditPreferencesDialog))
#define GEDIT_PREFERENCES_DIALOG_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GEDIT_TYPE_PREFERENCES_DIALOG, GeditPreferencesDialogClass))
#define GEDIT_IS_PREFERENCES_DIALOG(obj)         (GTK_CHECK_TYPE ((obj), GEDIT_TYPE_PREFERENCES_DIALOG))
#define GEDIT_IS_PREFERENCES_DIALOG_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GEDIT_TYPE_PREFERENCES_DIALOG))
#define GEDIT_PREFERENCES_DIALOG_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), GEDIT_TYPE_PREFERENCES_DIALOG, GeditPreferencesDialogClass))


typedef struct _GeditPreferencesDialog 		GeditPreferencesDialog;
typedef struct _GeditPreferencesDialogClass	GeditPreferencesDialogClass;

typedef struct _GeditPreferencesDialogPrivate	GeditPreferencesDialogPrivate;

struct _GeditPreferencesDialog
{
	GtkDialog parent_instance;

	GeditPreferencesDialogPrivate *priv;

};

struct  _GeditPreferencesDialogClass
{
	GtkDialogClass parent_class;
};

GtkType    	gedit_preferences_dialog_get_type 	(void) G_GNUC_CONST;

GtkWidget*	gedit_preferences_dialog_new		(GtkWindow *parent);

gboolean 	gedit_preferences_dialog_update_settings (GeditPreferencesDialog *dlg);

#endif /* __GEDIT_PREFERENCES_DIALOG_H__ */
