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
#include <dlfcn.h>
#include "screen.h"

typedef struct PiuScreenStruct PiuScreenRecord, *PiuScreen;
typedef struct PiuScreenMessageStruct  PiuScreenMessageRecord, *PiuScreenMessage;


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

@interface NSPiuScreenView : NSView {
	PiuScreen* piuScreen;
	void* library;
	txScreen* screen;
    NSTimeInterval time;
	NSTimer *timer;
    NSImage *touchImage;
	id *touches;
	BOOL touching;
}
@property (assign) PiuScreen* piuScreen;
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

static void fxScreenAbort(txScreen* screen);
static void fxScreenBufferChanged(txScreen* screen);
static int fxScreenCreateWorker(txScreen* screen, char* name);
static void fxScreenDeleteWorker(txScreen* screen, int worker);
static void fxScreenFormatChanged(txScreen* screen);
static void fxScreenPost(txScreen* screen, char* message, int size);
static void fxScreenStart(txScreen* screen, double interval);
static void fxScreenStop(txScreen* screen);

struct PiuScreenStruct {
	PiuHandlePart;
	PiuIdlePart;
	PiuBehaviorPart;
	PiuContentPart;
    NSPiuClipView *nsClipView;
    NSPiuScreenView *nsScreenView;
    txScreen* screen;
};

struct PiuScreenMessageStruct {
	void* buffer;
	int size;
};

@implementation NSPiuScreenView
@synthesize piuScreen;
@synthesize library;
@synthesize screen;
@synthesize time;
@synthesize timer;
@synthesize touchImage;
@synthesize touches;
@synthesize touching;
- (void)dealloc {
	if (library)
    	dlclose(library);
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
				[touchImage drawAtPoint:finger.point fromRect:NSZeroRect operation:NSCompositingOperationSourceOver fraction:1.0];
			}
		}
	}
}
- (void)launchMachine:(NSString*)path {
	NSString *info = nil;
	txScreenLaunchProc launch;
	self.library = dlopen([path UTF8String], RTLD_NOW | RTLD_LOCAL);
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
- (void)messageToApplication:(NSObject *)object {
	if (self.screen->invoke) {
		NSData* data = (NSData*)object;
		PiuScreenMessage message = (PiuScreenMessage)[data bytes];
		(*self.screen->invoke)(self.screen, message->buffer, message->size);
		free(message->buffer);
	}
}
- (void)messageToHost:(NSObject *)object {
	NSData* data = (NSData*)object;
	PiuScreenMessage message = (PiuScreenMessage)[data bytes];
	if ((*piuScreen)->behavior) {
		xsBeginHost((*piuScreen)->the);
		xsVars(4);
		xsVar(0) = xsReference((*piuScreen)->behavior);
		if (xsFindResult(xsVar(0), xsID_onMessage)) {
			xsVar(1) = xsReference((*piuScreen)->reference);
			if (message->size < 0) {
				xsVar(2) = xsDemarshallAlien(message->buffer);
				xsVar(3) = xsInteger(0 - message->size);
			}
			else if (message->size > 0)
				xsVar(2) = xsArrayBuffer(message->buffer, message->size);
			else
				xsVar(2) = xsString(message->buffer);
			(void)xsCallFunction3(xsResult, xsVar(0), xsVar(1), xsVar(2), xsVar(3));
		}
		xsEndHost((*piuScreen)->the);
	}
	free(message->buffer);
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

static void PiuScreenBind(void* it, PiuApplication* application, PiuView* view);
static void PiuScreenDelete(void* it);
static void PiuScreenDictionary(xsMachine* the, void* it);
static void PiuScreenMark(xsMachine* the, void* it, xsMarkRoot markRoot);
static void PiuScreenUnbind(void* it, PiuApplication* application, PiuView* view);
static void PiuScreenMessageBuild(PiuScreenMessage message, char* buffer, int size);

const PiuDispatchRecord ICACHE_FLASH_ATTR PiuScreenDispatchRecord = {
	"Screen",
	PiuScreenBind,
	PiuContentCascade,
	PiuContentDraw,
	PiuContentFitHorizontally,
	PiuContentFitVertically,
	PiuContentHit,
	PiuContentIdle,
	PiuContentInvalidate,
	PiuContentMeasureHorizontally,
	PiuContentMeasureVertically,
	PiuContentPlace,
	NULL,
	NULL,
	PiuContentReflow,
	PiuContentShowing,
	PiuContentShown,
	PiuContentSync,
	PiuScreenUnbind,
	PiuContentUpdate
};

const xsHostHooks ICACHE_FLASH_ATTR PiuScreenHooks = {
	PiuScreenDelete,
	PiuScreenMark,
	NULL
};

void PiuScreenBind(void* it, PiuApplication* application, PiuView* view)
{
	PiuScreen* self = it;
	PiuContentBind(it, application, view);
	
    NSPiuScreenView *screenView = [[NSPiuScreenView alloc] init];
	screenView.piuScreen = self;

	NSPiuClipView *clipView = [[NSPiuClipView alloc] init];
    [clipView setDocumentView:screenView];
    [clipView setDrawsBackground:NO];
	clipView.piuContent = (PiuContent*)self;
	
	(*self)->nsClipView = clipView;
	(*self)->nsScreenView = screenView;
	
	PiuDimension width = (*self)->coordinates.width;
	PiuDimension height = (*self)->coordinates.height;
    txScreen* screen = malloc(sizeof(txScreen) - 1 + (width * height * 4));
    memset(screen, 0, sizeof(txScreen) - 1 + (width * height * 4));
    screen->view = screenView;
    screen->abort = fxScreenAbort;
    screen->bufferChanged = fxScreenBufferChanged;
    screen->createWorker = fxScreenCreateWorker;
    screen->deleteWorker = fxScreenDeleteWorker;
    screen->formatChanged = fxScreenFormatChanged;
    screen->post = fxScreenPost;
    screen->start = fxScreenStart;
    screen->stop = fxScreenStop;
    screen->width = width;
    screen->height = height;
    (*self)->screen = screen;
    
    screenView.screen = screen;
    
    NSProcessInfo* processInfo = [NSProcessInfo processInfo];
    NSTimeInterval time = [processInfo systemUptime];
    NSDate* date = [NSDate dateWithTimeIntervalSinceNow:-time];
    screenView.time = [date timeIntervalSince1970];
    screenView.timer = nil;
  
    screenView.touchImage = [NSImage imageNamed:@"fingerprint"];
	screenView.touches = (id *)malloc(10 * sizeof(id));
    int i;
    for (i = 0; i < 10; i++)
		screenView.touches[i] = nil;
	 screenView.touching = NO;
	 
    [(*view)->nsView addSubview:clipView];
}

void PiuScreenDictionary(xsMachine* the, void* it) 
{
}

void PiuScreenDelete(void* it) 
{
}

void PiuScreenMark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	PiuContentMark(the, it, markRoot);
}

void PiuScreenUnbind(void* it, PiuApplication* application, PiuView* view)
{
	PiuScreen* self = it;
    [(*self)->nsClipView removeFromSuperview];
	free((*self)->screen);
	(*self)->screen = NULL;
    [(*self)->nsClipView release];
	(*self)->nsClipView = NULL;
    [(*self)->nsScreenView release];
	(*self)->nsScreenView = NULL;
	PiuContentUnbind(it, application, view);
}

void PiuScreenMessageBuild(PiuScreenMessage message, char* buffer, int size) 
{
	message->size = size;
	if (size < 0)
		message->buffer = buffer;
	else {
		if (!size)
			size = strlen(buffer) + 1;
		message->buffer = malloc(size);
		if (message->buffer)
			memcpy(message->buffer, buffer, size);
	}
}
void PiuScreen_create(xsMachine* the)
{
	PiuScreen* self;
	xsVars(2);
	xsSetHostChunk(xsThis, NULL, sizeof(PiuScreenRecord));
	self = PIU(Screen, xsThis);
	(*self)->the = the;
	(*self)->reference = xsToReference(xsThis);
	xsSetHostHooks(xsThis, (xsHostHooks*)&PiuScreenHooks);
	(*self)->dispatch = (PiuDispatch)&PiuScreenDispatchRecord;
	(*self)->flags = piuVisible;
	PiuContentDictionary(the, self);
	PiuScreenDictionary(the, self);
	PiuBehaviorOnCreate(self);
}

void PiuScreen_get_running(xsMachine* the)
{
	PiuScreen* self = PIU(Screen, xsThis);
	xsResult = (*self)->nsScreenView.library ? xsTrue : xsFalse;
}
	
void PiuScreen_launch(xsMachine* the)
{
	PiuScreen* self = PIU(Screen, xsThis);
	xsStringValue path = xsToString(xsArg(0));
    [(*self)->nsScreenView launchMachine:[NSString stringWithUTF8String:path]];
}
	
void PiuScreen_postMessage(xsMachine* the)
{
	PiuScreen* self = PIU(Screen, xsThis);
	PiuScreenMessageRecord message;
	if (xsToInteger(xsArgc) > 1) {
		int worker = (int)xsToInteger(xsArg(1));
		PiuScreenMessageBuild(&message, xsMarshallAlien(xsArg(0)), 0 - worker);
	}
	else {
		if (xsIsInstanceOf(xsArg(0), xsArrayBufferPrototype)) {
			int size = (int)xsGetArrayBufferLength(xsArg(0));
			PiuScreenMessageBuild(&message, xsToArrayBuffer(xsArg(0)), size);
		}
		else {
			xsStringValue string = xsToString(xsArg(0));
			PiuScreenMessageBuild(&message, string, 0);
		}
	}
	if (message.buffer) {
		NSData* data = [NSData dataWithBytes:&message length:sizeof(PiuScreenMessageRecord)];
		[(*self)->nsScreenView performSelectorOnMainThread:@selector(messageToApplication:) withObject:data waitUntilDone:NO];
	}
}

void PiuScreen_quit(xsMachine* the)
{
	PiuScreen* self = PIU(Screen, xsThis);
    [(*self)->nsScreenView quitMachine];
}

void fxScreenAbort(txScreen* screen)
{
	NSPiuScreenView *screenView = screen->view;
    [screenView performSelectorOnMainThread:@selector(quitMachine) withObject:nil waitUntilDone:NO];
}

void fxScreenBufferChanged(txScreen* screen)
{
	NSPiuScreenView *screenView = screen->view;
	[screenView display];
}

int fxScreenCreateWorker(txScreen* screen, char* name)
{
	NSPiuScreenView *screenView = screen->view;
	PiuScreen* self = screenView.piuScreen;
	int worker = 0;
	xsBeginHost((*self)->the);
	{
		xsVars(3);
		xsVar(0) = xsReference((*self)->behavior);
		if (xsFindResult(xsVar(0), xsID_onCreateWorker)) {
			xsVar(1) = xsReference((*self)->reference);
			xsVar(2) = xsString(name);
			xsResult = xsCallFunction2(xsResult, xsVar(0), xsVar(1), xsVar(2));
			worker = xsToInteger(xsResult);
		}
	}
	xsEndHost((*self)->the);
	return worker;
}

void fxScreenDeleteWorker(txScreen* screen, int worker)
{
	NSPiuScreenView *screenView = screen->view;
	PiuScreen* self = screenView.piuScreen;
	xsBeginHost((*self)->the);
	{
		xsVars(3);
		xsVar(0) = xsReference((*self)->behavior);
		if (xsFindResult(xsVar(0), xsID_onDeleteWorker)) {
			xsVar(1) = xsReference((*self)->reference);
			xsVar(2) = xsInteger(worker);
			(void)xsCallFunction2(xsResult, xsVar(0), xsVar(1), xsVar(2));
		}
	}
	xsEndHost((*self)->the);
}

void fxScreenFormatChanged(txScreen* screen)
{
}

void fxScreenPost(txScreen* screen, char* buffer, int size)
{
	NSPiuScreenView* screenView = screen->view;
	PiuScreenMessageRecord message;
	PiuScreenMessageBuild(&message, buffer, size);
	if (message.buffer) {
		NSData* data = [NSData dataWithBytes:&message length:sizeof(PiuScreenMessageRecord)];
		[screenView performSelectorOnMainThread:@selector(messageToHost:) withObject:data waitUntilDone:NO];
	}
}

void fxScreenStart(txScreen* screen, double interval)
{
	NSPiuScreenView *screenView = screen->view;
	screenView.timer = [NSTimer scheduledTimerWithTimeInterval:interval/1000 target:screenView selector:@selector(timerCallback:) userInfo:nil repeats:YES];
}

void fxScreenStop(txScreen* screen)
{
	NSPiuScreenView *screenView = screen->view;
	if (screenView.timer)
		[screenView.timer invalidate];
	screenView.timer = nil;
}


