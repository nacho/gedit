/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-file.c
 * This file is part of gedit
 *
 * Copyright (C) 1998, 1999 Alex Roberts, Evan Lawrence
 * Copyright (C) 2000, 2003 Chema Celorio, Paolo Maggi 
 * Copyright (C) 2004 Paolo Maggi
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
 * Modified by the gedit Team, 1998-2004. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libgnome/libgnome.h>
#include <libgnomeui/libgnomeui.h>
#include <libgnomevfs/gnome-vfs.h>

#include <eel/eel-vfs-extensions.h>
#include <eel/eel-alert-dialog.h>
#include <eel/eel-string.h>

#include "gedit2.h"
#include "gedit-file.h"
#include "gedit-debug.h"
#include "gedit-document.h"
#include "gedit-utils.h"
#include "gedit-mdi.h"
#include "gedit-recent.h" 
#include "gedit-file-selector-util.h"
#include "gedit-plugins-engine.h"
#include "recent-files/egg-recent-model.h"
#include "gedit-prefs-manager-app.h"
#include "gedit-metadata-manager.h"

#undef ENABLE_PROFILE 

#ifdef ENABLE_PROFILE
#define PROFILE(x) x
#else
#define PROFILE(x)
#endif

PROFILE (static GTimer *timer = NULL);

static gchar 	*get_dirname_from_uri 		(const char          *uri);

static gboolean  gedit_file_save_as_real 	(const gchar         *file_name, 
						 const GeditEncoding *encoding,
						 GeditMDIChild       *child);
static void	 open_files 			(void);
static void	 show_progress_bar 		(gchar               *uri, 
						 gboolean             show, 
						 gboolean             reverting);
static void 	 document_loading_cb 		(GeditDocument       *document,
						 gulong               size,
		     				 gulong               total_size,
		     				 gboolean             reverting);


static gchar* gedit_default_path = NULL;

/* Global variables for managing async loading */
static gint                 num_of_uris_to_open = 0;
static gint                 opened_uris 	= 0;
static GSList              *uris_to_open 	= NULL;
static const GeditEncoding *encoding_to_use 	= NULL;
static gint                 line 		= -1;
static GSList              *new_children 	= NULL;
static GSList		   *children_to_unref	= NULL;
static gint		    times_called	= 0;

static GtkWidget           *loading_window	= NULL;
static GtkWidget           *progress_bar	= NULL;

static gchar *
get_dirname_from_uri (const char *uri)
{
	GnomeVFSURI *vfs_uri;
	gchar *name;
	gchar *res;

	/* Make VFS version of URI. */
	vfs_uri = gnome_vfs_uri_new (uri);
	if (vfs_uri == NULL) {
		return NULL;
	}

	/* Extract name part. */
	name = gnome_vfs_uri_extract_dirname (vfs_uri);
	gnome_vfs_uri_unref (vfs_uri);

	res = g_strdup_printf ("file:///%s", name);
	g_free (name);

	return res;
}

void 
gedit_file_new (void)
{
	gint ret;
	GeditMDIChild* new_child = NULL;

	gedit_debug (DEBUG_FILE, "");

	new_child = gedit_mdi_child_new ();

	g_return_if_fail (new_child != NULL);
	g_return_if_fail (gedit_mdi != NULL);

	ret = bonobo_mdi_add_child (BONOBO_MDI (gedit_mdi), BONOBO_MDI_CHILD (new_child));
	g_return_if_fail (ret != FALSE);
	gedit_debug (DEBUG_COMMANDS, "Child added.");

	ret = bonobo_mdi_add_view (BONOBO_MDI (gedit_mdi), BONOBO_MDI_CHILD (new_child));
	g_return_if_fail (ret != FALSE);
	gedit_debug (DEBUG_COMMANDS, "View added.");
	
	gtk_widget_grab_focus (GTK_WIDGET (gedit_get_active_view ()));
}

void 
gedit_file_close (GtkWidget *view)
{
	BonoboMDIChild* child;

	gedit_debug (DEBUG_FILE, "");

	g_return_if_fail (view != NULL);

	child = bonobo_mdi_get_child_from_view (view);
	g_return_if_fail (child != NULL);

	if (g_list_length (bonobo_mdi_child_get_views (child)) > 1)
	{		
		bonobo_mdi_remove_view (BONOBO_MDI (gedit_mdi), view, FALSE);
		gedit_debug (DEBUG_COMMANDS, "View removed.");
	}
	else
	{
		bonobo_mdi_remove_child (BONOBO_MDI (gedit_mdi), child, FALSE);
		gedit_debug (DEBUG_COMMANDS, "Child removed.");
	}

	gedit_mdi_set_active_window_verbs_sensitivity (BONOBO_MDI (gedit_mdi));

	if (bonobo_mdi_get_active_child (BONOBO_MDI (gedit_mdi)) == NULL)
	{
		gedit_mdi_clear_active_window_statusbar (gedit_mdi);
	}

	gedit_debug (DEBUG_FILE, "END");
}

void
gedit_file_open (GeditMDIChild *active_child)
{
	const GeditEncoding *encoding = NULL;
	GSList              *files;
	gchar               *default_path;
	
	gedit_debug (DEBUG_FILE, "");

	default_path = NULL;
	
	if (active_child != NULL)
	{
		GeditDocument *doc;
		gchar *raw_uri;

		doc = active_child->document;
		g_return_if_fail (doc != NULL);

		raw_uri = gedit_document_get_raw_uri (doc);

		if ((raw_uri != NULL) && gedit_utils_uri_has_file_scheme (raw_uri))
		{
			default_path = get_dirname_from_uri (raw_uri);
		}

		g_free (raw_uri);
	}

	if (default_path == NULL)
		default_path = (gedit_default_path != NULL) ? 
			g_strdup (gedit_default_path) : NULL;

	files = gedit_file_selector_open_multi (
			GTK_WINDOW (gedit_get_active_window ()),
			TRUE,
		        _("Open File..."), 
			default_path,
			&encoding);

	gedit_file_open_uri_list (files, encoding, 0, TRUE);

	g_slist_free (files);
	g_free (default_path);
}

gboolean 
gedit_file_save (GeditMDIChild* child, gboolean force)
{
	gint ret;
	GeditDocument* doc = NULL;
	GError *error = NULL;
	gchar *uri = NULL;
	
	gedit_debug (DEBUG_FILE, "");

	g_return_val_if_fail (child != NULL, FALSE);
	
	doc = child->document;
	g_return_val_if_fail (doc != NULL, FALSE);
	
	if (gedit_document_is_untitled (doc))
	{
		gedit_debug (DEBUG_FILE, "Untitled");

		return gedit_file_save_as (child);
	}

	if (!force && !gedit_document_get_modified (doc))	
	{
		gchar *raw_uri;
		gboolean deleted = FALSE;
		
		gedit_debug (DEBUG_FILE, "Not modified");

		raw_uri = gedit_document_get_raw_uri (doc);
		if (raw_uri != NULL)
		{
			if (gedit_document_is_readonly (doc))
				deleted = FALSE;
			else
				deleted = !gedit_utils_uri_exists (raw_uri);
		}
		g_free (raw_uri);
			
		if (!deleted)
			return TRUE;
	}
	
	uri = gedit_document_get_uri (doc);
	g_return_val_if_fail (uri != NULL, FALSE);
	
	gedit_utils_flash_va (_("Saving document \"%s\"..."), uri);	
	
	ret = gedit_document_save (doc, &error);

	if (!ret)
	{
		GtkWidget *view;
		
		g_return_val_if_fail (error != NULL, FALSE);
		gedit_debug (DEBUG_FILE, "FAILED");

		view = GTK_WIDGET (g_list_nth_data (
					bonobo_mdi_child_get_views (BONOBO_MDI_CHILD (child)), 0));
		if (view != NULL)
		{
			GtkWindow *window;

			window = GTK_WINDOW (bonobo_mdi_get_window_from_view (view));
			gtk_window_present (window);

			bonobo_mdi_set_active_view (BONOBO_MDI (gedit_mdi), view);
		}
		
		gedit_utils_error_reporting_saving_file (uri, error,
					GTK_WINDOW (gedit_get_active_window ()));

		g_error_free (error);

		gedit_utils_flash_va (_("The document \"%s\" has not been saved."), uri);

		g_free (uri);

		return FALSE;
	}	
	else
	{
		EggRecentModel *recent;
		EggRecentItem *item;
		gchar *raw_uri;
		
		gedit_debug (DEBUG_FILE, "OK");

		gedit_utils_flash_va (_("The document \"%s\" has been saved."), uri);

		g_free (uri);

		raw_uri = gedit_document_get_raw_uri (doc);
		g_return_val_if_fail (raw_uri != NULL, TRUE);
		
		recent = gedit_recent_get_model ();
		item = egg_recent_item_new_from_uri (raw_uri);
		egg_recent_item_add_group (item, "gedit");
		egg_recent_model_add_full (recent, item);
		egg_recent_item_unref (item);

		g_free (raw_uri);
		
		return TRUE;
	}
}

gboolean
gedit_file_save_as (GeditMDIChild *child)
{
	gchar *file;
	gboolean ret = FALSE;
	GeditDocument *doc;
	GtkWidget *view;
	gchar *fname = NULL;
	gchar *untitled_name = NULL;
	gchar *path = NULL;
	const GeditEncoding *encoding;

	gedit_debug (DEBUG_FILE, "");

	g_return_val_if_fail (child != NULL, FALSE);

	doc = child->document;
	g_return_val_if_fail (doc != NULL, FALSE);

	view = GTK_WIDGET (g_list_nth_data (
			bonobo_mdi_child_get_views (BONOBO_MDI_CHILD (child)), 0));
	if (view != NULL)
	{
		GtkWindow *window;

		window = GTK_WINDOW (bonobo_mdi_get_window_from_view (view));
		gtk_window_present (window);

		bonobo_mdi_set_active_view (BONOBO_MDI (gedit_mdi), view);
	}
	
	if (gedit_document_is_untitled (doc))
	{
		path = (gedit_default_path != NULL) ? 
			g_strdup (gedit_default_path) : NULL;

		untitled_name = gedit_document_get_uri (doc);
		if (untitled_name == NULL)
			untitled_name = g_strdup ("Untitled"); /* Use ASCII */

		g_return_val_if_fail (untitled_name != NULL, FALSE);
	}
	else
	{
		gchar *raw_uri = gedit_document_get_raw_uri (doc);

		g_return_val_if_fail (raw_uri != NULL, FALSE);

		if (gedit_utils_uri_has_file_scheme (raw_uri))
		{
			fname = eel_uri_get_basename (raw_uri);
			g_return_val_if_fail (fname != NULL, FALSE);

			path = get_dirname_from_uri (raw_uri);
		}
		else
		{
			untitled_name = gedit_document_get_short_name (doc);
			g_return_val_if_fail (untitled_name != NULL, FALSE);

			path = (gedit_default_path != NULL) ? 
				g_strdup (gedit_default_path) : NULL;
		}

		g_free (raw_uri);
	}

	encoding = gedit_document_get_encoding (doc);

	file = gedit_file_selector_save (
			GTK_WINDOW (bonobo_mdi_get_active_window (BONOBO_MDI (gedit_mdi))),
			FALSE,
		        _("Save as..."), 
			path,
			fname,
			untitled_name,
			&encoding);
	
	g_free (fname);
	g_free (untitled_name);
	g_free (path);

	if (file != NULL) 
	{
		gchar *uri;
		gchar *file_utf8;

		uri = eel_make_uri_from_shell_arg (file);
		g_return_val_if_fail (uri != NULL, FALSE);

		file_utf8 = eel_format_uri_for_display (uri);
		if (file_utf8 != NULL)
			gedit_utils_flash_va (_("Saving document \"%s\"..."), file_utf8);
		
		ret = gedit_file_save_as_real (uri, encoding, child);
		
		if (ret)
		{			
			if (gedit_default_path != NULL)
				g_free (gedit_default_path);

			gedit_default_path = get_dirname_from_uri (file);

			if (file_utf8 != NULL)
			{
				gedit_utils_flash_va (_("The document \"%s\" has been saved."), 
						      file_utf8);
			}
		}
		else
		{
			if (file_utf8 != NULL)
			{
				gedit_utils_flash_va (_("The document \"%s\" has not been saved."),
						      file_utf8);
			}
		}

		gedit_debug (DEBUG_FILE, "File: %s", file);
		g_free (uri);
		g_free (file);

		if (file_utf8 != NULL)
			g_free (file_utf8);
		
	}

	return ret;
}

static gboolean
gedit_file_save_as_real (const gchar* file_name, const GeditEncoding *encoding, GeditMDIChild *child)
{
	gchar *uri;
	gboolean ret;
	GeditDocument *doc = NULL;
	GError *error = NULL;

	gedit_debug (DEBUG_FILE, "");

	g_return_val_if_fail (child != NULL, FALSE);

	doc = child->document;
	g_return_val_if_fail (doc != NULL, FALSE);

	uri = eel_make_uri_canonical (file_name);
	g_return_val_if_fail (uri != NULL, FALSE);
	
	ret = gedit_document_save_as (doc, uri, encoding, &error);

	if (!ret)
	{
		g_return_val_if_fail (error != NULL, FALSE);

		gedit_utils_error_reporting_saving_file (file_name, error,
					GTK_WINDOW (gedit_get_active_window ()));

		g_error_free (error);
		
		g_free (uri);

		return FALSE;
		
	}	
	else
	{
		EggRecentModel *recent;
		EggRecentItem *item;

		recent = gedit_recent_get_model ();
		item = egg_recent_item_new_from_uri (uri);
		egg_recent_item_add_group (item, "gedit");
		egg_recent_model_add_full (recent, item);
		egg_recent_item_unref (item);

		g_free (uri);

		return TRUE;
	}
}

gboolean
gedit_file_close_all (void)
{
	gboolean ret;
	gedit_debug (DEBUG_FILE, "");

	ret = gedit_mdi_remove_all (gedit_mdi);

	gedit_mdi_set_active_window_verbs_sensitivity (BONOBO_MDI (gedit_mdi));

	if (bonobo_mdi_get_active_child (BONOBO_MDI (gedit_mdi)) == NULL)
	{
		gedit_mdi_clear_active_window_statusbar (gedit_mdi); 
	}

	return ret;
}

void
gedit_file_exit (void)
{
	gedit_debug (DEBUG_FILE, "");
	
	if (!gedit_file_close_all ())
		return;

	gedit_debug (DEBUG_FILE, "All files closed.");
	
	bonobo_mdi_destroy (BONOBO_MDI (gedit_mdi));
	
	gedit_debug (DEBUG_FILE, "Unref gedit_mdi.");

	g_object_unref (G_OBJECT (gedit_mdi));

	gedit_debug (DEBUG_FILE, "Unref gedit_mdi: DONE");

	gedit_debug (DEBUG_FILE, "Unref gedit_app_server.");

	bonobo_object_unref (gedit_app_server);

	gedit_debug (DEBUG_FILE, "Unref gedit_app_server: DONE");

	gedit_prefs_manager_app_shutdown ();
	gedit_metadata_manager_shutdown ();
	gedit_plugins_engine_shutdown ();

	gtk_main_quit ();
}

void
gedit_file_save_all (void)
{
	guint i = 0;
	GeditMDIChild* child;
	GtkWidget* view;

	gedit_debug (DEBUG_FILE, "");

	view = bonobo_mdi_get_active_view (BONOBO_MDI (gedit_mdi));

	for (i = 0; i < g_list_length (bonobo_mdi_get_children (BONOBO_MDI (gedit_mdi))); i++)
	{
		child = GEDIT_MDI_CHILD (g_list_nth_data (
				bonobo_mdi_get_children (BONOBO_MDI (gedit_mdi)), i));

		gedit_file_save (child, FALSE);	
	}

	if (view != bonobo_mdi_get_active_view (BONOBO_MDI (gedit_mdi)))
	{
		GtkWindow *window;

		window = GTK_WINDOW (bonobo_mdi_get_window_from_view (view));
		gtk_window_present (window);

		bonobo_mdi_set_active_view (BONOBO_MDI (gedit_mdi), view);
	}
}

/* Displays a confirmation dialog for whether to revert a file. */
static gboolean
gedit_file_revert_dialog (GeditDocument *doc)
{
	GtkWidget *msgbox;
	AtkObject *obj;
	gint ret;
	gchar *name;
	gchar *primary_msg;
	gchar *secondary_msg;
	glong seconds;

	name = gedit_document_get_short_name (doc);
	primary_msg = g_strdup_printf (_("Revert unsaved changes to document \"%s\"?"),
	                               name);
	g_free (name);

	seconds = MAX (1, gedit_document_get_seconds_since_last_save_or_load (doc));

	if (seconds < 55)	
	{
		secondary_msg = g_strdup_printf (
					ngettext ("Changes made to the document in the last %ld second "
					    	  "will be permanently lost.",
						  "Changes made to the document in the last %ld seconds "
					    	  "will be permanently lost.",
						  seconds),
					seconds);
	}
	else if (seconds < 75) /* 55 <= seconds < 75 */
	{
		secondary_msg = g_strdup (_("Changes made to the document in the last minute "
					    "will be permanently lost."));
	}
	else if (seconds < 110) /* 75 <= seconds < 110 */
	{
		secondary_msg = g_strdup_printf (
					ngettext ("Changes made to the document in the last minute and "
						  "%ld second will be permanently lost.",
						  "Changes made to the document in the last minute and "
						  "%ld seconds will be permanently lost.",
						  seconds - 60 ),
					seconds - 60);
	}
	else if (seconds < 3600)
	{
		secondary_msg = g_strdup_printf (
					ngettext ("Changes made to the document in the last %ld minute "
					    	  "will be permanently lost.",
						  "Changes made to the document in the last %ld minutes "
					    	  "will be permanently lost.",
						  seconds / 60),
					seconds / 60);
	}
	else if (seconds < 7200)
	{
		gint minutes;
		seconds -= 3600;

		minutes = seconds / 60;
		if (minutes < 5)
		{
			secondary_msg = g_strdup (_("Changes made to the document in the last hour "
						    "will be permanently lost."));
		}
		else
		{
			secondary_msg = g_strdup_printf (
					ngettext ("Changes made to the document in the last hour and "
						  "%d minute will be permanently lost.",
						  "Changes made to the document in the last hour and "
						  "%d minutes will be permanently lost.",
						  minutes),
					minutes);
		}
	}
	else
	{
		gint hours;

		hours = seconds / 3600;

		secondary_msg = g_strdup_printf (
					ngettext ("Changes made to the document in the last %d hour "
					    	  "will be permanently lost.",
						  "Changes made to the document in the last %d hours "
					    	  "will be permanently lost.",
						  hours),
					hours);
	}

	msgbox = eel_alert_dialog_new (GTK_WINDOW (gedit_get_active_window ()),
				       GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				       GTK_MESSAGE_QUESTION,
				       GTK_BUTTONS_NONE,
				       primary_msg,
				       secondary_msg, 
				       NULL);
	g_free (primary_msg);
	g_free (secondary_msg);

	/* Add Cancel button */
	gtk_dialog_add_button (GTK_DIALOG (msgbox), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);

	/* Add Revert button */
	gedit_dialog_add_button (GTK_DIALOG (msgbox), 
			_("_Revert"), GTK_STOCK_REVERT_TO_SAVED, GTK_RESPONSE_YES);

	gtk_dialog_set_default_response	(GTK_DIALOG (msgbox), GTK_RESPONSE_CANCEL);

	obj = gtk_widget_get_accessible (msgbox);
	if (GTK_IS_ACCESSIBLE (obj))
		atk_object_set_name (obj, _("Question"));

	gtk_window_set_resizable (GTK_WINDOW (msgbox), FALSE);

	ret = gtk_dialog_run (GTK_DIALOG (msgbox));
		
	gtk_widget_destroy (msgbox);

	return (ret == GTK_RESPONSE_YES);
}

static void 
document_reverted_cb (GeditDocument *document,
		      const GError  *error)
{
	gchar *uri_to_display;
	gchar *raw_uri;
	
	raw_uri = gedit_document_get_raw_uri (document);
	g_return_if_fail (raw_uri != NULL);
	
	uri_to_display = gedit_document_get_uri (document);
	g_return_if_fail (uri_to_display != NULL);

	show_progress_bar (NULL, FALSE, FALSE);
	times_called = 0;

	PROFILE (
		g_message ("Received 'loaded' signal: %.3f", g_timer_elapsed (timer, NULL));
	)

	g_signal_handlers_disconnect_by_func (G_OBJECT (document),
				  	      G_CALLBACK (document_loading_cb),
				  	      (gpointer)TRUE);

	g_signal_handlers_disconnect_by_func (G_OBJECT (document),
				  	      G_CALLBACK (document_reverted_cb),
				  	      NULL);

	gedit_utils_set_status (NULL);

	if (error != NULL)
	{
		gedit_utils_flash_va (_("Error reverting the document \"%s\"."), uri_to_display);

		gedit_debug (DEBUG_DOCUMENT, "Error reverting file %s", uri_to_display);

		gedit_utils_error_reporting_reverting_file (raw_uri, error,
					GTK_WINDOW (gedit_get_active_window ()));

		gedit_utils_flash_va (_("The document \"%s\" has not been reverted."), uri_to_display);
	}
	else
	{
		gedit_utils_flash_va (_("The document \"%s\" has been reverted."), uri_to_display);
	}

	gedit_mdi_set_state (gedit_mdi, GEDIT_STATE_NORMAL);

	g_free (uri_to_display);
	g_free (raw_uri);
}

void
gedit_file_revert (GeditMDIChild *child)
{
	GeditDocument *doc = NULL;
	gchar *uri = NULL;

	gedit_debug (DEBUG_FILE, "");

	g_return_if_fail (child != NULL);
	
	doc = child->document;
	g_return_if_fail (doc != NULL);
	
	if (!gedit_file_revert_dialog (doc))
		return;
	
	uri = gedit_document_get_uri (doc);
	g_return_if_fail (uri != NULL);
	
	gedit_utils_flash_va (_("Reverting the document \"%s\"..."), uri);	
	
	g_signal_connect (G_OBJECT (doc),
			  "loading",
			  G_CALLBACK (document_loading_cb),
			  (gpointer)TRUE);
				  
	g_signal_connect (G_OBJECT (doc),
			  "loaded",
			  G_CALLBACK (document_reverted_cb),
			  NULL);
	
	gedit_document_revert (doc);
}

gboolean 
gedit_file_open_recent (EggRecentView *view, EggRecentItem *item, gpointer data)
{
	gboolean ret = FALSE;
	GeditView* active_view;
	gchar *uri_utf8;

	if (gedit_mdi_get_state (gedit_mdi) != GEDIT_STATE_NORMAL)
		return TRUE;
	
	uri_utf8 = egg_recent_item_get_uri_utf8 (item);

	gedit_debug (DEBUG_FILE, "Open : %s", uri_utf8);

	/* Note that gedit_file_open_single_uri takes a possibly mangled "uri", in UTF8 */

	/* FIXME */
	ret = gedit_file_open_single_uri (uri_utf8, NULL);
	
	if (!ret) 
	{
		EggRecentModel *model;
		gchar *uri;

		uri = egg_recent_item_get_uri (item);

		model = gedit_recent_get_model ();
		egg_recent_model_delete (model, uri);

		g_free (uri);
	}
		
	active_view = gedit_get_active_view ();
	if (active_view != NULL)
		gtk_widget_grab_focus (GTK_WIDGET (active_view));

	g_free (uri_utf8);

	gedit_debug (DEBUG_FILE, "END");

	return ret;
}

/*
 *  uri: a possibly mangled "uri", in UTF8
 */
gboolean 
gedit_file_open_single_uri (const gchar* uri, const GeditEncoding *encoding)
{
	GSList *uri_list;
	gchar *full_path;

	gedit_debug (DEBUG_FILE, "");
	
	if (uri == NULL) 
		return FALSE;

	full_path = eel_make_uri_from_input (uri);

	uri_list = g_slist_prepend (NULL, full_path);
	
	gedit_file_open_uri_list (uri_list, encoding, 0, FALSE);

	return TRUE;
}

/* FIXME: it is broken */
gboolean
gedit_file_open_from_stdin (GeditMDIChild *active_child)
{
	struct stat stats;
	gboolean ret = TRUE;
	GeditDocument *doc = NULL;
	GError *error = NULL;
	GeditMDIChild *child;
	GeditMDIChild* new_child = NULL;

	gedit_debug (DEBUG_FILE, "");
	
	fstat (STDIN_FILENO, &stats);
	
	if (stats.st_size  == 0)
		return FALSE;

	child = active_child;
	
	if (active_child == NULL ||
	    !gedit_document_is_untouched (active_child->document))	     
	{
		new_child = gedit_mdi_child_new ();

		g_return_val_if_fail (new_child != NULL, FALSE);
		g_return_val_if_fail (gedit_mdi != NULL, FALSE);

		ret = bonobo_mdi_add_child (BONOBO_MDI (gedit_mdi), BONOBO_MDI_CHILD (new_child));
		g_return_val_if_fail (ret != FALSE, FALSE);
		gedit_debug (DEBUG_FILE, "Child added.");

		child= new_child;
	}

	doc = child->document;
	g_return_val_if_fail (doc != NULL, FALSE);

	ret = gedit_document_load_from_stdin (doc, &error);

	if (error)
	{
		GtkWidget *dialog;
		/* FIXME: do a more user friendly error reporting */
		gchar *errstr;
	       	
		errstr = g_strdup_printf (_("Could not read data from stdin."));
		
		dialog = gtk_message_dialog_new (
				GTK_WINDOW (bonobo_mdi_get_active_window (BONOBO_MDI (gedit_mdi))),
				GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			   	GTK_MESSAGE_ERROR,
			   	GTK_BUTTONS_OK,
				errstr);

		gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

		gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);

		g_free (errstr);
		g_error_free (error);

		ret = FALSE;
	}
	
	if (new_child != NULL)
	{
		ret = bonobo_mdi_add_view (BONOBO_MDI (gedit_mdi), BONOBO_MDI_CHILD (new_child));
		g_return_val_if_fail (ret != FALSE, FALSE);
		gedit_debug (DEBUG_FILE, "View added.");
	}
	
	return ret;
}


static gboolean
create_new_file (const gchar *uri)
{
	GtkWidget *dialog;
	gboolean   created;
	gchar     *formatted_uri;

	formatted_uri = gnome_vfs_format_uri_for_display (uri);
	g_return_val_if_fail (formatted_uri != NULL, FALSE);
			
	created = FALSE;

	/* FIXME: Is it conforming to GNOME HIG ? - Paolo (Jan 02, 2004) */	
	dialog = gtk_message_dialog_new (GTK_WINDOW (gedit_get_active_window ()),
					 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
					 GTK_MESSAGE_QUESTION,
					 GTK_BUTTONS_YES_NO,
					 _("The file \"%s\" does not exist. Would you like to create it?"),
					 formatted_uri);

	g_free (formatted_uri);

	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_NO);

	gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_YES)
	{
		created = gedit_utils_create_empty_file (uri);

		if (!created)
		{
			gedit_utils_error_reporting_creating_file (uri, 
								   errno, 
								   GTK_WINDOW (dialog));
		}
	}
									
	gtk_widget_destroy (dialog);

	return created;
}

static gboolean
update_recent_files_real (const gchar *uri)
{
	EggRecentModel *recent;
	EggRecentItem  *item;

	recent = gedit_recent_get_model ();
	item = egg_recent_item_new_from_uri (uri);
	egg_recent_item_add_group (item, "gedit");
	egg_recent_model_add_full (recent, item);
	egg_recent_item_unref (item);

	return FALSE;
}

static void
update_recent_files (const gchar *uri)
{
	g_idle_add_full (G_PRIORITY_DEFAULT_IDLE,
			 (GSourceFunc)update_recent_files_real,
			 g_strdup (uri),
			 (GDestroyNotify)g_free);
}

static gboolean
remove_recent_file_real (const gchar *uri)
{
	EggRecentModel *recent;

	recent = gedit_recent_get_model ();
	egg_recent_model_delete (recent, uri);

	return FALSE;
}

static void
remove_recent_file (const gchar *uri)
{
	g_idle_add_full (G_PRIORITY_DEFAULT_IDLE,
			 (GSourceFunc)remove_recent_file_real,
			 g_strdup (uri),
			 (GDestroyNotify)g_free);
}


#define MAX_URI_IN_DIALOG_LENGTH 50

static void
show_loading_dialog (GtkWindow *parent, gchar *uri, gboolean reverting)
{
	GtkWidget *frame;
	GtkWidget *hbox;
	GtkWidget *image;
	GtkWidget *vbox;
	GtkWidget *label;
	gchar *str;
	gchar *uri_for_display;
	gchar *full_formatted_uri;
	GdkCursor *cursor;

	g_return_if_fail (GTK_IS_WINDOW (parent));
	g_return_if_fail (uri != NULL);

	full_formatted_uri = gnome_vfs_format_uri_for_display (uri);

	/* Truncate the URI so it doesn't get insanely wide.  */
        uri_for_display = eel_str_middle_truncate (full_formatted_uri, 
						   MAX_URI_IN_DIALOG_LENGTH);
	g_free (full_formatted_uri);

	g_return_if_fail (uri_for_display != NULL);

	loading_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_modal (GTK_WINDOW (loading_window), FALSE);
	gtk_window_set_resizable (GTK_WINDOW (loading_window), FALSE);
	gtk_window_set_destroy_with_parent (GTK_WINDOW (loading_window), TRUE);
	gtk_window_set_position (GTK_WINDOW (loading_window), GTK_WIN_POS_CENTER_ON_PARENT);
		
	gtk_window_set_decorated (GTK_WINDOW (loading_window), FALSE); 
	gtk_window_set_transient_for (GTK_WINDOW (loading_window), parent);

	frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
	gtk_container_add (GTK_CONTAINER (loading_window), frame);

	hbox = gtk_hbox_new (FALSE, 12);
 	gtk_container_add (GTK_CONTAINER (frame), hbox);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 12);

	image = gtk_image_new_from_stock ("gtk-open", GTK_ICON_SIZE_DIALOG);
	gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);

	vbox = gtk_vbox_new (FALSE, 12);
	gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);

	if (reverting)
	{
		str = g_strdup_printf ("<b>%s</b>\n%s", _("Reverting file:"), uri_for_display);
	}
	else
	{
		str = g_strdup_printf ("<b>%s</b>\n%s", _("Loading file:"), uri_for_display);		
	}
	
	label = gtk_label_new (str);
	g_free (uri_for_display);
	g_free (str);
	
	gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
	gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);

	progress_bar = gtk_progress_bar_new ();
	gtk_box_pack_start (GTK_BOX (vbox), progress_bar, FALSE, FALSE, 0);

	gtk_widget_show_all (loading_window);

	cursor = gdk_cursor_new_for_display (
			gtk_widget_get_display (loading_window),
			GDK_WATCH);
				
	gdk_window_set_cursor (GTK_WIDGET (loading_window)->window,
			       cursor);

	gdk_cursor_unref (cursor);	
}

static void
show_progress_bar (gchar *uri, gboolean show, gboolean reverting)
{
	if (show)
	{
		BonoboWindow *win;

		if (loading_window != NULL)
			return;
		
		win = gedit_get_active_window ();
		g_return_if_fail (win != NULL);

		show_loading_dialog (GTK_WINDOW (win), uri, reverting);
	}
	else
	{
		if (loading_window == NULL)
			return;

		gtk_object_destroy (GTK_OBJECT (loading_window));
		loading_window = NULL;
		progress_bar = NULL;
	}
}

static void
set_progress_bar (gulong size, gulong total_size)
{
	if (progress_bar == NULL)
		return;

	if (total_size == 0)
	{
		gtk_progress_bar_pulse (GTK_PROGRESS_BAR (progress_bar));		
	}
	else
	{
		gdouble frac;

		frac = (gdouble)size / (gdouble)total_size;

		gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (progress_bar),
					       frac);
	}
}

static void 
document_loading_cb (GeditDocument *document,
		     gulong         size,
		     gulong         total_size,
		     gboolean       reverting)
{
	static GTimer *d_timer = NULL;
	
	gchar *uri;
	double et;

	uri = (gchar*)uris_to_open->data;
	g_return_if_fail (uri != NULL);

	if (d_timer == NULL)
	{
		d_timer = g_timer_new ();
	}
	else
	{
		if (times_called == 0)
		{
			g_timer_reset (d_timer);
		}
	}

	et = g_timer_elapsed (d_timer, NULL);

	if (times_called == 1)
	{
		if ((total_size == 0) || (total_size > 51200UL) /* 50 KB */)
		{
			show_progress_bar (uri, TRUE, reverting);
		}
	}
	else
	{
		if ((times_called == 3) && (total_size != 0))
		{
			gdouble total_time;

			/* et : total_time = size : total_size */
			total_time = (et * total_size)/size;

			if ((total_time - et) > 3.0)
			{
				show_progress_bar (uri, TRUE, reverting);
			}
		}
		else
		{
			if (et > 3.0)
			{
				show_progress_bar (uri, TRUE, reverting);
			}
		}
	}

	set_progress_bar (size, total_size);

	times_called++;
}

static void 
document_loaded_cb (GeditDocument *document,
		    const GError  *error,
		    GeditMDIChild *new_child)
{
	gchar *uri;
	gchar *uri_to_display;
	
	uri = (gchar*)uris_to_open->data;
	g_return_if_fail (uri != NULL);

	show_progress_bar (NULL, FALSE, FALSE);
	times_called = 0;

	PROFILE (
		g_message ("Received 'loaded' signal: %.3f", g_timer_elapsed (timer, NULL));
	)

	g_signal_handlers_disconnect_by_func (G_OBJECT (document),
				  	      G_CALLBACK (document_loading_cb),
				  	      (gpointer)FALSE);

	g_signal_handlers_disconnect_by_func (G_OBJECT (document),
				  	      G_CALLBACK (document_loaded_cb),
				  	      new_child);

	uri_to_display = gnome_vfs_format_uri_for_display (uri);

	if (error != NULL)
	{
		gedit_utils_set_status (NULL);
		gedit_utils_flash_va (_("Error loading file \"%s\""), uri_to_display);

		gedit_debug (DEBUG_DOCUMENT, "Error loading file %s", uri_to_display);

		remove_recent_file (uri);

		gedit_utils_error_reporting_loading_file (uri, encoding_to_use, (GError *)error,
					GTK_WINDOW (gedit_get_active_window ()));

		if (new_child != NULL)
		{
			children_to_unref = g_slist_prepend (children_to_unref, new_child);
		}

		gedit_debug (DEBUG_DOCUMENT, "Returning %s", uri_to_display);
	}
	else
	{	
		opened_uris++;

		gedit_utils_set_status (NULL);
		gedit_utils_flash_va (_("Loaded file \"%s\""), uri_to_display);

		gedit_document_goto_line (document, MAX (0, line - 1));

		if (new_child != NULL)
		{
			bonobo_mdi_add_child (BONOBO_MDI (gedit_mdi), BONOBO_MDI_CHILD (new_child));
			gedit_debug (DEBUG_FILE, "Child added.");

			new_children = g_slist_prepend (new_children, new_child);
		}
	
		if (gedit_utils_uri_has_file_scheme (uri))
		{
			gchar *default_path;
			
			default_path = get_dirname_from_uri (uri);

			if (default_path != NULL)
			{
				g_free (gedit_default_path);
				
				gedit_default_path = default_path;
			}
		}

		PROFILE (
			g_message ("Updated gedit_default_path: %.3f", g_timer_elapsed (timer, NULL));
		)

		update_recent_files (uri);	
	}

	g_free (uri_to_display);
	
	/* Load the next file in the uris_to_open list */
	g_free (uri);
	uris_to_open = g_slist_delete_link (uris_to_open, uris_to_open);

	open_files ();
}

static void
open_files ()
{
	GeditDocument  *active_document;
	
	gchar          *uri;
	gchar          *uri_to_display;

	if (uris_to_open == NULL)
	{		
		/* Add all the views in the right order */
		new_children = g_slist_reverse (new_children);

		PROFILE (
			g_message ("Document Loaded: %.3f", g_timer_elapsed (timer, NULL));
		)
		
		bonobo_mdi_add_views (BONOBO_MDI (gedit_mdi),
				      new_children);
	
		PROFILE (
			g_message ("View added: %.3f", g_timer_elapsed (timer, NULL));
		)
	
		g_slist_free (new_children);
		new_children = NULL;

		/* Unref the children that were not opened to an error */
		g_slist_foreach (children_to_unref, (GFunc)g_object_unref, NULL);
		g_slist_free (children_to_unref);
		children_to_unref = NULL;
		
		if (num_of_uris_to_open > 1)
		{
			gedit_utils_set_status (NULL);
			gedit_utils_flash_va (ngettext("Loaded %d file",
						       "Loaded %d files", 
						       opened_uris), 
					      opened_uris);
		}

		/* Clean up temp variables */
		num_of_uris_to_open = 0;
		opened_uris = 0;
		encoding_to_use = NULL;
		line = -1;

		PROFILE (
			g_message ("Done all: %.3f", g_timer_elapsed (timer, NULL));
			g_timer_destroy (timer);
		)

		gedit_mdi_set_state (gedit_mdi, GEDIT_STATE_NORMAL);
		return;
	}
	
	uri = (gchar*)uris_to_open->data;
	g_return_if_fail (uri != NULL);
	
	gedit_debug (DEBUG_FILE, "File name: %s", uri);

	active_document = gedit_get_active_document ();
	
	uri_to_display = gnome_vfs_format_uri_for_display (uri);

	PROFILE (
		g_message ("URI to open: %.3f", g_timer_elapsed (timer, NULL));
	)

	gedit_utils_set_status_va (_("Loading file \"%s\"..."), uri_to_display);

	PROFILE (
		g_message ("URI to open (after flash): %.3f", g_timer_elapsed (timer, NULL));
	)

	if (active_document == NULL ||
	    !gedit_document_is_untouched (active_document))	     
	{
		GeditMDIChild *new_child;
		
		PROFILE (
			g_message ("Create new_child: %.3f", g_timer_elapsed (timer, NULL));
		)

		new_child = gedit_mdi_child_new_with_uri (uri, encoding_to_use);

		PROFILE (
			g_message ("new_child created: %.3f", g_timer_elapsed (timer, NULL));
		)

		if (new_child == NULL)
		{
			/* FIXME: this is a too generic error message - Paolo */
			gedit_utils_error_reporting_loading_file (uri, encoding_to_use, NULL,
					GTK_WINDOW (gedit_get_active_window ()));

			gedit_utils_flash_va (_("Error loading file \"%s\""), uri_to_display);

			g_free (uri_to_display);
			g_free (uri);
			 
			uris_to_open = g_slist_delete_link (uris_to_open, uris_to_open);

			open_files ();

			return;
		}

		g_signal_connect (G_OBJECT (new_child->document),
				  "loading",
				  G_CALLBACK (document_loading_cb),
				  (gpointer)FALSE);
				  
		g_signal_connect (G_OBJECT (new_child->document),
				  "loaded",
				  G_CALLBACK (document_loaded_cb),
				  new_child);

		g_return_if_fail (gedit_mdi != NULL);
	}
	else
	{	
		PROFILE (
			g_message ("Load file: %.3f", g_timer_elapsed (timer, NULL));
		)

		g_signal_connect (G_OBJECT (active_document),
				  "loading",
				  G_CALLBACK (document_loading_cb),
				  (gpointer)FALSE);
				  
		g_signal_connect (G_OBJECT (active_document),
				  "loaded",
				  G_CALLBACK (document_loaded_cb),
				  NULL);

		gedit_document_load (active_document, uri, encoding_to_use);

		PROFILE (
			g_message ("Start loading: %.3f", g_timer_elapsed (timer, NULL));
		)

	}
	
	g_free (uri_to_display);
}

/* Note that all the uris will be opened with the same encoding */
gboolean
gedit_file_open_uri_list (GSList *uri_list,
			  const GeditEncoding *encoding,
			  gint line_pos,
			  gboolean create)
{
	gedit_debug (DEBUG_FILE, "");
	
	if (uri_list == NULL)
		return TRUE;

	g_return_val_if_fail (uris_to_open == NULL, FALSE);

	g_return_val_if_fail (new_children == NULL, FALSE);
	g_return_val_if_fail (children_to_unref == NULL, FALSE);

	g_return_val_if_fail (num_of_uris_to_open == 0, FALSE);
	g_return_val_if_fail (opened_uris == 0, FALSE);
	g_return_val_if_fail (encoding_to_use == NULL, FALSE);
	
	g_return_val_if_fail (times_called == 0, FALSE);

	g_return_val_if_fail (loading_window == NULL, FALSE);
	g_return_val_if_fail (progress_bar == NULL, FALSE);

	PROFILE (
		timer = g_timer_new ();
	)

	while (uri_list != NULL)
	{
		gchar *uri;

		uri = gnome_vfs_make_uri_canonical ((const gchar*)uri_list->data);
		
		gedit_debug (DEBUG_FILE, "URI: %s", uri);

		if (uri != NULL)
		{
			if (!gedit_utils_uri_has_file_scheme (uri) || 
			     gedit_utils_uri_exists (uri))
			{
				uris_to_open = g_slist_prepend (uris_to_open, uri);
			}
			else
			{
				if (create)
				{
					if (create_new_file (uri))
					{
						uris_to_open = g_slist_prepend (uris_to_open, uri);
					}
				}
				else
				{
					uris_to_open = g_slist_prepend (uris_to_open, uri);
				}
			}		
		}	
			
		uri_list = g_slist_next (uri_list);
	}

	uris_to_open = g_slist_reverse (uris_to_open);

	if (uris_to_open != NULL)
	{
		num_of_uris_to_open = g_slist_length (uris_to_open);

		if (num_of_uris_to_open > 1)
		{
			gedit_utils_set_status_va (ngettext("Loading %d file...",
							    "Loading %d files...", 
							    num_of_uris_to_open),
						   num_of_uris_to_open);
		}

		encoding_to_use = encoding;
		line = line_pos > 0 ? line_pos : 0;

		gedit_mdi_set_state (gedit_mdi, GEDIT_STATE_LOADING);
		
		open_files ();
	}

	return TRUE;
}

