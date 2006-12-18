/*
 * gedit-io-error-message-area.c
 * This file is part of gedit
 *
 * Copyright (C) 2005 - Paolo Maggi 
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
 
/*
 * Verbose error reporting for file I/O operations (load, save, revert, create)
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <errno.h>
#include <string.h>

#include <glib/gi18n.h>
#include <libgnomevfs/gnome-vfs.h>

#include "gedit-utils.h"
#include "gedit-convert.h"
#include "gedit-document.h"
#include "gedit-io-error-message-area.h"
#include "gedit-message-area.h"
#include "gedit-prefs-manager.h"
#include <gedit/gedit-encodings-option-menu.h>

#define MAX_URI_IN_DIALOG_LENGTH 50

static gboolean
is_recoverable_error (const GError *error)
{
	gboolean is_recoverable = FALSE;

	if (error->domain == GEDIT_DOCUMENT_ERROR)
	{
		switch (error->code) {
		case GNOME_VFS_ERROR_ACCESS_DENIED:
		case GNOME_VFS_ERROR_TOO_MANY_OPEN_FILES:
		case GNOME_VFS_ERROR_NO_MEMORY:
		case GNOME_VFS_ERROR_HOST_NOT_FOUND:
		case GNOME_VFS_ERROR_HOST_HAS_NO_ADDRESS:
		case GNOME_VFS_ERROR_LOGIN_FAILED:
		case GNOME_VFS_ERROR_TIMEOUT:

			is_recoverable = TRUE;
		}
	}

	return is_recoverable;
}

static GtkWidget *
create_io_loading_error_message_area (const gchar *primary_text,
				      const gchar *secondary_text)
{
	GtkWidget *message_area;
	GtkWidget *hbox_content;
	GtkWidget *image;
	GtkWidget *vbox;
	gchar *primary_markup;
	gchar *secondary_markup;
	GtkWidget *primary_label;
	GtkWidget *secondary_label;
	
	message_area = gedit_message_area_new_with_buttons (
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					NULL);

	hbox_content = gtk_hbox_new (FALSE, 8);
	gtk_widget_show (hbox_content);

	image = gtk_image_new_from_stock ("gtk-dialog-error", GTK_ICON_SIZE_DIALOG);
	gtk_widget_show (image);
	gtk_box_pack_start (GTK_BOX (hbox_content), image, FALSE, FALSE, 0);
	gtk_misc_set_alignment (GTK_MISC (image), 0.5, 0);
  
	vbox = gtk_vbox_new (FALSE, 6);
	gtk_widget_show (vbox);
	gtk_box_pack_start (GTK_BOX (hbox_content), vbox, TRUE, TRUE, 0);

	primary_markup = g_strdup_printf ("<b>%s</b>", primary_text);
	primary_label = gtk_label_new (primary_markup);
	g_free (primary_markup);
	gtk_widget_show (primary_label);
	gtk_box_pack_start (GTK_BOX (vbox), primary_label, TRUE, TRUE, 0);
	gtk_label_set_use_markup (GTK_LABEL (primary_label), TRUE);
	gtk_label_set_line_wrap (GTK_LABEL (primary_label), TRUE);
	gtk_misc_set_alignment (GTK_MISC (primary_label), 0, 0.5);
	GTK_WIDGET_SET_FLAGS (primary_label, GTK_CAN_FOCUS);
	gtk_label_set_selectable (GTK_LABEL (primary_label), TRUE);
  
  	if (secondary_text != NULL)
  	{
  		secondary_markup = g_strdup_printf ("<small>%s</small>",
  						    secondary_text);
		secondary_label = gtk_label_new (secondary_markup);
		g_free (secondary_markup);
		gtk_widget_show (secondary_label);
		gtk_box_pack_start (GTK_BOX (vbox), secondary_label, TRUE, TRUE, 0);
		GTK_WIDGET_SET_FLAGS (secondary_label, GTK_CAN_FOCUS);
		gtk_label_set_use_markup (GTK_LABEL (secondary_label), TRUE);
		gtk_label_set_line_wrap (GTK_LABEL (secondary_label), TRUE);
		gtk_label_set_selectable (GTK_LABEL (secondary_label), TRUE);
		gtk_misc_set_alignment (GTK_MISC (secondary_label), 0, 0.5);
	}

	gedit_message_area_set_contents (GEDIT_MESSAGE_AREA (message_area),
					 hbox_content);
	return message_area;
}

GtkWidget *
gedit_io_loading_error_message_area_new (const gchar  *uri,
					 const GError *error)
{
	gchar *error_message = NULL;
	gchar *message_details = NULL;
	gchar *full_formatted_uri;
	gchar *scheme_string;
	gchar *scheme_markup;
       	gchar *uri_for_display;
       	gchar *temp_uri_for_display;
	GtkWidget *message_area;

	g_return_val_if_fail (uri != NULL, NULL);
	g_return_val_if_fail (error != NULL, NULL);
	g_return_val_if_fail (error->domain == GEDIT_DOCUMENT_ERROR, NULL);

	full_formatted_uri = gedit_utils_format_uri_for_display (uri);

	/* Truncate the URI so it doesn't get insanely wide. Note that even
	 * though the dialog uses wrapped text, if the URI doesn't contain
	 * white space then the text-wrapping code is too stupid to wrap it.
	 */
	temp_uri_for_display = gedit_utils_str_middle_truncate (full_formatted_uri, 
								MAX_URI_IN_DIALOG_LENGTH);								
	g_free (full_formatted_uri);

	uri_for_display = g_markup_printf_escaped ("<i>%s</i>", temp_uri_for_display);
	g_free (temp_uri_for_display);

	switch (error->code)
	{
	case GNOME_VFS_ERROR_NOT_FOUND:
		error_message = g_strdup_printf (_("Could not find the file %s."),
						 uri_for_display);
		message_details = g_strdup (_("Please check that you typed the "
				      	      "location correctly and try again."));
	break;

	case GNOME_VFS_ERROR_CORRUPTED_DATA:
		message_details = g_strdup (_("The file contains corrupted data."));
		break;

	case GNOME_VFS_ERROR_NOT_SUPPORTED:
		scheme_string = gnome_vfs_get_uri_scheme (uri);

		if ((scheme_string != NULL) && g_utf8_validate (scheme_string, -1, NULL))
		{
  			scheme_markup = g_markup_printf_escaped ("<i>%s:</i>", scheme_string);

			/* Translators: %s is a URI scheme (like for example http:, ftp:, etc.) */
			message_details = g_strdup_printf (_("gedit cannot handle %s locations."),
							   scheme_markup);
			g_free (scheme_markup);
		}
		else
		{
			message_details = g_strdup (_("gedit cannot handle this location."));
		}

		g_free (scheme_string);
		break;

	case GNOME_VFS_ERROR_WRONG_FORMAT:
		message_details = g_strdup (_("The file contains data in an invalid format."));
		break;

	case GNOME_VFS_ERROR_TOO_BIG:
		message_details = g_strdup (_("The file is too big."));
		break;

	case GNOME_VFS_ERROR_INVALID_URI:
		error_message = g_strdup_printf (_("%s is not a valid location."),
						 uri_for_display);
		message_details = g_strdup (_("Please check that you typed the "
					      "location correctly and try again."));
		break;

	case GNOME_VFS_ERROR_ACCESS_DENIED:
		message_details = g_strdup (_("You do not have the permissions necessary to open the file."));
		break;

	case GNOME_VFS_ERROR_TOO_MANY_OPEN_FILES:
		message_details = g_strdup (_("There are too many open files. Please "
					      "close some applications and try again."));
		break;

	case GNOME_VFS_ERROR_IS_DIRECTORY:
		error_message = g_strdup_printf (_("%s is a directory."),
						 uri_for_display);
		message_details = g_strdup (_("Please check that you typed the "
					      "location correctly and try again."));
		break;

	case GNOME_VFS_ERROR_NO_MEMORY:
		message_details = g_strdup (_("Not enough available memory to open "
					      "the file. Please close some running "
					      "applications and try again."));
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

			if (vfs_uri != NULL)
			{
				const gchar *hn = gnome_vfs_uri_get_host_name (vfs_uri);

				if (hn != NULL)
				{
					gchar *host_name = gedit_utils_make_valid_utf8 (hn);
					gchar *host_markup = g_markup_printf_escaped ("<i>%s</i>", host_name);
					g_free (host_name);

					/* Translators: %s is a host name */
					message_details = g_strdup_printf (
						_("Host %s could not be found. "
	        		  	  	  "Please check that your proxy settings "
				  	  	  "are correct and try again."),
					  	host_markup);

					g_free (host_markup);
				}
				else
				{
					/* use the same string as INVALID_HOST */
					message_details = g_strdup_printf (
						_("Host name was invalid. "
						  "Please check that you typed the location "
						  "correctly and try again."));
				}

				gnome_vfs_uri_unref (vfs_uri);		
			}
			else
			{
				/* use the same string as INVALID_HOST */
				message_details = g_strdup_printf (
					_("Host name was invalid. "
					  "Please check that you typed the location "
					  "correctly and try again."));
			}
		}
		break;

	case GNOME_VFS_ERROR_INVALID_HOST_NAME:
		message_details = g_strdup_printf (_("Host name was invalid. "
						     "Please check that you typed the location "
						     "correctly and try again."));
        		break;

	case GNOME_VFS_ERROR_HOST_HAS_NO_ADDRESS:
		message_details = g_strdup (_("Host name was empty. "
					      "Please check that your proxy settings "
					      "are correct and try again."));
		break;

	case GNOME_VFS_ERROR_LOGIN_FAILED:
		message_details = g_strdup (_("Attempt to log in failed. "
					      "Please check that you typed the location "
					      "correctly and try again."));
		break;

	case GEDIT_DOCUMENT_ERROR_NOT_REGULAR_FILE:
		message_details = g_strdup (_("The file you are trying to open is not a regular file."));
		break;

	case GNOME_VFS_ERROR_TIMEOUT:
		message_details = g_strdup (_("Connection timed out. Please try again."));
		break;

	case GNOME_VFS_ERROR_GENERIC:
		break;

	/*
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

	default:
		/* We should invent decent error messages for every case we actually experience. */
		g_warning ("Hit unhandled case %d (%s) in %s.", 
			   error->code, gnome_vfs_result_to_string (error->code), G_STRFUNC);	
		message_details = g_strdup_printf (_("Unexpected error: %s"), 
						   gnome_vfs_result_to_string (error->code));								 
		break;
	}

	if (error_message == NULL)
		error_message = g_strdup_printf (_("Could not open the file %s."),
						 uri_for_display);

	message_area = create_io_loading_error_message_area (error_message,
							     message_details);

	if (is_recoverable_error (error))
	{
		gedit_message_area_add_stock_button_with_text (GEDIT_MESSAGE_AREA (message_area),
							       _("_Retry"),
							       GTK_STOCK_REFRESH,
							       GTK_RESPONSE_OK);
	}

	g_free (uri_for_display);
	g_free (error_message);
	g_free (message_details);

	return message_area;
}

GtkWidget *
gedit_unrecoverable_reverting_error_message_area_new (const gchar  *uri,
						      const GError *error)
{
	gchar *error_message = NULL;
	gchar *message_details = NULL;
	gchar *full_formatted_uri;
	gchar *scheme_string;
	gchar *scheme_markup;
       	gchar *uri_for_display;
       	gchar *temp_uri_for_display;
	GtkWidget *message_area;

	g_return_val_if_fail (uri != NULL, NULL);
	g_return_val_if_fail (error != NULL, NULL);
	g_return_val_if_fail (error->domain == GEDIT_DOCUMENT_ERROR, NULL);

	full_formatted_uri = gedit_utils_format_uri_for_display (uri);

	/* Truncate the URI so it doesn't get insanely wide. Note that even
	 * though the dialog uses wrapped text, if the URI doesn't contain
	 * white space then the text-wrapping code is too stupid to wrap it.
	 */
	temp_uri_for_display = gedit_utils_str_middle_truncate (full_formatted_uri, 
								MAX_URI_IN_DIALOG_LENGTH);								
	g_free (full_formatted_uri);

	uri_for_display = g_markup_printf_escaped ("<i>%s</i>", temp_uri_for_display);
	g_free (temp_uri_for_display);

	switch (error->code)
	{
	case GNOME_VFS_ERROR_NOT_FOUND:
		message_details = g_strdup (_("gedit cannot find it. "
					      "Perhaps it has recently been deleted."));
		break;

	case GNOME_VFS_ERROR_CORRUPTED_DATA:
		message_details = g_strdup (_("The file contains corrupted data."));
		break;

	case GNOME_VFS_ERROR_NOT_SUPPORTED:
		scheme_string = gnome_vfs_get_uri_scheme (uri);

		if ((scheme_string != NULL) && g_utf8_validate (scheme_string, -1, NULL))
		{
  			scheme_markup = g_markup_printf_escaped ("<i>%s:</i>", scheme_string);

			/* Translators: %s is a URI scheme (like for example http:, ftp:, etc.) */
			message_details = g_strdup_printf (_("gedit cannot handle %s locations."),
							   scheme_markup);
			g_free (scheme_markup);
		}
		else
		{
			message_details = g_strdup (_("gedit cannot handle this location."));
		}	

		g_free (scheme_string);
       	        break;

	case GNOME_VFS_ERROR_WRONG_FORMAT:
		message_details = g_strdup (_("The file contains data in an invalid format."));
		break;

	case GNOME_VFS_ERROR_TOO_BIG:
		message_details = g_strdup (_("The file is too big."));
		break;

	case GNOME_VFS_ERROR_ACCESS_DENIED:
		message_details = g_strdup (_("You do not have the permissions necessary to open the file."));
		break;

	case GNOME_VFS_ERROR_TOO_MANY_OPEN_FILES:
		message_details = g_strdup (_("There are too many open files. Please "
					      "close some applications and try again."));
		break;

	case GNOME_VFS_ERROR_IS_DIRECTORY:
		message_details = g_strdup_printf (_("%s is a directory."),
						   uri_for_display);
		break;

	case GNOME_VFS_ERROR_NO_MEMORY:
		message_details = g_strdup (_("Not enough available memory to open "
					      "the file. Please close some running "
					      "applications and try again."));
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

			if (vfs_uri != NULL)
			{
				const gchar *hn = gnome_vfs_uri_get_host_name (vfs_uri);

				if (hn != NULL)
				{
					gchar *host_name = gedit_utils_make_valid_utf8 (hn);
					gchar *host_markup = g_markup_printf_escaped ("<i>%s</i>", host_name);
					g_free (host_name);

					/* Translators: %s is a host name */
					message_details = g_strdup_printf (
						_("Host %s could not be found. "
	        		  	  	"Please check that your proxy settings "
				  	  	"are correct and try again."),
					  	host_markup);

					g_free (host_markup);
				}
				else
				{
					/* use the same string as INVALID_HOST */
					message_details = g_strdup_printf (
						_("Host name was invalid. "
						  "Please check that you typed the location "
						  "correctly and try again."));
				}

				gnome_vfs_uri_unref (vfs_uri);		
			}
			else
			{
				/* use the same string as INVALID_HOST */
				message_details = g_strdup_printf (
					_("Host name was invalid. "
					  "Please check that you typed the location "
					  "correctly and try again."));
			}
		}
		break;

	case GNOME_VFS_ERROR_HOST_HAS_NO_ADDRESS:
		message_details = g_strdup (_("Host name was empty. "
					      "Please check that your proxy settings "
					      "are correct and try again."));
		break;

	case GNOME_VFS_ERROR_LOGIN_FAILED:
		message_details = g_strdup (_("Attempt to log in failed."));
		break;

	case GEDIT_DOCUMENT_ERROR_NOT_REGULAR_FILE:
		message_details = g_strdup_printf (_("%s is not a regular file."),
						   uri_for_display);
		break;

	case GNOME_VFS_ERROR_TIMEOUT:
		message_details = g_strdup (_("Connection timed out. Please try again."));
		break;

	/* this should never happen, revert should be insensitive */
//CHECK: we used to have this before new_mdi... is it really useful (see comment above)
/*
	case GEDIT_ERROR_UNTITLED:
		message_details = g_strdup (_("It is not possible to revert an Untitled document."));
		break;
*/

	case GNOME_VFS_ERROR_GENERIC:
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
		g_warning ("Hit unhandled case %d (%s) in %s.", 
			   error->code, gnome_vfs_result_to_string (error->code), G_STRFUNC);	
		message_details = g_strdup_printf (_("Unexpected error: %s"), 
						   gnome_vfs_result_to_string (error->code));								 
		break;
	}

	if (error_message == NULL)
		error_message = g_strdup_printf (_("Could not revert the file %s."),
						 uri_for_display);

	message_area = create_io_loading_error_message_area (error_message,
							     message_details);

	g_free (uri_for_display);
	g_free (error_message);
	g_free (message_details);

	return message_area;
}

static void
create_option_menu (GtkWidget *message_area, GtkWidget *vbox)
{
	GtkWidget *hbox;
	GtkWidget *label;
	GtkWidget *menu;
	gchar *label_markup;
	
	hbox = gtk_hbox_new (FALSE, 6);

	label_markup = g_strdup_printf ("<small>%s</small>",
					_("Ch_aracter Coding:"));
	label = gtk_label_new_with_mnemonic (label_markup);
	g_free (label_markup);
	gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
	menu = gedit_encodings_option_menu_new (TRUE);
	g_object_set_data (G_OBJECT (message_area), 
			   "gedit-message-area-encoding-menu", 
			   menu);
	
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), menu);				       		
	gtk_box_pack_start (GTK_BOX (hbox), 
			    label,
			    FALSE,
			    FALSE,
			    0);

	gtk_box_pack_end (GTK_BOX (hbox), 
			  menu,
			  TRUE,
			  TRUE,
			  0);

	gtk_widget_show_all (hbox);

	gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
}

static GtkWidget *
create_conversion_error_message_area (const gchar *primary_text,
				      const gchar *secondary_text)
{
	GtkWidget *message_area;
	GtkWidget *hbox_content;
	GtkWidget *image;
	GtkWidget *vbox;
	gchar *primary_markup;
	gchar *secondary_markup;
	GtkWidget *primary_label;
	GtkWidget *secondary_label;
	
	message_area = gedit_message_area_new ();

	gedit_message_area_add_stock_button_with_text (GEDIT_MESSAGE_AREA (message_area),						        
						       _("_Retry"),
						       GTK_STOCK_REDO,
						       GTK_RESPONSE_OK);
	gedit_message_area_add_button (GEDIT_MESSAGE_AREA (message_area),
				       GTK_STOCK_CANCEL,
				       GTK_RESPONSE_CANCEL);
				      
	hbox_content = gtk_hbox_new (FALSE, 8);
	gtk_widget_show (hbox_content);				
	
	image = gtk_image_new_from_stock ("gtk-dialog-error", GTK_ICON_SIZE_DIALOG);
	gtk_widget_show (image);
	gtk_box_pack_start (GTK_BOX (hbox_content), image, FALSE, FALSE, 0);
	gtk_misc_set_alignment (GTK_MISC (image), 0.5, 0);
  
	vbox = gtk_vbox_new (FALSE, 6);
	gtk_widget_show (vbox);
	gtk_box_pack_start (GTK_BOX (hbox_content), vbox, TRUE, TRUE, 0);
	
	primary_markup = g_strdup_printf ("<b>%s</b>", primary_text);
	primary_label = gtk_label_new (primary_markup);
	g_free (primary_markup);
	gtk_widget_show (primary_label);
	gtk_box_pack_start (GTK_BOX (vbox), primary_label, TRUE, TRUE, 0);
	gtk_label_set_use_markup (GTK_LABEL (primary_label), TRUE);
	gtk_label_set_line_wrap (GTK_LABEL (primary_label), TRUE);
	gtk_misc_set_alignment (GTK_MISC (primary_label), 0, 0.5);
	GTK_WIDGET_SET_FLAGS (primary_label, GTK_CAN_FOCUS);
	gtk_label_set_selectable (GTK_LABEL (primary_label), TRUE);
  
  	if (secondary_text != NULL)
  	{
  		secondary_markup = g_strdup_printf ("<small>%s</small>",
  						    secondary_text);
		secondary_label = gtk_label_new (secondary_markup);
		g_free (secondary_markup);
		gtk_widget_show (secondary_label);
		gtk_box_pack_start (GTK_BOX (vbox), secondary_label, TRUE, TRUE, 0);
		GTK_WIDGET_SET_FLAGS (secondary_label, GTK_CAN_FOCUS);
		gtk_label_set_use_markup (GTK_LABEL (secondary_label), TRUE);
		gtk_label_set_line_wrap (GTK_LABEL (secondary_label), TRUE);
		gtk_label_set_selectable (GTK_LABEL (secondary_label), TRUE);
		gtk_misc_set_alignment (GTK_MISC (secondary_label), 0, 0.5);
	}

	create_option_menu (message_area, vbox);
	
	gedit_message_area_set_contents (GEDIT_MESSAGE_AREA (message_area),
					 hbox_content);
	return message_area;
}

GtkWidget *
gedit_conversion_error_while_loading_message_area_new (
						const gchar         *uri,
						const GeditEncoding *encoding,
				    		const GError        *error)
{
	gchar *error_message = NULL;
	gchar *message_details = NULL;
	gchar *full_formatted_uri;
	gchar *encoding_name;
       	gchar *uri_for_display;
       	gchar *temp_uri_for_display;
	GtkWidget *message_area;
	
	g_return_val_if_fail (uri != NULL, NULL);
	g_return_val_if_fail (error != NULL, NULL);
	g_return_val_if_fail ((error->domain == G_CONVERT_ERROR) ||
			      (error->domain == GEDIT_CONVERT_ERROR), NULL);
	
	full_formatted_uri = gedit_utils_format_uri_for_display (uri);

	/* Truncate the URI so it doesn't get insanely wide. Note that even
	 * though the dialog uses wrapped text, if the URI doesn't contain
	 * white space then the text-wrapping code is too stupid to wrap it.
	 */
	temp_uri_for_display = gedit_utils_str_middle_truncate (full_formatted_uri, 
								MAX_URI_IN_DIALOG_LENGTH);								
	g_free (full_formatted_uri);
	
	uri_for_display = g_markup_printf_escaped ("<i>%s</i>", temp_uri_for_display);
	g_free (temp_uri_for_display);

	if (encoding != NULL)
		encoding_name = gedit_encoding_to_string (encoding);
	else
		encoding_name = g_strdup ("UTF-8");

	if (error->domain == GEDIT_CONVERT_ERROR)
	{
		g_return_val_if_fail (error->code == GEDIT_CONVERT_ERROR_AUTO_DETECTION_FAILED, NULL);
		
		error_message = g_strdup_printf (_("Could not open the file %s."),
							 uri_for_display);
		message_details = g_strconcat (_("gedit has not been able to detect "
				               "the character coding."), "\n", 
				               _("Please check that you are not trying to open a binary file."), "\n",
					       _("Select a character coding from the menu and try again."), NULL);
	}
	else 
	{
		
		error_message = g_strdup_printf (_("Could not open the file %s using the %s character coding."),
						 uri_for_display, 
						 encoding_name);
		message_details = g_strconcat (_("Please check that you are not trying to open a binary file."), "\n",
					       _("Select a different character coding from the menu and try again."), NULL);
	}
	
	message_area = create_conversion_error_message_area (error_message, message_details);

	g_free (uri_for_display);
	g_free (encoding_name);
	g_free (error_message);
	g_free (message_details);
	
	return message_area;
}

GtkWidget *
gedit_conversion_error_while_saving_message_area_new (
						const gchar         *uri,
						const GeditEncoding *encoding,
				    		const GError        *error)
{
	gchar *error_message = NULL;
	gchar *message_details = NULL;
	gchar *full_formatted_uri;
	gchar *encoding_name;
       	gchar *uri_for_display;
       	gchar *temp_uri_for_display;
	GtkWidget *message_area;
	
	g_return_val_if_fail (uri != NULL, NULL);
	g_return_val_if_fail (error != NULL, NULL);
	g_return_val_if_fail (error->domain == G_CONVERT_ERROR, NULL);
	g_return_val_if_fail (encoding != NULL, NULL);
	
	full_formatted_uri = gedit_utils_format_uri_for_display (uri);

	/* Truncate the URI so it doesn't get insanely wide. Note that even
	 * though the dialog uses wrapped text, if the URI doesn't contain
	 * white space then the text-wrapping code is too stupid to wrap it.
	 */
	temp_uri_for_display = gedit_utils_str_middle_truncate (full_formatted_uri, 
								MAX_URI_IN_DIALOG_LENGTH);								
	g_free (full_formatted_uri);
	
	uri_for_display = g_markup_printf_escaped ("<i>%s</i>", temp_uri_for_display);
	g_free (temp_uri_for_display);

	encoding_name = gedit_encoding_to_string (encoding);
	
	error_message = g_strdup_printf (_("Could not save the file %s using the %s character coding."),
					 uri_for_display, 
					 encoding_name);
	message_details = g_strconcat (_("The document contains one or more characters that cannot be encoded "
					 "using the specified character coding."), "\n",
				       _("Select a different character coding from the menu and try again."), NULL);
	
	message_area = create_conversion_error_message_area (
								error_message,
								message_details);

	g_free (uri_for_display);
	g_free (encoding_name);
	g_free (error_message);
	g_free (message_details);
	
	return message_area;
}

const GeditEncoding *
gedit_conversion_error_message_area_get_encoding (GtkWidget *message_area)
{
	gpointer menu;
	
	g_return_val_if_fail (GEDIT_IS_MESSAGE_AREA (message_area), NULL);

	menu = g_object_get_data (G_OBJECT (message_area), 
				  "gedit-message-area-encoding-menu");	
	g_return_val_if_fail (menu, NULL);
	
	return gedit_encodings_option_menu_get_selected_encoding
					(GEDIT_ENCODINGS_OPTION_MENU (menu));
}

GtkWidget *
gedit_file_already_open_warning_message_area_new (const gchar *uri)
{
	GtkWidget *message_area;
	GtkWidget *hbox_content;
	GtkWidget *image;
	GtkWidget *vbox;
	gchar *primary_markup;
	gchar *secondary_markup;
	GtkWidget *primary_label;
	GtkWidget *secondary_label;
	gchar *primary_text;
	const gchar *secondary_text;
	gchar *full_formatted_uri;
       	gchar *uri_for_display;
       	gchar *temp_uri_for_display;
	
	full_formatted_uri = gedit_utils_format_uri_for_display (uri);

	/* Truncate the URI so it doesn't get insanely wide. Note that even
	 * though the dialog uses wrapped text, if the URI doesn't contain
	 * white space then the text-wrapping code is too stupid to wrap it.
	 */
	temp_uri_for_display = gedit_utils_str_middle_truncate (full_formatted_uri, 
								MAX_URI_IN_DIALOG_LENGTH);								
	g_free (full_formatted_uri);
	
	uri_for_display = g_markup_printf_escaped ("<i>%s</i>", temp_uri_for_display);
	g_free (temp_uri_for_display);
	
	message_area = gedit_message_area_new ();
	gedit_message_area_add_button (GEDIT_MESSAGE_AREA (message_area),
				       _("_Edit Anyway"),
				       GTK_RESPONSE_YES);
	gedit_message_area_add_button (GEDIT_MESSAGE_AREA (message_area),
				       _("_Don't Edit"),
				       GTK_RESPONSE_CANCEL);
				       
	hbox_content = gtk_hbox_new (FALSE, 8);
	gtk_widget_show (hbox_content);				
	
	image = gtk_image_new_from_stock ("gtk-dialog-warning", GTK_ICON_SIZE_DIALOG);
	gtk_widget_show (image);
	gtk_box_pack_start (GTK_BOX (hbox_content), image, FALSE, FALSE, 0);
	gtk_misc_set_alignment (GTK_MISC (image), 0.5, 0);
  
	vbox = gtk_vbox_new (FALSE, 6);
	gtk_widget_show (vbox);
	gtk_box_pack_start (GTK_BOX (hbox_content), vbox, TRUE, TRUE, 0);

	primary_text = g_strdup_printf (_("This file (%s) is already open in another gedit window."), uri_for_display);
	g_free (uri_for_display);
	
	primary_markup = g_strdup_printf ("<b>%s</b>", primary_text);
	g_free (primary_text);
	primary_label = gtk_label_new (primary_markup);
	g_free (primary_markup);
	gtk_widget_show (primary_label);
	gtk_box_pack_start (GTK_BOX (vbox), primary_label, TRUE, TRUE, 0);
	gtk_label_set_use_markup (GTK_LABEL (primary_label), TRUE);
	gtk_label_set_line_wrap (GTK_LABEL (primary_label), TRUE);
	gtk_misc_set_alignment (GTK_MISC (primary_label), 0, 0.5);
	GTK_WIDGET_SET_FLAGS (primary_label, GTK_CAN_FOCUS);
	gtk_label_set_selectable (GTK_LABEL (primary_label), TRUE);

	secondary_text = _("gedit opened this instance of the file in a non-editable way. "
			   "Do you want to edit it anyway?");
  	secondary_markup = g_strdup_printf ("<small>%s</small>",
  					    secondary_text);
	secondary_label = gtk_label_new (secondary_markup);
	g_free (secondary_markup);
	gtk_widget_show (secondary_label);
	gtk_box_pack_start (GTK_BOX (vbox), secondary_label, TRUE, TRUE, 0);
	GTK_WIDGET_SET_FLAGS (secondary_label, GTK_CAN_FOCUS);
	gtk_label_set_use_markup (GTK_LABEL (secondary_label), TRUE);
	gtk_label_set_line_wrap (GTK_LABEL (secondary_label), TRUE);
	gtk_label_set_selectable (GTK_LABEL (secondary_label), TRUE);
	gtk_misc_set_alignment (GTK_MISC (secondary_label), 0, 0.5);

	gedit_message_area_set_contents (GEDIT_MESSAGE_AREA (message_area),
					 hbox_content);
	return message_area;
}

GtkWidget *
gedit_externally_modified_saving_error_message_area_new (
						const gchar  *uri,
						const GError *error)
{
	GtkWidget *message_area;
	GtkWidget *hbox_content;
	GtkWidget *image;
	GtkWidget *vbox;
	gchar *primary_markup;
	gchar *secondary_markup;
	GtkWidget *primary_label;
	GtkWidget *secondary_label;
	gchar *primary_text;
	const gchar *secondary_text;
	gchar *full_formatted_uri;
	gchar *uri_for_display;
	gchar *temp_uri_for_display;

	g_return_val_if_fail (uri != NULL, NULL);
	g_return_val_if_fail (error != NULL, NULL);
	g_return_val_if_fail (error->domain == GEDIT_DOCUMENT_ERROR, NULL);
	g_return_val_if_fail (error->code == GEDIT_DOCUMENT_ERROR_EXTERNALLY_MODIFIED, NULL);

	full_formatted_uri = gedit_utils_format_uri_for_display (uri);

	/* Truncate the URI so it doesn't get insanely wide. Note that even
	 * though the dialog uses wrapped text, if the URI doesn't contain
	 * white space then the text-wrapping code is too stupid to wrap it.
	 */
	temp_uri_for_display = gedit_utils_str_middle_truncate (full_formatted_uri, 
								MAX_URI_IN_DIALOG_LENGTH);								
	g_free (full_formatted_uri);

	uri_for_display = g_markup_printf_escaped ("<i>%s</i>", temp_uri_for_display);
	g_free (temp_uri_for_display);

	message_area = gedit_message_area_new ();
	gedit_message_area_add_stock_button_with_text (GEDIT_MESSAGE_AREA (message_area),
						       _("_Save Anyway"),
						       GTK_STOCK_SAVE,
						       GTK_RESPONSE_YES);
	gedit_message_area_add_button (GEDIT_MESSAGE_AREA (message_area),
				       _("_Don't Save"),
				       GTK_RESPONSE_CANCEL);

	hbox_content = gtk_hbox_new (FALSE, 8);
	gtk_widget_show (hbox_content);				

	image = gtk_image_new_from_stock ("gtk-dialog-warning", GTK_ICON_SIZE_DIALOG);
	gtk_widget_show (image);
	gtk_box_pack_start (GTK_BOX (hbox_content), image, FALSE, FALSE, 0);
	gtk_misc_set_alignment (GTK_MISC (image), 0.5, 0);

	vbox = gtk_vbox_new (FALSE, 6);
	gtk_widget_show (vbox);
	gtk_box_pack_start (GTK_BOX (hbox_content), vbox, TRUE, TRUE, 0);

	// FIXME: review this message, it's not clear since for the user the "modification"
	// could be interpreted as the changes he made in the document. beside "reading" is
	// not accurate (since last load/save)
	primary_text = g_strdup_printf (_("The file %s has been modified since reading it."),
					uri_for_display);
	g_free (uri_for_display);

	primary_markup = g_strdup_printf ("<b>%s</b>", primary_text);
	g_free (primary_text);
	primary_label = gtk_label_new (primary_markup);
	g_free (primary_markup);
	gtk_widget_show (primary_label);
	gtk_box_pack_start (GTK_BOX (vbox), primary_label, TRUE, TRUE, 0);
	gtk_label_set_use_markup (GTK_LABEL (primary_label), TRUE);
	gtk_label_set_line_wrap (GTK_LABEL (primary_label), TRUE);
	gtk_misc_set_alignment (GTK_MISC (primary_label), 0, 0.5);
	GTK_WIDGET_SET_FLAGS (primary_label, GTK_CAN_FOCUS);
	gtk_label_set_selectable (GTK_LABEL (primary_label), TRUE);

	secondary_text = _("If you save it, all the external changes could be lost. Save it anyway?");
  	secondary_markup = g_strdup_printf ("<small>%s</small>",
					    secondary_text);
	secondary_label = gtk_label_new (secondary_markup);
	g_free (secondary_markup);
	gtk_widget_show (secondary_label);
	gtk_box_pack_start (GTK_BOX (vbox), secondary_label, TRUE, TRUE, 0);
	GTK_WIDGET_SET_FLAGS (secondary_label, GTK_CAN_FOCUS);
	gtk_label_set_use_markup (GTK_LABEL (secondary_label), TRUE);
	gtk_label_set_line_wrap (GTK_LABEL (secondary_label), TRUE);
	gtk_label_set_selectable (GTK_LABEL (secondary_label), TRUE);
	gtk_misc_set_alignment (GTK_MISC (secondary_label), 0, 0.5);

	gedit_message_area_set_contents (GEDIT_MESSAGE_AREA (message_area),
					 hbox_content);

	return message_area;
}

GtkWidget *
gedit_no_backup_saving_error_message_area_new (const gchar  *uri,
					       const GError *error)
{
	GtkWidget *message_area;
	GtkWidget *hbox_content;
	GtkWidget *image;
	GtkWidget *vbox;
	gchar *primary_markup;
	gchar *secondary_markup;
	GtkWidget *primary_label;
	GtkWidget *secondary_label;
	gchar *primary_text;
	const gchar *secondary_text;
	gchar *full_formatted_uri;
	gchar *uri_for_display;
	gchar *temp_uri_for_display;

	g_return_val_if_fail (uri != NULL, NULL);
	g_return_val_if_fail (error != NULL, NULL);
	g_return_val_if_fail (error->domain == GEDIT_DOCUMENT_ERROR, NULL);
	g_return_val_if_fail (error->code == GEDIT_DOCUMENT_ERROR_CANT_CREATE_BACKUP, NULL);

	full_formatted_uri = gedit_utils_format_uri_for_display (uri);

	/* Truncate the URI so it doesn't get insanely wide. Note that even
	 * though the dialog uses wrapped text, if the URI doesn't contain
	 * white space then the text-wrapping code is too stupid to wrap it.
	 */
	temp_uri_for_display = gedit_utils_str_middle_truncate (full_formatted_uri, 
								MAX_URI_IN_DIALOG_LENGTH);								
	g_free (full_formatted_uri);

	uri_for_display = g_markup_printf_escaped ("<i>%s</i>", temp_uri_for_display);
	g_free (temp_uri_for_display);

	message_area = gedit_message_area_new ();
	gedit_message_area_add_stock_button_with_text (GEDIT_MESSAGE_AREA (message_area),
						       _("_Save Anyway"),
						       GTK_STOCK_SAVE,
						       GTK_RESPONSE_YES);
	gedit_message_area_add_button (GEDIT_MESSAGE_AREA (message_area),
				       _("_Don't Save"),
				       GTK_RESPONSE_CANCEL);

	hbox_content = gtk_hbox_new (FALSE, 8);
	gtk_widget_show (hbox_content);

	image = gtk_image_new_from_stock ("gtk-dialog-warning", GTK_ICON_SIZE_DIALOG);
	gtk_widget_show (image);
	gtk_box_pack_start (GTK_BOX (hbox_content), image, FALSE, FALSE, 0);
	gtk_misc_set_alignment (GTK_MISC (image), 0.5, 0);

	vbox = gtk_vbox_new (FALSE, 6);
	gtk_widget_show (vbox);
	gtk_box_pack_start (GTK_BOX (hbox_content), vbox, TRUE, TRUE, 0);

	// FIXME: review this messages

	if (gedit_prefs_manager_get_create_backup_copy ())
		primary_text = g_strdup_printf (_("Could not create a backup file while saving %s"),
						uri_for_display);
	else
		primary_text = g_strdup_printf (_("Could not create a temporary backup file while saving %s"),
						uri_for_display);

	g_free (uri_for_display);

	primary_markup = g_strdup_printf ("<b>%s</b>", primary_text);
	g_free (primary_text);
	primary_label = gtk_label_new (primary_markup);
	g_free (primary_markup);
	gtk_widget_show (primary_label);
	gtk_box_pack_start (GTK_BOX (vbox), primary_label, TRUE, TRUE, 0);
	gtk_label_set_use_markup (GTK_LABEL (primary_label), TRUE);
	gtk_label_set_line_wrap (GTK_LABEL (primary_label), TRUE);
	gtk_misc_set_alignment (GTK_MISC (primary_label), 0, 0.5);
	GTK_WIDGET_SET_FLAGS (primary_label, GTK_CAN_FOCUS);
	gtk_label_set_selectable (GTK_LABEL (primary_label), TRUE);

	secondary_text = _("gedit could not backup the old copy of the file before saving the new one. "
			   "You can ignore this warning and save the file anyway, but if an error "
			   "occurs while saving, you could lose the old copy of the file. Save anyway?");
  	secondary_markup = g_strdup_printf ("<small>%s</small>",
					    secondary_text);
	secondary_label = gtk_label_new (secondary_markup);
	g_free (secondary_markup);
	gtk_widget_show (secondary_label);
	gtk_box_pack_start (GTK_BOX (vbox), secondary_label, TRUE, TRUE, 0);
	GTK_WIDGET_SET_FLAGS (secondary_label, GTK_CAN_FOCUS);
	gtk_label_set_use_markup (GTK_LABEL (secondary_label), TRUE);
	gtk_label_set_line_wrap (GTK_LABEL (secondary_label), TRUE);
	gtk_label_set_selectable (GTK_LABEL (secondary_label), TRUE);
	gtk_misc_set_alignment (GTK_MISC (secondary_label), 0, 0.5);

	gedit_message_area_set_contents (GEDIT_MESSAGE_AREA (message_area),
					 hbox_content);

	return message_area;
}

GtkWidget *
gedit_unrecoverable_saving_error_message_area_new (const gchar  *uri,
						   const GError *error)
{
	gchar *error_message = NULL;
	gchar *message_details = NULL;
	gchar *full_formatted_uri;
	gchar *scheme_string;
	gchar *scheme_markup;
       	gchar *uri_for_display;
       	gchar *temp_uri_for_display;
	GtkWidget *message_area;

	g_return_val_if_fail (uri != NULL, NULL);
	g_return_val_if_fail (error != NULL, NULL);
	g_return_val_if_fail (error->domain == GEDIT_DOCUMENT_ERROR, NULL);

	full_formatted_uri = gedit_utils_format_uri_for_display (uri);

	/* Truncate the URI so it doesn't get insanely wide. Note that even
	 * though the dialog uses wrapped text, if the URI doesn't contain
	 * white space then the text-wrapping code is too stupid to wrap it.
	 */
	temp_uri_for_display = gedit_utils_str_middle_truncate (full_formatted_uri, 
								MAX_URI_IN_DIALOG_LENGTH);								
	g_free (full_formatted_uri);

	uri_for_display = g_markup_printf_escaped ("<i>%s</i>", temp_uri_for_display);
	g_free (temp_uri_for_display);

	switch (error->code)
	{
		case GNOME_VFS_ERROR_NOT_SUPPORTED:
			scheme_string = gnome_vfs_get_uri_scheme (uri);

			if ((scheme_string != NULL) && g_utf8_validate (scheme_string, -1, NULL))
			{
				scheme_markup = g_markup_printf_escaped ("<i>%s:</i>", scheme_string);
 
				/* Translators: %s is a URI scheme (like for example http:, ftp:, etc.) */
				message_details = g_strdup_printf (_("gedit cannot handle %s locations in write mode. "
								     "Please check that you typed the "
								     "location correctly and try again."),
								   scheme_markup);
				g_free (scheme_markup);
			}
			else
			{
				message_details = g_strdup (_("gedit cannot handle this location in write mode. "
							      "Please check that you typed the "
							      "location correctly and try again."));
			}

			g_free (scheme_string);
			break;

		case GNOME_VFS_ERROR_TOO_BIG:
			message_details = g_strdup (_("The disk where you are trying to save the file has "
						      "a limitation on file sizes.  Please try saving "
						      "a smaller file or saving it to a disk that does not "
						      "have this limitation."));
			break;

		case GNOME_VFS_ERROR_INVALID_URI:
			message_details = g_strdup (_("%s is not a valid location. " 
						      "Please check that you typed the "
						      "location correctly and try again."));
			break;

		case GNOME_VFS_ERROR_ACCESS_DENIED:
		case GNOME_VFS_ERROR_NOT_PERMITTED:		
			message_details = g_strdup (_("You do not have the permissions necessary to save the file. "
						      "Please check that you typed the "
						      "location correctly and try again."));

			break;

		case GNOME_VFS_ERROR_TOO_MANY_OPEN_FILES:
			message_details = g_strdup (_("There are too many open files. Please "
						      "close some applications and try again."));
			break;

		case GNOME_VFS_ERROR_IS_DIRECTORY:
			message_details = g_strdup (_("%s is a directory. "
						      "Please check that you typed the "
						      "location correctly and try again."));
			break;

		case GNOME_VFS_ERROR_NO_MEMORY:
			message_details = g_strdup (_("Not enough available memory to save "
						      "the file. Please close some running "
						      "applications and try again."));
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

				if (vfs_uri != NULL)
				{
					const gchar *hn = gnome_vfs_uri_get_host_name (vfs_uri);

					if (hn != NULL)
					{
						gchar *host_name = gedit_utils_make_valid_utf8 (hn);
						gchar *host_markup = g_markup_printf_escaped ("<i>%s</i>", host_name);
						g_free (host_name);		

						/* Translators: %s is a host name */
						message_details = g_strdup_printf (
							_("Host %s could not be found. "
		        		  	  	  "Please check that your proxy settings "
					  	  	  "are correct and try again."),
						  	host_markup);

						g_free (host_markup);
					}
					else
					{
						/* use the same string as INVALID_HOST */
						message_details = g_strdup_printf (
							_("Host name was invalid. "
							  "Please check that you typed the location "
							  "correctly and try again."));
					}

					gnome_vfs_uri_unref (vfs_uri);		
				}
				else
				{
					/* use the same string as INVALID_HOST */
					message_details = g_strdup_printf (
						_("Host name was invalid. "
						  "Please check that you typed the location "
						  "correctly and try again."));
				}
			}
			break;

		case GNOME_VFS_ERROR_INVALID_HOST_NAME:
			message_details = g_strdup_printf (_("Host name was invalid. "
							     "Please check that you typed the location "
							     "correctly and try again."));
				break;

		case GNOME_VFS_ERROR_HOST_HAS_NO_ADDRESS:
			message_details = g_strdup (_("Host name was empty. "
						      "Please check that your proxy settings "
						      "are correct and try again."));
			break;

		case GNOME_VFS_ERROR_LOGIN_FAILED:
			message_details = g_strdup (_("Attempt to log in failed. "
						      "Please check that you typed the location "
						      "correctly and try again."));
			break;

		case GNOME_VFS_ERROR_NO_SPACE:
			message_details = g_strdup (_("There is not enough disk space to save the file. "
						      "Please free some disk space and try again."));
			break;
			
		case GNOME_VFS_ERROR_READ_ONLY:
		case GNOME_VFS_ERROR_READ_ONLY_FILE_SYSTEM:		
			message_details = g_strdup (_("You are trying to save the file on a read-only disk. "
						      "Please check that you typed the location "
						      "correctly and try again."));
			break;
			
		case GNOME_VFS_ERROR_FILE_EXISTS:
			message_details = g_strdup (_("A file with the same name already exists. "
						      "Please use a different name."));
			break;
			
		case GNOME_VFS_ERROR_NAME_TOO_LONG:
			message_details = g_strdup (_("The disk where you are trying to save the file has "
						      "a limitation on length of the file names. "
						      "Please use a shorter name."));

		case GNOME_VFS_ERROR_TIMEOUT:
			message_details = g_strdup (_("Connection timed out. Please try again."));
			break;

		case GEDIT_DOCUMENT_ERROR_NOT_REGULAR_FILE:
			message_details = g_strdup_printf (_("%s is not a regular file. "
							     "Please check that you typed the location "
							     "correctly and try again."),
							   uri_for_display);

		case GNOME_VFS_ERROR_GENERIC:
			break;

		/*
		case GNOME_VFS_ERROR_NOT_FOUND:
		case GNOME_VFS_ERROR_CORRUPTED_DATA:
		case GNOME_VFS_ERROR_WRONG_FORMAT:
		case GNOME_VFS_ERROR_INTERNAL:
		case GNOME_VFS_ERROR_BAD_PARAMETERS:
		case GNOME_VFS_ERROR_IO:
		case GNOME_VFS_ERROR_BAD_FILE:
		case GNOME_VFS_ERROR_NOT_OPEN:
		case GNOME_VFS_ERROR_INVALID_OPEN_MODE:
		case GNOME_VFS_ERROR_EOF:
		case GNOME_VFS_ERROR_NOT_A_DIRECTORY:
		case GNOME_VFS_ERROR_IN_PROGRESS:
		case GNOME_VFS_ERROR_INTERRUPTED:
		case GNOME_VFS_ERROR_LOOP:
		case GNOME_VFS_ERROR_CANCELLED:
		case GNOME_VFS_ERROR_DIRECTORY_BUSY:
		case GNOME_VFS_ERROR_DIRECTORY_NOT_EMPTY:
		case GNOME_VFS_ERROR_TOO_MANY_LINKS:
		case GNOME_VFS_ERROR_NOT_SAME_FILE_SYSTEM:
		case GNOME_VFS_ERROR_SERVICE_NOT_AVAILABLE:
		case GNOME_VFS_ERROR_SERVICE_OBSOLETE,
		case GNOME_VFS_ERROR_PROTOCOL_ERROR,
		case GNOME_VFS_NUM_ERRORS:
		*/

		default: 
			g_warning ("Hit unhandled case %d (%s) in %s.", 
				   error->code, gnome_vfs_result_to_string (error->code), G_STRFUNC);	
			message_details = g_strdup_printf (_("Unexpected error: %s"), 
							   gnome_vfs_result_to_string (error->code));								 
			break;
	}

	if (error_message == NULL)
		error_message = g_strdup_printf (_("Could not save the file %s."),
						 uri_for_display);

	message_area = create_io_loading_error_message_area (error_message,
							     message_details);

	g_free (uri_for_display);
	g_free (error_message);
	g_free (message_details);

	return message_area;
}

