/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-metadata-manager.c
 * This file is part of gedit
 *
 * Copyright (C) 2003  Paolo Maggi 
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
 * Modified by the gedit Team, 2003. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <time.h>
#include <stdlib.h>

#include <libxml/xmlreader.h>

#include <libgnome/gnome-util.h>

#include "gedit-metadata-manager.h"
#include "gedit-debug.h"


#define METADATA_FILE 	"gedit-metadata.xml"

#define MAX_ITEMS	50

typedef struct _GeditMetadataManager GeditMetadataManager;

typedef struct _Item Item;

struct _Item
{
	time_t	 	 atime; /* time of last access */

	GHashTable	*values;
};
	
struct _GeditMetadataManager
{
	gboolean	 values_loaded; /* It is true if the file 
					   has been read */

	gboolean	 modified;	/* It is true if the file 
					   has top be written */

	guint 		 timeout_id;

	GHashTable	*items;
};

static gboolean gedit_metadata_manager_save (gpointer data);


static GeditMetadataManager *gedit_metadata_manager = NULL;

static void
item_free (gpointer data)
{
	Item *item;
	
	g_return_if_fail (data != NULL);

	gedit_debug (DEBUG_METADATA, "");

	item = (Item *)data;

	if (item->values != NULL)
		g_hash_table_destroy (item->values);

	g_free (item);
}

static gboolean
gedit_metadata_manager_init (void)
{
	gedit_debug (DEBUG_METADATA, "");

	if (gedit_metadata_manager != NULL)
		return TRUE;

	gedit_metadata_manager = g_new0 (GeditMetadataManager, 1);

	gedit_metadata_manager->values_loaded = FALSE;
	gedit_metadata_manager->modified = FALSE;

	gedit_metadata_manager->items = 
		g_hash_table_new_full (g_str_hash, 
				       g_str_equal, 
				       g_free,
				       item_free);

	gedit_metadata_manager->timeout_id = 
		g_timeout_add_full (G_PRIORITY_DEFAULT_IDLE,
				    2000, /* 2 sec */
				    (GSourceFunc)gedit_metadata_manager_save,
				    NULL,
				    NULL);
	
	return TRUE;
}

/* This function must be called before exiting gedit */
void
gedit_metadata_manager_shutdown (void)
{
	gedit_debug (DEBUG_METADATA, "");

	if (gedit_metadata_manager == NULL)
		return;

	g_source_remove (gedit_metadata_manager->timeout_id);

	gedit_metadata_manager_save (NULL);

	if (gedit_metadata_manager->items != NULL)
		g_hash_table_destroy (gedit_metadata_manager->items);

	g_free (gedit_metadata_manager);
	gedit_metadata_manager = NULL;
}

static void
parseItem (xmlDocPtr doc, xmlNodePtr cur)
{
	Item *item;
	
	xmlChar *uri;
	xmlChar *atime;
	
	gedit_debug (DEBUG_METADATA, "");

	if (xmlStrcmp (cur->name, (const xmlChar *)"document") != 0)
			return;

	uri = xmlGetProp (cur, "uri");
	if (uri == NULL)
		return;
	
	atime = xmlGetProp (cur, "atime");
	if (atime == NULL)
	{
		xmlFree (uri);
		return;
	}

	item = g_new0 (Item, 1);

	item->atime = atoi (atime);
	
	item->values = g_hash_table_new_full (g_str_hash, 
					      g_str_equal, 
					      g_free, 
					      g_free);

	cur = cur->xmlChildrenNode;
		
	while (cur != NULL)
	{
		if (xmlStrcmp (cur->name, (const xmlChar *)"entry") == 0)
		{
			xmlChar *key;
			xmlChar *value;
			
			key = xmlGetProp (cur, "key");
			value = xmlGetProp (cur, "value");

			if ((key != NULL) && (value != NULL))
				g_hash_table_insert (item->values,
						     g_strdup (key), 
						     g_strdup (value));

			if (key != NULL)
				xmlFree (key);
			if (value != NULL)
				xmlFree (value);
		}
		
		cur = cur->next;
	}

	g_hash_table_insert (gedit_metadata_manager->items,
			     g_strdup (uri),
			     item);

	xmlFree (uri);
	xmlFree (atime);
}

static gboolean
load_values ()
{
	xmlDocPtr doc;
	xmlNodePtr cur;
	gchar *file_name;

	gedit_debug (DEBUG_METADATA, "");

	g_return_val_if_fail (gedit_metadata_manager != NULL, FALSE);
	g_return_val_if_fail (gedit_metadata_manager->values_loaded == FALSE, FALSE);

	gedit_metadata_manager->values_loaded = TRUE;
		
	xmlKeepBlanksDefault (0);

	/* FIXME: file locking - Paolo */
	file_name = gnome_util_home_file (METADATA_FILE);
	if (!g_file_test (file_name, G_FILE_TEST_EXISTS))
	{
		g_free (file_name);
		return FALSE;
	}

	doc = xmlParseFile (file_name);
	g_free (file_name);

	if (doc == NULL)
	{
		return FALSE;
	}

	cur = xmlDocGetRootElement (doc);
	if (cur == NULL) 
	{
		g_message ("The metadata file '%s' is empty", METADATA_FILE);
		xmlFreeDoc (doc);
	
		return FALSE;
	}

	if (xmlStrcmp (cur->name, (const xmlChar *) "metadata")) 
	{
		g_message ("File '%s' is of the wrong type", METADATA_FILE);
		xmlFreeDoc (doc);
		
		return FALSE;
	}

	cur = xmlDocGetRootElement (doc);
	cur = cur->xmlChildrenNode;
	
	while (cur != NULL)
	{
		parseItem (doc, cur);

		cur = cur->next;
	}

	xmlFreeDoc (doc);

	return TRUE;
}

gchar *
gedit_metadata_manager_get (const gchar *uri,
			    const gchar *key)
{
	Item *item;
	gchar *value;
	
	gedit_debug (DEBUG_METADATA, "");

	g_return_val_if_fail (uri != NULL, NULL);
	g_return_val_if_fail (key != NULL, NULL);

	if (gedit_metadata_manager == NULL)
		gedit_metadata_manager_init ();

	if (!gedit_metadata_manager->values_loaded)
	{
		gboolean res;

		res = load_values ();

		if (!res)
			return NULL;
	}

	item = (Item *)g_hash_table_lookup (gedit_metadata_manager->items,
					    uri);

	if (item == NULL)
		return NULL;

	item->atime = time (NULL);
	
	if (item->values == NULL)
		return NULL;
	
	value = (gchar *)g_hash_table_lookup (item->values, key);

	if (value == NULL)
		return NULL;
	else
		return g_strdup (value);
}

void
gedit_metadata_manager_set (const gchar *uri,
			    const gchar *key,
			    const gchar *value)
{
	Item *item;

	gedit_debug (DEBUG_METADATA, "");

	g_return_if_fail (uri != NULL);
	g_return_if_fail (key != NULL);

	if (gedit_metadata_manager == NULL)
		gedit_metadata_manager_init ();

	if (!gedit_metadata_manager->values_loaded)
	{
		gboolean res;

		res = load_values ();

		if (!res)
			return;
	}

	item = (Item *)g_hash_table_lookup (gedit_metadata_manager->items,
					    uri);

	if (item == NULL)
	{
		item = g_new0 (Item, 1);

		g_hash_table_insert (gedit_metadata_manager->items,
				     g_strdup (uri),
				     item);
	}
	
	if (item->values == NULL)
		 item->values = g_hash_table_new_full (g_str_hash, 
				 		       g_str_equal, 
						       g_free, 
						       g_free);
	if (value != NULL)
		g_hash_table_insert (item->values,
				     g_strdup (key),
				     g_strdup (value));
	else
		g_hash_table_remove (item->values,
				     key);

	item->atime = time (NULL);

	gedit_metadata_manager->modified = TRUE;
}

static void
save_values (const gchar *key, const gchar *value, xmlNodePtr parent)
{
	xmlNodePtr xml_node;
	
	gedit_debug (DEBUG_METADATA, "");

	g_return_if_fail (key != NULL);
	
	if (value == NULL)
		return;
		
	xml_node = xmlNewChild (parent, NULL, "entry", NULL);

	xmlSetProp (xml_node, "key", key);
	xmlSetProp (xml_node, "value", value);

	gedit_debug (DEBUG_METADATA, "entry: %s = %s", key, value);
}

static void
save_item (const gchar *key, const gpointer *data, xmlNodePtr parent)
{	
	xmlNodePtr xml_node;
	const Item *item = (const Item *)data;
	gchar *atime;

	gedit_debug (DEBUG_METADATA, "");

	g_return_if_fail (key != NULL);
	
	if (item == NULL)
		return;
		
	xml_node = xmlNewChild (parent, NULL, "document", NULL);
	
	xmlSetProp (xml_node, "uri", key);

	gedit_debug (DEBUG_METADATA, "uri: %s", key);

	/* FIXME: is the cast right? - Paolo */
	atime = g_strdup_printf ("%d", (int)item->atime);
	xmlSetProp (xml_node, "atime", atime);	

	gedit_debug (DEBUG_METADATA, "atime: %s", atime);

	g_free (atime);

    	g_hash_table_foreach (item->values,
			      (GHFunc)save_values, xml_node); 
}

static void
get_oldest (const gchar *key, const gpointer value, const gchar ** key_to_remove)
{
	const Item *item = (const Item *)value;
	
	if (*key_to_remove == NULL)
	{
		*key_to_remove = key;
	}
	else
	{
		const Item *item_to_remove = 
			g_hash_table_lookup (gedit_metadata_manager->items,
					     *key_to_remove);

		g_return_if_fail (item_to_remove != NULL);

		if (item->atime < item_to_remove->atime)
		{
			*key_to_remove = key;
		}
	}	
}

static void
resize_items ()
{
	while (g_hash_table_size (gedit_metadata_manager->items) > MAX_ITEMS)
	{
		gpointer key_to_remove = NULL;

		g_hash_table_foreach (gedit_metadata_manager->items,
				      (GHFunc)get_oldest,
				      &key_to_remove);

		g_return_if_fail (key_to_remove != NULL);
		
		g_hash_table_remove (gedit_metadata_manager->items,
				     key_to_remove);
	}
}

static gboolean
gedit_metadata_manager_save (gpointer data)
{	
	xmlDocPtr  doc;
	xmlNodePtr root;
	gchar *file_name;

	gedit_debug (DEBUG_METADATA, "");
	
	if (!gedit_metadata_manager->modified)
		return TRUE;

	resize_items ();
		
	xmlIndentTreeOutput = TRUE;

	doc = xmlNewDoc ("1.0");
	if (doc == NULL)
		return TRUE;

	/* Create metadata root */
	root = xmlNewDocNode (doc, NULL, "metadata", NULL);
	xmlDocSetRootElement (doc, root);

    	g_hash_table_foreach (gedit_metadata_manager->items,
			  (GHFunc)save_item, root);        

	/* FIXME: lock file - Paolo */
	file_name = gnome_util_home_file (METADATA_FILE);
	xmlSaveFormatFile (file_name, doc, 1);
	g_free (file_name);
	
	xmlFreeDoc (doc); 

	gedit_metadata_manager->modified = FALSE;

	gedit_debug (DEBUG_METADATA, "DONE");

	return TRUE;
}

