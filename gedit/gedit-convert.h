/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-convert.h
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

#ifndef __GEDIT_CONVERT_H__
#define __GEDIT_CONVERT_H__

#include <glib.h>
#include <gedit/gedit-encodings.h>

typedef enum 
{
	GEDIT_CONVERT_ERROR_AUTO_DETECTION_FAILED = 1100,
	GEDIT_CONVERT_ERROR_BINARY_FILE,
	GEDIT_CONVERT_ERROR_ILLEGAL_SEQUENCE
} GeditConvertError;

#define GEDIT_CONVERT_ERROR gedit_convert_error_quark()
GQuark gedit_convert_error_quark (void);


gchar *gedit_convert_to_utf8   (const gchar          *content, 
				gsize                 len,
				const GeditEncoding **encoding,
				GError              **error);

gchar *gedit_convert_from_utf8 (const gchar          *content, 
				gsize                 len,
				const GeditEncoding  *encoding,
				gsize                *new_len,
				GError              **error);

#endif /* __GEDIT_CONVERT_H__ */

