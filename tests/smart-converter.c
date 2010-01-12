/*
 * smart-converter.c
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


#include "gedit-smart-charset-converter.h"
#include "gedit-encodings.h"
#include <gio/gio.h>
#include <glib.h>
#include <string.h>

#define TEXT_TO_CONVERT "this is some text to make the tests"

static GSList *
get_all_encodings ()
{
	GSList *encs = NULL;
	gint i = 0;

	while (TRUE)
	{
		const GeditEncoding *enc;

		enc = gedit_encoding_get_from_index (i);

		if (enc == NULL)
			break;

		encs = g_slist_prepend (encs, (gpointer)enc);
		i++;
	}

	return encs;
}

static void
do_test (const gchar *test_in,
         const gchar *enc,
         GSList      *encodings,
         gsize        nread,
         const gchar *test_out)
{
	GeditSmartCharsetConverter *converter;
	gchar *out, *out_aux;
	gsize bytes_read, bytes_read_aux;
	gsize bytes_written, bytes_written_aux;
	GConverterResult res;
	GError *err;

	if (enc != NULL)
	{
		encodings = NULL;
		encodings = g_slist_prepend (encodings, (gpointer)gedit_encoding_get_from_charset (enc));
	}

	converter = gedit_smart_charset_converter_new (encodings);

	out = g_malloc (200);
	out_aux = g_malloc (200);
	err = NULL;
	bytes_read_aux = 0;
	bytes_written_aux = 0;

	do
	{
		res = g_converter_convert (G_CONVERTER (converter),
		                           test_in + bytes_read_aux,
		                           nread,
		                           out_aux,
		                           200,
		                           G_CONVERTER_INPUT_AT_END,
		                           &bytes_read,
		                           &bytes_written,
		                           &err);
		memcpy (out + bytes_written_aux, out_aux, bytes_written);
		bytes_read_aux += bytes_read;
		bytes_written_aux += bytes_written;
		nread -= bytes_read;
	} while (res != G_CONVERTER_FINISHED && res != G_CONVERTER_ERROR);

	g_assert_no_error (err);
	out[bytes_written_aux] = '\0';

	g_assert_cmpstr (out, ==, test_out);
}

static void
do_test_roundtrip (const char *str, const char *charset)
{
	gsize len;
	gchar *buf, *p;
	GInputStream *in, *tmp;
	GCharsetConverter *c1;
	GeditSmartCharsetConverter *c2;
	gsize n, tot;
	GError *err;
	GSList *enc = NULL;

	len = strlen(str);
	buf = g_new0 (char, len);

	in = g_memory_input_stream_new_from_data (str, -1, NULL);

	c1 = g_charset_converter_new (charset, "UTF-8", NULL);

	tmp = in;
	in = g_converter_input_stream_new (in, G_CONVERTER (c1));
	g_object_unref (tmp);
	g_object_unref (c1);

	enc = g_slist_prepend (enc, (gpointer)gedit_encoding_get_from_charset (charset));
	c2 = gedit_smart_charset_converter_new (enc);
	g_slist_free (enc);

	tmp = in;
	in = g_converter_input_stream_new (in, G_CONVERTER (c2));
	g_object_unref (tmp);
	g_object_unref (c2);

	tot = 0;
	p = buf;
	n = len;
	while (TRUE)
	{
		gssize res;

		err = NULL;
		res = g_input_stream_read (in, p, n, NULL, &err);
		g_assert_no_error (err);
		if (res == 0)
		break;

		p += res;
		n -= res;
		tot += res;
	}

	g_assert_cmpint (tot, ==, len);
	g_assert_cmpstr (str, ==, buf);

	g_free (buf);
	g_object_unref (in);
}

static void
test_utf8_utf8 ()
{
	do_test (TEXT_TO_CONVERT, "UTF-8", NULL, strlen (TEXT_TO_CONVERT), TEXT_TO_CONVERT);

	do_test ("foobar\xc3\xa8\xc3\xa8\xc3\xa8zzzzzz", "UTF-8", NULL, 18, "foobar\xc3\xa8\xc3\xa8\xc3\xa8zzzzzz");
	do_test ("foobar\xc3\xa8\xc3\xa8\xc3\xa8zzzzzz", "UTF-8", NULL, 9, "foobar\xc3\xa8\xc3");

	/* FIXME: Use the utf8 stream for a fallback? */
	//do_test_with_error ("\xef\xbf\xbezzzzzz", encs, G_IO_ERROR_FAILED);
}

static void
test_xxx_xxx ()
{
	GSList *encs, *l;

	encs = get_all_encodings ();

	/* Here we just test all encodings it is just to know that the conversions
	   are done ok */
	for (l = encs; l != NULL; l = g_slist_next (l))
	{
		do_test_roundtrip (TEXT_TO_CONVERT, gedit_encoding_get_charset ((const GeditEncoding *)l->data));
	}

	g_slist_free (encs);
}

int main (int   argc,
          char *argv[])
{
	g_type_init ();
	g_test_init (&argc, &argv, NULL);

	g_test_add_func ("/smart-converter/utf8-utf8", test_utf8_utf8);
	g_test_add_func ("/smart-converter/xxx-xxx", test_xxx_xxx);

	return g_test_run ();
}
