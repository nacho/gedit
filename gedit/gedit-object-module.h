/*
 * gedit-object-module.h
 * This file is part of gedit
 *
 * Copyright (C) 2005 - Paolo Maggi
 * Copyright (C) 2008 - Jesse van den Kieboom
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
 
/* This is a modified version of gedit-module.h from Epiphany source code.
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
 * $Id: gedit-module.h 6263 2008-05-05 10:52:10Z sfre $
 */
 
#ifndef __GEDIT_OBJECT_MODULE_H__
#define __GEDIT_OBJECT_MODULE_H__

#include <glib-object.h>
#include <gmodule.h>
#include <stdarg.h>

G_BEGIN_DECLS

#define GEDIT_TYPE_OBJECT_MODULE		(gedit_object_module_get_type ())
#define GEDIT_OBJECT_MODULE(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_OBJECT_MODULE, GeditObjectModule))
#define GEDIT_OBJECT_MODULE_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GEDIT_TYPE_OBJECT_MODULE, GeditObjectModuleClass))
#define GEDIT_IS_OBJECT_MODULE(obj)			(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEDIT_TYPE_OBJECT_MODULE))
#define GEDIT_IS_OBJECT_MODULE_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE ((klass), GEDIT_TYPE_OBJECT_MODULE))
#define GEDIT_OBJECT_MODULE_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS((obj), GEDIT_TYPE_OBJECT_MODULE, GeditObjectModuleClass))

typedef struct _GeditObjectModule 		GeditObjectModule;
typedef struct _GeditObjectModulePrivate	GeditObjectModulePrivate;

struct _GeditObjectModule
{
	GTypeModule parent;

	GeditObjectModulePrivate *priv;
};

typedef struct _GeditObjectModuleClass GeditObjectModuleClass;

struct _GeditObjectModuleClass
{
	GTypeModuleClass parent_class;

	/* Virtual class methods */
	void		 (* garbage_collect)	();
};

GType		 gedit_object_module_get_type			(void) G_GNUC_CONST;
GeditObjectModule *gedit_object_module_new			(const gchar *module_name,
								 const gchar *path,
								 const gchar *type_registration);

GObject		*gedit_object_module_new_object			(GeditObjectModule *module, 
								 const gchar	   *first_property_name,
								 ...);

GType		 gedit_object_module_get_object_type		(GeditObjectModule *module);
const gchar	*gedit_object_module_get_path			(GeditObjectModule *module);
const gchar	*gedit_object_module_get_module_name		(GeditObjectModule *module);
const gchar 	*gedit_object_module_get_type_registration 	(GeditObjectModule *module);


/**
 * GEDIT_OBJECT_MODULE_REGISTER_TYPE_WITH_CODE(type_registration, TypeName, type_name, PARENT_TYPE, CODE):
 *
 * Utility macro used to register types with additional code.
 */
#define GEDIT_OBJECT_MODULE_REGISTER_TYPE_WITH_CODE(type_registration, TypeName, type_name, PARENT_TYPE, CODE) \
										\
static GType g_define_type_id = 0;						\
										\
GType										\
type_name##_get_type (void)							\
{										\
	return g_define_type_id;						\
}										\
										\
static void     type_name##_init              (TypeName        *self);		\
static void     type_name##_class_init        (TypeName##Class *klass);		\
static gpointer type_name##_parent_class = NULL;				\
static void     type_name##_class_intern_init (gpointer klass)			\
{										\
	type_name##_parent_class = g_type_class_peek_parent (klass);		\
	type_name##_class_init ((TypeName##Class *) klass);			\
}										\
										\
G_MODULE_EXPORT GType								\
type_registration (GTypeModule *module)						\
{										\
	static const GTypeInfo our_info =					\
	{									\
		sizeof (TypeName##Class),					\
		NULL, /* base_init */						\
		NULL, /* base_finalize */					\
		(GClassInitFunc) type_name##_class_intern_init,			\
		NULL,								\
		NULL, /* class_data */						\
		sizeof (TypeName),						\
		0, /* n_preallocs */						\
		(GInstanceInitFunc) type_name##_init				\
	};									\
										\
	g_define_type_id = g_type_module_register_type (module,			\
					    PARENT_TYPE,			\
					    #TypeName,				\
					    &our_info,				\
					    0);					\
										\
	CODE									\
										\
	return g_define_type_id;						\
}

/**
 * GEDIT_PLUGIN_REGISTER_TYPE(TypeName, type_name):
 * 
 * Utility macro used to register plugins.
 */
#define GEDIT_OBJECT_MODULE_REGISTER_TYPE(type_registration, TypeName, type_name, PARENT_TYPE) \
	GEDIT_OBJECT_MODULE_REGISTER_TYPE_WITH_CODE(type_registration, TypeName, type_name, PARENT_TYPE, ;)

/**
 * GEDIT_PLUGIN_DEFINE_TYPE_WITH_CODE(ObjectName, object_name, PARENT_TYPE, CODE):
 *
 * Utility macro used to register gobject types in plugins with additional code.
 */
#define GEDIT_OBJECT_MODULE_DEFINE_TYPE_WITH_CODE(ObjectName, object_name, PARENT_TYPE, CODE)	\
										\
static GType g_define_type_id = 0;						\
										\
GType										\
object_name##_get_type (void)							\
{										\
	return g_define_type_id;						\
}										\
										\
static void     object_name##_init              (ObjectName        *self);	\
static void     object_name##_class_init        (ObjectName##Class *klass);	\
static gpointer object_name##_parent_class = NULL;				\
static void     object_name##_class_intern_init (gpointer klass)		\
{										\
	object_name##_parent_class = g_type_class_peek_parent (klass);		\
	object_name##_class_init ((ObjectName##Class *) klass);			\
}										\
										\
GType										\
object_name##_register_type (GTypeModule *module)				\
{										\
	static const GTypeInfo our_info =					\
	{									\
		sizeof (ObjectName##Class),					\
		NULL, /* base_init */						\
		NULL, /* base_finalize */					\
		(GClassInitFunc) object_name##_class_intern_init,		\
		NULL,								\
		NULL, /* class_data */						\
		sizeof (ObjectName),						\
		0, /* n_preallocs */						\
		(GInstanceInitFunc) object_name##_init				\
	};									\
										\
	g_define_type_id = g_type_module_register_type (module,			\
					   	        PARENT_TYPE,		\
					                #ObjectName,		\
					                &our_info,		\
					                0);			\
										\
	CODE									\
										\
	return g_define_type_id;						\
}

/**
 * GEDIT_OBJECT_MODULE_DEFINE_TYPE(ObjectName, object_name, PARENT_TYPE):
 *
 * Utility macro used to register gobject types in plugins.
 */
#define GEDIT_OBJECT_MODULE_DEFINE_TYPE(ObjectName, object_name, PARENT_TYPE)	\
	GEDIT_OBJECT_MODULE_DEFINE_TYPE_WITH_CODE(ObjectName, object_name, PARENT_TYPE, ;)

/**
 * GEDIT_OBJECT_MODULE_IMPLEMENT_INTERFACE(TYPE_IFACE, iface_init):
 *
 * Utility macro used to register interfaces for gobject types in plugins.
 */
#define GEDIT_OBJECT_MODULE_IMPLEMENT_INTERFACE(object_name, TYPE_IFACE, iface_init)	\
	const GInterfaceInfo object_name##_interface_info = 			\
	{ 									\
		(GInterfaceInitFunc) iface_init,				\
		NULL, 								\
		NULL								\
	};									\
										\
	g_type_module_add_interface (module, 					\
				     g_define_type_id, 				\
				     TYPE_IFACE, 				\
				     &object_name##_interface_info);
G_END_DECLS

#endif
