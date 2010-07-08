/*
 * gedit-settings.c
 * This file is part of gedit
 *
 * Copyright (C) 2002-2005 - Paolo Maggi
 *               2009 - Ignacio Casal Quinteiro
 *
 * gedit is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * gedit is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with gedit; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */

#include <string.h>

#include "gedit-settings.h"
#include "gedit-app.h"
#include "gedit-debug.h"
#include "gedit-view-interface.h"
#include "gedit-window.h"
#include "gedit-plugins-engine.h"
#include "gedit-style-scheme-manager.h"
#include "gedit-dirs.h"
#include "gedit-utils.h"
#include "gedit-window-private.h"

#define GEDIT_SETTINGS_LOCKDOWN_COMMAND_LINE "disable-command-line"
#define GEDIT_SETTINGS_LOCKDOWN_PRINTING "disable-printing"
#define GEDIT_SETTINGS_LOCKDOWN_PRINT_SETUP "disable-print-setup"
#define GEDIT_SETTINGS_LOCKDOWN_SAVE_TO_DISK "disable-save-to-disk"

#define GEDIT_SETTINGS_SYSTEM_FONT "monospace-font-name"

#define GEDIT_SETTINGS_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE((object), GEDIT_TYPE_SETTINGS, GeditSettingsPrivate))

struct _GeditSettingsPrivate
{
	GSettings *lockdown;
	GSettings *interface;
	GSettings *editor;
	GSettings *ui;
	GSettings *plugins;
	
	gchar *old_scheme;
};

G_DEFINE_TYPE (GeditSettings, gedit_settings, G_TYPE_SETTINGS)

static void
gedit_settings_finalize (GObject *object)
{
	GeditSettings *gs = GEDIT_SETTINGS (object);

	g_free (gs->priv->old_scheme);

	G_OBJECT_CLASS (gedit_settings_parent_class)->finalize (object);
}

static void
gedit_settings_dispose (GObject *object)
{
	GeditSettings *gs = GEDIT_SETTINGS (object);
	
	if (gs->priv->lockdown != NULL)
	{
		g_object_unref (gs->priv->lockdown);
		gs->priv->lockdown = NULL;
	}
	
	if (gs->priv->interface != NULL)
	{
		g_object_unref (gs->priv->interface);
		gs->priv->interface = NULL;
	}
	
	if (gs->priv->editor != NULL)
	{
		g_object_unref (gs->priv->editor);
		gs->priv->editor = NULL;
	}
	
	if (gs->priv->ui != NULL)
	{
		g_object_unref (gs->priv->ui);
		gs->priv->ui = NULL;
	}
	
	if (gs->priv->plugins != NULL)
	{
		g_object_unref (gs->priv->plugins);
		gs->priv->plugins = NULL;
	}

	G_OBJECT_CLASS (gedit_settings_parent_class)->dispose (object);
}

static void
on_lockdown_changed (GSettings   *settings,
		     const gchar *key,
		     gpointer     useless)
{
	gboolean locked;
	GeditApp *app;
	
	locked = g_settings_get_boolean (settings, key);
	app = gedit_app_get_default ();
	
	if (strcmp (key, GEDIT_SETTINGS_LOCKDOWN_COMMAND_LINE) == 0)
	{
		_gedit_app_set_lockdown_bit (app, 
					     GEDIT_LOCKDOWN_COMMAND_LINE,
					     locked);
	}
	else if (strcmp (key, GEDIT_SETTINGS_LOCKDOWN_PRINTING) == 0)
	{
		_gedit_app_set_lockdown_bit (app, 
					     GEDIT_LOCKDOWN_PRINTING,
					     locked);
	}
	else if (strcmp (key, GEDIT_SETTINGS_LOCKDOWN_PRINT_SETUP) == 0)
	{
		_gedit_app_set_lockdown_bit (app, 
					     GEDIT_LOCKDOWN_PRINT_SETUP,
					     locked);
	}
	else if (strcmp (key, GEDIT_SETTINGS_LOCKDOWN_SAVE_TO_DISK) == 0)
	{
		_gedit_app_set_lockdown_bit (app, 
					     GEDIT_LOCKDOWN_SAVE_TO_DISK,
					     locked);
	}
}

static void
set_font (GeditSettings *gs,
	  const gchar *font)
{
	GList *views, *l;
	guint ts;
	
	g_settings_get (gs->priv->editor, GEDIT_SETTINGS_TABS_SIZE, "u", &ts);
	
	views = gedit_app_get_views (gedit_app_get_default ());
	
	for (l = views; l != NULL; l = g_list_next (l))
	{
		/* Note: we use def=FALSE to avoid GeditView to query gconf */
		gedit_view_set_font (GEDIT_VIEW (l->data), FALSE, font);

		gtk_source_view_set_tab_width (GTK_SOURCE_VIEW (l->data), ts);
	}
	
	g_list_free (views);
}

static void
on_system_font_changed (GSettings     *settings,
			const gchar   *key,
			GeditSettings *gs)
{
	
	gboolean use_default_font;
	gchar *font;
	
	use_default_font = g_settings_get_boolean (gs->priv->editor,
						   GEDIT_SETTINGS_USE_DEFAULT_FONT);
	if (!use_default_font)
		return;
	
	font = g_settings_get_string (settings, key);
	
	set_font (gs, font);
	
	g_free (font);
}

static void
on_use_default_font_changed (GSettings     *settings,
			     const gchar   *key,
			     GeditSettings *gs)
{
	gboolean def;
	gchar *font;

	def = g_settings_get_boolean (settings, key);
	
	if (def)
	{
		font = g_settings_get_string (gs->priv->interface,
					      GEDIT_SETTINGS_SYSTEM_FONT);
	}
	else
	{
		font = g_settings_get_string (gs->priv->editor,
					      GEDIT_SETTINGS_EDITOR_FONT);
	}
	
	set_font (gs, font);
	
	g_free (font);
}

static void
on_editor_font_changed (GSettings     *settings,
			const gchar   *key,
			GeditSettings *gs)
{
	gboolean use_default_font;
	gchar *font;
	
	use_default_font = g_settings_get_boolean (gs->priv->editor,
						   GEDIT_SETTINGS_USE_DEFAULT_FONT);
	if (use_default_font)
		return;
	
	font = g_settings_get_string (settings, key);
	
	set_font (gs, font);
	
	g_free (font);
}

static void
on_scheme_changed (GSettings     *settings,
		   const gchar   *key,
		   GeditSettings *gs)
{
	GtkSourceStyleScheme *style;
	gchar *scheme;
	GList *docs;
	GList *l;

	scheme = g_settings_get_string (settings, key);

	if (gs->priv->old_scheme != NULL && (strcmp (scheme, gs->priv->old_scheme) == 0))
		return;

	g_free (gs->priv->old_scheme);
	gs->priv->old_scheme = scheme;

	style = gtk_source_style_scheme_manager_get_scheme (
			gedit_get_style_scheme_manager (),
			scheme);

	if (style == NULL)
	{
		g_warning ("Default style scheme '%s' not found, falling back to 'classic'", scheme);
		
		style = gtk_source_style_scheme_manager_get_scheme (
			gedit_get_style_scheme_manager (),
			"classic");

		if (style == NULL) 
		{
			g_warning ("Style scheme 'classic' cannot be found, check your GtkSourceView installation.");
			return;
		}
	}

	docs = gedit_app_get_documents (gedit_app_get_default ());
	for (l = docs; l != NULL; l = g_list_next (l))
	{
		g_return_if_fail (GTK_IS_SOURCE_BUFFER (l->data));

		gtk_source_buffer_set_style_scheme (GTK_SOURCE_BUFFER (l->data),
						    style);
	}

	g_list_free (docs);
}

static void
on_auto_save_changed (GSettings     *settings,
		      const gchar   *key,
		      GeditSettings *gs)
{
	GList *docs, *l;
	gboolean auto_save;
	
	auto_save = g_settings_get_boolean (settings, key);
	
	docs = gedit_app_get_documents (gedit_app_get_default ());
	
	for (l = docs; l != NULL; l = g_list_next (l))
	{
		GeditTab *tab = gedit_tab_get_from_document (GEDIT_DOCUMENT (l->data));
		
		gedit_tab_set_auto_save_enabled (tab, auto_save);
	}
	
	g_list_free (docs);
}

static void
on_auto_save_interval_changed (GSettings     *settings,
			       const gchar   *key,
			       GeditSettings *gs)
{
	GList *docs, *l;
	gint auto_save_interval;
	
	g_settings_get (settings, key, "u", &auto_save_interval);
	
	docs = gedit_app_get_documents (gedit_app_get_default ());
	
	for (l = docs; l != NULL; l = g_list_next (l))
	{
		GeditTab *tab = gedit_tab_get_from_document (GEDIT_DOCUMENT (l->data));
		
		gedit_tab_set_auto_save_interval (tab, auto_save_interval);
	}
	
	g_list_free (docs);
}

static void
on_undo_actions_limit_changed (GSettings     *settings,
			       const gchar   *key,
			       GeditSettings *gs)
{
	GList *docs, *l;
	gint ul;
	
	ul = g_settings_get_int (settings, key);
	
	ul = CLAMP (ul, -1, 250);
	
	docs = gedit_app_get_documents (gedit_app_get_default ());
	
	for (l = docs; l != NULL; l = g_list_next (l))
	{
		gtk_source_buffer_set_max_undo_levels (GTK_SOURCE_BUFFER (l->data),
						       ul);
	}
	
	g_list_free (docs);
}

static void
on_wrap_mode_changed (GSettings     *settings,
		      const gchar   *key,
		      GeditSettings *gs)
{
	GtkWrapMode wrap_mode;
	GList *views, *l;

	wrap_mode = g_settings_get_enum (settings, key);

	views = gedit_app_get_views (gedit_app_get_default ());
	
	for (l = views; l != NULL; l = g_list_next (l))
	{
		GeditView *view = GEDIT_VIEW (l->data);

		if (GTK_IS_TEXT_VIEW (view))
		{
			gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (view),
						     wrap_mode);
		}
	}
	
	g_list_free (views);
}

static void
on_tabs_size_changed (GSettings     *settings,
		      const gchar   *key,
		      GeditSettings *gs)
{
	GList *views, *l;
	guint ts;
	
	g_settings_get (settings, key, "u", &ts);
	
	ts = CLAMP (ts, 1, 24);
	
	views = gedit_app_get_views (gedit_app_get_default ());
	
	for (l = views; l != NULL; l = g_list_next (l))
	{
		GeditView *view = GEDIT_VIEW (l->data);

		if (GTK_IS_SOURCE_VIEW (view))
		{
			gtk_source_view_set_tab_width (GTK_SOURCE_VIEW (view),
						       ts);
		}
	}
	
	g_list_free (views);
}

/* FIXME: insert_spaces and line_numbers are mostly the same it just changes
 the func called, maybe typedef the func and refactorize? */
static void
on_insert_spaces_changed (GSettings     *settings,
			  const gchar   *key,
			  GeditSettings *gs)
{
	GList *views, *l;
	gboolean spaces;
	
	spaces = g_settings_get_boolean (settings, key);
	
	views = gedit_app_get_views (gedit_app_get_default ());
	
	for (l = views; l != NULL; l = g_list_next (l))
	{
		GeditView *view = GEDIT_VIEW (l->data);

		if (GTK_IS_SOURCE_VIEW (view))
		{
			gtk_source_view_set_insert_spaces_instead_of_tabs (
						GTK_SOURCE_VIEW (view),
						spaces);
		}
	}
	
	g_list_free (views);
}

static void
on_auto_indent_changed (GSettings     *settings,
			const gchar   *key,
			GeditSettings *gs)
{
	GList *views, *l;
	gboolean enable;
	
	enable = g_settings_get_boolean (settings, key);
	
	views = gedit_app_get_views (gedit_app_get_default ());
	
	for (l = views; l != NULL; l = g_list_next (l))
	{
		GeditView *view = GEDIT_VIEW (l->data);

		if (GTK_IS_SOURCE_VIEW (view))
		{
			gtk_source_view_set_auto_indent (GTK_SOURCE_VIEW (view),
							 enable);
		}
	}
	
	g_list_free (views);
}

static void
on_display_line_numbers_changed (GSettings     *settings,
				 const gchar   *key,
				 GeditSettings *gs)
{
	GList *views, *l;
	gboolean line_numbers;
	
	line_numbers = g_settings_get_boolean (settings, key);
	
	views = gedit_app_get_views (gedit_app_get_default ());
	
	for (l = views; l != NULL; l = g_list_next (l))
	{
		GeditView *view = GEDIT_VIEW (l->data);

		if (GTK_IS_SOURCE_VIEW (view))
		{
			gtk_source_view_set_show_line_numbers (GTK_SOURCE_VIEW (view),
							       line_numbers);
		}
	}
	
	g_list_free (views);
}

static void
on_hl_current_line_changed (GSettings     *settings,
			    const gchar   *key,
			    GeditSettings *gs)
{
	GList *views, *l;
	gboolean hl;
	
	hl = g_settings_get_boolean (settings, key);
	
	views = gedit_app_get_views (gedit_app_get_default ());
	
	for (l = views; l != NULL; l = g_list_next (l))
	{
		GeditView *view = GEDIT_VIEW (l->data);

		if (GTK_IS_SOURCE_VIEW (view))
		{
			gtk_source_view_set_highlight_current_line (GTK_SOURCE_VIEW (view),
								    hl);
		}
	}
	
	g_list_free (views);
}

static void
on_bracket_matching_changed (GSettings     *settings,
			     const gchar   *key,
			     GeditSettings *gs)
{
	GList *docs, *l;
	gboolean enable;
	
	enable = g_settings_get_boolean (settings, key);
	
	docs = gedit_app_get_documents (gedit_app_get_default ());
	
	for (l = docs; l != NULL; l = g_list_next (l))
	{
		gtk_source_buffer_set_highlight_matching_brackets (GTK_SOURCE_BUFFER (l->data),
								   enable);
	}
	
	g_list_free (docs);
}

static void
on_display_right_margin_changed (GSettings     *settings,
				 const gchar   *key,
				 GeditSettings *gs)
{
	GList *views, *l;
	gboolean display;
	
	display = g_settings_get_boolean (settings, key);
	
	views = gedit_app_get_views (gedit_app_get_default ());
	
	for (l = views; l != NULL; l = g_list_next (l))
	{
		GeditView *view = GEDIT_VIEW (l->data);

		if (GTK_IS_SOURCE_VIEW (view))
		{
			gtk_source_view_set_show_right_margin (GTK_SOURCE_VIEW (view),
							       display);
		}
	}
	
	g_list_free (views);
}

static void
on_right_margin_position_changed (GSettings     *settings,
				  const gchar   *key,
				  GeditSettings *gs)
{
	GList *views, *l;
	gint pos;
	
	g_settings_get (settings, key, "u", &pos);
	
	pos = CLAMP (pos, 1, 160);
	
	views = gedit_app_get_views (gedit_app_get_default ());
	
	for (l = views; l != NULL; l = g_list_next (l))
	{
		GeditView *view = GEDIT_VIEW (l->data);

		if (GTK_IS_SOURCE_VIEW (view))
		{
			gtk_source_view_set_right_margin_position (GTK_SOURCE_VIEW (view),
								   pos);
		}
	}
	
	g_list_free (views);
}

static void
on_smart_home_end_changed (GSettings     *settings,
			   const gchar   *key,
			   GeditSettings *gs)
{
	GtkSourceSmartHomeEndType smart_he;
	GList *views, *l;

	smart_he = g_settings_get_enum (settings, key);

	views = gedit_app_get_views (gedit_app_get_default ());
	
	for (l = views; l != NULL; l = g_list_next (l))
	{
		GeditView *view = GEDIT_VIEW (l->data);

		if (GTK_IS_SOURCE_VIEW (view))
		{
			gtk_source_view_set_smart_home_end (GTK_SOURCE_VIEW (view),
							    smart_he);
		}
	}
	
	g_list_free (views);
}

static void
on_syntax_highlighting_changed (GSettings     *settings,
				const gchar   *key,
				GeditSettings *gs)
{
	const GList *windows;
	GList *docs, *l;
	gboolean enable;
	
	enable = g_settings_get_boolean (settings, key);
	
	docs = gedit_app_get_documents (gedit_app_get_default ());
	
	for (l = docs; l != NULL; l = g_list_next (l))
	{
		gtk_source_buffer_set_highlight_syntax (GTK_SOURCE_BUFFER (l->data),
							enable);
	}
	
	g_list_free (docs);

	/* update the sensitivity of the Higlight Mode menu item */
	windows = gedit_app_get_windows (gedit_app_get_default ());
	while (windows != NULL)
	{
		GtkUIManager *ui;
		GtkAction *a;

		ui = gedit_window_get_ui_manager (GEDIT_WINDOW (windows->data));

		a = gtk_ui_manager_get_action (ui,
					       "/MenuBar/ViewMenu/ViewHighlightModeMenu");

		gtk_action_set_sensitive (a, enable);

		windows = g_list_next (windows);
	}
}

static void
on_search_highlighting_changed (GSettings     *settings,
				const gchar   *key,
				GeditSettings *gs)
{
	GList *docs, *l;
	gboolean enable;
	
	enable = g_settings_get_boolean (settings, key);
	
	docs = gedit_app_get_documents (gedit_app_get_default ());
	
	for (l = docs; l != NULL; l = g_list_next (l))
	{
		gedit_document_set_enable_search_highlighting  (GEDIT_DOCUMENT (l->data),
								enable);
	}
	
	g_list_free (docs);
}

static void
on_max_recents_changed (GSettings     *settings,
			const gchar   *key,
			GeditSettings *gs)
{
	const GList *windows;
	gint max;
	
	g_settings_get (settings, key, "u", &max);
	
	windows = gedit_app_get_windows (gedit_app_get_default ());
	while (windows != NULL)
	{
		GeditWindow *w = GEDIT_WINDOW (windows->data);

		gtk_recent_chooser_set_limit (GTK_RECENT_CHOOSER (w->priv->toolbar_recent_menu),
					      max);

		windows = g_list_next (windows);
	}
	
	/* FIXME: we have no way at the moment to trigger the
	 * update of the inline recents in the File menu */
}

static void
gedit_settings_init (GeditSettings *gs)
{
	gs->priv = GEDIT_SETTINGS_GET_PRIVATE (gs);
	
	gs->priv->old_scheme = NULL;
}

static void
initialize (GeditSettings *gs)
{
	GSettings *prefs;
	
	prefs = g_settings_get_child (G_SETTINGS (gs), "preferences");
	gs->priv->editor = g_settings_get_child (prefs, "editor");
	gs->priv->ui = g_settings_get_child (prefs, "ui");
	g_object_unref (prefs);
	gs->priv->plugins = g_settings_get_child (G_SETTINGS (gs), "plugins");
	
	/* Load settings */
	gs->priv->lockdown = g_settings_new ("org.gnome.desktop.lockdown");
	
	g_signal_connect (gs->priv->lockdown,
			  "changed",
			  G_CALLBACK (on_lockdown_changed),
			  NULL);

	gs->priv->interface = g_settings_new ("org.gnome.desktop.interface");
	
	g_signal_connect (gs->priv->interface,
			  "changed::monospace-font-name",
			  G_CALLBACK (on_system_font_changed),
			  gs);

	/* editor changes */
	g_signal_connect (gs->priv->editor,
			  "changed::use-default-font",
			  G_CALLBACK (on_use_default_font_changed),
			  gs);
	g_signal_connect (gs->priv->editor,
			  "changed::editor-font",
			  G_CALLBACK (on_editor_font_changed),
			  gs);
	g_signal_connect (gs->priv->editor,
			  "changed::scheme",
			  G_CALLBACK (on_scheme_changed),
			  gs);
	g_signal_connect (gs->priv->editor,
			  "changed::auto-save",
			  G_CALLBACK (on_auto_save_changed),
			  gs);
	g_signal_connect (gs->priv->editor,
			  "changed::auto-save-interval",
			  G_CALLBACK (on_auto_save_interval_changed),
			  gs);
	g_signal_connect (gs->priv->editor,
			  "changed::undo-actions-limit",
			  G_CALLBACK (on_undo_actions_limit_changed),
			  gs);
	g_signal_connect (gs->priv->editor,
			  "changed::wrap-mode",
			  G_CALLBACK (on_wrap_mode_changed),
			  gs);
	g_signal_connect (gs->priv->editor,
			  "changed::tabs-size",
			  G_CALLBACK (on_tabs_size_changed),
			  gs);
	g_signal_connect (gs->priv->editor,
			  "changed::insert-spaces",
			  G_CALLBACK (on_insert_spaces_changed),
			  gs);
	g_signal_connect (gs->priv->editor,
			  "changed::auto-indent",
			  G_CALLBACK (on_auto_indent_changed),
			  gs);
	g_signal_connect (gs->priv->editor,
			  "changed::display-line-numbers",
			  G_CALLBACK (on_display_line_numbers_changed),
			  gs);
	g_signal_connect (gs->priv->editor,
			  "changed::highlight-current-line",
			  G_CALLBACK (on_hl_current_line_changed),
			  gs);
	g_signal_connect (gs->priv->editor,
			  "changed::bracket-matching",
			  G_CALLBACK (on_bracket_matching_changed),
			  gs);
	g_signal_connect (gs->priv->editor,
			  "changed::display-right-margin",
			  G_CALLBACK (on_display_right_margin_changed),
			  gs);
	g_signal_connect (gs->priv->editor,
			  "changed::right-margin-position",
			  G_CALLBACK (on_right_margin_position_changed),
			  gs);
	g_signal_connect (gs->priv->editor,
			  "changed::smart-home-end",
			  G_CALLBACK (on_smart_home_end_changed),
			  gs);
	g_signal_connect (gs->priv->editor,
			  "changed::syntax-highlighting",
			  G_CALLBACK (on_syntax_highlighting_changed),
			  gs);
	g_signal_connect (gs->priv->editor,
			  "changed::search-highlighting",
			  G_CALLBACK (on_search_highlighting_changed),
			  gs);

	/* ui changes */
	g_signal_connect (gs->priv->ui,
			  "changed::max-recents",
			  G_CALLBACK (on_max_recents_changed),
			  gs);
}

static void
gedit_settings_class_init (GeditSettingsClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = gedit_settings_finalize;
	object_class->dispose = gedit_settings_dispose;

	g_type_class_add_private (object_class, sizeof (GeditSettingsPrivate));
}

GSettings *
gedit_settings_new ()
{
	GeditSettings *settings;

	settings = g_object_new (GEDIT_TYPE_SETTINGS,
				 "schema", "org.gnome.gedit",
				 NULL);

	initialize (settings);

	return G_SETTINGS (settings);
}

GeditLockdownMask
gedit_settings_get_lockdown (GeditSettings *gs)
{
	guint lockdown = 0;
	gboolean command_line, printing, print_setup, save_to_disk;
	
	command_line = g_settings_get_boolean (gs->priv->lockdown,
					       GEDIT_SETTINGS_LOCKDOWN_COMMAND_LINE);
	printing = g_settings_get_boolean (gs->priv->lockdown,
					   GEDIT_SETTINGS_LOCKDOWN_PRINTING);
	print_setup = g_settings_get_boolean (gs->priv->lockdown,
					      GEDIT_SETTINGS_LOCKDOWN_PRINT_SETUP);
	save_to_disk = g_settings_get_boolean (gs->priv->lockdown,
					       GEDIT_SETTINGS_LOCKDOWN_SAVE_TO_DISK);

	if (command_line)
		lockdown |= GEDIT_LOCKDOWN_COMMAND_LINE;

	if (printing)
		lockdown |= GEDIT_LOCKDOWN_PRINTING;

	if (print_setup)
		lockdown |= GEDIT_LOCKDOWN_PRINT_SETUP;

	if (save_to_disk)
		lockdown |= GEDIT_LOCKDOWN_SAVE_TO_DISK;

	return lockdown;
}

gchar *
gedit_settings_get_system_font (GeditSettings *gs)
{
	gchar *system_font;

	g_return_val_if_fail (GEDIT_IS_SETTINGS (gs), NULL);
	
	system_font = g_settings_get_string (gs->priv->interface,
					     "monospace-font-name");
	
	return system_font;
}

GSList *
gedit_settings_get_list (GSettings   *settings,
			 const gchar *key)
{
	GSList *list = NULL;
	gchar **values;
	gsize i;

	g_return_val_if_fail (G_IS_SETTINGS (settings), NULL);
	g_return_val_if_fail (key != NULL, NULL);

	values = g_settings_get_strv (settings, key);
	i = 0;

	while (values[i] != NULL)
	{
		list = g_slist_prepend (list, values[i]);
		i++;
	}

	g_free (values);

	return g_slist_reverse (list);
}

void
gedit_settings_set_list (GSettings    *settings,
			 const gchar  *key,
			 const GSList *list)
{
	gchar **values = NULL;
	const GSList *l;

	g_return_if_fail (G_IS_SETTINGS (settings));
	g_return_if_fail (key != NULL);

	if (list != NULL)
	{
		gint i, len;

		len = g_slist_length ((GSList *)list);
		values = g_new (gchar *, len + 1);

		for (l = list, i = 0; l != NULL; l = g_slist_next (l), i++)
		{
			values[i] = l->data;
		}
		values[i] = NULL;
	}

	g_settings_set_strv (settings, key, (const gchar * const *)values);
	g_free (values);
}

/* ex:ts=8:noet: */
