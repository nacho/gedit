/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-mdi-child.h
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

#ifndef __GEDIT_MDI_CHILD_H__
#define __GEDIT_MDI_CHILD_H__

#include "bonobo-mdi.h"

#include "gedit-document.h"

#define GEDIT_TYPE_MDI_CHILD		(gedit_mdi_child_get_type ())
#define GEDIT_MDI_CHILD(o)		(GTK_CHECK_CAST ((o), GEDIT_TYPE_MDI_CHILD, GeditMDIChild))
#define GEDIT_MDI_CHILD_CLASS(klass)	(GTK_CHECK_CLASS_CAST ((klass), GEDIT_TYPE_MDI_CHILD, \
					GeditMDIChildClass))
#define GEDIT_IS_MDI_CHILD(o)		(GTK_CHECK_TYPE ((o), GEDIT_TYPE_MDI_CHILD))
#define GEDIT_IS_MDI_CHILD_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GEDIT_TYPE_MDI_CHILD))
#define GEDIT_MDI_CHILD_GET_CLASS(o)    (GTK_CHECK_GET_CLASS ((o), GEDIT_TYPE_MDI_CHILD, GeditMdiChildClass))


typedef struct _GeditMDIChild		GeditMDIChild;
typedef struct _GeditMDIChildClass	GeditMDIChildClass;

typedef struct _GeditMDIChildPrivate	GeditMDIChildPrivate;

struct _GeditMDIChild
{
	BonoboMDIChild child;
	
	GeditDocument* document;
	GeditMDIChildPrivate *priv;
};

struct _GeditMDIChildClass
{
	BonoboMDIChildClass parent_class;

	/* MDI child state changed */
	void (*state_changed)		(GeditMDIChild *child);

	void (*undo_redo_state_changed)	(GeditMDIChild *child);
};


GtkType        	gedit_mdi_child_get_type 	(void) G_GNUC_CONST;

GeditMDIChild*	gedit_mdi_child_new		(void);
GeditMDIChild*  gedit_mdi_child_new_with_uri 	(const gchar *uri, GError **error);

#endif /* __GEDIT_MDI_CHILD_H__ */

