/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-utils.c
 * This file is part of gedit
 *
 * Copyright (C) 1998, 1999 Alex Roberts, Evan Lawrence
 * Copyright (C) 2000, 2001 Chema Celorio, Paolo Maggi 
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
 * Boston, MA 02111-1307, USA. * *
 */
 
/*
 * Modified by the gedit Team, 1998-2001. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 */

#include "gedit-utils.h"
#include "gedit2.h"

#include "bonobo-mdi.h"
#include <libgnomeui/libgnomeui.h>
#include "gnome-vfs-helpers.h"

#include <glib/gunicode.h>

/* =================================================== */
/* Flash */

struct _MessageInfo {
  BonoboWindow * win;
  guint timeoutid;
  guint handlerid;
};

typedef struct _MessageInfo MessageInfo;

static gint
remove_message_timeout (MessageInfo * mi) 
{
	BonoboUIEngine *ui_engine;

	GDK_THREADS_ENTER ();	
	
  	ui_engine = bonobo_window_get_ui_engine (mi->win);
	g_return_val_if_fail (ui_engine != NULL, FALSE);

	bonobo_ui_engine_remove_hint (ui_engine);

	g_free (mi);

  	GDK_THREADS_LEAVE ();

  	return FALSE; /* removes the timeout */
}

/* Called if the win is destroyed before the timeout occurs. */
static void
remove_timeout_cb (GtkWidget *win, MessageInfo *mi ) 
{
 	gtk_timeout_remove (mi->timeoutid);
  	g_free (mi);
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

void 
bonobo_window_flash (BonoboWindow * win, const gchar * flash)
{
	BonoboUIEngine *ui_engine;
  	g_return_if_fail (win != NULL);
  	g_return_if_fail (BONOBO_IS_WINDOW (win));
  	g_return_if_fail (flash != NULL);
  	
	ui_engine = bonobo_window_get_ui_engine (win);
	g_return_if_fail (ui_engine != NULL);
	
	if (bonobo_ui_engine_xml_node_exists (ui_engine, "/status"))
	{
    		MessageInfo * mi;

		bonobo_ui_engine_remove_hint (ui_engine);
		bonobo_ui_engine_add_hint (ui_engine, flash);
    		
		mi = g_new(MessageInfo, 1);

    		mi->timeoutid = 
      			gtk_timeout_add (flash_length,
				(GtkFunction) remove_message_timeout,
				mi);
    
    		mi->handlerid = 
      			gtk_signal_connect (GTK_OBJECT(win),
				"destroy",
			   	GTK_SIGNAL_FUNC (remove_timeout_cb),
			   	mi );

    		mi->win       = win;
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

static gboolean
istr_has_prefix (const char *haystack, const char *needle)
{
	const char *h, *n;
	char hc, nc;

	/* Eat one character at a time. */
	h = haystack == NULL ? "" : haystack;
	n = needle == NULL ? "" : needle;
	do {
		if (*n == '\0') {
			return TRUE;
		}
		if (*h == '\0') {
			return FALSE;
		}
		hc = *h++;
		nc = *n++;
		hc = g_ascii_tolower (hc);
		nc = g_ascii_tolower (nc);
	} while (hc == nc);
	return FALSE;
}

gboolean
gedit_utils_uri_has_file_scheme (const gchar *uri)
{
	gchar* canonical_uri = NULL;
	gboolean res;
	
	canonical_uri = gnome_vfs_x_make_uri_canonical (uri);
	g_return_val_if_fail (canonical_uri != NULL, FALSE);

	res = istr_has_prefix (canonical_uri, "file");
	
	g_free (canonical_uri);

	return res;
}

gboolean
gedit_utils_is_uri_read_only (const gchar* uri)
{
	gchar* file_uri = NULL;
	gint res;

	/* FIXME: all remote files are marked as readonly */
	if (!gedit_utils_uri_has_file_scheme (uri))
		return TRUE;
			
	file_uri = gnome_vfs_x_format_uri_for_display (uri);
		
	res = access (file_uri, W_OK);

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

gchar *g_utf8_strcasestr(const gchar *haystack, const gchar *needle)
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
gboolean g_uft8_caselessnmatch (const char *s1, const char *s2, gssize n)
{
	gchar *casefold;
	gchar *normalized_s1;
      	gchar *normalized_s2;
	gint len_s1;
	gint len_s2;
	gboolean ret = FALSE;
	
	g_return_val_if_fail (s1 != NULL, FALSE);
	g_return_val_if_fail (s2 != NULL, FALSE);
	g_return_val_if_fail (n > 0, FALSE);

	casefold = g_utf8_casefold (s1, n);
	normalized_s1 = g_utf8_normalize (casefold, -1, G_NORMALIZE_ALL);
	g_free (casefold);

	casefold = g_utf8_casefold (s2, n);
	normalized_s2 = g_utf8_normalize (casefold, -1, G_NORMALIZE_ALL);
	g_free (casefold);

	len_s1 = strlen (normalized_s1);
	len_s2 = strlen (normalized_s2);

	if (len_s1 != len_s2)
		goto finally_2;

	ret = (memcmp (normalized_s1, normalized_s2, len_s1) == 0);
	
finally_2:
	g_free (normalized_s1);
	g_free (normalized_s1);	

	return ret;
}

/**********************************************************
 * The following code is a modified version of the gtk one
 **********************************************************/

	
gboolean      _gtk_text_btree_char_is_invisible (const GtkTextIter *iter);
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
          _gtk_text_btree_char_is_invisible (iter))
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
      if (g_uft8_caselessnmatch (line_text, *lines, strlen (*lines)) == 0)
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

  if ((flags & GTK_TEXT_SEARCH_CASE_INSENSITIVE) != 0)
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


 
