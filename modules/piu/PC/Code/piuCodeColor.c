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
 * This file incorporates work covered by the following copyright and  
 * permission notice:  
 *
 *       Copyright (C) 2010-2016 Marvell International Ltd.
 *       Copyright (C) 2002-2010 Kinoma, Inc.
 *
 *       Licensed under the Apache License, Version 2.0 (the "License");
 *       you may not use this file except in compliance with the License.
 *       You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *       Unless required by applicable law or agreed to in writing, software
 *       distributed under the License is distributed on an "AS IS" BASIS,
 *       WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *       See the License for the specific language governing permissions and
 *       limitations under the License.
 */

#include "piuCode.h"

void PiuCodeParserAdvance(PiuCodeParser parser)
{
	xsStringValue string; 
	parser->columnIndex++;
	parser->input += parser->size;
	string = fxUTF8Decode(parser->string + parser->input, &parser->character);
	if (parser->character <= 0) {
		parser->character = 0;
		parser->size = 0;
	}
	else {
		parser->size = string - (parser->string + parser->input);
	}
}

void PiuCodeParserBegin(PiuCode* self, PiuCodeParser parser)
{
	parser->the = (*self)->the;
	parser->slot = (*self)->string;
	parser->runs = (*self)->runs;
	parser->columnCount = 0;
	parser->columnIndex = -1;
	parser->lineCount = 1;
	parser->input = 0;
	parser->output = 0;
	parser->size = 0;
	parser->string = fxToString(parser->the, parser->slot);
	parser->tab = 4;
	parser->color = 0;
	parser->offset = 0;
	PiuTextBufferClear(parser->the, parser->runs);
	PiuCodeParserAdvance(parser);
}

void PiuCodeParserColorAt(PiuCodeParser parser, int32_t color, int32_t offset)
{
	PiuCodeRunRecord runRecord;
	if (parser->color != color) {
		if (offset > parser->output) {
			runRecord.kind = piuCodeColorKind;
			runRecord.color = parser->color;
			runRecord.count = offset - parser->output;
			PiuTextBufferAppend(parser->the, parser->runs, &runRecord, sizeof(PiuCodeRunRecord));
			parser->string = fxToString(parser->the, parser->slot);
			parser->output = offset;
		}
		parser->color = color;
	}
}

void PiuCodeParserEnd(PiuCode* self, PiuCodeParser parser)
{
	PiuCodeParserColorAt(parser, 0, parser->input);
	while (parser->character) {
		switch (parser->character) {
		case 9:
			PiuCodeParserTab(parser);
			break;
		case 10:
		case 13:
			PiuCodeParserReturn(parser);
			break;
		default:
			PiuCodeParserAdvance(parser);
			break;
		}
	}
	PiuCodeParserFill(parser);
	if (parser->columnCount < parser->columnIndex)
		parser->columnCount = parser->columnIndex;
	(*self)->columnCount = parser->columnCount;
	(*self)->lineCount = parser->lineCount;
	/*{
		PiuCodeRun run;
		char* string = self->string;
		FskGrowableArrayGetPointerToItem(self->runs, 0, (void **)&run);
		fprintf(stderr, "### %ld %ld\n", parser->lineCount, parser->columnCount);
		while (*string) {
			if (run->kind == piuCodeLineKind)
				fprintf(stderr, "\n#");
			else if (run->kind == piuCodeTabKind)
				fprintf(stderr, "<%d>", run->color);
			else {
				char c = string[run->count];
				string[run->count] = 0;
				fprintf(stderr, "(%d)%s", run->color, string);
				string[run->count] = c;
			}
			string += run->count;
			run++;
		}
		fprintf(stderr, "\n");
	}*/
}

void PiuCodeParserError(PiuCodeParser parser)
{
	xsMachine* the = parser->the;
	xsUnknownError("bad data");
}

void PiuCodeParserFill(PiuCodeParser parser)
{
	PiuCodeRunRecord runRecord;
	if (parser->input > parser->output) {
		runRecord.kind = piuCodeColorKind;
		runRecord.color = parser->color;
		runRecord.count = parser->input - parser->output;
		PiuTextBufferAppend(parser->the, parser->runs, &runRecord, sizeof(PiuCodeRunRecord));
		parser->string = fxToString(parser->the, parser->slot);
		parser->output = parser->input;
	}
}

void PiuCodeParserReturn(PiuCodeParser parser)
{
	uint32_t character = parser->character;
	PiuCodeRunRecord runRecord;
	PiuCodeParserFill(parser);
	PiuCodeParserAdvance(parser);
	if ((character == 13) && (parser->character == 10))
		PiuCodeParserAdvance(parser);
	runRecord.kind = piuCodeLineKind;
	runRecord.color = 0;
	runRecord.count = parser->input - parser->output;
	PiuTextBufferAppend(parser->the, parser->runs, &runRecord, sizeof(PiuCodeRunRecord));
	parser->string = fxToString(parser->the, parser->slot);
	parser->output = parser->input;
	if (parser->columnCount < parser->columnIndex)
		parser->columnCount = parser->columnIndex;
	parser->columnIndex = 0;
	parser->lineCount++;
}

void PiuCodeParserTab(PiuCodeParser parser)
{
	PiuCodeRunRecord runRecord;
	PiuCodeParserFill(parser);
	PiuCodeParserAdvance(parser);
	int32_t columnIndex = parser->columnIndex - 1;
	parser->columnIndex = columnIndex + parser->tab;
	parser->columnIndex -= parser->columnIndex % parser->tab;
	runRecord.kind = piuCodeTabKind;
	runRecord.color = parser->columnIndex - columnIndex;
	runRecord.count = 1;
	PiuTextBufferAppend(parser->the, parser->runs, &runRecord, sizeof(PiuCodeRunRecord));
	parser->string = fxToString(parser->the, parser->slot);
	parser->output = parser->input;
}
