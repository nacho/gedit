/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/**
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 * Authors:
 *   James Willcox <jwillcox@cs.indiana.edu>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <gconf/gconf-client.h>
#include <libbonoboui.h>
#include <libgnomevfs/gnome-vfs.h>
#include "gedit-recent.h"
#include "gedit-debug.h"

#define GEDIT_RECENT_BASE_KEY "/desktop/gnome/gal-recent"
#define GEDIT_RECENT_DEFAULT_LIMIT 10
#define GEDIT_RECENT_GLOBAL_LIMIT_ENV "GEDIT_RECENT_GLOBAL_LIMIT"
#define GEDIT_RECENT_VERB_NAME "-uri-"

static void gedit_recent_class_init      (GeditRecentClass * klass);
static void gedit_recent_init            (GeditRecent * recent);
static gchar *gedit_recent_gconf_key     (GeditRecent * recent);
static void gedit_recent_menu_cb         (BonoboUIComponent *uic, gpointer data,
					  const char *cname);
static void gedit_recent_notify_cb       (GConfClient *client, guint cnxn_id,
				         GConfEntry *entry, gpointer user_data);

static GSList * gedit_recent_delete_from_list (GeditRecent *recent,
					       GSList *list,
					       const gchar *uri);
static gchar * gedit_recent_escape_underlines (const gchar *str);
static GSList * gedit_recent_gconf_to_list    (GConfValue* value);
static void gedit_recent_update_menus (GeditRecent *recent, GSList *list);
static void gedit_recent_g_slist_deep_free (GSList *list);
static void gedit_recent_set_appname (GeditRecent *recent, gchar *appname);

struct _GeditRecent {
	GObject parent_instance;	/* We emit signals */

	gchar *appname; 		/* the app that this object is for */
	GConfClient *gconf_client;	/* we use GConf to store the stuff */
	unsigned int limit;		/* maximum number of items to store */

	BonoboUIComponent *uic;
	
	GeditRecent *global;		/* Another GeditRecent object,
					 * representing the global
					 * recent uri list
					 */

	gchar *path;			/* The menu path where our stuff
					 *  will go
					 */
	
	GHashTable *monitors;		/* A hash table holding
					 * GnomeVfsMonitorHandle objects.
					 */
};

struct _GeditRecentClass {
	GObjectClass parent_class;
	
	void (*activate) (GeditRecent *recent, const gchar *uri);
};

struct _GeditRecentMenuData {
	GeditRecent *recent;
	gchar *uri;
};

typedef struct _GeditRecentMenuData GeditRecentMenuData;

enum {
	ACTIVATE,
	LAST_SIGNAL
};

/* GObject properties */
enum {
	PROP_BOGUS,
	PROP_APPNAME,
	PROP_LIMIT,
	PROP_UI_COMPONENT,
	PROP_MENU_PATH
};

static guint gedit_recent_signals[LAST_SIGNAL] = { 0 };
static GtkObjectClass *parent_class = NULL;

/**
 * gedit_recent_get_type:
 * @:
 *
 * This returns a GType representing a GeditRecent object.
 *
 * Returns: a GType
 */
GType
gedit_recent_get_type (void)
{
	static GType gedit_recent_type = 0;

	if(!gedit_recent_type) {
		static const GTypeInfo gedit_recent_info = {
			sizeof (GeditRecentClass),
			NULL, /* base init */
			NULL, /* base finalize */
			(GClassInitFunc)gedit_recent_class_init, /* class init */
			NULL, /* class finalize */
			NULL, /* class data */
			sizeof (GeditRecent),
			0,
			(GInstanceInitFunc) gedit_recent_init
		};

		gedit_recent_type = g_type_register_static (G_TYPE_OBJECT,
							"GeditRecent",
							&gedit_recent_info, 0);
	}

	return gedit_recent_type;
}

static void
gedit_recent_set_property (GObject *object,
			   guint prop_id,
			   const GValue *value,
			   GParamSpec *pspec)
{
	GeditRecent *recent = GEDIT_RECENT (object);
	gchar *appname;

	switch (prop_id)
	{
		case PROP_APPNAME:
			appname = g_strdup (g_value_get_string (value));
			gedit_recent_set_appname (recent, appname);
		break;
		case PROP_LIMIT:
			gedit_recent_set_limit (GEDIT_RECENT (recent),
						g_value_get_int (value));
		break;
		case PROP_UI_COMPONENT:
			gedit_recent_set_ui_component (GEDIT_RECENT (recent),
						       BONOBO_UI_COMPONENT (g_value_get_object (value)));
		break;
		case PROP_MENU_PATH:
			recent->path = g_strdup (g_value_get_string (value));
		break;
		default:
		break;
	}
}

static void
gedit_recent_get_property (GObject *object,
			   guint prop_id,
			   GValue *value,
			   GParamSpec *pspec)
{
	GeditRecent *recent = GEDIT_RECENT (object);

	switch (prop_id)
	{
		case PROP_APPNAME:
			g_value_set_string (value, recent->appname);
		break;
		case PROP_LIMIT:
			g_value_set_int (value, recent->limit);
		break;
		case PROP_UI_COMPONENT:
			g_value_set_pointer (value, recent->uic);
		break;
		case PROP_MENU_PATH:
			g_value_set_string (value, g_strdup (recent->path));
		break;
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static void
gedit_recent_class_init (GeditRecentClass * klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);

	object_class->set_property = gedit_recent_set_property;
	object_class->get_property = gedit_recent_get_property;

	gedit_recent_signals[ACTIVATE] = g_signal_new ("activate",
			G_OBJECT_CLASS_TYPE (object_class),
			G_SIGNAL_RUN_LAST,
			G_STRUCT_OFFSET (GeditRecentClass, activate),
			NULL, NULL,
			g_cclosure_marshal_VOID__STRING,
			G_TYPE_NONE, 1,
			G_TYPE_STRING);

	g_object_class_install_property (object_class,
					 PROP_APPNAME,
					 g_param_spec_string ("appname",
						 	      "Application Name",
							      "The name of the application using this object.",
							      "gal-app",
							      G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
					 PROP_LIMIT,
					 g_param_spec_int    ("limit",
						 	      "Limit",
							      "The maximum number of items to be allowed in the list.",
							      1,
							      1000,
							      10,
							      G_PARAM_READWRITE));

	g_object_class_install_property (object_class,
					 PROP_UI_COMPONENT,
					 g_param_spec_object ("ui-component",
						 	      "UI Component",
							      "The BonoboUIComponent where we will put menus.",
							      bonobo_ui_component_get_type(),
							      G_PARAM_READWRITE));

	g_object_class_install_property (object_class,
					 PROP_MENU_PATH,
					 g_param_spec_string ("ui-path",
						 	      "Path",
							      "The path to put the menu items.",
							      "/menus/File/GeditRecentDocuments",
							      G_PARAM_READWRITE));

	klass->activate = NULL;
}


static void
gedit_recent_init (GeditRecent * recent)
{
	int argc=0;
	char **argv=NULL;

	if (!gconf_init (argc, argv, NULL))
	{
		g_warning ("GConf Initialization failed.");
		return;
	}
	
	if (!gnome_vfs_init ()) {
		g_warning ("gnome-vfs initialization failed.");
		return;
	}

	recent->gconf_client = gconf_client_get_default ();
	recent->monitors = g_hash_table_new (g_str_hash, g_str_equal);
}


/**
 * gedit_recent_new:
 * @appname: The name of your application.
 * @limit:  The maximum number of items allowed.
 *
 * This creates a new GeditRecent object.
 *
 * Returns: a GeditRecent object
 */
GeditRecent *
gedit_recent_new (const gchar *appname, gint limit)
{
	GeditRecent *recent;

	g_return_val_if_fail (appname, NULL);
	g_return_val_if_fail (limit > 0, NULL);

	recent = GEDIT_RECENT (g_object_new (gedit_recent_get_type (),
					   "appname",
					   appname,
					   "limit",
					   limit, NULL));

	g_return_val_if_fail (recent, NULL);
	
	return recent;
}

GeditRecent *
gedit_recent_new_with_ui_component (const gchar *appname,
				    gint limit,
				    BonoboUIComponent *uic,
				    const gchar *path)
{
	GeditRecent *recent;

	g_return_val_if_fail (appname, NULL);
	g_return_val_if_fail (limit > 0, NULL);
	g_return_val_if_fail (uic, NULL);
	g_return_val_if_fail (path, NULL);

	recent = GEDIT_RECENT (g_object_new (gedit_recent_get_type (),
					   "appname",
					   appname,
					   "limit",
					   limit,
					   "ui-path",
					   path,
					   "ui-component",
					   uic, NULL));

	return recent;
}

void
gedit_recent_set_ui_component (GeditRecent *recent, BonoboUIComponent *uic)
{
	GSList *list;

	g_return_if_fail (uic);

	recent->uic = uic;
	list = gedit_recent_get_list (recent);

	gedit_recent_update_menus (recent, list);

	gedit_recent_g_slist_deep_free (list);
}

static void
gedit_recent_clear_menu (GeditRecent *recent)
{
	gint i=1;
	gboolean done=FALSE;

	g_return_if_fail (recent);
	g_return_if_fail (recent->uic);

	while (!done)
	{
		gchar *verb_name = g_strdup_printf ("%s%s%d", recent->appname,GEDIT_RECENT_VERB_NAME, i);
		gchar *item_path = g_strconcat (recent->path, "/", verb_name, NULL);
		if (bonobo_ui_component_path_exists (recent->uic, item_path, NULL))
			bonobo_ui_component_rm (recent->uic, item_path, NULL);
		else
			done=TRUE;

		g_free (item_path);
		g_free (verb_name);

		i++;
	}
}

static void
gedit_recent_monitor_cb (GnomeVFSMonitorHandle *handle,
			 const gchar *monitor_uri,
			 const gchar *info_uri,
			 GnomeVFSMonitorEventType event_type,
			 gpointer data)
{
	GeditRecent *recent= GEDIT_RECENT (data);

	gedit_debug (DEBUG_RECENT, "Something happened to %s", monitor_uri);

	g_return_if_fail (recent);

	/* if a file was deleted, we just remove it from our list */
	switch (event_type) {
		case GNOME_VFS_MONITOR_EVENT_DELETED:
			gedit_recent_delete (recent, monitor_uri);
			break;
		default:
		break;
	}

}

static void
gedit_recent_monitor_uri (GeditRecent *recent, const gchar *uri)
{
	GnomeVFSMonitorHandle *handle=NULL;
	GnomeVFSResult result;

	g_return_if_fail (recent);
	g_return_if_fail (GEDIT_IS_RECENT (recent));
	g_return_if_fail (uri);

	handle = g_hash_table_lookup (recent->monitors, uri);
	if (handle == NULL) {

		gedit_debug (DEBUG_RECENT, "Monitoring: %s\n", uri);
		
		/* this is a new uri, so we need to monitor it */
		result = gnome_vfs_monitor_add (&handle,
				       g_strdup (uri),
				       GNOME_VFS_MONITOR_FILE,
				       gedit_recent_monitor_cb,
				       recent);
		if (result == GNOME_VFS_OK) {
			g_hash_table_insert (recent->monitors,
					     g_strdup (uri),
					     handle);
		}
#if 0
		else {
			const gchar *tmp = gnome_vfs_result_to_string (result);
			g_warning ("%s: %s", tmp, uri);
		}
#endif
	}
}

#if 0
static void
gedit_recent_monitor_cancel (GeditRecent *recent, const gchar *uri)
{
	g_return_if_fail (recent);
	g_return_if_fail (GEDIT_IS_RECENT (recent));
	g_return_if_fail (uri);

	g_hash_table_remove (recent->monitors, uri);
}
#endif


static void
gedit_recent_update_menus (GeditRecent *recent, GSList *list)
{
	BonoboUIComponent* ui_component;
	unsigned int i;
	gchar *label = NULL;
	gchar *verb_name = NULL;
	gchar *tip = NULL;
	gchar *escaped_name = NULL;
	gchar *item_path = NULL;
	gchar *uri;
	gchar* cmd;
	GeditRecentMenuData *md;

	gedit_debug (DEBUG_RECENT, "");

	g_return_if_fail (recent);

	ui_component = recent->uic;
	g_return_if_fail (BONOBO_IS_UI_COMPONENT (ui_component));

	gedit_recent_clear_menu (recent);
	
	bonobo_ui_component_freeze (ui_component, NULL);

	for (i = 1; i <= g_slist_length (list); ++i)
	{
		
		/* this is what gets passed to our private "activate" callback */
		md = (GeditRecentMenuData *)g_malloc (sizeof (GeditRecentMenuData));
		md->recent = recent;
		md->uri = g_strdup (g_slist_nth_data (list, i-1));

		/* Maybe we should use a gnome-vfs call here?? */
		uri = g_path_get_basename (g_slist_nth_data (list, i - 1));
	
		escaped_name = gedit_recent_escape_underlines (uri);

		tip =  g_strdup_printf (_("Open %s"), uri);

		verb_name = g_strdup_printf ("%s%s%d", recent->appname,GEDIT_RECENT_VERB_NAME, i);
		cmd = g_strdup_printf ("<cmd name = \"%s\" /> ", verb_name);
		bonobo_ui_component_set_translate (ui_component, "/commands/", cmd, NULL);
		bonobo_ui_component_add_verb (ui_component, verb_name,
					      gedit_recent_menu_cb, md); 
	        
		if (i < 10)
			label = g_strdup_printf ("_%d. %s", i, escaped_name);
		else
			label = g_strdup_printf ("%d. %s", i, escaped_name);
			
		
		
		item_path = g_strconcat (recent->path, "/", verb_name, NULL);

		if (bonobo_ui_component_path_exists (ui_component, item_path, NULL))
		{
			bonobo_ui_component_set_prop (ui_component, item_path, 
					              "label", label, NULL);

			bonobo_ui_component_set_prop (ui_component, item_path, 
					              "tip", tip, NULL);
		}
		else
		{
			gchar *xml;
			
			xml = g_strdup_printf ("<menuitem name=\"%s\" verb=\"%s\""
						" _label=\"%s\"  _tip=\"%s \" hidden=\"0\" />", 
						verb_name, verb_name, label, tip);

			bonobo_ui_component_set_translate (ui_component, recent->path, xml, NULL);

			g_free (xml); 
		}
		
		gedit_recent_monitor_uri (recent, md->uri);

		g_free (label);
		g_free (verb_name);
		g_free (tip);
		g_free (escaped_name);
		g_free (item_path);
		g_free (uri);
		g_free (cmd);
	}

	gedit_debug (DEBUG_RECENT, "END");


	bonobo_ui_component_thaw (ui_component, NULL);
}



/**
 * gedit_recent_add:
 * @recent:  A GeditRecent object.
 * @uri: The URI you want to add to the list.
 *
 * This function adds a URI to the list of recently used URIs.
 *
 * Returns: a gboolean
 */
gboolean
gedit_recent_add (GeditRecent * recent, const gchar * uri)
{
	GSList *uri_lst;
	gchar *gconf_key;

	g_return_val_if_fail (recent, FALSE);
	g_return_val_if_fail (GEDIT_IS_RECENT (recent), FALSE);
	g_return_val_if_fail (recent->gconf_client, FALSE);
	g_return_val_if_fail (uri, FALSE);

	gedit_debug (DEBUG_RECENT, "Adding: %s", uri);

	gconf_key = gedit_recent_gconf_key (recent);


	uri_lst = gconf_client_get_list (recent->gconf_client,
				       gconf_key,
				       GCONF_VALUE_STRING, NULL);

	/* if this is already in our list, remove it */
	uri_lst = gedit_recent_delete_from_list (recent, uri_lst, uri);

	/* prepend the new one */
	uri_lst = g_slist_prepend (uri_lst, g_strdup (uri));

	/* if we're over the limit, delete from the end */
	while (g_slist_length (uri_lst) > recent->limit)
	{
		gchar *tmp_uri;
		tmp_uri = g_slist_nth_data (uri_lst, g_slist_length (uri_lst)-1);
		uri_lst = g_slist_remove (uri_lst, tmp_uri);
	}
	
	gconf_client_set_list (recent->gconf_client,
			      gconf_key,
			      GCONF_VALUE_STRING,
			      uri_lst, NULL);

	gconf_client_suggest_sync (recent->gconf_client, NULL);

	/* add to the global list */
	if (recent->global)
		gedit_recent_add (GEDIT_RECENT (recent->global), uri);

	g_free (gconf_key);
	gedit_recent_g_slist_deep_free (uri_lst);

	return TRUE;
}


/**
 * gedit_recent_delete:
 * @recent:  A GeditRecent object.
 * @uri: The URI you want to delete from the list.
 *
 * This function deletes a URI from the list of recently used URIs.
 *
 * Returns: a gboolean
 */
gboolean
gedit_recent_delete (GeditRecent * recent, const gchar * uri)
{
	GSList *uri_lst;
	GSList *new_uri_lst;
	gboolean ret = FALSE;
	gchar *gconf_key;

	gedit_debug (DEBUG_RECENT, "Deleting: %s", uri);

	g_return_val_if_fail (recent, FALSE);
	g_return_val_if_fail (GEDIT_IS_RECENT (recent), FALSE);
	g_return_val_if_fail (recent->gconf_client, FALSE);
	g_return_val_if_fail (uri, FALSE);

	gconf_key = gedit_recent_gconf_key (recent);
	uri_lst = gconf_client_get_list (recent->gconf_client,
				       gconf_key,
				       GCONF_VALUE_STRING, NULL);

	new_uri_lst = gedit_recent_delete_from_list (recent, uri_lst, uri);

	/* if it wasn't deleted, no need to cause unneeded updates */
	/*
	if (new_uri_lst == uri_lst) {
		return FALSE;
	}
	else
		uri_lst = new_uri_lst;
	*/

	/* delete it from gconf */
	gconf_client_set_list (recent->gconf_client,
			       gconf_key,
			       GCONF_VALUE_STRING,
			       new_uri_lst,
			       NULL);
	gconf_client_suggest_sync (recent->gconf_client, NULL);

	/* delete from the global list */
	if (recent->global)
		gedit_recent_delete (GEDIT_RECENT (recent->global), uri);


	g_free (gconf_key);
	gedit_recent_g_slist_deep_free (new_uri_lst);

	return ret;
}

/**
 * gedit_recent_get_list:
 * @recent: A GeditRecent object.
 *
 * This returns a linked list of strings (URIs) currently held
 * by this object.
 *
 * Returns: A GSList *
 */
GSList *
gedit_recent_get_list (GeditRecent * recent)
{
	GSList *uri_lst;
	gchar *gconf_key = gedit_recent_gconf_key (recent);

	g_return_val_if_fail (recent, NULL);
	g_return_val_if_fail (recent->gconf_client, NULL);
	g_return_val_if_fail (GEDIT_IS_RECENT (recent), NULL);

	uri_lst = gconf_client_get_list (recent->gconf_client,
				       gconf_key,
				       GCONF_VALUE_STRING, NULL);

	g_free (gconf_key);

	return uri_lst;
}



/**
 * gedit_recent_set_limit:
 * @recent: A GeditRecent object.
 * @limit: The maximum number of items allowed in the list.
 *
 * Use this function to constrain the number of items allowed in the list.
 * The default is %GEDIT_RECENT_DEFAULT_LIMIT.
 *
 */
void
gedit_recent_set_limit (GeditRecent *recent, gint limit)
{
	GSList *list;
	int len;
	unsigned int i;

	g_return_if_fail (recent);
	g_return_if_fail (GEDIT_IS_RECENT (recent));
	g_return_if_fail (limit > 0);
	recent->limit = limit;

	list = gedit_recent_get_list (recent);
	len = g_slist_length (list);

	if (len <= limit) return;

	/* if we're over the limit, delete from the end */
	i=g_slist_length (list);
	while (i > recent->limit)
	{
		gchar *uri = g_slist_nth_data (list, i-1);
		gedit_recent_delete (recent, uri);

		i--;
	}

	gedit_recent_g_slist_deep_free (list);
}


/**
 * gedit_recent_get_limit:
 * @recent: A GeditRecent object.
 *
 */
gint
gedit_recent_get_limit (GeditRecent *recent)
{
	g_return_val_if_fail (recent, -1);
	g_return_val_if_fail (GEDIT_IS_RECENT (recent), -1);

	return recent->limit;
}


/**
 * gedit_recent_clear:
 * @recent: A GeditRecent object.
 *
 * This function clears the list of recently used URIs.
 *
 */
void
gedit_recent_clear (GeditRecent *recent)
{
	gchar *key;

	g_return_if_fail (recent);
	g_return_if_fail (recent->gconf_client);
	g_return_if_fail (GEDIT_IS_RECENT (recent));

	key = gedit_recent_gconf_key (recent);

	gconf_client_unset (recent->gconf_client, key, NULL);
}

static void
gedit_recent_set_appname (GeditRecent *recent, gchar *appname)
{
	gchar *key;
	gint notify_id;

	g_return_if_fail (recent);
	g_return_if_fail (appname);

	recent->appname = appname;

	/* if this isn't the global list embed a global one */
	if (strcmp (appname, GEDIT_RECENT_GLOBAL_LIST)) {
		recent->global = gedit_recent_new (GEDIT_RECENT_GLOBAL_LIST,
						 GEDIT_RECENT_DEFAULT_LIMIT);
	}

	/* Set up the gconf notification stuff */
	key = gedit_recent_gconf_key (recent);
	gconf_client_add_dir (recent->gconf_client,
			GEDIT_RECENT_BASE_KEY, GCONF_CLIENT_PRELOAD_NONE, NULL);
	notify_id = gconf_client_notify_add (recent->gconf_client,
					    key,
					    gedit_recent_notify_cb,
					    recent, NULL, NULL);



	g_free (key);


}

static GSList *
gedit_recent_delete_from_list (GeditRecent *recent, GSList *list,
			       const gchar *uri)
{
	unsigned int i;
	gchar *text;

	for (i = 0; i < g_slist_length (list); i++) {
		text = g_slist_nth_data (list, i);
		
		if (!strcmp (text, uri)) {
			list = g_slist_remove (list, text);
		}
	}

	return list;
}

static void
gedit_recent_menu_cb (BonoboUIComponent *uic, gpointer data, const char *cname)
{
	GeditRecentMenuData *md = (GeditRecentMenuData *) data;

	gedit_debug (DEBUG_RECENT, "In menu callback");

	g_return_if_fail (md);
	g_return_if_fail (md->uri);
	g_return_if_fail (md->recent);
	g_return_if_fail (GEDIT_IS_RECENT (md->recent));

	/* rethinking this....leave it up to the app?
	gedit_recent_add (md->recent, md->uri);
	*/
	
	g_signal_emit (G_OBJECT(md->recent), gedit_recent_signals[ACTIVATE], 0,
		       md->uri);


	gedit_debug (DEBUG_RECENT, "...Done.");
}


/* this takes a list of GConfValues, and returns a list of strings */
static GSList *
gedit_recent_gconf_to_list (GConfValue* value)
{    
	GSList* iter;
	GSList *list = NULL;

	g_return_val_if_fail (value, NULL);

	iter = gconf_value_get_list(value);

	while (iter != NULL)
	{
		GConfValue* element = iter->data;
		gchar *text = g_strdup (gconf_value_get_string (element));

		list = g_slist_prepend (list, text);

		iter = g_slist_next(iter);
	}

	list = g_slist_reverse (list);

	return list;
}

/* ripped out of gedit2 */
static gchar* 
gedit_recent_escape_underlines (const gchar* text)
{
	GString *str;
	gint length;
	const gchar *p;
 	const gchar *end;

  	g_return_val_if_fail (text != NULL, NULL);

    	length = strlen (text);

	str = g_string_new ("");

  	p = text;
  	end = text + length;

  	while (p != end)
    	{
      		const gchar *next;
      		next = g_utf8_next_char (p);

		switch (*p)
        	{
       			case '_':
          			g_string_append (str, "__");
          			break;
        		default:
          			g_string_append_len (str, p, next - p);
          			break;
        	}

      		p = next;
    	}

	return g_string_free (str, FALSE);
}

static void
gedit_recent_g_slist_deep_free (GSList *list)
{
	GSList *lst;

	g_return_if_fail (list);

	lst = list;
	while (lst) {
		g_free (lst->data);
		lst->data = NULL;
		lst = lst->next;
	}

	g_slist_free (list);
}

static gchar *
gedit_recent_gconf_key (GeditRecent * recent)
{
	gchar *key;

	g_return_val_if_fail (recent, NULL);

	key = g_strdup_printf ("%s/%s", GEDIT_RECENT_BASE_KEY, recent->appname);
	return key;
}

/*
static void
print_list (GSList *list)
{
	while (list) {
		g_print ("%s, ", (char *)list->data);

		list = list->next;
	}
	g_print ("\n\n");
}
*/
          
/* this is the gconf notification callback. */
static void
gedit_recent_notify_cb (GConfClient *client, guint cnxn_id,
			GConfEntry *entry, gpointer user_data)
{
	GSList *list=NULL;
	GeditRecent *recent = user_data;

	/* bail out if we don't have a menu */
	if (!recent->uic)
		return;

	/* this means the key was unset (cleared) */
	if (entry->value == NULL) {
		gedit_recent_clear_menu (recent);
		return;
	}

	list = gedit_recent_gconf_to_list (entry->value);

	if (recent->uic) {
		gedit_recent_update_menus (recent, list);
	}

	gedit_recent_g_slist_deep_free (list);
}
