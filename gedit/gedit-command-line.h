/*
 * gedit-command-line.h
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

#ifndef __GEDIT_COMMAND_LINE_H__
#define __GEDIT_COMMAND_LINE_H__

#include <glib-object.h>
#include <gedit/gedit-encodings.h>

G_BEGIN_DECLS

#define GEDIT_TYPE_COMMAND_LINE			(gedit_command_line_get_type ())
#define GEDIT_COMMAND_LINE(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_COMMAND_LINE, GeditCommandLine))
#define GEDIT_COMMAND_LINE_CONST(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_COMMAND_LINE, GeditCommandLine const))
#define GEDIT_COMMAND_LINE_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), GEDIT_TYPE_COMMAND_LINE, GeditCommandLineClass))
#define GEDIT_IS_COMMAND_LINE(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEDIT_TYPE_COMMAND_LINE))
#define GEDIT_IS_COMMAND_LINE_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEDIT_TYPE_COMMAND_LINE))
#define GEDIT_COMMAND_LINE_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GEDIT_TYPE_COMMAND_LINE, GeditCommandLineClass))

typedef struct _GeditCommandLine	GeditCommandLine;
typedef struct _GeditCommandLineClass	GeditCommandLineClass;
typedef struct _GeditCommandLinePrivate	GeditCommandLinePrivate;

typedef struct _GeditCommandLineGeometry GeditCommandLineGeometry;

struct _GeditCommandLine
{
	GInitiallyUnowned parent;

	GeditCommandLinePrivate *priv;
};

struct _GeditCommandLineClass
{
	GInitiallyUnownedClass parent_class;
};

GType			 gedit_command_line_get_type		(void) G_GNUC_CONST;

GeditCommandLine	*gedit_command_line_get_default		(void);

gboolean		 gedit_command_line_parse		(GeditCommandLine *command_line,
								 int              *argc,
								 char           ***argv);

gboolean		 gedit_command_line_get_new_window	(GeditCommandLine *command_line);
void			 gedit_command_line_set_new_window	(GeditCommandLine *command_line,
								 gboolean          new_window);

gboolean		 gedit_command_line_get_new_document	(GeditCommandLine *command_line);
gint			 gedit_command_line_get_line_position	(GeditCommandLine *command_line);
gint			 gedit_command_line_get_column_position	(GeditCommandLine *command_line);

GSList			*gedit_command_line_get_file_list	(GeditCommandLine *command_line);
const GeditEncoding	*gedit_command_line_get_encoding	(GeditCommandLine *command_line);

gboolean		 gedit_command_line_get_wait		(GeditCommandLine *command_line);
gboolean		 gedit_command_line_get_background	(GeditCommandLine *command_line);
gboolean		 gedit_command_line_get_standalone	(GeditCommandLine *command_line);

const gchar		*gedit_command_line_get_geometry	(GeditCommandLine *command_line);

G_END_DECLS

#endif /* __GEDIT_COMMAND_LINE_H__ */

/* ex:set ts=8 noet: */
