/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-convert.c
 * This file is part of gedit
 *
 * Copyright (C) 2003 - Paolo Maggi 
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
 * Boston, MA 02111-1307, USA. * *
 */
 
/*
 * Modified by the gedit Team, 2003. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 */

#include <string.h>
#include <stdio.h>

#include "gedit-convert.h"
#include "gedit-encodings.h"
#include "gedit-debug.h"
#include "gedit-prefs-manager.h"

static gchar *
gedit_convert_to_utf8_from_charset (const gchar *content,
				    gsize        len,
				    const gchar *charset)
{
	gchar *utf8_content = NULL;
	GError *conv_error = NULL;
	gchar* converted_contents = NULL;
	gsize bytes_written;

	g_return_val_if_fail (content != NULL, NULL);

	gedit_debug (DEBUG_UTILS, "Trying to convert from %s to UTF-8", charset);
			
	converted_contents = g_convert (content, len, "UTF-8",
					charset, NULL, &bytes_written,
					&conv_error); 
						
	if ((conv_error != NULL) || 
	    !g_utf8_validate (converted_contents, bytes_written, NULL))		
	{
		gedit_debug (DEBUG_UTILS, "Couldn't convert from %s to UTF-8.",
			     charset);
				
		if (converted_contents != NULL)
			g_free (converted_contents);

		if (conv_error != NULL)
		{
			g_error_free (conv_error);
			conv_error = NULL;
		}

		utf8_content = NULL;
	} 
	else 
	{
		gedit_debug (DEBUG_UTILS,
			"Converted from %s to UTF-8.",
			charset);
	
		utf8_content = converted_contents;
	}

	return utf8_content;
}

gchar *
gedit_convert_to_utf8 (const gchar  *content, 
		       gsize         len,
		       gchar       **encoding_used)
{
	GSList *encodings = NULL;
	GSList *start;
	const gchar *locale_charset;

	g_return_val_if_fail (!g_utf8_validate (content, len, NULL), 
			g_strndup (content, len < 0 ? strlen (content) : len));

	encodings = gedit_prefs_manager_get_encodings ();

	if (g_get_charset (&locale_charset) == FALSE) 
	{
		const GeditEncoding *locale_encoding;

		/* not using a UTF-8 locale, so try converting
		 * from that first */
		if (locale_charset != NULL)
		{
			locale_encoding = gedit_encoding_get_from_charset (locale_charset);
			encodings = g_slist_prepend (encodings,
						(gpointer)locale_encoding);
		}
	}

	start = encodings;

	while (encodings != NULL) 
	{
		GeditEncoding *enc;
		const gchar *charset;
		gchar *utf8_content;

		enc = (GeditEncoding *)encodings->data;

		charset = gedit_encoding_get_charset (enc);

		gedit_debug (DEBUG_UTILS, "Trying to convert %ld bytes of data into UTF-8.", len);
		fflush (stdout);
		utf8_content = gedit_convert_to_utf8_from_charset (content, len, charset);

		if (utf8_content != NULL) 
		{
			if (encoding_used != NULL)
			{
				if (*encoding_used != NULL)
					g_free (*encoding_used);
				
				*encoding_used = g_strdup (charset);
			}

			return utf8_content;
		}

		encodings = encodings->next;
	}

	g_slist_free (start);
	
	return NULL;
}

