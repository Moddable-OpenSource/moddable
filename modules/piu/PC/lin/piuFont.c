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
#include <cairo-ft.h>
#include <ft2build.h>

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

static cairo_t* extents_cr = NULL;
static FT_Library ftLibrary = NULL;

void PiuFontDelete(void* it)
{
}

PiuDimension PiuFontGetAscent(PiuFont* self)
{
	return (PiuDimension)c_ceil((*self)->ascent);
}

PiuDimension PiuFontGetHeight(PiuFont* self)
{
	cairo_font_extents_t extents;
	cairo_set_font_face(extents_cr, (*self)->face);
	cairo_set_font_size(extents_cr, (*self)->size);
	cairo_font_extents(extents_cr, &extents);
	(*self)->ascent = extents.ascent;
	(*self)->height = extents.ascent + extents.descent;
	return extents.ascent + extents.descent;
}

PiuDimension PiuFontGetWidth(PiuFont* self, xsSlot* slot, xsIntegerValue offset, xsIntegerValue length)
{
	return (PiuDimension)c_ceil(PiuFontGetWidthSubPixel(self, slot, offset, length));
}

double PiuFontGetWidthSubPixel(PiuFont* self, xsSlot* slot, xsIntegerValue offset, xsIntegerValue length)
{
	xsMachine* the = (*self)->the;
	xsStringValue string = PiuToString(slot);
	if (length < 0)
		length = c_strlen(string + offset);
	char* text = malloc(length + 1);
	memcpy(text, string + offset, length);
	text[length] = 0;
	cairo_text_extents_t extents;
	cairo_set_font_face(extents_cr, (*self)->face);
	cairo_set_font_size(extents_cr, (*self)->size);
	cairo_text_extents(extents_cr, text, &extents);
	free(text);
	return extents.x_advance;
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
	PiuFontList* fontList;
	PiuFont* font;
	char path[PATH_MAX];
	xsStringValue name;
	GBytes *bytes;
	FT_Byte* data;
	size_t size;
	FT_Face ftFace;
	cairo_font_face_t* face;
	cairo_font_extents_t extents;
	
	xsResult = xsGet(xsGlobal, xsID_fonts);
	if (xsTest(xsResult))
		fontList = PIU(FontList, xsResult);
	else {
		PiuFontListNew(the);
		xsSet(xsGlobal, xsID_fonts, xsResult);
		fontList = PIU(FontList, xsResult);
    	if (FT_Init_FreeType(&ftLibrary))
			xsUnknownError("cannot initialize FreeType");
		extents_cr = cairo_create(cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 100, 100));
	}
	font = (*fontList)->first;
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
	
	strcpy(path, PIU_SLASH_SIGNATURE);
	strcat(path, "/fonts/");
	name = xsName((*self)->family);
	if (name)
		c_strcat(path, name);
	else
		c_strcat(path, "undefined");
	if (((*self)->flags & (piuStyleCondensedBit | piuStyleItalicBit)) || ((*self)->weight)) {
		c_strcat(path, "-"); 
		if ((*self)->flags & piuStyleCondensedBit)
			c_strcat(path, "Condensed"); 
		switch ((*self)->weight) {
			case 1: c_strcat(path, "Ultralight"); break;
			case 2: c_strcat(path, "Thin"); break;
			case 3: c_strcat(path, "Light"); break;
			case 5: c_strcat(path, "Medium");break;
			case 6: c_strcat(path, "Semibold"); break;
			case 7: c_strcat(path, "Bold");break;
			case 8: c_strcat(path, "Heavy"); break;
			case 9: c_strcat(path, "Black");break;
		}
		if ((*self)->flags & piuStyleItalicBit)
			c_strcat(path, "Italic"); 
	}
	else
		c_strcat(path, "-Regular"); 
	strcat(path, ".ttf");

	bytes = g_resources_lookup_data(path, G_RESOURCE_LOOKUP_FLAGS_NONE, NULL);
	if (!bytes)
		xsUnknownError("font not found: %s", path);
	data = (FT_Byte*)g_bytes_get_data(bytes, &size);
	if (FT_New_Memory_Face(ftLibrary, data, size, 0, &ftFace))
		xsUnknownError("invalid font: %s", path);
    face = cairo_ft_font_face_create_for_ft_face(ftFace, 0);
	cairo_set_font_face(extents_cr, face);
	cairo_set_font_size(extents_cr, (*self)->size);
	cairo_font_extents(extents_cr, &extents);
	//fprintf(stderr, "PiuStyleLookupFont %s %f %f %f\n", ftFace->family_name, extents.ascent, extents.descent, extents.height);

	PiuFontNew(the);
	font = PIU(Font, xsResult);
    (*font)->next = (*fontList)->first;
    (*fontList)->first = font;
	
	(*font)->flags = (*self)->flags & piuStyleBits;
	(*font)->family = (*self)->family;
	(*font)->size = (*self)->size;
	(*font)->weight = (*self)->weight;
	
	(*font)->face = face;
	(*font)->ascent = extents.ascent;
	(*font)->delta = extents.height - extents.ascent;
	(*font)->height = extents.height;

	PangoAttrList* pangoAttributes = pango_attr_list_new();
// 	pango_attr_list_insert(pangoAttributes, pango_attr_size_new_absolute((*self)->size * PANGO_SCALE));
// 	pango_attr_list_insert(pangoAttributes, pango_attr_weight_new(PANGO_WEIGHT_MEDIUM));
	(*font)->pangoAttributes = pangoAttributes;
	
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
