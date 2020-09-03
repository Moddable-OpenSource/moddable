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

static void PiuLabelBind(void* it, PiuApplication* application, PiuView* view);
static void PiuLabelCascade(void* it);
static void PiuLabelComputeStyle(PiuLabel* self, PiuApplication* application, PiuView* view);
static void PiuLabelDictionary(xsMachine* the, void* it);
static void PiuLabelDraw(void* it, PiuView* view, PiuRectangle area);
static void PiuLabelMark(xsMachine* the, void* it, xsMarkRoot markRoot);
static void PiuLabelMeasureHorizontally(void* it);
static void PiuLabelMeasureVertically(void* it);
static void PiuLabelUnbind(void* it, PiuApplication* application, PiuView* view);
static void PiuLabelUncomputeStyle(PiuLabel* self, PiuApplication* application, PiuView* view);

const PiuDispatchRecord ICACHE_FLASH_ATTR PiuLabelDispatchRecord = {
	"Label",
	PiuLabelBind,
	PiuLabelCascade,
	PiuLabelDraw,
	PiuContentFitHorizontally,
	PiuContentFitVertically,
	PiuContentHit,
	PiuContentIdle,
	PiuContentInvalidate,
	PiuLabelMeasureHorizontally,
	PiuLabelMeasureVertically,
	PiuContentPlace,
	NULL,
	NULL,
	PiuContentReflow,
	PiuContentShowing,
	PiuContentShown,
	PiuContentSync,
	PiuLabelUnbind,
	PiuContentUpdate
};

const xsHostHooks ICACHE_FLASH_ATTR PiuLabelHooks = {
	PiuContentDelete,
	PiuLabelMark,
	NULL
};

void PiuLabelBind(void* it, PiuApplication* application, PiuView* view)
{
	PiuLabel* self = it;
	PiuContentBind(it, application, view);
	PiuLabelComputeStyle(self, application, view);
}

void PiuLabelCascade(void* it)
{
	PiuLabel* self = it;
	PiuApplication* application = (*self)->application;
	PiuLabelUncomputeStyle(self, application, (*application)->view);
	PiuContentCascade(it);
	PiuLabelComputeStyle(self, application, (*application)->view);
	PiuContentReflow(self, piuSizeChanged);
}

void PiuLabelComputeStyle(PiuLabel* self, PiuApplication* application, PiuView* view)
{
	xsMachine* the = (*self)->the;
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
	#ifdef piuGPU
		PiuStyleBind((*self)->computedStyle, application, view);
	#endif
	}
}

void PiuLabelDictionary(xsMachine* the, void* it) 
{
	PiuLabel* self = it;
	xsBooleanValue boolean;
	if (xsFindBoolean(xsArg(1), xsID_ellipsis, &boolean)) {
		if (boolean)
			(*self)->flags |= piuLabelEllipsis;
		else
			(*self)->flags &= ~piuLabelEllipsis;
	}
	if (xsFindResult(xsArg(1), xsID_string)) {
		xsSlot* string = PiuString(xsResult);
		(*self)->string = string;
	}
	else if (xsFindResult(xsArg(1), xsID_contents)) {
		xsResult = xsCall1(xsResult, xsID_join, xsString(""));
		xsSlot* string = PiuString(xsResult);
		(*self)->string = string;
	}
}

void PiuLabelDraw(void* it, PiuView* view, PiuRectangle area) 
{
	PiuLabel* self = it;
	PiuContentDraw(it, view, area);
	if ((*self)->string && (*self)->computedStyle) {
		PiuRectangleRecord bounds;
		PiuRectangleSet(&bounds, 0, 0, (*self)->bounds.width, (*self)->bounds.height);
		//PiuViewPushClip(view, 0, 0, bounds.width, bounds.height);
		PiuStyleDraw((*self)->computedStyle, (*self)->string, view, &bounds, ((*self)->flags & piuLabelEllipsis) ? 1 : 0, (*self)->state);
		//PiuViewPopClip(view);
	}
}

void PiuLabelMark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	PiuLabel self = it;
	PiuContentMark(the, it, markRoot);
	PiuMarkString(the, self->string);
	PiuMarkHandle(the, self->computedStyle);
}

void PiuLabelMeasureHorizontally(void* it) 
{
	PiuLabel* self = it;
	PiuAlignment horizontal = (*self)->coordinates.horizontal;
	if (!(horizontal & piuWidth)) {
		if ((*self)->string && (*self)->computedStyle)
			(*self)->coordinates.width = PiuStyleGetWidth((*self)->computedStyle, (*self)->string);
		else
			(*self)->coordinates.width = 0;
	}
}

void PiuLabelMeasureVertically(void* it) 
{
	PiuLabel* self = it;
	PiuAlignment vertical = (*self)->coordinates.vertical;
	if (!(vertical & piuHeight)) {
		if ((*self)->string && (*self)->computedStyle)
			(*self)->coordinates.height = PiuStyleGetHeight((*self)->computedStyle, (*self)->string);
		else
			(*self)->coordinates.height = 0;
	}
}

void PiuLabelUnbind(void* it, PiuApplication* application, PiuView* view)
{
	PiuLabel* self = it;
	PiuLabelUncomputeStyle(self, application, view);
	PiuContentUnbind(it, application, view);
}

void PiuLabelUncomputeStyle(PiuLabel* self, PiuApplication* application, PiuView* view)
{
	if ((*self)->computedStyle) {
	#ifdef piuGPU
		PiuStyleUnbind((*self)->computedStyle, application, view);
	#endif
		(*self)->computedStyle = NULL;
	}
}

void PiuLabel_create(xsMachine* the)
{
	PiuLabel* self;
	xsVars(4);
	xsSetHostChunk(xsThis, NULL, sizeof(PiuLabelRecord));
	self = PIU(Label, xsThis);
	(*self)->the = the;
	(*self)->reference = xsToReference(xsThis);
	xsSetHostHooks(xsThis, (xsHostHooks*)&PiuLabelHooks);
	(*self)->dispatch = (PiuDispatch)&PiuLabelDispatchRecord;
	(*self)->flags = piuVisible | piuLabelEllipsis;
	PiuContentDictionary(the, self);
	PiuLabelDictionary(the, self);
	PiuBehaviorOnCreate(self);
}

void PiuLabel_get_string(xsMachine *the)
{
	PiuLabel* self = PIU(Label, xsThis);
	if ((*self)->string)
		xsResult = *((*self)->string);
}

void PiuLabel_set_string(xsMachine *the)
{
	PiuLabel* self = PIU(Label, xsThis);
	xsSlot* string = PiuString(xsArg(0));
	(*self)->string = string;
	PiuContentReflow(self, piuSizeChanged);
}





