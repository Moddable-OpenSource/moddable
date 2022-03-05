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

#include "piuAll.h"
#include <CoreGraphics/CoreGraphics.h>
#include <CoreText/CoreText.h>
#import <Cocoa/Cocoa.h>

#include "mc.defines.h"

@interface NSPiuAppDelegate : NSObject {
	xsMachine* machine;
	PiuApplication* piuApplication;
	BOOL shouldQuit;
}
@property (assign) xsMachine* machine;
@property (assign) PiuApplication* piuApplication;
@property (assign) BOOL shouldQuit;
@end

@interface NSPiuWindow : NSWindow {
}
@end

@interface NSPiuView : NSView {
	PiuView* piuView;
    NSTimeInterval time;
	NSTimer *timer;
}
@property (assign) PiuView* piuView;
@property (assign) NSTimeInterval time;
@property (assign) NSTimer *timer;
@end

@interface NSPiuClipView : NSClipView {
	PiuContent* piuContent;
}
@property (assign) PiuContent* piuContent;
@end

struct PiuFontStruct {
	PiuHandlePart;
	xsMachine* the;
	PiuFont* next;
	PiuFlags flags;
	xsIdentifier family;
	PiuCoordinate size;
	PiuCoordinate weight;
	CTFontRef fref;
	CGFloat ascent;
	CGFloat height;
};

struct PiuTextureStruct {
	PiuHandlePart;
	PiuAssetPart;
	NSImage* image;
	xsNumberValue scale;
	PiuDimension width;
	PiuDimension height;
};

struct PiuViewStruct {
	PiuHandlePart;
	xsMachine* the;
	PiuApplication* application;
	NSPiuView *nsView;
	NSPiuWindow *nsWindow;
	CGContextRef context;
	CGColorSpaceRef colorSpace;
	CGColorRef color;
	PiuBoolean filtered;
	PiuBoolean transparent;
	PiuBoolean appearanceChanged;
};

extern xsMachine* ServiceThreadMain(void* context);
extern void PiuViewDrawRoundContent(PiuView* self, PiuCoordinate x, PiuCoordinate y, PiuDimension w, PiuDimension h, PiuDimension radius, PiuDimension border, PiuVariant variant, PiuColor fillColor, PiuColor strokeColor);
