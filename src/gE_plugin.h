/* gE_plugin.h - plugin header files.
 *
 * Copyright (C) 1998 The Free Software Foundation.
 * Contributed by Martin Baulig <martin@home-of-linux.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __GE_PLUGIN_H__
#define __GE_PLUGIN_H__

#include <unistd.h>
#include <glib.h>
#include <gmodule.h>

#define GEDIT_PLUGIN_INFO_KEY		"gedit_plugin_info"

typedef struct _gE_Plugin_Object	gE_Plugin_Object;
typedef struct _gE_Plugin_Info		gE_Plugin_Info;

typedef void (*gE_Plugin_InitFunc)	(gE_Plugin_Object *, gint);

struct _gE_Plugin_Info
{
	gchar *plugin_name;
	gE_Plugin_InitFunc init_func;
};

struct _gE_Plugin_Object
{
	gchar *name;
	gchar *plugin_name;
	gchar *library_name;
	gchar *config_path;
	GList *dependency_libs;
	gE_Plugin_Info *info;
	GModule *module;
	int context;
};

extern GList *gE_Plugin_List;

extern void gE_Plugin_Query_All (void);
extern gE_Plugin_Object *gE_Plugin_Query (gchar *);
extern void gE_Plugin_Register (gE_Plugin_Object *);
extern gboolean gE_Plugin_Load (gE_Plugin_Object *, gint);

#ifndef _IN_GEDIT

/* Only when included from plugin ... */

#include <libgnorba/gnorba.h>

extern void corba_exception (CORBA_Environment *);

extern CORBA_ORB global_orb;
extern PortableServer_POA root_poa;
extern PortableServer_POAManager root_poa_manager;
extern CORBA_Environment *global_ev;
extern CORBA_Object name_service;

#endif /* not _IN_GEDIT */


#endif /* __GE_PLUGIN_H__ */
