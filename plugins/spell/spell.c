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
#include "utils.h"

/*Chema , echale un vistazo a este codigo que se me atraviesa . Lo que he hecho es meter en un
 buffer (buffer) todo el texto , buffer_position => posicion respecto al buffer y
 spelling_position posicion respecto al widget de texto . Mira haber si sabes donde falla */

GtkWidget *spell;

gchar *buffer;
guint  buffer_length;

static void
destroy_plugin (PluginData *pd)
{
	gedit_debug (DEBUG_PLUGINS, "");

	g_free (pd->name);
}

static void
spell_check_finish ( GtkWidget *widget, gpointer data )
{
	gedit_debug (DEBUG_PLUGINS, "");

	if (buffer != NULL)
		g_free (buffer);
	buffer = NULL;
}

static void
spell_check_cancel_clicked (GtkWidget *widget, gpointer data)
{
	gnome_dialog_close (GNOME_DIALOG (widget));
}

static gchar *
spell_check_parse_word (void)
{
	static gint buffer_position;
	gint word_length;
	gchar *word;

	
	gedit_debug (DEBUG_PLUGINS, "");

	word_length = 0;
	while ( (buffer_position + word_length) <= buffer_length)
	{
		if (isalnum (buffer [buffer_position + word_length]))
		{
			word_length++;
			continue;
		}
#if 0
		g_print ("buff_pos:%i wordlenght:%i -%s-\n",
			 buffer_position, word_length, word);
#endif
		word = g_strndup ( buffer + buffer_position, word_length);

		/* We need to advance buffer_position to the start
		   of the next word for the next time */
		buffer_position += word_length;

		while (!isalnum (buffer [buffer_position]))
			buffer_position++;
		
		return word;
	}

	return NULL;
}
	
static gint
spell_check_start ( void )
{
	gchar *word = NULL;
	gint retval;

	g_print ("Spell check start\n");
	
	word = spell_check_parse_word ();
	g_print ("Word to check for : *%s*\n", word);

	/* if we are done */
	if (word == NULL)
		return FALSE;

	retval = 0;
	
	retval = gnome_spell_check (GNOME_SPELL(spell), word);

	g_print ("retval %i\n", retval);

	if (retval!=1)
		g_print ("!!!!!!!!!!!!!!!!!"
			 "!!!!!!!!!!!!!!!!!"
			 "!!!!!!!!!!!!!!!!!"
			 "!!!!!\n");
	if (retval == 0)
		return FALSE;
	else
		return TRUE;
}


static void
handled_word_cb ( GtkWidget *spell , gpointer data )
{
	GString *word_to_change ;
	GnomeSpellInfo *si = (GnomeSpellInfo *) GNOME_SPELL(spell)->spellinfo->data;
	gint len ;
	Document *doc = data;

	gedit_debug (DEBUG_PLUGINS, "");
	
	g_return_if_fail (doc!=NULL);

	g_print ("Returning from handled_word_cb\n");

	gtk_idle_add ((GtkFunction) spell_check_start, NULL);

	return;
	
#if 0
	len = strlen (si->word);
	
	if (si->replacement)
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

	gtk_idle_add ((GtkFunction) spell_check_start, NULL);
#endif
	
}

static void
spell_check (void)
{

	GladeXML *gui;
	GtkWidget *cancel;
	GtkWidget *dialog;
	GtkWidget *dialog_vbox;
	Document *doc;
    
	gedit_debug (DEBUG_PLUGINS, "");

	doc = gedit_document_current ();

	if (doc == NULL)
		return ;

	gui = glade_xml_new (GEDIT_GLADEDIR "/spell.glade",NULL);

	if (!gui)
	{
		g_warning ("Could not find spell.glade");
		return;
	}

	dialog      = glade_xml_get_widget (gui, "dialog");
	cancel      = glade_xml_get_widget (gui, "cancel_button");
	dialog_vbox = glade_xml_get_widget (gui, "vbox");
	

	g_return_if_fail (dialog_vbox != NULL);
	g_return_if_fail (dialog != NULL);
	g_return_if_fail (cancel != NULL);

	spell = gnome_spell_new();

	g_return_if_fail (spell  != NULL);

	gtk_box_pack_start (GTK_BOX (dialog_vbox),
			    spell, FALSE, FALSE, FALSE);
	gtk_widget_show (spell);

	gtk_signal_connect (GTK_OBJECT (dialog), "clicked",
			    GTK_SIGNAL_FUNC (spell_check_cancel_clicked), NULL);
	gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
			    GTK_SIGNAL_FUNC (spell_check_finish), NULL);
	gtk_signal_connect (GTK_OBJECT (spell), "handled_word",
			    GTK_SIGNAL_FUNC (handled_word_cb), doc);

	gnome_dialog_set_parent (GNOME_DIALOG (dialog), gedit_window_active());
	gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);

	buffer = gedit_document_get_buffer (doc);
	buffer_length = strlen (buffer);

	if (buffer_length < 1)
		return;


	gtk_widget_show_all (dialog);
	gtk_object_destroy (GTK_OBJECT (gui));

	gtk_idle_add ((GtkFunction) spell_check_start, NULL);
}



gint
init_plugin (PluginData *pd)
{
	gedit_debug (DEBUG_PLUGINS, "");

	pd->destroy_plugin = destroy_plugin;
	pd->name = _("Spell Check");
	pd->desc = _("Spell Check");
	pd->author = "Roberto Majadas <phoenix@nova.es>";
	pd->needs_a_document = TRUE;
	
	pd->private_data = (gpointer)spell_check;

	return PLUGIN_OK;
}
