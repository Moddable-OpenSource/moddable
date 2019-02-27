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

enum {
	XS_NO_JSON_TOKEN,
	XS_JSON_TOKEN_COLON,
	XS_JSON_TOKEN_COMMA,
	XS_JSON_TOKEN_EOF,
	XS_JSON_TOKEN_FALSE,
	XS_JSON_TOKEN_LEFT_BRACE,
	XS_JSON_TOKEN_LEFT_BRACKET,
	XS_JSON_TOKEN_NULL,
	XS_JSON_TOKEN_NUMBER,
	XS_JSON_TOKEN_RIGHT_BRACE,
	XS_JSON_TOKEN_RIGHT_BRACKET,
	XS_JSON_TOKEN_STRING,
	XS_JSON_TOKEN_TRUE,
};

static void fxParseJSONArray(PiuCodeParser parser);
static void fxParseJSONObject(PiuCodeParser parser);
static void fxParseJSONToken(PiuCodeParser parser);
static void fxParseJSONValue(PiuCodeParser parser);

void PiuCodeFormatJSON(PiuCodeParser parser) 
{
	fxParseJSONToken(parser);
	fxParseJSONValue(parser);
	if (parser->token != XS_JSON_TOKEN_EOF)
		PiuCodeParserError(parser);
}

void fxParseJSONArray(PiuCodeParser parser)
{
	fxParseJSONToken(parser);
	for (;;) {
		if (parser->token == XS_JSON_TOKEN_RIGHT_BRACKET)
			break;
		fxParseJSONValue(parser);
		if (parser->token != XS_JSON_TOKEN_COMMA)
			break;
		fxParseJSONToken(parser);
	}
	if (parser->token != XS_JSON_TOKEN_RIGHT_BRACKET)
		PiuCodeParserError(parser);
	fxParseJSONToken(parser);
}

void fxParseJSONObject(PiuCodeParser parser)
{
	fxParseJSONToken(parser);
	for (;;) {
		if (parser->token == XS_JSON_TOKEN_RIGHT_BRACE)
			break;
		if (parser->token != XS_JSON_TOKEN_STRING)
			PiuCodeParserError(parser);
		PiuCodeParserColorAt(parser, 1, parser->offset);
		PiuCodeParserColorAt(parser, 0, parser->input);
		fxParseJSONToken(parser);
		if (parser->token != XS_JSON_TOKEN_COLON)
			PiuCodeParserError(parser);
		fxParseJSONToken(parser);
		fxParseJSONValue(parser);
		if (parser->token != XS_JSON_TOKEN_COMMA)
			break;
		fxParseJSONToken(parser);
	}
	if (parser->token != XS_JSON_TOKEN_RIGHT_BRACE)
		PiuCodeParserError(parser);
	fxParseJSONToken(parser);
}

void fxParseJSONToken(PiuCodeParser parser) 
{
	int i;
	parser->token = XS_NO_JSON_TOKEN;
	while (parser->token == XS_NO_JSON_TOKEN) {
		parser->offset = parser->input;
		switch (parser->character) {
		case 0:
			parser->token = XS_JSON_TOKEN_EOF;
			break;
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
			
		case '-':
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			if (parser->character == '-')
				PiuCodeParserAdvance(parser);
			if (('0' <= parser->character) && (parser->character <= '9')) {
				if (parser->character == '0') {
					PiuCodeParserAdvance(parser);
				}
				else {
					PiuCodeParserAdvance(parser);
					while (('0' <= parser->character) && (parser->character <= '9')) {
						PiuCodeParserAdvance(parser);
					}
				}
				if (parser->character == '.') {
					PiuCodeParserAdvance(parser);
					if (('0' <= parser->character) && (parser->character <= '9')) {
						PiuCodeParserAdvance(parser);
						while (('0' <= parser->character) && (parser->character <= '9')) {
							PiuCodeParserAdvance(parser);
						}
					}
					else
						PiuCodeParserError(parser);
				}
				if ((parser->character == 'e') || (parser->character == 'E')) {
					PiuCodeParserAdvance(parser);
					if ((parser->character== '+') || (parser->character == '-')) {
						PiuCodeParserAdvance(parser);
					}
					if (('0' <= parser->character) && (parser->character <= '9')) {
						PiuCodeParserAdvance(parser);
						while (('0' <= parser->character) && (parser->character <= '9')) {
							PiuCodeParserAdvance(parser);
						}
					}
					else
						PiuCodeParserError(parser);
				}
			}
			else
				PiuCodeParserError(parser);	
			parser->token = XS_JSON_TOKEN_NUMBER;
			break;
		case ',':
			PiuCodeParserAdvance(parser);
			parser->token = XS_JSON_TOKEN_COMMA;
			break;	
		case ':':
			PiuCodeParserAdvance(parser);
			parser->token = XS_JSON_TOKEN_COLON;
			break;	
		case '[':
			PiuCodeParserAdvance(parser);
			parser->token = XS_JSON_TOKEN_LEFT_BRACKET;
			break;	
		case ']':
			PiuCodeParserAdvance(parser);
			parser->token = XS_JSON_TOKEN_RIGHT_BRACKET;
			break;	
		case '{':
			PiuCodeParserAdvance(parser);
			parser->token = XS_JSON_TOKEN_LEFT_BRACE;
			break;	
		case '}':
			PiuCodeParserAdvance(parser);
			parser->token = XS_JSON_TOKEN_RIGHT_BRACE;
			break;	
		case '"':
			PiuCodeParserAdvance(parser);
			for (;;) {
				if (parser->character < 32) {
					PiuCodeParserError(parser);				
					break;
				}
				else if (parser->character == '"') {
					PiuCodeParserAdvance(parser);
					break;
				}
				else if (parser->character == '\\') {
					PiuCodeParserAdvance(parser);
					switch (parser->character) {
					case '"':
					case '/':
					case '\\':
					case 'b':
					case 'f':
					case 'n':
					case 'r':
					case 't':
						PiuCodeParserAdvance(parser);
						break;
					case 'u':
						PiuCodeParserAdvance(parser);
						for (i = 0; i < 4; i++) {
							if (('0' <= parser->character) && (parser->character <= '9'))
								PiuCodeParserAdvance(parser);
							else if (('a' <= parser->character) && (parser->character <= 'f'))
								PiuCodeParserAdvance(parser);
							else if (('A' <= parser->character) && (parser->character <= 'F'))
								PiuCodeParserAdvance(parser);
							else
								PiuCodeParserError(parser);
						}
						break;
					default:
						PiuCodeParserError(parser);
						break;
					}
				}
				else {
					PiuCodeParserAdvance(parser);
				}
			}
			parser->token = XS_JSON_TOKEN_STRING;
			break;
		case 'f':
			PiuCodeParserAdvance(parser);
			if (parser->character != 'a') PiuCodeParserError(parser);	
			PiuCodeParserAdvance(parser);
			if (parser->character != 'l') PiuCodeParserError(parser);	
			PiuCodeParserAdvance(parser);
			if (parser->character != 's') PiuCodeParserError(parser);	
			PiuCodeParserAdvance(parser);
			if (parser->character != 'e') PiuCodeParserError(parser);	
			PiuCodeParserAdvance(parser);
			parser->token = XS_JSON_TOKEN_FALSE;
			break;
		case 'n':
			PiuCodeParserAdvance(parser);
			if (parser->character != 'u') PiuCodeParserError(parser);	
			PiuCodeParserAdvance(parser);
			if (parser->character != 'l') PiuCodeParserError(parser);	
			PiuCodeParserAdvance(parser);
			if (parser->character != 'l') PiuCodeParserError(parser);	
			PiuCodeParserAdvance(parser);
			parser->token = XS_JSON_TOKEN_NULL;
			break;
		case 't':
			PiuCodeParserAdvance(parser);
			if (parser->character != 'r') PiuCodeParserError(parser);	
			PiuCodeParserAdvance(parser);
			if (parser->character != 'u') PiuCodeParserError(parser);	
			PiuCodeParserAdvance(parser);
			if (parser->character != 'e') PiuCodeParserError(parser);	
			PiuCodeParserAdvance(parser);
			parser->token = XS_JSON_TOKEN_TRUE;
			break;
		default:
			PiuCodeParserError(parser);	
		}
	}
}

void fxParseJSONValue(PiuCodeParser parser)
{
	switch (parser->token) {
	case XS_JSON_TOKEN_FALSE:
	case XS_JSON_TOKEN_TRUE:
	case XS_JSON_TOKEN_NULL:
	case XS_JSON_TOKEN_NUMBER:
	case XS_JSON_TOKEN_STRING:
		PiuCodeParserColorAt(parser, 2, parser->offset);
		PiuCodeParserColorAt(parser, 0, parser->input);
		fxParseJSONToken(parser);
		break;
	case XS_JSON_TOKEN_LEFT_BRACE:
		fxParseJSONObject(parser);
		break;
	case XS_JSON_TOKEN_LEFT_BRACKET:
		fxParseJSONArray(parser);
		break;
	default:
		return;	
	}
}
