
#ifndef __GEDIT_APPLICATION_SERVER_H
#define __GEDIT_APPLICATION_SERVER_H

#include <gedit/GNOME_Gedit.h>
#include <bonobo/bonobo-control.h>
#include <bonobo/bonobo-object.h>

G_BEGIN_DECLS

#define GEDIT_APPLICATION_SERVER_TYPE         (gedit_application_server_get_type ())
#define GEDIT_APPLICATION_SERVER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GEDIT_APPLICATION_SERVER_TYPE, GeditApplicationServer))
#define GEDIT_APPLICATION_SERVER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), GEDIT_APPLICATION_SERVER_TYPE, GeditApplicationServerClass))
#define GEDIT_APPLICATION_SERVER_IS_OBJECT(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GEDIT_APPLICATION_SERVER_TYPE))
#define GEDIT_APPLICATION_SERVER_IS_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GEDIT_APPLICATION_SERVER_TYPE))
#define GEDIT_APPLICATION_SERVER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GEDIT_APPLICATION_SERVER_TYPE, GeditApplicationServerClass))

typedef struct
{
        BonoboObject parent;
} GeditApplicationServer;

typedef struct
{
        BonoboObjectClass parent_class;

        POA_GNOME_Gedit_Application__epv epv;
} GeditApplicationServerClass;

GType          gedit_application_server_get_type (void);

BonoboObject  *gedit_application_server_new      (GdkScreen *screen);

G_END_DECLS

#endif /* __GEDIT_APPLICATION_SERVER_H */
