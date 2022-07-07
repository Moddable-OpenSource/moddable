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

typedef int16_t PiuTextKind;

typedef struct PiuTextLineStruct PiuTextLineRecord, *PiuTextLine;

typedef struct PiuTextNodeBeginStruct PiuTextNodeBeginRecord, *PiuTextNodeBegin;
typedef struct PiuTextNodeEndStruct PiuTextNodeEndRecord, *PiuTextNodeEnd;
typedef struct PiuTextNodeStringStruct PiuTextNodeStringRecord, *PiuTextNodeString;

enum {
	piuTextWord = 0,
	piuTextSpace = 1,
	piuTextReturn = 2,
	piuTextEnd = 4,
	piuTextWordBreak = 8,

	piuTextStyled = 1 << 24,

};

typedef struct {
	PiuStyle* blockStyle;
	PiuStyle* spanStyle;
	PiuCoordinate blockWidth;
	PiuTextOffset lineOffset;
	PiuCoordinate lineWidth;
	PiuCoordinate lineAscent;
	PiuCoordinate lineHeight;
	PiuTextOffset wordOffset;
	PiuCoordinate wordWidth;
	PiuCoordinate wordAscent;
	PiuCoordinate wordHeight;
	PiuCoordinate spaceCount;
	PiuCoordinate spaceSum;
	PiuCoordinate spaceWidth;
	PiuCoordinate y;
} PiuTextFormatContextRecord, *PiuTextFormatContext;

struct PiuTextLineStruct {
	PiuTextOffset textOffset;
	PiuCoordinate x;
	xsIntegerValue portion;
	xsIntegerValue slop;
	PiuCoordinate top;
	PiuCoordinate base;
	PiuCoordinate bottom;
};

enum {
	piuTextNodeBeginBlockKind,
	piuTextNodeBeginSpanKind,
	piuTextNodeEndBlockKind,
	piuTextNodeEndSpanKind,
	piuTextNodeStringKind,
};

struct PiuTextNodeBeginStruct {
	PiuTextKind kind;
	PiuTextOffset parentOffset;
	PiuStyle* style;
	PiuStyle* computedStyle;
	PiuTextLink* link;
};

struct PiuTextNodeEndStruct {
	PiuTextKind kind;
	PiuTextOffset parentOffset;
};

struct PiuTextNodeStringStruct {
	PiuTextKind kind;
	PiuTextOffset fromOffset;
	xsSlot* string;
	PiuTextOffset toOffset;
	PiuTextOffset pad;
};

static void PiuTextLinkDelete(void* it);
static void PiuTextLinkDictionary(xsMachine* the, void* it);
static void PiuTextLinkMark(xsMachine* the, void* it, xsMarkRoot markRoot);

static PiuTextKind PiuTextAdvance(xsMachine* the, xsSlot* string, PiuTextOffset offset, PiuTextOffset limit, PiuTextOffset *nextOffset);
static void PiuTextBegin(PiuText* self);
static void PiuTextBeginNode(PiuText* self, PiuTextKind kind, PiuStyle* style, PiuTextLink* link);
static void PiuTextBind(void* it, PiuApplication* application, PiuView* view);
static void PiuTextBuild(PiuText* self, xsIntegerValue depth, PiuTextKind delta);
static void PiuTextCascade(void* it);
static void PiuTextComputeStyles(PiuText* self, PiuApplication* application, PiuView* view);
static void PiuTextConcat(PiuText* self, xsSlot* string);
static void PiuTextDictionary(xsMachine* the, void* it);
static void PiuTextDraw(void* it, PiuView* view, PiuRectangle area);
static void PiuTextEnd(PiuText* self);
static void PiuTextEndNode(PiuText* self, PiuTextKind kind);
static void PiuTextFitHorizontally(void* it);
static void PiuTextFormat(PiuText* self);
static void PiuTextFormatBlock(PiuText* self, PiuTextFormatContext ctx, PiuTextKind kind, PiuTextOffset length, PiuTextOffset textOffset);
static void PiuTextFormatLine(PiuText* self, PiuTextFormatContext ctx, PiuTextKind kind, PiuTextOffset length);
static void* PiuTextHit(void* it, PiuCoordinate x, PiuCoordinate y);
static void PiuTextMark(xsMachine* the, void* it, xsMarkRoot markRoot);
static void PiuTextMeasureVertically(void* it);
static void PiuTextUnbind(void* it, PiuApplication* application, PiuView* view);
static void PiuTextUncomputeStyles(PiuText* self, PiuApplication* application, PiuView* view);

const PiuDispatchRecord ICACHE_FLASH_ATTR PiuTextLinkDispatchRecord = {
	"Link",
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	PiuContentIdle,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

const xsHostHooks ICACHE_FLASH_ATTR PiuTextLinkHooks = {
	PiuTextLinkDelete,
	PiuTextLinkMark,
	NULL
};

const PiuDispatchRecord ICACHE_FLASH_ATTR PiuTextDispatchRecord = {
	"Text",
	PiuTextBind,
	PiuTextCascade,
	PiuTextDraw,
	PiuTextFitHorizontally,
	PiuContentFitVertically,
	PiuTextHit,
	PiuContentIdle,
	PiuContentInvalidate,
	PiuContentMeasureHorizontally,
	PiuTextMeasureVertically,
	PiuContentPlace,
	NULL,
	NULL,
	PiuContentReflow,
	PiuContentShowing,
	PiuContentShown,
	PiuContentSync,
	PiuTextUnbind,
	PiuContentUpdate
};

const xsHostHooks ICACHE_FLASH_ATTR PiuTextHooks = {
	PiuContentDelete,
	PiuTextMark,
	NULL
};

#define piuTextDepth 8

#define KIND(_OFFSET) *((PiuTextKind*)((uint8_t*)(*nodeBuffer) + _OFFSET))
#define LINE(_OFFSET) ((PiuTextLine)((uint8_t*)(*lineBuffer) + _OFFSET))
#define NODE(_OFFSET) ((void*)((uint8_t*)(*nodeBuffer) + _OFFSET))

void PiuTextBufferAppend(xsMachine *the, PiuTextBuffer* buffer, void* data, size_t size)
{
	size_t available = (*buffer)->available;
	size_t former = (*buffer)->current;
	size_t current = former + size;
	if (current > available) {
		void* chunk;
		available += size;
		chunk = fxRenewChunk(the, *buffer, available);
		if (!chunk) {
			chunk = fxNewChunk(the, available);
			c_memcpy(chunk, *buffer, former);
			*buffer = chunk;
		}
		(*buffer)->available = available;
	}
	c_memcpy(((uint8_t*)(*buffer)) + former, data, size);
	(*buffer)->current = current;
}

void PiuTextBufferClear(xsMachine* the, PiuTextBuffer* buffer)
{
	size_t available = (*buffer)->available;
	size_t current = (*buffer)->current = sizeof(PiuTextBufferRecord);
	c_memset(((uint8_t*)(*buffer)) + current, 0, available - current);
}

void PiuTextBufferGrow(xsMachine *the, PiuTextBuffer* buffer, size_t size)
{
	size_t available = (*buffer)->available;
	size_t former = (*buffer)->current;
	size_t current = former + size;
	if (current > available) {
		void* chunk;
		available += size;
		chunk = fxRenewChunk(the, *buffer, available);
		if (!chunk) {
			chunk = fxNewChunk(the, available);
			c_memcpy(chunk, *buffer, former);
			*buffer = chunk;
		}
		c_memset(((uint8_t*)(*buffer)) + former, 0, available - former);
		(*buffer)->available = available;
	}
	(*buffer)->current = current;
}

void PiuTextBufferNew(xsMachine* the, size_t available)
{
	PiuTextBuffer* buffer;
	xsResult = xsNewHostObject(NULL);
	xsSetHostChunk(xsResult, NULL, sizeof(PiuTextBufferRecord) + available);
	buffer = PIU(TextBuffer, xsResult);
	(*buffer)->reference = xsToReference(xsResult);
	(*buffer)->available = sizeof(PiuTextBufferRecord) + available;
	(*buffer)->current = sizeof(PiuTextBufferRecord);
}

void PiuTextLinkDelete(void* it)
{
}

void PiuTextLinkDictionary(xsMachine* the, void* it) 
{
	PiuTextLink* self = it;
	if (xsFindResult(xsArg(1), xsID_anchor)) {
		if (xsTest(xsArg(0))) {
			xsSetAt(xsArg(0), xsResult, xsThis);
		}
	}
	if (xsFindResult(xsArg(1), xsID_Behavior)) {
		if (xsIsInstanceOf(xsResult, xsFunctionPrototype)) {
			fxPush(xsResult);
			fxNew(the);
			fxRunCount(the, 0);
			xsResult = fxPop();
			(*self)->behavior = xsToReference(xsResult);
		}
	}
	else if (xsFindResult(xsArg(1), xsID_behavior)) {
		if (xsIsInstanceOf(xsResult, xsObjectPrototype)) {
			(*self)->behavior = xsToReference(xsResult);
		}
	}
}

void PiuTextLinkMark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	PiuTextLink self = it;
	PiuMarkReference(the, self->behavior);
}

void PiuTextLink_create(xsMachine* the)
{
	PiuTextLink* self;
	xsVars(4);
	xsSetHostChunk(xsThis, NULL, sizeof(PiuTextLinkRecord));
	self = PIU(TextLink, xsThis);
	(*self)->the = the;
	(*self)->reference = xsToReference(xsThis);
	xsSetHostHooks(xsThis, (xsHostHooks*)&PiuTextLinkHooks);
	(*self)->dispatch = (PiuDispatch)&PiuTextLinkDispatchRecord;
	(*self)->flags = piuVisible | piuActive | piuExclusiveTouch;
	PiuTextLinkDictionary(the, self);
	PiuBehaviorOnCreate(self);
}

void PiuTextLink_get_state(xsMachine* the)
{
	PiuTextLink* self = PIU(TextLink, xsThis);
	xsResult = xsNumber((*self)->state);
}

void PiuTextLink_set_state(xsMachine* the)
{
	PiuTextLink* self = PIU(TextLink, xsThis);
	double state = xsToNumber(xsArg(0));
	if ((*self)->state != state) {
		(*self)->state = state;
		PiuContentInvalidate((*self)->container, NULL);
	}
}

PiuTextKind PiuTextAdvance(xsMachine* the, xsSlot* string, PiuTextOffset offset, PiuTextOffset limit, PiuTextOffset *nextOffset)
{
	PiuTextOffset length = 0;
	PiuTextKind kind = piuTextWord;
	if (offset == limit)
		kind = piuTextEnd;
	else {
		xsIntegerValue c;
		xsStringValue p = xsToString(*string) + offset;
		xsStringValue q = fxUTF8Decode(p, &c);
		length = q - p;
		if ((c == C_EOF) || (c == 0))
			kind = piuTextEnd;
		if (length == 1) {
			if (c == '\n')
				kind = piuTextReturn;
			else if (c == '\t')
				kind = piuTextSpace;
			else if (c == ' ')
				kind = piuTextSpace;
		} 
		else {
			if (c == 0xA0)
				kind = piuTextSpace;
			else if ((0x4E00 <= c) && (c <= 0x9FFF))
				kind = piuTextWordBreak;
		}
	}
	*nextOffset = offset + length;
	return kind;
}

void PiuTextBegin(PiuText* self)
{
	xsMachine *the = (*self)->the;
	PiuTextBufferClear(the, (*self)->nodeBuffer);
	(*self)->nodeLink = NULL;
	(*self)->nodeOffset = -1;
	(*self)->textOffset = 0;
}

void PiuTextBeginNode(PiuText* self, PiuTextKind kind, PiuStyle* style, PiuTextLink* link)
{
	xsMachine *the = (*self)->the;
	PiuTextBuffer* nodeBuffer = (*self)->nodeBuffer;
	PiuTextOffset nodeOffset = (PiuTextOffset)(*nodeBuffer)->current;
	PiuTextNodeBegin nodeBegin;
	PiuTextBufferGrow(the, nodeBuffer, sizeof(PiuTextNodeBeginRecord));
	nodeBegin = NODE(nodeOffset);
	nodeBegin->kind = kind;
	nodeBegin->parentOffset = (*self)->nodeOffset;
	nodeBegin->style = style;
	if (link) {
		(*self)->nodeLink = link;
		(*link)->container = (PiuContainer*)self;
	}
	nodeBegin->link = (*self)->nodeLink;
	(*self)->nodeOffset = nodeOffset;
}

void PiuTextBind(void* it, PiuApplication* application, PiuView* view)
{
	PiuText* self = it;
	PiuContentBind(it, application, view);
	PiuTextComputeStyles(self, application, view);
}

void PiuTextBuild(PiuText* self, xsIntegerValue depth, PiuTextKind delta)
{
	xsMachine *the = (*self)->the;
	PiuStyle* style;
	PiuTextLink* link;
	if (xsTest(xsVar(depth))) {
		if (xsIsInstanceOf(xsVar(depth), xsArrayPrototype)) {
			xsIntegerValue i, c = xsToInteger(xsGet(xsVar(depth), xsID_length));
			for (i = 0; i < c; i++) {
				xsResult = xsGetAt(xsVar(depth) , xsInteger(i));
				if (xsIsInstanceOf(xsResult, xsObjectPrototype)) {
					depth++;
					if (depth == piuTextDepth)
						xsUnknownError("too deep");
					xsVar(depth) = xsGet(xsResult, xsID_style);
					if (xsTest(xsVar(depth)))
						style = PIU(Style, xsVar(depth));
					else
						style = NULL;
					xsVar(depth) = xsGet(xsResult, xsID_link);
					if (xsTest(xsVar(depth)))
						link = PIU(TextLink, xsVar(depth));
					else
						link = NULL;
					PiuTextBeginNode(self, piuTextNodeBeginBlockKind + delta, style, link);
					xsVar(depth) = xsGet(xsResult, xsID_spans);
					PiuTextBuild(self, depth, 1);
					PiuTextEndNode(self, piuTextNodeEndBlockKind + delta);
					depth--;
				}
				else {
					if (!delta)
						PiuTextBeginNode(self, piuTextNodeBeginBlockKind, NULL, NULL);
					PiuTextConcat(self, PiuString(xsResult));
					if (!delta)
						PiuTextEndNode(self, piuTextNodeEndBlockKind);
				}
			}
		}
		else {
			if (!delta)
				PiuTextBeginNode(self, piuTextNodeBeginBlockKind, NULL, NULL);
			PiuTextConcat(self, PiuString(xsVar(depth)));
			if (!delta)
				PiuTextEndNode(self, piuTextNodeEndBlockKind);
		}
	}
}

void PiuTextCascade(void* it)
{
	PiuText* self = it;
	PiuApplication* application = (*self)->application;
	PiuTextUncomputeStyles(self, application, (*application)->view);
	PiuContentCascade(it);
	PiuTextComputeStyles(self, application, (*application)->view);
	PiuContentInvalidate(self, NULL);
	PiuContentReflow(self, piuSizeChanged);
}

void PiuTextComputeStyles(PiuText* self, PiuApplication* application, PiuView* view)
{
	xsMachine *the = (*self)->the;
	PiuTextBuffer* nodeBuffer = (*self)->nodeBuffer;
	PiuTextOffset nodeOffset = sizeof(PiuTextBufferRecord);
	PiuTextOffset nodeLimit = (PiuTextOffset)(*nodeBuffer)->current;
	PiuStyle* style;
	(*self)->flags |= piuTextStyled;
	while (nodeOffset < nodeLimit) {
		switch (KIND(nodeOffset)) {
		case piuTextNodeBeginBlockKind:
		case piuTextNodeBeginSpanKind: {
			PiuStyleLink* list = (*application)->styleList;
			PiuStyleLink* chain = NULL;
			PiuTextNodeBegin node = NODE(nodeOffset);
			while (node) {
				PiuTextOffset parentOffset = node->parentOffset;
				style = node->style;
				if (style) {
					list = PiuStyleLinkMatch(the, list, chain, style);
					chain = list;
				}
				if (parentOffset >= 0)
					node = NODE(parentOffset);
				else {
					PiuContainer* container = (PiuContainer*)self;
					while (container) {
						style = (*container)->style;
						if (style) {
							list = PiuStyleLinkMatch(the, list, chain, style);
							chain = list;
						}
						container = (*container)->container;
					}
					break;
				}
			}
			if (chain) {
				style = PiuStyleLinkCompute(the, chain, application);
				node = NODE(nodeOffset);
				node->computedStyle = style;
			#ifdef piuGPU
				PiuStyleBind(node->computedStyle, application, view);
			#endif
			}
			else {
				(*self)->flags &= ~piuTextStyled;
			}
			nodeOffset += sizeof(PiuTextNodeBeginRecord);
			} break;
		case piuTextNodeEndBlockKind:
		case piuTextNodeEndSpanKind:
			nodeOffset += sizeof(PiuTextNodeEndRecord);
			break;
		case piuTextNodeStringKind:
			nodeOffset += sizeof(PiuTextNodeStringRecord);
			break;
		}
	}
}

void PiuTextConcat(PiuText* self, xsSlot* slot)
{
	xsMachine *the = (*self)->the;
	PiuTextBuffer* nodeBuffer = (*self)->nodeBuffer;
	PiuTextOffset nodeOffset = (PiuTextOffset)(*nodeBuffer)->current;
	PiuTextNodeString node;
	xsStringValue string = xsToString(*slot);
	xsIntegerValue length = c_strlen(string);
	PiuTextBufferGrow(the, nodeBuffer, sizeof(PiuTextNodeStringRecord));
	node = NODE(nodeOffset);
	node->kind = piuTextNodeStringKind;
	node->fromOffset = (*self)->textOffset;
	node->string = slot;
	node->toOffset = node->fromOffset + (PiuTextOffset)length;
	(*self)->textOffset = node->toOffset;
}

void PiuTextDictionary(xsMachine* the, void* it) 
{
	PiuText* self = it;
	if (xsFindResult(xsArg(1), xsID_blocks)) {
		xsVar(0) = xsResult;
		PiuTextBegin(self);
		PiuTextBuild(self, 0, 0);
		PiuTextEnd(self);
	}
	else if (xsFindResult(xsArg(1), xsID_string)) {
		PiuTextBegin(self);
		PiuTextBeginNode(self, piuTextNodeBeginBlockKind, NULL, NULL);
		PiuTextConcat(self, PiuString(xsResult));
		PiuTextEndNode(self, piuTextNodeEndBlockKind);
		PiuTextEnd(self);
	}
	else if (xsFindResult(xsArg(1), xsID_contents)) {
		xsVar(0) = xsResult;
		PiuTextBegin(self);
		PiuTextBuild(self, 0, 0);
		PiuTextEnd(self);
	}
}

void PiuTextDraw(void* it, PiuView* view, PiuRectangle area)
{
	PiuText* self = it;
	xsMachine *the = (*self)->the;
	PiuTextBuffer* lineBuffer = (*self)->lineBuffer;
	PiuTextOffset lineOffset = sizeof(PiuTextBufferRecord);
	PiuTextOffset lineLimit = (PiuTextOffset)(*lineBuffer)->current;
	PiuTextBuffer* nodeBuffer = (*self)->nodeBuffer;
	PiuTextOffset nodeOffset = sizeof(PiuTextBufferRecord);
	PiuTextOffset nodeLimit = (PiuTextOffset)(*nodeBuffer)->current;
	PiuStyle* style;
	PiuTextOffset fromOffset, toOffset;
	PiuCoordinate x, base, y;
	xsIntegerValue portion;
	xsIntegerValue slop;
	xsIntegerValue step;
	PiuCoordinate clipTop = area->y;
	PiuCoordinate clipBottom = clipTop + area->height;
	PiuTextLink* link = NULL;
	PiuTextLine previousLine, line = NULL, nextLine = LINE(lineOffset);
	xsIntegerValue top, bottom;
	
	PiuContentDraw(it, view, area);
	while (lineOffset < lineLimit) {
		previousLine = line;
		line = nextLine;
		lineOffset += sizeof(PiuTextLineRecord);
		nextLine = (lineOffset < lineLimit) ? LINE(lineOffset) : NULL;
		
		top = (previousLine) ? previousLine->top : line->top;
		bottom = (nextLine) ? nextLine->bottom : line->bottom;
		
		if (top >= clipBottom)
			break;
			
		if (bottom <= clipTop)
			continue;
		
		fromOffset = line->textOffset;
		toOffset = (nextLine) ? nextLine->textOffset : (*self)->textOffset;
		
		base = line->base;
		
		x = line->x;
		portion = line->portion;
		if (portion) {
			slop = 0;
			step = line->slop << 16;
			step /= portion;
		}
		else
			toOffset += line->slop;
		
		while ((fromOffset < toOffset) && (nodeOffset < nodeLimit)) {
			switch (KIND(nodeOffset)) {
			case piuTextNodeBeginBlockKind:
			case piuTextNodeBeginSpanKind: {
				PiuTextNodeBegin nodeBegin = NODE(nodeOffset);
				style = nodeBegin->computedStyle;
				link = nodeBegin->link;
				nodeOffset += sizeof(PiuTextNodeBeginRecord);
				} break;
			case piuTextNodeEndBlockKind: {
				style = NULL;
				link = NULL;
				nodeOffset += sizeof(PiuTextNodeEndRecord);
				} break;
			case piuTextNodeEndSpanKind: {
				PiuTextNodeEnd nodeEnd = NODE(nodeOffset);
				PiuTextNodeBegin nodeBegin = NODE(nodeEnd->parentOffset);
				style = nodeBegin->computedStyle;
				link = nodeBegin->link;
				nodeOffset += sizeof(PiuTextNodeEndRecord);
				} break;
			case piuTextNodeStringKind:  {
				PiuTextNodeString node = NODE(nodeOffset);
				if ((fromOffset <= node->toOffset) && (node->fromOffset < toOffset)) {
					PiuTextOffset fromStringOffset = (fromOffset >= node->fromOffset) ? fromOffset : node->fromOffset;
					PiuTextOffset toStringOffset = (toOffset <= node->toOffset) ? toOffset : node->toOffset;
					PiuTextOffset offset = fromStringOffset - node->fromOffset;
					PiuTextOffset limit = toStringOffset - node->fromOffset;
					PiuFont* font = (*style)->font;
					PiuColorRecord color;
					PiuColorsBlend((*style)->color, link ? (*link)->state : (*self)->state, &color);
					PiuViewPushColor(view, &color);
					y = base - PiuFontGetAscent(font);
					if (portion) {
						PiuTextOffset previousOffset = offset, nextOffset;
						for (;;) {
							PiuTextKind kind = PiuTextAdvance(the, node->string, offset, limit, &nextOffset);
							if (kind) {
								PiuTextOffset length = offset - previousOffset;
								if (length) {
									PiuViewDrawString(view, node->string, previousOffset, length, font, x, y, 0, 0);
									x += PiuFontGetWidth(font, node->string, previousOffset, length);
									previousOffset = offset;
								}
								if (kind == piuTextEnd) break;
								if (kind == piuTextSpace) {
									slop += step;
									x += (slop & 0xFFFF0000) >> 16;
									slop = slop & 0x0000FFFF;
									previousOffset = nextOffset;
								}
							}
							offset = nextOffset;
						}
					}
					else {
						PiuTextOffset length = limit - offset;
						if (length) {
							PiuViewDrawString(view, node->string, offset, length, font, x, y, 0, 0);
							x += PiuFontGetWidth(font, node->string, offset, length);
						}				
					}				
					fromOffset = toStringOffset;
					PiuViewPopColor(view);
				}
				if (fromOffset < toOffset)
					nodeOffset += sizeof(PiuTextNodeStringRecord);
				} break;
			}
		}
	}
}

void PiuTextEnd(PiuText* self)
{
	PiuApplication* application = (*self)->application;
	if (application)
		PiuTextComputeStyles(self, application, (*application)->view);
	PiuContentReflow(self, piuSizeChanged);
}

void PiuTextEndNode(PiuText* self, PiuTextKind kind)
{
	xsMachine *the = (*self)->the;
	PiuTextBuffer* nodeBuffer = (*self)->nodeBuffer;
	PiuTextOffset nodeOffset = (PiuTextOffset)(*nodeBuffer)->current;
	PiuTextNodeBegin nodeBegin;
	PiuTextNodeEnd nodeEnd;
	PiuTextBufferGrow(the, nodeBuffer, sizeof(PiuTextNodeEndRecord));
	nodeBegin = NODE((*self)->nodeOffset);
	(*self)->nodeOffset = nodeBegin->parentOffset;
	(*self)->nodeLink = nodeBegin->link;
	nodeEnd = NODE(nodeOffset);
	nodeEnd->kind = kind;
	nodeEnd->parentOffset = (kind == piuTextNodeEndSpanKind) ? (*self)->nodeOffset : (*self)->textOffset;
}

void PiuTextFitHorizontally(void* it) 
{
	PiuText* self = it;
	if (((*self)->coordinates.horizontal & piuLeftRight) != piuLeftRight)
		(*self)->bounds.width = (*self)->coordinates.width;
	(*self)->textWidth = (*self)->bounds.width;
	(*self)->textHeight = 0;
	if (((*self)->textWidth) && ((*self)->flags & piuTextStyled))
		PiuTextFormat(self);
	(*self)->flags &= ~(piuWidthChanged | piuContentsHorizontallyChanged);
	if (!((*self)->coordinates.vertical & piuHeight)) {
		PiuContentReflow(self, piuHeightChanged);
	}
}

void PiuTextFormat(PiuText* self)
{
	xsMachine *the = (*self)->the;
	PiuTextBuffer* nodeBuffer = (*self)->nodeBuffer;
	PiuTextOffset nodeOffset = sizeof(PiuTextBufferRecord);
	PiuTextOffset nodeLimit = (PiuTextOffset)(*nodeBuffer)->current;
	PiuTextFormatContextRecord _ctx;
	PiuTextFormatContext ctx = &_ctx;
	PiuCoordinate marginTop;
	PiuCoordinate marginBottom = 0;
	c_memset(ctx, 0, sizeof(PiuTextFormatContextRecord));
	PiuTextBufferClear(the, (*self)->lineBuffer);
	while (nodeOffset < nodeLimit) {
		switch (KIND(nodeOffset)) {
		case piuTextNodeBeginBlockKind: {
			PiuTextNodeBegin nodeBegin = NODE(nodeOffset);
			ctx->blockStyle = ctx->spanStyle = nodeBegin->computedStyle;
			ctx->blockWidth = (*self)->textWidth - (*ctx->blockStyle)->margins.left - (*ctx->blockStyle)->margins.right;
			marginTop = (*ctx->blockStyle)->margins.top;
			ctx->y += (marginTop > marginBottom) ? marginTop : marginBottom;
			nodeOffset += sizeof(PiuTextNodeBeginRecord);
			} break;
		case piuTextNodeBeginSpanKind: {
			PiuTextNodeBegin nodeBegin = NODE(nodeOffset);
			ctx->spanStyle = nodeBegin->computedStyle;
			nodeOffset += sizeof(PiuTextNodeBeginRecord);
			} break;
		case piuTextNodeEndBlockKind: {
			PiuTextNodeEnd nodeEnd = NODE(nodeOffset);
			PiuTextFormatBlock(self, ctx, piuTextReturn, 0, nodeEnd->parentOffset);
			marginBottom = (*ctx->blockStyle)->margins.bottom;
			ctx->blockStyle = ctx->spanStyle = NULL;
			nodeOffset += sizeof(PiuTextNodeEndRecord);
		} break;
		case piuTextNodeEndSpanKind: {
			PiuTextNodeEnd nodeEnd = NODE(nodeOffset);
			PiuTextNodeBegin nodeBegin = NODE(nodeEnd->parentOffset);
			ctx->spanStyle = nodeBegin->computedStyle;
			nodeOffset += sizeof(PiuTextNodeEndRecord);
		} break;
		case piuTextNodeStringKind: {
			PiuTextNodeString node = NODE(nodeOffset);
			PiuTextOffset previousOffset = 0, offset = 0, nextOffset, textOffset = node->fromOffset;
			PiuFont* spanFont = (*ctx->spanStyle)->font;
			PiuCoordinate spanAscent = PiuFontGetAscent(spanFont);
			PiuCoordinate spanHeight = PiuFontGetHeight(spanFont);
			for (;;) {
				PiuTextKind kind = PiuTextAdvance(the, node->string, offset, 0x7fff, &nextOffset);
				if (kind) {
					PiuTextOffset length = offset - previousOffset;
					if (length) {
						ctx->wordWidth += PiuFontGetWidth(spanFont, node->string, previousOffset, length);
						if (ctx->wordAscent < spanAscent)
							ctx->wordAscent = spanAscent;
						if (ctx->wordHeight < spanHeight)
							ctx->wordHeight = spanHeight;
						previousOffset = offset;
					}
					length = nextOffset - previousOffset;
					if (kind == piuTextSpace) {
						PiuTextFormatBlock(self, ctx, kind, length, textOffset + nextOffset);
						node = NODE(nodeOffset);
						if (length) {
							ctx->spaceWidth = PiuFontGetWidth(spanFont, node->string, previousOffset, length);
							previousOffset = nextOffset;
						}
					}
					else if (kind == piuTextReturn) {
						PiuTextFormatBlock(self, ctx, kind, length, textOffset + nextOffset);
						node = NODE(nodeOffset);
						if (length) {
							previousOffset = nextOffset;
						}
					}
					else if (kind == piuTextWordBreak) {
						PiuTextFormatBlock(self, ctx, kind, 0, textOffset + previousOffset);
						node = NODE(nodeOffset);
						if (length) {
							ctx->wordWidth = PiuFontGetWidth(spanFont, node->string, previousOffset, length);
							if (ctx->wordAscent < spanAscent)
								ctx->wordAscent = spanAscent;
							if (ctx->wordHeight < spanHeight)
								ctx->wordHeight = spanHeight;
							previousOffset = nextOffset;
						}
					}
					else
						break;
				}
				offset = nextOffset;
			}
			nodeOffset += sizeof(PiuTextNodeStringRecord);
			} break;
		}
	}
	ctx->y += marginBottom;
	(*self)->textHeight = ctx->y;	
}

void PiuTextFormatBlock(PiuText* self, PiuTextFormatContext ctx, PiuTextKind kind, PiuTextOffset length, PiuTextOffset textOffset)
{
	if (ctx->lineWidth + ctx->spaceWidth + ctx->wordWidth > ctx->blockWidth) {
		if (ctx->lineWidth > 0)				
			PiuTextFormatLine(self, ctx, piuTextWord, 0);
		ctx->spaceCount = 0;
		ctx->spaceSum = 0;
		ctx->lineOffset = ctx->wordOffset;
		ctx->lineWidth = ctx->wordWidth;
		ctx->lineAscent = ctx->wordAscent;
		ctx->lineHeight = ctx->wordHeight;
	}
	else {
		if (ctx->lineWidth > 0) {
			ctx->spaceCount++;
			ctx->spaceSum += ctx->spaceWidth;
		}	
		ctx->lineWidth += ctx->spaceWidth + ctx->wordWidth;
		if (ctx->lineAscent < ctx->wordAscent)
			ctx->lineAscent = ctx->wordAscent;
		if (ctx->lineHeight < ctx->wordHeight)
			ctx->lineHeight = ctx->wordHeight;
	}
	if ((kind == piuTextReturn) || (ctx->lineWidth > ctx->blockWidth)) { // @@ no cesure
		ctx->spaceCount = 0;
		ctx->spaceSum = 0;
		PiuTextFormatLine(self, ctx, kind, length);
		ctx->lineOffset = textOffset;
		ctx->lineWidth = 0;
		ctx->lineAscent = 0;
		ctx->lineHeight = 0;
	}
	ctx->spaceWidth = 0;
	ctx->wordOffset = textOffset;
	ctx->wordWidth = 0;
	ctx->wordAscent = 0;
	ctx->wordHeight = 0;
}

void PiuTextFormatLine(PiuText* self, PiuTextFormatContext ctx, PiuTextKind kind, PiuTextOffset length)
{
	xsMachine *the = (*self)->the;
	PiuTextBuffer* lineBuffer = (*self)->lineBuffer;
	PiuTextOffset lineOffset = (PiuTextOffset)(*lineBuffer)->current;
	PiuTextLine line;
	PiuCoordinate x = (*ctx->blockStyle)->margins.left;
	xsIntegerValue leading = (*ctx->blockStyle)->leading;
	xsIntegerValue ascent, height;
	
	PiuTextBufferGrow(the, lineBuffer, sizeof(PiuTextLineRecord));
	line = LINE(lineOffset);
	line->textOffset = ctx->lineOffset;
	
	switch ((*ctx->blockStyle)->horizontal) {
	case piuCenter:
#ifdef piuPC
		line->x = x + ((ctx->blockWidth - ctx->lineWidth) / 2);
#else
		line->x = x + ((ctx->blockWidth - ctx->lineWidth) >> 1);
#endif
		line->portion = line->slop = 0;
		break;
	case piuLeft:
		line->x = x;
		line->portion = line->slop = 0;
		break;
	case piuRight:
		line->x = x + ctx->blockWidth - ctx->lineWidth;
		line->portion = line->slop = 0;
		break;
	default:
		line->x = x;
		line->portion = ctx->spaceCount;
		line->slop = ctx->blockWidth - (ctx->lineWidth - ctx->spaceSum);
		break;	
	}
	if (kind == piuTextReturn) {
		line->portion = 0;
		line->slop = -length;
	}
	ascent = ctx->lineAscent;
	height = ctx->lineHeight;
	if (leading < 0) {
		leading = -leading;
		ascent = (ascent * leading) / 100;
		height = (height * leading) / 100;
	}
	else if (leading > 0) {
		if (0 < height && height < leading) {
			ascent = (ascent * leading) / height;
			height = leading;
		}
	}
	line->top = ctx->y + (PiuCoordinate)ascent - ctx->lineAscent;
	line->base = ctx->y + (PiuCoordinate)ascent;
	line->bottom = line->top + ctx->lineHeight;
	ctx->y += (PiuCoordinate)height;
}

void* PiuTextHit(void* it, PiuCoordinate x, PiuCoordinate y) 
{
	PiuText* self = it;
	xsMachine *the = (*self)->the;
	PiuTextBuffer* lineBuffer = (*self)->lineBuffer;
	PiuTextOffset lineOffset = sizeof(PiuTextBufferRecord);
	PiuTextOffset lineLimit = (PiuTextOffset)(*lineBuffer)->current;
	PiuTextBuffer* nodeBuffer = (*self)->nodeBuffer;
	PiuTextOffset nodeOffset = sizeof(PiuTextBufferRecord);
	PiuTextOffset nodeLimit = (PiuTextOffset)(*nodeBuffer)->current;
	PiuStyle* style;
	PiuTextOffset fromOffset, toOffset;
	xsIntegerValue portion;
	xsIntegerValue slop;
	xsIntegerValue step;
	PiuCoordinate lineLeft = 0;
	PiuCoordinate lineRight = 0;
	PiuTextLine line = NULL, nextLine = LINE(lineOffset);

	void* link = NULL;
	if (!((*self)->flags & piuActive))
		return NULL;
	if ((x < 0) || (y < 0) || (x >= (*self)->bounds.width) || (y >= (*self)->bounds.height))
		return NULL;
	while (lineOffset < lineLimit) {
		line = nextLine;
		lineOffset += sizeof(PiuTextLineRecord);
		nextLine = (lineOffset < lineLimit) ? LINE(lineOffset) : NULL;
		
		if (y < line->top)
			break;

    if (line->bottom < y)
			continue;
		
		fromOffset = line->textOffset;
		toOffset = (nextLine) ? nextLine->textOffset : (*self)->textOffset;
		
		lineLeft = line->x;
		portion = line->portion;
		if (portion) {
			slop = 0;
			step = line->slop << 16;
			step /= portion;
		}
		
		while ((fromOffset < toOffset) && (nodeOffset < nodeLimit)) {
			switch (KIND(nodeOffset)) {
			case piuTextNodeBeginBlockKind:
			case piuTextNodeBeginSpanKind: {
				PiuTextNodeBegin nodeBegin = NODE(nodeOffset);
				style = nodeBegin->computedStyle;
				link = nodeBegin->link;
				nodeOffset += sizeof(PiuTextNodeBeginRecord);
				} break;
			case piuTextNodeEndBlockKind: {
				style = NULL;
				link = NULL;
				nodeOffset += sizeof(PiuTextNodeEndRecord);
				} break;
			case piuTextNodeEndSpanKind: {
				PiuTextNodeEnd nodeEnd = NODE(nodeOffset);
				PiuTextNodeBegin nodeBegin = NODE(nodeEnd->parentOffset);
				style = nodeBegin->computedStyle;
				link = nodeBegin->link;
				nodeOffset += sizeof(PiuTextNodeEndRecord);
				} break;
			case piuTextNodeStringKind:  {
				PiuTextNodeString node = NODE(nodeOffset);
				if ((fromOffset <= node->toOffset) && (node->fromOffset < toOffset)) {
					PiuTextOffset fromStringOffset = (fromOffset >= node->fromOffset) ? fromOffset : node->fromOffset;
					PiuTextOffset toStringOffset = (toOffset <= node->toOffset) ? toOffset : node->toOffset;
					PiuTextOffset offset = fromStringOffset - node->fromOffset;
					PiuTextOffset limit = toStringOffset - node->fromOffset;
					PiuFont* font = (*style)->font;
					if (portion) {
						PiuTextOffset previousOffset = offset, nextOffset;
						for (;;) {
							PiuTextKind kind = PiuTextAdvance(the, node->string, offset, limit, &nextOffset);
							if (kind) {
								PiuTextOffset length = offset - previousOffset;
								if (length) {
									lineRight = lineLeft + PiuFontGetWidth(font, node->string, previousOffset, length);
									if ((lineLeft <= x) && (x < lineRight))
										return link ? link : self;
									lineLeft = lineRight;
									previousOffset = offset;
								}
								if (kind == piuTextEnd) break;
								if (kind == piuTextSpace) {
									slop += step;
									lineLeft += (slop & 0xFFFF0000) >> 16;
									slop = slop & 0x0000FFFF;
									previousOffset = nextOffset;
								}
							}
							offset = nextOffset;
						}
					}
					else {
						PiuTextOffset length = limit - offset;
						if (length) {
							lineRight = lineLeft + PiuFontGetWidth(font, node->string, offset, length);
							if ((lineLeft <= x) && (x < lineRight))
								return link ? link : self;
							lineLeft = lineRight;
						}				
					}				
					fromOffset = toStringOffset;
				}
				if (fromOffset < toOffset)
					nodeOffset += sizeof(PiuTextNodeStringRecord);
				} break;
			}
		}
	}
	return self;
}

void PiuTextMark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	PiuText self = it;
	PiuTextBuffer* nodeBuffer = self->nodeBuffer;
	
	PiuContentMark(the, it, markRoot);
	PiuMarkHandle(the, self->nodeBuffer);
	PiuMarkHandle(the, self->lineBuffer);
	
	if (nodeBuffer) {
		PiuTextOffset nodeOffset = sizeof(PiuTextBufferRecord);
		PiuTextOffset nodeLimit = (PiuTextOffset)(*nodeBuffer)->current;
		while (nodeOffset < nodeLimit) {
			switch (KIND(nodeOffset)) {
			case piuTextNodeBeginBlockKind:
			case piuTextNodeBeginSpanKind: {
				PiuTextNodeBegin node = NODE(nodeOffset);
				PiuMarkHandle(the, node->style);
				PiuMarkHandle(the, node->computedStyle);
				PiuMarkHandle(the, node->link);
				nodeOffset += sizeof(PiuTextNodeBeginRecord);
				} break;
			case piuTextNodeEndBlockKind:
			case piuTextNodeEndSpanKind:
				nodeOffset += sizeof(PiuTextNodeEndRecord);
				break;
			case piuTextNodeStringKind: {
				PiuTextNodeString node = NODE(nodeOffset);
				PiuMarkString(the, node->string);
				nodeOffset += sizeof(PiuTextNodeStringRecord);
				} break;
			}
		}
	}
}

void PiuTextMeasureVertically(void* it) 
{
	PiuText* self = it;
	if (!((*self)->coordinates.vertical & piuHeight))
		(*self)->coordinates.height = (*self)->textHeight;	
}

void PiuTextUnbind(void* it, PiuApplication* application, PiuView* view)
{
	PiuText* self = it;
	PiuTextUncomputeStyles(self, application, view);
	PiuContentUnbind(it, application, view);
}

void PiuTextUncomputeStyles(PiuText* self, PiuApplication* application, PiuView* view)
{
	PiuTextBuffer* nodeBuffer = (*self)->nodeBuffer;
	PiuTextOffset nodeOffset = sizeof(PiuTextBufferRecord);
	PiuTextOffset nodeLimit = (PiuTextOffset)(*nodeBuffer)->current;
	while (nodeOffset < nodeLimit) {
		switch (KIND(nodeOffset)) {
		case piuTextNodeBeginBlockKind:
		case piuTextNodeBeginSpanKind: {
			PiuTextNodeBegin node = NODE(nodeOffset);
			if (node->computedStyle) {
			#ifdef piuGPU
				PiuStyleUnbind(node->computedStyle, application, view);
			#endif
				node->computedStyle = NULL;
			}
			nodeOffset += sizeof(PiuTextNodeBeginRecord);
			} break;
		case piuTextNodeEndBlockKind:
		case piuTextNodeEndSpanKind:
			nodeOffset += sizeof(PiuTextNodeEndRecord);
			break;
		case piuTextNodeStringKind:
			nodeOffset += sizeof(PiuTextNodeStringRecord);
			break;
		}
	}
}

void PiuText_create(xsMachine* the)
{
	PiuText* self;
	xsVars(piuTextDepth);
	xsSetHostChunk(xsThis, NULL, sizeof(PiuTextRecord));
	self = PIU(Text, xsThis);
	(*self)->the = the;
	(*self)->reference = xsToReference(xsThis);
	xsSetHostHooks(xsThis, (xsHostHooks*)&PiuTextHooks);
	(*self)->dispatch = (PiuDispatch)&PiuTextDispatchRecord;
	(*self)->flags = piuVisible;
	PiuContentDictionary(the, self);
	
	PiuTextBufferNew(the, 512);
	(*self)->lineBuffer = PIU(TextBuffer, xsResult);
	PiuTextBufferNew(the, 512);
	(*self)->nodeBuffer = PIU(TextBuffer, xsResult);
	
	PiuTextDictionary(the, self);
	PiuBehaviorOnCreate(self);
}

void PiuText_get_blocks(xsMachine *the)
{
	PiuText* self = PIU(Text, xsThis);
	PiuTextBuffer* nodeBuffer = (*self)->nodeBuffer;
	PiuTextOffset nodeOffset = sizeof(PiuTextBufferRecord);
	PiuTextOffset nodeLimit = (PiuTextOffset)(*nodeBuffer)->current;
	xsIntegerValue depth = 0;
	xsVars(piuTextDepth);
	xsVar(depth) = xsNewArray(0);
	while (nodeOffset < nodeLimit) {
		switch (KIND(nodeOffset)) {
		case piuTextNodeBeginBlockKind:
		case piuTextNodeBeginSpanKind: {
			PiuTextNodeBegin node;
			PiuStyle* style;
			PiuTextLink* link;
			xsResult = xsNewObject();	
			xsCall1(xsVar(depth), xsID_push, xsResult);
			depth++;
			if (depth == piuTextDepth)
				xsUnknownError("too deep");
			node = NODE(nodeOffset);
			style = node->style;
			if (style) {
				xsVar(depth) = xsReference((*style)->reference);
				xsDefine(xsResult, xsID_style, xsVar(depth), xsDefault);
			}
			link = node->link;
			if (link) {
				xsVar(depth) = xsReference((*link)->reference);
				xsDefine(xsResult, xsID_link, xsVar(depth), xsDefault);
			}
			xsVar(depth) = xsNewArray(0);
			xsDefine(xsResult, xsID_spans, xsVar(depth), xsDefault);
			nodeOffset += sizeof(PiuTextNodeBeginRecord);
			} break;
		case piuTextNodeEndBlockKind:
		case piuTextNodeEndSpanKind:
			depth--;
			nodeOffset += sizeof(PiuTextNodeEndRecord);
			break;
		case piuTextNodeStringKind: {
			PiuTextNodeString node = NODE(nodeOffset);
			xsResult = *(node->string);
			xsCall1(xsVar(depth), xsID_push, xsResult);
			nodeOffset += sizeof(PiuTextNodeStringRecord);
			} break;
		}
	}
	xsResult = xsVar(depth);
}

void PiuText_get_string(xsMachine *the)
{
	PiuText* self = PIU(Text, xsThis);
	PiuTextBuffer* nodeBuffer = (*self)->nodeBuffer;
	PiuTextOffset nodeOffset = sizeof(PiuTextBufferRecord);
	PiuTextOffset nodeLimit = (PiuTextOffset)(*nodeBuffer)->current;
	while (nodeOffset < nodeLimit) {
		switch (KIND(nodeOffset)) {
		case piuTextNodeBeginBlockKind:
		case piuTextNodeBeginSpanKind:
			nodeOffset += sizeof(PiuTextNodeBeginRecord);
			break;
		case piuTextNodeEndBlockKind:
		case piuTextNodeEndSpanKind:
			nodeOffset += sizeof(PiuTextNodeEndRecord);
			break;
		case piuTextNodeStringKind: {
			PiuTextNodeString node = NODE(nodeOffset);
			if (xsTest(xsResult))
				xsResult = xsCall1(xsResult, xsID_concat, *(node->string));
			else
				xsResult = *(node->string);
			nodeOffset += sizeof(PiuTextNodeStringRecord);
			} break;
		}
	}
}

void PiuText_set_blocks(xsMachine *the)
{
	PiuText* self = PIU(Text, xsThis);
	xsVars(piuTextDepth);
	xsVar(0) = xsArg(0);
	PiuTextBegin(self);
	PiuTextBuild(self, 0, 0);
	PiuTextEnd(self);
}

void PiuText_set_string(xsMachine *the)
{
	PiuText* self = PIU(Text, xsThis);
	PiuTextBegin(self);
	PiuTextBeginNode(self, piuTextNodeBeginBlockKind, NULL, NULL);
	PiuTextConcat(self, PiuString(xsArg(0)));
	PiuTextEndNode(self, piuTextNodeEndBlockKind);
	PiuTextEnd(self);
}

void PiuText_begin(xsMachine *the)
{
	PiuText* self = PIU(Text, xsThis);
	PiuTextBegin(self);
}

void PiuText_beginBlock(xsMachine *the)
{
	PiuText* self = PIU(Text, xsThis);
	xsIntegerValue c = xsToInteger(xsArgc);
	PiuStyle* style = ((c > 0) && xsTest(xsArg(0))) ? PIU(Style, xsArg(0)) : NULL;
	PiuTextLink* link = ((c > 1) && xsTest(xsArg(1))) ? PIU(TextLink, xsArg(1)) : NULL;
	if ((*self)->nodeOffset != -1)
		xsUnknownError("block inside block");
	PiuTextBeginNode(self, piuTextNodeBeginBlockKind, style, link);
}

void PiuText_beginSpan(xsMachine *the)
{
	PiuText* self = PIU(Text, xsThis);
	xsIntegerValue c = xsToInteger(xsArgc);
	PiuStyle* style = ((c > 0) && xsTest(xsArg(0))) ? PIU(Style, xsArg(0)) : NULL;
	PiuTextLink* link = ((c > 1) && xsTest(xsArg(1))) ? PIU(TextLink, xsArg(1)) : NULL;
	if ((*self)->nodeOffset == -1)
		xsUnknownError("span outside block");
	PiuTextBeginNode(self, piuTextNodeBeginSpanKind, style, link);
}

void PiuText_concat(xsMachine *the)
{
	PiuText* self = PIU(Text, xsThis);
	PiuTextConcat(self, PiuString(xsArg(0)));;
}

void PiuText_end(xsMachine *the)
{
	PiuText* self = PIU(Text, xsThis);
	if ((*self)->nodeOffset != -1)
		xsUnknownError("mismatch");
	PiuTextEnd(self);
}

void PiuText_endBlock(xsMachine *the)
{
	PiuText* self = PIU(Text, xsThis);
	if ((*self)->nodeOffset == -1)
		xsUnknownError("no block");
	PiuTextEndNode(self, piuTextNodeEndBlockKind);
	if ((*self)->nodeOffset != -1)
		xsUnknownError("block inside block");
}

void PiuText_endSpan(xsMachine *the)
{
	PiuText* self = PIU(Text, xsThis);
	if ((*self)->nodeOffset == -1)
		xsUnknownError("no span");
	PiuTextEndNode(self, piuTextNodeEndSpanKind);
	if ((*self)->nodeOffset == -1)
		xsUnknownError("span outside block");
}




