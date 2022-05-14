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

#include "piuAll.h"

typedef struct {
	const char *name; 
	int value; 
	PiuFlags flags; 
} PiuStyleBuilderRecord, PiuStyleBuilder;

const PiuStyleBuilderRecord ICACHE_FLASH_ATTR piuStyleWeightBuilders[] = {
	{ "100", 1, piuStyleAbsoluteWeight },
	{ "ultralight", 1, piuStyleAbsoluteWeight },
	{ "200", 2, piuStyleAbsoluteWeight },
	{ "thin", 2, piuStyleAbsoluteWeight },
	{ "300", 3, piuStyleAbsoluteWeight },
	{ "light", 3, piuStyleAbsoluteWeight },
	{ "400", 4, piuStyleAbsoluteWeight },
	{ "normal", 4, piuStyleAbsoluteWeight },
	{ "500", 5, piuStyleAbsoluteWeight },
	{ "medium", 5, piuStyleAbsoluteWeight },
	{ "600", 6, piuStyleAbsoluteWeight },
	{ "semibold", 6, piuStyleAbsoluteWeight },
	{ "700", 7, piuStyleAbsoluteWeight },
	{ "bold", 7, piuStyleAbsoluteWeight },
	{ "800", 8, piuStyleAbsoluteWeight },
	{ "heavy", 8, piuStyleAbsoluteWeight },
	{ "900", 9, piuStyleAbsoluteWeight },
	{ "black", 9, piuStyleAbsoluteWeight },
	{ "bolder", 1, piuStyleRelativeWeight },
	{ "lighter", -1, piuStyleRelativeWeight },
	{ NULL, 0, 0 },
};

const PiuStyleBuilderRecord ICACHE_FLASH_ATTR piuStyleSizeBuilders[] = {
	{ "xx-small", 9, piuStyleAbsoluteSize },
	{ "x-small", 10, piuStyleAbsoluteSize },
	{ "small", 13, piuStyleAbsoluteSize },
	{ "medium", 16, piuStyleAbsoluteSize },
	{ "large", 18, piuStyleAbsoluteSize },
	{ "x-large", 24, piuStyleAbsoluteSize },
	{ "xx-large", 32, piuStyleAbsoluteSize },
	{ "larger", 1, piuStyleRelativeSize },
	{ "smaller", -1, piuStyleRelativeSize },
	{ NULL, 0, 0 },
};


static void PiuStyleLinkMark(xsMachine* the, void* it, xsMarkRoot markRoot);

static const xsHostHooks PiuStyleLinkHooks ICACHE_RODATA_ATTR = {
	NULL,
	PiuStyleLinkMark,
	NULL
};

static void PiuStyleMark(xsMachine* the, void* it, xsMarkRoot markRoot);

static const xsHostHooks PiuStyleHooks ICACHE_RODATA_ATTR = {
	PiuStyleDelete,
	PiuStyleMark,
	NULL
};

PiuStyle* PiuStyleLinkCompute(xsMachine *the, PiuStyleLink* chain, PiuApplication* application)
{
	PiuStyle* result = (*chain)->computedStyle;
	if (result == NULL) {
		PiuStyleLink* link = chain;
		xsResult = xsGet(xsGlobal, xsID_Style);
		xsResult = xsNewFunction0(xsResult);
		result = PIU(Style, xsResult);
		while (link) {
			PiuStyleOverride((*link)->style, result);
			link = (*link)->chain;
		}
		(*chain)->computedStyle = result;
		PiuStyleLookupFont(result);
	}
	return result;
}

void PiuStyleLinkMark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	PiuStyleLink self = it;
	PiuStyleLink* link = self->first;
	while (link) {
		PiuMarkHandle(the, link);
		link = (*link)->next;
	}
	PiuMarkHandle(the, self->style);
	PiuMarkHandle(the, self->computedStyle);
	
	
}

PiuStyleLink* PiuStyleLinkMatch(xsMachine *the, PiuStyleLink* list, PiuStyleLink* chain, PiuStyle* style)
{
	PiuStyleLink* link = (*list)->first;
	while (link) {
		if ((*link)->style == style)
			break;
		link = (*link)->next;
	}
	if (!link) {
		PiuStyleLinkNew(the);
		link = PIU(StyleLink, xsResult);
		(*link)->chain = chain;	
		(*link)->next = (*list)->first;
		(*list)->first = link;
		(*link)->style = style;	
	}
	return link;
}

void PiuStyleLinkNew(xsMachine* the)
{
	PiuStyleLink* self;
	xsResult = xsNewHostObject(NULL);
	xsSetHostChunk(xsResult, NULL, sizeof(PiuStyleLinkRecord));
	self = PIU(StyleLink, xsResult);
	(*self)->reference = xsToReference(xsResult);
	xsSetHostHooks(xsResult, &PiuStyleLinkHooks);
}

void PiuStyleCreate(xsMachine* the) 
{
	xsIntegerValue c = xsToInteger(xsArgc);
	PiuStyleRecord record;
	PiuStyle self = &record;
	xsStringValue string;
	xsIntegerValue integer;
	char buffer[256] = "";
	xsVars(1); // PiuColorsDictionary
	c_memset(self, 0, sizeof(record));
	self->reference = xsToReference(xsThis);
	self->the = the;
	self->color[0].a = 255;
	self->color[1].a = 255;
	self->color[2].a = 255;
	self->color[3].a = 255;
	if ((c > 0) && (xsTest(xsArg(0)))) {
		if (xsFindResult(xsArg(0), xsID_archive))
			self->archive = xsToReference(xsResult);
		if (xsFindString(xsArg(0), xsID_font, &string)) {
    		const PiuStyleBuilder* builder;
			xsStringValue p = string;
			xsStringValue q;
			char c;
			char state = 0;
			for (;;) {
				while ((c = c_read8(p))) {
					if (c != ' ')
						break;
					p++;
				}
				q = p;
				while ((c = c_read8(q))) {
					if (c == ' ')
						break;
					q++;
				}
				if (p == q)
					break;
				if ((state == 0) && (c_strncmp(p, "inherit", q - p))) {
					if (!c_strncmp(p, "italic", q - p))
						self->flags |= piuStyleItalic | piuStyleItalicBit;
					else if (!c_strncmp(p, "normal", q - p))
						self->flags |= piuStyleItalic;
					else
						state = 1;
				}
				if ((state == 1) && (c_strncmp(p, "inherit", q - p))) {
					for (builder = piuStyleWeightBuilders; builder->name; builder++) {
						if (!c_strncmp(p, builder->name, q - p)) {
							self->weight = (int8_t)builder->value;
							self->flags |= builder->flags;
							break;
						}
					}
					if (!builder->name)
						state = 2;
				}
				if ((state == 2) && (c_strncmp(p, "inherit", q - p))) {
					if (!c_strncmp(p, "condensed", q - p))
						self->flags |= piuStyleCondensed | piuStyleCondensedBit;
					else if (!c_strncmp(p, "normal", q - p))
						self->flags |= piuStyleCondensed;
					else
						state = 3;
				}
				if ((state == 3) && (c_strncmp(p, "inherit", q - p))) {
					for (builder = piuStyleSizeBuilders; builder->name; builder++) {
						if (!c_strncmp(p, builder->name, q - p)) {
							self->size = (int16_t)builder->value;
							self->flags |= builder->flags;
							break;
						}
					}
					if (!builder->name) {
						c = c_read8(p);
						if (('1' <= c) && (c <= '9')) {
							xsStringValue r = p;
							char d;
							PiuDimension size = 0;
							while ((r < q) && ((d = c_read8(r))) && ('0' <= d) && (d <= '9')) {
								size = (10 * size) + (d - '0');
								r++;
							}
							if (size) {
								if (!c_strncmp(r, "%", q - r)) {
									self->size = size;	
									self->flags |= piuStylePercentageSize;
								}
								else if (!c_strncmp(r, "px", q - r)) {
									self->size = size;	
									self->flags |= piuStyleAbsoluteSize;
								}
							}
						}
						else
							state = 4;
					}
				}
				if (state > 3)
					c_strncat(buffer, p, q - p);
				p = q;
				state++;
			}
			if (buffer[0]) {
				self->family = xsID(buffer);
				self->flags |= piuStyleFamily;
			}
		}
		if (xsFindResult(xsArg(0), xsID_color)) {
			PiuColorsDictionary(the, &xsResult, self->color);
			self->flags |= piuStyleColor;
		}
		if (xsFindString(xsArg(0), xsID_decoration, &string)) {
			if (!c_strcmp(string, "normal"))
				self->flags |= piuStyleDecoration;
      		else if (!c_strcmp(string, "underline")) {
				self->flags |= piuStyleDecoration | piuStyleUnderline;
      		}
			else
				xsErrorPrintf("invalid decoration");
		}
		if (xsFindString(xsArg(0), xsID_horizontal, &string)) {
			if (!c_strcmp(string, "center"))
				self->horizontal = piuCenter;
			else if (!c_strcmp(string, "left"))
				self->horizontal = piuLeft;
			else if (!c_strcmp(string, "right"))
				self->horizontal = piuRight;
			else if (!c_strcmp(string, "justify"))
				self->horizontal = piuLeftRight;
			else
				xsErrorPrintf("invalid horizontal alignment");
			self->flags |= piuStyleHorizontal;
		}
		if (xsFindString(xsArg(0), xsID_vertical, &string)) {
			if (!c_strcmp(string, "middle"))
				self->vertical = piuMiddle;
			else if (!c_strcmp(string, "top"))
				self->vertical = piuTop;
			else if (!c_strcmp(string, "bottom"))
				self->vertical = piuBottom;
			else
				xsErrorPrintf("invalid vertical alignment");
			self->flags |= piuStyleVertical;
		}
		if (xsFindInteger(xsArg(0), xsID_indentation, &integer)) {
			self->flags |= piuStyleIndentation;
			self->indentation = (PiuCoordinate)integer;
		}
		if (xsFindInteger(xsArg(0), xsID_leading, &integer)) {
			self->flags |= piuStyleLeading;
			self->leading = (PiuCoordinate)integer;
		}
		if (!xsFindResult(xsArg(0), xsID_margins))
			xsResult = xsArg(0);
		if (xsFindInteger(xsResult, xsID_left, &integer)) {
			self->flags |= piuStyleMarginLeft;
			self->margins.left = (PiuCoordinate)integer;
		}
		if (xsFindInteger(xsResult, xsID_top, &integer)) {
			self->flags |= piuStyleMarginTop;
			self->margins.top = (PiuCoordinate)integer;
		}
		if (xsFindInteger(xsResult, xsID_right, &integer)) {
			self->flags |= piuStyleMarginRight;
			self->margins.right = (PiuCoordinate)integer;
		}
		if (xsFindInteger(xsResult, xsID_bottom, &integer)) {
			self->flags |= piuStyleMarginBottom;
			self->margins.bottom = (PiuCoordinate)integer;
		}
	}
	xsSetHostChunk(xsThis, self, sizeof(record));
	xsSetHostHooks(xsThis, &PiuStyleHooks);
	xsResult = xsThis;
}

void PiuStyleDelete(void* it)
{
}

void PiuStyleDraw(PiuStyle* self, xsSlot* string, PiuView* view, PiuRectangle bounds, PiuBoolean ellipsis, PiuState state)
{
	PiuFont* font = (*self)->font;
	PiuCoordinate x = bounds->x;
	PiuCoordinate y = bounds->y;
	PiuCoordinate dx = bounds->width;
	PiuCoordinate dy = bounds->height;
	PiuMargins margins = &((*self)->margins);
	PiuCoordinate du, dv;
	PiuColorRecord color;
	switch ((*self)->horizontal) {
	case piuCenter:
		du = PiuStyleGetWidth(self, string);
		if (ellipsis && (dx < du)) {
			x += margins->left;
			dx -= margins->left + margins->right;
		}
		else {
#ifdef piuPC
			x += margins->left + ((dx - du) / 2);
#else
			x += margins->left + ((dx - du) >> 1);
#endif
			dx = 0;
		}
		break;
	case piuLeft:
		x += margins->left;
		if (ellipsis) {
			du = PiuStyleGetWidth(self, string);
			if (dx < du)
				dx -= margins->left + margins->right;
			else
				dx = 0;
		}
		else
			dx = 0;
		break;
	case piuRight:
		du = PiuStyleGetWidth(self, string);
		if (ellipsis && (dx < du)) {
			x += margins->left;
			dx -= margins->left + margins->right;
		}
		else {
			x += dx - du + margins->left;
			dx = 0;
		}
		break;
	}
	switch ((*self)->vertical) {
	case piuMiddle:
		dv = margins->top + PiuFontGetHeight(font) + margins->bottom;
#ifdef piuPC
		y += margins->top + ((dy - dv) / 2);
#else
		y += margins->top + ((dy - dv) >> 1);
#endif
		break;
	case piuTop:
		y += margins->top;
		break;
	case piuBottom:
		dv = PiuFontGetHeight(font);
		y += dy - margins->bottom - dv;
		break;
	}
	if (dx >= 0) {
		PiuColorsBlend((*self)->color, state, &color);
		PiuViewPushColor(view, &color);
		PiuViewDrawString(view, string, 0, -1, font, x, y, dx, du);
		PiuViewPopColor(view);
	}
}

PiuDimension PiuStyleGetWidth(PiuStyle* self, xsSlot* string)
{
	PiuFont* font = (*self)->font;
	return (*self)->margins.left + PiuFontGetWidth(font, string, 0, -1) + (*self)->margins.right;
}

PiuDimension PiuStyleGetHeight(PiuStyle* self, xsSlot* string)
{
	PiuFont* font = (*self)->font;
	return (*self)->margins.top + PiuFontGetHeight(font) + (*self)->margins.bottom;
}

void PiuStyleMark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	PiuStyle self = it;
	PiuMarkReference(the, self->archive);
	PiuMarkHandle(the, self->font);
}

void PiuStyleOverride(PiuStyle* self, PiuStyle* result)
{
	PiuFlags flags = (*self)->flags;
	
	if (flags & piuStyleCondensed) {
		if (flags & piuStyleCondensedBit)
			(*result)->flags |= piuStyleCondensedBit;
		else
			(*result)->flags &= ~piuStyleCondensedBit;
	}
	if (flags & piuStyleItalic) {
		if (flags & piuStyleItalicBit)
			(*result)->flags |= piuStyleItalicBit;
		else
			(*result)->flags &= ~piuStyleItalicBit;
	}
	if (flags & piuStyleFamily) {
		(*result)->archive = (*self)->archive;
		(*result)->family = (*self)->family;
	}
	if (flags & piuStyleAbsoluteSize)
		(*result)->size = (*self)->size;
	else if (flags & piuStylePercentageSize) {
		int32_t size = ((int32_t)((*result)->size) * (int32_t)((*self)->size)) / 100;
		if (size < 0) size = 0;
		else if (size > 0x7FFF) size = 0x7FFF;
		(*result)->size = (int16_t)size;
	}
	else if (flags & piuStyleRelativeSize) {
		const PiuStyleBuilder* builder;
		for (builder = piuStyleSizeBuilders; builder->name; builder++) {
			if ((*result)->size == (int16_t)builder->value) {
				builder += (*self)->size;
				if ((piuStyleSizeBuilders < builder) && (builder->name))
					(*result)->size = (int16_t)builder->value;
				break;
			}
		}
	}
	if (flags & piuStyleAbsoluteWeight)
		(*result)->weight = (*self)->weight;
	else if (flags & piuStyleRelativeWeight) {
		int16_t weight = (*result)->weight + (*self)->weight;
		if (weight < 1) weight = 1;
		else if (weight > 9) weight = 0;
	}
	if (flags & piuStyleHorizontal)
		(*result)->horizontal = (*self)->horizontal;
	if (flags & piuStyleVertical)
		(*result)->vertical = (*self)->vertical;
	if (flags & piuStyleIndentation)
		(*result)->indentation = (*self)->indentation;
	if (flags & piuStyleLeading)
		(*result)->leading = (*self)->leading;
	if (flags & piuStyleMarginLeft)
		(*result)->margins.left = (*self)->margins.left;
	if (flags & piuStyleMarginTop)
		(*result)->margins.top = (*self)->margins.top;
	if (flags & piuStyleMarginRight)
		(*result)->margins.right = (*self)->margins.right;
	if (flags & piuStyleMarginBottom)
		(*result)->margins.bottom = (*self)->margins.bottom;
	if (flags & piuStyleColor) {
		(*result)->color[0] = (*self)->color[0];
		(*result)->color[1] = (*self)->color[1];
		(*result)->color[2] = (*self)->color[2];
		(*result)->color[3] = (*self)->color[3];
	}
	if (flags & piuStyleDecoration) {
		if (flags & piuStyleUnderline)
			(*result)->flags |= piuStyleDecorationBits;
		else
			(*result)->flags &= ~piuStyleDecorationBits;
	}
}

#ifdef piuGPU
void PiuStyleBind(PiuStyle* self, PiuApplication* application, PiuView* view)
{
	PiuFont* font = (*self)->font;
	if (font) {
		PiuFontBind(font, application, view);
	}
}

void PiuStyleUnbind(PiuStyle* self, PiuApplication* application, PiuView* view)
{
	PiuFont* font = (*self)->font;
	if (font) {
		PiuFontUnbind(font, application, view);
	}
}
#endif			

void PiuStyle_get_bottom(xsMachine* the)
{
	PiuStyle* self = PIU(Style, xsThis);
	if ((*self)->flags & piuStyleMarginBottom)
		xsResult = xsPiuCoordinate((*self)->margins.bottom);
}

void PiuStyle_get_color(xsMachine* the)
{
	PiuStyle* self = PIU(Style, xsThis);
	xsVars(1);
	if ((*self)->flags & piuStyleColor)
		PiuColorsSerialize(the, (*self)->color);
}

void PiuStyle_get_family(xsMachine* the)
{
	PiuStyle* self = PIU(Style, xsThis);
	if ((*self)->flags & piuStyleFamily)
		xsResult = xsString(xsName((*self)->family));
}

void PiuStyle_get_horizontal(xsMachine* the)
{
	PiuStyle* self = PIU(Style, xsThis);
	if ((*self)->flags & piuStyleHorizontal) {
		switch((*self)->horizontal) {
		case piuCenter: xsResult = xsString("center"); break;
		case piuLeft: xsResult = xsString("left"); break;
		case piuRight: xsResult = xsString("right"); break;
		case piuLeftRight: xsResult = xsString("justify"); break;
		}
	}
}

void PiuStyle_get_indentation(xsMachine* the)
{
	PiuStyle* self = PIU(Style, xsThis);
	if ((*self)->flags & piuStyleIndentation)
		xsResult = xsPiuCoordinate((*self)->indentation);
}

void PiuStyle_get_leading(xsMachine* the)
{
	PiuStyle* self = PIU(Style, xsThis);
	if ((*self)->flags & piuStyleLeading)
		xsResult = xsPiuCoordinate((*self)->leading);
}

void PiuStyle_get_left(xsMachine* the)
{
	PiuStyle* self = PIU(Style, xsThis);
	if ((*self)->flags & piuStyleMarginLeft)
		xsResult = xsPiuCoordinate((*self)->margins.left);
}

void PiuStyle_get_margins(xsMachine* the)
{
	PiuStyle* self = PIU(Style, xsThis);
	xsResult = xsNewObject();
	if ((*self)->flags & piuStyleMarginLeft) 
		xsDefine(xsResult, xsID_left, xsPiuCoordinate((*self)->margins.left), xsDefault);
	if ((*self)->flags & piuStyleMarginRight)
		xsDefine(xsResult, xsID_right, xsPiuCoordinate((*self)->margins.right), xsDefault);
	if ((*self)->flags & piuStyleMarginTop)
		xsDefine(xsResult, xsID_top, xsPiuCoordinate((*self)->margins.top), xsDefault);
	if ((*self)->flags & piuStyleMarginBottom)
		xsDefine(xsResult, xsID_bottom, xsPiuCoordinate((*self)->margins.bottom), xsDefault);
}

void PiuStyle_get_right(xsMachine* the)
{
	PiuStyle* self = PIU(Style, xsThis);
	if ((*self)->flags & piuStyleMarginRight)
		xsResult = xsPiuCoordinate((*self)->margins.right);
}

void PiuStyle_get_size(xsMachine* the)
{
	PiuStyle* self = PIU(Style, xsThis);
	if ((*self)->flags & piuStyleAbsoluteSize) {
		char buffer[16] = "";
		fxIntegerToString(NULL, (*self)->size, buffer, sizeof(buffer));
		c_strcat(buffer, "px");
		xsResult = xsString(buffer);
	}
	else if ((*self)->flags & piuStylePercentageSize) {
		char buffer[16] = "";
		fxIntegerToString(NULL, (*self)->size, buffer, sizeof(buffer));
		c_strcat(buffer, "%");
		xsResult = xsString(buffer);
	}
	else if ((*self)->flags & piuStyleRelativeSize) {
		if ((*self)->size < 0)
			xsResult = xsString("smaller");
		else
			xsResult = xsString("larger");
	}
}

void PiuStyle_get_stretch(xsMachine* the)
{
	PiuStyle* self = PIU(Style, xsThis);
	if ((*self)->flags & piuStyleCondensed) {
		if ((*self)->flags & piuStyleCondensedBit)
			xsResult = xsString("condensed");
		else
			xsResult = xsString("normal");
	}
}

void PiuStyle_get_style(xsMachine* the)
{
	PiuStyle* self = PIU(Style, xsThis);
	if ((*self)->flags & piuStyleItalic) {
		if ((*self)->flags & piuStyleItalicBit)
			xsResult = xsString("italic");
		else
			xsResult = xsString("normal");
	}
}

void PiuStyle_get_top(xsMachine* the)
{
	PiuStyle* self = PIU(Style, xsThis);
	if ((*self)->flags & piuStyleMarginTop)
		xsResult = xsPiuCoordinate((*self)->margins.top);
}

void PiuStyle_get_vertical(xsMachine* the)
{
	PiuStyle* self = PIU(Style, xsThis);
	if ((*self)->flags & piuStyleVertical) {
		switch((*self)->vertical) {
		case piuMiddle: xsResult = xsString("middle"); break;
		case piuTop: xsResult = xsString("top"); break;
		case piuBottom: xsResult = xsString("bottom"); break;
		}
	}
}

void PiuStyle_get_weight(xsMachine* the)
{
	PiuStyle* self = PIU(Style, xsThis);
	if ((*self)->flags & piuStyleAbsoluteWeight) {
		xsResult = xsInteger((*self)->weight * 100);
	}
	else if ((*self)->flags & piuStyleRelativeWeight) {
		if ((*self)->size < 0)
			xsResult = xsString("lighter");
		else
			xsResult = xsString("bolder");
	}
}

void PiuStyle_measure(xsMachine* the)
{
	PiuStyle* self = PIU(Style, xsThis);
	(void)xsToString(xsArg(0));
	if ((*self)->font == NULL)
		PiuStyleLookupFont(self);
	if ((*self)->font != NULL) {
		PiuDimension width = PiuStyleGetWidth(self, &xsArg(0));
		PiuDimension height = PiuStyleGetHeight(self, &xsArg(0));
		xsResult = xsNewObject();
		xsDefine(xsResult, xsID_width, xsPiuDimension(width), xsDefault);
		xsDefine(xsResult, xsID_height, xsPiuDimension(height), xsDefault);
	}
}
