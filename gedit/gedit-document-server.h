
#ifndef __GEDIT_DOCUMENT_SERVER_H
#define __GEDIT_DOCUMENT_SERVER_H

#include <bonobo/bonobo-control.h>
#include <bonobo/bonobo-object.h>
#include <gedit/gedit-document.h>
#include <gedit/GNOME_Gedit.h>

G_BEGIN_DECLS

#define GEDIT_DOCUMENT_SERVER_TYPE         (gedit_document_server_get_type ())
#define GEDIT_DOCUMENT_SERVER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), GEDIT_DOCUMENT_SERVER_TYPE, GeditDocumentServer))
#define GEDIT_DOCUMENT_SERVER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), GEDIT_DOCUMENT_SERVER_TYPE, GeditDocumentServerClass))
#define GEDIT_DOCUMENT_SERVER_IS_OBJECT(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), GEDIT_DOCUMENT_SERVER_TYPE))
#define GEDIT_DOCUMENT_SERVER_IS_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), GEDIT_DOCUMENT_SERVER_TYPE))
#define GEDIT_DOCUMENT_SERVER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), GEDIT_DOCUMENT_SERVER_TYPE, GeditDocumentServerClass))

typedef struct
{
        BonoboObject parent;

	GeditDocument *doc;
} GeditDocumentServer;

typedef struct
{
        BonoboObjectClass parent_class;

        POA_GNOME_Gedit_Document__epv epv;
} GeditDocumentServerClass;

GType          gedit_document_server_get_type (void);

BonoboObject  *gedit_document_server_new      (GeditDocument *doc);

G_END_DECLS

#endif /* __GEDIT_DOCUMENT_SERVER_H */
