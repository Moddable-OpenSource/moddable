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

#include "piuMC.h"

static void PiuDieUpdate(void* it, PiuView* view, PiuRectangle area);
static void PiuDieMark(xsMachine* the, void* it, xsMarkRoot markRoot);

const PiuDispatchRecord ICACHE_FLASH_ATTR PiuDieDispatchRecord = {
	"Die",
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
	PiuDieUpdate
};

const xsHostHooks ICACHE_FLASH_ATTR PiuDieHooks = {
	PiuContentDelete,
	PiuDieMark,
	NULL
};

void PiuDieMark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	PiuDie self = it;
	PiuContainerMark(the, it, markRoot);
	PiuMarkHandle(the, self->current);
	PiuMarkHandle(the, self->swap);
	PiuMarkHandle(the, self->work);
}

void PiuDieUpdate(void* it, PiuView* view, PiuRectangle area) 
{
	PiuDie* self = it;
	PiuCoordinate x = (*self)->bounds.x;
	PiuCoordinate y = (*self)->bounds.y;
	PiuRegion* current = (*self)->current;
	PiuCoordinate *regionBegin0, *region0, *regionLimit0;
	int size, offset;
	PiuContent* content;
	PiuViewPushOrigin(view, x, y);
	PiuRectangleOffset(area, -x, -y);
	regionBegin0 = (*current)->data;;
	offset = 5;
	size = *regionBegin0 - 2; // last span
	region0 = regionBegin0 + offset;
	regionLimit0 = regionBegin0 + size;
	while (region0 < regionLimit0) {
		PiuCoordinate top = *region0++;
		PiuCoordinate segmentCount = *region0++;
		PiuCoordinate bottom = *(region0 + segmentCount);		
		while (segmentCount) {
			PiuCoordinate left = *region0++;
			segmentCount--;
			PiuCoordinate right = *region0++;
			segmentCount--;
			offset = region0 - regionBegin0;
			
			PiuViewPushClip(view, left, top, right - left, bottom - top);
			(*(*self)->dispatch->draw)(self, view, area);
			content = (*self)->first;
			while (content) {
				(*(*content)->dispatch->update)(content, view, area);
				content = (*content)->next;
			}
			PiuViewPopClip(view);
			
			regionBegin0 = (*current)->data;;
			region0 = regionBegin0 + offset;
			regionLimit0 = regionBegin0 + size;
		}
	}
	PiuRectangleOffset(area, x, y);
	PiuViewPopOrigin(view);
}

void PiuDie__create(xsMachine* the)
{
	PiuDie* self;
	xsIntegerValue length;
	xsVars(2);
	xsSetHostChunk(xsThis, NULL, sizeof(PiuDieRecord));
	self = PIU(Die, xsThis);
	(*self)->reference = xsToReference(xsThis);
	(*self)->the = the;
	xsSetHostHooks(xsThis, (xsHostHooks*)&PiuDieHooks);
	(*self)->dispatch = (PiuDispatch)&PiuDieDispatchRecord;
	(*self)->flags = piuVisible | piuContainer;
	PiuContentDictionary(the, self);
	PiuContainerDictionary(the, self);
	if (!xsFindInteger(xsArg(1), xsID_regionLength, &length))
		length = 256;
	PiuRegionNew(the, (PiuCoordinate)length);
	(*self)->current = PIU(Region, xsResult);
	PiuRegionNew(the, (PiuCoordinate)length);
	(*self)->swap = PIU(Region, xsResult);
	PiuRegionNew(the, (PiuCoordinate)length);
	(*self)->work = PIU(Region, xsResult);
	PiuBehaviorOnCreate(self);
}

void PiuDie_and(xsMachine* the)
{
    PiuDie* self = PIU(Die, xsThis);
	PiuCoordinate x = xsToPiuCoordinate(xsArg(0));
	PiuCoordinate y = xsToPiuCoordinate(xsArg(1));
    PiuDimension width = xsToPiuDimension(xsArg(2));
    PiuDimension height = xsToPiuDimension(xsArg(3));
    PiuRegionRecord regionRecord;
    PiuRegion region = &regionRecord;
    PiuRegion* tmp = (*self)->work;
    (*self)->work = (*self)->swap;
    (*self)->swap = tmp;
    regionRecord.reference = NULL;
    regionRecord.available = 11;
    PiuRegionRectangle(&region, x, y, width, height);
    if (!PiuRegionCombine((*self)->work, (*self)->swap, &region, piuRegionIntersectionOp))
        xsErrorPrintf("region overflowed");
	xsResult = xsThis;
}

void PiuDie_attach(xsMachine* the)
{
	PiuDie* self = PIU(Die, xsThis);
	PiuContent* content = PIU(Content, xsArg(0));
	PiuApplication* application = (*content)->application;
	PiuContainer* container = (*content)->container;
	PiuContent* previous = (*content)->previous;
	PiuContent* next = (*content)->next;
	PiuApplicationAdjust(application);
		
	if (previous) {
		(*previous)->next = (PiuContent*)self;
		(*content)->previous = NULL;
		(*self)->previous = previous;
	}
	else
		(*container)->first = (PiuContent*)self;
	if (next) {
		(*next)->previous = (PiuContent*)self;
		(*content)->next = NULL;
		(*self)->next = next;
	}
	else
		(*container)->last = (PiuContent*)self;
	(*content)->container = (PiuContainer*)self;
	(*self)->application = application;
	(*self)->container = container;
	(*self)->first = content;
	(*self)->last = content;
	
	(*self)->coordinates.horizontal = (*content)->coordinates.horizontal;
	(*self)->coordinates.left = (*content)->coordinates.left;
	(*self)->coordinates.right = (*content)->coordinates.right;
	(*self)->bounds.x = (*content)->bounds.x;
	(*self)->bounds.width = (*content)->bounds.width;
	(*self)->coordinates.vertical = (*content)->coordinates.vertical;
	(*self)->coordinates.top = (*content)->coordinates.top;
	(*self)->coordinates.bottom = (*content)->coordinates.bottom;
	(*self)->bounds.y = (*content)->bounds.y;
	(*self)->bounds.height = (*content)->bounds.height;
	
	(*content)->coordinates.horizontal = piuLeftRight;
	(*content)->coordinates.left = 0;
	(*content)->coordinates.right = 0;
	(*content)->bounds.x = 0;
	(*content)->coordinates.vertical = piuTopBottom;
	(*content)->coordinates.top = 0;
	(*content)->coordinates.bottom = 0;
	(*content)->bounds.y = 0;
}

void PiuDie_detach(xsMachine* the)
{
	PiuDie* self = PIU(Die, xsThis);
	PiuContent* content = (*self)->first;
	PiuContainer* container = (*self)->container;
	PiuContent* previous = (*self)->previous;
	PiuContent* next = (*self)->next;
	if (previous) {
		(*previous)->next = content;
		(*self)->previous = NULL;
		(*content)->previous = previous;
	}
	else
		(*container)->first = content;
	if (next) {
		(*next)->previous = content;
		(*self)->next = NULL;
		(*content)->next = next;
	}
	else
		(*container)->last = content;
	(*content)->container = container;
	(*self)->application = NULL;
	(*self)->container = NULL;
	(*self)->first = NULL;
	(*self)->last = NULL;
	
	(*content)->coordinates.horizontal = (*self)->coordinates.horizontal;
	(*content)->coordinates.left = (*self)->coordinates.left;
	(*content)->coordinates.right = (*self)->coordinates.right;
	(*content)->bounds.x = (*self)->bounds.x;
	(*content)->coordinates.vertical = (*self)->coordinates.vertical;
	(*content)->coordinates.top = (*self)->coordinates.top;
	(*content)->coordinates.bottom = (*self)->coordinates.bottom;
	(*content)->bounds.y = (*self)->bounds.y;
	
	xsResult = xsReference((*content)->reference);
}

void PiuDie_empty(xsMachine* the)
{
	PiuDie* self = PIU(Die, xsThis);
	if (!PiuRegionEmpty((*self)->work))
		xsErrorPrintf("region overflowed");
	xsResult = xsThis;
}

void PiuDie_cut(xsMachine* the)
{
	PiuDie* self = PIU(Die, xsThis);
	PiuApplication* application = (*self)->application;	
	if (!PiuRegionXOR((*self)->swap, (*self)->work, (*self)->current))
		xsErrorPrintf("region overflowed");
	if (!PiuRegionCopy((*self)->current, (*self)->work))
		xsErrorPrintf("region overflowed");
	if (application) {
		PiuCoordinate x, y;
		PiuContentToApplicationCoordinates(self, 0, 0, &x, &y);
		PiuRegionOffset((*self)->swap, x, y);
		PiuViewInvalidateRegion((*application)->view, (*self)->swap);	
	}
}

void PiuDie_fill(xsMachine* the)
{
	PiuDie* self = PIU(Die, xsThis);
	PiuCoordinate width = (*self)->bounds.width;
	PiuCoordinate height = (*self)->bounds.height;
	if (!PiuRegionRectangle((*self)->work, 0, 0, width, height))
		xsErrorPrintf("region overflowed");
	xsResult = xsThis;
}

void PiuDie_or(xsMachine* the)
{
    PiuDie* self = PIU(Die, xsThis);
	PiuCoordinate x = xsToPiuCoordinate(xsArg(0));
	PiuCoordinate y = xsToPiuCoordinate(xsArg(1));
    PiuDimension width = xsToPiuDimension(xsArg(2));
    PiuDimension height = xsToPiuDimension(xsArg(3));
    PiuRegionRecord regionRecord;
    PiuRegion region = &regionRecord;
    PiuRegion* tmp = (*self)->work;
    (*self)->work = (*self)->swap;
    (*self)->swap = tmp;
    regionRecord.reference = NULL;
    regionRecord.available = 11;
    PiuRegionRectangle(&region, x, y, width, height);
    if (!PiuRegionCombine((*self)->work, (*self)->swap, &region, piuRegionUnionOp))
        xsErrorPrintf("region overflowed");
	xsResult = xsThis;
}

void PiuDie_set(xsMachine* the)
{
	PiuDie* self = PIU(Die, xsThis);
	PiuCoordinate x = xsToPiuCoordinate(xsArg(0));
	PiuCoordinate y = xsToPiuCoordinate(xsArg(1));
    PiuDimension width = xsToPiuDimension(xsArg(2));
    PiuDimension height = xsToPiuDimension(xsArg(3));
	if (!PiuRegionRectangle((*self)->work, x, y, width, height))
		xsErrorPrintf("region overflowed");
	xsResult = xsThis;
}

void PiuDie_sub(xsMachine* the)
{
	PiuDie* self = PIU(Die, xsThis);
	PiuCoordinate x = xsToPiuCoordinate(xsArg(0));
	PiuCoordinate y = xsToPiuCoordinate(xsArg(1));
    PiuDimension width = xsToPiuDimension(xsArg(2));
    PiuDimension height = xsToPiuDimension(xsArg(3));
	PiuRegionRecord regionRecord;
	PiuRegion region = &regionRecord;
	PiuRegion* tmp = (*self)->work;
	(*self)->work = (*self)->swap;
	(*self)->swap = tmp;
	regionRecord.reference = NULL;
	regionRecord.available = 11;
	PiuRegionRectangle(&region, x, y, width, height);
	if (!PiuRegionCombine((*self)->work, (*self)->swap, &region, piuRegionDifferenceOp))
		xsErrorPrintf("region overflowed");
	xsResult = xsThis;
}

void PiuDie_xor(xsMachine* the)
{
    PiuDie* self = PIU(Die, xsThis);
	PiuCoordinate x = xsToPiuCoordinate(xsArg(0));
	PiuCoordinate y = xsToPiuCoordinate(xsArg(1));
    PiuDimension width = xsToPiuDimension(xsArg(2));
    PiuDimension height = xsToPiuDimension(xsArg(3));
    PiuRegionRecord regionRecord;
    PiuRegion region = &regionRecord;
    PiuRegion* tmp = (*self)->work;
    (*self)->work = (*self)->swap;
    (*self)->swap = tmp;
    regionRecord.reference = NULL;
    regionRecord.available = 11;
    PiuRegionRectangle(&region, x, y, width, height);
    if (!PiuRegionXOR((*self)->work, (*self)->swap, &region))
        xsErrorPrintf("region overflowed");
	xsResult = xsThis;
}
