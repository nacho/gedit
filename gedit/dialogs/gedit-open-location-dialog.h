/*
 * gedit-open-location-dialog.h
 * This file is part of gedit
 *
 * Copyright (C) 2001-2005 Paolo Maggi 
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
 * Modified by the gedit Team, 2001-2005. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 *
 * $Id$
 */

#ifndef __GEDIT_OPEN_LOCATION_DIALOG_H__
#define __GEDIT_OPEN_LOCATION_DIALOG_H__

#include <gtk/gtk.h>
#include <gedit/gedit-encodings.h>

G_BEGIN_DECLS

/*
 * Type checking and casting macros
 */
#define GEDIT_TYPE_OPEN_LOCATION_DIALOG              (gedit_open_location_dialog_get_type())
#define GEDIT_OPEN_LOCATION_DIALOG(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), GEDIT_TYPE_OPEN_LOCATION_DIALOG, GeditOpenLocationDialog))
#define GEDIT_OPEN_LOCATION_DIALOG_CONST(obj)        (G_TYPE_CHECK_INSTANCE_CAST((obj), GEDIT_TYPE_OPEN_LOCATION_DIALOG, GeditOpenLocationDialog const))
#define GEDIT_OPEN_LOCATION_DIALOG_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass), GEDIT_TYPE_OPEN_LOCATION_DIALOG, GeditOpenLocationDialogClass))
#define GEDIT_IS_OPEN_LOCATION_DIALOG(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), GEDIT_TYPE_OPEN_LOCATION_DIALOG))
#define GEDIT_IS_OPEN_LOCATION_DIALOG_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GEDIT_TYPE_OPEN_LOCATION_DIALOG))
#define GEDIT_OPEN_LOCATION_DIALOG_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), GEDIT_TYPE_OPEN_LOCATION_DIALOG, GeditOpenLocationDialogClass))

/* Private structure type */
typedef struct _GeditOpenLocationDialogPrivate GeditOpenLocationDialogPrivate;

/*
 * Main object structure
 */
typedef struct _GeditOpenLocationDialog GeditOpenLocationDialog;

struct _GeditOpenLocationDialog 
{
	GtkDialog dialog;

	/*< private > */
	GeditOpenLocationDialogPrivate *priv;
};

/*
 * Class definition
 */
typedef struct _GeditOpenLocationDialogClass GeditOpenLocationDialogClass;

struct _GeditOpenLocationDialogClass 
{
	GtkDialogClass parent_class;
};

/*
 * Public methods
 */
GType	 		 gedit_open_location_dialog_get_type 		(void) G_GNUC_CONST;

GtkWidget		*gedit_open_location_dialog_new			(GtkWindow               *parent);

gchar			*gedit_open_location_dialog_get_uri		(GeditOpenLocationDialog *dlg);

const GeditEncoding	*gedit_open_location_dialog_get_encoding	(GeditOpenLocationDialog *dlg);

/* 
 * The widget automatically runs the help viewer when the Help button is pressed,
 * so there is no need to catch the GTK_RESPONSE_HELP response.
 * 
 * GTK_RESPONSE_OK response is emitted when the "Open" button is pressed.
 */
   
G_END_DECLS

#endif  /* __GEDIT_OPEN_LOCATION_DIALOG_H__  */
