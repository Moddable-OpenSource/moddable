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
	int archiveFile;
	int archiveSize;
	txScreen* screen;
    NSTimeInterval time;
	NSTimer *timer;
}
@property (assign) PiuScreen* piuScreen;
@property (assign) void* library;
@property (assign) int archiveFile;
@property (assign) int archiveSize;
@property (assign) txScreen *screen;
@property (assign) NSTimeInterval time;
@property (assign) NSTimer *timer;
- (void)abortMachine:(NSObject *)object;
- (void)launchMachine:(NSString*)libraryPath with:(NSString*)archivePath;
- (void)quitMachine;
@end

static void fxScreenAbort(txScreen* screen, int status);
static void fxScreenBufferChanged(txScreen* screen);
static void fxScreenFormatChanged(txScreen* screen);
static void fxScreenPost(txScreen* screen, char* message, int size);
static void fxScreenRecordTouch(txScreen* screen, int kind, int index, int x, int y);
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
	PiuRectangleRecord hole;
	xsIntegerValue rotation;
	xsNumberValue transparency;
	xsBooleanValue circular;
	PiuPointRecord fingerprint;
};

struct PiuScreenMessageStruct {
	void* buffer;
	int size;
};

enum {
	piuRecordingTouches = 1 << 24,
	piuPlayingTouches = 1 << 25,
	piuDisplayFingerprint = 1 << 26,
};

@implementation NSPiuScreenView
@synthesize piuScreen;
@synthesize library;
@synthesize archiveFile;
@synthesize archiveSize;
@synthesize screen;
@synthesize time;
@synthesize timer;
- (void)dealloc {
	if (library)
    	dlclose(library);
    [super dealloc];
}
- (void)abortMachine:(NSObject *)object {
	NSData* data = (NSData*)object;
	int status;
	[data getBytes:&status length: sizeof(status)];
	if (piuScreen && (*piuScreen)->behavior) {
		xsBeginHost((*piuScreen)->the);
		xsVars(3);
		xsVar(0) = xsReference((*piuScreen)->behavior);
		if (xsFindResult(xsVar(0), xsID_onAbort)) {
			xsVar(1) = xsReference((*piuScreen)->reference);
			xsVar(2) = xsInteger(status);
			(void)xsCallFunction2(xsResult, xsVar(0), xsVar(1), xsVar(2));
		}
		xsEndHost((*piuScreen)->the);
	}
}
- (void)drawRect:(NSRect)rect {
	CGRect dstRect = NSRectToCGRect(self.bounds);
	CGRect srcRect = CGRectMake(0, 0, screen->width, screen->height);
	CGContextRef context = (CGContextRef)[[NSGraphicsContext currentContext] CGContext];
    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
	CGDataProviderRef provider = CGDataProviderCreateWithData(nil, screen->buffer, screen->width * screen->height * screenBytesPerPixel, nil);
    CGImageRef image = CGImageCreate(screen->width, screen->height, 8, 32, screen->width * screenBytesPerPixel, colorSpace, kCGBitmapByteOrder32Big | kCGImageAlphaNoneSkipLast, provider, nil, NO, kCGRenderingIntentDefault);
	PiuRectangle hole = &(*piuScreen)->hole;
	if (!PiuRectangleIsEmpty(hole)) {
		CGContextAddRect(context, dstRect);
		CGContextAddRect(context, CGRectMake(hole->x, dstRect.size.height - hole->height - hole->y, hole->width, hole->height));
		CGContextEOClip(context);
	}
	else if ((*piuScreen)->circular) {
		CGContextAddEllipseInRect(context, dstRect);
		CGContextEOClip(context);
	}
	CGContextTranslateCTM(context, dstRect.size.width/2, dstRect.size.height/2);
	CGContextRotateCTM(context, (*piuScreen)->rotation * (M_PI/180.0));
	CGContextTranslateCTM(context, -srcRect.size.width/2, -srcRect.size.height/2);
	CGContextSetAlpha(context, 1.0 - (*piuScreen)->transparency);
	CGContextDrawImage(context, srcRect, image);
	CGContextSetAlpha(context, 1.0);
	CGImageRelease(image);
	CGDataProviderRelease(provider);
	CGColorSpaceRelease(colorSpace);
	if ((*piuScreen)->flags & piuDisplayFingerprint) {
		PiuSkin* skin = (*piuScreen)->skin;
		if (skin) {
			PiuTexture* texture = (*skin)->data.pattern.texture;
			if (skin) {
				NSImage* image = (*texture)->image;
				if (image) {
					NSSize size = [image size];
					NSRect source = NSMakeRect(0, 0, size.width, size.height);
					NSRect destination = NSMakeRect((*piuScreen)->fingerprint.x - (size.width / 2), srcRect.size.height - (*piuScreen)->fingerprint.y - (size.height / 2), size.width, size.height);
					[image drawInRect:destination fromRect:source operation:NSCompositingOperationSourceOver fraction:1 respectFlipped:YES hints:nil];
				}
			}
		}
	}
}
- (void)launchMachine:(NSString*)libraryPath with:(NSString*)archivePath {
	NSString *name = nil;
	NSString *info = nil;
	txScreenLaunchProc launch;
	self.library = dlopen([libraryPath UTF8String], RTLD_NOW | RTLD_LOCAL);
	if (!self.library) {
		name = libraryPath;
		info = [NSString stringWithFormat:@"%s", dlerror()];
		goto bail;
	}
	launch = (txScreenLaunchProc)dlsym(self.library, "fxScreenLaunch");
	if (!launch) {
		name = libraryPath;
		info = [NSString stringWithFormat:@"%s", dlerror()];
		goto bail;
	}
	if (archivePath) {
		struct stat statbuf;
		self.archiveFile = open([archivePath fileSystemRepresentation], O_RDWR);
		if (self.archiveFile < 0) {
			name = archivePath;
			info = [NSString stringWithFormat:@"%s", strerror(errno)];
			goto bail;
		}
		fstat(self.archiveFile, &statbuf);
		self.archiveSize = statbuf.st_size;
		self.screen->archive = mmap(NULL, self.archiveSize, PROT_READ|PROT_WRITE, MAP_SHARED, self.archiveFile, 0);
		if (self.screen->archive == MAP_FAILED) {
			self.screen->archive = NULL;
			name = archivePath;
			info = [NSString stringWithFormat:@"%s", strerror(errno)];
			goto bail;
		}
	}
	(*launch)(self.screen);
	return;
bail:
	if (self.screen->archive) {
		munmap(self.screen->archive, self.archiveSize);
		self.screen->archive = NULL;
		self.archiveSize = 0;
	}
	if (self.archiveFile >= 0) {
		close(self.archiveFile);
		self.archiveFile = -1;
	}
	if (self.library) {
		dlclose(self.library);
		self.library = nil;
	}
	NSAlert *alert = [[[NSAlert alloc] init] autorelease];
	[alert setMessageText:[NSString stringWithFormat:@"Cannot open \"%@\"", name]];
	if (info)
		[alert setInformativeText:info];
	[alert runModal];
}
- (void)messageToApplication:(NSObject *)object {
	if (self.screen && self.screen->invoke) {
		NSData* data = (NSData*)object;
		PiuScreenMessage message = (PiuScreenMessage)[data bytes];
		(*self.screen->invoke)(self.screen, message->buffer, message->size);
		free(message->buffer);
	}
}
- (void)messageToHost:(NSObject *)object {
	NSData* data = (NSData*)object;
	PiuScreenMessage message = (PiuScreenMessage)[data bytes];
	if (piuScreen && (*piuScreen)->behavior) {
		xsBeginHost((*piuScreen)->the);
		xsVars(3);
		xsVar(0) = xsReference((*piuScreen)->behavior);
		if (xsFindResult(xsVar(0), xsID_onMessage)) {
			xsVar(1) = xsReference((*piuScreen)->reference);
			if (message->size > 0)
				xsVar(2) = xsArrayBuffer(message->buffer, message->size);
			else
				xsVar(2) = xsString(message->buffer);
			(void)xsCallFunction2(xsResult, xsVar(0), xsVar(1), xsVar(2));
		}
		xsEndHost((*piuScreen)->the);
	}
	free(message->buffer);
}
- (void)mouseDown:(NSEvent *)event {
	if ((*piuScreen)->flags & piuPlayingTouches) 
		return;
	if (self.screen->touch) {
		NSPoint point = [event locationInWindow];
		point = [self convertPoint:point fromView:nil];
		point = [self rotatePoint:point];
		if ((*piuScreen)->flags & piuRecordingTouches) 
			fxScreenRecordTouch(self.screen, touchEventBeganKind, 0, point.x, point.y);
		(*self.screen->touch)(self.screen, touchEventBeganKind, 0, point.x, point.y, 0);
	}
}
- (void)mouseDragged:(NSEvent *)event {
	if ((*piuScreen)->flags & piuPlayingTouches) 
		return;
	if (self.screen->touch)  {
		NSPoint point = [event locationInWindow];
		point = [self convertPoint:point fromView:nil];
		point = [self rotatePoint:point];
		if ((*piuScreen)->flags & piuRecordingTouches) 
			fxScreenRecordTouch(self.screen, touchEventMovedKind, 0, point.x, point.y);
		(*self.screen->touch)(self.screen, touchEventMovedKind, 0, point.x, point.y, 0);
	}
}
- (void)mouseUp:(NSEvent *)event {
	if ((*piuScreen)->flags & piuPlayingTouches) 
		return;
	if (self.screen->touch) {
		NSPoint point = [event locationInWindow];
		point = [self convertPoint:point fromView:nil];
		point = [self rotatePoint:point];
		if ((*piuScreen)->flags & piuRecordingTouches) 
			fxScreenRecordTouch(self.screen, touchEventEndedKind, 0, point.x, point.y);
		(*self.screen->touch)(self.screen, touchEventEndedKind, 0, point.x, point.y, 0);
	}
}
- (void)quitMachine {
	if (self.screen->quit) 
		(*self.screen->quit)(self.screen);
	if (self.screen->archive) {
		munmap(self.screen->archive, self.archiveSize);
		close(self.archiveFile);
		self.screen->archive = NULL;
		self.archiveSize = 0;
		self.archiveFile = -1;
	}
	if (self.library) {
    	dlclose(self.library);
    	self.library = nil;
    }
    [self display];
}
- (NSPoint)rotatePoint:(NSPoint)point {
	NSSize size = [self bounds].size;
    NSPoint result;
    switch ((*piuScreen)->rotation) {
    case 0:
    	result.x = point.x;
		result.y = size.height - point.y;
		break;
    case 90:
    	result.x = point.y;
        result.y = point.x;
		break;
    case 180:
    	result.x = size.width - point.x;
		result.y = point.y;
		break;
    case 270:
    	result.x = size.height - point.y;
		result.y = size.width - point.x;
		break;
    }
    return result;
}
- (void)timerCallback:(NSTimer*)theTimer {
	if (self.screen->idle) 
		(*self.screen->idle)(self.screen);
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
	if (((*self)->rotation == 90) || ((*self)->rotation == 270)) {
		PiuDimension tmp = width;
		width = height;
		height = tmp;
	}
    txScreen* screen = malloc(sizeof(txScreen) - 1 + (width * height * 4));
    memset(screen, 0, sizeof(txScreen) - 1 + (width * height * 4));
    screen->view = screenView;
    screen->abort = fxScreenAbort;
    screen->bufferChanged = fxScreenBufferChanged;
    screen->formatChanged = fxScreenFormatChanged;
    screen->post = fxScreenPost;
    screen->start = fxScreenStart;
    screen->stop = fxScreenStop;
	mxCreateMutex(&screen->workersMutex);
    screen->width = width;
    screen->height = height;
    (*self)->screen = screen;
    
    screenView.screen = screen;
    
    NSProcessInfo* processInfo = [NSProcessInfo processInfo];
    NSTimeInterval time = [processInfo systemUptime];
    NSDate* date = [NSDate dateWithTimeIntervalSinceNow:-time];
    screenView.time = [date timeIntervalSince1970];
    screenView.timer = nil;
    [(*view)->nsView addSubview:clipView];
}

void PiuScreenDictionary(xsMachine* the, void* it) 
{
	PiuScreen* self = it;
	xsIntegerValue integer;
	xsBooleanValue boolean;
	if (xsFindInteger(xsArg(1), xsID_rotation, &integer)) {
		(*self)->rotation = integer;
	}
	if (xsFindBoolean(xsArg(1), xsID_circular, &boolean)) {
		(*self)->circular = boolean;
	}
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
	mxDeleteMutex(&((*self)->screen->workersMutex));
	free((*self)->screen);
	(*self)->screen = NULL;
    [(*self)->nsClipView release];
	(*self)->nsClipView = NULL;
	(*self)->nsScreenView.screen = NULL;
	(*self)->nsScreenView.piuScreen = NULL;
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

void PiuScreen_get_hole(xsMachine* the)
{
	PiuScreen* self = PIU(Screen, xsThis);
	xsResult = xsNewObject();
	xsDefine(xsResult, xsID_x, xsPiuCoordinate((*self)->hole.x), xsDefault);
	xsDefine(xsResult, xsID_y, xsPiuCoordinate((*self)->hole.y), xsDefault);
	xsDefine(xsResult, xsID_width, xsPiuDimension((*self)->hole.width), xsDefault);
	xsDefine(xsResult, xsID_height, xsPiuDimension((*self)->hole.height), xsDefault);
}

void PiuScreen_set_hole(xsMachine* the)
{
	PiuScreen* self = PIU(Screen, xsThis);
	xsIntegerValue value;
	if (xsFindInteger(xsArg(0), xsID_x, &value)) {
		(*self)->hole.x = value;
	}
	if (xsFindInteger(xsArg(0), xsID_y, &value)) {
		(*self)->hole.y = value;
	}
	if (xsFindInteger(xsArg(0), xsID_width, &value)) {
		(*self)->hole.width = value;
	}
	if (xsFindInteger(xsArg(0), xsID_height, &value)) {
		(*self)->hole.height = value;
	}
	PiuContentInvalidate(self, NULL);
	[(*self)->nsScreenView display];
}

void PiuScreen_get_pixelFormat(xsMachine* the)
{
	PiuScreen* self = PIU(Screen, xsThis);
	xsResult = xsInteger((*self)->screen->pixelFormat);
}

void PiuScreen_get_playingTouches(xsMachine* the)
{
	PiuScreen* self = PIU(Screen, xsThis);
	xsResult = ((*self)->flags & piuPlayingTouches) ? xsTrue : xsFalse;
}

void PiuScreen_set_playingTouches(xsMachine* the)
{
	PiuScreen* self = PIU(Screen, xsThis);
	if (xsTest(xsArg(0)))
		(*self)->flags |= piuPlayingTouches;
	else
		(*self)->flags &= ~piuPlayingTouches;
}

void PiuScreen_get_recordingTouches(xsMachine* the)
{
	PiuScreen* self = PIU(Screen, xsThis);
	xsResult = ((*self)->flags & piuRecordingTouches) ? xsTrue : xsFalse;
}

void PiuScreen_set_recordingTouches(xsMachine* the)
{
	PiuScreen* self = PIU(Screen, xsThis);
	if (xsTest(xsArg(0)))
		(*self)->flags |= piuRecordingTouches;
	else
		(*self)->flags &= ~piuRecordingTouches;
}

void PiuScreen_get_running(xsMachine* the)
{
	PiuScreen* self = PIU(Screen, xsThis);
	xsResult = (*self)->nsScreenView.library ? xsTrue : xsFalse;
}
	
void PiuScreen_get_transparency(xsMachine* the)
{
	PiuScreen* self = PIU(Screen, xsThis);
	xsResult = xsNumber((*self)->transparency);
}

void PiuScreen_set_transparency(xsMachine* the)
{
	PiuScreen* self = PIU(Screen, xsThis);
	(*self)->transparency = xsToNumber(xsArg(0));
	PiuContentInvalidate(self, NULL);
	[(*self)->nsScreenView display];
}

void PiuScreen_launch(xsMachine* the)
{
	PiuScreen* self = PIU(Screen, xsThis);
	xsStringValue libraryPath = xsToString(xsArg(0));
	xsStringValue archivePath = (xsToInteger(xsArgc) > 1) ? xsToString(xsArg(1)) : NULL;
    [(*self)->nsScreenView launchMachine:[NSString stringWithUTF8String:libraryPath] 
    	with:archivePath ? [NSString stringWithUTF8String:archivePath] : NULL];
}
	
void PiuScreen_postMessage(xsMachine* the)
{
	PiuScreen* self = PIU(Screen, xsThis);
	PiuScreenMessageRecord message;
	if (xsIsInstanceOf(xsArg(0), xsArrayBufferPrototype)) {
		int size = (int)xsGetArrayBufferLength(xsArg(0));
		PiuScreenMessageBuild(&message, xsToArrayBuffer(xsArg(0)), size);
	}
	else {
		xsStringValue string = xsToString(xsArg(0));
		PiuScreenMessageBuild(&message, string, 0);
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

void PiuScreen_touch(xsMachine* the)
{
	PiuScreen* self = PIU(Screen, xsThis);
	txScreen* screen = (*self)->screen;
	if (screen->touch) {
		xsIntegerValue kind = xsToInteger(xsArg(0));
		xsIntegerValue index = xsToInteger(xsArg(1));
		xsIntegerValue x = xsToInteger(xsArg(2));
		xsIntegerValue y = xsToInteger(xsArg(3));
		if (kind == 0)
			(*self)->flags |= piuDisplayFingerprint;
		else if ((kind == 1) || (kind == 2))
			(*self)->flags &= ~piuDisplayFingerprint;
		(*self)->fingerprint.x = x;
		(*self)->fingerprint.y = y;
		fxScreenBufferChanged(screen);
		(*screen->touch)(screen, kind, index, x, y, 0);
	}
}

void PiuScreen_writePNG(xsMachine* the)
{
	PiuScreen* self = PIU(Screen, xsThis);
	txScreen* screen = (*self)->screen;
	CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
	CGDataProviderRef provider = CGDataProviderCreateWithData(nil, screen->buffer, screen->width * screen->height * screenBytesPerPixel, nil);
	CGImageRef image = CGImageCreate(screen->width, screen->height, 8, 32, screen->width * screenBytesPerPixel, colorSpace, kCGBitmapByteOrder32Big | kCGImageAlphaNoneSkipLast, provider, nil, NO, kCGRenderingIntentDefault);
    CFStringRef path = CFStringCreateWithCString(NULL, xsToString(xsArg(0)), kCFStringEncodingUTF8);
	CFURLRef url = CFURLCreateWithFileSystemPath(NULL, path, kCFURLPOSIXPathStyle, false);
    CFStringRef type = CFStringCreateWithCString(NULL, "public:png", kCFStringEncodingUTF8);
	CGImageDestinationRef destination = CGImageDestinationCreateWithURL(url, type, 1, NULL);
    CGImageDestinationAddImage(destination, image, NULL);
	CGImageDestinationFinalize(destination);
}

void fxScreenAbort(txScreen* screen, int status)
{
	NSPiuScreenView *screenView = screen->view;
	NSData* data = [NSData dataWithBytes:&status length:sizeof(status)];
    [screenView performSelectorOnMainThread:@selector(abortMachine:) withObject:data waitUntilDone:NO];
}

void fxScreenBufferChanged(txScreen* screen)
{
	NSPiuScreenView *screenView = screen->view;
	[screenView display];
}

void fxScreenFormatChanged(txScreen* screen)
{
	NSPiuScreenView *screenView = screen->view;
	PiuScreen* self = screenView.piuScreen;
	if ((*self)->behavior) {
		xsBeginHost((*self)->the);
		xsVars(3);
		xsVar(0) = xsReference((*self)->behavior);
		if (xsFindResult(xsVar(0), xsID_onPixelFormatChanged)) {
			xsVar(1) = xsReference((*self)->reference);
			xsVar(2) = xsInteger(screen->pixelFormat);
			(void)xsCallFunction2(xsResult, xsVar(0), xsVar(1), xsVar(2));
		}
		xsEndHost((*self)->the);
	}
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

void fxScreenRecordTouch(txScreen* screen, int kind, int index, int x, int y)
{
	NSPiuScreenView *screenView = screen->view;
	PiuScreen* self = screenView.piuScreen;
	if ((*self)->behavior) {
		xsBeginHost((*self)->the);
		xsVars(6);
		xsVar(0) = xsReference((*self)->behavior);
		if (xsFindResult(xsVar(0), xsID_onRecordTouch)) {
			xsVar(1) = xsReference((*self)->reference);
			xsVar(2) = xsInteger(kind);
			xsVar(3) = xsInteger(index);
			xsVar(4) = xsInteger(x);
			xsVar(5) = xsInteger(y);
			(void)xsCallFunction5(xsResult, xsVar(0), xsVar(1), xsVar(2), xsVar(3), xsVar(4), xsVar(5));
		}
		xsEndHost((*self)->the);
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

extern void PiuScreenWorkerCreateAux(xsMachine* the, txScreen* screen);

void PiuScreenWorkerCreate(xsMachine* the)
{
	PiuScreen* self = PIU(Screen, xsArg(1));
	PiuScreenWorkerCreateAux(the, (*self)->screen);
}
