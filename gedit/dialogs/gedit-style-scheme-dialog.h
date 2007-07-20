/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-style-scheme-dialog.h
 * This file is part of gedit
 *
 * Copyright (C) 2007 Paolo Borelli
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
 * Modified by the gedit Team, 2003. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 *
 * $Id: gedit-preferences-dialog.h 4429 2005-12-12 17:28:04Z pborelli $
 */

#ifndef __GEDIT_STYLE_SCHEME_DIALOG_H__
#define __GEDIT_STYLE_SCHEME_DIALOG_H__

#include <gtk/gtk.h>

#include "gedit-style-scheme-generator.h"

G_BEGIN_DECLS

/*
 * Type checking and casting macros
 */
#define GEDIT_TYPE_STYLE_SCHEME_DIALOG              (gedit_style_scheme_dialog_get_type())
#define GEDIT_STYLE_SCHEME_DIALOG(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), GEDIT_TYPE_STYLE_SCHEME_DIALOG, GeditStyleSchemeDialog))
#define GEDIT_STYLE_SCHEME_DIALOG_CONST(obj)        (G_TYPE_CHECK_INSTANCE_CAST((obj), GEDIT_TYPE_STYLE_SCHEME_DIALOG, GeditStyleSchemeDialog const))
#define GEDIT_STYLE_SCHEME_DIALOG_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass), GEDIT_TYPE_STYLE_SCHEME_DIALOG, GeditStyleSchemeDialogClass))
#define GEDIT_IS_STYLE_SCHEME_DIALOG(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), GEDIT_TYPE_STYLE_SCHEME_DIALOG))
#define GEDIT_IS_STYLE_SCHEME_DIALOG_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GEDIT_TYPE_STYLE_SCHEME_DIALOG))
#define GEDIT_STYLE_SCHEME_DIALOG_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), GEDIT_TYPE_STYLE_SCHEME_DIALOG, GeditStyleSchemeDialogClass))


/* Private structure type */
typedef struct _GeditStyleSchemeDialogPrivate GeditStyleSchemeDialogPrivate;

/*
 * Main object structure
 */
typedef struct _GeditStyleSchemeDialog GeditStyleSchemeDialog;

struct _GeditStyleSchemeDialog 
{
	GtkDialog dialog;
	
	/*< private > */
	GeditStyleSchemeDialogPrivate *priv;
};

/*
 * Class definition
 */
typedef struct _GeditStyleSchemeDialogClass GeditStyleSchemeDialogClass;

struct _GeditStyleSchemeDialogClass 
{
	GtkDialogClass parent_class;
};

/*
 * Public methods
 */
GType		 gedit_style_scheme_dialog_get_type	(void) G_GNUC_CONST;

GtkWidget	*gedit_style_scheme_dialog_new		(GeditStyleSchemeGenerator *generator);

G_END_DECLS

#endif /* __GEDIT_STYLE_SCHEME_DIALOG_H__ */

