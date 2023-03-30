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
#ifndef mxNoChunks
#define mxNoChunks 0
#endif
#ifndef mxPoisonSlots
#define mxPoisonSlots 0
#endif

#if mxPoisonSlots
#include <sanitizer/asan_interface.h>
#endif

#if mxStress
int gxStress = 0;

static int fxShouldStress()
{
	if (!gxStress)
		return 0;

	if (gxStress > 0)
		return 1;

	gxStress += 1;
	return 0 == gxStress;
}
#endif

#define mxChunkFlag 0x80000000

static txSize fxAdjustChunkSize(txMachine* the, txSize size);
static void* fxCheckChunk(txMachine* the, txChunk* chunk, txSize size, txSize offset);
static void* fxFindChunk(txMachine* the, txSize size, txBoolean *once);
static void* fxGrowChunk(txMachine* the, txSize size);
static void* fxGrowChunks(txMachine* the, txSize theSize); 
/* static */ void fxGrowSlots(txMachine* the, txSize theCount); 
static void fxMark(txMachine* the, void (*theMarker)(txMachine*, txSlot*));
static void fxMarkFinalizationRegistry(txMachine* the, txSlot* registry);
static void fxMarkInstance(txMachine* the, txSlot* theCurrent, void (*theMarker)(txMachine*, txSlot*));
static void fxMarkReference(txMachine* the, txSlot* theSlot);
static void fxMarkValue(txMachine* the, txSlot* theSlot);
static void fxMarkWeakStuff(txMachine* the);
static void fxSweep(txMachine* the);
static void fxSweepValue(txMachine* the, txSlot* theSlot);

#ifdef mxNever

long gxRenewChunkCases[4] = { 0, 0, 0, 0 };

typedef struct sxSample txSample;
struct sxSample {
	txNumber time;
	txNumber duration;
	long count;
	char* label;
};

void reportTime(txSample* theSample) 
{
	txNumber duration = theSample->duration;
	txNumber minutes;
	txNumber seconds;

	minutes = c_floor(duration / 60000000);
	duration -= minutes * 60000000;
	seconds = c_floor(duration / 1000000);
	duration -= seconds * 1000000;
	duration = c_floor(duration / 1000);
	fprintf(stderr, "%s * %ld = %ld:%02ld.%03ld\n", theSample->label, theSample->count, 
			(long)minutes, (long)seconds, (long)duration);
}

void startTime(txSample* theSample) 
{
	c_timeval tv;
	c_gettimeofday(&tv, NULL);
	theSample->time = ((txNumber)(tv.tv_sec) * 1000000.0) + ((txNumber)tv.tv_usec);
}

void stopTime(txSample* theSample) 
{
	c_timeval tv;
	txNumber time;
	c_gettimeofday(&tv, NULL);
	time = ((txNumber)(tv.tv_sec) * 1000000.0) + ((txNumber)tv.tv_usec);
	theSample->duration += time - theSample->time;
	theSample->count++;
}

txSample gxLifeTime = { 0, 0, 0, "life" };
txSample gxMarkTime = { 0, 0, 0, "mark" };
txSample gxSweepChunkTime = { 0, 0, 0, "sweep chunk" };
txSample gxSweepSlotTime = { 0, 0, 0, "sweep slot" };
txSample gxCompactChunkTime = { 0, 0, 0, "compact chunk" };

#endif

txSize fxAddChunkSizes(txMachine* the, txSize a, txSize b)
{
	txSize c;
#if __has_builtin(__builtin_add_overflow)
	if (__builtin_add_overflow(a, b, &c)) {
#else
	c = a + b;
	if (((a ^ c) & (b ^ c)) < 0) {
#endif
		fxAbort(the, XS_NOT_ENOUGH_MEMORY_EXIT);
	}
	return c;
}

txSize fxAdjustChunkSize(txMachine* the, txSize size)
{
	txSize adjust = sizeof(txChunk);
	txSize modulo = size & (sizeof(size_t) - 1);
	if (modulo)
		adjust += sizeof(size_t) - modulo;
	return fxAddChunkSizes(the, size, adjust);
}

void fxAllocate(txMachine* the, txCreation* theCreation)
{
#ifdef mxNever
	startTime(&gxLifeTime);
#endif
#if mxStress
	gxStress = 0;
#endif

	the->currentChunksSize = 0;
	the->peakChunksSize = 0;
	the->maximumChunksSize = 0;
	the->minimumChunksSize = theCreation->incrementalChunkSize;
	
	the->currentHeapCount = 0;
	the->peakHeapCount = 0;
	the->maximumHeapCount = 0;
	the->minimumHeapCount = theCreation->incrementalHeapCount;
	
	the->firstBlock = C_NULL;
	the->firstHeap = C_NULL;

#if mxNoChunks
#else
	fxGrowChunks(the, theCreation->initialChunkSize);
#endif

	the->stackBottom = fxAllocateSlots(the, theCreation->stackCount);
	the->stackTop = the->stackBottom + theCreation->stackCount;
	the->stackPrototypes = the->stackTop;
	the->stack = the->stackTop;
#ifdef mxInstrument
	the->stackPeak = the->stackTop;
#endif

	fxGrowSlots(the, theCreation->initialHeapCount);

	the->keyCount = (txID)theCreation->initialKeyCount;
	the->keyDelta = (txID)theCreation->incrementalKeyCount;
	the->keyIndex = 0;
	the->keyArray = (txSlot **)c_malloc_uint32(theCreation->initialKeyCount * sizeof(txSlot*));
	if (!the->keyArray)
		fxAbort(the, XS_NOT_ENOUGH_MEMORY_EXIT);

	the->nameModulo = theCreation->nameModulo;
	the->nameTable = (txSlot **)c_malloc_uint32(theCreation->nameModulo * sizeof(txSlot*));
	if (!the->nameTable)
		fxAbort(the, XS_NOT_ENOUGH_MEMORY_EXIT);

	the->symbolModulo = theCreation->symbolModulo;
	the->symbolTable = (txSlot **)c_malloc_uint32(theCreation->symbolModulo * sizeof(txSlot*));
	if (!the->symbolTable)
		fxAbort(the, XS_NOT_ENOUGH_MEMORY_EXIT);

	the->stackLimit = fxCStackLimit();

	the->cRoot = C_NULL;
	the->parserBufferSize = theCreation->parserBufferSize;
	the->parserTableModulo = theCreation->parserTableModulo;
}

void* fxCheckChunk(txMachine* the, txChunk* chunk, txSize size, txSize offset)
{
	if (chunk) {
		txByte* data = (txByte*)chunk;
#if mxNoChunks
		chunk->size = size;
		the->currentChunksSize += size;
#else
		txSize capacity = (txSize)(chunk->temporary - data);
	#ifdef mxSnapshot
		#if INTPTR_MAX == INT64_MAX
			chunk->dummy = 0;
		#endif
	#ifdef mxSnapshotRandomInit
		arc4random_buf(data + sizeof(txChunk), offset);
	#endif		
		offset += sizeof(txChunk);
		c_memset(data + offset, 0, capacity - offset);
	#endif
		chunk->size = size;
		the->currentChunksSize += capacity;
#endif
		if (the->peakChunksSize < the->currentChunksSize)
			the->peakChunksSize = the->currentChunksSize;
		return data + sizeof(txChunk);
	}
	fxReport(the, "# Chunk allocation: failed for %ld bytes\n", (long)size);
	fxAbort(the, XS_NOT_ENOUGH_MEMORY_EXIT);
	return C_NULL;
}

void fxCheckCStack(txMachine* the)
{
    char x;
    char *stack = &x;
	if (stack <= the->stackLimit) {
		fxAbort(the, XS_STACK_OVERFLOW_EXIT);
	}
}

void fxCollect(txMachine* the, txFlag theFlag)
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
	the->collectFlag |= theFlag & XS_ORGANIC_FLAG;

	if (theFlag & XS_COMPACT_FLAG) {
		fxMark(the, fxMarkValue);
		fxMarkWeakStuff(the);
		fxSweep(the);
	}
	else {
		fxMark(the, fxMarkReference);
		fxMarkWeakStuff(the);
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
					bSlot->kind = XS_UNDEFINED_KIND;
					bSlot->next = freeSlot;
				#if mxPoisonSlots
					ASAN_POISON_MEMORY_REGION(&bSlot->value, sizeof(bSlot->value));
				#endif
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
	
	if (theFlag) 
		the->collectFlag &= ~XS_TRASHING_FLAG;
	else  {
		if ((the->maximumHeapCount - the->currentHeapCount) < the->minimumHeapCount)
				the->collectFlag |= XS_TRASHING_FLAG;
			else
				the->collectFlag &= ~XS_TRASHING_FLAG;
	}
	the->collectFlag &= ~XS_ORGANIC_FLAG;
	
#if mxReport
	if (theFlag)
		fxReport(the, "# Chunk collection: reserved %ld used %ld peak %ld bytes\n", 
			(long)the->maximumChunksSize, (long)the->currentChunksSize, (long)the->peakChunksSize);
	aCount = 0;
	aSlot = the->firstHeap;
	while (aSlot) {
		aCount++;
		aSlot = aSlot->next;
	}
	fxReport(the, "# Slot collection: reserved %ld used %ld peak %ld bytes %d\n",
		(long)((the->maximumHeapCount - aCount) * sizeof(txSlot)),
		(long)(the->currentHeapCount * sizeof(txSlot)),
		(long)(the->peakHeapCount * sizeof(txSlot)),
		the->collectFlag & XS_TRASHING_FLAG);
#endif
#ifdef mxInstrument
	the->garbageCollectionCount++;
#endif
#if defined(mxInstrument) || defined(mxProfile)
	fxCheckProfiler(the, C_NULL);
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

void* fxFindChunk(txMachine* the, txSize size, txBoolean *once)
{
	txBlock* block;
	txChunk* chunk;
#if mxStress
	if (fxShouldStress()) {
		if (*once) {
			fxCollect(the, XS_COMPACT_FLAG | XS_ORGANIC_FLAG);
			*once = 0;
		}
	}
#endif
#if mxNoChunks
	chunk = c_malloc(size);
	chunk->size = size;
	chunk->temporary = (txByte*)the->firstBlock;
	the->firstBlock = (txBlock*)chunk;
	return chunk;
#endif
again:
	block = the->firstBlock;
	while (block) {
		if ((block->current + size) <= block->limit) {
			chunk = (txChunk*)(block->current);
			block->current += size;
			chunk->temporary = block->current;
			return chunk;
		}
		block = block->nextBlock;
	}
	if (*once) {
		fxCollect(the, XS_COMPACT_FLAG | XS_ORGANIC_FLAG);
		*once = 0;
		goto again;
	}
	return C_NULL;
}

void fxFree(txMachine* the) 
{
	txSlot* aHeap;

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
	
	while (the->firstHeap) {
		aHeap = the->firstHeap;
		the->firstHeap = aHeap->next;
		fxFreeSlots(the, aHeap);
	}
	the->firstHeap = C_NULL;

	if (the->stackBottom)
		fxFreeSlots(the, the->stackBottom);
	the->stackBottom = C_NULL;
	the->stackTop = C_NULL;
	the->stackPrototypes = C_NULL;
	the->stack = C_NULL;
	
#if mxNoChunks
	{
		txChunk** address;
		txChunk* chunk;
		address = (txChunk**)&(the->firstBlock);
		while ((chunk = *address)) {
			*address = (txChunk*)(chunk->temporary);
			c_free(chunk);
		}
	}
#else
	{
		txBlock* aBlock;
		while (the->firstBlock) {
			aBlock = the->firstBlock;
			the->firstBlock = aBlock->nextBlock;
			fxFreeChunks(the, aBlock);
		}
		the->firstBlock = C_NULL;
	}
#endif
	
#ifdef mxNever
	stopTime(&gxLifeTime);
	reportTime(&gxLifeTime);
	fprintf(stderr, "chunk: %d bytes\n", the->maximumChunksSize);
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

void* fxGrowChunk(txMachine* the, txSize size) 
{
	txBlock* block = fxGrowChunks(the, size);
	txChunk* chunk = C_NULL;
	if (block) {
		chunk = (txChunk*)(block->current);
		block->current += size;
		chunk->temporary = block->current;
	}
	return chunk;
}

void* fxGrowChunks(txMachine* the, txSize size) 
{
	txByte* buffer;
	txBlock* block = C_NULL;

	if (!the->minimumChunksSize && the->firstBlock) {
		fxReport(the, "# Chunk allocation: %d bytes failed in fixed size heap\n", size);
		fxAbort(the, XS_NOT_ENOUGH_MEMORY_EXIT);
	}

	if ((the->firstBlock != C_NULL) && (!(the->collectFlag & XS_SKIPPED_COLLECT_FLAG))) {
		txSize modulo = size % (the->minimumChunksSize ? the->minimumChunksSize : 16);
		if (modulo)
			size = fxAddChunkSizes(the, size, the->minimumChunksSize - modulo);
	}
	size = fxAddChunkSizes(the, size, sizeof(txBlock));
	buffer = fxAllocateChunks(the, size);
	if (buffer) {
	#ifdef mxSnapshot
		c_memset(buffer, 0, size);
	#endif
		if ((the->firstBlock != C_NULL) && (the->firstBlock->limit == buffer)) {
			the->firstBlock->limit += size;
			block = the->firstBlock;
		}
		else {
			block = (txBlock*)buffer;
			block->nextBlock = the->firstBlock;
			block->current = buffer + sizeof(txBlock);
			block->limit = buffer + size;
			block->temporary = C_NULL;
			the->firstBlock = block;
			size -= sizeof(txBlock);
		}
		the->maximumChunksSize += size;
	#if mxReport
		fxReport(the, "# Chunk allocation: reserved %ld used %ld peak %ld bytes\n", 
			(long)the->maximumChunksSize, (long)the->currentChunksSize, (long)the->peakChunksSize);
	#endif
	}
	return block;
}

void fxGrowKeys(txMachine* the, txID theCount) 
{
	if (the->keyDelta > 0) {
		txID keyDelta = (theCount > the->keyDelta) ? theCount : the->keyDelta;
		txID keyCount = (the->keyCount + keyDelta) - the->keyOffset;
		txSlot** keyArray = c_realloc(the->keyArray, keyCount * sizeof(txSlot*));
		if (keyArray == C_NULL)
			fxAbort(the, XS_NOT_ENOUGH_MEMORY_EXIT);
		the->keyArray = keyArray;
		the->keyCount = keyCount + the->keyOffset;
	}
	else 
		fxAbort(the, XS_NO_MORE_KEYS_EXIT);
}

void fxGrowSlots(txMachine* the, txSize theCount) 
{
	txSlot* aHeap;
	txSlot* aSlot;

	aHeap = fxAllocateSlots(the, theCount);
	if (!aHeap) {
		fxReport(the, "# Slot allocation: failed for %ld bytes\n", theCount * sizeof(txSlot));
		fxAbort(the, XS_NOT_ENOUGH_MEMORY_EXIT);
	}
	if ((void *)-1 == aHeap)
		return;
		
	if (the->firstHeap && (the->firstHeap->value.reference == aHeap)) {
		the->firstHeap->value.reference = aHeap + theCount;
		the->maximumHeapCount += theCount;		
		theCount -= 1;
		aSlot = aHeap;
	}
	else if ((aHeap + theCount) == the->firstHeap) {
		*aHeap = *(the->firstHeap);
		the->maximumHeapCount += theCount;
		theCount -= 1;
		the->firstHeap = aHeap;
		aSlot = aHeap + 1;
	}
	else {
		the->maximumHeapCount += theCount - 1;
		aHeap->next = the->firstHeap;
		aHeap->ID = 0;
		aHeap->flag = 0;
		aHeap->kind = 0;
		aHeap->value.reference = aHeap + theCount;
		theCount -= 2;
		the->firstHeap = aHeap;
		aSlot = aHeap + 1;
	}
    while (theCount--) {
		txSlot* next = aSlot + 1;
		aSlot->next = next;
		aSlot->flag = XS_NO_FLAG;
		aSlot->kind = XS_UNDEFINED_KIND;
	#if mxPoisonSlots
		ASAN_POISON_MEMORY_REGION(&aSlot->value, sizeof(aSlot->value));
	#endif
        aSlot = next;
    }
	aSlot->next = the->freeHeap;
	aSlot->flag = XS_NO_FLAG;
	aSlot->kind = XS_UNDEFINED_KIND;
#if mxPoisonSlots
	ASAN_POISON_MEMORY_REGION(&aSlot->value, sizeof(aSlot->value));
#endif
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
	
	anArray = the->aliasArray;
	anIndex = the->aliasCount;
	while (anIndex) {
		if ((aSlot = *anArray)) {
			if (!(aSlot->flag & XS_MARK_FLAG)) {
				(*theMarker)(the, aSlot);
				aSlot->flag |= XS_MARK_FLAG;
			}
		}
		anArray++;
		anIndex--;
	}
	
	aSlot = the->stackTop;
	while (aSlot > the->stack) {
        aSlot--;
		(*theMarker)(the, aSlot);
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

void fxMarkFinalizationRegistry(txMachine* the, txSlot* registry) 
{
	txSlot* slot = registry->value.finalizationRegistry.callback->next;
	txSlot* instance;
	while (slot) {
		slot = slot->next;
		if (slot) {
			instance = slot->value.finalizationCell.target;
			if (instance && !(instance->flag & XS_MARK_FLAG)) {
				slot->value.finalizationCell.target = C_NULL;
				registry->value.finalizationRegistry.flags |= XS_FINALIZATION_REGISTRY_CHANGED;
			}
			instance = slot->value.finalizationCell.token;
			if (instance && !(instance->flag & XS_MARK_FLAG))
				slot->value.finalizationCell.token = C_NULL;
			slot = slot->next;
		}
	}
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
					if (aTemporary && !(aTemporary->flag & XS_MARK_FLAG)) {
						aProperty->value.instance.prototype = theCurrent;
						theCurrent = aTemporary;
						theCurrent->value.instance.garbage = aProperty;
						aProperty = theCurrent;
					}
					else
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
					
				case XS_PROXY_KIND:
					aTemporary = aProperty->value.proxy.handler;
					if (aTemporary && !(aTemporary->flag & XS_MARK_FLAG)) {
						aProperty->flag |= XS_INSPECTOR_FLAG;
						aProperty->value.proxy.handler = theCurrent;
						theCurrent = aTemporary;
						theCurrent->value.instance.garbage = aProperty;
						aProperty = theCurrent;
					}
					else {
						aTemporary = aProperty->value.proxy.target;
						if (aTemporary && !(aTemporary->flag & XS_MARK_FLAG)) {
							aProperty->value.proxy.target = theCurrent;
							theCurrent = aTemporary;
							theCurrent->value.instance.garbage = aProperty;
							aProperty = theCurrent;
						}
						else
							aProperty = aProperty->next;
					}
					break;
					
				case XS_CLOSURE_KIND:
					aTemporary = aProperty->value.closure;
					if (aTemporary && !(aTemporary->flag & XS_MARK_FLAG)) {
						aTemporary->flag |= XS_MARK_FLAG; 
						if (aTemporary->kind == XS_REFERENCE_KIND) {
							aTemporary = aTemporary->value.reference;
							if (!(aTemporary->flag & XS_MARK_FLAG)) {
								aProperty->value.closure->value.reference = theCurrent;
								theCurrent = aTemporary;
								theCurrent->value.instance.garbage = aProperty;
								aProperty = theCurrent;
						
							}
						}
						else {
							(*theMarker)(the, aTemporary);
							aProperty = aProperty->next;
						}
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
			switch (aProperty->kind) {
			case XS_INSTANCE_KIND:
				aTemporary = aProperty->value.instance.prototype;
				aProperty->value.instance.prototype = theCurrent;
				theCurrent = aTemporary;
				aProperty = aProperty->next;
				break;
			case XS_REFERENCE_KIND:
				aTemporary = aProperty->value.reference;
				aProperty->value.reference = theCurrent;
				theCurrent = aTemporary;
				aProperty = aProperty->next;
				break;
			case XS_PROXY_KIND:
				if (aProperty->flag & XS_INSPECTOR_FLAG) {
					aProperty->flag &= ~XS_INSPECTOR_FLAG;
					aTemporary = aProperty->value.proxy.handler;
					aProperty->value.proxy.handler = theCurrent;
					theCurrent = aTemporary;
					
					aTemporary = aProperty->value.proxy.target;
					if (aTemporary && !(aTemporary->flag & XS_MARK_FLAG)) {
						aProperty->value.proxy.target = theCurrent;
						theCurrent = aTemporary;
						theCurrent->value.instance.garbage = aProperty;
						aProperty = theCurrent;
					}
					else {
						aProperty = aProperty->next;
					}
				}
				else {
					aTemporary = aProperty->value.proxy.target;
					aProperty->value.proxy.target = theCurrent;
					theCurrent = aTemporary;
					aProperty = aProperty->next;
				}
				break;
			case XS_CLOSURE_KIND:
				aTemporary = aProperty->value.closure->value.reference;
				aProperty->value.closure->value.reference = theCurrent;
				theCurrent = aTemporary;
				aProperty = aProperty->next;
				break;
			}
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
		if (aSlot && !(aSlot->flag & XS_MARK_FLAG)) {
			fxCheckCStack(the);
			fxMarkInstance(the, aSlot, fxMarkReference);
		}
		aSlot = theSlot->value.accessor.setter;
		if (aSlot && !(aSlot->flag & XS_MARK_FLAG)) {
			fxCheckCStack(the);
			fxMarkInstance(the, aSlot, fxMarkReference);
		}
		break;
	case XS_ARGUMENTS_SLOPPY_KIND:
	case XS_ARGUMENTS_STRICT_KIND:
	case XS_ARRAY_KIND:
	case XS_STACK_KIND:
		fxCheckCStack(the);
		if ((aSlot = theSlot->value.array.address)) {
			txIndex aLength = (((txChunk*)(((txByte*)aSlot) - sizeof(txChunk)))->size) / sizeof(txSlot);
			if (aLength > theSlot->value.array.length)
				aLength = theSlot->value.array.length;
			while (aLength) {
				fxMarkReference(the, aSlot);
				aSlot++;
				aLength--;
			}
		}
		break;
	case XS_CALLBACK_KIND:
		aSlot = theSlot->value.callback.closures;
		if (aSlot && !(aSlot->flag & XS_MARK_FLAG)) {
			fxCheckCStack(the);
			fxMarkInstance(the, aSlot, fxMarkValue);
		}
		break;
	case XS_CODE_KIND:
	case XS_CODE_X_KIND:
		aSlot = theSlot->value.code.closures;
		if (aSlot && !(aSlot->flag & XS_MARK_FLAG)) {
			fxCheckCStack(the);
			fxMarkInstance(the, aSlot, fxMarkReference);
		}
		break;
	case XS_HOME_KIND:
		aSlot = theSlot->value.home.object;
		if (aSlot && !(aSlot->flag & XS_MARK_FLAG)) {
			fxCheckCStack(the);
			fxMarkInstance(the, aSlot, fxMarkReference);
		}
		aSlot = theSlot->value.home.module;
		if (aSlot && !(aSlot->flag & XS_MARK_FLAG)) {
			fxCheckCStack(the);
			fxMarkInstance(the, aSlot, fxMarkReference);
		}
		break;
	case XS_MODULE_KIND:
	case XS_PROGRAM_KIND:
		fxCheckCStack(the);
		aSlot = theSlot->value.module.realm;
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
		
	case XS_ERROR_KIND:
		aSlot = theSlot->value.error.info;
		if (aSlot && (!(aSlot->flag & XS_MARK_FLAG)))
			fxMarkInstance(the, aSlot, fxMarkReference);
		break;
	case XS_LIST_KIND:
		fxCheckCStack(the);
		aSlot = theSlot->value.list.first;
		while (aSlot) {
			if (!(aSlot->flag & XS_MARK_FLAG)) {
				aSlot->flag |= XS_MARK_FLAG;
				fxMarkReference(the, aSlot);
			}
			aSlot = aSlot->next;
		}
		break;
		
	case XS_PRIVATE_KIND:
		aSlot = theSlot->value.private.check;
		if (!(aSlot->flag & XS_MARK_FLAG))
			fxMarkInstance(the, aSlot, fxMarkReference);
		aSlot = theSlot->value.private.first;
		while (aSlot) {
			aSlot->flag |= XS_MARK_FLAG;
			fxMarkReference(the, aSlot);
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
	case XS_WEAK_SET_KIND:
		aSlot = theSlot->value.weakList.first;
		while (aSlot) {
			if (!(aSlot->flag & XS_MARK_FLAG)) {
				aSlot->flag |= XS_MARK_FLAG;
				fxMarkReference(the, aSlot);
			}
			aSlot = aSlot->next;
		}
		break;
	case XS_WEAK_ENTRY_KIND:
		aSlot = theSlot->value.weakEntry.check;
		if (aSlot->flag & XS_MARK_FLAG) {
			aSlot = theSlot->value.weakEntry.value;
			if (!(aSlot->flag & XS_MARK_FLAG)) {
				aSlot->flag |= XS_MARK_FLAG; 
				fxMarkReference(the, aSlot);
			}
		}
		break;
	case XS_WEAK_REF_KIND:
		aSlot = theSlot->value.weakRef.target;
		if (aSlot) {
	#ifdef mxSnapshot
			if (the->collectFlag & XS_ORGANIC_FLAG) {
				fxMarkReference(the, aSlot);
			}
			else {
				theSlot->value.weakRef.link = the->firstWeakRefLink;
				the->firstWeakRefLink = theSlot;
			}
	#else
			theSlot->value.weakRef.link = the->firstWeakRefLink;
			the->firstWeakRefLink = theSlot;
	#endif
		}
		break;
	case XS_FINALIZATION_REGISTRY_KIND:
		aSlot = theSlot->value.finalizationRegistry.callback;
		if (aSlot) {
			fxCheckCStack(the);
			aSlot->flag |= XS_MARK_FLAG;
			fxMarkReference(the, aSlot);
			aSlot = aSlot->next;
			while (aSlot) {
				aSlot->flag |= XS_MARK_FLAG;
				fxMarkReference(the, aSlot); // holdings
				aSlot = aSlot->next;
				if (aSlot) {
					aSlot->flag |= XS_MARK_FLAG;
					// weak target and token
					aSlot = aSlot->next;
				}
			}
		}
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
	case XS_BIGINT_KIND:
		mxMarkChunk(theSlot->value.bigint.data);
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
		fxCheckCStack(the);
		if ((aSlot = theSlot->value.array.address)) {
			txChunk* chunk = (txChunk*)(((txByte*)aSlot) - sizeof(txChunk));
			if (!(chunk->size & mxChunkFlag)) {
				txIndex aLength = chunk->size / sizeof(txSlot);
				if (aLength > theSlot->value.array.length)
					aLength = theSlot->value.array.length;
				while (aLength) {
					fxMarkValue(the, aSlot);
					aSlot++;
					aLength--;
				}
				mxMarkChunk(theSlot->value.array.address);
			}
		}
		break;
	case XS_ARRAY_BUFFER_KIND:
		if (theSlot->value.arrayBuffer.address)
			mxMarkChunk(theSlot->value.arrayBuffer.address);
		break;
	case XS_CALLBACK_KIND:
		aSlot = theSlot->value.callback.closures;
		if (aSlot && !(aSlot->flag & XS_MARK_FLAG)) {
			fxCheckCStack(the);
			fxMarkInstance(the, aSlot, fxMarkValue);
		}
		break;
	case XS_CODE_KIND:
		mxMarkChunk(theSlot->value.code.address);
		/* continue */
	case XS_CODE_X_KIND:
		aSlot = theSlot->value.code.closures;
		if (aSlot && !(aSlot->flag & XS_MARK_FLAG)) {
			fxCheckCStack(the);
			fxMarkInstance(the, aSlot, fxMarkValue);
		}
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
	case XS_IDS_KIND:
		if (theSlot->value.IDs)
			mxMarkChunk(theSlot->value.IDs);
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
		if (aSlot && !(aSlot->flag & XS_MARK_FLAG)) {
			fxCheckCStack(the);
			fxMarkInstance(the, aSlot, fxMarkValue);
		}
		aSlot = theSlot->value.accessor.setter;
		if (aSlot && !(aSlot->flag & XS_MARK_FLAG)) {
			fxCheckCStack(the);
			fxMarkInstance(the, aSlot, fxMarkValue);
		}
		break;
	case XS_HOME_KIND:
		aSlot = theSlot->value.home.object;
		if (aSlot && !(aSlot->flag & XS_MARK_FLAG)) {
			fxCheckCStack(the);
			fxMarkInstance(the, aSlot, fxMarkValue);
		}
		aSlot = theSlot->value.home.module;
		if (aSlot && !(aSlot->flag & XS_MARK_FLAG)) {
			fxCheckCStack(the);
			fxMarkInstance(the, aSlot, fxMarkValue);
		}
		break;
	case XS_MODULE_KIND:
	case XS_PROGRAM_KIND:
		aSlot = theSlot->value.module.realm;
		if (aSlot && !(aSlot->flag & XS_MARK_FLAG)) {
			fxCheckCStack(the);
			fxMarkInstance(the, aSlot, fxMarkValue);
		}
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
		
	case XS_ERROR_KIND:
		aSlot = theSlot->value.error.info;
		if (aSlot && (!(aSlot->flag & XS_MARK_FLAG)))
			fxMarkInstance(the, aSlot, fxMarkValue);
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
		
	case XS_PRIVATE_KIND:
		aSlot = theSlot->value.private.check;
		if (!(aSlot->flag & XS_MARK_FLAG))
			fxMarkInstance(the, aSlot, fxMarkValue);
		aSlot = theSlot->value.private.first;
		while (aSlot) {
			aSlot->flag |= XS_MARK_FLAG;
			fxMarkValue(the, aSlot);
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
	case XS_WEAK_SET_KIND:
		aSlot = theSlot->value.weakList.first;
		while (aSlot) {
			if (!(aSlot->flag & XS_MARK_FLAG)) {
				aSlot->flag |= XS_MARK_FLAG;
				fxMarkValue(the, aSlot);
			}
			aSlot = aSlot->next;
		}
		break;
	case XS_WEAK_ENTRY_KIND:
		aSlot = theSlot->value.weakEntry.check;
		if (aSlot->flag & XS_MARK_FLAG) {
			aSlot = theSlot->value.weakEntry.value;
			if (!(aSlot->flag & XS_MARK_FLAG)) {
				aSlot->flag |= XS_MARK_FLAG; 
				fxMarkValue(the, aSlot);
			}
		}
		break;
	case XS_WEAK_REF_KIND:
		aSlot = theSlot->value.weakRef.target;
		if (aSlot) {
	#ifdef mxSnapshot
			if (the->collectFlag & XS_ORGANIC_FLAG) {
				fxMarkValue(the, aSlot);
			}
			else {
				theSlot->value.weakRef.link = the->firstWeakRefLink;
				the->firstWeakRefLink = theSlot;
			}
	#else
			theSlot->value.weakRef.link = the->firstWeakRefLink;
			the->firstWeakRefLink = theSlot;
	#endif
		}
		break;
	case XS_FINALIZATION_REGISTRY_KIND:
		aSlot = theSlot->value.finalizationRegistry.callback;
		if (aSlot) {
			aSlot->flag |= XS_MARK_FLAG;
			fxMarkValue(the, aSlot);
			aSlot = aSlot->next;
			while (aSlot) {
				aSlot->flag |= XS_MARK_FLAG;
                fxCheckCStack(the);
				fxMarkValue(the, aSlot); // holdings
				aSlot = aSlot->next;
				if (aSlot) {
					aSlot->flag |= XS_MARK_FLAG;
					// weak target and token
					aSlot = aSlot->next;
				}
			}
		}
		break;
		
	case XS_HOST_INSPECTOR_KIND:
		aSlot = theSlot->value.hostInspector.cache;
		if (!(aSlot->flag & XS_MARK_FLAG))
			fxMarkInstance(the, aSlot, fxMarkValue);
		break;	
	}
}

void fxMarkWeakStuff(txMachine* the) 
{
	txSlot* slot;
	txSlot** address;

	{
		txSlot* list;
		txSlot** listAddress = &the->firstWeakListLink;
		while ((list = *listAddress)) {
			if (list->flag & XS_MARK_FLAG) {
				txSlot* listEntry;
				txSlot** listEntryAddress = &list->value.weakList.first;
				while ((listEntry = *listEntryAddress)) {
					txSlot* value = listEntry->value.weakEntry.value;
					if ((value->flag & XS_MARK_FLAG) && (value->kind != XS_UNINITIALIZED_KIND)) {
						listEntryAddress = &listEntry->next;
					}
					else {
						listEntry->flag &= ~XS_MARK_FLAG;
						*listEntryAddress = listEntry->next;
					}
				}
				listAddress = &list->value.weakList.link;
			}
			else {
				txSlot* listEntry = list->value.weakList.first;
				while (listEntry) {
					txSlot* key = listEntry->value.weakEntry.check;
					if (key->flag & XS_MARK_FLAG) {
						txSlot* keyEntry;
						txSlot** keyEntryAddress = &key->next;
						while ((keyEntry = *keyEntryAddress)) {
							if (!(keyEntry->flag & XS_INTERNAL_FLAG))
								break;
							if ((keyEntry->kind == XS_WEAK_ENTRY_KIND) && (keyEntry->value.weakEntry.check == list)) {
								keyEntry->flag &= ~XS_MARK_FLAG;
								*keyEntryAddress = keyEntry->next;
								break;
							}
							keyEntryAddress = &keyEntry->next;
						}
					}
					listEntry = listEntry->next;
				}
				*listAddress = list->value.weakList.link;
			}
		}
	}
	address = &the->firstWeakRefLink;
	while ((slot = *address)) {
		if (!(slot->value.weakRef.target->flag & XS_MARK_FLAG))
			slot->value.weakRef.target = C_NULL;
		*address = C_NULL;
		address = &(slot->value.weakRef.link);
	}
	
	if (mxFinalizationRegistries.kind == XS_REFERENCE_KIND) {
		slot = mxFinalizationRegistries.value.reference->next;
		while (slot) {
			fxMarkFinalizationRegistry(the, slot->value.closure);
			slot = slot->next;
		}
	}
}

txSize fxMultiplyChunkSizes(txMachine* the, txSize a, txSize b)
{
	txSize c;
#if __has_builtin(__builtin_mul_overflow)
	if (__builtin_mul_overflow(a, b, &c) || (c < 0)) {
#else
	txNumber C = (txNumber)a * (txNumber)b;
	c = (txSize)C;
	if ((C > (txNumber)0x7FFFFFFF) || (C < (txNumber)0)) {
#endif
		fxAbort(the, XS_NOT_ENOUGH_MEMORY_EXIT);
	}
	return c;
}

void* fxNewChunk(txMachine* the, txSize size)
{
	txSize offset = size;
	txChunk* chunk;
	txBoolean once = 1;
	size = fxAdjustChunkSize(the, size);
	chunk = fxFindChunk(the, size, &once);
	if (!chunk) {
		chunk = fxGrowChunk(the, size);
	}
	return fxCheckChunk(the, chunk, size, offset);
}

void* fxNewGrowableChunk(txMachine* the, txSize size, txSize capacity)
{
#if mxNoChunks
	return fxNewChunk(the, size);
#else
	txSize offset = size;
	txChunk* chunk;
	txBoolean once = 1;
	size = fxAdjustChunkSize(the, size);
	capacity = fxAdjustChunkSize(the, capacity);
	chunk = fxFindChunk(the, capacity, &once);
	if (!chunk) {
		chunk = fxGrowChunk(the, capacity);
		if (!chunk) {
			chunk = fxFindChunk(the, size, &once);
			if (!chunk) {
				chunk = fxGrowChunk(the, size);
			}
		}
	}
	return fxCheckChunk(the, chunk, size, offset);
#endif
}

txSlot* fxNewSlot(txMachine* the) 
{
	txSlot* aSlot;
	txBoolean once = 1, allocate;
	
#if mxStress
	if (fxShouldStress()) {
		fxCollect(the, XS_COMPACT_FLAG | XS_ORGANIC_FLAG);
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
	#ifdef mxSnapshot
		#if mx32bitID
			aSlot->dummy = 0;
		#elif INTPTR_MAX == INT64_MAX
			aSlot->dummy = 0;
		#endif
	#endif
#if mxPoisonSlots
		ASAN_UNPOISON_MEMORY_REGION(&aSlot->value, sizeof(aSlot->value));
#endif
		the->currentHeapCount++;
		if (the->peakHeapCount < the->currentHeapCount)
			the->peakHeapCount = the->currentHeapCount;
		return aSlot;
	}
	if (once) {
		txBoolean wasThrashing = ((the->collectFlag & XS_TRASHING_FLAG) != 0), isThrashing;

		fxCollect(the, XS_ORGANIC_FLAG);

		isThrashing = ((the->collectFlag & XS_TRASHING_FLAG) != 0);
		allocate = wasThrashing && isThrashing;

		once = 0;
	}
	else
		allocate = 1;
	if (allocate) {
		if (!the->minimumHeapCount) {
			fxReport(the, "# Slot allocation: failed in fixed size heap\n");
			fxAbort(the, XS_NOT_ENOUGH_MEMORY_EXIT);
		}
		fxGrowSlots(the, !(the->collectFlag & XS_SKIPPED_COLLECT_FLAG) ? the->minimumHeapCount : 64);
	}
	goto again;
	return C_NULL;
}

void* fxRenewChunk(txMachine* the, void* theData, txSize size)
{
#if mxNoChunks
	txByte* aData = ((txByte*)theData) - sizeof(txChunk);
	txChunk* aChunk = (txChunk*)aData;
	size = fxAdjustChunkSize(the, size);
	if (size <= aChunk->size) {
		aChunk->size = size;
		return theData;
	}
	return C_NULL;
#else
	txByte* aData = ((txByte*)theData) - sizeof(txChunk);
	txChunk* aChunk = (txChunk*)aData;
	txSize capacity = (txSize)(aChunk->temporary - aData);
	txBlock* aBlock = the->firstBlock;
	size = fxAdjustChunkSize(the, size);
	if (size <= capacity) {
		the->currentChunksSize += size - aChunk->size;
		if (the->peakChunksSize < the->currentChunksSize)
			the->peakChunksSize = the->currentChunksSize;
		aChunk->size = size;
	#ifdef mxNever
		gxRenewChunkCases[0]++;
	#endif
		return theData;
	}
	while (aBlock) {
		if (aChunk->temporary == aBlock->current) {
			txSize delta = size - capacity;
			if (aBlock->current + delta <= aBlock->limit) {
				the->currentChunksSize += size - aChunk->size;
				if (the->peakChunksSize < the->currentChunksSize)
					the->peakChunksSize = the->currentChunksSize;
				aBlock->current += delta;
				aChunk->temporary = aBlock->current;
				aChunk->size = size;
			#ifdef mxSnapshot
				c_memset(aData + capacity, 0, delta);
			#endif
			#ifdef mxNever
				gxRenewChunkCases[1]++;
			#endif
				return theData;
			}
			else {
			#ifdef mxNever
				gxRenewChunkCases[2]++;
			#endif
				return C_NULL;
			}
		}
		aBlock = aBlock->nextBlock;
	}
#ifdef mxNever
	gxRenewChunkCases[3]++;
#endif
	return C_NULL;
#endif
}

void fxShare(txMachine* the)
{
	txID aliasCount = 0;
	txSlot *heap, *slot, *limit;

	heap = the->firstHeap;
	while (heap) {
		slot = heap + 1;
		limit = heap->value.reference;
		while (slot < limit) {
			if (slot->kind == XS_INSTANCE_KIND) {
				txBoolean frozen = (slot->flag & XS_DONT_PATCH_FLAG) ? 1 : 0;
				if (frozen) {
					txSlot *property = slot->next;
					while (property) {
						if (property->kind == XS_ARRAY_KIND) {
							txSlot* item = property->value.array.address;
							txInteger length = (txInteger)fxGetIndexSize(the, property);
							while (length > 0) {
								if (item->kind != XS_ACCESSOR_KIND) 
									if (!(item->flag & XS_DONT_SET_FLAG))
										frozen = 0;
								if (!(item->flag & XS_DONT_DELETE_FLAG))
									frozen = 0;
								item++;
								length--;
							}
						}
						else {
							if (property->kind != XS_ACCESSOR_KIND) 
								if (!(property->flag & XS_DONT_SET_FLAG))
									frozen = 0;
							if (!(property->flag & XS_DONT_DELETE_FLAG))
								frozen = 0;
						}
						property = property->next;
					}
				}
				if (frozen)
					slot->ID = XS_NO_ID;
				else
					slot->ID = aliasCount++;
			}
			else if (slot->kind == XS_CLOSURE_KIND) {
				txSlot* closure = slot->value.closure;
				if (closure->flag & XS_DONT_SET_FLAG)
					closure->flag |= XS_DONT_DELETE_FLAG;
				else {
					if (closure->ID == XS_NO_ID)
						closure->ID = aliasCount++;
					slot->flag &= ~XS_DONT_SET_FLAG;
				}
			}
			slot->flag |= XS_MARK_FLAG; 
			slot++;
		}
		heap = heap->next;
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
#if mxNoChunks
	txChunk** address;
	txChunk* chunk;
#else
	txBlock* aBlock;
	txByte* limit;
	txByte* next;
#endif
	txByte* current;
	txByte* temporary;
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
#if mxNoChunks
	address = (txChunk**)&(the->firstBlock);
	while ((chunk = *address)) {
		aSize = chunk->size;
		if (aSize & mxChunkFlag) {
			aSize &= ~mxChunkFlag;
			temporary = c_malloc(aSize);
			c_memcpy(temporary, chunk, aSize);
			((txChunk*)temporary)->size = aSize;
			chunk->temporary = temporary;
			address = (txChunk**)&(((txChunk*)temporary)->temporary);
			aTotal += aSize;
		}
		else {
			*address = (txChunk*)(chunk->temporary);
			c_free(chunk);
		}
	}
#else
	aBlock = the->firstBlock;
	while (aBlock) {
		current = ((txByte*)aBlock) + sizeof(txBlock);
		limit = aBlock->current;
		temporary = current;
		while (current < limit) {
			aSize = ((txChunk*)current)->size;
			next = ((txChunk*)current)->temporary;
			if (aSize & mxChunkFlag) {
				aSize &= ~mxChunkFlag;
				((txChunk*)current)->temporary = temporary;
				temporary += aSize;
				aTotal += aSize;
			}
			else {
				((txChunk*)current)->temporary = C_NULL;
			}
			((txChunk*)current)->size = (txSize)(next - current);
			current = next;
		}	
		aBlock->temporary = temporary;
		aBlock = aBlock->nextBlock;
	}
#endif
	the->currentChunksSize = aTotal;

	aCodeAddress = &(the->code);
	aSlot = the->frame;
	while (aSlot) {
		mxCheck(the, aSlot->kind == XS_FRAME_KIND);
		if ((aSlot->flag & XS_C_FLAG) == 0) {
			bSlot = (aSlot + 3)->value.reference->next;
			if (bSlot->kind == XS_CODE_KIND) {
				current = bSlot->value.code.address;
				temporary = (txByte*)(((txChunk*)(current - sizeof(txChunk)))->temporary);
				if (temporary) {
					temporary += sizeof(txChunk);
					*aCodeAddress += temporary - current;
				}
			}
		}
		else {
			current = *aCodeAddress;
			if (current) {
				temporary = (txByte*)(((txChunk*)(current - sizeof(txChunk)))->temporary);
				if (temporary)
					*aCodeAddress = temporary + sizeof(txChunk);
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
				current = bSlot->value.code.address;
				temporary = (txByte*)(((txChunk*)(current - sizeof(txChunk)))->temporary);
				if (temporary) {
					temporary += sizeof(txChunk);
					jump->code += temporary - current;
				}
			}
		}
		else {
			current = jump->code;
			if (current) {
				temporary = (txByte*)(((txChunk*)(current - sizeof(txChunk)))->temporary);
				if (temporary)
					jump->code = temporary + sizeof(txChunk);
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
			#ifndef mxLink
				if (bSlot->kind == XS_HOST_KIND) {
					if (bSlot->flag & XS_HOST_HOOKS_FLAG) {
						if (bSlot->value.host.variant.hooks->destructor)
							(*(bSlot->value.host.variant.hooks->destructor))(bSlot->value.host.data);
					}
					else if (bSlot->value.host.variant.destructor)
						(*(bSlot->value.host.variant.destructor))(bSlot->value.host.data);
				}
			#endif
// 				if (bSlot->kind == XS_MODULE_KIND) {
// 					char* name = fxGetKeyName(the, bSlot->value.module.id);
// 					fprintf(stderr, "gc module %d %s\n", bSlot->value.module.id, name);
// 				}
			#if mxInstrument
				if (bSlot->kind == XS_MODULE_KIND)
					the->loadedModulesCount--;
			#endif
				bSlot->kind = XS_UNDEFINED_KIND;
				bSlot->next = freeSlot;
			#if mxPoisonSlots
				ASAN_POISON_MEMORY_REGION(&bSlot->value, sizeof(bSlot->value));
			#endif
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

#if mxNoChunks
	address = (txChunk**)&(the->firstBlock);
	while ((chunk = *address)) {
		aSize = chunk->size;
		if (aSize & mxChunkFlag) {
			*address = (txChunk*)(chunk->temporary);
			c_free(chunk);
		}
		else {
			address = (txChunk**)&(chunk->temporary);
		}
	}
#else
	aBlock = the->firstBlock;
	while (aBlock) {
		txByte* former = C_NULL;
		current = ((txByte*)aBlock) + sizeof(txBlock);
		limit = aBlock->current;
		while (current < limit) {
			aSize = ((txChunk*)current)->size;
			next = current + aSize;
			if ((temporary = ((txChunk*)current)->temporary)) {
				if (former) {
					((txChunk*)former)->temporary = temporary;
					((txChunk*)former)->size = (txSize)(temporary - former);
				}
				if (temporary != current)
					c_memmove(temporary, current, aSize);
				former = temporary;
			}
			current = next;
		}
		if (former) {
			((txChunk*)former)->temporary = aBlock->temporary;
			((txChunk*)former)->size = (txSize)(aBlock->temporary - former);
		}
		aBlock->current = aBlock->temporary;
		aBlock->temporary = C_NULL;
		aBlock = aBlock->nextBlock;
	}
#endif
	
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
	case XS_BIGINT_KIND:
		mxSweepChunk(theSlot->value.bigint.data, txU4*);
		break;

	case XS_ARGUMENTS_SLOPPY_KIND:
	case XS_ARGUMENTS_STRICT_KIND:
	case XS_ARRAY_KIND:
	case XS_STACK_KIND:
		if ((aSlot = theSlot->value.array.address)) {
#if mxNoChunks
			mxSweepChunk(theSlot->value.array.address, txSlot*);
			aSlot = theSlot->value.array.address;
#endif
			txChunk* chunk = (txChunk*)(((txByte*)aSlot) - sizeof(txChunk));
			txIndex aLength = chunk->size / sizeof(txSlot);
			if (aLength > theSlot->value.array.length)
				aLength = theSlot->value.array.length;
			while (aLength) {
				fxSweepValue(the, aSlot);
				aSlot++;
				aLength--;
			}
#if mxNoChunks
#else
			mxSweepChunk(theSlot->value.array.address, txSlot*);
#endif
		}
		break;
	case XS_ARRAY_BUFFER_KIND:
		if (theSlot->value.arrayBuffer.address)
			mxSweepChunk(theSlot->value.arrayBuffer.address, txByte*);
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
	case XS_IDS_KIND:
		if (theSlot->value.IDs)
			mxSweepChunk(theSlot->value.IDs, txID*);
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
		mxSweepChunk(theSlot->value.table.address, txSlot**);
		break;
	}
}
