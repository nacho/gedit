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
#include <gedit/gedit-file-selector-utils.h>
#include <gedit/gedit-menus.h>


#define MENU_ITEM_LABEL	N_("Save _Copy") //fixme accel clash
#define MENU_ITEM_NAME	"SaveCopy"
#define MENU_ITEM_PATH	"/menu/File/EditOps_1/"
#define MENU_ITEM_TIP	N_("  blah ")


G_MODULE_EXPORT GeditPluginState update_ui (GeditPlugin *plugin, BonoboWindow *window);
G_MODULE_EXPORT GeditPluginState init (GeditPlugin *pd);
G_MODULE_EXPORT GeditPluginState destroy (GeditPlugin *pd);
G_MODULE_EXPORT GeditPluginState activate (GeditPlugin *pd);
G_MODULE_EXPORT GeditPluginState deactivate (GeditPlugin *pd);

static gchar *
get_contents (GeditDocument *doc, GeditEncoding *encoding, GError **error)
{
	gchar *chars;
	GtkTextIter start, end;

	// TODO make sure \n at the end

	gtk_text_buffer_get_bounds (GTK_TEXT_BUFFER (doc), &start, &end);
	chars = gtk_text_buffer_get_slice (GTK_TEXT_BUFFER (doc),
					   &start, &end, TRUE);

	if (encoding != gedit_encoding_get_utf8 ())
	{
		GError *conv_error = NULL;
		gchar* converted_file_contents = NULL;

		converted_file_contents = gedit_convert_from_utf8 (chars, 
								   -1, 
								   encoding,
								   &conv_error);

		if (conv_error != NULL)
		{
			/* Conversion error */
			g_propagate_error (error, conv_error);
			g_free (chars);
			return NULL;
		}
		else
		{
			g_free (chars);
			chars = converted_file_contents;
		}
	}

	return chars;
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
	GnomeVFSHandle *handle;
	mode_t saved_umask;
	guint perms = 0;
	GnomeVFSResult vfs_res;

	contents = get_contents (doc, encoding, error);
	if (contents == NULL)
		return FALSE;

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
		g_set_error (error,
			     GEDIT_DOCUMENT_IO_ERROR, 
			     vfs_res,
			     gnome_vfs_result_to_string (vfs_res));

		return FALSE;
	}

	vfs_res = write_to_file (handle, contents, strlen (contents));
	if (vfs_res != GNOME_VFS_OK)
	{
		g_set_error (error,
			     GEDIT_DOCUMENT_IO_ERROR,
			     vfs_res,
			     gnome_vfs_result_to_string (vfs_res));

		gnome_vfs_close (handle);
	}

	gnome_vfs_close (handle);
}

static void
save_copy_cb (BonoboUIComponent *uic,
	      gpointer user_data,
	      const gchar* verbname)
{
	GeditDocument *doc;
	gchar *file_uri;
	gboolean ret = FALSE;
	gchar *untitled_name = NULL;
	const GeditEncoding *encoding;

	gedit_debug (DEBUG_PLUGINS, "");

	doc = gedit_get_active_document ();
	g_return_if_fail (doc != NULL);

	encoding = gedit_document_get_encoding (doc);

	if (gedit_document_is_untitled (doc))
	{
		untitled_name = gedit_document_get_uri (doc);
		if (untitled_name == NULL)
			untitled_name = g_strdup ("Untitled");
	}
	else
	{
		untitled_name = gedit_document_get_short_name (doc);
	}
	g_return_if_fail (untitled_name != NULL);

	file_uri = gedit_file_selector_save (
			GTK_WINDOW (bonobo_mdi_get_active_window (BONOBO_MDI (gedit_mdi))),
			TRUE,
		        _("Save a Copy..."), 
			NULL,
			NULL,
			untitled_name,
			&encoding);

	g_free (untitled_name);

	if (file_uri != NULL) 
	{
		gchar *uri;
		GError *error = NULL;

		uri = gnome_vfs_make_uri_canonical (file_uri);
		g_return_val_if_fail (uri != NULL, FALSE);

		ret = real_save_copy (doc, uri, encoding, &error);
		if (!ret)
		{
			g_return_if_fail (error != NULL);

			gedit_error_reporting_saving_file (uri,
							   error,
							   GTK_WINDOW (gedit_get_active_window ()));

			g_error_free (error);
		}

		g_free (uri);
		g_free (file_uri);
	}

	return ret;
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

                pd->update_ui (plugin, BONOBO_WINDOW (top_windows->data));

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

