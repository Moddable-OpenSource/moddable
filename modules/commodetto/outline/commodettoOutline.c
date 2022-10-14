/*
 * Copyright (c) 2016-2021  Moddable Tech, Inc.
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

// Canvas and SVG arc math adapted from		
// 		https://github.com/pixelpicosean/mural
// 		https://mortoray.com/2017/02/16/rendering-an-svg-elliptical-arc-as-bezier-curves/

#include "xs.h"
#include "mc.xs.h"
#include "xsHost.h"
#define STANDALONE_ 1
#include "freetype.h"
#include "ftobjs.h"
#include "ftstroke.h"
#include "commodettoPocoOutline.h"

#define xsConstruct0(_FUNCTION) \
	(xsOverflow(-XS_FRAME_COUNT-0), \
	fxPush(_FUNCTION), \
	fxNew(the), \
	fxRunCount(the, 0), \
	fxPop())

extern void bufferToFTOutline(void *buffer, struct FT_Outline_ *outline);

void PocoOutlineCalculateCBox(PocoOutline pOutline)
{
	FT_Outline outline;
	FT_BBox box;

	if (pOutline->cboxValid)
		return;

	bufferToFTOutline(pOutline, &outline);
	FT_Outline_Get_CBox(&outline, &box);

	pOutline->xMin = box.xMin >> 6;
	pOutline->yMin = box.yMin >> 6;
	pOutline->w = ((box.xMax + 63) >> 6) - pOutline->xMin;
	pOutline->h = ((box.yMax + 63) >> 6) - pOutline->yMin;

	pOutline->cboxValid = 1;
}

#if (90 == kPocoRotation) || (180 == kPocoRotation) || (270 == kPocoRotation)
void PocoOutlineRotate(PocoOutline pOutline, int w, int h)
{
	if (!pOutline->reserved) {
		struct FT_Outline_ outline;
		int i;
		FT_Vector *points;
		bufferToFTOutline(pOutline, &outline);
		w <<= 6;
		h <<= 6;
		for (i = outline.n_points, points = outline.points; i > 0; i -= 1, points += 1) {
			int x = points->x, y = points->y;
		#if (90 == kPocoRotation)
			int t = x;
			x = h - y;
			y = t;
		#elif (180 == kPocoRotation)
			x = w - x;
			y = h - y;
		#else
			int t = x;
			x = y;
			y = w - t;
		#endif
			points->x = x;
			points->y = y;
		}
		pOutline->reserved = 1;
		pOutline->rw = w;
		pOutline->rh = h;
		pOutline->cboxValid = 0;
	}
}

void PocoOutlineUnrotate(PocoOutline pOutline)
{
	if (pOutline->reserved) {
		struct FT_Outline_ outline;
		int i;
		FT_Vector *points;
	#if (270 == kPocoRotation) || (180 == kPocoRotation)
		int w = pOutline->rw;
	#endif
	#if (90 == kPocoRotation) || (180 == kPocoRotation)
		int h = pOutline->rh;
	#endif
		bufferToFTOutline(pOutline, &outline);
		for (i = outline.n_points, points = outline.points; i > 0; i -= 1, points += 1) {
			int x = points->x, y = points->y;
		#if (90 == kPocoRotation)
			int t = x;
			x = y;
			y = h - t;
		#elif (180 == kPocoRotation)
			x = w - x;
			y = h - y;
		#else
			int t = x;
			x = w - y;
			y = t;
		#endif
			points->x = x;
			points->y = y;
		}
		pOutline->reserved = 0;
		pOutline->rw = 0;
		pOutline->rh = 0;
		pOutline->cboxValid = 0;
	}
}
#endif

//  ARGUMENTS

static xsIntegerValue xs_argToFixed(xsMachine* the, xsIntegerValue c, xsIntegerValue i, xsStringValue name)
{
	xsIntegerValue result;
	if (c <= i) xsTypeError("missing %s", name);
	if (xsTypeOf(xsArg(i)) == xsIntegerType) {
		xsIntegerValue value = xsToInteger(xsArg(i));
		result = value << 6;
	}
	else {
		xsNumberValue value = xsToNumber(xsArg(i));
		if (!c_isfinite(value)) xsTypeError("invalid %s", name);
		result = value * 64;
	}	
	return result;
}

static xsNumberValue xs_argToFloat(xsMachine* the, xsIntegerValue c, xsIntegerValue i, xsStringValue name)
{
	xsNumberValue result;
	if (c <= i) xsTypeError("missing %s", name);
	result = xsToNumber(xsArg(i));
	if (!c_isfinite(result)) xsTypeError("invalid %s", name);
	return result;
}

//  OUTLINE

void xs_outline_destructor(void *data)
{
	if (data)
		c_free(data);
}

void xs_outline(xsMachine *the)
{
}

void xs_outline_get_bounds(xsMachine *the)
{
	PocoOutline buffer = (PocoOutline)xsGetHostDataValidate(xsThis, xs_outline_destructor);
	PocoOutlineCalculateCBox(buffer);
	xsResult = xsNewObject();
	xsDefine(xsResult, xsID_x, xsInteger(buffer->xMin), xsDefault);
	xsDefine(xsResult, xsID_y, xsInteger(buffer->yMin), xsDefault);
	xsDefine(xsResult, xsID_width, xsInteger(buffer->w), xsDefault);
	xsDefine(xsResult, xsID_height, xsInteger(buffer->h), xsDefault);
}

void xs_outline_clone(xsMachine *the)
{
	PocoOutline buffer = (PocoOutline)xsGetHostDataValidate(xsThis, xs_outline_destructor);
	PocoOutline header = (PocoOutline)buffer;
	uint16_t n_points = header->n_points, n_contours = header->n_contours;
	int byteLength = sizeof(PocoOutlineRecord) + (n_points * 8) + (n_contours * 2) + n_points;
	void *data = c_malloc(byteLength);
	if (NULL == data)
		xsUnknownError("no memory");
	c_memcpy(data, buffer, byteLength);
	xsResult = xsNew0(xsThis, xsID_constructor);
	xsSetHostData(xsResult, data);
}

void xs_outline_rotate(xsMachine *the)
{
	struct FT_Outline_ outline;
	PocoOutline buffer = (PocoOutline)xsGetHostDataValidate(xsThis, xs_outline_destructor);
	int argc = xsToInteger(xsArgc);
	xsNumberValue angle = xsToNumber(xsArg(0));
	xsIntegerValue angleSin = c_sin(angle) * 256;
	xsIntegerValue angleCos = c_cos(angle) * 256;
	xsIntegerValue cx, cy;
	FT_Vector *points;
	int i;

#if (90 == kPocoRotation) || (180 == kPocoRotation) || (270 == kPocoRotation)
	PocoOutlineUnrotate(buffer);
#endif
	bufferToFTOutline(buffer, &outline);

	if (argc >= 3) {
		cx = xs_argToFixed(the, argc, 1, "cx");
		cy = xs_argToFixed(the, argc, 2, "cy");
	}
	else {
		cx = 0;
		cy = 0;
	}

	for (i = outline.n_points, points = outline.points; i > 0; i -= 1, points += 1) {
		xsIntegerValue x = points->x - cx, y = points->y - cy;
		points->x = (((x * angleCos) + (y * angleSin)) >> 8) + cx;
		points->y = (((y * angleCos) - (x * angleSin)) >> 8) + cy;
	}

	buffer->cboxValid = 0;
	xsResult = xsThis;
}

void xs_outline_scale(xsMachine *the)
{
	struct FT_Outline_ outline;
	PocoOutline buffer = (PocoOutline)xsGetHostDataValidate(xsThis, xs_outline_destructor);
	xsIntegerValue c = xsToInteger(xsArgc);
	xsIntegerValue x = xs_argToFixed(the, c, 0, "x");
	xsIntegerValue y = (c < 2) ? x : xs_argToFixed(the, c, 1, "y");
	FT_Vector *points;
	int i;

#if (90 == kPocoRotation) || (180 == kPocoRotation) || (270 == kPocoRotation)
	PocoOutlineUnrotate(buffer);
#endif
	bufferToFTOutline(buffer, &outline);

	for (i = outline.n_points, points = outline.points; i > 0; i -= 1, points += 1) {
		points->x = (x * points->x) >> 6;
		points->y = (y * points->y) >> 6;
	}

	buffer->cboxValid = 0;
	xsResult = xsThis;
}

void xs_outline_translate(xsMachine *the)
{
	struct FT_Outline_ outline;
	PocoOutline buffer = (PocoOutline)xsGetHostDataValidate(xsThis, xs_outline_destructor);
	xsIntegerValue c = xsToInteger(xsArgc);
	xsIntegerValue x = xs_argToFixed(the, c, 0, "x");
	xsIntegerValue y = xs_argToFixed(the, c, 1, "y");
	FT_Vector *points;
	int i;

#if (90 == kPocoRotation) || (180 == kPocoRotation) || (270 == kPocoRotation)
	PocoOutlineUnrotate(buffer);
#endif
	bufferToFTOutline(buffer, &outline);

	for (i = outline.n_points, points = outline.points; i > 0; i -= 1, points += 1) {
		points->x += x;
		points->y += y;
	}

	buffer->cboxValid = 0;
	xsResult = xsThis;
}

void xs_outline_fill(xsMachine* the)
{
	xsIntegerValue argc = xsToInteger(xsArgc);
	xsIntegerValue rule;
	xsIntegerValue pointCount = 0, contourCount = 0;
	xsIntegerValue subpathCount, subpathIndex, commandCount, commandIndex, kind;
	size_t size;
	uint8_t* buffer;
	PocoOutline outline;
	int32_t* points;
	uint16_t* contours;
	uint8_t* tags;
	xsVars(2);
	
	rule = (argc > 1) ? xsToInteger(xsArg(1)) : 0;
	
	subpathCount = xsToInteger(xsGet(xsArg(0), xsID_length));
	for (subpathIndex = 0; subpathIndex < subpathCount; subpathIndex++) {
		xsVar(0) = xsGetIndex(xsArg(0), subpathIndex);
		int32_t *data = xsToArrayBuffer(xsVar(0));
		xsIntegerValue byteLength = xsGetArrayBufferLength(xsVar(0));
		commandCount = *(uint32_t *)(byteLength - sizeof(uint32_t) + (uintptr_t)data) >> 2;
		if (commandCount > 3) {
			pointCount++;
			commandIndex = 3;
			while (commandIndex < commandCount) {
				kind = data[commandIndex++];
				pointCount += kind;
				commandIndex += kind << 1;
			}
			contourCount++;
		}
	}
	if (pointCount == 0)
		return;

	size = sizeof(PocoOutlineRecord) + (pointCount * 2 * 4) + (contourCount * 2) + (pointCount * 1);
	buffer = c_malloc(size);
	if (!buffer)
		xsUnknownError("no memory");
	
	outline = (PocoOutline)buffer;
	outline->n_points = pointCount;
	outline->n_contours = contourCount;
	outline->flags = rule;
	outline->cboxValid = 0;
	outline->reserved = 0;
	
	points = (int32_t*)(buffer + sizeof(PocoOutlineRecord));
	contours = (uint16_t*)(buffer + sizeof(PocoOutlineRecord) + (pointCount * 2 * 4));
	tags = (uint8_t*)(buffer + sizeof(PocoOutlineRecord) + (pointCount * 2 * 4) + (contourCount * 2));
	
	pointCount = 0;
	for (subpathIndex = 0; subpathIndex < subpathCount; subpathIndex++) {
		xsVar(0) = xsGetIndex(xsArg(0), subpathIndex);
		int32_t *data = xsToArrayBuffer(xsVar(0));
		xsIntegerValue byteLength = xsGetArrayBufferLength(xsVar(0));
		commandCount = *(uint32_t *)(byteLength - sizeof(uint32_t) + (uintptr_t)data) >> 2;
		if (commandCount > 3) {
			pointCount++;
			*points++ = data[1];
			*points++ = data[2];
			*tags++ = 1;
			commandIndex = 3;
			while (commandIndex < commandCount) {
				kind = data[commandIndex++];
				pointCount += kind;
				if (kind == 1) {
					*points++ = data[commandIndex++];
					*points++ = data[commandIndex++];
					*tags++ = 1;
				}
				else if (kind == 2) {
					*points++ = data[commandIndex++];
					*points++ = data[commandIndex++];
					*tags++ = 0;
					*points++ = data[commandIndex++];
					*points++ = data[commandIndex++];
					*tags++ = 1;
				}
				else if (kind == 3) {
					*points++ = data[commandIndex++];
					*points++ = data[commandIndex++];
					*tags++ = 2;
					*points++ = data[commandIndex++];
					*points++ = data[commandIndex++];
					*tags++ = 2;
					*points++ = data[commandIndex++];
					*points++ = data[commandIndex++];
					*tags++ = 1;
				}
			}
			*contours++ = pointCount - 1;
		}
	}

	xsResult = xsConstruct0(xsThis);
	xsSetHostData(xsResult, buffer);
}

static void *ftAlloc(FT_Memory memory, long size)
{
	return c_malloc(size);
}

static void ftFree(FT_Memory memory, void *block)
{
	if (block)
		c_free(block);
}

static void *ftRealloc(FT_Memory memory, long cur_size, long new_size, void *block)
{
	return c_realloc(block, new_size);
}

void xs_outline_stroke(xsMachine* the)
{
	xsIntegerValue argc = xsToInteger(xsArgc);
	xsIntegerValue width, cap, join, miterlimit;
	struct FT_MemoryRec_ memory;
	FT_LibraryRec library = {0};
	FT_Stroker stroker;
	xsIntegerValue subpathCount, subpathIndex, open, commandCount, commandIndex, kind;
	FT_Vector vector, vector1, vector2;
	xsIntegerValue pointCount = 0, contourCount = 0;
	size_t size;
	uint8_t* buffer;
	struct FT_Outline_ outline;
	xsVars(2);

	width = (argc > 1) ? xsToNumber(xsArg(1)) : 1;
	cap = (argc > 2) ? xsToInteger(xsArg(2)) : FT_STROKER_LINECAP_ROUND;
	join = (argc > 3) ? xsToInteger(xsArg(3)) : FT_STROKER_LINEJOIN_ROUND;
	miterlimit = (argc > 4) ? xsToNumber(xsArg(4)) : width;
	
	width = width * 64;
	miterlimit = miterlimit * 65536;

	memory.alloc = ftAlloc;
	memory.free = ftFree;
	memory.realloc = ftRealloc;
	library.memory = &memory;
	if (FT_Stroker_New(&library, &stroker))
		xsUnknownError("no memory");
		
	FT_Stroker_Set(stroker, width >> 1, cap, join, miterlimit);
	
	subpathCount = xsToInteger(xsGet(xsArg(0), xsID_length));
	for (subpathIndex = 0; subpathIndex < subpathCount; subpathIndex++) {
		xsVar(0) = xsGetIndex(xsArg(0), subpathIndex);
		int32_t *data = xsToArrayBuffer(xsVar(0));
		xsIntegerValue byteLength = xsGetArrayBufferLength(xsVar(0));
		commandCount = *(uint32_t *)(byteLength - sizeof(uint32_t) + (uintptr_t)data) >> 2;
		if (commandCount > 3) {
			open = data[0];
			vector.x = data[1];
			vector.y = data[2];
			FT_Stroker_BeginSubPath(stroker, &vector, open);
			commandIndex = 3;
			while (commandIndex < commandCount) {
				kind = data[commandIndex++];
				if (kind == 1) {
					vector.x = data[commandIndex++];
					vector.y = data[commandIndex++];
					FT_Stroker_LineTo(stroker, &vector);
				}
				else if (kind == 2) {
					vector1.x = data[commandIndex++];
					vector1.y = data[commandIndex++];
					vector.x = data[commandIndex++];
					vector.y = data[commandIndex++];
					FT_Stroker_ConicTo(stroker, &vector1, &vector);
				}
				else if (kind == 3) {
					vector1.x = data[commandIndex++];
					vector1.y = data[commandIndex++];
					vector2.x = data[commandIndex++];
					vector2.y = data[commandIndex++];
					vector.x = data[commandIndex++];
					vector.y = data[commandIndex++];
					FT_Stroker_CubicTo(stroker, &vector1, &vector2, &vector);
				}
			}
			FT_Stroker_EndSubPath(stroker);
		}
	}
	FT_Stroker_GetCounts(stroker, (FT_UInt*)&pointCount, (FT_UInt*)&contourCount);
	if (pointCount == 0) {
		FT_Stroker_Done(stroker);
		return;
	}
	size = sizeof(PocoOutlineRecord) + (pointCount * 2 * 4) + (contourCount * 2) + (pointCount * 1);
	buffer = c_malloc(size);
	if (!buffer) {
		FT_Stroker_Done(stroker);
		xsUnknownError("no memory");
	}
	((PocoOutline)buffer)->n_points = pointCount;
	((PocoOutline)buffer)->n_contours = contourCount;
	((PocoOutline)buffer)->flags = 0; // FT_OUTLINE_NONE
	((PocoOutline)buffer)->cboxValid = 0;
	((PocoOutline)buffer)->reserved = 0;

	bufferToFTOutline(buffer, &outline);
	outline.n_points = 0;
	outline.n_contours = 0;
	FT_Stroker_Export(stroker, &outline);
	FT_Stroker_Done(stroker);

	xsResult = xsConstruct0(xsThis);
	xsSetHostData(xsResult, buffer);
}

// PATH

static int32_t *getSubpathSpace(xsMachine* the, int count);

static void xs_outline_path_newSubpath(xsMachine* the, xsIntegerValue x, xsIntegerValue y, xsBooleanValue open)
{
	xsVar(0) = xsGet(xsThis, xsID_subpath);
	if (xsUndefinedType != xsTypeOf(xsVar(0))) {
		void *data = xsToArrayBuffer(xsVar(0));
		xsIntegerValue byteLength = xsGetArrayBufferLength(xsVar(0));
		byteLength = *(uint32_t *)(byteLength - sizeof(uint32_t) + (uintptr_t)data);
		xsSetArrayBufferLength(xsVar(0), byteLength + sizeof(uint32_t));
		data = xsToArrayBuffer(xsVar(0));
		*(uint32_t *)(byteLength + (uintptr_t)data) = byteLength;
	}
	
	xsVar(0) = xsArrayBufferResizable(NULL, sizeof(uint32_t), 0x7FFFFFFF);
	xsCall1(xsThis, xsID_push, xsVar(0));

	int32_t *subpath = getSubpathSpace(the, 3);
	*subpath++ = open;
	*subpath++ = x;
	*subpath++ = y;
}

static void xs_outline_path_renewSubpath(xsMachine* the)
{
	xsSetIndex(xsVar(0), 0, xsInteger(0));
	int32_t x = xsToInteger(xsGetIndex(xsVar(0), 1));
	int32_t y = xsToInteger(xsGetIndex(xsVar(0), 2));

	void *data = xsToArrayBuffer(xsVar(0));
	xsIntegerValue byteLength = xsGetArrayBufferLength(xsVar(0));
	uint32_t bytesUsed = *(uint32_t *)(byteLength - sizeof(uint32_t) + (uintptr_t)data);
	xsSetArrayBufferLength(xsVar(0), bytesUsed + sizeof(uint32_t));
	*(uint32_t *)(bytesUsed + (uintptr_t)data) = bytesUsed;

	xsVar(0) = xsArrayBufferResizable(NULL, sizeof(uint32_t), 0x7FFFFFFF);
	xsCall1(xsThis, xsID_push, xsVar(0));
	int32_t *subpath = getSubpathSpace(the, 3);
	*subpath++ = 1;
	*subpath++ = x;
	*subpath++ = y;
}

// SUBPATH

static int32_t *getSubpathSpace(xsMachine* the, int count)
{
	void *data = xsToArrayBuffer(xsVar(0));
	xsIntegerValue byteLength = xsGetArrayBufferLength(xsVar(0));
	uint32_t bytesUsed = *(uint32_t *)(byteLength - sizeof(uint32_t) + (uintptr_t)data);
	uint32_t bytesAvailable = byteLength - bytesUsed - sizeof(uint32_t);
	uint32_t bytesNeeded = count * sizeof(int32_t);

	if (bytesAvailable < bytesNeeded) {
		byteLength = bytesUsed + bytesNeeded + sizeof(uint32_t);
		byteLength += (byteLength >> 2);			// some slop
		byteLength = (byteLength + 3) & ~3;
		xsSetArrayBufferLength(xsVar(0), byteLength);
		data = xsToArrayBuffer(xsVar(0));
	}

	*(uint32_t *)(byteLength - sizeof(uint32_t) + (uintptr_t)data) = bytesUsed + bytesNeeded;
	return (int32_t *)(bytesUsed + (uintptr_t)data);
}

static void xs_outline_subpath_push1(xsMachine* the, xsIntegerValue x, xsIntegerValue y)
{
	int32_t *subpath = getSubpathSpace(the, 3);
	*subpath++ = 1;
	*subpath++ = x;
	*subpath++ = y;
}

static void xs_outline_subpath_push2(xsMachine* the, xsIntegerValue x1, xsIntegerValue y1, xsIntegerValue x, xsIntegerValue y)
{
	int32_t *subpath = getSubpathSpace(the, 5);
	*subpath++ = 2;
	*subpath++ = x1;
	*subpath++ = y1;
	*subpath++ = x;
	*subpath++ = y;
}

static void xs_outline_subpath_push3(xsMachine* the, xsIntegerValue x1, xsIntegerValue y1, xsIntegerValue x2, xsIntegerValue y2, xsIntegerValue x, xsIntegerValue y)
{
	int32_t *subpath = getSubpathSpace(the, 7);
	*subpath++ = 3;
	*subpath++ = x1;
	*subpath++ = y1;
	*subpath++ = x2;
	*subpath++ = y2;
	*subpath++ = x;
	*subpath++ = y;
}

// MATH

static xsIntegerValue xs_outline_arcToCubic(xsMachine* the, xsNumberValue theta, xsNumberValue delta, xsNumberValue* matrix, xsNumberValue* points)
{
	xsIntegerValue c, i, j;
	xsNumberValue* p;
	if (delta == 0)
		return 0;
	c = c_ceil(c_fabs(delta) / (C_M_PI / 2));
	if (c < 1) c = 1;
	else if (c > 4) c = 4;
	delta /= c;
	for (i = 0, p = points; i < c; i++) {
		xsNumberValue ratio, x1, y1, x2, y2;
		ratio = (4 * c_tan(delta / 4)) / 3;
		x1 = c_cos(theta);
		y1 = c_sin(theta);
		theta += delta;
		x2 = c_cos(theta);
		y2 = c_sin(theta);
		*p++ = x1;
		*p++ = y1;
		*p++ = x1 - y1 * ratio;
		*p++ = y1 + x1 * ratio;
		*p++ = x2 + y2 * ratio;
		*p++ = y2 - x2 * ratio;
		*p++ = x2;
		*p++ = y2;
	}
	for (i = 0, p = points; i < c; i++) {
		for (j = 0; j < 4; j++) {
			xsNumberValue x = *p;
			xsNumberValue y = *(p + 1);
			*p++ = (matrix[0] * x) + (matrix[1] * y) + (matrix[2]);
			*p++ = (matrix[3] * x) + (matrix[4] * y) + (matrix[5]);
		}
	}
	return c;
}

static xsNumberValue xs_outline_vectorsToAngle(xsNumberValue ux, xsNumberValue uy, xsNumberValue vx, xsNumberValue vy) {
	xsNumberValue dot, angle;
	dot = (ux * vx + uy * vy) / (c_sqrt(ux * ux + uy * uy) * c_sqrt(vx * vx + vy * vy));
	if (dot > 1) dot = 1;
	if (dot < -1) dot = -1;
	angle = c_acos(dot);
	if (((ux * vy) - (uy * vx)) < 0)
        angle = -angle;
    return angle;
}

// FREETYPE PATH

static void xs_outline_path_checkSubpath(xsMachine* the, xsIntegerValue x, xsIntegerValue y)
{
	xsVars(1);
	xsVar(0) = xsGet(xsThis, xsID_subpath);
	if (!xsTest(xsVar(0))) {
		xsTypeError("no subpath");
	}
}

void xs_outline_FreeTypePath_beginSubpath(xsMachine* the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	xsIntegerValue x = xs_argToFixed(the, c, 0, "x");
	xsIntegerValue y = xs_argToFixed(the, c, 1, "y");
	xsBooleanValue open = (c > 2) ? xsToBoolean(xsArg(2)) : 0;
	xsVars(1);
	xs_outline_path_newSubpath(the, x, y, open);
	xsSet(xsThis, xsID_subpath, xsVar(0));
}

void xs_outline_FreeTypePath_conicTo(xsMachine* the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	xsIntegerValue cx = xs_argToFixed(the, c, 0, "cx");
	xsIntegerValue cy = xs_argToFixed(the, c, 1, "cy");
	xsIntegerValue x = xs_argToFixed(the, c, 2, "x");
	xsIntegerValue y = xs_argToFixed(the, c, 3, "y");
	xs_outline_path_checkSubpath(the, x, y);
	xs_outline_subpath_push2(the, cx, cy, x, y);
}

void xs_outline_FreeTypePath_cubicTo(xsMachine* the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	xsIntegerValue c1x = xs_argToFixed(the, c, 0, "c1x");
	xsIntegerValue c1y = xs_argToFixed(the, c, 1, "c1y");
	xsIntegerValue c2x = xs_argToFixed(the, c, 2, "c2x");
	xsIntegerValue c2y = xs_argToFixed(the, c, 3, "c2y");
	xsIntegerValue x = xs_argToFixed(the, c, 4, "x");
	xsIntegerValue y = xs_argToFixed(the, c, 5, "y");
	xs_outline_path_checkSubpath(the, x, y);
	xs_outline_subpath_push3(the, c1x, c1y, c2x, c2y, x, y);
}

void xs_outline_FreeTypePath_endSubpath(xsMachine* the)
{
	xsDelete(xsThis, xsID_subpath);
}

void xs_outline_FreeTypePath_lineTo(xsMachine* the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	xsIntegerValue x = xs_argToFixed(the, c, 0, "x");
	xsIntegerValue y = xs_argToFixed(the, c, 1, "y");
	xs_outline_path_checkSubpath(the, x, y);
	xs_outline_subpath_push1(the, x, y);
}

// POLYGON PATH

void xs_outline_PolygonPath(xsMachine* the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	xsIntegerValue i = 0;
	xsVars(1);
	xsVar(0) = xsArrayBuffer(NULL, ((c >> 1) + c + 1) * sizeof(int32_t));
	int32_t *subpath = xsToArrayBuffer(xsVar(0));

	*subpath++ = 0;
	*subpath++ = xs_argToFixed(the, c, i++, "x");
	*subpath++ = xs_argToFixed(the, c, i++, "y");
	while (i < c) {
		*subpath++ = 1;
		*subpath++ = xs_argToFixed(the, c, i++, "x");
		*subpath++ = xs_argToFixed(the, c, i++, "y");
	}
	*subpath++ = ((c >> 1) + c) * sizeof(int32_t);
	xsResult = xsNewArray(1);
	xsSetIndex(xsResult, 0, xsVar(0));
}

static void xs_argToRadius(xsMachine* the, xsIntegerValue c, xsIntegerValue i, xsIntegerValue* x, xsIntegerValue* y, xsIntegerValue dx, xsIntegerValue dy)
{
	if (c > i) {
		if (xsTypeOf(xsArg(i)) == xsReferenceType) {
			xsArg(0) = xsGet(xsArg(i), xsID_x);
			xsArg(1) = xsGet(xsArg(i), xsID_y);
			*x = xs_argToFixed(the, c, 0, "r");
			*y = xs_argToFixed(the, c, 1, "r");
		}
		else
			*x = *y = xs_argToFixed(the, c, i, "r");
	}	
	else {
		*x = dx;
		*y = dy;
	}
}

void xs_outline_RoundRectPath(xsMachine* the)
{
#define TAN(R) ((xsIntegerValue)(((xsNumberValue)(R)) * 0.552284749831))
	xsIntegerValue c = xsToInteger(xsArgc);
	xsIntegerValue lx = xs_argToFixed(the, c, 0, "x");
	xsIntegerValue ty = xs_argToFixed(the, c, 1, "y");
	xsIntegerValue rx = lx + xs_argToFixed(the, c, 2, "w");
	xsIntegerValue by = ty + xs_argToFixed(the, c, 3, "h");
	xsIntegerValue tlx, tly, trx, try, brx, bry, blx, bly;
	xsVars(1);
	
	xs_argToRadius(the, c, 4, &tlx, &tly, 0, 0);	
	xs_argToRadius(the, c, 5, &trx, &try, tlx, tly);	
	xs_argToRadius(the, c, 6, &brx, &bry, tlx, tly);	
	xs_argToRadius(the, c, 7, &blx, &bly, trx, try);	

	xsVar(0) = xsArrayBuffer(NULL, 41 * sizeof(int32_t));
	int32_t *subpath = xsToArrayBuffer(xsVar(0));
	*subpath++ = 0;
	*subpath++ = lx;
	*subpath++ = ty + tly;
	*subpath++ = 3;
	*subpath++ = lx;
	*subpath++ = ty + tly - TAN(tly);
	*subpath++ = lx + tlx - TAN(tlx);
	*subpath++ = ty;
	*subpath++ = lx + tlx;
	*subpath++ = ty;

	*subpath++ = 1;
	*subpath++ = rx - trx;
	*subpath++ = ty;
	*subpath++ = 3;
	*subpath++ = rx - trx + TAN(trx);
	*subpath++ = ty;
	*subpath++ = rx;
	*subpath++ = ty + try - TAN(try);
	*subpath++ = rx;
	*subpath++ = ty + try;
	
	*subpath++ = 1;
	*subpath++ = rx;
	*subpath++ = by - bry;
	*subpath++ = 3;
	*subpath++ = rx;
	*subpath++ = by - bry + TAN(bry);
	*subpath++ = rx - brx + TAN(brx);
	*subpath++ = by;
	*subpath++ = rx - brx;
	*subpath++ = by;
	
	*subpath++ = 1;
	*subpath++ = lx + blx;
	*subpath++ = by;
	*subpath++ = 3;
	*subpath++ = lx + blx - TAN(blx);
	*subpath++ = by;
	*subpath++ = lx;
	*subpath++ = by - bly + TAN(bly);
	*subpath++ = lx;
	*subpath++ = by - bly;

	*subpath++ = 40 * sizeof(int32_t);
	
	xsResult = xsNewArray(1);
	xsSetIndex(xsResult, 0, xsVar(0));
}

// CANVAS PATH

static void xs_outline_path_ensureSubpath(xsMachine* the, xsIntegerValue x, xsIntegerValue y)
{
	xsVars(1);
	xsVar(0) = xsGet(xsThis, xsID_subpath);
	if (!xsTest(xsVar(0))) {
		xs_outline_path_newSubpath(the, x, y, 1);
		xsSet(xsThis, xsID_subpath, xsVar(0));
	}
}

void xs_outline_CanvasPath_arc(xsMachine* the)
{
	xsIntegerValue c = xsToInteger(xsArgc), i;
	xsNumberValue cx = xs_argToFloat(the, c, 0, "x");
	xsNumberValue cy = xs_argToFloat(the, c, 1, "y");
	xsNumberValue radius = xs_argToFloat(the, c, 2, "radius");
	xsNumberValue theta = xs_argToFloat(the, c, 3, "startAngle");
	xsNumberValue delta = xs_argToFloat(the, c, 4, "endAngle");
	xsBooleanValue counterclockwise = (c > 5) ? xsToBoolean(xsArg(5)) : 0;
	xsNumberValue matrix[6];
	xsNumberValue points[4 * 8];
	xsNumberValue* p = points;
	xsVars(1);
	xsVar(0) = xsGet(xsThis, xsID_subpath);
	if (radius < 0) xsRangeError("invalid radius");
	delta -= theta;
	if (delta < 0)
		delta += 2 * C_M_PI;
	if (counterclockwise)
		delta = -delta;
	matrix[0] = radius;
	matrix[1] = 0;
	matrix[2] = cx;
	matrix[3] = 0;
	matrix[4] = radius;
	matrix[5] = cy;
	c = xs_outline_arcToCubic(the, theta, delta, matrix, points);
	if (c) {
		if (xsTest(xsVar(0))) 
			xs_outline_subpath_push1(the, p[0]*64, p[1]*64);
		else
			xs_outline_path_newSubpath(the, p[0]*64, p[1]*64, 1);
		for (i = 0; i < c; i++) {
			xs_outline_subpath_push3(the, p[2]*64, p[3]*64, p[4]*64, p[5]*64, p[6]*64, p[7]*64);
			p += 8;
		}
	}
}

void xs_outline_CanvasPath_arcTo(xsMachine* the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	xsNumberValue x1 = xs_argToFloat(the, c, 0, "x1");
	xsNumberValue y1 = xs_argToFloat(the, c, 1, "y1");
	xsNumberValue x2 = xs_argToFloat(the, c, 2, "x2");
	xsNumberValue y2 = xs_argToFloat(the, c, 3, "y2");
	xsNumberValue radius = xs_argToFloat(the, c, 4, "radius");
	xs_outline_path_ensureSubpath(the, x1*64, y1*64);
	{
		xsIntegerValue i = xsToInteger(xsGet(xsVar(0), xsID_length));
		xsNumberValue x0 = xsToInteger(xsGetIndex(xsVar(0), i - 2)) / 64;
		xsNumberValue y0 = xsToInteger(xsGetIndex(xsVar(0), i - 1)) / 64;
   		
		xsNumberValue a1 = y0 - y1;
		xsNumberValue b1 = x0 - x1;
		xsNumberValue a2 = y2 - y1;
		xsNumberValue b2 = x2 - x1;
		xsNumberValue mm = c_fabs(a1 * b2 - b1 * a2);

		if (mm < 1.0e-8 || radius == 0) {
			xs_outline_subpath_push1(the, x1*64, y1*64);
		}
		else {
			xsNumberValue dd = a1 * a1 + b1 * b1;
			xsNumberValue cc = a2 * a2 + b2 * b2;
			xsNumberValue tt = a1 * a2 + b1 * b2;
			xsNumberValue k1 = radius * c_sqrt(dd) / mm;
			xsNumberValue k2 = radius * c_sqrt(cc) / mm;
			xsNumberValue j1 = k1 * tt / dd;
			xsNumberValue j2 = k2 * tt / cc;
			xsNumberValue cx = k1 * b2 + k2 * b1;
			xsNumberValue cy = k1 * a2 + k2 * a1;
			xsNumberValue px = b1 * (k2 + j1);
			xsNumberValue py = a1 * (k2 + j1);
			xsNumberValue qx = b2 * (k1 + j2);
			xsNumberValue qy = a2 * (k1 + j2);
			xsNumberValue startAngle = c_atan2(py - cy, px - cx);
			xsNumberValue endAngle = c_atan2(qy - cy, qx - cx);
			xsBooleanValue counterclockwise = (b1 * a2 > b2 * a1) ? 1 : 0;
			xsNumberValue matrix[6];
			xsNumberValue points[4 * 8];
			xsNumberValue* p = points;
			xsNumberValue delta = endAngle - startAngle;
			if (delta < 0)
				delta += 2 * C_M_PI;
			if (counterclockwise)
				delta = -delta;
		
			matrix[0] = radius;
			matrix[1] = 0;
			matrix[2] = x1 + cx;
			matrix[3] = 0;
			matrix[4] = radius;
			matrix[5] = y1 + cy;
			c = xs_outline_arcToCubic(the, startAngle, delta, matrix, points);
			if (c) {
				xs_outline_subpath_push1(the, p[0]*64, p[1]*64);
				for (i = 0; i < c; i++) {
					xs_outline_subpath_push3(the, p[2]*64, p[3]*64, p[4]*64, p[5]*64, p[6]*64, p[7]*64);
					p += 8;
				}
			}
		}
	}
}

void xs_outline_CanvasPath_bezierCurveTo(xsMachine* the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	xsIntegerValue cp1x = xs_argToFixed(the, c, 0, "cp1x");
	xsIntegerValue cp1y = xs_argToFixed(the, c, 1, "cp1y");
	xsIntegerValue cp2x = xs_argToFixed(the, c, 2, "cp2x");
	xsIntegerValue cp2y = xs_argToFixed(the, c, 3, "cp2y");
	xsIntegerValue x = xs_argToFixed(the, c, 4, "x");
	xsIntegerValue y = xs_argToFixed(the, c, 5, "y");
	xs_outline_path_ensureSubpath(the, cp1x, cp1y);
	xs_outline_subpath_push3(the, cp1x, cp1y, cp2x, cp2y, x, y);
}

void xs_outline_CanvasPath_closePath(xsMachine* the)
{
	xsVars(3);
	xsVar(0) = xsGet(xsThis, xsID_subpath);
	if (xsTest(xsVar(0))) {
		xs_outline_path_renewSubpath(the);
		xsSet(xsThis, xsID_subpath, xsVar(0));
	}
}

void xs_outline_CanvasPath_ellipse(xsMachine* the)
{
	xsIntegerValue c = xsToInteger(xsArgc), i;
	xsNumberValue cx = xs_argToFloat(the, c, 0, "x");
	xsNumberValue cy = xs_argToFloat(the, c, 1, "y");
	xsNumberValue rx = xs_argToFloat(the, c, 2, "radiusX");
	xsNumberValue ry = xs_argToFloat(the, c, 3, "radiusY");
	xsNumberValue rotation = xs_argToFloat(the, c, 4, "rotation");
	xsNumberValue theta = xs_argToFloat(the, c, 5, "startAngle");
	xsNumberValue delta = xs_argToFloat(the, c, 6, "endAngle");
	xsBooleanValue counterclockwise = (c > 7) ? xsToBoolean(xsArg(7)) : 0;
	xsNumberValue sina = c_sin(rotation);
	xsNumberValue cosa = c_cos(rotation);
	xsNumberValue matrix[6];
	xsNumberValue points[4 * 8];
	xsNumberValue* p = points;
	xsVars(1);
	xsVar(0) = xsGet(xsThis, xsID_subpath);
	if (rx < 0) xsRangeError("invalid radiusX");
	if (ry < 0) xsRangeError("invalid radiusY");
	delta -= theta;
	if (delta < 0)
		delta += 2 * C_M_PI;
	if (counterclockwise) {
		delta = -delta;
	}
	matrix[0] = cosa * rx;
	matrix[1] = -sina * ry;
	matrix[2] = cx;
	matrix[3] = sina * rx;
	matrix[4] = cosa * ry;
	matrix[5] = cy;
	c = xs_outline_arcToCubic(the, theta, delta, matrix, points);
	if (c) {
		if (xsTest(xsVar(0))) 
			xs_outline_subpath_push1(the, p[0]*64, p[1]*64);
		else
			xs_outline_path_newSubpath(the, p[0]*64, p[1]*64, 1);
		for (i = 0; i < c; i++) {
			xs_outline_subpath_push3(the, p[2]*64, p[3]*64, p[4]*64, p[5]*64, p[6]*64, p[7]*64);
			p += 8;
		}
	}
}

void xs_outline_CanvasPath_lineTo(xsMachine* the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	xsIntegerValue x = xs_argToFixed(the, c, 0, "x");
	xsIntegerValue y = xs_argToFixed(the, c, 1, "y");
	xs_outline_path_ensureSubpath(the, x, y);
	xs_outline_subpath_push1(the, x, y);
}

void xs_outline_CanvasPath_moveTo(xsMachine* the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	xsIntegerValue x = xs_argToFixed(the, c, 0, "x");
	xsIntegerValue y = xs_argToFixed(the, c, 1, "y");
	xsVars(1);
	xs_outline_path_newSubpath(the, x, y, 1);
	xsSet(xsThis, xsID_subpath, xsVar(0));
}

void xs_outline_CanvasPath_quadraticCurveTo(xsMachine* the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	xsIntegerValue cpx = xs_argToFixed(the, c, 0, "cpx");
	xsIntegerValue cpy = xs_argToFixed(the, c, 1, "cpy");
	xsIntegerValue x = xs_argToFixed(the, c, 2, "x");
	xsIntegerValue y = xs_argToFixed(the, c, 3, "y");
	xs_outline_path_ensureSubpath(the, cpx, cpy);
	xs_outline_subpath_push2(the, cpx, cpy, x, y);
}

void xs_outline_CanvasPath_rect(xsMachine* the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	xsIntegerValue x = xs_argToFixed(the, c, 0, "x");
	xsIntegerValue y = xs_argToFixed(the, c, 1, "y");
	xsIntegerValue w = xs_argToFixed(the, c, 2, "w");
	xsIntegerValue h = xs_argToFixed(the, c, 3, "h");
	xsVars(1);
	xs_outline_path_newSubpath(the, x, y, 0);
	xs_outline_subpath_push1(the, x + w, y);
	xs_outline_subpath_push1(the, x + w, y + h);
	xs_outline_subpath_push1(the, x, y + h);
	xs_outline_path_newSubpath(the, x, y, 1);
	xsSet(xsThis, xsID_subpath, xsVar(0));
}

// SVG PATH

typedef struct {
	xsIntegerValue offset;
	xsIntegerValue token;
	xsNumberValue number;
} SVGPathParserRecord, *SVGPathParser;

static void xs_outline_SVGPath_next(xsMachine* the, SVGPathParser parser)
{
	char buffer[32];
	xsStringValue s = xsToString(xsArg(0));
	xsStringValue p = s + parser->offset, q;
	size_t size;
	char c = *p;
	while ((c == 9) || (c == 10) || (c == 12) || (c == 13) || (c == 32)) {
		p++;
		c = *p;
	}
	switch (c) {
	case '+':
	case '-':
	case '.':
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		q = p;
		if ((*p == '+') || (*p == '-'))
			p++;
		if (('0' <= *p) && (*p <= '9')) {
			if (*p == '0') {
				p++;
			}
			else {
				p++;
				while (('0' <= *p) && (*p <= '9'))
					p++;
			}
		}
		if (*p == '.') {
			p++;
			if (('0' <= *p) && (*p <= '9')) {
				p++;
				while (('0' <= *p) && (*p <= '9'))
					p++;
			}
			else
				goto error;
		}
		if ((*p == 'e') || (*p == 'E')) {
			p++;
			if ((*p == '+') || (*p == '-'))
				p++;
			if (('0' <= *p) && (*p <= '9')) {
				p++;
				while (('0' <= *p) && (*p <= '9'))
					p++;
			}
			else
				goto error;
		}
		size = p - q;
		if (size + 1 > sizeof(buffer))
			xsUnknownError("number overflow");
		c_memcpy(buffer, q, size);
		buffer[size] = 0;
		parser->number = fxStringToNumber(the, buffer, 0);
		parser->offset = p - s;
		parser->token = '0';
		return;
	default:
        p++;
		parser->offset = p - s;
		parser->token = c;
		return;
	}
error:
	xsSyntaxError("invalid number");
}

static xsNumberValue xs_outline_SVGPath_number(xsMachine* the, SVGPathParser parser)
{
	xsNumberValue result = 0;
	if (parser->token == '0')
		result = parser->number;
	else
		xsSyntaxError("invalid command %c", parser->token);
	xs_outline_SVGPath_next(the, parser);
	return result;
}

static xsBooleanValue xs_outline_SVGPath_test(xsMachine* the, SVGPathParser parser)
{
	xsBooleanValue result = 0;
	if (parser->token == ',') {
		xs_outline_SVGPath_next(the, parser);
		result = 1;
	}
	else if (parser->token == '0')
		result = 1;
	return result;
}

void xs_outline_SVGPath(xsMachine* the)
{
	SVGPathParserRecord _parser;
	SVGPathParser parser = &_parser;
	xsNumberValue fx = 0, fy = 0, fx1 = 0, fy1 = 0, fx2 = 0, fy2 = 0;
	xsVars(3);
	xsResult = xsNewArray(0);
	xsThis = xsResult;
	parser->offset = 0;
	xs_outline_SVGPath_next(the, parser);
	for (;;) {
		xsIntegerValue token = parser->token;
		if (token == 0)
			break;
		xs_outline_SVGPath_next(the, parser);
		if ((token != 'M') && (token != 'm')) {
			if (!xsTest(xsVar(0)))
				xsSyntaxError("path must start with M");
		}
		if ((token == 'Z') || (token == 'z'))  {
			xs_outline_path_renewSubpath(the);
		}
		else if ((token == 'M') || (token == 'm') || (token == 'L') || (token == 'l'))  {
			do {
				xsNumberValue x, y;
				x = xs_outline_SVGPath_number(the, parser);
				xs_outline_SVGPath_test(the, parser);
				y = xs_outline_SVGPath_number(the, parser);
				if ((token == 'm') || (token == 'l')) {
					x += fx;
					y += fy;
				}
				if ((token == 'M') || (token == 'm'))
					xs_outline_path_newSubpath(the, x*64, y*64, 1);
				else
					xs_outline_subpath_push1(the, x*64, y*64);
				fx1 = x;
				fy1 = y;
				fx2 = x;
				fy2 = y;
				fx = x;
				fy = y;
				if (token == 'M') 
					token = 'L';
				else if (token == 'm') 
					token = 'l';
			} while (xs_outline_SVGPath_test(the, parser));
		}
		else if ((token == 'H') || (token == 'h'))  {
			do {
				xsNumberValue x, y;
				x = xs_outline_SVGPath_number(the, parser);
				y = fy;
				if (token == 'h')
					x += fx;
				xs_outline_subpath_push1(the, x*64, y*64);
				fx1 = x;
				fy1 = y;
				fx2 = x;
				fy2 = y;
				fx = x;
				fy = y;
			} while (xs_outline_SVGPath_test(the, parser));
		}
		else if ((token == 'V') || (token == 'v'))  {
			do {
				xsNumberValue x, y;
				x = fx;
				y = xs_outline_SVGPath_number(the, parser);
				if (token == 'v')
					y += fy;
				xs_outline_subpath_push1(the, x*64, y*64);
				fx1 = x;
				fy1 = y;
				fx2 = x;
				fy2 = y;
				fx = x;
				fy = y;
			} while (xs_outline_SVGPath_test(the, parser));
		}
		else if ((token == 'Q') || (token == 'q'))  {
			do {
				xsNumberValue x1, y1, x, y;
				x1 = xs_outline_SVGPath_number(the, parser);
				xs_outline_SVGPath_test(the, parser);
				y1 = xs_outline_SVGPath_number(the, parser);
				xs_outline_SVGPath_test(the, parser);
				x = xs_outline_SVGPath_number(the, parser);
				xs_outline_SVGPath_test(the, parser);
				y = xs_outline_SVGPath_number(the, parser);
				if (token == 'q') {
					x1 += fx;
					y1 += fy;
					x += fx;
					y += fy;
				}
				xs_outline_subpath_push2(the, x1*64, y1*64, x*64, y*64);
				fx1 = x1;
				fy1 = y1;
				fx2 = x;
				fy2 = y;
				fx = x;
				fy = y;
			} while (xs_outline_SVGPath_test(the, parser));
		}
		else if ((token == 'T') || (token == 't'))  {
			do {
				xsNumberValue x1, y1, x, y;
				x1 = 2 * fx - fx1;
				y1 = 2 * fy - fy1;
				x = xs_outline_SVGPath_number(the, parser);
				xs_outline_SVGPath_test(the, parser);
				y = xs_outline_SVGPath_number(the, parser);
				if (token == 't') {
					x += fx;
					y += fy;
				}
				xs_outline_subpath_push2(the, x1*64, y1*64, x*64, y*64);
				fx1 = x1;
				fy1 = y1;
				fx2 = x;
				fy2 = y;
				fx = x;
				fy = y;
			} while (xs_outline_SVGPath_test(the, parser));
		}
		else if ((token == 'C') || (token == 'c'))  {
			do {
				xsNumberValue x1, y1, x2, y2, x, y;
				x1 = xs_outline_SVGPath_number(the, parser);
				xs_outline_SVGPath_test(the, parser);
				y1 = xs_outline_SVGPath_number(the, parser);
				xs_outline_SVGPath_test(the, parser);
				x2 = xs_outline_SVGPath_number(the, parser);
				xs_outline_SVGPath_test(the, parser);
				y2 = xs_outline_SVGPath_number(the, parser);
				xs_outline_SVGPath_test(the, parser);
				x = xs_outline_SVGPath_number(the, parser);
				xs_outline_SVGPath_test(the, parser);
				y = xs_outline_SVGPath_number(the, parser);
				if (token == 'c') {
					x1 += fx;
					y1 += fy;
					x2 += fx;
					y2 += fy;
					x += fx;
					y += fy;
				}
				xs_outline_subpath_push3(the, x1*64, y1*64, x2*64, y2*64, x*64, y*64);
				fx2 = x2;
				fy2 = y2;
				fx1 = x;
				fy1 = y;
				fx = x;
				fy = y;
			} while (xs_outline_SVGPath_test(the, parser));
		}
		else if ((token == 'S') || (token == 's'))  {
			do {
				xsNumberValue x1, y1, x2, y2, x, y;
				x1 = 2 * fx - fx2;
				y1 = 2 * fy - fy2;
				x2 = xs_outline_SVGPath_number(the, parser);
				xs_outline_SVGPath_test(the, parser);
				y2 = xs_outline_SVGPath_number(the, parser);
				xs_outline_SVGPath_test(the, parser);
				x = xs_outline_SVGPath_number(the, parser);
				xs_outline_SVGPath_test(the, parser);
				y = xs_outline_SVGPath_number(the, parser);
				if (token == 's') {
					x2 += fx;
					y2 += fy;
					x += fx;
					y += fy;
				}
				xs_outline_subpath_push3(the, x1*64, y1*64, x2*64, y2*64, x*64, y*64);
				fx2 = x2;
				fy2 = y2;
				fx1 = x;
				fy1 = y;
				fx = x;
				fy = y;
			} while (xs_outline_SVGPath_test(the, parser));
		}
		else if ((token == 'A') || (token == 'a'))  {
			do {
				xsNumberValue rx, ry, a, fa, fs, x, y;
				xsNumberValue sina, cosa, tx, ty, px, py, rxs, rys, pxs, pys, q, pcx, pcy, cx, cy, ux, uy, vx, vy, theta, delta;
				xsIntegerValue c, i;
				xsNumberValue matrix[6];
				xsNumberValue points[4 * 8];
				xsNumberValue* p = points;

				rx = xs_outline_SVGPath_number(the, parser);
				xs_outline_SVGPath_test(the, parser);
				ry = xs_outline_SVGPath_number(the, parser);
				xs_outline_SVGPath_test(the, parser);
				a = xs_outline_SVGPath_number(the, parser);
				xs_outline_SVGPath_test(the, parser);
				fa = xs_outline_SVGPath_number(the, parser);
				xs_outline_SVGPath_test(the, parser);
				fs = xs_outline_SVGPath_number(the, parser);
				xs_outline_SVGPath_test(the, parser);
				x = xs_outline_SVGPath_number(the, parser);
				xs_outline_SVGPath_test(the, parser);
				y = xs_outline_SVGPath_number(the, parser);
				if (token == 'a') {
					x += fx;
					y += fy;
				}
				
				rx = c_fabs(rx);
				ry = c_fabs(ry);
				if (rx == 0 || ry == 0)
					goto skip;
			
				a *= 2 * C_M_PI / 360;
				sina = c_sin(a);
				cosa = c_cos(a);
				
   				//(F.6.5.1)
   				tx = (fx - x) / 2;
   				ty = (fy - y) / 2;
				px = (cosa * tx) + (sina * ty);
				py = (-sina * tx) + (cosa * ty);
				if (px == 0 && py == 0)
					goto skip;

    			//(F.6.5.2)
    			rxs = rx * rx;
				rys = ry * ry;
				pxs = px * px;
				pys = py * py;
				q = (pxs / rxs) + (pys / rys);
				if (q > 1) {
					q = c_sqrt(q);
					rx *= q;
					ry *= q;
					rxs = rx * rx;
					rys = ry * ry;
				}
				q = (rxs * pys) + (rys * pxs);
				q = ((rxs * rys) - q) / q;
				if (q < 0) q = 0;
				q = c_sqrt(q);
				if (fa == fs)
					q = -q;
				pcx = q * rx * py / ry;
				pcy = -q * ry * px /rx;

    			//(F.6.5.3)
				cx = (cosa * pcx) - (sina * pcy) + ((fx + x) / 2);
				cy = (sina * pcx) + (cosa * pcy) + ((fy + y) / 2);

   				//(F.6.5.5)
				ux = (px - pcx) / rx;
				uy = (py - pcy) / ry;
				theta = xs_outline_vectorsToAngle(1, 0, ux, uy);
				
	    		//(F.6.5.6)
				vx = (-px - pcx) / rx;
				vy = (-py - pcy) / ry;
				delta = xs_outline_vectorsToAngle(ux, uy, vx, vy);
				if ((fs == 0) && (delta > 0))
					delta -= 2 * C_M_PI;
				if ((fs == 1) && (delta < 0))
					delta += 2 * C_M_PI;
				
				matrix[0] = cosa * rx;
				matrix[1] = -sina * ry;
				matrix[2] = cx;
				matrix[3] = sina * rx;
				matrix[4] = cosa * ry;
				matrix[5] = cy;
				c = xs_outline_arcToCubic(the, theta, delta, matrix, points);
				if (c) {
					for (i = 0; i < c; i++) {
						xs_outline_subpath_push3(the, p[2]*64, p[3]*64, p[4]*64, p[5]*64, p[6]*64, p[7]*64);
						p += 8;
					}
				}
			skip:
				fx1 = x;
				fy1 = y;
				fx2 = x;
				fy2 = y;
				fx = x;
				fy = y;
			} while (xs_outline_SVGPath_test(the, parser));
		}
		else
			xsSyntaxError("invalid command %c", token);
	}
}

