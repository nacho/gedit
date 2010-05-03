#ifndef __GEDIT_APP_OSX_H__
#define __GEDIT_APP_OSX_H__

#include "gedit-app.h"
#include <ige-mac-integration.h>

G_BEGIN_DECLS

#define GEDIT_TYPE_APP_OSX		(gedit_app_osx_get_type ())
#define GEDIT_APP_OSX(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_APP_OSX, GeditAppOSX))
#define GEDIT_APP_OSX_CONST(obj)	(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_APP_OSX, GeditAppOSX const))
#define GEDIT_APP_OSX_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GEDIT_TYPE_APP_OSX, GeditAppOSXClass))
#define GEDIT_IS_APP_OSX(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEDIT_TYPE_APP_OSX))
#define GEDIT_IS_APP_OSX_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEDIT_TYPE_APP_OSX))
#define GEDIT_APP_OSX_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GEDIT_TYPE_APP_OSX, GeditAppOSXClass))

typedef struct _GeditAppOSX		GeditAppOSX;
typedef struct _GeditAppOSXClass	GeditAppOSXClass;
typedef struct _GeditAppOSXPrivate	GeditAppOSXPrivate;

struct _GeditAppOSX {
	GeditApp parent;
};

struct _GeditAppOSXClass {
	GeditAppClass parent_class;
};

GType gedit_app_osx_get_type (void) G_GNUC_CONST;
void gedit_app_osx_set_window_title (GeditAppOSX *app, GeditWindow *window, const gchar *title, GeditDocument *document);
gboolean gedit_app_osx_show_url (GeditAppOSX *app, const gchar *url);
gboolean gedit_app_osx_show_help (GeditAppOSX *app, const gchar *link_id);

G_END_DECLS

#endif /* __GEDIT_APP_OSX_H__ */

/* ex:ts=8:noet: */
