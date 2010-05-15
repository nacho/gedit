/*
 * gedit-fifo.c
 * This file is part of gedit
 *
 * Copyright (C) 2010 - Jesse van den Kieboom
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

#include "gedit-fifo.h"
#include <stdio.h>
#include <gio/gunixinputstream.h>
#include <gio/gunixoutputstream.h>
#include <errno.h>
#include <string.h>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <glib.h>
#include <glib/gstdio.h>

#define GEDIT_FIFO_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE((object), GEDIT_TYPE_FIFO, GeditFifoPrivate))

/* Properties */
enum
{
	PROP_0,
	PROP_FILE
};

typedef enum
{
	GEDIT_FIFO_OPEN_MODE_READ,
	GEDIT_FIFO_OPEN_MODE_WRITE
} GeditFifoOpenMode;

struct _GeditFifoPrivate
{
	GFile *file;
	GeditFifoOpenMode open_mode;
};

static void gedit_fifo_initable_iface_init (gpointer giface, gpointer iface_data);

G_DEFINE_TYPE_WITH_CODE (GeditFifo,
                         gedit_fifo,
                         G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE, gedit_fifo_initable_iface_init))

static gboolean
gedit_fifo_initable_init (GInitable     *initable,
                          GCancellable  *cancellable,
                          GError       **error)
{
	g_return_val_if_fail (GEDIT_IS_FIFO (initable), FALSE);

	if (cancellable && g_cancellable_set_error_if_cancelled (cancellable, error))
	{
		return FALSE;
	}

	return GEDIT_FIFO (initable)->priv->file != NULL;
}

static void
gedit_fifo_initable_iface_init (gpointer giface, gpointer iface_data)
{
	GInitableIface *iface = giface;

	iface->init = gedit_fifo_initable_init;
}

static void
gedit_fifo_finalize (GObject *object)
{
	GeditFifo *self = GEDIT_FIFO (object);

	if (self->priv->file)
	{
		if (self->priv->open_mode == GEDIT_FIFO_OPEN_MODE_WRITE)
		{
			gchar *path = g_file_get_path (self->priv->file);
			g_unlink (path);
			g_free (path);
		}

		g_object_unref (self->priv->file);
	}

	G_OBJECT_CLASS (gedit_fifo_parent_class)->finalize (object);
}

static void
gedit_fifo_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	GeditFifo *self = GEDIT_FIFO (object);
	
	switch (prop_id)
	{
		case PROP_FILE:
			self->priv->file = g_value_dup_object (value);
		break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
gedit_fifo_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	GeditFifo *self = GEDIT_FIFO (object);
	
	switch (prop_id)
	{
		case PROP_FILE:
			g_value_set_object (value, self->priv->file);
		break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
init_fifo (GeditFifo *fifo)
{
	gchar tmpl[] = "gedit-fifo.XXXXXX";
	gchar *tmp;
	gint fd;
	GError *error = NULL;

	fd = g_file_open_tmp (tmpl, &tmp, &error);

	if (fd == -1)
	{
		g_warning ("Could not generate temporary name for fifo: %s",
		           error->message);
		g_error_free (error);

		return;
	}

	close (fd);

	if (g_unlink (tmp) == -1)
	{
		return;
	}

	if (mkfifo (tmp, 0600) == -1)
	{
		g_warning ("Could not create named pipe for standard in: %s",
		           strerror (errno));
		return;
	}

	fifo->priv->file = g_file_new_for_path (tmp);
}

static void
gedit_fifo_constructed (GObject *object)
{
	GeditFifo *self = GEDIT_FIFO (object);

	if (!self->priv->file)
	{
		init_fifo (self);
	}
	else if (!g_file_query_exists (self->priv->file, NULL))
	{
		g_object_unref (self->priv->file);
		self->priv->file = NULL;
	}
}

static void
gedit_fifo_class_init (GeditFifoClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = gedit_fifo_finalize;

	object_class->set_property = gedit_fifo_set_property;
	object_class->get_property = gedit_fifo_get_property;

	object_class->constructed = gedit_fifo_constructed;

	g_object_class_install_property (object_class, PROP_FILE,
				 g_param_spec_object ("file",
						      "FILE",
						      "The fifo file",
						      G_TYPE_FILE,
						      G_PARAM_READWRITE |
						      G_PARAM_CONSTRUCT_ONLY |
						      G_PARAM_STATIC_STRINGS));

	g_type_class_add_private (object_class, sizeof(GeditFifoPrivate));
}

static void
gedit_fifo_init (GeditFifo *self)
{
	self->priv = GEDIT_FIFO_GET_PRIVATE (self);
}

GeditFifo *
gedit_fifo_new (GFile *file)
{
	return g_initable_new (GEDIT_TYPE_FIFO, NULL, NULL, "file", file, NULL);
}

static void
fifo_open_in_thread (GSimpleAsyncResult *res,
                     GObject            *object,
                     GCancellable       *cancellable)
{
	GError *error = NULL;
	gchar *path;
	gint fd;
	GeditFifo *fifo;
	gpointer stream;
	gint flags = 0;

	if (cancellable && g_cancellable_set_error_if_cancelled (cancellable, &error))
	{
		g_simple_async_result_set_from_error (res, error);
		g_error_free (error);
		return;
	}

	fifo = GEDIT_FIFO (object);
	
	switch (fifo->priv->open_mode)
	{
		case GEDIT_FIFO_OPEN_MODE_READ:
			flags = O_RDONLY;
		break;
		case GEDIT_FIFO_OPEN_MODE_WRITE:
			flags = O_WRONLY;
		break;
	}
	
	path = g_file_get_path (fifo->priv->file);
	fd = g_open (path, flags, 0);
	g_free (path);

	if (cancellable && g_cancellable_set_error_if_cancelled (cancellable, &error))
	{
		if (fd != -1)
		{
			close (fd);
		}

		g_simple_async_result_set_from_error (res, error);
		g_error_free (error);
		return;
	}

	if (fd == -1)
	{
		g_simple_async_result_set_error (res,
		                                 G_IO_ERROR,
		                                 g_io_error_from_errno (errno),
		                                 "%s",
		                                 strerror (errno));
		return;
	}

	if (fifo->priv->open_mode == GEDIT_FIFO_OPEN_MODE_WRITE)
	{
		stream = g_unix_output_stream_new (fd, TRUE);
	}
	else
	{
		stream = g_unix_input_stream_new (fd, TRUE);
	}

	g_simple_async_result_set_op_res_gpointer (res,
	                                           stream,
	                                           (GDestroyNotify)g_object_unref);
}

static void
async_open (GeditFifo           *fifo,
            GeditFifoOpenMode    open_mode,
            gint                 io_priority,
            GCancellable        *cancellable,
            GAsyncReadyCallback  callback,
            gpointer             user_data)
{
	GSimpleAsyncResult *ret;

	fifo->priv->open_mode = open_mode;

	ret = g_simple_async_result_new (G_OBJECT (fifo),
	                                 callback,
	                                 user_data,
	                                 fifo_open_in_thread);

	g_simple_async_result_run_in_thread (ret,
	                                     fifo_open_in_thread,
	                                     io_priority,
	                                     cancellable);
}

GInputStream *
gedit_fifo_open_read_finish (GeditFifo     *fifo,
                             GAsyncResult  *result,
                             GError       **error)
{
	GSimpleAsyncResult *simple;

	g_return_val_if_fail (GEDIT_IS_FIFO (fifo), NULL);
	g_return_val_if_fail (g_simple_async_result_is_valid (result,
	                                                      G_OBJECT (fifo),
	                                                      fifo_open_in_thread),
	                      NULL);

	simple = G_SIMPLE_ASYNC_RESULT (result);

	if (g_simple_async_result_propagate_error (simple, error))
	{
		return NULL;
	}

	return G_INPUT_STREAM (g_object_ref (g_simple_async_result_get_op_res_gpointer (simple)));
}

GOutputStream *
gedit_fifo_open_write_finish (GeditFifo     *fifo,
                              GAsyncResult  *result,
                              GError       **error)
{
	GSimpleAsyncResult *simple;

	g_return_val_if_fail (GEDIT_IS_FIFO (fifo), NULL);
	g_return_val_if_fail (g_simple_async_result_is_valid (result,
	                                                      G_OBJECT (fifo),
	                                                      fifo_open_in_thread),
	                      NULL);

	simple = G_SIMPLE_ASYNC_RESULT (result);

	if (g_simple_async_result_propagate_error (simple, error))
	{
		return NULL;
	}

	return G_OUTPUT_STREAM (g_simple_async_result_get_op_res_gpointer (simple));
}

void
gedit_fifo_open_read_async (GeditFifo           *fifo,
                            gint                 io_priority,
                            GCancellable        *cancellable,
                            GAsyncReadyCallback  callback,
                            gpointer             user_data)
{
	g_return_if_fail (GEDIT_IS_FIFO (fifo));

	async_open (fifo,
	            GEDIT_FIFO_OPEN_MODE_READ,
	            io_priority,
	            cancellable,
	            callback,
	            user_data);
}

void
gedit_fifo_open_write_async (GeditFifo           *fifo,
                             gint                 io_priority,
                             GCancellable        *cancellable,
                             GAsyncReadyCallback  callback,
                             gpointer             user_data)
{
	g_return_if_fail (GEDIT_IS_FIFO (fifo));

	async_open (fifo,
	            GEDIT_FIFO_OPEN_MODE_WRITE,
	            io_priority,
	            cancellable,
	            callback,
	            user_data);
}

GFile *
gedit_fifo_get_file (GeditFifo *fifo)
{
	g_return_val_if_fail (GEDIT_IS_FIFO (fifo), NULL);

	return g_file_dup (fifo->priv->file);
}

/* ex:ts=8:noet: */
