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

@implementation UIPiuAppDelegate
@synthesize machine;
@synthesize piuApplication;
@synthesize shouldQuit;
- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
  self.machine = ServiceThreadMain();
	xsBeginHost(self.machine);
	{
		xsCollectGarbage();
		xsResult = xsCall1(xsGlobal, xsID_require, xsString("main"));
		self.piuApplication = PIU(Application, xsResult);
		xsCollectGarbage();
	}
	xsEndHost();
	return YES;
}
- (void)applicationWillResignActive:(UIApplication *)application {
    // Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
    // Use this method to pause ongoing tasks, disable timers, and invalidate graphics rendering callbacks. Games should use this method to pause the game.
}
- (void)applicationDidEnterBackground:(UIApplication *)application {
    // Use this method to release shared resources, save user data, invalidate timers, and store enough application state information to restore your application to its current state in case it is terminated later.
    // If your application supports background execution, this method is called instead of applicationWillTerminate: when the user quits.
}
- (void)applicationWillEnterForeground:(UIApplication *)application {
    // Called as part of the transition from the background to the active state; here you can undo many of the changes made on entering the background.
}
- (void)applicationDidBecomeActive:(UIApplication *)application {
    // Restart any tasks that were paused (or not yet started) while the application was inactive. If the application was previously in the background, optionally refresh the user interface.
}
- (void)applicationWillTerminate:(UIApplication *)application {
    // Called when the application is about to terminate. Save data if appropriate. See also applicationDidEnterBackground:.
}
/*
- (void)application:(UIApplication *)application didChangeStatusBarFrame:(CGRect)oldStatusBarFrame {
    [[NSNotificationCenter defaultCenter] postNotificationName:@"trigger" object:self userInfo:NULL];
}
*/
@end

@implementation UIPiuWindow

@end

@implementation UIPiuViewController
BOOL StatusBarShown;
BOOL StatusBarLight;


- (void)hideStatusBar {
	StatusBarShown=NO;
	[self setNeedsStatusBarAppearanceUpdate];
}
- (void)setDefaultStatusBar {
	StatusBarLight=NO;
	[self setNeedsStatusBarAppearanceUpdate];
}
- (void)setLightStatusBar {
	StatusBarLight=YES;
	[self setNeedsStatusBarAppearanceUpdate];
}
- (void)showStatusBar {
	StatusBarShown=YES;
	[self setNeedsStatusBarAppearanceUpdate];

}
- (BOOL)prefersStatusBarHidden {
	return !StatusBarShown;
}
-(UIStatusBarStyle)preferredStatusBarStyle {
  if (StatusBarLight) {
  	return UIStatusBarStyleLightContent;
  }
  else {
    return UIStatusBarStyleDefault;
  }
}
@end

@implementation UIPiuView
@synthesize piuView;
@synthesize time;
@synthesize timer;


- (BOOL)canBecomeFirstResponder {
  return YES;
}
- (BOOL)becomeFirstResponder {
  BOOL result = [super becomeFirstResponder];
	if (piuView) {
		PiuApplication* application = (*piuView)->application;
		if (application && result)
			PiuApplicationSetFocus(application, NULL);
	}
  return result;
}
- (void)dealloc {
    [super dealloc];
}
- (void)drawRect:(CGRect)rect {
	if (piuView) {
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
			(*piuView)->context = (CGContextRef)UIGraphicsGetCurrentContext();
			(*(*application)->dispatch->update)(application, piuView, &area);
			(*piuView)->context = NULL;
		}
		xsEndHost((*piuView)->the);
	}
	else {
		CGContextRef context = (CGContextRef)UIGraphicsGetCurrentContext();
		CGContextSetRGBFillColor(context, 255, 255, 255, 1);
		CGContextFillRect(context, rect);
	}
}
- (void)layoutSubviews {
    if (piuView) {
        xsBeginHost((*piuView)->the);
        PiuApplicationResize((*piuView)->application);
        xsEndHost((*piuView)->the);
    }
}
- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
	UITouch *touch = [touches allObjects][0];
	NSTimeInterval when = 1000 * (time + [touch timestamp]);
	CGPoint point = [touch locationInView:self];
    [self becomeFirstResponder];
	if (piuView) {
		xsBeginHost((*piuView)->the);
		{
			PiuApplication* application = (*piuView)->application;
			PiuApplicationTouchBegan(application, 0, point.x, point.y, when);
			PiuApplicationAdjust(application);
		}
		xsEndHost((*piuView)->the);
	}
}
- (void)touchesCancelled:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
}
- (void)touchesMoved:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
	UITouch *touch = [touches allObjects][0];
	NSTimeInterval when = 1000 * (time + [touch timestamp]);
	CGPoint point = [touch locationInView:self];
	if (piuView) {
		xsBeginHost((*piuView)->the);
		{
			PiuApplication* application = (*piuView)->application;
			PiuApplicationTouchMoved(application, 0, point.x, point.y, when);
		}
		xsEndHost((*piuView)->the);
	}
}
- (void)touchesEnded:(NSSet<UITouch *> *)touches withEvent:(UIEvent *)event {
	UITouch *touch = [touches allObjects][0];
	NSTimeInterval when = 1000 * (time + [touch timestamp]);
	CGPoint point = [touch locationInView:self];
	if (piuView) {
		xsBeginHost((*piuView)->the);
		{
			PiuApplication* application = (*piuView)->application;
			PiuApplicationTouchEnded(application, 0, point.x, point.y, when);
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
@end

@implementation UIPiuClipView
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
     NSArray* subviews = [(*self)->uiView subviews];
     for (UIView* subView in subviews) {
     	if ([subView isKindOfClass:[UIPiuClipView class]]) {
     		UIPiuClipView* clipView = (UIPiuClipView*)subView;
     		UIView* view = clipView.subviews[0];
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
 			CGRect frame;
 			frame.origin.x = -(clipBounds.x - bounds.x);
 			frame.origin.y = -(clipBounds.y - bounds.y);
 			frame.size.width = bounds.width;
 			frame.size.height = bounds.height;
 			[view setFrame:frame];
 			frame.origin.x = clipBounds.x;
 			frame.origin.y = clipBounds.y;
 			frame.size.width = clipBounds.width;
 			frame.size.height = clipBounds.height;
 			[clipView setFrame:frame];
     	}
     }
}

void PiuViewChangeCursor(PiuView* self, int32_t shape)
{
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
	
	UIPiuWindow* uiWindow = [[UIPiuWindow alloc] init];
	UIPiuViewController* uiViewController = [[UIPiuViewController alloc] init];
	UIPiuView* uiView = [[[UIPiuView alloc] init] autorelease];
	uiViewController.view = uiView;
  [uiWindow setRootViewController:uiViewController];

	NSProcessInfo* processInfo = [NSProcessInfo processInfo];
	NSTimeInterval time = [processInfo systemUptime];
	NSDate* date = [NSDate dateWithTimeIntervalSinceNow:-time];
	uiView.time = [date timeIntervalSince1970];
	uiView.timer = NULL;

	(*self)->uiView = uiView;
    
  uiView.piuView = self;
    
	(*self)->uiViewController = uiViewController;
	(*self)->uiWindow = uiWindow;
	
	(*self)->colorSpace = CGColorSpaceCreateDeviceRGB();
	
  [uiWindow makeKeyAndVisible];
  xsResult = xsThis;
}

void PiuViewDelete(void* it)
{
}

void PiuViewDictionary(xsMachine* the, void* it)
{
}

void PiuViewDrawString(PiuView* self, xsSlot* slot, PiuCoordinate offset, PiuDimension length, PiuFont* font, PiuCoordinate x, PiuCoordinate y, PiuDimension w, PiuDimension sw)
{
	PiuViewDrawStringSubPixel(self, slot, offset, length, font, x, y, w, sw);
}

void PiuViewDrawStringSubPixel(PiuView* self, xsSlot* slot, PiuCoordinate offset, PiuDimension length, PiuFont* font, double x, double y, PiuDimension w, PiuDimension sw)
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
	CGContextRef context = (*self)->context;
	CGFloat scale = (*texture)->scale;
    PiuDimension tw = (*texture)->width;
    PiuDimension th = (*texture)->height;
	CGRect source = CGRectMake(scale * sx, scale * (th - sy - sh), scale * sw, scale * sh);
	CGRect destination = CGRectMake(x, y, sw, sh);
	CGContextSaveGState(context);
	CGContextTranslateCTM(context, 0.0, sh);
	CGContextScaleCTM(context, 1.0, -1.0);
	if ((source.origin.x != 0) || (source.origin.y != 0) || (source.size.width != tw) || (source.size.height != th)) {
		CGImageRef image = CGImageCreateWithImageInRect((*texture)->image, source);
		CGContextDrawImage(context, destination, image); 
	}
	else
		CGContextDrawImage(context, destination, (*texture)->image); 
	CGContextRestoreGState(context);
}

void PiuViewFillColor(PiuView* self, PiuCoordinate x, PiuCoordinate y, PiuDimension w, PiuDimension h)
{
	if ((w <= 0) || (h <= 0)) return;
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
	UIPiuView* uiView = (*self)->uiView;
	
	CGRect rect = uiView.bounds;
	*width = rect.size.width;
	*height = rect.size.height;
}

void PiuViewIdleCheck(PiuView* self, PiuBoolean idle)
{
	UIPiuView* uiView = (*self)->uiView;

	if (idle && !uiView.timer) {
		uiView.timer = [NSTimer scheduledTimerWithTimeInterval:0.001 target:uiView selector:@selector(timerCallback:) userInfo:nil repeats:YES];
	}
	else if (!idle && uiView.timer) {
		[uiView.timer invalidate];
		uiView.timer = NULL;
	}
}

void PiuViewInvalidate(PiuView* self, PiuRectangle area) 
{
	UIPiuView* uiView = (*self)->uiView;
	CGRect rect;
	if (area) {
		//fprintf(stderr, "invalidate %d %d %d %d\n", area->x, area->y, area->width, area->height);
		rect.origin.x = area->x;
		rect.origin.y = area->y;
		rect.size.width = area->width;
		rect.size.height = area->height;
	}
	else
		rect = uiView.bounds;
	[uiView setNeedsDisplayInRect:rect];
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
}

void PiuViewPushOrigin(PiuView* self, PiuCoordinate x, PiuCoordinate y)
{
	CGContextRef context = (*self)->context;
  CGContextSaveGState(context);
	CGContextTranslateCTM(context, x, y);
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
}

void PiuSystem_setClipboardString(xsMachine* the)
{
}

void PiuSystem_launchPath(xsMachine* the)
{
}

void PiuSystem_launchURL(xsMachine* the)
{
}
