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
#ifndef __GE_PREFS_H__
#define __GE_PREFS_H__

#include "gE_prefslib.h"

extern void gE_save_settings();
extern void gE_get_settings();
extern void gE_rc_parse(void);

typedef struct _gE_preference {

	guint auto_indent;
	gint show_tabs;
	gint tab_pos;
	guint show_status;
	gint show_tooltips;
	gint have_toolbar;
	gint have_tb_pix;
	gint have_tb_text;
	gint use_relief_toolbar;
	gchar *font;
	gint splitscreen;
	gchar *print_cmd;
	int num_recent; /* Number of recently accessed documents in the 
	                   Recent Documents menu */

} gE_preference;

extern gE_preference *settings;

#endif /* __GE_PREFS_H__ */
