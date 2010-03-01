/*
 * gedit-document-output-stream.c
 * This file is part of gedit
 *
 * Copyright (C) 2010 - Ignacio Casal Quinteiro
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

#include <string.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#include "gedit-document-output-stream.h"

/* NOTE: never use async methods on this stream, the stream is just
 * a wrapper around GtkTextBuffer api so that we can use GIO Stream
 * methods, but the undelying code operates on a GtkTextBuffer, so
 * there is no I/O involved and should be accessed only by the main
 * thread */

#define GEDIT_DOCUMENT_OUTPUT_STREAM_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE((object),\
							 GEDIT_TYPE_DOCUMENT_OUTPUT_STREAM,\
							 GeditDocumentOutputStreamPrivate))

#define MAX_UNICHAR_LEN 6

struct _GeditDocumentOutputStreamPrivate
{
	GeditDocument *doc;
	GtkTextIter    pos;

	gchar *buffer;
	gsize buflen;

	guint n_fallback_errors;

	guint is_initialized : 1;
	guint is_closed : 1;
};

enum
{
	PROP_0,
	PROP_DOCUMENT
};

G_DEFINE_TYPE (GeditDocumentOutputStream, gedit_document_output_stream, G_TYPE_OUTPUT_STREAM)

static gssize	gedit_document_output_stream_write (GOutputStream            *stream,
						    const void               *buffer,
						    gsize                     count,
						    GCancellable             *cancellable,
						    GError                  **error);

static gboolean	gedit_document_output_stream_close (GOutputStream     *stream,
						    GCancellable      *cancellable,
						    GError           **error);

static void
gedit_document_output_stream_set_property (GObject      *object,
					   guint         prop_id,
					   const GValue *value,
					   GParamSpec   *pspec)
{
	GeditDocumentOutputStream *stream = GEDIT_DOCUMENT_OUTPUT_STREAM (object);

	switch (prop_id)
	{
		case PROP_DOCUMENT:
			stream->priv->doc = GEDIT_DOCUMENT (g_value_get_object (value));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_document_output_stream_get_property (GObject    *object,
					   guint       prop_id,
					   GValue     *value,
					   GParamSpec *pspec)
{
	GeditDocumentOutputStream *stream = GEDIT_DOCUMENT_OUTPUT_STREAM (object);

	switch (prop_id)
	{
		case PROP_DOCUMENT:
			g_value_set_object (value, stream->priv->doc);
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static void
gedit_document_output_stream_finalize (GObject *object)
{
	GeditDocumentOutputStream *stream = GEDIT_DOCUMENT_OUTPUT_STREAM (object);

	g_free (stream->priv->buffer);

	G_OBJECT_CLASS (gedit_document_output_stream_parent_class)->finalize (object);
}

static void
gedit_document_output_stream_constructed (GObject *object)
{
	GeditDocumentOutputStream *stream = GEDIT_DOCUMENT_OUTPUT_STREAM (object);

	if (!stream->priv->doc)
	{
		g_critical ("This should never happen, a problem happened constructing the Document Output Stream!");
		return;
	}

	/* Init the undoable action */
	gtk_source_buffer_begin_not_undoable_action (GTK_SOURCE_BUFFER (stream->priv->doc));
	/* clear the buffer */
	gtk_text_buffer_set_text (GTK_TEXT_BUFFER (stream->priv->doc),
				  "", 0);
	gtk_text_buffer_set_modified (GTK_TEXT_BUFFER (stream->priv->doc),
				      FALSE);

	gtk_source_buffer_end_not_undoable_action (GTK_SOURCE_BUFFER (stream->priv->doc));
}

static void
gedit_document_output_stream_class_init (GeditDocumentOutputStreamClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GOutputStreamClass *stream_class = G_OUTPUT_STREAM_CLASS (klass);

	object_class->get_property = gedit_document_output_stream_get_property;
	object_class->set_property = gedit_document_output_stream_set_property;
	object_class->finalize = gedit_document_output_stream_finalize;
	object_class->constructed = gedit_document_output_stream_constructed;

	stream_class->write_fn = gedit_document_output_stream_write;
	stream_class->close_fn = gedit_document_output_stream_close;

	g_object_class_install_property (object_class,
					 PROP_DOCUMENT,
					 g_param_spec_object ("document",
							      "Document",
							      "The document which is written",
							      GEDIT_TYPE_DOCUMENT,
							      G_PARAM_READWRITE |
							      G_PARAM_CONSTRUCT_ONLY));

	g_type_class_add_private (object_class, sizeof (GeditDocumentOutputStreamPrivate));
}

static void
gedit_document_output_stream_init (GeditDocumentOutputStream *stream)
{
	stream->priv = GEDIT_DOCUMENT_OUTPUT_STREAM_GET_PRIVATE (stream);

	stream->priv->buffer = NULL;
	stream->priv->buflen = 0;

	stream->priv->n_fallback_errors = 0;

	stream->priv->is_initialized = FALSE;
	stream->priv->is_closed = FALSE;
}

static GeditDocumentNewlineType
get_newline_type (GtkTextIter *end)
{
	GeditDocumentNewlineType res;
	GtkTextIter copy;
	gunichar c;

	copy = *end;
	c = gtk_text_iter_get_char (&copy);

	if (g_unichar_break_type (c) == G_UNICODE_BREAK_CARRIAGE_RETURN)
	{
		if (gtk_text_iter_forward_char (&copy) &&
		    g_unichar_break_type (gtk_text_iter_get_char (&copy)) == G_UNICODE_BREAK_LINE_FEED)
		{
			res = GEDIT_DOCUMENT_NEWLINE_TYPE_CR_LF;
		}
		else
		{
			res = GEDIT_DOCUMENT_NEWLINE_TYPE_CR;
		}
	}
	else
	{
		res = GEDIT_DOCUMENT_NEWLINE_TYPE_LF;
	}

	return res;
}

GOutputStream *
gedit_document_output_stream_new (GeditDocument *doc)
{
	return G_OUTPUT_STREAM (g_object_new (GEDIT_TYPE_DOCUMENT_OUTPUT_STREAM,
					      "document", doc, NULL));
}

GeditDocumentNewlineType
gedit_document_output_stream_detect_newline_type (GeditDocumentOutputStream *stream)
{
	GeditDocumentNewlineType type;
	GtkTextIter iter;

	g_return_val_if_fail (GEDIT_IS_DOCUMENT_OUTPUT_STREAM (stream),
			      GEDIT_DOCUMENT_NEWLINE_TYPE_DEFAULT);

	type = GEDIT_DOCUMENT_NEWLINE_TYPE_DEFAULT;

	gtk_text_buffer_get_start_iter (GTK_TEXT_BUFFER (stream->priv->doc),
					&iter);

	if (gtk_text_iter_ends_line (&iter) || gtk_text_iter_forward_to_line_end (&iter))
	{
		type = get_newline_type (&iter);
	}

	return type;
}

guint
gedit_document_output_stream_get_num_fallbacks (GeditDocumentOutputStream *stream)
{
	g_return_val_if_fail (GEDIT_IS_DOCUMENT_OUTPUT_STREAM (stream), 0);

	return stream->priv->n_fallback_errors;
}

void
gedit_document_output_stream_insert_fallback (GeditDocumentOutputStream *stream,
                                              const gchar               *buffer)
{
	GtkSourceStyleScheme *scheme;
	GtkSourceStyle *style;
	GtkTextTag *tag;
	guint8 out[4];
	guint8 v;
	gchar *background;
	gchar *foreground;
	const gchar hex[] = "0123456789ABCDEF";

	g_return_if_fail (GEDIT_IS_DOCUMENT_OUTPUT_STREAM (stream));
	g_return_if_fail (buffer != NULL);

	scheme = gtk_source_buffer_get_style_scheme (GTK_SOURCE_BUFFER (stream->priv->doc));
	style = gtk_source_style_scheme_get_style (scheme,
	                                           "def:error");

	tag = NULL;

	if (style != NULL)
	{
		/* FIXME: Get attributes */
		g_object_get (G_OBJECT (style), "background", &background,
			      "foreground", &foreground, NULL);

		tag = gtk_text_buffer_create_tag (GTK_TEXT_BUFFER (stream->priv->doc),
			                          NULL,
			                          "background", background,
			                          "foreground", foreground,
			                          NULL);

		g_free (background);
		g_free (foreground);
	}

	/* if we are here is because we are pointing to an invalid char
	 * so we substitute it by an hex value */
	v = *(guint8 *)buffer;
	out[0] = '\\';
	out[1] = hex[(v & 0xf0) >> 4];
	out[2] = hex[(v & 0x0f) >> 0];
	out[3] = '\0';

	if (tag != NULL)
	{
		gtk_text_buffer_insert_with_tags (GTK_TEXT_BUFFER (stream->priv->doc),
		                                  &stream->priv->pos,
		                                  (const gchar *)out, 3,
		                                  tag, NULL);

		g_object_unref (tag);
	}
	else
	{
		gtk_text_buffer_insert (GTK_TEXT_BUFFER (stream->priv->doc),
		                        &stream->priv->pos,
		                       (const gchar *)out, 3);
	}

	stream->priv->n_fallback_errors++;
}

/* If the last char is a newline, remove it from the buffer (otherwise
   GtkTextView shows it as an empty line). See bug #324942. */
static void
remove_ending_newline (GeditDocumentOutputStream *stream)
{
	GtkTextIter end;
	GtkTextIter start;

	gtk_text_buffer_get_end_iter (GTK_TEXT_BUFFER (stream->priv->doc), &end);
	start = end;

	gtk_text_iter_set_line_offset (&start, 0);

	if (gtk_text_iter_ends_line (&start) &&
	    gtk_text_iter_backward_line (&start))
	{
		if (!gtk_text_iter_ends_line (&start))
		{
			gtk_text_iter_forward_to_line_end (&start);
		}

		/* Delete the empty line which is from 'start' to 'end' */
		gtk_text_buffer_delete (GTK_TEXT_BUFFER (stream->priv->doc),
		                        &start,
		                        &end);
	}
}

static void
end_append_text_to_document (GeditDocumentOutputStream *stream)
{
	remove_ending_newline (stream);

	gtk_text_buffer_set_modified (GTK_TEXT_BUFFER (stream->priv->doc),
				      FALSE);

	gtk_source_buffer_end_not_undoable_action (GTK_SOURCE_BUFFER (stream->priv->doc));
}

static void
insert_with_fallback (GeditDocumentOutputStream *stream,
                      const gchar               *buffer,
                      gsize                      count)
{
	GtkTextBuffer *text_buffer;
	GtkTextIter   *iter;
	gsize len;
	gssize written;

	text_buffer = GTK_TEXT_BUFFER (stream->priv->doc);
	iter = &stream->priv->pos;
	len = count;

	while (len != 0)
	{
		const gchar *end;
		gboolean valid;
		gsize nvalid;

		/* validate */
		valid = g_utf8_validate (buffer, len, &end);
		nvalid = end - buffer;

		gtk_text_buffer_insert (text_buffer, iter, buffer, nvalid);
		written += nvalid;

		/* If we inserted all return */
		if (nvalid == len)
			return;

		buffer += nvalid;
		len = len - nvalid;

		if ((len < MAX_UNICHAR_LEN) &&
		    (g_utf8_get_char_validated (buffer, len) == (gunichar)-2))
		{
			stream->priv->buffer = g_strndup (end, len);
			stream->priv->buflen = len;

			return;
		}

		gedit_document_output_stream_insert_fallback (stream, buffer);
		buffer++;
		len--;
	}
}

static gssize
gedit_document_output_stream_write (GOutputStream            *stream,
				    const void               *buffer,
				    gsize                     count,
				    GCancellable             *cancellable,
				    GError                  **error)
{
	GeditDocumentOutputStream *ostream;
	gchar *text;
	gsize len;
	gboolean freetext = FALSE;

	if (g_cancellable_set_error_if_cancelled (cancellable, error))
		return -1;

	ostream = GEDIT_DOCUMENT_OUTPUT_STREAM (stream);

	if (!ostream->priv->is_initialized)
	{
		/* Init the undoable action */
		gtk_source_buffer_begin_not_undoable_action (GTK_SOURCE_BUFFER (ostream->priv->doc));

		gtk_text_buffer_get_start_iter (GTK_TEXT_BUFFER (ostream->priv->doc),
						&ostream->priv->pos);
		ostream->priv->is_initialized = TRUE;
	}

	if (ostream->priv->buflen > 0)
	{
		len = ostream->priv->buflen + count;
		text = g_new (gchar , len + 1);
		memcpy (text, ostream->priv->buffer, ostream->priv->buflen);
		memcpy (text + ostream->priv->buflen, buffer, count);
		text[len] = '\0';
		g_free (ostream->priv->buffer);
		ostream->priv->buffer = NULL;
		ostream->priv->buflen = 0;
		freetext = TRUE;
	}
	else
	{
		text = (gchar *) buffer;
		len = count;
	}

	if (len != 0)
	{
		insert_with_fallback (ostream, text, len);
	}

	if (freetext)
		g_free (text);

	return count;
}

static gboolean
gedit_document_output_stream_close (GOutputStream     *stream,
				    GCancellable      *cancellable,
				    GError           **error)
{
	GeditDocumentOutputStream *ostream = GEDIT_DOCUMENT_OUTPUT_STREAM (stream);

	if (!ostream->priv->is_closed && ostream->priv->is_initialized)
	{
		if (ostream->priv->buflen > 0)
		{
			gedit_document_output_stream_insert_fallback (ostream,
			                                              ostream->priv->buffer);
		}

		end_append_text_to_document (ostream);
		ostream->priv->is_closed = TRUE;
	}

	return TRUE;
}
