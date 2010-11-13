/*
 * gedit-theatrics-stage.h
 * This file is part of gedit
 *
 * Copyright (C) 2010 - Ignacio Casal Quinteiro
 *
 * Based on Aaron Bockover <abockover@novell.com> work.
 *
 * gedit is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * gedit is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with gedit; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, 
 * Boston, MA  02110-1301  USA
 */

#ifndef __GEDIT_THEATRICS_STAGE_H__
#define __GEDIT_THEATRICS_STAGE_H__

#include <glib-object.h>
#include "gedit-theatrics-actor.h"

G_BEGIN_DECLS

#define GEDIT_TYPE_THEATRICS_STAGE		(gedit_theatrics_stage_get_type ())
#define GEDIT_THEATRICS_STAGE(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_THEATRICS_STAGE, GeditTheatricsStage))
#define GEDIT_THEATRICS_STAGE_CONST(obj)	(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_THEATRICS_STAGE, GeditTheatricsStage const))
#define GEDIT_THEATRICS_STAGE_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GEDIT_TYPE_THEATRICS_STAGE, GeditTheatricsStageClass))
#define GEDIT_IS_THEATRICS_STAGE(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEDIT_TYPE_THEATRICS_STAGE))
#define GEDIT_IS_THEATRICS_STAGE_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEDIT_TYPE_THEATRICS_STAGE))
#define GEDIT_THEATRICS_STAGE_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GEDIT_TYPE_THEATRICS_STAGE, GeditTheatricsStageClass))

typedef struct _GeditTheatricsStage		GeditTheatricsStage;
typedef struct _GeditTheatricsStageClass	GeditTheatricsStageClass;
typedef struct _GeditTheatricsStagePrivate	GeditTheatricsStagePrivate;

struct _GeditTheatricsStage
{
	GObject parent;

	GeditTheatricsStagePrivate *priv;
};

struct _GeditTheatricsStageClass
{
	GObjectClass parent_class;

	void (* actor_step) (GeditTheatricsStage *stage,
			     GeditTheatricsActor *actor);

	void (* iteration) (GeditTheatricsStage *stage);
};

GType			 gedit_theatrics_stage_get_type		(void) G_GNUC_CONST;

GeditTheatricsStage	*gedit_theatrics_stage_new		(void);

GeditTheatricsStage	*gedit_theatrics_stage_new_with_duration(guint actor_duration);

GeditTheatricsActor	*gedit_theatrics_stage_add		(GeditTheatricsStage *stage,
								 GObject             *target);

GeditTheatricsActor	*gedit_theatrics_stage_add_with_duration(GeditTheatricsStage *stage,
								 GObject             *target,
								 guint                duration);

void			 gedit_theatrics_stage_remove		(GeditTheatricsStage *stage,
								 GObject             *target);

void			 gedit_theatrics_stage_set_playing	(GeditTheatricsStage *stage,
								 gboolean             playing);

gboolean		 gedit_theatrics_stage_get_playing	(GeditTheatricsStage *stage);

G_END_DECLS

#endif /* __GEDIT_THEATRICS_STAGE_H__ */

/* ex:set ts=8 noet: */
