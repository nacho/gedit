/*
 * gedit-module.c
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
 
/* This is a modified version of ephy-module.c from Epiphany source code.
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

#include "config.h"

#include "gedit-module.h"
#include "gedit-debug.h"

typedef GType (*GeditModuleRegisterFunc) (GTypeModule *);

enum {
	PROP_0,
	PROP_MODULE_NAME,
	PROP_PATH
};

G_DEFINE_TYPE (GeditModule, gedit_module, G_TYPE_TYPE_MODULE);

static gboolean
gedit_module_load (GTypeModule *gmodule)
{
	GeditModule *module = GEDIT_MODULE (gmodule);
	GeditModuleRegisterFunc register_func;
	gchar *path;

	gedit_debug_message (DEBUG_PLUGINS, "Loading %s module from %s",
			     module->module_name, module->path);

	path = g_module_build_path (module->path, module->module_name);
	g_return_val_if_fail (path != NULL, FALSE);
	gedit_debug_message (DEBUG_PLUGINS, "Module filename: %s", path);

	module->library = g_module_open (path, 0);
	g_free (path);

	if (module->library == NULL)
	{
		g_warning ("%s: %s", module->module_name, g_module_error());

		return FALSE;
	}

	/* extract symbols from the lib */
	if (!g_module_symbol (module->library, "register_gedit_plugin",
			      (void *) &register_func))
	{
		g_warning ("%s: %s", module->module_name, g_module_error());
		g_module_close (module->library);

		return FALSE;
	}

	/* symbol can still be NULL even though g_module_symbol
	 * returned TRUE */
	if (register_func == NULL)
	{
		g_warning ("Symbol 'register_gedit_plugin' should not be NULL");
		g_module_close (module->library);

		return FALSE;
	}

	module->type = register_func (gmodule);

	if (module->type == 0)
	{
		g_warning ("Invalid gedit plugin contained by module %s", module->module_name);
		return FALSE;
	}

	return TRUE;
}

static void
gedit_module_unload (GTypeModule *gmodule)
{
	GeditModule *module = GEDIT_MODULE (gmodule);

	gedit_debug_message (DEBUG_PLUGINS, "Unloading %s", module->path);

	g_module_close (module->library);

	module->library = NULL;
	module->type = 0;
}

static void
gedit_module_class_real_garbage_collect (void)
{
	/* Do nothing */
}

static void
gedit_module_init (GeditModule *module)
{
	gedit_debug_message (DEBUG_PLUGINS, "GeditModule %p initialising", module);
}

static void
gedit_module_finalize (GObject *object)
{
	GeditModule *module = GEDIT_MODULE (object);

	gedit_debug_message (DEBUG_PLUGINS, "GeditModule %p finalising", module);

	g_free (module->path);
	g_free (module->module_name);

	G_OBJECT_CLASS (gedit_module_parent_class)->finalize (object);
}

static void
gedit_module_get_property (GObject    *object,
			   guint       prop_id,
			   GValue     *value,
			   GParamSpec *pspec)
{
	GeditModule *module = GEDIT_MODULE (object);

	switch (prop_id)
	{
		case PROP_MODULE_NAME:
			g_value_set_string (value, module->module_name);
			break;
		case PROP_PATH:
			g_value_set_string (value, module->path);
			break;
		default:
			g_return_if_reached ();
	}
}

static void
gedit_module_set_property (GObject      *object,
			   guint         prop_id,
			   const GValue *value,
			   GParamSpec   *pspec)
{
	GeditModule *module = GEDIT_MODULE (object);

	switch (prop_id)
	{
		case PROP_MODULE_NAME:
			module->module_name = g_value_dup_string (value);
			g_type_module_set_name (G_TYPE_MODULE (object),
						module->module_name);
			break;
		case PROP_PATH:
			module->path = g_value_dup_string (value);
			break;
		default:
			g_return_if_reached ();
	}
}

static void
gedit_module_class_init (GeditModuleClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GTypeModuleClass *module_class = G_TYPE_MODULE_CLASS (klass);

	object_class->set_property = gedit_module_set_property;
	object_class->get_property = gedit_module_get_property;
	object_class->finalize = gedit_module_finalize;

	module_class->load = gedit_module_load;
	module_class->unload = gedit_module_unload;

	klass->garbage_collect = gedit_module_class_real_garbage_collect;

	g_object_class_install_property (object_class,
					 PROP_MODULE_NAME,
					 g_param_spec_string ("module-name",
							      "Module Name",
							      "The module to load for this plugin",
							      NULL,
							      G_PARAM_READWRITE |
							      G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property (object_class,
					 PROP_PATH,
					 g_param_spec_string ("path",
							      "Path",
							      "The path to use when loading this module",
							      NULL,
							      G_PARAM_READWRITE |
							      G_PARAM_CONSTRUCT_ONLY));
}

void
gedit_module_class_garbage_collect (GeditModuleClass *klass)
{
	g_return_if_fail (GEDIT_IS_MODULE_CLASS (klass));

	GEDIT_MODULE_CLASS (klass)->garbage_collect ();
}

GObject *
gedit_module_new_object (GeditModule *module)
{
	g_return_val_if_fail (module->type != 0, NULL);

	gedit_debug_message (DEBUG_PLUGINS, "Creating object of type %s",
			     g_type_name (module->type));

	return g_object_new (module->type, NULL);
}

const gchar *
gedit_module_get_path (GeditModule *module)
{
	g_return_val_if_fail (GEDIT_IS_MODULE (module), NULL);

	return module->path;
}

const gchar *
gedit_module_get_module_name (GeditModule *module)
{
	g_return_val_if_fail (GEDIT_IS_MODULE (module), NULL);

	return module->module_name;
}
