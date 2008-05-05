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

#include <Python.h>
#include <pygobject.h>
#include <pygtk/pygtk.h>

#include <signal.h>

#include <gmodule.h>

#include "gedit-python-module.h"
#include "gedit-python-plugin.h"
#include "gedit-debug.h"

#if PY_VERSION_HEX < 0x02050000
typedef int Py_ssize_t;
#define PY_SSIZE_T_MAX INT_MAX
#define PY_SSIZE_T_MIN INT_MIN
#endif

/* Exported by pygedit module */
void pygedit_register_classes (PyObject *d);
void pygedit_add_constants (PyObject *module, const gchar *strip_prefix);
extern PyMethodDef pygedit_functions[];

/* Exported by pygeditutils module */
void pygeditutils_register_classes (PyObject *d);
extern PyMethodDef pygeditutils_functions[];

/* Exported by pygeditcommands module */
void pygeditcommands_register_classes (PyObject *d);
extern PyMethodDef pygeditcommands_functions[];

/* We retreive this to check for correct class hierarchy */
static PyTypeObject *PyGeditPlugin_Type;

G_DEFINE_TYPE (GeditPythonModule, gedit_python_module, GEDIT_TYPE_MODULE)

static gboolean
gedit_python_module_load (GTypeModule *gmodule)
{
	GeditModule *module = GEDIT_MODULE (gmodule);
	PyObject *main_module, *main_locals, *locals, *key, *value;
	PyObject *pymodule, *fromlist;
	Py_ssize_t pos = 0;
	
	g_return_val_if_fail (Py_IsInitialized (), FALSE);

	main_module = PyImport_AddModule ("__main__");
	if (main_module == NULL)
	{
		g_warning ("Could not get __main__.");
		return FALSE;
	}

	/* If we have a special path, we register it */
	if (module->path != NULL)
	{
		PyObject *sys_path = PySys_GetObject ("path");
		PyObject *path = PyString_FromString (module->path);

		if (PySequence_Contains(sys_path, path) == 0)
			PyList_Insert (sys_path, 0, path);

		Py_DECREF(path);
	}
	
	main_locals = PyModule_GetDict (main_module);

	/* we need a fromlist to be able to import modules with a '.' in the
	   name. */
	fromlist = PyTuple_New(0);
	pymodule = PyImport_ImportModuleEx (module->module_name, main_locals, main_locals, fromlist);
	Py_DECREF(fromlist);
	if (!pymodule)
	{
		PyErr_Print ();
		return FALSE;
	}

	locals = PyModule_GetDict (pymodule);
	while (PyDict_Next (locals, &pos, &key, &value))
	{
		if (!PyType_Check(value))
			continue;

		if (PyObject_IsSubclass (value, (PyObject*) PyGeditPlugin_Type))
		{
			module->type = gedit_python_object_get_type (gmodule, value);
			return TRUE;
		}
	}

	return FALSE;
}

static void
gedit_python_module_unload (GTypeModule *module)
{
	gedit_debug_message (DEBUG_PLUGINS, "Unloading Python module");
	GEDIT_MODULE (module)->type = 0;
}

static gint idle_garbage_collect_id = 0;

static gboolean
run_gc (gpointer data)
{
	while (PyGC_Collect ())
		;

	idle_garbage_collect_id = 0;
	return FALSE;
}

static void
gedit_python_module_class_garbage_collect (void)
{
	if (Py_IsInitialized())
	{
		/*
		 * We both run the GC right now and we schedule
		 * a further collection in the main loop.
		 */

		while (PyGC_Collect ())
			;

		if (idle_garbage_collect_id == 0)
			idle_garbage_collect_id = g_idle_add (run_gc, NULL);
	}
}

static void
gedit_python_module_init (GeditPythonModule *module)
{
	gedit_debug_message (DEBUG_PLUGINS, "Init of Python module");
}

static void
gedit_python_module_finalize (GObject *object)
{
	gedit_debug_message (DEBUG_PLUGINS, "Finalizing Python module %s",
			     g_type_name (GEDIT_MODULE (object)->type));

	G_OBJECT_CLASS (gedit_python_module_parent_class)->finalize (object);
}

static void
gedit_python_module_class_init (GeditPythonModuleClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GTypeModuleClass *gmodule_class = G_TYPE_MODULE_CLASS (klass);
	GeditModuleClass *module_class = GEDIT_MODULE_CLASS (klass);

	object_class->finalize = gedit_python_module_finalize;

	gmodule_class->load = gedit_python_module_load;
	gmodule_class->unload = gedit_python_module_unload;

	module_class->garbage_collect = gedit_python_module_class_garbage_collect;
}

/* --- these are not module methods, they are here out of convenience --- */

/* C equivalent of
 *    import pygtk
 *    pygtk.require ("2.0")
 */
static gboolean
check_pygtk2 (void)
{
	PyObject *pygtk, *mdict, *require;

	/* pygtk.require("2.0") */
	pygtk = PyImport_ImportModule ("pygtk");
	if (pygtk == NULL)
	{
		g_warning ("Error initializing Python interpreter: could not import pygtk.");
		return FALSE;
	}

	mdict = PyModule_GetDict (pygtk);
	require = PyDict_GetItemString (mdict, "require");
	PyObject_CallObject (require,
			     Py_BuildValue ("(S)", PyString_FromString ("2.0")));
	if (PyErr_Occurred())
	{
		g_warning ("Error initializing Python interpreter: pygtk 2 is required.");
		return FALSE;
	}

	return TRUE;
}

/* Note: the following two functions are needed because
 * init_pyobject and init_pygtk which are *macros* which in case
 * case of error set the PyErr and then make the calling
 * function return behind our back.
 * It's up to the caller to check the result with PyErr_Occurred()
 */
static void
gedit_init_pygobject (void)
{
	init_pygobject_check (2, 11, 5); /* FIXME: get from config */
}

static void
gedit_init_pygtk (void)
{
	PyObject *gtk, *mdict, *version, *required_version;

	init_pygtk ();

	/* there isn't init_pygtk_check(), do the version
	 * check ourselves */
	gtk = PyImport_ImportModule("gtk");
	mdict = PyModule_GetDict(gtk);
	version = PyDict_GetItemString (mdict, "pygtk_version");
	if (!version)
	{
		PyErr_SetString (PyExc_ImportError,
				 "PyGObject version too old");
		return;
	}

	required_version = Py_BuildValue ("(iii)", 2, 4, 0); /* FIXME */

	if (PyObject_Compare (version, required_version) == -1)
	{
		PyErr_SetString (PyExc_ImportError,
				 "PyGObject version too old");
		Py_DECREF (required_version);
		return;
	}

	Py_DECREF (required_version);
}

static void
old_gtksourceview_init (void)
{
	PyErr_SetString(PyExc_ImportError,
			"gtksourceview module not allowed, use gtksourceview2");
}

static void
gedit_init_pygtksourceview (void)
{
	PyObject *gtksourceview, *mdict, *version, *required_version;

	gtksourceview = PyImport_ImportModule("gtksourceview2");
	if (gtksourceview == NULL)
	{
		PyErr_SetString (PyExc_ImportError,
				 "could not import gtksourceview");
		return;
	}

	mdict = PyModule_GetDict (gtksourceview);
	version = PyDict_GetItemString (mdict, "pygtksourceview2_version");
	if (!version)
	{
		PyErr_SetString (PyExc_ImportError,
				 "PyGtkSourceView version too old");
		return;
	}

	required_version = Py_BuildValue ("(iii)", 0, 8, 0); /* FIXME */

	if (PyObject_Compare (version, required_version) == -1)
	{
		PyErr_SetString (PyExc_ImportError,
				 "PyGtkSourceView version too old");
		Py_DECREF (required_version);
		return;
	}

	Py_DECREF (required_version);

	/* Create a dummy 'gtksourceview' module to prevent
	 * loading of the old 'gtksourceview' modules that
	 * has conflicting symbols with the gtksourceview2 module.
	 * Raise an exception when trying to import it.
	 */
	PyImport_AppendInittab ("gtksourceview", old_gtksourceview_init);
}

gboolean
gedit_python_init (void)
{
	PyObject *mdict, *tuple;
	PyObject *gedit, *geditutils, *geditcommands;
	PyObject *gettext, *install, *gettext_args;
	struct sigaction old_sigint;
	gint res;
	char *argv[] = { "gedit", NULL };
	
	static gboolean init_failed = FALSE;

	if (init_failed)
	{
		/* We already failed to initialized Python, don't need to
		 * retry again */
		return FALSE;
	}
	
	if (Py_IsInitialized ())
	{
		/* Python has already been successfully initialized */
		return TRUE;
	}

	/* We are trying to initialize Python for the first time,
	   set init_failed to FALSE only if the entire initialization process
	   ends with success */
	init_failed = TRUE;

	/* Hack to make python not overwrite SIGINT: this is needed to avoid
	 * the crash reported on bug #326191 */

	/* CHECK: can't we use Py_InitializeEx instead of Py_Initialize in order
          to avoid to manage signal handlers ? - Paolo (Dec. 31, 2006) */

	/* Save old handler */
	res = sigaction (SIGINT, NULL, &old_sigint);  
	if (res != 0)
	{
		g_warning ("Error initializing Python interpreter: cannot get "
		           "handler to SIGINT signal (%s)",
		           strerror (errno));

		return FALSE;
	}

	/* Python initialization */
	Py_Initialize ();

	/* Restore old handler */
	res = sigaction (SIGINT, &old_sigint, NULL);
	if (res != 0)
	{
		g_warning ("Error initializing Python interpreter: cannot restore "
		           "handler to SIGINT signal (%s).",
		           strerror (errno));

		goto python_init_error;
	}

	PySys_SetArgv (1, argv);

	if (!check_pygtk2 ())
	{
		/* Warning message already printed in check_pygtk2 */
		goto python_init_error;
	}

	/* import gobject */	
	gedit_init_pygobject ();
	if (PyErr_Occurred ())
	{
		g_warning ("Error initializing Python interpreter: could not import pygobject.");

		goto python_init_error;		
	}

	/* import gtk */
	gedit_init_pygtk ();
	if (PyErr_Occurred ())
	{
		g_warning ("Error initializing Python interpreter: could not import pygtk.");

		goto python_init_error;
	}
	
	/* import gtksourceview */
	gedit_init_pygtksourceview ();
	if (PyErr_Occurred ())
	{
		PyErr_Print ();

		g_warning ("Error initializing Python interpreter: could not import pygtksourceview.");

		goto python_init_error;
	}	
	
	/* import gedit */
	gedit = Py_InitModule ("gedit", pygedit_functions);
	mdict = PyModule_GetDict (gedit);

	pygedit_register_classes (mdict);
	pygedit_add_constants (gedit, "GEDIT_");

	/* gedit version */
	tuple = Py_BuildValue("(iii)", 
			      GEDIT_MAJOR_VERSION,
			      GEDIT_MINOR_VERSION,
			      GEDIT_MICRO_VERSION);
	PyDict_SetItemString(mdict, "version", tuple);
	Py_DECREF(tuple);
	
	/* Retrieve the Python type for gedit.Plugin */
	PyGeditPlugin_Type = (PyTypeObject *) PyDict_GetItemString (mdict, "Plugin"); 
	if (PyGeditPlugin_Type == NULL)
	{
		PyErr_Print ();

		goto python_init_error;
	}

	/* import gedit.utils */
	geditutils = Py_InitModule ("gedit.utils", pygeditutils_functions);
	PyDict_SetItemString (mdict, "utils", geditutils);

	/* import gedit.commands */
	geditcommands = Py_InitModule ("gedit.commands", pygeditcommands_functions);
	PyDict_SetItemString (mdict, "commands", geditcommands);

	mdict = PyModule_GetDict (geditutils);
	pygeditutils_register_classes (mdict);
	
	mdict = PyModule_GetDict (geditcommands);
	pygeditcommands_register_classes (mdict);

	/* i18n support */
	gettext = PyImport_ImportModule ("gettext");
	if (gettext == NULL)
	{
		g_warning ("Error initializing Python interpreter: could not import gettext.");

		goto python_init_error;
	}

	mdict = PyModule_GetDict (gettext);
	install = PyDict_GetItemString (mdict, "install");
	gettext_args = Py_BuildValue ("ss", GETTEXT_PACKAGE, GEDIT_LOCALEDIR);
	PyObject_CallObject (install, gettext_args);
	Py_DECREF (gettext_args);
	
	/* Python has been successfully initialized */
	init_failed = FALSE;
	
	return TRUE;
	
python_init_error:

	g_warning ("Please check the installation of all the Python related packages required "
	           "by gedit and try again.");

	PyErr_Clear ();

	gedit_python_shutdown ();

	return FALSE;
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
