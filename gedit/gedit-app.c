/*
 * gedit-app.c
 * This file is part of gedit
 *
 * Copyright (C) 2005-2006 - Paolo Maggi 
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
 * Modified by the gedit Team, 2005. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 *
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include <glib/gi18n.h>

#include "gedit-app.h"
#include "gedit-prefs-manager-app.h"
#include "gedit-commands.h"
#include "gedit-notebook.h"
#include "gedit-debug.h"
#include "gedit-utils.h"

#define GEDIT_APP_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), GEDIT_TYPE_APP, GeditAppPrivate))

struct _GeditAppPrivate
{
	GList	    *windows;
	GeditWindow *active_window;
};

G_DEFINE_TYPE(GeditApp, gedit_app, G_TYPE_OBJECT)

static void
gedit_app_finalize (GObject *object)
{
	GeditApp *app = GEDIT_APP (object); 

	g_list_free (app->priv->windows);

	G_OBJECT_CLASS (gedit_app_parent_class)->finalize (object);
}

static void 
gedit_app_class_init (GeditAppClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = gedit_app_finalize;

	g_type_class_add_private (object_class, sizeof(GeditAppPrivate));
}

static void
gedit_app_init (GeditApp *app)
{
	app->priv = GEDIT_APP_GET_PRIVATE (app);	
}

static void
app_weak_notify (gpointer data,
		 GObject *where_the_app_was)
{
	gtk_main_quit ();
}

/**
 * gedit_app_get_default:
 *
 * Returns the #GeditApp object. This object is a singleton and
 * represents the running gedit instance.
 *
 * Return value: the #GeditApp pointer
 */
GeditApp *
gedit_app_get_default (void)
{
	static GeditApp *app = NULL;

	if (app != NULL)
		return app;

	app = GEDIT_APP (g_object_new (GEDIT_TYPE_APP, NULL));	

	g_object_add_weak_pointer (G_OBJECT (app),
				   (gpointer) &app);
	g_object_weak_ref (G_OBJECT (app),
			   app_weak_notify,
			   NULL);

	return app;
}

static gboolean
window_focus_in_event (GeditWindow   *window, 
		       GdkEventFocus *event, 
		       GeditApp      *app)
{
	/* updates active_view and active_child when a new toplevel receives focus */
	g_return_val_if_fail (GEDIT_IS_WINDOW (window), FALSE);

	app->priv->active_window = window;

	return FALSE;
}

static gboolean 
window_delete_event (GeditWindow *window,
                     GdkEvent    *event,
                     GeditApp    *app)
{
	GeditWindowState ws;

	ws = gedit_window_get_state (window);

	if (ws & 
	    (GEDIT_WINDOW_STATE_SAVING |
	     GEDIT_WINDOW_STATE_PRINTING |
	     GEDIT_WINDOW_STATE_SAVING_SESSION))
	    	return TRUE;
	
	_gedit_cmd_file_quit (NULL, window);
		
	/* Do not destroy the window */
	return TRUE;
}

static void
window_destroy (GeditWindow *window, 
		GeditApp    *app)
{
	app->priv->windows = g_list_remove (app->priv->windows,
					    window);

	if (window == app->priv->active_window)
	{
		app->priv->active_window = app->priv->windows != NULL ?
					   app->priv->windows->data : NULL;
	}

/* CHECK: I don't think we have to disconnect this function, since windows
   is being destroyed */
/*					     
	g_signal_handlers_disconnect_by_func (window, 
					      G_CALLBACK (window_focus_in_event),
					      app);
	g_signal_handlers_disconnect_by_func (window, 
					      G_CALLBACK (window_destroy),
					      app);
*/					      
	if (app->priv->windows == NULL)
	{
		g_object_unref (app);
	}
}

/* Generates a unique string for a window role */
static gchar *
gen_role (void)
{
	time_t t;
	static gint serial;

	t = time (NULL);

	return g_strdup_printf ("gedit-window-%d-%d-%d-%ld-%d@%s",
				getpid (),
				getgid (),
				getppid (),
				(long) t,
				serial++,
				g_get_host_name ());
}
	     
static GeditWindow *
gedit_app_create_window_real (GeditApp    *app,
			      gboolean     set_geometry,
			      const gchar *role)
{
	GeditWindow *window;

	gedit_debug (DEBUG_APP);

	/*
	 * We need to be careful here, there is a race condition:
	 * when another gedit is launched it checks active_window,
	 * so we must do our best to ensure that active_window
	 * is never NULL when at least a window exists.
	 */
	if (app->priv->windows == NULL)
	{
		app->priv->active_window = window = g_object_new (GEDIT_TYPE_WINDOW,
								  NULL);
	}
	else
	{
		window = g_object_new (GEDIT_TYPE_WINDOW, NULL);
	}

	app->priv->windows = g_list_prepend (app->priv->windows,
					     window);

	gedit_debug_message (DEBUG_APP, "Window created");

	if (role != NULL)
	{
		gtk_window_set_role (GTK_WINDOW (window), role);
	}
	else
	{
		gchar *newrole;

		newrole = gen_role ();
		gtk_window_set_role (GTK_WINDOW (window), newrole);
		g_free (newrole);
	}

	if (set_geometry)
	{
		GdkWindowState state;

		state = gedit_prefs_manager_get_window_state ();

		if ((state & GDK_WINDOW_STATE_MAXIMIZED) != 0)
		{
			gtk_window_set_default_size (GTK_WINDOW (window),
						     gedit_prefs_manager_get_default_window_width (),
						     gedit_prefs_manager_get_default_window_height ());

			gtk_window_maximize (GTK_WINDOW (window));
		}
		else
		{
			gtk_window_set_default_size (GTK_WINDOW (window),
						     gedit_prefs_manager_get_window_width (),
						     gedit_prefs_manager_get_window_height ());

			gtk_window_unmaximize (GTK_WINDOW (window));
		}

		if ((state & GDK_WINDOW_STATE_STICKY ) != 0)
			gtk_window_stick (GTK_WINDOW (window));
		else
			gtk_window_unstick (GTK_WINDOW (window));
	}

	g_signal_connect (window, 
			  "focus_in_event",
			  G_CALLBACK (window_focus_in_event), 
			  app);
	g_signal_connect (window,
			  "delete_event",
			  G_CALLBACK (window_delete_event),
			  app);			  
	g_signal_connect (window, 
			  "destroy",
			  G_CALLBACK (window_destroy),
			  app);

	return window;
}

/**
 * gedit_app_create_window:
 * @app: the #GeditApp
 *
 * Create a new #GeditWindow part of @app.
 *
 * Return value: the new #GeditWindow
 */
GeditWindow *
gedit_app_create_window (GeditApp  *app,
			 GdkScreen *screen)
{
	GeditWindow *window;

	window = gedit_app_create_window_real (app, TRUE, NULL);

	if (screen != NULL)
		gtk_window_set_screen (GTK_WINDOW (window), screen);

	return window;
}

/*
 * Same as _create_window, but doesn't set the geometry.
 * The session manager takes care of it. Used in gnome-session.
 */
GeditWindow *
_gedit_app_restore_window (GeditApp    *app,
			   const gchar *role)
{
	GeditWindow *window;

	window = gedit_app_create_window_real (app, FALSE, role);

	return window;
}

/**
 * gedit_app_get_windows:
 * @app: the #GeditApp
 *
 * Returns all the windows currently present in #GeditApp.
 *
 * Return value: the list of #GeditWindows objects. The list
 * should not be freed
 */
const GList *
gedit_app_get_windows (GeditApp *app)
{
	g_return_val_if_fail (GEDIT_IS_APP (app), NULL);

	return app->priv->windows;
}

/**
 * gedit_app_get_active_window:
 * @app: the #GeditApp
 *
 * Retrives the #GeditWindow currently active.
 *
 * Return value: the active #GeditWindow
 */
GeditWindow *
gedit_app_get_active_window (GeditApp *app)
{
	g_return_val_if_fail (GEDIT_IS_APP (app), NULL);

	/* make sure our active window is always realized:
	 * this is needed on startup if we launch two gedit fast
	 * enough that the second instance comes up before the
	 * first one shows its window.
	 */
	if (!GTK_WIDGET_REALIZED (GTK_WIDGET (app->priv->active_window)))
		gtk_widget_realize (GTK_WIDGET (app->priv->active_window));

	return app->priv->active_window;
}

static gboolean
is_in_workspace (GeditWindow *window,
		 GdkScreen   *screen,
		 gint         workspace)
{
	GdkScreen *s;
	GdkDisplay *display;
	const gchar *cur_name;
	const gchar *name;
	gint cur_n;
	gint n;
	gint ws;

	display = gdk_screen_get_display (screen);
	cur_name = gdk_display_get_name (display);
	cur_n = gdk_screen_get_number (screen);

	s = gtk_window_get_screen (GTK_WINDOW (window));
	display = gdk_screen_get_display (s);
	name = gdk_display_get_name (display);
	n = gdk_screen_get_number (s);

	ws = gedit_utils_get_window_workspace (GTK_WINDOW (window));

	return ((strcmp (cur_name, name) == 0) && cur_n == n &&
	        (ws == workspace || ws == GEDIT_ALL_WORKSPACES));
}

GeditWindow *
_gedit_app_get_window_in_workspace (GeditApp  *app,
				    GdkScreen *screen,
				    gint       workspace)
{
	GeditWindow *window;

	GList *l;

	g_return_val_if_fail (GEDIT_IS_APP (app), NULL);

	/* first try if the active window */
	window = app->priv->active_window;

	g_return_val_if_fail (GEDIT_IS_WINDOW (window), NULL);

	if (is_in_workspace (window, screen, workspace))
		return window;

	/* otherwise try to see if there is a window on this workspace */
	for (l = app->priv->windows; l != NULL; l = l->next)
	{
		window = l->data;

		if (is_in_workspace (window, screen, workspace))
			return window;
	}

	/* no window on this workspace... create a new one */
	return gedit_app_create_window (app, screen);
}

/**
 * gedit_app_get_documents:
 * @app: the #GeditApp
 *
 * Returns all the documents currently open in #GeditApp.
 *
 * Return value: a newly allocated list of #GeditDocument objects
 */
GList *
gedit_app_get_documents	(GeditApp *app)
{
	GList *res = NULL;
	GList *windows;

	g_return_val_if_fail (GEDIT_IS_APP (app), NULL);

	windows = app->priv->windows;

	while (windows != NULL)
	{
		res = g_list_concat (res,
				     gedit_window_get_documents (GEDIT_WINDOW (windows->data)));

		windows = g_list_next (windows);
	}

	return res;
}

/**
 * gedit_app_get_views:
 * @app: the #GeditApp
 *
 * Returns all the views currently present in #GeditApp.
 *
 * Return value: a newly allocated list of #GeditView objects
 */
GList *
gedit_app_get_views (GeditApp *app)
{
	GList *res = NULL;
	GList *windows;

	g_return_val_if_fail (GEDIT_IS_APP (app), NULL);

	windows = app->priv->windows;

	while (windows != NULL)
	{
		res = g_list_concat (res,
				     gedit_window_get_views (GEDIT_WINDOW (windows->data)));

		windows = g_list_next (windows);
	}

	return res;
}
