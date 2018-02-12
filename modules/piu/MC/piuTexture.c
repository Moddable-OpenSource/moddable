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

#include "piuMC.h"

static void PiuTextureMark(xsMachine* the, void* it, xsMarkRoot markRoot);

static xsHostHooks PiuTextureHoks ICACHE_RODATA_ATTR = {
	PiuTextureDelete,
	PiuTextureMark,
	NULL
};

void PiuTextureDelete(void* it)
{
}

void PiuTextureMark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
}

void PiuTexture_create(xsMachine* the) 
{
	PiuTextureRecord record;
	PiuTexture self = &record;
	c_memset(self, 0, sizeof(record));
	self->reference = xsToReference(xsThis);
	if (xsTest(xsArg(0))) {
		CommodettoBitmap cb = xsGetHostChunk(xsArg(0));
		self->flags |= piuTextureAlpha;
		self->mask.width = cb->w;
		self->mask.height = cb->h;
		self->mask.format = cb->format;
		self->mask.pixels = cb->bits.data;
		#if (90 == kPocoRotation) || (270 == kPocoRotation)
			self->width = cb->h;
			self->height = cb->w;
		#else
			self->width = cb->w;
			self->height = cb->h;
		#endif
	}
	if (xsTest(xsArg(1))) {
		CommodettoBitmap cb = xsGetHostChunk(xsArg(1));
		self->flags |= piuTextureColor;
		self->bits.width = cb->w;
		self->bits.height = cb->h;
		self->bits.format = cb->format;
		self->bits.pixels = cb->bits.data;
		#if (90 == kPocoRotation) || (270 == kPocoRotation)
			self->width = cb->h;
			self->height = cb->w;
		#else
			self->width = cb->w;
			self->height = cb->h;
		#endif
	}
	xsSetHostChunk(xsThis, self, sizeof(record));
	xsSetHostHooks(xsThis, &PiuTextureHoks);
	xsResult = xsThis;
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
