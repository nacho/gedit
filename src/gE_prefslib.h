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


struct _gE_pref {
	char *name;
	char *value;
};

typedef struct _gE_pref gE_pref;

extern GList *gE_prefs;

char *gE_prefs_open_file (char *filename, char *rw);
int gE_prefs_open ();
void gE_prefs_close ();
char *gE_prefs_get_data (char *name);
char *gE_prefs_get_default (char *name);
char *gE_prefs_get_char (char *name);
void gE_prefs_set_data (char *name, char *value);
void gE_prefs_set_char (char *name, char *value);
int gE_prefs_get_int (char *name);
void gE_prefs_set_int (char *name, int value);
