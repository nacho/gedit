/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * Spell Check plugin.
 * Roberto Majadas <phoenix@nova.es>
 *
 *
 */
 

#include <gnome.h>
#include <glade/glade.h>
#include <ctype.h>

#include "window.h"
#include "document.h"
#include "plugin.h"

/*Chema , echale un vistazo a este codigo que se me atraviesa . Lo que he hecho es meter en un
 buffer (buffer) todo el texto , buffer_position => posicion respecto al buffer y
 spelling_position posicion respecto al widget de texto . Mira haber si sabes donde falla */

GtkWidget *dialog ;
GtkWidget *spell ;

Document *doc ;

GString *buffer ;
gint spelling_position = 0 ;
gint buffer_position = 0 ;

static void
destroy_plugin (PluginData *pd)
{
	g_free (pd->name);
}

static void
spell_check_finish ( GtkWidget *w , gpointer data )
{
	g_string_free (buffer,TRUE);
	gnome_dialog_close (GNOME_DIALOG (dialog));
}

static gchar *
spell_check_parse_word ( )
{
	GString  *tmp_buffer= g_string_new (NULL) ;
	gchar c ;
	
	while (!isalnum (buffer->str[buffer_position]))
	{
		spelling_position = spelling_position + 1;
		buffer_position = buffer_position + 1;
		if (buffer_position > buffer->len)
		{
			return NULL ;
		}

	}

	while (isalnum (c= buffer->str[buffer_position]) || c == '\'')
	{
		tmp_buffer = g_string_append_c (tmp_buffer , c );

		buffer_position = buffer_position + 1;
		if (buffer_position > buffer->len)
		{
			return NULL ;
		}
	}
	
	return g_strdup (tmp_buffer->str) ;
	
}

static void
spell_check_start ( void )
{
	gint result =0 ;
	gchar *word = NULL ;

	while ( !result )
	{
		if (NULL != (word = spell_check_parse_word ()))
		{
			result = gnome_spell_check (GNOME_SPELL(spell), word);
			g_free (word);
		}
		else
		{
			result = 1;
		}
	}		
	
}


static void
handled_word_cb ( GtkWidget *spell , gpointer data )
{
	GString *word_to_change ;
	GnomeSpellInfo *si = (GnomeSpellInfo *) GNOME_SPELL(spell)->spellinfo->data;
	gint len ;
	
	len = strlen (si->word);
	
	if(si->replacement)
	{
	
                word_to_change = g_string_new (si->replacement);
	        gedit_document_replace_text ( doc , word_to_change->str , spelling_position , len  , TRUE );
	}
	else
	{
		
		word_to_change = g_string_new (si->word);
        }
	
	spelling_position = spelling_position + word_to_change->len + 1 ;

	g_string_free (word_to_change,TRUE);

	gtk_idle_add((GtkFunction) spell_check_start, NULL);

}

static void
spell_check (void){

     GladeXML *gui;
     GtkWidget *cancel;
    
     doc = gedit_document_current ();

     if (doc == NULL)
     {
	     return ;
     }

     buffer = g_string_new ( gedit_document_get_buffer (doc) );
     
     gui = glade_xml_new (GEDIT_GLADEDIR "/spell.glade",NULL);

     if (!gui)
     {
	     g_warning ("Could not find spell.glade");
	     return;
     }

     dialog     = glade_xml_get_widget (gui,"spell_check_dialog");
     cancel     = glade_xml_get_widget (gui,"cancel_button");     
     spell      = glade_xml_get_widget (gui,"spell_widget");
          
     gtk_signal_connect (GTK_OBJECT ( cancel ) , "clicked" , GTK_SIGNAL_FUNC( spell_check_finish ) , NULL);
     gtk_signal_connect (GTK_OBJECT ( dialog ) , "delete_event" , GTK_SIGNAL_FUNC( spell_check_finish ) , NULL);
     gtk_signal_connect (GTK_OBJECT ( spell ) , "handled_word" , GTK_SIGNAL_FUNC ( handled_word_cb ), NULL);

     gnome_dialog_set_parent (GNOME_DIALOG (dialog), gedit_window_active());
     gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);

     gtk_widget_show_all (dialog);
     gtk_object_destroy (GTK_OBJECT (gui));

     gtk_idle_add((GtkFunction) spell_check_start , NULL);
}



gint
init_plugin (PluginData *pd)
{
	/* initialise */
	pd->destroy_plugin = destroy_plugin;
	pd->name = _("Spell Check");
	pd->desc = _("Spell Check");
	pd->author = "Roberto Majadas <phoenix@nova.es>";
	
	pd->private_data = (gpointer)spell_check;

	return PLUGIN_OK;
}
