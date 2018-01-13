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

typedef struct PiuCodeStruct PiuCodeRecord, *PiuCode;
typedef struct PiuCodeIteratorStruct PiuCodeIteratorRecord, *PiuCodeIterator;
typedef struct PiuCodeParserStruct PiuCodeParserRecord, *PiuCodeParser;
typedef struct PiuCodeRunStruct PiuCodeRunRecord, *PiuCodeRun;
typedef struct PiuCodeResultStruct PiuCodeResultRecord, *PiuCodeResult;

struct PiuCodeStruct {
	PiuHandlePart;
	PiuIdlePart;
	PiuBehaviorPart;
	PiuContentPart;
	PiuStyle* computedStyle;
	xsSlot* string;
	uint32_t size;
	int32_t type;
	uint32_t length;
	int32_t from;
	int32_t fromColumn;
	int32_t fromLine;
	int32_t to;
	int32_t toColumn;
	int32_t toLine;
	int32_t columnCount;
	int32_t lineCount;
	double columnWidth;
	double lineHeight;
	PiuTextBuffer* runs;
	xsIntegerValue* code;
	xsIntegerValue* data;
	PiuTextBuffer* results;
	int32_t resultsSize;
};

struct PiuCodeIteratorStruct {
	char character;
	int16_t color;
	int16_t kind;
	int32_t limit;
	int32_t offset;
	PiuCodeRun run;
};

struct PiuCodeParserStruct {
	xsMachine* the;
	xsSlot* slot;
	PiuTextBuffer* runs;
	
	int32_t character;
	int32_t columnCount;
	int32_t columnIndex;
	int32_t lineCount;
	int32_t input;
	int32_t output;
	int32_t size;
	char* string;
	int32_t tab;
	
	int32_t color;
	int32_t offset;
	int32_t token;
};

struct PiuCodeRunStruct {
	int16_t kind;
	int16_t color;
	int32_t count;
};

struct PiuCodeResultStruct {
	int32_t count;
	int32_t from;
	int32_t fromColumn;
	int32_t fromLine;
	int32_t to;
	int32_t toColumn;
	int32_t toLine;
};

enum {
	piuCodeColorKind = 0,
	piuCodeLineKind,
	piuCodeTabKind,
};

extern void PiuCodeParserAdvance(PiuCodeParser parser);
extern void PiuCodeParserBegin(PiuCode* self, PiuCodeParser parser);
extern void PiuCodeParserColorAt(PiuCodeParser parser, int32_t color, int32_t offset);
extern void PiuCodeParserEnd(PiuCode* self, PiuCodeParser parser);
extern void PiuCodeParserError(PiuCodeParser parser);
extern void PiuCodeParserFill(PiuCodeParser parser);
extern void PiuCodeParserReturn(PiuCodeParser parser);
extern void PiuCodeParserTab(PiuCodeParser parser);

extern void PiuCodeFormatJS(PiuCodeParser parser);

extern void PiuCodeFormatJSON(PiuCodeParser parser);

extern void PiuCodeFormatXML(PiuCodeParser parser);







