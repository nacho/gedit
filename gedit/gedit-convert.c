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
 * Boston, MA 02111-1307, USA. 
 */
 
/*
 * Modified by the gedit Team, 2003. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 */

#include <string.h>
#include <stdio.h>

#include <libgnome/gnome-i18n.h>

#include "gedit-convert.h"
#include "gedit-encodings.h"
#include "gedit-debug.h"
#include "gedit-prefs-manager.h"

GQuark 
gedit_convert_error_quark (void)
{
	static GQuark quark;
	if (!quark)
		quark = g_quark_from_static_string ("gedit_convert_error");

	return quark;
}

static gchar *
gedit_convert_to_utf8_from_charset (const gchar  *content,
				    gsize         len,
				    const gchar  *charset,
				    GError 	**error)
{
	gchar *utf8_content = NULL;
	GError *conv_error = NULL;
	gchar* converted_contents;
	gsize bytes_written;

	g_return_val_if_fail (content != NULL, NULL);
	g_return_val_if_fail (len > 0, NULL);
	g_return_val_if_fail (charset != NULL, NULL);

	gedit_debug (DEBUG_UTILS, "Trying to convert from %s to UTF-8", charset);
			
	converted_contents = g_convert (content, 
					len, 
					"UTF-8",
					charset, 
					NULL, 
					&bytes_written,
					&conv_error); 
						
	if ((conv_error != NULL) || !g_utf8_validate (converted_contents, bytes_written, NULL))
	{
		gedit_debug (DEBUG_UTILS, "Couldn't convert from %s to UTF-8.",
			     charset);
				
		if (converted_contents != NULL)
			g_free (converted_contents);

		if (conv_error != NULL)
			g_propagate_error (error, conv_error);		
		else
		{
			gedit_debug (DEBUG_UTILS, "The file you are trying to open contain "
					"an invalid byte sequence.");
			
			g_set_error (error, GEDIT_CONVERT_ERROR, GEDIT_CONVERT_ERROR_ILLEGAL_SEQUENCE,
					"The file you are trying to open contain an invalid byte sequence.");
		}
	} 
	else 
	{
		g_return_val_if_fail (converted_contents != NULL, NULL);

		gedit_debug (DEBUG_UTILS,
			     "Converted from %s to UTF-8.", charset);
	
		utf8_content = converted_contents;
	}

	return utf8_content;
}

gchar *
gedit_convert_to_utf8   (const gchar          *content, 
			 gsize                 len,
			 const GeditEncoding **encoding,
			 GError              **error)
{
	gedit_debug (DEBUG_UTILS, "");
	
	g_return_val_if_fail (content != NULL, NULL);
	g_return_val_if_fail (encoding != NULL, NULL);
	
	if (len < 0)
		len = strlen (content);

	if (*encoding != NULL)
	{
		const gchar* charset;
		
		charset = gedit_encoding_get_charset (*encoding);

		g_return_val_if_fail (charset != NULL, NULL);

		return gedit_convert_to_utf8_from_charset (content, 
							   len,
							   charset,
							   error);
	}
	else
	{
		/* Automally detect used encoding */
		
		GSList *encodings = NULL;
		GSList *start;

		gedit_debug (DEBUG_UTILS,  "Automally detect used encoding");

		encodings = gedit_prefs_manager_get_auto_detected_encodings ();

		if (encodings == NULL)
		{
			gedit_debug (DEBUG_UTILS, "encodings == NULL");
			
			if (g_utf8_validate (content, len, NULL))
			{
				gedit_debug (DEBUG_UTILS, "validate OK.");

				return g_strndup (content, len);
			}
			else
			{
				gedit_debug (DEBUG_UTILS, "validate failed.");

				g_set_error (error, GEDIT_CONVERT_ERROR, 
					     GEDIT_CONVERT_ERROR_AUTO_DETECTION_FAILED,
				 	     "gedit has not been able to automatically determine "
					     "the encoding of the file you want to open.");

				return NULL;
			}
		}

		gedit_debug (DEBUG_UTILS, "encodings != NULL");

		start = encodings;
		
		while (encodings != NULL) 
		{
			const GeditEncoding *enc;
			const gchar *charset;
			gchar *utf8_content;

			enc = (const GeditEncoding *)encodings->data;

			gedit_debug (DEBUG_UTILS, "Get charset");

			charset = gedit_encoding_get_charset (enc);
			g_return_val_if_fail (charset != NULL, NULL);

			gedit_debug (DEBUG_UTILS, "Trying to convert %ld bytes of data from ' %s'to 'UTF-8'.", 
					len, charset);

			utf8_content = gedit_convert_to_utf8_from_charset (content, len, charset, NULL);

			if (utf8_content != NULL) 
			{
				*encoding = enc;

				return utf8_content;
			}

			encodings = g_slist_next (encodings);
		}

		gedit_debug (DEBUG_UTILS, "gedit has not been able to automatically determine the encoding of "
			     "the file you want to open.");

		g_set_error (error, GEDIT_CONVERT_ERROR, GEDIT_CONVERT_ERROR_AUTO_DETECTION_FAILED,
		 	     "gedit has not been able to automatically determine the encoding of "
			     "the file you want to open.");

		g_slist_free (start);
	}
	
	return NULL;
}

gchar *
gedit_convert_from_utf8 (const gchar          *content, 
		         gsize                 len,
		         const GeditEncoding  *encoding,
			 GError 	     **error)
{
	GError *conv_error         = NULL;
	gchar  *converted_contents = NULL;

	gedit_debug (DEBUG_UTILS, "");
	
	g_return_val_if_fail (content != NULL, NULL);
	g_return_val_if_fail (g_utf8_validate (content, len, NULL), NULL);
	g_return_val_if_fail (encoding != NULL, NULL);

	if (len < 0)
		len = strlen (content);

	if (encoding == gedit_encoding_get_utf8 ())
		return g_strndup (content, len);

	converted_contents = g_convert (content, 
					len, 
					gedit_encoding_get_charset (encoding), 
					"UTF-8",
					NULL, 
					NULL,
					&conv_error); 

	if (conv_error != NULL)
	{
		gedit_debug (DEBUG_UTILS, "Cannot convert from UTF-8 to %s",
				gedit_encoding_get_charset (encoding));

		if (converted_contents != NULL)
		{
			g_free (converted_contents);
			converted_contents = NULL;
		}

		g_propagate_error (error, conv_error);
	}

	return converted_contents;
}

