/*
 * gedit-plugins-engine.h
 * This file is part of gedit
 *
 * Copyright (C) 2002-2005 - Paolo Maggi 
 * Copyright (C) 2010 - Steve Frécinaux
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
 
/*
 * Modified by the gedit Team, 2002-2005. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 *
 * $Id$
 */

#ifndef __GEDIT_PLUGINS_ENGINE_H__
#define __GEDIT_PLUGINS_ENGINE_H__

#include <glib.h>
#include <libpeas/peas-engine.h>

G_BEGIN_DECLS

#define GEDIT_TYPE_PLUGINS_ENGINE              (gedit_plugins_engine_get_type ())
#define GEDIT_PLUGINS_ENGINE(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), GEDIT_TYPE_PLUGINS_ENGINE, GeditPluginsEngine))
#define GEDIT_PLUGINS_ENGINE_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass), GEDIT_TYPE_PLUGINS_ENGINE, GeditPluginsEngineClass))
#define GEDIT_IS_PLUGINS_ENGINE(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), GEDIT_TYPE_PLUGINS_ENGINE))
#define GEDIT_IS_PLUGINS_ENGINE_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GEDIT_TYPE_PLUGINS_ENGINE))
#define GEDIT_PLUGINS_ENGINE_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), GEDIT_TYPE_PLUGINS_ENGINE, GeditPluginsEngineClass))

typedef struct _GeditPluginsEngine		GeditPluginsEngine;
typedef struct _GeditPluginsEnginePrivate	GeditPluginsEnginePrivate;

struct _GeditPluginsEngine
{
	PeasEngine parent;
	GeditPluginsEnginePrivate *priv;
};

typedef struct _GeditPluginsEngineClass		GeditPluginsEngineClass;

struct _GeditPluginsEngineClass
{
	PeasEngineClass parent_class;
};

GType			 gedit_plugins_engine_get_type		(void) G_GNUC_CONST;

GeditPluginsEngine	*gedit_plugins_engine_get_default	(void);

G_END_DECLS

#endif  /* __GEDIT_PLUGINS_ENGINE_H__ */

/* ex:ts=8:noet: */
