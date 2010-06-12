/*
 * gedit-progress-info-bar.h
 * This file is part of gedit
 *
 * Copyright (C) 2005 - Paolo Maggi 
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

#ifndef __GEDIT_PROGRESS_INFO_BAR_H__
#define __GEDIT_PROGRESS_INFO_BAR_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

/*
 * Type checking and casting macros
 */
#define GEDIT_TYPE_PROGRESS_INFO_BAR              (gedit_progress_info_bar_get_type())
#define GEDIT_PROGRESS_INFO_BAR(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), GEDIT_TYPE_PROGRESS_INFO_BAR, GeditProgressInfoBar))
#define GEDIT_PROGRESS_INFO_BAR_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass), GEDIT_TYPE_PROGRESS_INFO_BAR, GeditProgressInfoBarClass))
#define GEDIT_IS_PROGRESS_INFO_BAR(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), GEDIT_TYPE_PROGRESS_INFO_BAR))
#define GEDIT_IS_PROGRESS_INFO_BAR_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GEDIT_TYPE_PROGRESS_INFO_BAR))
#define GEDIT_PROGRESS_INFO_BAR_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), GEDIT_TYPE_PROGRESS_INFO_BAR, GeditProgressInfoBarClass))

/* Private structure type */
typedef struct _GeditProgressInfoBarPrivate GeditProgressInfoBarPrivate;

/*
 * Main object structure
 */
typedef struct _GeditProgressInfoBar GeditProgressInfoBar;

struct _GeditProgressInfoBar 
{
	GtkInfoBar parent;

	/*< private > */
	GeditProgressInfoBarPrivate *priv;
};

/*
 * Class definition
 */
typedef struct _GeditProgressInfoBarClass GeditProgressInfoBarClass;

struct _GeditProgressInfoBarClass 
{
	GtkInfoBarClass parent_class;
};

/*
 * Public methods
 */
GType 		 gedit_progress_info_bar_get_type 		(void) G_GNUC_CONST;

GtkWidget	*gedit_progress_info_bar_new      		(const gchar          *stock_id,
								 const gchar          *markup,
								 gboolean              has_cancel);

void		 gedit_progress_info_bar_set_stock_image	(GeditProgressInfoBar *bar,
								 const gchar          *stock_id);

void		 gedit_progress_info_bar_set_markup		(GeditProgressInfoBar *bar,
								 const gchar          *markup);

void		 gedit_progress_info_bar_set_text		(GeditProgressInfoBar *bar,
								 const gchar          *text);

void		 gedit_progress_info_bar_set_fraction		(GeditProgressInfoBar *bar,
								 gdouble               fraction);

void		 gedit_progress_info_bar_pulse			(GeditProgressInfoBar *bar);
								 

G_END_DECLS

#endif  /* __GEDIT_PROGRESS_INFO_BAR_H__  */

/* ex:ts=8:noet: */
