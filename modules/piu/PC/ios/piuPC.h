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
#import <UIKit/UIKit.h>



@interface UIPiuAppDelegate : UIResponder <UIApplicationDelegate> {
	xsMachine* machine;
	PiuApplication* piuApplication;
	BOOL shouldQuit;
}
@property (assign) xsMachine* machine;
@property (assign) PiuApplication* piuApplication;
@property (assign) BOOL shouldQuit;
@end

@interface UIPiuWindow : UIWindow {
}
@end

@interface UIPiuView : UIView {
	PiuView* piuView;
  NSTimeInterval time;
	NSTimer *timer;
}
@property (assign) PiuView* piuView;
@property (assign) NSTimeInterval time;
@property (assign) NSTimer *timer;
@end

@interface UIPiuViewController : UIViewController {
}
- (void)hideStatusBar;
- (void)showStatusBar;
- (void)setDefaultStatusBar;
- (void)setLightStatusBar;
@end


@interface UIPiuClipView : UIView {
	PiuContent* piuContent;
}
@property (assign) PiuContent* piuContent;
@end

struct PiuFontStruct {
	PiuHandlePart;
	xsMachine* the;
	PiuFont* next;
	PiuFlags flags;
	xsIndex family;
	PiuCoordinate size;
	PiuCoordinate weight;
	CTFontRef fref;
	CGFloat ascent;
	CGFloat height;
};

struct PiuTextureStruct {
	PiuHandlePart;
	PiuAssetPart;
	CGImageRef image;
	xsNumberValue scale;
	PiuDimension width;
	PiuDimension height;
};

struct PiuViewStruct {
	PiuHandlePart;
	xsMachine* the;
	PiuApplication* application;
	UIPiuView *uiView;
	UIPiuViewController *uiViewController;
	UIPiuWindow *uiWindow;
	CGContextRef context;
	CGColorSpaceRef colorSpace;
	CGColorRef color;
};

extern xsMachine* ServiceThreadMain();
