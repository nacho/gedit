/*
 * gedit-message-type.h
 * This file is part of gedit
 *
 * Copyright (C) 2008-2010 - Jesse van den Kieboom
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

#ifndef __GEDIT_MESSAGE_TYPE_H__
#define __GEDIT_MESSAGE_TYPE_H__

#include <glib-object.h>
#include <stdarg.h>

#include "gedit-message.h"

G_BEGIN_DECLS

#define GEDIT_TYPE_MESSAGE_TYPE			(gedit_message_type_get_type ())
#define GEDIT_MESSAGE_TYPE(x)			((GeditMessageType *)(x))

typedef void (*GeditMessageTypeForeach)		(const gchar *key, 
						 GType 	      type, 
						 gboolean     required, 
						 gpointer     user_data);

typedef struct _GeditMessageType			GeditMessageType;

GType			 gedit_message_type_get_type 			(void) G_GNUC_CONST;

gboolean		 gedit_message_type_is_supported		(GType             type);
gchar			*gedit_message_type_identifier			(const gchar      *object_path,
									 const gchar      *method);
gboolean		 gedit_message_type_is_valid_object_path	(const gchar      *object_path);

GeditMessageType	*gedit_message_type_new				(const gchar      *object_path,
									 const gchar      *method,
									 guint	           num_optional,
									 ...) G_GNUC_NULL_TERMINATED;
GeditMessageType	*gedit_message_type_new_valist			(const gchar      *object_path,
									 const gchar      *method,
									 guint             num_optional,
									 va_list           var_args);

void			 gedit_message_type_set				(GeditMessageType *message_type,
									 guint             num_optional,
									 ...) G_GNUC_NULL_TERMINATED;
void			 gedit_message_type_set_valist			(GeditMessageType *message_type,
									 guint             num_optional,
									 va_list           var_args);

GeditMessageType	*gedit_message_type_ref				(GeditMessageType *message_type);
void			 gedit_message_type_unref			(GeditMessageType *message_type);


GeditMessage		*gedit_message_type_instantiate_valist		(GeditMessageType *message_type,
									 va_list           va_args);
GeditMessage		*gedit_message_type_instantiate			(GeditMessageType *message_type,
									 ...) G_GNUC_NULL_TERMINATED;

const gchar		*gedit_message_type_get_object_path		(GeditMessageType *message_type);
const gchar		*gedit_message_type_get_method			(GeditMessageType *message_type);

GType			 gedit_message_type_lookup			(GeditMessageType *message_type,
									 const gchar      *key);
						 
void			 gedit_message_type_foreach			(GeditMessageType *message_type,
									 GeditMessageTypeForeach   func,
									 gpointer          user_data);

G_END_DECLS

#endif /* __GEDIT_MESSAGE_TYPE_H__ */

/* ex:set ts=8 noet: */
