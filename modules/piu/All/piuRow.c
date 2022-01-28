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

static void PiuRowFitHorizontally(void* it);
static void PiuRowMeasureHorizontally(void* it);
static void PiuRowReflow(void* it, PiuFlags flags);

const PiuDispatchRecord ICACHE_FLASH_ATTR PiuRowDispatchRecord = {
	"Row",
	PiuContainerBind,
	PiuContainerCascade,
	PiuContentDraw,
	PiuRowFitHorizontally,
	PiuContainerFitVertically,
	PiuContainerHit,
	PiuContentIdle,
	PiuContainerInvalidate,
	PiuRowMeasureHorizontally,
	PiuContainerMeasureVertically,
	PiuContainerPlace,
	PiuContainerPlaceContentHorizontally,
	PiuContainerPlaceContentVertically,
	PiuRowReflow,
	PiuContainerShowing,
	PiuContainerShown,
	PiuContentSync,
	PiuContainerUnbind,
	PiuContainerUpdate
};

const xsHostHooks ICACHE_FLASH_ATTR PiuRowHooks = {
	PiuContentDelete,
	PiuContainerMark,
	NULL
};

void PiuRowFitHorizontally(void* it) 
{
	PiuRow* self = it;
	int32_t portion = (*self)->portion, sum = (*self)->sum;
	PiuCoordinate x = 0;
	PiuRectangle bounds;
	PiuCoordinates coordinates;
	PiuContent* content = (*self)->first;
	PiuContentFitHorizontally(it);
	if (portion) {
		int32_t slop = (*self)->bounds.width - sum;
		int32_t step = slop / portion;
		while (content) {
			coordinates = &((*content)->coordinates);
			if ((coordinates->horizontal & piuLeftRight) == piuLeftRight) {
				portion--;
				if (portion) {
					(*content)->bounds.width = step + coordinates->width;
					slop -= step;
				}
				else
					(*content)->bounds.width = slop + coordinates->width;
			}
			else
				(*content)->bounds.width = coordinates->width;
			(*(*content)->dispatch->fitHorizontally)(content);
			bounds = &((*content)->bounds);
			coordinates = &((*content)->coordinates);
			x += coordinates->left;
			bounds->x = x;
			x += bounds->width + coordinates->right;
			(*content)->flags &= ~piuXChanged;
			(*content)->flags |= piuPlaced;
			content = (*content)->next;
		}
	}
	else {
		while (content) {
			(*content)->bounds.width = (*content)->coordinates.width;
			(*(*content)->dispatch->fitHorizontally)(content);
			bounds = &((*content)->bounds);
			coordinates = &((*content)->coordinates);
			x += coordinates->left;
			bounds->x = x;
			x += bounds->width + coordinates->right;
			(*content)->flags &= ~piuXChanged;
			(*content)->flags |= piuPlaced;
			content = (*content)->next;
		}
	}
	(*self)->flags |= piuContentsPlaced;
}

void PiuRowMeasureHorizontally(void* it) 
{
	PiuRow* self = it;
	int32_t portion = 0, sum = 0;
	PiuCoordinates coordinates;
	PiuContent* content = (*self)->first;
	while (content) {
		if (((*content)->coordinates.horizontal & piuLeftRight) == piuLeftRight)
			portion++;
		(*(*content)->dispatch->measureHorizontally)(content);
		coordinates = &((*content)->coordinates);
		sum += coordinates->width + coordinates->left + coordinates->right;
		content = (*content)->next;
	}
	(*self)->portion = portion;
	(*self)->sum = sum;
	coordinates = &((*self)->coordinates);
	if (!(coordinates->horizontal & piuWidth))
		coordinates->width = sum;
}

void PiuRowReflow(void* it, PiuFlags flags)
{
	PiuContainer* self = it;
	PiuContainer* container = (*self)->container;
	if (flags & (piuHorizontallyChanged | piuOrderChanged))
		(*self)->flags |= piuWidthChanged;
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

void PiuRow_create(xsMachine* the)
{
	PiuRow* self;
	xsVars(4);
	xsSetHostChunk(xsThis, NULL, sizeof(PiuRowRecord));
	self = PIU(Row, xsThis);
	(*self)->the = the;
	(*self)->reference = xsToReference(xsThis);
	xsSetHostHooks(xsThis, (xsHostHooks*)&PiuRowHooks);
	(*self)->dispatch = (PiuDispatch)&PiuRowDispatchRecord;
	(*self)->flags = piuVisible | piuContainer;
	PiuContentDictionary(the, self);
	PiuContainerDictionary(the, self);
	PiuBehaviorOnCreate(self);
}

