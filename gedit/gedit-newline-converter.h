/*
 * gedit-newline-converter.h
 * This file is part of gedit
 *
 * Copyright (C) 2009 - Ignacio Casal Quinteiro
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


#ifndef __GEDIT_NEWLINE_CONVERTER_H__
#define __GEDIT_NEWLINE_CONVERTER_H__

#include <gio/gio.h>

G_BEGIN_DECLS

#define GEDIT_TYPE_NEWLINE_CONVERTER		(gedit_newline_converter_get_type ())
#define GEDIT_NEWLINE_CONVERTER(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GEDIT_TYPE_NEWLINE_CONVERTER, GeditNewlineConverter))
#define GEDIT_NEWLINE_CONVERTER_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), GEDIT_TYPE_NEWLINE_CONVERTER, GeditNewlineConverterClass))
#define GEDIT_IS_NEWLINE_CONVERTER(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GEDIT_TYPE_NEWLINE_CONVERTER))
#define GEDIT_IS_NEWLINE_CONVERTER_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GEDIT_TYPE_NEWLINE_CONVERTER))
#define GEDIT_NEWLINE_CONVERTER_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GEDIT_TYPE_NEWLINE_CONVERTER, GeditNewlineConverterClass))

typedef struct _GeditNewlineConverterClass GeditNewlineConverterClass;
typedef struct _GeditNewlineConverter GeditNewlineConverter;
typedef struct _GeditNewlineConverterPrivate GeditNewlineConverterPrivate;

/**
 * GeditNewlineConverter:
 *
 * Conversions between different endline characters.
 */
struct _GeditNewlineConverter
{
	GObject parent_instance;

	GeditNewlineConverterPrivate *priv;
};

struct _GeditNewlineConverterClass
{
	GObjectClass parent_class;
};

/**
 * GeditNewlineConverterNewlineType:
 * @GEDIT_NEWLINE_CONVERTER_NEWLINE_TYPE_LF: Selects "LF" line endings, common on most modern UNIX platforms.
 * @GEDIT_NEWLINE_CONVERTER_NEWLINE_TYPE_CR: Selects "CR" line endings.
 * @GEDIT_NEWLINE_CONVERTER_NEWLINE_TYPE_CR_LF: Selects "CR, LF" line ending, common on Microsoft Windows.
 *
 * #GeditNewlineConverterNewlineType is used when checking for or setting the line endings.
 **/
typedef enum
{
	GEDIT_NEWLINE_CONVERTER_NEWLINE_TYPE_CR,
	GEDIT_NEWLINE_CONVERTER_NEWLINE_TYPE_LF,
	GEDIT_NEWLINE_CONVERTER_NEWLINE_TYPE_CR_LF
} GeditNewlineConverterNewlineType;


GType			 gedit_newline_converter_get_type		(void) G_GNUC_CONST;

GeditNewlineConverter	*gedit_newline_converter_new			(void);

void			 gedit_newline_converter_set_newline_type	(GeditNewlineConverter           *converter,
									 GeditNewlineConverterNewlineType type);

G_END_DECLS

#endif /* __GEDIT_NEWLINE_CONVERTER_H__ */
