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

#include "xsScript.h"

static void fxGetNextKeyword(txParser* parser);
static txBoolean fxGetNextIdentiferX(txParser* parser, txU4* value);
static void fxGetNextNumber(txParser* parser, txNumber theNumber);
static void fxGetNextNumberB(txParser* parser);
static void fxGetNextNumberE(txParser* parser, int parseDot);
static void fxGetNextNumberO(txParser* parser, int c, int i);
static void fxGetNextNumberX(txParser* parser);
static void fxGetNextString(txParser* parser, int c);
static txBoolean fxGetNextStringD(int c, txU4* value);
static txBoolean fxGetNextStringX(int c, txU4* value);
static void fxGetNextTokenAux(txParser* parser);

#define XS_KEYWORD_COUNT 36
static const txKeyword ICACHE_RODATA_ATTR gxKeywords[XS_KEYWORD_COUNT] = {
	{ "break", XS_TOKEN_BREAK },
	{ "case", XS_TOKEN_CASE },
	{ "catch", XS_TOKEN_CATCH },
	{ "class", XS_TOKEN_CLASS },
	{ "const", XS_TOKEN_CONST },
	{ "continue", XS_TOKEN_CONTINUE },
	{ "debugger", XS_TOKEN_DEBUGGER },
	{ "default", XS_TOKEN_DEFAULT },
	{ "delete", XS_TOKEN_DELETE },
	{ "do", XS_TOKEN_DO },
	{ "else", XS_TOKEN_ELSE },
	{ "enum", XS_TOKEN_ENUM },
	{ "export", XS_TOKEN_EXPORT },
	{ "extends", XS_TOKEN_EXTENDS },
	{ "false", XS_TOKEN_FALSE },
	{ "finally", XS_TOKEN_FINALLY },
	{ "for", XS_TOKEN_FOR },
	{ "function", XS_TOKEN_FUNCTION },
	{ "if", XS_TOKEN_IF },
	{ "import", XS_TOKEN_IMPORT },
	{ "in", XS_TOKEN_IN },
	{ "instanceof", XS_TOKEN_INSTANCEOF },
	{ "new", XS_TOKEN_NEW }, 
	{ "null", XS_TOKEN_NULL }, 
	{ "return", XS_TOKEN_RETURN },
	{ "super", XS_TOKEN_SUPER },
	{ "switch", XS_TOKEN_SWITCH },
	{ "this", XS_TOKEN_THIS },
	{ "throw", XS_TOKEN_THROW },
	{ "true", XS_TOKEN_TRUE },
	{ "try", XS_TOKEN_TRY },
	{ "typeof", XS_TOKEN_TYPEOF },
	{ "var", XS_TOKEN_VAR },
	{ "void", XS_TOKEN_VOID },
	{ "while", XS_TOKEN_WHILE },
	{ "with", XS_TOKEN_WITH }
};

#define XS_STRICT_KEYWORD_COUNT 9
static const txKeyword ICACHE_RODATA_ATTR gxStrictKeywords[XS_STRICT_KEYWORD_COUNT] = {
	{ "implements", XS_TOKEN_IMPLEMENTS },
	{ "interface", XS_TOKEN_INTERFACE },
	{ "let", XS_TOKEN_LET },
	{ "package", XS_TOKEN_PACKAGE },
	{ "private", XS_TOKEN_PRIVATE },
	{ "protected", XS_TOKEN_PROTECTED },
	{ "public", XS_TOKEN_PUBLIC },
	{ "static", XS_TOKEN_STATIC },
	{ "yield", XS_TOKEN_YIELD }
};

static txString fxUTF8Buffer(txParser* parser, txInteger character, txString string, txString limit)
{
	if (string + fxUTF8Length(character) > limit) {
		fxReportParserError(parser, "buffer overflow");
		fxThrowParserError(parser, parser->errorCount);
	}
	return fxUTF8Encode(string, character);
} 

void fxCheckStrictKeyword(txParser* parser)
{
	int low, high, anIndex, aDelta;
	for (low = 0, high = XS_STRICT_KEYWORD_COUNT; high > low;
			(aDelta < 0) ? (low = anIndex + 1) : (high = anIndex)) {
		anIndex = low + ((high - low) / 2);
		aDelta = c_strcmp(gxStrictKeywords[anIndex].text, parser->buffer);
		if (aDelta == 0) {
			parser->token = gxStrictKeywords[anIndex].token;
			goto bail;
		}
	}
bail:
	if (parser->escaped2)
		fxReportParserError(parser, "escaped keyword");			
}

void fxGetNextCharacter(txParser* parser)
{
	txU4 aResult;
	txUTF8Sequence const *aSequence = NULL;
	txInteger aSize;

	aResult = (txU4)(*(parser->getter))(parser->stream);
	if (aResult & 0x80) {  // According to UTF-8, aResult should be 1xxx xxxx when it is not a ASCII
		if (aResult != (txU4)C_EOF) {
			for (aSequence = gxUTF8Sequences; aSequence->size; aSequence++) {
				if ((aResult & aSequence->cmask) == aSequence->cval)
					break;
			}
			if (aSequence->size == 0) {
				fxReportParserError(parser, "invalid character %d", aResult);
				aResult = (txU4)C_EOF;
			}
			else {
				aSize = aSequence->size - 1;
				while (aSize) {
					aSize--;
					aResult = (aResult << 6) | ((*(parser->getter))(parser->stream) & 0x3F);
				}
				aResult &= aSequence->lmask;
			}
		}
		if (aResult == 0x110000)
			aResult = 0;
	}
	parser->character = aResult;
}

void fxGetNextKeyword(txParser* parser)
{
	int low, high, anIndex, aDelta;
	
	parser->symbol2 = fxNewParserSymbol(parser, parser->buffer);
	parser->token2 = XS_TOKEN_IDENTIFIER;
	if (parser->token == XS_TOKEN_DOT) 
		return;
	for (low = 0, high = XS_KEYWORD_COUNT; high > low;
			(aDelta < 0) ? (low = anIndex + 1) : (high = anIndex)) {
		anIndex = low + ((high - low) / 2);
		aDelta = c_strcmp(gxKeywords[anIndex].text, parser->buffer);
		if (aDelta == 0) {
			parser->token2 = gxKeywords[anIndex].token;
			goto bail;
		}
	}
	if ((parser->flags & mxStrictFlag)) {
		for (low = 0, high = XS_STRICT_KEYWORD_COUNT; high > low;
				(aDelta < 0) ? (low = anIndex + 1) : (high = anIndex)) {
			anIndex = low + ((high - low) / 2);
			aDelta = c_strcmp(gxStrictKeywords[anIndex].text, parser->buffer);
			if (aDelta == 0) {
				parser->token2 = gxStrictKeywords[anIndex].token;
				goto bail;
			}
		}
	}
	if (c_strcmp("await", parser->buffer) == 0) {
		if (parser->flags & mxAsyncFlag) {
			parser->token2 = XS_TOKEN_AWAIT;
			goto bail;
		}
	}	
	if (c_strcmp("yield", parser->buffer) == 0) {
		if ((parser->flags & mxGeneratorFlag)){
			parser->token2 = XS_TOKEN_YIELD;
			goto bail;
		}
	}	
	return;
bail:
	if (parser->escaped2)
		fxReportParserError(parser, "escaped keyword");			
}

txBoolean fxGetNextIdentiferX(txParser* parser, txU4* value)
{
	fxGetNextCharacter(parser);
	if (parser->character == 'u') {
		fxGetNextCharacter(parser);
		if (parser->character == '{') {
			fxGetNextCharacter(parser);
			while (fxGetNextStringX(parser->character, value)) {
				fxGetNextCharacter(parser);
			}
			if (parser->character == '}') {
				fxGetNextCharacter(parser);
				return 1;
			}
		}
		else {
			if (fxGetNextStringX(parser->character, value)) {
				fxGetNextCharacter(parser);
				if (fxGetNextStringX(parser->character, value)) {
					fxGetNextCharacter(parser);
					if (fxGetNextStringX(parser->character, value)) {
						fxGetNextCharacter(parser);
						if (fxGetNextStringX(parser->character, value)) {
							fxGetNextCharacter(parser);
							return 1;
						}		
					}		
				}		
			}		
		}		
	}
	return 0;
}

void fxGetNextNumber(txParser* parser, txNumber theNumber)
{
	parser->number2 = theNumber;
	parser->integer2 = (txInteger)parser->number2;
	theNumber = parser->integer2;
	if (parser->number2 == theNumber)
		parser->token2 = XS_TOKEN_INTEGER;
	else
		parser->token2 = XS_TOKEN_NUMBER;
}

void fxGetNextNumberB(txParser* parser)
{
	txNumber aNumber = 0;
	int c, i = 0;
	for (;;) {
		fxGetNextCharacter(parser);
		c = parser->character;
		if (('0' <= c) && (c <= '1'))
			aNumber = (aNumber * 2) + (c - '0');
		else
			break;
		i++;
	}
	if (i == 0)
		fxReportParserError(parser, "invalid number");			
	fxGetNextNumber(parser, aNumber);
}

void fxGetNextNumberE(txParser* parser, int parseDot)
{
	txString p = parser->buffer;
	if (parser->character == '-') {
		*p++ = (char)parser->character;
		fxGetNextCharacter(parser);
	}
	if (!parseDot)
		*p++ = '.';
	while (('0' <= parser->character) && (parser->character <= '9')) {
		*p++ = (char)parser->character;
		fxGetNextCharacter(parser);
	}
	if (parseDot) {
		if (parser->character == '.') {
			*p++ = (char)parser->character;
			fxGetNextCharacter(parser);
			while (('0' <= parser->character) && (parser->character <= '9')) {
				*p++ = (char)parser->character;
				fxGetNextCharacter(parser);
			}
		}
		else
			*p++ = '.';
	}
	if ((parser->character == 'e') || (parser->character == 'E')) {
		int i = 0;
		*p++ = '0';
		*p++ = (char)parser->character;
		fxGetNextCharacter(parser);
		if ((parser->character == '+') || (parser->character == '-')) {
			*p++ = (char)parser->character;
			fxGetNextCharacter(parser);
		}
		while (('0' <= parser->character) && (parser->character <= '9')) {
			*p++ = (char)parser->character;
			fxGetNextCharacter(parser);
			i++;
		}
		if (i == 0)
			fxReportParserError(parser, "invalid number");			
	}
	*p++ = 0;
	fxGetNextNumber(parser, fxStringToNumber(parser->dtoa, parser->buffer, 1));
}

void fxGetNextNumberO(txParser* parser, int c, int i)
{
	txNumber aNumber = c - '0';
	for (;;) {
		fxGetNextCharacter(parser);
		c = parser->character;
		if (('0' <= c) && (c <= '7'))
			aNumber = (aNumber * 8) + (c - '0');
		else
			break;
		i++;
	}
	if (i == 0)
		fxReportParserError(parser, "invalid number");			
	fxGetNextNumber(parser, aNumber);
}

void fxGetNextNumberX(txParser* parser)
{
	txNumber aNumber = 0;
	int c, i = 0;
	for (;;) {
		fxGetNextCharacter(parser);
		c = parser->character;
		if (('0' <= c) && (c <= '9'))
			aNumber = (aNumber * 16) + (c - '0');
		else if (('a' <= c) && (c <= 'f'))
			aNumber = (aNumber * 16) + (10 + c - 'a');
		else if (('A' <= c) && (c <= 'F'))
			aNumber = (aNumber * 16) + (10 + c - 'A');
		else
			break;
		i++;
	}
	if (i == 0)
		fxReportParserError(parser, "invalid number");			
	fxGetNextNumber(parser, aNumber);
}

void fxGetNextRegExp(txParser* parser, txU4 c)
{
	txString p;
	txString q;
	txBoolean backslash = 0;
	txBoolean bracket = 0;
    txBoolean first = 1;
    txBoolean second = 1;
	p = parser->buffer;
	q = p + parser->bufferSize - 1;
    if (!c) {
		c = parser->character;
        second = 0;
    }
	for (;;) {
		if (c == (txU4)C_EOF) {
			fxReportParserError(parser, "end of file in regular expression");			
			break;
		}
		else if ((c == 10) || (c == 13) || (c == 0x2028) || (c == 0x2029)) {
			fxReportParserError(parser, "end of line in regular expression");			
			break;
		}
		else if (c == '*') {
			if (first) {
				fxReportParserError(parser, "invalid regular expression");
				break;
			}
			backslash = 0;
		}
		else if (c == '\\') {
			if (!backslash)
				backslash = 1;
			else
				backslash = 0;
		}
		else if (c == '[') {
			if (!backslash)
				bracket = 1;
			backslash = 0;
		}
		else if (c == ']') {
			if (!backslash)
				bracket = 0;
			backslash = 0;
		}
		else if (c == '/') {
			if (!backslash && !bracket)
				break;
			backslash = 0;
		}
		else {
			backslash = 0;
		}
		p = fxUTF8Buffer(parser, c, p, q);
        if (second)
            second = 0;
        else
            fxGetNextCharacter(parser);
		c = parser->character;
		first = 0;
	}
	*p = 0;
	parser->stringLength = p - parser->buffer;
	parser->string = fxNewParserString(parser, parser->buffer, parser->stringLength);
	p = parser->buffer;
	q = p + parser->bufferSize - 1;
	for (;;) {
		fxGetNextCharacter(parser);
		if (fxIsIdentifierNext(parser->character))
			p = fxUTF8Buffer(parser, parser->character, p, q);
		else 
			break;
	}
	*p = 0;
	parser->modifierLength = p - parser->buffer;
	parser->modifier = fxNewParserString(parser, parser->buffer, parser->modifierLength);
	if (!fxCompileRegExp(C_NULL, parser->string, parser->modifier, C_NULL, C_NULL, parser->buffer, parser->bufferSize))
		fxReportParserError(parser, parser->buffer);
	parser->token = XS_TOKEN_REGEXP;
}

void fxGetNextString(txParser* parser, int c)
{
	txString p = parser->buffer;
	txString q = p + parser->bufferSize - 1;
	for (;;) {
		if (parser->character == (txU4)C_EOF) {
			fxReportParserError(parser, "end of file in string");			
			break;
		}
		else if (parser->character == 10) {
			parser->line2++;
			if (c == '`') {
				p = fxUTF8Buffer(parser, parser->character, p, q);
				fxGetNextCharacter(parser);
			}
			else {
				fxReportParserError(parser, "end of line in string");			
				break;
			}
		}
		else if (parser->character == 13) {
			parser->line2++;
			if (c == '`') {
				p = fxUTF8Buffer(parser, 10, p, q);
				fxGetNextCharacter(parser);
				if (parser->character == 10)
					fxGetNextCharacter(parser);
			}
			else {
				fxReportParserError(parser, "end of line in string");			
				break;
			}
		}
		else if ((parser->character == 0x2028) || (parser->character == 0x2029)) {
			parser->line2++;
			p = fxUTF8Buffer(parser, parser->character, p, q);
			fxGetNextCharacter(parser);
		}
		else if (parser->character == (txU4)c) {
			break;
		}
		else if (parser->character == '$') {
			fxGetNextCharacter(parser);
			if ((c == '`') && (parser->character == '{'))
				break;
			p = fxUTF8Buffer(parser, '$', p, q);
		}
		else if (parser->character == '\\') {
			parser->escaped2 = 1;
			p = fxUTF8Buffer(parser, '\\', p, q);
			fxGetNextCharacter(parser);
			switch (parser->character) {
			case 10:
			case 0x2028:
			case 0x2029:
				parser->line2++;
				p = fxUTF8Buffer(parser, parser->character, p, q);
				fxGetNextCharacter(parser);
				break;
			case 13:
				parser->line2++;
				p = fxUTF8Buffer(parser, 10, p, q);
				fxGetNextCharacter(parser);
				if (parser->character == 10)
					fxGetNextCharacter(parser);
				break;
			default:
				p = fxUTF8Buffer(parser, parser->character, p, q);
				fxGetNextCharacter(parser);
				break;
			}	
		}	
		else {
			p = fxUTF8Buffer(parser, parser->character, p, q);
			fxGetNextCharacter(parser);
		}
	}	
	*p = 0;
	if (p == q) {
		fxReportParserError(parser, "string overflow");	
		fxThrowParserError(parser, parser->errorCount);
	}
	parser->rawLength2 = p - parser->buffer;
	parser->raw2 = fxNewParserString(parser, parser->buffer, parser->rawLength2);
	if (parser->escaped2) {
		txInteger character;
		int errorCount = 0;
		txString s;
		txU1* u;
		p = parser->buffer;
		q = p + parser->bufferSize - 1;	
		s = parser->raw2;
		while (*s) {
			if (p == q) {
				fxReportParserError(parser, "buffer overflow");	
				fxThrowParserError(parser, parser->errorCount);
			}
			if (*s == '\\') {
				s++;
				switch (*s) {
				case 10:
					s++;
					break;
				case 13:
					s++;
					if (*s == 10)
						s++;
					break;
				case 'b':
					s++;
					*p++ = '\b';
					break;
				case 'f':
					s++;
					*p++ = '\f';
					break;
				case 'n':
					s++;
					*p++ = '\n';
					break;
				case 'r':
					s++;
					*p++ = '\r';
					break;
				case 't':
					s++;
					*p++ = '\t';
					break;
				case 'v':
					s++;
					*p++ = '\v';
					break;
				case 'x':
					s++;
					if (fxParseHexEscape(&s, &character))
						p = fxUTF8Buffer(parser, character, p, q);
					else
						errorCount++;
					break;
				case 'u':
					s++;
					if (fxParseUnicodeEscape(&s, &character, 1, '\\'))
						p = fxUTF8Buffer(parser, character, p, q);
					else
						errorCount++;
					break;
				case '0':
				case '1':
				case '2':
				case '3':
				case '4':
				case '5':
				case '6':
				case '7':
					character = *s++ - '0';
					if ((parser->flags & mxStrictFlag) || (c == '`')) {
						if ((character == 0) && ((*s < '0') || ('9' < *s)))
							p = fxUTF8Buffer(parser, character, p, q);
						else
							errorCount++;
					}
					else {
						if ((0 <= character) && (character <= 3)) {
							if (('0' <= *s) && (*s <= '7'))
								character = (character * 8) + *s++ - '0';
						}
						if (('0' <= *s) && (*s <= '7'))
							character = (character * 8) + *s++ - '0';
						p = fxUTF8Buffer(parser, character, p, q);
					}
					break;
				default:
					u = (txU1*)s;
					if ((u[0] == 0xE2) && (u[1] == 0x80) && ((u[2] == 0xA8) || (u[2] == 0xA9))) { /* LS || PS */
						s += 3;
					}
					else
						*p++ = *s++;
					break;
				}
			}
			else {
				*p++ = *s++;
			}
		}
		*p = 0;
		parser->stringLength2 = p - parser->buffer;
		parser->string2 = fxNewParserString(parser, parser->buffer, parser->stringLength2);
		if (errorCount > 0) {
			if (c == '`')
				parser->escaped2 |= mxStringErrorFlag;
			else
				fxReportParserError(parser, "invalid escape sequence");	
		}	
	}
	else {
		parser->stringLength2 = parser->rawLength2;
		parser->string2 = parser->raw2;
	}
}

txBoolean fxGetNextStringD(int c, txU4* value)
{
	if (('0' <= c) && (c <= '9'))
		*value = (*value * 10) + (c - '0');
	else
		return 0;
	return 1;
}

txBoolean fxGetNextStringX(int c, txU4* value)
{
	if (('0' <= c) && (c <= '9'))
		*value = (*value * 16) + (c - '0');
	else if (('a' <= c) && (c <= 'f'))
		*value = (*value * 16) + (10 + c - 'a');
	else if (('A' <= c) && (c <= 'F'))
		*value = (*value * 16) + (10 + c - 'A');
	else
		return 0;
	return 1;
}

void fxGetNextToken(txParser* parser)
{
	if (!parser->ahead)
		fxGetNextTokenAux(parser);
	parser->line = parser->line2;
	parser->crlf = parser->crlf2;
	parser->escaped = parser->escaped2;
	parser->integer = parser->integer2;
	parser->modifierLength = parser->modifierLength2;
	parser->modifier = parser->modifier2;
	parser->number = parser->number2;
	parser->rawLength = parser->rawLength2;
	parser->raw = parser->raw2;
	parser->stringLength = parser->stringLength2;
	parser->string = parser->string2;
	parser->symbol = parser->symbol2;
	parser->token = parser->token2;
	parser->ahead = 0;
}

void fxGetNextToken2(txParser* parser)
{
	if (!parser->ahead)
		fxGetNextTokenAux(parser);
	parser->ahead = 1;
}

void fxGetNextTokenAux(txParser* parser)
{
	int c;
	txString p;
	txString q;
	txU4 t = 0;
	parser->crlf2 = 0;
	parser->escaped2 = 0;
	parser->integer2 = 0;
	parser->modifierLength2 = 0;
	parser->modifier2 = parser->emptyString;
	parser->number2 = 0;
	parser->rawLength2 = 0;
	parser->raw2 = parser->emptyString;
	parser->stringLength2 = 0;
	parser->string2 = parser->emptyString;
	parser->symbol2 = C_NULL;
	parser->token2 = XS_NO_TOKEN;
	while (parser->token2 == XS_NO_TOKEN) {
		switch (parser->character) {
		case C_EOF:
			parser->token2 = XS_TOKEN_EOF;
			break;
		case 10:	
		case 0x2028: // <LS>
		case 0x2029: // <PS>	
			parser->line2++;
			fxGetNextCharacter(parser);
			parser->crlf2 = 1;
			break;
		case 13:	
			parser->line2++;
			fxGetNextCharacter(parser);
			if (parser->character == 10)
				fxGetNextCharacter(parser);
			parser->crlf2 = 1;
			break;
			
		case 11:
		case 12:
		case 160:
		case ' ':
		case '\t':
			fxGetNextCharacter(parser);
			break;
			
		case '0':
			fxGetNextCharacter(parser);
			c = parser->character;
			if (c == '.') {
				fxGetNextCharacter(parser);
				c = parser->character;
				if ((('0' <= c) && (c <= '9')) || (c == 'e') || (c == 'E'))
					fxGetNextNumberE(parser, 0);
				else {
					parser->number2 = 0;
					parser->token2 = XS_TOKEN_NUMBER;
				}
			}
			else if ((c == 'b') || (c == 'B')) {
				fxGetNextNumberB(parser);
			}
			else if ((c == 'e') || (c == 'E')) {
				fxGetNextNumberE(parser, 0);
			}
			else if ((c == 'o') || (c == 'O')) {
				fxGetNextNumberO(parser, '0', 0);
			}
			else if ((c == 'x') || (c == 'X')) {
				fxGetNextNumberX(parser);
			}
			else if (('0' <= c) && (c <= '7')) {
				if ((parser->flags & mxStrictFlag))
					fxReportParserError(parser, "octal number (strict mode)");			
				fxGetNextNumberO(parser, c, 1);
			}
			else {
				parser->integer2 = 0;
				parser->token2 = XS_TOKEN_INTEGER;
			}
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
			fxGetNextNumberE(parser, 1);
			break;
		case '.':
			fxGetNextCharacter(parser);
			if (parser->character == '.') {	
				fxGetNextCharacter(parser);
				if (parser->character == '.') {	
					parser->token2 = XS_TOKEN_SPREAD;
					fxGetNextCharacter(parser);
				}
				else {
					fxReportParserError(parser, "invalid character %d", parser->character);
				}		
			}		
			else if (('0' <= parser->character) && (parser->character <= '9'))
				fxGetNextNumberE(parser, 0);
			else
				parser->token2 = XS_TOKEN_DOT;
			break;	
		case ',':
			parser->token2 = XS_TOKEN_COMMA;
			fxGetNextCharacter(parser);
			break;	
		case ';':
			parser->token2 = XS_TOKEN_SEMICOLON;
			fxGetNextCharacter(parser);
			break;	
		case ':':
			parser->token2 = XS_TOKEN_COLON;
			fxGetNextCharacter(parser);
			break;	
		case '?':
			parser->token2 = XS_TOKEN_QUESTION_MARK;
			fxGetNextCharacter(parser);
			break;	
		case '(':
			parser->token2 = XS_TOKEN_LEFT_PARENTHESIS;
			fxGetNextCharacter(parser);
			break;	
		case ')':
			parser->token2 = XS_TOKEN_RIGHT_PARENTHESIS;
			fxGetNextCharacter(parser);
			break;	
		case '[':
			parser->token2 = XS_TOKEN_LEFT_BRACKET;
			fxGetNextCharacter(parser);
			break;	
		case ']':
			parser->token2 = XS_TOKEN_RIGHT_BRACKET;
			fxGetNextCharacter(parser);
			break;	
		case '{':
			parser->token2 = XS_TOKEN_LEFT_BRACE;
			fxGetNextCharacter(parser);
			break;	
		case '}':
			parser->token2 = XS_TOKEN_RIGHT_BRACE;
			fxGetNextCharacter(parser);
			break;	
		case '=':
			fxGetNextCharacter(parser);
			if (parser->character == '=') {			
				fxGetNextCharacter(parser);
				if (parser->character == '=') {
					parser->token2 = XS_TOKEN_STRICT_EQUAL;
					fxGetNextCharacter(parser);
				}
				else
					parser->token2 = XS_TOKEN_EQUAL;
			}
			else if (parser->character == '>') {	
				parser->token2 = XS_TOKEN_ARROW;
				fxGetNextCharacter(parser);
			}
			else	
				parser->token2 = XS_TOKEN_ASSIGN;
			break;
		case '<':
			fxGetNextCharacter(parser);
			if (parser->character == '<') {
				fxGetNextCharacter(parser);
				if (parser->character == '=') {
					parser->token2 = XS_TOKEN_LEFT_SHIFT_ASSIGN;
					fxGetNextCharacter(parser);
				}
				else
					parser->token2 = XS_TOKEN_LEFT_SHIFT;
			}
			else  if (parser->character == '=') {
				parser->token2 = XS_TOKEN_LESS_EQUAL;
				fxGetNextCharacter(parser);
			}
			else
				parser->token2 = XS_TOKEN_LESS;
			break;	
		case '>':
			fxGetNextCharacter(parser);
			if (parser->character == '>') {			
				fxGetNextCharacter(parser);
				if (parser->character == '>') {			
					fxGetNextCharacter(parser);
					if (parser->character == '=') {
						parser->token2 = XS_TOKEN_UNSIGNED_RIGHT_SHIFT_ASSIGN;
						fxGetNextCharacter(parser);
					}
					else
						parser->token2 = XS_TOKEN_UNSIGNED_RIGHT_SHIFT;
				}
				else if (parser->character == '=') {
					parser->token2 = XS_TOKEN_SIGNED_RIGHT_SHIFT_ASSIGN;
					fxGetNextCharacter(parser);
				}
				else
					parser->token2 = XS_TOKEN_SIGNED_RIGHT_SHIFT;
			}
			else if (parser->character == '=') {
				parser->token2 = XS_TOKEN_MORE_EQUAL;
				fxGetNextCharacter(parser);
			}
			else
				parser->token2 = XS_TOKEN_MORE;
			break;	
		case '!':
			fxGetNextCharacter(parser);
			if (parser->character == '=') {			
				fxGetNextCharacter(parser);
				if (parser->character == '=') {
					parser->token2 = XS_TOKEN_STRICT_NOT_EQUAL;
					fxGetNextCharacter(parser);
				}
				else
					parser->token2 = XS_TOKEN_NOT_EQUAL;
			}
			else
				parser->token2 = XS_TOKEN_NOT;
			break;
		case '~':
			parser->token2 = XS_TOKEN_BIT_NOT;
			fxGetNextCharacter(parser);
			break;
		case '&':
			fxGetNextCharacter(parser);
			if (parser->character == '=') {	
				parser->token2 = XS_TOKEN_BIT_AND_ASSIGN;
				fxGetNextCharacter(parser);
			}
			else if (parser->character == '&') {
				parser->token2 = XS_TOKEN_AND;
				fxGetNextCharacter(parser);
			}
			else
				parser->token2 = XS_TOKEN_BIT_AND;
			break;
		case '|':
			fxGetNextCharacter(parser);
			if (parser->character == '=') {
				parser->token2 = XS_TOKEN_BIT_OR_ASSIGN;
				fxGetNextCharacter(parser);
			}
			else if (parser->character == '|') {
				parser->token2 = XS_TOKEN_OR;
				fxGetNextCharacter(parser);
			}
			else
				parser->token2 = XS_TOKEN_BIT_OR;
			break;
		case '^':
			fxGetNextCharacter(parser);
			if (parser->character == '=') {
				parser->token2 = XS_TOKEN_BIT_XOR_ASSIGN;
				fxGetNextCharacter(parser);
			}
			else
				parser->token2 = XS_TOKEN_BIT_XOR;
			break;
		case '+':	
			fxGetNextCharacter(parser);
			if (parser->character == '=') {
				parser->token2 = XS_TOKEN_ADD_ASSIGN;
				fxGetNextCharacter(parser);
			}
			else if (parser->character == '+') {
				parser->token2 = XS_TOKEN_INCREMENT;
				fxGetNextCharacter(parser);
			}
			else
				parser->token2 = XS_TOKEN_ADD;
			break;
		case '-':	
			fxGetNextCharacter(parser);
			if (parser->character == '=') {
				parser->token2 = XS_TOKEN_SUBTRACT_ASSIGN;
				fxGetNextCharacter(parser);
			}
			else if (parser->character == '-') {
				parser->token2 = XS_TOKEN_DECREMENT;
				fxGetNextCharacter(parser);
			}
			else
				parser->token2 = XS_TOKEN_SUBTRACT;
			break;
		case '*':	
			fxGetNextCharacter(parser);
			if (parser->character == '=') {
				parser->token2 = XS_TOKEN_MULTIPLY_ASSIGN;
				fxGetNextCharacter(parser);
			}
			else if (parser->character == '*') {
				fxGetNextCharacter(parser);
				if (parser->character == '=') {
					parser->token2 = XS_TOKEN_EXPONENTIATION_ASSIGN;
					fxGetNextCharacter(parser);
				}
				else
					parser->token2 = XS_TOKEN_EXPONENTIATION;
			}
			else
				parser->token2 = XS_TOKEN_MULTIPLY;
			break;
		case '/':
			fxGetNextCharacter(parser);
			if (parser->character == '*') {
				fxGetNextCharacter(parser);
				for (;;) {
					if (parser->character == (txU4)C_EOF) {
						fxReportParserError(parser, "end of file in comment");			
						break;
					}
					else if (parser->character == 10) {
						parser->line2++;
						fxGetNextCharacter(parser);
					}
					else if (parser->character == 13) {
						parser->line2++;
						fxGetNextCharacter(parser);
						if (parser->character == 10)
							fxGetNextCharacter(parser);
					}
					else if (parser->character == '*') {
						fxGetNextCharacter(parser);
						if (parser->character == '/') {
							fxGetNextCharacter(parser);
							break;
						}
					}
					else
						fxGetNextCharacter(parser);
				}
			}
			else if (parser->character == '/') {
				fxGetNextCharacter(parser);
				p = parser->buffer;
				q = p + parser->bufferSize - 1;
				while ((parser->character != (txU4)C_EOF) && (parser->character != 10) && (parser->character != 13) && (parser->character != 0x2028) && (parser->character != 0x2029)) {
					if (p < q)
						*p++ = (char)parser->character;
					fxGetNextCharacter(parser);
				}	
				*p = 0;
				p = parser->buffer;
				if (parser->flags & mxDebugFlag) {
					if (!c_strncmp(p, "@line ", 6)) {
						p += 6;
						t = 0;
						c = *p++;
						while (('0' <= c) && (c <= '9')) {
							t = (t * 10) + (c - '0');
							c = *p++;
						}
						if (!t) goto bail;
						if (c == ' ') {
							c = *p++;
							if (c != '"') goto bail;
							q = p;
							c = *q++;
							while ((c != 0) && (c != 10) && (c != 13) && (c != '"'))
								c = *q++;
							if (c != '"') goto bail;
							*(--q) = 0;
							parser->path = fxNewParserSymbol(parser, p);
						}
						parser->line2 = t - 1;
					}
					else if (!c_strncmp(p, "# sourceMappingURL=", 19) || !c_strncmp(p, "@ sourceMappingURL=", 19)) {
						p += 19;
						q = p;
						c = *q++;
						while ((c != 0) && (c != 10) && (c != 13))
							c = *q++;
						*q = 0;
						parser->name = fxNewParserString(parser, p, q - p);
					}
				}
			bail:
				;
			}
			else if (parser->character == '=') {
				parser->token2 = XS_TOKEN_DIVIDE_ASSIGN;
				fxGetNextCharacter(parser);
			}
			else 
				parser->token2 = XS_TOKEN_DIVIDE;
			break;
		case '%':	
			fxGetNextCharacter(parser);
			if (parser->character == '=') {
				parser->token2 = XS_TOKEN_MODULO_ASSIGN;
				fxGetNextCharacter(parser);
			}
			else
				parser->token2 = XS_TOKEN_MODULO;
			break;
		
		case '"':
		case '\'':
			c = parser->character;
			fxGetNextCharacter(parser);
			fxGetNextString(parser, c);
			parser->token2 = XS_TOKEN_STRING;
			fxGetNextCharacter(parser);
			break;
			
		case '`':
			fxGetNextCharacter(parser);
			fxGetNextString(parser, '`');
			if (parser->character == '{')
				parser->token2 = XS_TOKEN_TEMPLATE_HEAD;
			else
				parser->token2 = XS_TOKEN_TEMPLATE;
			fxGetNextCharacter(parser);
			break;
			
		case '@':
			if (parser->flags & mxCFlag)
				parser->token2 = XS_TOKEN_HOST;
            else
                fxReportParserError(parser, "invalid character @");
            fxGetNextCharacter(parser);
			break;
			
		default:
			p = parser->buffer;
			q = p + parser->bufferSize - 1;
			if (fxIsIdentifierFirst(parser->character)) {
				p = fxUTF8Buffer(parser, parser->character, p, q);				
				fxGetNextCharacter(parser);
			}
			else if (parser->character == '\\') {
				parser->escaped2 = 1;
				t = 0;
				if (fxGetNextIdentiferX(parser, &t) && fxIsIdentifierFirst(t))
					p = fxUTF8Buffer(parser, t, p, q);				
				else
					p = C_NULL;
			}
			else
				p = C_NULL;
			if (p) {
				for (;;) {
					if (p == q) {
						fxReportParserError(parser, "identifier overflow");			
						fxThrowParserError(parser, parser->errorCount);
					}
					if (fxIsIdentifierNext(parser->character)) {
						p = fxUTF8Buffer(parser, parser->character, p, q);				
						fxGetNextCharacter(parser);
					}
					else if (parser->character == '\\') {
						parser->escaped2 = 1;
						t = 0;
						if (fxGetNextIdentiferX(parser, &t) && fxIsIdentifierNext(t))
							p = fxUTF8Buffer(parser, t, p, q);				
						else {
							p = C_NULL;
							break;
						}
					}
					else {
						*p = 0;
						fxGetNextKeyword(parser);
						break;
					}
				}
			}
			if (!p) {
				fxReportParserError(parser, "invalid character %d", parser->character);
				fxGetNextCharacter(parser);
			}
			break;
		}
	}
}

void fxGetNextTokenTemplate(txParser* parser)
{
	parser->crlf2 = 0;
	parser->escaped2 = 0;
	parser->integer2 = 0;
	parser->modifierLength2 = 0;
	parser->modifier2 = parser->emptyString;
	parser->number2 = 0;
	parser->rawLength2 = 0;
	parser->raw2 = parser->emptyString;
	parser->stringLength2 = 0;
	parser->string2 = parser->emptyString;
	parser->symbol2 = C_NULL;
	parser->token2 = XS_NO_TOKEN;
	fxGetNextString(parser, '`');
	if (parser->character == '{')
		parser->token2 = XS_TOKEN_TEMPLATE_MIDDLE;
	else
		parser->token2 = XS_TOKEN_TEMPLATE_TAIL;
	fxGetNextCharacter(parser);
	parser->line = parser->line2;
	parser->crlf = parser->crlf2;
	parser->escaped = parser->escaped2;
	parser->integer = parser->integer2;
	parser->modifierLength = parser->modifierLength2;
	parser->modifier = parser->modifier2;
	parser->number = parser->number2;
	parser->rawLength = parser->rawLength2;
	parser->raw = parser->raw2;
	parser->stringLength = parser->stringLength2;
	parser->string = parser->string2;
	parser->symbol = parser->symbol2;
	parser->token = parser->token2;
	parser->ahead = 0;
}

void fxGetNextTokenJSON(txParser* parser)
{
	int c;
	txString p;
	txString q;

	parser->line = parser->line2;
	
	parser->crlf = parser->crlf2;
	parser->escaped = parser->escaped2;
	parser->integer = parser->integer2;
	parser->modifierLength = parser->modifierLength2;
	parser->modifier = parser->modifier2;
	parser->number = parser->number2;
	parser->rawLength = parser->rawLength2;
	parser->raw = parser->raw2;
	parser->stringLength = parser->stringLength2;
	parser->string = parser->string2;
	parser->symbol = parser->symbol2;
	parser->token = parser->token2;
	
	parser->crlf2 = 0;
	parser->escaped2 = 0;
	parser->integer2 = 0;
	parser->modifierLength2 = 0;
	parser->modifier2 = parser->emptyString;
	parser->number2 = 0;
	parser->rawLength2 = 0;
	parser->raw2 = parser->emptyString;
	parser->stringLength2 = 0;
	parser->string2 = parser->emptyString;
	parser->symbol2 = C_NULL;
	parser->token2 = XS_NO_TOKEN;
	
	while (parser->token2 == XS_NO_TOKEN) {
		switch (parser->character) {
		case C_EOF:
			parser->token2 = XS_TOKEN_EOF;
			break;
		case 10:	
			parser->line2++;
			fxGetNextCharacter(parser);
			parser->crlf2 = 1;
			break;
		case 13:	
			parser->line2++;
			fxGetNextCharacter(parser);
			if (parser->character == 10)
				fxGetNextCharacter(parser);
			parser->crlf2 = 1;
			break;
			
		case '\t':
		case ' ':
			fxGetNextCharacter(parser);
			break;
			
		case '0':
			fxGetNextCharacter(parser);
			c = parser->character;
			if (c == '.') {
				fxGetNextCharacter(parser);
				c = parser->character;
				if ((('0' <= c) && (c <= '9')) || (c == 'e') || (c == 'E'))
					fxGetNextNumberE(parser, 0);
				else {
					parser->number2 = 0;
					parser->token2 = XS_TOKEN_NUMBER;
				}
			}
			else if ((c == 'b') || (c == 'B')) {
				fxGetNextNumberB(parser);
			}
			else if ((c == 'e') || (c == 'E')) {
				fxGetNextNumberE(parser, 0);
			}
			else if ((c == 'o') || (c == 'O')) {
				fxGetNextNumberO(parser, '0', 0);
			}
			else if ((c == 'x') || (c == 'X')) {
				fxGetNextNumberX(parser);
			}
			else {
				parser->integer2 = 0;
				parser->token2 = XS_TOKEN_INTEGER;
			}
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
		case '-':	
			fxGetNextNumberE(parser, 1);
			break;
		case ',':
			parser->token2 = XS_TOKEN_COMMA;
			fxGetNextCharacter(parser);
			break;	
		case ':':
			parser->token2 = XS_TOKEN_COLON;
			fxGetNextCharacter(parser);
			break;	
		case '[':
			parser->token2 = XS_TOKEN_LEFT_BRACKET;
			fxGetNextCharacter(parser);
			break;	
		case ']':
			parser->token2 = XS_TOKEN_RIGHT_BRACKET;
			fxGetNextCharacter(parser);
			break;	
		case '{':
			parser->token2 = XS_TOKEN_LEFT_BRACE;
			fxGetNextCharacter(parser);
			break;	
		case '}':
			parser->token2 = XS_TOKEN_RIGHT_BRACE;
			fxGetNextCharacter(parser);
			break;	
		case '"':
			fxGetNextCharacter(parser);
			fxGetNextString(parser, '"');
			parser->token2 = XS_TOKEN_STRING;
			fxGetNextCharacter(parser);
			break;
		default:
			if (fxIsIdentifierFirst((char)parser->character)) {
				p = parser->buffer;
				q = p + parser->bufferSize - 1;
				for (;;) {
					if (p == q) {
						fxReportParserError(parser, "identifier overflow");			
						fxThrowParserError(parser, parser->errorCount);
					}
					*p++ = (char)parser->character;
					fxGetNextCharacter(parser);
					if (!fxIsIdentifierNext((char)parser->character))
						break;
				}
				*p = 0;
				if (!c_strcmp("false", parser->buffer)) 
					parser->token2 = XS_TOKEN_FALSE;
				else if (!c_strcmp("null", parser->buffer)) 
					parser->token2 = XS_TOKEN_NULL;
				else if (!c_strcmp("true", parser->buffer)) 
					parser->token2 = XS_TOKEN_TRUE;
				else {
					parser->symbol2 = fxNewParserSymbol(parser, parser->buffer);
					parser->token2 = XS_TOKEN_IDENTIFIER;
				}
			}
			else {
				fxReportParserError(parser, "invalid character %d", parser->character);
				fxGetNextCharacter(parser);
			}
			break;
		}
	}
}

typedef struct {
	char* name;
	int value;
} txEntity;

static int fxCompareEntities(const void *name, const void *entity);

#define XS_ENTITIES_COUNT 253
static const txEntity ICACHE_RODATA_ATTR gxEntities[XS_ENTITIES_COUNT] = {
  { "AElig", 0x00C6 }, { "Aacute", 0x00C1 }, { "Acirc", 0x00C2 }, { "Agrave", 0x00C0 }, { "Alpha", 0x0391 }, { "Aring", 0x00C5 }, { "Atilde", 0x00C3 }, { "Auml", 0x00C4 },
  { "Beta", 0x0392 }, { "Ccedil", 0x00C7 }, { "Chi", 0x03A7 }, { "Dagger", 0x2021 }, { "Delta", 0x0394 }, { "ETH", 0x00D0 }, { "Eacute", 0x00C9 }, { "Ecirc", 0x00CA },
  { "Egrave", 0x00C8 }, { "Epsilon", 0x0395 }, { "Eta", 0x0397 }, { "Euml", 0x00CB }, { "Gamma", 0x0393 }, { "Iacute", 0x00CD }, { "Icirc", 0x00CE }, { "Igrave", 0x00CC },
  { "Iota", 0x0399 }, { "Iuml", 0x00CF }, { "Kappa", 0x039A }, { "Lambda", 0x039B }, { "Mu", 0x039C }, { "Ntilde", 0x00D1 }, { "Nu", 0x039D }, { "OElig", 0x0152 },
  { "Oacute", 0x00D3 }, { "Ocirc", 0x00D4 }, { "Ograve", 0x00D2 }, { "Omega", 0x03A9 }, { "Omicron", 0x039F }, { "Oslash", 0x00D8 }, { "Otilde", 0x00D5 }, { "Ouml", 0x00D6 },
  { "Phi", 0x03A6 }, { "Pi", 0x03A0 }, { "Prime", 0x2033 }, { "Psi", 0x03A8 }, { "Rho", 0x03A1 }, { "Scaron", 0x0160 }, { "Sigma", 0x03A3 }, { "THORN", 0x00DE },
  { "Tau", 0x03A4 }, { "Theta", 0x0398 }, { "Uacute", 0x00DA }, { "Ucirc", 0x00DB }, { "Ugrave", 0x00D9 }, { "Upsilon", 0x03A5 }, { "Uuml", 0x00DC }, { "Xi", 0x039E },
  { "Yacute", 0x00DD }, { "Yuml", 0x0178 }, { "Zeta", 0x0396 }, { "aacute", 0x00E1 }, { "acirc", 0x00E2 }, { "acute", 0x00B4 }, { "aelig", 0x00E6 }, { "agrave", 0x00E0 },
  { "alefsym", 0x2135 }, { "alpha", 0x03B1 }, { "amp", 0x0026 }, { "and", 0x2227 }, { "ang", 0x2220 }, { "apos", 0x0027 }, { "aring", 0x00E5 }, { "asymp", 0x2248 },
  { "atilde", 0x00E3 }, { "auml", 0x00E4 }, { "bdquo", 0x201E }, { "beta", 0x03B2 }, { "brvbar", 0x00A6 }, { "bull", 0x2022 }, { "cap", 0x2229 }, { "ccedil", 0x00E7 },
  { "cedil", 0x00B8 }, { "cent", 0x00A2 }, { "chi", 0x03C7 }, { "circ", 0x02C6 }, { "clubs", 0x2663 }, { "cong", 0x2245 }, { "copy", 0x00A9 }, { "crarr", 0x21B5 },
  { "cup", 0x222A }, { "curren", 0x00A4 }, { "dArr", 0x21D3 }, { "dagger", 0x2020 }, { "darr", 0x2193 }, { "deg", 0x00B0 }, { "delta", 0x03B4 }, { "diams", 0x2666 },
  { "divide", 0x00F7 }, { "eacute", 0x00E9 }, { "ecirc", 0x00EA }, { "egrave", 0x00E8 }, { "empty", 0x2205 }, { "emsp", 0x2003 }, { "ensp", 0x2002 }, { "epsilon", 0x03B5 },
  { "equiv", 0x2261 }, { "eta", 0x03B7 }, { "eth", 0x00F0 }, { "euml", 0x00EB }, { "euro", 0x20AC }, { "exist", 0x2203 }, { "fnof", 0x0192 }, { "forall", 0x2200 },
  { "frac12", 0x00BD }, { "frac14", 0x00BC }, { "frac34", 0x00BE }, { "frasl", 0x2044 }, { "gamma", 0x03B3 }, { "ge", 0x2265 }, { "gt", 0x003E }, { "hArr", 0x21D4 },
  { "harr", 0x2194 }, { "hearts", 0x2665 }, { "hellip", 0x2026 }, { "iacute", 0x00ED }, { "icirc", 0x00EE }, { "iexcl", 0x00A1 }, { "igrave", 0x00EC }, { "image", 0x2111 },
  { "infin", 0x221E }, { "int", 0x222B }, { "iota", 0x03B9 }, { "iquest", 0x00BF }, { "isin", 0x2208 }, { "iuml", 0x00EF }, { "kappa", 0x03BA }, { "lArr", 0x21D0 },
  { "lambda", 0x03BB }, { "lang", 0x2329 }, { "laquo", 0x00AB }, { "larr", 0x2190 }, { "lceil", 0x2308 }, { "ldquo", 0x201C }, { "le", 0x2264 }, { "lfloor", 0x230A },
  { "lowast", 0x2217 }, { "loz", 0x25CA }, { "lrm", 0x200E }, { "lsaquo", 0x2039 }, { "lsquo", 0x2018 }, { "lt", 0x003C }, { "macr", 0x00AF }, { "mdash", 0x2014 },
  { "micro", 0x00B5 }, { "middot", 0x00B7 }, { "minus", 0x2212 }, { "mu", 0x03BC }, { "nabla", 0x2207 }, { "nbsp", 0x00A0 }, { "ndash", 0x2013 }, { "ne", 0x2260 },
  { "ni", 0x220B }, { "not", 0x00AC }, { "notin", 0x2209 }, { "nsub", 0x2284 }, { "ntilde", 0x00F1 }, { "nu", 0x03BD }, { "oacute", 0x00F3 }, { "ocirc", 0x00F4 },
  { "oelig", 0x0153 }, { "ograve", 0x00F2 }, { "oline", 0x203E }, { "omega", 0x03C9 }, { "omicron", 0x03BF }, { "oplus", 0x2295 }, { "or", 0x2228 }, { "ordf", 0x00AA },
  { "ordm", 0x00BA }, { "oslash", 0x00F8 }, { "otilde", 0x00F5 }, { "otimes", 0x2297 }, { "ouml", 0x00F6 }, { "para", 0x00B6 }, { "part", 0x2202 }, { "permil", 0x2030 },
  { "perp", 0x22A5 }, { "phi", 0x03C6 }, { "pi", 0x03C0 }, { "piv", 0x03D6 }, { "plusmn", 0x00B1 }, { "pound", 0x00A3 }, { "prime", 0x2032 }, { "prod", 0x220F },
  { "prop", 0x221D }, { "psi", 0x03C8 }, { "quot", 0x0022 }, { "rArr", 0x21D2 }, { "radic", 0x221A }, { "rang", 0x232A }, { "raquo", 0x00BB }, { "rarr", 0x2192 },
  { "rceil", 0x2309 }, { "rdquo", 0x201D }, { "real", 0x211C }, { "reg", 0x00AE }, { "rfloor", 0x230B }, { "rho", 0x03C1 }, { "rlm", 0x200F }, { "rsaquo", 0x203A },
  { "rsquo", 0x2019 }, { "sbquo", 0x201A }, { "scaron", 0x0161 }, { "sdot", 0x22C5 }, { "sect", 0x00A7 }, { "shy", 0x00AD }, { "sigma", 0x03C3 }, { "sigmaf", 0x03C2 },
  { "sim", 0x223C }, { "spades", 0x2660 }, { "sub", 0x2282 }, { "sube", 0x2286 }, { "sum", 0x2211 }, { "sup", 0x2283 }, { "sup1", 0x00B9 }, { "sup2", 0x00B2 },
  { "sup3", 0x00B3 }, { "supe", 0x2287 }, { "szlig", 0x00DF }, { "tau", 0x03C4 }, { "there4", 0x2234 }, { "theta", 0x03B8 }, { "thetasym", 0x03D1 }, { "thinsp", 0x2009 },
  { "thorn", 0x00FE }, { "tilde", 0x02DC }, { "times", 0x00D7 }, { "trade", 0x2122 }, { "uArr", 0x21D1 }, { "uacute", 0x00FA }, { "uarr", 0x2191 }, { "ucirc", 0x00FB },
  { "ugrave", 0x00F9 }, { "uml", 0x00A8 }, { "upsih", 0x03D2 }, { "upsilon", 0x03C5 }, { "uuml", 0x00FC }, { "weierp", 0x2118 }, { "xi", 0x03BE }, { "yacute", 0x00FD },
  { "yen", 0x00A5 }, { "yuml", 0x00FF }, { "zeta", 0x03B6 }, { "zwj", 0x200D }, { "zwnj", 0x200C },
};

int fxCompareEntities(const void *name, const void *entity)
{
	return c_strcmp((char*)name, ((txEntity*)entity)->name);
}

txString fxGetNextEntity(txParser* parser, txString p, txString q)
{
	txString r = p;
	txU4 t = 0;
	*p++ = '&';
	fxGetNextCharacter(parser);
	if (parser->character == '#') {
		*p++ = '#';
		fxGetNextCharacter(parser);
		if (parser->character == 'x') {
			*p++ = 'x';
			fxGetNextCharacter(parser);
			while (fxGetNextStringX(parser->character, &t)) {
				*p++ = parser->character;
				fxGetNextCharacter(parser);
			}
		}
		else {
			while (fxGetNextStringD(parser->character, &t)) {
				*p++ = parser->character;
				fxGetNextCharacter(parser);
			}
		}
	}
	else {
		txEntity* entity = C_NULL;
		int c = parser->character;
		while ((('0' <= c) && (c <= '9')) || (('A' <= c) && (c <= 'Z')) || (('a' <= c) && (c <= 'z'))) {
			*p++ = c;
			fxGetNextCharacter(parser);
			c = parser->character;
		}
		*p = 0;
		entity = (txEntity*)bsearch(r + 1, gxEntities, XS_ENTITIES_COUNT, sizeof(txEntity), fxCompareEntities);
		t = entity ? entity->value : 0;
	}
	if (parser->character == ';') {
		*p++ = ';';
		fxGetNextCharacter(parser);
		if (t)
			p = fxUTF8Buffer(parser, t, r, q);
	}
	return p;
}

void fxGetNextTokenJSXAttribute(txParser* parser)
{
	txString p = parser->buffer;
	txString q = p + parser->bufferSize - 1;
	txU4 quote = 0;
	parser->crlf2 = 0;
	parser->escaped2 = 0;
	parser->integer2 = 0;
	parser->modifierLength2 = 0;
	parser->modifier2 = parser->emptyString;
	parser->number2 = 0;
	parser->rawLength2 = 0;
	parser->raw2 = parser->emptyString;
	parser->stringLength2 = 0;
	parser->string2 = parser->emptyString;
	parser->symbol2 = C_NULL;
	parser->token2 = XS_NO_TOKEN;
	while (parser->token2 == XS_NO_TOKEN) {
		switch (parser->character) {
		case C_EOF:
			parser->token2 = XS_TOKEN_EOF;
			break;
		case 10:	
		case 0x2028: // <LS>
		case 0x2029: // <PS>	
			parser->line2++;
			if (quote)
				p = fxUTF8Buffer(parser, parser->character, p, q);
			fxGetNextCharacter(parser);
			parser->crlf2 = 1;
			break;
		case 13:	
			parser->line2++;
			if (quote)
				p = fxUTF8Buffer(parser, parser->character, p, q);
			fxGetNextCharacter(parser);
			if (parser->character == 10) {
				if (quote)
					p = fxUTF8Buffer(parser, parser->character, p, q);
				fxGetNextCharacter(parser);
			}
			parser->crlf2 = 1;
			break;
		case 11:
		case 12:
		case 160:
		case ' ':
		case '\t':
			if (quote)
				p = fxUTF8Buffer(parser, parser->character, p, q);
			fxGetNextCharacter(parser);
			break;
			
		case '{':
			if (quote)
				p = fxUTF8Buffer(parser, parser->character, p, q);
			else
				parser->token2 = XS_TOKEN_LEFT_BRACE;
			fxGetNextCharacter(parser);
			break;
			
		case '"':
		case '\'':
			if (quote) {
				if (quote == parser->character) {
					parser->stringLength2 = p - parser->buffer;
					parser->string2 = fxNewParserString(parser, parser->buffer, parser->stringLength2);
					parser->token2 = XS_TOKEN_STRING;
				}
				else
					p = fxUTF8Buffer(parser, parser->character, p, q);
			}
			else
				quote = parser->character;
			fxGetNextCharacter(parser);
			break;
			
		case '&':
			if (quote)
				p = fxGetNextEntity(parser, p, q);
			else {
				fxReportParserError(parser, "invalid character %d", parser->character);
				fxGetNextCharacter(parser);
			}
			break;
			
		default:
			if (quote)
				p = fxUTF8Buffer(parser, parser->character, p, q);
			else
				fxReportParserError(parser, "invalid character %d", parser->character);
			fxGetNextCharacter(parser);
			break;	
		}	
	}
	parser->line = parser->line2;
	parser->crlf = parser->crlf2;
	parser->escaped = parser->escaped2;
	parser->integer = parser->integer2;
	parser->modifierLength = parser->modifierLength2;
	parser->modifier = parser->modifier2;
	parser->number = parser->number2;
	parser->rawLength = parser->rawLength2;
	parser->raw = parser->raw2;
	parser->stringLength = parser->stringLength2;
	parser->string = parser->string2;
	parser->symbol = parser->symbol2;
	parser->token = parser->token2;
	parser->ahead = 0;
}

void fxGetNextTokenJSXChild(txParser* parser)
{
	txString p = parser->buffer;
	txString q = p + parser->bufferSize - 1;
	txString s = C_NULL;
	char c;
	txString after = C_NULL;
	txString before = C_NULL;
	txString text = C_NULL;
	parser->crlf2 = 0;
	parser->escaped2 = 0;
	parser->integer2 = 0;
	parser->modifierLength2 = 0;
	parser->modifier2 = parser->emptyString;
	parser->number2 = 0;
	parser->rawLength2 = 0;
	parser->raw2 = parser->emptyString;
	parser->stringLength2 = 0;
	parser->string2 = parser->emptyString;
	parser->symbol2 = C_NULL;
	parser->token2 = XS_NO_TOKEN;
	while (parser->token2 == XS_NO_TOKEN) {
		switch (parser->character) {
		case C_EOF:
			parser->token2 = XS_TOKEN_EOF;
			break;
		case 10:	
		case 0x2028: // <LS>
		case 0x2029: // <PS>	
			parser->line2++;
			p = fxUTF8Buffer(parser, parser->character, p, q);
			fxGetNextCharacter(parser);
			parser->crlf2 = 1;
			break;
		case 13:	
			parser->line2++;
			p = fxUTF8Buffer(parser, parser->character, p, q);
			fxGetNextCharacter(parser);
			if (parser->character == 10) {
				p = fxUTF8Buffer(parser, parser->character, p, q);
				fxGetNextCharacter(parser);
			}
			parser->crlf2 = 1;
			break;
			
		case '<':
			fxGetNextCharacter(parser);
			parser->token2 = XS_TOKEN_LESS;
			break;
		case '>':
			fxGetNextCharacter(parser);
			parser->token2 = XS_TOKEN_MORE;
			break;
		case '{':
			fxGetNextCharacter(parser);
			parser->token2 = XS_TOKEN_LEFT_BRACE;
			break;
		case '}':
			fxGetNextCharacter(parser);
			parser->token2 = XS_TOKEN_RIGHT_BRACE;
			break;
		case '&':
			p = fxGetNextEntity(parser, p, q);
			break;
		default:
			p = fxUTF8Buffer(parser, parser->character, p, q);
			fxGetNextCharacter(parser);
			break;	
		}	
	}
	parser->rawLength2 = p - parser->buffer;
	parser->raw2 = fxNewParserString(parser, parser->buffer, parser->rawLength2);
	if (parser->crlf2) {
		p = parser->buffer;
		q = p + parser->bufferSize - 1;
		s = parser->raw2;
		c = *s++;
		while (c) {
			if ((c == 10) || (c == 13)) {
				if (before)
					p = before;
				else
					before = p;
				after = p;
			}
			else if ((c == 9) || (c == 32)) {
				if (!before)
					before = p;
				*p++ = c;
			}
			else {
				if (after) {
					p = after;
					if (text)
						*p++ = 32;
				}
				after = C_NULL;
				before = C_NULL;
				text = p;
				*p++ = c;
			}
			c = *s++;
		}
		if (after)
			p = after;
	}
	parser->stringLength2 = p - parser->buffer;
	parser->string2 = fxNewParserString(parser, parser->buffer, parser->stringLength2);
	
	parser->line = parser->line2;
	parser->crlf = parser->crlf2;
	parser->escaped = parser->escaped2;
	parser->integer = parser->integer2;
	parser->modifierLength = parser->modifierLength2;
	parser->modifier = parser->modifier2;
	parser->number = parser->number2;
	parser->rawLength = parser->rawLength2;
	parser->raw = parser->raw2;
	parser->stringLength = parser->stringLength2;
	parser->string = parser->string2;
	parser->symbol = parser->symbol2;
	parser->token = parser->token2;
	parser->ahead = 0;
}
















