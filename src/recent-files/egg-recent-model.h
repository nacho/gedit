/* vim: set sw=8: -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
#ifndef __EGG_RECENT_MODEL_H__
#define __EGG_RECENT_MODEL_H__

#include "egg-recent-item.h"

G_BEGIN_DECLS

#define EGG_RECENT_MODEL(obj)		G_TYPE_CHECK_INSTANCE_CAST (obj, egg_recent_model_get_type (), EggRecentModel)
#define EGG_RECENT_MODEL_CLASS(klass) 	G_TYPE_CHECK_CLASS_CAST (klass, egg_recent_model_get_type (), EggRecentModelClass)
#define EGG_IS_RECENT_MODEL(obj)		G_TYPE_CHECK_INSTANCE_TYPE (obj, egg_recent_model_get_type ())

typedef struct _EggRecentModel EggRecentModel;

typedef struct _EggRecentModelClass EggRecentModelClass;

struct _EggRecentModelClass {
	GObjectClass parent_class;
			
	void (*changed) (EggRecentModel *model, GList *list);
};

typedef enum {
	EGG_RECENT_MODEL_SORT_MRU,
	EGG_RECENT_MODEL_SORT_LRU,
	EGG_RECENT_MODEL_SORT_NONE
} EggRecentModelSort;


/* group names for a couple of things */
#define EGG_RECENT_MODEL_GROUP_LAUNCHERS "Launchers"


GType    egg_recent_model_get_type     (void);

/* constructors */
EggRecentModel * egg_recent_model_new (EggRecentModelSort sort);

/* public methods */
void     egg_recent_model_set_filter_mime_types (EggRecentModel *model,
						   ...);

void     egg_recent_model_set_filter_groups (EggRecentModel *model, ...);

void     egg_recent_model_set_filter_uri_schemes (EggRecentModel *model,
						   ...);

void     egg_recent_model_set_sort (EggRecentModel *model,
				      EggRecentModelSort sort);

gboolean egg_recent_model_add_full (EggRecentModel *model,
				      EggRecentItem *item);

gboolean egg_recent_model_add	     (EggRecentModel *model,
				      const gchar *uri);

gboolean egg_recent_model_delete   (EggRecentModel *model,
				      const gchar *uri);

void egg_recent_model_clear        (EggRecentModel *model);

GList * egg_recent_model_get_list  (EggRecentModel *model);

void egg_recent_model_changed      (EggRecentModel *model);

void egg_recent_model_set_limit    (EggRecentModel *model, int limit);
int  egg_recent_model_get_limit    (EggRecentModel *model);

void egg_recent_model_remove_expired (EggRecentModel *model);

G_END_DECLS

#endif /* __EGG_RECENT_MODEL_H__ */
