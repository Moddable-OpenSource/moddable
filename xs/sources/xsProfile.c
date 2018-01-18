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

#ifdef mxProfile

static void fxRecordProfiling(txMachine* the, txInteger theFlag, txInteger theID);
static void fxStartProfilingAux(txMachine* the, txSlot* theFrame);
static int fxSubtractTV(c_timeval *result, c_timeval *x, c_timeval *y);
static void fxWriteProfileBOM(txMachine* the);
static void fxWriteProfileGrammar(txMachine* the, txSlot* theProperty, txSlot* theList);
static void fxWriteProfileGrammarArray(txMachine* the, txSlot* theProperty, txSlot* theList);
static void fxWriteProfileProperty(txMachine* the, txSlot* theProperty, txSlot* theList, txInteger theIndex);
static void fxWriteProfileRecords(txMachine* the);
static void fxWriteProfileSymbols(txMachine* the);

enum {
	XS_PROFILE_BEGIN = 0x40000000,
	XS_PROFILE_END = 0x80000000,
	XS_PROFILE_SECOND = 0xC0000000
};

void fxBeginFunction(txMachine* the, txSlot* aSlot)
{
	if (!the->profileFile)
		return;
	if (!mxIsReference(aSlot))
		return;
	aSlot = fxGetInstance(the, aSlot);
	if (!mxIsFunction(aSlot))
		return;
	aSlot = mxFunctionInstanceProfile(aSlot);
	if (aSlot->kind != XS_INTEGER_KIND)
		return;
	fxRecordProfiling(the, 	XS_PROFILE_BEGIN, aSlot->value.integer);
}

void fxBeginGC(txMachine* the)
{
	if (!the->profileFile)
		return;
	fxRecordProfiling(the, 	XS_PROFILE_BEGIN, 0);
}

void fxEndFunction(txMachine* the, txSlot* aSlot)
{
	if (!the->profileFile)
		return;
	if (!mxIsReference(aSlot))
		return;
	aSlot = fxGetInstance(the, aSlot);
	if (!mxIsFunction(aSlot))
		return;
	aSlot = mxFunctionInstanceProfile(aSlot);
	if (aSlot->kind != XS_INTEGER_KIND)
		return;
	fxRecordProfiling(the, 	XS_PROFILE_END, aSlot->value.integer);
}

void fxEndGC(txMachine* the)
{
	if (!the->profileFile)
		return;
	fxRecordProfiling(the, 	XS_PROFILE_END, 0);
}

void fxJumpFrames(txMachine* the, txSlot* from, txSlot* to)
{
	while (from != to) {
		fxEndFunction(the, from + 3);
		from = from->next;
	}
}

void fxRecordProfiling(txMachine* the, txInteger theFlag, txInteger theID)
{
	txProfileRecord* aRecord = the->profileCurrent;
	c_timeval deltaTV;
#if mxWindows
	LARGE_INTEGER counter;
	LARGE_INTEGER delta;
	LARGE_INTEGER million;

	QueryPerformanceCounter(&counter);
	delta.QuadPart = counter.QuadPart - the->profileCounter.QuadPart;
	if (delta.QuadPart <= 0)
		delta.QuadPart = 1;
	million.HighPart = 0;
	million.LowPart = 1000000;
	delta.QuadPart = (million.QuadPart * delta.QuadPart) / the->profileFrequency.QuadPart;
	deltaTV.tv_sec = (long)(delta.QuadPart / million.QuadPart);
	deltaTV.tv_usec = (long)(delta.QuadPart % million.QuadPart);
	the->profileCounter = counter;
#else
	c_timeval tv;
	c_gettimeofday(&tv, NULL);
	fxSubtractTV(&deltaTV, &tv, &(the->profileTV));
	the->profileTV = tv;
#endif
	if (deltaTV.tv_sec) {
		aRecord->profileID = deltaTV.tv_sec;
		aRecord->delta = XS_PROFILE_SECOND;
		aRecord++;
		if (aRecord == the->profileTop) {
			the->profileCurrent = aRecord;
			fxWriteProfileRecords(the);
			aRecord = the->profileCurrent;
		}
	}
	aRecord->profileID = theID;
	aRecord->delta = theFlag | deltaTV.tv_usec;
	aRecord++;
	if (aRecord == the->profileTop) {
		the->profileCurrent = aRecord;
		fxWriteProfileRecords(the);
		aRecord = the->profileCurrent;
	}
	the->profileCurrent = aRecord;
}

void fxStartProfilingAux(txMachine* the, txSlot* theFrame)
{
	if (theFrame) {
		fxStartProfilingAux(the, theFrame->next);
		fxBeginFunction(the, theFrame + 3);
	}
}

int fxSubtractTV(c_timeval *result, c_timeval *x, c_timeval *y)
{
  /* Perform the carry for the later subtraction by updating Y. */
  if (x->tv_usec < y->tv_usec) {
    int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
    y->tv_usec -= 1000000 * nsec;
    y->tv_sec += nsec;
  }
  if (x->tv_usec - y->tv_usec > 1000000) {
    int nsec = (x->tv_usec - y->tv_usec) / 1000000;
    y->tv_usec += 1000000 * nsec;
    y->tv_sec -= nsec;
  }
     
  /* Compute the time remaining to wait.
     `tv_usec' is certainly positive. */
  result->tv_sec = x->tv_sec - y->tv_sec;
  result->tv_usec = x->tv_usec - y->tv_usec;
     
  /* Return 1 if result is negative. */
  return x->tv_sec < y->tv_sec;
}

void fxWriteProfileBOM(txMachine* the)
{
	txID BOM = 0xFF00;
	
	fxWriteProfileFile(the, &BOM, sizeof(BOM));
}

void fxWriteProfileGrammar(txMachine* the, txSlot* theProperty, txSlot* theList)
{
	txSlot* anInstance;
	
	while (theProperty) {
		if (theProperty->kind == XS_REFERENCE_KIND) {
			anInstance = fxGetInstance(the, theProperty);
			if ((!(anInstance->next)) || (anInstance->next->kind != XS_ARRAY_KIND))
				fxWriteProfileProperty(the, theProperty, theList, -1);
		}
		theProperty = theProperty->next;
	}
}

void fxWriteProfileGrammarArray(txMachine* the, txSlot* theProperty, txSlot* theList)
{
	txSlot* anInstance;
	
	while (theProperty) {
		if (theProperty->kind == XS_REFERENCE_KIND) {
			anInstance = fxGetInstance(the, theProperty);
			if ((anInstance->next) && (anInstance->next->kind == XS_ARRAY_KIND))
				fxWriteProfileProperty(the, theProperty, theList, -1);
		}
		theProperty = theProperty->next;
	}
}

#define mxNameSize 256

int TABS = 0;
int TAB;
void fxWriteProfileProperty(txMachine* the, txSlot* theProperty, txSlot* theList, txInteger theIndex)
{
	txSlot* anInstance;
	txSlot* aSlot;
	char aName[mxNameSize];
	txSlot aKey;
	txSlot* aProperty;
	txInteger aCount;
	txInteger anIndex;
	
	if (theProperty->kind != XS_REFERENCE_KIND)
		return;
	anInstance = fxGetInstance(the, theProperty);
	if (anInstance->flag & XS_MARK_FLAG)
		return;
	anInstance->flag |= XS_MARK_FLAG;
	
	aName[0] = 0;
	if (theIndex >= 0) {
		c_strcat(aName, "[");
		fxIntegerToString(the->dtoa, theIndex, aName + 1, mxNameSize - 3);
		c_strcat(aName, "]");
	}	
	else {
		aSlot = fxGetKey(the, theProperty->ID);
		if (aSlot) {
			c_strcat(aName, ".");
			c_strncat(aName, aSlot->value.key.string, mxNameSize - c_strlen(aName) - 1);
		}
		else
			c_strcat(aName, "[]");
	}
	aName[mxNameSize - 1] = 0;
	theList->value.list.last->value.key.string = aName;
		
	aKey.next = C_NULL;	
	aKey.value.key.string = C_NULL;	
	aKey.value.key.sum = 0;	

	anInstance->value.instance.garbage = theList->value.list.last;
	theList->value.list.last->next = &aKey;
	theList->value.list.last = &aKey;

	if ((aProperty = anInstance->next)) {
		switch (aProperty->kind) {
		case XS_CALLBACK_KIND:
		case XS_CODE_KIND:
			aSlot = mxBehaviorGetProperty(the, anInstance, mxID(_prototype), XS_NO_ID, XS_ANY);
			if (aSlot && (aSlot->kind == XS_REFERENCE_KIND)) {
				aSlot->ID = mxID(_prototype);
				fxWriteProfileProperty(the, aSlot, theList, -1);
				aSlot->ID = XS_NO_ID;
			}
			aProperty = aSlot;
			aSlot = mxFunctionInstanceProfile(anInstance);
			if (aSlot->kind == XS_INTEGER_KIND) {
				txInteger id = aSlot->value.integer;
				fxWriteProfileFile(the, &id, sizeof(txInteger));
				aSlot = theList->value.list.first;
				while (aSlot) {
					if (aSlot->value.key.string) {
						fxWriteProfileFile(the, aSlot->value.key.string, c_strlen(aSlot->value.key.string));
					}
					aSlot = aSlot->next;
				}
				fxWriteProfileFile(the, &(aName[mxNameSize - 1]), sizeof(char));
			}
			break;
		case XS_ARRAY_KIND:
			aSlot = aProperty->value.array.address;
			aCount = aProperty->value.array.length;
			for (anIndex = 0; anIndex < aCount; anIndex++) {
				if (aSlot->ID)
					fxWriteProfileProperty(the, aSlot, theList, anIndex);
				aSlot++;
			}
			aProperty = aProperty->next;
			break;
		}
	}
	
	fxWriteProfileGrammar(the, aProperty, theList);
	fxWriteProfileGrammarArray(the, aProperty, theList);
	
	theList->value.list.last = anInstance->value.instance.garbage;
	theList->value.list.last->next = C_NULL;
	theList->value.list.last->value.key.string = C_NULL;
	anInstance->value.instance.garbage = C_NULL;
}

void fxWriteProfileRecords(txMachine* the)
{
	fxWriteProfileFile(the, the->profileBottom, (the->profileCurrent - the->profileBottom) * sizeof(txProfileRecord));
	the->profileCurrent = the->profileBottom;
}

void fxWriteProfileSymbols(txMachine* the)
{
	char aName[255];
	txInteger aProfileID;
	txSlot aKey;
	txSlot aList;
	txSlot* anInstance;
	txSlot* aProperty;
	txSlot* aSlot;
	txSlot* bSlot;
	txSlot* cSlot;
	
	fxWriteProfileFile(the, &(the->profileID), sizeof(txInteger));
	c_strcpy(aName, "(gc)");
	aProfileID = 0;
	fxWriteProfileFile(the, &aProfileID, sizeof(txInteger));
	fxWriteProfileFile(the, aName, c_strlen(aName) + 1);

	aKey.next = C_NULL;	
	aKey.value.key.string = C_NULL;	
	aKey.value.key.sum = 0;
		
	aList.value.list.first = &aKey; 
	aList.value.list.last = &aKey; 

	anInstance = mxGlobal.value.reference;
	anInstance->flag |= XS_MARK_FLAG; 
	
	aProperty = anInstance->next->next;
	fxWriteProfileGrammar(the, aProperty, &aList);
	fxWriteProfileGrammarArray(the, aProperty, &aList);

	aSlot = the->firstHeap;
	while (aSlot) {
		bSlot = aSlot + 1;
		cSlot = aSlot->value.reference;
		while (bSlot < cSlot) {
			bSlot->flag &= ~XS_MARK_FLAG; 
			bSlot++;
		}
		aSlot = aSlot->next;
	}
	if (the->sharedMachine) {
		aSlot = the->sharedMachine->firstHeap;
		while (aSlot) {
			bSlot = aSlot + 1;
			cSlot = aSlot->value.reference;
			while (bSlot < cSlot) {
				bSlot->flag &= ~XS_MARK_FLAG; 
				bSlot++;
			}
			aSlot = aSlot->next;
		}
	}
}

#endif

txS1 fxIsProfiling(txMachine* the)
{
#ifdef mxProfile
	return (the->profileFile) ? 1 : 0;
#else
	return 0;
#endif
}

void fxStartProfiling(txMachine* the)
{
#ifdef mxProfile
	if (the->profileFile)
		return;
	fxOpenProfileFile(the, "xsprofile.records.out");
	fxWriteProfileBOM(the);
#if mxWindows
	QueryPerformanceFrequency(&the->profileFrequency);
	QueryPerformanceCounter(&the->profileCounter);
#else
	c_gettimeofday(&(the->profileTV), NULL);
#endif	
	fxStartProfilingAux(the, the->frame);
#endif
}

void fxStopProfiling(txMachine* the)
{
#ifdef mxProfile
	txSlot* aFrame;

	if (!the->profileFile)
		return;
		
	aFrame = the->frame;
	while (aFrame) {
		fxEndFunction(the, aFrame + 3);
		aFrame = aFrame->next;
	}
	fxRecordProfiling(the, 	0, -1);
	fxWriteProfileRecords(the);
	fxCloseProfileFile(the);
	fxOpenProfileFile(the, "xsprofile.symbols.out");
	fxWriteProfileBOM(the);
	fxWriteProfileSymbols(the);
	fxCloseProfileFile(the);
#endif
}
