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

void PiuRectangleApplyAspect(PiuRectangle r0, PiuRectangle r1, PiuRectangle r2, PiuAspect aspect)
{
	if (!PiuRectangleIsEqual(r1, r2)) {
		if (aspect & piuAspectFill) {
			if (aspect & piuAspectFit)
				*r0 = *r2;
			else
				PiuRectangleScaleToFill(r0, r1, r2);
		}
		else {
			if (aspect & piuAspectFit)
				PiuRectangleScaleToFit(r0, r1, r2);
			else {
#ifdef piuPC
				r0->x = r2->x + ((r2->width - r1->width) / 2);
				r0->y = r2->y + ((r2->height - r1->height) / 2);
#else
				r0->x = r2->x + ((r2->width - r1->width) >> 1);
				r0->y = r2->y + ((r2->height - r1->height) >> 1);
#endif
				r0->width = r1->width;
				r0->height = r1->height;
			}
		}
	}
}

void PiuRectangleApplyMargins(PiuRectangle r0, PiuRectangle r1, PiuMargins margins)
{
	PiuCoordinate dl, dt, dr, db;
	dl = margins->left;
	dt = margins->top;
	dr = margins->right;
	db = margins->bottom;
	PiuRectangleSet(r0, r1->x + dl, r1->y + dt, r1->width - dl - dr, r1->height - dt - db);
}

PiuBoolean PiuRectangleContains(PiuRectangle r0, PiuRectangle r1)
{
	return	r0->x <= r1->x										&&
			r0->y <= r1->y										&&
			(r0->x + r0->width ) >= (r1->x + r1->width )	&&
			(r0->y + r0->height) >= (r1->y + r1->height);
}

void PiuRectangleEmpty(PiuRectangle r)
{
	r->x = 0;
	r->y = 0;
	r->width = 0;
	r->height = 0;
}

void PiuRectangleInset(PiuRectangle r, PiuCoordinate dx, PiuCoordinate dy)
{
	r->x += dx;
	dx = r->width - dx - dx;
	r->width = (dx <= 0) ? 0 : dx;

	r->y += dy;
	dy = r->height - dy - dy;
	r->height = (dy <= 0) ? 0 : dy;
}

PiuBoolean PiuRectangleIntersect(PiuRectangle r0, PiuRectangle r1, PiuRectangle r2)
{
	int32_t v1, v2;
	int32_t left, right, top, bottom;
	if (r1->x < r2->x)
		left = r2->x;
	else
		left = r1->x;
	v1 = r1->x + r1->width;
	v2 = r2->x + r2->width;
	if (v1 < v2)
		right = v1;
	else
		right = v2;
	if (r1->y < r2->y)
		top = r2->y;
	else
		top = r1->y;
	v1 = r1->y + r1->height;
	v2 = r2->y + r2->height;
	if (v1 < v2)
		bottom = v1;
	else
		bottom = v2;
	r0->x = left;
	r0->y = top;
	if ((left < right) && (top < bottom)) {
		r0->width = right - left;
		r0->height = bottom - top;
		return 1;
	}
	r0->width = 0;
	r0->height = 0;
	return 0;
}

PiuBoolean PiuRectangleIntersects(PiuRectangle r0, PiuRectangle r1)
{
	long d;
	return ((0 != r0->width) && (0 != r0->height) && (0 != r1->width) && (0 != r1->height)
			&&	(((d = r1->x - r0->x) < 0) ? (r1->width  > -d) : (r0->width  > d))
			&&	(((d = r1->y - r0->y) < 0) ? (r1->height > -d) : (r0->height > d))
	);
}

PiuBoolean PiuRectangleIsEmpty(PiuRectangle r)
{
	return (0 == r->width) || (0 == r->height);
}

PiuBoolean PiuRectangleIsEqual(PiuRectangle r1, PiuRectangle r2)
{
	return	(r1->x == r2->x) &&
			(r1->y == r2->y) &&
			(r1->width == r2->width) &&
			(r1->height == r2->height);
}

void PiuRectangleOffset(PiuRectangle r, PiuCoordinate dx, PiuCoordinate dy)
{
	r->x += dx;
	r->y += dy;
}

void PiuRectangleScaleToFill(PiuRectangle r0, PiuRectangle r1, PiuRectangle r2)
{
	PiuRectangleRecord r;

	if ((0 == r1->height) || (0 == r1->width)) {
		PiuRectangleEmpty(r0);
		return;
	}

	r.width = r2->width;
	r.height = (r1->height * r2->width) / r1->width;
	if (r.height < r2->height) {
		r.height = r2->height;
		r.width = (r1->width * r2->height) / r1->height;
	}

	r.x = (r2->width - r.width) / 2;
	r.y = (r2->height - r.height) / 2;
	PiuRectangleOffset(&r, r2->x, r2->y);

	*r0 = r;
}

void PiuRectangleScaleToFit(PiuRectangle r0, PiuRectangle r1, PiuRectangle r2)
{
	PiuRectangleRecord r;

	if ((0 == r1->height) || (0 == r1->width)) {
		PiuRectangleEmpty(r0);
		return;
	}

	r.width = r2->width;
	r.height = (r1->height * r2->width) / r1->width;
	if (r.height > r2->height) {
		r.height = r2->height;
		r.width = (r1->width * r2->height) / r1->height;
	}

	r.x = (r2->width - r.width) / 2;
	r.y = (r2->height - r.height) / 2;
	PiuRectangleOffset(&r, r2->x, r2->y);

	*r0 = r;
}

void PiuRectangleSet(PiuRectangle r, PiuCoordinate x, PiuCoordinate y, PiuDimension width, PiuDimension height)
{
	r->x = x;
	r->y = y;
	r->width = width;
	r->height = height;
}

void PiuRectangleUnion(PiuRectangle r0, PiuRectangle r1, PiuRectangle r2)
{
	PiuRectangleRecord r;
	PiuCoordinate v1, v2;

	// deal with degenerate cases first
	if ((0 == r1->width) || (0 == r1->height))
		*r0 = *r2;
	else
	if ((0 == r2->width) || (0 == r2->height))
		*r0 = *r1;
	else {
		// neither rectangle is empty, so do some work

		// left
		if (r1->x > r2->x)
			r.x = r2->x;
		else
			r.x = r1->x;

		// top
		if (r1->y > r2->y)
			r.y = r2->y;
		else
			r.y = r1->y;

		// right
		v1 = r1->x + r1->width;
		v2 = r2->x + r2->width;
		if (v1 > v2)
			r.width = v1 - r.x;
		else
			r.width = v2 - r.x;

		// bottom
		v1 = r1->y + r1->height;
		v2 = r2->y + r2->height;
		if (v1 > v2)
			r.height= v1 - r.y;
		else
			r.height = v2 - r.y;

		if ((r.width <= 0) || (r.height <= 0)) {	/* Fruitless paranoia: ... */
			r.width  = 0;							/* ... All the cases that should have caused this ... */
			r.height = 0;							/* ... have been already taken care of above ... */
		}											/* ... unless input width or height were less than zero */

		// finish up
		*r0 = r;									/* Copy the local result to the result parameter, guaranteeing that it works in-place */
	}
}


