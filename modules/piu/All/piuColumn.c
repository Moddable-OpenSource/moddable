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

static void PiuColumnFitVertically(void* it);
static void PiuColumnMeasureVertically(void* it);
static void PiuColumnReflow(void* it, PiuFlags flags);

const PiuDispatchRecord ICACHE_FLASH_ATTR PiuColumnDispatchRecord = {
	"Column",
	PiuContainerBind,
	PiuContainerCascade,
	PiuContentDraw,
	PiuContainerFitHorizontally,
	PiuColumnFitVertically,
	PiuContainerHit,
	PiuContentIdle,
	PiuContainerInvalidate,
	PiuContainerMeasureHorizontally,
	PiuColumnMeasureVertically,
	PiuContainerPlace,
	PiuContainerPlaceContentHorizontally,
	PiuContainerPlaceContentVertically,
	PiuColumnReflow,
	PiuContainerShowing,
	PiuContainerShown,
	PiuContentSync,
	PiuContainerUnbind,
	PiuContainerUpdate
};

const xsHostHooks ICACHE_FLASH_ATTR PiuColumnHooks = {
	PiuContentDelete,
	PiuContainerMark,
	NULL
};

void PiuColumnFitVertically(void* it) 
{
	PiuColumn* self = it;
	int32_t portion = (*self)->portion, sum = (*self)->sum;
	PiuCoordinate y = 0;
	PiuRectangle bounds;
	PiuCoordinates coordinates;
	PiuContent* content = (*self)->first;
	PiuContentFitVertically(it);
	if (portion) {
		int32_t slop = (*self)->bounds.height - sum;
		int32_t step = slop / portion;
		while (content) {
			coordinates = &((*content)->coordinates);
			if ((coordinates->vertical & piuTopBottom) == piuTopBottom) {
				portion--;
				if (portion) {
					(*content)->bounds.height = step + coordinates->height;
					slop -= step;
				}
				else
					(*content)->bounds.height = slop + coordinates->height;
			}
			else
				(*content)->bounds.height = coordinates->height;
			(*(*content)->dispatch->fitVertically)(content);
			bounds = &((*content)->bounds);
			coordinates = &((*content)->coordinates);
			y += coordinates->top;
			bounds->y = y;
			y += bounds->height + coordinates->bottom;
			(*content)->flags &= ~piuYChanged;
			(*content)->flags |= piuPlaced;
			content = (*content)->next;
		}
	}
	else {
		while (content) {
			(*content)->bounds.height = (*content)->coordinates.height;
			(*(*content)->dispatch->fitVertically)(content);
			bounds = &((*content)->bounds);
			coordinates = &((*content)->coordinates);
			y += coordinates->top;
			bounds->y = y;
			y += bounds->height + coordinates->bottom;
			(*content)->flags &= ~piuYChanged;
			(*content)->flags |= piuPlaced;
			content = (*content)->next;
		}
	}
	(*self)->flags |= piuContentsPlaced;
}

void PiuColumnMeasureVertically(void* it) 
{
	PiuColumn* self = it;
	int32_t portion = 0, sum = 0;
	PiuCoordinates coordinates;
	PiuContent* content = (*self)->first;
	while (content) {
		if (((*content)->coordinates.vertical & piuTopBottom) == piuTopBottom)
			portion++;
		(*(*content)->dispatch->measureVertically)(content);
		coordinates = &((*content)->coordinates);
		sum += coordinates->height + coordinates->top + coordinates->bottom;
		content = (*content)->next;
	}
	(*self)->portion = portion;
	(*self)->sum = sum;
	coordinates = &((*self)->coordinates);
	if (!(coordinates->vertical & piuHeight))
		coordinates->height = sum;
}

void PiuColumnReflow(void* it, PiuFlags flags)
{
	PiuContainer* self = it;
	PiuContainer* container = (*self)->container;
	if (flags & piuHorizontallyChanged) {
		if( ((*self)->coordinates.horizontal & piuLeftRightWidth) < piuLeftRight)
			(*self)->flags |= piuWidthChanged;
		else
			(*self)->flags |= piuContentsHorizontallyChanged;
	}
	else if (flags & piuContentsHorizontallyChanged)
		(*self)->flags |= piuContentsHorizontallyChanged;
	if (flags & (piuVerticallyChanged | piuOrderChanged))
		(*self)->flags |= piuHeightChanged;
	else if (flags & piuContentsVerticallyChanged)
		(*self)->flags |= piuContentsVerticallyChanged;
	(*self)->flags |= piuContentsPlaced;
	if (container)
		(*(*container)->dispatch->reflow)(container, (*self)->flags);
}

void PiuColumn_create(xsMachine* the)
{
	PiuColumn* self;
	xsVars(4);
	xsSetHostChunk(xsThis, NULL, sizeof(PiuColumnRecord));
	self = PIU(Column, xsThis);
	(*self)->the = the;
	(*self)->reference = xsToReference(xsThis);
	xsSetHostHooks(xsThis, (xsHostHooks*)&PiuColumnHooks);
	(*self)->dispatch = (PiuDispatch)&PiuColumnDispatchRecord;
	(*self)->flags = piuVisible | piuContainer;
	PiuContentDictionary(the, self);
	PiuContainerDictionary(the, self);
	PiuBehaviorOnCreate(self);
}

