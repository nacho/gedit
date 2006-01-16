/*
 * gedit-python-module.c
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <signal.h>

#include <gmodule.h>

#include <pygobject.h>
#include <pygtk/pygtk.h>

#include "gedit-python-module.h"
#include "gedit-python-plugin.h"
#include "gedit-debug.h"

#define GEDIT_PYTHON_MODULE_GET_PRIVATE(object) (G_TYPE_INSTANCE_GET_PRIVATE ((object), GEDIT_TYPE_PYTHON_MODULE, GeditPythonModulePrivate))

struct _GeditPythonModulePrivate
{
	gchar *module;
	gchar *path;
	GType type;
};

enum
{
	PROP_0,
	PROP_PATH,
	PROP_MODULE
};

/* Exported by pygedit module */
void pygedit_register_classes (PyObject *d);
void pygedit_add_constants (PyObject *module, const gchar *strip_prefix);
extern PyMethodDef pygedit_functions[];

/* We retreive this to check for correct class hierarchy */
static PyTypeObject *PyGeditPlugin_Type;

G_DEFINE_TYPE (GeditPythonModule, gedit_python_module, G_TYPE_TYPE_MODULE);

static void
gedit_python_module_init_python ()
{
	PyObject *pygtk, *mdict, *require, *path;
	PyObject *sys_path, *gedit, *gtk, *pygtk_version, *pygtk_required_version;
	struct sigaction old_sigint;
	gint res;
	char *argv[] = { "gedit", NULL };
	char *gettext_command;
	
	if (Py_IsInitialized ())
	{
		g_warning ("Python Should only be initialized once, since it's in class_init");
		g_return_if_reached ();
	}

	/* Hack to make python not overwrite SIGINT: this is needed to avoid
	 * the crash reported on bug #326191 */
	
	/* Save old handler */
	res = sigaction (SIGINT, NULL, &old_sigint);  
	if (res != 0)
	{
		g_warning ("Error initializing Python interpreter: cannot get "
		           "handler to SIGINT signal (%s)",
		           strerror (errno));

		return;
	}
	
	/* Python initialization */
	Py_Initialize ();
	
	/* Restore old handler */
	res = sigaction (SIGINT, &old_sigint, NULL);
	if (res != 0)
	{
		g_warning ("Error initializing Python interpreter: cannot restore "
		           "handler to SIGINT signal (%s)",
		           strerror (errno));
    
		return;
	}
	
	PySys_SetArgv (1, argv);

	/* pygtk.require("2.0") */
	pygtk = PyImport_ImportModule ("pygtk");
	mdict = PyModule_GetDict (pygtk);
	require = PyDict_GetItemString (mdict, "require");
	PyObject_CallObject (require, Py_BuildValue ("(S)", PyString_FromString ("2.0")));

	/* import gobject */
	init_pygobject ();

	/* import gtk */
	init_pygtk ();

	/* gtk.pygtk_version < (2, 4, 0) */
	gtk = PyImport_ImportModule ("gtk");
	mdict = PyModule_GetDict (gtk);
	pygtk_version = PyDict_GetItemString (mdict, "pygtk_version");
	pygtk_required_version = Py_BuildValue ("(iii)", 2, 4, 0);
	if (PyObject_Compare (pygtk_version, pygtk_required_version) == -1)
	{
		g_warning("PyGTK %s required, but %s found.",
				  PyString_AsString (PyObject_Repr (pygtk_required_version)),
				  PyString_AsString (PyObject_Repr (pygtk_version)));
		Py_DECREF (pygtk_required_version);
		return;
	}
	Py_DECREF (pygtk_required_version);

	/* sys.path.insert(0, ...) for system-wide plugins */
	sys_path = PySys_GetObject ("path");
	path = PyString_FromString (GEDIT_PLUGINDIR "/");
	PyList_Insert (sys_path, 0, path);
	Py_DECREF(path);

	/* import gedit */
	gedit = Py_InitModule ("gedit", pygedit_functions);
	mdict = PyModule_GetDict (gedit);

	pygedit_register_classes (mdict);
	pygedit_add_constants (gedit, "GEDIT_");   

	/* Retreive the Python type for gedit.Plugin */
	PyGeditPlugin_Type = (PyTypeObject *) PyDict_GetItemString (mdict, "Plugin"); 
	if (PyGeditPlugin_Type == NULL)
	{
		PyErr_Print ();
		return;
	}
	
	gettext_command = g_strdup_printf ("import gettext\n"
			"gettext.install (\"%s\", \"%s\")",
			GETTEXT_PACKAGE,
			GEDIT_LOCALEDIR);

	PyRun_SimpleString (gettext_command);
	g_free (gettext_command);
}

static gboolean
gedit_python_module_load (GTypeModule *gmodule)
{
	GeditPythonModulePrivate *priv = GEDIT_PYTHON_MODULE_GET_PRIVATE (gmodule);
	PyObject *main_module, *main_locals, *locals, *key, *value;
	PyObject *module;
	int pos = 0;

	main_module = PyImport_AddModule ("__main__");
	if (main_module == NULL)
	{
		g_warning ("Could not get __main__.");
		return FALSE;
	}

	/* If we have a special path, we register it */
	if (priv->path != NULL)
	{
		PyObject *sys_path = PySys_GetObject ("path");
		PyObject *path = PyString_FromString (priv->path);

		if (PySequence_Contains(sys_path, path) == 0)
			PyList_Insert (sys_path, 0, path);

		Py_DECREF(path);
	}
	
	main_locals = PyModule_GetDict (main_module);
	module = PyImport_ImportModuleEx (priv->module, main_locals, main_locals, NULL);
	if (!module)
	{
		PyErr_Print ();
		return FALSE;
	}

	locals = PyModule_GetDict (module);
	while (PyDict_Next (locals, &pos, &key, &value))
	{
		if (!PyType_Check(value))
			continue;

		if (PyObject_IsSubclass (value, (PyObject*) PyGeditPlugin_Type))
		{
			priv->type = gedit_python_object_get_type (gmodule, value);
			return TRUE;
		}
	}

	return FALSE;
}

static void
gedit_python_module_unload (GTypeModule *module)
{
	GeditPythonModulePrivate *priv = GEDIT_PYTHON_MODULE_GET_PRIVATE (module);
	gedit_debug_message (DEBUG_PLUGINS, "Unloading python module");
	
	priv->type = 0;
}

GObject *
gedit_python_module_new_object (GeditPythonModule *module)
{
	GeditPythonModulePrivate *priv = GEDIT_PYTHON_MODULE_GET_PRIVATE (module);
	gedit_debug_message (DEBUG_PLUGINS, "Creating object of type %s", g_type_name (priv->type));

	if (priv->type == 0)
		return NULL;

	return g_object_new (priv->type, NULL);
}

static void
gedit_python_module_init (GeditPythonModule *module)
{
	gedit_debug_message (DEBUG_PLUGINS, "Init of python module");
}

static void
gedit_python_module_finalize (GObject *object)
{
	GeditPythonModulePrivate *priv = GEDIT_PYTHON_MODULE_GET_PRIVATE (object);
	gedit_debug_message (DEBUG_PLUGINS, "Finalizing python module %s", g_type_name (priv->type));

	g_free (priv->module);
	g_free (priv->path);

	G_OBJECT_CLASS (gedit_python_module_parent_class)->finalize (object);
}

static void
gedit_python_module_get_property (GObject    *object,
				  guint       prop_id,
				  GValue     *value,
				  GParamSpec *pspec)
{
	/* no readable properties */
	g_return_if_reached ();
}

static void
gedit_python_module_set_property (GObject      *object,
				  guint         prop_id,
				  const GValue *value,
				  GParamSpec   *pspec)
{
	GeditPythonModule *mod = GEDIT_PYTHON_MODULE (object);

	switch (prop_id)
	{
		case PROP_MODULE:
			GEDIT_PYTHON_MODULE_GET_PRIVATE (mod)->module = g_value_dup_string (value);
			break;
		case PROP_PATH:
			GEDIT_PYTHON_MODULE_GET_PRIVATE (mod)->path = g_value_dup_string (value);
			break;
		default:
			g_return_if_reached ();
	}
}

static void
gedit_python_module_class_init (GeditPythonModuleClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);
	GTypeModuleClass *module_class = G_TYPE_MODULE_CLASS (class);

	object_class->finalize = gedit_python_module_finalize;
	object_class->get_property = gedit_python_module_get_property;
	object_class->set_property = gedit_python_module_set_property;

	g_object_class_install_property
			(object_class,
			 PROP_MODULE,
			 g_param_spec_string ("module",
					      "Module Name",
					      "The python module to load for this plugin",
					      NULL,
					      G_PARAM_WRITABLE | G_PARAM_READABLE | G_PARAM_CONSTRUCT_ONLY));
					      
	g_object_class_install_property
			(object_class,
			 PROP_PATH,
			 g_param_spec_string ("path",
					      "Path",
					      "The python path to use when loading this module",
					      NULL,
					      G_PARAM_WRITABLE | G_PARAM_READABLE | G_PARAM_CONSTRUCT_ONLY));

	g_type_class_add_private (object_class, sizeof (GeditPythonModulePrivate));
	
	module_class->load = gedit_python_module_load;
	module_class->unload = gedit_python_module_unload;

	/* Init python subsystem, this should happen only once
	 * in the process lifetime, and doing it here is ok since
	 * class_init is called once */
	gedit_python_module_init_python ();
}

GeditPythonModule *
gedit_python_module_new (const gchar *path,
			 const gchar *module)
{
	GeditPythonModule *result;

	if (module == NULL || module[0] == '\0')
		return NULL;

	result = g_object_new (GEDIT_TYPE_PYTHON_MODULE,
			       "module", module,
			       "path", path,
			       NULL);

	g_type_module_set_name (G_TYPE_MODULE (result), module);

	return result;
}


/* --- these are not module methods, they are here out of convenience --- */

static gint idle_garbage_collect_id = 0;

static gboolean
run_gc (gpointer data)
{
	while (PyGC_Collect ())
		;	

	idle_garbage_collect_id = 0;
	return FALSE;
}

void
gedit_python_garbage_collect (void)
{
	if (Py_IsInitialized() && idle_garbage_collect_id == 0)
	{
		idle_garbage_collect_id = g_idle_add (run_gc, NULL);
	}
}

void
gedit_python_shutdown (void)
{
	if (Py_IsInitialized ())
	{
		if (idle_garbage_collect_id != 0)
		{
			g_source_remove (idle_garbage_collect_id);
			idle_garbage_collect_id = 0;
		}

		while (PyGC_Collect ())
			;	

		Py_Finalize ();
	}
}

