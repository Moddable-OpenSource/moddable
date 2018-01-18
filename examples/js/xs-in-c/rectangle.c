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

#include "xsmc.h"

typedef struct {
	int x;
	int y;
	int w;
	int h;
} xsRectangleRecord, *xsRectangle;

void xs_rectangle(xsMachine *the)
{
	xsRectangleRecord r;
	if (xsmcArgc == 0) {
		r.x = r.y = r.w = r.h = 0;
	}
	else if (xsmcIsInstanceOf(xsArg(0), xsObjectPrototype)) {
		xsRectangle r1 = xsmcGetHostChunk(xsArg(0));
		r = *r1;
	}
	else {
		r.x = xsmcToInteger(xsArg(0));
		r.y = xsmcToInteger(xsArg(1));
		r.w = xsmcToInteger(xsArg(2));
		r.h = xsmcToInteger(xsArg(3));
	}
	xsmcSetHostChunk(xsThis, &r, sizeof(r));
}

void xs_rectangle_destructor(void *data)
{
}

void xs_rectangle_contains(xsMachine *the)
{
	xsRectangle r = xsmcGetHostChunk(xsThis);
	xsIntegerValue x = xsmcToInteger(xsArg(0));
	xsIntegerValue y = xsmcToInteger(xsArg(1));
	xsResult = (x >= r->x && x < r->x + r->w && y >= r->y && y < r->y + r->h) ? xsTrue : xsFalse;
}

void xs_rectangle_get_x(xsMachine *the)
{
	xsRectangle r = xsmcGetHostChunk(xsThis);
	xsmcSetInteger(xsResult, r->x);
}

void xs_rectangle_set_x(xsMachine *the)
{
	xsRectangle r = xsmcGetHostChunk(xsThis);
	r->x = xsmcToInteger(xsArg(0));
}

void xs_rectangle_get_y(xsMachine *the)
{
	xsRectangle r = xsmcGetHostChunk(xsThis);
	xsmcSetInteger(xsResult, r->y);
}

void xs_rectangle_set_y(xsMachine *the)
{
	xsRectangle r = xsmcGetHostChunk(xsThis);
	r->y = xsmcToInteger(xsArg(0));
}

void xs_rectangle_get_w(xsMachine *the)
{
	xsRectangle r = xsmcGetHostChunk(xsThis);
	xsmcSetInteger(xsResult, r->w);
}

void xs_rectangle_set_w(xsMachine *the)
{
	xsRectangle r = xsmcGetHostChunk(xsThis);
	r->w = xsmcToInteger(xsArg(0));
}

void xs_rectangle_get_h(xsMachine *the)
{
	xsRectangle r = xsmcGetHostChunk(xsThis);
	xsmcSetInteger(xsResult, r->h);
}

void xs_rectangle_set_h(xsMachine *the)
{
	xsRectangle r = xsmcGetHostChunk(xsThis);
	r->h = xsmcToInteger(xsArg(0));
}

static void Union(xsRectangle rUnion, xsRectangle r1, xsRectangle r2)
{
	xsRectangleRecord result;
	int v1, v2;
	result.x = (r1->x > r2->x) ? r2->x : r1->x;
	result.y = (r1->y > r2->y) ? r2->y : r1->y;
	v1 = r1->x + r1->w;
	v2 = r2->x + r2->w;
	result.w = (v1 > v2) ? v1 - result.x : v2 - result.x;
	v1 = r1->y + r1->h;
	v2 = r2->y + r2->h;
	result.h = (v1 > v2) ? v1 - result.y : v2 - result.y;
	*rUnion = result;
}

void xs_rectangle_union(xsMachine *the)
{
	xsIntegerValue i;
	xsRectangle r, r0 = xsmcGetHostChunk(xsThis);
	xsRectangleRecord rUnion;
	r = r0;
	for (i = 0; i < xsmcArgc; ++i) {
		Union(&rUnion, r, xsmcGetHostChunk(xsArg(i)));
		r = &rUnion;
	}
	*r0 = rUnion;
}
