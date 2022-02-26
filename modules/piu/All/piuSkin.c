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

static void PiuSkinDrawAux(PiuSkin* self, PiuView* view, PiuRectangle bounds, PiuVariant variant, PiuState state, PiuAlignment horizontal, PiuAlignment vertical);
static void PiuSkinMark(xsMachine* the, void* it, xsMarkRoot markRoot);

static const xsHostHooks PiuSkinHooks ICACHE_RODATA_ATTR = {
	PiuSkinDelete,
	PiuSkinMark,
	NULL
};

void PiuSkinCreate(xsMachine* the) 
{
	PiuSkinRecord record;
	PiuSkin self = &record;
	xsIntegerValue c = xsToInteger(xsArgc);
	xsIntegerValue integer;
	xsVars(4);
	c_memset(self, 0, sizeof(record));
	self->reference = xsToReference(xsThis);
	if ((c > 0) && (xsTest(xsArg(0)))) {
		if (xsFindResult(xsArg(0), xsID_Texture)) {
			self->flags |= piuSkinPattern;
			if (!xsIsInstanceOf(xsResult, xsFunctionPrototype))
				xsUnknownError("Texture is no function");
			xsResult = xsCallFunction0(xsResult, xsNull);
			xsVar(0) = xsGet(xsGlobal, xsID_Texture);
			xsVar(1) = xsGet(xsVar(0), xsID_prototype);
			if (!xsIsInstanceOf(xsResult, xsVar(1)))
				xsUnknownError("Texture is no texture template");
			self->data.pattern.texture = PIU(Texture, xsResult);
		}
		else if (xsFindResult(xsArg(0), xsID_texture)) {
			self->flags |= piuSkinPattern;
			if (!xsIsInstanceOf(xsResult, xsObjectPrototype))
				xsUnknownError("texture is no object");
			xsVar(0) = xsGet(xsGlobal, xsID_Texture);
			xsVar(1) = xsGet(xsVar(0), xsID_prototype);
			if (xsIsInstanceOf(xsResult, xsVar(1)))
				self->data.pattern.texture = PIU(Texture, xsResult);
			else {
				xsVar(2) = xsGet(xsGlobal, xsID_assetMap);
				if (xsTest(xsVar(2)))
					xsVar(3) = xsCall1(xsVar(2), xsID_get, xsResult);
				else {
					xsVar(2) = xsNew0(xsGlobal, xsID_Map);
					xsSet(xsGlobal, xsID_assetMap, xsVar(2));
					xsVar(3) = xsUndefined;
				}
				if (xsTest(xsVar(3))) {
					if (!xsIsInstanceOf(xsVar(3), xsVar(1)))
						xsUnknownError("texture is no texture dictionary");
				}
				else {
					xsVar(3) = xsNewFunction1(xsVar(0), xsResult);
					xsCall2(xsVar(2), xsID_set, xsResult, xsVar(3));
				}
				self->data.pattern.texture = PIU(Texture, xsVar(3));
			}
		}
		if (self->flags & piuSkinPattern) {
			if (xsFindInteger(xsArg(0), xsID_x, &integer))
				self->data.pattern.bounds.x = (PiuCoordinate)integer;
			if (xsFindInteger(xsArg(0), xsID_y, &integer))
				self->data.pattern.bounds.y = (PiuCoordinate)integer;
			if (xsFindInteger(xsArg(0), xsID_width, &integer))
				self->data.pattern.bounds.width = (PiuDimension)integer;
			if (xsFindInteger(xsArg(0), xsID_height, &integer))
				self->data.pattern.bounds.height = (PiuDimension)integer;
			if (xsFindInteger(xsArg(0), xsID_variants, &integer))
				self->data.pattern.delta.x = (PiuCoordinate)integer;
			if (xsFindInteger(xsArg(0), xsID_states, &integer))
				self->data.pattern.delta.y = (PiuCoordinate)integer;
			if (!xsFindResult(xsArg(0), xsID_tiles))
				xsResult = xsArg(0);
			if (xsFindInteger(xsResult, xsID_left, &integer)) {
				self->flags |= piuRepeatX;
				self->data.pattern.tiles.left = (PiuCoordinate)integer;
			}
			if (xsFindInteger(xsResult, xsID_right, &integer)) {
				self->flags |= piuRepeatX;
				self->data.pattern.tiles.right = (PiuCoordinate)integer;
			}
			if (xsFindInteger(xsResult, xsID_top, &integer)) {
				self->flags |= piuRepeatY;
				self->data.pattern.tiles.top = (PiuCoordinate)integer;
			}
			if (xsFindInteger(xsResult, xsID_bottom, &integer)) {
				self->flags |= piuRepeatY;
				self->data.pattern.tiles.bottom = (PiuCoordinate)integer;
			}
			if (xsFindResult(xsArg(0), xsID_color)) {
				self->flags |= piuSkinColorized;
				PiuColorsDictionary(the, &xsResult, self->data.pattern.color);
			}
		}
		else {
			if (!xsFindResult(xsArg(0), xsID_borders))
				xsResult = xsArg(0);
			if (xsFindInteger(xsResult, xsID_left, &integer))
				self->data.color.borders.left = (PiuCoordinate)integer;
			if (xsFindInteger(xsResult, xsID_right, &integer))
				self->data.color.borders.right = (PiuCoordinate)integer;
			if (xsFindInteger(xsResult, xsID_top, &integer))
				self->data.color.borders.top = (PiuCoordinate)integer;
			if (xsFindInteger(xsResult, xsID_bottom, &integer))
				self->data.color.borders.bottom = (PiuCoordinate)integer;
			if (xsFindResult(xsArg(0), xsID_fill))
				PiuColorsDictionary(the, &xsResult, self->data.color.fill);
			if (xsFindResult(xsArg(0), xsID_stroke))
				PiuColorsDictionary(the, &xsResult, self->data.color.stroke);
		}
	}
	xsSetHostChunk(xsThis, self, sizeof(record));
	xsSetHostHooks(xsThis, &PiuSkinHooks);
	xsResult = xsThis;
}

void PiuSkinDelete(void* it) 
{
}

void PiuSkinDraw(PiuSkin* self, PiuView* view, PiuRectangle bounds, PiuVariant variant, PiuState state, PiuAlignment horizontal, PiuAlignment vertical)
{
	PiuFlags flags = (*self)->flags;
	PiuColorRecord color;
	if (state < 0) state = 0;
	else if (3 < state) state = 3;
	if (flags & piuSkinPattern) {
		PiuState index = c_floor(state);
		if (flags & piuSkinColorized) {
			PiuColorsBlend((*self)->data.pattern.color, state, &color);
			PiuViewPushColorFilter(view, &color);
			PiuSkinDrawAux(self, view, bounds, variant, index, horizontal, vertical);
			PiuViewPopColorFilter(view);
		}
		else {
			color.r = color.g = color.b = 0;
			color.a = 255;
			PiuViewPushColor(view, &color);
			PiuSkinDrawAux(self, view, bounds, variant, index, horizontal, vertical);
			PiuViewPopColor(view);
			if (index < state) {
				color.a = (uint8_t)c_round((state - index) * 255.0);
				PiuViewPushColor(view, &color);
				PiuSkinDrawAux(self, view, bounds, variant, index + 1, horizontal, vertical);
				PiuViewPopColor(view);
			}
		}
	}
	else {
		PiuCoordinate x = bounds->x;
		PiuCoordinate y = bounds->y;
		PiuCoordinate dx = bounds->width;
		PiuCoordinate dy = bounds->height;
		PiuCoordinate dl = (*self)->data.color.borders.left;
		PiuCoordinate dr = (*self)->data.color.borders.right;
		PiuCoordinate dt = (*self)->data.color.borders.top;
		PiuCoordinate db = (*self)->data.color.borders.bottom;
		PiuColorsBlend((*self)->data.color.fill, state, &color);
		PiuViewPushColor(view, &color);
		PiuViewFillColor(view, x + dl, y + dt, dx - dl - dr, dy - dt - db);
		PiuViewPopColor(view);
		if (dl || dr || dt || db) {
			PiuColorsBlend((*self)->data.color.stroke, state, &color);
			PiuViewPushColor(view, &color);
			if (dt)
				PiuViewFillColor(view, x + dl, y, dx - dl, dt);
			if (dr)
				PiuViewFillColor(view, x + dx - dr, y + dt, dr, dy - dt);
			if (db)
				PiuViewFillColor(view, x, y + dy - db, dx - dr, db);
			if (dl)
				PiuViewFillColor(view, x, y, dl, dy - db);
			PiuViewPopColor(view);
		}
	}
}

void PiuSkinDrawAux(PiuSkin* self, PiuView* view, PiuRectangle bounds, PiuVariant variant, PiuState state, PiuAlignment horizontal, PiuAlignment vertical)
{
	PiuFlags flags = (*self)->flags;
	PiuTexture* texture = (*self)->data.pattern.texture;
	PiuCoordinate x = bounds->x;
	PiuCoordinate y = bounds->y;
	PiuCoordinate dx = bounds->width;
	PiuCoordinate dy = bounds->height;
	PiuCoordinate u = (*self)->data.pattern.bounds.x + ((*self)->data.pattern.delta.x * variant);
	PiuCoordinate v = (*self)->data.pattern.bounds.y + ((*self)->data.pattern.delta.y * (PiuCoordinate)state);
	PiuCoordinate du = (*self)->data.pattern.bounds.width;
	PiuCoordinate dv = (*self)->data.pattern.bounds.height;
	PiuCoordinate dl = (*self)->data.pattern.tiles.left;
	PiuCoordinate dr = (*self)->data.pattern.tiles.right;
	PiuCoordinate dt = (*self)->data.pattern.tiles.top;
	PiuCoordinate db = (*self)->data.pattern.tiles.bottom;
	if (flags & piuRepeatX) {
		if (flags & piuRepeatY) {
			PiuViewDrawTexture(view, texture, x, y, u, v, dl, dt);
			PiuViewFillTexture(view, texture, x + dl, y, dx - dl - dr, dt, u + dl, v, du - dl - dr, dt);
			PiuViewDrawTexture(view, texture, x + dx - dr, y, u + du - dr, v, dr, dt);
			PiuViewFillTexture(view, texture, x + dx - dr, y + dt, dr, dy - dt - db, u + du - dr, v + dt, dr, dv - dt - db);
			PiuViewDrawTexture(view, texture, x + dx - dr, y + dy - db, u + du - dr, v + dv - db, dr, db);
			PiuViewFillTexture(view, texture, x + dl, y + dy - db, dx - dl - dr, db, u + dl, v + dv - db, du - dl - dr, db);
			PiuViewDrawTexture(view, texture, x, y + dy - db, u, v + dv - db, dl, db);
			PiuViewFillTexture(view, texture, x, y + dt, dl, dy - dt - db, u, v + dt, dl, dv - dt - db);
			PiuViewFillTexture(view, texture, x + dl, y + dt, dx - dl - dr, dy - dt - db, u + dl, v + dt, du - dl - dr, dv - dt - db);
		}
		else {
			switch (vertical & piuTopBottom) {
			case piuTop: break;
			case piuBottom: y += dy - dv; break;
#ifdef piuPC
			default: y += ((dy - dv) / 2); break;
#else
			default: y += ((dy - dv + 1) >> 1); break;
#endif			
			}
			PiuViewDrawTexture(view, texture, x, y, u, v, dl, dv);
			PiuViewFillTexture(view, texture, x + dl, y, dx - dl - dr, dv, u + dl, v, du - dl - dr, dv);
			PiuViewDrawTexture(view, texture, x + dx - dr, y, u + du - dr, v, dr, dv);
		}
	}
	else {
		if (flags & piuRepeatY) {
			switch (horizontal & piuLeftRight) {
			case piuLeft: break;
			case piuRight: x += dx - du; break;
#ifdef piuPC
			default: x += ((dx - du) / 2); break;
#else
			default: x += ((dx - du + 1) >> 1); break;
#endif			
			}
			PiuViewDrawTexture(view, texture, x, y, u, v, du, dt);
			PiuViewFillTexture(view, texture, x, y + dt, du, dy - dt - db, u, v + dt, du, dv - dt - db);
			PiuViewDrawTexture(view, texture, x, y + dy - db, u, v + dv - db, du, db);
		}
		else {
			switch (horizontal & piuLeftRight) {
			case piuLeft: break;
			case piuRight: x += dx - du; break;
#ifdef piuPC
			default: x += ((dx - du) / 2); break;
#else
			default: x += ((dx - du + 1) >> 1); break;
#endif			
			}
			switch (vertical & piuTopBottom) {
			case piuTop: break;
			case piuBottom: y += dy - dv; break;
#ifdef piuPC
			default: y += ((dy - dv) / 2); break;
#else
			default: y += ((dy - dv + 1) >> 1); break;
#endif			
			}
			PiuViewDrawTexture(view, texture, x, y, u, v, du, dv);
		}
	}
}

#ifdef piuGPU
void PiuSkinBind(PiuSkin* self, PiuApplication* application, PiuView* view)
{
	if ((*self)->flags & piuSkinPattern) {
		PiuTexture* texture = (*self)->data.pattern.texture;
		if (texture)
			PiuTextureBind(texture, application, view);
	}
}

void PiuSkinUnbind(PiuSkin* self, PiuApplication* application, PiuView* view)
{
	if ((*self)->flags & piuSkinPattern) {
		PiuTexture* texture = (*self)->data.pattern.texture;
		if (texture)
			PiuTextureUnbind(texture, application, view);
	}
}
#endif			

PiuDimension PiuSkinGetWidth(PiuSkin* self)
{
	PiuFlags flags = (*self)->flags;
	PiuDimension result = 0;;
	if (flags & piuSkinPattern) {
		if (!(flags & piuRepeatX))
			result = (*self)->data.pattern.bounds.width;
	}
	return result;
}

PiuDimension PiuSkinGetHeight(PiuSkin* self)
{
	PiuFlags flags = (*self)->flags;
	PiuDimension result = 0;;
	if (flags & piuSkinPattern) {
		if (!(flags & piuRepeatY))
			result = (*self)->data.pattern.bounds.height;
	}
	return result;
}

void PiuSkinMark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	PiuSkin self = it;
	if (self->flags & piuSkinPattern)
		PiuMarkHandle(the, self->data.pattern.texture);
}

void PiuSkin_get_borders(xsMachine* the)
{
	PiuSkin* self = PIU(Skin, xsThis);
	PiuFlags flags = (*self)->flags;
	if (!(flags & piuSkinPattern)) {
		xsResult = xsNewObject();
		xsDefine(xsResult, xsID_left, xsPiuCoordinate((*self)->data.color.borders.left), xsDefault);
		xsDefine(xsResult, xsID_right, xsPiuCoordinate((*self)->data.color.borders.right), xsDefault);
		xsDefine(xsResult, xsID_top, xsPiuCoordinate((*self)->data.color.borders.top), xsDefault);
		xsDefine(xsResult, xsID_bottom, xsPiuCoordinate((*self)->data.color.borders.bottom), xsDefault);
	}
}

void PiuSkin_get_bottom(xsMachine* the)
{
	PiuSkin* self = PIU(Skin, xsThis);
	PiuFlags flags = (*self)->flags;
	if (flags & piuSkinPattern) {
		if (flags & piuRepeatY)
			xsResult = xsPiuCoordinate((*self)->data.pattern.tiles.bottom);
	}
	else
		xsResult = xsPiuCoordinate((*self)->data.color.borders.bottom);
}

void PiuSkin_get_bounds(xsMachine* the)
{
	PiuSkin* self = PIU(Skin, xsThis);
	PiuFlags flags = (*self)->flags;
	if (flags & piuSkinPattern) {
		xsResult = xsNewObject();
		xsDefine(xsResult, xsID_x, xsPiuCoordinate((*self)->data.pattern.bounds.x), xsDefault);
		xsDefine(xsResult, xsID_y, xsPiuCoordinate((*self)->data.pattern.bounds.y), xsDefault);
		xsDefine(xsResult, xsID_width, xsPiuDimension((*self)->data.pattern.bounds.width), xsDefault);
		xsDefine(xsResult, xsID_height, xsPiuDimension((*self)->data.pattern.bounds.height), xsDefault);
	}
}

void PiuSkin_get_color(xsMachine* the)
{
	PiuSkin* self = PIU(Skin, xsThis);
	PiuFlags flags = (*self)->flags;
    xsVars(1);
	if (flags & piuSkinPattern)
		PiuColorsSerialize(the, (*self)->data.pattern.color);
}

void PiuSkin_get_fill(xsMachine* the)
{
	PiuSkin* self = PIU(Skin, xsThis);
	PiuFlags flags = (*self)->flags;
    xsVars(1);
	if (!(flags & piuSkinPattern))
		PiuColorsSerialize(the, (*self)->data.color.fill);
}

void PiuSkin_get_left(xsMachine* the)
{
	PiuSkin* self = PIU(Skin, xsThis);
	PiuFlags flags = (*self)->flags;
	if (flags & piuSkinPattern) {
		if (flags & piuRepeatX)
			xsResult = xsPiuCoordinate((*self)->data.pattern.tiles.left);
	}
	else
		xsResult = xsPiuCoordinate((*self)->data.color.borders.left);
}

void PiuSkin_get_right(xsMachine* the)
{
	PiuSkin* self = PIU(Skin, xsThis);
	PiuFlags flags = (*self)->flags;
	if (flags & piuSkinPattern) {
		if (flags & piuRepeatX)
			xsResult = xsPiuCoordinate((*self)->data.pattern.tiles.right);
	}
	else
		xsResult = xsPiuCoordinate((*self)->data.color.borders.right);
}

void PiuSkin_get_states(xsMachine* the)
{
	PiuSkin* self = PIU(Skin, xsThis);
	PiuFlags flags = (*self)->flags;
	if (flags & piuSkinPattern)
		xsResult = xsPiuCoordinate((*self)->data.pattern.delta.y);
}

void PiuSkin_get_stroke(xsMachine* the)
{
	PiuSkin* self = PIU(Skin, xsThis);
	PiuFlags flags = (*self)->flags;
    xsVars(1);
	if (!(flags & piuSkinPattern))
		PiuColorsSerialize(the, (*self)->data.color.stroke);
}

void PiuSkin_get_texture(xsMachine* the)
{
	PiuSkin* self = PIU(Skin, xsThis);
	PiuFlags flags = (*self)->flags;
	if (flags & piuSkinPattern) {
		PiuTexture* texture = (*self)->data.pattern.texture;
		xsResult = xsReference((*((PiuHandle*)texture))->reference);
	}
}

void PiuSkin_get_tiles(xsMachine* the)
{
	PiuSkin* self = PIU(Skin, xsThis);
	PiuFlags flags = (*self)->flags;
	if (flags & piuSkinPattern) {
		xsResult = xsNewObject();
		if (flags & piuRepeatX) {
			xsDefine(xsResult, xsID_left, xsPiuCoordinate((*self)->data.pattern.tiles.left), xsDefault);
			xsDefine(xsResult, xsID_right, xsPiuCoordinate((*self)->data.pattern.tiles.right), xsDefault);
		}
		if (flags & piuRepeatY) {
			xsDefine(xsResult, xsID_top, xsPiuCoordinate((*self)->data.pattern.tiles.top), xsDefault);
			xsDefine(xsResult, xsID_bottom, xsPiuCoordinate((*self)->data.pattern.tiles.bottom), xsDefault);
		}
	}
}

void PiuSkin_get_top(xsMachine* the)
{
	PiuSkin* self = PIU(Skin, xsThis);
	PiuFlags flags = (*self)->flags;
	if (flags & piuSkinPattern) {
		if (flags & piuRepeatY)
			xsResult = xsPiuCoordinate((*self)->data.pattern.tiles.top);
	}
	else
		xsResult = xsPiuCoordinate((*self)->data.color.borders.top);
}

void PiuSkin_get_variants(xsMachine* the)
{
	PiuSkin* self = PIU(Skin, xsThis);
	PiuFlags flags = (*self)->flags;
	if (flags & piuSkinPattern)
		xsResult = xsPiuCoordinate((*self)->data.pattern.delta.x);
}

void PiuSkin_get_x(xsMachine *the)
{
	PiuSkin* self = PIU(Skin, xsThis);
	PiuFlags flags = (*self)->flags;
	if (flags & piuSkinPattern)
		xsResult = xsPiuCoordinate((*self)->data.pattern.bounds.x);
}

void PiuSkin_get_y(xsMachine *the)
{
	PiuSkin* self = PIU(Skin, xsThis);
	PiuFlags flags = (*self)->flags;
	if (flags & piuSkinPattern)
		xsResult = xsPiuCoordinate((*self)->data.pattern.bounds.y);
}

void PiuSkin_get_width(xsMachine *the)
{
	PiuSkin* self = PIU(Skin, xsThis);
	PiuFlags flags = (*self)->flags;
	if (flags & piuSkinPattern)
		xsResult = xsPiuDimension((*self)->data.pattern.bounds.width);
}

void PiuSkin_get_height(xsMachine *the)
{
	PiuSkin* self = PIU(Skin, xsThis);
	PiuFlags flags = (*self)->flags;
	if (flags & piuSkinPattern)
		xsResult = xsPiuDimension((*self)->data.pattern.bounds.height);
}

