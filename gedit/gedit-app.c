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
#include <unistd.h>

#include <glib/gi18n.h>
#include <libpeas/peas-extension-set.h>

#include "gedit-app.h"
#include "gedit-commands.h"
#include "gedit-notebook.h"
#include "gedit-debug.h"
#include "gedit-utils.h"
#include "gedit-enum-types.h"
#include "gedit-dirs.h"
#include "gedit-settings.h"
#include "gedit-app-activatable.h"
#include "gedit-plugins-engine.h"

#ifdef OS_OSX
#include "gedit-app-osx.h"
#else
#ifdef OS_WIN32
#include "gedit-app-win32.h"
#else
#include "gedit-app-x11.h"
#endif
#endif

#define GEDIT_PAGE_SETUP_FILE		"gedit-page-setup"
#define GEDIT_PRINT_SETTINGS_FILE	"gedit-print-settings"

#define GEDIT_APP_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), GEDIT_TYPE_APP, GeditAppPrivate))

/* Properties */
enum
{
	PROP_0,
	PROP_LOCKDOWN
};

struct _GeditAppPrivate
{
	GList	          *windows;
	GeditWindow       *active_window;

	GeditLockdownMask  lockdown;

	GtkPageSetup      *page_setup;
	GtkPrintSettings  *print_settings;
	
	GObject           *settings;
	GSettings         *window_settings;

	PeasExtensionSet  *extensions;
};

G_DEFINE_ABSTRACT_TYPE(GeditApp, gedit_app, G_TYPE_INITIALLY_UNOWNED)

static void
gedit_app_finalize (GObject *object)
{
	GeditApp *app = GEDIT_APP (object); 

	g_list_free (app->priv->windows);

	if (app->priv->page_setup)
		g_object_unref (app->priv->page_setup);
	if (app->priv->print_settings)
		g_object_unref (app->priv->print_settings);

	G_OBJECT_CLASS (gedit_app_parent_class)->finalize (object);
}

static void
gedit_app_dispose (GObject *object)
{
	GeditApp *app = GEDIT_APP (object);

	if (app->priv->window_settings != NULL)
	{
		g_object_unref (app->priv->window_settings);
		app->priv->window_settings = NULL;
	}

	if (app->priv->settings != NULL)
	{
		g_object_unref (app->priv->settings);
		app->priv->settings = NULL;
	}

	if (app->priv->extensions != NULL)
	{
		/* Note that unreffing the extensions will automatically remove
		   all extensions which in turn will deactivate the extension */
		g_object_unref (app->priv->extensions);
		app->priv->extensions = NULL;
	}

	G_OBJECT_CLASS (gedit_app_parent_class)->dispose (object);
}

static void
gedit_app_get_property (GObject    *object,
			guint       prop_id,
			GValue     *value,
			GParamSpec *pspec)
{
	GeditApp *app = GEDIT_APP (object);

	switch (prop_id)
	{
		case PROP_LOCKDOWN:
			g_value_set_flags (value, gedit_app_get_lockdown (app));
			break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
			break;
	}
}

static GObject *
gedit_app_constructor (GType                  gtype,
                       guint                  n_construct_params,
                       GObjectConstructParam *construct_params)
{
	static GObject *app = NULL;

	if (!app)
	{
		app = G_OBJECT_CLASS (gedit_app_parent_class)->constructor (gtype,
		                                                            n_construct_params,
		                                                            construct_params);

		g_object_add_weak_pointer (app, (gpointer *) &app);

		return app;
	}

	return g_object_ref (app);
}

static gboolean
gedit_app_last_window_destroyed_impl (GeditApp *app)
{
	return TRUE;
}

static gchar *
gedit_app_help_link_id_impl (GeditApp    *app,
                             const gchar *name,
                             const gchar *link_id)
{
	if (link_id)
	{
		return g_strdup_printf ("ghelp:%s?%s", name, link_id);
	}
	else
	{
		return g_strdup_printf ("ghelp:%s", name);
	}
}

static gboolean
gedit_app_show_help_impl (GeditApp    *app,
                          GtkWindow   *parent,
                          const gchar *name,
                          const gchar *link_id)
{
	GError *error = NULL;
	gboolean ret;
	gchar *link;

	if (name == NULL)
	{
		name = "gedit";
	}
	else if (strcmp (name, "gedit.xml") == 0)
	{
		g_warning ("%s: Using \"gedit.xml\" for the help name is deprecated, use \"gedit\" or simply NULL instead", G_STRFUNC);
		name = "gedit";
	}

	link = GEDIT_APP_GET_CLASS (app)->help_link_id (app, name, link_id);

	ret = gtk_show_uri (gtk_widget_get_screen (GTK_WIDGET (parent)),
	                    link,
	                    GDK_CURRENT_TIME,
	                    &error);

	g_free (link);

	if (error != NULL)
	{
		GtkWidget *dialog;

		dialog = gtk_message_dialog_new (parent,
						 GTK_DIALOG_DESTROY_WITH_PARENT,
						 GTK_MESSAGE_ERROR,
						 GTK_BUTTONS_CLOSE, 
						 _("There was an error displaying the help."));

		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
							  "%s", error->message);

		g_signal_connect (G_OBJECT (dialog),
				  "response",
				  G_CALLBACK (gtk_widget_destroy),
				  NULL);

		gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

		gtk_widget_show (dialog);

		g_error_free (error);
	}

	return ret;
}

static void
gedit_app_set_window_title_impl (GeditApp    *app,
                                 GeditWindow *window,
                                 const gchar *title)
{
	gtk_window_set_title (GTK_WINDOW (window), title);
}

static void
gedit_app_class_init (GeditAppClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = gedit_app_finalize;
	object_class->dispose = gedit_app_dispose;
	object_class->get_property = gedit_app_get_property;
	object_class->constructor = gedit_app_constructor;

	klass->last_window_destroyed = gedit_app_last_window_destroyed_impl;
	klass->show_help = gedit_app_show_help_impl;
	klass->help_link_id = gedit_app_help_link_id_impl;
	klass->set_window_title = gedit_app_set_window_title_impl;

	g_object_class_install_property (object_class,
					 PROP_LOCKDOWN,
					 g_param_spec_flags ("lockdown",
							     "Lockdown",
							     "The lockdown mask",
							     GEDIT_TYPE_LOCKDOWN_MASK,
							     0,
							     G_PARAM_READABLE |
							     G_PARAM_STATIC_STRINGS));

	g_type_class_add_private (object_class, sizeof (GeditAppPrivate));
}

static gboolean
ensure_user_config_dir (void)
{
	const gchar *config_dir;
	gboolean ret = TRUE;
	gint res;

	config_dir = gedit_dirs_get_user_config_dir ();
	if (config_dir == NULL)
	{
		g_warning ("Could not get config directory\n");
		return FALSE;
	}

	res = g_mkdir_with_parents (config_dir, 0755);
	if (res < 0)
	{
		g_warning ("Could not create config directory\n");
		ret = FALSE;
	}

	return ret;
}

static void
load_accels (void)
{
	gchar *filename;

	filename = g_build_filename (gedit_dirs_get_user_config_dir (),
				     "accels",
				     NULL);
	if (filename != NULL)
	{
		gedit_debug_message (DEBUG_APP, "Loading keybindings from %s\n", filename);
		gtk_accel_map_load (filename);
		g_free (filename);
	}
}

static void
save_accels (void)
{
	gchar *filename;

	filename = g_build_filename (gedit_dirs_get_user_config_dir (),
				     "accels",
				     NULL);
	if (filename != NULL)
	{
		gedit_debug_message (DEBUG_APP, "Saving keybindings in %s\n", filename);
		gtk_accel_map_save (filename);
		g_free (filename);
	}
}

static gchar *
get_page_setup_file (void)
{
	const gchar *config_dir;
	gchar *setup = NULL;

	config_dir = gedit_dirs_get_user_config_dir ();
	
	if (config_dir != NULL)
	{
		setup = g_build_filename (config_dir,
					  GEDIT_PAGE_SETUP_FILE,
					  NULL);
	}

	return setup;
}

static void
load_page_setup (GeditApp *app)
{
	gchar *filename;
	GError *error = NULL;

	g_return_if_fail (app->priv->page_setup == NULL);

	filename = get_page_setup_file ();

	app->priv->page_setup = gtk_page_setup_new_from_file (filename,
							      &error);
	if (error)
	{
		/* Ignore file not found error */
		if (error->domain != G_FILE_ERROR ||
		    error->code != G_FILE_ERROR_NOENT)
		{
			g_warning ("%s", error->message);
		}

		g_error_free (error);
	}

	g_free (filename);

	/* fall back to default settings */
	if (app->priv->page_setup == NULL)
		app->priv->page_setup = gtk_page_setup_new ();
}

static void
save_page_setup (GeditApp *app)
{
	gchar *filename;
	GError *error = NULL;

	if (app->priv->page_setup == NULL)
		return;

	filename = get_page_setup_file ();

	gtk_page_setup_to_file (app->priv->page_setup,
				filename,
				&error);
	if (error)
	{
		g_warning ("%s", error->message);
		g_error_free (error);
	}

	g_free (filename);
}

static gchar *
get_print_settings_file (void)
{
	const gchar *config_dir;
	gchar *settings = NULL;

	config_dir = gedit_dirs_get_user_config_dir ();

	if (config_dir != NULL)
	{
		settings = g_build_filename (config_dir,
					     GEDIT_PRINT_SETTINGS_FILE,
					     NULL);
	}

	return settings;
}

static void
load_print_settings (GeditApp *app)
{
	gchar *filename;
	GError *error = NULL;

	g_return_if_fail (app->priv->print_settings == NULL);

	filename = get_print_settings_file ();

	app->priv->print_settings = gtk_print_settings_new_from_file (filename,
								      &error);
	if (error)
	{
		/* Ignore file not found error */
		if (error->domain != G_FILE_ERROR ||
		    error->code != G_FILE_ERROR_NOENT)
		{
			g_warning ("%s", error->message);
		}

		g_error_free (error);
	}

	g_free (filename);

	/* fall back to default settings */
	if (app->priv->print_settings == NULL)
		app->priv->print_settings = gtk_print_settings_new ();
}

static void
save_print_settings (GeditApp *app)
{
	gchar *filename;
	GError *error = NULL;

	if (app->priv->print_settings == NULL)
		return;

	filename = get_print_settings_file ();

	gtk_print_settings_to_file (app->priv->print_settings,
				    filename,
				    &error);
	if (error)
	{
		g_warning ("%s", error->message);
		g_error_free (error);
	}

	g_free (filename);
}

static void
extension_added (PeasExtensionSet *extensions,
		 PeasPluginInfo   *info,
		 PeasExtension    *exten,
		 GeditApp         *app)
{
	peas_extension_call (exten, "activate");
}

static void
extension_removed (PeasExtensionSet *extensions,
		   PeasPluginInfo   *info,
		   PeasExtension    *exten,
		   GeditApp         *app)
{
	peas_extension_call (exten, "deactivate");
}

static void
gedit_app_init (GeditApp *app)
{
	app->priv = GEDIT_APP_GET_PRIVATE (app);

	load_accels ();

	/* Load settings */
	app->priv->settings = gedit_settings_new ();
	app->priv->window_settings = g_settings_new ("org.gnome.gedit.state.window");

	/* initial lockdown state */
	app->priv->lockdown = gedit_settings_get_lockdown (GEDIT_SETTINGS (app->priv->settings));

	app->priv->extensions = peas_extension_set_new (PEAS_ENGINE (gedit_plugins_engine_get_default ()),
							GEDIT_TYPE_APP_ACTIVATABLE,
							"app", app,
							NULL);

	g_signal_connect (app->priv->extensions,
			  "extension-added",
			  G_CALLBACK (extension_added),
			  app);

	g_signal_connect (app->priv->extensions,
			  "extension-removed",
			  G_CALLBACK (extension_removed),
			  app);

	peas_extension_set_call (app->priv->extensions, "activate");
}

/**
 * gedit_app_get_default:
 *
 * Returns the #GeditApp object. This object is a singleton and
 * represents the running gedit instance.
 *
 * Return value: (transfer none): the #GeditApp pointer
 */
GeditApp *
gedit_app_get_default (void)
{
	GeditApp *app;
	GType type;

#ifdef OS_OSX
	type = GEDIT_TYPE_APP_OSX;
#else
#ifdef OS_WIN32
	type = GEDIT_TYPE_APP_WIN32;
#else
	type = GEDIT_TYPE_APP_X11;
#endif
#endif

	app = GEDIT_APP (g_object_new (type, NULL));

	if (g_object_is_floating (app))
	{
		g_object_ref_sink (app);
	}
	else
	{
		g_object_unref (app);
	}

	return app;
}

static void
set_active_window (GeditApp    *app,
                   GeditWindow *window)
{
	app->priv->active_window = window;
}

static gboolean
window_focus_in_event (GeditWindow   *window, 
		       GdkEventFocus *event, 
		       GeditApp      *app)
{
	/* updates active_view and active_child when a new toplevel receives focus */
	g_return_val_if_fail (GEDIT_IS_WINDOW (window), FALSE);

	set_active_window (app, window);

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
	{
		return TRUE;
	}

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
		set_active_window (app, app->priv->windows != NULL ? app->priv->windows->data : NULL);
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
		if (!GEDIT_APP_GET_CLASS (app)->last_window_destroyed (app))
		{
			return;
		}

		/* Last window is gone... save some settings and exit */
		ensure_user_config_dir ();

		save_accels ();
		save_page_setup (app);
		save_print_settings (app);

		gtk_main_quit ();
	}
}

/* Generates a unique string for a window role */
static gchar *
gen_role (void)
{
	GTimeVal result;
	static gint serial;
	
	g_get_current_time (&result);

	return g_strdup_printf ("gedit-window-%ld-%ld-%d-%s",
				result.tv_sec,
				result.tv_usec,
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
		window = g_object_new (GEDIT_TYPE_WINDOW, NULL);
		set_active_window (app, window);
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
		gint w, h;

		state = g_settings_get_int (app->priv->window_settings,
					    GEDIT_SETTINGS_WINDOW_STATE);

		g_settings_get (app->priv->window_settings,
				GEDIT_SETTINGS_WINDOW_SIZE,
				"(ii)", &w, &h);
		gtk_window_set_default_size (GTK_WINDOW (window), w, h);

		if ((state & GDK_WINDOW_STATE_MAXIMIZED) != 0)
			gtk_window_maximize (GTK_WINDOW (window));
		else
			gtk_window_unmaximize (GTK_WINDOW (window));

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
 * @screen: (allow-none):
 *
 * Create a new #GeditWindow part of @app.
 *
 * Return value: (transfer none): the new #GeditWindow
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
 * Return value: (element-type Gedit.Window) (transfer none): the list of #GeditWindows objects.
 * The list should not be freed
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
 * Return value: (transfer none): the active #GeditWindow
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
	if (!gtk_widget_get_realized (GTK_WIDGET (app->priv->active_window)))
		gtk_widget_realize (GTK_WIDGET (app->priv->active_window));

	return app->priv->active_window;
}

static gboolean
is_in_viewport (GeditWindow  *window,
		GdkScreen    *screen,
		gint          workspace,
		gint          viewport_x,
		gint          viewport_y)
{
	GdkScreen *s;
	GdkDisplay *display;
	GdkWindow *gdkwindow;
	const gchar *cur_name;
	const gchar *name;
	gint cur_n;
	gint n;
	gint ws;
	gint sc_width, sc_height;
	gint x, y, width, height;
	gint vp_x, vp_y;

	/* Check for screen and display match */
	display = gdk_screen_get_display (screen);
	cur_name = gdk_display_get_name (display);
	cur_n = gdk_screen_get_number (screen);

	s = gtk_window_get_screen (GTK_WINDOW (window));
	display = gdk_screen_get_display (s);
	name = gdk_display_get_name (display);
	n = gdk_screen_get_number (s);

	if (strcmp (cur_name, name) != 0 || cur_n != n)
		return FALSE;

	/* Check for workspace match */
	ws = gedit_utils_get_window_workspace (GTK_WINDOW (window));
	if (ws != workspace && ws != GEDIT_ALL_WORKSPACES)
		return FALSE;

	/* Check for viewport match */
	gdkwindow = gtk_widget_get_window (GTK_WIDGET (window));
	gdk_window_get_position (gdkwindow, &x, &y);
	width = gdk_window_get_width (gdkwindow);
	height = gdk_window_get_height (gdkwindow);
	gedit_utils_get_current_viewport (screen, &vp_x, &vp_y);
	x += vp_x;
	y += vp_y;

	sc_width = gdk_screen_get_width (screen);
	sc_height = gdk_screen_get_height (screen);

	return x + width * .25 >= viewport_x &&
	       x + width * .75 <= viewport_x + sc_width &&
	       y  >= viewport_y &&
	       y + height <= viewport_y + sc_height;
}

/**
 * _gedit_app_get_window_in_viewport
 * @app: the #GeditApp
 * @screen: the #GdkScreen
 * @workspace: the workspace number
 * @viewport_x: the viewport horizontal origin
 * @viewport_y: the viewport vertical origin
 *
 * Since a workspace can be larger than the screen, it is divided into several
 * equal parts called viewports. This function retrives the #GeditWindow in
 * the given viewport of the given workspace.
 *
 * Return value: the #GeditWindow in the given viewport of the given workspace.
 */
GeditWindow *
_gedit_app_get_window_in_viewport (GeditApp  *app,
				   GdkScreen *screen,
				   gint       workspace,
				   gint       viewport_x,
				   gint       viewport_y)
{
	GeditWindow *window;

	GList *l;

	g_return_val_if_fail (GEDIT_IS_APP (app), NULL);

	/* first try if the active window */
	window = app->priv->active_window;

	g_return_val_if_fail (GEDIT_IS_WINDOW (window), NULL);

	if (is_in_viewport (window, screen, workspace, viewport_x, viewport_y))
		return window;

	/* otherwise try to see if there is a window on this workspace */
	for (l = app->priv->windows; l != NULL; l = l->next)
	{
		window = l->data;

		if (is_in_viewport (window, screen, workspace, viewport_x, viewport_y))
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
 * Return value: (element-type Gedit.Document) (transfer container):
 * a newly allocated list of #GeditDocument objects
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
 * Return value: (element-type Gedit.View) (transfer container):
 * a newly allocated list of #GeditView objects
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

/**
 * gedit_app_get_lockdown:
 * @app: a #GeditApp
 *
 * Gets the lockdown mask (see #GeditLockdownMask) for the application.
 * The lockdown mask determines which functions are locked down using
 * the GNOME-wise lockdown GConf keys.
 **/
GeditLockdownMask
gedit_app_get_lockdown (GeditApp *app)
{
	g_return_val_if_fail (GEDIT_IS_APP (app), GEDIT_LOCKDOWN_ALL);

	return app->priv->lockdown;
}

gboolean
gedit_app_show_help (GeditApp    *app,
                     GtkWindow   *parent,
                     const gchar *name,
                     const gchar *link_id)
{
	g_return_val_if_fail (GEDIT_IS_APP (app), FALSE);
	g_return_val_if_fail (parent == NULL || GTK_IS_WINDOW (parent), FALSE);

	return GEDIT_APP_GET_CLASS (app)->show_help (app, parent, name, link_id);
}

void
gedit_app_set_window_title (GeditApp    *app,
                            GeditWindow *window,
                            const gchar *title)
{
	GEDIT_APP_GET_CLASS (app)->set_window_title (app, window, title);
}

static void
app_lockdown_changed (GeditApp *app)
{
	GList *l;

	for (l = app->priv->windows; l != NULL; l = l->next)
		_gedit_window_set_lockdown (GEDIT_WINDOW (l->data),
					    app->priv->lockdown);

	g_object_notify (G_OBJECT (app), "lockdown");
}

void
_gedit_app_set_lockdown (GeditApp          *app,
			 GeditLockdownMask  lockdown)
{
	g_return_if_fail (GEDIT_IS_APP (app));

	app->priv->lockdown = lockdown;

	app_lockdown_changed (app);
}

void
_gedit_app_set_lockdown_bit (GeditApp          *app,
			     GeditLockdownMask  bit,
			     gboolean           value)
{
	g_return_if_fail (GEDIT_IS_APP (app));

	if (value)
		app->priv->lockdown |= bit;
	else
		app->priv->lockdown &= ~bit;

	app_lockdown_changed (app);
}

/* Returns a copy */
GtkPageSetup *
_gedit_app_get_default_page_setup (GeditApp *app)
{
	g_return_val_if_fail (GEDIT_IS_APP (app), NULL);

	if (app->priv->page_setup == NULL)
		load_page_setup (app);

	return gtk_page_setup_copy (app->priv->page_setup);
}

void
_gedit_app_set_default_page_setup (GeditApp     *app,
				   GtkPageSetup *page_setup)
{
	g_return_if_fail (GEDIT_IS_APP (app));
	g_return_if_fail (GTK_IS_PAGE_SETUP (page_setup));

	if (app->priv->page_setup != NULL)
		g_object_unref (app->priv->page_setup);

	app->priv->page_setup = g_object_ref (page_setup);
}

/* Returns a copy */
GtkPrintSettings *
_gedit_app_get_default_print_settings (GeditApp *app)
{
	g_return_val_if_fail (GEDIT_IS_APP (app), NULL);

	if (app->priv->print_settings == NULL)
		load_print_settings (app);

	return gtk_print_settings_copy (app->priv->print_settings);
}

void
_gedit_app_set_default_print_settings (GeditApp         *app,
				       GtkPrintSettings *settings)
{
	g_return_if_fail (GEDIT_IS_APP (app));
	g_return_if_fail (GTK_IS_PRINT_SETTINGS (settings));

	if (app->priv->print_settings != NULL)
		g_object_unref (app->priv->print_settings);

	app->priv->print_settings = g_object_ref (settings);
}

GObject *
_gedit_app_get_settings (GeditApp *app)
{
	g_return_val_if_fail (GEDIT_IS_APP (app), NULL);

	return app->priv->settings;
}

/* ex:set ts=8 noet: */
