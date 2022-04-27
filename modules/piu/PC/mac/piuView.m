/*
 * Copyright (c) 2016-2018  Moddable Tech, Inc.
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
int gxAppearanceChanged = 0;

void printComponent(CGFloat c) {
	xsIntegerValue i = c * 255;
	fprintf(stderr, "%2.2X", i);
}

void printColor(xsStringValue name, NSColor* color) {
	fprintf(stderr, "%s: #", name);
	NSColorSpace* colorSpace = [NSColorSpace deviceRGBColorSpace];
	color = [color colorUsingColorSpace:colorSpace];
	CGFloat r, g, b, a;
	[color getRed:&r green:&g blue:&b alpha:&a];
	printComponent(r);
	printComponent(g);
	printComponent(b);
	fprintf(stderr, " %f\n", a);
}

@implementation NSPiuAppDelegate
@synthesize machine;
@synthesize piuApplication;
@synthesize shouldQuit;
- (void)applicationWillFinishLaunching:(NSNotification *)notification {
    [[NSUserDefaults standardUserDefaults] setBool:NO forKey:@"NSFullScreenMenuItemEverywhere"];
}
- (void)applicationDidFinishLaunching:(NSNotification *)notification {
}
- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender {
    if (shouldQuit)
        return NSTerminateNow;
    return [self windowShouldClose:sender];
}
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
	return YES;
}
- (void)windowDidResize:(NSNotification *)notification {
	if (piuApplication) {
		xsBeginHost(machine);
		PiuApplicationResize(piuApplication);
		xsEndHost(machine);
	}
}
- (BOOL)windowShouldClose:(id)sender {
	xsBeginHost(machine);
	{
		xsVars(2);
		xsVar(0) = xsReference((*piuApplication)->behavior);
		xsVar(1) = xsReference((*piuApplication)->reference);
		if (xsFindResult(xsVar(0), xsID_onQuit))
			(void)xsCallFunction1(xsResult, xsVar(0), xsVar(1));
		else
			(void)xsCall0(xsVar(1), xsID_quit);
	}
	xsEndHost(machine);
	return shouldQuit;
}
- (void)application:(NSApplication *)application openFiles:(NSArray *)filenames
{
	NSInteger c = [filenames count], i;
	for (i = 0; i < c; i++) {
		NSString* filename = [filenames objectAtIndex:i];
		xsBeginHost(machine);
		{
			xsVars(2);
			PiuApplication* application = piuApplication;
			xsVar(0) = xsReference((*piuApplication)->behavior);
			if (xsFindResult(xsVar(0), xsID_onOpenFile)) {
				xsVar(1) = xsReference((*piuApplication)->reference);
				(void)xsCallFunction2(xsResult, xsVar(0), xsVar(1), xsString([filename UTF8String]));
			}
			PiuApplicationAdjust(application);
		}
		xsEndHost(machine);
	}
}
- (void)copy:(NSMenuItem *)sender {
	[self doMenu:sender];
}
- (void)cut:(NSMenuItem *)sender {
	[self doMenu:sender];
}
- (void)delete:(NSMenuItem *)sender {
	[self doMenu:sender];
}
- (void)paste:(NSMenuItem *)sender {
	[self doMenu:sender];
}
- (void)redo:(NSMenuItem *)sender {
	[self doMenu:sender];
}
- (void)selectAll:(NSMenuItem *)sender {
	[self doMenu:sender];
}
- (void)undo:(NSMenuItem *)sender {
	[self doMenu:sender];
}
- (void)doMenu:(NSMenuItem *)sender {
	xsIntegerValue tag = sender.tag;
    if (tag) {
		xsBeginHost(machine);
		xsVars(3);
		PiuApplicationDoMenu(piuApplication, tag);
		xsEndHost(machine);
	}
}
- (BOOL)validateMenuItem:(NSMenuItem *)item {
	xsIntegerValue tag = item.tag;
    if (tag) {
    	xsIntegerValue result;
		xsBeginHost(machine);
		xsVars(4);
		result = PiuApplicationCanMenu(piuApplication, tag);
		if (result & piuMenuTitled)
			[item setTitle:[NSString stringWithUTF8String:xsToString(xsGet(xsResult, xsID_title))]];
		[item setState:((result & piuMenuChecked) ? NSControlStateValueOn : NSControlStateValueOff)];
		xsEndHost(machine);
		return (result & piuMenuEnabled) ? YES : NO;
    }
    return YES;
}
@end

@implementation NSPiuWindow
- (BOOL)canBecomeKeyWindow {
    return YES;
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

@implementation NSPiuView
@synthesize piuView;
@synthesize time;
@synthesize timer;
- (BOOL)acceptsFirstResponder {
    return YES;
}
- (BOOL)becomeFirstResponder {
	if (piuView) {
		PiuApplication* application = (*piuView)->application;
		if (application)
			PiuApplicationSetFocus(application, NULL);
	}
    return YES;
}
- (void)dealloc {
    [super dealloc];
}
- (void)drawRect:(NSRect)rect {
	if (piuView) {
		if ((*piuView)->appearanceChanged) {
			(*piuView)->appearanceChanged = 0;
			NSColorSpace* colorSpace = [NSColorSpace deviceRGBColorSpace];
			NSColor* color = [[NSColor textColor] colorUsingColorSpace:colorSpace];
			CGFloat r, g, b, a;
			[color getRed:&r green:&g blue:&b alpha:&a];
			PiuApplication* application = (*piuView)->application;
			if ((*application)->behavior) {
				xsBeginHost((*piuView)->the);
				xsVars(2);
				xsVar(0) = xsReference((*application)->behavior);
				if (xsFindResult(xsVar(0), xsID_onAppearanceChanged)) {
					xsVar(1) = xsReference((*application)->reference);
					(void)xsCallFunction2(xsResult, xsVar(0), xsVar(1), xsInteger((xsIntegerValue)r));
				}
				xsEndHost((*piuView)->the);
			}
		}
		PiuRectangleRecord area;
		area.x = rect.origin.x;
		area.y = rect.origin.y;
		area.width = rect.size.width;
		area.height = rect.size.height;
		//fprintf(stderr, "update %d %d %d %d\n", area.x, area.y, area.width, area.height);
		xsBeginHost((*piuView)->the);
		{
			PiuApplication* application = (*piuView)->application;
			PiuApplicationAdjust(application);
#ifndef MAC_OS_X_VERSION_10_14
			(*piuView)->context = (CGContextRef)[[NSGraphicsContext currentContext] graphicsPort];
#else
			(*piuView)->context = [[NSGraphicsContext currentContext] CGContext];
#endif
			(*(*application)->dispatch->update)(application, piuView, &area);
			(*piuView)->context = NULL;
		}
		xsEndHost((*piuView)->the);
	}
	else {
    	[[NSColor clearColor] set];
		NSRectFill(rect);
	}
}
- (BOOL)isFlipped {
    return YES;
}
- (void)keyDown:(NSEvent *)event
{
	if (piuView) {
		xsBeginHost((*piuView)->the);
		{
			xsVars(4);
			PiuApplication* application = (*piuView)->application;
			NSEventModifierFlags flags = [event modifierFlags];
			PiuApplicationModifiersChanged(application, (flags & NSEventModifierFlagCommand) ? 1 : 0, (flags & NSEventModifierFlagOption) ? 1 : 0, (flags & NSEventModifierFlagShift) ? 1 : 0);
			NSString* string = [event charactersIgnoringModifiers];
			PiuApplicationKeyDown(application, (xsStringValue)[string UTF8String]);
			PiuApplicationAdjust(application);
		}
		xsEndHost((*piuView)->the);
	}
}
- (void)keyUp:(NSEvent *)event
{
	if (piuView) {
		xsBeginHost((*piuView)->the);
		{
			xsVars(4);
			PiuApplication* application = (*piuView)->application;
			NSEventModifierFlags flags = [event modifierFlags];
			PiuApplicationModifiersChanged(application, (flags & NSEventModifierFlagCommand) ? 1 : 0, (flags & NSEventModifierFlagOption) ? 1 : 0, (flags & NSEventModifierFlagShift) ? 1 : 0);
			NSString* string = [event charactersIgnoringModifiers];
			PiuApplicationKeyUp(application, (xsStringValue)[string UTF8String]);
			PiuApplicationAdjust(application);
		}
		xsEndHost((*piuView)->the);
	}
}
- (void)mouseDown:(NSEvent *)event {
	NSTimeInterval when = 1000 * (time + [event timestamp]);
	NSPoint point = [event locationInWindow];
	point = [self convertPoint:point fromView:nil];
	if (piuView) {
		xsBeginHost((*piuView)->the);
		{
			PiuApplication* application = (*piuView)->application;
			NSEventModifierFlags flags = [event modifierFlags];
			PiuApplicationModifiersChanged(application, (flags & NSEventModifierFlagCommand) ? 1 : 0, (flags & NSEventModifierFlagOption) ? 1 : 0, (flags & NSEventModifierFlagShift) ? 1 : 0);
			PiuApplicationTouchBegan(application, 0, point.x, point.y, when);
			PiuApplicationAdjust(application);
		}
		xsEndHost((*piuView)->the);
	}
}
- (void)mouseDragged:(NSEvent *)event {
	NSTimeInterval when = 1000 * (time + [event timestamp]);
	NSPoint point = [event locationInWindow];
	point = [self convertPoint:point fromView:nil];
	if (piuView) {
		xsBeginHost((*piuView)->the);
		{
			PiuApplication* application = (*piuView)->application;
			NSEventModifierFlags flags = [event modifierFlags];
			PiuApplicationModifiersChanged(application, (flags & NSEventModifierFlagCommand) ? 1 : 0, (flags & NSEventModifierFlagOption) ? 1 : 0, (flags & NSEventModifierFlagShift) ? 1 : 0);
			PiuApplicationTouchMoved(application, 0, point.x, point.y, when);
		}
		xsEndHost((*piuView)->the);
	}
}
- (void)mouseEntered:(NSEvent *)event {
	NSPoint point = [event locationInWindow];
	point = [self convertPoint:point fromView:nil];
	if (piuView) {
		xsBeginHost((*piuView)->the);
		{
			PiuApplication* application = (*piuView)->application;
			NSEventModifierFlags flags = [event modifierFlags];
			PiuApplicationModifiersChanged(application, (flags & NSEventModifierFlagCommand) ? 1 : 0, (flags & NSEventModifierFlagOption) ? 1 : 0, (flags & NSEventModifierFlagShift) ? 1 : 0);
			PiuApplicationMouseEntered(application, point.x, point.y);
		}
		xsEndHost((*piuView)->the);
	}
}
- (void)mouseExited:(NSEvent *)event {
	NSPoint point = [event locationInWindow];
	point = [self convertPoint:point fromView:nil];
	if (piuView) {
		xsBeginHost((*piuView)->the);
		{
			PiuApplication* application = (*piuView)->application;
			NSEventModifierFlags flags = [event modifierFlags];
			PiuApplicationModifiersChanged(application, (flags & NSEventModifierFlagCommand) ? 1 : 0, (flags & NSEventModifierFlagOption) ? 1 : 0, (flags & NSEventModifierFlagShift) ? 1 : 0);
			PiuApplicationMouseExited(application, point.x, point.y);
		}
		xsEndHost((*piuView)->the);
	}
}
- (void)mouseMoved:(NSEvent *)event {
	NSPoint point = [event locationInWindow];
	point = [self convertPoint:point fromView:nil];
	if (piuView) {
		xsBeginHost((*piuView)->the);
		{
			PiuApplication* application = (*piuView)->application;
			NSEventModifierFlags flags = [event modifierFlags];
			PiuApplicationModifiersChanged(application, (flags & NSEventModifierFlagCommand) ? 1 : 0, (flags & NSEventModifierFlagOption) ? 1 : 0, (flags & NSEventModifierFlagShift) ? 1 : 0);
			PiuApplicationMouseMoved(application, point.x, point.y);
		}
		xsEndHost((*piuView)->the);
	}
}
- (void)mouseUp:(NSEvent *)event {
	NSTimeInterval when = 1000 * (time + [event timestamp]);
	NSPoint point = [event locationInWindow];
	point = [self convertPoint:point fromView:nil];
	if (piuView) {
		xsBeginHost((*piuView)->the);
		{
			PiuApplication* application = (*piuView)->application;
			NSEventModifierFlags flags = [event modifierFlags];
			PiuApplicationModifiersChanged(application, (flags & NSEventModifierFlagCommand) ? 1 : 0, (flags & NSEventModifierFlagOption) ? 1 : 0, (flags & NSEventModifierFlagShift) ? 1 : 0);
			PiuApplicationTouchEnded(application, 0, point.x, point.y, when);
			PiuApplicationAdjust(application);
		}
		xsEndHost((*piuView)->the);
	}
}
- (void)scrollWheel:(NSEvent *)event {
	CGFloat dx = [event scrollingDeltaX];
	CGFloat dy = [event scrollingDeltaY];
	[[self window] makeFirstResponder:self];
	if (![event hasPreciseScrollingDeltas]) {
		dx *= 5;
		dy *= 5;
	}
	if (piuView) {
		xsBeginHost((*piuView)->the);
		{
			PiuApplication* application = (*piuView)->application;
			PiuApplicationMouseScrolled(application, dx, dy);
			PiuApplicationAdjust(application);
		}
		xsEndHost((*piuView)->the);
	}
}
- (void)timerCallback:(NSTimer*)theTimer {
	if (piuView) {
		xsBeginHost((*piuView)->the);
		{
			PiuApplication* application = (*piuView)->application;
			xsVars(2);
			PiuApplicationDeferContents(the, application);
			PiuApplicationIdleContents(application);
			PiuApplicationTouchIdle(application);
			PiuApplicationAdjust(application);
		}
		xsEndHost((*piuView)->the);
	}
}
- (void)viewDidChangeEffectiveAppearance {
	if (piuView)
		(*piuView)->appearanceChanged = 1;
}
@end

@implementation NSPiuClipView
@synthesize piuContent;
@end

static void PiuViewDrawTextureAux(PiuView* self, PiuTexture* texture, PiuCoordinate x, PiuCoordinate y, PiuCoordinate sx, PiuCoordinate sy, PiuDimension sw, PiuDimension sh);
static void PiuViewMark(xsMachine* the, void* it, xsMarkRoot markRoot);

static xsHostHooks PiuViewHooks = {
	PiuViewDelete,
	PiuViewMark,
	NULL
};

void PiuViewAdjust(PiuView* self) 
{
    NSArray* subviews = [(*self)->nsView subviews];
    for (NSView* subView in subviews) {
    	if ([subView isKindOfClass:[NSPiuClipView class]]) {
    		NSPiuClipView* clipView = (NSPiuClipView*)subView;
    		NSView* view = clipView.documentView;
    		PiuContent* content = clipView.piuContent;
			PiuRectangleRecord bounds = (*content)->bounds;
			PiuRectangleRecord clipBounds = bounds;
			PiuContainer* container = (*content)->container;
			while (container) {
				bounds.x += (*container)->bounds.x;
				bounds.y += (*container)->bounds.y;
				clipBounds.x += (*container)->bounds.x;
				clipBounds.y += (*container)->bounds.y;
				if ((*container)->flags & piuClip)
					PiuRectangleIntersect(&clipBounds, &clipBounds, &(*container)->bounds);
				container = (*container)->container;
			}
			NSRect frame;
			frame.origin.x = 0;
			frame.origin.y = 0;
			frame.size.width = bounds.width;
			frame.size.height = bounds.height;
			[view setFrame:frame];
			frame.origin.x = clipBounds.x;
			frame.origin.y = clipBounds.y;
			frame.size.width = clipBounds.width;
			frame.size.height = clipBounds.height;
			[clipView setFrame:frame];
			NSPoint origin;
			origin.x = clipBounds.x - bounds.x;
			origin.y = clipBounds.y - bounds.y;
			[clipView scrollToPoint:origin];
    	}
    }
    if ((*self)->context == NULL) {
        xsBeginHost((*self)->the);
        {
            PiuApplication* application = (*self)->application;
            PiuApplicationMouseMoved(application, (*application)->mouse.x, (*application)->mouse.y);
        }
        xsEndHost((**self)->the);
    }
}

void PiuViewChangeCursor(PiuView* self, int32_t shape)
{
	NSCursor* cursor;
	switch (shape) {
	case 1:
		cursor = [NSCursor crosshairCursor];
		break;
	case 2:
		cursor = [NSCursor IBeamCursor];
		break;
	case 3:
		cursor = [NSCursor pointingHandCursor];
		break;
	case 4:
		cursor = [NSCursor operationNotAllowedCursor];
		break;
	case 5:
		cursor = [NSCursor resizeLeftRightCursor];
		break;
	case 6:
		cursor = [NSCursor resizeUpDownCursor];
		break;
	default:
		cursor = [NSCursor arrowCursor];
		break;
	}
	[cursor set];
}

void PiuViewCreate(xsMachine* the) 
{
	PiuView* self;
	PiuApplication* application;
	xsSetHostChunk(xsThis, NULL, sizeof(PiuViewRecord));
	self = PIU(View, xsThis);
	(*self)->reference = xsToReference(xsThis);
	(*self)->the = the;
	xsSetHostHooks(xsThis, &PiuViewHooks);
	application = (*self)->application = PIU(Application, xsArg(0));
	(*application)->view = self;
	
	NSRect contentRect = NSMakeRect(0, 0, 640, 480);
	NSPiuWindow* nsWindow = [[NSPiuWindow alloc] initWithContentRect:contentRect styleMask:(NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable) backing:NSBackingStoreBuffered defer:NO];

    NSPiuView* nsView = [[[NSPiuView alloc] initWithFrame:contentRect] autorelease];
    
    NSProcessInfo* processInfo = [NSProcessInfo processInfo];
    NSTimeInterval time = [processInfo systemUptime];
    NSDate* date = [NSDate dateWithTimeIntervalSinceNow:-time];
    nsView.time = [date timeIntervalSince1970];
	nsView.timer = [NSTimer scheduledTimerWithTimeInterval:0.005 target:nsView selector:@selector(timerCallback:) userInfo:nil repeats:YES];

 	[nsWindow setAcceptsMouseMovedEvents:YES];
	[nsWindow setContentView:nsView];
    [nsWindow setDelegate:(id<NSWindowDelegate>)[[NSApplication sharedApplication] delegate]];
    [nsWindow registerForDraggedTypes:[NSArray arrayWithObjects:@"NSFilenamesPboardType",nil]];
    if (![nsWindow setFrameUsingName:@"PiuWindow"])
        [nsWindow cascadeTopLeftFromPoint:NSMakePoint(20, 20)];
	[nsWindow setFrameAutosaveName:@"PiuWindow"]; 
	if ([nsWindow respondsToSelector:@selector(setTabbingMode:)])
		[nsWindow setTabbingMode:NSWindowTabbingModeDisallowed];
	if (xsFindResult(xsArg(1), xsID_window)) {
		xsResult = xsGet(xsResult, xsID_title);
		nsWindow.title = [NSString stringWithUTF8String:xsToString(xsResult)];
	}
	
	(*self)->nsView = nsView;
    nsView.piuView = self;
    
	(*self)->nsWindow = nsWindow;
	
	(*self)->colorSpace = CGColorSpaceCreateDeviceRGB();
	(*self)->appearanceChanged = 1;
	
	xsResult = xsThis;
}

void PiuViewDelete(void* it)
{
}

void PiuViewDictionary(xsMachine* the, void* it)
{
	if (xsFindResult(xsArg(1), xsID_menus)) {
		NSApplication *application = [NSApplication sharedApplication];
		NSMenu *mainMenu = [[NSMenu new] autorelease];
		NSMenu *servicesMenu = [[NSMenu new] autorelease];
		NSMenu* menu;
		NSMenuItem *item;
		xsIntegerValue c, i, d, j;
		xsStringValue value;

		c = xsToInteger(xsGet(xsResult, xsID_length));
		xsVar(0) = xsGetAt(xsResult, xsInteger(1));
		xsVar(1) = xsGet(xsVar(0), xsID_items);
		d = xsToInteger(xsGet(xsVar(1), xsID_length));
		xsVar(2) = xsGetAt(xsVar(1), xsInteger(d - 1));
		value = xsToString(xsGet(xsVar(2), xsID_command));
		if (!c_strcmp(value, "Quit")) {
			(void)xsCall0(xsVar(1), xsID_pop);
			(void)xsCall0(xsVar(1), xsID_pop);
		}
		xsVar(0) = xsGetAt(xsResult, xsInteger(2));
		xsVar(1) = xsGet(xsVar(0), xsID_items);
		d = xsToInteger(xsGet(xsVar(1), xsID_length));
		xsVar(2) = xsGetAt(xsVar(1), xsInteger(d - 1));
		value = xsToString(xsGet(xsVar(2), xsID_command));
		if (!c_strcmp(value, "Preferences")) {
			(void)xsCall0(xsVar(1), xsID_pop);
			(void)xsCall0(xsVar(1), xsID_pop);
		}
		xsVar(0) = xsGetAt(xsResult, xsInteger(c - 1));
		xsVar(1) = xsGet(xsVar(0), xsID_items);
		d = xsToInteger(xsGet(xsVar(1), xsID_length));
		xsVar(2) = xsGetAt(xsVar(1), xsInteger(d - 1));
		value = xsToString(xsGet(xsVar(2), xsID_command));
		if (!c_strcmp(value, "About")) {
			(void)xsCall0(xsVar(1), xsID_pop);
			(void)xsCall0(xsVar(1), xsID_pop);
		}
		for (i = 0; i < c; i++) {
			xsVar(0) = xsGetAt(xsResult, xsInteger(i));
			xsVar(1) = xsGet(xsVar(0), xsID_title);
			NSString* string = [NSString stringWithUTF8String:xsToString(xsVar(1))];
			menu = [[[NSMenu alloc] initWithTitle:string] autorelease];
			[string release];
			item = [[NSMenuItem new] autorelease];
			[item setSubmenu:menu];
			[mainMenu addItem:item];

			xsVar(1) = xsGet(xsVar(0), xsID_items);
			d = xsToInteger(xsGet(xsVar(1), xsID_length));
			for (j = 0; j < d; j++) {
				xsVar(2) = xsGetAt(xsVar(1), xsInteger(j));
				if (xsTest(xsVar(2))) {
					char buffer[256];
					xsIdentifier index;
					value = xsToString(xsGet(xsVar(2), xsID_command));
					c_strcpy(buffer, "can");
					c_strcat(buffer, value);
					index = xsID(buffer);
					xsSet(xsVar(2), xsID_canID, xsInteger(index));
					
					value = xsToString(xsGet(xsVar(2), xsID_command));
					c_strcpy(buffer, "do");
					c_strcat(buffer, value);
					index = xsID(buffer);
					xsSet(xsVar(2), xsID_doID, xsInteger(index));
						
					value = xsToString(xsGet(xsVar(2), xsID_command));
					SEL action;
					if (!c_strcmp(value, "Services"))
						action = NULL;
					else if (!c_strcmp(value, "HideApplication"))
						action = @selector(hide:);
					else if (!c_strcmp(value, "HideOtherApplications"))
						action = @selector(hideOtherApplications:);
					else if (!c_strcmp(value, "ShowAllApplications"))
						action = @selector(unhideAllApplications:);
					else if (!c_strcmp(value, "Quit"))
						action = @selector(terminate:);
					else if (!c_strcmp(value, "Undo"))
						action = @selector(undo:);
					else if (!c_strcmp(value, "Redo"))
						action = @selector(redo:);
					else if (!c_strcmp(value, "Cut"))
						action = @selector(cut:);
					else if (!c_strcmp(value, "Copy"))
						action = @selector(copy:);
					else if (!c_strcmp(value, "Paste"))
						action = @selector(paste:);
					else if (!c_strcmp(value, "Clear"))
						action = @selector(delete:);
					else if (!c_strcmp(value, "SelectAll"))
						action = @selector(selectAll:);
					else
						action = @selector(doMenu:);
					
					xsVar(3) = xsGet(xsVar(2), xsID_titles);
					if (xsTest(xsVar(3))) {
						xsVar(4) = xsGet(xsVar(2), xsID_state);
						xsVar(4) = xsGetAt(xsVar(3), xsVar(4));
						xsSet(xsVar(2), xsID_title, xsVar(4));
					}

					NSString *title = [NSString stringWithUTF8String:xsToString(xsGet(xsVar(2), xsID_title))];
					NSString *key;
					NSEventModifierFlags mask = NSEventModifierFlagCommand;
					if (xsFindString(xsVar(2), xsID_key, &value)) {
						xsBooleanValue flag;
						if (xsFindBoolean(xsVar(2), xsID_shift, &flag) && flag)
							mask |= NSEventModifierFlagShift;
						if (xsFindBoolean(xsVar(2), xsID_option, &flag) && flag)
							mask |= NSEventModifierFlagOption;
						key = [[NSString stringWithUTF8String:value] lowercaseString];
					}
					else
						key = @"";
				
					item = [[[NSMenuItem alloc] initWithTitle:title action:action keyEquivalent:key] autorelease];
					[item setKeyEquivalentModifierMask:mask];
					[item setTag:(i << 8) | (j + 1)];
					if (action == NULL)
						[item setSubmenu:servicesMenu];
				}
				else
					item = [NSMenuItem separatorItem];
					
				[menu addItem:item];
			}
		}
		[application setMainMenu: mainMenu];
		[application setServicesMenu: servicesMenu];
		xsSet(xsThis, xsID_menus, xsResult);
	}
}

void PiuViewDrawRoundContent(PiuView* self, PiuCoordinate x, PiuCoordinate y, PiuDimension w, PiuDimension h, PiuDimension radius, PiuDimension border, PiuVariant variant, PiuColor fillColor, PiuColor strokeColor)
{
	if ((w <= 0) || (h <= 0)) return;
	CGFloat lx = x, ty = y, rx = x + w, by = y + h, r = radius, t, u = border;
	if (variant == 1)
		lx += r;
	else if (variant == 2)
		rx -= r;
	if (u > 0) {
		CGFloat delta = u / 2;
		lx += delta;
		ty += delta;
		rx -= delta;
		by -= delta;
		r -= delta;
	}
	t = r * 0.552284749831;
		
	
	NSBezierPath* path = [NSBezierPath bezierPath];
	[path moveToPoint:NSMakePoint(lx, ty + r)];
	[path curveToPoint:NSMakePoint(lx + r, ty) controlPoint1:NSMakePoint(lx, ty + r - t) controlPoint2:NSMakePoint(lx + r - t, ty)];
	[path lineToPoint:NSMakePoint(rx - r, ty)];
	[path curveToPoint:NSMakePoint(rx, ty + r) controlPoint1:NSMakePoint(rx - r + t, ty) controlPoint2:NSMakePoint(rx, ty + r - t)];
	[path lineToPoint:NSMakePoint(rx, by - r)];
	if (variant == 2)
		[path curveToPoint:NSMakePoint(rx + r, by) controlPoint1:NSMakePoint(rx, by - r + t) controlPoint2:NSMakePoint(rx + r - t, by)];
	else
		[path curveToPoint:NSMakePoint(rx - r, by) controlPoint1:NSMakePoint(rx, by - r + t) controlPoint2:NSMakePoint(rx - r + t, by)];
	if (variant == 1) {
		[path lineToPoint:NSMakePoint(lx - r, by)];
		[path curveToPoint:NSMakePoint(lx, by - r) controlPoint1:NSMakePoint(lx - r + t, by) controlPoint2:NSMakePoint(lx, by - r + t)];
	}
	else {
		[path lineToPoint:NSMakePoint(lx + r, by)];
		[path curveToPoint:NSMakePoint(lx, by - r) controlPoint1:NSMakePoint(lx + r - t, by) controlPoint2:NSMakePoint(lx, by - r + t)];
	}
	[path closePath];
	if (fillColor->a) {
		CGFloat r, g, b, a;
		r = (float)fillColor->r / 255;
		g = (float)fillColor->g / 255;
		b = (float)fillColor->b / 255;
		a = (float)fillColor->a / 255;
    	[[NSColor colorWithSRGBRed:r green:g blue:b alpha:a] set];
		[path fill];
	}
	if ((border > 0) && (strokeColor->a)) {
		CGFloat r, g, b, a;
		r = (float)strokeColor->r / 255;
		g = (float)strokeColor->g / 255;
		b = (float)strokeColor->b / 255;
		a = (float)strokeColor->a / 255;
    	[[NSColor colorWithSRGBRed:r green:g blue:b alpha:a] set];
    	path.lineWidth = u;
		[path stroke];
	}
}

void PiuViewDrawString(PiuView* self, xsSlot* slot, xsIntegerValue offset, xsIntegerValue length, PiuFont* font, PiuCoordinate x, PiuCoordinate y, PiuDimension w, PiuDimension sw)
{
	PiuViewDrawStringSubPixel(self, slot, offset, length, font, x, y, w, sw);
}

void PiuViewDrawStringSubPixel(PiuView* self, xsSlot* slot, xsIntegerValue offset, xsIntegerValue length, PiuFont* font, double x, double y, PiuDimension w, PiuDimension sw)
{
	xsMachine* the = (*self)->the;
	CGContextRef context = (*self)->context;
    CGContextSetTextMatrix(context, CGAffineTransformMakeScale(1.0f, -1.0f));
	xsStringValue value = PiuToString(slot);
	value += offset;
	if (length < 0)
		length = c_strlen(value);
	CFStringRef string = CFStringCreateWithBytesNoCopy(NULL, (uint8_t*)value, length, kCFStringEncodingUTF8, false, kCFAllocatorNull);
    CFRange range = CFRangeMake(0, CFStringGetLength(string));
	CFMutableAttributedStringRef attributedString = CFAttributedStringCreateMutable(kCFAllocatorDefault, 0);
	CFAttributedStringReplaceString(attributedString, CFRangeMake(0, 0), string);
	CFAttributedStringSetAttribute(attributedString, range, kCTFontAttributeName, (*font)->fref);
	CFAttributedStringSetAttribute(attributedString, range, kCTForegroundColorAttributeName, (*self)->color);
	if ((*font)->flags & piuStyleUnderline) {
		SInt32 underlineType = kCTUnderlineStyleSingle;
		CFNumberRef underline = CFNumberCreate(NULL, kCFNumberSInt32Type, &underlineType);
		CFAttributedStringSetAttribute(attributedString, range, kCTUnderlineStyleAttributeName, underline);
	}
	CTLineRef line = CTLineCreateWithAttributedString(attributedString);
	CGContextSetTextPosition(context, x, y + (*font)->ascent);
	CTLineDraw(line, context);
	CFRelease(line);
 	CFRelease(attributedString);
 	CFRelease(string);
}

void PiuViewDrawTexture(PiuView* self, PiuTexture* texture, PiuCoordinate x, PiuCoordinate y, PiuCoordinate sx, PiuCoordinate sy, PiuDimension sw, PiuDimension sh)
{
	PiuDimension tw = (*texture)->width;
	PiuDimension th = (*texture)->height;
	if (sx < 0) {
		x -= sx;
		sw += sx;
		sx = 0;
	}
	if (sx + sw > tw)
		sw = tw - sx;
	if (sy < 0) {
		y -= sy;
		sh += sy;
		sy = 0;
	}
	if (sy + sh > th)
		sh = th - sy;
	if ((sw <= 0) || (sh <= 0)) return;
	PiuViewDrawTextureAux(self, texture, x, y, sx, sy, sw, sh);
}

void PiuViewDrawTextureAux(PiuView* self, PiuTexture* texture, PiuCoordinate x, PiuCoordinate y, PiuCoordinate sx, PiuCoordinate sy, PiuDimension sw, PiuDimension sh)
{
	CGFloat scale = (*texture)->scale;
	PiuDimension th = (*texture)->height;
	if ((*self)->filtered) {
		if ((*self)->transparent) return;
		CGColorSpaceRef colorspace = CGColorSpaceCreateDeviceGray();
		CGContextRef maskContext = CGBitmapContextCreate(NULL, scale * sw, scale * sh, 8, scale * sw, colorspace, 0);
		CGColorSpaceRelease(colorspace);
		NSGraphicsContext *maskGraphicsContext = [NSGraphicsContext graphicsContextWithCGContext:maskContext flipped:YES];
		[NSGraphicsContext saveGraphicsState];
		[NSGraphicsContext setCurrentContext:maskGraphicsContext];
		[[NSColor whiteColor] setFill];
		CGContextFillRect(maskContext, CGRectMake(0, 0, scale * sw, scale * sh));
		NSRect source = NSMakeRect(scale * sx, scale * (th - sy - sh), scale * sw, scale * sh);
		NSRect destination = NSMakeRect(0, 0, scale * sw, scale * sh);
		[(*texture)->image drawInRect:destination fromRect:source operation:NSCompositingOperationDestinationIn fraction:1 respectFlipped:YES hints:nil];
		[NSGraphicsContext restoreGraphicsState];
		CGImageRef maskImage = CGBitmapContextCreateImage(maskContext);
		CGContextRelease(maskContext);
		CGContextRef context = (*self)->context;
		CGContextSaveGState(context);
        CGContextScaleCTM(context, 1 / scale, 1/ scale);
		CGRect rect = CGRectMake(scale * x, scale * y, scale * sw, scale * sh);
		CGContextClipToMask(context, rect, maskImage);
		CGContextSetFillColorWithColor(context, (*self)->color);
		CGContextFillRect(context, rect);
		CGContextRestoreGState(context);
		CGImageRelease(maskImage);
	}
	else {
		NSRect source = NSMakeRect(scale * sx, scale * (th - sy - sh), scale * sw, scale * sh);
		NSRect destination = NSMakeRect(x, y, sw, sh);
		[(*texture)->image drawInRect:destination fromRect:source operation:NSCompositingOperationSourceOver fraction:1 respectFlipped:YES hints:nil];
	}
}

void PiuViewFillColor(PiuView* self, PiuCoordinate x, PiuCoordinate y, PiuDimension w, PiuDimension h)
{
	if ((w <= 0) || (h <= 0)) return;
	if ((*self)->transparent) return;
	CGContextRef context = (*self)->context;
	CGContextSetFillColorWithColor(context, (*self)->color);
	CGRect rect = CGRectMake(x, y, w, h);
	CGContextFillRect(context, rect);
}

void PiuViewFillTexture(PiuView* self, PiuTexture* texture, PiuCoordinate x, PiuCoordinate y, PiuDimension w, PiuDimension h, PiuCoordinate sx, PiuCoordinate sy, PiuDimension sw, PiuDimension sh)
{
	PiuDimension tw = (*texture)->width;
	PiuDimension th = (*texture)->height;
	if (sx < 0) {
		if (w == sw) {
			x -= sx;
			w += sx;
		}
		sw += sx;
		sx = 0;
	}
	if (sx + sw > tw) {
		if (w == sw)
			w = tw - sx;
		sw = tw - sx;
	}
	if (sy < 0) {
		if (h == sh) {
			y -= sy;
			h += sy;
		}
		sh += sy;
		sy = 0;
	}
	if (sy + sh > th) {
		if (h == sh)
			h = th - sy;
		sh = th - sy;
	}
	if ((w <= 0) || (h <= 0) || (sw <= 0) || (sh <= 0)) return;
	PiuCoordinate xx, ww;
	while (h >= sh) {
		xx = x;
		ww = w;
		while (ww >= sw) {
			PiuViewDrawTextureAux(self, texture, xx, y, sx, sy, sw, sh);
			xx += sw;
			ww -= sw;
		}
		if (ww)
			PiuViewDrawTextureAux(self, texture, xx, y, sx, sy, ww, sh);
		y += sh;
		h -= sh;
	}
	if (h) {
		while (w >= sw) {
			PiuViewDrawTextureAux(self, texture, x, y, sx, sy, sw, h);
			x += sw;
			w -= sw;
		}
		if (w)
			PiuViewDrawTextureAux(self, texture, x, y, sx, sy, w, h);
	}
}

void PiuViewGetSize(PiuView* self, PiuDimension *width, PiuDimension *height)
{
	NSPiuView* nsView = (*self)->nsView;
	NSRect rect = nsView.bounds;
	*width = rect.size.width;
	*height = rect.size.height;
}

void PiuViewIdleCheck(PiuView* self, PiuInterval idle)
{
}

void PiuViewInvalidate(PiuView* self, PiuRectangle area) 
{
	NSPiuView* nsView = (*self)->nsView;
	NSRect rect;
	if (area) {
		//fprintf(stderr, "invalidate %d %d %d %d\n", area->x, area->y, area->width, area->height);
		rect.origin.x = area->x;
		rect.origin.y = area->y;
		rect.size.width = area->width;
		rect.size.height = area->height;
	}
	else
		rect = nsView.bounds;
	[nsView setNeedsDisplayInRect:rect];
}

void PiuViewMark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
}

void PiuViewPopClip(PiuView* self)
{
	CGContextRef context = (*self)->context;
    CGContextRestoreGState(context);
}

void PiuViewPopColor(PiuView* self)
{
	CGColorRelease((*self)->color);
	(*self)->color = NULL;
}

void PiuViewPopColorFilter(PiuView* self)
{
	CGColorRelease((*self)->color);
	(*self)->color = NULL;
	(*self)->filtered = 0;
}

void PiuViewPopOrigin(PiuView* self)
{
	CGContextRef context = (*self)->context;
	CGContextRestoreGState(context);
}

void PiuViewPushClip(PiuView* self, PiuCoordinate x, PiuCoordinate y, PiuDimension w, PiuDimension h)
{
	CGContextRef context = (*self)->context;
	CGRect rect = CGRectMake(x, y, w, h);
    CGContextSaveGState(context);
	CGContextClipToRect(context, rect);
}

void PiuViewPushColor(PiuView* self, PiuColor color)
{
	CGFloat components[4];
	components[0] = (float)color->r / 255;
	components[1] = (float)color->g / 255;
	components[2] = (float)color->b / 255;
	components[3] = (float)color->a / 255;
	(*self)->color = CGColorCreate((*self)->colorSpace, components);
	(*self)->transparent = (color->a == 0) ? 1 : 0;
}

void PiuViewPushColorFilter(PiuView* self, PiuColor color)
{
	CGFloat components[4];
	components[0] = (float)color->r / 255;
	components[1] = (float)color->g / 255;
	components[2] = (float)color->b / 255;
	components[3] = (float)color->a / 255;
	(*self)->color = CGColorCreate((*self)->colorSpace, components);
	(*self)->transparent = (color->a == 0) ? 1 : 0;
	(*self)->filtered = 1;
}

void PiuViewPushOrigin(PiuView* self, PiuCoordinate x, PiuCoordinate y)
{
	CGContextRef context = (*self)->context;
    CGContextSaveGState(context);
	CGContextTranslateCTM(context, x, y);
}

void PiuViewReflow(PiuView* self)
{
}

void PiuViewReschedule(PiuView* self)
{
	PiuApplicationIdleCheck((*self)->application);
}

double PiuViewTicks(PiuView* self)
{
    NSProcessInfo* processInfo = [NSProcessInfo processInfo];
    NSTimeInterval time = [processInfo systemUptime];
    return 1000 * time;
}

void PiuViewValidate(PiuView* self, PiuRectangle area) 
{
}

void PiuCursors_get_arrow(xsMachine* the)
{
	xsResult = xsInteger(0);
}

void PiuCursors_get_cross(xsMachine* the)
{
	xsResult = xsInteger(1);
}

void PiuCursors_get_iBeam(xsMachine* the)
{
	xsResult = xsInteger(2);
}

void PiuCursors_get_link(xsMachine* the)
{
	xsResult = xsInteger(3);
}

void PiuCursors_get_notAllowed(xsMachine* the)
{
	xsResult = xsInteger(4);
}

void PiuCursors_get_resizeColumn(xsMachine* the)
{
	xsResult = xsInteger(5);
}

void PiuCursors_get_resizeRow(xsMachine* the)
{
	xsResult = xsInteger(6);
}

void PiuSystem_getClipboardString(xsMachine* the)
{
	NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
	NSString* string = [pasteboard stringForType:NSPasteboardTypeString];
	xsResult = xsString([string UTF8String]);
}

void PiuSystem_setClipboardString(xsMachine* the)
{
	NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
	NSString* string = [NSString stringWithUTF8String:xsToString(xsArg(0))];
	[pasteboard declareTypes:[NSArray arrayWithObject:NSPasteboardTypeString] owner:nil];
	[pasteboard setString:string forType:NSPasteboardTypeString];
}

void PiuSystem_launchPath(xsMachine* the)
{
	NSString* string = [NSString stringWithUTF8String:xsToString(xsArg(0))];
	[[NSWorkspace sharedWorkspace] openURL:[NSURL fileURLWithPath:string]];
}

void PiuSystem_launchURL(xsMachine* the)
{
	NSString* string = [NSString stringWithUTF8String:xsToString(xsArg(0))];
	[[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:string]];
}

void PiuNavigationBar_create(xsMachine* the)
{
	xsDebugger();
}

void PiuStatusBar_create(xsMachine* the)
{
	xsDebugger();
}
