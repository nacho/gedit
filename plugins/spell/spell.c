/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * spell.c
 * This file is part of gedit
 *
 * Copyright (C) 2002 Paolo Maggi 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, 
 * Boston, MA 02111-1307, USA. 
 */
 
/*
 * Modified by the gedit Team, 2002. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h> /* For strlen */

#include <glib/gutils.h>
#include <libgnome/gnome-i18n.h>

#include <gedit-menus.h>
#include <gedit-plugin.h>
#include <gedit-debug.h>

#include <libgnomeui/gnome-stock-icons.h>

#include "gedit-spell-checker.h"
#include "gedit-spell-checker-dialog.h"
#include "gedit-spell-language-dialog.h"

#define MENU_ITEM_LABEL		N_("_Check Spelling...")
#define MENU_ITEM_PATH		"/menu/Tools/ToolsOps_1/"
#define MENU_ITEM_NAME		"SpellChecker"	
#define MENU_ITEM_TIP		N_("Check the spelling of the current document.")

#define MENU_ITEM_LABEL_LANG	N_("Set _Language")
#define MENU_ITEM_NAME_LANG	"SpellSetLanguage"	
#define MENU_ITEM_TIP_LANG	N_("Set the language of the current document.")

G_MODULE_EXPORT GeditPluginState update_ui (GeditPlugin *plugin, BonoboWindow *window);
G_MODULE_EXPORT GeditPluginState destroy (GeditPlugin *pd);
G_MODULE_EXPORT GeditPluginState activate (GeditPlugin *pd);
G_MODULE_EXPORT GeditPluginState deactivate (GeditPlugin *pd);
G_MODULE_EXPORT GeditPluginState init (GeditPlugin *pd);

typedef struct _CheckRange CheckRange;

struct _CheckRange
{
	GtkTextMark *start_mark;
	GtkTextMark *end_mark;

	gint mw_start; /* mispelled word start */
	gint mw_end;   /* end */

	GtkTextMark *current_mark;
};

static GQuark spell_checker_id = 0;
static GQuark check_range_id = 0;

static GeditSpellChecker *
get_spell_checker_from_document (GeditDocument *doc)
{
	GeditSpellChecker *spell;
	gpointer data;

	gedit_debug (DEBUG_PLUGINS, "");

	g_return_val_if_fail (doc != NULL, NULL);

	data = g_object_get_qdata (G_OBJECT (doc), spell_checker_id);

	if (data == NULL)
	{
		spell = gedit_spell_checker_new ();

		g_object_set_qdata_full (G_OBJECT (doc), 
					 spell_checker_id, 
					 spell, 
					 (GDestroyNotify)g_object_unref);
	}
	else
	{
		g_return_val_if_fail (GEDIT_IS_SPELL_CHECKER (data), NULL);
		spell = GEDIT_SPELL_CHECKER (data);
	}

	return spell;
}

static CheckRange*
get_check_range (GeditDocument *doc)
{
	CheckRange *range;

	gedit_debug (DEBUG_PLUGINS, "");

	g_return_val_if_fail (doc != NULL, NULL);

	range = (CheckRange *) g_object_get_qdata (G_OBJECT (doc), check_range_id);

	return range;
}

static void
update_current (GeditDocument *doc, gint current)
{
	CheckRange *range;
	GtkTextIter iter;
	GtkTextIter end_iter;

	gedit_debug (DEBUG_PLUGINS, "");

	g_return_if_fail (doc != NULL);
	g_return_if_fail (current >= 0);
	
	range = get_check_range (doc);
	g_return_if_fail (range != NULL);

	gtk_text_buffer_get_iter_at_offset (GTK_TEXT_BUFFER (doc), 
			&iter, current);

	if (!gtk_text_iter_inside_word (&iter))
	{	
		/* if we're not inside a word,
		 * we must be in some spaces.
		 * skip forward to the beginning of the next word. */
		if (!gtk_text_iter_is_end (&iter))
		{
			gtk_text_iter_forward_word_end (&iter);
			gtk_text_iter_backward_word_start (&iter);	
		}
	}
	else
	{
		if (!gtk_text_iter_starts_word (&iter))
			gtk_text_iter_backward_word_start (&iter);	
	}

	gtk_text_buffer_get_iter_at_mark (GTK_TEXT_BUFFER (doc), &end_iter,
			range->end_mark);

	if (gtk_text_iter_compare (&end_iter, &iter) < 0)
	{	
		gtk_text_buffer_move_mark (GTK_TEXT_BUFFER (doc), range->current_mark,
				&end_iter);
	}
	else
	{
		gtk_text_buffer_move_mark (GTK_TEXT_BUFFER (doc), range->current_mark,
				&iter);
	}
}

static void
set_check_range (GeditDocument *doc, gint start, gint end)
{
	CheckRange *range;
	GtkTextIter iter;
	
	gedit_debug (DEBUG_PLUGINS, "");

	g_return_if_fail (doc != NULL);
	g_return_if_fail (start >= 0);
	g_return_if_fail (start < gedit_document_get_char_count (doc));
	g_return_if_fail ((end >= start) || (end < 0));

	range = get_check_range (doc);

	if (range == NULL)
	{
		gedit_debug (DEBUG_PLUGINS, "There was not a previous check range");

		gtk_text_buffer_get_end_iter (GTK_TEXT_BUFFER (doc), &iter);
		
		range = g_new0 (CheckRange, 1);

		range->start_mark = gtk_text_buffer_create_mark (GTK_TEXT_BUFFER (doc),
				"check_range_start_mark", &iter, TRUE);

		range->end_mark = gtk_text_buffer_create_mark (GTK_TEXT_BUFFER (doc),
				"check_range_end_mark", &iter, FALSE);

		range->current_mark = gtk_text_buffer_create_mark (GTK_TEXT_BUFFER (doc),
				"check_range_current_mark", &iter, TRUE);
		
		g_object_set_qdata_full (G_OBJECT (doc), 
				 check_range_id, 
				 range, 
				 (GDestroyNotify)g_free);
	}
	
	gtk_text_buffer_get_iter_at_offset (GTK_TEXT_BUFFER (doc), 
			&iter, start);

	gtk_text_buffer_move_mark (GTK_TEXT_BUFFER (doc), range->start_mark, &iter);
	
	if (end < 0)
		end = gedit_document_get_char_count (doc);
	g_return_if_fail (end >= start);
	
	gtk_text_buffer_get_iter_at_offset (GTK_TEXT_BUFFER (doc), 
			&iter, end);

	if (!gtk_text_iter_inside_word (&iter))
	{	
		/* if we're neither inside a word,
		 * we must be in some spaces.
		 * skip backward to the end of the previous word. */
		if (!gtk_text_iter_is_end (&iter))
		{
			gtk_text_iter_backward_word_start (&iter);	
			gtk_text_iter_forward_word_end (&iter);
		}
	}
	else
	{
		if (!gtk_text_iter_ends_word (&iter))
			gtk_text_iter_forward_word_end (&iter);
	}

	gtk_text_buffer_move_mark (GTK_TEXT_BUFFER (doc), range->end_mark, &iter);
			
	range->mw_start = -1;
	range->mw_end = -1;
		
	update_current (doc, start);
}


static gboolean
get_current_word_extents (GeditDocument *doc, gint *start, gint *end)
{
	const CheckRange *range;
	
	GtkTextIter end_iter;
	GtkTextIter current_iter;

	gint range_end;

	gedit_debug (DEBUG_PLUGINS, "");

	g_return_val_if_fail (doc != NULL, FALSE);
	g_return_val_if_fail (start != NULL, FALSE);
	g_return_val_if_fail (end != NULL, FALSE);
	
	range = get_check_range (doc);
	g_return_val_if_fail (range != NULL, FALSE);

	gtk_text_buffer_get_iter_at_mark (GTK_TEXT_BUFFER (doc), 
			&end_iter, range->end_mark);

	range_end = gtk_text_iter_get_offset (&end_iter);

	gtk_text_buffer_get_iter_at_mark (GTK_TEXT_BUFFER (doc), 
			&current_iter, range->current_mark);

	end_iter = current_iter;

	if (!gtk_text_iter_is_end (&end_iter))
	{
		gedit_debug (DEBUG_PLUGINS, "Current is not end");
		
		gtk_text_iter_forward_word_end (&end_iter);
	}

	*start = gtk_text_iter_get_offset (&current_iter);
	*end = MIN (gtk_text_iter_get_offset (&end_iter), range_end);

	gedit_debug (DEBUG_PLUGINS, "Current word extends [%d, %d]", *start, *end);

	return (*start < *end);
}

static gboolean
goto_next_word (GeditDocument *doc)
{
	CheckRange *range;
	
	GtkTextIter current_iter;
	GtkTextIter old_current_iter;

	GtkTextIter end_iter;

	gint current;

	gedit_debug (DEBUG_PLUGINS, "");

	g_return_val_if_fail (doc != NULL, FALSE);

	range = get_check_range (doc);
	g_return_val_if_fail (range != NULL, FALSE);

	gtk_text_buffer_get_iter_at_mark (GTK_TEXT_BUFFER (doc), 
			&current_iter, range->current_mark);
	
	gtk_text_buffer_get_iter_at_mark (GTK_TEXT_BUFFER (doc), 
			&end_iter, range->end_mark);

	if (gtk_text_iter_compare (&current_iter, &end_iter) >= 0)
		return FALSE;

	old_current_iter = current_iter;
	
	gtk_text_iter_forward_word_ends (&current_iter, 2);
	
	gtk_text_iter_backward_word_start (&current_iter);

	current = gtk_text_iter_get_offset (&current_iter);

	if ((gtk_text_iter_compare (&old_current_iter, &current_iter) < 0) &&
	    (gtk_text_iter_compare (&current_iter, &end_iter) < 0))
	{
		update_current (doc, gtk_text_iter_get_offset (&current_iter));
		return TRUE;
	}
	else
		return FALSE;
}

static gchar*
get_next_mispelled_word (GeditDocument *doc)
{
	CheckRange *range;
	gint start, end;
	gchar *word;
	GeditSpellChecker *spell;

	g_return_val_if_fail (doc != NULL, NULL);

	range = get_check_range (doc);
	g_return_val_if_fail (range != NULL, NULL);

	spell = get_spell_checker_from_document (doc);
	g_return_val_if_fail (spell != NULL, NULL);

	if (!get_current_word_extents (doc, &start, &end))
		return NULL;

	word = gedit_document_get_chars (doc, start, end);
	g_return_val_if_fail (word != NULL, NULL);

	gedit_debug (DEBUG_PLUGINS, "Word to check: %s", word);

	while (gedit_spell_checker_check_word (spell, word, -1, NULL))
	{
		gboolean res;
		
		g_free (word);

		if (!goto_next_word (doc))
			return NULL;

		res = get_current_word_extents (doc, &start, &end);
		g_return_val_if_fail (res, NULL);

		word = gedit_document_get_chars (doc, start, end);
		g_return_val_if_fail (word != NULL, NULL);

		gedit_debug (DEBUG_PLUGINS, "Word to check: %s", word);
	}

	if (!goto_next_word (doc))
		update_current (doc, gedit_document_get_char_count (doc));

	if (word != NULL)
	{
		range->mw_start = start;
		range->mw_end = end;

		gedit_debug (DEBUG_PLUGINS, "Select [%d, %d]", start, end);

		gedit_document_set_selection (doc, start, end);
		gedit_view_scroll_to_cursor (gedit_get_active_view ());
	}
	else	
	{
		range->mw_start = -1;
		range->mw_end = -1;
	}

	return word;
}

static void
ignore_cb (GeditSpellCheckerDialog *dlg, const gchar *w, GeditDocument *doc)
{
	gchar *word = NULL;
	gedit_debug (DEBUG_PLUGINS, "");
	
	g_return_if_fail (doc != NULL);
	g_return_if_fail (w != NULL);

	word = get_next_mispelled_word (doc);
	if (word == NULL)
	{
		gedit_spell_checker_dialog_set_completed (dlg);
		
		return;
	}

	gedit_spell_checker_dialog_set_mispelled_word (GEDIT_SPELL_CHECKER_DIALOG (dlg),
			word, -1);
	g_free (word);

}

static void
change_cb (GeditSpellCheckerDialog *dlg, const gchar *word, const gchar *change, GeditDocument *doc)
{
	CheckRange *range;
	gchar *w = NULL;
	
	gedit_debug (DEBUG_PLUGINS, "");
	
	g_return_if_fail (doc != NULL);
	g_return_if_fail (word != NULL);
	g_return_if_fail (change != NULL);

	range = get_check_range (doc);
	g_return_if_fail (range != NULL);

	w = gedit_document_get_chars (doc, range->mw_start, range->mw_end);
	g_return_if_fail (w != NULL);

	if (strcmp (w, word) != 0)
	{
		g_free (w);
		return;
	}

	g_free (w);
       	
	gedit_document_begin_user_action (doc);

	gedit_document_delete_text (doc, range->mw_start, range->mw_end);
	gedit_document_insert_text (doc, range->mw_start, change, -1);

	gedit_document_end_user_action (doc);

	update_current (doc, range->mw_start + g_utf8_strlen (change, -1));

	/* go to next mispelled word */
	ignore_cb (dlg, word, doc);
}

static void
change_all_cb (GeditSpellCheckerDialog *dlg, const gchar *word, const gchar *change, GeditDocument *doc)
{
	CheckRange *range;
	gchar *w = NULL;

	gchar *last_searched_text;
	gchar *last_replaced_text;
	
	gedit_debug (DEBUG_PLUGINS, "");
	
	g_return_if_fail (doc != NULL);
	g_return_if_fail (word != NULL);
	g_return_if_fail (change != NULL);

	range = get_check_range (doc);
	g_return_if_fail (range != NULL);

	w = gedit_document_get_chars (doc, range->mw_start, range->mw_end);
	g_return_if_fail (w != NULL);

	if (strcmp (w, word) != 0)
	{
		g_free (w);
		return;
	}

	g_free (w);

	last_searched_text = gedit_document_get_last_searched_text (doc);
	last_replaced_text = gedit_document_get_last_replace_text (doc);
       	
	gedit_document_replace_all (doc, word, change, TRUE);

	update_current (doc, range->mw_start + g_utf8_strlen (change, -1));

	if (last_searched_text != NULL)
	{
		gedit_document_set_last_searched_text (doc, last_searched_text);

		g_free (last_searched_text);
	}

	if (last_replaced_text != NULL)
	{
		gedit_document_set_last_replace_text (doc, last_replaced_text);
	
		g_free (last_replaced_text);
	}

	/* go to next mispelled word */
	ignore_cb (dlg, word, doc);
}


static void
add_word_cb (GeditSpellCheckerDialog *dlg, const gchar *word, GeditDocument *doc)
{
	g_return_if_fail (doc != NULL);
	g_return_if_fail (word != NULL);

	/* go to next mispelled word */
	ignore_cb (dlg, word, doc);
}

	
static void
show_empty_document_dialog ()
{
	 GtkWidget *message_dlg; 
	
	 message_dlg = gtk_message_dialog_new (GTK_WINDOW (gedit_get_active_window ()),
			                       GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			                       GTK_MESSAGE_INFO,
			                       GTK_BUTTONS_OK,
					       _("The document is empty."));
	 gtk_dialog_set_default_response (GTK_DIALOG (message_dlg), GTK_RESPONSE_OK);
	 
	 gtk_dialog_run (GTK_DIALOG (message_dlg));
	 
	 gtk_widget_destroy (message_dlg);
}

static void
show_no_mispelled_words_dialog (gboolean sel)
{
	 GtkWidget *message_dlg; 
	
	 message_dlg = gtk_message_dialog_new (GTK_WINDOW (gedit_get_active_window ()),
			                       GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			                       GTK_MESSAGE_INFO,
			                       GTK_BUTTONS_OK,
					       sel ? 
					       _("The selected text does not contain mispelled words.") :
					       _("The document does not contain mispelled words."));

	 gtk_dialog_set_default_response (GTK_DIALOG (message_dlg), GTK_RESPONSE_OK);
	 
	 gtk_dialog_run (GTK_DIALOG (message_dlg));
	 
	 gtk_widget_destroy (message_dlg);
}


static void
set_language_cb (BonoboUIComponent *uic, gpointer user_data, const gchar* verbname)
{
	GeditDocument *doc;
	GeditSpellChecker *spell;

	gedit_debug (DEBUG_PLUGINS, "");
	
	doc = gedit_get_active_document ();
	g_return_if_fail (doc != NULL);

	spell = get_spell_checker_from_document (doc);
	g_return_if_fail (spell != NULL);

	gedit_spell_language_dialog_run (spell, GTK_WINDOW (gedit_get_active_window ()));
}

static void
spell_cb (BonoboUIComponent *uic, gpointer user_data, const gchar* verbname)
{
	GeditDocument *doc;
	GeditSpellChecker *spell;
	GtkWidget *dlg;
	gint start;
	gint end;
	gchar *word;
	gboolean sel = FALSE;
	
	gedit_debug (DEBUG_PLUGINS, "");
	
	doc = gedit_get_active_document ();
	g_return_if_fail (doc != NULL);

	spell = get_spell_checker_from_document (doc);
	g_return_if_fail (spell != NULL);

	if (gedit_document_get_char_count (doc) <= 0)
	{
		show_empty_document_dialog ();
		return;
	}

	if (gedit_document_get_selection (doc, &start, &end))
	{
		/* get selection points */
		set_check_range (doc, start, end);
		sel = TRUE;
	}
	else	
		set_check_range (doc, 0, -1);
	
	word = get_next_mispelled_word (doc);
	if (word == NULL)
	{
		show_no_mispelled_words_dialog (sel);
		return;
	}

	dlg = gedit_spell_checker_dialog_new_from_spell_checker (spell);
	gtk_window_set_modal (GTK_WINDOW (dlg), TRUE);
	gtk_window_set_transient_for (GTK_WINDOW (dlg), 
				      GTK_WINDOW (gedit_get_active_window ()));

	g_signal_connect (G_OBJECT (dlg), "ignore", G_CALLBACK (ignore_cb), doc);
	g_signal_connect (G_OBJECT (dlg), "ignore_all", G_CALLBACK (ignore_cb), doc);

	g_signal_connect (G_OBJECT (dlg), "change", G_CALLBACK (change_cb), doc);
	g_signal_connect (G_OBJECT (dlg), "change_all", G_CALLBACK (change_all_cb), doc);

	g_signal_connect (G_OBJECT (dlg), "add_word_to_personal", G_CALLBACK (add_word_cb), doc);

	gedit_spell_checker_dialog_set_mispelled_word (GEDIT_SPELL_CHECKER_DIALOG (dlg),
			word, -1);

	g_free (word);

	gtk_widget_show (dlg);
	
}	

G_MODULE_EXPORT GeditPluginState
update_ui (GeditPlugin *plugin, BonoboWindow *window)
{
	BonoboUIComponent *uic;
	GeditDocument *doc;
	
	gedit_debug (DEBUG_PLUGINS, "");
	
	g_return_val_if_fail (window != NULL, PLUGIN_ERROR);

	uic = gedit_get_ui_component_from_window (window);

	doc = gedit_get_active_document ();

	if ((doc == NULL) || gedit_document_is_readonly (doc))
		gedit_menus_set_verb_sensitive (uic, "/commands/" MENU_ITEM_NAME, FALSE);
	else
		gedit_menus_set_verb_sensitive (uic, "/commands/" MENU_ITEM_NAME, TRUE);

	return PLUGIN_OK;
}

G_MODULE_EXPORT GeditPluginState
destroy (GeditPlugin *plugin)
{
	gedit_debug (DEBUG_PLUGINS, "");

	return PLUGIN_OK;
}
	
G_MODULE_EXPORT GeditPluginState
activate (GeditPlugin *pd)
{
	GList *top_windows;
        gedit_debug (DEBUG_PLUGINS, "");

        top_windows = gedit_get_top_windows ();
        g_return_val_if_fail (top_windows != NULL, PLUGIN_ERROR);

        while (top_windows)
        {
		BonoboUIComponent *ui_component;

		gedit_menus_add_menu_item (BONOBO_WINDOW (top_windows->data),
				     MENU_ITEM_PATH, MENU_ITEM_NAME,
				     MENU_ITEM_LABEL, MENU_ITEM_TIP, GTK_STOCK_SPELL_CHECK,
				     spell_cb);

		ui_component = gedit_get_ui_component_from_window (
					BONOBO_WINDOW (top_windows->data));

		bonobo_ui_component_set_prop (
			ui_component, "/commands/" MENU_ITEM_NAME, "accel", "F7", NULL);

		gedit_menus_add_menu_item (BONOBO_WINDOW (top_windows->data),
				     MENU_ITEM_PATH, MENU_ITEM_NAME_LANG,
				     MENU_ITEM_LABEL_LANG, MENU_ITEM_TIP_LANG, GNOME_STOCK_BOOK_BLUE,
				     set_language_cb);

                pd->update_ui (pd, BONOBO_WINDOW (top_windows->data));

                top_windows = g_list_next (top_windows);
        }

        return PLUGIN_OK;
}

G_MODULE_EXPORT GeditPluginState
deactivate (GeditPlugin *pd)
{
	gedit_menus_remove_menu_item_all (MENU_ITEM_PATH, MENU_ITEM_NAME);
	gedit_menus_remove_menu_item_all (MENU_ITEM_PATH, MENU_ITEM_NAME_LANG);


	return PLUGIN_OK;
}

G_MODULE_EXPORT GeditPluginState
init (GeditPlugin *pd)
{
	/* initialize */
	gedit_debug (DEBUG_PLUGINS, "");
     
	pd->name = _("Spell checker");
	pd->desc = _("Checks the spelling of the current document.");
	pd->author = "Paolo Maggi <maggi@athena.polito.it>";
	pd->copyright = _("Copyright (C) 2002 - Paolo Maggi");
	
	pd->private_data = NULL;

	if (spell_checker_id == 0)
		spell_checker_id = g_quark_from_static_string ("GeditSpellCheckerID");
	
	if (check_range_id == 0)
		check_range_id = g_quark_from_static_string ("CheckRangeID");
	
	return PLUGIN_OK;
}




