/*
 * gedit-newline-converter.c
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


#include "config.h"

#include "gedit-newline-converter.h"
#include "gedit-enum-types.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#include <pango/pango.h>
#include <string.h>

#define GEDIT_NEWLINE_CONVERTER_GET_PRIVATE(object) (G_TYPE_INSTANCE_GET_PRIVATE ((object),\
										   GEDIT_TYPE_NEWLINE_CONVERTER, \
										   GeditNewlineConverterPrivate))

enum
{
	PROP_0,
	PROP_NEWLINE_TYPE
};

/**
 * SECTION:gedit-newline-converter
 * @short_description: Convert newlines
 * @include: gio/gio.h
 *
 * #GeditNewlineConverter is an implementation of #GConverter
 */

static void gedit_newline_converter_iface_init          (GConverterIface *iface);

struct _GeditNewlineConverterPrivate
{
	GeditNewlineConverterNewlineType newline_type;
};

G_DEFINE_TYPE_WITH_CODE (GeditNewlineConverter, gedit_newline_converter, G_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (G_TYPE_CONVERTER,
						gedit_newline_converter_iface_init))

static void
gedit_newline_converter_finalize (GObject *object)
{
	G_OBJECT_CLASS (gedit_newline_converter_parent_class)->finalize (object);
}

static void
gedit_newline_converter_set_property (GObject      *object,
				      guint         prop_id,
				      const GValue *value,
				      GParamSpec   *pspec)
{
	GeditNewlineConverter *newline;

	newline = GEDIT_NEWLINE_CONVERTER (object);

	switch (prop_id)
	{
		case PROP_NEWLINE_TYPE:
			newline->priv->newline_type = g_value_get_enum (value);
		break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
gedit_newline_converter_get_property (GObject    *object,
				      guint       prop_id,
				      GValue     *value,
				      GParamSpec *pspec)
{
	GeditNewlineConverter *newline;

	newline = GEDIT_NEWLINE_CONVERTER (object);

	switch (prop_id)
	{
		case PROP_NEWLINE_TYPE:
			g_value_set_enum (value, newline->priv->newline_type);
		break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
gedit_newline_converter_class_init (GeditNewlineConverterClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

	gobject_class->finalize = gedit_newline_converter_finalize;
	gobject_class->get_property = gedit_newline_converter_get_property;
	gobject_class->set_property = gedit_newline_converter_set_property;

	/**
	 * GDataStream:newline-type:
	 *
	 * The :newline-type property determines what is considered
	 * as a line ending when converting data.
	 */ 
	g_object_class_install_property (gobject_class,
	                                 PROP_NEWLINE_TYPE,
	                                 g_param_spec_enum ("newline-type",
	                                                    "Newline type",
	                                                    "The accepted types of line ending",
	                                                    GEDIT_TYPE_NEWLINE_CONVERTER_NEWLINE_TYPE,
	                                                    GEDIT_NEWLINE_CONVERTER_NEWLINE_TYPE_LF,
	                                                    G_PARAM_READWRITE | G_PARAM_STATIC_NAME | G_PARAM_STATIC_BLURB));

	g_type_class_add_private (gobject_class, sizeof (GeditNewlineConverterPrivate));
}

static void
gedit_newline_converter_init (GeditNewlineConverter *converter)
{
	converter->priv = GEDIT_NEWLINE_CONVERTER_GET_PRIVATE (converter);
}

/**
 * gedit_newline_converter_new:
 *
 * Creates a new #GeditNewlineConverter.
 *
 * Returns: a new #GeditNewlineConverter or %NULL on error.
 **/
GeditNewlineConverter *
gedit_newline_converter_new ()
{
	GeditNewlineConverter *newline;

	newline = g_object_new (GEDIT_TYPE_NEWLINE_CONVERTER, NULL);

	return newline;
}

static void
gedit_newline_converter_reset (GConverter *converter)
{
}

static GConverterResult
gedit_newline_converter_convert (GConverter *converter,
				 const void *inbuf,
				 gsize       inbuf_size,
				 void       *outbuf,
				 gsize       outbuf_size,
				 GConverterFlags flags,
				 gsize      *bytes_read,
				 gsize      *bytes_written,
				 GError    **error)
{
	GeditNewlineConverter *newline;
	gchar *inbufp, *outbufp;
	gsize in_left, out_left;
	gsize pos_in, pos_out;
	gsize size;
	gint paragraph_delimiter_index;
	gint next_paragraph_start;

	newline = GEDIT_NEWLINE_CONVERTER (converter);

	/* Check if we finished converting or there was an error */
	if (inbuf_size == 0)
	{
		if (flags & G_CONVERTER_INPUT_AT_END)
			return G_CONVERTER_FINISHED;

		if (flags & G_CONVERTER_FLUSH)
			return G_CONVERTER_FLUSHED;

		g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_PARTIAL_INPUT,
				     _("Incomplete multibyte sequence in input"));
		return G_CONVERTER_ERROR;
	}

	if (outbuf_size < 2)
	{
		g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_NO_SPACE,
				     _("Not enough space in destination"));
		return G_CONVERTER_ERROR;
	}

	inbufp = (char *)inbuf;
	outbufp = (char *)outbuf;
	in_left = inbuf_size;
	out_left = outbuf_size;
	*bytes_read = 0;
	*bytes_written = 0;

	while (in_left > 0 && out_left > 0)
	{
		pango_find_paragraph_boundary (inbufp + *bytes_read,
					       in_left, &paragraph_delimiter_index,
					       &next_paragraph_start);

		/* No boundaries */
		if ((in_left == paragraph_delimiter_index) == TRUE &&
		    (in_left == next_paragraph_start) == TRUE)
		{
			size = MIN (out_left, in_left);

			memcpy (outbufp + *bytes_written, inbufp + *bytes_read,
				size);
			*bytes_read += size;
			*bytes_written += size;

			break;
		}

		/* We found a delimiter but it is higher that outbuf_size */
		if (paragraph_delimiter_index > out_left)
		{
			memcpy (outbufp + *bytes_written, inbufp + *bytes_read,
				out_left);
			*bytes_read += out_left;
			*bytes_written += out_left;

			break;
		}

		/* We better leave the delimiter conversion for the next iteration */
		if (paragraph_delimiter_index == out_left)
		{
			memcpy (outbufp + *bytes_written, inbufp + *bytes_read,
				out_left - 1);
			*bytes_read += out_left - 1;
			*bytes_written += out_left - 1;

			break;
		}

		/* We found a delimiter so we manage it */
		size = paragraph_delimiter_index; /* As it is the index we already have the index + 1 */

		memcpy (outbufp + *bytes_written, inbufp + *bytes_read, size);

		*bytes_written += size;
		*bytes_read += size;
		in_left -= size;
		out_left -= size;

		pos_out = *bytes_written;
		pos_in = *bytes_read;

		switch (newline->priv->newline_type)
		{
			case GEDIT_NEWLINE_CONVERTER_NEWLINE_TYPE_CR:
				outbufp[pos_out] = 13;
				*bytes_written += 1;
				*bytes_read += 1;
				out_left -= 1;
				in_left -= 1;
				g_print ("cr\n\n");

				if (inbufp[pos_in + 1] == 10) /* LF */
				{
					*bytes_read += 1;
					in_left -= 1;
				}
			break;

			case GEDIT_NEWLINE_CONVERTER_NEWLINE_TYPE_LF:
				outbufp[pos_out] = 10;
				*bytes_written += 1;
				*bytes_read += 1;
				out_left -= 1;
				in_left -= 1;

				if (inbufp[pos_in] == 13 && inbufp[pos_in + 1] == 10) /* CR LF */
				{
					/* We skip one char */
					*bytes_read += 1;
					in_left -= 1;
				}
			break;

			case GEDIT_NEWLINE_CONVERTER_NEWLINE_TYPE_CR_LF:
				outbufp[pos_out] = 13;
				outbufp[pos_out + 1] = 10;
				*bytes_written += 2;
				*bytes_read += 1;
				out_left -= 2;
				in_left -= 1;

				if (inbufp[pos_in] == 13 && inbufp[pos_in + 1] == 10)
				{
					*bytes_read += 1;
					in_left -= 1;
				}
			break;

		}
	}

	return G_CONVERTER_CONVERTED;
}

static void
gedit_newline_converter_iface_init (GConverterIface *iface)
{
	iface->convert = gedit_newline_converter_convert;
	iface->reset = gedit_newline_converter_reset;
}

void
gedit_newline_converter_set_newline_type (GeditNewlineConverter           *converter,
					  GeditNewlineConverterNewlineType type)
{
	g_return_if_fail (GEDIT_IS_NEWLINE_CONVERTER (converter));

	if (converter->priv->newline_type != type)
	{
		converter->priv->newline_type = type;

		g_object_notify (G_OBJECT (converter), "newline-type");
	}
}
