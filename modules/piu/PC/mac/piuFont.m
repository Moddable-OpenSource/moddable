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
	return c_ceil((*self)->ascent);
}

PiuDimension PiuFontGetHeight(PiuFont* self)
{
	return c_ceil((*self)->height);
}

PiuDimension PiuFontGetWidth(PiuFont* self, xsSlot* slot, xsIntegerValue offset, xsIntegerValue length)
{
	return c_ceil(PiuFontGetWidthSubPixel(self, slot, offset, length));
}

double PiuFontGetWidthSubPixel(PiuFont* self, xsSlot* slot, xsIntegerValue offset, xsIntegerValue length)
{
	xsMachine* the = (*self)->the;
	xsStringValue value = PiuToString(slot);
	value += offset;
	if (length < 0)
		length = c_strlen(value);
	CFStringRef string = CFStringCreateWithBytesNoCopy(NULL, (uint8_t*)value, length, kCFStringEncodingUTF8, false, kCFAllocatorNull);
    CFRange range = CFRangeMake(0, CFStringGetLength(string));
	CFMutableAttributedStringRef attributedString = CFAttributedStringCreateMutable(kCFAllocatorDefault, 0);
	CFAttributedStringReplaceString(attributedString, CFRangeMake(0, 0), string);
	CFAttributedStringSetAttribute(attributedString, range, kCTFontAttributeName, (*self)->fref);
    CTLineRef line = CTLineCreateWithAttributedString(attributedString);
    CGFloat width = CTLineGetOffsetForStringIndex(line, range.length, NULL);
  	CFRelease(line);
 	CFRelease(attributedString);
 	CFRelease(string);
 	return width;
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
				&& ((*font)->flags == ((*self)->flags & (piuStyleBits | piuStyleDecorationBits)))) {
			(*self)->font = font;
			return;
		}
		font = (*font)->next;
	}
	char buffer[256];
	xsStringValue name = xsName((*self)->family);
	if (!name) {
		NSFont *systemFont = [NSFont systemFontOfSize:(*self)->size];
		NSString* string = systemFont.fontName;
		name = (xsStringValue)[string UTF8String];
	}
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
	//fprintf(stderr, "%x %s %d %d -> %s\n", (*self)->flags, name, (*self)->size, (*self)->weight, buffer);
	
	CFStringRef string = CFStringCreateWithBytesNoCopy(NULL, (uint8_t*)buffer, c_strlen(buffer), kCFStringEncodingUTF8, false, kCFAllocatorNull);
	CTFontDescriptorRef descriptor = CTFontDescriptorCreateWithNameAndSize(string, (*self)->size);
  
  CTFontRef fref = CTFontCreateWithFontDescriptor(descriptor, 0.0, NULL);
  if ((*self)->flags & piuStyleItalicBit) {
  	CTFontRef frefItalic = CTFontCreateCopyWithSymbolicTraits(fref, 0.0, NULL, kCTFontItalicTrait, kCTFontItalicTrait);
  	CFRelease(fref);
  	fref = frefItalic;
  }
  CFRelease(descriptor);
	CFRelease(string);
	
	CGFloat ascent = CTFontGetAscent(fref);
	CGFloat descent = CTFontGetDescent(fref);
	CGFloat leading = CTFontGetLeading(fref);
	
	PiuFontNew(the);
	font = PIU(Font, xsResult);
    (*font)->next = (*fontList)->first;
    (*fontList)->first = font;
	
  (*font)->flags = (*self)->flags & (piuStyleBits | piuStyleDecorationBits);
	(*font)->family = (*self)->family;
	(*font)->size = (*self)->size;
	(*font)->weight = (*self)->weight;
	
	(*font)->fref = fref;
	(*font)->ascent = ascent;
	(*font)->height = ascent + descent + leading;
	
	(*self)->font = font;
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

void PiuFontListNew(xsMachine* the)
{
	PiuFontList* self;
	xsResult = xsNewHostObject(NULL);
	xsSetHostChunk(xsResult, NULL, sizeof(PiuFontListRecord));
	self = PIU(FontList, xsResult);
	(*self)->reference = xsToReference(xsResult);
	xsSetHostHooks(xsResult, &PiuFontListHooks);
}
