/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#ifndef __GEDIT_RECENT_H__
#define __GEDIT_RECENT_H__

#include <libbonoboui.h>

G_BEGIN_DECLS

#define GEDIT_RECENT(obj)		G_TYPE_CHECK_INSTANCE_CAST (obj, gedit_recent_get_type (), GeditRecent)
#define GEDIT_RECENT_CLASS(klass) 	G_TYPE_CHECK_CLASS_CAST (klass, gedit_recent_get_type (), GeditRecentClass)
#define GEDIT_IS_RECENT(obj)		G_TYPE_CHECK_INSTANCE_TYPE (obj, gedit_recent_get_type ())

#define GEDIT_RECENT_GLOBAL_LIST "gedit-recent-global"

typedef struct _GeditRecent GeditRecent;

typedef struct _GeditRecentClass GeditRecentClass;

GType        gedit_recent_get_type (void);
GeditRecent *   gedit_recent_new      (const gchar *appname, gint limit);
GeditRecent *	gedit_recent_new_with_ui_component (const gchar *appname,
						gint limit,
						BonoboUIComponent *uic,
						const gchar *path);

void gedit_recent_set_ui_component (GeditRecent *recent, BonoboUIComponent *uic);

gboolean     gedit_recent_add      (GeditRecent *recent, const gchar *uri);
gboolean     gedit_recent_delete   (GeditRecent *recent, const gchar *uri);
void         gedit_recent_clear    (GeditRecent *recent);
GSList * gedit_recent_get_list     (GeditRecent *recent);
void     gedit_recent_set_limit    (GeditRecent *recent, gint limit);
gint     gedit_recent_get_limit    (GeditRecent *recent);

G_END_DECLS

#endif /* __GEDIT_RECENT_H__ */
