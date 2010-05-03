#include "gedit-app-x11.h"

#define GEDIT_APP_X11_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE((object), GEDIT_TYPE_APP_X11, GeditAppX11Private))

G_DEFINE_TYPE (GeditAppX11, gedit_app_x11, GEDIT_TYPE_APP)

static void
gedit_app_x11_finalize (GObject *object)
{
	G_OBJECT_CLASS (gedit_app_x11_parent_class)->finalize (object);
}

static void
gedit_app_x11_class_init (GeditAppX11Class *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = gedit_app_x11_finalize;
}

static void
gedit_app_x11_init (GeditAppX11 *self)
{
}

/* ex:ts=8:noet: */
