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

typedef struct {
	char* data;
	size_t offset;
	size_t size;
} PiuTextureStream;

static cairo_status_t PiuTextureCreateAux(void *closure, unsigned char *data, unsigned int length)
{
	PiuTextureStream* stream = closure;	
	if (stream->offset + length <= stream->size) {
		memcpy(data, stream->data + stream->offset, length);
		stream->offset += length;
		return CAIRO_STATUS_SUCCESS;
	}
	return CAIRO_STATUS_READ_ERROR;
}

void PiuTextureCreate(xsMachine* the) 
{
	PiuTextureRecord record;
	PiuTexture self = &record;
	char path[PATH_MAX];
	xsNumberValue number;
	xsVars(2);
	c_memset(self, 0, sizeof(record));
	self->reference = xsToReference(xsThis);
	xsVar(0) = xsGet(xsArg(0), xsID_path);
	if (!xsTest(xsVar(0)))
		xsUnknownError("no path");
	xsVar(1) = xsGet(xsArg(0), xsID_base);
	if (xsTest(xsVar(1))) {
		xsStringValue slash;
		strcpy(path, xsToString(xsVar(1)));
		slash = strrchr(path, '/');
		strcpy(slash + 1, xsToString(xsVar(0)));
		self->image = cairo_image_surface_create_from_png(path);
	}
	else {
		strcpy(path, PIU_SLASH_SIGNATURE);
		strcat(path, "/");
		strcat(path, xsToString(xsVar(0)));
		GBytes *bytes = g_resources_lookup_data(path, G_RESOURCE_LOOKUP_FLAGS_NONE, NULL);
		if (bytes) {
			PiuTextureStream stream;	
			stream.data = (char*)g_bytes_get_data(bytes, &stream.size);
			stream.offset = 0;
			self->image = cairo_image_surface_create_from_png_stream(PiuTextureCreateAux, &stream);
		}
	}
	if (!self->image)
		xsUnknownError("texture not found");
    self->scale = 1;
	self->width = (PiuDimension)cairo_image_surface_get_width(self->image);
	self->height = (PiuDimension)cairo_image_surface_get_height(self->image);
	if (xsFindNumber(xsArg(0), xsID_scale, &number)) {
    	if (number == 2) {
    		self->scale = 2;
    		self->width /= 2;
    		self->height /= 2;
    	}
    }
	//fprintf(stderr, "PiuTextureCreate %s %f %d %d\n", path, self->scale, self->width, self->height);
	xsSetHostChunk(xsThis, self, sizeof(record));
	xsSetHostHooks(xsThis, &PiuTextureHooks);
	xsResult = xsThis;
}

void PiuTextureDelete(void* it)
{
	PiuTexture self = it;
	if (self->image)
		cairo_surface_destroy(self->image);
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
