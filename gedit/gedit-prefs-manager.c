/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-prefs-manager.c
 * This file is part of gedit
 *
 * Copyright (C) 2002  Paolo Maggi 
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
 * Modified by the gedit Team, 2002. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include <gconf/gconf-value.h>

#include <libgnome/gnome-i18n.h>

#include "gedit-prefs-manager.h"
#include "gedit-prefs-manager-private.h"

#include "gedit-debug.h"

#include "gedit-encodings.h"

#define DEFINE_BOOL_PREF(name, key, def) gboolean 			\
gedit_prefs_manager_get_ ## name (void)					\
{									\
	gedit_debug (DEBUG_PREFS, "");					\
									\
	return gedit_prefs_manager_get_bool (key,	\
					     (def));			\
}									\
									\
void 									\
gedit_prefs_manager_set_ ## name (gboolean v)				\
{									\
	gedit_debug (DEBUG_PREFS, "");					\
									\
	gedit_prefs_manager_set_bool (key,		\
				      v);				\
}									\
				      					\
gboolean								\
gedit_prefs_manager_ ## name ## _can_set (void)				\
{									\
	gedit_debug (DEBUG_PREFS, "");					\
									\
	return gedit_prefs_manager_key_is_writable (key);\
}	



#define DEFINE_INT_PREF(name, key, def) gint	 			\
gedit_prefs_manager_get_ ## name (void)			 		\
{									\
	gedit_debug (DEBUG_PREFS, "");					\
									\
	return gedit_prefs_manager_get_int (key,		\
					    (def));			\
}									\
									\
void 									\
gedit_prefs_manager_set_ ## name (gint v)				\
{									\
	gedit_debug (DEBUG_PREFS, "");					\
									\
	gedit_prefs_manager_set_int (key,		\
				     v);				\
}									\
				      					\
gboolean								\
gedit_prefs_manager_ ## name ## _can_set (void)				\
{									\
	gedit_debug (DEBUG_PREFS, "");					\
									\
	return gedit_prefs_manager_key_is_writable (key);\
}		



#define DEFINE_STRING_PREF(name, key, def) gchar*	 		\
gedit_prefs_manager_get_ ## name (void)			 		\
{									\
	gedit_debug (DEBUG_PREFS, "");					\
									\
	return gedit_prefs_manager_get_string (key,	\
					       def);			\
}									\
									\
void 									\
gedit_prefs_manager_set_ ## name (const gchar* v)			\
{									\
	gedit_debug (DEBUG_PREFS, "");					\
									\
	gedit_prefs_manager_set_string (key,		\
				        v);				\
}									\
				      					\
gboolean								\
gedit_prefs_manager_ ## name ## _can_set (void)				\
{									\
	gedit_debug (DEBUG_PREFS, "");					\
									\
	return gedit_prefs_manager_key_is_writable (key);\
}		


GeditPrefsManager *gedit_prefs_manager = NULL;


static GtkWrapMode 	 get_wrap_mode_from_string 		(const gchar* str);

static gboolean 	 gconf_client_get_bool_with_default 	(GConfClient* client, 
								 const gchar* key, 
								 gboolean def, 
								 GError** err);

static gchar		*gconf_client_get_string_with_default 	(GConfClient* client, 
								 const gchar* key,
								 const gchar* def, 
								 GError** err);

static gint		 gconf_client_get_int_with_default 	(GConfClient* client, 
								 const gchar* key,
								 gint def, 
								 GError** err);

static gboolean		 gedit_prefs_manager_get_bool		(const gchar* key, 
								 gboolean def);

static gint		 gedit_prefs_manager_get_int		(const gchar* key, 
								 gint def);

static gchar		*gedit_prefs_manager_get_string		(const gchar* key, 
								 const gchar* def);

gboolean		 gconf_client_set_color 		(GConfClient* client, 
								 const gchar* key,
								 GdkColor val, 
								 GError** err);


gboolean
gedit_prefs_manager_init (void)
{
	gedit_debug (DEBUG_PREFS, "");

	if (gedit_prefs_manager == NULL)
	{
		GConfClient *gconf_client;

		gconf_client = gconf_client_get_default ();
		if (gconf_client == NULL)
		{
			g_warning (_("Cannot initialize preferences manager."));
			return FALSE;
		}
		
		gedit_prefs_manager = g_new0 (GeditPrefsManager, 1);

		gedit_prefs_manager->gconf_client = gconf_client;
	}

	if (gedit_prefs_manager->gconf_client == NULL)
	{
		g_free (gedit_prefs_manager);
		gedit_prefs_manager = NULL;
	}

	return gedit_prefs_manager != NULL;
	
}

void
gedit_prefs_manager_shutdown ()
{
	gedit_debug (DEBUG_PREFS, "");

	g_return_if_fail (gedit_prefs_manager != NULL);

	g_object_unref (gedit_prefs_manager->gconf_client);
	gedit_prefs_manager->gconf_client = NULL;
}

static gboolean		 
gedit_prefs_manager_get_bool (const gchar* key, gboolean def)
{
	gedit_debug (DEBUG_PREFS, "");

	g_return_val_if_fail (gedit_prefs_manager != NULL, def);
	g_return_val_if_fail (gedit_prefs_manager->gconf_client != NULL, def);

	return gconf_client_get_bool_with_default (gedit_prefs_manager->gconf_client,
						   key,
						   def,
						   NULL);
}	

static gint 
gedit_prefs_manager_get_int (const gchar* key, gint def)
{
	gedit_debug (DEBUG_PREFS, "");

	g_return_val_if_fail (gedit_prefs_manager != NULL, def);
	g_return_val_if_fail (gedit_prefs_manager->gconf_client != NULL, def);

	return gconf_client_get_int_with_default (gedit_prefs_manager->gconf_client,
						  key,
						  def,
						  NULL);
}	

static gchar *
gedit_prefs_manager_get_string (const gchar* key, const gchar* def)
{
	gedit_debug (DEBUG_PREFS, "");

	g_return_val_if_fail (gedit_prefs_manager != NULL, 
			      def ? g_strdup (def) : NULL);
	g_return_val_if_fail (gedit_prefs_manager->gconf_client != NULL, 
			      def ? g_strdup (def) : NULL);

	return gconf_client_get_string_with_default (gedit_prefs_manager->gconf_client,
						     key,
						     def,
						     NULL);
}	

static void		 
gedit_prefs_manager_set_bool (const gchar* key, gboolean value)
{
	gedit_debug (DEBUG_PREFS, "");

	g_return_if_fail (gedit_prefs_manager != NULL);
	g_return_if_fail (gedit_prefs_manager->gconf_client != NULL);
	g_return_if_fail (gconf_client_key_is_writable (
				gedit_prefs_manager->gconf_client, key, NULL));
			
	gconf_client_set_bool (gedit_prefs_manager->gconf_client, key, value, NULL);
}

static void		 
gedit_prefs_manager_set_int (const gchar* key, gint value)
{
	gedit_debug (DEBUG_PREFS, "");

	g_return_if_fail (gedit_prefs_manager != NULL);
	g_return_if_fail (gedit_prefs_manager->gconf_client != NULL);
	g_return_if_fail (gconf_client_key_is_writable (
				gedit_prefs_manager->gconf_client, key, NULL));
			
	gconf_client_set_int (gedit_prefs_manager->gconf_client, key, value, NULL);
}

static void		 
gedit_prefs_manager_set_string (const gchar* key, const gchar* value)
{
	gedit_debug (DEBUG_PREFS, "");

	g_return_if_fail (value != NULL);
	
	g_return_if_fail (gedit_prefs_manager != NULL);
	g_return_if_fail (gedit_prefs_manager->gconf_client != NULL);
	g_return_if_fail (gconf_client_key_is_writable (
				gedit_prefs_manager->gconf_client, key, NULL));
			
	gconf_client_set_string (gedit_prefs_manager->gconf_client, key, value, NULL);
}

static gboolean 
gedit_prefs_manager_key_is_writable (const gchar* key)
{
	gedit_debug (DEBUG_PREFS, "");

	g_return_val_if_fail (gedit_prefs_manager != NULL, FALSE);
	g_return_val_if_fail (gedit_prefs_manager->gconf_client != NULL, FALSE);

	return gconf_client_key_is_writable (gedit_prefs_manager->gconf_client, key, NULL);
}

static gchar* 
gdk_color_to_string (GdkColor color)
{
	gedit_debug (DEBUG_PREFS, "");

	return g_strdup_printf ("#%04x%04x%04x",
				color.red, 
				color.green,
				color.blue);
}

static GdkColor
gconf_client_get_color_with_default (GConfClient* client, const gchar* key,
                      		     const gchar* def, GError** err)
{
	gchar *str_color = NULL;
	GdkColor color;
	
	gedit_debug (DEBUG_PREFS, "");

      	g_return_val_if_fail (client != NULL, color);
      	g_return_val_if_fail (GCONF_IS_CLIENT (client), color);  
	g_return_val_if_fail (key != NULL, color);
	g_return_val_if_fail (def != NULL, color);

	str_color = gconf_client_get_string_with_default (client, 
							  key, 
							  def,
							  NULL);

	g_return_val_if_fail (str_color != NULL, color);

	gdk_color_parse (str_color, &color);
	g_free (str_color);
	
	return color;
}

gboolean
gconf_client_set_color (GConfClient* client, const gchar* key,
                        GdkColor val, GError** err)
{
	gchar *str_color = NULL;
	gboolean res;
	
	gedit_debug (DEBUG_PREFS, "");

	g_return_val_if_fail (client != NULL, FALSE);
	g_return_val_if_fail (GCONF_IS_CLIENT (client), FALSE);  
	g_return_val_if_fail (key != NULL, FALSE);

	str_color = gdk_color_to_string (val);
	g_return_val_if_fail (str_color != NULL, FALSE);

	res = gconf_client_set_string (client,
				       key,
				       str_color,
				       err);
	g_free (str_color);

	return res;
}

static GdkColor
gedit_prefs_manager_get_color (const gchar* key, const gchar* def)
{
	GdkColor color;

	gedit_debug (DEBUG_PREFS, "");

	if (def != NULL)
		gdk_color_parse (def, &color);

	g_return_val_if_fail (gedit_prefs_manager != NULL, color);
	g_return_val_if_fail (gedit_prefs_manager->gconf_client != NULL, color);

	return gconf_client_get_color_with_default (gedit_prefs_manager->gconf_client,
						    key,
						    def,
						    NULL);
}	

static void		 
gedit_prefs_manager_set_color (const gchar* key, GdkColor value)
{
	gedit_debug (DEBUG_PREFS, "");

	g_return_if_fail (gedit_prefs_manager != NULL);
	g_return_if_fail (gedit_prefs_manager->gconf_client != NULL);
	g_return_if_fail (gconf_client_key_is_writable (
				gedit_prefs_manager->gconf_client, key, NULL));
			
	gconf_client_set_color (gedit_prefs_manager->gconf_client, key, value, NULL);
}


/* Use default font */
DEFINE_BOOL_PREF (use_default_font,
		  GPM_USE_DEFAULT_FONT,
		  GPM_DEFAULT_USE_DEFAULT_FONT)

/* Editor font */
DEFINE_STRING_PREF (editor_font,
		    GPM_EDITOR_FONT,
		    GPM_DEFAULT_EDITOR_FONT);


/* Use default colors */
DEFINE_BOOL_PREF (use_default_colors,
		  GPM_USE_DEFAULT_COLORS,
		  GPM_DEFAULT_USE_DEFAULT_COLORS);


/* Background color */	
GdkColor
gedit_prefs_manager_get_background_color (void)
{
	gedit_debug (DEBUG_PREFS, "");

	return gedit_prefs_manager_get_color (GPM_BACKGROUND_COLOR,
					      GPM_DEFAULT_BACKGROUND_COLOR);
}

void
gedit_prefs_manager_set_background_color (GdkColor color)
{
	gedit_debug (DEBUG_PREFS, "");
	
	gedit_prefs_manager_set_color (GPM_BACKGROUND_COLOR,
				       color);
}

gboolean
gedit_prefs_manager_background_color_can_set (void)
{
	gedit_debug (DEBUG_PREFS, "");
	
	return gedit_prefs_manager_key_is_writable (GPM_BACKGROUND_COLOR);
}
	
/* Text color */	
GdkColor
gedit_prefs_manager_get_text_color (void)
{
	gedit_debug (DEBUG_PREFS, "");

	return gedit_prefs_manager_get_color (GPM_TEXT_COLOR,
					      GPM_DEFAULT_TEXT_COLOR);
}

void
gedit_prefs_manager_set_text_color (GdkColor color)
{
	gedit_debug (DEBUG_PREFS, "");
	
	gedit_prefs_manager_set_color (GPM_TEXT_COLOR,
				       color);
}

gboolean
gedit_prefs_manager_text_color_can_set (void)
{
	gedit_debug (DEBUG_PREFS, "");
	
	return gedit_prefs_manager_key_is_writable (GPM_TEXT_COLOR);
}

/* Selected text color */	
GdkColor
gedit_prefs_manager_get_selected_text_color (void)
{
	gedit_debug (DEBUG_PREFS, "");

	return gedit_prefs_manager_get_color (GPM_SELECTED_TEXT_COLOR,
					      GPM_DEFAULT_SELECTED_TEXT_COLOR);
}

void
gedit_prefs_manager_set_selected_text_color (GdkColor color)
{
	gedit_debug (DEBUG_PREFS, "");
	
	gedit_prefs_manager_set_color (GPM_SELECTED_TEXT_COLOR,
				       color);
}

gboolean
gedit_prefs_manager_selected_text_color_can_set (void)
{
	gedit_debug (DEBUG_PREFS, "");
	
	return gedit_prefs_manager_key_is_writable (GPM_SELECTED_TEXT_COLOR);
}

/* Selection color */	
GdkColor
gedit_prefs_manager_get_selection_color (void)
{
	gedit_debug (DEBUG_PREFS, "");

	return gedit_prefs_manager_get_color (GPM_SELECTION_COLOR,
					      GPM_DEFAULT_SELECTION_COLOR);
}

void
gedit_prefs_manager_set_selection_color (GdkColor color)
{
	gedit_debug (DEBUG_PREFS, "");
	
	gedit_prefs_manager_set_color (GPM_SELECTION_COLOR,
				       color);
}

gboolean
gedit_prefs_manager_selection_color_can_set (void)
{
	gedit_debug (DEBUG_PREFS, "");
	
	return gedit_prefs_manager_key_is_writable (GPM_SELECTION_COLOR);
}


/* Create backup copy */
DEFINE_BOOL_PREF (create_backup_copy,
		  GPM_CREATE_BACKUP_COPY,
		  GPM_DEFAULT_CREATE_BACKUP_COPY);

/* Backup extension. This is configurable only using gconftool or gconf-editor */
gchar*	 		\
gedit_prefs_manager_get_backup_extension (void)
{
	gedit_debug (DEBUG_PREFS, "");

	return gedit_prefs_manager_get_string (GPM_BACKUP_COPY_EXTENSION,	
					       GPM_DEFAULT_BACKUP_COPY_EXTENSION);
}

/* Auto save */
DEFINE_BOOL_PREF (auto_save,
		  GPM_AUTO_SAVE,
		  GPM_DEFAULT_AUTO_SAVE);

/* Auto save interval */
DEFINE_INT_PREF (auto_save_interval,
		 GPM_AUTO_SAVE_INTERVAL,
		 GPM_DEFAULT_AUTO_SAVE_INTERVAL);


/* Save encoding */
GeditSaveEncodingSetting 
gedit_prefs_manager_get_save_encoding (void)
{
	gchar *str;
	GeditSaveEncodingSetting res;
	
	gedit_debug (DEBUG_PREFS, "");
	
	str = gedit_prefs_manager_get_string (GPM_SAVE_ENCODING,
					      GPM_DEFAULT_SAVE_ENCODING);

	if (strcmp (str, "GEDIT_SAVE_CURRENT_LOCALE_IF_POSSIBLE") == 0)
		res = GEDIT_SAVE_CURRENT_LOCALE_IF_POSSIBLE;
	else
	{
		if (strcmp (str, "GEDIT_SAVE_ORIGINAL_FILE_ENCODING_IF_POSSIBLE") == 0)
			res = GEDIT_SAVE_ORIGINAL_FILE_ENCODING_IF_POSSIBLE;
		else
		{
			if (strcmp (str, "GEDIT_SAVE_ORIGINAL_FILE_ENCODING_IF_POSSIBLE_NCL") == 0)
				res = GEDIT_SAVE_ORIGINAL_FILE_ENCODING_IF_POSSIBLE_NCL;
			else	
				res = GEDIT_SAVE_ALWAYS_UTF8;
		}
	}

	g_free (str);

	return res;
}


void
gedit_prefs_manager_set_save_encoding (GeditSaveEncodingSetting se)
{
	const gchar * str;
	
	gedit_debug (DEBUG_PREFS, "");

	switch (se)
	{
		case GEDIT_SAVE_CURRENT_LOCALE_IF_POSSIBLE:
			str = "GEDIT_SAVE_CURRENT_LOCALE_IF_POSSIBLE";
			break;

		case GEDIT_SAVE_ORIGINAL_FILE_ENCODING_IF_POSSIBLE:
			str = "GEDIT_SAVE_ORIGINAL_FILE_ENCODING_IF_POSSIBLE";
			break;

		case GEDIT_SAVE_ORIGINAL_FILE_ENCODING_IF_POSSIBLE_NCL:
			str = "GEDIT_SAVE_ORIGINAL_FILE_ENCODING_IF_POSSIBLE_NCL";
			break;

		default: /* GEDIT_SAVE_ALWAYS_UTF8 */
			str = "GEDIT_SAVE_ALWAYS_UTF8";
	}

	gedit_prefs_manager_set_string (GPM_SAVE_ENCODING,
					str);
}

gboolean
gedit_prefs_manager_save_encoding_can_set (void)
{
	gedit_debug (DEBUG_PREFS, "");
	
	return gedit_prefs_manager_key_is_writable (GPM_SAVE_ENCODING);
}

/* Undo actions limit: if < 1 then no limits */
DEFINE_INT_PREF (undo_actions_limit,
		 GPM_UNDO_ACTIONS_LIMIT,
		 GPM_DEFAULT_UNDO_ACTIONS_LIMIT)

static GtkWrapMode 
get_wrap_mode_from_string (const gchar* str)
{
	GtkWrapMode res;

	g_return_val_if_fail (str != NULL, GTK_WRAP_WORD);
	
	if (strcmp (str, "GTK_WRAP_NONE") == 0)
		res = GTK_WRAP_NONE;
	else
	{
		if (strcmp (str, "GTK_WRAP_CHAR") == 0)
			res = GTK_WRAP_CHAR;
		else
			res = GTK_WRAP_WORD;
	}

	return res;
}

/* Wrap mode */
GtkWrapMode
gedit_prefs_manager_get_wrap_mode (void)
{
	gchar *str;
	GtkWrapMode res;
	
	gedit_debug (DEBUG_PREFS, "");
	
	str = gedit_prefs_manager_get_string (GPM_WRAP_MODE,
					      GPM_DEFAULT_WRAP_MODE);

	res = get_wrap_mode_from_string (str);

	g_free (str);

	return res;
}
	
void
gedit_prefs_manager_set_wrap_mode (GtkWrapMode wp)
{
	const gchar * str;
	
	gedit_debug (DEBUG_PREFS, "");

	switch (wp)
	{
		case GTK_WRAP_NONE:
			str = "GTK_WRAP_NONE";
			break;

		case GTK_WRAP_CHAR:
			str = "GTK_WRAP_CHAR";
			break;

		default: /* GTK_WRAP_WORD */
			str = "GTK_WRAP_WORD";
	}

	gedit_prefs_manager_set_string (GPM_WRAP_MODE,
					str);
}

	
gboolean
gedit_prefs_manager_wrap_mode_can_set (void)
{
	gedit_debug (DEBUG_PREFS, "");
	
	return gedit_prefs_manager_key_is_writable (GPM_WRAP_MODE);
}


/* Tabs size */
DEFINE_INT_PREF (tabs_size, 
		 GPM_TABS_SIZE, 
		 GPM_DEFAULT_TABS_SIZE);

/* Insert spaces */
DEFINE_BOOL_PREF (insert_spaces, 
		  GPM_INSERT_SPACES, 
		  GPM_DEFAULT_INSERT_SPACES)

/* Auto indent */
DEFINE_BOOL_PREF (auto_indent, 
		  GPM_AUTO_INDENT, 
		  GPM_DEFAULT_AUTO_INDENT)

/* Display line numbers */
DEFINE_BOOL_PREF (display_line_numbers, 
		  GPM_DISPLAY_LINE_NUMBERS, 
		  GPM_DEFAULT_DISPLAY_LINE_NUMBERS)

/* Toolbar visibility */
DEFINE_BOOL_PREF (toolbar_visible,
		  GPM_TOOLBAR_VISIBLE,
		  GPM_DEFAULT_TOOLBAR_VISIBLE);


/* Toolbar suttons style */
GeditToolbarSetting 
gedit_prefs_manager_get_toolbar_buttons_style (void) 
{
	gchar *str;
	GeditToolbarSetting res;
	
	gedit_debug (DEBUG_PREFS, "");
	
	str = gedit_prefs_manager_get_string (GPM_TOOLBAR_BUTTONS_STYLE,
					      GPM_DEFAULT_TOOLBAR_BUTTONS_STYLE);

	if (strcmp (str, "GEDIT_TOOLBAR_ICONS") == 0)
		res = GEDIT_TOOLBAR_ICONS;
	else
	{
		if (strcmp (str, "GEDIT_TOOLBAR_ICONS_AND_TEXT") == 0)
			res = GEDIT_TOOLBAR_ICONS_AND_TEXT;
		else 
		{
			if (strcmp (str, "GEDIT_TOOLBAR_ICONS_BOTH_HORIZ") == 0)
				res = GEDIT_TOOLBAR_ICONS_BOTH_HORIZ;
			else
				res = GEDIT_TOOLBAR_SYSTEM;
		}
	}

	g_free (str);

	return res;

}

void
gedit_prefs_manager_set_toolbar_buttons_style (GeditToolbarSetting tbs)
{
	const gchar * str;
	
	gedit_debug (DEBUG_PREFS, "");

	switch (tbs)
	{
		case GEDIT_TOOLBAR_ICONS:
			str = "GEDIT_TOOLBAR_ICONS";
			break;

		case GEDIT_TOOLBAR_ICONS_AND_TEXT:
			str = "GEDIT_TOOLBAR_ICONS_AND_TEXT";
			break;

	        case GEDIT_TOOLBAR_ICONS_BOTH_HORIZ:
			str = "GEDIT_TOOLBAR_ICONS_BOTH_HORIZ";
			break;
		default: /* GEDIT_TOOLBAR_SYSTEM */
			str = "GEDIT_TOOLBAR_SYSTEM";
	}

	gedit_prefs_manager_set_string (GPM_TOOLBAR_BUTTONS_STYLE,
					str);

}

gboolean
gedit_prefs_manager_toolbar_buttons_style_can_set (void)
{
	gedit_debug (DEBUG_PREFS, "");
	
	return gedit_prefs_manager_key_is_writable (GPM_TOOLBAR_BUTTONS_STYLE);

}

/* Statusbar visiblity */
DEFINE_BOOL_PREF (statusbar_visible,
		  GPM_STATUSBAR_VISIBLE,
		  GPM_DEFAULT_STATUSBAR_VISIBLE)

/* Print header */
DEFINE_BOOL_PREF (print_header,
		  GPM_PRINT_HEADER,
		  GPM_DEFAULT_PRINT_HEADER)


/* Print Wrap mode */
GtkWrapMode
gedit_prefs_manager_get_print_wrap_mode (void)
{
	gchar *str;
	GtkWrapMode res;
	
	gedit_debug (DEBUG_PREFS, "");
	
	str = gedit_prefs_manager_get_string (GPM_PRINT_WRAP_MODE,
					      GPM_DEFAULT_PRINT_WRAP_MODE);

	if (strcmp (str, "GTK_WRAP_NONE") == 0)
		res = GTK_WRAP_NONE;
	else
	{
		if (strcmp (str, "GTK_WRAP_WORD") == 0)
			res = GTK_WRAP_WORD;
		else
			res = GTK_WRAP_CHAR;
	}

	g_free (str);

	return res;
}
	
void
gedit_prefs_manager_set_print_wrap_mode (GtkWrapMode pwp)
{
	const gchar * str;
	
	gedit_debug (DEBUG_PREFS, "");

	switch (pwp)
	{
		case GTK_WRAP_NONE:
			str = "GTK_WRAP_NONE";
			break;

		case GTK_WRAP_WORD:
			str = "GTK_WRAP_WORD";
			break;

		default: /* GTK_WRAP_CHAR */
			str = "GTK_WRAP_CHAR";
	}

	gedit_prefs_manager_set_string (GPM_PRINT_WRAP_MODE,
					str);
}

	
gboolean
gedit_prefs_manager_print_wrap_mode_can_set (void)
{
	gedit_debug (DEBUG_PREFS, "");
	
	return gedit_prefs_manager_key_is_writable (GPM_PRINT_WRAP_MODE);
}

/* Print line numbers */	
DEFINE_INT_PREF (print_line_numbers,
		 GPM_PRINT_LINE_NUMBERS,
		 GPM_DEFAULT_PRINT_LINE_NUMBERS)
		 

/* Font used to print the body of documents */
DEFINE_STRING_PREF (print_font_body,
		    GPM_PRINT_FONT_BODY,
		    GPM_DEFAULT_PRINT_FONT_BODY);

const gchar *
gedit_prefs_manager_get_default_print_font_body (void)
{
	return GPM_DEFAULT_PRINT_FONT_BODY;
}

/* Font used to print headers */
DEFINE_STRING_PREF (print_font_header,
		    GPM_PRINT_FONT_HEADER,
		    GPM_DEFAULT_PRINT_FONT_HEADER);

const gchar *
gedit_prefs_manager_get_default_print_font_header (void)
{
	return GPM_DEFAULT_PRINT_FONT_HEADER;
}

/* Font used to print line numbers */
DEFINE_STRING_PREF (print_font_numbers,
		    GPM_PRINT_FONT_NUMBERS,
		    GPM_DEFAULT_PRINT_FONT_NUMBERS);

const gchar *
gedit_prefs_manager_get_default_print_font_numbers (void)
{
	return GPM_DEFAULT_PRINT_FONT_NUMBERS;
}


/* Max number of files in "Recent Files" menu. 
 * This is configurable only using gconftool or gconf-editor 
 */
gint
gedit_prefs_manager_get_max_recents (void)
{
	gedit_debug (DEBUG_PREFS, "");

	return gedit_prefs_manager_get_int (GPM_MAX_RECENTS,	
					    GPM_DEFAULT_MAX_RECENTS);

}

/* Encodings */
GSList *
gedit_prefs_manager_get_encodings (void)
{
	GSList *strings;
	GSList *res = NULL;
	gedit_debug (DEBUG_PREFS, "");

	g_return_val_if_fail (gedit_prefs_manager != NULL, NULL);
	g_return_val_if_fail (gedit_prefs_manager->gconf_client != NULL, NULL);

	strings = gconf_client_get_list (gedit_prefs_manager->gconf_client,
				GPM_ENCODINGS,
				GCONF_VALUE_STRING, 
				NULL);

	if (strings != NULL)
	{	
		GSList *tmp;
		const GeditEncoding *enc;

		tmp = strings;
		
		while (tmp)
		{
		      const char *charset = tmp->data;

		      if (strcmp (charset, "current") == 0)
			      g_get_charset (&charset);
      
		      g_return_val_if_fail (charset != NULL, NULL);
		      enc = gedit_encoding_get_from_charset (charset);
		      
		      if (enc != NULL)
				res = g_slist_prepend (res, (gpointer)enc);

		      tmp = g_slist_next (tmp);
		}

		g_slist_foreach (strings, (GFunc) g_free, NULL);
		g_slist_free (strings);    

	 	res = g_slist_reverse (res);
	}

	return res;
}

void
gedit_prefs_manager_set_encodings (const GSList *encs)
{	
	GSList *list = NULL;
	
	g_return_if_fail (gedit_prefs_manager != NULL);
	g_return_if_fail (gedit_prefs_manager->gconf_client != NULL);
	g_return_if_fail (gedit_prefs_manager_encodings_can_set ());

	while (encs != NULL)
	{
		const GeditEncoding *enc;
		const gchar *charset;
		
		enc = (const GeditEncoding *)encs->data;

		charset = gedit_encoding_get_charset (enc);
		g_return_if_fail (charset != NULL);

		list = g_slist_prepend (list, (gpointer)charset);

		encs = g_slist_next (encs);
	}

	list = g_slist_reverse (list);
		
	gconf_client_set_list (gedit_prefs_manager->gconf_client,
			GPM_ENCODINGS,
			GCONF_VALUE_STRING,
		       	list,
			NULL);	
	
	g_slist_free (list);
}

gboolean
gedit_prefs_manager_encodings_can_set (void)
{
	gedit_debug (DEBUG_PREFS, "");
	
	return gedit_prefs_manager_key_is_writable (GPM_ENCODINGS);

}


/* The following functions are taken from gconf-client.c 
 * and partially modified. 
 * The licensing terms on these is: 
 *
 * 
 * GConf
 * Copyright (C) 1999, 2000, 2000 Red Hat Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */


static const gchar* 
gconf_value_type_to_string(GConfValueType type)
{
  switch (type)
    {
    case GCONF_VALUE_INT:
      return "int";
      break;
    case GCONF_VALUE_STRING:
      return "string";
      break;
    case GCONF_VALUE_FLOAT:
      return "float";
      break;
    case GCONF_VALUE_BOOL:
      return "bool";
      break;
    case GCONF_VALUE_SCHEMA:
      return "schema";
      break;
    case GCONF_VALUE_LIST:
      return "list";
      break;
    case GCONF_VALUE_PAIR:
      return "pair";
      break;
    case GCONF_VALUE_INVALID:
      return "*invalid*";
      break;
    default:
      g_return_val_if_fail (FALSE, NULL);
      break;
    }
}

/* Emit the proper signals for the error, and fill in err */
static gboolean
handle_error (GConfClient* client, GError* error, GError** err)
{
  if (error != NULL)
    {
      gconf_client_error(client, error);
      
      if (err == NULL)
        {
          gconf_client_unreturned_error(client, error);

          g_error_free(error);
        }
      else
        *err = error;

      return TRUE;
    }
  else
    return FALSE;
}

static gboolean
check_type (const gchar* key, GConfValue* val, GConfValueType t, GError** err)
{
  if (val->type != t)
    {
      /*
      gconf_set_error(err, GCONF_ERROR_TYPE_MISMATCH,
                      _("Expected `%s' got `%s' for key %s"),
                      gconf_value_type_to_string(t),
                      gconf_value_type_to_string(val->type),
                      key);
      */
      g_set_error (err, GCONF_ERROR, GCONF_ERROR_TYPE_MISMATCH,
	  	   _("Expected `%s' got `%s' for key %s"),
                   gconf_value_type_to_string(t),
                   gconf_value_type_to_string(val->type),
                   key);
	      
      return FALSE;
    }
  else
    return TRUE;
}

static gboolean
gconf_client_get_bool_with_default (GConfClient* client, const gchar* key,
                        	    gboolean def, GError** err)
{
  GError* error = NULL;
  GConfValue* val;

  g_return_val_if_fail (err == NULL || *err == NULL, def);

  val = gconf_client_get (client, key, &error);

  if (val != NULL)
    {
      gboolean retval = def;

      g_return_val_if_fail (error == NULL, retval);
      
      if (check_type (key, val, GCONF_VALUE_BOOL, &error))
        retval = gconf_value_get_bool (val);
      else
        handle_error (client, error, err);

      gconf_value_free (val);

      return retval;
    }
  else
    {
      if (error != NULL)
        handle_error (client, error, err);
      return def;
    }
}

static gchar*
gconf_client_get_string_with_default (GConfClient* client, const gchar* key,
                        	      const gchar* def, GError** err)
{
  GError* error = NULL;
  gchar* val;

  g_return_val_if_fail (err == NULL || *err == NULL, def ? g_strdup (def) : NULL);

  val = gconf_client_get_string (client, key, &error);

  if (val != NULL)
    {
      g_return_val_if_fail (error == NULL, def ? g_strdup (def) : NULL);
      
      return val;
    }
  else
    {
      if (error != NULL)
        *err = error;
      return def ? g_strdup (def) : NULL;
    }
}

static gint
gconf_client_get_int_with_default (GConfClient* client, const gchar* key,
                        	   gint def, GError** err)
{
  GError* error = NULL;
  GConfValue* val;

  g_return_val_if_fail (err == NULL || *err == NULL, def);

  val = gconf_client_get (client, key, &error);

  if (val != NULL)
    {
      gint retval = def;

      g_return_val_if_fail (error == NULL, def);
      
      if (check_type (key, val, GCONF_VALUE_INT, &error))
        retval = gconf_value_get_int(val);
      else
        handle_error (client, error, err);

      gconf_value_free (val);

      return retval;
    }
  else
    {
      if (error != NULL)
        handle_error (client, error, err);
      return def;
    }
}



