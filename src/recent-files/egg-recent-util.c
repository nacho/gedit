#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include "egg-recent-util.h"

#define EGG_RECENT_UTIL_HOSTNAME_SIZE 512

/* ripped out of gedit2 */
gchar* 
egg_recent_util_escape_underlines (const gchar* text)
{
	GString *str;
	gint length;
	const gchar *p;
 	const gchar *end;

  	g_return_val_if_fail (text != NULL, NULL);

    	length = strlen (text);

	str = g_string_new ("");

  	p = text;
  	end = text + length;

  	while (p != end)
    	{
      		const gchar *next;
      		next = g_utf8_next_char (p);

		switch (*p)
        	{
       			case '_':
          			g_string_append (str, "__");
          			break;
        		default:
          			g_string_append_len (str, p, next - p);
          			break;
        	}

      		p = next;
    	}

	return g_string_free (str, FALSE);
}

const gchar *
egg_recent_util_get_stock_icon (const gchar *mime_type)
{
	if (!strncmp (mime_type, "x-directory", 11))
		return GTK_STOCK_OPEN;
	else
		return GTK_STOCK_NEW;
}

gchar *
egg_recent_util_get_unique_id (void)
{
	char hostname[EGG_RECENT_UTIL_HOSTNAME_SIZE];
	time_t the_time;
	guint32 rand;
	int pid;
	
	gethostname (hostname, EGG_RECENT_UTIL_HOSTNAME_SIZE);
	
	time (&the_time);
	rand = g_random_int ();
	pid = getpid ();

	return g_strdup_printf ("%s-%d-%d-%d", hostname, (int)time, (int)rand, (int)pid);
}
