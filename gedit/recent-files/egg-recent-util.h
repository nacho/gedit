
#ifndef __EGG_RECENT_UTIL__
#define __EGG_RECENT_UTIL__

G_BEGIN_DECLS

gchar * egg_recent_util_escape_underlines (const gchar *uri);
const gchar * egg_recent_util_get_stock_icon (const gchar *mime_type);
gchar * egg_recent_util_get_unique_id (void);

G_END_DECLS

#endif /* __EGG_RECENT_UTIL__ */
