/*
 * gedit-osx-delegate.h
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

#ifndef GEDIT_OSX_DELEGATE_H_
#define GEDIT_OSX_DELEGATE_H_

#import <Foundation/NSAppleEventManager.h>

@interface GeditOSXDelegate : NSObject
{
}

-(id) init;
-(void) openFiles:(NSAppleEventDescriptor*)event
        withReply:(NSAppleEventDescriptor*)reply;

@end

#endif /* GEDIT_OSX_DELEGATE_H_ */
/* ex:set ts=8 noet: */
