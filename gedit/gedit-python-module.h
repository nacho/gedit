/*
 * gedit-python-module.h
 * This file is part of gedit
 *
 * Copyright (C) 2005 Raphael Slinckx
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

#ifndef __GEDIT_PYTHON_MODULE_H__
#define __GEDIT_PYTHON_MODULE_H__

#include <glib-object.h>
#include "gedit-module.h"

G_BEGIN_DECLS

#define GEDIT_TYPE_PYTHON_MODULE		(gedit_python_module_get_type ())
#define GEDIT_PYTHON_MODULE(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_PYTHON_MODULE, GeditPythonModule))
#define GEDIT_PYTHON_MODULE_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GEDIT_TYPE_PYTHON_MODULE, GeditPythonModuleClass))
#define GEDIT_IS_PYTHON_MODULE(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEDIT_TYPE_PYTHON_MODULE))
#define GEDIT_IS_PYTHON_MODULE_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEDIT_TYPE_PYTHON_MODULE))
#define GEDIT_PYTHON_MODULE_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), GEDIT_TYPE_PYTHON_MODULE, GeditPythonModuleClass))

typedef GeditModule		GeditPythonModule;
typedef GeditModuleClass	GeditPythonModuleClass;

GType			 gedit_python_module_get_type		(void);

/* --- Python utils --- */

/* Must be called before loading python modules */
gboolean		gedit_python_init			(void);

void			gedit_python_shutdown			(void);

G_END_DECLS

#endif
