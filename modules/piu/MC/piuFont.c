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
#include "mc.defines.h"

static void PiuFontDelete(void* it);
static void PiuFontMark(xsMachine* the, void* it, xsMarkRoot markRoot);
static void PiuFontListMark(xsMachine* the, void* it, xsMarkRoot markRoot);

static const xsHostHooks PiuFontHooks ICACHE_RODATA_ATTR = {
	PiuFontDelete,
	PiuFontMark,
	NULL
};

static const xsHostHooks PiuFontListHooks ICACHE_RODATA_ATTR = {
	NULL,
	PiuFontListMark,
	NULL
};

void PiuFontDelete(void* it)
{
}

PiuDimension PiuFontGetAscent(PiuFont* self)
{
	return (*self)->ascent;
}

PiuGlyph PiuFontGetGlyph(PiuFont* self, xsIntegerValue character, uint8_t needPixels)
{
	static PiuGlyphRecord piuGlyph;
	static PocoBitmapRecord bitmap;
	PiuTexture* texture = (*self)->texture;
	CFEGlyph cfeGlyph;
	CFESetFontData(gCFE, (*self)->buffer, (*self)->bufferSize);
	CFESetFontSize(gCFE, (*self)->size);
	cfeGlyph = CFEGetGlyphFromUnicode(gCFE, character, needPixels);
	if (cfeGlyph) {
		piuGlyph.advance = cfeGlyph->advance;
		if (needPixels) {
			piuGlyph.dx = cfeGlyph->dx;
			piuGlyph.dy = cfeGlyph->dy;
			piuGlyph.sx = cfeGlyph->sx;
			piuGlyph.sy = cfeGlyph->sy;
			piuGlyph.sw = cfeGlyph->w;
			piuGlyph.sh = cfeGlyph->h;
			if (texture) {
				PiuFlags flags = (*texture)->flags;
				piuGlyph.bits = (flags & piuTextureColor) ? &((*texture)->bits) : NULL;
				piuGlyph.mask = (flags & piuTextureAlpha) ? &((*texture)->mask) : NULL;
			}
			else {
				piuGlyph.bits = NULL;
				piuGlyph.mask = &bitmap;
				bitmap.format = cfeGlyph->format;
				bitmap.pixels = (PocoPixel *)cfeGlyph->bits;
#if (0 == kPocoRotation) || (180 == kPocoRotation)
				bitmap.width = cfeGlyph->w;
				bitmap.height = cfeGlyph->h;
#elif (90 == kPocoRotation) || (270 == kPocoRotation)
				bitmap.width = cfeGlyph->h;
				bitmap.height = cfeGlyph->w;
#endif
			}
		}
		return &piuGlyph;
	}
	return NULL;
}

PiuDimension PiuFontGetHeight(PiuFont* self)
{
	return (*self)->height;
}

PiuDimension PiuFontGetWidth(PiuFont* self, xsSlot* string, xsIntegerValue offset, xsIntegerValue length)
{
	xsMachine* the = (*self)->the;
	xsStringValue text = PiuToString(string);
	xsIntegerValue character = 0;
	PiuGlyph glyph;
	PiuDimension width = 0;
	text += offset;
	while (length) {
		xsStringValue formerText = text;
#if MODDEF_CFE_KERN
		xsIntegerValue formerCharacter = character;
#endif
		text = fxUTF8Decode(text, &character);
		if (character <= 0)
			break;
		length -= text - formerText;
		glyph = PiuFontGetGlyph(self, character, 0);
		if (!glyph) {
			character = '?';
			glyph = PiuFontGetGlyph(self, character, 0);
			if (!glyph)
				continue;
		}
		width += glyph->advance;
#if MODDEF_CFE_KERN
		if (formerCharacter)
			width += CFEGetKerningOffset(gCFE, formerCharacter, character);
#endif
	}
	return width;
}

void PiuFontMark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	PiuFont self = it;
	PiuMarkHandle(the, self->texture);
}

void PiuFontNew(xsMachine* the)
{
	PiuFont* self;
	xsResult = xsNewHostObject(NULL);
	xsSetHostChunk(xsResult, NULL, sizeof(PiuFontRecord));
	self = PIU(Font, xsResult);
	(*self)->reference = xsToReference(xsResult);
	(*self)->the = the;
	xsSetHostHooks(xsResult, &PiuFontHooks);
}

#ifdef piuGPU
void PiuFontBind(PiuFont* self, PiuApplication* application, PiuView* view)
{
	PiuTexture* texture = (*self)->texture;
	if (texture)
		PiuTextureBind(texture, application, view);
}

void PiuFontUnbind(PiuFont* self, PiuApplication* application, PiuView* view)
{
	PiuTexture* texture = (*self)->texture;
	if (texture)
		PiuTextureUnbind(texture, application, view);
}
#endif

void PiuStyleLookupFont(PiuStyle* self)
{
	xsMachine* the = (*self)->the;
	void* archive = NULL;
	PiuFontList* fontList;
	PiuFont* font = NULL;
	xsStringValue name;
	char path[256];
	void* buffer;
	size_t bufferSize;
	int32_t ascent, descent, leading;
	
	if ((*self)->archive) {
		xsResult = xsReference((*self)->archive);
		archive = xsGetHostData(xsResult);
	}
	xsResult = xsGet(xsGlobal, xsID_fonts);
	if (xsTest(xsResult)) {
		fontList = PIU(FontList, xsResult);
		font = (*fontList)->first;
		while (font) {
			if (((*font)->family == (*self)->family)
					&& ((*font)->size == (*self)->size)
					&& ((*font)->weight == (*self)->weight)
					&& ((*font)->flags == ((*self)->flags & piuStyleBits))) {
//				fprintf(stderr, "CACHE\n");
				(*self)->font = font;
				return;
			}
			font = (*font)->next;
		}
	}
	else {
		gCFE = CFENew();
		PiuFontListNew(the);
		xsSet(xsGlobal, xsID_fonts, xsResult);
		fontList = PIU(FontList, xsResult);
	}
	
	name = xsName((*self)->family);
	if (name)
		c_strcpy(path, name);
	else
		c_strcpy(path, "undefined");
	if (((*self)->flags & (piuStyleCondensedBit | piuStyleItalicBit)) || ((*self)->weight) || ((*self)->size)) {
		xsIntegerValue flag = 0;
		c_strcat(path, "-");
		if ((*self)->flags & piuStyleCondensedBit)
			c_strcat(path, "Condensed");
		else
			flag |= 1;
		switch ((*self)->weight) {
			case 1: c_strcat(path, "Ultralight"); break;
			case 2: c_strcat(path, "Thin"); break;
			case 3: c_strcat(path, "Light"); break;
			case 5: c_strcat(path, "Medium");break;
			case 6: c_strcat(path, "Semibold"); break;
			case 7: c_strcat(path, "Bold");break;
			case 8: c_strcat(path, "Heavy"); break;
			case 9: c_strcat(path, "Black");break;
			default: flag |= 2; break;
		}
		if ((*self)->flags & piuStyleItalicBit)
			c_strcat(path, "Italic"); 
		else
			flag |= 4;
		if (flag == 7)
			c_strcat(path, "Regular");
	}
	
	PiuFontNew(the);
	font = PIU(Font, xsResult);
#if MODDEF_CFE_TTF
	c_strcat(path, ".ttf");
//	fprintf(stderr, "%s %d %d %d\n", path, (*self)->size, (*self)->weight, (*self)->flags & piuStyleBits);
	buffer = (uint8_t *)fxGetResource(the, archive, path, &bufferSize);
	if (!buffer)
		xsURIError("font not found: %s", path);
	(*font)->next = (*fontList)->first;
	(*fontList)->first = font;
#else
	if ((*self)->size) {
		xsIntegerValue length = c_strlen(path) + 1;
		c_strcat(path, "-");
		fxIntegerToString(NULL, (*self)->size, path + length, sizeof(path) - length);
	}
	name = path + c_strlen(path);
	c_strcpy(name, ".bf4");
	buffer = (uint8_t *)fxGetResource(the, archive, path, &bufferSize);
	if (!buffer) {
		c_strcpy(name, ".fnt");
		buffer = (uint8_t *)fxGetResource(the, archive, path, &bufferSize);
		if (!buffer)
			xsURIError("font not found: %s", path);
		c_strcpy(name, ".png");
		(*font)->next = (*fontList)->first;
		(*fontList)->first = font;
		
		xsResult = xsNewObject();
		if (archive)
			xsDefine(xsResult, xsID_archive, xsReference((*self)->archive), xsDefault);
		xsDefine(xsResult, xsID_path, xsString(path), xsDefault);
		xsResult = xsNew1(xsGlobal, xsID_Texture, xsResult);
		(*font)->texture = PIU(Texture, xsResult);
	}
	else {
		(*font)->next = (*fontList)->first;
		(*fontList)->first = font;
	}
#endif

	CFESetFontData(gCFE, buffer, bufferSize);
	CFESetFontSize(gCFE, (*self)->size);
	CFEGetFontMetrics(gCFE, &ascent, &descent, &leading);
    
	(*font)->flags = (*self)->flags & piuStyleBits;
	(*font)->family = (*self)->family;
	(*font)->size = (*self)->size;
	(*font)->weight = (*self)->weight;
	
	(*font)->buffer = buffer;
	(*font)->bufferSize = bufferSize;
	(*font)->ascent = ascent;
	(*font)->height = ascent + descent + leading;
	
	(*self)->font = font;
}

void PiuFontListLockCache(xsMachine* the)
{
	if (gCFE)
		CFELockCache(gCFE, 1);
}

void PiuFontListMark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	PiuFontList self = it;
	PiuFont* font = self->first;
	while (font) {
		PiuMarkHandle(the, font);
		font = (*font)->next;
	}
}

void PiuFontListUnlockCache(xsMachine* the)
{
	if (gCFE)
		CFELockCache(gCFE, 0);
}

void PiuFontListNew(xsMachine* the)
{
	PiuFontList* self;
	xsResult = xsNewHostObject(NULL);
	xsSetHostChunk(xsResult, NULL, sizeof(PiuFontListRecord));
	self = PIU(FontList, xsResult);
	(*self)->reference = xsToReference(xsResult);
	xsSetHostHooks(xsResult, &PiuFontListHooks);
}

