/*
 * gedit-spell-plugin.c
 * 
 * Copyright (C) 2002-2005 Paolo Maggi 
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "gedit-spell-plugin.h"

#include <string.h> /* For strlen */

#include <glib/gi18n.h>
#include <glib/gutils.h>
#include <gmodule.h>

#include <libgnomeui/gnome-stock-icons.h>

#include <gedit/gedit-debug.h>
#include <gedit/gedit-metadata-manager.h>
#include <gedit/gedit-prefs-manager.h>
#include <gedit/gedit-statusbar.h>

#include "gedit-spell-checker.h"
#include "gedit-spell-checker-dialog.h"
#include "gedit-spell-language-dialog.h"
#include "gedit-automatic-spell-checker.h"

#define WINDOW_DATA_KEY "GeditSpellPluginWindowData"
#define MENU_PATH "/MenuBar/ToolsMenu/ToolsOps_1"

#define GEDIT_SPELL_PLUGIN_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), GEDIT_TYPE_SPELL_PLUGIN, GeditSpellPluginPrivate))

GEDIT_PLUGIN_REGISTER_TYPE(GeditSpellPlugin, gedit_spell_plugin)

typedef struct
{
	GtkActionGroup *action_group;
	guint           ui_id;
	guint           message_cid;
} WindowData;

static void	spell_cb	(GtkAction *action, GeditWindow *window);
static void	set_language_cb	(GtkAction *action, GeditWindow *window);
static void	auto_spell_cb	(GtkAction *action, GeditWindow *window);

/* UI actions. */
static const GtkActionEntry action_entries[] =
{
	{ "CheckSpell",
	  GTK_STOCK_SPELL_CHECK,
	  N_("_Check Spelling"),
	  "F7",
	  N_("Check the current document for incorrect spelling"),
	  G_CALLBACK (spell_cb)
	},

	{ "ConfigSpell",
	  GNOME_STOCK_BOOK_BLUE,
	  N_("Set _Language..."),
	  NULL,
	  N_("Set the language of the current document"),
	  G_CALLBACK (set_language_cb)
	}
};

static const GtkToggleActionEntry toggle_action_entries[] =
{
	{ "AutoSpell",
	  NULL,
	  N_("_Autocheck Spelling"),
	  NULL,
	  N_("Automatically spell-check the current document"),
	  G_CALLBACK (auto_spell_cb),
	  FALSE
	}
};

typedef struct _CheckRange CheckRange;

struct _CheckRange
{
	GtkTextMark *start_mark;
	GtkTextMark *end_mark;

	gint mw_start; /* misspelled word start */
	gint mw_end;   /* end */

	GtkTextMark *current_mark;
};

static GQuark spell_checker_id = 0;
static GQuark check_range_id = 0;

static void
gedit_spell_plugin_init (GeditSpellPlugin *plugin)
{
	gedit_debug_message (DEBUG_PLUGINS, "GeditSpellPlugin initializing");
}

static void
gedit_spell_plugin_finalize (GObject *object)
{
	gedit_debug_message (DEBUG_PLUGINS, "GeditSpellPlugin finalizing");

	G_OBJECT_CLASS (gedit_spell_plugin_parent_class)->finalize (object);
}

static void 
set_spell_language_cb (GeditSpellChecker   *spell,
		       const GeditLanguage *lang,
		       GeditDocument 	   *doc)
{
	gchar *uri;

	g_return_if_fail (GEDIT_IS_DOCUMENT (doc));
	g_return_if_fail (lang != NULL);

	uri = gedit_document_get_uri (doc);

	if (uri != NULL)
	{
		gchar *key;

		key = gedit_language_to_key (lang);
		g_return_if_fail (key != NULL);

		gedit_metadata_manager_set (uri, 
					    "spell-language",
					    key);

		g_free (key);
		g_free (uri);
	}
}

static GeditSpellChecker *
get_spell_checker_from_document (GeditDocument *doc)
{
	GeditSpellChecker *spell;
	gpointer data;

	gedit_debug (DEBUG_PLUGINS);

	g_return_val_if_fail (doc != NULL, NULL);

	data = g_object_get_qdata (G_OBJECT (doc), spell_checker_id);

	if (data == NULL)
	{
		gchar *uri;

		spell = gedit_spell_checker_new ();

		uri = gedit_document_get_uri (doc);

		if (uri != NULL)
		{
			gchar *key;
			const GeditLanguage *lang = NULL;

			key = gedit_metadata_manager_get (uri,
							  "spell-language");

			if (key != NULL)
			{
				lang = gedit_language_from_key (key);
				g_free (key);
			}

			if (lang != NULL)
				gedit_spell_checker_set_language (spell,
								  lang,
								  NULL);

			g_free (uri);	
		}

		g_object_set_qdata_full (G_OBJECT (doc), 
					 spell_checker_id, 
					 spell, 
					 (GDestroyNotify) g_object_unref);

		g_signal_connect (G_OBJECT (spell),
				  "set_language",
				  G_CALLBACK (set_spell_language_cb),
				  doc);
	}
	else
	{
		g_return_val_if_fail (GEDIT_IS_SPELL_CHECKER (data), NULL);
		spell = GEDIT_SPELL_CHECKER (data);
	}

	return spell;
}

static CheckRange *
get_check_range (GeditDocument *doc)
{
	CheckRange *range;

	gedit_debug (DEBUG_PLUGINS);

	g_return_val_if_fail (doc != NULL, NULL);

	range = (CheckRange *) g_object_get_qdata (G_OBJECT (doc), check_range_id);

	return range;
}

static void
update_current (GeditDocument *doc,
		gint           current)
{
	CheckRange *range;
	GtkTextIter iter;
	GtkTextIter end_iter;

	gedit_debug (DEBUG_PLUGINS);

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
set_check_range (GeditDocument *doc,
		 gint           start,
		 gint           end)
{
	CheckRange *range;
	GtkTextIter iter;
	
	gedit_debug (DEBUG_PLUGINS);

	g_return_if_fail (doc != NULL);
	g_return_if_fail (start >= 0);
	g_return_if_fail (start < gtk_text_buffer_get_char_count (GTK_TEXT_BUFFER (doc)));
	g_return_if_fail ((end >= start) || (end < 0));

	range = get_check_range (doc);

	if (range == NULL)
	{
		gedit_debug_message (DEBUG_PLUGINS, "There was not a previous check range");

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

	gtk_text_buffer_move_mark (GTK_TEXT_BUFFER (doc),
				   range->start_mark,
				   &iter);
	
	if (end < 0)
		end = gtk_text_buffer_get_char_count (GTK_TEXT_BUFFER (doc));
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

static gchar *
get_current_word (GeditDocument *doc, gint *start, gint *end)
{
	const CheckRange *range;
	GtkTextIter end_iter;
	GtkTextIter current_iter;
	gint range_end;

	gedit_debug (DEBUG_PLUGINS);

	g_return_val_if_fail (doc != NULL, NULL);
	g_return_val_if_fail (start != NULL, NULL);
	g_return_val_if_fail (end != NULL, NULL);

	range = get_check_range (doc);
	g_return_val_if_fail (range != NULL, NULL);

	gtk_text_buffer_get_iter_at_mark (GTK_TEXT_BUFFER (doc), 
			&end_iter, range->end_mark);

	range_end = gtk_text_iter_get_offset (&end_iter);

	gtk_text_buffer_get_iter_at_mark (GTK_TEXT_BUFFER (doc), 
			&current_iter, range->current_mark);

	end_iter = current_iter;

	if (!gtk_text_iter_is_end (&end_iter))
	{
		gedit_debug_message (DEBUG_PLUGINS, "Current is not end");

		gtk_text_iter_forward_word_end (&end_iter);
	}

	*start = gtk_text_iter_get_offset (&current_iter);
	*end = MIN (gtk_text_iter_get_offset (&end_iter), range_end);

	gedit_debug_message (DEBUG_PLUGINS, "Current word extends [%d, %d]", *start, *end);

	if (!(*start < *end))
		return NULL;

	return gtk_text_buffer_get_slice (GTK_TEXT_BUFFER (doc),
					  &current_iter,
					  &end_iter,
					  TRUE);
}

static gboolean
goto_next_word (GeditDocument *doc)
{
	CheckRange *range;
	GtkTextIter current_iter;
	GtkTextIter old_current_iter;
	GtkTextIter end_iter;

	gedit_debug (DEBUG_PLUGINS);

	g_return_val_if_fail (doc != NULL, FALSE);

	range = get_check_range (doc);
	g_return_val_if_fail (range != NULL, FALSE);

	gtk_text_buffer_get_iter_at_mark (GTK_TEXT_BUFFER (doc), 
					  &current_iter,
					  range->current_mark);
	gtk_text_buffer_get_end_iter (GTK_TEXT_BUFFER (doc), &end_iter);

	if (gtk_text_iter_compare (&current_iter, &end_iter) >= 0)
		return FALSE;

	old_current_iter = current_iter;

	gtk_text_iter_forward_word_ends (&current_iter, 2);
	gtk_text_iter_backward_word_start (&current_iter);

	if ((gtk_text_iter_compare (&old_current_iter, &current_iter) < 0) &&
	    (gtk_text_iter_compare (&current_iter, &end_iter) < 0))
	{
		update_current (doc, gtk_text_iter_get_offset (&current_iter));
		return TRUE;
	}

	return FALSE;
}

static gchar *
get_next_misspelled_word (GeditView *view)
{
	GeditDocument *doc;
	CheckRange *range;
	gint start, end;
	gchar *word;
	GeditSpellChecker *spell;

	g_return_val_if_fail (view != NULL, NULL);

	doc = GEDIT_DOCUMENT (gtk_text_view_get_buffer (GTK_TEXT_VIEW (view)));
	g_return_val_if_fail (doc != NULL, NULL);

	range = get_check_range (doc);
	g_return_val_if_fail (range != NULL, NULL);

	spell = get_spell_checker_from_document (doc);
	g_return_val_if_fail (spell != NULL, NULL);

	word = get_current_word (doc, &start, &end);
	if (word == NULL)
		return NULL;

	gedit_debug_message (DEBUG_PLUGINS, "Word to check: %s", word);

	while (gedit_spell_checker_check_word (spell, word, -1, NULL))
	{
		g_free (word);

		if (!goto_next_word (doc))
			return NULL;

		/* may return null if we reached the end of the selection */
		word = get_current_word (doc, &start, &end);
		if (word == NULL)
			return NULL;

		gedit_debug_message (DEBUG_PLUGINS, "Word to check: %s", word);
	}

	if (!goto_next_word (doc))
		update_current (doc, gtk_text_buffer_get_char_count (GTK_TEXT_BUFFER (doc)));

	if (word != NULL)
	{
		GtkTextIter s, e;

		range->mw_start = start;
		range->mw_end = end;

		gedit_debug_message (DEBUG_PLUGINS, "Select [%d, %d]", start, end);

		gtk_text_buffer_get_iter_at_offset (GTK_TEXT_BUFFER (doc), &s, start);
		gtk_text_buffer_get_iter_at_offset (GTK_TEXT_BUFFER (doc), &e, end);

		gtk_text_buffer_select_range (GTK_TEXT_BUFFER (doc), &s, &e);

		gedit_view_scroll_to_cursor (view);
	}
	else
	{
		range->mw_start = -1;
		range->mw_end = -1;
	}

	return word;
}

static void
ignore_cb (GeditSpellCheckerDialog *dlg,
	   const gchar             *w,
	   GeditView               *view)
{
	gchar *word = NULL;

	gedit_debug (DEBUG_PLUGINS);

	g_return_if_fail (w != NULL);
	g_return_if_fail (view != NULL);

	word = get_next_misspelled_word (view);
	if (word == NULL)
	{
		gedit_spell_checker_dialog_set_completed (dlg);
		
		return;
	}

	gedit_spell_checker_dialog_set_misspelled_word (GEDIT_SPELL_CHECKER_DIALOG (dlg),
							word,
							-1);

	g_free (word);
}

static void
change_cb (GeditSpellCheckerDialog *dlg,
	   const gchar             *word,
	   const gchar             *change,
	   GeditView               *view)
{
	GeditDocument *doc;
	CheckRange *range;
	gchar *w = NULL;
	GtkTextIter start, end;

	gedit_debug (DEBUG_PLUGINS);

	g_return_if_fail (view != NULL);
	g_return_if_fail (word != NULL);
	g_return_if_fail (change != NULL);

	doc = GEDIT_DOCUMENT (gtk_text_view_get_buffer (GTK_TEXT_VIEW (view)));
	g_return_if_fail (doc != NULL);

	range = get_check_range (doc);
	g_return_if_fail (range != NULL);

	gtk_text_buffer_get_iter_at_offset (GTK_TEXT_BUFFER (doc), &start, range->mw_start);
	if (range->mw_end < 0)
		gtk_text_buffer_get_end_iter (GTK_TEXT_BUFFER (doc), &end);
	else
		gtk_text_buffer_get_iter_at_offset (GTK_TEXT_BUFFER (doc), &end, range->mw_end);

	w = gtk_text_buffer_get_slice (GTK_TEXT_BUFFER (doc), &start, &end, TRUE);
	g_return_if_fail (w != NULL);

	if (strcmp (w, word) != 0)
	{
		g_free (w);
		return;
	}

	g_free (w);

	gtk_text_buffer_begin_user_action (GTK_TEXT_BUFFER(doc));

	gtk_text_buffer_delete (GTK_TEXT_BUFFER (doc), &start, &end);
	gtk_text_buffer_insert (GTK_TEXT_BUFFER (doc), &start, change, -1);

	gtk_text_buffer_end_user_action (GTK_TEXT_BUFFER(doc));

	update_current (doc, range->mw_start + g_utf8_strlen (change, -1));

	/* go to next misspelled word */
	ignore_cb (dlg, word, view);
}

static void
change_all_cb (GeditSpellCheckerDialog *dlg,
	       const gchar             *word,
	       const gchar             *change,
	       GeditView               *view)
{
	GeditDocument *doc;
	CheckRange *range;
	gchar *w = NULL;
	GtkTextIter start, end;
	gint flags = 0;

	gedit_debug (DEBUG_PLUGINS);

	g_return_if_fail (view != NULL);
	g_return_if_fail (word != NULL);
	g_return_if_fail (change != NULL);

	doc = GEDIT_DOCUMENT (gtk_text_view_get_buffer (GTK_TEXT_VIEW (view)));
	g_return_if_fail (doc != NULL);

	range = get_check_range (doc);
	g_return_if_fail (range != NULL);

	gtk_text_buffer_get_iter_at_offset (GTK_TEXT_BUFFER (doc), &start, range->mw_start);
	if (range->mw_end < 0)
		gtk_text_buffer_get_end_iter (GTK_TEXT_BUFFER (doc), &end);
	else
		gtk_text_buffer_get_iter_at_offset (GTK_TEXT_BUFFER (doc), &end, range->mw_end);

	w = gtk_text_buffer_get_slice (GTK_TEXT_BUFFER (doc), &start, &end, TRUE);
	g_return_if_fail (w != NULL);

	if (strcmp (w, word) != 0)
	{
		g_free (w);
		return;
	}

	g_free (w);

	GEDIT_SEARCH_SET_CASE_SENSITIVE (flags, TRUE);
	GEDIT_SEARCH_SET_ENTIRE_WORD (flags, TRUE);

	/* CHECK: currently this function does escaping etc */
	gedit_document_replace_all (doc, word, change, flags);

	update_current (doc, range->mw_start + g_utf8_strlen (change, -1));

	/* go to next misspelled word */
	ignore_cb (dlg, word, view);
}

static void
add_word_cb (GeditSpellCheckerDialog *dlg,
	     const gchar             *word,
	     GeditView               *view)
{
	g_return_if_fail (view != NULL);
	g_return_if_fail (word != NULL);

	/* go to next misspelled word */
	ignore_cb (dlg, word, view);
}

static void
set_language_cb (GtkAction   *action,
		 GeditWindow *window)
{
	GeditDocument *doc;
	GeditSpellChecker *spell;

	gedit_debug (DEBUG_PLUGINS);

	doc = gedit_window_get_active_document (window);
	g_return_if_fail (doc != NULL);

	spell = get_spell_checker_from_document (doc);
	g_return_if_fail (spell != NULL);

	gedit_spell_language_dialog_run (spell, GTK_WINDOW (window));
}

static void
spell_cb (GtkAction   *action,
	  GeditWindow *window)
{
	GeditView *view;
	GeditDocument *doc;
	GeditSpellChecker *spell;
	GtkWidget *dlg;
	gint start, end;
	GtkTextIter sel_start, sel_end;
	gchar *word;
	gboolean sel = FALSE;

	gedit_debug (DEBUG_PLUGINS);

	view = gedit_window_get_active_view (window);
	g_return_if_fail (view != NULL);

	doc = GEDIT_DOCUMENT (gtk_text_view_get_buffer (GTK_TEXT_VIEW (view)));
	g_return_if_fail (doc != NULL);

	spell = get_spell_checker_from_document (doc);
	g_return_if_fail (spell != NULL);

	if (gtk_text_buffer_get_char_count (GTK_TEXT_BUFFER (doc)) <= 0)
	{
		WindowData *data;
		GtkWidget *statusbar;

		data = (WindowData *) g_object_get_data (G_OBJECT (window), WINDOW_DATA_KEY);
		g_return_if_fail (data != NULL);

		statusbar = gedit_window_get_statusbar (window);
		gedit_statusbar_flash_message (GEDIT_STATUSBAR (statusbar),
					       data->message_cid,
					       _("The document is empty."));

		return;
	}

	if (gtk_text_buffer_get_selection_bounds (GTK_TEXT_BUFFER (doc),
						  &sel_start,
						  &sel_end))
	{
		/* get selection points */
		start = gtk_text_iter_get_offset (&sel_start);
		end = gtk_text_iter_get_offset (&sel_end);
		set_check_range (doc, start, end);
		sel = TRUE;
	}
	else
	{
		set_check_range (doc, 0, -1);
	}

	word = get_next_misspelled_word (view);
	if (word == NULL)
	{
		WindowData *data;
		GtkWidget *statusbar;

		data = (WindowData *) g_object_get_data (G_OBJECT (window), WINDOW_DATA_KEY);
		g_return_if_fail (data != NULL);

		statusbar = gedit_window_get_statusbar (window);
		gedit_statusbar_flash_message (GEDIT_STATUSBAR (statusbar),
					       data->message_cid,
					       _("No misspelled words"));

		return;
	}

	dlg = gedit_spell_checker_dialog_new_from_spell_checker (spell);
	gtk_window_set_modal (GTK_WINDOW (dlg), TRUE);
	gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (window));

	g_signal_connect (dlg, "ignore", G_CALLBACK (ignore_cb), view);
	g_signal_connect (dlg, "ignore_all", G_CALLBACK (ignore_cb), view);

	g_signal_connect (dlg, "change", G_CALLBACK (change_cb), view);
	g_signal_connect (dlg, "change_all", G_CALLBACK (change_all_cb), view);

	g_signal_connect (dlg, "add_word_to_personal", G_CALLBACK (add_word_cb), view);

	gedit_spell_checker_dialog_set_misspelled_word (GEDIT_SPELL_CHECKER_DIALOG (dlg),
							word,
							-1);

	g_free (word);

	gtk_widget_show (dlg);
}

static void
auto_spell_cb (GtkAction   *action,
	       GeditWindow *window)
{
	GeditAutomaticSpellChecker *autospell;
	GeditDocument *doc;
	GeditSpellChecker *spell;
	gboolean active;

	gedit_debug (DEBUG_PLUGINS);

	active = gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action));

	gedit_debug_message (DEBUG_PLUGINS, active ? "Auto Spell activated" : "Auto Spell deactivated");

	doc = gedit_window_get_active_document (window);
	if (doc == NULL)
		return;

	spell = get_spell_checker_from_document (doc);
	g_return_if_fail (spell != NULL);

	autospell = gedit_automatic_spell_checker_get_from_document (doc);

	if (active)
	{
		if (autospell == NULL)
		{
			GeditView *active_view;

			active_view = gedit_window_get_active_view (window);
			g_return_if_fail (active_view != NULL);

			autospell = gedit_automatic_spell_checker_new (doc, spell);
			gedit_automatic_spell_checker_attach_view (autospell, active_view);
			gedit_automatic_spell_checker_recheck_all (autospell);
		}
	}
	else
	{
		if (autospell != NULL)
			gedit_automatic_spell_checker_free (autospell);
	}
}

static void
free_window_data (WindowData *data)
{
	g_return_if_fail (data != NULL);

	g_object_unref (data->action_group);
	g_free (data);
}

static void
update_ui_real (GeditWindow *window,
		WindowData *data)
{
	GeditView *view;
	
	gedit_debug (DEBUG_PLUGINS);

	view = gedit_window_get_active_view (window);

	// TODO, see old plugin

	gtk_action_group_set_sensitive (data->action_group,
					(view != NULL) &&
					gtk_text_view_get_editable (GTK_TEXT_VIEW (view)));
}

static void
impl_activate (GeditPlugin *plugin,
	       GeditWindow *window)
{
	GtkUIManager *manager;
	WindowData *data;

	gedit_debug (DEBUG_PLUGINS);

	data = g_new (WindowData, 1);

	manager = gedit_window_get_ui_manager (window);

	data->action_group = gtk_action_group_new ("GeditSpellPluginActions");
	gtk_action_group_set_translation_domain (data->action_group, 
						 GETTEXT_PACKAGE);
	gtk_action_group_add_actions (data->action_group, 
				      action_entries,
				      G_N_ELEMENTS (action_entries), 
				      window);
	gtk_action_group_add_toggle_actions (data->action_group, 
					     toggle_action_entries,
					     G_N_ELEMENTS (toggle_action_entries),
					     window);

	gtk_ui_manager_insert_action_group (manager, data->action_group, -1);

	data->ui_id = gtk_ui_manager_new_merge_id (manager);

	data->message_cid = gtk_statusbar_get_context_id
			(GTK_STATUSBAR (gedit_window_get_statusbar (window)), 
			 "spell_plugin_message");

	g_object_set_data_full (G_OBJECT (window),
				WINDOW_DATA_KEY, 
				data,
				(GDestroyNotify) free_window_data);

	gtk_ui_manager_add_ui (manager,
			       data->ui_id,
			       MENU_PATH,
			       "CheckSpell",
			       "CheckSpell",
			       GTK_UI_MANAGER_MENUITEM, 
			       FALSE);

	gtk_ui_manager_add_ui (manager, 
			       data->ui_id, 
			       MENU_PATH,
			       "AutoSpell", 
			       "AutoSpell",
			       GTK_UI_MANAGER_MENUITEM, 
			       FALSE);

	gtk_ui_manager_add_ui (manager, 
			       data->ui_id, 
			       MENU_PATH,
			       "ConfigSpell", 
			       "ConfigSpell",
			       GTK_UI_MANAGER_MENUITEM, 
			       FALSE);

	

	update_ui_real (window, data);
}

static void
impl_deactivate	(GeditPlugin *plugin,
		 GeditWindow *window)
{
	GtkUIManager *manager;
	WindowData *data;

	gedit_debug (DEBUG_PLUGINS);

	manager = gedit_window_get_ui_manager (window);

	data = (WindowData *) g_object_get_data (G_OBJECT (window), WINDOW_DATA_KEY);
	g_return_if_fail (data != NULL);

	gtk_ui_manager_remove_ui (manager, data->ui_id);
	gtk_ui_manager_remove_action_group (manager, data->action_group);

	g_object_set_data (G_OBJECT (window), WINDOW_DATA_KEY, NULL);
}

static void
impl_update_ui (GeditPlugin *plugin,
		GeditWindow *window)
{
	WindowData *data;

	gedit_debug (DEBUG_PLUGINS);

	data = (WindowData *) g_object_get_data (G_OBJECT (window), WINDOW_DATA_KEY);
	g_return_if_fail (data != NULL);

	update_ui_real (window, data);
}

static void
gedit_spell_plugin_class_init (GeditSpellPluginClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GeditPluginClass *plugin_class = GEDIT_PLUGIN_CLASS (klass);

	object_class->finalize = gedit_spell_plugin_finalize;

	plugin_class->activate = impl_activate;
	plugin_class->deactivate = impl_deactivate;
	plugin_class->update_ui = impl_update_ui;

	if (spell_checker_id == 0)
		spell_checker_id = g_quark_from_static_string ("GeditSpellCheckerID");
	
	if (check_range_id == 0)
		check_range_id = g_quark_from_static_string ("CheckRangeID");
}
