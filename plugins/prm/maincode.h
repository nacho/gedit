/*
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

#ifndef __MAINCODE_H__
#define __MAINCODE_H__
#define PROGRAMTITLE "Projektmanager"
#define VERSION "0.0.1"
typedef struct _prj_manager_menubar
{
	GtkWidget *bar;
	
	
	GtkWidget *fileroot;
	GtkWidget *filemenu;
	GtkWidget *filemenu_open;
	GtkWidget *filemenu_new;
	
	GtkWidget *helproot;
	GtkWidget *helpmenu;
	GtkWidget *helpmenu_about;
}prj_manager_menubar;
typedef struct _prj_manager_window
{
	gint selectedrow;	
	GtkCTreeNode *rowptr;		
	gint plugin_context;
	GtkWidget *window;
	GtkWidget *vbox;
	prj_manager_menubar *menubar;
	GtkWidget *scroller;
	GtkWidget *tree;
	GtkWidget *treeroot;
	char test[24];
}prj_manager_window;
typedef struct _prj_data
{	
	prj_manager_window * ptr;
	gint ready;
	gint ok;
	GtkWidget *widget;
	gpointer temp2;
}prj_data;
typedef struct _prj_tree_data
{
	gpointer treedataroot;
	gshort is_includefile;
	
		gchar *filename;
		gchar *path;
}prj_tree_data;
#endif
