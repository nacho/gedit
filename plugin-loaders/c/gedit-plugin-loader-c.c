#include "gedit-plugin-loader-c.h"
#include <gedit/gedit-object-module.h>

#define GEDIT_PLUGIN_LOADER_C_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE((object), GEDIT_TYPE_PLUGIN_LOADER_C, GeditPluginLoaderCPrivate))

struct _GeditPluginLoaderCPrivate
{
	GHashTable *loaded_plugins;
};

static void gedit_plugin_loader_iface_init (gpointer g_iface, gpointer iface_data);

GEDIT_PLUGIN_LOADER_REGISTER_TYPE (GeditPluginLoaderC, gedit_plugin_loader_c);

static const gchar *
gedit_plugin_loader_iface_get_name (void)
{
	return "C";
}

static GeditPlugin *
gedit_plugin_loader_iface_load (GeditPluginLoader *loader,
				GeditPluginInfo   *info,
				const gchar       *path)
{
	GeditPluginLoaderC *cloader = GEDIT_PLUGIN_LOADER_C (loader);
	GeditObjectModule *module;
	GeditPlugin *result;
	
	module = (GeditObjectModule *)g_hash_table_lookup (cloader->priv->loaded_plugins, info);
	
	if (module == NULL)
	{
		module = gedit_object_module_new (gedit_plugin_info_get_module_name (info),
						  path,
						  "register_gedit_plugin");
		g_hash_table_insert (cloader->priv->loaded_plugins, info, module);
	}

	if (!g_type_module_use (G_TYPE_MODULE (module)))
	{
		g_warning ("Could not load plugin module: %s", gedit_plugin_info_get_name (info));

		return NULL;
	}
	
	/* create new plugin object */
	result = (GeditPlugin *)gedit_object_module_new_object (module, NULL);
	
	if (!result)
	{
		g_warning ("Could not create plugin object: %s", gedit_plugin_info_get_name (info));
		g_type_module_unuse (G_TYPE_MODULE (module));
		
		return NULL;
	}
	
	g_type_module_unuse (G_TYPE_MODULE (module));
	
	return result;
}

static void
gedit_plugin_loader_iface_unload (GeditPluginLoader *loader,
				  GeditPluginInfo   *info)
{
	//GeditPluginLoaderC *cloader = GEDIT_PLUGIN_LOADER_C (loader);
	
	/* this is a no-op, since the type module will be properly unused as
	   the last reference to the plugin dies. When the plugin is activated
	   again, the library will be reloaded */
}

static void
gedit_plugin_loader_iface_init (gpointer g_iface, 
				gpointer iface_data)
{
	GeditPluginLoaderInterface *iface = (GeditPluginLoaderInterface *)g_iface;
	
	iface->get_name = gedit_plugin_loader_iface_get_name;
	iface->load = gedit_plugin_loader_iface_load;
	iface->unload = gedit_plugin_loader_iface_unload;
}

static void
gedit_plugin_loader_c_finalize (GObject *object)
{
	GeditPluginLoaderC *cloader = GEDIT_PLUGIN_LOADER_C (object);
	GList *infos;
	GList *item;

	infos = g_hash_table_get_keys (cloader->priv->loaded_plugins);
	
	for (item = infos; item; item = item->next)
	{
		GeditPluginInfo *info = (GeditPluginInfo *)item->data;
		
		if (gedit_plugin_info_is_active (info))
		{
			g_warning ("There are still C plugins loaded during destruction");
			break;
		}
	}
	
	g_hash_table_destroy (cloader->priv->loaded_plugins);
	
	G_OBJECT_CLASS (gedit_plugin_loader_c_parent_class)->finalize (object);
}

static void
gedit_plugin_loader_c_class_init (GeditPluginLoaderCClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	
	object_class->finalize = gedit_plugin_loader_c_finalize;

	g_type_class_add_private (object_class, sizeof (GeditPluginLoaderCPrivate));
}

static void
gedit_plugin_loader_c_init (GeditPluginLoaderC *self)
{
	self->priv = GEDIT_PLUGIN_LOADER_C_GET_PRIVATE (self);
	
	/* loaded_plugins maps GeditPluginInfo to a GeditObjectModule */
	self->priv->loaded_plugins = g_hash_table_new (g_direct_hash,
						       g_direct_equal);
}

GeditPluginLoaderC *
gedit_plugin_loader_c_new ()
{
	GObject *loader = g_object_new (GEDIT_TYPE_PLUGIN_LOADER_C, NULL);

	return GEDIT_PLUGIN_LOADER_C (loader);
}
