#ifndef __GEDIT_APP_X11_H__
#define __GEDIT_APP_X11_H__

#include "gedit-app.h"

G_BEGIN_DECLS

#define GEDIT_TYPE_APP_X11		(gedit_app_x11_get_type ())
#define GEDIT_APP_X11(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_APP_X11, GeditAppX11))
#define GEDIT_APP_X11_CONST(obj)	(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_APP_X11, GeditAppX11 const))
#define GEDIT_APP_X11_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GEDIT_TYPE_APP_X11, GeditAppX11Class))
#define GEDIT_IS_APP_X11(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEDIT_TYPE_APP_X11))
#define GEDIT_IS_APP_X11_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEDIT_TYPE_APP_X11))
#define GEDIT_APP_X11_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GEDIT_TYPE_APP_X11, GeditAppX11Class))

typedef struct _GeditAppX11		GeditAppX11;
typedef struct _GeditAppX11Class	GeditAppX11Class;
typedef struct _GeditAppX11Private	GeditAppX11Private;

struct _GeditAppX11 {
	GeditApp parent;
};

struct _GeditAppX11Class {
	GeditAppClass parent_class;
};

GType gedit_app_x11_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __GEDIT_APP_X11_H__ */

/* ex:ts=8:noet: */
