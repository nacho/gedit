/* 
 * Sample plugin demo
 * Alex Roberts <bse@error.fsnet.co.uk>
 *
 * Prints "Hello World" into the current document
 */
 
#include <config.h>
#include <gnome.h>

#include "window.h"
#include "document.h"
#include "utils.h"

#if 0
  #include "../../src/view.h"
  #include "../../src/plugin.h"
#else
  #include "view.h"
  #include "plugin.h"
#endif

static void
destroy_plugin (PluginData *pd)
{
	gedit_debug (DEBUG_PLUGINS, "");

	g_free (pd->name);
}


/* the function that actually does the wrok */
static void
insert_hello (void)
{
	GeditView *view = gedit_view_active();

	gedit_debug (DEBUG_PLUGINS, "");

	if (!view)
	     return;

	gedit_document_insert_text (view->doc, "Hello World", gedit_view_get_position (view), TRUE);
	
}
	

gint
init_plugin (PluginData *pd)
{
	/* initialize */
	gedit_debug (DEBUG_PLUGINS, "");
     
	pd->destroy_plugin = destroy_plugin;
	pd->name = _("Hello World");
	pd->desc = _("Sample 'hello world' plugin.");
	pd->long_desc = _("Sample 'hello world' plugin.");
	pd->author = "Alex Roberts <bse@error.fsnet.co.uk>";
	pd->needs_a_document = TRUE;
	pd->modifies_an_existing_doc = TRUE;

	pd->private_data = (gpointer)insert_hello;
	
	return PLUGIN_OK;
}




