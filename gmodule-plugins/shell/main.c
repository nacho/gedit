/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 3 -*-
 *
 * Shell plugin for gEdit
 * Copyright (C) 1998 Martin Baulig
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

#include <config.h>
#include <gnome.h>

#include <gE_plugin.h>

#include <pwd.h>
#include <zvt/zvtterm.h>

static void init_plugin(gE_Plugin_Object *, gint);
static gboolean start_plugin(gE_Plugin_Object *, gint);

gint edit_context = 0;

gE_Plugin_Info gedit_plugin_info =
{
   "Shell Plugin",
   init_plugin,
   start_plugin,
};

#define DEFAULT_FONT "-misc-fixed-medium-r-normal--20-200-75-75-c-100-iso8859-1"

void
init_plugin(gE_Plugin_Object * plugin, gint context)
{
   start_plugin(plugin, context);
}

void
child_died_cb(ZvtTerm * zterm, int docid)
{
   fprintf(stderr, "Terminal %p child %d died.\n", zterm, docid);

   gE_plugin_document_close(docid);
}

gboolean
start_plugin(gE_Plugin_Object * plugin, gint context)
{
   struct passwd *pw;
   GtkWidget *container,
   *zterm;
   gint docid;

   docid = gE_plugin_create_widget(context, _("Shell"), &container, NULL);

   fprintf(stderr, "Plugin context %d, widget %p, docid %d.\n",
	   context, container, docid);

   zterm = zvt_term_new();
   gtk_widget_ensure_style(zterm);

   zvt_term_set_scrollback(ZVT_TERM(zterm), 5000);
   zvt_term_set_font_name(ZVT_TERM(zterm), DEFAULT_FONT);

   gtk_signal_connect(GTK_OBJECT(zterm), "child_died",
		      child_died_cb, (gpointer) docid);

   switch (zvt_term_forkpty(ZVT_TERM(zterm))) {
     case -1:
	perror("ERROR: unable to fork:");
	exit(1);
	break;
     case 0:
	pw = getpwuid(getpid());
	if (pw) {
	   execl(pw->pw_shell, rindex(pw->pw_shell, '/'), 0);
	} else {
	   execl("/bin/bash", "bash", 0);
	}
	perror("ERROR: Cannot exec command:");
	exit(1);
     default:
   }

   gtk_container_add(GTK_CONTAINER(container), zterm);
   gtk_widget_show_all(container);

   return TRUE;
}
