/*
 * savecopy.c
 * This file is part of gedit
 *
 * Copyright (C) 2005 - Paolo Borelli and Paolo Maggi
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
 */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include <glib/gi18n.h>
#include <libgnome/gnome-help.h>
#include <libgnomevfs/gnome-vfs.h>

#include <gedit/gedit-plugin.h>
#include <gedit/gedit-debug.h>
#include <gedit/gedit-convert.h>
#include <gedit/gedit-encodings.h>
#include <gedit/gedit-encodings-option-menu.h>
#include <gedit/gedit-menus.h>
#include <gedit/gedit-utils.h>


#define MENU_ITEM_LABEL	N_("Sa_ve Copy...")
#define MENU_ITEM_NAME	"SaveCopy"
#define MENU_ITEM_PATH	"/menu/File/FileOps_0/"
#define MENU_ITEM_TIP	N_("Save a copy of the current document")


G_MODULE_EXPORT GeditPluginState update_ui (GeditPlugin *plugin, BonoboWindow *window);
G_MODULE_EXPORT GeditPluginState init (GeditPlugin *pd);
G_MODULE_EXPORT GeditPluginState destroy (GeditPlugin *pd);
G_MODULE_EXPORT GeditPluginState activate (GeditPlugin *pd);
G_MODULE_EXPORT GeditPluginState deactivate (GeditPlugin *pd);


static gchar *
get_buffer_contents (GeditDocument *doc)
{
	GtkTextIter start, end;

	gtk_text_buffer_get_bounds (GTK_TEXT_BUFFER (doc), &start, &end);
	return gtk_text_buffer_get_slice (GTK_TEXT_BUFFER (doc),
					  &start, &end, TRUE);
}

static GnomeVFSResult
write_to_file (GnomeVFSHandle *handle, gchar *data, GnomeVFSFileSize len)
{
	GnomeVFSFileSize written;
	GnomeVFSResult res = GNOME_VFS_OK;

	while (len > 0)
	{
		res = gnome_vfs_write (handle, data, len, &written);

		len -= written;
		data += written;

		if (res != GNOME_VFS_OK)
			break;
	}

	return res;
}

/* No backups and it overwrites an existing file */
static gboolean
real_save_copy (GeditDocument *doc,
		const gchar *uri,
		const GeditEncoding *encoding,
		GError **error)
{
	gchar *contents;
	gboolean add_cr;
	gsize len = 0;
	gsize new_len = 0;
	GnomeVFSHandle *handle;
	mode_t saved_umask;
	guint perms = 0;
	GnomeVFSResult vfs_res;

	contents = get_buffer_contents (doc);
	if (contents == NULL)
		return FALSE;

	len = strlen (contents);

	add_cr = FALSE;
	
	if (len >= 1)
	{
		add_cr = (*(contents + len - 1) != '\n');
	}

	if (encoding != gedit_encoding_get_utf8 ())
	{
		GError *conv_error = NULL;
		gchar *converted_file_contents = NULL;

		converted_file_contents = gedit_convert_from_utf8 (contents, 
								   len, 
								   encoding,
								   &new_len,
								   &conv_error);

		if (conv_error != NULL)
		{
			/* Conversion error */
			g_propagate_error (error, conv_error);
			g_free (contents);
			return FALSE;
		}
		else
		{
			g_free (contents);
			contents = converted_file_contents;
		}
	}
	else
	{
		new_len = len;
	}

	/* init default permissions */
	saved_umask = umask(0);
	perms = (GNOME_VFS_PERM_USER_READ   |
		 GNOME_VFS_PERM_USER_WRITE  |
		 GNOME_VFS_PERM_GROUP_READ  |
		 GNOME_VFS_PERM_GROUP_WRITE |
		 GNOME_VFS_PERM_OTHER_READ  |
		 GNOME_VFS_PERM_OTHER_WRITE) &
		~saved_umask;
	umask (saved_umask);

	vfs_res = gnome_vfs_create (&handle,
				    uri,
				    GNOME_VFS_OPEN_WRITE,
				    FALSE,
				    perms);
	if (vfs_res != GNOME_VFS_OK)
	{
		g_free (contents);

		g_set_error (error,
			     GEDIT_DOCUMENT_IO_ERROR, 
			     vfs_res,
			     gnome_vfs_result_to_string (vfs_res));

		return FALSE;
	}

	vfs_res = write_to_file (handle, contents, new_len);
	g_free (contents);
	if (vfs_res != GNOME_VFS_OK)
	{
		g_set_error (error,
			     GEDIT_DOCUMENT_IO_ERROR,
			     vfs_res,
			     gnome_vfs_result_to_string (vfs_res));

		gnome_vfs_close (handle);

		return FALSE;
	}

	/* make sure the file is \n terminated.
	 * However if writing it fails we do not abort
	 */
	if (add_cr)
	{
		GnomeVFSFileSize written;

		if (encoding != gedit_encoding_get_utf8 ())
		{
			gchar *converted_n = NULL;
		
			converted_n = gedit_convert_from_utf8 ("\n", 
							       -1, 
							       encoding,
							       &new_len,
							       NULL);

			if (converted_n == NULL)
			{
				g_warning ("Cannot add '\\n' at the end of the file.");
			}
			else
			{
				vfs_res = gnome_vfs_write (handle, converted_n, new_len, &written);

				if ((vfs_res != GNOME_VFS_OK) || (written != new_len))
					g_warning ("Cannot add '\\n' at the end of the file.");

				g_free (converted_n);
			}
		}
		else
		{
			vfs_res = gnome_vfs_write (handle, "\n", 1, &written);

			if ((vfs_res != GNOME_VFS_OK) || (written != 1))
				g_warning ("Cannot add '\\n' at the end of the file.");
		}
	}

	gnome_vfs_close (handle);

	return TRUE;
}

#define MAX_URI_IN_DIALOG_LENGTH 50

static void
run_copy_error_dialog (GtkWindow *parent,
		       const gchar *dest_uri,
		       const gchar *details)
{
	GtkWidget *dialog;
	gchar *formatted_dest_uri;
	gchar *dest_uri_for_display;

	formatted_dest_uri = gnome_vfs_format_uri_for_display (dest_uri);

	/* Truncate the URI so it doesn't get insanely wide. Note that even
	 * though the dialog uses wrapped text, if the URI doesn't contain
	 * white space then the text-wrapping code is too stupid to wrap it.
	 */
	dest_uri_for_display = gedit_utils_str_middle_truncate (formatted_dest_uri, 
							   MAX_URI_IN_DIALOG_LENGTH);
	g_free (formatted_dest_uri);

	dialog = gtk_message_dialog_new (parent,
					 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
					 GTK_MESSAGE_ERROR,
					 GTK_BUTTONS_OK,
					 _("Could not save a copy of the file to \"%s\""),
					 dest_uri_for_display);

	if (details != NULL && strcmp (details, " ") != 0)
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
							  details);

	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
	gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);

	g_free (dest_uri_for_display);
}

#undef MAX_URI_IN_DIALOG_LENGTH

static gboolean
replace_existing_file (GtkWindow *parent, const gchar *uri)
{
	GtkWidget *dialog;
	gint ret;
	gchar *full_formatted_uri;
       	gchar *uri_for_display	;

	full_formatted_uri = gnome_vfs_format_uri_for_display (uri);
	g_return_val_if_fail (full_formatted_uri != NULL, FALSE);

	/* Truncate the URI so it doesn't get insanely wide. Note that even
	 * though the dialog uses wrapped text, if the URI doesn't contain
	 * white space then the text-wrapping code is too stupid to wrap it.
	 */
        uri_for_display = gedit_utils_str_middle_truncate (full_formatted_uri, 50);
	g_return_val_if_fail (uri_for_display != NULL, FALSE);
	g_free (full_formatted_uri);

	dialog = gtk_message_dialog_new (parent,
					 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
					 GTK_MESSAGE_QUESTION,
					 GTK_BUTTONS_NONE,
					 _("A file named \"%s\" already exists.\n"),
					 uri_for_display);

	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
						 _("Do you want to replace it with the "
						   "one you are saving?"));

	g_free (uri_for_display);

	gtk_dialog_add_button (GTK_DIALOG (dialog), 
			       GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);

	gedit_dialog_add_button (GTK_DIALOG (dialog), 
			_("_Replace"), GTK_STOCK_REFRESH, GTK_RESPONSE_YES);

	gtk_dialog_set_default_response	(GTK_DIALOG (dialog), GTK_RESPONSE_CANCEL);

	gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

	ret = gtk_dialog_run (GTK_DIALOG (dialog));

	gtk_widget_destroy (dialog);

	return (ret == GTK_RESPONSE_YES);
}

static gpointer
analyze_response (GtkFileChooser *chooser, gint response, const gchar *orig_uri)
{
	gchar *uri;

	if (response == GTK_RESPONSE_CANCEL ||
	    response == GTK_RESPONSE_DELETE_EVENT) 
	{
		gtk_widget_hide (GTK_WIDGET (chooser));

		return NULL;
	}

	uri = gtk_file_chooser_get_uri (chooser);

	if ((uri == NULL) || (strlen (uri) == 0)) 
	{
		g_free (uri);

		return NULL;
	}

	if (gedit_utils_uri_exists (uri))
	{
		gchar *canonical_uri = gnome_vfs_make_uri_canonical (uri);
		g_return_val_if_fail (canonical_uri != NULL, NULL);

		/* we don't allow the copy to overwrite the original */
		if (orig_uri != NULL && gnome_vfs_uris_match (orig_uri, canonical_uri))
		{
			run_copy_error_dialog (GTK_WINDOW (chooser),
					       uri,
					      _("You are trying to overwrite the original file"));

			g_free (uri);

			return NULL;
		}
		else if (!replace_existing_file (GTK_WINDOW (chooser), uri)) 
		{
			g_free (uri);

			return NULL;
		}
	}

	gtk_widget_hide (GTK_WIDGET (chooser));

	return uri;
}

static gboolean
all_text_files_filter (const GtkFileFilterInfo *filter_info,
		       gpointer                 data)
{
	if (filter_info->mime_type == NULL)
		return TRUE;
	
	if ((strncmp (filter_info->mime_type, "text/", 5) == 0) ||
            (strcmp (filter_info->mime_type, "application/x-desktop") == 0) ||
	    (strcmp (filter_info->mime_type, "application/x-perl") == 0) ||
            (strcmp (filter_info->mime_type, "application/x-python") == 0) ||
	    (strcmp (filter_info->mime_type, "application/x-php") == 0))
	{
	    return TRUE;
	}

	return FALSE;
}

static gboolean
uri_is_in_dir (const gchar *uri, const gchar *dir_uri)
{
	gboolean ret = FALSE;
	gchar *file_dir;

	file_dir = gedit_utils_uri_get_dirname (uri);

	if (gnome_vfs_uris_match (file_dir, dir_uri))
		ret = TRUE;

	g_free (file_dir);

	return ret;
}

static gboolean
get_filename_and_extension (const gchar *name,
			    gchar **name_without_ext,
			    gchar **ext)
{
	gchar *p;
	gchar *basename;
	gchar *extension;
	gboolean ret;

	p = strrchr (name, '.');

	if (p && *(p + 1) != '\0')
	{
		basename = g_malloc (p - name + 1);
		strncpy (basename, name, p - name);
		basename[p - name] = '\0';

		extension = g_strdup (p + 1);

		ret = TRUE;
	}
	else
	{
		basename = g_strdup (name);

		extension = NULL;

		ret = FALSE;
	}

	*name_without_ext = basename;
	*ext = extension;

	return ret;
}

static gchar *
run_copy_file_chooser (GtkWindow *parent,
		       GeditDocument *orig,
		       const GeditEncoding **encoding)
{
	gchar *orig_uri;
	GtkWidget *chooser;
	GtkFileFilter *filter;
	GtkWidget *hbox;
	GtkWidget *label;
	GtkWidget *menu;
	gchar *name = NULL;
	gchar *cur_dir_uri;
	gint res;
	gpointer retval;

	orig_uri = gedit_document_get_raw_uri (orig);

	chooser = gtk_file_chooser_dialog_new (_("Save Copy..."),
					       parent,
					       GTK_FILE_CHOOSER_ACTION_SAVE,
					       GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					       GTK_STOCK_SAVE, GTK_RESPONSE_OK,
					       NULL);

	gtk_window_set_modal (GTK_WINDOW (chooser), TRUE);
	gtk_dialog_set_default_response (GTK_DIALOG (chooser), GTK_RESPONSE_OK);

	gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (chooser), FALSE);

	/* we specify the suggested filename in the entry, but we don't set
	 * the whole uri: we don't want the user to try to overwrite and saving
	 * a copy in the same dir is not very useful.
	 * If we rally are in the same dir we suffix the suggested name with "(Copy)".
	 */
	cur_dir_uri = gtk_file_chooser_get_current_folder_uri (GTK_FILE_CHOOSER (chooser));
	if (orig_uri != NULL && uri_is_in_dir (orig_uri, cur_dir_uri))
	{
		gchar *tmp = gedit_document_get_short_name (orig);

		if (tmp != NULL)
		{
			gchar *basename = NULL;
			gchar *ext = NULL;

			if (get_filename_and_extension (tmp, &basename, &ext))
			{
				gchar *tmp2;

				/* translators: %s is a filename */
				tmp2 = g_strdup_printf (_("%s (copy)"), basename);

				/* we use another tmp so that the translatable
				 * format is the same as below */
				name = g_strconcat (tmp2, ".", ext, NULL);
				g_free (tmp2);
			}
			else
			{
				/* translators: %s is a filename */
				name = g_strdup_printf (_("%s (copy)"), basename);
			}

			g_free (basename);
			g_free (ext);
			g_free (tmp);
		}
	}
	else
	{
		name = gedit_document_get_short_name (orig);
	}

	gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (chooser), name);
	g_free (cur_dir_uri);
	g_free (name);

	/* Filters */
	filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, _("All Files"));
	gtk_file_filter_add_pattern (filter, "*");
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser), filter);

	/* Make this filter the default */
	gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (chooser), filter);

	filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, _("All Text Files"));
	gtk_file_filter_add_custom (filter, 
				    GTK_FILE_FILTER_MIME_TYPE,
				    all_text_files_filter, 
				    NULL, 
				    NULL);
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser), filter);

	/* encoding option menu */
	hbox = gtk_hbox_new (FALSE, 6);

	label = gtk_label_new_with_mnemonic (_("_Character Coding:"));
	menu = gedit_encodings_option_menu_new (TRUE);

	gtk_label_set_mnemonic_widget (GTK_LABEL (label), menu);				       		
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_end (GTK_BOX (hbox), menu, TRUE, TRUE, 0);
	gtk_file_chooser_set_extra_widget (GTK_FILE_CHOOSER (chooser), hbox);
	gtk_widget_show_all (hbox);

	if (*encoding != NULL)
		gedit_encodings_option_menu_set_selected_encoding (
				GEDIT_ENCODINGS_OPTION_MENU (menu),
				*encoding);

	do 
	{
		res = gtk_dialog_run (GTK_DIALOG (chooser));

		retval = analyze_response (GTK_FILE_CHOOSER (chooser), res, orig_uri);
	}
	while (GTK_WIDGET_VISIBLE (chooser));

	if ((retval != NULL) && (encoding != NULL))
		*encoding = gedit_encodings_option_menu_get_selected_encoding (GEDIT_ENCODINGS_OPTION_MENU (menu));

	gtk_widget_destroy (chooser);
	g_free (orig_uri);

	return retval;
}

static void
save_copy_cb (BonoboUIComponent *uic,
	      gpointer user_data,
	      const gchar* verbname)
{
	GeditDocument *doc;
	gchar *file_uri;
	const GeditEncoding *encoding;

	gedit_debug (DEBUG_PLUGINS, "");

	doc = gedit_get_active_document ();
	g_return_if_fail (doc != NULL);

	encoding = gedit_document_get_encoding (doc);

	file_uri = run_copy_file_chooser (
			GTK_WINDOW (bonobo_mdi_get_active_window (BONOBO_MDI (gedit_mdi))),
			doc,
			&encoding);

	if (file_uri != NULL) 
	{
		gchar *uri;
		GError *error = NULL;
		gboolean ret;

		uri = gnome_vfs_make_uri_canonical (file_uri);
		g_return_if_fail (uri != NULL);

		ret = real_save_copy (doc, uri, encoding, &error);
		if (!ret)
		{
			g_return_if_fail (error != NULL);

			run_copy_error_dialog (
				GTK_WINDOW (bonobo_mdi_get_active_window (BONOBO_MDI (gedit_mdi))),
				uri, error->message);

			g_error_free (error);
		}

		g_free (uri);
		g_free (file_uri);
	}
}

G_MODULE_EXPORT GeditPluginState
update_ui (GeditPlugin *plugin, BonoboWindow *window)
{
	BonoboUIComponent *uic;
	GeditMDI *mdi;
	GeditDocument *doc;

	gedit_debug (DEBUG_PLUGINS, "");
	g_return_val_if_fail (window != NULL, PLUGIN_ERROR);

	mdi = gedit_get_mdi ();
	g_return_val_if_fail (window != NULL, PLUGIN_ERROR);

	uic = gedit_get_ui_component_from_window (window);
	doc = gedit_get_active_document ();

	if (doc == NULL ||
	    gedit_document_is_readonly (doc) ||
	    gedit_mdi_get_state (mdi) != GEDIT_STATE_NORMAL)
	{
		gedit_menus_set_verb_sensitive (uic, "/commands/" MENU_ITEM_NAME, FALSE);
	}
	else
	{
		gedit_menus_set_verb_sensitive (uic, "/commands/" MENU_ITEM_NAME, TRUE);
	}

	return PLUGIN_OK;
}

G_MODULE_EXPORT GeditPluginState
activate (GeditPlugin *plugin)
{
	GList *top_windows;
	gedit_debug (DEBUG_PLUGINS, "");

	top_windows = gedit_get_top_windows ();
	g_return_val_if_fail (top_windows != NULL, PLUGIN_ERROR);

        while (top_windows)
        {
		gedit_menus_add_menu_item (BONOBO_WINDOW (top_windows->data),
				     MENU_ITEM_PATH, MENU_ITEM_NAME,
				     MENU_ITEM_LABEL, MENU_ITEM_TIP, NULL,
				     save_copy_cb);

                plugin->update_ui (plugin, BONOBO_WINDOW (top_windows->data));

                top_windows = g_list_next (top_windows);
        }

	return PLUGIN_OK;
}

G_MODULE_EXPORT GeditPluginState
deactivate (GeditPlugin *plugin)
{
	gedit_menus_remove_menu_item_all (MENU_ITEM_PATH, MENU_ITEM_NAME);

	return PLUGIN_OK;
}

G_MODULE_EXPORT GeditPluginState
init (GeditPlugin *plugin)
{
	gedit_debug (DEBUG_PLUGINS, "");

	return PLUGIN_OK;
}

G_MODULE_EXPORT GeditPluginState
destroy (GeditPlugin *plugin)
{
	gedit_debug (DEBUG_PLUGINS, "");

	return PLUGIN_OK;
}

