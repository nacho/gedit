#ifndef __GEDIT_PLUGIN_LOADER_C_H__
#define __GEDIT_PLUGIN_LOADER_C_H__

#include <gedit/gedit-plugin-loader.h>

G_BEGIN_DECLS

#define GEDIT_TYPE_PLUGIN_LOADER_C		(gedit_plugin_loader_c_get_type ())
#define GEDIT_PLUGIN_LOADER_C(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_PLUGIN_LOADER_C, GeditPluginLoaderC))
#define GEDIT_PLUGIN_LOADER_C_CONST(obj)	(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_PLUGIN_LOADER_C, GeditPluginLoaderC const))
#define GEDIT_PLUGIN_LOADER_C_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GEDIT_TYPE_PLUGIN_LOADER_C, GeditPluginLoaderCClass))
#define GEDIT_IS_PLUGIN_LOADER_C(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEDIT_TYPE_PLUGIN_LOADER_C))
#define GEDIT_IS_PLUGIN_LOADER_C_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEDIT_TYPE_PLUGIN_LOADER_C))
#define GEDIT_PLUGIN_LOADER_C_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GEDIT_TYPE_PLUGIN_LOADER_C, GeditPluginLoaderCClass))

typedef struct _GeditPluginLoaderC		GeditPluginLoaderC;
typedef struct _GeditPluginLoaderCClass		GeditPluginLoaderCClass;
typedef struct _GeditPluginLoaderCPrivate	GeditPluginLoaderCPrivate;

struct _GeditPluginLoaderC {
	GObject parent;
	
	GeditPluginLoaderCPrivate *priv;
};

struct _GeditPluginLoaderCClass {
	GObjectClass parent_class;
};

GType gedit_plugin_loader_c_get_type (void) G_GNUC_CONST;
GeditPluginLoaderC *gedit_plugin_loader_c_new(void);

/* All the loaders must implement this function */
G_MODULE_EXPORT GType register_gedit_plugin_loader (GTypeModule * module);

G_END_DECLS

#endif /* __GEDIT_PLUGIN_LOADER_C_H__ */
