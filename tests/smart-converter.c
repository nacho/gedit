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

static gchar *
get_text_with_encoding (const gchar *text,
			const GeditEncoding *encoding)
{
	GCharsetConverter *converter;
	GConverterResult res;
	gchar *conv_text;
	gsize read, written;
	GError *err = NULL;

	converter = g_charset_converter_new (gedit_encoding_get_charset (encoding),
					     "UTF-8",
					     NULL);

	conv_text = g_malloc (200);

	res = g_converter_convert (G_CONVERTER (converter),
				   text,
				   strlen (text),
				   conv_text,
				   200,
				   G_CONVERTER_INPUT_AT_END,
				   &read,
				   &written,
				   &err);

	g_assert (err == NULL);
	conv_text[written] = '\0';

	return conv_text;
}

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
         GSList      *encodings,
         gsize        nread,
         const gchar *test_out)
{
	GeditSmartCharsetConverter *converter;
	gchar *out;
	gsize bytes_read;
	gsize bytes_written;
	GError *err;

	converter = gedit_smart_charset_converter_new (encodings);

	out = g_malloc (200);
	err = NULL;

	g_converter_convert (G_CONVERTER (converter),
	                     test_in,
	                     nread,
	                     out,
	                     200,
	                     G_CONVERTER_INPUT_AT_END,
	                     &bytes_read,
	                     &bytes_written,
	                     &err);

	g_assert (err == NULL);
	out[bytes_written] = '\0';

	g_assert_cmpstr (out, ==, test_out);
}

static void
do_test_with_error (const gchar *test_in,
                    GSList      *encodings,
                    gint         error_code)
{
	GeditSmartCharsetConverter *converter;
	gchar *out;
	gsize len;
	gsize bytes_read;
	gsize bytes_written;
	GError *err;

	converter = gedit_smart_charset_converter_new (encodings);

	out = g_malloc (200);
	len = strlen (test_in);
	err = NULL;

	g_converter_convert (G_CONVERTER (converter),
	                     test_in,
	                     len,
	                     out,
	                     200,
	                     G_CONVERTER_INPUT_AT_END,
	                     &bytes_read,
	                     &bytes_written,
	                     &err);

	g_assert (err->code == error_code);
}
#if 0
static void
do_test_few_memory (const gchar *test_in,
                    const gchar *test_out,
                    GeditNewlineConverterNewlineType type,
                    gint memory) /* less than 200 */
{
  GeditConverter *converter;
  GConverterResult res;
  gchar *out;
  gchar *out_aux;
  gsize len;
  gsize bytes_read;
  gsize bytes_read_aux;
  gsize bytes_written;
  gsize bytes_written_aux;
  GError *err;
  gint i;

  converter = gedit_newline_converter_new ();
  gedit_newline_converter_set_newline_type (converter, type);

  out = g_malloc (200);
  out_aux = g_malloc (memory);
  len = strlen (test_in);
  err = NULL;
  i = 0;
  bytes_read_aux = 0;
  bytes_written_aux = 0;

  do
    {
      res = g_converter_convert (G_CONVERTER (converter),
                                 test_in + bytes_read_aux,
                                 len,
                                 out_aux,
                                 memory,
                                 G_CONVERTER_INPUT_AT_END,
                                 &bytes_read,
                                 &bytes_written,
                                 &err);
      memcpy (out + bytes_written_aux, out_aux, bytes_written);
      bytes_read_aux += bytes_read;
      bytes_written_aux += bytes_written;
      len -= bytes_read;
    } while (res != G_CONVERTER_FINISHED && res != G_CONVERTER_ERROR);

  g_assert (err == NULL);
  out[bytes_written_aux] = '\0';
  g_assert_cmpstr (out, ==, test_out);
}
#endif

static void
test_utf8_utf8 ()
{
	GSList *encs = NULL;

	encs = g_slist_prepend (encs, (gpointer)gedit_encoding_get_utf8 ());

	do_test (TEXT_TO_CONVERT, encs, strlen (TEXT_TO_CONVERT), TEXT_TO_CONVERT);

	do_test ("foobar\xc3\xa8\xc3\xa8\xc3\xa8zzzzzz", encs, 18, "foobar\xc3\xa8\xc3\xa8\xc3\xa8zzzzzz");
	do_test ("foobar\xc3\xa8\xc3\xa8\xc3\xa8zzzzzz", encs, 9, "foobar\xc3\xa8\xc3");

	/* FIXME: Use the utf8 stream for a fallback? */
	//do_test_with_error ("\xef\xbf\xbezzzzzz", encs, G_IO_ERROR_FAILED);

	g_slist_free (encs);
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
		gchar *text;
		GSList *test_enc = NULL;

		text = get_text_with_encoding (TEXT_TO_CONVERT, (const GeditEncoding *)l->data);
		test_enc = g_slist_prepend (test_enc, l->data);

		//do_test (text, test_enc, TEXT_TO_CONVERT);
		g_slist_free (test_enc);
		g_free (text);
	}
}

int main (int   argc,
          char *argv[])
{
	g_type_init ();
	g_test_init (&argc, &argv, NULL);

	g_test_add_func ("/smart-converter/utf8-utf8", test_utf8_utf8);
	//g_test_add_func ("/smart-converter/xxx-xxx", test_xxx_xxx);

	return g_test_run ();
}
