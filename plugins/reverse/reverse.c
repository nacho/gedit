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
     gchar *buf ;
     gchar tmp ;
     gint len ;
     gint i;

     if (!doc)
	  return;

     /*
     if ((len=gedit_document_get_buffer_length (doc))==0){
	  gnome_app_error (gedit_window_active(), _("There isn't any text to reverse."));
	  return ;
  }*/

     buf = gedit_document_get_buffer (doc);

     for (i=0; i < ( len / 2 ); i++)
     {
	  tmp = buf[i];
	  buf[i] = buf[len - i - 1];
	  buf[len - i - 1] = tmp;
     }
     
     gedit_document_delete_text (doc, 0, len, TRUE);
     gedit_document_insert_text (doc, buf, 0, FALSE);
     
     g_free (buf);
     
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

