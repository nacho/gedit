/*
 * gedit-python-plugin.c
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

#include <config.h>

#include "gedit-python-plugin.h"
#include "gedit-plugin.h"
#include "gedit-debug.h"

#include <pygobject.h>
#include <string.h>

static GObjectClass *parent_class;

static void
call_python_method (GeditPythonObject *object,
		    GeditWindow       *window,
		    gchar             *method)
{
	PyGILState_STATE state = pyg_gil_state_ensure ();                                    
	PyObject *py_ret = NULL;

	if (!PyObject_HasAttrString (object->instance, method))
	{
		pyg_gil_state_release (state);
		return;
	}

	py_ret = PyObject_CallMethod (object->instance,
				      method,
				      "(N)",
				      pygobject_new (G_OBJECT (window)));
	if (!py_ret)
		PyErr_Print ();

	Py_XDECREF (py_ret);
	pyg_gil_state_release (state);
}

static void
impl_update_ui (GeditPlugin *plugin,
		GeditWindow *window)
{
	call_python_method ((GeditPythonObject *) plugin, window, "update_ui");
}

static void
impl_deactivate (GeditPlugin *plugin,
		 GeditWindow *window)
{
	call_python_method ((GeditPythonObject *) plugin, window, "deactivate");
}

static void
impl_activate (GeditPlugin *plugin,
	       GeditWindow *window)
{
	call_python_method ((GeditPythonObject *) plugin, window, "activate");
}

static void 
gedit_python_object_init (GeditPythonObject *object)
{
	GeditPythonObjectClass *class;

	gedit_debug_message (DEBUG_PLUGINS, "Creating python plugin instance");

	class = (GeditPythonObjectClass*) (((GTypeInstance*) object)->g_class);

	object->instance = PyObject_CallObject (class->type, NULL);
	if (object->instance == NULL)
		PyErr_Print();
}

static void
gedit_python_object_finalize (GObject *object)
{
	gedit_debug_message (DEBUG_PLUGINS, "Finalizing python plugin instance");

	Py_DECREF (((GeditPythonObject *) object)->instance);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gedit_python_object_class_init (GeditPythonObjectClass *klass,
				gpointer                class_data)
{
	GeditPluginClass *plugin_class = GEDIT_PLUGIN_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	klass->type = (PyObject*) class_data;

	G_OBJECT_CLASS (klass)->finalize = gedit_python_object_finalize;

	plugin_class->activate = impl_activate;
	plugin_class->deactivate = impl_deactivate;
	plugin_class->update_ui = impl_update_ui;
}

GType
gedit_python_object_get_type (GTypeModule *module, 
			      PyObject    *type)
{
	GTypeInfo *info;
	GType gtype;
	gchar *type_name;

	info = g_new0 (GTypeInfo, 1);

	info->class_size = sizeof (GeditPythonObjectClass);
	info->class_init = (GClassInitFunc) gedit_python_object_class_init;
	info->instance_size = sizeof (GeditPythonObject);
	info->instance_init = (GInstanceInitFunc) gedit_python_object_init;
	info->class_data = type;
	Py_INCREF (type);

	type_name = g_strdup_printf ("%s+GeditPythonPlugin",
				     PyString_AsString (PyObject_GetAttrString (type, "__name__")));

	gedit_debug_message (DEBUG_PLUGINS, "Registering python plugin instance: %s", type_name);
	gtype = g_type_module_register_type (module, 
					     GEDIT_TYPE_PLUGIN,
					     type_name,
					     info, 0);
	g_free (type_name);

	return gtype;
}
