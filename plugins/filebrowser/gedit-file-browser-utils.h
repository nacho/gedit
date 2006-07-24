#ifndef __GEDIT_FILE_BROWSER_UTILS_H__
#define __GEDIT_FILE_BROWSER_UTILS_H__

#include <gedit/gedit-window.h>

GdkPixbuf *gedit_file_browser_utils_pixbuf_from_theme     (gchar const *name,
                                                           GtkIconSize size);

GdkPixbuf *gedit_file_browser_utils_pixbuf_from_mime_type (gchar const *uri,
                                                           gchar const *mime,
                                                           GtkIconSize size);
gchar * gedit_file_browser_utils_uri_basename             (gchar const * uri);
gboolean gedit_file_browser_utils_confirmation_dialog     (GeditWindow * window,
                                                           GtkMessageType type,
                                                           gchar const *message,
		                                           gchar const *secondary, 
		                                           gchar const * button_stock, 
		                                           gchar const * button_label);
#endif /* __GEDIT_FILE_BROWSER_UTILS_H__ */
