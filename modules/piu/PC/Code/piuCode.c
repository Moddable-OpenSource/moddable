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

#include "piuCode.h"

enum {
	teCharDone = 0,
	teCharWhiteSpace,
	teCharText,
	teCharSeparator
};
static const char *teSeparators = ".,:;-()\"\\/?<>{}[]+=-!@#%^&*~`|";

static void PiuCodeBind(void* it, PiuApplication* application, PiuView* view);
static void PiuCodeCascade(void* it);
static int32_t PiuCodeClassifyCharacter(int32_t character);
static void PiuCodeComputeStyle(PiuCode* self);
static void PiuCodeDictionary(xsMachine* the, void* it);
static void PiuCodeDraw(void* it, PiuView* view, PiuRectangle area);
static int32_t PiuCodeFindBlockBackwards(PiuCode* self, PiuCodeIterator iter, char d);
static int32_t PiuCodeFindBlockForwards(PiuCode* self, PiuCodeIterator iter, char d);
static void PiuCodeFindColor(PiuCode* self, int32_t hit, int32_t* from, int32_t* to);
static int32_t PiuCodeFindWordBreak(PiuCode* self, int32_t result, int32_t direction);
static void PiuCodeFormat(PiuCode* self);
static void PiuCodeHilite(PiuCode* self, PiuView* view, int32_t fromColumn, int32_t fromLine, int32_t toColumn, int32_t toLine);
static int32_t PiuCodeHitOffset(PiuCode* self, double x, double y);
static void PiuCodeInvalidate(void* it, PiuRectangle area);
static void PiuCodeMark(xsMachine* the, void* it, xsMarkRoot markRoot);
static void PiuCodeMeasureHorizontally(void* it);
static void PiuCodeMeasureVertically(void* it);
static void PiuCodeOffsetToColumnLine(PiuCode* self, int32_t offset, int32_t* offsetColumn, int32_t* offsetLine);
static void PiuCodeSearch(PiuCode* self, uint32_t size);
static void PiuCodeSelect(PiuCode* self, int32_t selectionOffset, uint32_t selectionLength);
static void PiuCodeUnbind(void* it, PiuApplication* application, PiuView* view);

static void PiuCodeIteratorFirst(PiuCode* self, PiuCodeIterator iter, int32_t offset);
static void PiuCodeIteratorLast(PiuCode* self, PiuCodeIterator iter, int32_t offset);
static xsBooleanValue PiuCodeIteratorNext(PiuCode* self, PiuCodeIterator iter);
static xsBooleanValue PiuCodeIteratorPrevious(PiuCode* self, PiuCodeIterator iter);

const PiuDispatchRecord ICACHE_FLASH_ATTR PiuCodeDispatchRecord = {
	"Code",
	PiuCodeBind,
	PiuCodeCascade,
	PiuCodeDraw,
	PiuContentFitHorizontally,
	PiuContentFitVertically,
	PiuContentHit,
	PiuContentIdle,
	PiuCodeInvalidate,
	PiuCodeMeasureHorizontally,
	PiuCodeMeasureVertically,
	PiuContentPlace,
	NULL,
	NULL,
	PiuContentReflow,
	PiuContentShowing,
	PiuContentShown,
	PiuContentSync,
	PiuCodeUnbind,
	PiuContentUpdate
};

const xsHostHooks ICACHE_FLASH_ATTR PiuCodeHooks = {
	PiuContentDelete,
	PiuCodeMark,
	NULL
};

int32_t fxUTF8Advance(xsStringValue string, int32_t offset, int32_t direction)
{
	uint8_t* text = (uint8_t*)string + offset;
	if (direction >= 0) {
		unsigned char aByte = (unsigned char)*text;
		if (!(aByte & 0x80))
			return 1;
		if ((aByte & 0xe0) != 0xe0)
			return 2;
		if ((aByte & 0xf0) != 0xf0)
			return 3;
		if ((aByte & 0xf8) == 0xf0)
			return 4;
	}
	else {
		if ((text[-1] & 0xc0) == 0x80) {
			if ((text[-2] & 0xc0) == 0x80) {
				if ((text[-3] & 0xc0) == 0x80) {
					if ((text[-4] & 0xc0) == 0x80)
						return -4;
				}
				else
					return -3;
			}
			else
				return -2;
		}
		else
			return -1;
	}

	return 0;			// error
}

void PiuCodeBind(void* it, PiuApplication* application, PiuView* view)
{
	PiuCode* self = it;
	PiuContentBind(it, application, view);
	PiuCodeComputeStyle(self);
}

void PiuCodeCascade(void* it)
{
	PiuCode* self = it;
	PiuContentCascade(it);
	PiuCodeComputeStyle(self);
	PiuContentReflow(self, piuSizeChanged);
}

int32_t PiuCodeClassifyCharacter(int32_t character)
{
	const char *c;

	if (0 == character)
		return teCharDone;

	if ((' ' == character) || (9 == character) || (10 == character) || (13 == character) || (12288 == character))		// 12288: double-byte space
		return teCharWhiteSpace;

	c = teSeparators;
	while (*c)
		if (*c++ == (char)character)
			return teCharSeparator;

	return teCharText;
}

void PiuCodeComputeStyle(PiuCode* self)
{
	xsMachine* the = (*self)->the;
	PiuApplication* application = (*self)->application;
	PiuContainer* container = (PiuContainer*)self;
	PiuStyleLink* list = (*application)->styleList;
	PiuStyleLink* chain = NULL;
	while (container) {
		PiuStyle* style = (*container)->style;
		if (style) {
			list = PiuStyleLinkMatch(the, list, chain, style);
			chain = list;
		}
		container = (*container)->container;
	}
	if (chain) {
		PiuStyle* result = PiuStyleLinkCompute(the, chain, application);
		(*self)->computedStyle = result;
		xsResult = xsString("w");
		(*self)->columnWidth = PiuFontGetWidthSubPixel((*result)->font, &xsResult, 0, 1);
		(*self)->lineHeight = PiuFontGetHeight((*result)->font);
	}
}

void PiuCodeDictionary(xsMachine* the, void* it) 
{
	PiuCode* self = it;
	xsSlot* slot;
	xsStringValue string;
	if (!xsFindResult(xsArg(1), xsID_string))
		xsResult = xsString("");
	slot = PiuString(xsResult);
	string = PiuToString(slot);
	(*self)->string = slot;
	(*self)->size = c_strlen(string);
	(*self)->length = fxUnicodeLength(string);
	if (xsFindString(xsArg(1), xsID_type, &string)) {
		if (!c_strcmp(string, "js"))
			(*self)->type = 1;
		else if (!c_strcmp(string, "json"))
			(*self)->type = 2;
		else if (!c_strcmp(string, "xml"))
			(*self)->type = 3;
	}
	PiuCodeFormat(self);
	PiuCodeSearch(self, (*self)->size);
	PiuCodeSelect(self, 0, 0);
}

void PiuCodeIntersect(int32_t l1, int32_t r1, int32_t l2, int32_t r2, int32_t segments[6])
{
	segments[0] = (l1 < l2) ? l1 : l2;
	segments[1] = (r1 < l2) ? r1 : l2;

	segments[2] = (l1 < l2) ? l2 : l1;
	segments[3] = (r1 < r2) ? r1 : r2;
	
	segments[4] = (l1 < r2) ? r2 : l1;
	segments[5] = (r1 < r2) ? r2 : r1;
}

void PiuCodeDraw(void* it, PiuView* view, PiuRectangle area)
{
	PiuCode* self = it;
	xsMachine* the = (*self)->the;
	PiuSkin* skin = (*self)->skin;
	PiuStyle* style = (*self)->computedStyle;
	PiuTextBuffer* results = (*self)->results;
	if (skin) {
		PiuCodeResult result = (PiuCodeResult)((char*)(*results) + sizeof(PiuTextBufferRecord));
		if (result->count > 0) {
			PiuViewPushColor(view, &((*skin)->data.color.fill[0]));
			while (result->count > 0) {
				PiuCodeHilite(self, view, result->fromColumn, result->fromLine, result->toColumn, result->toLine);
				result = (PiuCodeResult)(((char*)result) + (*self)->resultsSize);
			}
			PiuViewPopColor(view);
		}
		PiuViewPushColor(view, &((*skin)->data.color.fill[3]));
		PiuCodeHilite(self, view, (*self)->fromColumn, (*self)->fromLine, (*self)->toColumn, (*self)->toLine);
		PiuViewPopColor(view);
	}
	{
		int32_t selectionFrom = (*self)->from;
		int32_t selectionTo = (*self)->to;
		double clipLeft = area->x;
		double clipRight = clipLeft + area->width;
		double clipTop = area->y;
		double clipBottom = clipTop + area->height;
		xsBooleanValue flag = (clipTop <= ((*style)->margins.top + (*self)->lineHeight)) ? 1 : 0;
		double x = (*style)->margins.left;
		double y = (*style)->margins.top;
		double columnWidth = (*self)->columnWidth;
		double lineHeight = (*self)->lineHeight;
		xsStringValue string = PiuToString((*self)->string);
		PiuTextBuffer* runs = (*self)->runs;
		PiuCodeRun run = (PiuCodeRun)((uint8_t*)(*runs) + sizeof(PiuTextBufferRecord));
		xsIntegerValue offset = 0;
		while (*(string + offset)) {
			switch(run->kind) {
			case piuCodeLineKind:
				x = (*style)->margins.left;
				y += lineHeight;
				if (clipBottom <= y)
					return;
				if (clipTop <= (y + lineHeight))
					flag = 1;
				break;
			case piuCodeTabKind:
				if (flag)
					x += run->color * columnWidth;
				break;
			default:
				if (flag && run->count) {
					double right = x + (fxUTF8ToUnicodeOffset(string + offset, run->count) * columnWidth);
					if ((clipLeft < right) && (x < clipRight)) {
						PiuColorRecord runColor = (*style)->color[run->color];
						PiuColorRecord black = { 0, 0, 0, 255 };
						if ((*self)->variant == 1) {
							int32_t from = offset;
							int32_t to = offset + run->count;
							int32_t to0 = (to < selectionFrom) ? to : selectionFrom;
							int32_t to1 = (to < selectionTo) ? to : selectionTo;
							if (from < to0) {
								PiuViewPushColor(view, &runColor);
								PiuViewDrawStringSubPixel(view, (*self)->string, from, to0 - from, (*style)->font, x, y, 0, 0);
								PiuViewPopColor(view);
								x += fxUTF8ToUnicodeOffset(string + from, to0 - from) * columnWidth;
								from = to0;
							}
							if (from < to1) {
								PiuViewPushColor(view, &black);
								PiuViewDrawStringSubPixel(view, (*self)->string, from, to1 - from, (*style)->font, x, y, 0, 0);
								PiuViewPopColor(view);
								x += fxUTF8ToUnicodeOffset(string + from, to1 - from) * columnWidth;
								from = to1;
							}
							if (from < to) {
								PiuViewPushColor(view, &runColor);
								PiuViewDrawStringSubPixel(view, (*self)->string, from, to - from, (*style)->font, x, y, 0, 0);
								PiuViewPopColor(view);
								x += fxUTF8ToUnicodeOffset(string + from, to - from) * columnWidth;
								from = to;
							}
						}
						else {
							PiuCodeResult result = (PiuCodeResult)((char*)(*results) + sizeof(PiuTextBufferRecord));
							if (result->count > 0) {
								int32_t from = offset;
								int32_t to = offset + run->count;
								while ((result->count > 0) && (result->to < from)) {
									result = (PiuCodeResult)(((char*)result) + (*self)->resultsSize);
								}
								while ((result->count > 0) && (result->from < to)) {
									if (from < result->from) {
										PiuViewPushColor(view, &runColor);
										PiuViewDrawStringSubPixel(view, (*self)->string, from, result->from - from, (*style)->font, x, y, 0, 0);
										PiuViewPopColor(view);
										x += fxUTF8ToUnicodeOffset(string + from, result->from - from) * columnWidth;
										from = result->from;
									}
									int32_t resultTo = (result->to < to) ? result->to : to;
									int32_t to0 = (resultTo < selectionFrom) ? resultTo : selectionFrom;
									int32_t to1 = (resultTo < selectionTo) ? resultTo : selectionTo;
								
									if (from < to0) {
										PiuViewPushColor(view, &black);
										PiuViewDrawStringSubPixel(view, (*self)->string, from, to0 - from, (*style)->font, x, y, 0, 0);
										PiuViewPopColor(view);
										x += fxUTF8ToUnicodeOffset(string + from, to0 - from) * columnWidth;
										from = to0;
									}
									if (from < to1) {
										PiuViewPushColor(view, &runColor);
										PiuViewDrawStringSubPixel(view, (*self)->string, from, to1 - from, (*style)->font, x, y, 0, 0);
										PiuViewPopColor(view);
										x += fxUTF8ToUnicodeOffset(string + from, to1 - from) * columnWidth;
										from = to1;
									}
									if (from < resultTo) {
										PiuViewPushColor(view, &black);
										PiuViewDrawStringSubPixel(view, (*self)->string, from, resultTo - from, (*style)->font, x, y, 0, 0);
										PiuViewPopColor(view);
										x += fxUTF8ToUnicodeOffset(string + from, resultTo - from) * columnWidth;
										from = resultTo;
									}
								
									if (result->to < to) {
										PiuViewPushColor(view, &black);
										PiuViewDrawStringSubPixel(view, (*self)->string, from, result->to - from, (*style)->font, x, y, 0, 0);
										PiuViewPopColor(view);
										x += fxUTF8ToUnicodeOffset(string + from, result->to - from) * columnWidth;
										from = result->to;
									}
									else {
										PiuViewPushColor(view, &black);
										PiuViewDrawStringSubPixel(view, (*self)->string, from, to - from, (*style)->font, x, y, 0, 0);
										PiuViewPopColor(view);
										x += fxUTF8ToUnicodeOffset(string + from, to - from) * columnWidth;
										from = to;
									}
									result = (PiuCodeResult)(((char*)result) + (*self)->resultsSize);
								}
								if (from < to) {
									PiuViewPushColor(view, &runColor);
									PiuViewDrawStringSubPixel(view, (*self)->string, from, to - from, (*style)->font, x, y, 0, 0);
									PiuViewPopColor(view);
								}
							}
							else {
								PiuViewPushColor(view, &((*style)->color[run->color]));
								PiuViewDrawStringSubPixel(view, (*self)->string, offset, run->count, (*style)->font, x, y, 0, 0);
								PiuViewPopColor(view);
							}
						}
					}
					x = right;
				}
				break;
			}
			offset += run->count;
			run++;
		}	
	}	
}

int32_t PiuCodeFindBlockBackwards(PiuCode* self, PiuCodeIterator iter, char d)
{
	while (PiuCodeIteratorPrevious(self, iter)) {
		if (iter->color == 0) {
			char c = iter->character;
			int32_t result = iter->offset;
			if (c == d)
				return result;
			if (c == ')')
				result = PiuCodeFindBlockBackwards(self, iter, '(');
			else if (c == ']')
				result = PiuCodeFindBlockBackwards(self, iter, '[');
			else if (c == '}')
				result = PiuCodeFindBlockBackwards(self, iter, '{');
			if (result < 0)
				return result;
		}
	}
	return -1;
}

int32_t PiuCodeFindBlockForwards(PiuCode* self, PiuCodeIterator iter, char d)
{
	while (PiuCodeIteratorNext(self, iter)) {
		if (iter->color == 0) {
			char c = iter->character;
			int32_t result = iter->offset;
			if (c == d)
				return result;
			if (c == '(')
				result = PiuCodeFindBlockForwards(self, iter, ')');
			else if (c == '[')
				result = PiuCodeFindBlockForwards(self, iter, ']');
			else if (c == '{')
				result = PiuCodeFindBlockForwards(self, iter, '}');
			if (result < 0)
				return result;
		}
	}
	return -1;
}

void PiuCodeFindColor(PiuCode* self, int32_t hit, int32_t* from, int32_t* to)
{
	xsMachine* the = (*self)->the;
	xsStringValue string = PiuToString((*self)->string);
	PiuTextBuffer* runs = (*self)->runs;
	PiuCodeRun run = (PiuCodeRun)((uint8_t*)(*runs) + sizeof(PiuTextBufferRecord));
	int16_t color = 0;
	int32_t start = 0;
	int32_t stop = 0;
	while (*string) {
		if (run->kind == piuCodeColorKind) {
			if (run->color != color) {
				color = run->color;
				start = stop;
			}
		}
		stop += run->count;
		if ((start <= hit) && (hit < stop))
			break;
		string += run->count;
		run++;
	}	
	*from = start;
	*to = stop;
}

int32_t PiuCodeFindWordBreak(PiuCode* self, int32_t result, int32_t direction)
{
	if ((direction < 0) && (result == 0))
		result = 0;
	else if ((direction > 0) && (result >= (int32_t)(*self)->length))
		result = (*self)->length;
	else {
		xsMachine* the = (*self)->the;
		xsStringValue string = PiuToString((*self)->string);
		int32_t character;
		int32_t classification;
		if (direction > 0)
			string += fxUnicodeToUTF8Offset(string, result);
		else
			string += fxUnicodeToUTF8Offset(string, result - 1);
		fxUTF8Decode(string, &character);
		classification = PiuCodeClassifyCharacter(character);
		do {
			int32_t advance = fxUTF8Advance(string, 0, direction);
			if (0 == advance)
				break;
			string += advance;
			result += direction;
			fxUTF8Decode(string, &character);
		} while (PiuCodeClassifyCharacter(character) == classification);
	}
	return result;
}

void PiuCodeFormat(PiuCode* self) 
{
	xsMachine* the = (*self)->the;
	PiuCodeParserRecord parserRecord;
	PiuCodeParser parser = &parserRecord;
	xsTry {
		PiuCodeParserBegin(self, parser);
		if ((*self)->type == 1)
			PiuCodeFormatJS(parser);
		else if ((*self)->type == 2)
			PiuCodeFormatJSON(parser);
		else if ((*self)->type == 3)
			PiuCodeFormatXML(parser);
		PiuCodeParserEnd(self, parser);
	}
	xsCatch {
		PiuCodeParserEnd(self, parser);
	}
}

void PiuCodeHilite(PiuCode* self, PiuView* view, int32_t fromColumn, int32_t fromLine, int32_t toColumn, int32_t toLine)
{
	PiuStyle* style = (*self)->computedStyle;
	PiuCoordinate x, y;
	PiuDimension width, height;
	int32_t line = toLine - fromLine;
	if (line == 0) {
		int32_t column = toColumn - fromColumn;
		x = (PiuCoordinate)c_floor((*style)->margins.left + (fromColumn * (*self)->columnWidth));
		if (column == 0) {
			x--;
			width = 2;
		}
		else
			width = (PiuDimension)c_ceil((toColumn - fromColumn) * (*self)->columnWidth);
		y = (PiuCoordinate)c_floor((*style)->margins.top + (fromLine * (*self)->lineHeight));
		height = (PiuDimension)c_ceil((*self)->lineHeight);
		PiuViewFillColor(view, x, y, width, height);
	}
	else {
		x = (PiuCoordinate)c_floor((*style)->margins.left + (fromColumn * (*self)->columnWidth));
		width = 0x7FFF;
		y = (PiuCoordinate)c_floor((*style)->margins.top + (fromLine * (*self)->lineHeight));
		height = (PiuDimension)c_ceil((*self)->lineHeight);
		PiuViewFillColor(view, x, y, width, height);
		if (line > 1) {
			x = (*style)->margins.left;
			width = 0x7FFF;
			y = (PiuCoordinate)c_floor((*style)->margins.top + ((fromLine + 1) * (*self)->lineHeight));
			height = (PiuDimension)c_ceil((line - 1) * (*self)->lineHeight);
			PiuViewFillColor(view, x, y, width, height);
		}
		x = (*style)->margins.left;
		width = (PiuDimension)c_ceil(toColumn * (*self)->columnWidth);
		y = (PiuCoordinate)c_floor((*style)->margins.top + (toLine * (*self)->lineHeight));
		height = (PiuDimension)c_ceil((*self)->lineHeight);
		PiuViewFillColor(view, x, y, width, height);
	}
}

int32_t PiuCodeHitOffset(PiuCode* self, double x, double y)
{
	xsMachine* the = (*self)->the;
	xsStringValue string = PiuToString((*self)->string);
	PiuStyle* style = (*self)->computedStyle;
	xsBooleanValue flag = 0;
	double left = 0;
	double right = 0;
	double top = 0;
	double bottom = (*self)->lineHeight;
	PiuTextBuffer* runs = (*self)->runs;
	PiuCodeRun run = (PiuCodeRun)((uint8_t*)(*runs) + sizeof(PiuTextBufferRecord));
	xsStringValue text = string;
	x -= (*style)->margins.left;
	y -= (*style)->margins.top;
	if (x < 0)
		x = 0;
	if (y < 0)
		y = 0;
	x += (*self)->columnWidth / 2;
	if ((top <= y) && (y < bottom))
		flag = 1;
	while (*text) {
		switch(run->kind) {
		case piuCodeLineKind:
			if (flag)
				return fxUTF8ToUnicodeOffset(string, text - string);
			left = 0;
			top = bottom;
			bottom += (*self)->lineHeight;
			if ((top <= y) && (y < bottom))
				flag = 1;
			break;
		case piuCodeTabKind:
			if (flag) {
				right = left + (run->color * (*self)->columnWidth);
				if ((left <= x) && (x < right))
					return fxUTF8ToUnicodeOffset(string, text - string);
				left = right;
			}
			break;
		default:
			if (flag) {
				right = left + (fxUTF8ToUnicodeOffset(string, run->count) * (*self)->columnWidth);
				if ((left <= x) && (x < right))
					return fxUTF8ToUnicodeOffset(string, text - string) + fxUnicodeToUTF8Offset(text, (xsIntegerValue)((x - left) / (*self)->columnWidth));
				left = right;
			}
			break;
		}
		text += run->count;
		run++;
	}	
	return (*self)->length;
}

void PiuCodeInvalidate(void* it, PiuRectangle area) 
{
	PiuContent* self = it;
	if ((*self)->flags & piuVisible) {
		PiuContainer* container = (*self)->container;
		if (container) {
			PiuRectangleRecord bounds;
			PiuRectangleSet(&bounds, (*self)->bounds.x, (*self)->bounds.y, 0x7FFF, (*self)->bounds.height);
			(*(*container)->dispatch->invalidate)((*self)->container, &bounds);
		}
	}
}

void PiuCodeMark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	PiuCode self = it;
	PiuContentMark(the, it, markRoot);
	PiuMarkString(the, self->string);
	PiuMarkHandle(the, self->results);
	PiuMarkHandle(the, self->runs);
}

void PiuCodeMeasureHorizontally(void* it) 
{
	PiuCode* self = it;
	PiuStyle* style = (*self)->computedStyle;
	uint16_t horizontal = (*self)->coordinates.horizontal & piuLeftRightWidth;
	if ((horizontal != piuLeftRight) && (!(horizontal & piuWidth))) {
		(*self)->coordinates.width = (PiuDimension)c_ceil((*style)->margins.left + ((*self)->columnCount * (*self)->columnWidth) + (*style)->margins.right);
	}
}

void PiuCodeMeasureVertically(void* it) 
{
	PiuCode* self = it;
	PiuStyle* style = (*self)->computedStyle;
	uint16_t vertical = (*self)->coordinates.vertical & piuTopBottomHeight;
	if ((vertical != piuTopBottom) && (!(vertical & piuHeight))) {
		(*self)->coordinates.height = (PiuDimension)c_ceil((*style)->margins.top + ((*self)->lineCount * (*self)->lineHeight) + (*style)->margins.bottom);
	}
}

void PiuCodeOffsetToColumnLine(PiuCode* self, int32_t offset, int32_t* offsetColumn, int32_t* offsetLine) 
{
	xsMachine* the = (*self)->the;
	xsStringValue string = PiuToString((*self)->string);
	PiuTextBuffer* runs = (*self)->runs;
	PiuCodeRun run = (PiuCodeRun)((uint8_t*)(*runs) + sizeof(PiuTextBufferRecord));
	int32_t lineIndex = 0;
	int32_t columnIndex = 0;
	while (*string) {
		switch(run->kind) {
		case piuCodeLineKind:
			if (offset == 0) 
				goto bail;
			lineIndex++;
			columnIndex = 0;
			break;
		case piuCodeTabKind:
			if (offset == 0) 
				goto bail;
			columnIndex += run->color;
			break;
		default:
			if (offset < run->count) {
				columnIndex += fxUTF8ToUnicodeOffset(string, offset);
				goto bail;
			}
			columnIndex += fxUTF8ToUnicodeOffset(string, run->count);
			break;
		}
		offset -= run->count;
		string += run->count;
		run++;
	}	
bail:
	*offsetColumn = columnIndex;
	*offsetLine = lineIndex;
}

void PiuCodeSearch(PiuCode* self, uint32_t size)
{
	xsMachine* the = (*self)->the;
	xsStringValue string;
	PiuTextBuffer* results = (*self)->results;
	size_t former;
	PiuCodeResult result;
	PiuTextBufferClear(the, results);
	if ((*self)->code) {
        int32_t offset = 0;
		int32_t itemCount = 0;
		int32_t itemSize = 0;
		itemCount = 0;
		itemSize = sizeof(PiuCodeResultRecord);
		for (;;) {
			former = (*results)->current;
			PiuTextBufferGrow(the, results, itemSize);
			result = (PiuCodeResult)((char*)(*results) + former);
			string = PiuToString((*self)->string);
			result->count = fxMatchRegExp(NULL, (*self)->code, (*self)->data, string, offset);
            if (result->count <= 0) {
                break;
            }
            if ((*self)->data[0] == (*self)->data[1]) {
                break;
            }
			offset = (*self)->data[1];
			result->from = (*self)->data[0];
			result->to = offset;
			PiuCodeOffsetToColumnLine(self, result->from, &result->fromColumn, &result->fromLine);
			PiuCodeOffsetToColumnLine(self, result->to, &result->toColumn, &result->toLine);
			result->from = fxUTF8ToUnicodeOffset(string, result->from);
			result->to = fxUTF8ToUnicodeOffset(string, result->to);
			itemCount++;
		}
		(*self)->resultsSize = itemSize;
	}
	else {
		former = (*results)->current;
		PiuTextBufferGrow(the, results, sizeof(PiuCodeResultRecord));
		result = (PiuCodeResult)((char*)(*results) + former);
		result->count = -1;
		(*self)->resultsSize = sizeof(PiuCodeResultRecord);
	}
}

void PiuCodeSelect(PiuCode* self, int32_t selectionOffset, uint32_t selectionLength)
{
	xsMachine* the = (*self)->the;
	xsStringValue string = PiuToString((*self)->string);
	int32_t from = selectionOffset;
	int32_t to = selectionOffset + selectionLength;
	int32_t offset;
	if (from < 0)
		from = 0;
	else if (from > (int32_t)(*self)->length)
		from = (*self)->length;
	if (to < 0)
		to = 0;
	else if (to > (int32_t)(*self)->length)
		to = (*self)->length;
	(*self)->from = from;
	(*self)->to = to;
	offset = fxUnicodeToUTF8Offset(string, from);
	PiuCodeOffsetToColumnLine(self, offset, &(*self)->fromColumn, &(*self)->fromLine);
	if (to == from) {
		(*self)->toColumn = (*self)->fromColumn;
		(*self)->toLine = (*self)->fromLine;
	}
	else {
		offset = fxUnicodeToUTF8Offset(string, (*self)->to);
		PiuCodeOffsetToColumnLine(self, offset, &(*self)->toColumn, &(*self)->toLine);
	}
	PiuCodeInvalidate(self, NULL);
}

void PiuCodeUnbind(void* it, PiuApplication* application, PiuView* view)
{
	PiuCode* self = it;
	(*self)->computedStyle = NULL;
	PiuContentUnbind(it, application, view);
}

void PiuCodeIteratorFirst(PiuCode* self, PiuCodeIterator iter, int32_t offset)
{
	PiuTextBuffer* runs = (*self)->runs;
	iter->run = (PiuCodeRun)((uint8_t*)(*runs) + sizeof(PiuTextBufferRecord));
	iter->offset = 0;
	if (iter->run->kind == piuCodeLineKind)
		iter->limit = iter->offset + 1;
	else if (iter->run->kind == piuCodeTabKind)
		iter->limit = iter->offset + 1;
	else {
		iter->color = iter->run->color;
		iter->limit = iter->offset + iter->run->count;
	}
	while (PiuCodeIteratorNext(self, iter)) {
		if (iter->offset == offset)
			break;
	}
}

void PiuCodeIteratorLast(PiuCode* self, PiuCodeIterator iter, int32_t offset)
{
	PiuTextBuffer* runs = (*self)->runs;
	iter->run = (PiuCodeRun)((uint8_t*)(*runs) + (*runs)->current - sizeof(PiuCodeRunRecord));
	iter->offset = (*self)->size;
	if (iter->run->kind == piuCodeLineKind)
		iter->limit = iter->offset - 1;
	else if (iter->run->kind == piuCodeTabKind)
		iter->limit = iter->offset - 1;
	else {
		iter->color = iter->run->color;
		iter->limit = iter->offset - iter->run->count;
	}
	while (PiuCodeIteratorPrevious(self, iter)) {
		if (iter->offset == offset)
			break;
	}
}


xsBooleanValue PiuCodeIteratorNext(PiuCode* self, PiuCodeIterator iter)
{
	xsMachine* the = (*self)->the;
	iter->offset++;
	iter->character = *(PiuToString((*self)->string) + iter->offset);
	if (!iter->character)
		return 0;
	if (iter->offset >= iter->limit) {
		iter->run++;
		if (iter->run->kind == piuCodeColorKind)
			iter->color = iter->run->color;
		iter->limit = iter->offset + iter->run->count;
	}
	return 1;
}

xsBooleanValue PiuCodeIteratorPrevious(PiuCode* self, PiuCodeIterator iter)
{
	xsMachine* the = (*self)->the;
	if (!iter->offset)
		return 0;
	if (iter->offset == iter->limit) {
		iter->run--;
		if (iter->run->kind == piuCodeColorKind)
			iter->color = iter->run->color;
		iter->limit = iter->offset - iter->run->count;
	}
	iter->offset--;
	iter->character = *(PiuToString((*self)->string) + iter->offset);
	return 1;
}


void PiuCode_create(xsMachine *the)
{
	PiuCode* self;
	xsVars(4);
	xsSetHostChunk(xsThis, NULL, sizeof(PiuCodeRecord));
	self = PIU(Code, xsThis);
	(*self)->the = the;
	(*self)->reference = xsToReference(xsThis);
	xsSetHostHooks(xsThis, (xsHostHooks*)&PiuCodeHooks);
	(*self)->dispatch = (PiuDispatch)&PiuCodeDispatchRecord;
	(*self)->flags = piuVisible | piuActive;
	PiuContentDictionary(the, self);
	PiuTextBufferNew(the, 512);
	(*self)->results = PIU(TextBuffer, xsResult);
	PiuTextBufferNew(the, 512);
	(*self)->runs = PIU(TextBuffer, xsResult);
	PiuCodeDictionary(the, self);
	PiuBehaviorOnCreate(self);
}

void PiuCode_get_columnCount(xsMachine *the)
{
	PiuCode* self = PIU(Code, xsThis);
	xsResult = xsInteger((*self)->columnCount);
}

void PiuCode_get_columnWidth(xsMachine *the)
{
	PiuCode* self = PIU(Code, xsThis);
	xsResult = xsNumber((*self)->columnWidth);
}

void PiuCode_get_length(xsMachine *the)
{
	PiuCode* self = PIU(Code, xsThis);
	xsResult = xsInteger((*self)->length);
}

void PiuCode_get_lineCount(xsMachine *the)
{
	PiuCode* self = PIU(Code, xsThis);
	xsResult = xsInteger((*self)->lineCount);
}

void PiuCode_get_lineHeight(xsMachine *the)
{
	PiuCode* self = PIU(Code, xsThis);
	xsResult = xsNumber((*self)->lineHeight);
}

void PiuCode_get_resultCount(xsMachine *the)
{
	PiuCode* self = PIU(Code, xsThis);
	PiuTextBuffer* results = (*self)->results;
	xsResult = xsInteger((((*results)->current - sizeof(PiuTextBufferRecord)) / (*self)->resultsSize) - 1);
}

void PiuCode_get_selectionBounds(xsMachine *the)
{
	PiuCode* self = PIU(Code, xsThis);
	if ((*self)->application) {
		PiuStyle* style = (*self)->computedStyle;
		int32_t line = (*self)->toLine - (*self)->fromLine;
		PiuRectangleRecord bounds;
		PiuRectangleSet(&bounds, 0, 0, (*self)->bounds.width, (*self)->bounds.height);
		if (line == 0) {
			bounds.x = (PiuCoordinate)(c_floor((*self)->fromColumn * (*self)->columnWidth));
			bounds.width = (PiuDimension)(c_ceil(((*self)->toColumn - (*self)->fromColumn) * (*self)->columnWidth));
			bounds.y = (PiuCoordinate)(c_floor((*self)->fromLine * (*self)->lineHeight));
			bounds.height = (PiuDimension)(c_ceil((*self)->lineHeight));
		}
		else {
			bounds.x = 0;
			bounds.width = (*self)->bounds.width;
			bounds.y = (PiuCoordinate)(c_floor((*self)->fromLine * (*self)->lineHeight));
			bounds.height = (PiuDimension)(c_ceil(line * (*self)->lineHeight));
		}
		bounds.x += (*style)->margins.left;
		bounds.y += (*style)->margins.top;
		xsResult = xsNewObject();
		xsDefine(xsResult, xsID_x, xsPiuCoordinate(bounds.x), xsDefault);
		xsDefine(xsResult, xsID_y, xsPiuCoordinate(bounds.y), xsDefault);
		xsDefine(xsResult, xsID_width, xsPiuDimension(bounds.width), xsDefault);
		xsDefine(xsResult, xsID_height, xsPiuDimension(bounds.height), xsDefault);
	}
}

void PiuCode_get_selectionLength(xsMachine *the)
{
	PiuCode* self = PIU(Code, xsThis);
	xsResult = xsInteger((*self)->to - (*self)->from);
}

void PiuCode_get_selectionOffset(xsMachine *the)
{
	PiuCode* self = PIU(Code, xsThis);
	xsResult = xsInteger((*self)->from);
}

void PiuCode_get_selectionString(xsMachine *the)
{
	PiuCode* self = PIU(Code, xsThis);
	xsResult = xsCall2(*((*self)->string), xsID_slice, xsInteger((*self)->from), xsInteger((*self)->to));
}

void PiuCode_get_string(xsMachine *the)
{
	PiuCode* self = PIU(Code, xsThis);
	xsResult = *((*self)->string);
}

void PiuCode_get_type(xsMachine *the)
{
	PiuCode* self = PIU(Code, xsThis);
	switch ((*self)->type) {
		case 1:
			xsResult = xsString("js");
			break;
		case 2:
			xsResult = xsString("json");
			break;
		case 3:
			xsResult = xsString("xml");
			break;
		default:
			xsResult = xsString("text");
			break;
	}
}

void PiuCode_set_string(xsMachine *the)
{
	PiuCode* self = PIU(Code, xsThis);
	xsSlot* slot = PiuString(xsArg(0));
	xsStringValue string = PiuToString(slot);
	(*self)->string = slot;
	(*self)->size = c_strlen(string);
	(*self)->length = fxUnicodeLength(string);
	PiuCodeFormat(self);
	PiuCodeSearch(self, (*self)->size);
	PiuCodeSelect(self, (*self)->from, (*self)->to - (*self)->from);
	PiuContentReflow(self, piuSizeChanged);
}

void PiuCode_set_type(xsMachine *the)
{
	PiuCode* self = PIU(Code, xsThis);
	xsStringValue string = xsToString(xsArg(0));
	if (!c_strcmp(string, "js"))
		(*self)->type = 1;
	else if (!c_strcmp(string, "json"))
		(*self)->type = 2;
	else if (!c_strcmp(string, "xml"))
		(*self)->type = 3;
	else
		(*self)->type = 0;
	PiuContentReflow(self, piuSizeChanged);
}

void PiuCode_colorize(xsMachine *the)
{
	PiuCode* self = PIU(Code, xsThis);
	PiuTextBuffer* runs = (*self)->runs;
	PiuCodeRun run = (PiuCodeRun)((uint8_t*)(*runs) + sizeof(PiuTextBufferRecord));
	PiuCodeRun limit = (PiuCodeRun)((uint8_t*)(*runs) + (*runs)->current);
	int32_t start = 0, offset;
	int16_t color;
	xsIntegerValue c = xsToInteger(xsGet(xsArg(0), xsID_length)), i;
	for (i = 0; i < c; i++) {
		xsResult = xsGetAt(xsArg(0), xsInteger(i));
		offset = xsToInteger(xsGet(xsResult, xsID_offset));
		color = (int16_t)xsToInteger(xsGet(xsResult, xsID_color));
		while (run < limit) {
			if ((start == offset) && (run->kind == piuCodeColorKind)) {
				run->color = color;
				break;
			}
			start += run->count;
			run++;
		}	
	}
}

void PiuCode_find(xsMachine *the)
{
	PiuCode* self = PIU(Code, xsThis);
	uint32_t argc = xsToInteger(xsArgc);
	xsStringValue pattern = NULL;
	xsBooleanValue caseless = 0;
	PiuTextBuffer* results = (*self)->results;
	PiuCodeResult result;
	if (argc > 0)
		pattern = xsToString(xsArg(0));
	if (argc > 1)
		caseless = xsToBoolean(xsArg(1));
	if ((*self)->code || (*self)->data) {
		fxDeleteRegExp(NULL, (*self)->code, (*self)->data);
		(*self)->code = NULL;
		(*self)->data = NULL;
	}	
	if (pattern && pattern[0]) {
		xsStringValue modifier = (caseless) ? "imu" : "mu";
		fxCompileRegExp(NULL, pattern, modifier, &(*self)->code, &(*self)->data, NULL, 0);
	}
	PiuCodeSearch(self, (*self)->size);
	result = (PiuCodeResult)((char*)(*results) + sizeof(PiuTextBufferRecord));
	if (result->count >= 0) {
		xsResult = xsTrue;
		while (result->count >= 0) {
			if ((*self)->from <= result->from) {
				PiuCodeSelect(self, result->from, result->to - result->from);
				return;
			}
			result = (PiuCodeResult)(((char*)result) + (*self)->resultsSize);
		}
		result = (PiuCodeResult)((char*)(*results) + sizeof(PiuTextBufferRecord));
		PiuCodeSelect(self, result->from, result->to - result->from);
	}
	else {
		xsResult = xsFalse;
		PiuCodeSelect(self, (*self)->from, 0);
	}
}

void PiuCode_findAgain(xsMachine *the)
{
	PiuCode* self = PIU(Code, xsThis);
	PiuTextBuffer* results = (*self)->results;
	uint32_t argc = xsToInteger(xsArgc);
	xsIntegerValue direction = 1;
	int32_t count, index;
	PiuCodeResult result, wrap;
	if (argc > 0)
		direction = xsToInteger(xsArg(0));
	count = (((*results)->current - sizeof(PiuTextBufferRecord)) / (*self)->resultsSize) - 1;
	if (count) {
		xsResult = xsTrue;
		if (direction > 0) {
			result = wrap = (PiuCodeResult)((char*)(*results) + sizeof(PiuTextBufferRecord));
			for (index = 0; index < count; index++) {
				if ((*self)->to <= result->from) {
					PiuCodeSelect(self, result->from, result->to - result->from);
					return;
				}
				result = (PiuCodeResult)(((char*)result) + (*self)->resultsSize);
			}
		}
		else {
			result = wrap = (PiuCodeResult)((char*)(*results) + (*results)->current - (*self)->resultsSize - (*self)->resultsSize);
			for (index = count - 1; index >= 0; index--) {
				if ((*self)->from >= result->to) {
					PiuCodeSelect(self, result->from, result->to - result->from);
					return;
				}
				result = (PiuCodeResult)(((char*)result) - (*self)->resultsSize);
			}
		}
		PiuCodeSelect(self, wrap->from, wrap->to - wrap->from);
	}
	else
		xsResult = xsFalse;
}

void PiuCode_findBlock(xsMachine *the)
{
	PiuCode* self = PIU(Code, xsThis);
	PiuCodeIteratorRecord iteratorRecord;
	PiuCodeIterator iter = &iteratorRecord;
	int32_t offset = xsToInteger(xsArg(0));
	int32_t from = -1, to = -1;
	char c = *(PiuToString((*self)->string) + offset), d = 0;
	if ((c == '"') || (c == '\'')) {
		PiuCodeFindColor(self, offset, &from, &to);
		goto bail;
	}
	if (c == '(') d = ')';
	else if (c == '[') d = ']';
	else if (c == '{') d = '}';
	if (d) {
		PiuCodeIteratorFirst(self, iter, offset);
		if (iter->color == 0) {
			to = PiuCodeFindBlockForwards(self, iter, d);
			if (to >= 0) {
				from = offset;
				to++;
			}
		}
		goto bail;
	}
	if (c == ')') d = '(';
	else if (c == ']') d = '[';
	else if (c == '}') d = '{';
	if (d) {
		PiuCodeIteratorLast(self, iter, offset);
		if (iter->color == 0) {
			from = PiuCodeFindBlockBackwards(self, iter, d);
			if (from >= 0)
				to = offset + 1;
		}
		goto bail;
	}
bail:
	if (from < to) {
		xsResult = xsNewObject();
		xsDefine(xsResult, xsID_from, xsInteger(from), xsDefault);
		xsDefine(xsResult, xsID_to, xsInteger(to), xsDefault);
	}
}

void PiuCode_findLineBreak(xsMachine *the)
{
	PiuCode* self = PIU(Code, xsThis);
	uint32_t argc = xsToInteger(xsArgc);
	int32_t result = xsToInteger(xsArg(0));
	int32_t direction = (argc > 1) ? (xsToBoolean(xsArg(1)) ? +1 : -1) : + 1;
	if ((direction < 0) && (result == 0))
		result = 0;
	else if ((direction > 0) && (result >= (int32_t)(*self)->length))
		result = (*self)->length;
	else {
		char *s = PiuToString((*self)->string);
		char *p = s + fxUnicodeToUTF8Offset(s, result);
		char c;
		if (direction > 0) {
			result = (*self)->length;
			while ((c = *p)) {
				if ((c == 10) || (c == 13)) {
					result = fxUTF8ToUnicodeOffset(s, p + 1 - s);
					break;
				}
				p++;
			}
		}
		else {
			p--;
			result = 0;
			while (p >= s) {
				c = *p;
				if ((c == 10) || (c == 13)) {
					result = fxUTF8ToUnicodeOffset(s, p + 1 - s);
					break;
				}
				p--;
			}
		}
	}
	xsResult = xsInteger(result);
}

void PiuCode_findWordBreak(xsMachine *the)
{
	PiuCode* self = PIU(Code, xsThis);
	uint32_t argc = xsToInteger(xsArgc);
	int32_t offset = xsToInteger(xsArg(0));
	int32_t direction = (argc > 1) ? (xsToBoolean(xsArg(1)) ? +1 : -1) : +1;
	xsResult = xsInteger(PiuCodeFindWordBreak(self, offset, direction));
}

void PiuCode_hitOffset(xsMachine *the)
{
	PiuCode* self = PIU(Code, xsThis);
	if ((*self)->application) {
		PiuCoordinate x = xsToPiuCoordinate(xsArg(0));
		PiuCoordinate y = xsToPiuCoordinate(xsArg(1));
		xsResult = xsInteger(PiuCodeHitOffset(self, x, y));
	}
}

void PiuCode_isSpace(xsMachine *the)
{
	PiuCode* self = PIU(Code, xsThis);
	xsStringValue string = PiuToString((*self)->string);
	xsIntegerValue character;
	int32_t offset = xsToInteger(xsArg(0));
	uint32_t classification;
	string += fxUnicodeToUTF8Offset(string, offset);
	fxUTF8Decode(string, &character);
	classification = PiuCodeClassifyCharacter(character);
	xsResult = xsBoolean(classification == teCharWhiteSpace);
}

void PiuCode_locate(xsMachine *the)
{
	PiuCode* self = PIU(Code, xsThis);
	if ((*self)->application) {
		xsStringValue string = PiuToString((*self)->string);
		PiuStyle* style = (*self)->computedStyle;
		int32_t offset = fxUnicodeToUTF8Offset(string, xsToInteger(xsArg(0)));
		int32_t column, line;
		PiuCodeOffsetToColumnLine(self, offset, &column, &line);
		xsResult = xsNewObject();
		xsDefine(xsResult, xsID_x, xsPiuCoordinate((xsIntegerValue)c_floor((*style)->margins.left + (column * (*self)->columnWidth))), xsDefault);
		xsDefine(xsResult, xsID_y, xsPiuCoordinate((xsIntegerValue)c_floor((*style)->margins.top + (line * (*self)->lineHeight))), xsDefault);
		xsDefine(xsResult, xsID_width, xsPiuDimension((xsIntegerValue)c_ceil((*self)->columnWidth)), xsDefault);
		xsDefine(xsResult, xsID_height, xsPiuDimension((xsIntegerValue)c_ceil((*self)->lineHeight)), xsDefault);
	}
}

void PiuCode_select(xsMachine *the)
{
	PiuCode* self = PIU(Code, xsThis);
	xsIntegerValue offset = xsToInteger(xsArg(0));
	xsIntegerValue length = xsToInteger(xsArg(1));
	PiuCodeSelect(self, offset, length);
}



