/*
 * Copyright (C) 2009 Ignacio Casal Quinteiro
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#include "gedit-newline-converter.h"
#include <gio/gio.h>
#include <glib.h>
#include <string.h>

static void
do_test (const gchar *test_in,
         const gchar *test_out,
         GeditNewlineConverterNewlineType type)
{
  GeditNewlineConverter *converter;
  gchar *out;
  gsize len;
  gsize bytes_read;
  gsize bytes_written;
  GError *err;

  converter = gedit_newline_converter_new ();
  gedit_newline_converter_set_newline_type (converter, type);

  out = g_malloc (200);
  len = strlen (test_in);
  err = NULL;

  g_converter_convert (G_CONVERTER (converter),
                       test_in,
                       len,
                       out,
                       200,
                       0,
                       &bytes_read,
                       &bytes_written,
                       &err);

  g_assert (err == NULL);
  out[bytes_written] = '\0';
  g_assert_cmpstr (out, ==, test_out);
}

static void
do_test_few_memory (const gchar *test_in,
                    const gchar *test_out,
                    GeditNewlineConverterNewlineType type,
                    gint memory) /* less than 200 */
{
  GeditNewlineConverter *converter;
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

static void
test_lf_lf ()
{
  do_test ("hello\nhello\n", "hello\nhello\n", GEDIT_NEWLINE_CONVERTER_NEWLINE_TYPE_LF);
  do_test ("hello\nhello\nblah", "hello\nhello\nblah", GEDIT_NEWLINE_CONVERTER_NEWLINE_TYPE_LF);
  do_test_few_memory ("hello\nhello\n", "hello\nhello\n", GEDIT_NEWLINE_CONVERTER_NEWLINE_TYPE_LF, 5);
  do_test_few_memory ("hello\nhello\nblah", "hello\nhello\nblah", GEDIT_NEWLINE_CONVERTER_NEWLINE_TYPE_LF, 5);
}

static void
test_cr_cr ()
{
  do_test ("hello\rhello\r", "hello\rhello\r", GEDIT_NEWLINE_CONVERTER_NEWLINE_TYPE_CR);
  do_test ("hello\rhello\rblah", "hello\rhello\rblah", GEDIT_NEWLINE_CONVERTER_NEWLINE_TYPE_CR);
  do_test_few_memory ("hello\rhello\r", "hello\rhello\r", GEDIT_NEWLINE_CONVERTER_NEWLINE_TYPE_CR, 4);
  do_test_few_memory ("hello\rhello\rblah", "hello\rhello\rblah", GEDIT_NEWLINE_CONVERTER_NEWLINE_TYPE_CR, 4);
}

static void
test_lf_cr ()
{
  do_test ("hello\nhello\n", "hello\rhello\r", GEDIT_NEWLINE_CONVERTER_NEWLINE_TYPE_CR);
  do_test ("hello\nhello\nblah", "hello\rhello\rblah", GEDIT_NEWLINE_CONVERTER_NEWLINE_TYPE_CR);
  do_test_few_memory ("hello\nhello\n", "hello\rhello\r", GEDIT_NEWLINE_CONVERTER_NEWLINE_TYPE_CR, 3);
  do_test_few_memory ("hello\nhello\nblah", "hello\rhello\rblah", GEDIT_NEWLINE_CONVERTER_NEWLINE_TYPE_CR, 3);
}

static void
test_cr_lf ()
{
  do_test ("\r\rhello\rhello\r", "\n\nhello\nhello\n", GEDIT_NEWLINE_CONVERTER_NEWLINE_TYPE_LF);
  do_test ("hello\rhello\rblah", "hello\nhello\nblah", GEDIT_NEWLINE_CONVERTER_NEWLINE_TYPE_LF);
  do_test_few_memory ("hello\rhello\r", "hello\nhello\n", GEDIT_NEWLINE_CONVERTER_NEWLINE_TYPE_LF, 2);
  do_test_few_memory ("hello\rhello\rblah", "hello\nhello\nblah", GEDIT_NEWLINE_CONVERTER_NEWLINE_TYPE_LF, 2);
}

static void
test_crlf_lf ()
{
  do_test ("hello\r\nhello\r\nblah", "hello\nhello\nblah", GEDIT_NEWLINE_CONVERTER_NEWLINE_TYPE_LF);
  do_test ("hello\r\nhello\r\n", "hello\nhello\n", GEDIT_NEWLINE_CONVERTER_NEWLINE_TYPE_LF);
  do_test_few_memory ("hello\r\nhello\r\nblah", "hello\nhello\nblah", GEDIT_NEWLINE_CONVERTER_NEWLINE_TYPE_LF, 2);
  do_test_few_memory ("hello\r\nhello\r\n", "hello\nhello\n", GEDIT_NEWLINE_CONVERTER_NEWLINE_TYPE_LF, 2);
}

static void
test_cr_crlf ()
{
  do_test ("hello\rhello\r", "hello\r\nhello\r\n", GEDIT_NEWLINE_CONVERTER_NEWLINE_TYPE_CR_LF);
  do_test ("hello\rhello\rblah", "hello\r\nhello\r\nblah", GEDIT_NEWLINE_CONVERTER_NEWLINE_TYPE_CR_LF);
  do_test_few_memory ("hello\rhello\r", "hello\r\nhello\r\n", GEDIT_NEWLINE_CONVERTER_NEWLINE_TYPE_CR_LF, 3);
  do_test_few_memory ("hello\rhello\rblah", "hello\r\nhello\r\nblah", GEDIT_NEWLINE_CONVERTER_NEWLINE_TYPE_CR_LF, 3);
}

static void
test_crlf_crlf ()
{
  do_test ("hello\r\nhello\r\n", "hello\r\nhello\r\n", GEDIT_NEWLINE_CONVERTER_NEWLINE_TYPE_CR_LF);
  do_test ("hello\r\nhello\r\nblah", "hello\r\nhello\r\nblah", GEDIT_NEWLINE_CONVERTER_NEWLINE_TYPE_CR_LF);
}

static void
test_mixed ()
{
  do_test ("hello\n\r\r\nhello\r\n", "hello\n\n\nhello\n", GEDIT_NEWLINE_CONVERTER_NEWLINE_TYPE_LF);
  do_test ("hello\n\r\r\nhello\r\n", "hello\r\n\r\n\r\nhello\r\n", GEDIT_NEWLINE_CONVERTER_NEWLINE_TYPE_CR_LF);
}

int main (int   argc,
          char *argv[])
{
  g_type_init ();
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/newline-converter/lf-lf", test_lf_lf);
  g_test_add_func ("/newline-converter/cr-cr", test_cr_cr);
  g_test_add_func ("/newline-converter/lf-cr", test_lf_cr);
  g_test_add_func ("/newline-converter/cr-lf", test_cr_lf);
  g_test_add_func ("/newline-converter/crlf-cr", test_crlf_lf);
  g_test_add_func ("/newline-converter/lf-crlf", test_cr_crlf);
  g_test_add_func ("/newline-converter/crlf-crlf", test_crlf_crlf);

  return g_test_run ();
}
