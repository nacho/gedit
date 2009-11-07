/*
 * gedit-notebook.h
 * This file is part of gedit
 *
 * Copyright (C) 2005 - Paolo Maggi 
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
 * Modified by the gedit Team, 2005. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 */

/* This file is a modified version of the epiphany file ephy-notebook.h
 * Here the relevant copyright:
 *
 *  Copyright (C) 2002 Christophe Fergeau
 *  Copyright (C) 2003 Marco Pesenti Gritti
 *  Copyright (C) 2003, 2004 Christian Persch
 *
 */
 
#ifndef GEDIT_NOTEBOOK_H
#define GEDIT_NOTEBOOK_H

#include <gedit/gedit-page.h>

#include <glib.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

/*
 * Type checking and casting macros
 */
#define GEDIT_TYPE_NOTEBOOK		(gedit_notebook_get_type ())
#define GEDIT_NOTEBOOK(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GEDIT_TYPE_NOTEBOOK, GeditNotebook))
#define GEDIT_NOTEBOOK_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), GEDIT_TYPE_NOTEBOOK, GeditNotebookClass))
#define GEDIT_IS_NOTEBOOK(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GEDIT_TYPE_NOTEBOOK))
#define GEDIT_IS_NOTEBOOK_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GEDIT_TYPE_NOTEBOOK))
#define GEDIT_NOTEBOOK_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GEDIT_TYPE_NOTEBOOK, GeditNotebookClass))

/* Private structure type */
typedef struct _GeditNotebookPrivate	GeditNotebookPrivate;

/*
 * Main object structure
 */
typedef struct _GeditNotebook		GeditNotebook;
 
struct _GeditNotebook
{
	GtkNotebook notebook;

	/*< private >*/
        GeditNotebookPrivate *priv;
};

/*
 * Class definition
 */
typedef struct _GeditNotebookClass	GeditNotebookClass;

struct _GeditNotebookClass
{
        GtkNotebookClass parent_class;

	/* Signals */
	void	 (* gedit_page_added)
				     (GeditNotebook *notebook,
				      GeditPage     *page);
	void	 (* gedit_page_removed)
				     (GeditNotebook *notebook,
				      GeditPage     *page);
	void	 (* page_detached)   (GeditNotebook *notebook,
				      GeditPage     *page);
	void	 (* pages_reordered) (GeditNotebook *notebook);
	void	 (* page_close_request)
				     (GeditNotebook *notebook,
				      GeditPage     *tab);
};

/*
 * Public methods
 */
GType		gedit_notebook_get_type		(void) G_GNUC_CONST;

GtkWidget      *gedit_notebook_new		(void);

void		gedit_notebook_add_page		(GeditNotebook *nb,
						 GeditPage     *page,
						 gint           position,
						 gboolean       jump_to);

void		gedit_notebook_remove_page	(GeditNotebook *nb,
						 GeditPage     *page);

void		gedit_notebook_remove_all_pages	(GeditNotebook *nb);

void		gedit_notebook_reorder_page	(GeditNotebook *src,
			    			 GeditPage     *page,
			    			 gint           dest_position);
			    			 
void            gedit_notebook_move_page	(GeditNotebook *src,
						 GeditNotebook *dest,
						 GeditPage     *tab,
						 gint           dest_position);

/* FIXME: do we really need this function ? */
void		gedit_notebook_set_always_show_tabs
						(GeditNotebook *nb,
						 gboolean       show_pages);

void		gedit_notebook_set_close_buttons_sensitive
						(GeditNotebook *nb,
						 gboolean       sensitive);

gboolean	gedit_notebook_get_close_buttons_sensitive
						(GeditNotebook *nb);

void		gedit_notebook_set_page_drag_and_drop_enabled
						(GeditNotebook *nb,
						 gboolean       enable);

gboolean	gedit_notebook_get_page_drag_and_drop_enabled
						(GeditNotebook *nb);

G_END_DECLS

#endif /* GEDIT_NOTEBOOK_H */
