/*
 * Copyright (c) 2025  Moddable Tech, Inc.
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

 #include "piuPebble.h"

typedef struct PiuSVGImageStruct PiuSVGImageRecord, *PiuSVGImage;
struct PiuSVGImageStruct {
	PiuHandlePart;
	PiuIdlePart;
	PiuBehaviorPart;
	PiuContentPart;
	xsIntegerValue id;
	xsSlot* path;
	GDrawCommandFrame* dcf;
	GDrawCommandImage* dci;
	GDrawCommandList* dcl;
	GDrawCommandSequence* dcs;
	PiuDimension dataWidth;
	PiuDimension dataHeight;
	xsNumberValue o;
	xsNumberValue cx;
	xsNumberValue cy;
	xsNumberValue r;
	xsNumberValue sx;
	xsNumberValue sy;
	xsNumberValue tx;
	xsNumberValue ty;
	xsBooleanValue transforming;
};

static void PiuSVGImageBind(void* it, PiuApplication* application, PiuView* view);
static void PiuSVGImageDelete(void* it);
static void PiuSVGImageDictionary(xsMachine* the, void* it);
static void PiuSVGImageDraw(void* it, PiuView* view, PiuRectangle area);
static void PiuSVGImageDrawAux(void* it, PiuView* view, PiuCoordinate x, PiuCoordinate y, PiuDimension sw, PiuDimension sh);
static void PiuSVGImageInvalidate(void* it, PiuRectangle area);
static void PiuSVGImageMark(xsMachine* the, void* it, xsMarkRoot markRoot);
static void PiuSVGImageMeasureHorizontally(void* it);
static void PiuSVGImageMeasureVertically(void* it);
static void PiuSVGImageSync(void* it);
static void PiuSVGImageUnbind(void* it, PiuApplication* application, PiuView* view);

const PiuDispatchRecord ICACHE_FLASH_ATTR PiuSVGImageDispatchRecord = {
	"SVGImage",
	PiuSVGImageBind,
	PiuContentCascade,
	PiuSVGImageDraw,
	PiuContentFitHorizontally,
	PiuContentFitVertically,
	PiuContentHit,
	PiuContentIdle,
	PiuSVGImageInvalidate,
	PiuSVGImageMeasureHorizontally,
	PiuSVGImageMeasureVertically,
	PiuContentPlace,
	NULL,
	NULL,
	PiuContentReflow,
	PiuContentShowing,
	PiuContentShown,
	PiuSVGImageSync,
	PiuSVGImageUnbind,
	PiuContentUpdate
};

const xsHostHooks ICACHE_FLASH_ATTR PiuSVGImageHooks = {
	PiuSVGImageDelete,
	PiuSVGImageMark,
	NULL
};

mxImport xsIntegerValue _xsmcGetBuffer(xsMachine *the, xsSlot *slot, void **data, xsUnsignedValue *count, xsBooleanValue writable);

void PiuSVGImageBind(void* it, PiuApplication* application, PiuView* view)
{
	PiuSVGImage* self = it;
	xsBeginHost((*self)->the);
	xsVars(2);
	GDrawCommandImage *dci = NULL;
	GDrawCommandSequence *dcs = NULL;
	if ((*self)->id) {
		dci = gdraw_command_image_create_with_resource((*self)->id);
		if (dci == NULL)
			dcs = gdraw_command_sequence_create_with_resource((*self)->id);
	}
	else if ((*self)->path) {
		void* data;
		xsUnsignedValue count;
		xsVar(0) = *((*self)->path);
		xsVar(0) = xsNew1(xsGlobal, xsID_Resource, xsVar(0));
		_xsmcGetBuffer(the, &xsVar(0), &data, &count, 0);
  		dci = (GDrawCommandImage *)(((uint8_t*)data) + PDCI_DATA_OFFSET);
		if (!gdraw_command_image_validate(dci, count - PDCI_DATA_OFFSET)) {
			dci = NULL;
			dcs = (GDrawCommandSequence *)(((uint8_t*)data) + PDCS_DATA_OFFSET);
			if (!gdraw_command_sequence_validate(dcs, count - PDCS_DATA_OFFSET)) {
				dcs = NULL;
			}
		}
	}
	if (dci) {
		(*self)->dci = dci;
		GSize size = gdraw_command_image_get_bounds_size(dci);
		(*self)->dataWidth = size.w;
		(*self)->dataHeight = size.h;
	}
	else if (dcs) {
		(*self)->dcs = dcs;
		GSize size = gdraw_command_sequence_get_bounds_size(dcs);
		(*self)->dataWidth = size.w;
		(*self)->dataHeight = size.h;
		uint32_t duration = 0;
		uint32_t c = gdraw_command_sequence_get_num_frames(dcs);
		for (uint32_t i = 0; i < c; i++) {
			GDrawCommandFrame* dcf = gdraw_command_sequence_get_frame_by_index(dcs, 0);
			duration += gdraw_command_frame_get_duration(dcf);
		}
		PiuIdle selfIdle = PiuContentUseIdle(it);
		selfIdle->duration = duration;
		(*self)->dcf = gdraw_command_sequence_get_frame_by_index(dcs, 0);
	}
	(*self)->cx = (xsNumberValue)(*self)->dataWidth / 2.0;
	(*self)->cy = (xsNumberValue)(*self)->dataHeight / 2.0;
	(*self)->sx = (*self)->sy = 1;
	(*self)->o = 1;
	(*self)->transforming = 1;
	xsEndHost((*self)->the);
	PiuContentBind(it, application, view);
}

void PiuSVGImageDelete(void* it)
{
	PiuSVGImage self = it;
	if (self->id) {
		if (self->dci) {
			gdraw_command_image_destroy(self->dci);
			self->dci = NULL;
		}
		if (self->dcs) {
			gdraw_command_sequence_destroy(self->dcs);
			self->dcs = NULL;
			self->dcf = NULL;
		}
	}
	if (self->dcl) {
		gdraw_command_list_destroy(self->dcl);
		self->dcl = NULL;
	}
}

void PiuSVGImageDictionary(xsMachine* the, void* it) 
{
	PiuSVGImage* self = it;
	xsBooleanValue boolean;
	if (xsFindBoolean(xsArg(1), xsID_clip, &boolean)) {
		if (boolean)
			(*self)->flags |= piuClip;
		else
			(*self)->flags &= ~piuClip;
	}
	if (xsFindResult(xsArg(1), xsID_path)) {
		if (xsTypeOf(xsResult) == xsIntegerType) {
			(*self)->id = xsToInteger(xsResult);
		}
		else {
			xsSlot* path = PiuString(xsResult);
			(*self)->path = path;
		}
	}
}

void PiuSVGImageDraw(void* it, PiuView* view, PiuRectangle area) 
{
	PiuSVGImage* self = it;
	PiuContentDraw(it, view, area);
	if ((*self)->dci || (*self)->dcs) {
		PiuRectangleRecord bounds;
		PiuRectangleSet(&bounds, 0, 0, (*self)->bounds.width, (*self)->bounds.height);
		if ((*self)->flags & piuClip)
			PiuViewPushClip(view, 0, 0, bounds.width, bounds.height);
		PiuViewDrawContent(view, PiuSVGImageDrawAux, it, 0, 0, bounds.width, bounds.height);
		if ((*self)->flags & piuClip)
			PiuViewPopClip(view);
	}
}

extern GContext *getPocoPebbleGContext(Poco poco);

typedef struct {
	xsIntegerValue cx;
	xsIntegerValue cy;
	xsIntegerValue c0r0;
	xsIntegerValue c0r1;
	xsIntegerValue c1r0;
	xsIntegerValue c1r1;
	xsIntegerValue tx;
	xsIntegerValue ty;
	xsIntegerValue s;
} Transform;

static bool doTransform(GDrawCommand *command, uint32_t index, void *context)
{
	Transform* t = context;
	const uint16_t c = gdraw_command_get_num_points(command);
	for (uint16_t i = 0; i < c; i++) {
		GPoint p = gdraw_command_get_point(command, i);
		xsIntegerValue x = p.x - t->cx, y = p.y - t->cy;
		p.x = (((x * t->c0r0) + (y * t->c0r1)) >> 8) + t->tx;
		p.y = (((y * t->c1r1) - (x * t->c1r0)) >> 8) + t->ty;
		gdraw_command_set_point(command, i, p);
	}
	uint16_t weight = gdraw_command_get_stroke_width(command);
	weight = (weight * t->s) >> 8;
	gdraw_command_set_stroke_width(command, weight);
	return true;
}

void PiuSVGImageDrawAux(void* it, PiuView* view, PiuCoordinate x, PiuCoordinate y, PiuDimension sw, PiuDimension sh)
{
	PiuSVGImage* self = it;
	xsBeginHost((*self)->the);
	GContext *ctx = (*view)->ctx;
	GPoint offset = {
		.x = c_round(x + (*self)->cx),
		.y = c_round(y + (*self)->cy),
	};
	GDrawCommandList *list = (*self)->dcl;
	if ((*self)->transforming) {
		Transform t;
		if (list)
			gdraw_command_list_destroy(list);
		if ((*self)->dci) {
			list = gdraw_command_image_get_command_list((*self)->dci);
		}
		else if ((*self)->dcs) {
			list = gdraw_command_frame_get_command_list((*self)->dcf);
		}
		list = gdraw_command_list_clone(list);
		xsNumberValue a = (*self)->r;
		xsNumberValue sx = (*self)->sx;
		xsNumberValue sy = (*self)->sy;
		xsNumberValue sina = c_sin(a);
		xsNumberValue cosa = c_cos(a);
		t.c0r0 = sx * cosa * 256;
		t.c0r1 = sy * sina * 256;
		t.c1r0 = sx * sina * 256;
		t.c1r1 = sy * cosa * 256;
		t.cx = (*self)->cx * 8;
		t.cy = (*self)->cy * 8;
		t.tx = (*self)->tx * 8;
		t.ty = (*self)->ty * 8;
		t.s = (sx + sy) * 128;
		gdraw_command_list_iterate(list, doTransform, &t);
		(*self)->dcl = list;
	}
	graphics_context_move_draw_box(ctx, offset);
	gdraw_command_list_draw(ctx, list);
	graphics_context_move_draw_box(ctx, GPoint(-offset.x, -offset.y));
	(*self)->transforming = 0;
	xsEndHost((*self)->the);
}

void PiuSVGImageInvalidate(void* it, PiuRectangle area) 
{
	PiuContent* self = it;
	if ((*self)->flags & piuVisible) {
		PiuContainer* container = (*self)->container;
		if (container) {
			PiuRectangleRecord bounds;
			if ((*self)->flags & piuClip)
				PiuRectangleSet(&bounds, (*self)->bounds.x, (*self)->bounds.y, (*self)->bounds.width, (*self)->bounds.height);
			else {
				PiuApplication* application = ((*self)->application);
				PiuRectangleSet(&bounds, (*application)->bounds.x, (*application)->bounds.y, (*application)->bounds.width, (*application)->bounds.height);
			}
			(*(*container)->dispatch->invalidate)((*self)->container, &bounds);
		}
	}
}

void PiuSVGImageMark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	PiuSVGImage self = it;
	PiuContentMark(the, it, markRoot);
	if (self->path)
		PiuMarkString(the, self->path);
}

void PiuSVGImageMeasureHorizontally(void* it) 
{
	PiuSVGImage* self = it;
	PiuAlignment horizontal = (*self)->coordinates.horizontal;
	if (!(horizontal & piuWidth))
		(*self)->coordinates.width = (*self)->dataWidth;
}

void PiuSVGImageMeasureVertically(void* it) 
{
	PiuSVGImage* self = it;
	PiuAlignment vertical = (*self)->coordinates.vertical;
	if (!(vertical & piuHeight))
		(*self)->coordinates.height = (*self)->dataHeight;
}

void PiuSVGImageSync(void* it)
{
	PiuSVGImage* self = it;
	if ((*self)->dcs) {
		double time = PiuContentUseIdle(it)->time;
		GDrawCommandFrame *dcf = gdraw_command_sequence_get_frame_by_elapsed((*self)->dcs, time);
		if ((*self)->dcf != dcf) {
			(*self)->dcf = dcf;
			(*self)->transforming = 1;
			PiuSVGImageInvalidate(self, NULL);
		}
	}	
}

void PiuSVGImageUnbind(void* it, PiuApplication* application, PiuView* view)
{
	PiuSVGImage* self = it;
	PiuContentUnbind(it, application, view);
	if ((*self)->id) {
		if ((*self)->dci) {
			gdraw_command_image_destroy((*self)->dci);
			(*self)->dci = NULL;
		}
		if ((*self)->dcs) {
			gdraw_command_sequence_destroy((*self)->dcs);
			(*self)->dcs = NULL;
			(*self)->dcf = NULL;
		}
	}
	if ((*self)->dcl) {
		gdraw_command_list_destroy((*self)->dcl);
		(*self)->dcl = NULL;
	}
}

void PiuSVGImage_create(xsMachine* the)
{
	PiuSVGImage* self;
	xsVars(4);
	xsSetHostChunk(xsThis, NULL, sizeof(PiuSVGImageRecord));
	self = PIU(SVGImage, xsThis);
	(*self)->the = the;
	(*self)->reference = xsToReference(xsThis);
	xsSetHostHooks(xsThis, (xsHostHooks*)&PiuSVGImageHooks);
	(*self)->dispatch = (PiuDispatch)&PiuSVGImageDispatchRecord;
	(*self)->recordSize = PiuRecordSize(sizeof(PiuSVGImageRecord));
	(*self)->flags = piuVisible;
	PiuContentDictionary(the, self);
	PiuSVGImageDictionary(the, self);
	PiuBehaviorOnCreate(self);
}

void PiuSVGImage_get_opacity(xsMachine* the)
{
	PiuSVGImage* self = PIU(SVGImage, xsThis);
	xsResult = xsNumber((*self)->o);
}

void PiuSVGImage_set_opacity(xsMachine* the)
{
	PiuSVGImage* self = PIU(SVGImage, xsThis);
	(*self)->o = xsToNumber(xsArg(0));
	PiuSVGImageInvalidate(self, NULL);
}

void PiuSVGImage_center(xsMachine* the)
{
	PiuSVGImage* self = PIU(SVGImage, xsThis);
	xsIntegerValue c = xsToInteger(xsArgc);
	(*self)->cx = (c > 0) ? xsToNumber(xsArg(0)) : (xsNumberValue)(*self)->dataWidth / 2.0;
	(*self)->cy = (c > 1) ? xsToNumber(xsArg(1)) : (xsNumberValue)(*self)->dataHeight / 2.0;
	(*self)->transforming = 1;
	PiuSVGImageInvalidate(self, NULL);
}

void PiuSVGImage_get_cx(xsMachine* the)
{
	PiuSVGImage* self = PIU(SVGImage, xsThis);
	xsResult = xsNumber((*self)->cx);
}

void PiuSVGImage_set_cx(xsMachine* the)
{
	PiuSVGImage* self = PIU(SVGImage, xsThis);
	(*self)->cx = xsToNumber(xsArg(0));
	(*self)->transforming = 1;
	PiuSVGImageInvalidate(self, NULL);
}

void PiuSVGImage_get_cy(xsMachine* the)
{
	PiuSVGImage* self = PIU(SVGImage, xsThis);
	xsResult = xsNumber((*self)->cy);
}

void PiuSVGImage_set_cy(xsMachine* the)
{
	PiuSVGImage* self = PIU(SVGImage, xsThis);
	(*self)->cy = xsToNumber(xsArg(0));
	(*self)->transforming = 1;
	PiuSVGImageInvalidate(self, NULL);
}

void PiuSVGImage_rotate(xsMachine* the)
{
	PiuSVGImage* self = PIU(SVGImage, xsThis);
	xsIntegerValue c = xsToInteger(xsArgc);
	(*self)->r = (c > 0) ? xsToNumber(xsArg(0)) : 0;
	(*self)->transforming = 1;
	PiuSVGImageInvalidate(self, NULL);
}

void PiuSVGImage_get_r(xsMachine* the)
{
	PiuSVGImage* self = PIU(SVGImage, xsThis);
	xsResult = xsNumber((*self)->r);
}

void PiuSVGImage_set_r(xsMachine* the)
{
	PiuSVGImage* self = PIU(SVGImage, xsThis);
	(*self)->r = xsToNumber(xsArg(0));
	(*self)->transforming = 1;
	PiuSVGImageInvalidate(self, NULL);
}

void PiuSVGImage_scale(xsMachine* the)
{
	PiuSVGImage* self = PIU(SVGImage, xsThis);
	xsIntegerValue c = xsToInteger(xsArgc);
	(*self)->sx = (c > 0) ? xsToNumber(xsArg(0)) : 1;
	(*self)->sy = (c > 1) ? xsToNumber(xsArg(1)) : (*self)->sx;
	(*self)->transforming = 1;
	PiuSVGImageInvalidate(self, NULL);
}

void PiuSVGImage_get_s(xsMachine* the)
{
	PiuSVGImage* self = PIU(SVGImage, xsThis);
	xsResult = xsNumber(((*self)->sx + (*self)->sy) / 2);
}

void PiuSVGImage_set_s(xsMachine* the)
{
	PiuSVGImage* self = PIU(SVGImage, xsThis);
	(*self)->sx = (*self)->sy = xsToNumber(xsArg(0));
	(*self)->transforming = 1;
	PiuSVGImageInvalidate(self, NULL);
}

void PiuSVGImage_get_sx(xsMachine* the)
{
	PiuSVGImage* self = PIU(SVGImage, xsThis);
	xsResult = xsNumber((*self)->sx);
}

void PiuSVGImage_set_sx(xsMachine* the)
{
	PiuSVGImage* self = PIU(SVGImage, xsThis);
	(*self)->sx = xsToNumber(xsArg(0));
	(*self)->transforming = 1;
	PiuSVGImageInvalidate(self, NULL);
}

void PiuSVGImage_get_sy(xsMachine* the)
{
	PiuSVGImage* self = PIU(SVGImage, xsThis);
	xsResult = xsNumber((*self)->sy);
}

void PiuSVGImage_set_sy(xsMachine* the)
{
	PiuSVGImage* self = PIU(SVGImage, xsThis);
	(*self)->sy = xsToNumber(xsArg(0));
	(*self)->transforming = 1;
	PiuSVGImageInvalidate(self, NULL);
}

void PiuSVGImage_translate(xsMachine* the)
{
	PiuSVGImage* self = PIU(SVGImage, xsThis);
	xsIntegerValue c = xsToInteger(xsArgc);
	(*self)->tx = (c > 0) ? xsToNumber(xsArg(0)) : 0;
	(*self)->ty = (c > 1) ? xsToNumber(xsArg(1)) : (*self)->tx;
	(*self)->transforming = 1;
	PiuSVGImageInvalidate(self, NULL);
}

void PiuSVGImage_get_tx(xsMachine* the)
{
	PiuSVGImage* self = PIU(SVGImage, xsThis);
	xsResult = xsNumber((*self)->tx);
}

void PiuSVGImage_set_tx(xsMachine* the)
{
	PiuSVGImage* self = PIU(SVGImage, xsThis);
	(*self)->tx = xsToNumber(xsArg(0));
	(*self)->transforming = 1;
	PiuSVGImageInvalidate(self, NULL);
}

void PiuSVGImage_get_ty(xsMachine* the)
{
	PiuSVGImage* self = PIU(SVGImage, xsThis);
	xsResult = xsNumber((*self)->ty);
}

void PiuSVGImage_set_ty(xsMachine* the)
{
	PiuSVGImage* self = PIU(SVGImage, xsThis);
	(*self)->ty = xsToNumber(xsArg(0));
	(*self)->transforming = 1;
	PiuSVGImageInvalidate(self, NULL);
}


