/*
 * modeline-parser.c
 * Emacs, Kate and Vim-style modelines support for gedit.
 *
 * Copyright (C) 2005-2007 - Steve Frécinaux <code@istique.net>
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
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <gedit/gedit-prefs-manager.h>
#include <gedit/gedit-debug.h>
#include "modeline-parser.h"

typedef struct _ModelineOptions
{
	/* these options are similar to the GtkSourceView properties of the
	 * same names.
	 */
	gboolean	insert_spaces;
	guint		tab_width;
	guint		indent_width;
	GtkWrapMode	wrap_mode;
	gboolean	display_right_margin;
	guint		right_margin_position;
} ModelineOptions;

static gboolean
skip_whitespaces (gchar **s)
{
	while (**s != '\0' && g_ascii_isspace (**s))
		(*s)++;
	return **s != '\0';
}

/* Parse vi(m) modelines.
 * Vi(m) modelines looks like this: 
 *   - first form:   [text]{white}{vi:|vim:|ex:}[white]{options}
 *   - second form:  [text]{white}{vi:|vim:|ex:}[white]se[t] {options}:[text]
 * They can happen on the three first or last lines.
 */
static gchar *
parse_vim_modeline (gchar           *s,
		    ModelineOptions *options)
{
	gboolean in_set = FALSE;
	gboolean neg;
	guint intval;
	GString *key, *value;

	key = g_string_sized_new (8);
	value = g_string_sized_new (8);

	while (*s != '\0' && !(in_set && *s == ':'))
	{
		while (*s != '\0' && (*s == ':' || g_ascii_isspace (*s)))
			s++;

		if (*s == '\0')
			break;

		if (strncmp (s, "set ", 4) == 0 ||
		    strncmp (s, "se ", 3) == 0)
		{
			s = strchr(s, ' ') + 1;
			in_set = TRUE;
		}

		neg = FALSE;
		if (strncmp (s, "no", 2) == 0)
		{
			neg = TRUE;
			s += 2;
		}

		g_string_assign (key, "");
		g_string_assign (value, "");

		while (*s != '\0' && *s != ':' && *s != '=' &&
		       !g_ascii_isspace (*s))
		{
			g_string_append_c (key, *s);
			s++;
		}

		if (*s == '=')
		{
			s++;
			while (*s != '\0' && *s != ':' &&
			       !g_ascii_isspace (*s))
			{
				g_string_append_c (value, *s);
				s++;
			}
		}

		if (strcmp (key->str, "et") == 0 ||
		    strcmp (key->str, "expandtab") == 0)
		{
			options->insert_spaces = !neg;
		}
		else if (strcmp (key->str, "ts") == 0 ||
			 strcmp (key->str, "tabstop") == 0)
		{
			intval = atoi (value->str);
			if (intval) options->tab_width = intval;
		}
		else if (strcmp (key->str, "sw") == 0 ||
			 strcmp (key->str, "shiftwidth") == 0)
		{
			intval = atoi (value->str);
			if (intval) options->indent_width = intval;
		}
		else if (strcmp (key->str, "wrap") == 0)
		{
			options->wrap_mode = neg ? GTK_WRAP_NONE
						 : GTK_WRAP_WORD;
		}
		else if (strcmp (key->str, "textwidth") == 0)
		{
			intval = atoi (value->str);
			if (intval) options->right_margin_position = intval;
			options->display_right_margin = intval != 0;
		}
	}

	g_string_free (key, TRUE);
	g_string_free (value, TRUE);

	return s;
}

/* Parse emacs modelines.
 * Emacs modelines looks like this: "-*- key1: value1; key2: value2 -*-"
 * They can happen on the first line, or on the second one if the first line is
 * a shebang (#!)
 * See http://www.delorie.com/gnu/docs/emacs/emacs_486.html
 */
static gchar *
parse_emacs_modeline (gchar         *s,
		      ModelineOptions *options)
{
	guint intval;
	GString *key, *value;

	key = g_string_sized_new (8);
	value = g_string_sized_new (8);

	while (*s != '\0')
	{
		while (*s != '\0' && (*s == ';' || g_ascii_isspace (*s)))
			s++;
		if (*s == '\0' || strncmp (s, "-*-", 3) == 0)
			break;

		g_string_assign (key, "");
		g_string_assign (value, "");

		while (*s != '\0' && *s != ':' && *s != ';' &&
		       !g_ascii_isspace (*s))
		{
			g_string_append_c (key, *s);
			s++;
		}

		if (!skip_whitespaces (&s))
			break;

		if (*s != ':')
			continue;
		s++;

		if (!skip_whitespaces (&s))
			break;

		while (*s != '\0' && *s != ';' && !g_ascii_isspace (*s))
		{
			g_string_append_c (value, *s);
			s++;
		}

		gedit_debug_message (DEBUG_PLUGINS,
				     "Emacs modeline bit: %s = %s",
				     key->str, value->str);

		if (strcmp (key->str, "tab-width") == 0)
		{
			intval = atoi (value->str);
			if (intval) options->tab_width = intval;
		}
		else if (strcmp (key->str, "indent-offset") == 0)
		{
			intval = atoi (value->str);
			if (intval) options->indent_width = intval;
		}
		else if (strcmp (key->str, "indent-tabs-mode") == 0)
		{
			intval = strcmp (value->str, "nil") == 0;
			options->insert_spaces = intval;
		}
		else if (strcmp (key->str, "autowrap") == 0)
		{
			intval = strcmp (value->str, "nil") != 0;
			options->wrap_mode = intval ? GTK_WRAP_WORD
						    : GTK_WRAP_NONE;
		}
	}

	g_string_free (key, TRUE);
	g_string_free (value, TRUE);

	return *s == '\0' ? s : s + 2;
}

/*
 * Parse kate modelines.
 * Kate modelines are of the form "kate: key1 value1; key2 value2;"
 * These can happen on the 10 first or 10 last lines of the buffer.
 * See http://wiki.kate-editor.org/index.php/Modelines
 */
static gchar *
parse_kate_modeline (gchar *s,
		     ModelineOptions *options)
{
	guint intval;
	GString *key, *value;

	key = g_string_sized_new (8);
	value = g_string_sized_new (8);

	while (*s != '\0')
	{
		while (*s != '\0' && (*s == ';' || g_ascii_isspace (*s)))
			s++;
		if (*s == '\0')
			break;

		g_string_assign (key, "");
		g_string_assign (value, "");

		while (*s != '\0' && *s != ';' && !g_ascii_isspace (*s))
		{
			g_string_append_c (key, *s);
			s++;
		}

		if (!skip_whitespaces (&s))
			break;
		if (*s == ';')
			continue;

		while (*s != '\0' && *s != ';' &&
		       !g_ascii_isspace (*s))
		{
			g_string_append_c (value, *s);
			s++;
		}

		gedit_debug_message (DEBUG_PLUGINS,
				     "Kate modeline bit: %s = %s",
				     key->str, value->str);

		if (strcmp (key->str, "tab-width") == 0)
		{
			intval = atoi (value->str);
			if (intval) options->tab_width = intval;
		}
		else if (strcmp (key->str, "indent-width") == 0)
		{
			intval = atoi (value->str);
			if (intval) options->indent_width = intval;
		}
		else if (strcmp (key->str, "space-indent") == 0)
		{
			intval = strcmp (value->str, "on") == 0 ||
			         strcmp (value->str, "true") == 0 ||
			         strcmp (value->str, "1") == 0;
			options->insert_spaces = intval;
		}
		else if (strcmp (key->str, "word-wrap") == 0)
		{
			intval = strcmp (value->str, "on") == 0 ||
			         strcmp (value->str, "true") == 0 ||
			         strcmp (value->str, "1") == 0;
			options->wrap_mode = intval ? GTK_WRAP_WORD
						    : GTK_WRAP_NONE;
		}
		else if (strcmp (key->str, "word-wrap-column") == 0)
		{
			intval = atoi (value->str);
			if (intval) options->right_margin_position = intval;
			options->display_right_margin = intval != 0;
		}
	}

	g_string_free (key, TRUE);
	g_string_free (value, TRUE);

	return s;
}

static void
parse_modeline (gchar           *s,
		gint             line_number,
		gint             line_count,
		ModelineOptions *options)
{
	gchar prev = ' ';

	/* look for the beginning of a modeline */
	for (; *s != '\0'; prev = *(s++))
	{
		if (!g_ascii_isspace (prev))
			continue;

		if ((line_number < 3 || line_number >= line_count - 3) &&
		    (strncmp (s, "ex:", 3) == 0 ||
		     strncmp (s, "vi:", 3) == 0 ||
		     strncmp (s, "vim:", 4) == 0))
		{
			gedit_debug_message (DEBUG_PLUGINS, "Vim modeline on line %d", line_number);

		    	while (*s != ':') s++;
		    	s = parse_vim_modeline (s + 1, options);
		}
		else if (line_number == 0 && strncmp (s, "-*-", 3) == 0)
		{
			gedit_debug_message (DEBUG_PLUGINS, "Emacs modeline on line %d", line_number);

			s = parse_emacs_modeline (s + 3, options);
		}
		else if ((line_number < 10 || line_number >= line_count - 10) &&
			 strncmp (s, "kate:", 5) == 0)
		{
			gedit_debug_message (DEBUG_PLUGINS, "Kate modeline on line %d", line_number);

			s = parse_kate_modeline (s + 5, options);
		}
	}
}

void
apply_modeline (GtkSourceView *view)
{
	ModelineOptions options;
	GtkTextBuffer *buffer;
	GtkTextIter iter, liter;
	gint line_number;
	gint line_count;

	/* Default values for modeline options */
	options.insert_spaces = gedit_prefs_manager_get_insert_spaces ();
	options.tab_width = gedit_prefs_manager_get_tabs_size ();
	options.indent_width = -1; /* not set by default */
	options.wrap_mode = gedit_prefs_manager_get_wrap_mode ();
	options.display_right_margin =
			gedit_prefs_manager_get_display_right_margin ();
	options.right_margin_position =
			gedit_prefs_manager_get_right_margin_position ();

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (view));
	gtk_text_buffer_get_start_iter (buffer, &iter);
	line_number = 0;
	line_count = gtk_text_buffer_get_line_count (buffer);

	/* Parse the modelines on the 10 first lines... */
	while (line_number < 10 && line_number < line_count)
	{
		gchar *line;

		liter = iter;
		gtk_text_iter_forward_to_line_end (&iter);
		line = gtk_text_buffer_get_text (buffer, &liter, &iter, TRUE);

		parse_modeline (line, line_number, line_count, &options);

		g_free (line);
		line_number ++;
	}

	/* ...and on the 10 last ones (modelines are not allowed in between) */
	if (line_number < line_count - 10)
	{
		guint remaining_lines = line_count - line_number - 1;
		gtk_text_buffer_get_end_iter (buffer, &iter);
		gtk_text_iter_backward_lines (&iter,
					      MIN (10, remaining_lines));
		line_number = line_count - 11;
	}

	while (line_number < line_count)
	{
		gchar *line;

		liter = iter;
		gtk_text_iter_forward_to_line_end (&iter);
		line = gtk_text_buffer_get_text (buffer, &liter, &iter, TRUE);

		parse_modeline (line, line_number, line_count, &options);

		g_free (line);
		line_number ++;
	}

	/* Apply the options we got from modelines */
	gtk_source_view_set_insert_spaces_instead_of_tabs
						(view, options.insert_spaces);
	gtk_source_view_set_tab_width (view, options.tab_width);
	gtk_source_view_set_indent_width (view, options.indent_width);
	gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (view), options.wrap_mode);
	gtk_source_view_set_right_margin_position (view, options.right_margin_position);
	gtk_source_view_set_show_right_margin (view, options.display_right_margin);
}
