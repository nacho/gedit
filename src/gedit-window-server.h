
#ifndef __GEDIT_WINDOW_SERVER_H
#define __GEDIT_WINDOW_SERVER_H

#include <bonobo/bonobo-control.h>
#include <bonobo/bonobo-object.h>
#include <bonobo/bonobo-window.h>
#include "GNOME_Gedit.h"

G_BEGIN_DECLS

#define GEDIT_WINDOW_SERVER_TYPE         (gedit_window_server_get_type ())
#define GEDIT_WINDOW_SERVER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GEDIT_WINDOW_SERVER_TYPE, GeditWindowServer))
#define GEDIT_WINDOW_SERVER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), GEDIT_WINDOW_SERVER_TYPE, GeditWindowServerClass))
#define GEDIT_WINDOW_SERVER_IS_OBJECT(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GEDIT_WINDOW_SERVER_TYPE))
#define GEDIT_WINDOW_SERVER_IS_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GEDIT_WINDOW_SERVER_TYPE))
#define GEDIT_WINDOW_SERVER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GEDIT_WINDOW_SERVER_TYPE, GeditWindowServerClass))

typedef struct
{
        BonoboObject parent;

	BonoboWindow *win;
} GeditWindowServer;

typedef struct
{
        BonoboObjectClass parent_class;

        POA_GNOME_Gedit_Window__epv epv;
} GeditWindowServerClass;

GType          gedit_window_server_get_type (void);

BonoboObject  *gedit_window_server_new      (BonoboWindow *win);

G_END_DECLS

#endif /* __GEDIT_WINDOW_SERVER_H */
