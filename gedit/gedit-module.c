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

#include <gmodule.h>

typedef struct _GeditModuleClass GeditModuleClass;

struct _GeditModuleClass
{
	GTypeModuleClass parent_class;
};

struct _GeditModule
{
	GTypeModule parent_instance;

	GModule *library;

	gchar *path;
	GType type;
};

typedef GType (*GeditModuleRegisterFunc) (GTypeModule *);

static void gedit_module_init		(GeditModule *action);
static void gedit_module_class_init	(GeditModuleClass *class);

static GObjectClass *parent_class = NULL;

GType
gedit_module_get_type (void)
{
	static GType type = 0;

	if (G_UNLIKELY (type == 0))
	{
		static const GTypeInfo type_info =
		{
			sizeof (GeditModuleClass),
			(GBaseInitFunc) NULL,
			(GBaseFinalizeFunc) NULL,
			(GClassInitFunc) gedit_module_class_init,
			(GClassFinalizeFunc) NULL,
			NULL,
			sizeof (GeditModule),
			0, /* n_preallocs */
			(GInstanceInitFunc) gedit_module_init,
		};

		type = g_type_register_static (G_TYPE_TYPE_MODULE,
					       "GeditModule",
					       &type_info, 0);
	}

	return type;
}

static gboolean
gedit_module_load (GTypeModule *gmodule)
{
	GeditModule *module = GEDIT_MODULE (gmodule);
	GeditModuleRegisterFunc register_func;

	gedit_debug_message (DEBUG_PLUGINS, "Loading %s", module->path);

	module->library = g_module_open (module->path, 0);

	if (module->library == NULL)
	{
		g_warning (g_module_error());

		return FALSE;
	}

	/* extract symbols from the lib */
	if (!g_module_symbol (module->library, "register_gedit_plugin",
			      (void *) &register_func))
	{
		g_warning (g_module_error());
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
		g_warning ("Invalid gedit plugin contained by module %s", module->path);
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

const gchar *
gedit_module_get_path (GeditModule *module)
{
	g_return_val_if_fail (GEDIT_IS_MODULE (module), NULL);

	return module->path;
}

GObject *
gedit_module_new_object (GeditModule *module)
{
	gedit_debug_message (DEBUG_PLUGINS, "Creating object of type %s", g_type_name (module->type));

	if (module->type == 0)
	{
		return NULL;
	}

	return g_object_new (module->type, NULL);
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

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
gedit_module_class_init (GeditModuleClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);
	GTypeModuleClass *module_class = G_TYPE_MODULE_CLASS (class);

	parent_class = (GObjectClass *) g_type_class_peek_parent (class);

	object_class->finalize = gedit_module_finalize;

	module_class->load = gedit_module_load;
	module_class->unload = gedit_module_unload;
}

GeditModule *
gedit_module_new (const gchar *path)
{
	GeditModule *result;

	if (path == NULL || path[0] == '\0')
	{
		return NULL;
	}

	result = g_object_new (GEDIT_TYPE_MODULE, NULL);

	g_type_module_set_name (G_TYPE_MODULE (result), path);
	result->path = g_strdup (path);

	return result;
}
