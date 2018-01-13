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

static xsBooleanValue fxIsXMLIdentifierFirst(uint32_t c);
static xsBooleanValue fxIsXMLIdentifierNext(uint32_t c);
static void fxParseXMLComment(PiuCodeParser parser);
static void fxParseXMLData(PiuCodeParser parser);
static void fxParseXMLName(PiuCodeParser parser);
static void fxParseXMLProcessingInstruction(PiuCodeParser parser);
static void fxParseXMLSpace(PiuCodeParser parser, int* length);
static void fxParseXMLStartTag(PiuCodeParser parser);
static void fxParseXMLStopTag(PiuCodeParser parser);
static void fxParseXMLValue(PiuCodeParser parser);

void PiuCodeFormatXML(PiuCodeParser parser) 
{
	while (parser->character) {
		fxParseXMLSpace(parser, NULL);
		if (parser->character == '<') {
			parser->offset = parser->input;
			PiuCodeParserAdvance(parser);
			if (parser->character == '/') {
				PiuCodeParserAdvance(parser);
				fxParseXMLStopTag(parser);
			}
			else if (parser->character == '?') {
				PiuCodeParserAdvance(parser);
				fxParseXMLProcessingInstruction(parser);
			}
			else if (parser->character == '!') {
				PiuCodeParserAdvance(parser);
				if (parser->character == '-') {
					PiuCodeParserAdvance(parser);
					if (parser->character != '-') PiuCodeParserError(parser);
					PiuCodeParserAdvance(parser);
					fxParseXMLComment(parser);
				}
				else if (parser->character == '[') {
					PiuCodeParserAdvance(parser);
					if (parser->character != 'C') PiuCodeParserError(parser);
					PiuCodeParserAdvance(parser);
					if (parser->character != 'D') PiuCodeParserError(parser);
					PiuCodeParserAdvance(parser);
					if (parser->character != 'A') PiuCodeParserError(parser);
					PiuCodeParserAdvance(parser);
					if (parser->character != 'T') PiuCodeParserError(parser);
					PiuCodeParserAdvance(parser);
					if (parser->character != 'A') PiuCodeParserError(parser);
					PiuCodeParserAdvance(parser);
					if (parser->character != '[') PiuCodeParserError(parser);
					PiuCodeParserAdvance(parser);
					fxParseXMLData(parser);
				}
				else PiuCodeParserError(parser);
			}
			else
				fxParseXMLStartTag(parser);
			PiuCodeParserColorAt(parser, 0, parser->input);
		}
		else
			PiuCodeParserAdvance(parser);
	}
}

xsBooleanValue fxIsXMLIdentifierFirst(uint32_t c)
{
	return ((('A' <= c) && (c <= 'Z')) || (('a' <= c) && (c <= 'z')) || (c == '_') || (c == ':')) ? 1 : 0;
}

xsBooleanValue fxIsXMLIdentifierNext(uint32_t c)
{
	return ((('0' <= c) && (c <= '9')) || (('A' <= c) && (c <= 'Z')) || (('a' <= c) && (c <= 'z')) || (c == '_') || (c == ':') || (c == '-')) ? 1 : 0;
}

void fxParseXMLComment(PiuCodeParser parser)
{
	PiuCodeParserColorAt(parser, 3, parser->offset);
	while (parser->character) {
		fxParseXMLSpace(parser, NULL);
		if (parser->character == '-') {
			PiuCodeParserAdvance(parser);
			if (parser->character == '-') {
				PiuCodeParserAdvance(parser);
				if (parser->character == '>') {
					PiuCodeParserAdvance(parser);
					break;
				}
			}
		}
		PiuCodeParserAdvance(parser);
	}
}

void fxParseXMLData(PiuCodeParser parser)
{
	PiuCodeParserColorAt(parser, 1, parser->offset);
	PiuCodeParserColorAt(parser, 0, parser->input);
	while (parser->character) {
		fxParseXMLSpace(parser, NULL);
		parser->offset = parser->input;
		if (parser->character == ']') {
			PiuCodeParserAdvance(parser);
			if (parser->character == ']') {
				PiuCodeParserAdvance(parser);
				if (parser->character == '>') {
					PiuCodeParserAdvance(parser);
					PiuCodeParserColorAt(parser, 1, parser->offset);
					break;
				}
			}
		}
		else
			PiuCodeParserAdvance(parser);
	}
}

void fxParseXMLName(PiuCodeParser parser)
{
	if (fxIsXMLIdentifierFirst(parser->character)) {
		PiuCodeParserAdvance(parser);
		while (fxIsXMLIdentifierNext(parser->character))
			PiuCodeParserAdvance(parser);
		return;
	}
	PiuCodeParserError(parser);
}

void fxParseXMLProcessingInstruction(PiuCodeParser parser)
{
	int length;
	PiuCodeParserColorAt(parser, 1, parser->offset);
	fxParseXMLName(parser);
	fxParseXMLSpace(parser, &length);
	while ((parser->character) && (parser->character != '?')) {
		if (!length)
			PiuCodeParserError(parser);
		fxParseXMLName(parser);
		fxParseXMLSpace(parser, NULL);
		if (parser->character != '=')
			PiuCodeParserError(parser);
		PiuCodeParserAdvance(parser);
		fxParseXMLSpace(parser, NULL);
		fxParseXMLValue(parser);
		fxParseXMLSpace(parser, &length);
	}
	if (parser->character != '?') PiuCodeParserError(parser);
	PiuCodeParserAdvance(parser);
	if (parser->character != '>') PiuCodeParserError(parser);
	PiuCodeParserAdvance(parser);
}

void fxParseXMLSpace(PiuCodeParser parser, int* length)
{
	int offset = parser->input;
	for (;;) {
		switch (parser->character) {
		case 9:
			PiuCodeParserTab(parser);
			break;
		case 10:
		case 13:
			PiuCodeParserReturn(parser);
			break;
		case ' ':
			PiuCodeParserAdvance(parser);
			break;
		default:
            if (length)
                *length = parser->input - offset;
			return;
		}
	}
}

void fxParseXMLStartTag(PiuCodeParser parser)
{
	int length;
	PiuCodeParserColorAt(parser, 1, parser->offset);
	fxParseXMLName(parser);
	fxParseXMLSpace(parser, &length);
	while ((parser->character) && (parser->character != '/') && (parser->character != '>')) {
		if (!length)
			PiuCodeParserError(parser);
		fxParseXMLName(parser);
		fxParseXMLSpace(parser, NULL);
		if (parser->character != '=')
			PiuCodeParserError(parser);
		PiuCodeParserAdvance(parser);
		fxParseXMLSpace(parser, NULL);
		fxParseXMLValue(parser);
		fxParseXMLSpace(parser, &length);
	}
	if (parser->character == '/')
		PiuCodeParserAdvance(parser);
	if (parser->character != '>')
		PiuCodeParserError(parser);
	PiuCodeParserAdvance(parser);
}

void fxParseXMLStopTag(PiuCodeParser parser)
{
	PiuCodeParserColorAt(parser, 1, parser->offset);
	fxParseXMLName(parser);
	fxParseXMLSpace(parser, NULL);
	if (parser->character != '>')
		PiuCodeParserError(parser);
	PiuCodeParserAdvance(parser);
}

void fxParseXMLValue(PiuCodeParser parser)
{
	int32_t character = parser->character;
	if ((character != '"') && (character != '\''))
		PiuCodeParserError(parser);
	PiuCodeParserColorAt(parser, 2, parser->input);
	PiuCodeParserAdvance(parser);
	while ((parser->character) && (parser->character != character)) {
		fxParseXMLSpace(parser, NULL);
		PiuCodeParserAdvance(parser);
	}
	if (parser->character != character)
		PiuCodeParserError(parser);
	PiuCodeParserAdvance(parser);
	PiuCodeParserColorAt(parser, 1, parser->input);
	parser->color = 1;
}

