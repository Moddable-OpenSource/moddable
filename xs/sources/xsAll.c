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

txString fxAdornStringC(txMachine* the, txString prefix, txSlot* string, txString suffix)
{
	txSize stringSize = c_strlen(string->value.string);
	txSize prefixSize = prefix ? c_strlen(prefix) : 0;
	txSize suffixSize = suffix ? c_strlen(suffix) : 0;
	txSize resultSize = stringSize + prefixSize + suffixSize + 1;
	txString result = (txString)fxNewChunk(the, resultSize);
	if (prefix && prefixSize)
		c_memcpy(result, prefix, prefixSize);
	if (stringSize)
		c_memcpy(result + prefixSize, string->value.string, stringSize);
	if (suffix && suffixSize)
		c_memcpy(result + prefixSize + stringSize, suffix, suffixSize);
	result[prefixSize + stringSize + suffixSize] = 0;
	string->kind = XS_STRING_KIND;
	string->value.string = result;
	return result;
}

txSlot* fxArgToCallback(txMachine* the, txInteger argi)
{
	if (mxArgc > argi) {
		txSlot* slot = mxArgv(argi);
		if (slot->kind == XS_REFERENCE_KIND) {
			slot = slot->value.reference;
			if (slot->next && ((slot->next->kind == XS_CODE_KIND) || (slot->next->kind == XS_CODE_X_KIND) || (slot->next->kind == XS_CALLBACK_KIND)))
				return slot;
		}
	}
	mxTypeError("callback is no function");
	return C_NULL;
}

txNumber fxArgToInteger(txMachine* the, txInteger i, txNumber value)
{
	if (mxArgc > i) {
		value = fxToNumber(the, mxArgv(i));
		if (c_isnan(value))
			value = 0;
		else if (c_isfinite(value))
			value = c_trunc(value);
	}
	return value;
}

void fxBufferFrameName(txMachine* the, txString buffer, txSize size, txSlot* frame, txString suffix)
{
	txSlot* target = frame + 2; 
	txSlot* function = frame + 3; 
	txSlot* _this = frame + 4;
	if (function->kind == XS_REFERENCE_KIND) {
		function = function->value.reference;
		if (mxIsFunction(function)) {
			if (target->kind == XS_UNDEFINED_KIND) {
				txSlot* home = mxFunctionInstanceHome(function)->value.home.object;
				if (home) {
					if (mxIsFunction(home)) {
						fxBufferFunctionName(the, buffer, size, home, ".");
					}
					else {
						txSlot* constructor = mxBehaviorGetProperty(the, home, mxID(_constructor), XS_NO_ID, XS_OWN);
						if (constructor) {
							if (constructor->kind == XS_REFERENCE_KIND) {
								constructor = constructor->value.reference;
								if (mxIsFunction(constructor))
									fxBufferFunctionName(the, buffer, size, constructor, ".prototype.");
							}
						}
						else if (_this->kind == XS_REFERENCE_KIND) {
							fxBufferObjectName(the, buffer, size, _this->value.reference, ".");
						}
					}
				}
			}
			fxBufferFunctionName(the, buffer, size, function, "");
		}
	}
	else
		c_strncat(buffer, "(host)", size - c_strlen(buffer) - 1);
	c_strncat(buffer, suffix, size - c_strlen(buffer) - 1);
}

void fxBufferFunctionName(txMachine* the, txString buffer, txSize size, txSlot* function, txString suffix)
{
	txSlot* slot = mxFunctionInstanceCode(function);
	if (slot->ID != XS_NO_ID) {
		txSlot* key = fxGetKey(the, slot->ID);
		if (key) {
			if ((key->kind == XS_KEY_KIND) || (key->kind == XS_KEY_X_KIND)) {
				c_strncat(buffer, key->value.key.string, size - c_strlen(buffer) - 1);
				c_strncat(buffer, suffix, size - c_strlen(buffer) - 1);
				return;
			}
			if ((key->kind == XS_STRING_KIND) || (key->kind == XS_STRING_X_KIND)) {
				c_strncat(buffer, "[", size - c_strlen(buffer) - 1);
				c_strncat(buffer, key->value.string, size - c_strlen(buffer) - 1);
				c_strncat(buffer, "]", size - c_strlen(buffer) - 1);
				c_strncat(buffer, suffix, size - c_strlen(buffer) - 1);
				return;
			}
		}
	}
	c_strncat(buffer, "?", size - c_strlen(buffer) - 1);
	c_strncat(buffer, suffix, size - c_strlen(buffer) - 1);
}

void fxBufferObjectName(txMachine* the, txString buffer, txSize size, txSlot* object, txString suffix)
{
	txSlot* slot = mxBehaviorGetProperty(the, object, mxID(_Symbol_toStringTag), XS_NO_ID, XS_ANY);
	if (slot && ((slot->kind == XS_STRING_KIND) || (slot->kind == XS_STRING_X_KIND)) && !c_isEmpty(slot->value.string))
		c_strncat(buffer, slot->value.string, size - c_strlen(buffer) - 1);
	else
		c_strncat(buffer, "?", size - c_strlen(buffer) - 1);
	c_strncat(buffer, suffix, size - c_strlen(buffer) - 1);
}

txString fxConcatString(txMachine* the, txSlot* a, txSlot* b)
{
	txSize aSize = c_strlen(a->value.string);
	txSize bSize = c_strlen(b->value.string);
	txString result = (txString)fxNewChunk(the, aSize + bSize + 1);
	c_memcpy(result, a->value.string, aSize);
	c_memcpy(result + aSize, b->value.string, bSize + 1);
	a->value.string = result;
	a->kind = XS_STRING_KIND;
	return result;
}

txString fxConcatStringC(txMachine* the, txSlot* a, txString b)
{
	txSize aSize = c_strlen(a->value.string);
	txSize bSize = c_strlen(b);
	txSize resultSize = aSize + bSize + 1;
	txString result = C_NULL;
	if (a->kind == XS_STRING_KIND)
		result = (txString)fxRenewChunk(the, a->value.string, resultSize);
	if (!result) {
		result = (txString)fxNewChunk(the, resultSize);
		c_memcpy(result, a->value.string, aSize);
		a->value.string = result;
		a->kind = XS_STRING_KIND;
	}
	c_memcpy(result + aSize, b, bSize + 1);
	return result;
}

txString fxCopyString(txMachine* the, txSlot* a, txSlot* b)
{
	txString result = b->value.string;
	a->value.string = result;
	a->kind = b->kind;
	return result;
}

txString fxCopyStringC(txMachine* the, txSlot* a, txString b)
{
	txSize bSize = c_strlen(b);
	txString result = (txString)fxNewChunk(the, bSize + 1);
	c_memcpy(result, b, bSize + 1);
	a->value.string = result;
	a->kind = XS_STRING_KIND;
	return result;
}

txBoolean fxIsCanonicalIndex(txMachine* the, txID id)
{
	txSlot* key = fxGetKey(the, id);
	if (key && (key->flag & XS_DONT_ENUM_FLAG)) {
		txString string = key->value.key.string;
		char buffer[256], c;
		txNumber number;
		c = c_read8(string);
		if (('+' != c) && ('-' != c) && ('.' != c) && ('I' != c) && !(('0' <= c) && ('9' >= c)))
			return 0;
		number = fxStringToNumber(the->dtoa, string, 1);
		if (number == -0)
			return 1;
		fxNumberToString(the->dtoa, number, buffer, sizeof(buffer), 0, 0);
		if (!c_strcmp(string, buffer)) {
			return 1;
		}
	}
	return 0;
}

txString fxResizeString(txMachine* the, txSlot* a, txSize theSize)
{
	txString result = C_NULL;
	if (a->kind == XS_STRING_KIND)
		result = (txString)fxRenewChunk(the, a->value.string, theSize);
	if (!result) {
		txChunk* aChunk = (txChunk*)(a->value.string - sizeof(txChunk));
		txSize aSize = aChunk->size - sizeof(txChunk); 
		result = (txString)fxNewChunk(the, theSize);
		aChunk = (txChunk*)(result - sizeof(txChunk));
		theSize = aChunk->size - sizeof(txChunk);
		c_memcpy(result, a->value.string, (aSize < theSize) ? aSize : theSize);
		a->value.string = result;
		a->kind = XS_STRING_KIND;
	}
	return result;
}

int fxStringGetter(void* theStream)
{
	txStringStream* aStream = (txStringStream*)theStream;
	int result = C_EOF;
	
	if (aStream->offset < aStream->size) {
		result = *(aStream->slot->value.string + aStream->offset);
		aStream->offset++;
	}
	return result;
}

int fxStringCGetter(void* theStream)
{
	txStringCStream* aStream = (txStringCStream*)theStream;
	int result = C_EOF;
	
	if (aStream->offset < aStream->size) {
		result = *(aStream->buffer + aStream->offset);
		aStream->offset++;
	}
	return result;
}

void fxJump(txMachine* the)
{
	txJump* aJump = the->firstJump;
#ifdef mxProfile
	fxJumpFrames(the, the->frame, aJump->frame);
#endif	
	c_longjmp(aJump->buffer, 1);
}
