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

static void PiuContainerBindContent(PiuContainer* self, PiuContent* content);
static void PiuContainerUnbindContent(PiuContainer* self, PiuContent* content);

const PiuDispatchRecord ICACHE_FLASH_ATTR PiuContainerDispatchRecord = {
	"Container",
	PiuContainerBind,
	PiuContainerCascade,
	PiuContentDraw,
	PiuContainerFitHorizontally,
	PiuContainerFitVertically,
	PiuContainerHit,
	PiuContentIdle,
	PiuContainerInvalidate,
	PiuContainerMeasureHorizontally,
	PiuContainerMeasureVertically,
	PiuContainerPlace,
	PiuContainerPlaceContentHorizontally,
	PiuContainerPlaceContentVertically,
	PiuContainerReflow,
	PiuContainerShowing,
	PiuContainerShown,
	PiuContentSync,
	PiuContainerUnbind,
	PiuContainerUpdate
};

const xsHostHooks ICACHE_FLASH_ATTR PiuContainerHooks = {
	PiuContentDelete,
	PiuContainerMark,
	NULL
};

void PiuContainerAdjustHorizontally(void* it) 
{
	PiuContainer* self = it;
	PiuContent* content = (*self)->first;
	PiuCoordinates coordinates;
	(*self)->flags &= ~piuContentsHorizontallyChanged;
	while (content) {
		if ((*content)->flags & piuWidthChanged) {
			PiuContentInvalidate(content, NULL);
			(*(*content)->dispatch->measureHorizontally)(content);
			coordinates = &((*content)->coordinates);
			if ((coordinates->horizontal & piuLeftRight) == piuLeftRight)
				(*content)->bounds.width = (*self)->bounds.width - coordinates->left - coordinates->right;
			else
				(*content)->bounds.width = coordinates->width;
			(*(*content)->dispatch->fitHorizontally)(content);
			(*content)->flags |= piuXChanged;
		}
		else if ((*content)->flags & piuContentsHorizontallyChanged) 
			PiuContainerAdjustHorizontally(content);
		if ((*content)->flags & piuXChanged)
			(*(*self)->dispatch->placeContentHorizontally)(self, content);
		content = (*content)->next;
	}
}

void PiuContainerAdjustVertically(void* it) 
{
	PiuContainer* self = it;
	PiuContent* content = (*self)->first;
	PiuCoordinates coordinates;
	(*self)->flags &= ~piuContentsVerticallyChanged;
	while (content) {
		if ((*content)->flags & piuHeightChanged) {
			PiuContentInvalidate(content, NULL);
			(*(*content)->dispatch->measureVertically)(content);
			coordinates = &((*content)->coordinates);
			if ((coordinates->vertical & piuTopBottom) == piuTopBottom)
				(*content)->bounds.height = (*self)->bounds.height - coordinates->top - coordinates->bottom;
			else
				(*content)->bounds.height = coordinates->height;
			(*(*content)->dispatch->fitVertically)(content);
			(*content)->flags |= piuYChanged;
		}
		else if ((*content)->flags & piuContentsVerticallyChanged) 
			PiuContainerAdjustVertically(content);
		if ((*content)->flags & piuYChanged)
			(*(*self)->dispatch->placeContentVertically)(self, content);
		content = (*content)->next;
	}
}

void PiuContainerBind(void* it, PiuApplication* application, PiuView* view)
{
	PiuContainer* self = it;
	PiuContent* content = (*self)->first;
	PiuContentBind(it, application, view);
	(*self)->view = view;
	while (content) {
		(*(*content)->dispatch->bind)(content, application, view);
		content = (*content)->next;
	}
}

void PiuContainerBindContent(PiuContainer* self, PiuContent* content)
{
	PiuApplication* application = (*self)->application;
	if (application) {
		PiuView* view = (*self)->view;
		(*(*content)->dispatch->bind)(content, application, view);
	}
}

void PiuContainerCascade(void* it) 
{
	PiuContainer* self = it;
	PiuContent* content = (*self)->first;
	PiuContentCascade(it);
	while (content) {
		(*(*content)->dispatch->cascade)(content);
		content = (*content)->next;
	}
}

xsIntegerValue PiuContainerCount(PiuContainer* self)
{
	PiuContent* content = (*self)->first;
	xsIntegerValue result = 0;;
	while (content) {
		result++;
		content = (*content)->next;
	}
	return result;
}

void PiuContainerDictionary(xsMachine* the, void* it) 
{
	PiuContainer* self = it;
	xsBooleanValue boolean;
	if (xsFindBoolean(xsArg(1), xsID_clip, &boolean)) {
		if (boolean)
			(*self)->flags |= piuClip;
		else
			(*self)->flags &= ~piuClip;
	}
	if (xsFindResult(xsArg(1), xsID_contents)) {
		(void)xsCall1(xsThis, xsID__recurse, xsResult);
	}
}

void PiuContainerFitHorizontally(void* it) 
{
	PiuContainer* self = it;
	PiuContent* content = (*self)->first;
	PiuDimension width;
	PiuContentFitHorizontally(it);
	width = (*self)->bounds.width;
	while (content) {
		PiuCoordinates coordinates = &(*content)->coordinates;
		if ((coordinates->horizontal & piuLeftRight) == piuLeftRight)
			(*content)->bounds.width = width - coordinates->left - coordinates->right;
		else
			(*content)->bounds.width = coordinates->width;
		(*(*content)->dispatch->fitHorizontally)(content);
		(*(*self)->dispatch->placeContentHorizontally)(self, content);
		content = (*content)->next;
	}
	(*self)->flags |= piuContentsPlaced;
}

void PiuContainerFitVertically(void* it) 
{
	PiuContainer* self = it;
	PiuContent* content = (*self)->first;
	PiuDimension height;
	PiuContentFitVertically(it);
	height = (*self)->bounds.height;
	while (content) {
		PiuCoordinates coordinates = &((*content)->coordinates);
		if ((coordinates->vertical & piuTopBottom) == piuTopBottom)
			(*content)->bounds.height = height - coordinates->top - coordinates->bottom;
		else
			(*content)->bounds.height = coordinates->height;
		(*(*content)->dispatch->fitVertically)(content);
		(*(*self)->dispatch->placeContentVertically)(self, content);
		content = (*content)->next;
	}
	(*self)->flags |= piuContentsPlaced;
}

void* PiuContainerHit(void* it, PiuCoordinate x, PiuCoordinate y)
{
	PiuContainer* self = it;
	PiuContent* content;
	PiuContent* result;
	if ((*self)->flags & piuVisible) {
		PiuBoolean hit = ((0 <= x) && (0 <= y) && (x < (*self)->bounds.width) && (y < (*self)->bounds.height));
		if (!hit && ((*self)->flags & piuClip))
			return NULL;
		content = (*self)->last;
		while (content) {
			result = (*(*content)->dispatch->hit)(content, x - (*content)->bounds.x, y - (*content)->bounds.y);
			if (result)
				return result;
			content = (*content)->previous;
		}
		if (hit && ((*self)->flags & piuActive))
			return (PiuContent*)self;
	}
	return NULL;
}

void PiuContainerInvalidate(void* it, PiuRectangle area) 
{
	if (area) {
		PiuContainer* self = it;
		if ((*self)->flags & piuVisible) {
			PiuContainer* container = (*self)->container;
			if (container && !PiuRectangleIsEmpty(area)) {
				PiuRectangleOffset(area, (*self)->bounds.x, (*self)->bounds.y);
				if ((*self)->flags & piuClip)
					PiuRectangleIntersect(area, &(*self)->bounds, area);
				(*(*container)->dispatch->invalidate)(container, area);
			}
		}
	}
	else {
		PiuContentInvalidate(it, area);
	}
}

void PiuContainerMark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	PiuContainer self = it;
	PiuContent* content = self->first;
	PiuContentMark(the, it, markRoot);
	while (content) {
		PiuMarkHandle(the, content);
		content = (*content)->next;
	}
	PiuMarkHandle(the, self->transition);
}

void PiuContainerMeasureHorizontally(void* it) 
{
	PiuContainer* self = it;
	PiuContent* content = (*self)->first;
	PiuDimension max = 0, width;
	PiuCoordinates coordinates;
	PiuContentMeasureHorizontally(self);
	max = (*self)->coordinates.width;
	while (content) {
		(*(*content)->dispatch->measureHorizontally)(content);
		coordinates = &((*content)->coordinates);
		width = coordinates->width + coordinates->left + coordinates->right;
		if (max < width)
			max = width;
		content = (*content)->next;
	}
	coordinates = &((*self)->coordinates);
	if (!(coordinates->horizontal & piuWidth))
		coordinates->width = max;
}

void PiuContainerMeasureVertically(void* it) 
{
	PiuContainer* self = it;
	PiuContent* content = (*self)->first;
	PiuCoordinates coordinates;
	PiuDimension max = 0, height;
	PiuContentMeasureVertically(self);
	max = (*self)->coordinates.height;
	while (content) {
		(*(*content)->dispatch->measureVertically)(content);
		coordinates = &((*content)->coordinates);
		height = coordinates->height + coordinates->top + coordinates->bottom;
		if (max < height)
			max = height;
		content = (*content)->next;
	}
	coordinates = &((*self)->coordinates);
	if (!(coordinates->vertical & piuHeight))
		coordinates->height = max;
}

void PiuContainerPlace(void* it) 
{
	PiuContainer* self = it;
	PiuContent* content = (*self)->first;
	if ((*self)->flags & piuContentsPlaced) {
		(*self)->flags &= ~piuContentsPlaced;
		while (content) {
			(*(*content)->dispatch->place)(content);
			content = (*content)->next;
		}
	}
	PiuContentPlace(it);
}

void PiuContainerPlaceContentHorizontally(void* it, PiuContent* content) 
{
	PiuContainer* self = it;
	PiuRectangle bounds =  &((*content)->bounds);
	PiuCoordinates coordinates = &((*content)->coordinates);
	PiuAlignment horizontal = coordinates->horizontal;
	PiuDimension width = (*self)->bounds.width;
	if (horizontal & piuLeft)
		bounds->x = coordinates->left;
	else if (horizontal & piuRight)
		bounds->x = width - bounds->width - coordinates->right;
	else
#ifdef piuPC
        bounds->x = (PiuCoordinate)round((width - bounds->width) / 2);
#else
		bounds->x = ((width - bounds->width + 1) >> 1);
#endif
	(*content)->flags &= ~piuXChanged;
	(*content)->flags |= piuPlaced;
}

void PiuContainerPlaceContentVertically(void* it, PiuContent* content) 
{
	PiuContainer* self = it;
	PiuRectangle bounds =  &((*content)->bounds);
	PiuCoordinates coordinates = &((*content)->coordinates);
	PiuAlignment vertical = coordinates->vertical;
	PiuDimension height = (*self)->bounds.height;
	if (vertical & piuTop)
		bounds->y = coordinates->top;
	else if (vertical & piuBottom)
		bounds->y = height - bounds->height - coordinates->bottom;
	else
#ifdef piuPC
        bounds->y = (PiuCoordinate)round((height - bounds->height) / 2);
#else
        bounds->y = ((height - bounds->height + 1) >> 1);
#endif
	(*content)->flags &= ~piuYChanged;
	(*content)->flags |= piuPlaced;
}

void PiuContainerReflow(void* it, PiuFlags flags)
{
	PiuContainer* self = it;
	PiuContainer* container = (*self)->container;
	if (flags & piuHorizontallyChanged) {
		if (((*self)->coordinates.horizontal & piuLeftRightWidth) < piuLeftRight)
			(*self)->flags |= piuWidthChanged;
		else
			(*self)->flags |= piuContentsHorizontallyChanged;
	}
	else if (flags & piuContentsHorizontallyChanged)
		(*self)->flags |= piuContentsHorizontallyChanged;
	if (flags & piuVerticallyChanged) {
		if (((*self)->coordinates.vertical & piuTopBottomHeight) < piuTopBottom)
			(*self)->flags |= piuHeightChanged;
		else
			(*self)->flags |= piuContentsVerticallyChanged;
	}
	else if (flags & piuContentsVerticallyChanged)
		(*self)->flags |= piuContentsVerticallyChanged;
	(*self)->flags |= piuContentsPlaced;
	if (container)
		(*(*container)->dispatch->reflow)(container, (*self)->flags);
}

void PiuContainerShowing(void* it, PiuBoolean showIt)
{
	PiuContainer* self = it;
	PiuContent* content = (*self)->first;
	PiuContentShowing(it, showIt);
	while (content) {
		(*(*content)->dispatch->showing)(content, showIt);
		content = (*content)->next;
	}
}

void PiuContainerShown(void* it, PiuBoolean showIt)
{
	PiuContainer* self = it;
	PiuContent* content = (*self)->first;
	PiuContentShown(it, showIt);
	while (content) {
		(*(*content)->dispatch->shown)(content, showIt);
		content = (*content)->next;
	}
}

void PiuContainerUnbind(void* it, PiuApplication* application, PiuView* view)
{
	PiuContainer* self = it;
	PiuContent* content = (*self)->last;

	PiuTransition* transition = (*self)->transition;
	if (transition)
		PiuTransitionComplete(transition, self);

	while (content) {
		(*(*content)->dispatch->unbind)(content, application, view);
		content = (*content)->previous;
	}
	(*self)->view = NULL;
	PiuContentUnbind(it, application, view);
}

void PiuContainerUnbindContent(PiuContainer* self, PiuContent* content)
{
	PiuApplication* application = (*self)->application;
	if (application) {
		PiuView* view = (*self)->view;
		(*(*content)->dispatch->unbind)(content, application, view);
	}
}

void PiuContainerUpdate(void* it, PiuView* view, PiuRectangle area) 
{
	PiuContainer* self = it;
	if ((*self)->flags & piuVisible) {
		PiuRectangle bounds = &((*self)->bounds);
		PiuBoolean clip = ((*self)->flags & piuClip) ? 1 : 0;
		PiuBoolean intersect;
		PiuRectangleRecord intersection;
		if (clip)
			intersect = PiuRectangleIntersect(&intersection, bounds, area);
		else
			intersect = PiuRectangleIntersects(bounds, area);
		if (!clip || intersect) {
			PiuContent* content = (*self)->first;
			PiuCoordinate x = bounds->x;
			PiuCoordinate y = bounds->y;
  			PiuRectangle temporary;
			PiuViewPushOrigin(view, x, y);
			PiuRectangleOffset(area, -x, -y);
			if (intersect)
				(*(*self)->dispatch->draw)(self, view, area);
			if (clip) {
				PiuViewPushClip(view, 0, 0, bounds->width, bounds->height);
				temporary = &intersection;
				PiuRectangleOffset(temporary, -x, -y);
			}
			else
				temporary = area;
			while (content) {
				(*(*content)->dispatch->update)(content, view, temporary);
				content = (*content)->next;
			}
			if (clip)
				PiuViewPopClip(view);
			PiuRectangleOffset(area, x, y);
			PiuViewPopOrigin(view);
		}
	}
// 	if ((*self)->flags & piuVisible) {
// 		PiuRectangle bounds = &((*self)->bounds);
// 		if (PiuRectangleIntersects(bounds, area)) {
// 			PiuCoordinate x = bounds->x;
// 			PiuCoordinate y = bounds->y;
// 			PiuContent* content = (*self)->first;
//  			PiuViewPushOrigin(view, x, y);
// 			PiuRectangleOffset(area, -x, -y);
// 			if ((*self)->flags & piuClip)
// 				PiuViewPushClip(view, 0, 0, bounds->width, bounds->height);
// 			(*(*self)->dispatch->draw)(self, view, area);
// 			while (content) {
// 				(*(*content)->dispatch->update)(content, view, area);
// 				content = (*content)->next;
// 			}
// 			if ((*self)->flags & piuClip)
// 				PiuViewPopClip(view);
// 			PiuRectangleOffset(area, x, y);
// 			PiuViewPopOrigin(view);
// 		}
// 		else if (!((*self)->flags & piuClip)) {
// 			PiuCoordinate x = bounds->x;
// 			PiuCoordinate y = bounds->y;
// 			PiuContent* content = (*self)->first;
//  			PiuViewPushOrigin(view, x, y);
// 			PiuRectangleOffset(area, -x, -y);
// 			while (content) {
// 				(*(*content)->dispatch->update)(content, view, area);
// 				content = (*content)->next;
// 			}
// 			PiuRectangleOffset(area, x, y);
// 			PiuViewPopOrigin(view);
// 		}
// 	}
}

void PiuContainer_create(xsMachine* the)
{
	PiuContainer* self;
	xsVars(4);
	xsSetHostChunk(xsThis, NULL, sizeof(PiuContainerRecord));
	self = PIU(Container, xsThis);
	(*self)->the = the;
	(*self)->reference = xsToReference(xsThis);
	xsSetHostHooks(xsThis, (xsHostHooks*)&PiuContainerHooks);
	(*self)->dispatch = (PiuDispatch)&PiuContainerDispatchRecord;
	(*self)->flags = piuVisible | piuContainer;
	PiuContentDictionary(the, self);
	PiuContainerDictionary(the, self);
	PiuBehaviorOnCreate(self);
}

void PiuContainer_get_clip(xsMachine *the)
{
	PiuContainer* self = PIU(Container, xsThis);
	xsResult = ((*self)->flags & piuClip) ? xsTrue : xsFalse;
}

void PiuContainer_get_first(xsMachine *the)
{
	PiuContainer* self = PIU(Container, xsThis);
	PiuContent* result = (*self)->first;
	if (result)
		xsResult = xsReference((*result)->reference);
}

void PiuContainer_get_last(xsMachine *the)
{
	PiuContainer* self = PIU(Container, xsThis);
	PiuContent* result = (*self)->last;
	if (result)
		xsResult = xsReference((*result)->reference);
}

void PiuContainer_get_length(xsMachine *the)
{
	PiuContainer* self = PIU(Container, xsThis);
	xsResult = xsInteger(PiuContainerCount(self));
}

void PiuContainer_get_transitioning(xsMachine *the)
{
	PiuContainer* self = PIU(Container, xsThis);
	xsResult = (*self)->transition ? xsTrue : xsFalse;
}

void PiuContainer_set_clip(xsMachine *the)
{
	PiuContainer* self = PIU(Container, xsThis);
	PiuContentInvalidate(self, NULL);
	if (xsTest(xsArg(0)))
		(*self)->flags |= piuClip;
	else
		(*self)->flags &= ~piuClip;
	PiuContentInvalidate(self, NULL);
}

void PiuContainer_add(xsMachine* the)
{
	PiuContainer* self = PIU(Container, xsThis);
	PiuContent* content = PIU(Content, xsArg(0));
	xsAssert(self != NULL);
	xsAssert(content != NULL);
	xsAssert((*content)->container == NULL);
	xsAssert((*content)->previous == NULL);
	xsAssert((*content)->next == NULL);
	if ((*self)->first) {
		(*content)->previous = (*self)->last;
		(*((*self)->last))->next = content;
	}
	else
		(*self)->first = content;
	(*self)->last = content;
	(*content)->container = self;
	PiuContainerBindContent(self, content);
	PiuContentReflow(content, piuSizeChanged);
}

void PiuContainer_content(xsMachine *the)
{
	PiuContainer* self = PIU(Container, xsThis);
	xsIntegerValue c = xsToInteger(xsArgc);
	PiuContent* content = NULL;
	xsAssert(self != NULL);
	if (self && (c > 0)) {
		xsNumberValue number = xsToNumber(xsArg(0));
		if (!c_isnan(number)) {
			xsIntegerValue at = (xsIntegerValue)number;
			xsIntegerValue index = 0;
			content = (*self)->first;
			while (content) {
				if (at == index)
					break;
				index++;	
				content = (*content)->next;
			}
		}
		else {
			xsStringValue string = xsToString(xsArg(0));
			content = (*self)->first;
			while (content) {
				if ((*content)->name) {
					xsStringValue name = PiuToString((*content)->name);
					if (!c_strcmp(name, string))
						break;
				}
				content = (*content)->next;
			}
		}
	}
	if (content)
		xsResult = xsReference((*content)->reference);
}

void PiuContainer_empty(xsMachine *the)
{
	PiuContainer* self = PIU(Container, xsThis);
	xsIntegerValue c = xsToInteger(xsArgc);
	xsIntegerValue start = 0;
	xsIntegerValue stop = 0;
	xsIntegerValue length;
	xsAssert(self != NULL);
	if ((c > 0) && xsTest(xsArg(0)))
		start = xsToInteger(xsArg(0));
	if ((c > 1) && xsTest(xsArg(1)))
		stop = xsToInteger(xsArg(1));
	length = PiuContainerCount(self);
	if (start < 0) {
		start += length;
		if (start < 0)
			start = 0;
	}
	else if (start > length)
		start = length;
	if (stop <= 0) {
		stop += length;
		if (stop < 0)
			stop = 0;
	}
	else if (stop > length)
		stop = length;
	if (start < stop) {
		PiuContent* content = (*self)->first;
		PiuContent* previous = NULL;
		PiuContent* next = NULL;
		xsIntegerValue index = 0;
		while (index < start) {
			content = (*content)->next;
			index++;
		}
		while (index < stop) {
			PiuContentReflow(content, piuSizeChanged);
			PiuContentInvalidate(content, NULL);
			PiuContainerUnbindContent(self, content);
			content = (*content)->next;
			index++;
		}
		content = (*self)->first;
		index = 0;
		while (index < start) {
			previous = content;
			content = (*content)->next;
			index++;
		}
		while (index < stop) {
			next = (*content)->next;
			(*content)->container = NULL;
			(*content)->next = NULL;
			(*content)->previous = NULL;
			content = next;
			index++;
		}
		if (previous)
			(*previous)->next = next;
		else
			(*self)->first = next;
		if (next)
			(*next)->previous = previous;
		else
			(*self)->last = previous;
	}
}

void PiuContainer_firstThat(xsMachine *the)
{
	PiuContainer* self = PIU(Container, xsThis);
	xsVars(2);
	xsIdentifier id = xsToID(xsArg(0));
	xsIntegerValue c = xsToInteger(xsArgc);
	PiuContent* content;
	xsAssert(self != NULL);
	content = (*self)->first;
	while (content) {
		PiuContent_delegateAux(the, content, id, c);
		if (xsTest(xsResult)) {
			xsResult = xsVar(1);
			return;
		}
		content = (*content)->next;
	}
}

void PiuContainer_insert(xsMachine *the)
{
	PiuContainer* self = PIU(Container, xsThis);
	PiuContent* content = PIU(Content, xsArg(0));
	PiuContent* before = PIU(Content, xsArg(1));
	PiuContent* previous;
	xsAssert(self != NULL);
	xsAssert(content != NULL);
	xsAssert((*content)->container == NULL);
	xsAssert((*content)->previous == NULL);
	xsAssert((*content)->next == NULL);
	xsAssert(before != NULL);
	xsAssert((*before)->container == self);
	previous = (*before)->previous;
	(*before)->previous = content;
	(*content)->next = before;
	(*content)->previous = previous;
	if (previous)
		(*previous)->next = content;
	else
		(*self)->first = content;
	(*content)->container = self;
	PiuContainerBindContent(self, content);
	PiuContentReflow(content, piuSizeChanged);
}

void PiuContainer_lastThat(xsMachine *the)
{
	PiuContainer* self = PIU(Container, xsThis);
	xsVars(2);
	xsIdentifier id = xsToID(xsArg(0));
	xsIntegerValue c = xsToInteger(xsArgc);
	PiuContent* content;
	xsAssert(self != NULL);
	content = (*self)->last;
	while (content) {
		PiuContent_delegateAux(the, content, id, c);
		if (xsTest(xsResult)) {
			xsResult = xsVar(1);
			return;
		}
		content = (*content)->previous;
	}
}

void PiuContainer_remove(xsMachine *the)
{
	PiuContainer* self = PIU(Container, xsThis);
	PiuContent* content = PIU(Content, xsArg(0));
	PiuContent* previous;
	PiuContent* next;
	xsAssert(self != NULL);
	xsAssert(content != NULL);
	xsAssert((*content)->container == self);
	PiuContentReflow(content, piuSizeChanged);
	PiuContentInvalidate(content, NULL);
	PiuContainerUnbindContent(self, content);
	previous = (*content)->previous;
	next = (*content)->next;
	if (previous) {
		(*content)->previous = NULL;
		(*previous)->next = next;
	}
	else
		(*self)->first = next;
	if (next) {
		(*content)->next = NULL;
		(*next)->previous = previous;
	}
	else
		(*self)->last = previous;
	(*content)->container = NULL;
}

void PiuContainer_replace(xsMachine *the)
{
	PiuContainer* self = PIU(Container, xsThis);
	PiuContent* content = PIU(Content, xsArg(0));
	PiuContent* by = PIU(Content, xsArg(1));
	PiuContent* previous;
	PiuContent* next;
	xsAssert(self != NULL);
	xsAssert(content != NULL);
	xsAssert((*content)->container == self);
	xsAssert(by != NULL);
	xsAssert((*by)->container == NULL);
	xsAssert((*by)->previous == NULL);
	xsAssert((*by)->next == NULL);
	PiuContentReflow(content, piuSizeChanged);
	PiuContentInvalidate(content, NULL);
	PiuContainerUnbindContent(self, content);
	previous = (*content)->previous;
	next = (*content)->next;
	if (previous) {
		(*content)->previous = NULL;
		(*previous)->next = by;
	}
	else
		(*self)->first = by;
	if (next) {
		(*content)->next = NULL;
		(*next)->previous = by;
	}
	else
		(*self)->last = by;
	(*content)->container = NULL;
	(*by)->container = self;
	(*by)->previous = previous;
	(*by)->next = next;
	PiuContainerBindContent(self, by);
	PiuContentReflow(by, piuSizeChanged);
}

void PiuContainer_run(xsMachine *the)
{
	xsIntegerValue c = xsToInteger(xsArgc), i;
	PiuContainer* self = PIU(Container, xsThis);
	if ((*self)->application) {
		if (xsTest(xsArg(0))) {
			PiuTransition* transition = PIU(Transition, xsArg(0));
			xsAssert((*transition)->container == NULL);
			xsResult = xsNew1(xsGlobal, xsID_Array, xsNumber(c));
			(void)xsCall0(xsResult, xsID_fill);
			xsSetAt(xsResult, xsInteger(0), xsThis);
			for (i = 1; i < c; i++)
				xsSetAt(xsResult, xsInteger(i), xsArg(i));
			xsSet(xsArg(0), xsID_parameters, xsResult);
			PiuTransitionRun(transition, self);
		}
		else {
			PiuTransition* transition = (*self)->transition;
			if (transition)
				PiuTransitionComplete(transition, self);
		}
	}
}

void PiuContainer_swap(xsMachine *the)
{
	PiuContainer* self = PIU(Container, xsThis);
	PiuContent* content0 = PIU(Content, xsArg(0));
	PiuContent* previous0;
	PiuContent* next0;
	PiuContent* content1 = PIU(Content, xsArg(1));
	PiuContent* previous1;
	PiuContent* next1;
	xsAssert(self != NULL);
	xsAssert(content0 != NULL);
	xsAssert((*content0)->container == self);
	xsAssert(content1 != NULL);
	xsAssert((*content1)->container == self);
	PiuContentInvalidate(content0, NULL);
	PiuContentInvalidate(content1, NULL);
	previous0 = (*content0)->previous;
	next0 = (*content0)->next;
	previous1 = (*content1)->previous;
	next1 = (*content1)->next;
	if (!previous0) {
		(*self)->first = content1;
		(*content1)->previous = NULL;
	}
	else if (previous0 != content1) {
		(*previous0)->next = content1;
		(*content1)->previous = previous0;
	}
	else
		(*content1)->previous = content0;
	if (!next0)	{
		(*self)->last = content1;
		(*content1)->next = NULL;
	}
	else if (next0 != content1)	{
		(*next0)->previous = content1;
		(*content1)->next = next0;
	}
	else
		(*content1)->next = content0;
	if (!previous1)	{
		(*self)->first = content0;
		(*content0)->previous = NULL;
	}
	else if (previous1 != content0)	{
		(*previous1)->next = content0;
		(*content0)->previous = previous1;
	}
	else
		(*content0)->previous = content1;
	if (!next1)	{
		(*self)->last = content0;
		(*content0)->next = NULL;
	}
	else if (next1 != content0)	{
		(*next1)->previous = content0;
		(*content0)->next = next1;
	}
	else
		(*content0)->next = content1;
	PiuContentReflow(content0, piuOrderChanged);
	PiuContentReflow(content1, piuOrderChanged);
}
