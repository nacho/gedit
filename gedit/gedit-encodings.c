/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-encodings.c
 * This file is part of gedit
 *
 * Copyright (C) 2002 Paolo Maggi 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, 
 * Boston, MA 02111-1307, USA.
 */
 
/*
 * Modified by the gedit Team, 2002. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 */

#include "gedit-encodings.h"

#include <bonobo/bonobo-i18n.h>
#include <string.h>

struct _GeditEncoding
{
  gint   index;
  gchar *charset;
  gchar *name;
};

/* 
 * The original versions of the following tables are taken from profterm 
 *
 * Copyright (C) 2002 Red Hat, Inc.
 */

typedef enum
{

  GEDIT_ENCODING_ISO_8859_1,
  GEDIT_ENCODING_ISO_8859_3,
  GEDIT_ENCODING_ISO_8859_4,
  GEDIT_ENCODING_ISO_8859_5,
  GEDIT_ENCODING_ISO_8859_6,
  GEDIT_ENCODING_ISO_8859_7,
  GEDIT_ENCODING_ISO_8859_8,
  GEDIT_ENCODING_ISO_8859_8_I,
  GEDIT_ENCODING_ISO_8859_9,
  GEDIT_ENCODING_ISO_8859_10,
  GEDIT_ENCODING_ISO_8859_13,
  GEDIT_ENCODING_ISO_8859_14,
  GEDIT_ENCODING_ISO_8859_15,
  GEDIT_ENCODING_ISO_8859_16,

  GEDIT_ENCODING_UTF_7,
  GEDIT_ENCODING_UTF_16,
  GEDIT_ENCODING_UCS_2,
  GEDIT_ENCODING_UCS_4,

  GEDIT_ENCODING_ARMSCII_8,
  GEDIT_ENCODING_BIG5,
  GEDIT_ENCODING_BIG5_HKSCS,
  GEDIT_ENCODING_CP_866,

  GEDIT_ENCODING_EUC_JP,
  GEDIT_ENCODING_EUC_KR,
  GEDIT_ENCODING_EUC_TW,

  GEDIT_ENCODING_GB18030,
  GEDIT_ENCODING_GB2312,
  GEDIT_ENCODING_GBK,
  GEDIT_ENCODING_GEOSTD8,
  GEDIT_ENCODING_HZ,

  GEDIT_ENCODING_IBM_850,
  GEDIT_ENCODING_IBM_852,
  GEDIT_ENCODING_IBM_855,
  GEDIT_ENCODING_IBM_857,
  GEDIT_ENCODING_IBM_862,
  GEDIT_ENCODING_IBM_864,

  GEDIT_ENCODING_ISO_2022_JP,
  GEDIT_ENCODING_ISO_2022_KR,
  GEDIT_ENCODING_ISO_IR_111,
  GEDIT_ENCODING_JOHAB,
  GEDIT_ENCODING_KOI8_R,
  GEDIT_ENCODING_KOI8_U,

#if 0
  /* GLIBC iconv doesn't seem to have these mac things */
  GEDIT_ENCODING_MAC_ARABIC,
  GEDIT_ENCODING_MAC_CE,
  GEDIT_ENCODING_MAC_CROATIAN,
  GEDIT_ENCODING_MAC_CYRILLIC,
  GEDIT_ENCODING_MAC_DEVANAGARI,
  GEDIT_ENCODING_MAC_FARSI,
  GEDIT_ENCODING_MAC_GREEK,
  GEDIT_ENCODING_MAC_GUJARATI,
  GEDIT_ENCODING_MAC_GURMUKHI,
  GEDIT_ENCODING_MAC_HEBREW,
  GEDIT_ENCODING_MAC_ICELANDIC,
  GEDIT_ENCODING_MAC_ROMAN,
  GEDIT_ENCODING_MAC_ROMANIAN,
  GEDIT_ENCODING_MAC_TURKISH,
  GEDIT_ENCODING_MAC_UKRAINIAN,
#endif
  
  GEDIT_ENCODING_SHIFT_JIS,
  GEDIT_ENCODING_TCVN,
  GEDIT_ENCODING_TIS_620,
  GEDIT_ENCODING_UHC,
  GEDIT_ENCODING_VISCII,

  GEDIT_ENCODING_WINDOWS_1250,
  GEDIT_ENCODING_WINDOWS_1251,
  GEDIT_ENCODING_WINDOWS_1252,
  GEDIT_ENCODING_WINDOWS_1253,
  GEDIT_ENCODING_WINDOWS_1254,
  GEDIT_ENCODING_WINDOWS_1255,
  GEDIT_ENCODING_WINDOWS_1256,
  GEDIT_ENCODING_WINDOWS_1257,
  GEDIT_ENCODING_WINDOWS_1258,

  GEDIT_ENCODING_LAST
  
} GeditEncodingIndex;

static GeditEncoding encodings [] = {

  { GEDIT_ENCODING_ISO_8859_1,
    "ISO-8859-1", N_("Western") },
  { GEDIT_ENCODING_ISO_8859_3,
    "ISO-8859-3", N_("South European") },
  { GEDIT_ENCODING_ISO_8859_4,
    "ISO-8859-4", N_("Baltic") },
  { GEDIT_ENCODING_ISO_8859_5,
    "ISO-8859-5", N_("Cyrillic") },
  { GEDIT_ENCODING_ISO_8859_6,
    "ISO-8859-6", N_("Arabic") },
  { GEDIT_ENCODING_ISO_8859_7,
    "ISO-8859-7", N_("Greek") },
  { GEDIT_ENCODING_ISO_8859_8,
    "ISO-8859-8", N_("Hebrew Visual") },
  { GEDIT_ENCODING_ISO_8859_8_I,
    "ISO-8859-8-I", N_("Hebrew") },
  { GEDIT_ENCODING_ISO_8859_9,
    "ISO-8859-9", N_("Turkish") },
  { GEDIT_ENCODING_ISO_8859_10,
    "ISO-8859-10", N_("Nordic") },
  { GEDIT_ENCODING_ISO_8859_13,
    "ISO-8859-13", N_("Baltic") },
  { GEDIT_ENCODING_ISO_8859_14,
    "ISO-8859-14", N_("Celtic") },
  { GEDIT_ENCODING_ISO_8859_15,
    "ISO-8859-15", N_("Western") },
  { GEDIT_ENCODING_ISO_8859_16,
    "ISO-8859-16", N_("Romanian") },

  { GEDIT_ENCODING_UTF_7,
    "UTF-7", N_("Unicode") },
  { GEDIT_ENCODING_UTF_16,
    "UTF-16", N_("Unicode") },
  { GEDIT_ENCODING_UCS_2,
    "UCS-2", N_("Unicode") },
  { GEDIT_ENCODING_UCS_4,
    "UCS-4", N_("Unicode") },

  { GEDIT_ENCODING_ARMSCII_8,
    "ARMSCII-8", N_("Armenian") },
  { GEDIT_ENCODING_BIG5,
    "BIG5", N_("Chinese Traditional") },
  { GEDIT_ENCODING_BIG5_HKSCS,
    "BIG5-HKSCS", N_("Chinese Traditional") },
  { GEDIT_ENCODING_CP_866,
    "CP866", N_("Cyrillic/Russian") },

  { GEDIT_ENCODING_EUC_JP,
    "EUC-JP", N_("Japanese") },
  { GEDIT_ENCODING_EUC_KR,
    "EUC-KR", N_("Korean") },
  { GEDIT_ENCODING_EUC_TW,
    "EUC-TW", N_("Chinese Traditional") },

  { GEDIT_ENCODING_GB18030,
    "GB18030", N_("Chinese Simplified") },
  { GEDIT_ENCODING_GB2312,
    "GB2312", N_("Chinese Simplified") },
  { GEDIT_ENCODING_GBK,
    "GBK", N_("Chinese Simplified") },
  { GEDIT_ENCODING_GEOSTD8,
    "GEORGIAN-ACADEMY", N_("Georgian") }, /* FIXME GEOSTD8 ? */
  { GEDIT_ENCODING_HZ,
    "HZ", N_("Chinese Simplified") },

  { GEDIT_ENCODING_IBM_850,
    "IBM850", N_("Western") },
  { GEDIT_ENCODING_IBM_852,
    "IBM852", N_("Central European") },
  { GEDIT_ENCODING_IBM_855,
    "IBM855", N_("Cyrillic") },
  { GEDIT_ENCODING_IBM_857,
    "IBM857", N_("Turkish") },
  { GEDIT_ENCODING_IBM_862,
    "IBM862", N_("Hebrew") },
  { GEDIT_ENCODING_IBM_864,
    "IBM864", N_("Arabic") },

  { GEDIT_ENCODING_ISO_2022_JP,
    "ISO2022JP", N_("Japanese") },
  { GEDIT_ENCODING_ISO_2022_KR,
    "ISO2022KR", N_("Korean") },
  { GEDIT_ENCODING_ISO_IR_111,
    "ISO-IR-111", N_("Cyrillic") },
  { GEDIT_ENCODING_JOHAB,
    "JOHAB", N_("Korean") },
  { GEDIT_ENCODING_KOI8_R,
    "KOI8R", N_("Cyrillic") },
  { GEDIT_ENCODING_KOI8_U,
    "KOI8U", N_("Cyrillic/Ukrainian") },

#if 0
  /* GLIBC iconv doesn't seem to have these mac things */
  { GEDIT_ENCODING_MAC_ARABIC,
    "MAC_ARABIC", N_("Arabic") },
  { GEDIT_ENCODING_MAC_CE,
    "MAC_CE", N_("Central European") },
  { GEDIT_ENCODING_MAC_CROATIAN,
    "MAC_CROATIAN", N_("Croatian") },
  { GEDIT_ENCODING_MAC_CYRILLIC,
    "MAC-CYRILLIC", N_("Cyrillic") },
  { GEDIT_ENCODING_MAC_DEVANAGARI,
    "MAC_DEVANAGARI", N_("Hindi") },
  { GEDIT_ENCODING_MAC_FARSI,
    "MAC_FARSI", N_("Farsi") },
  { GEDIT_ENCODING_MAC_GREEK,
    "MAC_GREEK", N_("Greek") },
  { GEDIT_ENCODING_MAC_GUJARATI,
    "MAC_GUJARATI", N_("Gujarati") },
  { GEDIT_ENCODING_MAC_GURMUKHI,
    "MAC_GURMUKHI", N_("Gurmukhi") },
  { GEDIT_ENCODING_MAC_HEBREW,
    "MAC_HEBREW", N_("Hebrew") },
  { GEDIT_ENCODING_MAC_ICELANDIC,
    "MAC_ICELANDIC", N_("Icelandic") },
  { GEDIT_ENCODING_MAC_ROMAN,
    "MAC_ROMAN", N_("Western") },
  { GEDIT_ENCODING_MAC_ROMANIAN,
    "MAC_ROMANIAN", N_("Romanian") },
  { GEDIT_ENCODING_MAC_TURKISH,
    "MAC_TURKISH", N_("Turkish") },
  { GEDIT_ENCODING_MAC_UKRAINIAN,
    "MAC_UKRAINIAN", N_("Cyrillic/Ukrainian") },
#endif
  
  { GEDIT_ENCODING_SHIFT_JIS,
    "SHIFT_JIS", N_("Japanese") },
  { GEDIT_ENCODING_TCVN,
    "TCVN", N_("Vietnamese") },
  { GEDIT_ENCODING_TIS_620,
    "TIS-620", N_("Thai") },
  { GEDIT_ENCODING_UHC,
    "UHC", N_("Korean") },
  { GEDIT_ENCODING_VISCII,
    "VISCII", N_("Vietnamese") },

  { GEDIT_ENCODING_WINDOWS_1250,
    "WINDOWS-1250", N_("Central European") },
  { GEDIT_ENCODING_WINDOWS_1251,
    "WINDOWS-1251", N_("Cyrillic") },
  { GEDIT_ENCODING_WINDOWS_1252,
    "WINDOWS-1252", N_("Western") },
  { GEDIT_ENCODING_WINDOWS_1253,
    "WINDOWS-1253", N_("Greek") },
  { GEDIT_ENCODING_WINDOWS_1254,
    "WINDOWS-1254", N_("Turkish") },
  { GEDIT_ENCODING_WINDOWS_1255,
    "WINDOWS-1255", N_("Hebrew") },
  { GEDIT_ENCODING_WINDOWS_1256,
    "WINDOWS-1256", N_("Arabic") },
  { GEDIT_ENCODING_WINDOWS_1257,
    "WINDOWS-1257", N_("Baltic") },
  { GEDIT_ENCODING_WINDOWS_1258,
    "WINDOWS-1258", N_("Vietnamese") }
};

static void
gedit_encoding_lazy_init (void)
{

	static gboolean initialized = FALSE;
	gint i;
	
	if (initialized)
		return;

	g_return_if_fail (G_N_ELEMENTS (encodings) == GEDIT_ENCODING_LAST);
  
	i = 0;
	while (i < GEDIT_ENCODING_LAST)
	{
		g_return_if_fail (encodings[i].index == i);

		/* Translate the names */
		encodings[i].name = _(encodings[i].name);
      
		++i;
    	}

	initialized = TRUE;
}

const GeditEncoding *
gedit_encoding_get_from_charset (const gchar *charset)
{
	gint i;

	gedit_encoding_lazy_init ();

	i = 0; 
	while (i < GEDIT_ENCODING_LAST)
	{
		if (strcmp (charset, encodings[i].charset) == 0)
			return &encodings[i];
      
		++i;
	}
 
	return NULL;
}

const GeditEncoding *
gedit_encoding_get_from_index (gint index)
{
	g_return_val_if_fail (index >= 0, NULL);

	if (index >= GEDIT_ENCODING_LAST)
		return NULL;

	gedit_encoding_lazy_init ();

	return &encodings [index];
}


gchar *
gedit_encoding_to_string (const GeditEncoding* enc)
{
	g_return_val_if_fail (enc != NULL, NULL);
	g_return_val_if_fail (enc->name != NULL, NULL);
	g_return_val_if_fail (enc->charset != NULL, NULL);

	gedit_encoding_lazy_init ();

    	return g_strdup_printf ("%s (%s)", enc->name, enc->charset);
}

const gchar *
gedit_encoding_get_charset (const GeditEncoding* enc)
{
	g_return_val_if_fail (enc != NULL, NULL);
	g_return_val_if_fail (enc->charset != NULL, NULL);

	gedit_encoding_lazy_init ();

	return enc->charset;
}

