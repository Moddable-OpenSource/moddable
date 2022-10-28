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

static xsSlot callback;

void PiuSystem_alert(xsMachine* the)
{
	int argc = xsToInteger(xsArgc);
	NSAlert *alert = [[[NSAlert alloc] init] autorelease];
	xsStringValue string;
	if ((argc > 0) && xsTest(xsArg(0))) {
		if (xsFindString(xsArg(0), xsID_type, &string)) {
			if (!c_strcmp(string, "about"))
				[alert setAlertStyle:NSAlertStyleInformational];
			else if (!c_strcmp(string, "stop"))
				[alert setAlertStyle:NSAlertStyleCritical];
			else if (!c_strcmp(string, "note"))
				[alert setAlertStyle:NSAlertStyleWarning];
		}
		if (xsFindString(xsArg(0), xsID_prompt, &string))
			[alert setMessageText:[NSString stringWithUTF8String:string]];
		if (xsFindString(xsArg(0), xsID_info, &string))
			[alert setInformativeText:[NSString stringWithUTF8String:string]];
		if (xsFindResult(xsArg(0), xsID_buttons)) {
			if (xsIsInstanceOf(xsResult, xsArrayPrototype)) {
				xsIntegerValue c = xsToInteger(xsGet(xsResult, xsID_length)), i;
				for (i = 0; i < c; i++) {
					string = xsToString(xsGetIndex(xsResult, i));
					[alert addButtonWithTitle:[NSString stringWithUTF8String:string]];
				}
			}
		}
	}
	if ((argc > 1) && xsTest(xsArg(1)))
		callback = xsArg(1);
	else
		callback = xsNull;
	xsRemember(callback);
	[alert beginSheetModalForWindow:[NSApp mainWindow] completionHandler:^(NSInteger result) {
		[alert.window close]; 
		xsBeginHost(the);
		{
			xsVars(1);
			{
				xsTry {
					xsResult = xsAccess(callback);
					xsForget(callback);
					callback = xsNull;
					if (xsTest(xsResult)) {
						xsVar(0) = (result == 1000) ? xsTrue : (result == 1001) ? xsUndefined : xsFalse;
						(void)xsCallFunction1(xsResult, xsNull, xsVar(0));
					}
				}
				xsCatch {
				}
			}
		}
		xsEndHost(the);
	}];
}

@class PiuSystem_open_delegate;
@interface PiuSystem_open_delegate : NSObject <NSOpenSavePanelDelegate> {
    NSString *_name;
}
- (instancetype)initWith:(NSString *)name;
- (void)dealloc;
- (BOOL)panel:(id)sender shouldEnableURL:(NSURL *)url;
@end
@implementation PiuSystem_open_delegate {
}
- (instancetype)initWith:(NSString *)name {
	self = [super init];
	if (self) {
		_name = [name retain];
	}
	return self;
}
- (void)dealloc {
    [_name release];
    [super dealloc];
}
- (BOOL)panel:(id)sender shouldEnableURL:(NSURL *)url {
    NSNumber *isDirectory;
    [url getResourceValue:&isDirectory forKey:NSURLIsDirectoryKey error:nil];
	return [isDirectory boolValue] || [_name isEqualToString:[url lastPathComponent]];
}
@end

static void PiuSystem_open(xsMachine* the, xsBooleanValue flag)
{
	int argc = xsToInteger(xsArgc);
	NSOpenPanel *openPanel = [NSOpenPanel openPanel];
	xsStringValue string;
	if ((argc > 0) && xsTest(xsArg(0))) {
// 		if (xsFindString(xsArg(0), xsID_fileType, &string))
// 			[openPanel setAllowedContentTypes: [NSArray arrayWithObject:[NSString stringWithUTF8String:string]]];
		if (xsFindString(xsArg(0), xsID_message, &string))
			[openPanel setMessage:[NSString stringWithUTF8String:string]];
		if (xsFindString(xsArg(0), xsID_name, &string))
			[openPanel setDelegate:[[PiuSystem_open_delegate alloc] initWith:[NSString stringWithUTF8String:string]]];
		if (xsFindString(xsArg(0), xsID_prompt, &string))
			[openPanel setPrompt:[NSString stringWithUTF8String:string]];
	}
	[openPanel setAllowsMultipleSelection:NO];
	[openPanel setCanChooseDirectories:flag ? YES : NO];
	[openPanel setCanChooseFiles:flag ? NO : YES];
	if ((argc > 1) && xsTest(xsArg(1)))
		callback = xsArg(1);
	else
		callback = xsNull;
	xsRemember(callback);
	[openPanel beginSheetModalForWindow:[NSApp mainWindow] completionHandler:^(NSInteger result) {		
		xsBeginHost(the);
		{
			xsVars(1);
			{
				xsTry {
					xsResult = xsAccess(callback);
					xsForget(callback);
					callback = xsNull;
					if (xsTest(xsResult)) {
						if (result)
							xsVar(0) = xsString((xsStringValue)[[[openPanel URLs] objectAtIndex:0] fileSystemRepresentation]);
						(void)xsCallFunction1(xsResult, xsNull, xsVar(0));
					}
				}
				xsCatch {
				}
			}
		}
		xsEndHost(the);
		id delegate = [openPanel delegate];
		if (delegate)
			[delegate release];
	}];
}

void PiuSystem_openDirectory(xsMachine* the)
{
	PiuSystem_open(the, 1);
}

void PiuSystem_openFile(xsMachine* the)
{
	PiuSystem_open(the, 0);
}

void PiuSystem_save(xsMachine* the, xsBooleanValue flag)
{
	xsIntegerValue argc = xsToInteger(xsArgc);
	NSSavePanel* savePanel = [NSSavePanel savePanel];
	xsStringValue string;
	if ((argc > 0) && xsTest(xsArg(0))) {
		if (xsFindString(xsArg(0), xsID_name, &string))
			[savePanel setNameFieldStringValue:[NSString stringWithUTF8String:string]];
		if (xsFindString(xsArg(0), xsID_prompt, &string))
			[savePanel setPrompt:[NSString stringWithUTF8String:string]];
	}
	if ((argc > 1) && xsTest(xsArg(1)))
		callback = xsArg(1);
	else
		callback = xsNull;
	xsRemember(callback);
	[savePanel beginSheetModalForWindow:[NSApp mainWindow] completionHandler:^(NSInteger result) { 
		xsBeginHost(the);
		{
			xsVars(1);
			{
				xsTry {
					xsResult = xsAccess(callback);
					xsForget(callback);
					callback = xsNull;
					if (xsTest(xsResult)) {
						if (result)
							xsVar(0) = xsString((xsStringValue)[[savePanel URL] fileSystemRepresentation]);
						(void)xsCallFunction1(xsResult, xsNull, xsVar(0));
					}
				}
				xsCatch {
				}
			}
		}
		xsEndHost(the);
	}];
}


void PiuSystem_saveDirectory(xsMachine* the)
{
	PiuSystem_save(the, 1);
}

void PiuSystem_saveFile(xsMachine* the)
{
	PiuSystem_save(the, 0);
}
