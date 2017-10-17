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
	xsStringValue string;
	xsNumberValue number;
	
	c_memset(self, 0, sizeof(record));
	self->reference = xsToReference(xsThis);
	
	if (!xsFindString(xsArg(0), xsID_path, &string)) 
		xsUnknownError("no path");
	NSString* path = [NSString stringWithUTF8String:string];
	NSString* extension = [path pathExtension];
	
	NSURL* base;
	if (xsFindString(xsArg(0), xsID_base, &string))
		base = [NSURL fileURLWithPath:[NSString stringWithUTF8String:string] isDirectory:NO];
	else
		base = [[NSBundle mainBundle] resourceURL];
	NSURL* url = [NSURL URLWithString:path relativeToURL:base];
	CGDataProviderRef dataProvider = CGDataProviderCreateWithURL((CFURLRef)url);
	if ([extension isEqualToString:@"png"])
		self->image = CGImageCreateWithPNGDataProvider(dataProvider, NULL, true, kCGRenderingIntentPerceptual);
	else
		self->image = CGImageCreateWithJPEGDataProvider(dataProvider, NULL, true, kCGRenderingIntentPerceptual);
	CGImageRetain(self->image);
    self->scale = 1;
	self->width = CGImageGetWidth(self->image);
	self->height = CGImageGetHeight(self->image);
    
	if (xsFindNumber(xsArg(0), xsID_scale, &number)) {
			xsIntegerValue scale = c_floor(number);
    	self->scale = scale;
    	self->width /= scale;
    	self->height /= scale;
    }
    
	xsSetHostChunk(xsThis, self, sizeof(record));
	xsSetHostHooks(xsThis, &PiuTextureHooks);
	xsResult = xsThis;
}

void PiuTextureDelete(void* it)
{
	PiuTexture self = it;
	CGImageRelease(self->image);
}

void PiuTextureMark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
}

void PiuTexture_get_width(xsMachine* the)
{
	PiuTexture* self = PIU(Texture, xsThis);
	xsResult = xsInteger((*self)->width);
}

void PiuTexture_get_height(xsMachine* the)
{
	PiuTexture* self = PIU(Texture, xsThis);
	xsResult = xsInteger((*self)->height);
}
