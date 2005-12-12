/*
 * gedit-module.h
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
 
/* This is a modified version of gedit-module.h from Epiphany source code.
 * Here the original copyright assignment:
 *
 *  Copyright (C) 2003 Marco Pesenti Gritti
 *  Copyright (C) 2003, 2004 Christian Persch
 *
 */

/*
 * Modified by the gedit Team, 2005. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 *
 * $Id$
 */
 
#ifndef GEDIT_MODULE_H
#define GEDIT_MODULE_H

#include <glib-object.h>

G_BEGIN_DECLS

#define GEDIT_TYPE_MODULE		(gedit_module_get_type ())
#define GEDIT_MODULE(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_MODULE, GeditModule))
#define GEDIT_MODULE_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GEDIT_TYPE_MODULE, GeditModuleClass))
#define GEDIT_IS_MODULE(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEDIT_TYPE_MODULE))
#define GEDIT_IS_MODULE_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((obj), GEDIT_TYPE_MODULE))
#define GEDIT_MODULE_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), GEDIT_TYPE_MODULE, GeditModuleClass))

typedef struct _GeditModule	GeditModule;

GType		 gedit_module_get_type		(void) G_GNUC_CONST;;

GeditModule	*gedit_module_new		(const gchar *path);

const gchar	*gedit_module_get_path		(GeditModule *module);

GObject		*gedit_module_new_object	(GeditModule *module);

G_END_DECLS

#endif
