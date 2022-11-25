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

static PiuCoordinate PiuScrollerConstraintHorizontally(PiuScroller* self);
static PiuCoordinate PiuScrollerConstraintVertically(PiuScroller* self);
static void PiuScrollerDictionary(xsMachine* the, void* it);
static void PiuScrollerFitHorizontally(void* it);
static void PiuScrollerFitVertically(void* it);
static void* PiuScrollerHit(void* it, PiuCoordinate x, PiuCoordinate y);
static void PiuScrollerInvalidate(void* it, PiuRectangle area);
static void PiuScrollerPlace(void* it);
static void PiuScrollerPlaceContentHorizontally(void* it, PiuContent* content);
static void PiuScrollerPlaceContentVertically(void* it, PiuContent* content);
static void PiuScrollerMeasureHorizontally(void* it);
static void PiuScrollerMeasureVertically(void* it);
static void PiuScrollerReflow(void* it, PiuFlags flags);
static void PiuScrollerReveal(PiuScroller* self, PiuRectangle bounds, xsBooleanValue centerVertically, xsBooleanValue centerHorizontally);
static void PiuScrollerScrollBy(PiuScroller* self, PiuCoordinate dx, PiuCoordinate dy);
static void PiuScrollerUpdate(void* it, PiuView* view, PiuRectangle area);

const PiuDispatchRecord ICACHE_FLASH_ATTR PiuScrollerDispatchRecord = {
	"Scroller",
	PiuContainerBind,
	PiuContainerCascade,
	PiuContentDraw,
	PiuScrollerFitHorizontally,
	PiuScrollerFitVertically,
	PiuScrollerHit,
	PiuContentIdle,
	PiuScrollerInvalidate,
	PiuScrollerMeasureHorizontally,
	PiuScrollerMeasureVertically,
	PiuScrollerPlace,
	PiuScrollerPlaceContentHorizontally,
	PiuScrollerPlaceContentVertically,
	PiuScrollerReflow,
	PiuContainerShowing,
	PiuContainerShown,
	PiuContentSync,
	PiuContainerUnbind,
	PiuScrollerUpdate
};

const xsHostHooks ICACHE_FLASH_ATTR PiuScrollerHooks = {
	PiuContentDelete,
	PiuContainerMark,
	NULL
};

PiuCoordinate PiuScrollerConstraintHorizontally(PiuScroller* self) 
{
	PiuContent* content = (*self)->first;
	if (content) {
		PiuAlignment horizontal = (*content)->coordinates.horizontal;
		PiuCoordinate offset = (*self)->delta.x;
		PiuCoordinate range = (*content)->bounds.width;
		PiuCoordinate size = (*self)->bounds.width;
		if (size < range) {
			PiuCoordinate c, d;
			if ((*self)->flags & piuLooping) {
				c = 0;
				d = c - offset;
#ifdef piuPC
				d = (PiuCoordinate)c_fmod(d, range);
#else
				d %= range;
#endif
				if (d > 0)
					d -= range;
			}
			else {
				PiuCoordinate min = size - range;
				PiuCoordinate max = 0;
				if (horizontal & piuLeft)
					c = 0;
				else if (horizontal & piuRight)
					c = size - range;
				else
#ifdef piuPC
					c = (size - range + 1) / 2;
#else
					c = (size - range + 1) >> 1;
#endif
				d = c - offset;
				if (d < min)
					d = min;
				if (d > max)
					d = max;
			}
			return (PiuCoordinate)(c - d);
		}
	}
	return 0;
}

PiuCoordinate PiuScrollerConstraintVertically(PiuScroller* self) 
{
	PiuContent* content = (*self)->first;
	if (content) {
		PiuAlignment vertical = (*content)->coordinates.vertical;
		PiuCoordinate offset = (*self)->delta.y;
		PiuCoordinate range = (*content)->bounds.height;
		PiuCoordinate size = (*self)->bounds.height;
		if (size < range) {
			PiuCoordinate c, d;
			if ((*self)->flags & piuLooping) {
				c = 0;
				d = c - offset;
#ifdef piuPC
				d = (PiuCoordinate)c_fmod(d, range);
#else
				d %= range;
#endif
				if (d > 0)
					d -= range;
			}
			else {
				PiuCoordinate min = size - range;
				PiuCoordinate max = 0;
				if (vertical & piuTop)
					c = 0;
				else if (vertical & piuBottom)
					c = size - range;
				else
#ifdef piuPC
					c = (size - range + 1) / 2;
#else
					c = (size - range + 1) >> 1;
#endif
				d = c - offset;
				if (d < min)
					d = min;
				if (d > max)
					d = max;
			}
			return (PiuCoordinate)(c - d);
		}
	}
	return 0;
}

void PiuScrollerDictionary(xsMachine* the, void* it) 
{
	PiuScroller* self = it;
	xsBooleanValue boolean;
	if (xsFindBoolean(xsArg(1), xsID_looping, &boolean)) {
		if (boolean)
			(*self)->flags |= piuLooping;
		else
			(*self)->flags &= ~piuLooping;
	}
}

void PiuScrollerFitHorizontally(void* it) 
{
	PiuScroller* self = it;
	PiuContent* content = (*self)->first;
	if (content) {
		PiuContainerFitHorizontally(it);
		(*self)->flags |= piuXScrolled;
	}
}

void PiuScrollerFitVertically(void* it) 
{
	PiuScroller* self = it;
	PiuContent* content = (*self)->first;
	if (content) {
		PiuContainerFitVertically(it);
		(*self)->flags |= piuYScrolled;
	}
}

void* PiuScrollerHit(void* it, PiuCoordinate x, PiuCoordinate y)
{
	PiuScroller* self = it;
	PiuContent* content;
	PiuContent* result;
	if ((*self)->flags & piuVisible) {
		PiuBoolean hit = ((0 <= x) && (0 <= y) && (x < (*self)->bounds.width) && (y < (*self)->bounds.height));
		content = (*self)->last;
		if (content) {
			while ((*content)->previous) {
				result = (*(*content)->dispatch->hit)(content, x - (*content)->bounds.x, y - (*content)->bounds.y);
				if (result)
					return result;
				content = (*content)->previous;
			}
			if (!hit && ((*self)->flags & piuClip))
				return NULL;
			result = (*(*content)->dispatch->hit)(content, x - (*content)->bounds.x, y - (*content)->bounds.y);
			if (result)
				return result;
		}
		if (hit && ((*self)->flags & piuActive))
			return (PiuContent*)self;
	}
	return NULL;
}

void PiuScrollerInvalidate(void* it, PiuRectangle area) 
{
	if (area) {
		PiuScroller* self = it;
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

void PiuScrollerMeasureHorizontally(void* it) 
{
	PiuScroller* self = it;
	PiuContent* content = (*self)->first;
	PiuCoordinates coordinates;
	if (content) {
		PiuContainerMeasureHorizontally(it);
		coordinates = &((*self)->coordinates);
		if (!(coordinates->horizontal & piuWidth)) 
			coordinates->width = (*content)->coordinates.width;
	}
}

void PiuScrollerMeasureVertically(void* it) 
{
	PiuScroller* self = it;
	PiuContent* content = (*self)->first;
	PiuCoordinates coordinates;
	if (content) {
		PiuContainerMeasureVertically(it);
		coordinates = &((*self)->coordinates);
		if (!(coordinates->vertical & piuHeight))
			coordinates->height = (*content)->coordinates.height;
	}
}

void PiuScrollerPlace(void* it) 
{
	PiuContainer* self = it;
	PiuContent* content = (*self)->first;
	PiuContainerPlace(it);
	if ((*self)->flags & piuScrolled) {
		(*self)->flags &= ~piuScrolled;
		if (content) {
			PiuBehaviorOnScrolled(self);
			while (content) {
				PiuBehaviorOnScrolled(content);
				content = (*content)->next;
			}
		}
	}
}

void PiuScrollerPlaceContentHorizontally(void* it, PiuContent* content) 
{
	PiuScroller* self = it;
	PiuContainerPlaceContentHorizontally(it, content);
	if ((*self)->first == content) {
		if (!((*self)->flags & piuTracking) || ((*self)->flags & piuLooping))
			(*self)->delta.x = PiuScrollerConstraintHorizontally(self);
#ifdef piuPC
		(*content)->bounds.x -= (PiuCoordinate)round((*self)->delta.x);
#else
		(*content)->bounds.x -= (*self)->delta.x;
#endif
	}
}

void PiuScrollerPlaceContentVertically(void* it, PiuContent* content) 
{
	PiuScroller* self = it;
	PiuContainerPlaceContentVertically(it, content);
	if ((*self)->first == content) {
		if (!((*self)->flags & piuTracking) || ((*self)->flags & piuLooping))
			(*self)->delta.y = PiuScrollerConstraintVertically(self);
#ifdef piuPC
		(*content)->bounds.y -= (PiuCoordinate)round((*self)->delta.y);
#else
		(*content)->bounds.y -= (*self)->delta.y;
#endif
	}
}

void PiuScrollerReflow(void* it, PiuFlags flags)
{
	PiuScroller* self = it;
	PiuContent* content = (*self)->first;
	if (content) {
		if (flags & piuHorizontallyChanged)
			(*self)->flags |= piuXScrolled;
		if (flags & piuVerticallyChanged)
			(*self)->flags |= piuYScrolled;
	}
	PiuContainerReflow(it, flags);
}

void PiuScrollerReveal(PiuScroller* self, PiuRectangle bounds, xsBooleanValue centerVertically, xsBooleanValue centerHorizontally) 
{
	PiuCoordinate start, stop, min, max, x, y;
	start = bounds->y - (*self)->delta.y;
	stop = start + bounds->height;
	min = 0;
	max = (*self)->bounds.height;
	if (centerVertically) {
		if ((stop > max) || (start < min))
			y = (start + stop - max) / 2;
		else
			y = 0;
	}
	else {
		if (stop > max) {
			if ((start - (stop - max)) < min)
				y = start - min;
			else
				y = stop - max;
		}
		else if (start < min)
			y = start - min;
		else
			y = 0;
	}
	start = bounds->x - (*self)->delta.x;
	stop = start + bounds->width;
	min = 0;
	max = (*self)->bounds.width;
	if (centerHorizontally) {
		if ((stop > max) || (start < min))
			x = (start + stop - max) / 2;
		else
			x = 0;
	}
	else {
		if (stop > max) {
			if ((start - (stop - max)) < min)
				x = start - min;
			else
				x = stop - max;
		}
		else if (start < min)
			x = start - min;
		else
			x = 0;
	}
	if (x || y)
		PiuScrollerScrollBy(self, x, y);
}

void PiuScrollerScrollBy(PiuScroller* self, PiuCoordinate dx, PiuCoordinate dy) 
{
	PiuContent* content = (*self)->first;
	if (content) {
		PiuFlags flags = 0;
		if (((*content)->coordinates.horizontal & piuLeftRightWidth) == piuLeftRight) dx = 0;
		if (((*content)->coordinates.vertical & piuTopBottomHeight) == piuTopBottom) dy = 0;
		if (dx || dy) {
			if ((*self)->flags & (piuClip | piuLooping))
				PiuContentInvalidate(self, NULL);
			else
				PiuContentInvalidate(content, NULL);
			if (dx) {
				flags |= piuXChanged;
				(*self)->delta.x += dx;
			}
			if (dy) {
				flags |= piuYChanged;
				(*self)->delta.y += dy;
			}
			if (flags)
				PiuContentReflow(content, flags);
		}
	}
}

void PiuScrollerUpdate(void* it, PiuView* view, PiuRectangle area) 
{
	PiuScroller* self = it;
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
			if (content) {
				(*(*content)->dispatch->update)(content, view, temporary);
				if ((*self)->flags & piuLooping) {
					PiuRectangle loop;
					bounds = &((*self)->bounds);
					loop = &((*content)->bounds);
					if (loop->width > bounds->width)
						loop->x += loop->width;
					if (loop->height > bounds->height)
						loop->y += loop->height;
					(*(*content)->dispatch->update)(content, view, temporary);
					bounds = &((*self)->bounds);
					loop = &((*content)->bounds);
					if (loop->width > bounds->width)
						loop->x -= loop->width;
					if (loop->height > bounds->height)
						loop->y -= loop->height;
				}
				content = (*content)->next;
				while (content) {
					(*(*content)->dispatch->update)(content, view, temporary);
					content = (*content)->next;
				}
			}
			if (clip)
				PiuViewPopClip(view);
			PiuRectangleOffset(area, x, y);
			PiuViewPopOrigin(view);
		}
	}
}

void PiuScroller_create(xsMachine* the)
{
	PiuScroller* self;
	xsVars(4);
	xsSetHostChunk(xsThis, NULL, sizeof(PiuScrollerRecord));
	self = PIU(Scroller, xsThis);
	(*self)->the = the;
	(*self)->reference = xsToReference(xsThis);
	xsSetHostHooks(xsThis, (xsHostHooks*)&PiuScrollerHooks);
	(*self)->dispatch = (PiuDispatch)&PiuScrollerDispatchRecord;
	(*self)->flags = piuVisible | piuContainer;
	PiuContentDictionary(the, self);
	PiuContainerDictionary(the, self);
	PiuScrollerDictionary(the, self);
	PiuBehaviorOnCreate(self);
}

void PiuScroller_get_constraint(xsMachine* the)
{
	PiuScroller* self = PIU(Scroller, xsThis);
	PiuCoordinate x = PiuScrollerConstraintHorizontally(self);
	PiuCoordinate y = PiuScrollerConstraintVertically(self);
	xsResult = xsNewObject();
	xsDefine(xsResult, xsID_x, xsPiuCoordinate(x), xsDefault);
	xsDefine(xsResult, xsID_y, xsPiuCoordinate(y), xsDefault);
}

void PiuScroller_get_looping(xsMachine* the)
{
	PiuScroller* self = PIU(Scroller, xsThis);
	xsResult = ((*self)->flags & piuLooping) ? xsTrue : xsFalse;
}

void PiuScroller_get_scroll(xsMachine* the)
{
	PiuScroller* self = PIU(Scroller, xsThis);
	PiuCoordinate x = (*self)->delta.x;
	PiuCoordinate y = (*self)->delta.y;
	xsResult = xsNewObject();
	xsDefine(xsResult, xsID_x, xsPiuCoordinate(x), xsDefault);
	xsDefine(xsResult, xsID_y, xsPiuCoordinate(y), xsDefault);
}

void PiuScroller_get_tracking(xsMachine* the)
{
	PiuScroller* self = PIU(Scroller, xsThis);
	xsResult = ((*self)->flags & piuTracking) ? xsTrue : xsFalse;
}

	
void PiuScroller_set_looping(xsMachine* the)
{
	PiuScroller* self = PIU(Scroller, xsThis);
	if (xsTest(xsArg(0)))
		(*self)->flags |= piuLooping;
	else
		(*self)->flags &= ~piuLooping;
}

void PiuScroller_set_scroll(xsMachine* the)
{
	PiuScroller* self = PIU(Scroller, xsThis);
	xsIntegerValue x = 0, y = 0;
	if (xsFindInteger(xsArg(0), xsID_x, &x))
		x -= (*self)->delta.x;
	if (xsFindInteger(xsArg(0), xsID_y, &y))
		y -= (*self)->delta.y;
	PiuScrollerScrollBy(self, (PiuCoordinate)x, (PiuCoordinate)y);
}

void PiuScroller_set_tracking(xsMachine* the)
{
	PiuScroller* self = PIU(Scroller, xsThis);
	if (xsTest(xsArg(0)))
		(*self)->flags |= piuTracking;
	else
		(*self)->flags &= ~piuTracking;
}

	
void PiuScroller_reveal(xsMachine* the)
{
	PiuScroller* self = PIU(Scroller, xsThis);
	xsIntegerValue x, y, width, height;
	if (xsFindInteger(xsArg(0), xsID_x, &x)) {
		if (xsFindInteger(xsArg(0), xsID_y, &y)) { 
			if (xsFindInteger(xsArg(0), xsID_width, &width)) {
				if (xsFindInteger(xsArg(0), xsID_height, &height)) {
					PiuRectangleRecord bounds;
					xsBooleanValue centerVertically = (xsToInteger(xsArgc) > 1) ? xsToBoolean(xsArg(1)) : 0;
					xsBooleanValue centerHorizontally = (xsToInteger(xsArgc) > 2) ? xsToBoolean(xsArg(2)) : 0;
					bounds.x = (PiuCoordinate)x;
					bounds.y = (PiuCoordinate)y;
					bounds.width = (PiuDimension)width;
					bounds.height = (PiuDimension)height;
					PiuScrollerReveal(self, &bounds, centerVertically, centerHorizontally);
				}
			}
		}
	}
}

void PiuScroller_scrollBy(xsMachine* the)
{
	PiuScroller* self = PIU(Scroller, xsThis);
	PiuCoordinate x = xsToPiuCoordinate(xsArg(0));
	PiuCoordinate y = xsToPiuCoordinate(xsArg(1));
	PiuScrollerScrollBy(self, x, y);
}

void PiuScroller_scrollTo(xsMachine* the)
{
	PiuScroller* self = PIU(Scroller, xsThis);
	PiuCoordinate x = xsToPiuCoordinate(xsArg(0));
	PiuCoordinate y = xsToPiuCoordinate(xsArg(1));
	PiuScrollerScrollBy(self, x - (*self)->delta.x, y - (*self)->delta.y);
}








