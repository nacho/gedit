/*
 * gedit-view-container.h
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
 *
 * $Id$
 */

#ifndef __GEDIT_VIEW_CONTAINER_H__
#define __GEDIT_VIEW_CONTAINER_H__

#include <gtk/gtk.h>

#include <gedit/gedit-view-interface.h>
#include <gedit/gedit-document-interface.h>

G_BEGIN_DECLS

typedef enum
{
	GEDIT_VIEW_CONTAINER_STATE_NORMAL = 0,
	GEDIT_VIEW_CONTAINER_STATE_LOADING,
	GEDIT_VIEW_CONTAINER_STATE_REVERTING,
	GEDIT_VIEW_CONTAINER_STATE_SAVING,	
	GEDIT_VIEW_CONTAINER_STATE_PRINTING,
	GEDIT_VIEW_CONTAINER_STATE_PRINT_PREVIEWING,
	GEDIT_VIEW_CONTAINER_STATE_SHOWING_PRINT_PREVIEW,
	GEDIT_VIEW_CONTAINER_STATE_GENERIC_NOT_EDITABLE,
	GEDIT_VIEW_CONTAINER_STATE_LOADING_ERROR,
	GEDIT_VIEW_CONTAINER_STATE_REVERTING_ERROR,	
	GEDIT_VIEW_CONTAINER_STATE_SAVING_ERROR,
	GEDIT_VIEW_CONTAINER_STATE_GENERIC_ERROR,
	GEDIT_VIEW_CONTAINER_STATE_CLOSING,
	GEDIT_VIEW_CONTAINER_STATE_EXTERNALLY_MODIFIED_NOTIFICATION,
	GEDIT_VIEW_CONTAINER_NUM_OF_STATES /* This is not a valid state */
} GeditViewContainerState;

typedef enum
{
	GEDIT_VIEW_CONTAINER_MODE_TEXT = 0,
	GEDIT_VIEW_CONTAINER_MODE_BINARY
} GeditViewContainerMode;

/*
 * Type checking and casting macros
 */
#define GEDIT_TYPE_VIEW_CONTAINER              (gedit_view_container_get_type())
#define GEDIT_VIEW_CONTAINER(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), GEDIT_TYPE_VIEW_CONTAINER, GeditViewContainer))
#define GEDIT_VIEW_CONTAINER_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass), GEDIT_TYPE_VIEW_CONTAINER, GeditViewContainerClass))
#define GEDIT_IS_VIEW_CONTAINER(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), GEDIT_TYPE_VIEW_CONTAINER))
#define GEDIT_IS_VIEW_CONTAINER_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), GEDIT_TYPE_VIEW_CONTAINER))
#define GEDIT_VIEW_CONTAINER_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), GEDIT_TYPE_VIEW_CONTAINER, GeditViewContainerClass))

/* Private structure type */
typedef struct _GeditViewContainerPrivate GeditViewContainerPrivate;

/*
 * Main object structure
 */
typedef struct _GeditViewContainer GeditViewContainer;

struct _GeditViewContainer 
{
	GtkVBox vbox;

	/*< private > */
	GeditViewContainerPrivate *priv;
};

/*
 * Class definition
 */
typedef struct _GeditViewContainerClass GeditViewContainerClass;

struct _GeditViewContainerClass 
{
	GtkVBoxClass parent_class;
	
	void	(* view_added)			(GeditViewContainer *container,
						 GeditView          *view);
	void	(* view_removed)		(GeditViewContainer *container,
						 GeditView          *view);
	void	(* active_view_changed)		(GeditViewContainer *container,
						 GeditView          *view);
};

/*
 * Public methods
 */
GType 			 gedit_view_container_get_type 		(void) G_GNUC_CONST;

GeditView		*gedit_view_container_get_view		(GeditViewContainer *container);

GList			*gedit_view_container_get_views		(GeditViewContainer *container);

/* This is only an helper function */
GeditDocument		*gedit_view_container_get_document	(GeditViewContainer *container);

GeditViewContainer	*gedit_view_container_get_from_view	(GeditView          *view);

GeditViewContainer	*gedit_view_container_get_from_document	(GeditDocument      *doc);

GeditViewContainerState	 gedit_view_container_get_state		(GeditViewContainer *container);

GeditViewContainerMode	 gedit_view_container_get_mode		(GeditViewContainer *container);

gboolean		 gedit_view_container_get_auto_save_enabled
								(GeditViewContainer *container);

void			 gedit_view_container_set_auto_save_enabled
								(GeditViewContainer *container,
								 gboolean            enable);

gint			 gedit_view_container_get_auto_save_interval 
								(GeditViewContainer *container);

void			 gedit_view_container_set_auto_save_interval 
								(GeditViewContainer *container,
								 gint                interval);

/*
 * Non exported methods
 */
GtkWidget		*_gedit_view_container_new 		(void);

/* Whether create is TRUE, creates a new empty document if location does 
   not refer to an existing file */
GtkWidget		*_gedit_view_container_new_from_uri	(const gchar         *uri,
								 const GeditEncoding *encoding,
								 gint                 line_pos,
								 gboolean             create);
gchar			*_gedit_view_container_get_name		(GeditViewContainer  *container);
gchar			*_gedit_view_container_get_tooltips	(GeditViewContainer  *container);
GdkPixbuf		*_gedit_view_container_get_icon		(GeditViewContainer  *container);
void			 _gedit_view_container_load		(GeditViewContainer  *container,
								 const gchar         *uri,
								 const GeditEncoding *encoding,
								 gint                 line_pos,
								 gboolean             create);
void			 _gedit_view_container_revert		(GeditViewContainer  *container);
void			 _gedit_view_container_save		(GeditViewContainer  *container);
void			 _gedit_view_container_save_as		(GeditViewContainer  *container,
								 const gchar         *uri,
								 const GeditEncoding *encoding);

void			 _gedit_view_container_print		(GeditViewContainer  *container);
void			 _gedit_view_container_print_preview	(GeditViewContainer  *container);

void			 _gedit_view_container_mark_for_closing	(GeditViewContainer  *container);

gboolean		 _gedit_view_container_can_close	(GeditViewContainer  *container);

#if !GTK_CHECK_VERSION (2, 17, 4)
void			 _gedit_view_container_page_setup	(GeditViewContainer  *container);
#endif

G_END_DECLS

#endif  /* __GEDIT_VIEW_CONTAINER_H__  */
