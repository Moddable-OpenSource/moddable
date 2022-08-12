/*
 * Copyright (c) 2016-2022  Moddable Tech, Inc.
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

#include "xsScript.h"

static void fxSourceMapLines(txParser* parser, txString mappings);
static txInteger fxSourceMapValue(txParser* parser, txString* p);

#define mxAssert(ASSERTION,MESSAGE) if (!(ASSERTION)) { fxReportParserError(parser, parser->line, MESSAGE); return; }

void fxParserSourceMap(txParser* parser, void* theStream, txGetter theGetter, txUnsigned flags, txString* name)
{
	txInteger line = parser->line;
	txNode* root = parser->root;
	txNode* object;
    txNode* item;
    txNode* node;
	txPropertyAtNode* property;
	txString id;
	txInteger version = 0;
	txString source = NULL;
	txString mappings = NULL;
	
	parser->line = 0;
	parser->root = NULL;
	parser->stream = theStream;
	parser->getter = theGetter;
	
	parser->line2 = 1;
	parser->crlf2 = 0;
	parser->escaped2 = 0;
	parser->integer2 = 0;
	parser->modifierLength2 = 0;
	parser->modifier2 = parser->emptyString;
	parser->number2 = 0;
	parser->stringLength2 = 0;
	parser->string2 = parser->emptyString;
	parser->symbol2 = C_NULL;
	parser->token2 = XS_NO_TOKEN;
	
	parser->lookahead = 0;
#if mxCESU8
	parser->surrogate = 0;
#endif
	fxGetNextCharacter(parser);
	fxGetNextCharacter(parser);
	fxGetNextTokenJSON(parser);
	fxGetNextTokenJSON(parser);
	fxJSONValue(parser);
	if (parser->errorCount)
		return;
	object = parser->root;
	parser->line = line;
	parser->root = root;
	
	mxAssert(object->description->token == XS_TOKEN_OBJECT, "source map: no object");
	mxAssert(((txObjectNode*)object)->items, "source map: no properties");
	item = ((txObjectNode*)object)->items->first;
	while (item) {
		mxAssert(item->description->token == XS_TOKEN_PROPERTY_AT, "source map: no property");
		property = (txPropertyAtNode*)item;
		mxAssert(property->at->description->token == XS_TOKEN_STRING, "source map: no property name");
		id = ((txStringNode*)(property->at))->value;
		if (!c_strcmp(id, "version")) {
			mxAssert(property->value->description->token == XS_TOKEN_INTEGER, "source map: version is no integer");
			version = ((txIntegerNode*)(property->value))->value;
		}
		else if (!c_strcmp(id, "sources")) {
			mxAssert(property->value->description->token == XS_TOKEN_ARRAY, "source map: sources is no array");
			node = ((txArrayNode*)(property->value))->items->first;
			mxAssert(node->description->token == XS_TOKEN_STRING, "source map: sources[0] is no string");
			source = ((txStringNode*)(node))->value;
		}
		else if (!c_strcmp(id, "mappings")) {
			mxAssert(property->value->description->token == XS_TOKEN_STRING, "source map: mappings is no string");
			mappings = ((txStringNode*)(property->value))->value;
		}
		item = item->next;
	}
	mxAssert(version == 3, "source map: version is not 3");
	mxAssert(source, "source map: no source");
	mxAssert(mappings, "source map: no mappings");
	
	parser->lines = fxNewParserChunkClear(parser, (1 + line) * sizeof(txInteger));

	fxSourceMapLines(parser, mappings);
	*name = source;
}

void fxSourceMapLines(txParser* parser, txString mappings)
{
	enum { BEGIN, SOURCE, LINE, COLUMN, NAME, END };
	txInteger generatedLine = 0;
//	txInteger generatedColumn = 0;
//	txInteger name = 0;
//	txInteger source = 0;
	txInteger sourceLine = 0;
//	txInteger sourceColumn = 0;
	txInteger state = BEGIN;
	txString p = mappings;
	char c;
	while ((c = *p)) {
		if (c == ';') {
			p++;
			if ((state >= NAME) && (generatedLine < parser->line))
				parser->lines[1 + generatedLine] = 1 + sourceLine;
			generatedLine++;
//			generatedColumn = 0;
			state = BEGIN;
		}
		else if (c == ',') {
			p++;
			if ((state >= NAME) && (generatedLine < parser->line))
				parser->lines[1 + generatedLine] = 1 + sourceLine;
			state = BEGIN;
		}
		else {
			txInteger value = fxSourceMapValue(parser, &p);
			switch(state) {
			case BEGIN:
//				generatedColumn += value;
				state = SOURCE;
				break;
			case SOURCE:
//				source += value;
				state = LINE;
				break;
			case LINE:
				sourceLine += value;
				state = COLUMN;
				break;
			case COLUMN:
//				sourceColumn += value;
				state = NAME;
				break;
			case NAME:
//				name += value;
				state = END;
				break;
			}
		}
	}
}

#define VLQ_SHIFT 5
// binary: 100000
#define VLQ_CONTINUATION_BIT (1 << VLQ_SHIFT)
// binary: 011111
#define VLQ_MASK_BITS (VLQ_CONTINUATION_BIT - 1)

txInteger fxSourceMapValue(txParser* parser, txString* p)
{
	txInteger continuation, digit, result = 0, shift = 0;
	txString q = *p;
	do {
		char c = *q++;
		if (('A' <= c) && (c <= 'Z'))
			digit = c - 'A';
		else if (('a' <= c) && (c <= 'z'))
			digit = c - 'a' + 26;
		else if (('0' <= c) && (c <= '9'))
			digit = c - '0' + 52;
		else if (c == '+')
			digit = 62;
		else if (c == '/')
			digit = 63;
		else
			fxReportParserError(parser, parser->line, "source map: unexpected character"); 
		continuation = digit & VLQ_CONTINUATION_BIT;
		digit &= VLQ_MASK_BITS;
		result += (digit << shift);
		shift += VLQ_SHIFT;
	} while (continuation);
	*p = q;
	shift = result >> 1;
	return ((result & 1) == 1) ? -shift : shift;
}







