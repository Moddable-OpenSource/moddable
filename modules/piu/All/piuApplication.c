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

#include "piuAll.h"

static void PiuApplicationInvalidate(void* it, PiuRectangle area);
static void PiuApplicationMark(xsMachine* the, void* it, xsMarkRoot markRoot);
static void PiuApplicationReflow(void* it, PiuFlags flags);
static void PiuDeferLinkMark(xsMachine* the, void* it, xsMarkRoot markRoot);
static void PiuTouchLinkAppendSample(PiuTouchLink* link, PiuCoordinate x, PiuCoordinate y, double ticks);
static void PiuTouchLinkMark(xsMachine* the, void* it, xsMarkRoot markRoot);

const PiuDispatchRecord ICACHE_FLASH_ATTR PiuApplicationDispatchRecord = {
	"Application",
	PiuContainerBind,
	PiuContainerCascade,
	PiuContentDraw,
	PiuContainerFitHorizontally,
	PiuContainerFitVertically,
	PiuContainerHit,
	PiuContentIdle,
	PiuApplicationInvalidate,
	PiuContainerMeasureHorizontally,
	PiuContainerMeasureVertically,
	PiuContainerPlace,
	PiuContainerPlaceContentHorizontally,
	PiuContainerPlaceContentVertically,
	PiuApplicationReflow,
	PiuContainerShowing,
	PiuContainerShown,
	PiuContentSync,
	PiuContainerUnbind,
	PiuContainerUpdate
};

const xsHostHooks ICACHE_FLASH_ATTR PiuApplicationHooks = {
	PiuContentDelete,
	PiuApplicationMark,
	NULL
};

const xsHostHooks ICACHE_FLASH_ATTR PiuDeferLinkHooks = {
	PiuDeferLinkDelete,
	PiuDeferLinkMark,
	NULL
};

const xsHostHooks ICACHE_FLASH_ATTR PiuTouchLinkHooks = {
	PiuTouchLinkDelete,
	PiuTouchLinkMark,
	NULL
};

void PiuApplicationAdjust(PiuApplication* self)
{
#ifdef piuPC
	if ((*self)->flags & piuMenusChanged) {
		(*self)->flags &= ~piuMenusChanged;
		xsBeginHost((*self)->the);
		xsResult = xsReference((*self)->reference);
		xsCall0(xsResult, xsID_updateMenus);
		xsEndHost((*self)->the);
	}
#endif	
	if (!((*self)->flags & piuAdjusting)) {
		(*self)->flags |= piuAdjusting;
		if ((*self)->flags & (piuContentsChanged | piuContentsPlaced)) {
			while ((*self)->flags & (piuContentsChanged | piuContentsPlaced)) {
				while ((*self)->flags & piuContentsChanged) {
					if ((*self)->flags & piuContentsHorizontallyChanged)
						PiuContainerAdjustHorizontally(self);
					if ((*self)->flags & piuContentsVerticallyChanged)
						PiuContainerAdjustVertically(self);
				}
				if ((*self)->flags & piuContentsPlaced)
					PiuContainerPlace(self);
			}
			PiuViewAdjust((*self)->view);
		}
		(*self)->flags &= ~piuAdjusting;
	}
}

void PiuApplicationCaptureTouch(PiuApplication* self, PiuContent* it, xsIntegerValue index, PiuCoordinate x,  PiuCoordinate y, double ticks)
{
	if (index < (*self)->touchLinkCount) {
		PiuTouchLink* link = (*self)->touchLinks[index];
		PiuContent* content = (*link)->content;
		if (content) {
			PiuFlags touchFlag = 1 << index;
			while (content) {
				if ((*content)->touches & touchFlag) {
					if (content != it) {
						(*content)->touches &= ~touchFlag;
						PiuBehaviorOnTouchCancelled(content, index, x, y, ticks, link);
					}
				}
				content = (PiuContent*)((*content)->container);
			}
			(*link)->content = it;
		}
	}
}

void PiuApplicationDeferContents(xsMachine* the, PiuApplication* self)
{
	PiuDeferLink* link;
	PiuContent* content;
	xsIntegerValue c, i;
	(*self)->deferLoop = (*self)->deferChain;
	(*self)->deferChain = NULL;
	while ((link = (*self)->deferLoop)) {
		(*self)->deferLoop = (*link)->deferLink;
		content = (*link)->content;
		if ((*content)->behavior) {
			xsVar(0) = xsReference((*content)->behavior);
			if (xsFindResult(xsVar(0), (*link)->id)) {
				xsVar(1) = xsReference((*content)->reference);
				c = (*link)->argc;
				xsOverflow(-(XS_FRAME_COUNT + 1 + c));
				fxPush(xsVar(0));
				fxPush(xsResult);
				fxCall(the);
				fxPush(xsVar(1));
				for (i = 0; i < c; i++) {
					xsVar(0) = xsAccess((*link)->argv[i]);
					fxPush(xsVar(0));
				}
				fxRunCount(the, 1 + c);
				the->stack++;
			}
		}
	}
}

void PiuApplicationIdleCheck(PiuApplication* self)
{
	PiuInterval idle = 0;
	xsIntegerValue count = (*self)->touchLinkCount;
	xsIntegerValue index;
	
	if ((*self)->deferChain) {
		idle = 1;
		goto bail;
	}
	for (index = 0; index < count; index++) {
		PiuTouchLink* link = (*self)->touchLinks[index];
		PiuContent* content = (*link)->content;
		if (content) {
			idle = 1;
			goto bail;
		}
	}
	if ((*self)->idleChain) {
		PiuIdleLink* link = (*self)->idleChain;
		PiuTick ticks = PiuViewTicks((*self)->view);
		while (link) {
			PiuInterval interval = (*link)->ticks + (*link)->interval - ticks;
			if (interval <= 0) {
				idle = 1;
				goto bail;
			}
			else if (idle == 0)
				idle = interval;
			else if (idle > interval)
				idle = interval;
			link = (*link)->idleLink;
		}
	}
bail:
	PiuViewIdleCheck((*self)->view, idle);
}

void PiuApplicationIdleContents(PiuApplication* self)
{
	PiuIdleLink* link = (*self)->idleChain;
	PiuTick ticks = PiuViewTicks((*self)->view);
	while (link) {
		PiuInterval interval = ticks - (*link)->ticks;
		(*self)->idleLoop = (*link)->idleLink;
		if ((*link)->interval <= interval) {
			(*link)->ticks = ticks;
			(*link)->dispatch->idle(link, interval);
		}
		link = (*self)->idleLoop;
	}
}

void PiuApplicationInvalidate(void* it, PiuRectangle area)
{
	PiuApplication* self = it;
	PiuViewInvalidate((*self)->view, area);
}

void PiuApplicationMark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	PiuApplication self = it;
	xsIntegerValue c, i;
	PiuContainerMark(the, it, markRoot);
	PiuMarkHandle(the, self->view);
	PiuMarkHandle(the, self->styleList);
	PiuMarkHandle(the, self->deferChain);
	PiuMarkHandle(the, self->deferLoop);
	c = self->touchLinkCount;
	for (i = 0; i < c; i++)
		PiuMarkHandle(the, self->touchLinks[i]);
}

void PiuApplicationReflow(void* it, PiuFlags flags)
{
	PiuApplication* self = it;
	if (flags & piuHorizontallyChanged)
		(*self)->flags |= piuContentsHorizontallyChanged;
	else if (flags & piuContentsHorizontallyChanged)
		(*self)->flags |= piuContentsHorizontallyChanged;
	if (flags & piuVerticallyChanged)
		(*self)->flags |= piuContentsVerticallyChanged;
	else if (flags & piuContentsVerticallyChanged)
		(*self)->flags |= piuContentsVerticallyChanged;
	(*self)->flags |= piuContentsPlaced;
    PiuViewReflow((*self)->view);
}

void PiuApplicationResize(PiuApplication* self)
{
	PiuView* view = (*self)->view;
	PiuDimension width, height;
	PiuViewInvalidate(view, NULL);
	PiuViewGetSize(view, &width, &height);
	(*self)->coordinates.horizontal = piuWidth;
	(*self)->coordinates.left = (*self)->coordinates.right = (*self)->bounds.x = 0;
	(*self)->coordinates.width = (*self)->bounds.width = width;
	(*self)->coordinates.vertical = piuHeight;
	(*self)->coordinates.top = (*self)->coordinates.bottom = (*self)->bounds.y = 0;
	(*self)->coordinates.height = (*self)->bounds.height = height;
	(*self)->flags |= piuAdjusting;
	PiuContainerMeasureHorizontally(self);
	PiuContainerFitHorizontally(self);
	PiuContainerMeasureVertically(self);
	PiuContainerFitVertically(self);
	(*self)->flags &= ~piuAdjusting;
    PiuViewReflow(view);
}

void PiuApplicationSetFocus(PiuApplication* self, void* it)
{
	if (!it) it = self;
	if ((*self)->focus != it) {
		if ((*self)->focus) {
			PiuBehaviorOnDefaultID((*self)->focus, xsID_onUnfocused);
		}
		(*self)->focus = it;
		if ((*self)->focus) {
			PiuBehaviorOnDefaultID((*self)->focus, xsID_onFocused);
		}
	}
}

void PiuApplicationStartContent(PiuApplication* self, void* it)
{
	PiuIdleLink* link = it;
	PiuIdleLink* current = (*self)->idleChain;
	while (current) {
		if (current == link)
			break;
		current = (*current)->idleLink;
	}
	if (!current) {
		(*link)->idleLink = (*self)->idleChain;
		(*self)->idleChain = link;
	}
	(*link)->ticks = PiuViewTicks((*self)->view);
	PiuViewReschedule((*self)->view);
}

void PiuApplicationStopContent(PiuApplication* self, void* it)
{
	PiuIdleLink* link = it;
	PiuIdleLink* former = NULL;
	PiuIdleLink* current = (*self)->idleChain;
	while (current) {
		if (current == link) {
			link = (*current)->idleLink;
			(*current)->idleLink = NULL;
			if (former)
				(*former)->idleLink = link;
			else
				(*self)->idleChain = link;
			if ((*self)->idleLoop == current)
				(*self)->idleLoop = link;
			break;
		}
		former = current;
		current = (*current)->idleLink;
	}
	PiuViewReschedule((*self)->view);
}

void PiuApplicationTouchBegan(PiuApplication* self, xsIntegerValue index, PiuCoordinate x, PiuCoordinate y, xsNumberValue ticks)
{
	if (index < (*self)->touchLinkCount) {
		PiuFlags touchFlag = 1 << index;
		PiuTouchLink* link;
		PiuContent* content;
		link = (*self)->touchLinks[index];
		(*link)->content = content = (*(*self)->dispatch->hit)(self, x, y);
		if (content) {
			if (((*content)->touches == 0) || ((*content)->flags & piuMultipleTouch))
				(*content)->touches |= touchFlag;
			content = (PiuContent*)((*content)->container);
			while (content) {
				if (((*content)->flags & piuActive) && ((*content)->flags & piuBackgroundTouch)) {
					if (((*content)->touches == 0) || ((*content)->flags & piuMultipleTouch))
						(*content)->touches |= touchFlag;
				}
				content = (PiuContent*)((*content)->container);
			}
		}
		link = (*self)->touchLinks[index];
		content = (*link)->content;
		if (content) {
			PiuTouchLinkAppendSample(link, x, y, ticks);
			while (content) {
				if ((*content)->touches & touchFlag)
					PiuBehaviorOnTouchBegan(content, index, x, y, ticks, link);
				content = (PiuContent*)((*content)->container);
			}
		}
	}
	PiuViewReschedule((*self)->view);
}

void PiuApplicationTouchEnded(PiuApplication* self, xsIntegerValue index, PiuCoordinate x, PiuCoordinate y, xsNumberValue ticks)
{
	if (index < (*self)->touchLinkCount) {
		PiuTouchLink* link = (*self)->touchLinks[index];
		PiuContent* content = (*link)->content;
		if (content) {
			PiuFlags touchFlag = 1 << index;
			PiuTouchLinkAppendSample(link, x, y, ticks);
			while (content) {
				if ((*content)->touches & touchFlag) {
					(*content)->touches &= ~touchFlag;
					PiuBehaviorOnTouchEnded(content, index, x, y, ticks, link);
				}
				content = (PiuContent*)((*content)->container);
			}
			(*link)->content = NULL;
		}
		(*link)->index = 0;
	}
	PiuViewReschedule((*self)->view);
}

void PiuApplicationTouchIdle(PiuApplication* self)
{
	xsIntegerValue count = (*self)->touchLinkCount;
	xsIntegerValue index;
	for (index = 0; index < count; index++) {
		PiuTouchLink* link = (*self)->touchLinks[index];
		if ((*link)->index) {
			PiuContent* content = (*link)->content;
			if (content) {
				PiuFlags touchFlag = 1 << index;
				PiuTouchSample sample = &((*link)->samples[(*link)->index - 1]);
				while (content) {
					if ((*content)->touches & touchFlag)
						PiuBehaviorOnTouchMoved(content, index, sample->x, sample->y, sample->ticks, link);
					content = (PiuContent*)((*content)->container);
				}
			}
			(*link)->index = 0;
		}
	}
}

void PiuApplicationTouchMoved(PiuApplication* self, xsIntegerValue index, PiuCoordinate x, PiuCoordinate y, xsNumberValue ticks)
{
	if (index < (*self)->touchLinkCount) {
		PiuTouchLink* link = (*self)->touchLinks[index];
		PiuContent* content = (*link)->content;
		if (content) {
			PiuTouchLinkAppendSample(link, x, y, ticks);
		}
	}
}

void PiuDeferLinkCreate(xsMachine* the)
{
	PiuDeferLink* self;
	xsSetHostChunk(xsThis, NULL, sizeof(PiuDeferLinkRecord));
	self = PIU(DeferLink, xsThis);
	(*self)->reference = xsToReference(xsThis);
	xsSetHostHooks(xsThis, (xsHostHooks*)&PiuDeferLinkHooks);
}

void PiuDeferLinkDelete(void* it)
{
}

void PiuDeferLinkMark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	PiuDeferLink self = it;
	xsIntegerValue c, i;
	PiuMarkHandle(the, self->content);
	c = self->argc;
	for (i = 0; i < c; i++)
		(*markRoot)(the, &self->argv[i]);
	PiuMarkHandle(the, self->deferLink);
}

void PiuTouchLinkAppendSample(PiuTouchLink* link, PiuCoordinate x, PiuCoordinate y, double ticks)
{
	PiuTouchSample sample;
	if ((*link)->index == piuTouchSampleCount) {
		c_memmove(&((*link)->samples[0]), &((*link)->samples[1]), (piuTouchSampleCount - 1) * sizeof(PiuTouchSampleRecord));
		(*link)->index--;
	}
	sample = &((*link)->samples[(*link)->index]);
	sample->x = x;
	sample->y = y;
	sample->ticks = ticks;
	(*link)->index++;
}

void PiuTouchLinkCreate(xsMachine* the)
{
	PiuTouchLink* self;
	xsSetHostChunk(xsThis, NULL, sizeof(PiuTouchLinkRecord));
	self = PIU(TouchLink, xsThis);
	(*self)->reference = xsToReference(xsThis);
	xsSetHostHooks(xsThis, (xsHostHooks*)&PiuTouchLinkHooks);
}

void PiuTouchLinkDelete(void* it)
{
}

void PiuTouchLinkMark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	PiuTouchLink self = it;
	PiuMarkHandle(the, self->content);
}

void PiuTouchLink_get_length(xsMachine* the)
{
	PiuTouchLink* link = PIU(TouchLink, xsThis);
	xsResult = xsInteger((*link)->index);
}

void PiuTouchLink_peek(xsMachine* the)
{
	PiuTouchLink* link = PIU(TouchLink, xsThis);
	uint32_t index = (uint32_t)xsToInteger(xsArg(0));
	if (index < (*link)->index) {
		PiuTouchSampleRecord sample = (*link)->samples[index];
		xsResult = xsNewObject();
		xsDefine(xsResult, xsID_x, xsPiuCoordinate(sample.x), xsDefault);
		xsDefine(xsResult, xsID_y, xsPiuCoordinate(sample.y), xsDefault);
		xsDefine(xsResult, xsID_ticks, xsNumber(sample.ticks), xsDefault);
	}
}

void PiuApplication_create(xsMachine* the)
{
	PiuApplication* self;
	xsIntegerValue c, i;
	xsVars(5);
// 	xsLog("application free %d\n", system_get_free_heap_size());
	if (!xsFindInteger(xsArg(1), xsID_touchCount, &c))
		c = 1;
	xsSetHostChunk(xsThis, NULL, sizeof(PiuApplicationRecord) + ((c - 1) * sizeof(PiuTouchLink*)));
	self = PIU(Application, xsThis);
	xsSetContext(the, self);
	(*self)->the = the;
	(*self)->reference = xsToReference(xsThis);
	xsSetHostHooks(xsThis, (xsHostHooks*)&PiuApplicationHooks);
	(*self)->dispatch = (PiuDispatch)&PiuApplicationDispatchRecord;
	(*self)->flags = piuVisible | piuContainer | piuDisplaying;
	if (!xsFindResult(xsArg(1), xsID__DeferLink))
		xsErrorPrintf("no DeferLink");
	(*self)->DeferLink = xsResult;
	if (!xsFindResult(xsArg(1), xsID__Skin))
		xsErrorPrintf("no Skin");
	(*self)->Skin = xsResult;
	if (!xsFindResult(xsArg(1), xsID__Style))
		xsErrorPrintf("no Style");
	(*self)->Style = xsResult;
	if (!xsFindResult(xsArg(1), xsID__Texture))
		xsErrorPrintf("no Texture");
	(*self)->Texture = xsResult;
	
	PiuStyleLinkNew(the);
	(*self)->styleList = PIU(StyleLink, xsResult);
	
	if (!xsFindResult(xsArg(1), xsID__TouchLink))
		xsErrorPrintf("no TouchLink");
	xsVar(0) = xsResult;
	(*self)->touchLinkCount = c;
	for (i = 0; i < c; i++) {
		xsResult = xsNewFunction0(xsVar(0));
		(*self)->touchLinks[i] = PIU(TouchLink, xsResult);
	}
	
	(*self)->application = self;
	(*self)->focus = (PiuContent*)self;
	
	if (!xsFindResult(xsArg(1), xsID__View))
		xsErrorPrintf("no View");
	xsNewFunction2(xsResult, xsThis, xsArg(1));
	
	PiuContentDictionary(the, self);
	PiuContainerDictionary(the, self);
	PiuViewDictionary(the, self);
	PiuBehaviorOnCreate(self);
	PiuApplicationResize(self);
}

#ifdef piuPC

static void PiuApplicationFindMenu(PiuApplication* self, xsIntegerValue id);

xsIntegerValue PiuApplicationCanMenu(PiuApplication* self, xsIntegerValue id)
{
	xsMachine* the = (*self)->the;
	xsIntegerValue result = 0;
	PiuApplicationFindMenu(self, id);
	if (xsTest(xsVar(2))) {
		xsIdentifier canID = (xsIdentifier)xsToInteger(xsGet(xsVar(2), xsID_canID));
		PiuContent* content = (*self)->focus;
		while (content) {
			if ((*content)->behavior) {
				xsVar(0) = xsReference((*content)->behavior);
				if (xsFindResult(xsVar(0), canID)) {
					xsVar(1) = xsReference((*content)->reference);
					xsVar(3) = xsCallFunction2(xsResult, xsVar(0), xsVar(1), xsVar(2));
					if (xsTest(xsVar(3)))
						result |= piuMenuEnabled;
					break;
				}
			}
			content = (PiuContent*)(*content)->container;
		}
		xsVar(3) = xsGet(xsVar(2), xsID_check);
		if (xsTest(xsVar(3)))
			result |= piuMenuChecked;
		xsVar(3) = xsGet(xsVar(2), xsID_titles);
		if (xsTest(xsVar(3))) {
			xsVar(0) = xsGet(xsVar(2), xsID_state);
			xsVar(1) = xsGetAt(xsVar(3), xsVar(0));
			xsSet(xsVar(2), xsID_title, xsVar(1));
			result |= piuMenuTitled;
		}
		xsResult = xsVar(2);
	}
	return result;
}

void PiuApplicationDoMenu(PiuApplication* self, xsIntegerValue id)
{
	xsMachine* the = (*self)->the;
	PiuApplicationFindMenu(self, id);
	if (xsTest(xsVar(2))) {
		xsIdentifier doID = (xsIdentifier)xsToInteger(xsGet(xsVar(2), xsID_doID));
		PiuContent* content = (*self)->focus;
		while (content) {
			if ((*content)->behavior) {
				xsVar(0) = xsReference((*content)->behavior);
				if (xsFindResult(xsVar(0), doID)) {
					xsIntegerValue value;
					xsVar(1) = xsReference((*content)->reference);
					if (xsFindInteger(xsVar(2), xsID_value, &value))
						xsVar(2) = xsInteger(value);
					else
						xsVar(2) = xsUndefined;
					(void)xsCallFunction2(xsResult, xsVar(0), xsVar(1), xsVar(2));
					PiuApplicationAdjust(self);
					break;
				}
			}
			content = (PiuContent*)(*content)->container;
		}
	}
}

void PiuApplicationFindMenu(PiuApplication* self, xsIntegerValue id)
{
	xsMachine* the = (*self)->the;
	xsIntegerValue c, i, d, j;
	xsVar(2) = xsReference((*self)->reference);
	xsVar(2) = xsGet(xsVar(2), xsID_menus);
	c = xsToInteger(xsGet(xsVar(2), xsID_length));
	i = (id & 0x0000FF00) >> 8;
	if ((0 <= i) && (i < c)) {
		xsVar(2) = xsGetAt(xsVar(2), xsInteger(i));
		xsVar(2) = xsGet(xsVar(2), xsID_items);
		d = xsToInteger(xsGet(xsVar(2), xsID_length));
		j = (id & 0x000000FF) - 1;
		if ((0 <= j) && (j < d)) {
			xsVar(2) = xsGetAt(xsVar(2), xsInteger(j));
		}
	}
}

void PiuApplicationKeyDown(PiuApplication* self, xsStringValue string)
{
	xsMachine* the = (*self)->the;
	PiuContent* content = (*self)->focus;
	xsVar(2) = xsString(string);
	while (content) {
		if ((*content)->behavior) {
			xsVar(0) = xsReference((*content)->behavior);
			if (xsFindResult(xsVar(0), xsID_onKeyDown)) {
				xsVar(1) = xsReference((*content)->reference);
				xsVar(3) = xsCallFunction2(xsResult, xsVar(0), xsVar(1), xsVar(2));
				if (xsTest(xsVar(3)))
					break;
			}
		}
		content = (PiuContent*)(*content)->container;
	}
}

void PiuApplicationKeyUp(PiuApplication* self, xsStringValue string)
{
	xsMachine* the = (*self)->the;
	PiuContent* content = (*self)->focus;
	xsVar(2) = xsString(string);
	while (content) {
		if ((*content)->behavior) {
			xsVar(0) = xsReference((*content)->behavior);
			if (xsFindResult(xsVar(0), xsID_onKeyUp)) {
				xsVar(1) = xsReference((*content)->reference);
				xsVar(3) = xsCallFunction2(xsResult, xsVar(0), xsVar(1), xsVar(2));
				if (xsTest(xsVar(3)))
					break;
			}
		}
		content = (PiuContent*)(*content)->container;
	}
}

void PiuApplicationModifiersChanged(PiuApplication* self, PiuBoolean controlKey, PiuBoolean optionKey, PiuBoolean shiftKey)
{
	xsMachine* the = (*self)->the;
	xsSet(xsGlobal, xsID_controlKey, xsBoolean(controlKey));
	xsSet(xsGlobal, xsID_optionKey, xsBoolean(optionKey));
	xsSet(xsGlobal, xsID_shiftKey, xsBoolean(shiftKey));
}

void PiuApplicationMouseEntered(PiuApplication* self, PiuCoordinate x, PiuCoordinate y)
{
	PiuContent* current = (*(*self)->dispatch->hit)(self, x, y);
	(*self)->hover = current;
	while (current) {
		if (PiuBehaviorOnMouseID(current, xsID_onMouseEntered, x, y))
			break;
		current = (PiuContent*)((*current)->container);
	}
	(*self)->mouse.x = x;
	(*self)->mouse.y = y;
}

void PiuApplicationMouseExited(PiuApplication* self, PiuCoordinate x, PiuCoordinate y)
{
	PiuContent* former = (*self)->hover;
	while (former) {
		if (PiuBehaviorOnMouseID(former, xsID_onMouseExited, x, y))
			break;
		former = (PiuContent*)((*former)->container);
	}
	(*self)->hover = NULL;
	(*self)->mouse.x = x;
	(*self)->mouse.y = y;
}

void PiuApplicationMouseMoved(PiuApplication* self, PiuCoordinate x, PiuCoordinate y)
{
	PiuContent* former = (*self)->hover;
	PiuContent* current = (*(*self)->dispatch->hit)(self, x, y);
	if (former != current) {
		(*self)->cursorShape = 0;
		while (former) {
			if (PiuBehaviorOnMouseID(former, xsID_onMouseExited, x, y))
				break;
			former = (PiuContent*)((*former)->container);
		}
		(*self)->hover = current;
		while (current) {
			if (PiuBehaviorOnMouseID(current, xsID_onMouseEntered, x, y))
				break;
			current = (PiuContent*)((*current)->container);
		}
		PiuViewChangeCursor((*self)->view, (*self)->cursorShape);
	}
	else {
		while (current) {
			if (PiuBehaviorOnMouseID(former, xsID_onMouseMoved, x, y))
				break;
			current = (PiuContent*)((*current)->container);
		}
		PiuViewChangeCursor((*self)->view, (*self)->cursorShape);
	}
	(*self)->mouse.x = x;
	(*self)->mouse.y = y;
}

void PiuApplicationMouseScrolled(PiuApplication* self, PiuCoordinate dx, PiuCoordinate dy)
{
	PiuContent* current = (*self)->hover;
	while (current) {
		if (PiuBehaviorOnMouseID(current, xsID_onMouseScrolled, dx, dy))
			break;
		current = (PiuContent*)((*current)->container);
	}
}

void PiuApplication_get_cursor(xsMachine* the)
{
	PiuApplication* self = PIU(Application, xsThis);
	xsResult = xsInteger((*self)->cursorShape);
}

void PiuApplication_set_cursor(xsMachine* the)
{
	PiuApplication* self = PIU(Application, xsThis);
	(*self)->cursorShape = xsToInteger(xsArg(0));
}

void PiuApplication_invalidateMenus(xsMachine* the)
{
	PiuApplication* self = PIU(Application, xsThis);
	(*self)->flags |= piuMenusChanged;
}

#endif

const void *fxGetResource(xsMachine* the, void* archive, const char* path, size_t* size)
{
	const void* data;
	if (archive)
		data = fxGetArchiveData(the, archive, (xsStringValue)path, size);
	else {
		data = fxGetArchiveData(the, the->archive, (xsStringValue)path, size);
		if (!data)
			data = mcGetResource(the, path, size);
	}
	return data;
}

xsBooleanValue fxFindBoolean(xsMachine* the, xsSlot* slot, xsIdentifier id, xsBooleanValue* value)
{
	xsBooleanValue result;
	xsOverflow(-1);
	fxPush(*slot);
	fxGetID(the, id);
	if (fxTypeOf(the, the->stack) != xsUndefinedType) {
		*value = fxToBoolean(the, the->stack);
		result = 1;
	}
	else
		result = 0;
	the->stack++;
	return result;
}

xsBooleanValue fxFindInteger(xsMachine* the, xsSlot* slot, xsIdentifier id, xsIntegerValue* value)
{
	xsBooleanValue result;
	xsOverflow(-1);
	fxPush(*slot);
	fxGetID(the, id);
	if (fxTypeOf(the, the->stack) != xsUndefinedType) {
		*value = fxToInteger(the, the->stack);
		result = 1;
	}
	else
		result = 0;
	the->stack++;
	return result;
}

xsBooleanValue fxFindNumber(xsMachine* the, xsSlot* slot, xsIdentifier id, xsNumberValue* value)
{
	xsBooleanValue result;
	xsOverflow(-1);
	fxPush(*slot);
	fxGetID(the, id);
	if (fxTypeOf(the, the->stack) != xsUndefinedType) {
		*value = fxToNumber(the, the->stack);
		result = 1;
	}
	else
		result = 0;
	the->stack++;
	return result;
}

xsBooleanValue fxFindString(xsMachine* the, xsSlot* slot, xsIdentifier id, xsStringValue* value)
{
	xsBooleanValue result;
	xsOverflow(-1);
	fxPush(*slot);
	fxGetID(the, id);
	if (fxTypeOf(the, the->stack) != xsUndefinedType) {
		*value = fxToString(the, the->stack);
		result = 1;
	}
	else
		result = 0;
	the->stack++;
	return result;
}

xsBooleanValue fxFindResult(xsMachine* the, xsSlot* slot, xsIdentifier id)
{
	xsBooleanValue result;
	xsOverflow(-1);
	fxPush(*slot);
	if (fxHasID(the, id)) {
		fxPush(*slot);
		fxGetID(the, id);
		xsResult = *the->stack;
		the->stack++;
		result = 1;
	}
	else
		result = 0;
	return result;
}

xsSlot* fxDuplicateString(xsMachine* the, xsSlot* slot)
{
	xsSlot* result;
	xsOverflow(-1);
	fxPush(*slot);
	fxToString(the, the->stack);
	result = fxDuplicateSlot(the, the->stack);
	the->stack++;
	return result;
}



