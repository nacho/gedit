/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-mdi.h
 * This file is part of gedit
 *
 * Copyright (C) 1998, 1999 Alex Roberts, Evan Lawrence
 * Copyright (C) 2000, 2001 Chema Celorio, Paolo Maggi 
 * Copyright (C) 2002  Paolo Maggi 
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
 * Modified by the gedit Team, 1998-2002. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 */

#ifndef __GEDIT_MDI_H__
#define __GEDIT_MDI_H__

#include <bonobo-mdi.h>
#include "gedit-recent.h"
#include "recent-files/egg-recent-view.h"

#define GEDIT_TYPE_MDI			(gedit_mdi_get_type ())
#define GEDIT_MDI(obj)			(GTK_CHECK_CAST ((obj), GEDIT_TYPE_MDI, GeditMDI))
#define GEDIT_MDI_CLASS(klass)		(GTK_CHECK_CLASS_CAST ((klass), GEDIT_TYPE_MDI, GeditMDIClass))
#define GEDIT_IS_MDI(obj)		(GTK_CHECK_TYPE ((obj), GEDIT_TYPE_MDI))
#define GEDIT_IS_MDI_CLASS(klass)  	(GTK_CHECK_CLASS_TYPE ((klass), GEDIT_TYPE_MDI))
#define GEDIT_MDI_GET_CLASS(obj)  	(GTK_CHECK_GET_CLASS ((obj), GEDIT_TYPE_MDI, GeditMdiClass))


typedef struct _GeditMDI		GeditMDI;
typedef struct _GeditMDIClass		GeditMDIClass;

typedef struct _GeditMDIPrivate		GeditMDIPrivate;

struct _GeditMDI
{
	BonoboMDI mdi;
	
	GeditMDIPrivate *priv;
};

struct _GeditMDIClass
{
	BonoboMDIClass parent_class;
};


GtkType        	gedit_mdi_get_type 	(void) G_GNUC_CONST;

GeditMDI*	gedit_mdi_new		(void);

void		gedit_mdi_set_active_window_title (BonoboMDI *mdi);

/* FIXME: should be static ??? */
void 		gedit_mdi_set_active_window_verbs_sensitivity (BonoboMDI *mdi);

void 		gedit_mdi_clear_active_window_statusbar (GeditMDI *mdi);

EggRecentView	*gedit_mdi_get_recent_view_from_window (BonoboWindow* win);

#endif /* __GEDIT_MDI_H__ */

