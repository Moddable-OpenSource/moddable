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

static void PiuFontDelete(void* it);
static void PiuFontMark(xsMachine* the, void* it, xsMarkRoot markRoot);
static void PiuFontListMark(xsMachine* the, void* it, xsMarkRoot markRoot);

static xsHostHooks PiuFontHooks = {
	PiuFontDelete,
	PiuFontMark,
	NULL
};

static const xsHostHooks PiuFontListHooks = {
	NULL,
	PiuFontListMark,
	NULL
};

void PiuFontDelete(void* it)
{
}

PiuDimension PiuFontGetAscent(PiuFont* self)
{
	return (PiuDimension)c_ceil((*self)->ascent);
}

PiuDimension PiuFontGetHeight(PiuFont* self)
{
	return (PiuDimension)c_ceil((*self)->height);
}

PiuDimension PiuFontGetWidth(PiuFont* self, xsSlot* slot, xsIntegerValue offset, xsIntegerValue length)
{
	return (PiuDimension)c_ceil(PiuFontGetWidthSubPixel(self, slot, offset, length));
}

double PiuFontGetWidthSubPixel(PiuFont* self, xsSlot* slot, xsIntegerValue offset, xsIntegerValue length)
{
	xsMachine* the = (*self)->the;
	HDC hdc = GetDC(NULL);
	Graphics graphics(hdc,FALSE);
	graphics.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);
	const StringFormat* pStringFormat = StringFormat::GenericTypographic();
	xsStringValue string = PiuToString(slot);
	if (length < 0)
		length = c_strlen(string + offset);
	xsIntegerValue wideStringLength = MultiByteToWideChar(CP_UTF8, 0, string + offset, length, NULL, 0);
	PWSTR wideString = (PWSTR)c_malloc(wideStringLength * 2);
	xsElseThrow(wideString != NULL);
	MultiByteToWideChar(CP_UTF8, 0, string + offset, length, wideString, wideStringLength);
	PointF origin(0, 0);
	RectF bounds;
	graphics.MeasureString(wideString, wideStringLength, (*self)->object, origin, pStringFormat, &bounds);
	c_free(wideString);
	SizeF size;
	bounds.GetSize(&size);
	ReleaseDC(NULL, hdc);
	return size.Width;
}

void PiuFontMark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
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

void PiuStyleLookupFont(PiuStyle* self)
{
	xsMachine* the = (*self)->the;
	xsResult = xsGet(xsGlobal, xsID_fonts);
	PiuFontList* fontList;
	if (xsTest(xsResult))
		fontList = PIU(FontList, xsResult);
	else {
		PiuFontListNew(the);
		xsSet(xsGlobal, xsID_fonts, xsResult);
		fontList = PIU(FontList, xsResult);
	}
	PiuFont* font = (*fontList)->first;
	while (font) {
		if (((*font)->family == (*self)->family)
				&& ((*font)->size == (*self)->size)
				&& ((*font)->weight == (*self)->weight)
				&& ((*font)->flags == ((*self)->flags & piuStyleBits))) {
			(*self)->font = font;
			return;
		}
		font = (*font)->next;
	}
	
	char buffer[256];
	xsStringValue name = xsName((*self)->family);
	if (name)
		c_strcpy(buffer, name);
	else
		c_strcpy(buffer, "undefined");
	if (((*self)->flags & (piuStyleCondensedBit | piuStyleItalicBit)) || ((*self)->weight)) {
		c_strcat(buffer, "-"); 
		if ((*self)->flags & piuStyleCondensedBit)
			c_strcat(buffer, "Condensed"); 
		switch ((*self)->weight) {
			case 1: c_strcat(buffer, "Ultralight"); break;
			case 2: c_strcat(buffer, "Thin"); break;
			case 3: c_strcat(buffer, "Light"); break;
			case 5: c_strcat(buffer, "Medium");break;
			case 6: c_strcat(buffer, "Semibold"); break;
			case 7: c_strcat(buffer, "Bold");break;
			case 8: c_strcat(buffer, "Heavy"); break;
			case 9: c_strcat(buffer, "Black");break;
		}
		if ((*self)->flags & piuStyleItalicBit)
			c_strcat(buffer, "Italic"); 
	}
	else
		c_strcat(buffer, "-Regular"); 
	HRSRC rsrc = FindResource(gInstance, buffer, "TTF");
	if (rsrc == NULL)
		xsUnknownError("font not found %s", buffer);
		
	PiuFontNew(the);
	font = PIU(Font, xsResult);
	
	HGLOBAL global = LoadResource(gInstance, rsrc);
	xsElseThrow(global != NULL);
	void* memory = LockResource(global);
	xsElseThrow(memory != NULL);
	(*font)->collection = new PrivateFontCollection();
	xsElseThrow((*font)->collection != NULL);
	Status status = (*font)->collection->AddMemoryFont(memory, SizeofResource(gInstance, rsrc));
	xsElseThrow(status == Ok);
	FontFamily fontFamily[1];
	int found;
	(*font)->collection->GetFamilies(1, fontFamily, &found);
	xsElseThrow(found == 1);
	FontStyle fontStyle = FontStyleRegular;
	(*font)->object = new Font(&fontFamily[0], (REAL)(*self)->size, fontStyle, UnitPixel);
	xsElseThrow((*font)->object != NULL);
	UINT16 emHeight = fontFamily[0].GetEmHeight(fontStyle);
	REAL ascent = (REAL)(*self)->size * fontFamily[0].GetCellAscent(fontStyle) / emHeight;
	REAL descent = (REAL)(*self)->size * fontFamily[0].GetCellDescent(fontStyle) / emHeight;
	REAL height = (REAL)(*self)->size * fontFamily[0].GetLineSpacing(fontStyle) / emHeight;
	
    (*font)->next = (*fontList)->first;
    (*fontList)->first = font;
	
	(*font)->flags = (*self)->flags & piuStyleBits;
	(*font)->family = (*self)->family;
	(*font)->size = (*self)->size;
	(*font)->weight = (*self)->weight;
	
	(*font)->ascent = ascent * (height / (ascent + descent));
	(*font)->delta = (*font)->ascent - ascent;
	(*font)->height = height;
	
	(*self)->font = font;
}

void PiuFontListMark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	PiuFontList self = (PiuFontList)it;
	PiuFont* font = self->first;
	while (font) {
		PiuMarkHandle(the, font);
		font = (*font)->next;
	}
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
