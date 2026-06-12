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

static void PiuTextureMark(xsMachine* the, void* it, xsMarkRoot markRoot);

static xsHostHooks PiuTextureHooks = {
	PiuTextureDelete,
	PiuTextureMark,
	NULL
};

static GColor gBitmapGray4Palette[4] = {
	{ argb:0xC0 }, { argb:0x80 }, { argb:0x40 }, { argb:0x00 },
};

void PiuTextureCreate(xsMachine* the) 
{
	PiuTextureRecord record;
	PiuTexture self = &record;
	c_memset(self, 0, sizeof(record));
	self->reference = xsToReference(xsThis);
	if (xsTypeOf(xsArg(0)) == xsIntegerType) {
		int id = xsToInteger(xsArg(0));
		GBitmap *bitmap = gbitmap_create_with_resource(id);
		if (!bitmap)
			xsUnknownError("texture not found");
		self->gbitmap = bitmap;
		self->width = bitmap->bounds.size.w;
		self->height = bitmap->bounds.size.h;
	}
	else {
		if (xsTest(xsArg(0))) {
			CommodettoBitmap cb = xsGetHostChunk(xsArg(0));
			self->flags |= piuTextureAlpha;
			self->mask.addr = cb->bits.data;
			self->mask.info.version = GBITMAP_VERSION_1;
			self->mask.bounds = GRect(0, 0, cb->w, cb->h);
			self->width = cb->w;
			self->height = cb->h;
			if (cb->format == kCommodettoBitmapMonochromeAligned) {
				self->mask.row_size_bytes = ((cb->w + 31) >> 5) * 4;
				self->mask.info.format = GBitmapFormat1Bit;
			}
			else if (cb->format == kCommodettoBitmapGray4) {
				self->mask.row_size_bytes = (cb->w + 3) >> 2;
				self->mask.info.format = GBitmapFormat2BitPalette;
				self->mask.palette = gBitmapGray4Palette;
			}
			else
				xsUnknownError("invalid texture format");
		}
		if (xsTest(xsArg(1))) {
			CommodettoBitmap cb = xsGetHostChunk(xsArg(1));
			self->flags |= piuTextureColor;
			self->bits.addr = cb->bits.data;
			self->bits.info.version = GBITMAP_VERSION_1;
			self->bits.bounds = GRect(0, 0, cb->w, cb->h);
			self->width = cb->w;
			self->height = cb->h;
			if (cb->format == kCommodettoBitmapMonochromeAligned) {
				self->bits.row_size_bytes = ((cb->w + 31) >> 5) * 4;
				self->bits.info.format = GBitmapFormat1Bit;
			}
			else if (cb->format == kCommodettoBitmapARGB2222) {
				self->bits.row_size_bytes = cb->w;
				self->bits.info.format = GBitmapFormat8Bit;
			}
			else
				xsUnknownError("invalid texture format");
		}
	}
	xsSetHostChunk(xsThis, self, sizeof(record));
	xsSetHostHooks(xsThis, &PiuTextureHooks);
	xsResult = xsThis;
}

void PiuTextureDelete(void* it)
{
	if (it) {
		PiuTexture self = it;
		if (self->gbitmap) {
			gbitmap_destroy(self->gbitmap);
			self->gbitmap = C_NULL;
		}
	}
}

void PiuTextureMark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
}

void PiuTexture_get_width(xsMachine* the)
{
	PiuTexture* self = PIU(Texture, xsThis);
	xsResult = xsPiuDimension((*self)->width);
}

void PiuTexture_get_height(xsMachine* the)
{
	PiuTexture* self = PIU(Texture, xsThis);
	xsResult = xsPiuDimension((*self)->height);
}
