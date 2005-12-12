/*
 * gedit-plugin.h
 * This file is part of gedit
 *
 * Copyright (C) 2002-2005 - Paolo Maggi 
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

#ifndef __GEDIT_PLUGIN_H__
#define __GEDIT_PLUGIN_H__

#include <glib-object.h>

#include <gedit/gedit-window.h>

/* TODO: add a .h file that includes all the .h files normally needed to
 * develop a plugin */ 

G_BEGIN_DECLS

/*
 * Type checking and casting macros
 */
#define GEDIT_TYPE_PLUGIN              (gedit_plugin_get_type())
#define GEDIT_PLUGIN(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), GEDIT_TYPE_PLUGIN, GeditPlugin))
#define GEDIT_PLUGIN_CONST(obj)        (G_TYPE_CHECK_INSTANCE_CAST((obj), GEDIT_TYPE_PLUGIN, GeditPlugin const))
#define GEDIT_PLUGIN_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass), GEDIT_TYPE_PLUGIN, GeditPluginClass))
#define GEDIT_IS_PLUGIN(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), GEDIT_TYPE_PLUGIN))
#define GEDIT_IS_PLUGIN_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GEDIT_TYPE_PLUGIN))
#define GEDIT_PLUGIN_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), GEDIT_TYPE_PLUGIN, GeditPluginClass))

/*
 * Main object structure
 */
typedef struct _GeditPlugin GeditPlugin;

struct _GeditPlugin 
{
	GObject parent;
};

/*
 * Class definition
 */
typedef struct _GeditPluginClass GeditPluginClass;

struct _GeditPluginClass 
{
	GObjectClass parent_class;

	/* Virtual public methods */
	
	void 		(*activate)		(GeditPlugin *plugin,
						 GeditWindow *window);
	void 		(*deactivate)		(GeditPlugin *plugin,
						 GeditWindow *window);

	void 		(*update_ui)		(GeditPlugin *plugin,
						 GeditWindow *window);

	GtkWidget      *(*create_configure_dialog)
						(GeditPlugin *plugin);

	/* Padding for future expansion */
	void		(*_gedit_reserved1)	(void);
	void		(*_gedit_reserved2)	(void);
	void		(*_gedit_reserved3)	(void);
	void		(*_gedit_reserved4)	(void);						
};

/*
 * Public methods
 */
GType 		 gedit_plugin_get_type 		(void) G_GNUC_CONST;

void 		 gedit_plugin_activate		(GeditPlugin *plugin,
						 GeditWindow *window);
void 		 gedit_plugin_deactivate	(GeditPlugin *plugin,
						 GeditWindow *window);
				 
void 		 gedit_plugin_update_ui		(GeditPlugin *plugin,
						 GeditWindow *window);

gboolean	 gedit_plugin_is_configurable	(GeditPlugin *plugin);
GtkWidget	*gedit_plugin_create_configure_dialog		
						(GeditPlugin *plugin);

/*
 * Utility macro used to register plugins
 *
 * use: GEDIT_PLUGIN_REGISTER_TYPE(GeditSamplePlugin, gedit_sample_plugin)
 */

#define GEDIT_PLUGIN_REGISTER_TYPE(PluginName, plugin_name)			\
										\
static GType plugin_name##_type = 0;						\
										\
GType										\
plugin_name##_get_type (void)							\
{										\
	return plugin_name##_type;						\
}										\
										\
static void     plugin_name##_init              (PluginName        *self);	\
static void     plugin_name##_class_init        (PluginName##Class *klass);	\
static gpointer plugin_name##_parent_class = NULL;				\
static void     plugin_name##_class_intern_init (gpointer klass)		\
{										\
	plugin_name##_parent_class = g_type_class_peek_parent (klass);		\
	plugin_name##_class_init ((PluginName##Class *) klass);			\
}										\
										\
G_MODULE_EXPORT GType								\
register_gedit_plugin (GTypeModule *module)					\
{										\
	static const GTypeInfo our_info =					\
	{									\
		sizeof (PluginName##Class),					\
		NULL, /* base_init */						\
		NULL, /* base_finalize */					\
		(GClassInitFunc) plugin_name##_class_intern_init,		\
		NULL,								\
		NULL, /* class_data */						\
		sizeof (PluginName),						\
		0, /* n_preallocs */						\
		(GInstanceInitFunc) plugin_name##_init				\
	};									\
										\
	gedit_debug_message (DEBUG_PLUGINS, "Registering " #PluginName);	\
										\
	/* Initialise the i18n stuff */						\
	bindtextdomain (GETTEXT_PACKAGE, GEDIT_LOCALEDIR);			\
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");			\
										\
	plugin_name##_type = g_type_module_register_type (module,		\
					    GEDIT_TYPE_PLUGIN,			\
					    #PluginName,			\
					    &our_info,				\
					    0);					\
	return plugin_name##_type;						\
}


G_END_DECLS

#endif  /* __GEDIT_PLUGIN_H__ */


