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

static void PiuLayoutFitHorizontally(void* it);
static void PiuLayoutFitVertically(void* it);
static void PiuLayoutMeasureHorizontally(void* it);
static void PiuLayoutMeasureVertically(void* it);
static void PiuLayoutPlace(void* it);

const PiuDispatchRecord ICACHE_FLASH_ATTR PiuLayoutDispatchRecord = {
	"Layout",
	PiuContainerBind,
	PiuContainerCascade,
	PiuContentDraw,
	PiuLayoutFitHorizontally,
	PiuLayoutFitVertically,
	PiuContainerHit,
	PiuContentIdle,
	PiuContainerInvalidate,
	PiuLayoutMeasureHorizontally,
	PiuLayoutMeasureVertically,
	PiuLayoutPlace,
	PiuContainerPlaceContentHorizontally,
	PiuContainerPlaceContentVertically,
	PiuContainerReflow,
	PiuContainerShowing,
	PiuContainerShown,
	PiuContentSync,
	PiuContainerUnbind,
	PiuContainerUpdate
};

const xsHostHooks ICACHE_FLASH_ATTR PiuLayoutHooks = {
	PiuContentDelete,
	PiuContainerMark,
	NULL
};

void PiuLayoutFitHorizontally(void* it) 
{
	PiuLayout* self = it;
	PiuBehaviorOnFitHorizontally(self, (*self)->bounds.width);
	PiuContainerFitHorizontally(it);
}

void PiuLayoutFitVertically(void* it) 
{
	PiuLayout* self = it;
	PiuBehaviorOnFitVertically(self, (*self)->bounds.height);
	PiuContainerFitVertically(it);
}

void PiuLayoutMeasureHorizontally(void* it) 
{
	PiuLayout* self = it;
	PiuContainerMeasureHorizontally(it);
	PiuDimension width = PiuBehaviorOnMeasureHorizontally(self, (*self)->coordinates.width);
	(*self)->coordinates.width = width;
}

void PiuLayoutMeasureVertically(void* it) 
{
	PiuLayout* self = it;
	PiuContainerMeasureVertically(it);
	PiuDimension height = PiuBehaviorOnMeasureVertically(self, (*self)->coordinates.height);
	(*self)->coordinates.height = height;
}

void PiuLayoutPlace(void* it) 
{
	PiuContainerPlace(it);
	PiuBehaviorOnDefaultID(it, xsID_onAdapt);
}

void PiuLayout_create(xsMachine* the)
{
	PiuLayout* self;
	xsVars(4);
	xsSetHostChunk(xsThis, NULL, sizeof(PiuLayoutRecord));
	self = PIU(Layout, xsThis);
	(*self)->the = the;
	(*self)->reference = xsToReference(xsThis);
	xsSetHostHooks(xsThis, (xsHostHooks*)&PiuLayoutHooks);
	(*self)->dispatch = (PiuDispatch)&PiuLayoutDispatchRecord;
	(*self)->flags = piuVisible | piuContainer;
	PiuContentDictionary(the, self);
	PiuContainerDictionary(the, self);
	PiuBehaviorOnCreate(self);
}
