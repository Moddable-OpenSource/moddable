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

typedef struct PiuFieldStruct PiuFieldRecord, *PiuField;

@interface NSPiuTextField : NSTextField <NSTextFieldDelegate> {
	PiuField* piuField;
}
@property (assign) PiuField* piuField;
@end

struct PiuFieldStruct {
	PiuHandlePart;
	PiuIdlePart;
	PiuBehaviorPart;
	PiuContentPart;
	PiuStyle* computedStyle;
    NSPiuClipView *nsClipView;
    NSPiuTextField *nsTextField;
};

@implementation NSPiuTextField
@synthesize piuField;
- (BOOL)becomeFirstResponder {
    PiuApplication* application =  (*piuField)->application;
    PiuApplicationSetFocus(application, piuField);
    BOOL success = [super becomeFirstResponder];
    if (success) {
        NSTextView* textField = (NSTextView*) [self currentEditor];
        if ([textField respondsToSelector: @selector(setInsertionPointColor:)])
            [textField setInsertionPointColor: [self textColor]];
    }
    return success;
}
- (void)controlTextDidBeginEditing:(NSNotification *)notification {
//    NSLog(@"controlTextDidBeginEditing: stringValue == %@", [self stringValue]);
}
- (void)controlTextDidEndEditing:(NSNotification *)notification {
//   NSLog(@"controlTextDidEndEditing: stringValue == %@", [self stringValue]);
	if ((*piuField)->behavior) {
		xsBeginHost((*piuField)->the);
		xsVars(2);
		xsVar(0) = xsReference((*piuField)->behavior);
		if (xsFindResult(xsVar(0), xsID_onEnter)) {
			xsVar(1) = xsReference((*piuField)->reference);
			(void)xsCallFunction1(xsResult, xsVar(0), xsVar(1));
		}
		xsEndHost((*piuField)->the);
	}
}
- (void)controlTextDidChange:(NSNotification *)notification {
 //   NSLog(@"controlTextDidChange: stringValue == %@", [self stringValue]);
	if ((*piuField)->behavior) {
		xsBeginHost((*piuField)->the);
		xsVars(2);
		xsVar(0) = xsReference((*piuField)->behavior);
		if (xsFindResult(xsVar(0), xsID_onStringChanged)) {
			xsVar(1) = xsReference((*piuField)->reference);
			(void)xsCallFunction1(xsResult, xsVar(0), xsVar(1));
		}
		xsEndHost((*piuField)->the);
	}
}
- (void)setFrame:(NSRect)frame {
	PiuStyle* style = (*piuField)->computedStyle;
	frame.origin.x += (*style)->margins.left;
	frame.size.width -= (*style)->margins.left + (*style)->margins.right;
	frame.origin.y += (*style)->margins.top;
	
	CTFontRef fref = (CTFontRef)(self.font);
	CGFloat ascent = CTFontGetAscent(fref);
	CGFloat descent = CTFontGetDescent(fref);
	CGFloat height = ascent + descent;
	switch ((*style)->vertical) {
	case piuTop:
		break;
	case piuBottom:
		frame.origin.y += frame.size.height - height;
		break;
	default:
		frame.origin.y += (frame.size.height - height) / 2;
		break;
	}
	frame.size.height = height;
	[super setFrame:frame];
}
@end

static void PiuFieldBind(void* it, PiuApplication* application, PiuView* view);
static void PiuFieldCascade(void* it);
static void PiuFieldComputeStyle(PiuField* self);
static void PiuFieldDictionary(xsMachine* the, void* it);
static void PiuFieldMark(xsMachine* the, void* it, xsMarkRoot markRoot);
static void PiuFieldUnbind(void* it, PiuApplication* application, PiuView* view);

const PiuDispatchRecord ICACHE_FLASH_ATTR PiuFieldDispatchRecord = {
	"Field",
	PiuFieldBind,
	PiuFieldCascade,
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
	PiuFieldUnbind,
	PiuContentUpdate
};

const xsHostHooks ICACHE_FLASH_ATTR PiuFieldHooks = {
	PiuContentDelete,
	PiuFieldMark,
	NULL
};

void PiuFieldBind(void* it, PiuApplication* application, PiuView* view)
{
	PiuField* self = it;
	PiuContentBind(it, application, view);
	PiuFieldComputeStyle(self);
	PiuSkin* skin = (*self)->skin;
	PiuStyle* style = (*self)->computedStyle;
 	CGFloat r, g, b, a;
	NSPiuTextField *textField = (*self)->nsTextField;
	r = (float)(*skin)->data.color.fill[0].r / 255;
	g = (float)(*skin)->data.color.fill[0].g / 255;
	b = (float)(*skin)->data.color.fill[0].b / 255;
	a = (float)(*skin)->data.color.fill[0].a / 255;
    [textField setBackgroundColor:[NSColor colorWithSRGBRed:r green:g blue:b alpha:a]];
	r = (float)(*style)->color[0].r / 255;
	g = (float)(*style)->color[0].g / 255;
	b = (float)(*style)->color[0].b / 255;
	a = (float)(*style)->color[0].a / 255;
	[textField setTextColor:[NSColor colorWithSRGBRed:r green:g blue:b alpha:a]];	
    [(*view)->nsView addSubview:(*self)->nsClipView];
}

void PiuFieldCascade(void* it)
{
	PiuField* self = it;
	PiuContentCascade(it);
	PiuFieldComputeStyle(self);
	PiuContentReflow(self, piuSizeChanged);
}

void PiuFieldComputeStyle(PiuField* self)
{
	xsMachine* the = (*self)->the;
	PiuApplication* application = (*self)->application;
	PiuContainer* container = (PiuContainer*)self;
	PiuStyleLink* list = (*application)->styleList;
	PiuStyleLink* chain = NULL;
	while (container) {
		PiuStyle* style = (*container)->style;
		if (style) {
			list = PiuStyleLinkMatch(the, list, chain, style);
			chain = list;
		}
		container = (*container)->container;
	}
	if (chain) {
		PiuStyle* result = PiuStyleLinkCompute(the, chain, application);
		(*self)->computedStyle = result;
	}
}

void PiuFieldDictionary(xsMachine* the, void* it) 
{
	PiuField* self = it;
	xsStringValue string;
	if (xsFindString(xsArg(1), xsID_string, &string)) 
		(*self)->nsTextField.stringValue = [NSString stringWithUTF8String:string];
	if (xsFindString(xsArg(1), xsID_placeholder, &string)) {
		(*self)->nsTextField.placeholderAttributedString =
			[[NSAttributedString alloc] initWithString:[NSString stringWithUTF8String:string] attributes:@{ 
				NSForegroundColorAttributeName: [NSColor grayColor],
           		NSFontAttributeName: (*self)->nsTextField.font
			}];	
	}
}

void PiuFieldMark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	PiuField self = it;
	PiuContentMark(the, it, markRoot);
	PiuMarkHandle(the, self->computedStyle);
}

void PiuFieldUnbind(void* it, PiuApplication* application, PiuView* view)
{
	PiuField* self = it;
    [(*self)->nsClipView removeFromSuperview];
	(*self)->computedStyle = NULL;
	PiuContentUnbind(it, application, view);
}

void PiuField_create(xsMachine* the)
{
	PiuField* self;
	xsVars(4);
	xsSetHostChunk(xsThis, NULL, sizeof(PiuFieldRecord));
	self = PIU(Field, xsThis);
	(*self)->the = the;
	(*self)->reference = xsToReference(xsThis);
	xsSetHostHooks(xsThis, (xsHostHooks*)&PiuFieldHooks);
	(*self)->dispatch = (PiuDispatch)&PiuFieldDispatchRecord;
	(*self)->flags = piuVisible;
	PiuContentDictionary(the, self);
	
    NSPiuTextField *textField = [[NSPiuTextField alloc] init];
	[textField setBezeled:NO];
    [textField setDelegate:(id)textField];
    [textField setDrawsBackground:YES];
	[textField setFocusRingType:NSFocusRingTypeNone];
	[textField setMaximumNumberOfLines:1];
	textField.piuField = self;

	NSPiuClipView *clipView = [[NSPiuClipView alloc] init];
    [clipView setDocumentView:textField];
    [clipView setDrawsBackground:NO];
	clipView.piuContent = (PiuContent*)self;
	
	(*self)->nsClipView = clipView;
	(*self)->nsTextField = textField;
	
	PiuFieldDictionary(the, self);
	PiuBehaviorOnCreate(self);
}

void PiuField_get_placeholder(xsMachine *the)
{
	PiuField* self = PIU(Field, xsThis);
	NSString* string = (*self)->nsTextField.placeholderString;
	xsResult = xsString([string UTF8String]);
}

void PiuField_get_string(xsMachine *the)
{
	PiuField* self = PIU(Field, xsThis);
	NSString* string = (*self)->nsTextField.stringValue;
	xsResult = xsString([string UTF8String]);
}

void PiuField_set_placeholder(xsMachine *the)
{
	PiuField* self = PIU(Field, xsThis);
	xsStringValue string = xsToString(xsArg(0));
	(*self)->nsTextField.placeholderAttributedString = 
		[[NSAttributedString alloc] initWithString:[NSString stringWithUTF8String:string] attributes:@{ 
			NSForegroundColorAttributeName: [NSColor grayColor],
			NSFontAttributeName: (*self)->nsTextField.font
		}];	
}

void PiuField_set_string(xsMachine *the)
{
	PiuField* self = PIU(Field, xsThis);
	xsStringValue string = xsToString(xsArg(0));
	(*self)->nsTextField.stringValue = [NSString stringWithUTF8String:string];
}

void PiuField_focus(xsMachine *the)
{
	PiuField* self = PIU(Field, xsThis);
	if ((*self)->application) {
		NSPiuTextField *textField = (*self)->nsTextField;
		[[textField window] makeFirstResponder:textField];
	}
}

