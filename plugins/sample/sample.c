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
	gE_view *view;
	gE_document *doc = gE_document_current();
	
	for (i = 0; i < g_list_length (doc->views); i++) {

		view = g_list_nth_data (doc->views, i);
		
		gtk_text_freeze (GTK_TEXT (view->text));
		
		gtk_text_set_point (GTK_TEXT(view->text), 
							gtk_text_get_point (GTK_TEXT(view->text)));
		gtk_text_insert (GTK_TEXT (view->text), NULL, NULL, NULL,
				"Hello World", 11);
		
		gtk_text_thaw (GTK_TEXT (view->text));
		
		
		gtk_text_freeze (GTK_TEXT (view->split_screen));

		gtk_text_set_point (GTK_TEXT(view->split_screen), 
							gtk_text_get_point (GTK_TEXT(view->text)));
		gtk_text_insert (GTK_TEXT (view->split_screen), NULL, NULL, NULL,
				"Hello World", 11);

		gtk_text_thaw (GTK_TEXT (view->split_screen));	
	}
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
