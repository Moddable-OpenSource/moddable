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

void PiuBehaviorOnCreate(void* it)
{
	PiuContent* content = it;
	if ((*content)->behavior) {
		xsMachine* the = (*content)->the;
		xsVar(0) = xsReference((*content)->behavior);
		if (xsFindResult(xsVar(0), xsID_onCreate)) {
			xsVar(1) = xsReference((*content)->reference);
			(void)xsCallFunction3(xsResult, xsVar(0), xsVar(1), xsArg(0), xsArg(1));
		}
	}
}

void PiuBehaviorOnDefaultID(void* it, xsIdentifier id)
{
	PiuContent* content = it;
	if ((*content)->behavior) {
		xsBeginHost((*content)->the);
		xsVars(2);
		xsVar(0) = xsReference((*content)->behavior);
		if (xsFindResult(xsVar(0), id)) {
			xsVar(1) = xsReference((*content)->reference);
			(void)xsCallFunction1(xsResult, xsVar(0), xsVar(1));
		}
		xsEndHost((*content)->the);
	}
}

void PiuBehaviorOnDraw(void* it, PiuRectangle area)
{
	PiuContent* content = it;
	if ((*content)->behavior) {
		xsBeginHost((*content)->the);
		xsVars(2);
		xsVar(0) = xsReference((*content)->behavior);
		if (xsFindResult(xsVar(0), xsID_onDraw)) {
			xsVar(1) = xsReference((*content)->reference);
			(void)xsCallFunction5(xsResult, xsVar(0), xsVar(1), xsPiuCoordinate(area->x), xsPiuCoordinate(area->y), xsPiuDimension(area->width), xsPiuDimension(area->height));
		}
		xsEndHost((*content)->the);
	}
}

void PiuBehaviorOnFitID(void* it, xsIdentifier id, PiuDimension dimension)
{
	PiuContent* content = it;
	if ((*content)->behavior) {
		xsBeginHost((*content)->the);
		xsVars(2);
		xsVar(0) = xsReference((*content)->behavior);
		if (xsFindResult(xsVar(0), id)) {
			xsVar(1) = xsReference((*content)->reference);
			(void)xsCallFunction2(xsResult, xsVar(0), xsVar(1), xsPiuDimension(dimension));
		}
		xsEndHost((*content)->the);
	}
}

PiuDimension PiuBehaviorOnMeasureID(void* it, xsIdentifier id, PiuDimension dimension)
{
	PiuContent* content = it;
	if ((*content)->behavior) {
		xsBeginHost((*content)->the);
		xsVars(2);
		xsVar(0) = xsReference((*content)->behavior);
		if (xsFindResult(xsVar(0), id)) {
			xsVar(1) = xsReference((*content)->reference);
			dimension = xsToPiuDimension(xsCallFunction2(xsResult, xsVar(0), xsVar(1), xsPiuDimension(dimension)));
		}
		xsEndHost((*content)->the);
	}
	return dimension;
}

PiuBoolean PiuBehaviorOnMouseID(void* it, xsIdentifier id, PiuCoordinate x, PiuCoordinate y)
{
	PiuBoolean result = 0;
	PiuContent* content = it;
	if ((*content)->behavior) {
		xsBeginHost((*content)->the);
		xsVars(2);
		xsVar(0) = xsReference((*content)->behavior);
		if (xsFindResult(xsVar(0), id)) {
			xsVar(1) = xsReference((*content)->reference);
			result = (PiuBoolean)xsToBoolean(xsCallFunction3(xsResult, xsVar(0), xsVar(1), xsPiuCoordinate(x), xsPiuCoordinate(y)));
		}
		xsEndHost((*content)->the);
	}
	return result;
}

void PiuBehaviorOnTouchID(void* it, xsIdentifier id, xsIntegerValue index, PiuCoordinate x, PiuCoordinate y, double ticks, PiuTouchLink* link)
{
	PiuContent* content = it;
	if ((*content)->behavior) {
		xsBeginHost((*content)->the);
		xsVars(3);
		xsVar(0) = xsReference((*content)->behavior);
		if (xsFindResult(xsVar(0), id)) {
			xsVar(1) = xsReference((*content)->reference);
			xsVar(2) = xsReference((*link)->reference);
			(void)xsCallFunction6(xsResult, xsVar(0), xsVar(1), xsInteger(index), xsPiuCoordinate(x), xsPiuCoordinate(y), xsNumber(ticks), xsVar(2));
		}
		xsEndHost((*content)->the);
	}
}


