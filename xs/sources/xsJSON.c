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
	txSlot* slot;
	txSize offset;
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
} txJSONStringifier;

static void fxParseJSON(txMachine* the, txJSONParser* theParser);
static void fxParseJSONArray(txMachine* the, txJSONParser* theParser);
static void fxParseJSONObject(txMachine* the, txJSONParser* theParser);
static void fxParseJSONToken(txMachine* the, txJSONParser* theParser);
static void fxParseJSONValue(txMachine* the, txJSONParser* theParser);
static void fxReviveJSON(txMachine* the, txSlot* reviver);

static void fxStringifyJSON(txMachine* the, txJSONStringifier* theStringifier);
static void fxStringifyJSONChar(txMachine* the, txJSONStringifier* theStringifier, char c);
static void fxStringifyJSONChars(txMachine* the, txJSONStringifier* theStringifier, char* s, txSize theSize);
static void fxStringifyJSONIndent(txMachine* the, txJSONStringifier* theStringifier);
static void fxStringifyJSONInteger(txMachine* the, txJSONStringifier* theStringifier, txInteger theInteger);
static void fxStringifyJSONName(txMachine* the, txJSONStringifier* theStringifier, txInteger* theFlag);
static void fxStringifyJSONNumber(txMachine* the, txJSONStringifier* theStringifier, txNumber theNumber);
static void fxStringifyJSONProperty(txMachine* the, txJSONStringifier* theStringifier, txInteger* theFlag);
static void fxStringifyJSONString(txMachine* the, txJSONStringifier* theStringifier, txString theString);
static void fxStringifyJSONUnicodeEscape(txMachine* the, txJSONStringifier* theStringifier, txInteger character);

static txSlot* fxToJSONKeys(txMachine* the, txSlot* reference);

void fxBuildJSON(txMachine* the)
{
	txSlot* slot;
	mxPush(mxObjectPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_JSON_parse), 2, mxID(_parse), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_JSON_stringify), 3, mxID(_stringify), XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "JSON", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxPull(mxJSONObject);
}

void fx_JSON_parse(txMachine* the)
{
	volatile txJSONParser* aParser = C_NULL;
	mxTry(the) {
		if (mxArgc < 1)
			mxSyntaxError("no buffer");
		aParser = c_malloc(sizeof(txJSONParser));
		if (NULL == aParser)
			fxAbort(the, XS_NOT_ENOUGH_MEMORY_EXIT);
		c_memset((txJSONParser*)aParser, 0, sizeof(txJSONParser));
		if ((mxArgc > 1) && mxIsReference(mxArgv(1)) && fxIsArray(the, mxArgv(1)->value.reference))
			aParser->keys = fxToJSONKeys(the, mxArgv(1));
		fxToString(the, mxArgv(0));
		aParser->slot = mxArgv(0);
		aParser->offset = 0;
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
			mxPushSlot(mxArgv(1));
			mxCall();
			mxPushUndefined();
			fxKeyAt(the, id, XS_NO_ID, the->stack);
			mxPushSlot(mxResult);
			fxReviveJSON(the, mxArgv(1));
			mxPullSlot(mxResult);
		}
	}
	mxCatch(the) {
		if (aParser)
			c_free((txJSONParser*)aParser);
		fxJump(the);
	}
}

void fxParseJSON(txMachine* the, txJSONParser* theParser)
{
	mxPush(mxEmptyString);
	theParser->string = the->stack;
	theParser->line = 1;
	fxParseJSONToken(the, theParser);
	fxParseJSONValue(the, theParser);
	if (theParser->token != XS_JSON_TOKEN_EOF)
		mxSyntaxError("%ld: missing EOF", theParser->line);
}

void fxParseJSONArray(txMachine* the, txJSONParser* theParser)
{
	txSlot* anArray;
	txIndex aLength;
	txSlot* anItem;

	fxParseJSONToken(the, theParser);
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
		fxParseJSONToken(the, theParser);
	}
	anArray->next->value.array.length = aLength;
	fxCacheArray(the, anArray);
	if (theParser->token != XS_JSON_TOKEN_RIGHT_BRACKET)
		mxSyntaxError("%ld: missing ]", theParser->line);
	fxParseJSONToken(the, theParser);
}

void fxParseJSONToken(txMachine* the, txJSONParser* theParser)
{
	txInteger character;
	txBoolean escaped;
	txNumber number;
	txSize offset;
	txSize size;
	txString p, s;

	theParser->integer = 0;
	theParser->number = 0;
	theParser->string->value.string = mxEmptyString.value.string;
	theParser->token = XS_NO_JSON_TOKEN;
	p = theParser->slot->value.string + theParser->offset;
	while (theParser->token == XS_NO_JSON_TOKEN) {
		switch (*p) {
		case 0:
			theParser->token = XS_JSON_TOKEN_EOF;
			break;
		case 10:
			p++;
			theParser->line++;
			break;
		case 13:
			p++;
			theParser->line++;
			if (*p == 10)
				p++;
			break;
		case '\t':
		case ' ':
			p++;
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
			if (*p == '-')
				p++;
			if (('0' <= *p) && (*p <= '9')) {
				if (*p == '0') {
					p++;
				}
				else {
					p++;
					while (('0' <= *p) && (*p <= '9'))
						p++;
				}
				if (*p == '.') {
					p++;
					if (('0' <= *p) && (*p <= '9')) {
						p++;
						while (('0' <= *p) && (*p <= '9'))
							p++;
					}
					else
						goto error;
				}
				if ((*p == 'e') || (*p == 'E')) {
					p++;
					if ((*p == '+') || (*p == '-'))
						p++;
					if (('0' <= *p) && (*p <= '9')) {
						p++;
						while (('0' <= *p) && (*p <= '9'))
							p++;
					}
					else
						goto error;
				}
			}
			else
				goto error;
			size = p - s;
			if ((size_t)(size + 1) > sizeof(the->nameBuffer))
				mxSyntaxError("%ld: number overflow", theParser->line);
			c_memcpy(the->nameBuffer, s, size);
			the->nameBuffer[size] = 0;
			theParser->number = fxStringToNumber(the->dtoa, the->nameBuffer, 0);
			theParser->integer = (txInteger)theParser->number;
			number = theParser->integer;
			if ((theParser->number == number) && (theParser->number != -0))
				theParser->token = XS_JSON_TOKEN_INTEGER;
			else
				theParser->token = XS_JSON_TOKEN_NUMBER;
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
			p++;
			escaped = 0;
			offset = p - theParser->slot->value.string;
			size = 0;
			for (;;) {
				p = fxUTF8Decode(p, &character);
				if ((0 <= character) && (character < 32)) {
					goto error;
				}
				else if (character == '"') {
					break;
				}
				else if (character == '\\') {
					escaped = 1;
					switch (*p) {
					case '"':
					case '/':
					case '\\':
					case 'b':
					case 'f':
					case 'n':
					case 'r':
					case 't':
						p++;
						size++;
						break;
					case 'u':
						p++;
						if (fxParseUnicodeEscape(&p, &character, 0, '\\'))
							size += fxUTF8Length(character);
						else
							goto error;
						break;
					default:
						goto error;
					}
				}
				else {
					size += fxUTF8Length(character);
				}
			}
			s = theParser->string->value.string = fxNewChunk(the, size + 1);
			p = theParser->slot->value.string + offset;
			if (escaped) {
				for (;;) {
					if (*p == '"') {
						p++;
						*s = 0;
						break;
					}
					else if (*p == '\\') {
						p++;
						switch (*p) {
						case '"':
						case '/':
						case '\\':
							*s++ = *p++;
							break;
						case 'b':
							p++;
							*s++ = '\b';
							break;
						case 'f':
							p++;
							*s++ = '\f';
							break;
						case 'n':
							p++;
							*s++ = '\n';
							break;
						case 'r':
							p++;
							*s++ = '\r';
							break;
						case 't':
							p++;
							*s++ = '\t';
							break;
						case 'u':
							p++;
							fxParseUnicodeEscape(&p, &character, 0, '\\');
							s = fxUTF8Encode(s, character);
							break;
						}
					}
					else {
						*s++ = *p++;
					}
				}
			}
			else {
				c_memcpy(s, p, size);
				p += size + 1;
				s[size] = 0;
			}
			theParser->token = XS_JSON_TOKEN_STRING;
			break;
		case 'f':
			p++;
			if (*p != 'a') goto error;	
			p++;
			if (*p != 'l') goto error;	
			p++;
			if (*p != 's') goto error;	
			p++;
			if (*p != 'e') goto error;	
			p++;
			theParser->token = XS_JSON_TOKEN_FALSE;
			break;
		case 'n':
			p++;
			if (*p != 'u') goto error;
			p++;
			if (*p != 'l') goto error;
			p++;
			if (*p != 'l') goto error;
			p++;
			theParser->token = XS_JSON_TOKEN_NULL;
			break;
		case 't':
			p++;
			if (*p != 'r') goto error;
			p++;
			if (*p != 'u') goto error;
			p++;
			if (*p != 'e') goto error;
			p++;
			theParser->token = XS_JSON_TOKEN_TRUE;
			break;
		default:
		error:
			mxSyntaxError("%ld: invalid character", theParser->line);	
			break;
		}
	}
	theParser->offset = p - theParser->slot->value.string;
}

void fxParseJSONObject(txMachine* the, txJSONParser* theParser)
{
	txSlot* anObject;
	txSlot* at;
	txIndex index;
	txID id;
	txSlot* aProperty;

	fxParseJSONToken(the, theParser);
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
		fxParseJSONToken(the, theParser);
		if (theParser->token != XS_JSON_TOKEN_COLON) {
			mxSyntaxError("%ld: missing :", theParser->line);
			break;
		}
		fxParseJSONToken(the, theParser);
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
		fxParseJSONToken(the, theParser);
	}
	if (theParser->token != XS_JSON_TOKEN_RIGHT_BRACE)
		mxSyntaxError("%ld: missing }", theParser->line);
	fxParseJSONToken(the, theParser);
}

void fxParseJSONValue(txMachine* the, txJSONParser* theParser)
{
	switch (theParser->token) {
	case XS_JSON_TOKEN_FALSE:
		mxPushBoolean(0);
		fxParseJSONToken(the, theParser);
		break;
	case XS_JSON_TOKEN_TRUE:
		mxPushBoolean(1);
		fxParseJSONToken(the, theParser);
		break;
	case XS_JSON_TOKEN_NULL:
		mxPushNull();
		fxParseJSONToken(the, theParser);
		break;
	case XS_JSON_TOKEN_INTEGER:
		mxPushInteger(theParser->integer);
		fxParseJSONToken(the, theParser);
		break;
	case XS_JSON_TOKEN_NUMBER:
		mxPushNumber(theParser->number);
		fxParseJSONToken(the, theParser);
		break;
	case XS_JSON_TOKEN_STRING:
		mxPushString(theParser->string->value.string);
		fxParseJSONToken(the, theParser);
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

void fxReviveJSON(txMachine* the, txSlot* reviver)
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
				mxPushSlot(reference);
				mxPushSlot(reviver);
				mxCall();
				mxPushUndefined();
				fxKeyAt(the, 0, index, the->stack);
				mxPushSlot(reference);
				fxGetAll(the, 0, index);
				fxReviveJSON(the, reviver);
				if (mxIsUndefined(the->stack)) {
					mxBehaviorDeleteProperty(the, reference->value.reference, 0, index);
				}
				else {
					mxBehaviorDefineOwnProperty(the, reference->value.reference, 0, index, the->stack, XS_GET_ONLY);
				}
				mxPop();
				index++;
			}
		}
		else {
			txSlot* at = fxNewInstance(the);
			mxBehaviorOwnKeys(the, instance, XS_EACH_NAME_FLAG, at);
			while ((at = at->next)) {
				mxPushSlot(reference);
				mxPushSlot(reviver);
                mxCall();
				mxPushUndefined();
				fxKeyAt(the, at->value.at.id, at->value.at.index, the->stack);
				mxPushSlot(reference);
				fxGetAll(the, at->value.at.id, at->value.at.index);
				fxReviveJSON(the, reviver);
				if (mxIsUndefined(the->stack)) {
					mxBehaviorDeleteProperty(the, reference->value.reference, at->value.at.id, at->value.at.index);
				}
				else {
					mxBehaviorDefineOwnProperty(the, reference->value.reference, at->value.at.id, at->value.at.index, the->stack, XS_GET_ONLY);
				}
				mxPop();
			}
			mxPop();
		}
	}
	mxRunCount(2);
}

void fx_JSON_stringify(txMachine* the)
{
	volatile txJSONStringifier aStringifier;
	mxTry(the) {
		c_memset((txJSONStringifier*)&aStringifier, 0, sizeof(aStringifier));
		fxStringifyJSON(the, (txJSONStringifier*)&aStringifier);
		if (aStringifier.offset) {
			fxStringifyJSONChar(the, (txJSONStringifier*)&aStringifier, 0);
			mxResult->value.string = (txString)fxNewChunk(the, aStringifier.offset);
			c_memcpy(mxResult->value.string, aStringifier.buffer, aStringifier.offset);
			mxResult->kind = XS_STRING_KIND;
		}
		c_free(aStringifier.buffer);
	}
	mxCatch(the) {
		if (aStringifier.buffer)
			c_free(aStringifier.buffer);
		fxJump(the);
	}
}

void fxStringifyJSON(txMachine* the, txJSONStringifier* theStringifier)
{
	txSlot* aSlot;
	txInteger aFlag;
	txSlot* instance;
	
	aSlot = fxGetInstance(the, mxThis);
	theStringifier->offset = 0;
	theStringifier->size = 1024;
	theStringifier->buffer = c_malloc(1024);
	if (!theStringifier->buffer)
		fxAbort(the, XS_NOT_ENOUGH_MEMORY_EXIT);

	if (mxArgc > 1) {
		aSlot = mxArgv(1);
		if (mxIsReference(aSlot)) {
			if (fxIsCallable(the, aSlot))
				theStringifier->replacer = mxArgv(1);
			else if (fxIsArray(the, fxGetInstance(the, aSlot)))
				theStringifier->keys = fxToJSONKeys(the, aSlot);
		}
	}
	if (mxArgc > 2) {
		aSlot = mxArgv(2);
		if (mxIsReference(aSlot)) {
			txSlot* instance = fxGetInstance(the, aSlot);
			if (mxIsNumber(instance)) {
				fxToNumber(the, aSlot);
			}
			else if (mxIsString(instance)) {
				fxToString(the, aSlot);
			}
		}
		if ((aSlot->kind == XS_INTEGER_KIND) || (aSlot->kind == XS_NUMBER_KIND)) {
			txInteger aCount = fxToInteger(the, aSlot), anIndex;
			if (aCount < 0)
				aCount = 0;
			else if (aCount > 10)
				aCount = 10;
			for (anIndex = 0; anIndex < aCount; anIndex++)
				theStringifier->indent[anIndex] = ' ';
		}
		else if (mxIsStringPrimitive(aSlot))
			c_strncpy((char *)theStringifier->indent, aSlot->value.string, 10);
	}

	theStringifier->stack = the->stack;
	mxPush(mxObjectPrototype);
	instance = fxNewObjectInstance(the);
	aFlag = 0;
	if (mxArgc > 0)
		mxPushSlot(mxArgv(0));
	else
		mxPushUndefined();
	fxNextSlotProperty(the, instance, the->stack, mxID(__empty_string_), XS_NO_FLAG);
	mxPush(mxEmptyString);
	fxStringifyJSONProperty(the, theStringifier, &aFlag);
	the->stack++;
}

void fxStringifyJSONChar(txMachine* the, txJSONStringifier* theStringifier, char c)
{
    //fprintf(stderr, "%c", c);
	if (theStringifier->offset == theStringifier->size) {
		char* aBuffer;
		theStringifier->size += 1024;
		aBuffer = c_realloc(theStringifier->buffer, theStringifier->size);
		if (!aBuffer)
			mxUnknownError("out of memory");
		theStringifier->buffer = aBuffer;
	}
	theStringifier->buffer[theStringifier->offset] = c;
	theStringifier->offset++;
}

void fxStringifyJSONChars(txMachine* the, txJSONStringifier* theStringifier, char* s, txSize theSize)
{
    //fprintf(stderr, "%s", s);
    if (!theSize)
    	theSize = c_strlen(s);
	if ((theStringifier->offset + theSize) >= theStringifier->size) {
		char* aBuffer;
		theStringifier->size += ((theSize / 1024) + 1) * 1024;
		aBuffer = c_realloc(theStringifier->buffer, theStringifier->size);
		if (!aBuffer)
			mxUnknownError("out of memory");
		theStringifier->buffer = aBuffer;
	}
	c_memcpy(theStringifier->buffer + theStringifier->offset, s, theSize);
	theStringifier->offset += theSize;
}

void fxStringifyJSONIndent(txMachine* the, txJSONStringifier* theStringifier)
{
	txInteger aLevel;
	if (theStringifier->indent[0]) {
		fxStringifyJSONChar(the, theStringifier, '\n');
		for (aLevel = 0; aLevel < theStringifier->level; aLevel++)
			fxStringifyJSONChars(the, theStringifier, theStringifier->indent, 0);
	}
}

void fxStringifyJSONInteger(txMachine* the, txJSONStringifier* theStringifier, txInteger theInteger)
{
	char aBuffer[256];
	fxIntegerToString(the->dtoa, theInteger, aBuffer, sizeof(aBuffer));
	fxStringifyJSONChars(the, theStringifier, aBuffer, 0);
}

void fxStringifyJSONName(txMachine* the, txJSONStringifier* theStringifier, txInteger* theFlag)
{
	txSlot* aSlot = the->stack;
	if (*theFlag & 1) {
		fxStringifyJSONChar(the, theStringifier, ',');
		fxStringifyJSONIndent(the, theStringifier);
	}
	else
		*theFlag |= 1;
	if (*theFlag & 2) {
		if (aSlot->kind == XS_INTEGER_KIND) {
			fxStringifyJSONChar(the, theStringifier, '"');
			fxStringifyJSONInteger(the, theStringifier, aSlot->value.integer);
			fxStringifyJSONChar(the, theStringifier, '"');
		}
		else
			fxStringifyJSONString(the, theStringifier, aSlot->value.string);
		fxStringifyJSONChar(the, theStringifier, ':');
		if (theStringifier->indent[0])
			fxStringifyJSONChar(the, theStringifier, ' ');
	}
	the->stack++; // POP KEY
}

void fxStringifyJSONNumber(txMachine* the, txJSONStringifier* theStringifier, txNumber theNumber)
{
	int fpclass = c_fpclassify(theNumber);
	if ((fpclass != FP_NAN) && (fpclass != FP_INFINITE)) {
		char aBuffer[256];
		fxNumberToString(the->dtoa, theNumber, aBuffer, sizeof(aBuffer), 0, 0);
		fxStringifyJSONChars(the, theStringifier, aBuffer, 0);
	}
	else
		fxStringifyJSONChars(the, theStringifier, "null", 0);
}

void fxStringifyJSONProperty(txMachine* the, txJSONStringifier* theStringifier, txInteger* theFlag)
{
	txSlot* aWrapper = the->stack + 2;
	txSlot* aValue = the->stack + 1;
	txSlot* aKey = the->stack;
	txSlot* anInstance;
	txSlot* aSlot;
	txInteger aFlag;
	txIndex aLength, anIndex;
	
	if (mxIsReference(aValue) || mxIsBigInt(aValue)) {
		/* THIS */
		mxPushSlot(aValue);
		/* FUNCTION */
		mxDub();
		fxGetID(the, mxID(_toJSON));
		if (mxIsReference(the->stack) && mxIsFunction(the->stack->value.reference))  {
			mxCall();
			mxPushSlot(aKey);
			fxToString(the, the->stack);
			mxRunCount(1);
			mxPullSlot(aValue);
		}
		the->stack = aKey;
	}
	if (theStringifier->replacer) {
		/* THIS */
		mxPushSlot(aWrapper);
		/* FUNCTION */
		mxPushSlot(theStringifier->replacer);
		mxCall();
		/* ARGUMENTS */
		mxPushSlot(aKey);
		fxToString(the, the->stack);
		mxPushSlot(aValue);
		/* COUNT */
		mxRunCount(2);
		mxPullSlot(aValue);
		the->stack = aKey;
	}
	if (mxIsReference(aValue)) {
		mxPushSlot(aValue);
		anInstance = fxToInstance(the, the->stack);
		if (anInstance->flag & XS_LEVEL_FLAG)
			mxTypeError("cyclic value");
		the->stack = aKey;
		aSlot = anInstance->next;
		if (aSlot && (aSlot->flag & XS_INTERNAL_FLAG)) {
			if ((aSlot->kind == XS_INTEGER_KIND) || (aSlot->kind == XS_NUMBER_KIND)) {
				fxToNumber(the, aValue);
			}
			else if (mxIsStringPrimitive(aSlot)) {
				fxToString(the, aValue);
			}
			else if ((aSlot->kind == XS_BOOLEAN_KIND) || (aSlot->kind == XS_BIGINT_KIND) || (aSlot->kind == XS_BIGINT_X_KIND)) {
				aValue->kind = aSlot->kind;
				aValue->value = aSlot->value;
			}
		}
	}
	if (aValue->kind == XS_NULL_KIND) {
		fxStringifyJSONName(the, theStringifier, theFlag);
		fxStringifyJSONChars(the, theStringifier, "null", 0);
	}
	else if (aValue->kind == XS_BOOLEAN_KIND) {
		fxStringifyJSONName(the, theStringifier, theFlag);
		fxStringifyJSONChars(the, theStringifier, aValue->value.boolean ? "true" : "false", 0);
	}
	else if (aValue->kind == XS_INTEGER_KIND) {
		fxStringifyJSONName(the, theStringifier, theFlag);
		fxStringifyJSONInteger(the, theStringifier, aValue->value.integer);
	}
	else if (aValue->kind == XS_NUMBER_KIND) {
		fxStringifyJSONName(the, theStringifier, theFlag);
		fxStringifyJSONNumber(the, theStringifier, aValue->value.number);
	}
	else if ((aValue->kind == XS_STRING_KIND) || (aValue->kind == XS_STRING_X_KIND)) {
		fxStringifyJSONName(the, theStringifier, theFlag);
		fxStringifyJSONString(the, theStringifier, aValue->value.string);
	}
	else if ((aValue->kind == XS_BIGINT_KIND) || (aValue->kind == XS_BIGINT_X_KIND)) {
		mxTypeError("stringify bigint");
	}
	else if ((aValue->kind == XS_REFERENCE_KIND) && !fxIsCallable(the, aValue)) {
		mxTry(the) {
			fxStringifyJSONName(the, theStringifier, theFlag);
			anInstance->flag |= XS_LEVEL_FLAG;
			if (fxIsArray(the, anInstance)) {
				fxStringifyJSONChar(the, theStringifier, '[');
				theStringifier->level++;
				fxStringifyJSONIndent(the, theStringifier);
				aFlag = 4;
				mxPushReference(anInstance);
				fxGetID(the, mxID(_length));
				aLength = fxToInteger(the, the->stack);
				mxPop();
				for (anIndex = 0; anIndex < aLength; anIndex++) {
					mxPushReference(anInstance);
					fxGetID(the, anIndex);
					mxPushInteger(anIndex);
					fxStringifyJSONProperty(the, theStringifier, &aFlag);
				}
				theStringifier->level--;
				fxStringifyJSONIndent(the, theStringifier);
				fxStringifyJSONChar(the, theStringifier, ']');
			}
			else {
				fxStringifyJSONChar(the, theStringifier, '{');
				theStringifier->level++;
				fxStringifyJSONIndent(the, theStringifier);
				aFlag = 2;
				{
					txSlot* at;
					txSlot* property;
					if (theStringifier->keys) {
						mxPushUndefined();
						at = theStringifier->keys->value.reference;
					}
					else {
						at = fxNewInstance(the);
						mxBehaviorOwnKeys(the, anInstance, XS_EACH_NAME_FLAG, at);
					}
					mxPushUndefined();
					property = the->stack;
					mxPushReference(anInstance);
					while ((at = at->next)) {
						if (mxBehaviorGetOwnProperty(the, anInstance, at->value.at.id, at->value.at.index, property) && !(property->flag & XS_DONT_ENUM_FLAG)) {
							mxPushReference(anInstance);
							fxGetAll(the, at->value.at.id, at->value.at.index);
							if (at->value.at.id) {
								txSlot* key = fxGetKey(the, at->value.at.id);
								if (mxGetKeySlotKind(key) == XS_KEY_KIND)
									mxPushString(key->value.key.string);
								else
									mxPushStringX(key->value.key.string);
							}
							else
								mxPushInteger((txInteger)at->value.at.index);
							fxStringifyJSONProperty(the, theStringifier, &aFlag);
						}
					}
					mxPop();
					mxPop();
					mxPop();
				}
				theStringifier->level--;
				fxStringifyJSONIndent(the, theStringifier);
				fxStringifyJSONChar(the, theStringifier, '}');
			}
			anInstance->flag &= ~XS_LEVEL_FLAG;
		}
		mxCatch(the) {
			anInstance->flag &= ~XS_LEVEL_FLAG;
			fxJump(the);
		}
	}
	else {
		if (*theFlag & 4) {
			if (*theFlag & 1) {
				fxStringifyJSONChar(the, theStringifier, ',');
				fxStringifyJSONIndent(the, theStringifier);
			}
			else
				*theFlag |= 1;
			fxStringifyJSONChars(the, theStringifier, "null", 0);
		}
	}
	the->stack++; // POP VALUE
}

void fxStringifyJSONString(txMachine* the, txJSONStringifier* theStringifier, txString theString)
{
	txString string;
	txInteger character;	
	fxStringifyJSONChar(the, theStringifier, '"');
	while (((string = fxUTF8Decode(theString, &character))) && (character != C_EOF)) {
		if (character < 8)
			fxStringifyJSONUnicodeEscape(the, theStringifier, character);
		else if (character == 8)
			fxStringifyJSONChars(the, theStringifier, "\\b", 0); 
		else if (character == 9)
			fxStringifyJSONChars(the, theStringifier, "\\t", 0); 
		else if (character == 10)
			fxStringifyJSONChars(the, theStringifier, "\\n", 0); 
		else if (character == 11)
			fxStringifyJSONUnicodeEscape(the, theStringifier, character); 
		else if (character == 12)
			fxStringifyJSONChars(the, theStringifier, "\\f", 0);
		else if (character == 13)
			fxStringifyJSONChars(the, theStringifier, "\\r", 0);
		else if (character < 32)
			fxStringifyJSONUnicodeEscape(the, theStringifier, character);
		else if (character < 34)
			fxStringifyJSONChar(the, theStringifier, (char)character);
		else if (character == 34)
			fxStringifyJSONChars(the, theStringifier, "\\\"", 0);
		else if (character < 92)
			fxStringifyJSONChar(the, theStringifier, (char)character);
		else if (character == 92)
			fxStringifyJSONChars(the, theStringifier, "\\\\", 0);
		else if (character < 127)
			fxStringifyJSONChar(the, theStringifier, (char)character);
		else if ((0xD800 <= character) && (character <= 0xDFFF))
			fxStringifyJSONUnicodeEscape(the, theStringifier, character);
		else
			fxStringifyJSONChars(the, theStringifier, theString, string - theString);
		theString = string;
	}
	fxStringifyJSONChar(the, theStringifier, '"');
}

void fxStringifyJSONUnicodeEscape(txMachine* the, txJSONStringifier* theStringifier, txInteger character)
{
	char buffer[16];
	txString p = buffer;
	*p++ = '\\'; 
	*p++ = 'u'; 
	p = fxStringifyUnicodeEscape(p, character, '\\');
	fxStringifyJSONChars(the, theStringifier, buffer, p - buffer);
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
		else if (slot->kind == XS_INTEGER_KIND) {
			if (fxIntegerToIndex(the->dtoa, slot->value.integer, &index))
				id = 0;
			else {
				fxToString(the, slot);
				goto again;
			}
		}
		else if (slot->kind == XS_NUMBER_KIND){
			if (fxNumberToIndex(the->dtoa, slot->value.number, &index))
				id = 0;
			else {
				fxToString(the, slot);
				goto again;
			}
		}
		else if (slot->kind == XS_REFERENCE_KIND) {
			txSlot* instance = slot->value.reference;
			if (mxIsNumber(instance) || mxIsString(instance)) {
				fxToString(the, slot);
				goto again;
			}
		}
		if (id != XS_NO_ID) {
			txSlot* already = list->next;
			while (already) {
				if ((already->value.at.id == id) && (already->value.at.index == index))
					break;
				already = already->next;
			}
			if (!already) {
				item = item->next = fxNewSlot(the);
				item->value.at.id = id;
				item->value.at.index = index;
				item->kind = XS_AT_KIND;
			}
		}
		mxPop();
		i++;
	}
	return the->stack;
}
