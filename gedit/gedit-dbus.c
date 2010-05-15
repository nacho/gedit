/*
 * gedit-dbus.c
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

#include "gedit-dbus.h"
#include <errno.h>
#include <stdlib.h>
#include <gdk/gdk.h>
#include "gedit-utils.h"
#include "gedit-command-line.h"
#include "gedit-window.h"
#include "gedit-app.h"
#include "gedit-commands.h"

#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#endif

#ifdef G_OS_UNIX
#include <gio/gunixinputstream.h>
#include <gio/gunixconnection.h>
#include <gio/gunixfdlist.h>
#include <unistd.h>
#include "gedit-fifo.h"
#endif

typedef struct _WaitData WaitData;
typedef void (*WaitHandlerFunc)(GObject *object, WaitData *data);

struct _WaitData
{
	GeditDBus *dbus;
	GeditWindow *window;
	guint32 wait_id;

	guint num_handlers;
	WaitHandlerFunc func;

	guint close_window : 1;
};

typedef struct _DisplayParameters DisplayParameters;

struct _DisplayParameters
{
	gchar *display_name;
	gint32 screen_number;
	gint32 workspace;
	gint32 viewport_x;
	gint32 viewport_y;
};

typedef struct _OpenParameters OpenParameters;

struct _OpenParameters
{
	const GeditEncoding *encoding;
	gint32 line_position;
	gint32 column_position;
};

#define GEDIT_DBUS_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE((object), GEDIT_TYPE_DBUS, GeditDBusPrivate))

typedef struct
{
	GeditDBus *dbus;
	GCancellable *cancellable;

	GeditWindow *window;

	OpenParameters oparams;
	WaitData *wait_data;

	guint jump_to : 1;
} AsyncData;

struct _GeditDBusPrivate
{
	GeditDBusResult result;
	GMainLoop *main_loop;
	guint32 wait_id;

	guint32 next_wait_id;

	GeditFifo *stdin_fifo;
	GInputStream *stdin_in_stream;
	GOutputStream *stdin_out_stream;
	GCancellable *stdin_cancellable;
};

G_DEFINE_TYPE (GeditDBus, gedit_dbus, G_TYPE_OBJECT)

static void
async_window_destroyed (AsyncData *async,
                        GObject   *where_the_object_was)
{
	g_cancellable_cancel (async->cancellable);
	async->window = NULL;
}

static void
async_data_free (AsyncData *async)
{
	g_object_unref (async->cancellable);

	if (async->window)
	{
		g_object_weak_unref (G_OBJECT (async->window),
		                     (GWeakNotify)async_window_destroyed,
		                     async);
	}

	g_slice_free (AsyncData, async);
}

static AsyncData *
async_data_new (GeditDBus *dbus)
{
	AsyncData *async;

	async = g_slice_new0 (AsyncData);

	async->dbus = dbus;
	async->cancellable = g_cancellable_new ();

	dbus->priv->stdin_cancellable = g_object_ref (async->cancellable);

	return async;
}

static void
gedit_dbus_dispose (GObject *object)
{
	GeditDBus *dbus = GEDIT_DBUS (object);

	if (dbus->priv->stdin_cancellable)
	{
		g_cancellable_cancel (dbus->priv->stdin_cancellable);
		g_object_unref (dbus->priv->stdin_cancellable);

		dbus->priv->stdin_cancellable = NULL;
	}

	if (dbus->priv->stdin_fifo)
	{
		g_object_unref (dbus->priv->stdin_fifo);
		dbus->priv->stdin_fifo = NULL;
	}

	if (dbus->priv->stdin_out_stream)
	{
		g_object_unref (dbus->priv->stdin_out_stream);
		dbus->priv->stdin_out_stream = NULL;
	}

	if (dbus->priv->stdin_in_stream)
	{
		g_object_unref (dbus->priv->stdin_in_stream);
		dbus->priv->stdin_in_stream = NULL;
	}

	G_OBJECT_CLASS (gedit_dbus_parent_class)->dispose (object);
}

static void
gedit_dbus_class_init (GeditDBusClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = gedit_dbus_dispose;

	g_type_class_add_private (object_class, sizeof(GeditDBusPrivate));
}

static void
gedit_dbus_init (GeditDBus *self)
{
	self->priv = GEDIT_DBUS_GET_PRIVATE (self);
}

GeditDBus *
gedit_dbus_new ()
{
	return g_object_new (GEDIT_TYPE_DBUS, NULL);
}

static guint32
get_startup_timestamp (void)
{
	const gchar *startup_id_env;
	gchar *time_str;
	gchar *end;
	gulong retval = 0;

	/* we don't unset the env, since startup-notification
	 * may still need it */
	startup_id_env = g_getenv ("DESKTOP_STARTUP_ID");

	if (startup_id_env == NULL)
	{
		return 0;
	}

	time_str = g_strrstr (startup_id_env, "_TIME");

	if (time_str == NULL)
	{
		return 0;
	}

	errno = 0;

	/* Skip past the "_TIME" part */
	time_str += 5;

	retval = strtoul (time_str, &end, 0);

	if (end == time_str || errno != 0)
	{
		return 0;
	}
	else
	{
		return retval;
	}
}

static GeditDBusResult
activate_service (GeditDBus *dbus,
                  guint     *result)
{
	GDBusConnection *conn;
	GVariant *ret;

	conn = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, NULL);

	if (conn == NULL)
	{
		return GEDIT_DBUS_RESULT_FAILED;
	}

	ret = g_dbus_connection_call_sync (conn,
	                                   "org.freedesktop.DBus",
	                                   "/org/freedesktop/DBus",
	                                   "org.freedesktop.DBus",
	                                   "StartServiceByName",
	                                   g_variant_new ("(su)", "org.gnome.gedit", 0),
	                                   G_DBUS_CALL_FLAGS_NONE,
	                                   -1,
	                                   NULL,
	                                   NULL);

	g_object_unref (conn);

	if (ret)
	{
		if (result)
		{
			g_variant_get (ret, "(u)", result);
		}

		g_variant_unref (ret);
		return GEDIT_DBUS_RESULT_SUCCESS;
	}
	else
	{
		return GEDIT_DBUS_RESULT_FAILED;
	}
}

static void
get_display_arguments (GeditDBus         *dbus,
                       DisplayParameters *dparams)
{
	GdkScreen *screen;
	GdkDisplay *display;

	screen = gdk_screen_get_default ();
	display = gdk_screen_get_display (screen);

	dparams->display_name = g_strdup (gdk_display_get_name (display));
	dparams->screen_number = gdk_screen_get_number (screen);

	dparams->workspace = gedit_utils_get_current_workspace (screen);
	gedit_utils_get_current_viewport (screen, &dparams->viewport_x, &dparams->viewport_y);
}

static GVariant *
compose_open_parameters (GeditDBus *dbus)
{
	GVariantBuilder file_list;
	GVariantBuilder options;
	GSList *item;
	const GeditEncoding *encoding;
	DisplayParameters dparams;
	GeditCommandLine *command_line;
	const char *geometry;

	command_line = gedit_command_line_get_default ();

	/* Compose files as uris */
	g_variant_builder_init (&file_list, G_VARIANT_TYPE ("as"));

	item = gedit_command_line_get_file_list (command_line);

	while (item)
	{
		gchar *uri = g_file_get_uri (item->data);
		g_variant_builder_add (&file_list, "s", uri);
		g_free (uri);

		item = g_slist_next (item);
	}

	/* Compose additional options */
	g_variant_builder_init (&options, G_VARIANT_TYPE ("a{sv}"));

	/* see if we need to add the pipe_path */
	if (dbus->priv->stdin_fifo)
	{
		GFile *file;
		gchar *path;

		file = gedit_fifo_get_file (dbus->priv->stdin_fifo);
		path = g_file_get_path (file);

		g_variant_builder_add (&options,
		                       "{sv}",
		                       "pipe_path",
		                       g_variant_new_string (path));

		g_object_unref (file);
		g_free (path);
	}

	/* add the encoding, line position, column position */
	encoding = gedit_command_line_get_encoding (command_line);

	if (encoding)
	{
		g_variant_builder_add (&options,
		                       "{sv}", "encoding",
		                       g_variant_new_string (gedit_encoding_get_charset (encoding)));
	}

	g_variant_builder_add (&options,
	                       "{sv}",
	                       "line_position",
	                       g_variant_new_int32 (gedit_command_line_get_line_position (command_line)));

	g_variant_builder_add (&options,
	                       "{sv}",
	                       "column_position",
	                       g_variant_new_int32 (gedit_command_line_get_column_position (command_line)));

	/* whether the make a new window, new document */
	g_variant_builder_add (&options,
	                       "{sv}",
	                       "new_window",
	                       g_variant_new_boolean (gedit_command_line_get_new_window (command_line)));

	g_variant_builder_add (&options,
	                       "{sv}",
	                       "new_document",
	                       g_variant_new_boolean (gedit_command_line_get_new_document (command_line)));

	/* whether to wait */
	g_variant_builder_add (&options,
	                       "{sv}",
	                       "wait",
	                       g_variant_new_boolean (gedit_command_line_get_wait (command_line)));

	/* the proper startup time */
	g_variant_builder_add (&options,
	                       "{sv}",
	                       "startup_time",
	                       g_variant_new_uint32 (get_startup_timestamp ()));

	/* display parameters like display name, screen, workspace, viewport */
	get_display_arguments (dbus, &dparams);

	g_variant_builder_add (&options,
	                       "{sv}",
	                       "display_name",
	                       g_variant_new_string (dparams.display_name));

	g_free (dparams.display_name);

	g_variant_builder_add (&options,
	                       "{sv}",
	                       "screen_number",
	                       g_variant_new_int32 (dparams.screen_number));

	g_variant_builder_add (&options,
	                       "{sv}",
	                       "workspace",
	                       g_variant_new_int32 (dparams.workspace));

	g_variant_builder_add (&options,
	                       "{sv}",
	                       "viewport_x",
	                       g_variant_new_int32 (dparams.viewport_x));

	g_variant_builder_add (&options,
	                       "{sv}",
	                       "viewport_y",
	                       g_variant_new_int32 (dparams.viewport_y));

	/* set geometry */
	geometry = gedit_command_line_get_geometry (command_line);

	if (geometry)
	{
		g_variant_builder_add (&options,
		                       "{sv}",
		                       "geometry",
		                       g_variant_new_string (geometry));
	}

	return g_variant_new ("(asa{sv})", &file_list, &options);
}

static void
slave_open_ready_cb (GDBusConnection *connection,
                     GAsyncResult    *result,
                     GeditDBus       *dbus)
{
	GDBusMessage *ret;
	GError *error = NULL;
	GeditCommandLine *command_line;

	ret = g_dbus_connection_send_message_with_reply_finish (connection,
	                                                        result,
	                                                        &error);
	command_line = gedit_command_line_get_default ();

	if (ret == NULL)
	{
		g_warning ("Failed to call gedit service: %s", error->message);
		g_error_free (error);

		dbus->priv->result = GEDIT_DBUS_RESULT_FAILED;
		g_main_loop_quit (dbus->priv->main_loop);
	}
	else
	{
		g_variant_get (g_dbus_message_get_body (ret),
		               "(u)",
		               &dbus->priv->wait_id);

		dbus->priv->result = GEDIT_DBUS_RESULT_SUCCESS;

		if (!gedit_command_line_get_wait (command_line) &&
		    !dbus->priv->stdin_cancellable)
		{
			g_main_loop_quit (dbus->priv->main_loop);
		}
	}
}

static void
on_open_proxy_signal (GDBusProxy *proxy,
                      gchar      *sender_name,
                      gchar      *signal_name,
                      GVariant   *parameters,
                      GeditDBus  *dbus)
{
	if (g_strcmp0 (signal_name, "WaitDone") == 0)
	{
		guint wait_id;

		g_variant_get (parameters, "(u)", &wait_id);

		if (wait_id == dbus->priv->wait_id)
		{
			g_main_loop_quit (dbus->priv->main_loop);
		}
	}
}

#ifdef G_OS_UNIX
static void
stdin_write_finish (GOutputStream *stream,
                    GAsyncResult  *result,
                    AsyncData     *async)
{
	GError *error = NULL;
	gssize written;
	GeditDBusPrivate *priv;
	GeditCommandLine *command_line;

	if (g_cancellable_is_cancelled (async->cancellable))
	{
		async_data_free (async);
		return;
	}

	written = g_output_stream_splice_finish (stream, result, &error);
	priv = async->dbus->priv;

	g_object_unref (priv->stdin_out_stream);
	g_object_unref (priv->stdin_in_stream);

	priv->stdin_out_stream = NULL;
	priv->stdin_in_stream = NULL;

	if (written == -1)
	{
		g_warning ("Failed to write stdin: %s", error->message);
		g_error_free (error);
	}

	async_data_free (async);

	g_object_unref (priv->stdin_fifo);
	priv->stdin_fifo = NULL;

	g_object_unref (priv->stdin_cancellable);
	priv->stdin_cancellable = NULL;

	command_line = gedit_command_line_get_default ();

	if (priv->main_loop && !gedit_command_line_get_wait (command_line))
	{
		/* only quit the main loop if it's there and if we don't need
		   to wait */
		g_main_loop_quit (priv->main_loop);
	}
}

static void
stdin_pipe_ready_to_write (GeditFifo    *fifo,
                           GAsyncResult *result,
                           AsyncData    *async)
{
	GeditDBusPrivate *priv;
	GOutputStream *stream;
	GError *error = NULL;

	if (g_cancellable_is_cancelled (async->cancellable))
	{
		async_data_free (async);
		return;
	}

	stream = gedit_fifo_open_write_finish (fifo, result, &error);

	if (stream == NULL)
	{
		g_warning ("Could not open fifo for writing: %s", error->message);
		g_error_free (error);

		/* Can't do that then */
		async_data_free (async);
		return;
	}

	priv = async->dbus->priv;

	priv->stdin_out_stream = stream;
	priv->stdin_in_stream = g_unix_input_stream_new (STDIN_FILENO, TRUE);

	g_output_stream_splice_async (priv->stdin_out_stream,
	                              priv->stdin_in_stream,
	                              G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE |
	                              G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET,
	                              G_PRIORITY_DEFAULT,
	                              async->cancellable,
	                              (GAsyncReadyCallback)stdin_write_finish,
	                              async);
}

static void
open_command_add_stdin_pipe (GeditDBus *dbus)
{
	AsyncData *async;

	dbus->priv->stdin_fifo = gedit_fifo_new (NULL);

	if (!dbus->priv->stdin_fifo)
	{
		g_warning ("Failed to create fifo for standard in");
		return;
	}

	async = async_data_new (dbus);

	gedit_fifo_open_write_async (dbus->priv->stdin_fifo,
	                             G_PRIORITY_DEFAULT,
	                             async->cancellable,
	                             (GAsyncReadyCallback)stdin_pipe_ready_to_write,
	                             async);
}
#endif

static void
open_command_add_stdin (GeditDBus       *dbus,
                        GDBusConnection *connection,
                        GDBusMessage    *message)
{
#ifdef G_OS_UNIX
	GUnixFDList *fdlist;
	GError *error = NULL;
	gint ret;

	if (!gedit_utils_can_read_from_stdin ())
	{
		return;
	}

	if (!(g_dbus_connection_get_capabilities (connection) &
	      G_DBUS_CAPABILITY_FLAGS_UNIX_FD_PASSING))
	{
		/* Fallback with named pipe */
		open_command_add_stdin_pipe (dbus);
		return;
	}

	fdlist = g_unix_fd_list_new ();
	ret = g_unix_fd_list_append (fdlist, STDIN_FILENO, &error);

	if (ret == -1)
	{
		g_warning ("Could not read from standard in: %s", error->message);
		g_error_free (error);
	}
	else
	{
		/* Here we can close STDIN because it's dupped */
		close (STDIN_FILENO);
	}

	g_dbus_message_set_unix_fd_list (message, fdlist);
	g_object_unref (fdlist);
#endif
}

static void
command_line_proxy_appeared (GDBusConnection *connection,
                             const gchar     *name,
                             const gchar     *owner_name,
                             GDBusProxy      *proxy,
                             GeditDBus       *dbus)
{
	GeditCommandLine *command_line;
	GDBusMessage *message;

	command_line = gedit_command_line_get_default ();

	if (gedit_command_line_get_wait (command_line))
	{
		g_signal_connect (proxy,
		                  "g-signal",
		                  G_CALLBACK (on_open_proxy_signal),
		                  dbus);
	}

	message = g_dbus_message_new_method_call (g_dbus_proxy_get_unique_bus_name (proxy),
	                                          "/org/gnome/gedit",
	                                          "org.gnome.gedit.CommandLine",
	                                          "Open");

	open_command_add_stdin (dbus, connection, message);
	g_dbus_message_set_body (message, compose_open_parameters (dbus));

	g_dbus_connection_send_message_with_reply (g_dbus_proxy_get_connection (proxy),
	                                           message,
	                                           -1,
	                                           NULL,
	                                           NULL,
	                                           (GAsyncReadyCallback)slave_open_ready_cb,
	                                           dbus);

	g_object_unref (message);
}

static void
command_line_proxy_vanished (GDBusConnection *connection,
                             const gchar     *name,
                             GeditDBus       *dbus)
{
	dbus->priv->result = GEDIT_DBUS_RESULT_FAILED;
	g_main_loop_quit (dbus->priv->main_loop);
}

static GeditDBusResult
handle_slave (GeditDBus *dbus)
{
	/* send the requested commands to the master */
	GDBusConnection *conn;
	GDBusCallFlags flags;
	GeditCommandLine *command_line;

	conn = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, NULL);

	if (conn == NULL)
	{
		g_warning ("Could not connect to session bus");
		return GEDIT_DBUS_RESULT_FAILED;
	}

	flags = G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES;
	command_line = gedit_command_line_get_default ();

	if (!gedit_command_line_get_wait (command_line))
	{
		flags |= G_DBUS_PROXY_FLAGS_DO_NOT_CONNECT_SIGNALS;
	}

	g_bus_watch_proxy (G_BUS_TYPE_SESSION,
	                   "org.gnome.gedit",
	                   G_BUS_NAME_WATCHER_FLAGS_NONE,
	                   "/org/gnome/gedit",
	                   "org.gnome.gedit.CommandLine",
	                   G_TYPE_DBUS_PROXY,
	                   flags,
	                   (GBusProxyAppearedCallback)command_line_proxy_appeared,
	                   (GBusProxyVanishedCallback)command_line_proxy_vanished,
	                   dbus,
	                   NULL);

	dbus->priv->main_loop = g_main_loop_new (NULL, FALSE);
	g_main_loop_run (dbus->priv->main_loop);

	g_object_unref (conn);
	g_main_loop_unref (dbus->priv->main_loop);

	return dbus->priv->result;
}

static GeditDBusResult
handle_master (GeditDBus *dbus)
{
	/* let the main gedit thing do its thing */
	if (g_getenv ("DBUS_STARTER_ADDRESS"))
	{
		/* started as a service, we don't want to popup a window on
		   some random display */
		return GEDIT_DBUS_RESULT_PROCEED_SERVICE;
	}
	else
	{
		return GEDIT_DBUS_RESULT_PROCEED;
	}
}

static GeditDBusResult
handle_service (GeditDBus *dbus)
{
	guint result;
	GeditCommandLine *command_line;

	if (activate_service (dbus, &result) == GEDIT_DBUS_RESULT_FAILED)
	{
		g_warning ("Could not activate gedit service");
		return GEDIT_DBUS_RESULT_FAILED;
	}

	command_line = gedit_command_line_get_default ();

	/* Finally, act as a slave. */
	return handle_slave (dbus);
}

static GSList *
variant_iter_list_to_locations (GVariantIter *iter)
{
	gchar *uri;
	GSList *ret = NULL;

	while (g_variant_iter_loop (iter, "s", &uri))
	{
		ret = g_slist_prepend (ret, g_file_new_for_uri (uri));
	}

	return g_slist_reverse (ret);
}

static GdkDisplay *
display_open_if_needed (const gchar *name)
{
	GSList *displays;
	GSList *l;
	GdkDisplay *display = NULL;

	displays = gdk_display_manager_list_displays (gdk_display_manager_get ());

	for (l = displays; l != NULL; l = l->next)
	{
		if (g_strcmp0 (gdk_display_get_name (l->data), name) == 0)
		{
			display = l->data;
			break;
		}
	}

	g_slist_free (displays);

	return display != NULL ? display : gdk_display_open (name);
}

static GeditWindow *
window_from_display_arguments (gboolean           new_window,
                               DisplayParameters *dparams,
                               gboolean           create)
{
	GdkScreen *screen;
	GeditApp *app;
	GeditWindow *ret;

	/* get correct screen using the display_name and screen_number */
	if (dparams->display_name != NULL && *dparams->display_name)
	{
		GdkDisplay *display;

		display = display_open_if_needed (dparams->display_name);
		screen = gdk_display_get_screen (display,
		                                 dparams->screen_number == -1 ? 0 : dparams->screen_number);
	}

	app = gedit_app_get_default ();

	if (new_window)
	{
		ret = gedit_app_create_window (app, screen);
		gedit_window_create_tab (ret, TRUE);

		return ret;
	}

	if (screen != NULL)
	{
		ret = _gedit_app_get_window_in_viewport (app,
		                                         screen,
		                                         dparams->workspace == -1 ? 0 : dparams->workspace,
		                                         dparams->viewport_x == -1 ? 0 : dparams->viewport_x,
		                                         dparams->viewport_y == -1 ? 0 : dparams->viewport_y);
	}
	else
	{
		ret = gedit_app_get_active_window (app);
	}

	if (!ret && create)
	{
		ret = gedit_app_create_window (app, screen);
		gedit_window_create_tab (ret, TRUE);
	}

	return ret;
}

static gboolean
is_empty_window (GeditWindow *window,
                 gboolean     check_untouched)
{
	GList *views;
	gboolean ret = FALSE;

	views = gedit_window_get_views (window);

	if (!views)
	{
		ret = TRUE;
	}
	else if (check_untouched && views->next == NULL)
	{
		GeditView *view = GEDIT_VIEW (views->data);
		GeditDocument *doc = GEDIT_DOCUMENT (gtk_text_view_get_buffer (GTK_TEXT_VIEW (view)));

		if (gedit_document_is_untouched (doc) &&
		    gedit_tab_get_state (gedit_tab_get_from_document (doc)) == GEDIT_TAB_STATE_NORMAL)
		{
			ret = TRUE;
		}
	}

	g_list_free (views);
	return ret;
}

static void
set_interaction_time_and_present (GeditWindow *window,
                                  guint        startup_time)
{
	/* set the proper interaction time on the window.
	* Fall back to roundtripping to the X server when we
	* don't have the timestamp, e.g. when launched from
	* terminal. We also need to make sure that the window
	* has been realized otherwise it will not work. lame.
	*/

	if (!GTK_WIDGET_REALIZED (window))
	{
		gtk_widget_realize (GTK_WIDGET (window));
	}

#ifdef GDK_WINDOWING_X11
	if (startup_time <= 0)
	{
		startup_time = gdk_x11_get_server_time (GTK_WIDGET (window)->window);
	}

	gdk_x11_window_set_user_time (GTK_WIDGET (window)->window, startup_time);
#endif

	gtk_window_present (GTK_WINDOW (window));
}

static void
wait_handler_dbus (GObject  *object,
                   WaitData *data)
{
	GDBusConnection *conn;

	conn = g_bus_get_sync (G_BUS_TYPE_SESSION, NULL, NULL);

	if (conn == NULL)
	{
		g_warning ("Could not emit WaitDone signal because session bus is gone");
		return;
	}

	/* Emit the WaitDone signal */
	g_dbus_connection_emit_signal (conn,
	                               NULL,
	                               "/org/gnome/gedit",
	                               "org.gnome.gedit.CommandLine",
	                               "WaitDone",
	                               g_variant_new ("(u)", data->wait_id),
	                               NULL);
	g_object_unref (conn);

	if (data->window && object != G_OBJECT (data->window) && data->close_window &&
	    is_empty_window (data->window, FALSE))
	{
		/* Close the window */
		gtk_widget_destroy (GTK_WIDGET (data->window));
	}
}

static void
unref_wait_handler (WaitData  *data,
                    GObject   *object)
{
	if (data->num_handlers == 0)
	{
		return;
	}

	--data->num_handlers;

	if (data->num_handlers == 0)
	{
		data->func (object, data);
	
		/* Free the wait data */
		g_slice_free (WaitData, data);
	}
}

static void
install_wait_handler (GeditDBus       *dbus,
                      WaitData        *data,
                      GObject         *object,
                      WaitHandlerFunc  func)
{
	++data->num_handlers;
	data->func = func;

	g_object_weak_ref (object, (GWeakNotify)unref_wait_handler, data);
}

#ifdef G_OS_UNIX
static GeditTab *
tab_from_stream (GeditWindow    *window,
                 GInputStream   *stream,
                 OpenParameters *oparams,
                 gboolean        jump_to)
{
	GList *documents;
	GeditDocument *doc = NULL;
	GeditTab *tab = NULL;

	documents = gedit_window_get_documents (window);

	if (documents)
	{
		doc = GEDIT_DOCUMENT (documents->data);
		tab = gedit_tab_get_from_document (doc);
	}

	if (documents && !documents->next &&
	    gedit_document_is_untouched (doc) &&
	    gedit_tab_get_state (tab) == GEDIT_TAB_STATE_NORMAL)
	{
		/* open right in that document */
		GeditDocument *doc = GEDIT_DOCUMENT (documents->data);

		tab = gedit_tab_get_from_document (doc);

		_gedit_tab_load_stream (tab,
		                        stream,
		                        oparams->encoding,
		                        oparams->line_position,
		                        oparams->column_position);
	}
	else
	{
		tab = gedit_window_create_tab_from_stream (window,
		                                           stream,
		                                           oparams->encoding,
		                                           oparams->line_position,
		                                           oparams->column_position,
		                                           jump_to);
	}

	g_list_free (documents);
	return tab;
}
#endif

static GSList *
create_tabs_for_fds (GeditDBus             *dbus,
                     GDBusMethodInvocation *invocation,
                     GeditWindow           *window,
                     OpenParameters        *oparams,
                     gboolean               jump_to)
{
#ifdef G_OS_UNIX
	GDBusMessage *message;
	GDBusConnection *connection;
	GUnixFDList *fdlist;
	GSList *ret = NULL;
	gint num;
	gint i;

	connection = g_dbus_method_invocation_get_connection (invocation);

	if (!(g_dbus_connection_get_capabilities (connection) &
	      G_DBUS_CAPABILITY_FLAGS_UNIX_FD_PASSING))
	{
		return NULL;
	}

	message = g_dbus_method_invocation_get_message (invocation);
	fdlist = g_dbus_message_get_unix_fd_list (message);

	if (!fdlist)
	{
		return NULL;
	}

	num = g_unix_fd_list_get_length (fdlist);

	for (i = 0; i < num; ++i)
	{
		gint fd;
		GError *error = NULL;

		fd = g_unix_fd_list_get (fdlist, i, &error);

		if (fd == -1)
		{
			g_warning ("Could not open stream for service: %s", error->message);
			g_error_free (error);
			error = NULL;
		}
		else
		{
			GeditTab *tab;
			GInputStream *stream;

			/* fd is dupped, so we close it when the stream closes */
			stream = g_unix_input_stream_new (fd, TRUE);

			tab = tab_from_stream (window,
			                       stream,
			                       oparams,
			                       jump_to);

			g_object_unref (stream);

			if (tab)
			{
				ret = g_slist_prepend (ret, tab);
				jump_to = FALSE;
			}
		}
	}

	return g_slist_reverse (ret);
#else
	return NULL;
#endif
}

#ifdef G_OS_UNIX
static void
stdin_pipe_ready_to_read (GeditFifo    *fifo,
                          GAsyncResult *result,
                          AsyncData    *async)
{
	GInputStream *stream;
	GError *error = NULL;
	GeditTab *tab;

	if (g_cancellable_is_cancelled (async->cancellable))
	{
		async_data_free (async);
		return;
	}

	stream = gedit_fifo_open_read_finish (fifo, result, &error);

	if (!stream)
	{
		g_warning ("Opening stdin pipe error: %s", error->message);

		g_error_free (error);

		g_object_unref (async->dbus->priv->stdin_cancellable);
		async->dbus->priv->stdin_cancellable = NULL;

		g_object_unref (fifo);
		async->dbus->priv->stdin_fifo = NULL;

		async_data_free (async);
		return;
	}

	tab = tab_from_stream (async->window,
	                       stream,
	                       &async->oparams,
	                       async->jump_to);

	g_object_unref (stream);

	if (async->wait_data)
	{
		install_wait_handler (async->dbus,
		                      async->wait_data,
		                      G_OBJECT (tab),
		                      wait_handler_dbus);
	}
}
#endif

static gboolean
handle_open_pipe (GeditDBus      *dbus,
                  const gchar    *pipe_path,
                  GeditWindow    *window,
                  OpenParameters *oparams,
                  gboolean        jump_to,
                  WaitData       *wait_data)
{
#ifdef G_OS_UNIX
	/* We'll do this async */
	GFile *file;
	AsyncData *async;

	if (!pipe_path)
	{
		return FALSE;
	}

	file = g_file_new_for_path (pipe_path);
	dbus->priv->stdin_fifo = gedit_fifo_new (file);
	g_object_unref (file);

	if (dbus->priv->stdin_fifo == NULL)
	{
		return FALSE;
	}

	async = async_data_new (dbus);
	async->window = window;
	async->oparams = *oparams;
	async->jump_to = jump_to;
	async->wait_data = wait_data;

	g_object_weak_ref (G_OBJECT (window),
	                   (GWeakNotify)async_window_destroyed,
	                   async);

	gedit_fifo_open_read_async (dbus->priv->stdin_fifo,
	                            G_PRIORITY_DEFAULT,
	                            async->cancellable,
	                            (GAsyncReadyCallback)stdin_pipe_ready_to_read,
	                            async);

	return TRUE;
#else
	return FALSE;
#endif
}

static gboolean
extract_optional_parameters (GHashTable  *parameters,
                             ...) G_GNUC_NULL_TERMINATED;

static gboolean
extract_optional_parameters (GHashTable  *parameters,
                             ...)
{
	va_list va_args;
	const gchar *key;
	gboolean ret = FALSE;

	va_start (va_args, parameters);

	while ((key = va_arg (va_args, const gchar *)) != NULL)
	{
		GVariant *value;

		value = g_hash_table_lookup (parameters, key);

		if (!value)
		{
			/* ignore the next */
			va_arg (va_args, gpointer);
			continue;
		}

		ret = TRUE;

		g_variant_get_va (value,
		                  g_variant_get_type_string (value),
		                  NULL,
		                  &va_args);
	}

	va_end (va_args);
	return ret;
}

static GHashTable *
optional_parameters_hash_table (GVariantIter *iter)
{
	GVariant *value;
	GHashTable *ret;
	const gchar *key;

	ret = g_hash_table_new_full (g_str_hash,
	                             g_str_equal,
	                             (GDestroyNotify)g_free,
	                             (GDestroyNotify)g_variant_unref);

	while (g_variant_iter_loop (iter, "{sv}", &key, &value))
	{
		g_hash_table_insert (ret,
		                     g_strdup (key),
		                     g_variant_ref (value));
	}

	return ret;
}

static GSList *
handle_open_fds (GeditDBus             *dbus,
                 GDBusMethodInvocation *invocation,
                 GSList                *loaded,
                 GeditWindow           *window,
                 OpenParameters        *oparams)
{
	GSList *item;
	GSList *tabs;
	GSList *ret = NULL;

	/* This is for creating tabs from unix file descriptors supplied with
	   the dbus message */
	tabs = create_tabs_for_fds (dbus,
	                            invocation,
	                            window,
	                            oparams,
	                            loaded == NULL);

	for (item = tabs; item; item = g_slist_next (item))
	{
		ret = g_slist_prepend (ret, gedit_tab_get_document (item->data));
	}

	g_slist_free (tabs);

	return g_slist_concat (loaded, g_slist_reverse (ret));

}

static void
dbus_handle_open (GeditDBus             *dbus,
                  GVariant              *parameters,
                  GDBusMethodInvocation *invocation)
{
	GVariantIter *file_list;
	GVariantIter *optional_parameters;
	GHashTable *options_hash;

	gchar *charset_encoding = NULL;
	guint32 startup_time;

	OpenParameters open_parameters = {NULL, 0, 0};

	gboolean new_window = FALSE;
	gboolean new_document = FALSE;
	gboolean wait = FALSE;
	gchar *pipe_path = NULL;

	GSList *locations = NULL;
	GeditWindow *window;
	GSList *loaded_documents = NULL;
	gboolean empty_window;
	WaitData *data;
	guint32 wait_id = 0;

	g_variant_get (parameters,
	               "(asa{sv})",
	               &file_list,
	               &optional_parameters);

	locations = variant_iter_list_to_locations (file_list);
	g_variant_iter_free (file_list);

	options_hash = optional_parameters_hash_table (optional_parameters);
	g_variant_iter_free (optional_parameters);

	{
		DisplayParameters display_parameters = {NULL, -1, -1, -1, -1};
		gchar *geometry;

		extract_optional_parameters (options_hash,
		                             "new_window", &new_window,
		                             "display_name", &display_parameters.display_name,
		                             "screen_number", &display_parameters.screen_number,
		                             "workspace", &display_parameters.workspace,
		                             "viewport_x", &display_parameters.viewport_x,
		                             "viewport_y", &display_parameters.viewport_y,
		                             NULL);

		window = window_from_display_arguments (new_window,
		                                        &display_parameters,
		                                        TRUE);

		g_free (display_parameters.display_name);

		if (extract_optional_parameters (options_hash,
		                                 "geometry", &geometry,
		                                 NULL))
		{
			gtk_window_parse_geometry (GTK_WINDOW (window),
			                           geometry);
			g_free (geometry);
		}
	}

	extract_optional_parameters (options_hash, "encoding", &charset_encoding, NULL);

	if (charset_encoding && *charset_encoding)
	{
		open_parameters.encoding = gedit_encoding_get_from_charset (charset_encoding);
	}

	g_free (charset_encoding);

	empty_window = is_empty_window (window, TRUE);

	extract_optional_parameters (options_hash,
	                             "line_position", &open_parameters.line_position,
	                             "column_position", &open_parameters.column_position,
	                             NULL);

	if (locations)
	{
		loaded_documents = _gedit_cmd_load_files_from_prompt (window,
		                                                      locations,
		                                                      open_parameters.encoding,
		                                                      open_parameters.line_position,
		                                                      open_parameters.column_position);
	}

	g_slist_free (locations);

	loaded_documents = handle_open_fds (dbus,
	                                    invocation,
	                                    loaded_documents,
	                                    window,
	                                    &open_parameters);

	extract_optional_parameters (options_hash,
	                             "wait", &wait,
	                             "pipe_path", &pipe_path,
	                             "new_document", &new_document,
	                             "startup_time", &startup_time,
	                             NULL);

	set_interaction_time_and_present (window, startup_time);

	if (!wait)
	{
		gboolean jump_to = loaded_documents == NULL;

		if (new_document)
		{
			gedit_window_create_tab (window, jump_to);
			jump_to = FALSE;
		}

		handle_open_pipe (dbus,
		                  pipe_path,
		                  window,
		                  &open_parameters,
		                  jump_to,
		                  NULL);
	}
	else
	{
		gboolean jump_to = loaded_documents == NULL;
		gboolean has_pipe;

		data = g_slice_new (WaitData);

		data->dbus = dbus;
		data->window = window;
		data->close_window = empty_window;
		data->wait_id = ++dbus->priv->next_wait_id;
		data->num_handlers = 0;

		/* for the return value */
		wait_id = data->wait_id;

		if (new_document)
		{
			GeditTab *tab;
			tab = gedit_window_create_tab (window, jump_to);
			jump_to = FALSE;

			loaded_documents = g_slist_append (loaded_documents,
			                                   gedit_tab_get_document (tab));
		}

		has_pipe = handle_open_pipe (dbus,
		                             pipe_path,
		                             window,
		                             &open_parameters,
		                             jump_to,
		                             data);

		/* Install wait handler on the window if there were no documents
		   opened */
		if (loaded_documents == NULL && !has_pipe)
		{
			/* Add wait handler on the window */
			install_wait_handler (dbus,
			                      data,
			                      G_OBJECT (window),
			                      wait_handler_dbus);
		}
		else
		{
			GSList *item;

			/* Add wait handler on the documents */
			for (item = loaded_documents; item; item = item->next)
			{
				install_wait_handler (dbus,
				                      data,
				                      G_OBJECT (item->data),
				                      wait_handler_dbus);
			}
		}
	}

	g_free (pipe_path);

	g_slist_free (loaded_documents);
	g_hash_table_destroy (options_hash);

	g_dbus_method_invocation_return_value (invocation,
	                                       g_variant_new ("(u)", wait_id));
}

static void
dbus_command_line_method_call_cb (GDBusConnection       *connection,
                                  const gchar           *sender,
                                  const gchar           *object_path,
                                  const gchar           *interface_name,
                                  const gchar           *method_name,
                                  GVariant              *parameters,
                                  GDBusMethodInvocation *invocation,
                                  gpointer               user_data)
{
	g_return_if_fail (g_strcmp0 (object_path, "/org/gnome/gedit") == 0);
	g_return_if_fail (g_strcmp0 (interface_name, "org.gnome.gedit.CommandLine") == 0);

	if (g_strcmp0 (method_name, "Open") == 0)
	{
		dbus_handle_open (user_data, parameters, invocation);
	}
	else
	{
		g_warning ("Unsupported method called on gedit service: %s", method_name);
	}
}

static const gchar introspection_xml[] =
	"<node>"
	"  <interface name='org.gnome.gedit.CommandLine'>"
	"    <method name='Open'>"
	"      <arg type='as' name='files' direction='in'/>"
	"      <arg type='a{sv}' name='options' direction='in'/>"
	"      <arg type='u' name='wait_id' direction='out'/>"
	"    </method>"
	"    <signal name='WaitDone'>"
	"      <arg type='u' name='wait_id'/>"
	"    </signal>"
	"  </interface>"
	"</node>";

static const GDBusInterfaceVTable command_line_vtable = {
	dbus_command_line_method_call_cb,
};

static gboolean
register_dbus_interface (GeditDBus       *dbus,
                         GDBusConnection *connection)
{
	guint ret;
	GDBusNodeInfo *info;

	info = g_dbus_node_info_new_for_xml (introspection_xml, NULL);

	ret = g_dbus_connection_register_object (connection,
	                                         "/org/gnome/gedit",
	                                         info->interfaces[0],
	                                         &command_line_vtable,
	                                         dbus,
	                                         NULL,
	                                         NULL);

	return ret != 0;
}

static void
bus_acquired_cb (GDBusConnection *connection,
                 const gchar     *name,
                 GeditDBus       *dbus)
{
	if (connection == NULL)
	{
		g_warning ("Failed to acquire dbus connection");
		dbus->priv->result = GEDIT_DBUS_RESULT_PROCEED;

		g_main_loop_quit (dbus->priv->main_loop);
	}
	else
	{
		/* setup the dbus interface that other gedit processes can call. we do
		   this here even though we might not own the name, because the docs say
		   it should be done here... */
		register_dbus_interface (dbus, connection);
	}
}

static void
name_acquired_cb (GDBusConnection *connection,
                  const gchar     *name,
                  GeditDBus       *dbus)
{
	dbus->priv->result = GEDIT_DBUS_RESULT_SUCCESS;
	g_main_loop_quit (dbus->priv->main_loop);
}

static void
name_lost_cb (GDBusConnection *connection,
              const gchar     *name,
              GeditDBus       *dbus)
{
	dbus->priv->result = GEDIT_DBUS_RESULT_FAILED;
	g_main_loop_quit (dbus->priv->main_loop);
}

GeditDBusResult
gedit_dbus_run (GeditDBus *dbus)
{
	guint id;
	GeditCommandLine *command_line;

	g_return_val_if_fail (GEDIT_IS_DBUS (dbus), GEDIT_DBUS_RESULT_PROCEED);

	command_line = gedit_command_line_get_default ();

	if (gedit_command_line_get_standalone (command_line))
	{
		return GEDIT_DBUS_RESULT_PROCEED;
	}

	if (gedit_command_line_get_wait (command_line) ||
	    gedit_command_line_get_background (command_line))
	{
		GeditDBusResult ret = handle_service (dbus);

		/* We actually continue if it failed, because it's nicer to
		   still start some kind of gedit in that case */
		if (ret != GEDIT_DBUS_RESULT_FAILED)
		{
			return ret;
		}
	}

	id = g_bus_own_name (G_BUS_TYPE_SESSION,
	                     "org.gnome.gedit",
	                     G_BUS_NAME_OWNER_FLAGS_NONE,
	                     (GBusAcquiredCallback)bus_acquired_cb,
	                     (GBusNameAcquiredCallback)name_acquired_cb,
	                     (GBusNameLostCallback)name_lost_cb,
	                     dbus,
	                     NULL);

	dbus->priv->main_loop = g_main_loop_new (NULL, FALSE);
	g_main_loop_run (dbus->priv->main_loop);
	g_main_loop_unref (dbus->priv->main_loop);

	switch (dbus->priv->result)
	{
		case GEDIT_DBUS_RESULT_PROCEED:
			/* could not initialize dbus, gonna be standalone */
			return GEDIT_DBUS_RESULT_PROCEED;
		break;
		case GEDIT_DBUS_RESULT_FAILED:
			/* there is already a gedit process */
			return handle_slave (dbus);
		break;
		case GEDIT_DBUS_RESULT_SUCCESS:
			/* we are the main gedit process */
			return handle_master (dbus);
		break;
		default:
			g_assert_not_reached ();
		break;
	}
}

/* ex:ts=8:noet: */
