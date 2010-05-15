/*
 * gedit-app-osx.c
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

#include "gedit-app-osx.h"

#include <gdk/gdkquartz.h>
#include <Carbon/Carbon.h>

#import "gedit-osx-delegate.h"

G_DEFINE_TYPE (GeditAppOSX, gedit_app_osx, GEDIT_TYPE_APP)

static void
gedit_app_osx_finalize (GObject *object)
{
	G_OBJECT_CLASS (gedit_app_osx_parent_class)->finalize (object);
}

static gboolean
gedit_app_osx_last_window_destroyed_impl (GeditApp *app)
{
	if (!GPOINTER_TO_INT (g_object_get_data (G_OBJECT (window), "gedit-is-quitting-all")))
	{
		/* Create hidden proxy window on OS X to handle the menu */
		gedit_app_create_window (app, NULL);
		return FALSE;
	}

	return GEDIT_APP_CLASS (gedit_app_osx_parent_class)->last_window_destroyed (app);
}

static gboolean
gedit_app_osx_show_url (GeditAppOSX *app,
                        const gchar *url)
{
	return [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:[NSString stringWithUTF8String:url]]];
}

static gboolean
gedit_app_osx_show_help_impl (GeditApp    *app,
                              GtkWindow   *parent,
                              const gchar *name,
                              const gchar *link_id)
{
	gboolean ret = FALSE;

	if (name == NULL || strcmp(name, "gedit.xml") == NULL || strcmp(name, "gedit") == 0)
	{
		gchar *link;

		if (link_id)
		{
			link = g_strdup_printf ("http://library.gnome.org/users/gedit/stable/%s",
						link_id);
		}
		else
		{
			link = g_strdup ("http://library.gnome.org/users/gedit/stable/");
		}

		ret = gedit_osx_show_url (link);
		g_free (link);
	}

	return ret;
}

static void
gedit_app_osx_set_window_title_impl (GeditApp    *app,
                                     GeditWindow *window,
                                     const gchar *title)
{
	NSWindow *native;
	GeditDocument *document;

	g_return_if_fail (GEDIT_IS_WINDOW (window));

	if (GTK_WIDGET (window)->window == NULL)
	{
		return;
	}

	native = gdk_quartz_window_get_nswindow (GTK_WIDGET (window)->window);
	document = gedit_app_get_active_document (app);

	if (document)
	{
		bool ismodified;

		if (gedit_document_is_untitled (document))
		{
			[native setRepresentedURL:nil];
		}
		else
		{
			const gchar *uri = gedit_document_get_uri (document);
			NSURL *nsurl = [NSURL URLWithString:[NSString stringWithUTF8String:uri]];
			
			[native setRepresentedURL:nsurl];
		}

		ismodified = !gedit_document_is_untouched (document); 
		[native setDocumentEdited:ismodified];
	}
	else
	{
		[native setRepresentedURL:nil];
		[native setDocumentEdited:false];
	}

	GEDIT_APP (gedit_app_osx_parent_class)->set_window_title (app, window, title);
}

static void
gedit_app_osx_class_init (GeditAppOSXClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GeditAppClass *app_class = GEDIT_APP_CLASS (klass);

	object_class->finalize = gedit_app_osx_finalize;

	app_class->last_window_destroyed = gedit_app_osx_last_window_destroyed_impl;
	app_class->show_help = gedit_app_show_help_impl;
}

static void
destroy_delegate (GeditOSXDelegate *delegate)
{
	[delegate dealloc];
}

static void
gedit_app_osx_init (GeditAppOSX *self)
{
	GeditOSXDelegate *delegate = [[GeditOSXDelegate alloc] init];

	g_object_set_data_full (G_OBJECT (app),
	                        "GeditOSXDelegate",
	                        delegate,
	                        (GDestroyNotify)destroy_delegate);

	ige_mac_menu_set_global_key_handler_enabled (FALSE);

	/* manually set name and icon */
	g_set_application_name("gedit");
	gtk_window_set_default_icon_name ("accessories-text-editor");
}

/* ex:ts=8:noet: */
