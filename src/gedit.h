/* 
 * gedit
 * Copyright (C) 1998, 1999 Alex Roberts and Evan Lawrence
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

#ifndef __GEDIT_H__
#define __GEDIT_H__

#define GEDIT_ID	"gedit "VERSION

typedef struct _gedit_data gedit_data;

struct _gedit_data
{
	Window *window;
	Document *document;
	gpointer temp1;
	gpointer temp2;
	gboolean flag;	/* general purpose flag to indicate if action completed */
};

#endif /* __GEDIT_H__ */
