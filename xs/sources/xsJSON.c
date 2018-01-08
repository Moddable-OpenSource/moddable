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

#include "xsAll.h"

enum {
	XS_NO_JSON_TOKEN,
	XS_JSON_TOKEN_COLON,
	XS_JSON_TOKEN_COMMA,
	XS_JSON_TOKEN_EOF,
	XS_JSON_TOKEN_FALSE,
	XS_JSON_TOKEN_INTEGER,
	XS_JSON_TOKEN_LEFT_BRACE,
	XS_JSON_TOKEN_LEFT_BRACKET,
	XS_JSON_TOKEN_NULL,
	XS_JSON_TOKEN_NUMBER,
	XS_JSON_TOKEN_RIGHT_BRACE,
	XS_JSON_TOKEN_RIGHT_BRACKET,
	XS_JSON_TOKEN_STRING,
	XS_JSON_TOKEN_TRUE,
};

typedef struct {
	char text[30];
	txInteger token;
} txJSONKeyword;

typedef struct {
	txSlot* slot;
	txString data;
	txSize offset;
	txSize size;
	txInteger integer;
	txNumber number;
	txSlot* string;
	txInteger token;
	txSlot* keys;
	txInteger line;
} txJSONParser;

typedef struct {
	txString buffer;
	char indent[16];
	txInteger level;
	txSize offset;
	txSize size;
	txSlot* replacer;
	txSlot* keys;
	txSlot* stack;
} txJSONSerializer;

static void fxGetNextJSONToken(txMachine* the, txJSONParser* theParser);
static txBoolean fxGetNextJSONX(txU1 c, txU4* value);
static void fxParseJSON(txMachine* the, txJSONParser* theParser);
static void fxParseJSONValue(txMachine* the, txJSONParser* theParser);
static void fxParseJSONObject(txMachine* the, txJSONParser* theParser);
static void fxParseJSONArray(txMachine* the, txJSONParser* theParser);
static void fxReviveJSON(txMachine* the, txSlot* reviver, txSlot* holder);

static void fxSerializeJSON(txMachine* the, txJSONSerializer* theSerializer);
static void fxSerializeJSONChar(txMachine* the, txJSONSerializer* theSerializer, char c);
static void fxSerializeJSONChars(txMachine* the, txJSONSerializer* theSerializer, char* s);
static void fxSerializeJSONIndent(txMachine* the, txJSONSerializer* theSerializer);
static void fxSerializeJSONInteger(txMachine* the, txJSONSerializer* theSerializer, txInteger theInteger);
static void fxSerializeJSONName(txMachine* the, txJSONSerializer* theSerializer, txInteger* theFlag);
static void fxSerializeJSONNumber(txMachine* the, txJSONSerializer* theSerializer, txNumber theNumber);
static void fxSerializeJSONProperty(txMachine* the, txJSONSerializer* theSerializer, txInteger* theFlag);
static void fxSerializeJSONString(txMachine* the, txJSONSerializer* theStream, txString theString);
static void fxSerializeJSONStringEscape(txMachine* the, txJSONSerializer* theStream, txU4 character);

static txSlot* fxToJSONKeys(txMachine* the, txSlot* reference);

void fxBuildJSON(txMachine* the)
{
	txSlot* slot;
	mxPush(mxObjectPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_JSON_parse), 2, mxID(_parse), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_JSON_stringify), 3, mxID(_stringify), XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "JSON", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	slot = fxGlobalSetProperty(the, mxGlobal.value.reference, mxID(_JSON), XS_NO_ID, XS_OWN);
	slot->flag = XS_DONT_ENUM_FLAG;
	slot->kind = the->stack->kind;
	slot->value = the->stack->value;
	the->stack++;
}

void fx_JSON_parse(txMachine* the)
{
	volatile txJSONParser* aParser = C_NULL;
	txSlot* slot;

	mxTry(the) {
		if (mxArgc < 1)
			mxSyntaxError("no buffer");
		aParser = c_malloc(sizeof(txJSONParser));
		if (NULL == aParser)
			mxUnknownError("out of memory");
		c_memset((txJSONParser*)aParser, 0, sizeof(txJSONParser));
// 		if ((mxArgc > 2) && mxIsReference(mxArgv(2)))
// 			aParser->keys = mxArgv(2);
// 
		if ((mxArgc > 1) && mxIsReference(mxArgv(1)) && fxIsArray(the, mxArgv(1)->value.reference))
			aParser->keys = fxToJSONKeys(the, mxArgv(1));

		slot = mxArgv(0);
		if (slot->kind == XS_REFERENCE_KIND) {
			slot = slot->value.reference->next;
			if (slot && (slot->flag & XS_INTERNAL_FLAG) && (slot->kind == XS_HOST_KIND)) {
				aParser->data = slot->value.host.data;
				aParser->offset = 0;
				mxPushSlot(mxArgv(0));
				fxGetID(the, mxID(_length));
				aParser->size = fxToInteger(the, the->stack++);
			}
		}
		if (!aParser->data) {
			fxToString(the, mxArgv(0));
			aParser->slot = mxArgv(0);
			aParser->offset = 0;
			aParser->size = c_strlen(aParser->slot->value.string);
		}
		fxParseJSON(the, (txJSONParser*)aParser);
		mxPullSlot(mxResult);
		if (aParser->keys)
			mxPop();
		c_free((txJSONParser*)aParser);
        aParser = C_NULL;
		if ((mxArgc > 1) && mxIsReference(mxArgv(1)) && mxIsCallable(mxArgv(1)->value.reference)) {
			txSlot* instance;
			txID id;
			mxPush(mxObjectPrototype);
			instance = fxNewObjectInstance(the);
			id = fxID(the, "");
			mxBehaviorDefineOwnProperty(the, instance, id, XS_NO_ID, mxResult, XS_GET_ONLY);
			mxPushUndefined();
			fxKeyAt(the, id, XS_NO_ID, the->stack);
			mxPushSlot(mxResult);
			fxReviveJSON(the, mxArgv(1), instance);
			mxPullSlot(mxResult);
		}
	}
	mxCatch(the) {
		if (aParser)
			c_free((txJSONParser*)aParser);
		fxJump(the);
	}
}

void fxGetNextJSONToken(txMachine* the, txJSONParser* theParser)
{
	txU1* p;
	txU1* q;
	txU1* r;
	txU1* s;
	txU1 c;
	txInteger i;
	txBoolean escaped;
	txNumber number;
	txU4 size;
	txU4 value;
	const txUTF8Sequence* sequence;
	txU1* string;

	theParser->integer = 0;
	theParser->number = 0;
	theParser->string->value.string = mxEmptyString.value.string;
	theParser->token = XS_NO_JSON_TOKEN;
	r = (theParser->data) ? (txU1*)theParser->data : (txU1*)theParser->slot->value.string;
	p = r + theParser->offset;
	q = r + theParser->size;
	c = (p < q) ? *p : 0;
	while (theParser->token == XS_NO_JSON_TOKEN) {
		switch (c) {
		case 0:
			theParser->token = XS_JSON_TOKEN_EOF;
			break;
		case 10:
			c = (++p < q) ? *p : 0;
			theParser->line++;
			break;
		case 13:
			c = (++p < q) ? *p : 0;
			if (c != 10)
				theParser->line++;
			break;
		
		case '\t':
		case ' ':
			c = (++p < q) ? *p : 0;
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
			s = p;
			if (c == '-')
				c = (++p < q) ? *p : 0;
			if (('0' <= c) && (c <= '9')) {
				if (c == '0') {
					c = (++p < q) ? *p : 0;
				}
				else {
					c = (++p < q) ? *p : 0;
					while (('0' <= c) && (c <= '9')) {
						c = (++p < q) ? *p : 0;
					}
				}
				if (c == '.') {
					c = (++p < q) ? *p : 0;
					if (('0' <= c) && (c <= '9')) {
						c = (++p < q) ? *p : 0;
						while (('0' <= c) && (c <= '9')) {
							c = (++p < q) ? *p : 0;
						}
					}
					else
						mxSyntaxError("%ld: invalid character in number", theParser->line);
				}
				if ((c == 'e') || (c == 'E')) {
					c = (++p < q) ? *p : 0;
					if ((c== '+') || (c == '-')) {
						c = (++p < q) ? *p : 0;
					}
					if (('0' <= c) && (c <= '9')) {
						c = (++p < q) ? *p : 0;
						while (('0' <= c) && (c <= '9')) {
							c = (++p < q) ? *p : 0;
						}
					}
					else
						mxSyntaxError("%ld: invalid character in number", theParser->line);
				}
				size = p - s;
				if ((size + 1) > sizeof(the->nameBuffer))
					mxSyntaxError("%ld: number overflow", theParser->line);
				c_memcpy(the->nameBuffer, s, size);
				the->nameBuffer[size] = 0;
				theParser->number = fxStringToNumber(the->dtoa, the->nameBuffer, 0);
				theParser->integer = (txInteger)theParser->number;
				number = theParser->integer;
				if (theParser->number == number)
					theParser->token = XS_JSON_TOKEN_INTEGER;
				else
					theParser->token = XS_JSON_TOKEN_NUMBER;
			}
			else
				mxSyntaxError("%ld: invalid character in number", theParser->line);
			break;
		case ',':
			p++;
			theParser->token = XS_JSON_TOKEN_COMMA;
			break;	
		case ':':
			p++;
			theParser->token = XS_JSON_TOKEN_COLON;
			break;	
		case '[':
			p++;
			theParser->token = XS_JSON_TOKEN_LEFT_BRACKET;
			break;	
		case ']':
			p++;
			theParser->token = XS_JSON_TOKEN_RIGHT_BRACKET;
			break;	
		case '{':
			p++;
			theParser->token = XS_JSON_TOKEN_LEFT_BRACE;
			break;	
		case '}':
			p++;
			theParser->token = XS_JSON_TOKEN_RIGHT_BRACE;
			break;	
		case '"':
			c = (++p < q) ? *p : 0;
			s = p;
			escaped = 0;
			size = 0;
			for (;;) {
				if (((p + 3) < q) && (c == 0xF4) && (p[1] == 0x90) && (p[2] == 0x80) && (p[3] == 0x80)) { // 0x110000
					mxSyntaxError("%ld: invalid character in string", theParser->line);				
					break;
				}
				else if ((0 <= c) && (c < 32)) {
					mxSyntaxError("%ld: invalid character in string", theParser->line);				
					break;
				}
				else if (c == '"') {
					break;
				}
				else if (c == '\\') {
					escaped = 1;
					c = (++p < q) ? *p : 0;
					switch (c) {
					case '"':
					case '/':
					case '\\':
					case 'b':
					case 'f':
					case 'n':
					case 'r':
					case 't':
						size++;
						c = (++p < q) ? *p : 0;
						break;
					case 'u':
						value = 0;
						for (i = 0; i < 4; i++) {
							c = (++p < q) ? *p : 0;
							if (!fxGetNextJSONX(c, &value))
								mxSyntaxError("%ld: invalid character in string", theParser->line);
						}
						if ((0x0000D800 <= value) && (value <= 0x0000DBFF) && ((p + 1) < q) && (p[0] == '\\') && (p[1] == 'u')) {
							txU4 surrogate = 0;
							txU1* t = p;
							p++;
							for (i = 0; i < 4; i++) {
								c = (++p < q) ? *p : 0;
								if (!fxGetNextJSONX(c, &surrogate))
									mxSyntaxError("%ld: invalid character in string", theParser->line);
							}
							if ((0x0000DC00 <= surrogate) && (surrogate <= 0x0000DFFF))
								value = 0x00010000 + ((value & 0x03FF) << 10) + (surrogate & 0x03FF);
							else
								p = t;
						}
						if (value == 0)
							value = 0x110000;
						for (sequence = gxUTF8Sequences; sequence->size; sequence++)
							if (value <= sequence->lmask)
								break;
						size += sequence->size;
						c = (++p < q) ? *p : 0;
						break;
					default:
						mxSyntaxError("%ld: invalid character in string", theParser->line);
						break;
					}
				}
				else {
					size++;
					c = (++p < q) ? *p : 0;
				}
			}
			{
				txSize after = p - r;
				txSize before = s - r;
				theParser->string->value.string = (txString)fxNewChunk(the, size + 1);
				string = (txU1*)theParser->string->value.string;
				r = (theParser->data) ? (txU1*)theParser->data : (txU1*)theParser->slot->value.string;
				p = r + after;
				q = r + theParser->size;
				s = r + before;
			}
			if (escaped) {
				p = s;
				c = *p;
				for (;;) {
					if (c == '"') {
						break;
					}
					else if (c == '\\') {
						p++; c = *p;
						switch (c) {
						case '"':
						case '/':
						case '\\':
							*string++ = c;
							p++; c = *p;
							break;
						case 'b':
							*string++ = '\b';
							p++; c = *p;
							break;
						case 'f':
							*string++ = '\f';
							p++; c = *p;
							break;
						case 'n':
							*string++ = '\n';
							p++; c = *p;
							break;
						case 'r':
							*string++ = '\r';
							p++; c = *p;
							break;
						case 't':
							*string++ = '\t';
							p++; c = *p;
							break;
						case 'u':
							value = 0;
							for (i = 0; i < 4; i++) {
								p++; c = *p;
								fxGetNextJSONX(c, &value);
							}
							p++;
							if ((0x0000D800 <= value) && (value <= 0x0000DBFF) && ((p + 1) < q) && (p[0] == '\\') && (p[1] == 'u')) {
								txU4 surrogate = 0;
								txU1* t = p;
								p++;
								for (i = 0; i < 4; i++) {
									p++; c = *p;
									fxGetNextJSONX(c, &surrogate);
								}
								p++;
								if ((0x0000DC00 <= surrogate) && (surrogate <= 0x0000DFFF))
									value = 0x00010000 + ((value & 0x03FF) << 10) + (surrogate & 0x03FF);
								else
									p = t;
							}
							c = *p;
							string = fsX2UTF8(value, (txU1*)string, 0x7FFFFFFF);
							break;
						}
					}
					else {
						*string++ = c;
						p++; c = *p;
					}
				}
				*string = 0;
			}
			else {
				c_memcpy(string, s, size);
				string[size] = 0;
			}
			p++;
			theParser->token = XS_JSON_TOKEN_STRING;
			break;
		default:
			if ((q - p >= 5) && (!c_strncmp((txString)p, "false", 5))) {
				p += 5;
				theParser->token = XS_JSON_TOKEN_FALSE;
			}
			else if ((q - p >= 4) && (!c_strncmp((txString)p, "null", 4))) {
				p += 4;
				theParser->token = XS_JSON_TOKEN_NULL;
			}
			else if ((q - p >= 4) && (!c_strncmp((txString)p, "true", 4))) {
				p += 4;
				theParser->token = XS_JSON_TOKEN_TRUE;
			}
			else
				mxSyntaxError("%ld: invalid character", theParser->line);	
			break;
		}
	}
	theParser->offset = p - r;
}

txBoolean fxGetNextJSONX(txU1 c, txU4* value)
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

void fxParseJSON(txMachine* the, txJSONParser* theParser)
{
	mxPush(mxEmptyString);
	theParser->string = the->stack;
	theParser->line = 1;
	fxGetNextJSONToken(the, theParser);
	fxParseJSONValue(the, theParser);
	if (theParser->token != XS_JSON_TOKEN_EOF)
		mxSyntaxError("%ld: missing EOF", theParser->line);
}

void fxParseJSONValue(txMachine* the, txJSONParser* theParser)
{
	switch (theParser->token) {
	case XS_JSON_TOKEN_FALSE:
		mxPushBoolean(0);
		fxGetNextJSONToken(the, theParser);
		break;
	case XS_JSON_TOKEN_TRUE:
		mxPushBoolean(1);
		fxGetNextJSONToken(the, theParser);
		break;
	case XS_JSON_TOKEN_NULL:
		mxPushNull();
		fxGetNextJSONToken(the, theParser);
		break;
	case XS_JSON_TOKEN_INTEGER:
		mxPushInteger(theParser->integer);
		fxGetNextJSONToken(the, theParser);
		break;
	case XS_JSON_TOKEN_NUMBER:
		mxPushNumber(theParser->number);
		fxGetNextJSONToken(the, theParser);
		break;
	case XS_JSON_TOKEN_STRING:
		mxPushString(theParser->string->value.string);
		fxGetNextJSONToken(the, theParser);
		break;
	case XS_JSON_TOKEN_LEFT_BRACE:
		fxParseJSONObject(the, theParser);
		break;
	case XS_JSON_TOKEN_LEFT_BRACKET:
		fxParseJSONArray(the, theParser);
		break;
	default:
		mxPushUndefined();
		mxSyntaxError("%ld: invalid value", theParser->line);
		break;
	}
}

void fxParseJSONObject(txMachine* the, txJSONParser* theParser)
{
	txSlot* anObject;
	txSlot* at;
	txIndex index;
	txID id;
	txSlot* aProperty;

	fxGetNextJSONToken(the, theParser);
	mxPush(mxObjectPrototype);
	anObject = fxNewObjectInstance(the);
	for (;;) {
		if (theParser->token == XS_JSON_TOKEN_RIGHT_BRACE)
			break;
		if (theParser->token != XS_JSON_TOKEN_STRING) {
			mxSyntaxError("%ld: missing name", theParser->line);
			break;
		}
		mxPushString(theParser->string->value.string);
		at = the->stack;
		index = XS_NO_ID;
		if (theParser->keys) {
			at->kind = XS_UNDEFINED_KIND;
			if (fxStringToIndex(the->dtoa, at->value.string, &index))
				id = 0;
			else {
				id = fxFindName(the, at->value.string);
				if (!id) id = XS_NO_ID;
			}
			if (id != XS_NO_ID) {
				txSlot* item = theParser->keys->value.reference->next;
				while (item) {
					if ((item->value.at.id == id) && (item->value.at.index == index)) {
						at->value.at.id = id;
						at->value.at.index = index;
						at->kind = XS_AT_KIND;
						break;
					}
					item = item->next;
				}
			}
		}
		else {
			if (fxStringToIndex(the->dtoa, at->value.string, &index))
				id = 0;
			else
				id = fxNewName(the, at);
			at->value.at.id = id;
            at->value.at.index = index;
			at->kind = XS_AT_KIND;
		}
		fxGetNextJSONToken(the, theParser);
		if (theParser->token != XS_JSON_TOKEN_COLON) {
			mxSyntaxError("%ld: missing :", theParser->line);
			break;
		}
		fxGetNextJSONToken(the, theParser);
		fxParseJSONValue(the, theParser);
		if ((at->kind == XS_AT_KIND) && (the->stack->kind != XS_UNDEFINED_KIND)) {
			aProperty = mxBehaviorSetProperty(the, anObject, at->value.at.id, at->value.at.index, XS_OWN);
			aProperty->kind = the->stack->kind;
			aProperty->value = the->stack->value;
		}
		the->stack++;
		the->stack++;
		if (theParser->token != XS_JSON_TOKEN_COMMA)
			break;
		fxGetNextJSONToken(the, theParser);
	}
	if (theParser->token != XS_JSON_TOKEN_RIGHT_BRACE)
		mxSyntaxError("%ld: missing }", theParser->line);
	fxGetNextJSONToken(the, theParser);
}

void fxParseJSONArray(txMachine* the, txJSONParser* theParser)
{
	txSlot* anArray;
	txIndex aLength;
	txSlot* anItem;

	fxGetNextJSONToken(the, theParser);
	mxPush(mxArrayPrototype);
	anArray = fxNewArrayInstance(the);
	aLength = 0;
	anItem = fxLastProperty(the, anArray);
	for (;;) {
		if (theParser->token == XS_JSON_TOKEN_RIGHT_BRACKET)
			break;
		fxParseJSONValue(the, theParser);
		aLength++;
		anItem->next = fxNewSlot(the);
		anItem = anItem->next;
		anItem->kind = the->stack->kind;
		anItem->value = the->stack->value;
		the->stack++;
		if (theParser->token != XS_JSON_TOKEN_COMMA)
			break;
		fxGetNextJSONToken(the, theParser);
	}
	anArray->next->value.array.length = aLength;
	fxCacheArray(the, anArray);
	if (theParser->token != XS_JSON_TOKEN_RIGHT_BRACKET)
		mxSyntaxError("%ld: missing ]", theParser->line);
	fxGetNextJSONToken(the, theParser);
}

void fxReviveJSON(txMachine* the, txSlot* reviver, txSlot* holder)
{
	txSlot* reference = the->stack;
	if (mxIsReference(reference)) {
		txSlot* instance = reference->value.reference;
		if (fxIsArray(the, instance)) {
			txIndex length, index;
			mxPushSlot(reference);
			fxGetID(the, mxID(_length));
			length = (txIndex)fxToLength(the, the->stack);
			mxPop();
			index = 0;
			while (index < length) {
				mxPushUndefined();
				fxKeyAt(the, XS_NO_ID, index, the->stack);
				mxPushSlot(reference);
				fxGetIndex(the, index);
				fxReviveJSON(the, reviver, instance);
				if (mxIsUndefined(the->stack)) {
					mxPushSlot(reference);
					fxDeleteIndex(the, index);
				}
				else {
					mxPushSlot(reference);
					fxSetIndex(the, index);
				}
				mxPop();
				index++;
			}
		}
		else {
			txSlot* at = fxNewInstance(the);
			mxBehaviorOwnKeys(the, instance, XS_EACH_NAME_FLAG, at);
			while ((at = at->next)) {
				mxPushUndefined();
				fxKeyAt(the, at->value.at.id, at->value.at.index, the->stack);
				mxPushSlot(reference);
				fxGetAll(the, at->value.at.id, at->value.at.index);
				fxReviveJSON(the, reviver, instance);
				if (mxIsUndefined(the->stack)) {
					mxPushSlot(reference);
					fxDeleteAll(the, at->value.at.id, at->value.at.index);
				}
				else {
					mxPushSlot(reference);
					fxSetAll(the, at->value.at.id, at->value.at.index);
				}
				mxPop();
			}
			mxPop();
		}
	}
	mxPushInteger(2);
	mxPushReference(holder);
	mxPushSlot(reviver);
	fxCall(the);
}

void fx_JSON_stringify(txMachine* the)
{
	volatile txJSONSerializer aSerializer;
	mxTry(the) {
		c_memset((txJSONSerializer*)&aSerializer, 0, sizeof(aSerializer));
		fxSerializeJSON(the, (txJSONSerializer*)&aSerializer);
		if (aSerializer.offset) {
			fxSerializeJSONChar(the, (txJSONSerializer*)&aSerializer, 0);
			mxResult->value.string = (txString)fxNewChunk(the, aSerializer.offset);
			c_memcpy(mxResult->value.string, aSerializer.buffer, aSerializer.offset);
			mxResult->kind = XS_STRING_KIND;
		}
		c_free(aSerializer.buffer);
	}
	mxCatch(the) {
		if (aSerializer.buffer)
			c_free(aSerializer.buffer);
		fxJump(the);
	}
}

void fxSerializeJSON(txMachine* the, txJSONSerializer* theSerializer)
{
	txSlot* aSlot;
	txInteger aFlag;
	
	aSlot = fxGetInstance(the, mxThis);
	theSerializer->offset = 0;
	theSerializer->size = 1024;
	theSerializer->buffer = c_malloc(1024);
	if (!theSerializer->buffer)
		mxUnknownError("out of memory");

	if (mxArgc > 1) {
		aSlot = mxArgv(1);
		if (mxIsReference(aSlot)) {
			aSlot = fxGetInstance(the, aSlot);
			if (mxIsCallable(aSlot))
				theSerializer->replacer = mxArgv(1);
			else if (fxIsArray(the, aSlot))
				theSerializer->keys = fxToJSONKeys(the, mxArgv(1));
		}
	}
	if (mxArgc > 2) {
		aSlot = mxArgv(2);
		if (mxIsReference(aSlot)) {
			aSlot = fxGetInstance(the, aSlot);
			if (mxIsNumber(aSlot) || mxIsString(aSlot))
				aSlot = aSlot->next;
		}
		if ((aSlot->kind == XS_INTEGER_KIND) || (aSlot->kind == XS_NUMBER_KIND)) {
			txInteger aCount = fxToInteger(the, aSlot), anIndex;
			if (aCount < 0)
				aCount = 0;
			else if (aCount > 10)
				aCount = 10;
			for (anIndex = 0; anIndex < aCount; anIndex++)
				theSerializer->indent[anIndex] = ' ';
		}
		else if (mxIsStringPrimitive(aSlot))
			c_strncpy((char *)theSerializer->indent, aSlot->value.string, 10);
	}

	theSerializer->stack = the->stack;
	mxPush(mxObjectPrototype);
	fxNewObjectInstance(the);
	aFlag = 0;
	if (mxArgc > 0)
		mxPushSlot(mxArgv(0));
	else
		mxPushUndefined();
	mxPush(mxEmptyString);
	fxSerializeJSONProperty(the, theSerializer, &aFlag);
	the->stack++;
}

void fxSerializeJSONChar(txMachine* the, txJSONSerializer* theSerializer, char c)
{
    //fprintf(stderr, "%c", c);
	if (theSerializer->offset == theSerializer->size) {
		char* aBuffer;
		theSerializer->size += 1024;
		aBuffer = c_realloc(theSerializer->buffer, theSerializer->size);
		if (!aBuffer)
			mxUnknownError("out of memory");
		theSerializer->buffer = aBuffer;
	}
	theSerializer->buffer[theSerializer->offset] = c;
	theSerializer->offset++;
}

void fxSerializeJSONChars(txMachine* the, txJSONSerializer* theSerializer, char* s)
{
    //fprintf(stderr, "%s", s);
	txSize aSize = c_strlen(s);
	if ((theSerializer->offset + aSize) >= theSerializer->size) {
		char* aBuffer;
		theSerializer->size += ((aSize / 1024) + 1) * 1024;
		aBuffer = c_realloc(theSerializer->buffer, theSerializer->size);
		if (!aBuffer)
			mxUnknownError("out of memory");
		theSerializer->buffer = aBuffer;
	}
	c_memcpy(theSerializer->buffer + theSerializer->offset, s, aSize);
	theSerializer->offset += aSize;
}

void fxSerializeJSONIndent(txMachine* the, txJSONSerializer* theSerializer)
{
	txInteger aLevel;
	if (theSerializer->indent[0]) {
		fxSerializeJSONChar(the, theSerializer, '\n');
		for (aLevel = 0; aLevel < theSerializer->level; aLevel++)
			fxSerializeJSONChars(the, theSerializer, theSerializer->indent);
	}
}

void fxSerializeJSONInteger(txMachine* the, txJSONSerializer* theSerializer, txInteger theInteger)
{
	char aBuffer[256];
	fxIntegerToString(the->dtoa, theInteger, aBuffer, sizeof(aBuffer));
	fxSerializeJSONChars(the, theSerializer, aBuffer);
}

void fxSerializeJSONName(txMachine* the, txJSONSerializer* theSerializer, txInteger* theFlag)
{
	txSlot* aSlot = the->stack;
	if (*theFlag & 1) {
		fxSerializeJSONChars(the, theSerializer, ",");
		fxSerializeJSONIndent(the, theSerializer);
	}
	else
		*theFlag |= 1;
	if (*theFlag & 2) {
		if (aSlot->kind == XS_INTEGER_KIND) {
			fxSerializeJSONChar(the, theSerializer, '"');
			fxSerializeJSONInteger(the, theSerializer, aSlot->value.integer);
			fxSerializeJSONChar(the, theSerializer, '"');
		}
		else
			fxSerializeJSONString(the, theSerializer, aSlot->value.string);
		fxSerializeJSONChars(the, theSerializer, ":");
	}
	the->stack++; // POP KEY
}

void fxSerializeJSONNumber(txMachine* the, txJSONSerializer* theSerializer, txNumber theNumber)
{
	int fpclass = c_fpclassify(theNumber);
	if ((fpclass != FP_NAN) && (fpclass != FP_INFINITE)) {
		char aBuffer[256];
		fxNumberToString(the->dtoa, theNumber, aBuffer, sizeof(aBuffer), 0, 0);
		fxSerializeJSONChars(the, theSerializer, aBuffer);
	}
	else
		fxSerializeJSONChars(the, theSerializer, "null");
}

void fxSerializeJSONProperty(txMachine* the, txJSONSerializer* theSerializer, txInteger* theFlag)
{
	txSlot* aWrapper = the->stack + 2;
	txSlot* aValue = the->stack + 1;
	txSlot* aKey = the->stack;
	txSlot* anInstance;
	txInteger aFlag;
	txIndex aLength, anIndex;
	
	if (mxIsReference(aValue)) {
		anInstance = fxGetInstance(the, aValue);
		if (anInstance->flag & XS_LEVEL_FLAG)
			mxTypeError("cyclic value");
		mxPushSlot(aKey);
		/* COUNT */
		mxPushInteger(1);
		/* THIS */
		mxPushSlot(aValue);
		/* FUNCTION */
		mxPushSlot(aValue);
		fxGetID(the, mxID(_toJSON));
		if (mxIsReference(the->stack) && mxIsFunction(the->stack->value.reference))  {
			fxCall(the);
			mxPullSlot(aValue);
		}
		else {
			the->stack = aKey;
		}
	}
	else
		anInstance = C_NULL;
	if (theSerializer->replacer) {
		mxPushSlot(aKey);
		mxPushSlot(aValue);
		/* COUNT */
		mxPushInteger(2);
		/* THIS */
		mxPushSlot(aWrapper);
		/* FUNCTION */
		mxPushSlot(theSerializer->replacer);
		fxCall(the);
		mxPullSlot(aValue);
	}
again:
	switch (aValue->kind) {
	case XS_NULL_KIND:
		fxSerializeJSONName(the, theSerializer, theFlag);
		fxSerializeJSONChars(the, theSerializer, "null");
		break;
	case XS_BOOLEAN_KIND:
		fxSerializeJSONName(the, theSerializer, theFlag);
		fxSerializeJSONChars(the, theSerializer, aValue->value.boolean ? "true" : "false");
		break;
	case XS_INTEGER_KIND:
		fxSerializeJSONName(the, theSerializer, theFlag);
		fxSerializeJSONInteger(the, theSerializer, aValue->value.integer);
		break;
	case XS_NUMBER_KIND:
		fxSerializeJSONName(the, theSerializer, theFlag);
		fxSerializeJSONNumber(the, theSerializer, aValue->value.number);
		break;
	case XS_STRING_KIND:
	case XS_STRING_X_KIND:
		fxSerializeJSONName(the, theSerializer, theFlag);
		fxSerializeJSONString(the, theSerializer, aValue->value.string);
		break;
	case XS_REFERENCE_KIND:
		anInstance = fxGetInstance(the, aValue);
		aValue = anInstance->next;
		if (aValue && (aValue->flag & XS_INTERNAL_FLAG) && (aValue->kind != XS_PROXY_KIND)) {
			goto again;
		}
		fxSerializeJSONName(the, theSerializer, theFlag);
		anInstance->flag |= XS_LEVEL_FLAG;
		if (fxIsArray(the, anInstance)) {
			fxSerializeJSONChar(the, theSerializer, '[');
			theSerializer->level++;
			fxSerializeJSONIndent(the, theSerializer);
			aFlag = 4;
			mxPushReference(anInstance);
			fxGetID(the, mxID(_length));
			aLength = fxToInteger(the, the->stack);
			mxPop();
			for (anIndex = 0; anIndex < aLength; anIndex++) {
				mxPushReference(anInstance);
				fxGetID(the, anIndex);
				mxPushInteger(anIndex);
				fxSerializeJSONProperty(the, theSerializer, &aFlag);
			}
			theSerializer->level--;
			fxSerializeJSONIndent(the, theSerializer);
			fxSerializeJSONChar(the, theSerializer, ']');
		}
		else {
			fxSerializeJSONChar(the, theSerializer, '{');
			theSerializer->level++;
			fxSerializeJSONIndent(the, theSerializer);
			aFlag = 2;
			{
				txSlot* at;
				txSlot* property;
				at = fxNewInstance(the);
				mxBehaviorOwnKeys(the, anInstance, XS_EACH_NAME_FLAG, at);
				mxPushUndefined();
				property = the->stack;
				while ((at = at->next)) {
					if (mxBehaviorGetOwnProperty(the, anInstance, at->value.at.id, at->value.at.index, property) && !(property->flag & XS_DONT_ENUM_FLAG)) {
						mxPushReference(anInstance);
						fxGetAll(the, at->value.at.id, at->value.at.index);
						if (at->value.at.id) {
							txSlot* key = fxGetKey(the, at->value.at.id);
							mxPushString(key->value.key.string);
						}
						else
							mxPushInteger((txInteger)at->value.at.index);
						fxSerializeJSONProperty(the, theSerializer, &aFlag);
					}
				}
				mxPop();
				mxPop();
			}
			theSerializer->level--;
			fxSerializeJSONIndent(the, theSerializer);
			fxSerializeJSONChar(the, theSerializer, '}');
		}
		anInstance->flag &= ~XS_LEVEL_FLAG;
		break;
	default:
		if (*theFlag & 4) {
			if (*theFlag & 1) {
				fxSerializeJSONChars(the, theSerializer, ",");
				fxSerializeJSONIndent(the, theSerializer);
			}
			else
				*theFlag |= 1;
			fxSerializeJSONChars(the, theSerializer, "null");
		}
		break;
	}
	the->stack++; // POP VALUE
}


void fxSerializeJSONString(txMachine* the, txJSONSerializer* theStream, txString theString)
{
	txU1* aString = (txU1*)theString;
	txU4 character;
	const txUTF8Sequence *sequence;
	txInteger size;
	
	fxSerializeJSONChar(the, theStream, '"');
	while ((character = c_read8(aString++))) {
		for (sequence = gxUTF8Sequences; sequence->size; sequence++) {
			if ((character & sequence->cmask) == sequence->cval)
				break;
		}
		size = sequence->size - 1;
		while (size > 0) {
			size--;
			character = (character << 6) | (c_read8(aString++) & 0x3F);
		}
		character &= sequence->lmask;
		if (character == 0x110000)
			fxSerializeJSONStringEscape(the, theStream, 0);
		else if (character < 8)
			fxSerializeJSONStringEscape(the, theStream, character);
		else if (character == 8)
			fxSerializeJSONChars(the, theStream, "\\b"); 
		else if (character == 9)
			fxSerializeJSONChars(the, theStream, "\\t"); 
		else if (character == 10)
			fxSerializeJSONChars(the, theStream, "\\n"); 
		else if (character == 11)
			fxSerializeJSONStringEscape(the, theStream, character); 
		else if (character == 12)
			fxSerializeJSONChars(the, theStream, "\\f");
		else if (character == 13)
			fxSerializeJSONChars(the, theStream, "\\r");
		else if (character < 32)
			fxSerializeJSONStringEscape(the, theStream, character);
		else if (character < 34)
			fxSerializeJSONChar(the, theStream, (char)character);
		else if (character == 34)
			fxSerializeJSONChars(the, theStream, "\\\"");
		else if (character < 92)
			fxSerializeJSONChar(the, theStream, (char)character);
		else if (character == 92)
			fxSerializeJSONChars(the, theStream, "\\\\");
		else if (character < 127)
			fxSerializeJSONChar(the, theStream, (char)character);
		else
			fxSerializeJSONStringEscape(the, theStream, character);
	}
	fxSerializeJSONChar(the, theStream, '"');
}

void fxSerializeJSONStringEscape(txMachine* the, txJSONSerializer* theStream, txU4 character)
{
	static const char gxDigits[] ICACHE_FLASH_ATTR = "0123456789abcdef";
	char buffer[16];
	char* p = buffer;
	txU4 surrogate;
	if (character > 0xFFFF) {
		character -= 0x10000;
		surrogate = 0xDC00 | (character & 0x3FF);
		character = 0xD800 | (character >> 10);
	}
	else
		surrogate = 0;
	*p++ = '\\'; 
	*p++ = 'u';
	*p++ = c_read8(gxDigits + ((character & 0x0000F000) >> 12));
	*p++ = c_read8(gxDigits + ((character & 0x00000F00) >> 8));
	*p++ = c_read8(gxDigits + ((character & 0x000000F0) >> 4));
	*p++ = c_read8(gxDigits + (character & 0x0000000F));
	if (surrogate) {
		*p++ = '\\'; 
		*p++ = 'u';
		*p++ = c_read8(gxDigits + ((surrogate & 0x0000F000) >> 12));
		*p++ = c_read8(gxDigits + ((surrogate & 0x00000F00) >> 8));
		*p++ = c_read8(gxDigits + ((surrogate & 0x000000F0) >> 4));
		*p++ = c_read8(gxDigits + (surrogate & 0x0000000F));
	}
	*p = 0;
	fxSerializeJSONChars(the, theStream, buffer);
}

txSlot* fxToJSONKeys(txMachine* the, txSlot* reference)
{
	txSlot* list = fxNewInstance(the);
	txSlot* item = list;
	txSlot* slot;
	txIndex length, i;
	mxPushSlot(reference);
	fxGetID(the, mxID(_length));
	length = (txIndex)fxToLength(the, the->stack);
	mxPop();
	i = 0;
	while (i < length) {
		txID id = XS_NO_ID;
		txIndex index = XS_NO_ID;
		mxPushSlot(reference);
		fxGetIndex(the, i);
		slot = the->stack;
	again:
		if ((slot->kind == XS_STRING_KIND) || (slot->kind == XS_STRING_X_KIND)) {
			if (fxStringToIndex(the->dtoa, slot->value.string, &index)) {
				id = 0;
			}
			else {
				if (slot->kind == XS_STRING_X_KIND)
					id = fxNewNameX(the, slot->value.string);
				else
					id = fxNewName(the, slot);
			}
		}
		else if ((slot->kind == XS_INTEGER_KIND) && fxIntegerToIndex(the->dtoa, slot->value.integer, &index)) {
			id = 0;
		}
		else if ((slot->kind == XS_NUMBER_KIND) && fxNumberToIndex(the->dtoa, slot->value.number, &index)) {
			id = 0;
		}
		else if (slot->kind == XS_REFERENCE_KIND) {
			txSlot* instance = slot->value.reference;
			if (mxIsNumber(instance) || mxIsString(instance)) {
				slot = instance->next;
				goto again;
			}
		}
		if (id != XS_NO_ID) {
			item = item->next = fxNewSlot(the);
			item->value.at.id = id;
			item->value.at.index = index;
			item->kind = XS_AT_KIND;
		}
		mxPop();
		i++;
	}
	return the->stack;
}
