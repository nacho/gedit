/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * gedit-taglist-plugin-parser.c
 * This file is part of the gedit taglist plugin
 *
 * Copyright (C) 2002 Paolo Maggi 
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
 * Modified by the gedit Team, 2002. See the AUTHORS file for a 
 * list of people on the gedit Team.  
 * See the ChangeLog files for a list of changes. 
 */

#include <libxml/parser.h>
#include <glib.h>

#include <gedit-debug.h>

#include "gedit-taglist-plugin-parser.h"


TagList *taglist = NULL;

static gboolean	 parse_tag (Tag *tag, xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur);
static gboolean	 parse_tag_group (TagGroup *tg, const gchar *fn, 
				  xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur);
static TagList 	*parse_taglist_file (const gchar* filename);

static void	 free_tag (Tag *tag);
static void	 free_tag_group (TagGroup *tag_group);

static gboolean
parse_tag (Tag *tag, xmlDocPtr doc, xmlNsPtr ns, xmlNodePtr cur) 
{
	gedit_debug (DEBUG_PLUGINS, "  Tag name: %s", tag->name);

	/* We don't care what the top level element name is */
	cur = cur->xmlChildrenNode;
    
	while (cur != NULL) 
	{
		if ((!xmlStrcmp (cur->name, (const xmlChar *)"Begin")) &&
		    (cur->ns == ns))
		{			
			tag->begin = xmlNodeListGetString (doc, cur->xmlChildrenNode, 1);

			gedit_debug (DEBUG_PLUGINS, "    - Begin: %s", tag->begin);
		}

		if ((!xmlStrcmp (cur->name, (const xmlChar *)"End")) &&
		    (cur->ns == ns))			
		{
			tag->end = xmlNodeListGetString (doc, cur->xmlChildrenNode, 1);

			gedit_debug (DEBUG_PLUGINS, "    - End: %s", tag->end);
		}

		cur = cur->next;
	}

	if ((tag->begin == NULL) && (tag->end = NULL))
		return FALSE;

	return TRUE;
}

static gboolean
parse_tag_group (TagGroup *tg, const gchar* fn, xmlDocPtr doc, 
		 xmlNsPtr ns, xmlNodePtr cur) 
{
	gedit_debug (DEBUG_PLUGINS, "Parse TagGroup: %s", tg->name);

	/* We don't care what the top level element name is */
    	cur = cur->xmlChildrenNode;
    
	while (cur != NULL) 
	{
		if ((xmlStrcmp (cur->name, (const xmlChar *) "Tag")) || (cur->ns != ns)) 
		{
			g_warning ("The tag list file '%s' is of the wrong type, "
				   "was '%s', 'Tag' expected.", fn, cur->name);
				
			return FALSE;
		}
		else
		{
			Tag *tag;

			tag = g_new0 (Tag, 1);

			/* Get Tag name */
			tag->name = xmlGetProp (cur, (const xmlChar *) "name");

			if (tag->name == NULL)
			{
				/* Error: No name */
				g_warning ("The tag list file '%s' is of the wrong type, "
				   "Tag without name.", fn);

				g_free (tag);

				return FALSE;
			}
			else
			{
				/* Parse Tag */
				if (parse_tag (tag, doc, ns, cur))
				{
					/* Append Tag to TagGroup */
					tg->tags = g_list_append (tg->tags, tag);
				}
				else
				{
					/* Error parsing Tag */
					g_warning ("The tag list file '%s' is of the wrong type, "
			   		   	   "error parsing Tag '%s' in TagGroup '%s'.", 
					   	   fn, tag->name, tg->name);
	
					free_tag (tag);
					
					return FALSE;
				}
			}
		}

		cur = cur->next;		
	}

	return TRUE;
}


static TagList *
parse_taglist_file (const gchar* filename)
{
	xmlDocPtr doc;
	
	xmlNsPtr ns;
	xmlNodePtr cur;

	gedit_debug (DEBUG_PLUGINS, "Parse file: %s", filename);

	/*
	* build an XML tree from a the file;
	*/
	doc = xmlParseFile (filename);
	if (doc == NULL) 
		return taglist;

	/*
	* Check the document is of the right kind
	*/
    
	cur = xmlDocGetRootElement (doc);

	if (cur == NULL) 
	{
		g_warning ("The tag list file '%s' is empty.", filename);		
		xmlFreeDoc(doc);		
		return taglist;
	}

	ns = xmlSearchNsByHref (doc, cur,
			(const xmlChar *) "http://gedit.sourceforge.net/some-location");

	if (ns == NULL) 
	{
		g_warning ("The tag list file '%s' is of the wrong type, "
			   "gedit namespace not found.", filename);
		xmlFreeDoc (doc);
		
		return taglist;
	}

    	if (xmlStrcmp(cur->name, (const xmlChar *) "TagList")) 
	{
		g_warning ("The tag list file '%s' is of the wrong type, "
			   "root node != TagList.", filename);
		xmlFreeDoc (doc);
		
		return taglist;
	}

	/* 
	 * If needed, allocate taglist
	 */

	if (taglist == NULL)
		taglist = g_new0 (TagList, 1);
	
	/*
	 * Walk the tree.
	 *
	 * First level we expect a list TagGroup 
	 */
	cur = cur->xmlChildrenNode;
	
	while (cur != NULL)
     	{ 	
		if ((xmlStrcmp (cur->name, (const xmlChar *) "TagGroup")) || (cur->ns != ns)) 
		{
			g_warning ("The tag list file '%s' is of the wrong type, "
				   "was '%s', 'TagGroup' expected.", filename, cur->name);
			xmlFreeDoc (doc);
		
			return taglist;
		}
		else
		{
			TagGroup *tag_group;

			tag_group = g_new0 (TagGroup, 1);

			/* Get TagGroup name */
			tag_group->name = xmlGetProp (cur, (const xmlChar *) "name");

			if (tag_group->name == NULL)
			{
				/* Error: No name */
				g_warning ("The tag list file '%s' is of the wrong type, "
				   "TagGroup without name.", filename);

				g_free (tag_group);
			}
			else
			{
				/* Name found */
				
				/* Parse tag group */
				if (parse_tag_group (tag_group, filename, doc, ns, cur))
				{
					/* Append TagGroup to TagList */
					taglist->tag_groups = 
						g_list_append (taglist->tag_groups, tag_group);
				}
				else
				{
					/* Error parsing TagGroup */
					g_warning ("The tag list file '%s' is of the wrong type, "
				   		   "error parsing TagGroup '%s'.", 
						   filename, tag_group->name);

					free_tag_group (tag_group);
				}
			}
		}
	
		cur = cur->next;
    	}

	xmlFreeDoc (doc);

	gedit_debug (DEBUG_PLUGINS, "END");

	return taglist;
}


static void
free_tag (Tag *tag)
{
	gedit_debug (DEBUG_PLUGINS, "Tag: %s", tag->name);

	g_return_if_fail (tag != NULL);

	free (tag->name);

	if (tag->begin != NULL)
		free (tag->begin);

	if (tag->end != NULL)
		free (tag->end);

	g_free (tag);
}

static void
free_tag_group (TagGroup *tag_group)
{
	gedit_debug (DEBUG_PLUGINS, "Tag group: %s", tag_group->name);

	g_return_if_fail (tag_group != NULL);

	free (tag_group->name);

	while (tag_group->tags)
	{
		free_tag ((Tag *)tag_group->tags->data);

		tag_group->tags = g_list_next (tag_group->tags);
	}

	g_list_free (tag_group->tags);
	
	g_free (tag_group);

	gedit_debug (DEBUG_PLUGINS, "END");
}

void
free_taglist (void)
{
	gedit_debug (DEBUG_PLUGINS, "");

	if (taglist == NULL)
		return;
	
	while (taglist->tag_groups)
	{
		free_tag_group ((TagGroup *)taglist->tag_groups->data);

		taglist->tag_groups = g_list_next (taglist->tag_groups);
	}

	g_list_free (taglist->tag_groups);
	
	g_free (taglist);

	taglist = NULL;

	gedit_debug (DEBUG_PLUGINS, "END");
}


TagList* create_taglist (void)
{
	gedit_debug (DEBUG_PLUGINS, "");

	g_return_val_if_fail (taglist == NULL, taglist);

	xmlKeepBlanksDefault (0);

	/* FIXME: search user's taglists */	
	return parse_taglist_file (GEDIT_TAGLIST);
}
