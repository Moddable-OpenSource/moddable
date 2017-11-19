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

#include "piuPC.h"

static void PiuTextureMark(xsMachine* the, void* it, xsMarkRoot markRoot);

static xsHostHooks PiuTextureHooks = {
	PiuTextureDelete,
	PiuTextureMark,
	NULL
};

void PiuTextureCreate(xsMachine* the) 
{
	PiuTextureRecord record;
	PiuTexture self = &record;
	xsNumberValue number;
	xsVars(2);
	c_memset(self, 0, sizeof(record));
	self->reference = xsToReference(xsThis);
	
	xsVar(0) = xsGet(xsArg(0), xsID_path);
	if (!xsTest(xsVar(0)))
		xsUnknownError("no path");
	xsVar(1) = xsGet(xsArg(0), xsID_base);
	if (xsTest(xsVar(1))) {
		xsStringValue base = xsToString(xsVar(1));
		xsStringValue slash = strrchr(base, '\\');
		xsVar(1) = xsCall2(xsVar(1), xsID_slice, xsInteger(0), xsInteger(slash - base + 1));
		xsVar(1) = xsCall1(xsVar(1), xsID_concat, xsVar(0));
		wchar_t* path = xsToStringCopyW(xsVar(1));
		self->image = new Image(path, FALSE);
		free(path);
	}
	else {
		xsStringValue name = xsToString(xsVar(0));
		HRSRC rsrc = FindResource(gInstance, name, "PNG");
		HGLOBAL global = LoadResource(gInstance, rsrc);
		void* bytes = LockResource(global);
		ULONG size = SizeofResource(gInstance, rsrc);
		IStream* stream;
		CreateStreamOnHGlobal(NULL, TRUE, &stream);
		ULONG written;
		stream->Write(bytes, size, &written);
		self->image = new Image(stream, FALSE);
	}
    self->scale = 1;
	self->width = (PiuDimension)self->image->GetWidth();
	self->height = (PiuDimension)self->image->GetHeight();
	if (xsFindNumber(xsArg(0), xsID_scale, &number)) {
    	if (number == 2) {
    		self->scale = 2;
    		self->width /= 2;
    		self->height /= 2;
    	}
    }
	xsSetHostChunk(xsThis, self, sizeof(record));
	xsSetHostHooks(xsThis, &PiuTextureHooks);
	xsResult = xsThis;
}

void PiuTextureDelete(void* it)
{
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
