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

#include <libgnomeui/libgnomeui.h>
#include <glib/gunicode.h>
#include <libgnome/gnome-i18n.h>
#include <libgnomevfs/gnome-vfs.h>
#include <eel/eel-vfs-extensions.h>
#include <eel/eel-string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <string.h>

#include "gedit-utils.h"
#include "gedit2.h"
#include "bonobo-mdi.h"
#include "gedit-document.h"
#include "gedit-prefs-manager.h"
#include "gedit-encodings.h"
#include "gedit-debug.h"

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
 	gtk_timeout_remove (mi->timeoutid);
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
		gtk_timeout_remove (current_mi->timeoutid);
		remove_message_timeout (current_mi);
	}
	
	if (bonobo_ui_component_path_exists (ui_component, "/status", NULL))
	{
    		MessageInfo * mi;

		bonobo_ui_component_set_status (ui_component, flash, NULL);
    		
		mi = g_new(MessageInfo, 1);

    		mi->timeoutid = 
      			gtk_timeout_add (flash_length,
				(GtkFunction) remove_message_timeout,
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
gedit_utils_flash (gchar *msg)
{
	g_return_if_fail (msg != NULL);
	
	bonobo_window_flash (bonobo_mdi_get_active_window (BONOBO_MDI (gedit_mdi)), msg);

	/* Update UI */
	while (gtk_events_pending ())
		  gtk_main_iteration ();
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

gboolean
gedit_utils_uri_has_file_scheme (const gchar *uri)
{
	gchar* canonical_uri = NULL;
	gboolean res;
	
	canonical_uri = eel_make_uri_canonical (uri);
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
			
	canonical_uri = eel_make_uri_canonical (uri);
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

GtkWidget* 
gedit_button_new_with_stock_image (const gchar* text, const gchar* stock_id)
{
	GtkWidget *button;
	GtkStockItem item;
	GtkWidget *label;
	GtkWidget *image;
	GtkWidget *hbox;
	GtkWidget *align;

	button = gtk_button_new ();

 	if (GTK_BIN (button)->child)
    		gtk_container_remove (GTK_CONTAINER (button),
				      GTK_BIN (button)->child);

  	if (gtk_stock_lookup (stock_id, &item))
    	{
      		label = gtk_label_new_with_mnemonic (text);

		gtk_label_set_mnemonic_widget (GTK_LABEL (label), GTK_WIDGET (button));
      
		image = gtk_image_new_from_stock (stock_id, GTK_ICON_SIZE_BUTTON);
      		hbox = gtk_hbox_new (FALSE, 2);

      		align = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
      
      		gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 0);
      		gtk_box_pack_end (GTK_BOX (hbox), label, FALSE, FALSE, 0);
      
      		gtk_container_add (GTK_CONTAINER (button), align);
      		gtk_container_add (GTK_CONTAINER (align), hbox);
      		gtk_widget_show_all (align);

      		return button;
    	}

      	label = gtk_label_new_with_mnemonic (text);
      	gtk_label_set_mnemonic_widget (GTK_LABEL (label), GTK_WIDGET (button));
  
  	gtk_misc_set_alignment (GTK_MISC (label), 0.5, 0.5);

  	gtk_widget_show (label);
  	gtk_container_add (GTK_CONTAINER (button), label);

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

	button = gedit_button_new_with_stock_image (text, stock_id);
	g_return_val_if_fail (button != NULL, NULL);

	GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);

	gtk_widget_show (button);

	gtk_dialog_add_action_widget (dialog, button, response_id);	

	return button;
}

/*
 * ATTENTION:
 * THIS IS AN ATTEMPT TO MAKE A FULLY INTERNATIONALIZED
 * CASELESS SEARCH
 * IT STILL HAS SOME BUGS
 */

static gchar *g_utf8_strcasestr(const gchar *haystack, const gchar *needle);

static gchar *
g_utf8_strcasestr(const gchar *haystack, const gchar *needle)
{
	gsize needle_len;
	gsize haystack_len;
	gchar *ret = NULL;
	gchar *p;
	gchar *casefold;
	gchar *caseless_haystack;
	gchar *caseless_needle;
	
	g_return_val_if_fail (haystack != NULL, NULL);
	g_return_val_if_fail (needle != NULL, NULL);
	      
	casefold = g_utf8_casefold (haystack, -1);
	caseless_haystack = g_utf8_normalize (casefold, -1, G_NORMALIZE_ALL);
	g_free (casefold);

	casefold = g_utf8_casefold (needle, -1);
	caseless_needle = g_utf8_normalize (casefold, -1, G_NORMALIZE_ALL);
	g_free (casefold);

	needle_len = g_utf8_strlen (caseless_needle, -1);
	haystack_len = g_utf8_strlen (caseless_haystack, -1);

	if (needle_len == 0)
	{
		ret = (gchar *)haystack;
		goto finally_1;
	}

      	if (haystack_len < needle_len)
	{
		ret = NULL;
		goto finally_1;
	}
			
	p = (gchar*)haystack;
	needle_len = strlen (caseless_needle);
	
	/* Not so efficient */
	while (*p)
	{
		if ((memcmp (caseless_haystack, caseless_needle, needle_len) == 0))
		{
			ret = p;
			goto finally_1;
		}

		g_free (caseless_haystack);
					
		p = g_utf8_next_char (p);	

		casefold = g_utf8_casefold (p, -1);
		caseless_haystack = g_utf8_normalize (casefold, -1, G_NORMALIZE_ALL);
		g_free (casefold);
	}      

finally_1:

	g_free (caseless_needle);
	g_free (caseless_haystack);
	
	return ret;
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

	if (len_s1 != len_s2)
		goto finally_2;

	ret = (memcmp (normalized_s1, normalized_s2, len_s1) == 0);
	
finally_2:
	g_free (normalized_s1);
	g_free (normalized_s2);	

	return ret;
}

/**********************************************************
 * The following code is a modified version of the gtk one
 **********************************************************/

/*	
gboolean      _gtk_text_btree_char_is_invisible (const GtkTextIter *iter);
*/
#define GTK_TEXT_UNKNOWN_CHAR 0xFFFC


static void
forward_chars_with_skipping (GtkTextIter *iter,
                             gint         count,
                             gboolean     skip_invisible,
                             gboolean     skip_nontext)
{

  gint i;

  g_return_if_fail (count >= 0);

  i = count;

  while (i > 0)
    {
      gboolean ignored = FALSE;

      if (skip_nontext &&
          gtk_text_iter_get_char (iter) == GTK_TEXT_UNKNOWN_CHAR)
        ignored = TRUE;

      if (!ignored &&
          skip_invisible &&
          /* _gtk_text_btree_char_is_invisible (iter)*/ FALSE)
        ignored = TRUE;

      gtk_text_iter_forward_char (iter);

      if (!ignored)
        --i;
    }
}

static gboolean
lines_match (const GtkTextIter *start,
             const gchar **lines,
             gboolean visible_only,
             gboolean slice,
             GtkTextIter *match_start,
             GtkTextIter *match_end)
{
  GtkTextIter next;
  gchar *line_text;
  const gchar *found;
  gint offset;

  if (*lines == NULL || **lines == '\0')
    {
      if (match_start)
        *match_start = *start;

      if (match_end)
        *match_end = *start;
      return TRUE;
    }

  next = *start;
  gtk_text_iter_forward_line (&next);

  /* No more text in buffer, but *lines is nonempty */
  if (gtk_text_iter_equal (start, &next))
    {
      return FALSE;
    }

  if (slice)
    {
      if (visible_only)
        line_text = gtk_text_iter_get_visible_slice (start, &next);
      else
        line_text = gtk_text_iter_get_slice (start, &next);
    }
  else
    {
      if (visible_only)
        line_text = gtk_text_iter_get_visible_text (start, &next);
      else
        line_text = gtk_text_iter_get_text (start, &next);
    }

  if (match_start) /* if this is the first line we're matching */
    found = g_utf8_strcasestr (line_text, *lines);
  else
    {
      /* If it's not the first line, we have to match from the
       * start of the line.
       */
      if (g_utf8_caselessnmatch (line_text, *lines, strlen (line_text), strlen (*lines)) == 0)
        found = line_text;
      else
        found = NULL;
    }

  if (found == NULL)
    {
      g_free (line_text);
      return FALSE;
    }

  /* Get offset to start of search string */
  offset = g_utf8_strlen (line_text, found - line_text);

  next = *start;

  /* If match start needs to be returned, set it to the
   * start of the search string.
   */
  if (match_start)
    {
      *match_start = next;

      forward_chars_with_skipping (match_start, offset,
                                   visible_only, !slice);
    }

  /* Go to end of search string */
  offset += g_utf8_strlen (*lines, -1);

  forward_chars_with_skipping (&next, offset,
                               visible_only, !slice);

  g_free (line_text);

  ++lines;

  if (match_end)
    *match_end = next;

  /* pass NULL for match_start, since we don't need to find the
   * start again.
   */
  return lines_match (&next, lines, visible_only, slice, NULL, match_end);
}

/* strsplit () that retains the delimiter as part of the string. */
static gchar **
strbreakup (const char *string,
            const char *delimiter,
            gint        max_tokens)
{
  GSList *string_list = NULL, *slist;
  gchar **str_array, *s;
  guint i, n = 1;

  g_return_val_if_fail (string != NULL, NULL);
  g_return_val_if_fail (delimiter != NULL, NULL);

  if (max_tokens < 1)
    max_tokens = G_MAXINT;

  s = strstr (string, delimiter);
  if (s)
    {
      guint delimiter_len = strlen (delimiter);

      do
        {
          guint len;
          gchar *new_string;

          len = s - string + delimiter_len;
          new_string = g_new (gchar, len + 1);
          strncpy (new_string, string, len);
          new_string[len] = 0;
          string_list = g_slist_prepend (string_list, new_string);
          n++;
          string = s + delimiter_len;
          s = strstr (string, delimiter);
        }
      while (--max_tokens && s);
    }
  if (*string)
    {
      n++;
      string_list = g_slist_prepend (string_list, g_strdup (string));
    }

  str_array = g_new (gchar*, n);

  i = n - 1;

  str_array[i--] = NULL;
  for (slist = string_list; slist; slist = slist->next)
    str_array[i--] = slist->data;

  g_slist_free (string_list);

  return str_array;
}

/**
 * gedit_text_iter_forward_search:
 * @iter: start of search
 * @str: a search string
 * @flags: flags affecting how the search is done
 * @match_start: return location for start of match, or %NULL
 * @match_end: return location for end of match, or %NULL
 * @limit: bound for the search, or %NULL for the end of the buffer
 * 
 * Searches forward for @str. Any match is returned as the range
 * @match_start, @match_end. The search will not continue past
 * @limit. Note that a search is a linear or O(n) operation, so you
 * may wish to use @limit to avoid locking up your UI on large
 * buffers.
 * 
 * If the #GTK_TEXT_SEARCH_VISIBLE_ONLY flag is present, the match may
 * have invisible text interspersed in @str. i.e. @str will be a
 * possibly-noncontiguous subsequence of the matched range. similarly,
 * if you specify #GTK_TEXT_SEARCH_TEXT_ONLY, the match may have
 * pixbufs or child widgets mixed inside the matched range. If these
 * flags are not given, the match must be exact; the special 0xFFFC
 * character in @str will match embedded pixbufs or child widgets.
 *
 * Return value: whether a match was found
 **/
gboolean
gedit_text_iter_forward_search (const GtkTextIter *iter,
                              const gchar       *str,
                              GtkTextSearchFlags flags,
                              GtkTextIter       *match_start,
                              GtkTextIter       *match_end,
                              const GtkTextIter *limit)
{
  gchar **lines = NULL;
  GtkTextIter match;
  gboolean retval = FALSE;
  GtkTextIter search;
  gboolean visible_only;
  gboolean slice;
  
  g_return_val_if_fail (iter != NULL, FALSE);
  g_return_val_if_fail (str != NULL, FALSE);

  if ((flags & GTK_TEXT_SEARCH_CASE_INSENSITIVE) == 0)
	  return gtk_text_iter_forward_search (iter, str, flags,
			 match_start, match_end, limit); 

  if (limit &&
      gtk_text_iter_compare (iter, limit) >= 0)
    return FALSE;
  
  if (*str == '\0')
    {
      /* If we can move one char, return the empty string there */
      match = *iter;
      
      if (gtk_text_iter_forward_char (&match))
        {
          if (limit &&
              gtk_text_iter_equal (&match, limit))
            return FALSE;
          
          if (match_start)
            *match_start = match;
          if (match_end)
            *match_end = match;
          return TRUE;
        }
      else
        return FALSE;
    }

  visible_only = (flags & GTK_TEXT_SEARCH_VISIBLE_ONLY) != 0;
  slice = (flags & GTK_TEXT_SEARCH_TEXT_ONLY) == 0;
  
  /* locate all lines */

  lines = strbreakup (str, "\n", -1);

  search = *iter;

  do
    {
      /* This loop has an inefficient worst-case, where
       * gtk_text_iter_get_text () is called repeatedly on
       * a single line.
       */
      GtkTextIter end;

      if (limit &&
          gtk_text_iter_compare (&search, limit) >= 0)
        break;
      
      if (lines_match (&search, (const gchar**)lines,
                       visible_only, slice, &match, &end))
        {
          if (limit == NULL ||
              (limit &&
               gtk_text_iter_compare (&end, limit) < 0))
            {
              retval = TRUE;
              
              if (match_start)
                *match_start = match;
              
              if (match_end)
	        *match_end = end;
	      
            }
          
          break;
        }
    }
  while (gtk_text_iter_forward_line (&search));

  g_strfreev ((gchar**)lines);

  return retval;
}

/*************************************************************
 * ERROR REPORTING CODE
 ************************************************************/

#define MAX_URI_IN_DIALOG_LENGTH 50

void
gedit_utils_error_reporting_loading_file (
		const gchar *uri,
		GError *error,
		GtkWindow *parent)
{
	gchar *scheme_string;
	gchar *error_message;
	gchar *full_formatted_uri;
       	gchar *uri_for_display	;
	
	GnomeVFSURI *vfs_uri;
	
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

	switch (error->code)
	{
		case GNOME_VFS_ERROR_NOT_FOUND:
			error_message = g_strdup_printf (
                        	_("Could not find the file \"%s\".\n\n"
			   	  "Please, check that you typed the location correctly and try again."),
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
				   "Please, check that you typed the location correctly and try again."),
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
				  "Please, check that you typed the location correctly and try again."),
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
				gchar *host_name;
				vfs_uri = gnome_vfs_uri_new (uri);
				
				if (vfs_uri == NULL)
					host_name = g_strdup ("XXX");
				else
				{
					host_name = eel_make_valid_utf8 (
							gnome_vfs_uri_get_host_name (vfs_uri));

					gnome_vfs_uri_unref (vfs_uri);		
				}
				
                		error_message = g_strdup_printf (
					_("Could not open the file \"%s\" because no host \"%s\" " 
				  	  "could be found.\n\n"
                		  	  "Please, check that you typed the location correctly "
				  	  "and that your proxy settings are correct and then "
				  	  "try again."),
					  uri_for_display,
					  host_name);

				g_free (host_name);
			}

			break;

		case GNOME_VFS_ERROR_INVALID_HOST_NAME:
			error_message = g_strdup_printf (
                        	_("Could not open the file \"%s\" because the host name was invalid.\n\n"
				   "Please, check that you typed the location correctly and try again."),
                         	uri_for_display);
                	break;

		case GNOME_VFS_ERROR_HOST_HAS_NO_ADDRESS:
			error_message = g_strdup_printf (
				_("Could not open the file \"%s\" because the host name was empty.\n\n"
				  "Please, check that your proxy settings are correct and try again."),
				uri_for_display);
			break;
		
		case GNOME_VFS_ERROR_LOGIN_FAILED:
			error_message = g_strdup_printf (
				_("Could not open the file \"%s\" because the attempt to "
				  "log in failed.\n\n"
				  "Please, check that you typed the location correctly and try again."),
				uri_for_display);		
			break;

		case GEDIT_ERROR_INVALID_UTF8_DATA:
			error_message = g_strdup_printf (
                        	_("Could not open the file \"%s\" because "
			   	  "it contains invalid UTF-8 data.\n\n"
				  "Probably, you are trying to open a binary file."),
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
		default:
			error_message = g_strdup_printf (
                        	_("Could not open the file \"%s\"."),
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
		GError *error,
		GtkWindow *parent)
{
	gchar *scheme_string;
	gchar *error_message;
	gchar *full_formatted_uri;
       	gchar *uri_for_display	;
	
	GnomeVFSURI *vfs_uri;
	
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
				gchar *host_name;
				vfs_uri = gnome_vfs_uri_new (uri);
				
				if (vfs_uri == NULL)
					host_name = g_strdup ("XXX");
				else
				{
					host_name = eel_make_valid_utf8 (
							gnome_vfs_uri_get_host_name (vfs_uri));

					gnome_vfs_uri_unref (vfs_uri);		
				}
				
				error_message = g_strdup_printf (
					_("Could not revert the file \"%s\" because no host \"%s\" " 
				  	  "could be found.\n\n"
                		  	  "Please, check that your proxy settings are correct and "
				  	  "try again."),
					  uri_for_display,
					  host_name);

				g_free (host_name);
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
	while (cur != end) {
		const gchar *next;
		next = g_utf8_next_char (cur);

		if (prev && (*prev == '\\')) {
			switch (*cur) {
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
					str = g_string_append (str, cur);
				break;
			}
		} else if (*cur != '\\') {
			str = g_string_append_len (str, cur, next - cur);
		}

		if (!drop_prev)
			prev = cur;
		else {
			prev = NULL;
			drop_prev = FALSE;
		}

		cur = next;
	}

	return g_string_free (str, FALSE);
}

gboolean
gedit_utils_create_empty_file (const gchar *uri)
{
	gchar *canonical_uri;
	gchar *filename;
	int fd;
	
	g_return_val_if_fail (uri != NULL, FALSE);
	
	/* Get filename from uri */
	if (!gedit_utils_uri_has_file_scheme (uri))
		return FALSE;
			
	canonical_uri = eel_make_uri_canonical (uri);
	g_return_val_if_fail (canonical_uri != NULL, FALSE);
	
	gedit_debug (DEBUG_FILE, "CANONICAL URI: %s", canonical_uri);

	filename = gnome_vfs_get_local_path_from_uri (canonical_uri);
	g_free (canonical_uri);
	
	if (filename == NULL)
	{
		gedit_debug (DEBUG_FILE, "FILENAME: NULL");

		return FALSE;
	}
	
	fd = open (filename, O_CREAT | O_EXCL | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

	g_free (filename);
	
	if (fd == -1)
		return FALSE;
	
	return (close (fd) == 0);
}




#define GEDIT_STDIN_BUFSIZE 1024

gchar *
gedit_utils_get_stdin (void)
{
	GString * file_contents;
	gchar *tmp_buf = NULL;
	struct stat stats;
	guint buffer_length;
	GnomeVFSResult	res;
	
	fstat (STDIN_FILENO, &stats);
	
	if (stats.st_size  == 0)
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



static gchar *
gedit_utils_convert_to_utf8_from_charset (const gchar *content,
					  gsize len,
					  const gchar *charset)
{
	gchar *utf8_content = NULL;
	GError *conv_error = NULL;
	gchar* converted_contents = NULL;
	gsize bytes_written;

	g_return_val_if_fail (content != NULL, NULL);

	gedit_debug (DEBUG_UTILS, "Trying to convert from %s to UTF-8", charset);
			
	converted_contents = g_convert (content, len, "UTF-8",
					charset, NULL, &bytes_written,
					&conv_error); 
						
	if ((conv_error != NULL) || 
	    !g_utf8_validate (converted_contents, bytes_written, NULL))		
	{
		gedit_debug (DEBUG_UTILS, "Couldn't convert from %s to UTF-8.",
			     charset);
				
		if (converted_contents != NULL)
			g_free (converted_contents);

		if (conv_error != NULL)
		{
			g_error_free (conv_error);
			conv_error = NULL;
		}

		utf8_content = NULL;
	} else {
		gedit_debug (DEBUG_UTILS,
			"Converted from %s to UTF-8.",
			charset);
		utf8_content = converted_contents;
	}

	return utf8_content;
}

gchar *
gedit_utils_convert_to_utf8 (const gchar *content, gsize len,
			     gchar **encoding_used)
{
	GSList *encodings = NULL;
	GSList *start;
	const gchar *locale_charset;

	if (g_utf8_validate (content, len, NULL))
		return g_strdup (content);

	encodings = gedit_prefs_manager_get_encodings ();

	if (g_get_charset (&locale_charset) == FALSE) {
		const GeditEncoding *locale_encoding;

		/* not using a UTF-8 locale, so try converting
		 * from that first */
		if (locale_charset != NULL)
		{
			locale_encoding = gedit_encoding_get_from_charset (locale_charset);
			encodings = g_slist_prepend (encodings,
						(gpointer)locale_encoding);
		}
	}

	start = encodings;

	while (encodings != NULL) {
		GeditEncoding *enc;
		const gchar *charset;
		gchar *utf8_content;

		enc = (GeditEncoding *)encodings->data;

		charset = gedit_encoding_get_charset (enc);

		gedit_debug (DEBUG_UTILS, "Trying to convert %ld bytes of data into UTF-8.", len);
		fflush (stdout);
		utf8_content = gedit_utils_convert_to_utf8_from_charset
							(content, len, charset);

		if (utf8_content != NULL) {
			*encoding_used = g_strdup (charset);
			return utf8_content;
		}

		encodings = encodings->next;
	}

	g_slist_free (start);
	
	return NULL;
}
