/* 
 * Reverse plugin
 * Roberto Majadas <phoenix@nova.es>
 *
 * Reverse text
 */


#include <gnome.h>

#include "document.h"
#include "plugin.h"


static void
destroy_plugin (PluginData *pd)
{
	g_free (pd->name);
}

static void
reverse (void)
{
     Document *doc = gedit_document_current();
     gchar *buffer ;
     gchar tmp ;
     gint buffer_length ;
     gint i;

     if (!doc)
	  return;

     buffer_length = gedit_document_get_buffer_length (doc);
     buffer = gedit_document_get_buffer (doc);

     for (i=0; i < ( buffer_length / 2 ); i++)
     {
	  tmp = buffer [i];
	  buffer [i] = buffer [buffer_length - i - 1];
	  buffer [buffer_length - i - 1] = tmp;
     }
     
     gedit_document_delete_text (doc, 0, buffer_length, TRUE);
     gedit_document_insert_text (doc, buffer, 0, TRUE);
     
     g_free (buffer);
     
}
gint
init_plugin (PluginData *pd)
{
	/* initialize */
     
	pd->destroy_plugin = destroy_plugin;
	pd->name = _("Reverse");
	pd->desc = _("Reverse text");
	pd->author = "Roberto Majadas <phoenix@nova.es>";
	
	pd->private_data = (gpointer)reverse;
	
	return PLUGIN_OK;
}

