/* vi:set ts=8 sts=0 sw=8:
 *
 * gEdit
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
#ifndef __DIALOG_H__
#define __DIALOG_H__

/*
 * this assumes that the GNOME_STOCK_BUTTON_* defs are strings.  obviously,
 * will need to be updated if GNOME_STOCK_BUTTON_* changes type.
 */
#ifdef WITHOUT_GNOME
#define GE_BUTTON_OK     "Ok"
#define GE_BUTTON_CANCEL "Cancel"
#define GE_BUTTON_YES    "Yes"
#define GE_BUTTON_NO     "No"
#define GE_BUTTON_CLOSE  "Close"
#define GE_BUTTON_APPLY  "Apply"
#define GE_BUTTON_HELP   "Help"
#else
#define GE_BUTTON_OK     GNOME_STOCK_BUTTON_OK
#define GE_BUTTON_CANCEL GNOME_STOCK_BUTTON_CANCEL
#define GE_BUTTON_YES    GNOME_STOCK_BUTTON_YES
#define GE_BUTTON_NO     GNOME_STOCK_BUTTON_NO
#define GE_BUTTON_CLOSE  GNOME_STOCK_BUTTON_CLOSE
#define GE_BUTTON_APPLY  GNOME_STOCK_BUTTON_APPLY
#define GE_BUTTON_HELP   GNOME_STOCK_BUTTON_HELP
#endif

extern int ge_dialog(char *title, char *msg, short num, char **buttons,
	short dflt, GtkSignalFunc **cbs, void **cbd, gboolean blocking);

#endif	/* __DIALOG_H__ */
