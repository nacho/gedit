/*
 * gedit-replace-dialog.h
 * This file is part of gedit
 *
 * Copyright (C) 2005 Paolo Maggi
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
 * Modified by the gedit Team, 2005. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 *
 * $Id$
 */

#ifndef __GEDIT_REPLACE_DIALOG_H__
#define __GEDIT_REPLACE_DIALOG_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

/*
 * Type checking and casting macros
 */
#define GEDIT_TYPE_REPLACE_DIALOG              (gedit_replace_dialog_get_type())
#define GEDIT_REPLACE_DIALOG(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), GEDIT_TYPE_REPLACE_DIALOG, GeditReplaceDialog))
#define GEDIT_REPLACE_DIALOG_CONST(obj)        (G_TYPE_CHECK_INSTANCE_CAST((obj), GEDIT_TYPE_REPLACE_DIALOG, GeditReplaceDialog const))
#define GEDIT_REPLACE_DIALOG_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass), GEDIT_TYPE_REPLACE_DIALOG, GeditReplaceDialogClass))
#define GEDIT_IS_REPLACE_DIALOG(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), GEDIT_TYPE_REPLACE_DIALOG))
#define GEDIT_IS_REPLACE_DIALOG_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GEDIT_TYPE_REPLACE_DIALOG))
#define GEDIT_REPLACE_DIALOG_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), GEDIT_TYPE_REPLACE_DIALOG, GeditReplaceDialogClass))

/* Private structure type */
typedef struct _GeditReplaceDialogPrivate GeditReplaceDialogPrivate;

/*
 * Main object structure
 */
typedef struct _GeditReplaceDialog GeditReplaceDialog;

struct _GeditReplaceDialog 
{
	GtkDialog dialog;

	/*< private > */
	GeditReplaceDialogPrivate *priv;
};

/*
 * Class definition
 */
typedef struct _GeditReplaceDialogClass GeditReplaceDialogClass;

struct _GeditReplaceDialogClass
{
	GtkDialogClass parent_class;
};

enum
{
	GEDIT_REPLACE_DIALOG_FIND_RESPONSE = 100,
	GEDIT_REPLACE_DIALOG_REPLACE_RESPONSE,
	GEDIT_REPLACE_DIALOG_REPLACE_ALL_RESPONSE
};

/*
 * Public methods
 */
GType 		 gedit_replace_dialog_get_type 		(void) G_GNUC_CONST;

GtkWidget	*gedit_replace_dialog_new		(GtkWindow          *parent);

void		 gedit_replace_dialog_present_with_time	(GeditReplaceDialog *dialog,
							 guint32             timestamp);

void		 gedit_replace_dialog_set_search_text	(GeditReplaceDialog *dialog,
							 const gchar        *text);
const gchar	*gedit_replace_dialog_get_search_text	(GeditReplaceDialog *dialog);

void		 gedit_replace_dialog_set_replace_text	(GeditReplaceDialog *dialog,
							 const gchar        *text);
const gchar	*gedit_replace_dialog_get_replace_text	(GeditReplaceDialog *dialog);

void		 gedit_replace_dialog_set_match_case	(GeditReplaceDialog *dialog,
							 gboolean            match_case);
gboolean	 gedit_replace_dialog_get_match_case	(GeditReplaceDialog *dialog);

void		 gedit_replace_dialog_set_entire_word	(GeditReplaceDialog *dialog,
							 gboolean            entire_word);
gboolean	 gedit_replace_dialog_get_entire_word	(GeditReplaceDialog *dialog);

void		 gedit_replace_dialog_set_backwards	(GeditReplaceDialog *dialog,
							 gboolean            backwards);
gboolean	 gedit_replace_dialog_get_backwards	(GeditReplaceDialog *dialog);

void		 gedit_replace_dialog_set_wrap_around	(GeditReplaceDialog *dialog,
							 gboolean            wrap_around);
gboolean	 gedit_replace_dialog_get_wrap_around	(GeditReplaceDialog *dialog);
   
G_END_DECLS

#endif  /* __GEDIT_REPLACE_DIALOG_H__  */

/* ex:set ts=8 noet: */
