/* spell.c - Spell plugin.
 *
 * Copyright (C) 1998 Martin Wahlen.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * Ugly code: but hey, I am new at this:-)
 *
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include <gnome.h>
#include "client.h"

/* Should be broken down into more structs one for doc and one for GUI stuff*/
typedef struct _SpellPlugin SpellPlugin;

struct _SpellPlugin {
	GtkWidget *window;
	GtkWidget *hbox;
	GtkWidget *done;
	GtkWidget *spell;
	gint context;
	gint docid;
	guint index;
	guint oldindex;
	guint offs;
	gint handling;
	char *buffer;
	char *result;
};

SpellPlugin *plugin;

/*Destroy handler: free the result buffer and tell gEdit to close the pipe*/ 
void spell_destroy(GtkWidget *widget, gpointer data)
{
	g_free(plugin->result);
	client_finish(plugin->context);
	gtk_exit(0);
}

void spell_exit(GtkWidget *widget, gpointer data)
{
	gint newdoc, i;

  	if(plugin->oldindex < strlen(plugin->buffer)) {
		for(i=plugin->oldindex; i<strlen(plugin->buffer); i++)
        		plugin->result[i+(plugin->offs)]=plugin->buffer[i];
  	}
  	newdoc = client_document_new(plugin->context, "Spell Checked");
  	client_text_append(newdoc,plugin->result, strlen(plugin->result));
  	client_document_show(newdoc); 
  	g_free(plugin->result);
  	client_finish(plugin->context);
  	gtk_exit(0);
}

static gchar*
parse_text (gchar* text, guint *old_index ) {
        gchar c;
        gchar buf[1024];
        guint i=0, len;
        guint index,text_start;

        index = *old_index;
        len = strlen(text);

        /* skip non isalnum chars */
        for ( ; index < len; ++index ) {
                c = text[index];
                if ( isalnum(c) || c == '\'' ) break;
        }

        if ( index == len ) {
		spell_exit(NULL, NULL);
		exit(0);
        }

        buf[i]= c;
        text_start = index;
        ++index;
        for ( ; index < len; ++index ) {
                c = text[index];
                if ( isalnum(c) || c == '\'' ) {
                        buf[++i] = c;
                } else
                        break;
        }
        buf[i+1] = 0;

        *old_index = index;
        return g_strdup(buf);
}

/*
 *Drawing magic! 
*/
SpellPlugin *spell_dialog_new()
{
	plugin = (SpellPlugin *) g_malloc(sizeof(SpellPlugin));
	plugin->spell = gtk_spell_new();
	plugin->hbox = gtk_hbox_new(TRUE,0);
	plugin->done = gnome_stock_button ("Button_Close");
	/* plugin->done = gtk_button_new_with_label ("Done"); */
	plugin->window = gtk_dialog_new();
	gtk_window_set_title (GTK_WINDOW(plugin->window), "Spell Check");	
	gtk_box_pack_start (GTK_BOX (plugin->hbox), plugin->done, FALSE,
				FALSE, 3);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (plugin->window)->vbox),
				plugin->spell, FALSE, FALSE, 3);

        gtk_box_pack_start (GTK_BOX (GTK_DIALOG (plugin->window)->vbox),
				plugin->hbox, FALSE, FALSE, 3);

	gtk_signal_connect (GTK_OBJECT(plugin->window), "destroy",
			 	(GtkSignalFunc) spell_destroy, NULL);	

	gtk_signal_connect (GTK_OBJECT(plugin->done), "clicked",
                                (GtkSignalFunc) spell_exit, NULL);

	gtk_widget_show(plugin->done);
	gtk_widget_show(plugin->hbox);
	gtk_widget_show(plugin->spell);
	gtk_widget_show(plugin->window);	

	return plugin; 
}

void spell_start_check()
{
        char *word, *result;

        if(!GTK_IS_SPELL(plugin->spell))
                return;
	
        while(plugin->handling && ((word=parse_text(plugin->buffer, &(plugin->index))) != NULL))
        {
        	result = gtk_spell_check(GTK_SPELL(plugin->spell), 
				(gchar *) word);
                plugin->handling = !result;
        }
}

void handled_word_callback(GtkWidget *spell, gpointer data)
{
        gchar *r;
        guint start, len;
        gint i;
	GtkSpellInfo *si = (GtkSpellInfo *) GTK_SPELL(spell)->spellinfo->data;

	g_return_if_fail(GTK_IS_SPELL(spell));

	len = strlen(si->word);
        start = plugin->index - len;

	if(si->replacement){
                r = si->replacement;
	} else {
		r = si->word;
        }

	for(i=plugin->oldindex; i<start; i++)
		plugin->result[i+(plugin->offs)]=plugin->buffer[i];
	strcpy(plugin->offs + i + plugin->result,r);
	plugin->oldindex = plugin->index;
	if(si->replacement)
		plugin->offs += strlen(si->replacement) - strlen(si->word);
	g_print("%s\n",plugin->result);	

	plugin->handling = 1;
	gtk_idle_add((GtkFunction) spell_start_check, NULL);
}

int main(int argc, char *argv[]){
	gint context;
	client_info info;

  	info.menu_location = "[Plugins]Spell Check...";

	context = client_init( &argc, &argv, &info );
	gnome_init("spellcheck-plugin",NULL, argc, argv, 0, NULL);
	plugin = spell_dialog_new();
	gtk_signal_connect (GTK_OBJECT (plugin->spell),
        	"handled_word",
        	GTK_SIGNAL_FUNC (handled_word_callback), NULL);

	plugin->context = context;
	plugin->docid = client_document_current(plugin->context);
	plugin->buffer = client_text_get(plugin->docid);
	plugin->result = g_malloc(strlen(plugin->buffer) * 2);
	plugin->index = 0;
	plugin->offs = 0;
	plugin->oldindex = 0;
	plugin->handling = 1;
	gtk_idle_add((GtkFunction) spell_start_check, NULL);
	gtk_main();
	
	return 0;
}
