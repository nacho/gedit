/*
 * gedit-osx-delegate.m
 * This file is part of gedit
 *
 * Copyright (C) 2009-2010 - Jesse van den Kieboom
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

#import "gedit-osx-delegate.h"
#import <Foundation/NSAppleEventManager.h>
#import <Foundation/NSAppleEventDescriptor.h>
#import <Foundation/NSData.h>
#include <glib.h>
#include <gedit/gedit-app.h>
#include <gedit/gedit-commands.h>

@implementation GeditOSXDelegate
-(id)init
{
	if ((self = [super init]))
	{
		NSAppleEventManager* em = [NSAppleEventManager sharedAppleEventManager];

	    [em setEventHandler:self
	            andSelector:@selector(openFiles:withReply:)
	          forEventClass:kCoreEventClass
	             andEventID:kAEOpenDocuments];
	}
	
	return self;
}

static GeditWindow *
get_window(NSAppleEventDescriptor *event)
{
	GeditApp *app = gedit_app_get_default ();
	return gedit_app_get_active_window (app);
}

- (void)openFiles:(NSAppleEventDescriptor*)event
        withReply:(NSAppleEventDescriptor*)reply
{
	NSAppleEventDescriptor *fileList = [event paramDescriptorForKeyword:keyDirectObject];
	NSInteger i;
	GSList *uris = NULL;
	
	if (!fileList)
	{
		return;
	}
	
	for (i = 1; i <= [fileList numberOfItems]; ++i)
	{
		NSAppleEventDescriptor *fileAliasDesc = [fileList descriptorAtIndex:i];
		NSAppleEventDescriptor *fileURLDesc;
		NSData *fileURLData;
		gchar *url;
		
		if (!fileAliasDesc)
		{
			continue;
		}
		
		fileURLDesc = [fileAliasDesc coerceToDescriptorType:typeFileURL];
		
		if (!fileURLDesc)
		{
			continue;
		}
		
		fileURLData = [fileURLDesc data];
		
		if (!fileURLData)
		{
			continue;
		}
		
		url = g_strndup([fileURLData bytes], [fileURLData length]);
		uris = g_slist_prepend (uris, url);
	}
	
	if (uris != NULL)
	{
		GeditWindow *window = get_window (event);
		gedit_commands_load_uris (window, uris, NULL, 0);

		g_slist_foreach (uris, (GFunc)g_free, NULL);
		g_slist_free (uris);
	}
}

@end
