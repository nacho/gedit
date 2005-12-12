/*
 * gedit-python-plugin.h
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
#ifndef GEDIT_PYTHON_OBJECT_H
#define GEDIT_PYTHON_OBJECT_H

#include <Python.h>
#include <glib-object.h>
#include "gedit-plugin.h"

G_BEGIN_DECLS

typedef struct _GeditPythonObject		GeditPythonObject;
typedef struct _GeditPythonObjectClass	GeditPythonObjectClass;

struct _GeditPythonObject
{
	GeditPlugin parent_slot;
	PyObject *instance;
};

struct _GeditPythonObjectClass
{
	GeditPluginClass parent_slot;
	PyObject *type;
};

GType gedit_python_object_get_type (GTypeModule *module, PyObject *type);

G_END_DECLS

#endif
