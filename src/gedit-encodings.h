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


const GeditEncoding			*gedit_encoding_get_from_charset 	(const gchar *charset);
const GeditEncoding			*gedit_encoding_get_from_index  	(gint index);

gchar 					*gedit_encoding_to_string		(const GeditEncoding* enc);
const gchar				*gedit_encoding_get_charset		(const GeditEncoding* enc);



#endif  /* __GEDIT_ENCODINGS_H__ */


