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

@interface UIPiuTextField : UITextField <UITextFieldDelegate> {
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
    UIPiuClipView *uiClipView;
    UIPiuTextField *uiTextField;
};

@implementation UIPiuTextField
@synthesize piuField;
- (BOOL)becomeFirstResponder {
    PiuApplication* application =  (*piuField)->application;
    PiuApplicationSetFocus(application, piuField);
    return [super becomeFirstResponder];
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
//    NSLog(@"controlTextDidChange: stringValue == %@", [self stringValue]);
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
- (void)setFrame:(CGRect)frame {
    if (piuField) {
	PiuStyle* style = (*piuField)->computedStyle;
	frame.origin.x += (*style)->margins.left;
	frame.size.width -= (*style)->margins.left + (*style)->margins.right;
	frame.origin.y += (*style)->margins.top;
	frame.size.height -= (*style)->margins.top + (*style)->margins.bottom;
	switch ((*style)->vertical) {
	case piuTop:
		break;
	case piuBottom:
		frame.origin.y += frame.size.height - PiuFontGetHeight((*style)->font);
		break;
	default:
		frame.origin.y += (frame.size.height - PiuFontGetHeight((*style)->font)) / 2;
		break;
	}
	[super setFrame:frame];
    }
}
@end

static void PiuFieldBind(void* it, PiuApplication* application);
static void PiuFieldCascade(void* it);
static void PiuFieldComputeStyle(PiuField* self);
static void PiuFieldDictionary(xsMachine* the, void* it);
static void PiuFieldMark(xsMachine* the, void* it, xsMarkRoot markRoot);
static void PiuFieldUnbind(void* it, PiuApplication* application);

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

void PiuFieldBind(void* it, PiuApplication* application)
{
	PiuField* self = it;
	PiuContentBind(it, application);
	PiuFieldComputeStyle(self);
	PiuView* view = (*application)->view;
    [(*view)->uiView addSubview:(*self)->uiClipView];
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
		PiuFont* font = (*result)->font;
		[(*self)->uiTextField setFont:(UIFont*)(*font)->fref];
		PiuColorRecord color =  (*result)->color[0];
		[(*self)->uiTextField setTextColor: [UIColor colorWithRed: color.r/255 green: color.g/255 blue: color.b/255 alpha: color.a/255]];
		
		
	}
}

void PiuFieldDictionary(xsMachine* the, void* it) 
{
	PiuField* self = it;
	xsStringValue string;
	if (xsFindString(xsArg(1), xsID_string, &string)) 
		(*self)->uiTextField.text = [NSString stringWithUTF8String:string];
	if (xsFindString(xsArg(1), xsID_placeholder, &string)) 
		(*self)->uiTextField.placeholder = [NSString stringWithUTF8String:string];
}

void PiuFieldMark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	PiuField self = it;
	PiuContentMark(the, it, markRoot);
	PiuMarkHandle(the, self->computedStyle);
}

void PiuFieldUnbind(void* it, PiuApplication* application)
{
	PiuField* self = it;
    [(*self)->uiClipView removeFromSuperview];
	(*self)->computedStyle = NULL;
	PiuContentUnbind(it, application);
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
	
  UIPiuTextField *textField = [[UIPiuTextField alloc] init];
  [textField setDelegate:(id)textField];
	textField.piuField = self;

	UIPiuClipView *clipView = [[UIPiuClipView alloc] init];
  [clipView addSubview:textField];
  clipView.clipsToBounds = YES;
	clipView.piuContent = (PiuContent*)self;
	
	(*self)->uiClipView = clipView;
	(*self)->uiTextField = textField;
	
	PiuFieldDictionary(the, self);
	PiuBehaviorOnCreate(self);
}

void PiuField_get_placeholder(xsMachine *the)
{
	PiuField* self = PIU(Field, xsThis);
	NSString* string = (*self)->uiTextField.placeholder;
	xsResult = xsString([string UTF8String]);
}

void PiuField_get_string(xsMachine *the)
{
	PiuField* self = PIU(Field, xsThis);
	NSString* string = (*self)->uiTextField.text;
	xsResult = xsString([string UTF8String]);
}

void PiuField_set_placeholder(xsMachine *the)
{
	PiuField* self = PIU(Field, xsThis);
	xsStringValue string = xsToString(xsArg(0));
	(*self)->uiTextField.placeholder = [NSString stringWithUTF8String:string];
}

void PiuField_set_string(xsMachine *the)
{
	PiuField* self = PIU(Field, xsThis);
	xsStringValue string = xsToString(xsArg(0));
	(*self)->uiTextField.text = [NSString stringWithUTF8String:string];
}

void PiuField_focus(xsMachine *the)
{
	PiuField* self = PIU(Field, xsThis);
	if ((*self)->application) {
		UIPiuTextField *textField = (*self)->uiTextField;
// 		[[textField window] makeFirstResponder:textField];
		[textField becomeFirstResponder];
	}
}

