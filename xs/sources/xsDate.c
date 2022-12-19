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

typedef struct sxDateTime {
	txNumber year;
	txNumber month;
	txNumber date;
	txNumber hours;
	txNumber minutes;
	txNumber seconds;
	txNumber milliseconds;
	txInteger day;
	txInteger offset;
} txDateTime;

static txSlot* fxNewDateInstance(txMachine* the);

static void fx_Date_aux(txMachine* the, txFlag secure);
static txInteger fx_Date_parse_number(txByte* theCharacter, txString* theString);
static txInteger fx_Date_parse_fraction(txByte* theCharacter, txString* theString);
static txBoolean fx_Date_prototype_get_aux(txMachine* the, txDateTime* td, txBoolean utc, txSlot* slot);
static void fx_Date_prototype_set_aux(txMachine* the, txDateTime* td, txBoolean utc, txSlot* slot);

static txSlot* fxDateCheck(txMachine* the);
static txNumber fxDateClip(txNumber value);
static txNumber fxDateFullYear(txMachine* the, txSlot* slot);
static txNumber fxDateMerge(txDateTime* dt, txBoolean utc);
static txString fxDatePrint2Digits(txString p, txInteger value);	
static txString fxDatePrint3Digits(txString p, txInteger value);	
static txString fxDatePrint4Digits(txString p, txInteger value);	
static txString fxDatePrintDate(txString p, txInteger year, txInteger month, txInteger date);	
static txString fxDatePrintDateUTC(txString p, txInteger year, txInteger month, txInteger date);
static txString fxDatePrintDay(txString p, txInteger day);	
static txString fxDatePrintTime(txString p, txInteger hours, txInteger minutes, txInteger seconds);	
static txString fxDatePrintTimezone(txString p, txInteger offset);	
static txString fxDatePrintYear(txString p, txInteger value);	
static txInteger fxDateSimilarYear(txInteger year);
static void fxDateSplit(txNumber value, txBoolean utc, txDateTime* dt);

void fxBuildDate(txMachine* the)
{
	txSlot* slot;
	mxPush(mxObjectPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Date_prototype_getMilliseconds), 0, mxID(_getMilliseconds), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Date_prototype_getSeconds), 0, mxID(_getSeconds), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Date_prototype_getMinutes), 0, mxID(_getMinutes), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Date_prototype_getHours), 0, mxID(_getHours), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Date_prototype_getDay), 0, mxID(_getDay), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Date_prototype_getDate), 0, mxID(_getDate), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Date_prototype_getMonth), 0, mxID(_getMonth), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Date_prototype_getYear), 0, mxID(_getYear), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Date_prototype_getFullYear), 0, mxID(_getFullYear), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Date_prototype_getUTCMilliseconds), 0, mxID(_getUTCMilliseconds), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Date_prototype_getUTCSeconds), 0, mxID(_getUTCSeconds), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Date_prototype_getUTCMinutes), 0, mxID(_getUTCMinutes), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Date_prototype_getUTCHours), 0, mxID(_getUTCHours), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Date_prototype_getUTCDay), 0, mxID(_getUTCDay), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Date_prototype_getUTCDate), 0, mxID(_getUTCDate), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Date_prototype_getUTCMonth), 0, mxID(_getUTCMonth), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Date_prototype_getUTCFullYear), 0, mxID(_getUTCFullYear), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Date_prototype_valueOf), 0, mxID(_getTime), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Date_prototype_getTimezoneOffset), 0, mxID(_getTimezoneOffset), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Date_prototype_setDate), 1, mxID(_setDate), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Date_prototype_setFullYear), 3, mxID(_setFullYear), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Date_prototype_setHours), 4, mxID(_setHours), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Date_prototype_setMilliseconds), 1, mxID(_setMilliseconds), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Date_prototype_setMinutes), 3, mxID(_setMinutes), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Date_prototype_setMonth), 2, mxID(_setMonth), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Date_prototype_setSeconds), 2, mxID(_setSeconds), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Date_prototype_setTime), 1, mxID(_setTime), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Date_prototype_setYear), 1, mxID(_setYear), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Date_prototype_setUTCDate), 1, mxID(_setUTCDate), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Date_prototype_setUTCFullYear), 3, mxID(_setUTCFullYear), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Date_prototype_setUTCHours), 4, mxID(_setUTCHours), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Date_prototype_setUTCMilliseconds), 1, mxID(_setUTCMilliseconds), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Date_prototype_setUTCMinutes), 3, mxID(_setUTCMinutes), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Date_prototype_setUTCMonth), 2, mxID(_setUTCMonth), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Date_prototype_setUTCSeconds), 2, mxID(_setUTCSeconds), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Date_prototype_toDateString), 0, mxID(_toDateString), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Date_prototype_toISOString), 0, mxID(_toISOString), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Date_prototype_toJSON), 1, mxID(_toJSON), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Date_prototype_toDateString), 0, mxID(_toLocaleDateString), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Date_prototype_toString), 0, mxID(_toLocaleString), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Date_prototype_toTimeString), 0, mxID(_toLocaleTimeString), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Date_prototype_toString), 0, mxID(_toString), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Date_prototype_toTimeString), 0, mxID(_toTimeString), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Date_prototype_toUTCString), 0, mxID(_toUTCString), XS_DONT_ENUM_FLAG);
	slot = fxNextSlotProperty(the, slot, slot, mxID(_toGMTString), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Date_prototype_valueOf), 0, mxID(_valueOf), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Date_prototype_toPrimitive), 1, mxID(_Symbol_toPrimitive), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxDatePrototype = *the->stack;
	slot = fxBuildHostConstructor(the, mxCallback(fx_Date), 7, mxID(_Date));
	mxDateConstructor = *the->stack;
	slot = fxLastProperty(the, slot);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Date_now), 0, mxID(_now), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Date_parse), 1, mxID(_parse), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Date_UTC), 7, mxID(_UTC), XS_DONT_ENUM_FLAG);
	mxPop();
}

txSlot* fxNewDateInstance(txMachine* the)
{
	txSlot* instance;
	txSlot* property;
	instance = fxNewObjectInstance(the);
	property = fxNextNumberProperty(the, instance, 0, XS_NO_ID, XS_INTERNAL_FLAG);
	property->kind = XS_DATE_KIND;
	property->value.number = C_NAN;
	return instance;
}

void fx_Date(txMachine* the)
{
	fx_Date_aux(the, 0);
}

void fx_Date_aux(txMachine* the, txFlag secure)
{
	txInteger c = mxArgc;
	txDateTime dt;
	txSlot* instance;
	if (mxIsUndefined(mxTarget)) {
		char buffer[256];
		txString p = buffer;
		txDateTime dt;
		if (secure)
			mxTypeError("secure mode");
		mxResult->value.number = fxDateNow();
		mxResult->kind = XS_NUMBER_KIND;
		fxDateSplit(mxResult->value.number, 0, &dt);
		p = fxDatePrintDay(p, dt.day);
		*p++ = ' ';
		p = fxDatePrintDate(p, (txInteger)dt.year, (txInteger)dt.month, (txInteger)dt.date);
		*p++ = ' ';
		p = fxDatePrintTime(p, (txInteger)dt.hours, (txInteger)dt.minutes, (txInteger)dt.seconds);
		*p++ = ' ';
		p = fxDatePrintTimezone(p, (txInteger)dt.offset);
		*p = 0;
		fxCopyStringC(the, mxResult, buffer);
		return;
	}
	mxPushSlot(mxTarget);
	fxGetPrototypeFromConstructor(the, &mxDatePrototype);
	instance = fxNewDateInstance(the);
	mxPullSlot(mxResult);
	if (c > 1) {
		dt.year = fxDateFullYear(the, mxArgv(0));
		dt.month = fxToNumber(the, mxArgv(1));
		dt.date = (c > 2) ? fxToNumber(the, mxArgv(2)) : 1;
		dt.hours = (c > 3) ? fxToNumber(the, mxArgv(3)) : 0;
		dt.minutes = (c > 4) ? fxToNumber(the, mxArgv(4)) : 0;
		dt.seconds = (c > 5) ?fxToNumber(the, mxArgv(5)) : 0;
		dt.milliseconds = (c > 6) ? fxToNumber(the, mxArgv(6)) : 0;
		instance->next->value.number = fxDateMerge(&dt, 0);
		return;
	}
	if (c > 0) {
		txSlot* slot = mxArgv(0);
		if (slot->kind == XS_REFERENCE_KIND) {
			txSlot* date = slot->value.reference;
			if ((date->next) && (date->next->kind == XS_DATE_KIND)) {
				instance->next->value.number = date->next->value.number;
				return;
			}
		}
		fxToPrimitive(the, slot, XS_NO_HINT);
		if ((slot->kind == XS_STRING_KIND) || (slot->kind == XS_STRING_X_KIND)) {
			mxPushSlot(mxFunction);
			mxDub();
			mxGetID(mxID(_parse));
			mxCall();
			mxPushSlot(slot);
			mxRunCount(1);
			instance->next->value.number = the->stack->value.number;
			mxPop();
			return;
		}
		instance->next->value.number = fxDateClip(fxToNumber(the, slot));
		return;
	}
	if (secure)
		mxTypeError("secure mode");
	instance->next->value.number = fxDateNow();
}

void fx_Date_secure(txMachine* the)
{
	fx_Date_aux(the, 1);
}

void fx_Date_now(txMachine* the)
{
	mxResult->value.number = fxDateNow();
	mxResult->kind = XS_NUMBER_KIND;
}

void fx_Date_now_secure(txMachine* the)
{
	mxTypeError("secure mode");
}

txInteger fx_Date_parse_number(txByte* theCharacter, txString* theString)
{
	txByte c = *theCharacter;
	txString p = *theString;
	txInteger aResult = c - '0';
	c = c_read8(p++);
	while (('0' <= c) && (c <= '9')) {
		aResult = (aResult * 10) + c - '0';
		c = c_read8(p++);
	}
	*theCharacter = c;
	*theString = p;
	return aResult;
}

txInteger fx_Date_parse_fraction(txByte* theCharacter, txString* theString)
{
	txByte c = *theCharacter;
	txString p = *theString;
	txNumber fraction = 100;
	txNumber aResult = ((c - '0') * fraction);
	c = c_read8(p++);
	while (('0' <= c) && (c <= '9')) {
		fraction /= 10;
		aResult = aResult + ((c - '0') * fraction);
		c = c_read8(p++);
	}
	*theCharacter = c;
	*theString = p;
	return (txInteger)c_trunc(aResult);
}

void fx_Date_parse(txMachine* the)
{
	#define mxDayCount 7
	static const char* const gxDays[mxDayCount] ICACHE_RODATA_ATTR = {
		"monday", 
		"tuesday", 
		"wednesday", 
		"thursday", 
		"friday",
		"saturday", 
		"sunday"
	};
	#define mxMonthCount 12
	static const char* const gxMonths[mxMonthCount] ICACHE_RODATA_ATTR = {
		"january",
		"february",
		"march",
		"april",
		"may",
		"june",
		"july",
		"august",
		"september",
		"october",
		"november",
		"december"
	};
	#define mxZoneCount 11
	static const char* const gxZones[mxZoneCount] ICACHE_RODATA_ATTR = {
		"gmt", "ut", "utc",
		"est", "edt",
		"cst", "cdt",
		"mst", "mdt",
		"pst", "pdt"
	};
	static const int gxDeltas[mxZoneCount] ICACHE_XS6RO2_ATTR = {
		0, 0, 0,
		-5, -4,
		-6, -5,
		-7, -6,
		-8, -7
	};
	
	txString aString;
	txDateTime dt;
	txString p;
	txString q;
	txByte c;
	char buffer[10];	/* base type should be the same as txString */
	txInteger aComment;
	txInteger aDelta;
	txInteger aValue;
	txSize aLength;
	txInteger i;
	txInteger yearSign = 1;
		
	if (mxArgc < 1)
		goto fail;
	aString = fxToString(the, mxArgv(0));
	
	dt.seconds = -1;
	dt.minutes = -1;
	dt.hours = -1;
	dt.date = -1;
	dt.month = -1;
	dt.year = -1;
	dt.milliseconds = -1;
	aComment = 0;
	aDelta = -1;

	c = c_read8(aString++);
	while (c) {
		if (c == '(') {
			aComment++;
			c = c_read8(aString++);
			continue;
		}
		else if (c == ')') {
			if (aComment) {
				aComment--;
				c = c_read8(aString++);
				continue;
			}
			else
				goto fail;
		}
		else if (aComment) {
			c = c_read8(aString++);
			continue;
		}	
		
		if ((c <= ' ') || (c == ',')) {
			c = c_read8(aString++);
			continue;
		}
			
		else if ((c == '-') | (c == '+')) {
            txInteger aSign;
			if (c == '-')	
				aSign = -1;
			else
				aSign = 1;
			c = c_read8(aString++);
			if (('0' <= c) && (c <= '9')) {
				aValue = fx_Date_parse_number(&c, &aString);
				if (c == '-') {
					if (dt.year >= 0)
						goto fail;
					dt.year = aValue;
					yearSign = aSign;
					c = c_read8(aString++);
					if (('0' <= c) && (c <= '9')) {
						dt.month = fx_Date_parse_number(&c, &aString) - 1;
						if (c == '-') {
							c = c_read8(aString++);
							if (('0' <= c) && (c <= '9'))
								dt.date = fx_Date_parse_number(&c, &aString);
							else
								dt.date = 1;
						}
					}
					else
						dt.month = 0;
				}
				else {
					if ((aDelta != 0) && (aDelta != -1))
						goto fail;
					if (c == ':') {
						aDelta = 60 * aValue;
						c = c_read8(aString++);
						if (('0' <= c) && (c <= '9')) {
							aDelta += fx_Date_parse_number(&c, &aString);
						}
					}
					else {
						if (aValue < 24)
							aDelta = aValue * 60;
						else
							aDelta = (aValue % 100) + ((aValue / 100) * 60);
					}
					aDelta *= aSign;
				}
			}
			else
				goto fail;
		}		
		else if (('0' <= c) && (c <= '9')) {
			aValue = fx_Date_parse_number(&c, &aString);
			if (c == ':') {
				if (dt.hours >= 0) 
					goto fail;
				dt.hours = aValue;	
				c = c_read8(aString++);
				if (('0' <= c) && (c <= '9')) {
					dt.minutes = fx_Date_parse_number(&c, &aString);
					if (c == ':') {
						c = c_read8(aString++);
						if (('0' <= c) && (c <= '9')) {
							dt.seconds = fx_Date_parse_number(&c, &aString);
							if (c == '.') {
								c = c_read8(aString++);
								if (('0' <= c) && (c <= '9')) {
									dt.milliseconds = fx_Date_parse_fraction(&c, &aString);
								}
							}
						}
						else
							dt.seconds = 0;
					}
				}
				else
					dt.seconds = 0;
			}
			else if (c == '/') {
				if (dt.year >= 0)
					goto fail;
				dt.year = /*(aValue < 100) ? aValue + 1900 :*/ aValue;
				c = c_read8(aString++);
				if (('0' <= c) && (c <= '9')) {
					dt.month = fx_Date_parse_number(&c, &aString) - 1;
					if (c == '/') {
						c = c_read8(aString++);
						if (('0' <= c) && (c <= '9')) {
							dt.date = fx_Date_parse_number(&c, &aString);
						}
						else
							dt.date = 1;
					}
				}
				else
					dt.month = 0;
			}
			else if (c == '-') {
				if (dt.year >= 0)
					goto fail;
				dt.year = /*(aValue < 100) ? aValue + 1900 :*/ aValue;
				c = c_read8(aString++);
				if (('0' <= c) && (c <= '9')) {
					dt.month = fx_Date_parse_number(&c, &aString) - 1;
					if (c == '-') {
						c = c_read8(aString++);
						if (('0' <= c) && (c <= '9'))
							dt.date = fx_Date_parse_number(&c, &aString);
						else
							dt.date = 1;
					}
				}
				else
					dt.month = 0;
			}
			else {
				if (aValue < 70) {
					if (dt.date < 0)
						dt.date = aValue;
					else
						goto fail;
				}
				else {
					if (dt.year < 0)
						dt.year = /*(aValue < 100) ? aValue + 1900 :*/ aValue;
					else
						goto fail;
				}
			}
		}				
		else if ((('a' <= c) && (c <= 'z')) || (('A' <= c) && (c <= 'Z'))) {
			txSize cmpLength;
			p = buffer;
			q = p + sizeof(buffer) - 1;
			do {
				if (p == q) goto fail;
				*p++ = (c >= 'a') ? c : (c + ('a' - 'A'));
				c = c_read8(aString++);
			} while ((('a' <= c) && (c <= 'z')) || (('A' <= c) && (c <= 'Z')));
			*p = 0;
			aLength = mxPtrDiff(p - (txString)buffer);
			cmpLength = (aLength >= 3) ? aLength : 3;
			if (c_strcmp("am", buffer) == 0) {
				if ((dt.hours < 0) || (12 <  dt.hours))
					goto fail;
				if (dt.hours == 12)
					dt.hours = 0;
				continue;
			}
			if (c_strcmp("pm", buffer) == 0) {
				if ((dt.hours < 0) || (12 <  dt.hours))
					goto fail;
				if (dt.hours != 12)
					dt.hours += 12;
				continue;
			}
			for (i = 0; i < mxDayCount; i++)
				if (c_strncmp(gxDays[i], buffer, cmpLength) == 0)
					break;
			if (i < mxDayCount)
				continue;
			for (i = 0; i < mxMonthCount; i++)
				if (c_strncmp(gxMonths[i], buffer, cmpLength) == 0)
					break;
			if (i < mxMonthCount) {
				if (dt.month < 0) {
					dt.month = i;
					continue;
				}
				else
					goto fail;
			}
			for (i = 0; i < mxZoneCount; i++)
				if (c_strcmp(gxZones[i], buffer) == 0)
					break;
			if (i < mxZoneCount) {
				if (aDelta == -1) {
					aDelta = gxDeltas[i] * 60;
					continue;
				}
				else
					goto fail;
			}
			if (c_strcmp("t", buffer) == 0) {
				if (dt.year < 0) 
					goto fail;
				continue;
			}
			if (c_strcmp("z", buffer) == 0) {
				if (dt.hours < 0) 
					goto fail;
				aDelta = 0;
				continue;
			}
			goto fail;
		}
		else
			goto fail;
	}
   if (dt.year < 0)
       goto fail;
	if ((yearSign < 0) && (dt.year == 0))
       goto fail;
	if (dt.month < 0)
		dt.month = 0;
	if (dt.date < 0)
		dt.date = 1;
	if ((aDelta < 0) && (dt.hours < 0) && (dt.minutes < 0) && (dt.seconds < 0) && (dt.milliseconds < 0))
		aDelta = 0;
	if (dt.hours < 0)
		dt.hours = 0;
	if (dt.minutes < 0)
		dt.minutes = 0;
    if (dt.seconds < 0)
        dt.seconds = 0;
    if (dt.milliseconds < 0)
        dt.milliseconds = 0;
    dt.year *= yearSign;
	mxResult->value.number = fxDateMerge(&dt, (aDelta != -1) ? 1 : 0);
	if (aDelta != -1)
		mxResult->value.number -= (txNumber)aDelta * 60000.0;
	mxResult->kind = XS_NUMBER_KIND;
	return;
fail:
	mxResult->value.number = C_NAN;
	mxResult->kind = XS_NUMBER_KIND;
	//mxSyntaxError("invalid parameter");
}

void fx_Date_UTC(txMachine* the)
{
	txInteger c = mxArgc;
	txDateTime dt;
	dt.year = (c > 0) ? fxDateFullYear(the, mxArgv(0)) : C_NAN;
	dt.month = (c > 1) ? fxToNumber(the, mxArgv(1)) : 0;
	dt.date = (c > 2) ? fxToNumber(the, mxArgv(2)) : 1;
	dt.hours = (c > 3) ? fxToNumber(the, mxArgv(3)) : 0;
	dt.minutes = (c > 4) ? fxToNumber(the, mxArgv(4)) : 0;
	dt.seconds = (c > 5) ?fxToNumber(the, mxArgv(5)) : 0;
	dt.milliseconds = (c > 6) ? fxToNumber(the, mxArgv(6)) : 0;
	mxResult->value.number = fxDateMerge(&dt, 1);
    mxResult->kind = XS_NUMBER_KIND;
}

txBoolean fx_Date_prototype_get_aux(txMachine* the, txDateTime* dt, txBoolean utc, txSlot* slot)
{
	txSlot* instance = mxThis->value.reference;
	txNumber number;
	if (instance->ID) {
		txSlot* alias = the->aliasArray[instance->ID];
		if (alias) {
			instance = alias;
			slot = instance->next;
		}
	}
	number = slot->value.number;
	if (c_isnan(number)) {
		mxResult->value.number = C_NAN;
		mxResult->kind = XS_NUMBER_KIND;
		return 0;
	}
	fxDateSplit(slot->value.number, utc, dt);
	return 1;
}

void fx_Date_prototype_set_aux(txMachine* the, txDateTime* dt, txBoolean utc, txSlot* slot)
{
	txSlot* instance = mxThis->value.reference;
	txNumber number;
	if (instance->ID) {
		txSlot* alias = the->aliasArray[instance->ID];
		if (alias)
			instance = alias;
		else
			instance = fxAliasInstance(the, instance);
		slot = instance->next;
	}
	number = slot->value.number;
	if (c_isnan(number))
		return;
	if (slot->flag & XS_DONT_SET_FLAG)
		mxTypeError("Date instance is read-only");
	mxResult->value.number = slot->value.number = fxDateMerge(dt, utc);
	mxResult->kind = XS_NUMBER_KIND;
}

void fx_Date_prototype_getMilliseconds(txMachine* the)
{
	txDateTime dt;
	txSlot* slot = fxDateCheck(the);
	if (fx_Date_prototype_get_aux(the, &dt, 0, slot)) {
		mxResult->value.integer = (txInteger)dt.milliseconds;
		mxResult->kind = XS_INTEGER_KIND;
	}
}

void fx_Date_prototype_getSeconds(txMachine* the)
{
	txDateTime dt;
	txSlot* slot = fxDateCheck(the);
	if (fx_Date_prototype_get_aux(the, &dt, 0, slot)) {
		mxResult->value.integer = (txInteger)dt.seconds;
		mxResult->kind = XS_INTEGER_KIND;
	}
}

void fx_Date_prototype_getMinutes(txMachine* the)
{
	txDateTime dt;
	txSlot* slot = fxDateCheck(the);
	if (fx_Date_prototype_get_aux(the, &dt, 0, slot)) {
		mxResult->value.integer = (txInteger)dt.minutes;
		mxResult->kind = XS_INTEGER_KIND;
	}
}

void fx_Date_prototype_getHours(txMachine* the)
{
	txDateTime dt;
	txSlot* slot = fxDateCheck(the);
	if (fx_Date_prototype_get_aux(the, &dt, 0, slot)) {
		mxResult->value.integer = (txInteger)dt.hours;
		mxResult->kind = XS_INTEGER_KIND;
	}
}

void fx_Date_prototype_getDay(txMachine* the)
{
	txDateTime dt;
	txSlot* slot = fxDateCheck(the);
	if (fx_Date_prototype_get_aux(the, &dt, 0, slot)) {
		mxResult->value.integer = dt.day;
		mxResult->kind = XS_INTEGER_KIND;
	}
}

void fx_Date_prototype_getDate(txMachine* the)
{
	txDateTime dt;
	txSlot* slot = fxDateCheck(the);
	if (fx_Date_prototype_get_aux(the, &dt, 0, slot)) {
		mxResult->value.integer = (txInteger)dt.date;
		mxResult->kind = XS_INTEGER_KIND;
	}
}

void fx_Date_prototype_getMonth(txMachine* the)
{
	txDateTime dt;
	txSlot* slot = fxDateCheck(the);
	if (fx_Date_prototype_get_aux(the, &dt, 0, slot)) {
		mxResult->value.integer = (txInteger)dt.month;
		mxResult->kind = XS_INTEGER_KIND;
	}
}

void fx_Date_prototype_getYear(txMachine* the)
{
	txDateTime dt;
	txSlot* slot = fxDateCheck(the);
	if (fx_Date_prototype_get_aux(the, &dt, 0, slot)) {
		mxResult->value.integer = (txInteger)dt.year - 1900;
		mxResult->kind = XS_INTEGER_KIND;
	}
}

void fx_Date_prototype_getFullYear(txMachine* the)
{
	txDateTime dt;
	txSlot* slot = fxDateCheck(the);
	if (fx_Date_prototype_get_aux(the, &dt, 0, slot)) {
		mxResult->value.integer = (txInteger)dt.year;
		mxResult->kind = XS_INTEGER_KIND;
	}
}

void fx_Date_prototype_getUTCMilliseconds(txMachine* the)
{
	txDateTime dt;
	txSlot* slot = fxDateCheck(the);
	if (fx_Date_prototype_get_aux(the, &dt, 1, slot)) {
		mxResult->value.integer = (txInteger)dt.milliseconds;
		mxResult->kind = XS_INTEGER_KIND;
	}
}

void fx_Date_prototype_getUTCSeconds(txMachine* the)
{
	txDateTime dt;
	txSlot* slot = fxDateCheck(the);
	if (fx_Date_prototype_get_aux(the, &dt, 1, slot)) {
		mxResult->value.integer = (txInteger)dt.seconds;
		mxResult->kind = XS_INTEGER_KIND;
	}
}

void fx_Date_prototype_getUTCMinutes(txMachine* the)
{
	txDateTime dt;
	txSlot* slot = fxDateCheck(the);
	if (fx_Date_prototype_get_aux(the, &dt, 1, slot)) {
		mxResult->value.integer = (txInteger)dt.minutes;
		mxResult->kind = XS_INTEGER_KIND;
	}
}

void fx_Date_prototype_getUTCHours(txMachine* the)
{
	txDateTime dt;
	txSlot* slot = fxDateCheck(the);
	if (fx_Date_prototype_get_aux(the, &dt, 1, slot)) {
		mxResult->value.integer = (txInteger)dt.hours;
		mxResult->kind = XS_INTEGER_KIND;
	}
}

void fx_Date_prototype_getUTCDay(txMachine* the)
{
	txDateTime dt;
	txSlot* slot = fxDateCheck(the);
	if (fx_Date_prototype_get_aux(the, &dt, 1, slot)) {
		mxResult->value.integer = dt.day;
		mxResult->kind = XS_INTEGER_KIND;
	}
}

void fx_Date_prototype_getUTCDate(txMachine* the)
{
	txDateTime dt;
	txSlot* slot = fxDateCheck(the);
	if (fx_Date_prototype_get_aux(the, &dt, 1, slot)) {
		mxResult->value.integer = (txInteger)dt.date;
		mxResult->kind = XS_INTEGER_KIND;
	}
}

void fx_Date_prototype_getUTCMonth(txMachine* the)
{
	txDateTime dt;
	txSlot* slot = fxDateCheck(the);
	if (fx_Date_prototype_get_aux(the, &dt, 1, slot)) {
		mxResult->value.integer = (txInteger)dt.month;
		mxResult->kind = XS_INTEGER_KIND;
	}
}

void fx_Date_prototype_getUTCFullYear(txMachine* the)
{
	txDateTime dt;
	txSlot* slot = fxDateCheck(the);
	if (fx_Date_prototype_get_aux(the, &dt, 1, slot)) {
		mxResult->value.integer = (txInteger)dt.year;
		mxResult->kind = XS_INTEGER_KIND;
	}
}

void fx_Date_prototype_getTimezoneOffset(txMachine* the)
{
	txDateTime dt;
	txSlot* slot = fxDateCheck(the);
	if (fx_Date_prototype_get_aux(the, &dt, 0, slot)) {
		mxResult->value.integer = 0 - dt.offset;
		mxResult->kind = XS_INTEGER_KIND;
	}
}

void fx_Date_prototype_setMilliseconds(txMachine* the)
{
	txInteger c = mxArgc;
	txDateTime dt;
	txSlot* slot = fxDateCheck(the);
	fx_Date_prototype_get_aux(the, &dt, 0, slot);
	dt.milliseconds = (c > 0) ? fxToNumber(the, mxArgv(0)) : C_NAN;
	fx_Date_prototype_set_aux(the, &dt, 0, slot);
}

void fx_Date_prototype_setSeconds(txMachine* the)
{
	txInteger c = mxArgc;
	txDateTime dt;
	txSlot* slot = fxDateCheck(the);
	fx_Date_prototype_get_aux(the, &dt, 0, slot);
	dt.seconds = (c > 0) ? fxToNumber(the, mxArgv(0)) : C_NAN;
	if (c > 1) dt.milliseconds = fxToNumber(the, mxArgv(1));
	fx_Date_prototype_set_aux(the, &dt, 0, slot);
}

void fx_Date_prototype_setMinutes(txMachine* the)
{
	txInteger c = mxArgc;
	txDateTime dt;
	txSlot* slot = fxDateCheck(the);
	fx_Date_prototype_get_aux(the, &dt, 0, slot);
	dt.minutes = (c > 0) ? fxToNumber(the, mxArgv(0)) : C_NAN;
	if (c > 1) dt.seconds = fxToNumber(the, mxArgv(1));
	if (c > 2) dt.milliseconds = fxToNumber(the, mxArgv(2));
	fx_Date_prototype_set_aux(the, &dt, 0, slot);
}

void fx_Date_prototype_setHours(txMachine* the)
{
	txInteger c = mxArgc;
	txDateTime dt;
	txSlot* slot = fxDateCheck(the);
	fx_Date_prototype_get_aux(the, &dt, 0, slot);
	dt.hours = (c > 0) ? fxToNumber(the, mxArgv(0)) : C_NAN;
	if (c > 1) dt.minutes = fxToNumber(the, mxArgv(1));
	if (c > 2) dt.seconds = fxToNumber(the, mxArgv(2));
	if (c > 3) dt.milliseconds = fxToNumber(the, mxArgv(3));
	fx_Date_prototype_set_aux(the, &dt, 0, slot);
}

void fx_Date_prototype_setDate(txMachine* the)
{
	txInteger c = mxArgc;
	txDateTime dt;
	txSlot* slot = fxDateCheck(the);
	fx_Date_prototype_get_aux(the, &dt, 0, slot);
	dt.date = (c > 0) ? fxToNumber(the, mxArgv(0)) : C_NAN;
	fx_Date_prototype_set_aux(the, &dt, 0, slot);
}

void fx_Date_prototype_setMonth(txMachine* the)
{
	txInteger c = mxArgc;
	txDateTime dt;
	txSlot* slot = fxDateCheck(the);
	fx_Date_prototype_get_aux(the, &dt, 0, slot);
	dt.month = (c > 0) ? fxToNumber(the, mxArgv(0)) : C_NAN;
	if (c > 1) dt.date = fxToNumber(the, mxArgv(1));
	fx_Date_prototype_set_aux(the, &dt, 0, slot);
}

void fx_Date_prototype_setYear(txMachine* the)
{
	txInteger c = mxArgc;
	txDateTime dt;
	txSlot* slot = fxDateCheck(the);
	if (c_isnan(slot->value.number)) {
		slot->value.number = 0;
		fx_Date_prototype_get_aux(the, &dt, 1, slot);
	}
	else
		fx_Date_prototype_get_aux(the, &dt, 0, slot);
	dt.year = (mxArgc > 0) ? fxDateFullYear(the, mxArgv(0)) : C_NAN;
	if (c > 1) dt.month = fxToNumber(the, mxArgv(1));
	if (c > 2) dt.date = fxToNumber(the, mxArgv(2));
	fx_Date_prototype_set_aux(the, &dt, 0, slot);
}

void fx_Date_prototype_setFullYear(txMachine* the)
{
	txInteger c = mxArgc;
	txDateTime dt;
	txSlot* slot = fxDateCheck(the);
	if (c_isnan(slot->value.number)) {
		slot->value.number = 0;
		fx_Date_prototype_get_aux(the, &dt, 1, slot);
	}
	else
		fx_Date_prototype_get_aux(the, &dt, 0, slot);
	dt.year = (c > 0) ? fxToNumber(the, mxArgv(0)) : C_NAN;
	if (c > 1) dt.month = fxToNumber(the, mxArgv(1));
	if (c > 2) dt.date = fxToNumber(the, mxArgv(2));
	fx_Date_prototype_set_aux(the, &dt, 0, slot);
}

void fx_Date_prototype_setTime(txMachine* the)
{
	txSlot* slot = fxDateCheck(the);
	if (!slot) mxTypeError("this is no date");
	if (mxArgc < 1)
		slot->value.number = C_NAN;
	else {
		txNumber number = fxToNumber(the, mxArgv(0));
		int fpclass = c_fpclassify(number);
		if (fpclass != FP_NAN) {
			if (c_fabs(number) > 8.64e15)
				number = C_NAN;
			else
				number = c_trunc(number);
		}
		slot->value.number = number;
	}
	mxResult->value.number = slot->value.number;
	mxResult->kind = XS_NUMBER_KIND;
}

void fx_Date_prototype_setUTCMilliseconds(txMachine* the)
{
	txInteger c = mxArgc;
	txDateTime dt;
	txSlot* slot = fxDateCheck(the);
	fx_Date_prototype_get_aux(the, &dt, 1, slot);
	dt.milliseconds = (c > 0) ? fxToNumber(the, mxArgv(0)) : C_NAN;
	fx_Date_prototype_set_aux(the, &dt, 1, slot);
}

void fx_Date_prototype_setUTCSeconds(txMachine* the)
{
	txInteger c = mxArgc;
	txDateTime dt;
	txSlot* slot = fxDateCheck(the);
	fx_Date_prototype_get_aux(the, &dt, 1, slot);
	dt.seconds = (c > 0) ? fxToNumber(the, mxArgv(0)) : C_NAN;
	if (c > 1) dt.milliseconds = fxToNumber(the, mxArgv(1));
	fx_Date_prototype_set_aux(the, &dt, 1, slot);
}

void fx_Date_prototype_setUTCMinutes(txMachine* the)
{
	txInteger c = mxArgc;
	txDateTime dt;
	txSlot* slot = fxDateCheck(the);
	fx_Date_prototype_get_aux(the, &dt, 1, slot);
	dt.minutes = (c > 0) ? fxToNumber(the, mxArgv(0)) : C_NAN;
	if (c > 1) dt.seconds = fxToNumber(the, mxArgv(1));
	if (c > 2) dt.milliseconds = fxToNumber(the, mxArgv(2));
	fx_Date_prototype_set_aux(the, &dt, 1, slot);
}

void fx_Date_prototype_setUTCHours(txMachine* the)
{
	txInteger c = mxArgc;
	txDateTime dt;
	txSlot* slot = fxDateCheck(the);
	fx_Date_prototype_get_aux(the, &dt, 1, slot);
	dt.hours = (c > 0) ? fxToNumber(the, mxArgv(0)) : C_NAN;
	if (c > 1) dt.minutes = fxToNumber(the, mxArgv(1));
	if (c > 2) dt.seconds = fxToNumber(the, mxArgv(2));
	if (c > 3) dt.milliseconds = fxToNumber(the, mxArgv(3));
	fx_Date_prototype_set_aux(the, &dt, 1, slot);
}

void fx_Date_prototype_setUTCDate(txMachine* the)
{
	txInteger c = mxArgc;
	txDateTime dt;
	txSlot* slot = fxDateCheck(the);
	fx_Date_prototype_get_aux(the, &dt, 1, slot);
	dt.date = (c > 0) ? fxToNumber(the, mxArgv(0)) : C_NAN;
	fx_Date_prototype_set_aux(the, &dt, 1, slot);
}

void fx_Date_prototype_setUTCMonth(txMachine* the)
{
	txInteger c = mxArgc;
	txDateTime dt;
	txSlot* slot = fxDateCheck(the);
	fx_Date_prototype_get_aux(the, &dt, 1, slot);
	dt.month = (c > 0) ? fxToNumber(the, mxArgv(0)) : C_NAN;
	if (c > 1) dt.date = fxToNumber(the, mxArgv(1));
	fx_Date_prototype_set_aux(the, &dt, 1, slot);
}

void fx_Date_prototype_setUTCFullYear(txMachine* the)
{
	txInteger c = mxArgc;
	txDateTime dt;
	txSlot* slot = fxDateCheck(the);
	if (c_isnan(slot->value.number)) slot->value.number = 0;
	fx_Date_prototype_get_aux(the, &dt, 1, slot);
	dt.year = (c > 0) ? fxToNumber(the, mxArgv(0)) : C_NAN;
	if (c > 1) dt.month = fxToNumber(the, mxArgv(1));
	if (c > 2) dt.date = fxToNumber(the, mxArgv(2));
	fx_Date_prototype_set_aux(the, &dt, 1, slot);
}

void fx_Date_prototype_toDateString(txMachine* the)
{
	char buffer[256];
	txDateTime dt;
	txSlot* slot = fxDateCheck(the);
	if (fx_Date_prototype_get_aux(the, &dt, 0, slot)) {
		txString p = buffer;
		p = fxDatePrintDay(p, dt.day);
		*p++ = ' ';
		p = fxDatePrintDate(p, (txInteger)dt.year, (txInteger)dt.month, (txInteger)dt.date);
		*p = 0;
	}
	else
		c_strcpy(buffer, "Invalid Date");
	fxCopyStringC(the, mxResult, buffer);
}

void fx_Date_prototype_toISOString(txMachine* the)
{
	char buffer[256];
	txDateTime dt;
	txSlot* slot = fxDateCheck(the);
	if (fx_Date_prototype_get_aux(the, &dt, 1, slot)) {
		txString p = buffer;
		p = fxDatePrintYear(p, (txInteger)dt.year);
		*p++ = '-';
		p = fxDatePrint2Digits(p, (txInteger)dt.month + 1);
		*p++ = '-';
		p = fxDatePrint2Digits(p, (txInteger)dt.date);
		*p++ = 'T';
		p = fxDatePrintTime(p, (txInteger)dt.hours, (txInteger)dt.minutes, (txInteger)dt.seconds);
		*p++ = '.';
		p = fxDatePrint3Digits(p, (txInteger)dt.milliseconds);
		*p++ = 'Z';
		*p = 0;
	}
	else
        mxRangeError("invalid date");
	fxCopyStringC(the, mxResult, buffer);
}

void fx_Date_prototype_toJSON(txMachine* the)
{
	fxToInstance(the, mxThis);
	mxPushSlot(mxThis);
	fxToPrimitive(the, the->stack, XS_NUMBER_HINT);
	if ((the->stack->kind == XS_NUMBER_KIND) && !c_isfinite(the->stack->value.number)) {
		mxPop();
		mxResult->kind = XS_NULL_KIND;
	}
	else {		
		mxPop();
		mxPushSlot(mxThis);
		mxDub();
		mxGetID(mxID(_toISOString));
		mxCall();
		mxRunCount(0);
		mxPullSlot(mxResult);
	}
}

void fx_Date_prototype_toPrimitive(txMachine* the)
{
	if (mxIsReference(mxThis)) {
		txInteger hint = XS_NO_HINT;
		txInteger ids[2], i;
		if (mxArgc > 0) {
			txSlot* slot = mxArgv(0);
			if ((slot->kind == XS_STRING_KIND) || (slot->kind == XS_STRING_X_KIND)) {
				if (!c_strcmp(slot->value.string, "default"))
					hint = XS_STRING_HINT;
				else if (!c_strcmp(slot->value.string, "number"))
					hint = XS_NUMBER_HINT;
				else if (!c_strcmp(slot->value.string, "string"))
					hint = XS_STRING_HINT;
			}
		}
		if (hint == XS_STRING_HINT) {
		 	ids[0] = mxID(_toString);
		 	ids[1] = mxID(_valueOf);
		}
		else if (hint == XS_NUMBER_HINT) {
		 	ids[0] = mxID(_valueOf);
		 	ids[1] = mxID(_toString);
		}
 		else
     		mxTypeError("invalid hint");
		for (i = 0; i < 2; i++) {
			mxPushSlot(mxThis);
			mxPushSlot(mxThis);
			mxGetID(ids[i]);
			if (fxIsCallable(the, the->stack)) {
				mxCall();
				mxRunCount(0);
				if (mxIsReference(the->stack))
					mxPop();
				else {
					mxPullSlot(mxResult);
					return;
      			}
			}
			else {
				mxPop();
				mxPop();
			}
		}
		if (hint == XS_STRING_HINT)
            mxTypeError("cannot coerce object to string");
        else
            mxTypeError("cannot coerce object to number");
	}
	else {
        mxTypeError("invalid this");
	}
}

void fx_Date_prototype_toString(txMachine* the)
{
	char buffer[256];
	txDateTime dt;
	txSlot* slot = fxDateCheck(the);
	if (fx_Date_prototype_get_aux(the, &dt, 0, slot)) {
		txString p = buffer;
		p = fxDatePrintDay(p, dt.day);
		*p++ = ' ';
		p = fxDatePrintDate(p, (txInteger)dt.year, (txInteger)dt.month, (txInteger)dt.date);
		*p++ = ' ';
		p = fxDatePrintTime(p, (txInteger)dt.hours, (txInteger)dt.minutes, (txInteger)dt.seconds);
		*p++ = ' ';
		p = fxDatePrintTimezone(p, (txInteger)dt.offset);
		*p = 0;
	}
	else
		c_strcpy(buffer, "Invalid Date");
	fxCopyStringC(the, mxResult, buffer);
}

void fx_Date_prototype_toTimeString(txMachine* the)
{
	char buffer[256];
	txDateTime dt;
	txSlot* slot = fxDateCheck(the);
	if (fx_Date_prototype_get_aux(the, &dt, 0, slot)) {
		txString p = buffer;
		p = fxDatePrintTime(p, (txInteger)dt.hours, (txInteger)dt.minutes, (txInteger)dt.seconds);
		*p++ = ' ';
		p = fxDatePrintTimezone(p, (txInteger)dt.offset);
		*p = 0;
	}
	else
		c_strcpy(buffer, "Invalid Date");
	fxCopyStringC(the, mxResult, buffer);
}

void fx_Date_prototype_toUTCString(txMachine* the)
{
	char buffer[256];
	txDateTime dt;
	txSlot* slot = fxDateCheck(the);
	if (fx_Date_prototype_get_aux(the, &dt, 1, slot)) {
		txString p = buffer;
		p = fxDatePrintDay(p, dt.day);
		*p++ = ',';
		*p++ = ' ';
		p = fxDatePrintDateUTC(p, (txInteger)dt.year, (txInteger)dt.month, (txInteger)dt.date);
		*p++ = ' ';
		p = fxDatePrintTime(p, (txInteger)dt.hours, (txInteger)dt.minutes, (txInteger)dt.seconds);
		*p++ = ' ';
		*p++ = 'G';
		*p++ = 'M';
		*p++ = 'T';
		*p = 0;
	}
	else
		c_strcpy(buffer, "Invalid Date");
	fxCopyStringC(the, mxResult, buffer);
}

void fx_Date_prototype_valueOf(txMachine* the)
{
	txSlot* slot = fxDateCheck(the);
	mxResult->kind = XS_NUMBER_KIND;
	mxResult->value = slot->value;
}

// Thanks Google V8 for the years offset
// Thanks Mozilla JS for the similar years

static const txString gxDayNames[] ICACHE_XS6RO2_ATTR = {
	"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};
static const txString gxMonthNames[] ICACHE_XS6RO2_ATTR = {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};
static const txInteger gxCommonYearMonthsDays[12] ICACHE_XS6RO2_ATTR = { 
	0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 
};
static const txInteger gxLeapYearMonthsDays[12] ICACHE_XS6RO2_ATTR = { 
	0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335 
};
static const txInteger gxSimilarCommonYears[7] ICACHE_XS6RO2_ATTR = { 
	1978, 1973, 1974, 1975, 1981, 1971, 1977 
};
static const txInteger gxSimilarLeapYears[7] ICACHE_XS6RO2_ATTR = { 
	1984, 1996, 1980, 1992, 1976, 1988, 1972 
};

#define mx1YearDays 365
#define mx4YearsDays ((4 * mx1YearDays) + 1)
#define mx100YearsDays ((25 * mx4YearsDays) - 1)
#define mx400YearsDays ((4 * mx100YearsDays) + 1)
#define mxDayMilliseconds 86400000.0
#define mxYearDays(YEAR) \
	((365 * (YEAR)) + ((YEAR) / 4) - ((YEAR) / 100) + ((YEAR) / 400) + 1)
#define mxIsLeapYear(YEAR) \
	(((YEAR) % 4) ? 0 : ((YEAR) % 100) ? 1 : ((YEAR) % 400) ? 0 : 1)
#define mxYearsOffset 400000

txSlot* fxDateCheck(txMachine* the)
{
	txSlot* it = mxThis;
	if (it->kind == XS_REFERENCE_KIND) {
		txSlot* instance = it->value.reference;
		it = instance->next;
		if ((it) && (it->flag & XS_INTERNAL_FLAG) && (it->kind == XS_DATE_KIND))
			return it;
	}
	mxTypeError("this is no date");
	return C_NULL;
}

txNumber fxDateClip(txNumber value)
{
	if (!c_isfinite(value))
		value = C_NAN;
	else if (c_fabs(value) > 8.64e15)
		value = C_NAN;
	else {
		value = c_trunc(value);
		if (value == 0)
			value = 0;
	}
	return value;	
}

txNumber fxDateFullYear(txMachine* the, txSlot* slot)
{
	txNumber result = fxToNumber(the, slot);
	if (c_isfinite(result)) {
		txInteger value = fxToInteger(the, slot);
		if ((0 <= value) && (value <= 99))
			result = 1900 + value;
	}
	return result;
}

txNumber fxDateMerge(txDateTime* dt, txBoolean utc)
{
	txInteger year, month, leap;
	txNumber value;
	if ((!c_isfinite(dt->year))
	|| (!c_isfinite(dt->month))
	|| (!c_isfinite(dt->date))
	|| (!c_isfinite(dt->hours))
	|| (!c_isfinite(dt->minutes))
	|| (!c_isfinite(dt->seconds))
	|| (!c_isfinite(dt->milliseconds)))
		return C_NAN;
	year = (txInteger)c_trunc(dt->year);
	month = (txInteger)c_trunc(dt->month);
	year += month / 12;
	month %= 12;
	if (month < 0) {
		year--;
		month += 12;
	}
	leap = mxIsLeapYear(year);
	year += mxYearsOffset - 1;
	value = mxYearDays(year) - mxYearDays(1970 + mxYearsOffset - 1);
	if (leap)
		value += gxLeapYearMonthsDays[month];
	else
		value += gxCommonYearMonthsDays[month];
	value += c_trunc(dt->date) - 1;
	value *= mxDayMilliseconds;
	value += c_trunc(dt->hours) * 60 * 60 * 1000;
	value += c_trunc(dt->minutes) * 60 * 1000;
	value += c_trunc(dt->seconds) * 1000;
    value += c_trunc(dt->milliseconds);
	if (!utc) {
		txNumber former = value;
		txInteger similar;
		c_tm tm;
		c_time_t time;
		fxDateSplit(value, 1, dt);
		similar = fxDateSimilarYear((txInteger)dt->year);
		c_memset(&tm, 0, sizeof(c_tm));
		tm.tm_year = similar - 1900;
		tm.tm_mon = (txInteger)dt->month;
		tm.tm_mday = (txInteger)dt->date;
		tm.tm_hour = (txInteger)dt->hours;
		tm.tm_min = (txInteger)dt->minutes;
		tm.tm_sec = (txInteger)dt->seconds;
		tm.tm_isdst =-1;
		time = c_mktime(&tm);	
		if (time == -1)
			value = NAN;
		else {
			value = (txNumber)time;
			value *= 1000;
			value += dt->milliseconds;
			if (similar != year) {
				dt->year = similar;
				value += former - fxDateMerge(dt, 1);
			}
		}
	}
	return fxDateClip(value);
}

txNumber fxDateNow()
{
	c_timeval tv;
	c_gettimeofday(&tv, NULL);
	return fxDateClip(((txNumber)(tv.tv_sec) * 1000.0) + ((txNumber)(tv.tv_usec / 1000)));
}

txString fxDatePrint2Digits(txString p, txInteger value)
{
	*p++ = '0' + value / 10;
	*p++ = '0' + value % 10;
	return p;
}

txString fxDatePrint3Digits(txString p, txInteger value)
{
	*p++ = '0' + value / 100;
	return fxDatePrint2Digits(p, value % 100);
}

txString fxDatePrint4Digits(txString p, txInteger value)
{
	*p++ = '0' + value / 1000;
	return fxDatePrint3Digits(p, value % 1000);
}

txString fxDatePrintDate(txString p, txInteger year, txInteger month, txInteger date)
{
	c_strcpy(p, gxMonthNames[month]);
	p += 3;
	*p++ = ' ';
	p = fxDatePrint2Digits(p, date);
	*p++ = ' ';
	p = fxDatePrintYear(p, year);
	return p;
}

txString fxDatePrintDateUTC(txString p, txInteger year, txInteger month, txInteger date)
{
	p = fxDatePrint2Digits(p, date);
	*p++ = ' ';
	c_strcpy(p, gxMonthNames[month]);
	p += 3;
	*p++ = ' ';
	p = fxDatePrintYear(p, year);
	return p;
}

txString fxDatePrintDay(txString p, txInteger day)
{
	c_strcpy(p, gxDayNames[day]);
	p += 3;
	return p;
}

txString fxDatePrintTime(txString p, txInteger hours, txInteger minutes, txInteger seconds)
{
	p = fxDatePrint2Digits(p, hours);
	*p++ = ':';
	p = fxDatePrint2Digits(p, minutes);
	*p++ = ':';
	p = fxDatePrint2Digits(p, seconds);
	return p;
}

txString fxDatePrintTimezone(txString p, txInteger offset)
{
	*p++ = 'G';
	*p++ = 'M';
	*p++ = 'T';
	if (offset < 0) {
		offset = 0 - offset;
		*p++ = '-';
	}
	else
		*p++ = '+';
	p = fxDatePrint2Digits(p, offset / 60);
	p = fxDatePrint2Digits(p, offset % 60);
	return p;
}

txString fxDatePrintYear(txString p, txInteger value)
{
	if ((value < 0) || (9999 < value)) {
		if (value < 0) {
			*p++ = '-';
			value = -value;
		}
		else
			*p++ = '+';
		if (99999 < value) {
			*p++ = '0' + value / 100000;
			value %= 100000;
		}
		if (9999 < value) {
			*p++ = '0' + value / 10000;
			value %= 10000;
		}
	}
	return fxDatePrint4Digits(p, value);
}

txInteger fxDateSimilarYear(txInteger year)
{
	txInteger leap, day;
	if ((1970 <= year) && (year < 2038))
		return year;
	leap = mxIsLeapYear(year);
	year += mxYearsOffset - 1;
	day = mxYearDays(year) - mxYearDays(1970 + mxYearsOffset - 1);
	day = (day + 4) % 7;
	if (day < 0)
		day += 7;	
	return (leap) ? gxSimilarLeapYears[day] : gxSimilarCommonYears[day];
}

void fxDateSplit(txNumber value, txBoolean utc, txDateTime* dt)
{
	txInteger date, time, year, leap, month;
	const txInteger* monthsDays;
	date = (txInteger)c_trunc(value / mxDayMilliseconds);
	time = (txInteger)c_fmod(value, mxDayMilliseconds);
	if (time < 0) {
		date--;
		time += (txInteger)mxDayMilliseconds;
	}
	dt->offset = 0;
	dt->day = (date + 4) % 7;
	if (dt->day < 0)
		dt->day += 7;	
	dt->milliseconds = time % 1000;
	time /= 1000;
	dt->seconds = time % 60;
	time /= 60;
	dt->minutes = time % 60;
	time /= 60;
	dt->hours = time;
    date += mxYearDays(1970 + mxYearsOffset);
    year = 400 * (date / mx400YearsDays);
	date %= mx400YearsDays;
	date--;
	year += 100 * (date / mx100YearsDays);
	date %= mx100YearsDays;
	date++;
	year += 4 * (date / mx4YearsDays);
	date %= mx4YearsDays;
	date--;
	year += date / mx1YearDays;
  	date %= mx1YearDays;
	year -= mxYearsOffset;
	leap = mxIsLeapYear(year);
  	date += leap;
  	monthsDays = leap ? gxLeapYearMonthsDays : gxCommonYearMonthsDays;
  	for (month = 0; month < 11;	month++) {
  		if (date < monthsDays[month + 1])
  			break;
  	}
    date -= monthsDays[month];
	dt->date = date + 1;
	dt->month = month;
	dt->year = year;
	if (!utc) {
		txNumber former = value;
		txInteger similar;
		c_time_t time;
		c_tm tm;
		similar = fxDateSimilarYear(year);
		if (similar != year) {
			dt->year = similar;
			value = fxDateMerge(dt, 1);
		}
		time = (txInteger)c_floor(value / 1000.0);
		tm = *c_localtime(&time);
		dt->milliseconds = c_floor(c_fmod(value, 1000.0));
		dt->day = tm.tm_wday;
		dt->seconds = tm.tm_sec;
		dt->minutes = tm.tm_min;
		dt->hours = tm.tm_hour;
		dt->date = tm.tm_mday;
		dt->month = tm.tm_mon;
		dt->year = tm.tm_year + 1900 + year - similar;
		dt->offset = (txInteger)c_trunc((fxDateMerge(dt, 1) - former) / 60000.0);
	}
}




