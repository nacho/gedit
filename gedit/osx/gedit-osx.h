#ifndef __GEDIT_OSX_H__
#define __GEDIT_OSX_H__

#include <gtk/gtk.h>
#include <gedit/gedit-window.h>

void 	gedit_osx_set_window_title 	(GeditWindow   *window, 
					 gchar const   *title,
					 GeditDocument *document);

gboolean gedit_osx_show_url 		(const gchar *url);
gboolean gedit_osx_show_help		(const gchar *link_id);

#endif /* __GEDIT_OSX_H__ */
