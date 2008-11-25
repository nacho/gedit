#ifndef __GEDIT_PLUGIN_LOADER_PYTHON_H__
#define __GEDIT_PLUGIN_LOADER_PYTHON_H__

#include <gedit/gedit-plugin-loader.h>

G_BEGIN_DECLS

#define GEDIT_TYPE_PLUGIN_LOADER_PYTHON		(gedit_plugin_loader_python_get_type ())
#define GEDIT_PLUGIN_LOADER_PYTHON(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_PLUGIN_LOADER_PYTHON, GeditPluginLoaderPython))
#define GEDIT_PLUGIN_LOADER_PYTHON_CONST(obj)	(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_PLUGIN_LOADER_PYTHON, GeditPluginLoaderPython const))
#define GEDIT_PLUGIN_LOADER_PYTHON_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GEDIT_TYPE_PLUGIN_LOADER_PYTHON, GeditPluginLoaderPythonClass))
#define GEDIT_IS_PLUGIN_LOADER_PYTHON(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEDIT_TYPE_PLUGIN_LOADER_PYTHON))
#define GEDIT_IS_PLUGIN_LOADER_PYTHON_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEDIT_TYPE_PLUGIN_LOADER_PYTHON))
#define GEDIT_PLUGIN_LOADER_PYTHON_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GEDIT_TYPE_PLUGIN_LOADER_PYTHON, GeditPluginLoaderPythonClass))

typedef struct _GeditPluginLoaderPython		GeditPluginLoaderPython;
typedef struct _GeditPluginLoaderPythonClass		GeditPluginLoaderPythonClass;
typedef struct _GeditPluginLoaderPythonPrivate	GeditPluginLoaderPythonPrivate;

struct _GeditPluginLoaderPython {
	GObject parent;
	
	GeditPluginLoaderPythonPrivate *priv;
};

struct _GeditPluginLoaderPythonClass {
	GObjectClass parent_class;
};

GType gedit_plugin_loader_python_get_type (void) G_GNUC_CONST;
GeditPluginLoaderPython *gedit_plugin_loader_python_new(void);

/* All the loaders must implement this function */
G_MODULE_EXPORT GType register_gedit_plugin_loader (GTypeModule * module);

G_END_DECLS

#endif /* __GEDIT_PLUGIN_LOADER_PYTHON_H__ */

