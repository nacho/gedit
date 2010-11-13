/*
 * gedit-theatrics-actor.h
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

#ifndef __GEDIT_THEATRICS_ACTOR_H__
#define __GEDIT_THEATRICS_ACTOR_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define GEDIT_TYPE_THEATRICS_ACTOR		(gedit_theatrics_actor_get_type ())
#define GEDIT_THEATRICS_ACTOR(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_THEATRICS_ACTOR, GeditTheatricsActor))
#define GEDIT_THEATRICS_ACTOR_CONST(obj)	(G_TYPE_CHECK_INSTANCE_CAST ((obj), GEDIT_TYPE_THEATRICS_ACTOR, GeditTheatricsActor const))
#define GEDIT_THEATRICS_ACTOR_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), GEDIT_TYPE_THEATRICS_ACTOR, GeditTheatricsActorClass))
#define GEDIT_IS_THEATRICS_ACTOR(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEDIT_TYPE_THEATRICS_ACTOR))
#define GEDIT_IS_THEATRICS_ACTOR_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), GEDIT_TYPE_THEATRICS_ACTOR))
#define GEDIT_THEATRICS_ACTOR_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), GEDIT_TYPE_THEATRICS_ACTOR, GeditTheatricsActorClass))

typedef struct _GeditTheatricsActor		GeditTheatricsActor;
typedef struct _GeditTheatricsActorClass	GeditTheatricsActorClass;
typedef struct _GeditTheatricsActorPrivate	GeditTheatricsActorPrivate;

struct _GeditTheatricsActor
{
	GObject parent;
	
	GeditTheatricsActorPrivate *priv;
};

struct _GeditTheatricsActorClass
{
	GObjectClass parent_class;
};

GType			 gedit_theatrics_actor_get_type		(void) G_GNUC_CONST;

GeditTheatricsActor	*gedit_theatrics_actor_new		(GObject *target,
								 guint    duration);

void			 gedit_theatrics_actor_reset		(GeditTheatricsActor *actor,
								 guint                duration);

void			 gedit_theatrics_actor_step		(GeditTheatricsActor *actor);

GObject			*gedit_theatrics_actor_get_target	(GeditTheatricsActor *actor);

gboolean		 gedit_theatrics_actor_get_expired	(GeditTheatricsActor *actor);

gboolean		 gedit_theatrics_actor_get_can_expire	(GeditTheatricsActor *actor);

void			 gedit_theatrics_actor_set_can_expire	(GeditTheatricsActor *actor,
								 gboolean             can_expire);

guint			 gedit_theatrics_actor_get_duration	(GeditTheatricsActor *actor);

GTimeVal		 gedit_theatrics_actor_get_start_time	(GeditTheatricsActor *actor);

gdouble			 gedit_theatrics_actor_get_frames	(GeditTheatricsActor *actor);

gdouble			 gedit_theatrics_actor_get_frames_per_second
								(GeditTheatricsActor *actor);

gdouble			 gedit_theatrics_actor_get_percent	(GeditTheatricsActor *actor);

G_END_DECLS

#endif /* __GEDIT_THEATRICS_ACTOR_H__ */

/* ex:set ts=8 noet: */
