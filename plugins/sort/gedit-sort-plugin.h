/*
 * gedit-sort-plugin.h
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef __GEDIT_SORT_PLUGIN_H__
#define __GEDIT_SORT_PLUGIN_H__

#include <glib.h>
#include <glib-object.h>
#include <libpeas/peas-extension-base.h>
#include <libpeas/peas-object-module.h>

G_BEGIN_DECLS

#define GEDIT_TYPE_SORT_PLUGIN		(gedit_sort_plugin_get_type ())
#define GEDIT_SORT_PLUGIN(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), GEDIT_TYPE_SORT_PLUGIN, GeditSortPlugin))
#define GEDIT_SORT_PLUGIN_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), GEDIT_TYPE_SORT_PLUGIN, GeditSortPluginClass))
#define GEDIT_IS_SORT_PLUGIN(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), GEDIT_TYPE_SORT_PLUGIN))
#define GEDIT_IS_SORT_PLUGIN_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), GEDIT_TYPE_SORT_PLUGIN))
#define GEDIT_SORT_PLUGIN_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), GEDIT_TYPE_SORT_PLUGIN, GeditSortPluginClass))

typedef struct _GeditSortPlugin		GeditSortPlugin;
typedef struct _GeditSortPluginPrivate	GeditSortPluginPrivate;
typedef struct _GeditSortPluginClass	GeditSortPluginClass;

struct _GeditSortPlugin
{
	PeasExtensionBase parent;

	/*< private >*/
	GeditSortPluginPrivate *priv;
};

struct _GeditSortPluginClass
{
	PeasExtensionBaseClass parent_class;
};

GType			gedit_sort_plugin_get_type	(void) G_GNUC_CONST;

G_MODULE_EXPORT void	peas_register_types		(PeasObjectModule *module);

G_END_DECLS

#endif /* __GEDIT_SORT_PLUGIN_H__ */
/* ex:ts=8:noet: */
