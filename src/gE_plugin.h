/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 3 -*-
 *
 * gE_plugin.h - plugin header files.
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

/* If we are _IN_GEDIT, we include config.h to get WITH_GMODULE_PLUGINS
 * only if we really have them.
 *
 * Declare WITH_GMODULE_PLUGINS if we are included from such a plugin.
 */

#ifdef _IN_GEDIT
#include <config.h>
#else
#ifndef WITH_GMODULE_PLUGINS
#define WITH_GMODULE_PLUGINS 1
#endif
#endif

#ifdef WITH_GMODULE_PLUGINS

/* We have gmodule plugins or we are included from such a plugin ... */

#include <glib.h>
#include <gmodule.h>

#define GEDIT_PLUGIN_INFO_KEY		"gedit_plugin_info"

typedef struct _gE_Plugin_Object gE_Plugin_Object;
typedef struct _gE_Plugin_Info gE_Plugin_Info;

typedef void (*gE_Plugin_InitFunc) (gE_Plugin_Object *, gint);
typedef gboolean(*gE_Plugin_StartFunc) (gE_Plugin_Object *, gint);

struct _gE_Plugin_Info {
   gchar *plugin_name;
   gE_Plugin_InitFunc init_func;
   gE_Plugin_StartFunc start_func;
};

struct _gE_Plugin_Object {
   gchar *name;
   gchar *plugin_name;
   gchar *library_name;
   gchar *config_path;
   GList *dependency_libs;
   gE_Plugin_Info *info;
   GModule *module;
   int context;
};

#ifdef _IN_GEDIT

/* Only used internally in gEdit. Do not use this in plugins ! */

extern GList *gE_Plugin_List;

extern void gE_Plugin_Query_All(void);
extern gE_Plugin_Object *gE_Plugin_Query(gchar *);
extern void gE_Plugin_Register(gE_Plugin_Object *);
extern gboolean gE_Plugin_Load(gE_Plugin_Object *, gint);

#endif /* _IN_GEDIT */

#endif /* WITH_GMODULE_PLUGINS */

/* The following functions can be used in plugins to access the
 * functionality of gEdit.
 *
 * Please do not use other functions than the ones declared here.
 */

extern int gE_plugin_document_create(gint, gchar *);
extern void gE_plugin_text_append(gint, gchar *, gint);
extern void gE_plugin_text_insert(gint, gchar *, gint, gint);
extern void gE_plugin_document_show(gint);
extern int gE_plugin_document_current(gint);
extern char *gE_plugin_document_filename(gint);
extern int gE_plugin_document_open(gint, gchar *);
extern gboolean gE_plugin_document_close(gint);
extern void gE_plugin_set_auto_indent(gint, gint);
extern void gE_plugin_set_status_bar(gint, gint);
extern void gE_plugin_set_word_wrap(gint, gint);
extern void gE_plugin_set_line_wrap(gint, gint);
extern void gE_plugin_set_read_only(gint, gint);
extern void gE_plugin_set_split_screen(gint, gint);

#ifndef WITHOUT_GNOME
extern void gE_plugin_set_scroll_ball(gint, gint);

#endif
extern char *gE_plugin_text_get(gint);
extern gboolean gE_plugin_program_quit(void);
extern GtkText *gE_plugin_get_widget(gint);
extern int gE_plugin_create_widget(gint, gchar *,
				   GtkWidget **, GtkWidget **);

#ifndef _IN_GEDIT

/* Only when included from plugin ... */

#include <libgnorba/gnorba.h>

extern void corba_exception(CORBA_Environment *);

extern CORBA_ORB global_orb;
extern PortableServer_POA root_poa;
extern PortableServer_POAManager root_poa_manager;
extern CORBA_Environment *global_ev;
extern CORBA_Object name_service;

#endif /* not _IN_GEDIT */

#endif /* __GE_PLUGIN_H__ */
