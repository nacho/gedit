/* gEdit
 * Copyright (C) 1998 Alex Roberts and Evan Lawrence
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
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


struct _gedit_pref {
	char *name;
	char *value;
};

typedef struct _gedit_pref gE_pref;

extern GList *gedit_prefs;

char *gedit_prefs_open_file (char *filename, char *rw);
int gedit_prefs_open ();
void gedit_prefs_close ();
char *gedit_prefs_get_data (char *name);
char *gedit_prefs_get_default (char *name);
char *gedit_prefs_get_char (char *name);
void gedit_prefs_set_data (char *name, char *value);
void gedit_prefs_set_char (char *name, char *value);
int gedit_prefs_get_int (char *name);
void gedit_prefs_set_int (char *name, int value);
