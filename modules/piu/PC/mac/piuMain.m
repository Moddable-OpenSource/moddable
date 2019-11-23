/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Runtime.
 * 
 *   The Moddable SDK Runtime is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 * 
 *   The Moddable SDK Runtime is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 * 
 *   You should have received a copy of the GNU Lesser General Public License
 *   along with the Moddable SDK Runtime.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "piuPC.h"

int main(int argc, const char **argv)
{
    @autoreleasepool {
		NSApplication *application = [NSApplication sharedApplication];
		NSPiuAppDelegate *appDelegate =  [NSPiuAppDelegate new];
		[application setDelegate:(id<NSApplicationDelegate>)appDelegate];
	
		appDelegate.shouldQuit = FALSE;
		appDelegate.machine = ServiceThreadMain(NULL);
	
		xsBeginHost(appDelegate.machine);
		{
			xsCollectGarbage();
			xsResult = xsAwaitImport("main", XS_IMPORT_DEFAULT);
			appDelegate.piuApplication = PIU(Application, xsResult);
			xsCollectGarbage();
		}
		xsEndHost();
		
		PiuView* view = (*appDelegate.piuApplication)->view;
    	[(*view)->nsWindow makeKeyAndOrderFront:NSApp];
    
		[application run];
    }
	return 0;
}

void fxAbort(xsMachine *the, int status)
{
	NSApplication* application = [NSApplication sharedApplication];
	NSPiuAppDelegate *appDelegate = [application delegate];
	appDelegate.shouldQuit = TRUE;
	[application terminate:NULL];
}

void PiuApplication_createMenus(xsMachine *the)
{
}

void PiuApplication_get_title(xsMachine *the)
{
	PiuApplication* self = PIU(Application, xsThis);
	PiuView* view = (*self)->view;
	NSString* string = (*view)->nsWindow.title;
	xsResult = xsString([string UTF8String]);
}

void PiuApplication_set_title(xsMachine *the)
{
	PiuApplication* self = PIU(Application, xsThis);
	PiuView* view = (*self)->view;
	xsStringValue string = xsToString(xsArg(0));
	(*view)->nsWindow.title = [NSString stringWithUTF8String:string];
}

void PiuApplication_gotoFront(xsMachine *the)
{
	NSApplication* application = [NSApplication sharedApplication];
	[application activateIgnoringOtherApps:YES];
}

void PiuApplication_purge(xsMachine* the)
{
	xsCollectGarbage();
}

void PiuApplication_quit(xsMachine *the)
{
	NSPiuAppDelegate *appDelegate = [[NSApplication sharedApplication] delegate];
	appDelegate.shouldQuit = TRUE;
}

void PiuApplication_updateMenus(xsMachine *the)
{
}

int mcCountResources(xsMachine* the)
{
	return 0;
}

const char* mcGetResourceName(xsMachine* the, int i)
{
	return NULL;
}

const void *mcGetResource(xsMachine* the, const char* path, size_t* size)
{
	*size = 0;
	return NULL;\
}
