/* gedit-session - Basic session management for gedit
 *
 * Copyright (C) 2002 Ximian, Inc.
 *
 * Authors Federico Mena-Quintero <federico@ximian.com>
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


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libgnome/gnome-config.h>
#include <libgnomeui/gnome-client.h>
#include "bonobo-mdi-session.h"
#include "gedit2.h"
#include "gedit-mdi-child.h"
#include "gedit-session.h"
#include "gedit-file.h"
#include "gedit-debug.h"
#include "gedit-plugins-engine.h"
#include "gedit-prefs-manager.h"

/* The master client we use for SM */
static GnomeClient *master_client = NULL;

/* argv[0] from main(); used as the command to restart the program */
static const char *program_argv0 = NULL;

static void 
interaction_function (GnomeClient *client, gint key, GnomeDialogType dialog_type, gpointer shutdown)
{
	const gchar *prefix;

	gedit_debug (DEBUG_SESSION, "");

	/* Save all unsaved files */

	if (GPOINTER_TO_INT (shutdown))		
		gedit_file_save_all ();

	/* Save session data */

	prefix = gnome_client_get_config_prefix (client);

	gedit_debug (DEBUG_SESSION, "Prefix: %s", prefix);

	gnome_config_push_prefix (prefix);

	bonobo_mdi_save_state (BONOBO_MDI (gedit_mdi), "Session");

	gnome_config_pop_prefix ();
	gnome_config_sync ();

	gnome_interaction_key_return (key, FALSE);

	gedit_debug (DEBUG_SESSION, "END");
}

/* save_yourself handler for the master client */
static gboolean
client_save_yourself_cb (GnomeClient *client,
			 gint phase,
			 GnomeSaveStyle save_style,
			 gboolean shutdown,
			 GnomeInteractStyle interact_style,
			 gboolean fast,
			 gpointer data)
{
	const gchar *prefix;

	char *argv[] = { "rm", "-r", NULL };

	gedit_debug (DEBUG_SESSION, "");

	gnome_client_request_interaction (client, 
					  GNOME_DIALOG_NORMAL, 
					  interaction_function,
					  GINT_TO_POINTER (shutdown));
	
	prefix = gnome_client_get_config_prefix (client);

	gedit_debug (DEBUG_SESSION, "Prefix: %s", prefix);

	/* Tell the session manager how to discard this save */

	argv[2] = gnome_config_get_real_path (prefix);
	gnome_client_set_discard_command (client, 3, argv);

	/* Tell the session manager how to clone or restart this instance */

	argv[0] = (char *) program_argv0;
	argv[1] = NULL; /* "--debug-session"; */
	
	gnome_client_set_clone_command (client, 1 /*2*/, argv);
	gnome_client_set_restart_command (client, 1 /*2*/, argv);

	gedit_debug (DEBUG_SESSION, "END");

	return TRUE;
}

/* die handler for the master client */
static void
client_die_cb (GnomeClient *client, gpointer data)
{
	gedit_debug (DEBUG_SESSION, "");

	if (!client->save_yourself_emitted)
		gedit_file_close_all ();

	gedit_debug (DEBUG_FILE, "All files closed.");
	
	bonobo_mdi_destroy (BONOBO_MDI (gedit_mdi));
	
	gedit_debug (DEBUG_FILE, "Unref gedit_mdi.");

	g_object_unref (G_OBJECT (gedit_mdi));

	gedit_debug (DEBUG_FILE, "Unref gedit_mdi: DONE");

	gedit_debug (DEBUG_FILE, "Unref gedit_app_server.");

	bonobo_object_unref (gedit_app_server);

	gedit_debug (DEBUG_FILE, "Unref gedit_app_server: DONE");

	gedit_prefs_manager_shutdown ();

	gedit_plugins_engine_shutdown ();

	gtk_main_quit ();
}

/**
 * gedit_session_init:
 * 
 * Initializes session management support.  This function should be called near
 * the beginning of the program.
 **/
void
gedit_session_init (const char *argv0)
{
	gedit_debug (DEBUG_SESSION, "");
	
	if (master_client)
		return;

	program_argv0 = argv0;

	master_client = gnome_master_client ();

	g_signal_connect (master_client, "save_yourself",
			  G_CALLBACK (client_save_yourself_cb),
			  NULL);
	g_signal_connect (master_client, "die",
			  G_CALLBACK (client_die_cb),
			  NULL);
}

/**
 * gedit_session_is_restored:
 * 
 * Returns whether this gedit is running from a restarted session.
 * 
 * Return value: TRUE if the session manager restarted us, FALSE otherwise.
 * This should be used to determine whether to pay attention to command line
 * arguments in case the session was not restored.
 **/
gboolean
gedit_session_is_restored (void)
{
	gboolean restored;
	
	gedit_debug (DEBUG_SESSION, "");

	if (!master_client)
		return FALSE;

	restored = (gnome_client_get_flags (master_client) & GNOME_CLIENT_RESTORED) != 0;

	gedit_debug (DEBUG_SESSION, restored ? "RESTORED" : "NOT RESTORED");

	return restored;
}

/* Callback used from bonobo_mdi_restore_state() */
static BonoboMDIChild *
mdi_child_create_cb (const gchar *str)
{
	GeditMDIChild *child;
	GError *error = NULL;

	gedit_debug (DEBUG_SESSION, "");

	/* The config string is simply a filename */
	if (str != NULL)
	{
		gedit_debug (DEBUG_SESSION, "URI: %s", str);

		child = gedit_mdi_child_new_with_uri (str, &error);
	}
	else
		child = gedit_mdi_child_new ();

	if (error != NULL) {
		/* FIXME: we are restoring the session; where should we report errors? */
		g_error_free (error);
		return NULL;
	}

	return BONOBO_MDI_CHILD (child);
}

/**
 * gedit_session_load:
 * 
 * Loads the session by fetching the necessary information from the session
 * manager and opening files.
 * 
 * Return value: TRUE if the session was loaded successfully, FALSE otherwise.
 **/
gboolean
gedit_session_load (void)
{
	int retval;

	gedit_debug (DEBUG_SESSION, "");

	gnome_config_push_prefix (gnome_client_get_config_prefix (master_client));

	retval = bonobo_mdi_restore_state (BONOBO_MDI (gedit_mdi), "Session",
					   mdi_child_create_cb);

	gnome_config_pop_prefix ();
	return retval;
}
