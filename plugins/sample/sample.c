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

static void
destroy_plugin (PluginData *pd)
{
	g_free (pd->name);
}


/* the function that actually does the wrok */
static void
insert_hello (void)
{
	View *view = gedit_view_active();

	if (!view)
	     return;

	gedit_document_insert_text (view->doc, "Hello World", gedit_view_get_position (view), TRUE);
	
}
	

gint
init_plugin (PluginData *pd)
{
	/* initialize */
     
	pd->destroy_plugin = destroy_plugin;
	pd->name = _("Hello World");
	pd->desc = _("Sample 'hello world' plugin.");
	pd->author = "Alex Roberts <bse@error.fsnet.co.uk>";
	
	pd->private_data = (gpointer)insert_hello;
	
	return PLUGIN_OK;
}











