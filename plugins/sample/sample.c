/* 
 * Sample plugin demo
 * Alex Roberts <bse@error.fsnet.co.uk>
 *
 * Prints "Hello World" into the current document
 */
 
#include <gnome.h>
#include <config.h>

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
	gint i;
	View *view = VIEW (mdi->active_view);
	Document *doc = gedit_document_current();
		
	i = gedit_view_get_position (view);

	gtk_text_freeze (GTK_TEXT (view->text));
	gtk_editable_insert_text (GTK_EDITABLE (view->text), "Hello World", 11, &i);
	
		
	gtk_text_thaw (GTK_TEXT (view->text));
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
