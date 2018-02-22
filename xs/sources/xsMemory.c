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

#ifndef mxReport
#define mxReport 0
#endif
#ifndef mxStress
#define mxStress 0
#endif
#ifndef mxFill
#define mxFill 0
#endif

#if mxStress
int gxStress = 0;
#endif

#define mxChunkFlag 0x80000000

//#define mxRoundSize(_SIZE) ((_SIZE + (sizeof(txChunk) - 1)) & ~(sizeof(txChunk) - 1))
#define mxRoundSize(_SIZE) ((_SIZE + (sizeof(txSize) - 1)) & ~(sizeof(txSize) - 1))

static void fxGrowChunks(txMachine* the, txSize theSize); 
static void fxGrowSlots(txMachine* the, txSize theCount); 
static void fxMark(txMachine* the, void (*theMarker)(txMachine*, txSlot*));
static void fxMarkInstance(txMachine* the, txSlot* theCurrent, void (*theMarker)(txMachine*, txSlot*));
static void fxMarkReference(txMachine* the, txSlot* theSlot);
static void fxMarkValue(txMachine* the, txSlot* theSlot);
static void fxMarkWeakMapTable(txMachine* the, txSlot* table, void (*theMarker)(txMachine*, txSlot*));
static void fxMarkWeakSetTable(txMachine* the, txSlot* table);
static void fxMarkWeakTables(txMachine* the, void (*theMarker)(txMachine*, txSlot*));
static void fxSweep(txMachine* the);
static void fxSweepValue(txMachine* the, txSlot* theSlot);

//#define mxNever 1
#ifdef mxNever

txSize gxRenewChunkCases[4] = { 0, 0, 0, 0 };

typedef struct sxSample txSample;
struct sxSample {
	wide time;
	wide duration;
	long count;
	char* label;
};

void reportTime(txSample* theSample) 
{
	long aDurationMinute;
	long aRemainder;
	long aDurationSecond;
	long aDurationMilli;

	aDurationMinute = WideDivide(&(theSample->duration), 60000000, &aRemainder);
	aDurationSecond = aRemainder / 1000000;
	aRemainder = aRemainder % 1000000;
	aDurationMilli = aRemainder / 1000;  
	fprintf(stderr, "%s * %ld = %ld:%02ld.%03ld\n", theSample->label, theSample->count, 
			aDurationMinute, aDurationSecond, aDurationMilli);
}

void startTime(txSample* theSample) 
{
	Microseconds((UnsignedWide*)&(theSample->time));
}

void stopTime(txSample* theSample) 
{
	wide aTime;
	
	Microseconds((UnsignedWide*)&aTime);
	WideSubtract(&aTime, &(theSample->time));
	WideAdd(&(theSample->duration), &aTime);
	theSample->count++;
}

txSample gxLifeTime = { { 0, 0 }, { 0, 0 }, 0, "life" };
txSample gxMarkTime = { { 0, 0 }, { 0, 0 }, 0, "mark" };
txSample gxSweepChunkTime = { { 0, 0 }, { 0, 0 }, 0, "sweep chunk" };
txSample gxSweepSlotTime = { { 0, 0 }, { 0, 0 }, 0, "sweep slot" };
txSample gxCompactChunkTime = { { 0, 0 }, { 0, 0 }, 0, "compact chunk" };

#endif


void fxCheckStack(txMachine* the, txSlot* slot)
{
	while (slot < the->stackTop) {
		if ((slot->kind != XS_FRAME_KIND) && ((slot + 1)->kind != XS_FRAME_KIND))
			mxCheck(the, slot->next == C_NULL);
		switch(slot->kind) {
		case XS_REFERENCE_KIND:
			mxCheck(the, slot->value.reference->kind == XS_INSTANCE_KIND);
			break;
		default:
			break;
		}
		slot++;
	}
}

void fxAllocate(txMachine* the, txCreation* theCreation)
{
#ifdef mxNever
	startTime(&gxLifeTime);
#endif

	the->currentChunksSize = 0;
	the->peakChunksSize = 0;
	the->maximumChunksSize = 0;
	the->minimumChunksSize = theCreation->incrementalChunkSize - sizeof(txBlock);
	
	the->currentHeapCount = 0;
	the->peakHeapCount = 0;
	the->maximumHeapCount = 0;
	the->minimumHeapCount = theCreation->incrementalHeapCount;
	
	the->firstBlock = C_NULL;
	the->firstHeap = C_NULL;

	fxGrowChunks(the, theCreation->initialChunkSize);

	the->stackBottom = fxAllocateSlots(the, theCreation->stackCount);
	the->stackTop = the->stackBottom + theCreation->stackCount;
	the->stackPrototypes = the->stackTop;
	the->stack = the->stackTop;
#ifdef mxInstrument
	the->stackPeak = the->stackTop;
#endif

	fxGrowSlots(the, theCreation->initialHeapCount);

	the->keyCount = (txID)theCreation->keyCount;
	the->keyIndex = 0;
	the->keyArray = (txSlot **)c_malloc_uint32(theCreation->keyCount * sizeof(txSlot*));
	if (!the->keyArray)
		fxJump(the);

	the->nameModulo = theCreation->nameModulo;
	the->nameTable = (txSlot **)c_malloc_uint32(theCreation->nameModulo * sizeof(txSlot*));
	if (!the->nameTable)
		fxJump(the);

	the->symbolModulo = theCreation->symbolModulo;
	the->symbolTable = (txSlot **)c_malloc_uint32(theCreation->symbolModulo * sizeof(txSlot*));
	if (!the->symbolTable)
		fxJump(the);

	the->cRoot = C_NULL;
}

void fxCollect(txMachine* the, txBoolean theFlag)
{
	txSize aCount;
	txSlot* freeSlot;
	txSlot* aSlot;
	txSlot* bSlot;
	txSlot* cSlot;

	if ((the->collectFlag & XS_COLLECTING_FLAG) == 0) {
		the->collectFlag |= XS_SKIPPED_COLLECT_FLAG;
		return;
	}

#ifdef mxProfile
	fxBeginGC(the);
#endif

	fxMarkHost(the, theFlag ? fxMarkValue : fxMarkReference);

	if (theFlag) {
		fxMark(the, fxMarkValue);
		fxMarkWeakTables(the, fxMarkValue);
		fxSweep(the);
	}
	else {
		fxMark(the, fxMarkReference);
		fxMarkWeakTables(the, fxMarkReference);
	#ifdef mxNever
		startTime(&gxSweepSlotTime);
	#endif
		aCount = 0;
		freeSlot = C_NULL;
		aSlot = the->firstHeap;
		while (aSlot) {
			bSlot = aSlot + 1;
			cSlot = aSlot->value.reference;
			while (bSlot < cSlot) {
				if (bSlot->flag & XS_MARK_FLAG) {
					bSlot->flag &= ~XS_MARK_FLAG; 
					
					if (bSlot->kind == XS_REFERENCE_KIND)
						mxCheck(the, bSlot->value.reference->kind == XS_INSTANCE_KIND);
					
					aCount++;
				}
				else {
					if (bSlot->kind == XS_HOST_KIND) {
						if (bSlot->flag & XS_HOST_HOOKS_FLAG) {
							if (bSlot->value.host.variant.hooks->destructor)
								(*(bSlot->value.host.variant.hooks->destructor))(bSlot->value.host.data);
						}
						else if (bSlot->value.host.variant.destructor)
							(*(bSlot->value.host.variant.destructor))(bSlot->value.host.data);
					}
				#if mxInstrument
					if (bSlot->kind == XS_MODULE_KIND)
						the->loadedModulesCount--;
				#endif
				#if mxFill
					c_memset(bSlot, 0xFF, sizeof(txSlot));
				#endif
					bSlot->kind = XS_UNDEFINED_KIND;
					bSlot->next = freeSlot;
					freeSlot = bSlot;
				}
				bSlot++;
			}
			aSlot = aSlot->next;
		}
		the->currentHeapCount = aCount;
		the->freeHeap = freeSlot;
	#ifdef mxNever
		stopTime(&gxSweepSlotTime);
	#endif
	}
	
	aSlot = the->stack;
	while (aSlot < the->stackTop) {
		aSlot->flag &= ~XS_MARK_FLAG; 
		aSlot++;
	}
	
	fxSweepHost(the);
	
	if (!theFlag) {
		if ((the->maximumHeapCount - the->currentHeapCount) < the->minimumHeapCount)
				the->collectFlag |= XS_TRASHING_FLAG;
			else
				the->collectFlag &= ~XS_TRASHING_FLAG;
	}
	
#if mxReport
	if (theFlag)
		fxReport(the, "# Chunk collection: reserved %ld used %ld peak %ld bytes\n", 
			(long)the->maximumChunksSize, (long)the->currentChunksSize, (long)the->peakChunksSize);
	fxReport(the, "# Slot collection: reserved %ld used %ld peak %ld bytes %ld\n",
		(long)(the->maximumHeapCount * sizeof(txSlot)),
		(long)(the->currentHeapCount * sizeof(txSlot)),
		(long)(the->peakHeapCount * sizeof(txSlot)),
		the->collectFlag & XS_TRASHING_FLAG);
#endif
#ifdef mxInstrument
	the->garbageCollectionCount++;
#endif
#ifdef mxProfile
	fxEndGC(the);
#endif
}

txSlot* fxDuplicateSlot(txMachine* the, txSlot* theSlot)
{
	txSlot* result;
	
	result = fxNewSlot(the);
	result->ID = theSlot->ID;
	result->kind = theSlot->kind;
	result->flag = theSlot->flag & ~XS_MARK_FLAG;
	result->value = theSlot->value;
	return result;
}

void fxFree(txMachine* the) 
{
	txSlot* aHeap;
	txBlock* aBlock;

	if (the->aliasArray)
		c_free_uint32(the->aliasArray);
	the->aliasArray = C_NULL;

	if (the->symbolTable)
		c_free_uint32(the->symbolTable);
	the->symbolTable = C_NULL;
	if (the->nameTable)
		c_free_uint32(the->nameTable);
	the->nameTable = C_NULL;
	if (the->keyArray)
		c_free_uint32(the->keyArray);
	the->keyArray = C_NULL;

	if (the->stackBottom)
		fxFreeSlots(the, the->stackBottom);
	the->stackBottom = C_NULL;
	the->stackTop = C_NULL;
	the->stackPrototypes = C_NULL;
	the->stack = C_NULL;
	
	while (the->firstHeap) {
		aHeap = the->firstHeap;
		the->firstHeap = aHeap->next;
		fxFreeSlots(the, aHeap);
	}
	the->firstHeap = C_NULL;
	
	while (the->firstBlock) {
		aBlock = the->firstBlock;
		the->firstBlock = aBlock->nextBlock;
		fxFreeChunks(the, aBlock);
	}
	the->firstBlock = C_NULL;
	
#ifdef mxNever
	stopTime(&gxLifeTime);
	reportTime(&gxLifeTime);
	fprintf(stderr, "chunk: %ld bytes\n", the->maximumChunksSize);
	fprintf(stderr, "slot: %ld bytes\n", the->maximumHeapCount * sizeof(txSlot));
	reportTime(&gxMarkTime);
	reportTime(&gxSweepChunkTime);
	reportTime(&gxSweepSlotTime);
	reportTime(&gxCompactChunkTime);
	fprintf(stderr, "renew: %ld %ld %ld %ld\n", 
			gxRenewChunkCases[0], 
			gxRenewChunkCases[1], 
			gxRenewChunkCases[2], 
			gxRenewChunkCases[3]);
#endif
}

#ifndef roundup
#define roundup(x, y)	((((x) + (y) - 1) / (y)) * (y))
#endif

void fxGrowChunks(txMachine* the, txSize theSize) 
{
	txByte* aData;
	txBlock* aBlock;

	if (!(the->collectFlag & XS_SKIPPED_COLLECT_FLAG))
		theSize = roundup(theSize, the->minimumChunksSize);
	theSize += sizeof(txBlock);
	aData = fxAllocateChunks(the, theSize);
	if (!aData) {
		fxReport(the, "# Chunk allocation: failed for %ld bytes\n", theSize);
		fxJump(the);
	}
	if ((the->firstBlock != C_NULL) && (the->firstBlock->limit == aData)) {
		the->firstBlock->limit += theSize;
		aBlock = the->firstBlock;
	}
	else {
		aBlock = (txBlock*)aData;
		aBlock->nextBlock = the->firstBlock;
		aBlock->current = aData + sizeof(txBlock);
		aBlock->limit = aData + theSize;
		aBlock->temporary = C_NULL;
		the->firstBlock = aBlock;
	}
	the->maximumChunksSize += theSize;
#if mxReport
	fxReport(the, "# Chunk allocation: reserved %ld used %ld peak %ld bytes\n", 
		the->maximumChunksSize, the->currentChunksSize, the->peakChunksSize);
#endif
}

void fxGrowSlots(txMachine* the, txSize theCount) 
{
	txSlot* aHeap;
	txSlot* aSlot;

	aHeap = fxAllocateSlots(the, theCount);
	if (!aHeap) {
		fxReport(the, "# Slot allocation: failed for %ld bytes\n", theCount * sizeof(txSlot));
		fxJump(the);
	}

	if ((aHeap + theCount) == the->firstHeap) {
		*aHeap = *(the->firstHeap);
		the->maximumHeapCount += theCount;
		theCount -= 1;
	}
	else {
		the->maximumHeapCount += theCount;
		aHeap->next = the->firstHeap;
		aHeap->ID = 0;
		aHeap->flag = 0;
		aHeap->kind = 0;
		aHeap->value.reference = aHeap + theCount;
		theCount -= 2;
	}
	the->firstHeap = aHeap;
	aSlot = aHeap + 1;
    while (theCount--) {
		txSlot* next = aSlot + 1;
		aSlot->next = next;
		aSlot->kind = XS_UNDEFINED_KIND;
        aSlot = next;
    }
	aSlot->next = the->freeHeap;
	aSlot->kind = XS_UNDEFINED_KIND;
	the->freeHeap = aHeap + 1;
	the->collectFlag &= ~XS_TRASHING_FLAG;
#if mxReport
	fxReport(the, "# Slot allocation: reserved %ld used %ld peak %ld bytes\n", 
		(long)(the->maximumHeapCount * sizeof(txSlot)),
		(long)(the->currentHeapCount * sizeof(txSlot)),
		(long)(the->peakHeapCount * sizeof(txSlot)));
#endif
}

void fxMark(txMachine* the, void (*theMarker)(txMachine*, txSlot*))
{
	txInteger anIndex;
	txSlot** anArray;
	txSlot* aSlot;

#ifdef mxNever
	startTime(&gxMarkTime);
#endif
	anArray = the->keyArray;
	anIndex = the->keyIndex;
//#if mxOptimize
//	anArray += the->keyOffset;
	anIndex -= the->keyOffset;
//#endif
	while (anIndex) {
		if ((aSlot = *anArray)) {
			aSlot->flag |= XS_MARK_FLAG;
			(*theMarker)(the, aSlot);
		}
		anArray++;
		anIndex--;
	}
	
	(*theMarker)(the, &mxGlobal);
	anArray = the->aliasArray;
	anIndex = the->aliasCount;
	while (anIndex) {
		if ((aSlot = *anArray))
			if (!(aSlot->flag & XS_MARK_FLAG))
				fxMarkInstance(the, aSlot, theMarker);
		anArray++;
		anIndex--;
	}
	
	aSlot = the->stack;
	while (aSlot < the->stackTop) {
		(*theMarker)(the, aSlot);
		aSlot++;
	}
	aSlot = the->cRoot;
	while (aSlot) {
		(*theMarker)(the, aSlot);
		aSlot = aSlot->next;
	}
#ifdef mxNever
	stopTime(&gxMarkTime);
#endif
}

void fxMarkInstance(txMachine* the, txSlot* theCurrent, void (*theMarker)(txMachine*, txSlot*))
{
	txSlot* aProperty;
	txSlot* aTemporary;

	mxCheck(the, theCurrent->kind == XS_INSTANCE_KIND);
	aProperty = theCurrent;
	theCurrent->value.instance.garbage = C_NULL;
	for (;;) {
		if (aProperty) {
			if (!(aProperty->flag & XS_MARK_FLAG)) {
				aProperty->flag |= XS_MARK_FLAG;
				switch (aProperty->kind) {
				case XS_INSTANCE_KIND:
					aTemporary = aProperty->value.instance.prototype;
					if (aTemporary && !(aTemporary->flag & XS_MARK_FLAG))
						fxMarkInstance(the, aTemporary, theMarker);
					aProperty = aProperty->next;
					break;
				case XS_REFERENCE_KIND:
					aTemporary = aProperty->value.reference;
					if (!(aTemporary->flag & XS_MARK_FLAG)) {
						aProperty->value.reference = theCurrent;
						theCurrent = aTemporary;
						theCurrent->value.instance.garbage = aProperty;
						aProperty = theCurrent;
					}
					else
						aProperty = aProperty->next;
					break;
				default:
					(*theMarker)(the, aProperty);
					aProperty = aProperty->next;
					break;	
				}
			}
			else
				aProperty = aProperty->next;
		}
		else if (theCurrent->value.instance.garbage) {
			aProperty = theCurrent->value.instance.garbage;
			theCurrent->value.instance.garbage = C_NULL;
			
			aTemporary = aProperty->value.reference;
			aProperty->value.reference = theCurrent;
			theCurrent = aTemporary;

			aProperty = aProperty->next;
		}
		else
			break;
	}
}

void fxMarkReference(txMachine* the, txSlot* theSlot)
{
	txSlot* aSlot;
	switch (theSlot->kind) {
	case XS_REFERENCE_KIND:
		aSlot = theSlot->value.reference;
		if (!(aSlot->flag & XS_MARK_FLAG))
			fxMarkInstance(the, aSlot, fxMarkReference);
		break;
	case XS_CLOSURE_KIND:
		aSlot = theSlot->value.closure;
		if (aSlot && (!(aSlot->flag & XS_MARK_FLAG))) {
			aSlot->flag |= XS_MARK_FLAG; 
			fxMarkReference(the, aSlot);
		}
		break;
	case XS_INSTANCE_KIND:
		if (!(theSlot->flag & XS_MARK_FLAG))
			fxMarkInstance(the, theSlot, fxMarkReference);
		break;
	case XS_ACCESSOR_KIND:
		aSlot = theSlot->value.accessor.getter;
		if (aSlot && !(aSlot->flag & XS_MARK_FLAG))
			fxMarkInstance(the, aSlot, fxMarkReference);
		aSlot = theSlot->value.accessor.setter;
		if (aSlot && !(aSlot->flag & XS_MARK_FLAG))
			fxMarkInstance(the, aSlot, fxMarkReference);
		break;
	case XS_ARGUMENTS_SLOPPY_KIND:
	case XS_ARGUMENTS_STRICT_KIND:
	case XS_ARRAY_KIND:
	case XS_STACK_KIND:
		if ((aSlot = theSlot->value.array.address)) {
			txIndex aLength = (((txChunk*)(((txByte*)aSlot) - sizeof(txChunk)))->size) / sizeof(txSlot);
			while (aLength) {
				fxMarkReference(the, aSlot);
				aSlot++;
				aLength--;
			}
		}
		break;
	case XS_CODE_KIND:
	case XS_CODE_X_KIND:
		aSlot = theSlot->value.code.closures;
		if (aSlot && !(aSlot->flag & XS_MARK_FLAG))
			fxMarkInstance(the, aSlot, fxMarkReference);
		break;
	case XS_HOME_KIND:
		aSlot = theSlot->value.home.object;
		if (aSlot && !(aSlot->flag & XS_MARK_FLAG))
			fxMarkInstance(the, aSlot, fxMarkReference);
		aSlot = theSlot->value.home.module;
		if (aSlot && !(aSlot->flag & XS_MARK_FLAG))
			fxMarkInstance(the, aSlot, fxMarkReference);
		break;
	case XS_EXPORT_KIND:
		aSlot = theSlot->value.export.closure;
		if (aSlot && !(aSlot->flag & XS_MARK_FLAG)) {
			aSlot->flag |= XS_MARK_FLAG; 
			fxMarkReference(the, aSlot);
		}
		aSlot = theSlot->value.export.module;
		if (aSlot && !(aSlot->flag & XS_MARK_FLAG))
			fxMarkInstance(the, aSlot, fxMarkReference);
		break;
	case XS_HOST_KIND:
		if (theSlot->value.host.data) {
			if ((theSlot->flag & XS_HOST_HOOKS_FLAG) && (theSlot->value.host.variant.hooks->marker))
				(*theSlot->value.host.variant.hooks->marker)(the, theSlot->value.host.data, fxMarkReference);
		}
		break;
	case XS_PROXY_KIND:
		aSlot = theSlot->value.proxy.handler;
		if (aSlot && !(aSlot->flag & XS_MARK_FLAG))
			fxMarkInstance(the, aSlot, fxMarkReference);
		aSlot = theSlot->value.proxy.target;
		if (aSlot && !(aSlot->flag & XS_MARK_FLAG))
			fxMarkInstance(the, aSlot, fxMarkReference);
		break;
		
	case XS_LIST_KIND:
		aSlot = theSlot->value.list.first;
		while (aSlot) {
			if (!(aSlot->flag & XS_MARK_FLAG)) {
				aSlot->flag |= XS_MARK_FLAG;
				fxMarkReference(the, aSlot);
			}
			aSlot = aSlot->next;
		}
		break;
	case XS_MAP_KIND:
	case XS_SET_KIND:
		{
			txSlot** anAddress = theSlot->value.table.address;
			txInteger aLength = theSlot->value.table.length;
			while (aLength) {
				aSlot = *anAddress;
				while (aSlot) {
					aSlot->flag |= XS_MARK_FLAG; 
					aSlot = aSlot->next;
				}
				anAddress++;
				aLength--;
			}
		}
		break;
	case XS_WEAK_MAP_KIND:
		theSlot->value.table.address[theSlot->value.table.length] = the->firstWeakMapTable;
		the->firstWeakMapTable = theSlot;
		break;
	case XS_WEAK_SET_KIND:
		theSlot->value.table.address[theSlot->value.table.length] = the->firstWeakSetTable;
		the->firstWeakSetTable = theSlot;
		break;
		
	case XS_HOST_INSPECTOR_KIND:
		aSlot = theSlot->value.hostInspector.cache;
		if (!(aSlot->flag & XS_MARK_FLAG))
			fxMarkInstance(the, aSlot, fxMarkReference);
		break;	
	}
}

void fxMarkValue(txMachine* the, txSlot* theSlot)
{
#define mxMarkChunk(_THE_DATA) \
	((txChunk*)(((txByte*)_THE_DATA) - sizeof(txChunk)))->size |= mxChunkFlag

	txSlot* aSlot;
	switch (theSlot->kind) {
	case XS_STRING_KIND:
		mxMarkChunk(theSlot->value.string);
		break;
	case XS_REFERENCE_KIND:
		aSlot = theSlot->value.reference;
		if (!(aSlot->flag & XS_MARK_FLAG))
			fxMarkInstance(the, aSlot, fxMarkValue);
		break;
	case XS_CLOSURE_KIND:
		aSlot = theSlot->value.closure;
		if (aSlot && (!(aSlot->flag & XS_MARK_FLAG))) {
			aSlot->flag |= XS_MARK_FLAG; 
			fxMarkValue(the, aSlot);
		}
		break;
	case XS_INSTANCE_KIND:
		if (!(theSlot->flag & XS_MARK_FLAG))
			fxMarkInstance(the, theSlot, fxMarkValue);
		break;
		
	case XS_ARGUMENTS_SLOPPY_KIND:
	case XS_ARGUMENTS_STRICT_KIND:
	case XS_ARRAY_KIND:
	case XS_STACK_KIND:
		if ((aSlot = theSlot->value.array.address)) {
			txIndex aLength = (((txChunk*)(((txByte*)aSlot) - sizeof(txChunk)))->size) / sizeof(txSlot);
			while (aLength) {
				fxMarkValue(the, aSlot);
				aSlot++;
				aLength--;
			}
			mxMarkChunk(theSlot->value.array.address);
		}
		break;
	case XS_ARRAY_BUFFER_KIND:
		if (theSlot->value.arrayBuffer.address)
			mxMarkChunk(theSlot->value.arrayBuffer.address);
		break;
	case XS_CALLBACK_KIND:
		if (theSlot->value.callback.IDs)
			mxMarkChunk(theSlot->value.callback.IDs);
		break;
	case XS_CODE_KIND:
		mxMarkChunk(theSlot->value.code.address);
		/* continue */
	case XS_CODE_X_KIND:
		aSlot = theSlot->value.code.closures;
		if (aSlot && !(aSlot->flag & XS_MARK_FLAG))
			fxMarkInstance(the, aSlot, fxMarkValue);
		break;
	case XS_GLOBAL_KIND:
		mxMarkChunk(theSlot->value.table.address);
		break;
	case XS_HOST_KIND:
		if (theSlot->value.host.data) {
			if ((theSlot->flag & XS_HOST_HOOKS_FLAG) && (theSlot->value.host.variant.hooks->marker))
				(*theSlot->value.host.variant.hooks->marker)(the, theSlot->value.host.data, fxMarkValue);
			if (theSlot->flag & XS_HOST_CHUNK_FLAG)
				mxMarkChunk(theSlot->value.host.data);
		}
		break;
	case XS_PROXY_KIND:
		aSlot = theSlot->value.proxy.handler;
		if (aSlot && !(aSlot->flag & XS_MARK_FLAG))
			fxMarkInstance(the, aSlot, fxMarkValue);
		aSlot = theSlot->value.proxy.target;
		if (aSlot && !(aSlot->flag & XS_MARK_FLAG))
			fxMarkInstance(the, aSlot, fxMarkValue);
		break;
	case XS_REGEXP_KIND:
		if (theSlot->value.regexp.code)
			mxMarkChunk(theSlot->value.regexp.code);
		if (theSlot->value.regexp.data)
			mxMarkChunk(theSlot->value.regexp.data);
		break;
		
	case XS_ACCESSOR_KIND:
		aSlot = theSlot->value.accessor.getter;
		if (aSlot && !(aSlot->flag & XS_MARK_FLAG))
			fxMarkInstance(the, aSlot, fxMarkValue);
		aSlot = theSlot->value.accessor.setter;
		if (aSlot && !(aSlot->flag & XS_MARK_FLAG))
			fxMarkInstance(the, aSlot, fxMarkValue);
		break;
	case XS_HOME_KIND:
		aSlot = theSlot->value.home.object;
		if (aSlot && !(aSlot->flag & XS_MARK_FLAG))
			fxMarkInstance(the, aSlot, fxMarkValue);
		aSlot = theSlot->value.home.module;
		if (aSlot && !(aSlot->flag & XS_MARK_FLAG))
			fxMarkInstance(the, aSlot, fxMarkValue);
		break;
	case XS_EXPORT_KIND:
		aSlot = theSlot->value.export.closure;
		if (aSlot && !(aSlot->flag & XS_MARK_FLAG)) {
			aSlot->flag |= XS_MARK_FLAG; 
			fxMarkValue(the, aSlot);
		}
		aSlot = theSlot->value.export.module;
		if (aSlot && !(aSlot->flag & XS_MARK_FLAG))
			fxMarkInstance(the, aSlot, fxMarkValue);
		break;
	case XS_KEY_KIND:
		if (theSlot->value.key.string)
			mxMarkChunk(theSlot->value.key.string);
		break;
		
	case XS_LIST_KIND:
		aSlot = theSlot->value.list.first;
		while (aSlot) {
			if (!(aSlot->flag & XS_MARK_FLAG)) {
				aSlot->flag |= XS_MARK_FLAG;
				fxMarkValue(the, aSlot);
			}
			aSlot = aSlot->next;
		}
		break;
	case XS_MAP_KIND:
	case XS_SET_KIND:
		{
			txSlot** anAddress = theSlot->value.table.address;
			txInteger aLength = theSlot->value.table.length;
			while (aLength) {
				aSlot = *anAddress;
				while (aSlot) {
					aSlot->flag |= XS_MARK_FLAG; 
					aSlot = aSlot->next;
				}
				anAddress++;
				aLength--;
			}
		}
		mxMarkChunk(theSlot->value.table.address);
		break;
	case XS_WEAK_MAP_KIND:
		mxMarkChunk(theSlot->value.table.address);
		theSlot->value.table.address[theSlot->value.table.length] = the->firstWeakMapTable;
		the->firstWeakMapTable = theSlot;
		break;
	case XS_WEAK_SET_KIND:
		mxMarkChunk(theSlot->value.table.address);
		theSlot->value.table.address[theSlot->value.table.length] = the->firstWeakSetTable;
		the->firstWeakSetTable = theSlot;
		break;
		
	case XS_HOST_INSPECTOR_KIND:
		aSlot = theSlot->value.hostInspector.cache;
		if (!(aSlot->flag & XS_MARK_FLAG))
			fxMarkInstance(the, aSlot, fxMarkValue);
		break;	
	}
}

void fxMarkWeakMapTable(txMachine* the, txSlot* table, void (*theMarker)(txMachine*, txSlot*)) 
{
	txSlot** address = table->value.table.address;
	txInteger modulo = table->value.table.length;
	while (modulo) {
		txSlot** link = address;
		txSlot* entry;
		while ((entry = *link)) {
			txSlot* result = entry->value.entry.slot;
			if (result->value.reference->flag & XS_MARK_FLAG) {
				entry->flag |= XS_MARK_FLAG;
				result->flag |= XS_MARK_FLAG;
				result = result->next;
				result->flag |= XS_MARK_FLAG;
				(*theMarker)(the, result);
				link = &(entry->next);
			}
			else
				*link = entry->next;
		}
		address++;
		modulo--;
	}
}

void fxMarkWeakSetTable(txMachine* the, txSlot* table) 
{
	txSlot** address = table->value.table.address;
	txInteger modulo = table->value.table.length;
	while (modulo) {
		txSlot** link = address;
		txSlot* entry;
		while ((entry = *link)) {
			txSlot* result = entry->value.entry.slot;
			if (result->value.reference->flag & XS_MARK_FLAG) {
				entry->flag |= XS_MARK_FLAG;
				result->flag |= XS_MARK_FLAG;
				link = &(entry->next);
			}
			else
				*link = entry->next;
		}
		address++;
		modulo--;
	}
}

void fxMarkWeakTables(txMachine* the, void (*theMarker)(txMachine*, txSlot*)) 
{
	txSlot* table;
	txSlot** address;
	address = &the->firstWeakMapTable;
	while ((table = *address)) {
		fxMarkWeakMapTable(the, table, theMarker);
		*address = C_NULL;
		address = &(table->value.table.address[table->value.table.length]);
	}
	address = &the->firstWeakSetTable;
	while ((table = *address)) {
		fxMarkWeakSetTable(the, table);
		*address = C_NULL;
		address = &(table->value.table.address[table->value.table.length]);
	}
}

void* fxNewChunk(txMachine* the, txSize theSize)
{
	txBlock* aBlock;
	txByte* aData;
	txBoolean once = 1;
	
#if mxStress
	if (gxStress) {
		fxCollect(the, 1);
		once = 0;
	}
#endif
    //if (theSize > 1000)
    //	fprintf(stderr, "# fxNewChunk %ld\n", theSize);
	theSize = mxRoundSize(theSize) + sizeof(txChunk);
again:
	aBlock = the->firstBlock;
	while (aBlock) {
		if ((aBlock->current + theSize) <= aBlock->limit) {
			aData = aBlock->current;
			((txChunk*)aData)->size = theSize;
			((txChunk*)aData)->temporary = C_NULL;
			aBlock->current += theSize;
			the->currentChunksSize += theSize;
			if (the->peakChunksSize < the->currentChunksSize)
				the->peakChunksSize = the->currentChunksSize;
			return aData + sizeof(txChunk);
		}
		aBlock = aBlock->nextBlock;
	}
	if (once) {
		fxCollect(the, 1);
		once = 0;
	}
	else {
		fxGrowChunks(the, theSize);
	}
	goto again;
	
	return C_NULL;
}

txSlot* fxNewSlot(txMachine* the) 
{
	txSlot* aSlot;
	txBoolean once = 1;
	
#if mxStress
	if (gxStress) {
		fxCollect(the, 1);
		once = 0;
	}
#endif
again:
	aSlot = the->freeHeap;
	if (aSlot) {
		the->freeHeap = aSlot->next;
		aSlot->next = C_NULL;
		aSlot->ID = XS_NO_ID;
		aSlot->flag = XS_NO_FLAG;
		the->currentHeapCount++;
		if (the->peakHeapCount < the->currentHeapCount)
			the->peakHeapCount = the->currentHeapCount;
		return aSlot;
	}
	if (once) {
		txBoolean wasThrashing = ((the->collectFlag & XS_TRASHING_FLAG) != 0), isThrashing;

		fxCollect(the, 0);

		isThrashing = ((the->collectFlag & XS_TRASHING_FLAG) != 0);
		if (wasThrashing && isThrashing)
			fxGrowSlots(the, !(the->collectFlag & XS_SKIPPED_COLLECT_FLAG) ? the->minimumHeapCount : 64);

		once = 0;
	}
	else
		fxGrowSlots(the, !(the->collectFlag & XS_SKIPPED_COLLECT_FLAG) ? the->minimumHeapCount : 64);
	goto again;
	return C_NULL;
}

void* fxRenewChunk(txMachine* the, void* theData, txSize theSize)
{
	txByte* aData = ((txByte*)theData) - sizeof(txChunk);
	txChunk* aChunk = (txChunk*)aData;
	txBlock* aBlock = the->firstBlock;
	theSize = mxRoundSize(theSize) + sizeof(txChunk); 
	
	if (aChunk->size == theSize) {
	#ifdef mxNever
		gxRenewChunkCases[0]++;
	#endif
		return theData;
	}
	aData += aChunk->size;
	theSize -= aChunk->size;
	while (aBlock) {
		if (aBlock->current == aData) {
			if (aData + theSize <= aBlock->limit) {
				aBlock->current += theSize;
				aChunk->size += theSize;
				the->currentChunksSize += theSize;
				if (the->peakChunksSize < the->currentChunksSize)
					the->peakChunksSize = the->currentChunksSize;
			#ifdef mxNever
				gxRenewChunkCases[1]++;
			#endif
				return theData;
			}
			else {
			#ifdef mxNever
				gxRenewChunkCases[3]++;
			#endif
				return C_NULL;
			}
		}
		aBlock = aBlock->nextBlock;
	}
	if (theSize < 0) {
		the->currentChunksSize += theSize;
		if (the->peakChunksSize < the->currentChunksSize)
			the->peakChunksSize = the->currentChunksSize;
		aChunk->size += theSize;
		aData += theSize;
		((txChunk*)aData)->size = -theSize;
		((txChunk*)aData)->temporary = C_NULL;
	#ifdef mxNever
		gxRenewChunkCases[2]++;
	#endif
		return theData;
	}
#ifdef mxNever
	gxRenewChunkCases[3]++;
#endif
	return C_NULL;
}

void fxShare(txMachine* the)
{
	txID aliasCount = 0;
	txSlot* aSlot;
	txSlot* bSlot;
	txSlot* cSlot;

	fxCollect(the, 1);
	aSlot = the->firstHeap;
	while (aSlot) {
		bSlot = aSlot + 1;
		cSlot = aSlot->value.reference;
		while (bSlot < cSlot) {
			if (bSlot->kind == XS_INSTANCE_KIND) {
				bSlot->ID = aliasCount++;
				bSlot->flag |= XS_MARK_FLAG; 
			}
			
			bSlot++;
		}
		aSlot = aSlot->next;
	}
	the->aliasCount = aliasCount;
	/*
	fxReport(the, "# Share\n");
	fxReport(the, "# \tSlots: %ld\n", the->currentHeapCount);
	fxReport(the, "# \t\tSymbols: %ld\n", the->keyIndex);
	fxReport(the, "# \t\tInstances: %ld\n", aliasCount);
	fxReport(the, "# \tChunks: %ld bytes\n", the->currentChunksSize);
	*/
}

void fxSweep(txMachine* the)
{
	txSize aTotal;
	txBlock* aBlock;
	txByte* mByte;
	txByte* nByte;
	txByte* pByte;
	txSize aSize;
	txByte** aCodeAddress;
	txSlot* aSlot;
	txSlot* bSlot;
	txSlot* cSlot;
	txSlot* freeSlot;
	txJump* jump;

#ifdef mxNever
	startTime(&gxSweepChunkTime);
#endif

	aTotal = 0;
	aBlock = the->firstBlock;
	while (aBlock) {
		mByte = ((txByte*)aBlock) + sizeof(txBlock);
		nByte = aBlock->current;
		pByte = mByte;
		while (mByte < nByte) {
			aSize = ((txChunk*)mByte)->size;
			if (aSize & mxChunkFlag) {
				aSize &= ~mxChunkFlag;
				((txChunk*)mByte)->size = aSize;
				((txChunk*)mByte)->temporary = pByte;
				pByte += aSize;
				aTotal += aSize;
			}
			mByte += aSize;
		}	
		aBlock->temporary = pByte;
		aBlock = aBlock->nextBlock;
	}
	the->currentChunksSize = aTotal;

	aCodeAddress = &(the->code);
	aSlot = the->frame;
	while (aSlot) {
		mxCheck(the, aSlot->kind == XS_FRAME_KIND);
		if ((aSlot->flag & XS_C_FLAG) == 0) {
			bSlot = (aSlot + 3)->value.reference->next;
			if (bSlot->kind == XS_CODE_KIND) {
				mByte = bSlot->value.code.address;
				pByte = (txByte*)(((txChunk*)(mByte - sizeof(txChunk)))->temporary);
				if (pByte) {
					pByte += sizeof(txChunk);
					aSize = pByte - mByte;
					*aCodeAddress = *aCodeAddress + aSize;
				}
			}
		}
		else {
			mByte = *aCodeAddress;
			if (mByte) {
				pByte = (txByte*)(((txChunk*)(mByte - sizeof(txChunk)))->temporary);
				if (pByte)
					*aCodeAddress = pByte + sizeof(txChunk);
			}
		}
		aCodeAddress = &(aSlot->value.frame.code);
		aSlot = aSlot->next;
	}
	
	jump = the->firstJump;
	while (jump) {
		if (jump->flag) {
			aSlot = jump->frame;
			bSlot = (aSlot + 3)->value.reference->next;
			if (bSlot->kind == XS_CODE_KIND) {
				mByte = bSlot->value.code.address;
				pByte = (txByte*)(((txChunk*)(mByte - sizeof(txChunk)))->temporary);
				if (pByte) {
					pByte += sizeof(txChunk);
					jump->code += pByte - mByte;
				}
			}
		}
		else {
			mByte = jump->code;
			if (mByte) {
				pByte = (txByte*)(((txChunk*)(mByte - sizeof(txChunk)))->temporary);
				if (pByte)
					jump->code = pByte + sizeof(txChunk);
			}
		}
		jump = jump->nextJump;
	}
	
	aSlot = the->stack;
	while (aSlot < the->stackTop) {
		fxSweepValue(the, aSlot);
		aSlot++;
	}
	aSlot = the->cRoot;
	while (aSlot) {
		fxSweepValue(the, aSlot);
		aSlot = aSlot->next;
	}

#ifdef mxNever
	stopTime(&gxSweepChunkTime);
	startTime(&gxSweepSlotTime);
#endif
	
	aTotal = 0;
	freeSlot = C_NULL;
	aSlot = the->firstHeap;
	while (aSlot) {
		bSlot = aSlot + 1;
		cSlot = aSlot->value.reference;
		while (bSlot < cSlot) {
			if (bSlot->flag & XS_MARK_FLAG) {
				bSlot->flag &= ~XS_MARK_FLAG; 
				fxSweepValue(the, bSlot);
				aTotal++;
			}
			else {
				if (bSlot->kind == XS_HOST_KIND) {
					if (bSlot->flag & XS_HOST_HOOKS_FLAG) {
						if (bSlot->value.host.variant.hooks->destructor)
							(*(bSlot->value.host.variant.hooks->destructor))(bSlot->value.host.data);
					}
					else if (bSlot->value.host.variant.destructor)
						(*(bSlot->value.host.variant.destructor))(bSlot->value.host.data);
				}
			#if mxInstrument
				if (bSlot->kind == XS_MODULE_KIND)
					the->loadedModulesCount--;
			#endif
			#if mxFill
				c_memset(bSlot, 0xFF, sizeof(txSlot));
			#endif
				bSlot->kind = XS_UNDEFINED_KIND;
				bSlot->next = freeSlot;
				freeSlot = bSlot;
			}
			bSlot++;
		}
		aSlot = aSlot->next;
	}
	the->currentHeapCount = aTotal;
	the->freeHeap = freeSlot;
	
#ifdef mxNever
	stopTime(&gxSweepSlotTime);
	startTime(&gxCompactChunkTime);
#endif

	aBlock = the->firstBlock;
	while (aBlock) {
		mByte = ((txByte*)aBlock) + sizeof(txBlock);
		nByte = aBlock->current;
		while (mByte < nByte) {
			aSize = ((txChunk*)mByte)->size;
			if ((pByte = ((txChunk*)mByte)->temporary)) {
				((txChunk*)mByte)->temporary = C_NULL;
				if (pByte != mByte)
					c_memmove(pByte, mByte, aSize);
			}
			mByte += aSize;
		}	
	#if mxFill
		c_memset(aBlock->temporary, 0xFF, aBlock->current - aBlock->temporary);
	#endif
		aBlock->current = aBlock->temporary;
		aBlock->temporary = C_NULL;
		aBlock = aBlock->nextBlock;
	}
	
#ifdef mxNever
	stopTime(&gxCompactChunkTime);
#endif
}

void fxSweepValue(txMachine* the, txSlot* theSlot)
{
	txSlot* aSlot;
	txByte* data;
#define mxSweepChunk(_THE_DATA, _THE_DATA_TYPE) \
	if ((data = (txByte*)(((txChunk*)(((txByte*)(_THE_DATA)) - sizeof(txChunk)))->temporary))) \
		((_THE_DATA)) = (_THE_DATA_TYPE)(data + sizeof(txChunk))

	switch (theSlot->kind) {
	case XS_STRING_KIND:
		mxSweepChunk(theSlot->value.string, txString);
		break;

	case XS_ARGUMENTS_SLOPPY_KIND:
	case XS_ARGUMENTS_STRICT_KIND:
	case XS_ARRAY_KIND:
	case XS_STACK_KIND:
		if ((aSlot = theSlot->value.array.address)) {
			txIndex aLength = (((txChunk*)(((txByte*)aSlot) - sizeof(txChunk)))->size) / sizeof(txSlot);
			while (aLength) {
				fxSweepValue(the, aSlot);
				aSlot++;
				aLength--;
			}
			mxSweepChunk(theSlot->value.array.address, txSlot*);
		}
		break;
	case XS_ARRAY_BUFFER_KIND:
		if (theSlot->value.arrayBuffer.address)
			mxSweepChunk(theSlot->value.arrayBuffer.address, txByte*);
		break;
	case XS_CALLBACK_KIND:
		if (theSlot->value.callback.IDs)
			mxSweepChunk(theSlot->value.callback.IDs, txID*);
		break;
	case XS_CODE_KIND:
		mxSweepChunk(theSlot->value.code.address, txByte*);
		break;
	case XS_GLOBAL_KIND:
		mxSweepChunk(theSlot->value.table.address, txSlot**);
		break;
	case XS_HOST_KIND:
		if (theSlot->value.host.data) {
			if ((theSlot->flag & XS_HOST_HOOKS_FLAG) && (theSlot->value.host.variant.hooks->sweeper))
				(*theSlot->value.host.variant.hooks->sweeper)(the, theSlot->value.host.data, fxSweepValue);
			if (theSlot->flag & XS_HOST_CHUNK_FLAG)
				mxSweepChunk(theSlot->value.host.data, void*);
		}
		break;
	case XS_REGEXP_KIND:
		if (theSlot->value.regexp.code)
			mxSweepChunk(theSlot->value.regexp.code, void*);
		if (theSlot->value.regexp.data)
			mxSweepChunk(theSlot->value.regexp.data, void*);
		break;
	case XS_KEY_KIND:
		if (theSlot->value.key.string)
			mxSweepChunk(theSlot->value.key.string, txString);
		break;
	case XS_MAP_KIND:
	case XS_SET_KIND:
	case XS_WEAK_MAP_KIND:
	case XS_WEAK_SET_KIND:
		mxSweepChunk(theSlot->value.table.address, txSlot**);
		break;
	}
}
