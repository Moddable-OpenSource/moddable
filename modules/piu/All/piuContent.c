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

static void PiuContentMoveBy(PiuContent* self, PiuCoordinate dx, PiuCoordinate dy);
static void PiuContentSetCoordinates(void* it, PiuCoordinates coordinates);
static void PiuContentSetDuration(PiuContent* self, double duration);
static void PiuContentSetFraction(PiuContent* self, double fraction);
static void PiuContentSetInterval(PiuContent* self, PiuInterval interval);
static void PiuContentStart(PiuContent* self);
static void PiuContentStop(PiuContent* self);

static PiuBoolean PiuContent_distributeAux(xsMachine *the, PiuContainer* container, xsIdentifier id, xsIntegerValue c);

const PiuDispatchRecord ICACHE_FLASH_ATTR PiuContentDispatchRecord = {
	"Content",
	PiuContentBind,
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
	PiuContentUnbind,
	PiuContentUpdate
};

const xsHostHooks ICACHE_FLASH_ATTR PiuContentHooks = {
	PiuContentDelete,
	PiuContentMark,
	NULL
};

void PiuContentBind(void* it, PiuApplication* application, PiuView* view)
{
	PiuContent* self = it;
#ifdef piuGPU
	PiuSkin* skin = (*self)->skin;
	if (skin)
		PiuSkinBind(skin, application, view);
#endif
	(*self)->application = application;
	if ((*self)->flags & piuIdling)
		PiuApplicationStartContent((*self)->application, self);
	(*self)->flags |= piuDisplaying;
}

void PiuContentCascade(void* it) 
{
}

void PiuContentDelete(void* it)
{
}

void PiuContentDictionary(xsMachine* the, void* it) 
{
	PiuContent* self = it;
	xsBooleanValue boolean;
	xsIntegerValue integer;
	xsNumberValue number;

	if (xsFindBoolean(xsArg(1), xsID_active, &boolean)) {
		if (boolean)
			(*self)->flags |= piuActive;
		else
			(*self)->flags &= ~piuActive;
	}
	if (xsFindBoolean(xsArg(1), xsID_backgroundTouch, &boolean)) {
		if (boolean)
			(*self)->flags |= piuBackgroundTouch;
		else
			(*self)->flags &= ~piuBackgroundTouch;
	}
	if (xsFindBoolean(xsArg(1), xsID_exclusiveTouch, &boolean)) {
		if (boolean)
			(*self)->flags |= piuExclusiveTouch;
		else
			(*self)->flags &= ~piuExclusiveTouch;
	}
	if (xsFindBoolean(xsArg(1), xsID_multipleTouch, &boolean)) {
		if (boolean)
			(*self)->flags |= piuMultipleTouch;
		else
			(*self)->flags &= ~piuMultipleTouch;
	}
	if (xsFindBoolean(xsArg(1), xsID_visible, &boolean)) {
		if (boolean)
			(*self)->flags |= piuVisible;
		else
			(*self)->flags &= ~piuVisible;
	}

	if (xsFindResult(xsArg(1), xsID_anchor)) {
		if (xsTest(xsArg(0))) {
			xsSetAt(xsArg(0), xsResult, xsThis);
		}
	}
	if (xsFindResult(xsArg(1), xsID_Behavior)) {
		if (xsIsInstanceOf(xsResult, xsFunctionPrototype)) {
			fxPush(xsResult);
			fxNew(the);
			fxRunCount(the, 0);
			xsResult = fxPop();
			(*self)->behavior = xsToReference(xsResult);
		}
	}
	else if (xsFindResult(xsArg(1), xsID_behavior)) {
		if (xsIsInstanceOf(xsResult, xsObjectPrototype)) {
			(*self)->behavior = xsToReference(xsResult);
		}
	}
	if (xsFindResult(xsArg(1), xsID_name)) {
		xsSlot* name = PiuString(xsResult);
		(*self)->name = name;
	}
	
	if (xsFindInteger(xsArg(1), xsID_left, &integer)) {
		(*self)->coordinates.horizontal |= piuLeft;
		(*self)->coordinates.left = (PiuCoordinate)integer;
	}
	if (xsFindInteger(xsArg(1), xsID_width, &integer)) {
		(*self)->coordinates.horizontal |= piuWidth;
		(*self)->coordinates.width = (PiuCoordinate)integer;
	}
	if (xsFindInteger(xsArg(1), xsID_right, &integer)) {
		(*self)->coordinates.horizontal |= piuRight;
		(*self)->coordinates.right = (PiuCoordinate)integer;
	}
	if (xsFindInteger(xsArg(1), xsID_top, &integer)) {
		(*self)->coordinates.vertical |= piuTop;
		(*self)->coordinates.top = (PiuCoordinate)integer;
	}
	if (xsFindInteger(xsArg(1), xsID_height, &integer)) {
		(*self)->coordinates.vertical |= piuHeight;
		(*self)->coordinates.height = (PiuCoordinate)integer;
	}
	if (xsFindInteger(xsArg(1), xsID_bottom, &integer)) {
		(*self)->coordinates.vertical |= piuBottom;
		(*self)->coordinates.bottom = (PiuCoordinate)integer;
	}
	
	if (xsFindNumber(xsArg(1), xsID_duration, &number))
		PiuContentSetDuration(self, number);
	if (xsFindNumber(xsArg(1), xsID_fraction, &number))
		PiuContentSetFraction(self, number);
#ifdef piuPC
	if (xsFindNumber(xsArg(1), xsID_interval, &number))
		PiuContentSetInterval(self, number);
#else
	if (xsFindInteger(xsArg(1), xsID_interval, &integer))
		PiuContentSetInterval(self, integer);
#endif
	else
		(*self)->interval = 1;
	if (xsFindBoolean(xsArg(1), xsID_loop, &boolean)) {
		if (boolean)
			(*self)->flags |= piuLoop;
		else
			(*self)->flags &= ~piuLoop;
	}
	if (xsFindNumber(xsArg(1), xsID_time, &number))
		PiuContentSetTime(self, number);

	if (xsFindResult(xsArg(1), xsID_Skin)) {
		if (!xsIsInstanceOf(xsResult, xsFunctionPrototype))
			xsUnknownError("Skin is no function");
		xsResult = xsCallFunction0(xsResult, xsNull);
		xsVar(0) = xsGet(xsGlobal, xsID_Skin);
		xsVar(1) = xsGet(xsVar(0), xsID_prototype);
		if (!xsIsInstanceOf(xsResult, xsVar(1)))
			xsUnknownError("Skin is no skin template");
		(*self)->skin = PIU(Skin, xsResult);
	}
	else if (xsFindResult(xsArg(1), xsID_skin)) {
		if (!xsIsInstanceOf(xsResult, xsObjectPrototype))
			xsUnknownError("skin is no object");
		xsVar(0) = xsGet(xsGlobal, xsID_Skin);
		xsVar(1) = xsGet(xsVar(0), xsID_prototype);
		if (xsIsInstanceOf(xsResult, xsVar(1)))
			(*self)->skin = PIU(Skin, xsResult);
		else {
			xsVar(2) = xsGet(xsGlobal, xsID_assetMap);
			if (xsTest(xsVar(2)))
				xsVar(3) = xsCall1(xsVar(2), xsID_get, xsResult);
			else {
				xsVar(2) = xsNew0(xsGlobal, xsID_Map);
				xsSet(xsGlobal, xsID_assetMap, xsVar(2));
				xsVar(3) = xsUndefined;
			}
			if (xsTest(xsVar(3))) {
				if (!xsIsInstanceOf(xsVar(3), xsVar(1)))
					xsUnknownError("skin is no skin dictionary");
			}
			else {
				xsVar(3) = xsNewFunction1(xsVar(0), xsResult);
				xsCall2(xsVar(2), xsID_set, xsResult, xsVar(3));
			}
			(*self)->skin = PIU(Skin, xsVar(3));
		}
	}
	if (xsFindResult(xsArg(1), xsID_Style)) {
		if (!xsIsInstanceOf(xsResult, xsFunctionPrototype))
			xsUnknownError("Style is no function");
		xsResult = xsCallFunction0(xsResult, xsNull);
		xsVar(0) = xsGet(xsGlobal, xsID_Style);
		xsVar(1) = xsGet(xsVar(0), xsID_prototype);
		if (!xsIsInstanceOf(xsResult, xsVar(1)))
			xsUnknownError("Style is no style template");
		(*self)->style = PIU(Style, xsResult);
	}
	else if (xsFindResult(xsArg(1), xsID_style)) {
		if (!xsIsInstanceOf(xsResult, xsObjectPrototype))
			xsUnknownError("style is no object");
		xsVar(0) = xsGet(xsGlobal, xsID_Style);
		xsVar(1) = xsGet(xsVar(0), xsID_prototype);
		if (xsIsInstanceOf(xsResult, xsVar(1)))
			(*self)->style = PIU(Style, xsResult);
		else {
			xsVar(2) = xsGet(xsGlobal, xsID_assetMap);
			if (xsTest(xsVar(2)))
				xsVar(3) = xsCall1(xsVar(2), xsID_get, xsResult);
			else {
				xsVar(2) = xsNew0(xsGlobal, xsID_Map);
				xsSet(xsGlobal, xsID_assetMap, xsVar(2));
				xsVar(3) = xsUndefined;
			}
			if (xsTest(xsVar(3))) {
				if (!xsIsInstanceOf(xsVar(3), xsVar(1)))
					xsUnknownError("style is no style dictionary");
			}
			else {
				xsVar(3) = xsNewFunction1(xsVar(0), xsResult);
				xsCall2(xsVar(2), xsID_set, xsResult, xsVar(3));
			}
			(*self)->style = PIU(Style, xsVar(3));
		}
	}
	if (xsFindNumber(xsArg(1), xsID_state, &number))
		(*self)->state = number;
	if (xsFindInteger(xsArg(1), xsID_variant, &integer))
		(*self)->variant = (PiuVariant)integer;
}

void PiuContentDraw(void* it, PiuView* view, PiuRectangle area) 
{
	PiuContent* self = it;
	PiuSkin* skin = (*self)->skin;
	if (skin) {
		PiuRectangleRecord bounds;
		PiuRectangleSet(&bounds, 0, 0, (*self)->bounds.width, (*self)->bounds.height);
		if (bounds.width && bounds.height)
			PiuSkinDraw(skin, view, &bounds, (*self)->variant, (*self)->state, (*self)->coordinates.horizontal, (*self)->coordinates.vertical);
	}
}

void PiuContentFitHorizontally(void* it) 
{
	PiuContent* self = it;
	if (((*self)->coordinates.horizontal & piuLeftRight) != piuLeftRight)
		(*self)->bounds.width = (*self)->coordinates.width;
	(*self)->flags &= ~(piuWidthChanged | piuContentsHorizontallyChanged);
}

void PiuContentFitVertically(void* it) 
{
	PiuContent* self = it;
	if (((*self)->coordinates.vertical & piuTopBottom) != piuTopBottom)
		(*self)->bounds.height = (*self)->coordinates.height;
	(*self)->flags &= ~(piuHeightChanged | piuContentsVerticallyChanged);
}

void PiuContentFromApplicationCoordinates(void* it, PiuCoordinate x0, PiuCoordinate y0, PiuCoordinate *x1, PiuCoordinate *y1)
{
	PiuContent* self = it;
	PiuContainer* container = (*self)->container;
	x0 -= (*self)->bounds.x;
	y0 -= (*self)->bounds.y;
	while (container) {
		x0 -= (*container)->bounds.x;
		y0 -= (*container)->bounds.y;
		container = (*container)->container;
	}
	*x1 = x0;
	*y1 = y0;
}

void* PiuContentHit(void* it, PiuCoordinate x, PiuCoordinate y) 
{
	PiuContent* self = it;
	if ((*self)->flags & piuActive) {
		if ((0 <= x) && (0 <= y) && (x < (*self)->bounds.width) && (y < (*self)->bounds.height))
			return self;
	}
	return NULL;
}

void PiuContentIdle(void* it, PiuInterval interval)
{
	PiuContent* self = it;
	PiuContentSetTime(self, (*self)->time + interval);
	if ((*self)->duration && ((*self)->time == (*self)->duration)) {
		if ((*self)->flags & piuLoop) {
			(*self)->time = 0;
		}
		else {
			PiuContentStop(self);
			PiuBehaviorOnFinished(self);
		}
	}
}

void PiuContentInvalidate(void* it, PiuRectangle area) 
{
	PiuContent* self = it;
	if ((*self)->flags & piuVisible) {
		PiuContainer* container = (*self)->container;
		if (container) {
			if (area) {
				if (!PiuRectangleIsEmpty(area)) {
					PiuRectangleOffset(area, (*self)->bounds.x, (*self)->bounds.y);
					(*(*container)->dispatch->invalidate)((*self)->container, area);
				}
			}
			else {
				PiuRectangleRecord bounds;
				PiuRectangleSet(&bounds, (*self)->bounds.x, (*self)->bounds.y, (*self)->bounds.width, (*self)->bounds.height);
				(*(*container)->dispatch->invalidate)((*self)->container, &bounds);
			}
		}
	}
}

PiuBoolean PiuContentIsShown(void* it)
{
	PiuContent* self = it;
	PiuContainer* container = (*self)->container;
	if ((*self)->application == it)
		return 1;
	if (((*self)->flags & piuVisible) && container)
		return PiuContentIsShown(container);
	return 0;
}

void PiuContentMark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	PiuContent self = it;
	PiuMarkReference(the, self->behavior);
	PiuMarkHandle(the, self->container);
	PiuMarkHandle(the, self->skin);
	PiuMarkHandle(the, self->style);
	PiuMarkString(the, self->name);
}

void PiuContentMeasureHorizontally(void* it) 
{
	PiuContent* self = it;
	PiuAlignment horizontal = (*self)->coordinates.horizontal;
	if (!(horizontal & piuWidth)) {
		PiuSkin* skin = (*self)->skin;
		(*self)->coordinates.width = (skin) ? PiuSkinGetWidth(skin) : 0;
	}
}

void PiuContentMeasureVertically(void* it) 
{
	PiuContent* self = it;
	PiuAlignment vertical = (*self)->coordinates.vertical;
	if (!(vertical & piuHeight)) {
		PiuSkin* skin = (*self)->skin;
		(*self)->coordinates.height = (skin) ? PiuSkinGetHeight(skin) : 0;
	}
}

void PiuContentMoveBy(PiuContent* self, PiuCoordinate dx, PiuCoordinate dy)
{
	PiuAlignment horizontal = (*self)->coordinates.horizontal & piuLeftRightWidth;
	PiuAlignment vertical = (*self)->coordinates.vertical & piuTopBottomHeight;
	if ((horizontal == piuLeftRight) || (horizontal == piuWidth) || (horizontal == piuCenter)) dx = 0;
	if ((vertical == piuTopBottom) || (vertical == piuHeight) || (vertical == piuMiddle)) dy = 0;
	if (dx || dy) {
		PiuContainer* container = (*self)->container;
		if (container)
			PiuContentInvalidate(self, NULL);
		if (horizontal & piuLeft)
			(*self)->coordinates.left += dx;
		else
			(*self)->coordinates.right -= dx;
		if (vertical & piuTop)
			(*self)->coordinates.top += dy;
		else
			(*self)->coordinates.bottom -= dy;
		if (container) {
			PiuFlags flags = 0;
			if (dx) flags |= piuXChanged;
			if (dy) flags |= piuYChanged;
			PiuContentReflow(self, flags);
		}
	}
}

void PiuContentPlace(void* it) 
{
	PiuContent* self = it;
	if ((*self)->flags & piuPlaced) {
		(*self)->flags &= ~piuPlaced;
		PiuContentInvalidate(self, NULL);
	}
	if ((*self)->flags & piuDisplaying) {
		(*self)->flags &= ~piuDisplaying;
		PiuBehaviorOnDisplaying(self);
	}
}

void PiuContentReflow(void* it, uint32_t flags) 
{
	PiuContent* self = it;
	PiuContainer* container = (*self)->container;
	(*self)->flags |= flags;
	if (flags & (piuPositionChanged | piuSizeChanged))
		(*self)->flags &= ~piuPlaced;
	if (container)
		(*(*container)->dispatch->reflow)(container, flags);
}

void PiuContentSetCoordinates(void* it, PiuCoordinates coordinates) 
{
	PiuContent* self = it;
	PiuAlignment horizontal = (*self)->coordinates.horizontal;
	PiuAlignment vertical = (*self)->coordinates.vertical;
	PiuBoolean flag;
	if (horizontal != coordinates->horizontal) 
		flag = 1;
	else if (vertical != coordinates->vertical)
		flag = 1;
	else if ((horizontal & piuLeft) && ((*self)->coordinates.left != coordinates->left))
		flag = 1;
	else if ((horizontal & piuRight) && ((*self)->coordinates.right != coordinates->right))
		flag = 1;
	else if ((horizontal & piuWidth) && ((*self)->coordinates.width != coordinates->width))
		flag = 1;
	else if ((vertical & piuTop) && ((*self)->coordinates.top != coordinates->top))
		flag = 1;
	else if ((vertical & piuBottom) && ((*self)->coordinates.bottom != coordinates->bottom))
		flag = 1;
	else if ((vertical & piuHeight) && ((*self)->coordinates.height != coordinates->height))
		flag = 1;
	else
		flag = 0;
	if (flag) {
		PiuContainer* container = (*self)->container;
		if (container)
			PiuContentInvalidate(self, NULL);
		(*self)->coordinates = *coordinates;
		if (container)
			PiuContentReflow(self, piuSizeChanged);
	}
}

void PiuContentSetDuration(PiuContent* self, double duration)
{
	if (duration < 0)
		duration = 0;
	(*self)->duration = duration;
	PiuContentSetTime(self, (*self)->time);
}

void PiuContentSetFraction(PiuContent* self, double fraction)
{
	double duration = (*self)->duration;
	if (duration)
		PiuContentSetTime(self, duration * fraction);
}

void PiuContentSetInterval(PiuContent* self, PiuInterval interval)
{
	if (interval < 1)
		interval = 1;
	(*self)->interval = interval;
}

void PiuContentSetTime(PiuContent* self, double time)
{
	double duration = (*self)->duration;
	if (time < 0)
		time = 0;
	if (duration && (time > duration))
		time = duration;
	if ((*self)->time != time) {
		(*self)->time = time;
		(*(*self)->dispatch->sync)(self);
		PiuBehaviorOnTimeChanged(self);
	}
}

void PiuContentShow(PiuContent* self, PiuBoolean showIt)
{
	PiuBoolean visible = ((*self)->flags & piuVisible) ? 1 : 0;
	if (visible != showIt) {
		PiuBoolean flag = (*self)->container ? PiuContentIsShown((*self)->container) : 0;
		if (flag) 
			(*(*self)->dispatch->showing)(self, showIt);
		if (showIt) {
			(*self)->flags |= piuVisible;
			PiuContentInvalidate(self, NULL);
		}
		else {
			PiuContentInvalidate(self, NULL);
			(*self)->flags &= ~piuVisible;
		}
		if (flag) 
			(*(*self)->dispatch->shown)(self, showIt);
	}
}

void PiuContentShowing(void* it, PiuBoolean showIt) 
{
	PiuContent* self = it;
	PiuContentInvalidate(self, NULL);
}

void PiuContentShown(void* it, PiuBoolean showIt) 
{
	PiuContent* self = it;
	PiuContentInvalidate(self, NULL);
}

void PiuContentSizeBy(PiuContent* self, PiuCoordinate dx, PiuCoordinate dy)
{
	if (!((*self)->coordinates.horizontal & piuWidth)) dx = 0;
	if (!((*self)->coordinates.vertical & piuHeight)) dy = 0;
	if (dx || dy) {
		PiuContainer* container = (*self)->container;
		if (container)
			PiuContentInvalidate(self, NULL);
		(*self)->coordinates.width += dx;
		(*self)->coordinates.height += dy;
		if (container) {
			PiuFlags flags = 0;
			if (dx) flags |= piuWidthChanged;
			if (dy) flags |= piuHeightChanged;
			PiuContentReflow(self, flags);
		}
	}
}

void PiuContentStart(PiuContent* self)
{
	if (!((*self)->flags & piuIdling)) {
		(*self)->flags |= piuIdling;
		if ((*self)->application)
			PiuApplicationStartContent((*self)->application, self);
	}
}

void PiuContentStop(PiuContent* self)
{
	if ((*self)->flags & piuIdling) {
		(*self)->flags &= ~piuIdling;
		if ((*self)->application)
			PiuApplicationStopContent((*self)->application, self);
	}
}

void PiuContentSync(void* it) 
{
}

void PiuContentToApplicationCoordinates(void* it, PiuCoordinate x0, PiuCoordinate y0, PiuCoordinate *x1, PiuCoordinate *y1)
{
	PiuContent* self = it;
	PiuContainer* container = (*self)->container;
	x0 += (*self)->bounds.x;
	y0 += (*self)->bounds.y;
	while (container) {
		x0 += (*container)->bounds.x;
		y0 += (*container)->bounds.y;
		container = (*container)->container;
	}
	*x1 = x0;
	*y1 = y0;
}

void PiuContentUnbind(void* it, PiuApplication* application, PiuView* view)
{
	PiuContent* self = it;
#ifdef piuGPU
	PiuSkin* skin = (*self)->skin;
	if (skin)
		PiuSkinUnbind(skin, application, view);
#endif
#ifdef piuPC
	if ((*application)->hover == self)
		(*application)->hover = NULL;
#endif
	if ((*application)->focus == self)
		PiuApplicationSetFocus(application, NULL);
	if ((*self)->flags & piuIdling)
		PiuApplicationStopContent((*self)->application, self);
	(*self)->application = NULL;
}

void PiuContentUpdate(void* it, PiuView* view, PiuRectangle area) 
{
	PiuContent* self = it;
	PiuRectangle bounds = &((*self)->bounds);
	if (((*self)->flags & piuVisible) && PiuRectangleIntersects(bounds, area)) {
		PiuCoordinate x = bounds->x;
		PiuCoordinate y = bounds->y;
		PiuViewPushOrigin(view, x, y);
		PiuRectangleOffset(area, -x, -y);
		(*(*self)->dispatch->draw)(self, view, area);
		PiuRectangleOffset(area, x, y);
		PiuViewPopOrigin(view);
	}
}

void PiuContent_create(xsMachine* the)
{
	PiuContent* self;
	xsVars(4);
	xsSetHostChunk(xsThis, NULL, sizeof(PiuContentRecord));
	self = PIU(Content, xsThis);
	(*self)->the = the;
	(*self)->reference = xsToReference(xsThis);
	xsSetHostHooks(xsThis, (xsHostHooks*)&PiuContentHooks);
	(*self)->dispatch = (PiuDispatch)&PiuContentDispatchRecord;
	(*self)->flags = piuVisible;
	PiuContentDictionary(the, self);
	PiuBehaviorOnCreate(self);
}

void PiuContent_get_active(xsMachine *the)
{
	PiuContent* self = PIU(Content, xsThis);
	xsResult = ((*self)->flags & piuActive) ? xsTrue : xsFalse;
}

void PiuContent_get_application(xsMachine *the)
{
	PiuContent* self = PIU(Content, xsThis);
	PiuApplication* result = (*self)->application;
	if (result)
		xsResult = xsReference((*result)->reference);
}
void PiuContent_get_backgroundTouch(xsMachine *the)
{
	PiuContent* self = PIU(Content, xsThis);
	xsResult = ((*self)->flags & piuBackgroundTouch) ? xsTrue : xsFalse;
}

void PiuContent_get_behavior(xsMachine *the)
{
	PiuContent* self = PIU(Content, xsThis);
	xsSlot* behavior = (*self)->behavior;
	if (behavior)
		xsResult = xsReference(behavior);
}

void PiuContent_get_bounds(xsMachine *the)
{
	PiuContent* self = PIU(Content, xsThis);
	PiuCoordinate x, y;
	if ((*self)->application) {
		PiuApplicationAdjust((*self)->application);
		PiuContentToApplicationCoordinates(self, 0, 0, &x, &y);
		xsResult = xsNewObject();
		xsDefine(xsResult, xsID_x, xsPiuCoordinate(x), xsDefault);
		xsDefine(xsResult, xsID_y, xsPiuCoordinate(y), xsDefault);
		xsDefine(xsResult, xsID_width, xsPiuDimension((*self)->bounds.width), xsDefault);
		xsDefine(xsResult, xsID_height, xsPiuDimension((*self)->bounds.height), xsDefault);
	}
}

void PiuContent_get_container(xsMachine *the)
{
	PiuContent* self = PIU(Content, xsThis);
	PiuContainer* result = (*self)->container;
	if (result)
		xsResult = xsReference((*result)->reference);
}

void PiuContent_get_coordinates(xsMachine *the)
{
	PiuContent* self = PIU(Content, xsThis);
	PiuCoordinates coordinates = &((*self)->coordinates);
	PiuAlignment horizontal = coordinates->horizontal;
	PiuAlignment vertical = coordinates->vertical;
	PiuCoordinate left = coordinates->left;
	PiuCoordinate width = coordinates->width;
	PiuCoordinate top = coordinates->top;
	PiuCoordinate right = coordinates->right;
	PiuCoordinate height = coordinates->height;
	PiuCoordinate bottom = coordinates->bottom;
	xsResult = xsNewObject();
	if (horizontal & piuLeft)
		xsDefine(xsResult, xsID_left, xsPiuCoordinate(left), xsDefault);
	if (horizontal & piuWidth)
		xsDefine(xsResult, xsID_width, xsPiuDimension(width), xsDefault);
	if (horizontal & piuRight)
		xsDefine(xsResult, xsID_right, xsPiuCoordinate(right), xsDefault);
	if (vertical & piuTop)
		xsDefine(xsResult, xsID_top, xsPiuCoordinate(top), xsDefault);
	if (vertical & piuHeight)
		xsDefine(xsResult, xsID_height, xsPiuDimension(height), xsDefault);
	if (vertical & piuBottom)
		xsDefine(xsResult, xsID_bottom, xsPiuCoordinate(bottom), xsDefault);
}

void PiuContent_get_duration(xsMachine *the)
{
	PiuContent* self = PIU(Content, xsThis);
	xsResult = xsNumber((*self)->duration);
}

void PiuContent_get_exclusiveTouch(xsMachine *the)
{
	PiuContent* self = PIU(Content, xsThis);
	xsResult = ((*self)->flags & piuExclusiveTouch) ? xsTrue : xsFalse;
}

void PiuContent_get_fraction(xsMachine *the)
{
	PiuContent* self = PIU(Content, xsThis);
	if ((*self)->duration)
		xsResult = xsNumber((*self)->time / (*self)->duration);
}

void PiuContent_get_index(xsMachine *the)
{
	PiuContent* self = PIU(Content, xsThis);
	PiuContainer* container = (*self)->container;
	xsIntegerValue result;
	if (container) {
		PiuContent* content = (*container)->first;
		result = 0;
		while (content) {
			if (content == self)
				break;
			result++;
			content = (*content)->next;
		}
	}
	else
		result = -1;
	xsResult = xsInteger(result);
}

void PiuContent_get_interval(xsMachine *the)
{
	PiuContent* self = PIU(Content, xsThis);
#ifdef piuPC
	xsResult = xsNumber((*self)->interval);
#else
	xsResult = xsInteger((*self)->interval);
#endif
}

void PiuContent_get_loop(xsMachine *the)
{
	PiuContent* self = PIU(Content, xsThis);
	xsResult = ((*self)->flags & piuLoop) ? xsTrue : xsFalse;
}

void PiuContent_get_multipleTouch(xsMachine *the)
{
	PiuContent* self = PIU(Content, xsThis);
	xsResult = ((*self)->flags & piuMultipleTouch) ? xsTrue : xsFalse;
}

void PiuContent_get_name(xsMachine *the)
{
	PiuContent* self = PIU(Content, xsThis);
	if ((*self)->name)
		xsResult = *((*self)->name);
}

void PiuContent_get_next(xsMachine *the)
{
	PiuContent* self = PIU(Content, xsThis);
	PiuContent* result = (*self)->next;
	if (result)
		xsResult = xsReference((*result)->reference);
}

void PiuContent_get_offset(xsMachine *the)
{
	PiuContent* self = PIU(Content, xsThis);
	if ((*self)->application) {
		PiuApplicationAdjust((*self)->application);
		xsResult = xsNewObject();
		xsDefine(xsResult, xsID_x, xsPiuCoordinate((*self)->bounds.x), xsDefault);
		xsDefine(xsResult, xsID_y, xsPiuCoordinate((*self)->bounds.y), xsDefault);
	}
}

void PiuContent_get_position(xsMachine *the)
{
	PiuContent* self = PIU(Content, xsThis);
	PiuCoordinate x, y;
	if ((*self)->application) {
		PiuApplicationAdjust((*self)->application);
		PiuContentToApplicationCoordinates(self, 0, 0, &x, &y);
		xsResult = xsNewObject();
		xsDefine(xsResult, xsID_x, xsPiuCoordinate(x), xsDefault);
		xsDefine(xsResult, xsID_y, xsPiuCoordinate(y), xsDefault);
	}
}

void PiuContent_get_previous(xsMachine *the)
{
	PiuContent* self = PIU(Content, xsThis);
	PiuContent* result = (*self)->previous;
	if (result)
		xsResult = xsReference((*result)->reference);
}

void PiuContent_get_running(xsMachine *the)
{
	PiuContent* self = PIU(Content, xsThis);
	xsResult = ((*self)->flags & piuIdling) ? xsTrue : xsFalse;
}

void PiuContent_get_size(xsMachine *the)
{
	PiuContent* self = PIU(Content, xsThis);
	if ((*self)->application) {
		PiuApplicationAdjust((*self)->application);
		xsResult = xsNewObject();
		xsDefine(xsResult, xsID_width, xsPiuDimension((*self)->bounds.width), xsDefault);
		xsDefine(xsResult, xsID_height, xsPiuDimension((*self)->bounds.height), xsDefault);
	}
}

void PiuContent_get_skin(xsMachine *the)
{
	PiuContent* self = PIU(Content, xsThis);
	PiuSkin* result = (*self)->skin;
	if (result)
		xsResult = xsReference((*result)->reference);
}

void PiuContent_get_state(xsMachine *the)
{
	PiuContent* self = PIU(Content, xsThis);
	xsResult = xsNumber((*self)->state);
}

void PiuContent_get_style(xsMachine *the)
{
	PiuContent* self = PIU(Content, xsThis);
	PiuStyle* result = (*self)->style;
	if (result)
		xsResult = xsReference((*result)->reference);
}

void PiuContent_get_time(xsMachine *the)
{
	PiuContent* self = PIU(Content, xsThis);
	xsResult = xsNumber((*self)->time);
}

void PiuContent_get_type(xsMachine *the)
{
	PiuContent* self = PIU(Content, xsThis);
	fxStringX(the, &xsResult, (*self)->dispatch->type);
}

void PiuContent_get_variant(xsMachine *the)
{
	PiuContent* self = PIU(Content, xsThis);
	xsResult = xsInteger((*self)->variant);
}

void PiuContent_get_visible(xsMachine *the)
{
	PiuContent* self = PIU(Content, xsThis);
	xsResult = ((*self)->flags & piuVisible) ? xsTrue : xsFalse;
}

void PiuContent_get_x(xsMachine *the)
{
	PiuContent* self = PIU(Content, xsThis);
	PiuCoordinate x, y;
	if ((*self)->application) {
		PiuApplicationAdjust((*self)->application);
		PiuContentToApplicationCoordinates(self, 0, 0, &x, &y);
		xsResult = xsPiuCoordinate(x);
	}
}

void PiuContent_get_y(xsMachine *the)
{
	PiuContent* self = PIU(Content, xsThis);
	PiuCoordinate x, y;
	if ((*self)->application) {
		PiuApplicationAdjust((*self)->application);
		PiuContentToApplicationCoordinates(self, 0, 0, &x, &y);
		xsResult = xsPiuCoordinate(y);
	}
}

void PiuContent_get_width(xsMachine *the)
{
	PiuContent* self = PIU(Content, xsThis);
	if ((*self)->coordinates.horizontal & piuWidth)
		xsResult = xsPiuDimension((*self)->coordinates.width);
	else if ((*self)->application) {
		PiuApplicationAdjust((*self)->application);
		xsResult = xsPiuDimension((*self)->bounds.width);
	}
}

void PiuContent_get_height(xsMachine *the)
{
	PiuContent* self = PIU(Content, xsThis);
	if ((*self)->coordinates.vertical & piuHeight)
		xsResult = xsPiuDimension((*self)->coordinates.height);
	else if ((*self)->application) {
		PiuApplicationAdjust((*self)->application);
		xsResult = xsPiuDimension((*self)->bounds.height);
	}
}

void PiuContent_set_active(xsMachine *the)
{
	PiuContent* self = PIU(Content, xsThis);
	if (xsTest(xsArg(0)))
		(*self)->flags |= piuActive;
	else
		(*self)->flags &= ~piuActive;
}

void PiuContent_set_backgroundTouch(xsMachine *the)
{
	PiuContent* self = PIU(Content, xsThis);
	if (xsTest(xsArg(0)))
		(*self)->flags |= piuBackgroundTouch;
	else
		(*self)->flags &= ~piuBackgroundTouch;
}

void PiuContent_set_behavior(xsMachine *the)
{
	PiuContent* self = PIU(Content, xsThis);
	(*self)->behavior = xsToReference(xsArg(0));
}

void PiuContent_set_coordinates(xsMachine *the)
{
	PiuContent* self = PIU(Content, xsThis);
	xsIntegerValue value;
	PiuCoordinatesRecord coordinates;
	c_memset(&coordinates, 0, sizeof(coordinates));
	if (xsFindInteger(xsArg(0), xsID_left, &value)) {
		coordinates.horizontal |= piuLeft;
		coordinates.left = (PiuCoordinate)value;
	}
	if (xsFindInteger(xsArg(0), xsID_width, &value)) {
		coordinates.horizontal |= piuWidth;
		coordinates.width = (PiuCoordinate)value;
	}
	if (xsFindInteger(xsArg(0), xsID_right, &value)) {
		coordinates.horizontal |= piuRight;
		coordinates.right = (PiuCoordinate)value;
	}
	if (xsFindInteger(xsArg(0), xsID_top, &value)) {
		coordinates.vertical |= piuTop;
		coordinates.top = (PiuCoordinate)value;
	}
	if (xsFindInteger(xsArg(0), xsID_height, &value)) {
		coordinates.vertical |= piuHeight;
		coordinates.height = (PiuCoordinate)value;
	}
	if (xsFindInteger(xsArg(0), xsID_bottom, &value)) {
		coordinates.vertical |= piuBottom;
		coordinates.bottom = (PiuCoordinate)value;
	}
	PiuContentSetCoordinates(self, &coordinates);
}

void PiuContent_set_duration(xsMachine *the)
{
	PiuContent* self = PIU(Content, xsThis);
	PiuContentSetDuration(self, xsToNumber(xsArg(0)));
}

void PiuContent_set_exclusiveTouch(xsMachine *the)
{
	PiuContent* self = PIU(Content, xsThis);
	if (xsTest(xsArg(0)))
		(*self)->flags |= piuExclusiveTouch;
	else
		(*self)->flags &= ~piuExclusiveTouch;
}

void PiuContent_set_fraction(xsMachine *the)
{
	PiuContent* self = PIU(Content, xsThis);
	PiuContentSetFraction(self, xsToNumber(xsArg(0)));
}

void PiuContent_set_interval(xsMachine *the)
{
	PiuContent* self = PIU(Content, xsThis);
#ifdef piuPC
	PiuContentSetInterval(self, xsToNumber(xsArg(0)));
#else
	PiuContentSetInterval(self, xsToInteger(xsArg(0)));
#endif
}

void PiuContent_set_loop(xsMachine *the)
{
	PiuContent* self = PIU(Content, xsThis);
	if (xsTest(xsArg(0)))
		(*self)->flags |= piuLoop;
	else
		(*self)->flags &= ~piuLoop;
}

void PiuContent_set_multipleTouch(xsMachine *the)
{
	PiuContent* self = PIU(Content, xsThis);
	if (xsTest(xsArg(0)))
		(*self)->flags |= piuMultipleTouch;
	else
		(*self)->flags &= ~piuMultipleTouch;
}

void PiuContent_set_name(xsMachine *the)
{
	PiuContent* self = PIU(Content, xsThis);
	if (xsTest(xsArg(0))) {
		xsSlot* name = PiuString(xsArg(0));
		(*self)->name = name;
	}
	else
		(*self)->name = NULL;
}

void PiuContent_set_offset(xsMachine *the)
{
	PiuContent* self = PIU(Content, xsThis);
	xsIntegerValue x = 0, y = 0;
	if ((*self)->application) {
		PiuApplicationAdjust((*self)->application);
		if (xsFindInteger(xsArg(0), xsID_x, &x))
			x -= (*self)->bounds.x;
		if (xsFindInteger(xsArg(0), xsID_y, &y))
			y -= (*self)->bounds.y;
		PiuContentMoveBy(self, (PiuCoordinate)x, (PiuCoordinate)y);
	}
}

void PiuContent_set_position(xsMachine *the)
{
	PiuContent* self = PIU(Content, xsThis);
	PiuCoordinate x0, y0;
	xsIntegerValue x1 = 0, y1 = 0;
	if ((*self)->application) {
		PiuApplicationAdjust((*self)->application);
		PiuContentToApplicationCoordinates(self, 0, 0, &x0, &y0);
		if (xsFindInteger(xsArg(0), xsID_x, &x1))
			x1 -= x0;
		if (xsFindInteger(xsArg(0), xsID_y, &y1))
			y1 -= y0;
		PiuContentMoveBy(self, (PiuCoordinate)x1, (PiuCoordinate)y1);
	}
}

void PiuContent_set_size(xsMachine *the)
{
	PiuContent* self = PIU(Content, xsThis);
	xsIntegerValue width = 0, height = 0;
	if ((*self)->application) {
		PiuApplicationAdjust((*self)->application);
		if (xsFindInteger(xsArg(0), xsID_width, &width))
			width -= (*self)->bounds.width;
		if (xsFindInteger(xsArg(0), xsID_height, &height))
			height -= (*self)->bounds.height;
		PiuContentSizeBy(self, (PiuCoordinate)width, (PiuCoordinate)height);
	}
}

void PiuContent_set_skin(xsMachine *the)
{
	PiuContent* self = PIU(Content, xsThis);
#ifdef piuGPU
	PiuApplication* application = (*self)->application;	
#endif
	PiuSkin* skin = NULL;
	if (xsTest(xsArg(0)))
		skin = PIU(Skin, xsArg(0));
#ifdef piuGPU
	if (application && (*self)->skin)
		PiuSkinUnbind((*self)->skin, application, (*application)->view);
#endif
	(*self)->skin = skin;
#ifdef piuGPU
	if (application && (*self)->skin)
		PiuSkinBind((*self)->skin, application, (*application)->view);
#endif
	PiuContentReflow(self, piuSizeChanged);
}

void PiuContent_set_state(xsMachine *the)
{
	PiuContent* self = PIU(Content, xsThis);
	PiuState state = xsToNumber(xsArg(0));
	if ((*self)->state != state) {
		(*self)->state = state;
		(*(*self)->dispatch->invalidate)(self, NULL);
	}
}

void PiuContent_set_style(xsMachine *the)
{
	PiuContent* self = PIU(Content, xsThis);
	PiuStyle* style = NULL;
	if (xsTest(xsArg(0)))
		style = PIU(Style, xsArg(0));
	(*self)->style = style;
	if ((*self)->application)
		(*(*self)->dispatch->cascade)(self);
}

void PiuContent_set_time(xsMachine *the)
{
	PiuContent* self = PIU(Content, xsThis);
	PiuContentSetTime(self, xsToNumber(xsArg(0)));
}

void PiuContent_set_variant(xsMachine *the)
{
	PiuContent* self = PIU(Content, xsThis);
	PiuVariant variant = (PiuVariant)xsToInteger(xsArg(0)); 
	if ((*self)->variant != variant) {
		(*self)->variant = variant;
		(*(*self)->dispatch->invalidate)(self, NULL);
	}
}

void PiuContent_set_visible(xsMachine *the)
{
	PiuContent* self = PIU(Content, xsThis);
	PiuContentShow(self, (PiuBoolean)xsTest(xsArg(0)));
}

void PiuContent_set_x(xsMachine *the)
{
	PiuContent* self = PIU(Content, xsThis);
	PiuCoordinate x, y;
	if ((*self)->application) {
		PiuApplicationAdjust((*self)->application);
		PiuContentToApplicationCoordinates(self, 0, 0, &x, &y);
		PiuContentMoveBy(self, xsToPiuCoordinate(xsArg(0)) - x, 0);
	}
}

void PiuContent_set_y(xsMachine *the)
{
	PiuContent* self = PIU(Content, xsThis);
	PiuCoordinate x, y;
	if ((*self)->application) {
		PiuApplicationAdjust((*self)->application);
		PiuContentToApplicationCoordinates(self, 0, 0, &x, &y);
		PiuContentMoveBy(self, 0, xsToPiuCoordinate(xsArg(0)) - y);
	}
}

void PiuContent_set_width(xsMachine *the)
{
	PiuContent* self = PIU(Content, xsThis);
	if ((*self)->application) {
		PiuApplicationAdjust((*self)->application);
		PiuContentSizeBy(self, xsToPiuCoordinate(xsArg(0)) - (*self)->bounds.width, 0);
	}
}

void PiuContent_set_height(xsMachine *the)
{
	PiuContent* self = PIU(Content, xsThis);
	if ((*self)->application) {
		PiuApplicationAdjust((*self)->application);
		PiuContentSizeBy(self, 0, xsToPiuCoordinate(xsArg(0)) - (*self)->bounds.height);
	}
}

void PiuContent_adjust(xsMachine *the)
{
	PiuContent* self = PIU(Content, xsThis);
	xsAssert(self != NULL);
	if ((*self)->container)
		PiuContentReflow(self, piuSizeChanged);
}

void PiuContent_bubble(xsMachine *the)
{
	PiuContent* self = PIU(Content, xsThis);
	xsIdentifier id = xsToID(xsArg(0));
	xsIntegerValue c = xsToInteger(xsArgc);
	xsVars(2);
	xsAssert(self != NULL);
	while (self) {
		PiuContent_delegateAux(the, self, id, c);
		if (xsTest(xsResult)) {
			xsResult = xsVar(1);
			return;
		}
		self = (PiuContent*)((*self)->container);
	}
}

void PiuContent_captureTouch(xsMachine *the)
{
	PiuContent* self = PIU(Content, xsThis);
	xsAssert(self != NULL);
	if ((*self)->application){
		xsIntegerValue id = xsToInteger(xsArg(0));
		PiuCoordinate x = xsToPiuCoordinate(xsArg(1));
		PiuCoordinate y = xsToPiuCoordinate(xsArg(2));
		xsNumberValue ticks = xsToNumber(xsArg(2));
		PiuApplicationCaptureTouch((*self)->application, self, id, x, y, ticks);
	}
}

void PiuContent_defer(xsMachine *the)
{
	PiuContent* self = PIU(Content, xsThis);
	xsIntegerValue c = xsToInteger(xsArgc), i;
	xsIdentifier id = xsToID(xsArg(0));
	PiuApplication* application;
	PiuDeferLink* link;
	PiuDeferLink* former;
	PiuDeferLink** address;
	xsVars(2);
	xsAssert(self != NULL);
	xsAssert((*self)->application != NULL);
	if (c > 8)
		xsErrorPrintf("too many parameters"); 
	application = (*self)->application;
	xsVar(0) = (*application)->DeferLink;
	xsVar(1) = xsNewFunction0(xsVar(0));
	link = PIU(DeferLink, xsVar(1));
	(*link)->content = PIU(Content, xsThis);
	(*link)->id = id;
	(*link)->argc = c - 1;
	for (i = 1; i < c; i++)
		(*link)->argv[i - 1] = xsArg(i);
	address = &((*application)->deferChain);
	while ((former = *address))
		address = &((*former)->deferLink);
	*address = link;
	PiuViewReschedule((*application)->view);
}

void PiuContent_delegate(xsMachine *the)
{
	PiuContent* self = PIU(Content, xsThis);
	xsIdentifier id = xsToID(xsArg(0));
	xsIntegerValue c = xsToInteger(xsArgc);
	xsVars(2);
	xsAssert(self != NULL);
	xsAssert((*self)->application != NULL);
	if (c > 8)
		xsErrorPrintf("too many parameters"); 
	PiuContent_delegateAux(the, self, id, c);
}

void PiuContent_delegateAux(xsMachine *the, PiuContent* content, xsIdentifier id, xsIntegerValue c)
{
	xsIntegerValue i;
	if ((*content)->behavior) {
		xsVar(0) = xsReference((*content)->behavior);
		if (xsFindResult(xsVar(0), id)) {
			xsVar(1) = xsReference((*content)->reference);
			xsOverflow(-(XS_FRAME_COUNT + c));
			fxPush(xsVar(0));
			fxPush(xsResult);
			fxCall(the);
			fxPush(xsVar(1));
			for (i = 1; i < c; i++) {
				fxPush(xsArg(i));
			}
			fxRunCount(the, c);
			xsResult = fxPop();
			return;
		}
	}
	xsResult = xsUndefined;
}

void PiuContent_distribute(xsMachine *the)
{
	PiuContent* self = PIU(Content, xsThis);
	xsIdentifier id = xsToID(xsArg(0));
	xsIntegerValue c = xsToInteger(xsArgc);
	xsVars(2);
	xsAssert(self != NULL);
	if (PiuContent_distributeAux(the, (PiuContainer*)self, id, c))
		xsResult = xsVar(1);
	else
		xsResult = xsUndefined;
}

PiuBoolean PiuContent_distributeAux(xsMachine *the, PiuContainer* container, xsIdentifier id, xsIntegerValue c)
{
	if ((*container)->flags & piuContainer) {
		PiuContent* content = (*container)->first;
		while (content) {
			if (PiuContent_distributeAux(the, (PiuContainer*)content, id, c))
				return 1;
			content = (*content)->next;
		}
	}
	PiuContent_delegateAux(the, (PiuContent*)container, id, c);
	return (PiuBoolean)xsTest(xsResult);
}

void PiuContent_focus(xsMachine *the)
{
	PiuContent* self = PIU(Content, xsThis);
	if ((*self)->application) {
		PiuApplicationSetFocus((*self)->application, self);
	}
}

void PiuContent_hit(xsMachine *the)
{
	PiuContent* self = PIU(Content, xsThis);
	PiuContent* result = NULL;
	xsAssert(self != NULL);
	if ((*self)->application) {
		PiuCoordinate x = xsToPiuCoordinate(xsArg(0));
		PiuCoordinate y = xsToPiuCoordinate(xsArg(1));
		PiuContentFromApplicationCoordinates(self, x, y, &x, &y);
		result = (*(*self)->dispatch->hit)(self, x, y);
	}
	if (result)
		xsResult = xsReference((*result)->reference);
}

void PiuContent_measure(xsMachine *the)
{
	PiuContent* self = PIU(Content, xsThis);
	xsAssert(self != NULL);
	xsResult = xsNewObject();
	xsDefine(xsResult, xsID_width, xsPiuDimension((*self)->coordinates.width), xsDefault);
	xsDefine(xsResult, xsID_height, xsPiuDimension((*self)->coordinates.height), xsDefault);
}

void PiuContent_moveBy(xsMachine *the)
{
	PiuContent* self = PIU(Content, xsThis);
	PiuCoordinate x = xsToPiuCoordinate(xsArg(0));
	PiuCoordinate y = xsToPiuCoordinate(xsArg(1));
	xsAssert(self != NULL);
	PiuContentMoveBy(self, x, y);
}

void PiuContent_render(xsMachine *the)
{
    PiuContent* self = PIU(Content, xsThis);
    xsAssert(self != NULL);
    if ((*self)->application)
        PiuApplicationAdjust((*self)->application);
}

void PiuContent_sizeBy(xsMachine *the)
{
	PiuContent* self = PIU(Content, xsThis);
	PiuCoordinate x = xsToPiuCoordinate(xsArg(0));
	PiuCoordinate y = xsToPiuCoordinate(xsArg(1));
	xsAssert(self != NULL);
	PiuContentSizeBy(self, x, y);
}

void PiuContent_start(xsMachine *the)
{
	PiuContent* self = PIU(Content, xsThis);
	xsAssert(self != NULL);
	PiuContentStart(self);
}

void PiuContent_stop(xsMachine *the)
{
	PiuContent* self = PIU(Content, xsThis);
	xsAssert(self != NULL);
	PiuContentStop(self);
}



