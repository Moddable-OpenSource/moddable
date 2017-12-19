/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Tools.
 * 
 *   The Moddable SDK Tools is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 * 
 *   The Moddable SDK Tools is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 * 
 *   You should have received a copy of the GNU General Public License
 *   along with the Moddable SDK Tools.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#import <Cocoa/Cocoa.h>
#include <dlfcn.h>
#include "screen.h"

NSString* gPixelFormatNames[pixelFormatCount] = {
	@"16-bit RGB 565 Little Endian",
	@"16-bit RGB 565 Big Endian",
	@"8-bit Gray",
	@"8-bit RGB 332",
	@"4-bit Gray",
	@"4-bit Color Look-up Table",
};

@interface TouchFinger : NSObject {
	id identity;
	NSPoint point;
}
@property (retain) id identity;
@property (assign) NSPoint point;
@end

@implementation TouchFinger
@synthesize identity;
@synthesize point;
- (void)dealloc {
    [identity release];
    [super dealloc];
}
@end

@interface CustomWindow : NSWindow {
    NSPoint initialLocation;
}
@property (assign) NSPoint initialLocation;
@end

@interface CustomView : NSView {
    NSImage *screenImage;
}
@property (retain) NSImage *screenImage;
@end

@interface ScreenView : NSView {
	NSString *filename;
	void* library;
	txScreen* screen;
    NSTimeInterval time;
	NSTimer *timer;
    NSImage *touchImage;
	id *touches;
	BOOL touching;
}
@property (retain) NSString *filename;
@property (assign) void* library;
@property (assign) txScreen *screen;
@property (assign) NSTimeInterval time;
@property (assign) NSTimer *timer;
@property (retain) NSImage *touchImage;
@property (assign) id *touches;
@property (assign) BOOL touching;
- (void)launchMachine:(NSString*)string;
- (void)quitMachine;
@end

@interface AppDelegate : NSObject <NSApplicationDelegate> {
	ScreenView *screenView;
	NSWindow *window;
}
@property (retain) ScreenView *screenView;
@property (retain) NSWindow *window;
@end

static void fxScreenAbort(txScreen* screen);
static void fxScreenBufferChanged(txScreen* screen);
static void fxScreenFormatChanged(txScreen* screen);
static void fxScreenStart(txScreen* screen, double interval);
static void fxScreenStop(txScreen* screen);

@implementation AppDelegate
@synthesize screenView;
@synthesize window;
- (void)dealloc {
    [screenView release];
    [window release];
    [super dealloc];
}
- (void) applicationWillFinishLaunching:(NSNotification *)notification {
	NSMenu* menubar = [[NSMenu new] autorelease];
	NSMenu *servicesMenu = [[NSMenu new] autorelease];
	NSMenuItem* item;

	NSMenu* applicationMenu = [[NSMenu new] autorelease];
	item = [[[NSMenuItem alloc] initWithTitle:@"About Screen Test" action:@selector(about:) keyEquivalent:@""] autorelease];
	[applicationMenu addItem:item];
    [applicationMenu addItem:[NSMenuItem separatorItem]];
	item = [[[NSMenuItem alloc] initWithTitle:@"Touch Mode"  action:@selector(toggleTouchMode:) keyEquivalent:@"t"] autorelease];
	[applicationMenu addItem:item];
    [applicationMenu addItem:[NSMenuItem separatorItem]];
	item = [[[NSMenuItem alloc] initWithTitle:@"Service" action:NULL keyEquivalent:@""] autorelease];
	[item setSubmenu:servicesMenu];
  	[applicationMenu addItem:item];
	item = [[[NSMenuItem alloc] initWithTitle:@"Hide Screen Test" action:@selector(hide:) keyEquivalent:@"h"] autorelease];
  	[applicationMenu addItem:item];
	item = [[[NSMenuItem alloc] initWithTitle:@"Hide Others" action:@selector(hideOtherApplications:) keyEquivalent:@"h"] autorelease];
	[item setKeyEquivalentModifierMask:NSEventModifierFlagOption];
  	[applicationMenu addItem:item];
	item = [[[NSMenuItem alloc] initWithTitle:@"Show All" action:@selector(unhideAllApplications:) keyEquivalent:@""] autorelease];
  	[applicationMenu addItem:item];
    [applicationMenu addItem:[NSMenuItem separatorItem]];
	item = [[[NSMenuItem alloc] initWithTitle:@"Quit Screen Test"  action:@selector(terminate:) keyEquivalent:@"q"] autorelease];
	[applicationMenu addItem:item];
	item = [[NSMenuItem new] autorelease];
	[item setSubmenu:applicationMenu];
	[menubar addItem:item];
	
	NSMenu* fileMenu = [[[NSMenu alloc] initWithTitle:@"File"] autorelease];
	item = [[[NSMenuItem alloc] initWithTitle:@"Open..." action:@selector(openLibrary:) keyEquivalent:@"o"] autorelease];
	[fileMenu addItem:item];
	item = [[[NSMenuItem alloc] initWithTitle:@"Close" action:@selector(closeLibrary:) keyEquivalent:@"w"] autorelease];
	[fileMenu addItem:item];
    [fileMenu addItem:[NSMenuItem separatorItem]];
	item = [[[NSMenuItem alloc] initWithTitle:@"Get Info"  action:@selector(getInfo:) keyEquivalent:@"i"] autorelease];
	[fileMenu addItem:item];
	item = [[NSMenuItem new] autorelease];
	[item setSubmenu:fileMenu];
	[menubar addItem:item];

    NSMenu* screenMenu = [[[NSMenu alloc] initWithTitle:@"Size"] autorelease];
	NSArray *paths = [[NSBundle mainBundle] pathsForResourcesOfType:@"json" inDirectory:@"screens"];
	NSUInteger c = [paths count], i;
	for (i = 0; i < c; i++) {
		NSString *path = [paths objectAtIndex:i];
		NSData *data = [[NSData alloc] initWithContentsOfFile:path];
		NSDictionary *json = [NSJSONSerialization JSONObjectWithData:data options:0 error:NULL];
    	NSString* title = [json valueForKeyPath:@"title"];
		item = [[[NSMenuItem alloc] initWithTitle:title action:@selector(selectScreen:) keyEquivalent:@""] autorelease];
        item.representedObject = path;
        item.tag = i;
		[screenMenu addItem:item];
	}
	item = [[NSMenuItem new] autorelease];
	[item setSubmenu:screenMenu];
	[menubar addItem:item];
	
	NSMenu* helpMenu = [[[NSMenu alloc] initWithTitle:@"Help"] autorelease];
	item = [[[NSMenuItem alloc] initWithTitle:@"Moddable Developer" action:@selector(support:) keyEquivalent:@""] autorelease];
	[helpMenu addItem:item];
	item = [[NSMenuItem new] autorelease];
	[item setSubmenu:helpMenu];
	[menubar addItem:item];
	
	[NSApp setMainMenu:menubar];
	[NSApp setServicesMenu: servicesMenu];
	
    NSUserDefaults *userDefaults = [NSUserDefaults standardUserDefaults];
    i = [userDefaults integerForKey:@"screenTag"];
    item = [screenMenu itemWithTag:i];
    item.state = 1;
    [self createScreen:item.representedObject];
}
- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    [self.window makeKeyAndOrderFront:NSApp];
}
- (void)application:(NSApplication *)application openFiles:(NSArray *)filenames
{
	NSInteger c = [filenames count], i;
	NSString* filename;
	for (i = 0; i < c; i++) {
		[self.screenView quitMachine];
		filename = [filenames objectAtIndex:i];
        [self.screenView launchMachine:filename];
	}
}
- (void)about:(NSMenuItem *)sender {
	NSAlert *alert = [[[NSAlert alloc] init] autorelease];
	[alert setAlertStyle:NSAlertStyleInformational];
	[alert setMessageText:@"Screen Test"];
	[alert setInformativeText:@"Copyright 2017 Moddable Tech, Inc.\nAll rights reserved.\n\nThis application incorporates open source software from Marvell, Inc. and others."];
	[alert beginSheetModalForWindow:window completionHandler:^(NSInteger result) {
		[alert.window close]; 
	}];
}
- (void)closeLibrary:(NSMenuItem *)sender {
	[self.screenView quitMachine];
}
- (void)createScreen:(NSString *)jsonPath {
    NSString *screenImagePath = [[jsonPath stringByDeletingPathExtension] stringByAppendingPathExtension:@"png"];
    NSImage *screenImage = [[NSImage alloc] initByReferencingFile:screenImagePath];
    NSSize size = [screenImage size];
    NSRect contentRect = NSMakeRect(0, 0, size.width, size.height);
    CustomWindow* customWindow = [[CustomWindow alloc] initWithContentRect:contentRect styleMask:NSBorderlessWindowMask backing:NSBackingStoreBuffered defer:NO];
    CustomView *customView = [[[CustomView alloc] initWithFrame:contentRect] autorelease];
    NSData *data = [[NSData alloc] initWithContentsOfFile:jsonPath];
    NSDictionary *json = [NSJSONSerialization JSONObjectWithData:data options:0 error:NULL];
    NSInteger x = [[json valueForKeyPath:@"x"] integerValue];
    NSInteger y = [[json valueForKeyPath:@"y"] integerValue];
    NSInteger width = [[json valueForKeyPath:@"width"] integerValue];
    NSInteger height = [[json valueForKeyPath:@"height"] integerValue];
    ScreenView *_screenView = [[[ScreenView alloc] initWithFrame:NSMakeRect(x, size.height - (y + height), width, height)] autorelease];
    
    txScreen* screen = malloc(sizeof(txScreen) - 1 + (width * height * screenBytesPerPixel));
    memset(screen, 0, sizeof(txScreen) - 1 + (width * height * screenBytesPerPixel));
    screen->view = _screenView;
    screen->abort = fxScreenAbort;
    screen->bufferChanged = fxScreenBufferChanged;
    screen->formatChanged = fxScreenFormatChanged;
    screen->start = fxScreenStart;
    screen->stop = fxScreenStop;
    screen->width = width;
    screen->height = height;
    _screenView.screen = screen;
    
    NSProcessInfo* processInfo = [NSProcessInfo processInfo];
    NSTimeInterval time = [processInfo systemUptime];
    NSDate* date = [NSDate dateWithTimeIntervalSinceNow:-time];
    _screenView.time = [date timeIntervalSince1970];
    _screenView.timer = nil;
  
    _screenView.touchImage = [NSImage imageNamed:@"fingerprint"];
	_screenView.touches = (id *)malloc(10 * sizeof(id));
    int i;
    for (i = 0; i < 10; i++)
		_screenView.touches[i] = nil;
	 _screenView.touching = NO;
	 
    customView.screenImage = screenImage;
    [customView addSubview:_screenView];
    [customWindow setContentView:customView];
    [customWindow setAlphaValue:1.0];
    [customWindow setOpaque:NO];
    if (![customWindow setFrameUsingName:@"screen"])
        [customWindow cascadeTopLeftFromPoint:NSMakePoint(20, 20)];
    [customWindow registerForDraggedTypes:[NSArray arrayWithObjects:NSFilenamesPboardType,nil]];
	self.screenView = _screenView;
    self.window = customWindow;
}
- (void)deleteScreen {
    [self.window close];
}
- (void)getInfo:(NSMenuItem *)sender {
	NSAlert *alert = [[[NSAlert alloc] init] autorelease];
	[alert setAlertStyle:NSAlertStyleInformational];
	[alert setMessageText:[[screenView.filename stringByDeletingLastPathComponent] lastPathComponent]];
	[alert setInformativeText:gPixelFormatNames[screenView.screen->pixelFormat]];
	[alert beginSheetModalForWindow:window completionHandler:^(NSInteger result) {
		[alert.window close]; 
	}];
}
- (void)openLibrary:(NSMenuItem *)sender {
	NSOpenPanel *openPanel = [NSOpenPanel openPanel];
	[openPanel setAllowedFileTypes: [NSArray arrayWithObject:@"so"]];
	[openPanel setAllowsMultipleSelection:NO];
	[openPanel setCanChooseDirectories:NO];
	[openPanel setCanChooseFiles:YES];
	[openPanel beginSheetModalForWindow:[NSApp mainWindow] completionHandler:^(NSInteger result) {		
		NSArray *urls = [openPanel URLs];
		NSMutableArray *filenames = [NSMutableArray arrayWithCapacity:[urls count]];
		[urls enumerateObjectsUsingBlock:^(id obj, NSUInteger idx, BOOL *stop) {
			[filenames addObject:[obj path]];
		}];	
    	[self application:[NSApplication sharedApplication] openFiles:filenames];
	}];
}
- (void)selectScreen:(NSMenuItem *)sender {
    if (sender.state)
        return;
    NSUserDefaults *userDefaults = [NSUserDefaults standardUserDefaults];
    NSInteger screenTag = [userDefaults integerForKey:@"screenTag"];
    NSMenu *menu = [sender menu];
    NSMenuItem *current = [menu itemWithTag:screenTag];
    current.state = 0;
    sender.state = 1;
    [userDefaults setInteger:sender.tag forKey:@"screenTag"];
    
    NSString *_filename = self.screenView.filename;
    if (_filename) {
		[self.screenView quitMachine];
    }
    [self deleteScreen];
    [self createScreen:sender.representedObject];
    if (_filename) {
		[self.screenView launchMachine:_filename];
    }
    [self.window makeKeyAndOrderFront:NSApp];
}
- (void)support:(NSMenuItem *)sender {
	[[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:@"http://moddable.tech"]];
}
- (void)toggleTouchMode:(NSMenuItem *)sender {
    if (sender.state) {
		CGAssociateMouseAndMouseCursorPosition(true);
		CGDisplayShowCursor(kCGDirectMainDisplay);
    	screenView.touching = NO;
    	[screenView setAcceptsTouchEvents:NO];
    	screenView.wantsRestingTouches = NO;
    	sender.state = 0;
    }
    else {
		NSRect frame = [window frame];
		CGPoint point;
		point.x = frame.origin.x + (frame.size.width / 2);
		point.y = [[window screen] frame].size.height - (frame.origin.y + (frame.size.height / 2));
		CGAssociateMouseAndMouseCursorPosition(false);
		CGDisplayMoveCursorToPoint(kCGDirectMainDisplay, point);
		CGDisplayHideCursor(kCGDirectMainDisplay);
    	screenView.touching = YES;
    	[screenView setAcceptsTouchEvents:YES];
        screenView.wantsRestingTouches = YES;
     	sender.state = 1;
    }
}
- (BOOL)validateMenuItem:(NSMenuItem *)item {
    if ((item.action == @selector(closeLibrary:)) || (item.action == @selector(getInfo:))) {
		return (screenView.library) ? YES : NO;
    }
    return YES;
}
@end

@implementation CustomWindow
@synthesize initialLocation;
- (BOOL)canBecomeKeyWindow {
    return YES;
}
- (void)mouseDown:(NSEvent *)theEvent {
    self.initialLocation = [theEvent locationInWindow];
}
- (void)mouseDragged:(NSEvent *)theEvent {
    NSRect windowFrame = [self frame];
    NSPoint newOrigin = windowFrame.origin;
    NSPoint currentLocation = [theEvent locationInWindow];
    newOrigin.x += (currentLocation.x - initialLocation.x);
    newOrigin.y += (currentLocation.y - initialLocation.y);
    [self setFrameOrigin:newOrigin];
}
- (void)mouseUp:(NSEvent *)theEvent {
	[self saveFrameUsingName:@"screen"];
}
- (NSDragOperation)draggingEntered:(id )sender
{
    return NSDragOperationGeneric;
}
- (NSDragOperation)draggingUpdated:(id )sender
{
    return NSDragOperationGeneric;
}
- (BOOL)prepareForDragOperation:(id )sender
{
    return YES;
}
- (BOOL)performDragOperation:(id )sender {
	NSPasteboard *pasteboard = [sender draggingPasteboard];
	NSArray *filenames = [pasteboard propertyListForType:@"NSFilenamesPboardType"];
	NSApplication* application = [NSApplication sharedApplication];
    [[application delegate] application:application openFiles:filenames];
	return YES;
}
- (void)concludeDragOperation:(id )sender
{
}
@end

@implementation CustomView
@synthesize screenImage;
- (void)dealloc {
	[screenImage release];
    [super dealloc];
}
- (void)drawRect:(NSRect)rect {
    [[NSColor clearColor] set];
    NSRectFill([self frame]);
    [screenImage drawAtPoint:NSZeroPoint fromRect:NSZeroRect operation:NSCompositeSourceOver fraction:1.0];
}
@end

@implementation ScreenView
@synthesize filename;
@synthesize library;
@synthesize screen;
@synthesize time;
@synthesize timer;
@synthesize touchImage;
@synthesize touches;
@synthesize touching;
- (void)dealloc {
	if (screen)
    	free(screen);
	if (library)
    	dlclose(library);
    [filename release];
    [touchImage release];
    [super dealloc];
}
- (void)drawRect:(NSRect)rect {
	CGRect bounds = NSRectToCGRect(self.bounds);
	CGContextRef context = (CGContextRef)[[NSGraphicsContext currentContext] graphicsPort];
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
	CGDataProviderRef provider = CGDataProviderCreateWithData(nil, screen->buffer, screen->width * screen->height * screenBytesPerPixel, nil);
    CGImageRef image = CGImageCreate(screen->width, screen->height, 8, 32, screen->width * screenBytesPerPixel, colorSpace, kCGBitmapByteOrder32Big, provider, nil, NO, kCGRenderingIntentDefault);
	CGContextDrawImage(context, bounds, image);
	CGImageRelease(image);
	CGDataProviderRelease(provider);
	CGColorSpaceRelease(colorSpace);
	if (touching) {
		int i;
		for (i = 0; i < 10;  i++) {
			TouchFinger *finger = touches[i];
			if (finger) {
				[touchImage drawAtPoint:finger.point fromRect:NSZeroRect operation:NSCompositeSourceOver fraction:1.0];
			}
		}
	}
}
- (void)launchMachine:(NSString*)path {
	NSString *info = nil;
	txScreenLaunchProc launch;
	self.filename = path;
	self.library = dlopen([path UTF8String], RTLD_NOW);
	if (!self.library) {
		info = [NSString stringWithFormat:@"%s", dlerror()];
		goto bail;
	}
	launch = (txScreenLaunchProc)dlsym(self.library, "fxScreenLaunch");
	if (!launch) {
		info = [NSString stringWithFormat:@"%s", dlerror()];
		goto bail;
	}
	(*launch)(self.screen);
	return;
bail:
	if (self.library) {
		dlclose(self.library);
		self.library = nil;
	}
	NSAlert *alert = [[[NSAlert alloc] init] autorelease];
	[alert setMessageText:[NSString stringWithFormat:@"Cannot open \"%@\"", path]];
	if (info)
		[alert setInformativeText:info];
	[alert runModal];
}
- (void)mouseDown:(NSEvent *)event {
	if (touching)
		return;
	NSTimeInterval when = 1000 * (time + [event timestamp]);
	NSPoint point = [event locationInWindow];
	point = [self convertPoint:point fromView:nil];
	point.y = [self frame].size.height - point.y;
	if (self.screen->touch) 
		(*self.screen->touch)(self.screen, touchEventBeganKind, 0, point.x, point.y, when);
}
- (void)mouseDragged:(NSEvent *)event {
	if (touching)
		return;
	NSTimeInterval when = 1000 * (time + [event timestamp]);
	NSPoint point = [event locationInWindow];
	point = [self convertPoint:point fromView:nil];
	point.y = [self frame].size.height - point.y;
	if (self.screen->touch) 
		(*self.screen->touch)(self.screen, touchEventMovedKind, 0, point.x, point.y, when);
}
- (void)mouseUp:(NSEvent *)event {
	if (touching)
		return;
	NSTimeInterval when = 1000 * (time + [event timestamp]);
	NSPoint point = [event locationInWindow];
	point = [self convertPoint:point fromView:nil];
	point.y = [self frame].size.height - point.y;
	if (self.screen->touch) 
		(*self.screen->touch)(self.screen, touchEventEndedKind, 0, point.x, point.y, when);
}
- (void)quitMachine {
	if (self.screen->quit) 
		(*self.screen->quit)(self.screen);
	if (self.library) {
    	dlclose(self.library);
    	self.library = nil;
    }
    [self display];
}
- (void)timerCallback:(NSTimer*)theTimer {
	if (self.screen->idle) 
		(*self.screen->idle)(self.screen);
}
- (void)touchesBeganWithEvent:(NSEvent *)event {
	if (!touching)
		return;
    NSSet *set = [event touchesMatchingPhase:NSTouchPhaseBegan inView:nil];
    NSEnumerator *enumerator = [set objectEnumerator];
    NSTouch* touch;
    int i;
	NSSize size = [self frame].size;
	NSTimeInterval when = 1000 * (time + [event timestamp]);
	while ((touch = [enumerator nextObject])) {
		NSPoint point = touch.normalizedPosition;
		point.x *= size.width;
		point.y *= size.height;
		for (i = 0; i < 10;  i++) {
			if (touches[i] == nil) {
				TouchFinger *finger = [TouchFinger alloc];
				finger.identity = touch.identity;
				finger.point = point;
    			touches[i] = [finger retain];
  				break;
			}
		}
		if (self.screen->touch) 
			(*self.screen->touch)(self.screen, touchEventBeganKind, i, point.x, size.height - point.y, when);
	}  
}
- (void)touchesCancelledWithEvent:(NSEvent *)event {
	if (!touching)
		return;
    NSSet *set = [event touchesMatchingPhase:NSTouchPhaseCancelled inView:nil];
    NSEnumerator *enumerator = [set objectEnumerator];
    NSTouch* touch;
    int i;
	NSSize size = [self frame].size;
	NSTimeInterval when = 1000 * (time + [event timestamp]);
	while ((touch = [enumerator nextObject])) {
		NSPoint point = touch.normalizedPosition;
		point.x *= size.width;
		point.y *= size.height;
		for (i = 0; i < 10;  i++) {
			TouchFinger *finger = touches[i];
    		if (finger && ([finger.identity isEqual:touch.identity])) {
    			[finger release];
				touches[i] = nil;
   				break;
			}
		}
		if (self.screen->touch) 
			(*self.screen->touch)(self.screen, touchEventCancelledKind, i, point.x, size.height - point.y, when);
	}
}
- (void)touchesEndedWithEvent:(NSEvent *)event {
	if (!touching)
		return;
    NSSet *set = [event touchesMatchingPhase:NSTouchPhaseEnded inView:nil];
    NSEnumerator *enumerator = [set objectEnumerator];
    NSTouch* touch;
    int i;
	NSSize size = [self frame].size;
	NSTimeInterval when = 1000 * (time + [event timestamp]);
	while ((touch = [enumerator nextObject])) {
		NSPoint point = touch.normalizedPosition;
		point.x *= size.width;
		point.y *= size.height;
		for (i = 0; i < 10;  i++) {
			TouchFinger *finger = touches[i];
    		if (finger && ([finger.identity isEqual:touch.identity])) {
    			[finger release];
				touches[i] = nil;
    			break;
			}
		}
		if (self.screen->touch) 
			(*self.screen->touch)(self.screen, touchEventEndedKind, i, point.x, size.height - point.y, when);
	}
}
- (void)touchesMovedWithEvent:(NSEvent *)event {
	if (!touching)
		return;
    NSSet *set = [event touchesMatchingPhase:NSTouchPhaseMoved inView:nil];
    NSEnumerator *enumerator = [set objectEnumerator];
    NSTouch* touch;
    int i;
	NSSize size = [self frame].size;
	NSTimeInterval when = 1000 * (time + [event timestamp]);
	while ((touch = [enumerator nextObject])) {
		NSPoint point = touch.normalizedPosition;
		point.x *= size.width;
		point.y *= size.height;
		for (i = 0; i < 10;  i++) {
			TouchFinger *finger = touches[i];
    		if (finger && ([finger.identity isEqual:touch.identity])) {
				finger.point = point;
   				break;
			}
		}
		if (self.screen->touch) 
			(*self.screen->touch)(self.screen, touchEventMovedKind, i, point.x, size.height - point.y, when);
	}
}
@end

void fxScreenAbort(txScreen* screen)
{
	ScreenView *screenView = screen->view;
    [screenView performSelectorOnMainThread:@selector(quitMachine) withObject:nil waitUntilDone:NO];
}

void fxScreenBufferChanged(txScreen* screen)
{
	ScreenView *screenView = screen->view;
	[screenView display];
}

void fxScreenFormatChanged(txScreen* screen)
{
}

void fxScreenStart(txScreen* screen, double interval)
{
	ScreenView *screenView = screen->view;
	screenView.timer = [NSTimer scheduledTimerWithTimeInterval:interval/1000 target:screenView selector:@selector(timerCallback:) userInfo:nil repeats:YES];
}

void fxScreenStop(txScreen* screen)
{
	ScreenView *screenView = screen->view;
	if (screenView.timer)
		[screenView.timer invalidate];
	screenView.timer = nil;
}

int main(int argc, const char **argv)
{
	[NSApplication sharedApplication];
	[NSApp setDelegate: [AppDelegate new]];
	return NSApplicationMain(argc, argv);
}
