/*
 * Copyright (c) 2016-2018  Moddable Tech, Inc.
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

struct PiuFontStruct {
	PiuHandlePart;
	xsMachine* the;
	const char* name;
	int32_t nameLength;
	PiuFont* next;
	uint8_t *buffer;
	uint32_t offset;
	PiuDimension height;
	PiuDimension ascent;
	PiuTexture* texture;
};

static void PiuFontDelete(void* it);
static void PiuFontMark(xsMachine* the, void* it, xsMarkRoot markRoot);
static void PiuFontParse(xsMachine* the, PiuFont* self);

static xsHostHooks PiuFontHooks ICACHE_RODATA_ATTR = {
	PiuFontDelete,
	PiuFontMark,
	NULL
};

void PiuFontDelete(void* it)
{
}

PiuDimension PiuFontGetAscent(PiuFont* self)
{
	return (*self)->ascent;
}

PiuGlyph PiuFontGetGlyph(PiuFont* self, xsStringValue *string, uint8_t needPixels)
{
	static PiuGlyphRecord glyph;
	static PocoBitmapRecord pack;
	PiuTexture* texture = (*self)->texture;
	const unsigned char *chars = (*self)->buffer + (*self)->offset;
	uint32_t charCount = c_read32(chars) / 20;
	const uint8_t *cc = PocoBMFGlyphFromUTF8((uint8_t**)string, chars + 4, charCount);
	if (!cc)
		return NULL;
	glyph.advance = c_read16(cc + 16);
	if (needPixels) {
		glyph.sx = c_read16(cc + 4);
		glyph.sy = c_read16(cc + 6);
		glyph.sw = c_read16(cc + 8);
		glyph.sh = c_read16(cc + 10);
		glyph.dx = c_read16(cc + 12);
		glyph.dy = c_read16(cc + 14);
		if (texture) {
			PiuFlags flags = (*texture)->flags;
			glyph.bits = (flags & piuTextureColor) ? &((*texture)->bits) : NULL;
			glyph.mask = (flags & piuTextureAlpha) ? &((*texture)->mask) : NULL;
		}
		else {
			glyph.bits = NULL;
			glyph.mask = &pack;
			pack.format = kCommodettoBitmapGray16 | kCommodettoBitmapPacked;
			pack.pixels = (PocoPixel *)((glyph.sx | (glyph.sy << 16)) + (char *)cc);
	#if (0 == kPocoRotation) || (180 == kPocoRotation)
			pack.width = glyph.sw;
			pack.height = glyph.sh;
	#elif (90 == kPocoRotation) || (270 == kPocoRotation)
			pack.width = glyph.sh;
			pack.height = glyph.sw;
	#endif
			glyph.sx = 0;
			glyph.sy = 0;
		}
	}
	return &glyph;
}

PiuDimension PiuFontGetHeight(PiuFont* self)
{
	return (*self)->height;
}

PiuDimension PiuFontGetWidth(PiuFont* self, xsSlot* string, xsIntegerValue offset, xsIntegerValue length)
{
	xsMachine* the = (*self)->the;
	xsStringValue text = PiuToString(string);
	PiuDimension width = 0;
	text += offset;
	while (length) {
		char *prev = text;
		PiuGlyph glyph = PiuFontGetGlyph(self, &text, 0);
		length -= (text - prev);
		if (!glyph) {
			char missing, *pp;
			if (!c_read8(prev))
				break;
			missing = '?';
			pp = &missing;
			glyph = PiuFontGetGlyph(self, &pp, 0);
			if (!glyph)
				continue;
		}
		width += glyph->advance;
	}
	return width;
}

void PiuFontMark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	PiuFont self = it;
	PiuMarkHandle(the, self->next);
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

void PiuFontParse(xsMachine* the, PiuFont* self) 
{
	uint8_t *buffer = (*self)->buffer, version;
	int size, firstChar, lastChar;
	if (0x42 != c_read8(buffer))
		xsErrorPrintf("Invalid BMF header");
	buffer++;
	if (0x4D != c_read8(buffer))
		xsErrorPrintf("Invalid BMF header");
	buffer++;
	if (0x46 != c_read8(buffer))
		xsErrorPrintf("Invalid BMF header");
	buffer++;
	version = c_read8(buffer);
	if ((3 != version) && (4 != version))
		xsErrorPrintf("Invalid BMF header");
	buffer++;

	// skip block 1
	if (1 != c_read8(buffer))
		xsErrorPrintf("can't find info block");
	buffer++;
	size = c_read32(buffer);
    buffer += 4;
// 	fontSize = c_read16(buffer);
//     buffer += 2;
// 	fontBits = c_read8(buffer);
// 	italic = (fontBits & 0x20) ? 1 : 0;
// 	bold = (fontBits & 0x10) ? 1 : 0;
//     buffer += 12;
//     fontName = (char*)buffer;
	buffer += size;

	// get lineHeight from block 2
	if (2 != c_read8(buffer))
		xsErrorPrintf("can't find common block");
	buffer++;
	size = c_read32(buffer);
	buffer += 4;
	(*self)->height = c_read16(buffer);
	buffer += 2;
	(*self)->ascent = c_read16(buffer);
	buffer += 2;
	buffer += size - 4;

	// skip block 3
	if (3 != c_read8(buffer))
		xsErrorPrintf("can't find pages block");
	buffer++;
	size = c_read32(buffer);
    buffer += 4;
    if (version == 3) {
    	xsResult = xsGet(xsGlobal, xsID_Texture);
		xsResult = xsNewFunction1(xsResult, xsString(buffer));
		(*self)->texture = PIU(Texture, xsResult);
	}
    buffer += size;

	// use block 4
	if (4 != c_read8(buffer))
		xsErrorPrintf("can't find chars block");
	buffer++;
	(*self)->offset = buffer - (*self)->buffer;		// position of size of chars table
	size = c_read32(buffer);
	if (size % 20)
		xsErrorPrintf("bad chars block size");
	buffer += 4;

	firstChar = c_read32(buffer);
	lastChar = firstChar + (size / 20);
}

void PiuFontListLockCache(xsMachine* the)
{
}

void PiuFontListUnlockCache(xsMachine* the)
{
}

void PiuStyleLookupFont(PiuStyle* self)
{
	xsMachine* the = (*self)->the;
	char buffer[256];
	xsStringValue string = xsName((*self)->family);
	PiuFontList* fontList;
	PiuFont* font;
	if (string)
		c_strcpy(buffer, string);
	else
		c_strcpy(buffer, "undefined");
	if (((*self)->flags & (piuStyleCondensedBit | piuStyleItalicBit)) || ((*self)->weight) || ((*self)->size)) {
		xsIntegerValue flag = 0;
		c_strcat(buffer, "-");
		if ((*self)->flags & piuStyleCondensedBit)
			c_strcat(buffer, "Condensed");
		else
			flag |= 1;
		switch ((*self)->weight) {
			case 1: c_strcat(buffer, "Ultralight"); break;
			case 2: c_strcat(buffer, "Thin"); break;
			case 3: c_strcat(buffer, "Light"); break;
			case 5: c_strcat(buffer, "Medium");break;
			case 6: c_strcat(buffer, "Semibold"); break;
			case 7: c_strcat(buffer, "Bold");break;
			case 8: c_strcat(buffer, "Heavy"); break;
			case 9: c_strcat(buffer, "Black");break;
			default: flag |= 2; break;
		}
		if ((*self)->flags & piuStyleItalicBit)
			c_strcat(buffer, "Italic"); 
		else
			flag |= 4;
		if (flag == 7)
			c_strcat(buffer, "Regular");
		if ((*self)->size) {
			xsIntegerValue length = c_strlen(buffer) + 1;
			c_strcat(buffer, "-");
			fxIntegerToString(NULL, (*self)->size, buffer + length, sizeof(buffer) - length);
		}
	}
	xsResult = xsGet(xsGlobal, xsID_fonts);
	if (xsTest(xsResult))
		fontList = PIU(FontList, xsResult);
	else {
		int c = mcCountResources(the), i;
		PiuFontListNew(the);
		xsSet(xsGlobal, xsID_fonts, xsResult);
		fontList = PIU(FontList, xsResult);
		for (i = 0; i < c; i++) {
			const char* name = mcGetResourceName(the, i);
			char* extension = c_strrchr(name, '.');
			if ((!c_strcmp(extension, ".bf4")) || (!c_strcmp(extension, ".fnt"))) {
				PiuFont* font;
				size_t size;
				PiuFontNew(the);
				font = PIU(Font, xsResult);
				(*font)->next = (*fontList)->first;
				(*fontList)->first = font;
				(*font)->name = name;
				(*font)->nameLength = extension - name;
				(*font)->buffer = (uint8_t *)mcGetResource(the, name, &size);
				PiuFontParse(the, font);
			}
		}
	}
	font = (*fontList)->first;
	while (font) {
		if (!c_strncmp(buffer, (*font)->name, (*font)->nameLength)) {
			(*self)->font = font;
			return;
		}
		font = (*font)->next;
	}
	xsURIError("font not found: %s", buffer);
}
