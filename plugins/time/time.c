/* 
 * Sample plugin demo
 * Alex Roberts <bse@error.fsnet.co.uk>
 *
 * Prints "Hello World" into the current document
 */

#include <config.h> 
#include <gnome.h>

#include "../../src/window.h"
#include "../../src/document.h"
#include "../../src/view.h"
#include "../../src/plugin.h"


/* first the gE_plugin centric code */

static void destroy_plugin (PluginData *pd)
{
	g_free (pd->name);
}


/* Gratiously ripped out of GIMP (app/general.c), with a fiew minor changes */
char *
get_time (void)
{
	static char static_buf[21];
  	gchar *tmp, *out = NULL;
  	time_t clock;
  	struct tm *now;
  	const char *format = NULL;
  	size_t out_length = 0;
  	

  	clock = time (NULL);
  	/*now = gmtime (&clock);*/
  	now = localtime (&clock);
	  	
  	tmp = static_buf;
	
  	/* date format derived from ISO 8601:1988 */
  	/*sprintf(tmp, "%04d-%02d-%02d%c%02d:%02d:%02d%c",
	  now->tm_year + 1900, now->tm_mon + 1, now->tm_mday,
	  ' ',
	  now->tm_hour, now->tm_min, now->tm_sec,
	  '\000'); 
	
	  return tmp;
	*/
	format = "%a %b %e %H:%M:%S %Z %Y";

	do
	{
		out_length += 200;
		out = (char *) realloc (out, out_length);
	}
  	while (strftime (out, out_length, format, now) == 0);

  	
  	return out;
}

/* the function that actually does the wrok */
static void
insert_time (void)
{
	gint i;
	View *view;
	Document *doc = gedit_document_current();
	static gchar *the_time;

	if (!doc)
	     return;

  	view = VIEW (mdi->active_view);
  	the_time = get_time();

	i = gedit_view_get_position (view);

	gtk_text_freeze (GTK_TEXT (view->text));
	gtk_editable_insert_text (GTK_EDITABLE (view->text), the_time,
				  strlen(the_time), &i);
	
		
	gtk_text_thaw (GTK_TEXT (view->text));
		
	g_free (the_time);
}

gint
init_plugin (PluginData *pd)
{
	/* initialise */
	pd->destroy_plugin = destroy_plugin;
	pd->name = _("Insert Time");
	pd->desc = _("Inserts the current date and time");
	pd->author = "Alex Roberts <bse@error.fsnet.co.uk>";
	
	pd->private_data = (gpointer)insert_time;
	
	return PLUGIN_OK;
}
