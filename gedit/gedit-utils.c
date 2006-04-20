/*
 * gedit-utils.c
 * This file is part of gedit
 *
 * Copyright (C) 1998, 1999 Alex Roberts, Evan Lawrence
 * Copyright (C) 2000, 2002 Chema Celorio, Paolo Maggi 
 * Copyright (C) 2003-2005 Paolo Maggi 
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
 * Modified by the gedit Team, 1998-2005. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 *
 * $Id$
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#include <gdk/gdkx.h>
#include <glib/gunicode.h>
#include <glib/gi18n.h>
#include <glade/glade-xml.h>
#include <libgnomevfs/gnome-vfs.h>
#include <libgnome/gnome-url.h>

#include "gedit-utils.h"

#include "gedit-document.h"
#include "gedit-prefs-manager.h"
#include "gedit-debug.h"
#include "gedit-convert.h"

#define STDIN_DELAY_MICROSECONDS 100000

gboolean
gedit_utils_uri_has_file_scheme (const gchar *uri)
{
	gchar *canonical_uri = NULL;
	gboolean res;

	canonical_uri = gnome_vfs_make_uri_canonical (uri);
	g_return_val_if_fail (canonical_uri != NULL, FALSE);

	res = g_str_has_prefix (canonical_uri, "file:");

	g_free (canonical_uri);

	return res;
}

gboolean
gedit_utils_uri_has_writable_scheme (const gchar *uri)
{
	gchar *canonical_uri;
	gchar *scheme;
	GSList *writable_schemes;
	gboolean res;

	canonical_uri = gnome_vfs_make_uri_canonical (uri);
	g_return_val_if_fail (canonical_uri != NULL, FALSE);

	scheme = gnome_vfs_get_uri_scheme (canonical_uri);
	g_return_val_if_fail (scheme != NULL, FALSE);

	g_free (canonical_uri);

	writable_schemes = gedit_prefs_manager_get_writable_vfs_schemes ();

	/* CHECK: should we use g_ascii_strcasecmp? - Paolo (Nov 6, 2005) */
	res = (g_slist_find_custom (writable_schemes,
				    scheme,
				    (GCompareFunc)strcmp) != NULL);

	g_slist_foreach (writable_schemes, (GFunc)g_free, NULL);
	g_slist_free (writable_schemes);

	g_free (scheme);

	return res;
}

void
gedit_utils_menu_position_under_widget (GtkMenu  *menu,
					gint     *x,
					gint     *y,
					gboolean *push_in,
					gpointer  user_data)
{
	GtkWidget *w = GTK_WIDGET (user_data);
	GtkRequisition requisition;

	gdk_window_get_origin (w->window, x, y);
	gtk_widget_size_request (GTK_WIDGET (menu), &requisition);

	if (gtk_widget_get_direction (w) == GTK_TEXT_DIR_RTL)
	{
		*x += w->allocation.x + w->allocation.width - requisition.width;
	}
	else
	{
		*x += w->allocation.x;
	}

	*y += w->allocation.y + w->allocation.height;

	*push_in = TRUE;
}

GtkWidget *
gedit_gtk_button_new_with_stock_icon (const gchar *label,
				      const gchar *stock_id)
{
	GtkWidget *button;

	button = gtk_button_new_with_mnemonic (label);
	gtk_button_set_image (GTK_BUTTON (button),
			      gtk_image_new_from_stock (stock_id,
							GTK_ICON_SIZE_BUTTON));

        return button;
}

GtkWidget *
gedit_dialog_add_button (GtkDialog   *dialog,
			 const gchar *text,
			 const gchar *stock_id,
			 gint         response_id)
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
	normalized_s1 = g_utf8_normalize (casefold, -1, G_NORMALIZE_NFD);
	g_free (casefold);

	casefold = g_utf8_casefold (s2, n2);
	normalized_s2 = g_utf8_normalize (casefold, -1, G_NORMALIZE_NFD);
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

/**
 * gedit_utils_set_atk_name_description
 * @widget : The Gtk widget for which name/description to be set
 * @name : Atk name string
 * @description : Atk description string
 * Description : This function sets up name and description
 * for a specified gtk widget.
 */
void
gedit_utils_set_atk_name_description (GtkWidget *widget, 
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

/**
 * gedit_set_atk__relation
 * @obj1,@obj2 : specified widgets.
 * @rel_type : the type of relation to set up.
 * Description : This function establishes atk relation
 * between 2 specified widgets.
 */
void
gedit_utils_set_atk_relation (GtkWidget *obj1, 
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
	
	gedit_debug_message (DEBUG_UTILS, "text_uri: %s", text_uri);

	uri = gnome_vfs_uri_new (text_uri);
	g_return_val_if_fail (uri != NULL, FALSE);

	res = gnome_vfs_uri_exists (uri);

	gnome_vfs_uri_unref (uri);

	gedit_debug_message (DEBUG_UTILS, res ? "TRUE" : "FALSE");

	return res;
}

gchar *
gedit_utils_escape_search_text (const gchar* text)
{
	GString *str;
	gint length;
	const gchar *p;
 	const gchar *end;

	if (text == NULL)
		return NULL;

	gedit_debug_message (DEBUG_SEARCH, "Text: %s", text);

    	length = strlen (text);

	str = g_string_new ("");

  	p = text;
  	end = text + length;

  	while (p != end)
    	{
      		const gchar *next;
      		next = g_utf8_next_char (p);

		switch (*p)
        	{
       			case '\n':
          			g_string_append (str, "\\n");
          			break;
			case '\r':
          			g_string_append (str, "\\r");
          			break;
			case '\t':
          			g_string_append (str, "\\t");
          			break;
        		default:
          			g_string_append_len (str, p, next - p);
          			break;
        	}

      		p = next;
    	}

	return g_string_free (str, FALSE);
}

gchar *
gedit_utils_unescape_search_text (const gchar *text)
{
	GString *str;
	gint length;
	gboolean drop_prev = FALSE;
	const gchar *cur;
	const gchar *end;
	const gchar *prev;
	
	if (text == NULL)
		return NULL;

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
gedit_warning (GtkWindow *parent, const gchar *format, ...)
{
	va_list args;
	gchar *str;
	GtkWidget *dialog;

	g_return_if_fail (format != NULL);

	va_start (args, format);
	str = g_strdup_vprintf (format, args);
	va_end (args);

	dialog = gtk_message_dialog_new_with_markup (
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

/*
 * Doubles underscore to avoid spurious menu accels.
 */
gchar * 
gedit_utils_escape_underscores (const gchar* text,
				gssize       length)
{
	GString *str;
	const gchar *p;
	const gchar *end;

	g_return_val_if_fail (text != NULL, NULL);

	if (length < 0)
		length = strlen (text);

	str = g_string_sized_new (length);

	p = text;
	end = text + length;

	while (p != end)
	{
		const gchar *next;
		next = g_utf8_next_char (p);

		switch (*p)
		{
			case '_':
				g_string_append (str, "__");
				break;
			default:
				g_string_append_len (str, p, next - p);
				break;
		}

		p = next;
	}

	return g_string_free (str, FALSE);
}

/*
 * Replaces '/' with '-' to avoid problems in xml paths.
 */
gchar *
gedit_utils_escape_slashes (const gchar *text,
			    gssize       length)
{
	GString *str;
	const gchar *p;
	const gchar *end;

	g_return_val_if_fail (text != NULL, NULL);

	if (length < 0)
		length = strlen (text);

	str = g_string_sized_new (length);

	p = text;
	end = text + length;

	while (p != end)
	{
		const gchar *next;
		next = g_utf8_next_char (p);

		if (*p == '/')
			g_string_append (str, "-");
		else
			g_string_append_len (str, p, next - p);

		p = next;
	}

	return g_string_free (str, FALSE);
}

/* the following functions are taken from eel */

gchar *
gedit_utils_str_middle_truncate (const gchar *string,
				 guint        truncate_length)
{
	GString     *truncated;
	guint        length;
	guint        n_chars;
	guint        num_left_chars;
	guint        right_offset;
	guint        delimiter_length;
	const gchar *delimiter = "\342\200\246";

	g_return_val_if_fail (string != NULL, NULL);

	length = strlen (string);

	g_return_val_if_fail (g_utf8_validate (string, length, NULL), NULL);

	/* It doesnt make sense to truncate strings to less than
	 * the size of the delimiter plus 2 characters (one on each
	 * side)
	 */
	delimiter_length = g_utf8_strlen (delimiter, -1);
	if (truncate_length < (delimiter_length + 2)) {
		return g_strdup (string);
	}

	n_chars = g_utf8_strlen (string, length);

	/* Make sure the string is not already small enough. */
	if (n_chars <= truncate_length) {
		return g_strdup (string);
	}

	/* Find the 'middle' where the truncation will occur. */
	num_left_chars = (truncate_length - delimiter_length) / 2;
	right_offset = n_chars - truncate_length + num_left_chars + delimiter_length;

	truncated = g_string_new_len (string,
				      g_utf8_offset_to_pointer (string, num_left_chars) - string);
	g_string_append (truncated, delimiter);
	g_string_append (truncated, g_utf8_offset_to_pointer (string, right_offset));
		
	return g_string_free (truncated, FALSE);
}

gchar *
gedit_utils_make_valid_utf8 (const char *name)
{
	GString *string;
	const char *remainder, *invalid;
	int remaining_bytes, valid_bytes;

	string = NULL;
	remainder = name;
	remaining_bytes = strlen (name);

	while (remaining_bytes != 0) {
		if (g_utf8_validate (remainder, remaining_bytes, &invalid)) {
			break;
		}
		valid_bytes = invalid - remainder;

		if (string == NULL) {
			string = g_string_sized_new (remaining_bytes);
		}
		g_string_append_len (string, remainder, valid_bytes);
		/* append U+FFFD REPLACEMENT CHARACTER */
		g_string_append (string, "\357\277\275");

		remaining_bytes -= valid_bytes + 1;
		remainder = invalid + 1;
	}

	if (string == NULL) {
		return g_strdup (name);
	}

	g_string_append (string, remainder);
	
	g_assert (g_utf8_validate (string->str, -1, NULL));

	return g_string_free (string, FALSE);
}

/* Note that this function replace home dir with ~ */
gchar *
gedit_utils_uri_get_dirname (const gchar *uri)
{
	gchar *res;
	gchar *str;

	// CHECK: does it work with uri chaining? - Paolo
	str = g_path_get_dirname (uri);
	g_return_val_if_fail (str != NULL, ".");

	if ((strlen (str) == 1) && (*str == '.'))
	{
		g_free (str);
		
		return NULL;
	}

	res = gedit_utils_replace_home_dir_with_tilde (str);

	g_free (str);
	
	return res;
}

gchar *
gedit_utils_replace_home_dir_with_tilde (const gchar *uri)
{
	gchar *tmp;
	gchar *home;

	g_return_val_if_fail (uri != NULL, NULL);

	/* Note that g_get_home_dir returns a const string */
	tmp = (gchar *)g_get_home_dir ();

	if (tmp == NULL)
		return g_strdup (uri);

	home = g_filename_to_utf8 (tmp, -1, NULL, NULL, NULL);
	if (home == NULL)
		return g_strdup (uri);

	if (strcmp (uri, home) == 0)
	{
		g_free (home);
		
		return g_strdup ("~");
	}

	tmp = home;
	home = g_strdup_printf ("%s/", tmp);
	g_free (tmp);

	if (g_str_has_prefix (uri, home))
	{
		gchar *res;

		res = g_strdup_printf ("~/%s", uri + strlen (home));

		g_free (home);
		
		return res;		
	}

	g_free (home);

	return g_strdup (uri);
}

/* the following two functions are courtesy of galeon */

/**
 * gedit_utils_get_current_workspace: Get the current workspace
 *
 * Get the currently visible workspace for the #GdkScreen.
 *
 * If the X11 window property isn't found, 0 (the first workspace)
 * is returned.
 */
guint
gedit_utils_get_current_workspace (GdkScreen *screen)
{
	GdkWindow *root_win;
	GdkDisplay *display;
	Atom type;
	gint format;
	gulong nitems;
	gulong bytes_after;
	guint *current_desktop;
	guint ret = 0;

	g_return_val_if_fail (GDK_IS_SCREEN (screen), 0);

	root_win = gdk_screen_get_root_window (screen);
	display = gdk_screen_get_display (screen);

	XGetWindowProperty (GDK_DISPLAY_XDISPLAY (display), GDK_WINDOW_XID (root_win),
			    gdk_x11_get_xatom_by_name_for_display (display, "_NET_CURRENT_DESKTOP"),
			    0, G_MAXLONG,
			    False, XA_CARDINAL, &type, &format, &nitems,
			    &bytes_after, (gpointer)&current_desktop);

	if (type == XA_CARDINAL && format == 32 && nitems > 0)
	{
		ret = current_desktop[0];
		XFree (current_desktop);
	}

	return ret;
}

/**
 * gedit_utils_get_window_workspace: Get the workspace the window is on
 *
 * This function gets the workspace that the #GtkWindow is visible on,
 * it returns GEDIT_ALL_WORKSPACES if the window is sticky, or if
 * the window manager doesn support this function
 */
guint
gedit_utils_get_window_workspace (GtkWindow *gtkwindow)
{
	GdkWindow *window;
	GdkDisplay *display;
	Atom type;
	gint format;
	gulong nitems;
	gulong bytes_after;
	guint *workspace;
	guint ret = GEDIT_ALL_WORKSPACES;

	g_return_val_if_fail (GTK_IS_WINDOW (gtkwindow), 0);
	g_return_val_if_fail (GTK_WIDGET_REALIZED (GTK_WIDGET (gtkwindow)), 0);

	window = GTK_WIDGET (gtkwindow)->window;
	display = gdk_drawable_get_display (window);

	XGetWindowProperty (GDK_DISPLAY_XDISPLAY (display), GDK_WINDOW_XID (window),
			    gdk_x11_get_xatom_by_name_for_display (display, "_NET_WM_DESKTOP"),
			    0, G_MAXLONG,
			    False, XA_CARDINAL, &type, &format, &nitems,
			    &bytes_after, (gpointer)&workspace);

	if (type == XA_CARDINAL && format == 32 && nitems > 0)
	{
		ret = workspace[0];
		XFree (workspace);
	}

	return ret;
}

void
gedit_utils_activate_url (GtkAboutDialog *about,
			  const gchar    *url,
			  gpointer        data)
{
	gnome_url_show (url, NULL);
}

static gboolean
is_valid_scheme_character (gchar c)
{
	return g_ascii_isalnum (c) || c == '+' || c == '-' || c == '.';
}

static gboolean
has_valid_scheme (const gchar *uri)
{
	const gchar *p;

	p = uri;

	if (!is_valid_scheme_character (*p)) {
		return FALSE;
	}

	do {
		p++;
	} while (is_valid_scheme_character (*p));

	return *p == ':';
}

gboolean
gedit_utils_is_valid_uri (const gchar *uri)
{
	const guchar *p;

	if (uri == NULL)
		return FALSE;

	if (!has_valid_scheme (uri))
		return FALSE;

	/* We expect to have a fully valid set of characters */
	for (p = (const guchar *)uri; *p; p++) {
		if (*p == '%')
		{
			++p;
			if (!g_ascii_isxdigit (*p))
				return FALSE;

			++p;		
			if (!g_ascii_isxdigit (*p))
				return FALSE;
		}
		else
		{
			if (*p <= 32 || *p >= 128)
				return FALSE;
		}
	}

	return TRUE;
}


/**
 * gedit_utils_get_glade_widgets:
 * @filename: the path to the glade file
 * @root_node: the root node in the glade file
 * @error_widget: a pointer were a #GtkLabel
 * @widget_name: the name of the first widget
 * @...: a pointer were the first widget is returned, followed by more
 *       name / widget pairs and terminated by NULL.
 *
 * This function gets the requested widgets from a glade file. In case
 * of error it returns FALSE and sets error_widget to a GtkLabel containing
 * the error message to display.
 *
 * Returns FALSE if an error occurs, TRUE on success.
 */
gboolean
gedit_utils_get_glade_widgets (const gchar *filename,
			       const gchar *root_node,
			       GtkWidget **error_widget,
			       const gchar *widget_name,
			       ...)
{
	GtkWidget *label;
	GladeXML *gui;
	va_list args;
	const gchar *name;
	gchar *msg;
	gchar *filename_markup;
	gchar *msg_plain;
	gboolean ret = TRUE;

	g_return_val_if_fail (filename != NULL, FALSE);
	g_return_val_if_fail (error_widget != NULL, FALSE);
	g_return_val_if_fail (widget_name != NULL, FALSE);

	*error_widget = NULL;

	gui = glade_xml_new (filename, root_node, NULL);
	if (!gui)
	{
		filename_markup = g_markup_printf_escaped ("<i>%s</i>", filename);
		msg_plain = g_strdup_printf (_("Unable to find file %s."),
				filename_markup);
		msg = g_strconcat ("<span size=\"large\" weight=\"bold\">",
				msg_plain, "</span>\n\n",
				_("Please check your installation."), NULL);
		label = gtk_label_new (msg);

		gtk_label_set_use_markup (GTK_LABEL (label), TRUE);

		g_free (filename_markup);
		g_free (msg_plain);
		g_free (msg);

		*error_widget = label;

		return FALSE;
	}

	va_start (args, widget_name);
	for (name = widget_name; name; name = va_arg (args, const gchar *) )
	{
		GtkWidget **wid;

		wid = va_arg (args, GtkWidget **);
		*wid = glade_xml_get_widget (gui, name);
		if (*wid == NULL)
		{
			filename_markup = g_markup_printf_escaped ("<i>%s</i>", filename);
			msg_plain = g_strdup_printf (
					_("Unable to find the required widgets inside file %s."),
					filename_markup);
			msg = g_strconcat ("<span size=\"large\" weight=\"bold\">",
					msg_plain, "</span>\n\n",
					_("Please check your installation."), NULL);
			label = gtk_label_new (msg);

			gtk_label_set_use_markup (GTK_LABEL (label), TRUE);

			g_free (filename_markup);
			g_free (msg_plain);
			g_free (msg);

			*error_widget = label;

			ret = FALSE;

			break;
		}
	}
	va_end (args);

	g_object_unref (gui);

	return ret;
}

gchar *
gedit_utils_make_canonical_uri_from_shell_arg (const gchar *str)
{	
	gchar *uri;
	gchar *canonical_uri;

	g_return_val_if_fail (str != NULL, NULL);
	g_return_val_if_fail (*str != '\0', NULL);
	
	/* Note for the future: 
	 *
	 * <federico> paolo: and flame whoever tells 
	 * you that file:///gnome/test_files/hëllò 
	 * doesn't work --- that's not a valid URI
	 *
	 * <paolo> federico: well, another solution that 
	 * does not requires patch to _from_shell_args 
	 * is to check that the string returned by it 
	 * contains only ASCII chars
	 * <federico> paolo: hmmmm, isn't there 
	 * gnome_vfs_is_uri_valid() or something?
	 * <paolo>: I will use gedit_utils_is_valid_uri ()
	 *
	 */
	 
	uri = gnome_vfs_make_uri_from_shell_arg (str);
	canonical_uri = gnome_vfs_make_uri_canonical (uri);
	g_free (uri);
	
	/* g_print ("URI: %s\n", canonical_uri); */
	
	if (gedit_utils_is_valid_uri (canonical_uri))
		return canonical_uri;
	
	return NULL;
}

/**
 * gedit_utils_format_uri_for_display:
 * @uri: uri to be displayed.
 *
 * Filter, modify, unescape and change @uri to make it appropriate
 * for display to users.
 * 
 * Rules:
 * <ul>
 * <li>file: uri without fragments should appear as local paths.</li>
 * <li>file: uri with fragments should appear as file:uri.</li>
 * <li>All other uri appear as expected.</li>
 * </ul>
 *
 * This function is very similar to gnome_vfs_format_uri_for_display but remove
 * the password from the resulting string
 *
 * Return value: a string which represents @uri and can be displayed.
 */
gchar *
gedit_utils_format_uri_for_display (const gchar *uri)
{
	GnomeVFSURI *vfs_uri;
	
	g_return_val_if_fail (uri != NULL, NULL);

	/* Note: vfs_uri may be NULL for some valid but
	 * unsupported uris */
	vfs_uri = gnome_vfs_uri_new (uri);
	
	if (vfs_uri == NULL)
	{
		/* We may disclose the password here, but there is nothing we
		 * can do since we cannot get a valid vfs_uri */
		return gnome_vfs_format_uri_for_display (uri);
	}
	else
	{	
		gchar *name;
		gchar *uri_for_display;
		
		name = gnome_vfs_uri_to_string (vfs_uri, GNOME_VFS_URI_HIDE_PASSWORD);
		g_return_val_if_fail (name != NULL, gnome_vfs_format_uri_for_display (uri));
		
		uri_for_display = gnome_vfs_format_uri_for_display (name);
		g_free (name);
		
		gnome_vfs_uri_unref (vfs_uri);
		
		return uri_for_display;
	}
}
