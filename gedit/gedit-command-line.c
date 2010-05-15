/*
 * gedit-command-line.c
 * This file is part of gedit
 *
 * Copyright (C) 2010 - Jesse van den Kieboom
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

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n.h>
#include <gio/gio.h>
#include <gtk/gtk.h>

#include "gedit-command-line.h"
#include "eggsmclient.h"

#define GEDIT_COMMAND_LINE_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE((object), GEDIT_TYPE_COMMAND_LINE, GeditCommandLinePrivate))

struct _GeditCommandLinePrivate
{
	/* These are directly set as options */
	gchar *line_column_position;
	gchar *encoding_charset;
	gchar *geometry;
	gboolean new_window;
	gboolean new_document;
	gchar **remaining_args;

	gboolean wait;
	gboolean background;
	gboolean standalone;

	/* This result from post-processing command line arguments */
	gint line_position;
	gint column_position;
	GSList *file_list;
	const GeditEncoding *encoding;
};

G_DEFINE_TYPE (GeditCommandLine, gedit_command_line, G_TYPE_INITIALLY_UNOWNED)

static void
gedit_command_line_finalize (GObject *object)
{
	GeditCommandLine *command_line = GEDIT_COMMAND_LINE (object);

	g_free (command_line->priv->encoding_charset);
	g_free (command_line->priv->line_column_position);
	g_strfreev (command_line->priv->remaining_args);

	g_free (command_line->priv->geometry);

	g_slist_foreach (command_line->priv->file_list, (GFunc)g_object_unref, NULL);
	g_slist_free (command_line->priv->file_list);

	G_OBJECT_CLASS (gedit_command_line_parent_class)->finalize (object);
}

static GObject *
gedit_command_line_constructor (GType                  gtype,
                                guint                  n_construct_params,
                                GObjectConstructParam *construct_params)
{
	static GObject *command_line = NULL;

	if (!command_line)
	{
		command_line = G_OBJECT_CLASS (gedit_command_line_parent_class)->constructor (gtype,
		                                                                              n_construct_params,
		                                                                              construct_params);

		g_object_add_weak_pointer (command_line, (gpointer *) &command_line);
		return command_line;
	}

	return g_object_ref (command_line);
}
static void
gedit_command_line_class_init (GeditCommandLineClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = gedit_command_line_finalize;
	object_class->constructor = gedit_command_line_constructor;

	g_type_class_add_private (object_class, sizeof(GeditCommandLinePrivate));
}

static void
gedit_command_line_init (GeditCommandLine *self)
{
	self->priv = GEDIT_COMMAND_LINE_GET_PRIVATE (self);
}

GeditCommandLine *
gedit_command_line_get_default (void)
{
	GeditCommandLine *command_line;

	command_line = g_object_new (GEDIT_TYPE_COMMAND_LINE, NULL);

	if (g_object_is_floating (command_line))
	{
		g_object_ref_sink (command_line);
	}
	else
	{
		g_object_unref (command_line);
	}

	return command_line;
}

static void
show_version_and_quit (void)
{
	g_print ("%s - Version %s\n", g_get_application_name (), VERSION);

	exit (0);
}

static void
list_encodings_and_quit (void)
{
	gint i = 0;
	const GeditEncoding *enc;

	while ((enc = gedit_encoding_get_from_index (i)) != NULL)
	{
		g_print ("%s\n", gedit_encoding_get_charset (enc));

		++i;
	}

	exit (0);
}

static void
get_line_column_position (GeditCommandLine *command_line,
                          const gchar      *arg)
{
	gchar **split;

	split = g_strsplit (arg, ":", 2);

	if (split != NULL)
	{
		if (split[0] != NULL)
		{
			command_line->priv->line_position = atoi (split[0]);
		}

		if (split[1] != NULL)
		{
			command_line->priv->column_position = atoi (split[1]);
		}
	}

	g_strfreev (split);
}

static void
process_remaining_arguments (GeditCommandLine *command_line)
{
	gint i;

	if (!command_line->priv->remaining_args)
	{
		return;
	}

	for (i = 0; command_line->priv->remaining_args[i]; i++)
	{
		if (*command_line->priv->remaining_args[i] == '+')
		{
			if (*(command_line->priv->remaining_args[i] + 1) == '\0')
			{
				/* goto the last line of the document */
				command_line->priv->line_position = G_MAXINT;
				command_line->priv->column_position = 0;
			}
			else
			{
				get_line_column_position (command_line, command_line->priv->remaining_args[i] + 1);
			}
		}
		else
		{
			GFile *file;

			file = g_file_new_for_commandline_arg (command_line->priv->remaining_args[i]);
			command_line->priv->file_list = g_slist_prepend (command_line->priv->file_list, file);
		}
	}

	command_line->priv->file_list = g_slist_reverse (command_line->priv->file_list);
}

static void
process_command_line (GeditCommandLine *command_line)
{
	/* Parse encoding */
	if (command_line->priv->encoding_charset)
	{
		command_line->priv->encoding = gedit_encoding_get_from_charset (command_line->priv->encoding_charset);

		if (command_line->priv->encoding == NULL)
		{
			g_print (_("%s: invalid encoding.\n"), command_line->priv->encoding_charset);
		}

		g_free (command_line->priv->encoding_charset);
		command_line->priv->encoding_charset = NULL;
	}

	/* Parse remaining arguments */
	process_remaining_arguments (command_line);
}

gboolean
gedit_command_line_parse (GeditCommandLine   *command_line,
                          int                *argc,
                          char             ***argv)
{
	GOptionContext *context;
	GError *error = NULL;

	const GOptionEntry options[] =
	{
		/* Version */
		{
			"version", 'V', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK,
			show_version_and_quit, N_("Show the application's version"), NULL
		},

		/* List available encodings */
		{
			"list-encodings", '\0', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK,
			list_encodings_and_quit, N_("Display list of possible values for the encoding option"),
			NULL
		},

		/* Encoding */
		{
			"encoding", '\0', 0, G_OPTION_ARG_STRING,
			&command_line->priv->encoding_charset,
			N_("Set the character encoding to be used to open the files listed on the command line"),
			N_("ENCODING")
		},

		/* Open a new window */
		{
			"new-window", '\0', 0, G_OPTION_ARG_NONE,
			&command_line->priv->new_window,
			N_("Create a new top-level window in an existing instance of gedit"),
			NULL
		},

		/* Create a new empty document */
		{
			"new-document", '\0', 0, G_OPTION_ARG_NONE,
			&command_line->priv->new_document,
			N_("Create a new document in an existing instance of gedit"),
			NULL
		},

		/* Window geometry */
		{
			"geometry", 'g', 0, G_OPTION_ARG_STRING,
			&command_line->priv->geometry,
			N_("Set the X geometry window size (WIDTHxHEIGHT+X+Y)"),
			N_("GEOMETRY")
		},

		/* Wait for closing documents */
		{
			"wait", 'w', 0, G_OPTION_ARG_NONE,
			&command_line->priv->wait,
			N_("Open files and block process until files are closed"),
			NULL
		},

		/* Run in the background */
		{
			"background", 'b', 0, G_OPTION_ARG_NONE,
			&command_line->priv->background,
			N_("Run gedit in the background"),
			NULL
		},
		
		/* Wait for closing documents */
		{
			"standalone", 's', 0, G_OPTION_ARG_NONE,
			&command_line->priv->standalone,
			N_("Run gedit in standalone mode"),
			NULL
		},

		/* collects file arguments */
		{
			G_OPTION_REMAINING, '\0', 0, G_OPTION_ARG_FILENAME_ARRAY,
			&command_line->priv->remaining_args,
			NULL,
			N_("[FILE...] [+LINE[:COLUMN]]")
		},

		{NULL}
	};

	/* Setup command line options */
	context = g_option_context_new (_("- Edit text files"));
	g_option_context_add_main_entries (context, options, GETTEXT_PACKAGE);
	g_option_context_add_group (context, gtk_get_option_group (FALSE));
	g_option_context_add_group (context, egg_sm_client_get_option_group ());

	gtk_init (argc, argv);

	if (!g_option_context_parse (context, argc, argv, &error))
	{
		g_print(_("%s\nRun '%s --help' to see a full list of available command line options.\n"),
			error->message, (*argv)[0]);

		g_error_free (error);
		return FALSE;
	}

	g_option_context_free (context);

	/* Do some post-processing */
	process_command_line (command_line);

	return TRUE;
}

gboolean
gedit_command_line_get_new_window (GeditCommandLine *command_line)
{
	g_return_val_if_fail (GEDIT_IS_COMMAND_LINE (command_line), FALSE);
	return command_line->priv->new_window;
}

void
gedit_command_line_set_new_window (GeditCommandLine *command_line,
                                   gboolean          new_window)
{
	g_return_if_fail (GEDIT_IS_COMMAND_LINE (command_line));

	command_line->priv->new_window = new_window;
}

gboolean
gedit_command_line_get_new_document (GeditCommandLine *command_line)
{
	g_return_val_if_fail (GEDIT_IS_COMMAND_LINE (command_line), FALSE);
	return command_line->priv->new_document;
}

gint
gedit_command_line_get_line_position (GeditCommandLine *command_line)
{
	g_return_val_if_fail (GEDIT_IS_COMMAND_LINE (command_line), 0);
	return command_line->priv->line_position;
}

gint
gedit_command_line_get_column_position (GeditCommandLine *command_line)
{
	g_return_val_if_fail (GEDIT_IS_COMMAND_LINE (command_line), 0);
	return command_line->priv->column_position;
}

GSList *
gedit_command_line_get_file_list (GeditCommandLine *command_line)
{
	g_return_val_if_fail (GEDIT_IS_COMMAND_LINE (command_line), NULL);
	return command_line->priv->file_list;
}

const GeditEncoding *
gedit_command_line_get_encoding (GeditCommandLine *command_line)
{
	g_return_val_if_fail (GEDIT_IS_COMMAND_LINE (command_line), NULL);
	return command_line->priv->encoding;
}

gboolean
gedit_command_line_get_wait (GeditCommandLine *command_line)
{
	g_return_val_if_fail (GEDIT_IS_COMMAND_LINE (command_line), FALSE);
	return command_line->priv->wait;
}

gboolean
gedit_command_line_get_background (GeditCommandLine *command_line)
{
	g_return_val_if_fail (GEDIT_IS_COMMAND_LINE (command_line), FALSE);
	return command_line->priv->background;
}

gboolean
gedit_command_line_get_standalone (GeditCommandLine *command_line)
{
	g_return_val_if_fail (GEDIT_IS_COMMAND_LINE (command_line), FALSE);
	return command_line->priv->standalone;
}

const gchar *
gedit_command_line_get_geometry (GeditCommandLine *command_line)
{
	g_return_val_if_fail (GEDIT_IS_COMMAND_LINE (command_line), NULL);
	return command_line->priv->geometry;
}

/* ex:ts=8:noet: */
