/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-io-error-dialogs.c
 * This file is part of gedit
 *
 * Copyright (C) 1998, 1999 Alex Roberts, Evan Lawrence
 * Copyright (C) 2000, 2002 Chema Celorio, Paolo Maggi 
 * Copyright (C) 2003, 2004 Paolo Maggi 
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
#include "gedit-io-error-dialogs.h"

#define MAX_URI_IN_DIALOG_LENGTH 50


void
gedit_error_reporting_loading_file (const gchar *uri,
				    const GeditEncoding *encoding,
				    GError *error,
				    GtkWindow *parent)
{
	gchar *error_message = NULL;
	gchar *message_details = NULL;
	gchar *full_formatted_uri;
	gchar *scheme_string;
       	gchar *uri_for_display;
	gchar *encoding_name;
	GtkWidget *dialog;

	g_return_if_fail (uri != NULL);

	full_formatted_uri = gnome_vfs_format_uri_for_display (uri);

	/* Truncate the URI so it doesn't get insanely wide. Note that even
	 * though the dialog uses wrapped text, if the URI doesn't contain
	 * white space then the text-wrapping code is too stupid to wrap it.
	 */
	uri_for_display = gedit_utils_str_middle_truncate (full_formatted_uri, 
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
			error_message = g_strdup_printf (_("Could not find the file \"%s\""),
							 uri_for_display);
			message_details = g_strdup (_("Please, check that you typed the "
						      "location correctly and try again."));
			break;

		case GNOME_VFS_ERROR_CORRUPTED_DATA:
			error_message = g_strdup_printf (_("Could not open the file \"%s\""),
							 uri_for_display);
			message_details = g_strdup (_("The file contains corrupted data."));
			break;

		case GNOME_VFS_ERROR_NOT_SUPPORTED:
			scheme_string = gnome_vfs_get_uri_scheme (uri);

			error_message = g_strdup_printf (_("Could not open the file \"%s\""),
							 uri_for_display);

			if ((scheme_string != NULL) && g_utf8_validate (scheme_string, -1, NULL))
			{
				message_details = g_strdup_printf (_("gedit cannot handle %s: locations."),
								   scheme_string);
			}
			else
			{
				message_details = g_strdup (_("gedit cannot handle this location."));
			}

			g_free (scheme_string);
			break;

		case GNOME_VFS_ERROR_WRONG_FORMAT:
			error_message = g_strdup_printf (_("Could not open the file \"%s\""),
							 uri_for_display);
			message_details = g_strdup (_("The file contains data in an invalid format."));
			break;

		case GNOME_VFS_ERROR_TOO_BIG:
			error_message = g_strdup_printf (_("Could not open the file \"%s\""),
							 uri_for_display);
			message_details = g_strdup (_("The file is too big."));
			break;

		case GNOME_VFS_ERROR_INVALID_URI:
			error_message = g_strdup_printf (_("\"%s\" is not a valid location"),
							 uri_for_display);
			message_details = g_strdup (_("Please, check that you typed the "
						      "location correctly and try again."));
                	break;

		case GNOME_VFS_ERROR_ACCESS_DENIED:
			error_message = g_strdup_printf (_("Could not open the file \"%s\""),
							 uri_for_display);
			message_details = g_strdup (_("Access was denied."));
			break;

		case GNOME_VFS_ERROR_TOO_MANY_OPEN_FILES:
			error_message = g_strdup_printf (_("Could not open the file \"%s\""),
							 uri_for_display);
			message_details = g_strdup (_("there are too many open files. Please, "
						      "close some open file and try again."));
			break;

		case GNOME_VFS_ERROR_IS_DIRECTORY:
			error_message = g_strdup_printf (_("\"%s\" is a directory"),
							 uri_for_display);
			message_details = g_strdup (_("Please, check that you typed the "
						      "location correctly and try again."));
			break;

		case GNOME_VFS_ERROR_NO_MEMORY:
			error_message = g_strdup_printf (_("Could not open the file \"%s\""),
							 uri_for_display);
			message_details = g_strdup (_("Not enough available memory to open "
						      "the file. Please, close some running "
						      "application and try again."));
			break;

		case GNOME_VFS_ERROR_HOST_NOT_FOUND:
			/* This case can be hit for user-typed strings like "foo" due to
			 * the code that guesses web addresses when there's no initial "/".
			 * But this case is also hit for legitimate web addresses when
			 * the proxy is set up wrong.
			 */
			{
				GnomeVFSURI *vfs_uri;

				error_message = g_strdup_printf (_("Could not open the file \"%s\""),
								 uri_for_display);

				vfs_uri = gnome_vfs_uri_new (uri);

				if (vfs_uri != NULL)
				{
					const gchar *hn = gnome_vfs_uri_get_host_name (vfs_uri);

					if (hn != NULL)
					{
						gchar *host_name = gedit_utils_make_valid_utf8 (hn);

						message_details = g_strdup_printf (
							_("Host \"%s\" could not be found. "
        	        		  	  	"Please, check that your proxy settings "
					  	  	"are correct and try again."),
						  	host_name);

						g_free (host_name);
					}
					else
					{
						/* use the same string as INVALID_HOST */
						message_details = g_strdup_printf (
							_("Host name was invalid. "
							  "Please, check that you typed the location "
							  "correctly and try again."));
					}

					gnome_vfs_uri_unref (vfs_uri);		
				}
				else
				{
					/* use the same string as INVALID_HOST */
					message_details = g_strdup_printf (
						_("Host name was invalid. "
						  "Please, check that you typed the location "
						  "correctly and try again."));
				}
			}
			break;

		case GNOME_VFS_ERROR_INVALID_HOST_NAME:
			error_message = g_strdup_printf (_("Could not open the file \"%s\""),
							 uri_for_display);
			message_details = g_strdup_printf (_("Host name was invalid. "
							     "Please, check that you typed the location "
							     "correctly and try again."));
               		break;

		case GNOME_VFS_ERROR_HOST_HAS_NO_ADDRESS:
			error_message = g_strdup_printf (_("Could not open the file \"%s\""),
							 uri_for_display);
			message_details = g_strdup (_("Host name was empty. "
						      "Please, check that your proxy settings "
						      "are correct and try again."));
			break;

		case GNOME_VFS_ERROR_LOGIN_FAILED:
			error_message = g_strdup_printf (_("Could not open the file \"%s\""),
							 uri_for_display);
			message_details = g_strdup (_("Attempt to log in failed. "
						      "Please, check that you typed the location "
						      "correctly and try again."));
			break;

		case GEDIT_ERROR_INVALID_UTF8_DATA:
			error_message = g_strdup_printf (_("Could not open the file \"%s\""),
							 uri_for_display);
			message_details = g_strdup (_("The file contains invalid data. "
						      "Probably, you are trying to open a binary file."));
			break;

		case GNOME_VFS_ERROR_GENERIC:
			error_message = g_strdup_printf (_("Could not open the file \"%s\""),
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
			error_message = g_strdup_printf (_("Could not open the file \"%s\""),
							 uri_for_display);
			message_details = g_strdup (_("gedit was not able to automatically detect "
						      "the character coding. "
						      "Please, check that you are not trying to open a binary file "
						      "and try again selecting a character coding in the 'Open File...' "
						      "(or 'Open Location') dialog."));
			break;

		case GEDIT_CONVERT_ERROR_ILLEGAL_SEQUENCE:
			error_message = g_strdup_printf (_("Could not open the file \"%s\" using "
							   "the %s character coding"),
							 uri_for_display, encoding_name);
			message_details = g_strdup (_("Please, check that you are not trying"
						      "to open a binary file "
						      "and that you selected the right "
						      "character coding in the 'Open File... ' "
						      "(or 'Open Location') dialog and try again."));
			break;

		case GEDIT_CONVERT_ERROR_BINARY_FILE:
			error_message = g_strdup_printf (_("Could not open the file \"%s\""),
							 uri_for_display);
			message_details = g_strdup (_("The file contains data in an invalid data. "
						      "Probably, you are trying to open a binary file."));
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
			error_message = g_strdup_printf (_("Could not open the file \"%s\" using "
							   "the %s character coding"),
							 uri_for_display, encoding_name);
			message_details = g_strdup (_("Please, check that you are not trying"
						      "to open a binary file "
						      "and that you selected the right "
						      "character coding in the 'Open File... ' "
						      "(or 'Open Location') dialog and try again."));
			break;
		}
	}
	
	if (error_message == NULL)
		error_message = g_strdup_printf (_("Could not open the file \"%s\""),
						 uri_for_display);

	if ((message_details == NULL) && (error != NULL) && (error->message != NULL))
			message_details = g_strdup (error->message);

	dialog = gtk_message_dialog_new (parent,
					 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
					 GTK_MESSAGE_ERROR,
					 GTK_BUTTONS_OK,
					 error_message);

	if (message_details != NULL)
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
							  message_details);

	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
	gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);

	g_free (uri_for_display);
	g_free (encoding_name);
	g_free (error_message);
	g_free (message_details);
}

void
gedit_error_reporting_reverting_file (const gchar *uri,
				      const GError *error,
				      GtkWindow *parent)
{
	gchar *error_message = NULL;
	gchar *message_details = NULL;
	gchar *scheme_string;
	gchar *full_formatted_uri;
       	gchar *uri_for_display;
	GtkWidget *dialog;

	g_return_if_fail (uri != NULL);
	g_return_if_fail (error != NULL);

	full_formatted_uri = gnome_vfs_format_uri_for_display (uri);

	/* Truncate the URI so it doesn't get insanely wide. Note that even
	 * though the dialog uses wrapped text, if the URI doesn't contain
	 * white space then the text-wrapping code is too stupid to wrap it.
	 */
	uri_for_display = gedit_utils_str_middle_truncate (full_formatted_uri, 
							   MAX_URI_IN_DIALOG_LENGTH);
	g_free (full_formatted_uri);

	switch (error->code)
	{
	case GNOME_VFS_ERROR_NOT_FOUND:
		error_message = g_strdup_printf (_("Could not revert the file \"%s\""),
						 uri_for_display);
		message_details = g_strdup (_("gedit cannot find it. "
					      "Perhaps, it has recently been deleted."));
		break;

	case GNOME_VFS_ERROR_CORRUPTED_DATA:
		error_message = g_strdup_printf (_("Could not revert the file \"%s\""),
						 uri_for_display);
		message_details = g_strdup (_("The file contains corrupted data."));
		break;

	case GNOME_VFS_ERROR_NOT_SUPPORTED:
		error_message = g_strdup_printf (_("Could not revert the file \"%s\""),
						 uri_for_display);

		scheme_string = gnome_vfs_get_uri_scheme (uri);

		if ((scheme_string != NULL) && g_utf8_validate (scheme_string, -1, NULL))
		{
			message_details = g_strdup_printf (_("gedit cannot handle %s: locations."),
							   scheme_string);
		}
		else
		{
			message_details = g_strdup (_("gedit cannot handle this location."));
		}	

		g_free (scheme_string);
       	        break;

	case GNOME_VFS_ERROR_WRONG_FORMAT:
		error_message = g_strdup_printf (_("Could not revert the file \"%s\""),
						 uri_for_display);
		message_details = g_strdup (_("The file contains data in an invalid format."));
		break;

	case GNOME_VFS_ERROR_TOO_BIG:
		error_message = g_strdup_printf (_("Could not revert the file \"%s\""),
						 uri_for_display);
		message_details = g_strdup (_("The file is too big."));
		break;

	case GNOME_VFS_ERROR_ACCESS_DENIED:
		error_message = g_strdup_printf (_("Could not revert the file \"%s\""),
						 uri_for_display);
		message_details = g_strdup (_("Access was denied."));
		break;

	case GNOME_VFS_ERROR_TOO_MANY_OPEN_FILES:
		error_message = g_strdup_printf (_("Could not revert the file \"%s\""),
						 uri_for_display);
		message_details = g_strdup (_("There are too many open files. "
					      "Please, close some open file and try again."));
		break;

	case GNOME_VFS_ERROR_NO_MEMORY:
		error_message = g_strdup_printf (_("Could not revert the file \"%s\""),
						 uri_for_display);
		message_details = g_strdup (_("Not enough available memory. "
					      "Please, close some running application and try again."));
               	break;

	case GNOME_VFS_ERROR_HOST_NOT_FOUND:
		/* This case can be hit for user-typed strings like "foo" due to
		 * the code that guesses web addresses when there's no initial "/".
		 * But this case is also hit for legitimate web addresses when
		 * the proxy is set up wrong.
		 */
		{
			GnomeVFSURI *vfs_uri;

			error_message = g_strdup_printf (_("Could not revert the file \"%s\""),
							 uri_for_display);

			vfs_uri = gnome_vfs_uri_new (uri);

			if (vfs_uri != NULL)
			{
				const gchar *hn = gnome_vfs_uri_get_host_name (vfs_uri);

				if (hn != NULL)
				{
					gchar *host_name = gedit_utils_make_valid_utf8 (hn);

					message_details = g_strdup_printf (
						_("Host \"%s\" could not be found. "
        	        		  	  "Please, check that your proxy settings "
					  	  "are correct and try again."),
						  host_name);

						g_free (host_name);
				}

				gnome_vfs_uri_unref (vfs_uri);		
			}
		}
		break;

	case GNOME_VFS_ERROR_HOST_HAS_NO_ADDRESS:
		error_message = g_strdup_printf (_("Could not revert the file \"%s\""),
						 uri_for_display);
		message_details = g_strdup (_("The host name was empty. "
					      "Please, check that your proxy settings "
					      "are correct and try again."));
		break;

	case GNOME_VFS_ERROR_LOGIN_FAILED:
		error_message = g_strdup_printf (_("Could not revert the file \"%s\""),
						 uri_for_display);
		message_details = g_strdup (_("Attempt to log in failed."));		
		break;

	case GEDIT_ERROR_INVALID_UTF8_DATA:
		error_message = g_strdup_printf (_("Could not revert the file \"%s\""),
						 uri_for_display);
		message_details = g_strdup (_("The file contains invalid UTF-8 data. "
					      "Probably, you are trying to revert a binary file."));
		break;

	case GEDIT_ERROR_UNTITLED:
		error_message = g_strdup_printf (_("Could not revert the file \"%s\""),
						 uri_for_display);
		message_details = g_strdup (_("It is not possible to revert an Untitled document."));
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
		error_message = g_strdup_printf (_("Could not revert the file \"%s\""),
						 uri_for_display);
		break;
	}

	dialog = gtk_message_dialog_new (parent,
					 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
					 GTK_MESSAGE_ERROR,
					 GTK_BUTTONS_OK,
					 error_message);

	if (message_details != NULL)
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
							  message_details);

	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
	gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);

	g_free (uri_for_display);
	g_free (error_message);
	g_free (message_details);
}

void
gedit_error_reporting_saving_file (const gchar *uri,
				   GError *error,
				   GtkWindow *parent)
{
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
	uri_for_display = gedit_utils_str_middle_truncate (full_formatted_uri, 
							   MAX_URI_IN_DIALOG_LENGTH);
	g_free (full_formatted_uri);

	error_message = g_strdup_printf (_("Could not save the file \"%s\""),
					 uri_for_display);

	dialog = gtk_message_dialog_new (parent,
					 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
					 GTK_MESSAGE_ERROR,
					 GTK_BUTTONS_OK,
					 error_message);

	if (error->message != NULL && strcmp (error->message, " ") != 0)
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
							  error->message);

	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
	gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);

	g_free (uri_for_display);
	g_free (error_message);
}

void
gedit_error_reporting_creating_file (const gchar *uri,
				     gint error_code,
				     GtkWindow *parent)
{
	gchar *error_message = NULL;
	gchar *message_details = NULL;
	gchar *full_formatted_uri;
	gchar *uri_for_display;
	GtkWidget *dialog;

	g_return_if_fail (uri != NULL);

	full_formatted_uri = gnome_vfs_format_uri_for_display (uri);

	/* Truncate the URI so it doesn't get insanely wide. Note that even
	 * though the dialog uses wrapped text, if the URI doesn't contain
	 * white space then the text-wrapping code is too stupid to wrap it.
	 */
	uri_for_display = gedit_utils_str_middle_truncate (full_formatted_uri, 
							   MAX_URI_IN_DIALOG_LENGTH);
	g_free (full_formatted_uri);

	switch (error_code)
	{
	case EEXIST:
		error_message = g_strdup_printf (_("The file \"%s\" already exists"),
						 uri_for_display);
		break;

	case EISDIR:
		error_message = g_strdup_printf (_("\"%s\" is a directory"), uri_for_display);
		message_details = g_strdup (_("Please, check that you typed the location correctly."));
               	break;

	case EACCES:
	case EROFS:
	case ETXTBSY:
		error_message = g_strdup_printf (_("Could not create the file \"%s\""),
						 uri_for_display);
		message_details = g_strdup (_("Make sure you have the appropriate write permissions."));
		break;

	case ENAMETOOLONG:
		error_message = g_strdup_printf (_("Could not create the file \"%s\""),
						 uri_for_display);
		message_details = g_strdup (_("The file name is too long."));
		break;

	case ENOENT:
		error_message = g_strdup_printf (_("Could not create the file \"%s\""),
						 uri_for_display);
		message_details = g_strdup (_("A directory component in the file name "
					      "does not exist or is a dangling symbolic link."));
		break;

	case ENOSPC:
		error_message = g_strdup_printf (_("Could not create the file \"%s\""),
						 uri_for_display);
		message_details = g_strdup (_("There is not enough disk space to create the file. "
					      "Please free some disk space and try again."));
		break;

	default:
		error_message = g_strdup_printf (_("Could not create the file \"%s\""),
						 uri_for_display);
	}

	dialog = gtk_message_dialog_new (parent,
					 GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
					 GTK_MESSAGE_ERROR,
					 GTK_BUTTONS_OK,
					 error_message);

	if (message_details != NULL)
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
							  message_details);

	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);
	gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);

	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);

	g_free (uri_for_display);
	g_free (error_message);
	g_free (message_details);
}

