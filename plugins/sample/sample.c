/* 
 * Sample plugin demo
 * Alex Roberts <bse@error.fsnet.co.uk>
 *
 * Prints "Hello World" into the current document
 */
 
#include <gnome.h>
#include <config.h>

#include "../../src/main.h"
#include "../../src/gE_mdi.h"
#include "../../src/gE_view.h"
#include "../../src/gE_plugin.h"


/* first the gE_plugin centric code */

static void destroy_plugin (gE_Plugin_Data *pd)
{
	g_free (pd->name);
	
}


/* the function that actually does the wrok */
static void insert_hello ()
{
	gint i;
	gE_view *view = GE_VIEW (mdi->active_view);
	gE_document *doc = gE_document_current();
	
		
	i = gE_view_get_position (view);

	gtk_text_freeze (GTK_TEXT (view->text));
	gtk_editable_insert_text (GTK_EDITABLE (view->text), "Hello World", 11, &i);
	
		
	gtk_text_thaw (GTK_TEXT (view->text));
		
		
}
	


gint init_plugin (gE_Plugin_Data *pd)
{
	/* initialise */
	pd->destroy_plugin = destroy_plugin;
	pd->name = _("Hello World");
	pd->desc = _("Sample 'hello world' plugin.");
	pd->author = "Alex Roberts <bse@error.fsnet.co.uk>";
	
	pd->private_data = (gpointer)insert_hello;
	
	return PLUGIN_OK;
}
