/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-encodings.h
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

#ifndef __GEDIT_ENCODINGS_H__
#define __GEDIT_ENCODINGS_H__

#include <glib.h>

typedef struct _GeditEncoding GeditEncoding;

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

const GeditEncoding			*gedit_encoding_get_from_charset 	(const gchar *charset);
const GeditEncoding			*gedit_encoding_get_from_index  	(gint indext);

gchar 					*gedit_encoding_to_string		(const GeditEncoding* enc);
const gchar				*gedit_encoding_get_charset		(const GeditEncoding* enc);



#endif  /* __GEDIT_ENCODINGS_H__ */


