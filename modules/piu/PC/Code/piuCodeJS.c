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

typedef struct {
	char* string;
	xsBooleanValue noRegExp;
} txKeyword;

static xsBooleanValue fxIsJSIdentifierFirst(uint32_t c);
static xsBooleanValue fxIsJSIdentifierNext(uint32_t c);
static xsBooleanValue fxIsKeyword(char* string, uint32_t length, xsBooleanValue* noRegExp);
static void fxParseJSCode(PiuCodeParser parser, xsBooleanValue template);
static void fxParseJSNumberB(PiuCodeParser parser);
static void fxParseJSNumberE(PiuCodeParser parser, int parseDot);
static void fxParseJSNumberO(PiuCodeParser parser);
static void fxParseJSNumberX(PiuCodeParser parser);
static void fxParseJSString(PiuCodeParser parser);

#define XS_KEYWORD_COUNT 46
static const txKeyword gxKeywords[XS_KEYWORD_COUNT] = {
	{ "break", 0 },
	{ "case", 0 },
	{ "catch", 0 },
	{ "class", 0 },
	{ "const", 0 },
	{ "continue", 0 },
	{ "debugger", 0 },
	{ "default", 0 },
	{ "delete", 0 },
	{ "do", 0 },
	{ "else", 0 },
	{ "enum", 0 },
	{ "export", 0 },
	{ "extends", 0 },
	{ "false", 1 },
	{ "finally", 0 },
	{ "for", 0 },
	{ "function", 0 },
	{ "if", 0 },
	{ "implements", 0 },
	{ "import", 0 },
	{ "in", 0 },
	{ "instanceof", 0 },
	{ "interface", 0 },
	{ "let", 0 },
	{ "new", 0 }, 
	{ "null", 1 }, 
	{ "package", 0 },
	{ "private", 0 },
	{ "protected", 0 },
	{ "public", 0 },
	{ "return", 0 },
	{ "static", 0 },
	{ "super", 1 },
	{ "switch", 0 },
	{ "this", 1 },
	{ "throw", 0 },
	{ "true", 1 },
	{ "try", 0 },
	{ "typeof", 0 },
	{ "undefined", 1 },
	{ "var", 0 },
	{ "void", 0 },
	{ "while", 0 },
	{ "with", 0 },
	{ "yield", 0 },
};

void PiuCodeFormatJS(PiuCodeParser parser) 
{
	fxParseJSCode(parser, 0);
}

xsBooleanValue fxIsJSIdentifierFirst(uint32_t c)
{
	return ((('A' <= c) && (c <= 'Z')) || (('a' <= c) && (c <= 'z')) || (c == '$') || (c == '_')) ? 1 : 0;
}

xsBooleanValue fxIsJSIdentifierNext(uint32_t c)
{
	return ((('0' <= c) && (c <= '9')) || (('A' <= c) && (c <= 'Z')) || (('a' <= c) && (c <= 'z')) || (c == '$') || (c == '_')) ? 1 : 0;
}

xsBooleanValue fxIsKeyword(char* string, uint32_t length, xsBooleanValue* noRegExp)
{
	char* keywordString;
	uint32_t keywordLength;
	int low, high, index, delta;
	for (low = 0, high = XS_KEYWORD_COUNT; high > low; (delta < 0) ? (low = index + 1) : (high = index)) {
		index = low + ((high - low) / 2);
		keywordString = gxKeywords[index].string;
		keywordLength = c_strlen(keywordString);
		delta = c_strncmp(gxKeywords[index].string, (const char*)string, length);
		if (delta == 0) {
			if (keywordLength < length)
				delta = 1;		
			else if (keywordLength > length)
				delta = -1;		
			else {
				*noRegExp = gxKeywords[index].noRegExp;
				return 1;
			}
		}
	}
	return 0;
}

void fxParseJSCode(PiuCodeParser parser, xsBooleanValue template) 
{
	int32_t braces = 0;
	int32_t offset;
	uint32_t c;
	xsBooleanValue noRegExp = 0;
	for (;;) {
		offset = parser->input;
		if (!parser->character)
			break;
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
			
		case '0':
			PiuCodeParserAdvance(parser);
			c = parser->character;
			if (c == '.') {
				PiuCodeParserAdvance(parser);
				c = parser->character;
				if ((('0' <= c) && (c <= '9')) || (c == 'e') || (c == 'E'))
					fxParseJSNumberE(parser, 0);
			}
			else if ((c == 'b') || (c == 'B'))
				fxParseJSNumberB(parser);
			else if ((c == 'e') || (c == 'E'))
				fxParseJSNumberE(parser, 0);
			else if ((c == 'o') || (c == 'O'))
				fxParseJSNumberO(parser);
			else if ((c == 'x') || (c == 'X'))
				fxParseJSNumberX(parser);
			else if (('0' <= c) && (c <= '7'))
				fxParseJSNumberO(parser);
			PiuCodeParserColorAt(parser, 2, offset);
			PiuCodeParserColorAt(parser, 0, parser->input);
			noRegExp = 1;
			break;
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			fxParseJSNumberE(parser, 1);
			PiuCodeParserColorAt(parser, 2, offset);
			PiuCodeParserColorAt(parser, 0, parser->input);
			noRegExp = 1;
			break;
		case '.':
			PiuCodeParserAdvance(parser);
			c = parser->character;
			if (('0' <= c) && (c <= '9')) {
				fxParseJSNumberE(parser, 0);
				PiuCodeParserColorAt(parser, 2, offset);
				PiuCodeParserColorAt(parser, 0, parser->input);
			}
			noRegExp = 1;
			break;
				
		case '{':
			if (template)
				braces++;
            PiuCodeParserAdvance(parser);
			noRegExp = 0;
			break;
		case '}':
			if (template) {
				if (braces == 0) {
					PiuCodeParserColorAt(parser, 2, offset);
					return;
				}
				braces--;
			}
            PiuCodeParserAdvance(parser);
			noRegExp = 0;
			break;
		case ']':
		case ')':
            PiuCodeParserAdvance(parser);
			noRegExp = 1;
			break;
		case '"':
		case '\'':
		case '`':
			PiuCodeParserColorAt(parser, 2, offset);
			fxParseJSString(parser);
			noRegExp = 1;
			break;
		
		case '/':
			PiuCodeParserAdvance(parser);
			if (parser->character == '*') {
				PiuCodeParserAdvance(parser);
				PiuCodeParserColorAt(parser, 3, offset);
				for (;;) {
					if (parser->character == 0) {
						break;
					}
					else if (parser->character == 9) {
						PiuCodeParserTab(parser);
					}
					else if ((parser->character == 10) || (parser->character == 13)) {
						PiuCodeParserReturn(parser);
					}
					else if (parser->character == '*') {
						PiuCodeParserAdvance(parser);
						if (parser->character == '/') {
							PiuCodeParserAdvance(parser);
							PiuCodeParserColorAt(parser, 0, parser->input);
							break;
						}
					}
					else
						PiuCodeParserAdvance(parser);
				}
			}
			else if (parser->character == '/') {
				PiuCodeParserAdvance(parser);
				PiuCodeParserColorAt(parser, 3, offset);
				for (;;) {
					if (parser->character == 0) {
						break;
					}
					else if (parser->character == 9) {
						PiuCodeParserTab(parser);
					}
					else if ((parser->character == 10) || (parser->character == 13)) {
						PiuCodeParserReturn(parser);
						PiuCodeParserColorAt(parser, 0, parser->input);
						break;
					}
					else
						PiuCodeParserAdvance(parser);
				}
			}
			else  if (!noRegExp) {
				PiuCodeParserColorAt(parser, 2, offset);
				for (;;) {
					if (parser->character == 0) {
						break;
					}
					else if (parser->character == 9) {
						PiuCodeParserTab(parser);
					}
					else if ((parser->character == 10) || (parser->character == 13)) {
						PiuCodeParserReturn(parser);
						PiuCodeParserColorAt(parser, 0, parser->input);
						break;
					}
					else if (parser->character == '/') {
						PiuCodeParserAdvance(parser);
						while (('a' <= parser->character) && (parser->character <= 'z'))
							PiuCodeParserAdvance(parser);
						PiuCodeParserColorAt(parser, 0, parser->input);
						break;
					}
					else if (parser->character == '\\') {
						PiuCodeParserAdvance(parser);
						if (parser->character == '/')
							PiuCodeParserAdvance(parser);
					}
					else
						PiuCodeParserAdvance(parser);
				}
				noRegExp = 1;
			}
			break;
			
		default:
			if (fxIsJSIdentifierFirst(parser->character)) {
				for (;;) {
					PiuCodeParserAdvance(parser);
					if (!fxIsJSIdentifierNext(parser->character))
						break;
				}
				if (fxIsKeyword(parser->string + offset, parser->input - offset, &noRegExp)) {
					PiuCodeParserColorAt(parser, 1, offset);
					PiuCodeParserColorAt(parser, 0, parser->input);
				}
				else
					noRegExp = 1;
			}
			else {
				PiuCodeParserAdvance(parser);
				noRegExp = 0;
			}
			break;		
		}
	}
}

void fxParseJSNumberB(PiuCodeParser parser)
{
	uint32_t c;
	for (;;) {
		PiuCodeParserAdvance(parser);
		c = parser->character;
		if (('0' <= c) && (c <= '1'))
			continue;
		break;
	}
}

void fxParseJSNumberE(PiuCodeParser parser, int parseDot)
{
	while (('0' <= parser->character) && (parser->character <= '9')) {
		PiuCodeParserAdvance(parser);
	}
	if (parseDot) {
		if (parser->character == '.') {
			PiuCodeParserAdvance(parser);
			while (('0' <= parser->character) && (parser->character <= '9')) {
				PiuCodeParserAdvance(parser);
			}
		}
	}
	if ((parser->character == 'e') || (parser->character == 'E')) {
		PiuCodeParserAdvance(parser);
		if ((parser->character == '+') || (parser->character == '-')) {
			PiuCodeParserAdvance(parser);
		}
		while (('0' <= parser->character) && (parser->character <= '9')) {
			PiuCodeParserAdvance(parser);
		}
	}
}

void fxParseJSNumberO(PiuCodeParser parser)
{
	uint32_t c;
	for (;;) {
		PiuCodeParserAdvance(parser);
		c = parser->character;
		if (('0' <= c) && (c <= '7'))
			continue;
		break;
	}
}

void fxParseJSNumberX(PiuCodeParser parser)
{
	int c;
	for (;;) {
		PiuCodeParserAdvance(parser);
		c = parser->character;
		if (('0' <= c) && (c <= '9'))
			continue;
		if (('a' <= c) && (c <= 'f'))
			continue;
		if (('A' <= c) && (c <= 'F'))
			continue;
		break;
	}
}

void fxParseJSString(PiuCodeParser parser)
{
	int32_t c = parser->character;
	PiuCodeParserAdvance(parser);
	for (;;) {
		if (parser->character == 0) {
			break;
		}
		else if (parser->character == 9) {
			PiuCodeParserTab(parser);
		}
		else if ((parser->character == 10) || (parser->character == 13)) {
			PiuCodeParserReturn(parser);
			if (c != '`') {
				PiuCodeParserColorAt(parser, 0, parser->input);
				break;
			}
		}
		else if (parser->character == c) {
			PiuCodeParserAdvance(parser);
			PiuCodeParserColorAt(parser, 0, parser->input);
			break;
		}
		else if (parser->character == '$') {
			PiuCodeParserAdvance(parser);
			if ((c == '`') && (parser->character == '{')) {
				PiuCodeParserAdvance(parser);
				PiuCodeParserColorAt(parser, 0, parser->input);
				fxParseJSCode(parser, 1);
			}
		}
		else if (parser->character == '\\') {
			PiuCodeParserAdvance(parser);
			if (parser->character == 9) {
				PiuCodeParserTab(parser);
			}
			else if ((parser->character == 10) || (parser->character == 13)) {
				PiuCodeParserReturn(parser);
			}
			else {
				PiuCodeParserAdvance(parser);
			}	
		}	
		else
			PiuCodeParserAdvance(parser);
	}	
}
