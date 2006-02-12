/*
 * gedit-session.c - Basic session management for gedit
 * This file is part of gedit
 *
 * Copyright (C) 2002 Ximian, Inc.
 * Copyright (C) 2005 - Paolo Maggi 
 *
 * Author: Federico Mena-Quintero <federico@ximian.com>
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
 * Modified by the gedit Team, 2002-2005. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes.
 *
 * $Id$
 */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <unistd.h>
#include <string.h>

#include <libgnome/gnome-util.h>
#include <libgnomeui/gnome-client.h>
#include <libxml/tree.h>
#include <libxml/xmlwriter.h>

#include "gedit-session.h"

#include "gedit-debug.h"
#include "gedit-plugins-engine.h"
#include "gedit-prefs-manager-app.h"
#include "gedit-metadata-manager.h"
#include "gedit-window.h"
#include "gedit-app.h"
#include "gedit-commands.h"
#include "dialogs/gedit-close-confirmation-dialog.h"

/* The master client we use for SM */
static GnomeClient *master_client = NULL;

/* argv[0] from main(); used as the command to restart the program */
static const char *program_argv0 = NULL;

/* globals vars used during the interaction. We are
 * assuming there can only be one interaction at a time
 */
static gint interaction_key;
static GSList *window_dirty_list;

static void	ask_next_confirmation	(void);

#define GEDIT_SESSION_LIST_OF_DOCS_TO_SAVE "gedit-session-list-of-docs-to-save-key"

static gchar *
get_session_dir ()
{
	gchar *gedit_dir;
	gchar *session_dir;

	gedit_dir = gnome_util_home_file ("gedit");
	session_dir = g_build_filename (gedit_dir, "sessions", NULL);	
	g_free (gedit_dir);

	return session_dir;
}

static gchar *
get_session_file_path (GnomeClient *client)
{
	const gchar *prefix;
	gchar *session_dir;
	gchar *session_file;
	gchar *session_path;

	prefix = gnome_client_get_config_prefix (client);
	gedit_debug_message (DEBUG_SESSION, "Prefix: %s", prefix);

	session_file = g_strndup (prefix, strlen (prefix) - 1);
	gedit_debug_message (DEBUG_SESSION, "Session File: %s", session_file);

	session_dir = get_session_dir ();

	session_path = g_build_filename (session_dir,
					 session_file,
					 NULL);

	g_free (session_dir);
	g_free (session_file);

	gedit_debug_message (DEBUG_SESSION, "Session Path: %s", session_path);

	return session_path;
}

static gboolean
ensure_session_dir (void)
{
	gboolean ret = TRUE;
	gchar *dir;

	dir = get_session_dir ();

	if (g_file_test (dir, G_FILE_TEST_IS_DIR) == FALSE)
		ret = (g_mkdir_with_parents (dir, 488) == 0);

	g_free (dir);

	return ret;
}

static int
save_window_session (xmlTextWriterPtr  writer,
		     GeditWindow      *window)
{
	const gchar *role;
	GeditPanel *panel;
	GList *docs, *l;
	int ret;
	GeditDocument *active_document;

	gedit_debug (DEBUG_SESSION);

	active_document = gedit_window_get_active_document (window);

	ret = xmlTextWriterStartElement (writer, (xmlChar *) "window");
	if (ret < 0)
		return ret;
	
	role = gtk_window_get_role (GTK_WINDOW (window));
	if (role != NULL)
	{
		ret = xmlTextWriterWriteAttribute (writer, "role", role);
		if (ret < 0)
			return ret;
	}

	ret = xmlTextWriterStartElement (writer, (xmlChar *) "side-pane");
	if (ret < 0)
		return ret;

	panel = gedit_window_get_side_panel (window);
	ret = xmlTextWriterWriteAttribute (writer,
					   "visible", 
					   GTK_WIDGET_VISIBLE (panel) ? "yes": "no");
	if (ret < 0)
		return ret;

	ret = xmlTextWriterEndElement (writer); /* side-pane */
	if (ret < 0)
		return ret;

	ret = xmlTextWriterStartElement (writer, (xmlChar *) "bottom-panel");
	if (ret < 0)
		return ret;

	panel = gedit_window_get_bottom_panel (window);
	ret = xmlTextWriterWriteAttribute (writer,
					   "visible", 
					   GTK_WIDGET_VISIBLE (panel) ? "yes" : "no");
	if (ret < 0)
		return ret;

	ret = xmlTextWriterEndElement (writer); /* bottom-panel */
	if (ret < 0)
		return ret;

	docs = gedit_window_get_documents (window);
	l = docs;
	while (l != NULL)
	{
		gchar *uri;
		
		ret = xmlTextWriterStartElement (writer, (xmlChar *) "document");
		if (ret < 0)
			return ret;

		uri = gedit_document_get_uri (GEDIT_DOCUMENT (l->data));

		if (uri != NULL)
		{
			ret = xmlTextWriterWriteAttribute (writer,
							   "uri", 
							   uri);

			g_free (uri);

			if (ret < 0)
				return ret;
		}

		if (active_document == GEDIT_DOCUMENT (l->data))
		{
			ret = xmlTextWriterWriteAttribute (writer,
							   "active", 
							   "yes");
			if (ret < 0)
				return ret;
		}

		ret = xmlTextWriterEndElement (writer); /* document */
		if (ret < 0)
			return ret;

		l = g_list_next (l);
	}
	g_list_free (docs);	

	ret = xmlTextWriterEndElement (writer); /* window */

	return ret;
}

static void
save_session ()
{
	int ret;
	gchar *fname;
	xmlTextWriterPtr writer;
	const GList *windows;

	fname = get_session_file_path (master_client);

	gedit_debug_message (DEBUG_SESSION, "Session file: %s", fname);

	if (!ensure_session_dir ())
	{
		g_warning ("Cannot create or write in session directory");
		return;
	}

	writer = xmlNewTextWriterFilename (fname, 0);
	if (writer == NULL)
	{
		g_warning ("Cannot write the session file '%s'", fname);
		return;
	}

	ret = xmlTextWriterSetIndent (writer, 1);
	if (ret < 0)
		goto out;

	ret = xmlTextWriterSetIndentString (writer, (const xmlChar *) " ");
	if (ret < 0)
		goto out;

	/* create and set the root node for the session */
	ret = xmlTextWriterStartElement (writer, (const xmlChar *) "session");
	if (ret < 0)
		goto out;

	windows = gedit_app_get_windows (gedit_app_get_default ());
	while (windows != NULL)
	{
		ret = save_window_session (writer, 
					   GEDIT_WINDOW (windows->data));
		if (ret < 0)
			goto out;

		windows = g_list_next (windows);
	}

	ret = xmlTextWriterEndElement (writer); /* session */
	if (ret < 0)
		goto out;

	ret = xmlTextWriterEndDocument (writer);		

out:
	xmlFreeTextWriter (writer);

	if (ret < 0)
		unlink (fname);

	g_free (fname);
}

static void
finish_interaction (gboolean cancel_shutdown)
{
	/* save session file even if shutdown was cancelled */
	save_session ();

	gnome_interaction_key_return (interaction_key,
				      cancel_shutdown);

	interaction_key = 0;
}

static void
window_handled (GeditWindow *window)
{
	window_dirty_list = g_slist_remove (window_dirty_list, window);

	/* whee... we made it! */
	if (window_dirty_list == NULL)
		finish_interaction (FALSE);
	else
		ask_next_confirmation ();
}

static void
window_state_change (GeditWindow *window,
		     GParamSpec  *pspec,
		     gpointer     data)
{
	GeditWindowState state;
	GList *unsaved_docs;
	GList *docs_to_save;
	GList *l;
	gboolean done = TRUE;

	state = gedit_window_get_state (window);

	/* we are still saving */
	if (state & GEDIT_WINDOW_STATE_SAVING)
		return;

	unsaved_docs = gedit_window_get_unsaved_documents (window);

	docs_to_save =	g_object_get_data (G_OBJECT (window),
					   GEDIT_SESSION_LIST_OF_DOCS_TO_SAVE);


	for (l = docs_to_save; l != NULL; l = l->next)
	{
		if (g_list_find (unsaved_docs, l->data))
		{
			done = FALSE;
			break;
		}
	}

	if (done)
	{
		g_signal_handlers_disconnect_by_func (window, window_state_change, data);
		g_list_free (docs_to_save);
		g_object_set_data (G_OBJECT (window),
				   GEDIT_SESSION_LIST_OF_DOCS_TO_SAVE,
				   NULL);

		window_handled (window);
	}

	g_list_free (unsaved_docs);
}

static void
close_confirmation_dialog_response_handler (GeditCloseConfirmationDialog *dlg,
					    gint                          response_id,
					    GeditWindow                  *window)
{
	const GList *selected_documents;
	GSList *l;

	gedit_debug (DEBUG_COMMANDS);

	switch (response_id)
	{
		case GTK_RESPONSE_YES:
			/* save selected docs */

			g_signal_connect (window,
					  "notify::state",
					  G_CALLBACK (window_state_change),
					  NULL);

			selected_documents = gedit_close_confirmation_dialog_get_selected_documents (dlg);

			g_return_if_fail (g_object_get_data (G_OBJECT (window),
							     GEDIT_SESSION_LIST_OF_DOCS_TO_SAVE) == NULL);

			g_object_set_data (G_OBJECT (window),
					   GEDIT_SESSION_LIST_OF_DOCS_TO_SAVE,
					   g_list_copy (selected_documents));

			_gedit_cmd_file_save_documents_list (window, selected_documents);

			// FIXME: also need to lock the window to prevent further changes..

			break;

		case GTK_RESPONSE_NO:
			/* dont save */
			window_handled (window);
			break;

		default:
			/* disconnect window_state_changed where needed */
			for (l = window_dirty_list; l != NULL; l = l->next)
				g_signal_handlers_disconnect_by_func (window,
						window_state_change, NULL);
			g_slist_free (window_dirty_list);
			window_dirty_list = NULL;

			/* cancel shutdown */
			finish_interaction (TRUE);

			break;
	}

	gtk_widget_destroy (GTK_WIDGET (dlg));
}

static void
show_confirmation_dialog (GeditWindow *window)
{
	GList *unsaved_docs;
	GtkWidget *dlg;

	gedit_debug (DEBUG_SESSION);

	unsaved_docs = gedit_window_get_unsaved_documents (window);

	g_return_if_fail (unsaved_docs != NULL);

	if (unsaved_docs->next == NULL)
	{
		/* There is only one unsaved document */
		GeditTab *tab;
		GeditDocument *doc;

		doc = GEDIT_DOCUMENT (unsaved_docs->data);

		tab = gedit_tab_get_from_document (doc);
		g_return_if_fail (tab != NULL);

		gedit_window_set_active_tab (window, tab);

		dlg = gedit_close_confirmation_dialog_new_single (
						GTK_WINDOW (window),
						doc,
						TRUE);
	}
	else
	{
		dlg = gedit_close_confirmation_dialog_new (GTK_WINDOW (window),
							   unsaved_docs,
							   TRUE);
	}

	g_list_free (unsaved_docs);

	g_signal_connect (dlg,
			  "response",
			  G_CALLBACK (close_confirmation_dialog_response_handler),
			  window);

	gtk_widget_show (dlg);
}

static void
ask_next_confirmation ()
{
	g_return_if_fail (window_dirty_list != NULL);

	/* pop up the confirmation dialog for the first window
	 * in the dirty list. The next confirmation is asked once
	 * this one has been handled.
	 */
	show_confirmation_dialog (GEDIT_WINDOW (window_dirty_list->data));
}

static void
save_all_docs_and_save_session ()
{
	GeditApp *app;
	const GList *l;

	app = gedit_app_get_default ();

	if (window_dirty_list != NULL)
	{
		g_critical ("global variable window_dirty_list not NULL");
		window_dirty_list = NULL;
	}

	for (l = gedit_app_get_windows (app); l != NULL; l = l->next)
	{
		if (gedit_window_get_unsaved_documents (GEDIT_WINDOW (l->data)) != NULL)
		{
			window_dirty_list = g_slist_prepend (window_dirty_list, l->data);
		}
	}

	/* no modified docs, go on and save session */
	if (window_dirty_list == NULL)
	{
		finish_interaction (FALSE);

		return;
	}

	ask_next_confirmation ();
}

static void
interaction_function (GnomeClient     *client,
		      gint             key,
		      GnomeDialogType  dialog_type,
		      gpointer         shutdown)
{
	gedit_debug (DEBUG_SESSION);

	/* sanity checks */
	g_return_if_fail (client == master_client);

	if (interaction_key != 0)
		g_critical ("global variable interaction_key not NULL");
	interaction_key = key;

	/* If we are shutting down, give the user the chance to save
	 * first, otherwise just ignore untitled documents documents.
	 */
	if (GPOINTER_TO_INT (shutdown))
	{
		save_all_docs_and_save_session ();
	}
	else
	{
		finish_interaction (FALSE);
	}
}

/* save_yourself handler for the master client */
static gboolean
client_save_yourself_cb (GnomeClient        *client,
			 gint                phase,
			 GnomeSaveStyle      save_style,
			 gboolean            shutdown,
			 GnomeInteractStyle  interact_style,
			 gboolean            fast,
			 gpointer            data)
{
	gchar *argv[] = { "rm", "-f", NULL };

	gedit_debug (DEBUG_SESSION);

	gnome_client_request_interaction (client, 
					  GNOME_DIALOG_NORMAL, 
					  interaction_function,
					  GINT_TO_POINTER (shutdown));

	/* Tell the session manager how to discard this save */
	argv[2] = get_session_file_path (client);
	gnome_client_set_discard_command (client, 3, argv);

	g_free (argv[2]);

	/* Tell the session manager how to clone or restart this instance */

	argv[0] = (char *) program_argv0;
	argv[1] = NULL;

	gnome_client_set_clone_command (client, 1, argv);
	gnome_client_set_restart_command (client, 1, argv);

	gedit_debug_message (DEBUG_SESSION, "END");

	return TRUE;
}

/* die handler for the master client */
static void
client_die_cb (GnomeClient *client, gpointer data)
{
#if 0
	gedit_debug (DEBUG_SESSION);

	if (!client->save_yourself_emitted)
		gedit_file_close_all ();

	gedit_debug_message (DEBUG_FILE, "All files closed.");
	
	bonobo_mdi_destroy (BONOBO_MDI (gedit_mdi));
	
	gedit_debug_message (DEBUG_FILE, "Unref gedit_mdi.");

	g_object_unref (G_OBJECT (gedit_mdi));

	gedit_debug_message (DEBUG_FILE, "Unref gedit_mdi: DONE");

	gedit_debug_message (DEBUG_FILE, "Unref gedit_app_server.");

	bonobo_object_unref (gedit_app_server);

	gedit_debug_message (DEBUG_FILE, "Unref gedit_app_server: DONE");
#endif
	gedit_prefs_manager_app_shutdown ();
	gedit_metadata_manager_shutdown ();
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
	gedit_debug (DEBUG_SESSION);
	
	if (master_client)
		return;

	program_argv0 = argv0;
	
	master_client = gnome_master_client ();

	g_signal_connect (master_client,
			  "save_yourself",
			  G_CALLBACK (client_save_yourself_cb),
			  NULL);
	g_signal_connect (master_client,
			  "die",
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

	gedit_debug (DEBUG_SESSION);

	if (!master_client)
		return FALSE;

	restored = (gnome_client_get_flags (master_client) & GNOME_CLIENT_RESTORED) != 0;

	gedit_debug_message (DEBUG_SESSION, restored ? "RESTORED" : "NOT RESTORED");

	return restored;
}

static void
parse_window (xmlNodePtr node)
{
	GeditWindow *window;
	xmlChar *role;
	xmlNodePtr child;

	role = xmlGetProp (node, (const xmlChar *) "role");
	gedit_debug_message (DEBUG_SESSION, "Window role: %s", role);

	window = _gedit_app_restore_window (gedit_app_get_default (), role);

	if (role != NULL)
		xmlFree (role);

	if (window == NULL)
	{
		g_warning ("Couldn't restore window");
		return;
	}

	child = node->children;

	while (child != NULL)
	{
		if (strcmp ((char *) child->name, "side-pane") == 0)
		{
			xmlChar *visible;
			GeditPanel *panel;

			visible = xmlGetProp (child, (const xmlChar *) "visible");
			panel = gedit_window_get_side_panel (window);

			if ((visible != NULL) &&
			    (strcmp ((char *) visible, "yes") == 0))
			{
				gedit_debug_message (DEBUG_SESSION, "Side panel visible");
				gtk_widget_show (GTK_WIDGET (panel));
			}
			else
			{
				gedit_debug_message (DEBUG_SESSION, "Side panel _NOT_ visible");
				gtk_widget_hide (GTK_WIDGET (panel));
			}

			if (visible != NULL)
				xmlFree (visible);	
		}
		else if (strcmp ((char *) child->name, "bottom-panel") == 0)
		{
			xmlChar *visible;
			GeditPanel *panel;

			visible = xmlGetProp (child, (const xmlChar *) "visible");
			panel = gedit_window_get_bottom_panel (window);

			if ((visible != NULL) &&
			    (strcmp ((char *) visible, "yes") == 0))
			{
				gedit_debug_message (DEBUG_SESSION, "Bottom panel visible");
				gtk_widget_show (GTK_WIDGET (panel));
			}
			else
			{
				gedit_debug_message (DEBUG_SESSION, "Bottom panel _NOT_ visible");
				gtk_widget_hide (GTK_WIDGET (panel));
			}

			if (visible != NULL)
				xmlFree (visible);
		}
		else if  (strcmp ((char *) child->name, "document") == 0)
		{
			xmlChar *uri;
			xmlChar *active;

			uri = xmlGetProp (child, (const xmlChar *) "uri");
			if (uri != NULL)
			{
				gboolean jump_to;

				active =  xmlGetProp (child, (const xmlChar *) "active");
				if (active != NULL)
				{
					jump_to = (strcmp ((char *) active, "yes") == 0);
					xmlFree (active);
				}
				else
				{
					jump_to = FALSE;
				}

				gedit_debug_message (DEBUG_SESSION,
						     "URI: %s (%s)",
						     (gchar *) uri,
						     jump_to ? "active" : "not active");

				gedit_window_create_tab_from_uri (window,
								  (const gchar *)uri,
								  NULL,
								  0,
								  FALSE,
								  jump_to);

				xmlFree (uri);
			}
		}
		
		child = child->next;
	}
	gtk_widget_show (GTK_WIDGET (window));
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
	xmlDocPtr doc;
        xmlNodePtr child;
	gchar *fname;

	gedit_debug (DEBUG_SESSION);

	fname = get_session_file_path (master_client);
	gedit_debug_message (DEBUG_SESSION, "Session file: %s", fname);
	
	doc = xmlParseFile (fname);
	g_free (fname);

	if (doc == NULL)
		return FALSE;

	child = xmlDocGetRootElement (doc);

	/* skip the session node */
	child = child->children;

	while (child != NULL)
	{
		if (xmlStrEqual (child->name, (const xmlChar *) "window"))
		{
			gedit_debug_message (DEBUG_SESSION, "Restore window");

			parse_window (child);

			// ephy_gui_window_update_user_time (widget, user_time);

			//gtk_widget_show (widget);
		}

		child = child->next;
	}

	xmlFreeDoc (doc);

	return TRUE;
}
