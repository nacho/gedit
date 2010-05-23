/*
 * gedit-message.h
 * This file is part of gedit
 *
 * Copyright (C) 2008, 2010 - Jesse van den Kieboom
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

#ifndef __GEDIT_MESSAGE_H__
#define __GEDIT_MESSAGE_H__

#include <glib-object.h>
#include <stdarg.h>

G_BEGIN_DECLS

#define GEDIT_TYPE_MESSAGE			(gedit_message_get_type ())
#define GEDIT_MESSAGE(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_MESSAGE, GeditMessage))
#define GEDIT_MESSAGE_CONST(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_MESSAGE, GeditMessage const))
#define GEDIT_MESSAGE_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), GEDIT_TYPE_MESSAGE, GeditMessageClass))
#define GEDIT_IS_MESSAGE(obj)			(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEDIT_TYPE_MESSAGE))
#define GEDIT_IS_MESSAGE_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE ((klass), GEDIT_TYPE_MESSAGE))
#define GEDIT_MESSAGE_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS ((obj), GEDIT_TYPE_MESSAGE, GeditMessageClass))

typedef struct _GeditMessage		GeditMessage;
typedef struct _GeditMessageClass	GeditMessageClass;
typedef struct _GeditMessagePrivate	GeditMessagePrivate;

struct _GeditMessage
{
	GObject parent;
	
	GeditMessagePrivate *priv;
};

struct _GeditMessageClass
{
	GObjectClass parent_class;
};

GType		 gedit_message_get_type			(void) G_GNUC_CONST;

struct _GeditMessageType gedit_message_get_message_type (GeditMessage  *message);

void		 gedit_message_get			(GeditMessage  *message,
							 ...) G_GNUC_NULL_TERMINATED;
void		 gedit_message_get_valist		(GeditMessage  *message,
							 va_list        var_args);
void		 gedit_message_get_value		(GeditMessage  *message,
							 const gchar   *key,
							 GValue        *value);

void		 gedit_message_set			(GeditMessage  *message,
							 ...) G_GNUC_NULL_TERMINATED;
void		 gedit_message_set_valist		(GeditMessage  *message,
							 va_list        var_args);
void		 gedit_message_set_value		(GeditMessage  *message,
							 const gchar   *key,
							 GValue        *value);
void		 gedit_message_set_valuesv		(GeditMessage  *message,
							 const gchar  **keys,
							 GValue        *values,
							 gint           n_values);

const gchar	*gedit_message_get_object_path		(GeditMessage  *message);
const gchar	*gedit_message_get_method		(GeditMessage  *message);

gboolean	 gedit_message_has_key			(GeditMessage  *message,
							 const gchar   *key);

GType		 gedit_message_get_key_type 		(GeditMessage  *message,
							 const gchar   *key);

gboolean	 gedit_message_validate			(GeditMessage  *message);

G_END_DECLS

#endif /* __GEDIT_MESSAGE_H__ */

/* ex:ts=8:noet: */
