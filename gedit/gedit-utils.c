/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-utils.c
 * This file is part of gedit
 *
 * Copyright (C) 1998, 1999 Alex Roberts, Evan Lawrence
 * Copyright (C) 2000, 2002 Chema Celorio, Paolo Maggi 
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
 * Modified by the gedit Team, 1998-2002. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libgnomeui/libgnomeui.h>
#include <glib/gunicode.h>
#include <libgnome/gnome-i18n.h>
#include <libgnomevfs/gnome-vfs.h>
#include <eel/eel-gtk-extensions.h>
#include <eel/eel-vfs-extensions.h>
#include <eel/eel-string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>

#include <string.h>

#include "gedit-utils.h"
#include "gedit2.h"
#include "bonobo-mdi.h"
#include "gedit-document.h"
#include "gedit-prefs-manager.h"
#include "gedit-debug.h"
#include "gedit-convert.h"

#define STDIN_DELAY_MICROSECONDS 100000

/* =================================================== */
/* Flash */

struct _MessageInfo {
  BonoboWindow * win;
  guint timeoutid;
  guint handlerid;
};

typedef struct _MessageInfo MessageInfo;

MessageInfo *current_mi = NULL;

static gint remove_message_timeout (MessageInfo * mi);
static void remove_timeout_cb (GtkWidget *win, MessageInfo *mi);
static void bonobo_window_flash (BonoboWindow * win, const gchar * flash);

static gint
remove_message_timeout (MessageInfo * mi) 
{
	BonoboUIComponent *ui_component;

	GDK_THREADS_ENTER ();	
	
  	ui_component = bonobo_mdi_get_ui_component_from_window (mi->win);
	g_return_val_if_fail (ui_component != NULL, FALSE);

	bonobo_ui_component_set_status (ui_component, " ", NULL);

	g_signal_handler_disconnect (G_OBJECT (mi->win), mi->handlerid);
	
	g_free (mi);
	current_mi = NULL;

  	GDK_THREADS_LEAVE ();

  	return FALSE; /* removes the timeout */
}

/* Called if the win is destroyed before the timeout occurs. */
static void
remove_timeout_cb (GtkWidget *win, MessageInfo *mi) 
{
 	g_source_remove (mi->timeoutid);
  	g_free (mi);

	if (mi == current_mi)
	       	current_mi = NULL;
}

static const guint32 flash_length = 3000; /* 3 seconds, I hope */

/**
 * bonobo_win_flash
 * @app: Pointer a Bonobo window object
 * @flash: Text of message to be flashed
 *
 * Description:
 * Flash the message in the statusbar for a few moments; if no
 * statusbar, do nothing. For trivial little status messages,
 * e.g. "Auto saving..."
 **/

static void 
bonobo_window_flash (BonoboWindow * win, const gchar * flash)
{
	BonoboUIComponent *ui_component;
  	g_return_if_fail (win != NULL);
  	g_return_if_fail (BONOBO_IS_WINDOW (win));
  	g_return_if_fail (flash != NULL);
  	
	ui_component = bonobo_mdi_get_ui_component_from_window (win);
	g_return_if_fail (ui_component != NULL);
	
	if (current_mi != NULL)
	{
		g_source_remove (current_mi->timeoutid);
		remove_message_timeout (current_mi);
	}
	
	if (bonobo_ui_component_path_exists (ui_component, "/status", NULL))
	{
    		MessageInfo * mi;

		bonobo_ui_component_set_status (ui_component, flash, NULL);
    		
		mi = g_new(MessageInfo, 1);

    		mi->timeoutid = 
      			g_timeout_add (flash_length,
				(GSourceFunc) remove_message_timeout,
				mi);
    
    		mi->handlerid = 
      			g_signal_connect (GTK_OBJECT (win),
				"destroy",
			   	G_CALLBACK (remove_timeout_cb),
			   	mi);

    		mi->win       = win;

		current_mi = mi;
  	}   
}

/* ========================================================== */

/**
 * gedit_utils_flash:
 * @msg: Message to flash on the statusbar
 *
 * Flash a temporary message on the statusbar of gedit.
 **/
void
gedit_utils_flash (const gchar *msg)
{
	g_return_if_fail (msg != NULL);
	
	bonobo_window_flash (bonobo_mdi_get_active_window (BONOBO_MDI (gedit_mdi)), msg);
}

/**
 * gedit_utils_flash_va:
 * @format:
 **/
void
gedit_utils_flash_va (gchar *format, ...)
{
	va_list args;
	gchar *msg;

	g_return_if_fail (format != NULL);

	va_start (args, format);
	msg = g_strdup_vprintf (format, args);
	va_end (args);

	gedit_utils_flash (msg);
	g_free (msg);
}

void
gedit_utils_set_status (const gchar *msg)
{
	BonoboWindow *win;
	BonoboUIComponent *ui_component;

	win = gedit_get_active_window ();
  	g_return_if_fail (BONOBO_IS_WINDOW (win));
  	
	ui_component = bonobo_mdi_get_ui_component_from_window (win);
	g_return_if_fail (ui_component != NULL);
	
	if (current_mi != NULL)
	{
		g_source_remove (current_mi->timeoutid);
		remove_message_timeout (current_mi);
	}
	
	if (bonobo_ui_component_path_exists (ui_component, "/status", NULL))
	{
		bonobo_ui_component_set_status (ui_component, (msg != NULL) ? msg : " ", NULL);
    		
		current_mi =  NULL;
  	}   
}

void
gedit_utils_set_status_va (gchar *format, ...)
{
	va_list args;
	gchar *msg;

	g_return_if_fail (format != NULL);

	va_start (args, format);
	msg = g_strdup_vprintf (format, args);
	va_end (args);

	gedit_utils_set_status (msg);
	g_free (msg);
}

gboolean
gedit_utils_uri_has_file_scheme (const gchar *uri)
{
	gchar *canonical_uri = NULL;
	gboolean res;

	canonical_uri = gnome_vfs_make_uri_canonical (uri);
	g_return_val_if_fail (canonical_uri != NULL, FALSE);

	res = eel_istr_has_prefix (canonical_uri, "file:");

	g_free (canonical_uri);

	return res;
}

gboolean
gedit_utils_is_uri_read_only (const gchar* uri)
{
	gchar* file_uri = NULL;
	gchar* canonical_uri = NULL;

	gint res;

	g_return_val_if_fail (uri != NULL, TRUE);
	
	gedit_debug (DEBUG_FILE, "URI: %s", uri);

	/* FIXME: all remote files are marked as readonly */
	if (!gedit_utils_uri_has_file_scheme (uri))
		return TRUE;
		
	canonical_uri = gnome_vfs_make_uri_canonical (uri);
	g_return_val_if_fail (canonical_uri != NULL, TRUE);

	gedit_debug (DEBUG_FILE, "CANONICAL URI: %s", canonical_uri);

	file_uri = gnome_vfs_get_local_path_from_uri (canonical_uri);
	if (file_uri == NULL)
	{
		gedit_debug (DEBUG_FILE, "FILE URI: NULL");

		return TRUE;
	}
	
	res = access (file_uri, W_OK);

	g_free (canonical_uri);
	g_free (file_uri);

	return res;	
}

/* lifted from eel */
static GtkWidget *
gedit_gtk_button_new_with_stock_icon (const gchar *label, const gchar *stock_id)
{
	GtkWidget *button, *l, *image, *hbox, *align;

	/* This is mainly copied from gtk_button_construct_child(). */
	button = gtk_button_new ();
	l = gtk_label_new_with_mnemonic (label);
	gtk_label_set_mnemonic_widget (GTK_LABEL (l), GTK_WIDGET (button));
	image = gtk_image_new_from_stock (stock_id, GTK_ICON_SIZE_BUTTON);
	hbox = gtk_hbox_new (FALSE, 2);
	align = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
	gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
	gtk_box_pack_end (GTK_BOX (hbox), l, FALSE, FALSE, 0);
	gtk_container_add (GTK_CONTAINER (button), align);
	gtk_container_add (GTK_CONTAINER (align), hbox);
	gtk_widget_show_all (align);

	return button;
}

GtkWidget*
gedit_dialog_add_button (GtkDialog *dialog, const gchar* text, const gchar* stock_id,
			 gint response_id)
{
	GtkWidget *button;

	g_return_val_if_fail (GTK_IS_DIALOG (dialog), NULL);
	g_return_val_if_fail (text != NULL, NULL);
	g_return_val_if_fail (stock_id != NULL, NULL);

	button = gedit_gtk_button_new_with_stock_icon (text, stock_id);
	g_return_val_if_fail (button != NULL, NULL);

	GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);

	gtk_widget_show (button);

	gtk_dialog_add_action_widget (dialog, button, response_id);	

	return button;
}

/*
 * n: len of the string in bytes
 */
gboolean 
g_utf8_caselessnmatch (const char *s1, const char *s2, gssize n1, gssize n2)
{
	gchar *casefold;
	gchar *normalized_s1;
      	gchar *normalized_s2;
	gint len_s1;
	gint len_s2;
	gboolean ret = FALSE;

	g_return_val_if_fail (s1 != NULL, FALSE);
	g_return_val_if_fail (s2 != NULL, FALSE);
	g_return_val_if_fail (n1 > 0, FALSE);
	g_return_val_if_fail (n2 > 0, FALSE);

	casefold = g_utf8_casefold (s1, n1);
	normalized_s1 = g_utf8_normalize (casefold, -1, G_NORMALIZE_ALL);
	g_free (casefold);

	casefold = g_utf8_casefold (s2, n2);
	normalized_s2 = g_utf8_normalize (casefold, -1, G_NORMALIZE_ALL);
	g_free (casefold);

	len_s1 = strlen (normalized_s1);
	len_s2 = strlen (normalized_s2);

	if (len_s1 < len_s2)
		goto finally_2;

	ret = (strncmp (normalized_s1, normalized_s2, len_s2) == 0);
	
finally_2:
	g_free (normalized_s1);
	g_free (normalized_s2);	

	return ret;
}


/*************************************************************
 * ERROR REPORTING CODE
 ************************************************************/

#define MAX_URI_IN_DIALOG_LENGTH 50

void
gedit_utils_error_reporting_loading_file (
		const gchar *uri,
		const GeditEncoding *encoding,
		GError *error,
		GtkWindow *parent)
{
	gchar *scheme_string;
	gchar *error_message = NULL;
	gchar *full_formatted_uri;
       	gchar *uri_for_display	;
	gchar *encoding_name = NULL;
	GtkWidget *dialog;

	g_return_if_fail (uri != NULL);
	
	full_formatted_uri = eel_format_uri_for_display (uri);

	/* Truncate the URI so it doesn't get insanely wide. Note that even
	 * though the dialog uses wrapped text, if the URI doesn't contain
	 * white space then the text-wrapping code is too stupid to wrap it.
	 */
        uri_for_display = eel_str_middle_truncate (full_formatted_uri, 
			MAX_URI_IN_DIALOG_LENGTH);
	g_free (full_formatted_uri);

	if (encoding != NULL)
		encoding_name = gedit_encoding_to_string (encoding);
	else
		encoding_name = g_strdup ("UTF-8");
	
	if ((error != NULL) && (error->domain == GEDIT_DOCUMENT_IO_ERROR))
	{
		switch (error->code)
		{
			case GNOME_VFS_ERROR_NOT_FOUND:
				error_message = g_strdup_printf (
       	                 		_("Could not find the file \"%s\".\n\n"
					  "Please, check that you typed the location correctly "
					  "and try again."),
       		                 	uri_for_display);
				break;
	
			case GNOME_VFS_ERROR_CORRUPTED_DATA:
				error_message = g_strdup_printf (
       	                 		_("Could not open the file \"%s\" because "
				   	   "it contains corrupted data."),
				 	uri_for_display);
				break;			 
	
			case GNOME_VFS_ERROR_NOT_SUPPORTED:
				scheme_string = eel_uri_get_scheme (uri);
                
				if ((scheme_string != NULL) && g_utf8_validate (scheme_string, -1, NULL))
				{
					error_message = g_strdup_printf (
						_("Could not open the file \"%s\" because "
						  "gedit cannot handle %s: locations."),
       	                        	 	uri_for_display, scheme_string);
				}
				else
					error_message = g_strdup_printf (
						_("Could not open the file \"%s\""),
       	                         		uri_for_display);
			
				if (scheme_string != NULL)
					g_free (scheme_string);
	
       		 	        break;
					
			case GNOME_VFS_ERROR_WRONG_FORMAT:
				error_message = g_strdup_printf (
       		                 	_("Could not open the file \"%s\" because "
				   	  "it contains data in an invalid format."),
				 	uri_for_display);
				break;	

			case GNOME_VFS_ERROR_TOO_BIG:
				error_message = g_strdup_printf (
                        		_("Could not open the file \"%s\" because "
			   		  "it is too big."),
			 		uri_for_display);
				break;	

			case GNOME_VFS_ERROR_INVALID_URI:
				error_message = g_strdup_printf (
                	        	_("\"%s\" is not a valid location.\n\n"
					  "Please, check that you typed the location "
					  "correctly and try again."),
                         		uri_for_display);
	                	break;
	
			case GNOME_VFS_ERROR_ACCESS_DENIED:
				error_message = g_strdup_printf (
					_("Could not open the file \"%s\" because "
					  "access was denied."),
	                         	uri_for_display);
        	        	break;

			case GNOME_VFS_ERROR_TOO_MANY_OPEN_FILES:
				error_message = g_strdup_printf (
					_("Could not open the file \"%s\" because "
					  "there are too many open files.\n\n"
					  "Please, close some open file and try again."),
                	         	uri_for_display);
                		break;

			case GNOME_VFS_ERROR_IS_DIRECTORY:
				error_message = g_strdup_printf (
					_("\"%s\" is a directory.\n\n"
					  "Please, check that you typed the location "
					  "correctly and try again."),
                         		uri_for_display);
	                	break;

			case GNOME_VFS_ERROR_NO_MEMORY:
				error_message = g_strdup_printf (
					_("Not enough available memory to open the file \"%s\". "
					  "Please, close some running application and try again."),
	                         	uri_for_display);
        	        	break;

			case GNOME_VFS_ERROR_HOST_NOT_FOUND:
				/* This case can be hit for user-typed strings like "foo" due to
			 	* the code that guesses web addresses when there's no initial "/".
			 	* But this case is also hit for legitimate web addresses when
			 	* the proxy is set up wrong.
		 		*/
				{
					GnomeVFSURI *vfs_uri;

					vfs_uri = gnome_vfs_uri_new (uri);

					if (vfs_uri == NULL)
					{
						/* use the same string as INVALID_HOST */
						error_message = g_strdup_printf (
						_("Could not open the file \"%s\" because the host name "
						  "was invalid.\n\n"
						  "Please, check that you typed the location correctly "
						  "and try again."),
						  uri_for_display);
					}
					else
					{
						const gchar *hn = gnome_vfs_uri_get_host_name (vfs_uri);

						if (hn == NULL)
						{
							/* use the same string as INVALID_HOST */
							error_message = g_strdup_printf (
							_("Could not open the file \"%s\" because the host name "
							  "was invalid.\n\n"
							  "Please, check that you typed the location correctly "
							  "and try again."),
							  uri_for_display);
						}
						else
						{
							gchar *host_name = eel_make_valid_utf8 (hn);

							error_message = g_strdup_printf (
							_("Could not open the file \"%s\" because no host \"%s\" " 
							  "could be found.\n\n"
							  "Please, check that you typed the location correctly "
							  "and that your proxy settings are correct and then "
							  "try again."),
							  uri_for_display, host_name);

							g_free (host_name);
						}
						gnome_vfs_uri_unref (vfs_uri);		
					}
				}
				break;

			case GNOME_VFS_ERROR_INVALID_HOST_NAME:
				error_message = g_strdup_printf (
                        		_("Could not open the file \"%s\" because the host name "
					  "was invalid.\n\n"
				   	  "Please, check that you typed the location correctly "
					  "and try again."),
	                         	uri_for_display);
                		break;

			case GNOME_VFS_ERROR_HOST_HAS_NO_ADDRESS:
				error_message = g_strdup_printf (
					_("Could not open the file \"%s\" because the host name "
					  "was empty.\n\n"
					  "Please, check that your proxy settings are correct "
					  "and try again."),
					uri_for_display);
				break;
		
			case GNOME_VFS_ERROR_LOGIN_FAILED:
				error_message = g_strdup_printf (
					_("Could not open the file \"%s\" because the attempt to "
					  "log in failed.\n\n"
					  "Please, check that you typed the location correctly "
					  "and try again."),
					uri_for_display);		
				break;

			case GEDIT_ERROR_INVALID_UTF8_DATA:
				error_message = g_strdup_printf (
                	        	_("Could not open the file \"%s\" because "
				   	  "it contains invalid data.\n\n"
				  	  "Probably, you are trying to open a binary file."),
				 	uri_for_display);

				break;

			case GNOME_VFS_ERROR_GENERIC:
				error_message = g_strdup_printf (
                        			_("Could not open the file \"%s\"."),
					 	uri_for_display);
				break;
			/*
			case GNOME_VFS_ERROR_GENERIC:
			case GNOME_VFS_ERROR_INTERNAL:
			case GNOME_VFS_ERROR_BAD_PARAMETERS:
			case GNOME_VFS_ERROR_IO:
			case GNOME_VFS_ERROR_BAD_FILE:
			case GNOME_VFS_ERROR_NO_SPACE:
			case GNOME_VFS_ERROR_READ_ONLY:
			case GNOME_VFS_ERROR_NOT_OPEN:
			case GNOME_VFS_ERROR_INVALID_OPEN_MODE:
			case GNOME_VFS_ERROR_EOF:
			case GNOME_VFS_ERROR_NOT_A_DIRECTORY:
			case GNOME_VFS_ERROR_IN_PROGRESS:
			case GNOME_VFS_ERROR_INTERRUPTED:
			case GNOME_VFS_ERROR_FILE_EXISTS:
			case GNOME_VFS_ERROR_LOOP:
			case GNOME_VFS_ERROR_NOT_PERMITTED:
			case GNOME_VFS_ERROR_CANCELLED:
			case GNOME_VFS_ERROR_DIRECTORY_BUSY:
			case GNOME_VFS_ERROR_DIRECTORY_NOT_EMPTY:
			case GNOME_VFS_ERROR_TOO_MANY_LINKS:
			case GNOME_VFS_ERROR_READ_ONLY_FILE_SYSTEM:
			case GNOME_VFS_ERROR_NOT_SAME_FILE_SYSTEM:
			case GNOME_VFS_ERROR_NAME_TOO_LONG:
			case GNOME_VFS_ERROR_SERVICE_NOT_AVAILABLE:
			case GNOME_VFS_ERROR_SERVICE_OBSOLETE,
			case GNOME_VFS_ERROR_PROTOCOL_ERROR,
			case GNOME_VFS_NUM_ERRORS:
			*/
		}
	}
	else if (error->domain == GEDIT_CONVERT_ERROR)
	{
		switch (error->code)
		{
			case GEDIT_CONVERT_ERROR_AUTO_DETECTION_FAILED:
				error_message = g_strdup_printf (
                	        	_("Could not open the file \"%s\" because "
				   	  "gedit has not been able to automatically detect "
					  "the character coding.\n\n"
					  "Please, check that you are not trying to open a binary file "
					  "and try again selecting a character coding in the 'Open File...' "
					  "(or 'Open Location') dialog."),
				 	uri_for_display);

				break;

			case GEDIT_CONVERT_ERROR_ILLEGAL_SEQUENCE:
				error_message = g_strdup_printf (
                	        	_("Could not open the file \"%s\" using "
					  "the %s character coding.\n\n"
					  "Please, check that you are not trying to open a binary file "
					  "and that you selected the right "
					  "character coding in the 'Open File...' (or 'Open Location') "
					  "dialog and try again."),
					uri_for_display, encoding_name);

				break;
			
			case GEDIT_CONVERT_ERROR_BINARY_FILE:
				error_message = g_strdup_printf (
                	        	_("Could not open the file \"%s\" because "
				   	  "it contains invalid data.\n\n"
				  	  "Probably, you are trying to open a binary file."),
				 	uri_for_display);

				break;
	  
		}
	}
	else if (error->domain == G_CONVERT_ERROR)
	{
		switch (error->code)
		{
			case G_CONVERT_ERROR_NO_CONVERSION:
			case G_CONVERT_ERROR_ILLEGAL_SEQUENCE:
			case G_CONVERT_ERROR_FAILED:
			case G_CONVERT_ERROR_PARTIAL_INPUT:
				error_message = g_strdup_printf (
					_("Could not open the file \"%s\" using "
					  "the %s character coding.\n\n"
					  "Please, check that you are not trying to open a binary file "
					  "and that you selected the right "
					  "character coding in the 'Open File...' (or 'Open Location') "
					  "dialog and try again."),
					uri_for_display, encoding_name);
				break;

		}
	}
	
	if (error_message == NULL)
	{
		if ((error == NULL) || (error->message == NULL))
			error_message = g_strdup_printf (
                        			_("Could not open the file \"%s\"."),
					 	uri_for_display);
		else
			error_message = g_strdup_printf (
                        			_("Could not open the file \"%s\".\n\n%s."),
					 	uri_for_display, error->message);

	}

	g_free (encoding_name);

	dialog = gtk_message_dialog_new (
			parent,
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		   	GTK_MESSAGE_ERROR,
		   	GTK_BUTTONS_OK,
			error_message);
			
	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

	gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);

	g_free (uri_for_display);
	g_free (error_message);
}

void
gedit_utils_error_reporting_saving_file (
		const gchar *uri,
		GError *error,
		GtkWindow *parent)
{
	gchar *error_message;
	gchar *full_formatted_uri;
       	gchar *uri_for_display	;
	
	GtkWidget *dialog;

	g_return_if_fail (uri != NULL);
	g_return_if_fail (error != NULL);
	
	full_formatted_uri = eel_format_uri_for_display (uri);

	/* Truncate the URI so it doesn't get insanely wide. Note that even
	 * though the dialog uses wrapped text, if the URI doesn't contain
	 * white space then the text-wrapping code is too stupid to wrap it.
	 */
        uri_for_display = eel_str_middle_truncate (full_formatted_uri, 
			MAX_URI_IN_DIALOG_LENGTH);
	g_free (full_formatted_uri);

	if (strcmp (error->message, " ") == 0)
		error_message = g_strdup_printf (
					_("Could not save the file \"%s\"."),
				  	uri_for_display);
	else
		error_message = g_strdup_printf (
					_("Could not save the file \"%s\".\n\n%s"),
				 	uri_for_display, error->message);
		
	dialog = gtk_message_dialog_new (
			parent,
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		   	GTK_MESSAGE_ERROR,
		   	GTK_BUTTONS_OK,
			error_message);
			
	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

	gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);

	g_free (uri_for_display);
	g_free (error_message);
}

void
gedit_utils_error_reporting_reverting_file (
		const gchar *uri,
		const GError *error,
		GtkWindow *parent)
{
	gchar *scheme_string;
	gchar *error_message;
	gchar *full_formatted_uri;
       	gchar *uri_for_display	;
	GtkWidget *dialog;

	g_return_if_fail (uri != NULL);
	g_return_if_fail (error != NULL);
	
	full_formatted_uri = gnome_vfs_format_uri_for_display (uri);

	/* Truncate the URI so it doesn't get insanely wide. Note that even
	 * though the dialog uses wrapped text, if the URI doesn't contain
	 * white space then the text-wrapping code is too stupid to wrap it.
	 */
        uri_for_display = eel_str_middle_truncate (full_formatted_uri, 
			MAX_URI_IN_DIALOG_LENGTH);
	g_free (full_formatted_uri);

	switch (error->code)
	{
		case GNOME_VFS_ERROR_NOT_FOUND:
			error_message = g_strdup_printf (
                        	_("Could not revert the file \"%s\" because gedit cannot find it.\n\n"
			   	  "Perhaps, it has recently been deleted."),
                         	uri_for_display);
			break;

		case GNOME_VFS_ERROR_CORRUPTED_DATA:
			error_message = g_strdup_printf (
                        	_("Could not revert the file \"%s\" because "
			   	   "it contains corrupted data."),
			 	uri_for_display);
			break;			 

		case GNOME_VFS_ERROR_NOT_SUPPORTED:
			scheme_string = eel_uri_get_scheme (uri);
                
			if ((scheme_string != NULL) && g_utf8_validate (scheme_string, -1, NULL))
			{
				error_message = g_strdup_printf (
					_("Could not revert the file \"%s\" because "
					  "gedit cannot handle %s: locations."),
                                	uri_for_display, scheme_string);
			}
			else
				error_message = g_strdup_printf (
					_("Could not revert the file \"%s\"."),
                                	uri_for_display);
	
			if (scheme_string != NULL)
				g_free (scheme_string);

        	        break;
				
		case GNOME_VFS_ERROR_WRONG_FORMAT:
			error_message = g_strdup_printf (
                        	_("Could not revert the file \"%s\" because "
			   	  "it contains data in an invalid format."),
			 	uri_for_display);
			break;	

		case GNOME_VFS_ERROR_TOO_BIG:
			error_message = g_strdup_printf (
                        	_("Could not revert the file \"%s\" because "
			   	   "it is too big."),
			 	uri_for_display);
			break;	
			
		case GNOME_VFS_ERROR_ACCESS_DENIED:
			error_message = g_strdup_printf (
				_("Could not revert the file \"%s\" because "
				  "access was denied."),
                         	uri_for_display);
                	break;

		case GNOME_VFS_ERROR_TOO_MANY_OPEN_FILES:
			error_message = g_strdup_printf (
				_("Could not revert the file \"%s\" because "
				  "there are too many open files.\n\n"
				  "Please, close some open file and try again."),
                         	uri_for_display);
                	break;
	
		case GNOME_VFS_ERROR_NO_MEMORY:
			error_message = g_strdup_printf (
				_("Not enough available memory to revert the file \"%s\". "
				  "Please, close some running application and try again."),
                         	uri_for_display);
                	break;

		case GNOME_VFS_ERROR_HOST_NOT_FOUND:
			/* This case can be hit for user-typed strings like "foo" due to
		 	* the code that guesses web addresses when there's no initial "/".
		 	* But this case is also hit for legitimate web addresses when
		 	* the proxy is set up wrong.
		 	*/
			{
				GnomeVFSURI *vfs_uri;

				vfs_uri = gnome_vfs_uri_new (uri);

				if (vfs_uri == NULL)
				{
					/* use the default string */
					error_message = g_strdup_printf (
		                        	_("Could not revert the file \"%s\"."),
					 	uri_for_display);
				}
				else
				{
					const gchar *hn = gnome_vfs_uri_get_host_name (vfs_uri);

					if (hn == NULL)
					{
						/* use the default string */
						error_message = g_strdup_printf (
			                        	_("Could not revert the file \"%s\"."),
						 	uri_for_display);
					}
					else
					{
						gchar *host_name = eel_make_valid_utf8 (hn);

						error_message = g_strdup_printf (
						_("Could not revert the file \"%s\" because no host \"%s\" " 
					  	  "could be found.\n\n"
        	        		  	  "Please, check that your proxy settings are correct and "
					  	  "try again."),
						  uri_for_display, host_name);

						g_free (host_name);
					}
					gnome_vfs_uri_unref (vfs_uri);		
				}
			}
			break;
            
		case GNOME_VFS_ERROR_HOST_HAS_NO_ADDRESS:
			error_message = g_strdup_printf (
				_("Could not revert the file \"%s\" because the host name was empty.\n\n"
				  "Please, check that your proxy settings are correct and try again."),
				uri_for_display);
			break;
		
		case GNOME_VFS_ERROR_LOGIN_FAILED:
			error_message = g_strdup_printf (
				_("Could not revert the file \"%s\" because the attempt to "
				  "log in failed."),
				uri_for_display);		
			break;

		case GEDIT_ERROR_INVALID_UTF8_DATA:
			error_message = g_strdup_printf (
                        	_("Could not revert the file \"%s\" because "
			   	  "it contains invalid UTF-8 data.\n\n"
				  "Probably, you are trying to revert a binary file."),
			 	uri_for_display);

			break;
			
		case GEDIT_ERROR_UNTITLED:
			error_message = g_strdup_printf (
				_("It is not possible to revert an Untitled document."));
			break;

		/*
		case GNOME_VFS_ERROR_INVALID_URI:
		case GNOME_VFS_ERROR_INVALID_HOST_NAME:
		case GNOME_VFS_ERROR_GENERIC:
		case GNOME_VFS_ERROR_INTERNAL:
		case GNOME_VFS_ERROR_BAD_PARAMETERS:
		case GNOME_VFS_ERROR_IO:
		case GNOME_VFS_ERROR_BAD_FILE:
		case GNOME_VFS_ERROR_NO_SPACE:
		case GNOME_VFS_ERROR_READ_ONLY:
		case GNOME_VFS_ERROR_NOT_OPEN:
		case GNOME_VFS_ERROR_INVALID_OPEN_MODE:
		case GNOME_VFS_ERROR_EOF:
		case GNOME_VFS_ERROR_NOT_A_DIRECTORY:
		case GNOME_VFS_ERROR_IN_PROGRESS:
		case GNOME_VFS_ERROR_INTERRUPTED:
		case GNOME_VFS_ERROR_FILE_EXISTS:
		case GNOME_VFS_ERROR_LOOP:
		case GNOME_VFS_ERROR_NOT_PERMITTED:
		case GNOME_VFS_ERROR_CANCELLED:
		case GNOME_VFS_ERROR_DIRECTORY_BUSY:
		case GNOME_VFS_ERROR_DIRECTORY_NOT_EMPTY:
		case GNOME_VFS_ERROR_TOO_MANY_LINKS:
		case GNOME_VFS_ERROR_READ_ONLY_FILE_SYSTEM:
		case GNOME_VFS_ERROR_NOT_SAME_FILE_SYSTEM:
		case GNOME_VFS_ERROR_NAME_TOO_LONG:
		case GNOME_VFS_ERROR_SERVICE_NOT_AVAILABLE:
		case GNOME_VFS_ERROR_SERVICE_OBSOLETE,
		case GNOME_VFS_ERROR_PROTOCOL_ERROR,
		case GNOME_VFS_NUM_ERRORS:
		case GNOME_VFS_ERROR_IS_DIRECTORY:

		*/
		default:
			error_message = g_strdup_printf (
                        	_("Could not revert the file \"%s\"."),
			 	uri_for_display);

			break;
	}
	
	dialog = gtk_message_dialog_new (
			parent,
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		   	GTK_MESSAGE_ERROR,
		   	GTK_BUTTONS_OK,
			error_message);
			
	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

	gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);

	g_free (uri_for_display);
	g_free (error_message);
}

void
gedit_utils_error_reporting_creating_file (
		const gchar *uri,
		gint error_code,
		GtkWindow *parent)
{
	gchar *error_message;
	gchar *full_formatted_uri;
       	gchar *uri_for_display	;
	
	GtkWidget *dialog;

	g_return_if_fail (uri != NULL);
	
	full_formatted_uri = eel_format_uri_for_display (uri);

	/* Truncate the URI so it doesn't get insanely wide. Note that even
	 * though the dialog uses wrapped text, if the URI doesn't contain
	 * white space then the text-wrapping code is too stupid to wrap it.
	 */
        uri_for_display = eel_str_middle_truncate (full_formatted_uri, 
			MAX_URI_IN_DIALOG_LENGTH);
	g_free (full_formatted_uri);

	switch (error_code)
	{
		case EEXIST: 
			error_message = g_strdup_printf (_("The file \"%s\" already exists."),
				uri_for_display);
			break;

		case EISDIR:
			error_message = g_strdup_printf (
				_("\"%s\" is a directory.\n\n"
				  "Please, check that you typed the location correctly."),
                         	uri_for_display);
                	break;

		case EACCES:
		case EROFS:
		case ETXTBSY:
			error_message = g_strdup_printf (
				_("Cannot create the file \"%s\".\n\n"
				  "Make sure you have the appropriate write permissions."),
				uri_for_display);
			break;

		case ENAMETOOLONG:
			error_message = g_strdup_printf (
				_("Cannot create the file \"%s\".\n\n"
				  "The file name is too long."),
				uri_for_display);
			break;
		
		case ENOENT:
			error_message = g_strdup_printf (
				_("Cannot create the file \"%s\".\n\n"
				  "A directory component in the file name does not exist or "
		                  "is a dangling symbolic link."),
				uri_for_display);	
			break;


		case ENOSPC:
			error_message = g_strdup_printf (
				 _("There is not enough disk space to create the file \"%s\".\n\n"
				   "Please free some disk space and try again."),
				 uri_for_display);
			break;

		default:
			error_message = g_strdup_printf (_("Could not create the file \"%s\"."),
							 uri_for_display);
	}
	
	dialog = gtk_message_dialog_new (
			parent,
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		   	GTK_MESSAGE_ERROR,
		   	GTK_BUTTONS_OK,
			error_message);
			
	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

	gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);

	g_free (uri_for_display);
	g_free (error_message);
}


/*
 * gedit_utils_set_atk_name_description
 * @widget : The Gtk widget for which name/description to be set
 * @name : Atk name string
 * @description : Atk description string
 * Description : This function sets up name and description
 * for a specified gtk widget.
 */

void
gedit_utils_set_atk_name_description (	GtkWidget *widget, 
					const gchar *name,
					const gchar *description)
{
	AtkObject *aobj;

	aobj = gtk_widget_get_accessible (widget);

	if (!(GTK_IS_ACCESSIBLE (aobj)))
		return;

	if(name)
		atk_object_set_name (aobj, name);

	if(description)
		atk_object_set_description (aobj, description);
}
/*
 * gedit_set_atk__relation
 * @obj1,@obj2 : specified widgets.
 * @rel_type : the type of relation to set up.
 * Description : This function establishes atk relation
 * between 2 specified widgets.
 */
void
gedit_utils_set_atk_relation (	GtkWidget *obj1, 
				GtkWidget *obj2, 
				AtkRelationType rel_type )
{
	AtkObject *atk_obj1, *atk_obj2;
	AtkRelationSet *relation_set;
	AtkObject *targets[1];
	AtkRelation *relation;

	atk_obj1 = gtk_widget_get_accessible (obj1);
	atk_obj2 = gtk_widget_get_accessible (obj2);

	if (!(GTK_IS_ACCESSIBLE (atk_obj1)) || !(GTK_IS_ACCESSIBLE (atk_obj2)))
		return;

	relation_set = atk_object_ref_relation_set (atk_obj1);
	targets[0] = atk_obj2;

	relation = atk_relation_new (targets, 1, rel_type);
	atk_relation_set_add (relation_set, relation);

	g_object_unref (G_OBJECT (relation));
}

gboolean
gedit_utils_uri_exists (const gchar* text_uri)
{
	GnomeVFSURI *uri;
	gboolean res;
		
	g_return_val_if_fail (text_uri != NULL, FALSE);
	
	gedit_debug (DEBUG_FILE, "text_uri: %s", text_uri);

	uri = gnome_vfs_uri_new (text_uri);
	g_return_val_if_fail (uri != NULL, FALSE);

	res = gnome_vfs_uri_exists (uri);

	gnome_vfs_uri_unref (uri);

	gedit_debug (DEBUG_FILE, res ? "TRUE" : "FALSE");

	return res;
}

gchar *
gedit_utils_convert_search_text (const gchar *text)
{
	GString *str;
	gint length;
	gboolean drop_prev = FALSE;
	const gchar *cur;
	const gchar *end;
	const gchar *prev;

	g_return_val_if_fail (text != NULL, NULL);

	length = strlen (text);

	str = g_string_new ("");

	cur = text;
	end = text + length;
	prev = NULL;
	
	while (cur != end) 
	{
		const gchar *next;
		next = g_utf8_next_char (cur);

		if (prev && (*prev == '\\')) 
		{
			switch (*cur) 
			{
				case 'n':
					str = g_string_append (str, "\n");
				break;
				case 'r':
					str = g_string_append (str, "\r");
				break;
				case 't':
					str = g_string_append (str, "\t");
				break;
				case '\\':
					str = g_string_append (str, "\\");
					drop_prev = TRUE;
				break;
				default:
					str = g_string_append (str, "\\");
					str = g_string_append_len (str, cur, next - cur);
				break;
			}
		} 
		else if (*cur != '\\') 
		{
			str = g_string_append_len (str, cur, next - cur);
		} 
		else if ((next == end) && (*cur == '\\')) 
		{
			str = g_string_append (str, "\\");
		}
		
		if (!drop_prev)
		{
			prev = cur;
		}
		else 
		{
			prev = NULL;
			drop_prev = FALSE;
		}

		cur = next;
	}

	return g_string_free (str, FALSE);
}

#define GEDIT_STDIN_BUFSIZE 1024

gchar *
gedit_utils_get_stdin (void)
{
	GString * file_contents;
	gchar *tmp_buf = NULL;
	guint buffer_length;
	GnomeVFSResult	res;
	fd_set rfds;
	struct timeval tv;
	
	FD_ZERO (&rfds);
	FD_SET (0, &rfds);

	/* wait for 1/4 of a second */
	tv.tv_sec = 0;
	tv.tv_usec = STDIN_DELAY_MICROSECONDS;

	if (select (1, &rfds, NULL, NULL, &tv) != 1)
		return NULL;

	tmp_buf = g_new0 (gchar, GEDIT_STDIN_BUFSIZE + 1);
	g_return_val_if_fail (tmp_buf != NULL, FALSE);

	file_contents = g_string_new (NULL);
	
	while (feof (stdin) == 0)
	{
		buffer_length = fread (tmp_buf, 1, GEDIT_STDIN_BUFSIZE, stdin);
		tmp_buf [buffer_length] = '\0';
		g_string_append (file_contents, tmp_buf);

		if (ferror (stdin) != 0)
		{
			res = gnome_vfs_result_from_errno (); 
		
			g_free (tmp_buf);
			g_string_free (file_contents, TRUE);
			return NULL;
		}
	}

	fclose (stdin);

	return g_string_free (file_contents, FALSE);
}

void 
gedit_warning (GtkWindow *parent, gchar *format, ...)
{
	va_list args;
	gchar *str;
	GtkWidget *dialog;

	g_return_if_fail (format != NULL);

	va_start (args, format);
	str = g_strdup_vprintf (format, args);
	va_end (args);

	dialog = gtk_message_dialog_new (
			parent,
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		   	GTK_MESSAGE_ERROR,
		   	GTK_BUTTONS_OK,
			str);

	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

	gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
	gtk_dialog_set_has_separator (GTK_DIALOG (dialog), FALSE);

	gtk_dialog_run (GTK_DIALOG (dialog));

	gtk_widget_destroy (dialog);

	g_free (str);
}

