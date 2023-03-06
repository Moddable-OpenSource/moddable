/*
 * Copyright (c) 2016-2018  Moddable Tech, Inc.
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

#if mx32bitID
#define mxSymbolBit 0x80000000
#define mxSymbolMask 0x7FFFFFFF
#else
#define mxSymbolBit 0x8000
#define mxSymbolMask 0x7FFF
#endif

typedef struct sxMarshallBuffer txMarshallBuffer; 
struct sxMarshallBuffer {
	txByte* base;
	txByte* current;
	txSlot* link;
	txID* symbolMap;
	txSize size;
	txID symbolCount;
	txSize symbolSize;
	txSlot* stack;
};

static void fxDemarshallChunk(txMachine* the, void* theData, void** theDataAddress);
static txID fxDemarshallKey(txMachine* the, txID id, txID* theSymbolMap, txBoolean alien);
static void fxDemarshallReference(txMachine* the, txSlot* theSlot, txSlot** theSlotAddress, txID* theSymbolMap, txBoolean alien);
static void fxDemarshallSlot(txMachine* the, txSlot* theSlot, txSlot* theResult, txID* theSymbolMap, txBoolean alien);
static void fxMarshallChunk(txMachine* the, void* theData, void** theDataAddress, txMarshallBuffer* theBuffer);
static txID fxMarshallKey(txMachine* the, txID id, txMarshallBuffer* theBuffer, txBoolean alien);
static void fxMarshallReference(txMachine* the, txSlot* theSlot, txSlot** theSlotAddress, txMarshallBuffer* theBuffer, txBoolean alien);
static txBoolean fxMarshallSlot(txMachine* the, txSlot* theSlot, txSlot** theSlotAddress, txMarshallBuffer* theBuffer, txBoolean alien);
static void fxMeasureChunk(txMachine* the, void* theData, txMarshallBuffer* theBuffer);
static void fxMeasureKey(txMachine* the, txID theID, txMarshallBuffer* theBuffer, txBoolean alien);
static void fxMeasureReference(txMachine* the, txSlot* theSlot, txMarshallBuffer* theBuffer, txBoolean alien);
static void fxMeasureSlot(txMachine* the, txSlot* theSlot, txMarshallBuffer* theBuffer, txBoolean alien);
static void fxMeasureThrow(txMachine* the, txMarshallBuffer* theBuffer, txString message);

#define mxMarshallAlign(POINTER,SIZE) \
	if (((SIZE) &= ((sizeof(txNumber) - 1)))) (POINTER) += sizeof(txNumber) - (SIZE)

void fxDemarshall(txMachine* the, void* theData, txBoolean alien)
{
	txFlag aFlag;
	txByte* p;
	txByte* q;
	txID aSymbolCount;
	txID aSymbolLength;
	txID* aSymbolMap;
	txID* aSymbolPointer;
	txSlot* aSlot;
	txChunk* aChunk;
	txInteger skipped;
	txIndex aLength;
	
	if (!theData) {
		the->stack->kind = XS_UNDEFINED_KIND;
		return;
	}
	p = (txByte*)theData;
	q = p + *((txSize*)(p));
	aFlag = (txFlag)the->collectFlag;
	the->collectFlag &= ~(XS_COLLECTING_FLAG | XS_SKIPPED_COLLECT_FLAG);
	{
		mxTry(the) {
			p += sizeof(txSize);
			aSymbolCount = *((txID*)p);
			p += sizeof(txID);
			aSymbolMap = aSymbolPointer = (txID*)p;
			p += aSymbolCount * sizeof(txID);
			while (aSymbolCount) {
				txID id;
				aSymbolLength = *aSymbolPointer;
				if ((aSymbolLength & mxSymbolBit) == 0)
					id = fxNewNameC(the, (char *)p);
				else {
					aSymbolLength = aSymbolLength & mxSymbolMask;
					id = the->keyIndex;
					if (id == the->keyCount)
						fxGrowKeys(the, 1);
					if (aSymbolLength > 1) {
						aSlot = fxNewSlot(the);
						aSlot->kind = XS_STRING_KIND;
						aSlot->value.string = (txString)fxNewChunk(the, mxStringLength((char *)p) + 1);
						c_strcpy(aSlot->value.key.string, (char *)p);
					}
					else
						aSlot = C_NULL;
					the->keyArray[id - the->keyOffset] = aSlot;
					the->keyIndex++;
				}
				*aSymbolPointer++ = id;
				aSymbolCount--;
				p += aSymbolLength;
			}
			aLength = mxPtrDiff(p - (txByte*)theData);
			mxMarshallAlign(p, aLength);
			mxPushUndefined();
			fxDemarshallSlot(the, (txSlot*)p, the->stack, aSymbolMap, alien);
		}
		mxCatch(the) {
			the->stack->kind = XS_UNDEFINED_KIND;
			break;
		}
		while (p < q) {
			aSlot = (txSlot*)p;
			p += sizeof(txSlot);
			switch (aSlot->kind) {
			case XS_STRING_KIND:
			case XS_BIGINT_KIND:
			case XS_ARRAY_BUFFER_KIND:
				aChunk = (txChunk*)p;
				p += aChunk->size;
				mxMarshallAlign(p, aChunk->size);
				break;
			case XS_REGEXP_KIND:
				if (aSlot->value.regexp.code) {
					aChunk = (txChunk*)p;
					p += aChunk->size;
					mxMarshallAlign(p, aChunk->size);
				}
				if (aSlot->value.regexp.data) {
					aChunk = (txChunk*)p;
					p += aChunk->size;
					mxMarshallAlign(p, aChunk->size);
				}
				break;
			case XS_INSTANCE_KIND:
				aSlot->value.instance.garbage = C_NULL;
				break;
			}
		}
	}
	skipped = the->collectFlag & XS_SKIPPED_COLLECT_FLAG;
	the->collectFlag = aFlag;
	if (skipped)
		fxCollectGarbage(the);
}

void fxDemarshallChunk(txMachine* the, void* theData, void** theDataAddress)
{
	txSize aSize = ((txChunk*)(((txByte*)theData) - sizeof(txChunk)))->size - sizeof(txChunk);
	txByte* aResult = (txByte *)fxNewChunk(the, aSize);
	c_memcpy(aResult, theData, aSize);
	*theDataAddress = aResult;
}

txID fxDemarshallKey(txMachine* the, txID id, txID* theSymbolMap, txBoolean alien)
{
	if (id != XS_NO_ID) {
		if (alien)
			id = theSymbolMap[id - 1];
		else if (id >= the->keyOffset)
			id = theSymbolMap[id - the->keyOffset];
	}
	return id;
}

void fxDemarshallReference(txMachine* the, txSlot* theSlot, txSlot** theSlotAddress, txID* theSymbolMap, txBoolean alien)
{
	if (!alien && (theSlot->flag & XS_DONT_MARSHALL_FLAG))
		*theSlotAddress = theSlot;
	else if (theSlot->value.instance.garbage)
		*theSlotAddress = theSlot->value.instance.garbage;
	else {
		*theSlotAddress = fxNewSlot(the);
		fxDemarshallSlot(the, theSlot, *theSlotAddress, theSymbolMap, alien);
	}
}

void fxDemarshallSlot(txMachine* the, txSlot* theSlot, txSlot* theResult, txID* theSymbolMap, txBoolean alien)
{
	txSlot* aSlot;
	txIndex aLength, index;
	txSlot* aResult;
	txSlot** aSlotAddress;

	if (!(theSlot->flag & XS_INTERNAL_FLAG))
		theResult->ID = fxDemarshallKey(the, theSlot->ID, theSymbolMap, alien);
	else
		theResult->ID = theSlot->ID;
	theResult->flag = theSlot->flag;
	switch (theSlot->kind) {
	case XS_UNDEFINED_KIND:
	case XS_NULL_KIND:
	case XS_BOOLEAN_KIND:
	case XS_INTEGER_KIND:
	case XS_NUMBER_KIND:
	case XS_DATE_KIND:
	case XS_STRING_X_KIND:
	case XS_BIGINT_X_KIND:
	case XS_DATA_VIEW_KIND:
	case XS_KEY_X_KIND:
	case XS_BUFFER_INFO_KIND:
		theResult->value = theSlot->value;
		theResult->kind = theSlot->kind;
		break;
		
	case XS_STRING_KIND:
		fxDemarshallChunk(the, theSlot->value.string, (void **)&theResult->value.string);
		theResult->kind = theSlot->kind;
		break;
	case XS_BIGINT_KIND:
		fxDemarshallChunk(the, theSlot->value.bigint.data, (void **)&theResult->value.bigint.data);
		theResult->value.bigint.size = theSlot->value.bigint.size;
		theResult->value.bigint.sign = theSlot->value.bigint.sign;
		theResult->kind = theSlot->kind;
		break;
	case XS_ARRAY_BUFFER_KIND: 
		if (theSlot->value.arrayBuffer.address)
			fxDemarshallChunk(the, theSlot->value.arrayBuffer.address, (void **)&(theResult->value.arrayBuffer.address));
		else
			theResult->value.arrayBuffer.address = C_NULL;
		theResult->kind = theSlot->kind;
		break;
	case XS_REGEXP_KIND:
		if (theSlot->value.regexp.code)
			fxDemarshallChunk(the, theSlot->value.regexp.code, (void**)&(theResult->value.regexp.code));
		else
			theResult->value.regexp.code = C_NULL;
		if (theSlot->value.regexp.data)
			fxDemarshallChunk(the, theSlot->value.regexp.data, (void**)&(theResult->value.regexp.data));
		else
			theResult->value.regexp.data = C_NULL;
		theResult->kind = theSlot->kind;
		break;	
	case XS_KEY_KIND:
		if (theSlot->value.key.string)
			fxDemarshallChunk(the, theSlot->value.key.string, (void **)&theResult->value.key.string);
		else
			theResult->value.key.string = C_NULL;
		theResult->value.key.sum = theSlot->value.key.sum;
		theResult->kind = theSlot->kind;
		break;
		
	case XS_SYMBOL_KIND:
		theResult->value.symbol = fxDemarshallKey(the, theSlot->value.symbol, theSymbolMap, alien);
		theResult->kind = theSlot->kind;
		break;
		
	case XS_HOST_KIND:
		theResult->value.host.data = fxRetainSharedChunk(theSlot->value.host.data);
		theResult->value.host.variant.destructor = fxReleaseSharedChunk;
		theResult->kind = theSlot->kind;
		break;
	case XS_MAP_KIND:
	case XS_SET_KIND:
        theResult->value.table.length = theSlot->value.table.length;
		aSlotAddress = (txSlot**)fxNewChunk(the, theResult->value.table.length * sizeof(txSlot*));
		c_memset(aSlotAddress, 0, theResult->value.table.length * sizeof(txSlot*));
		theResult->value.table.address = aSlotAddress;
		theResult->kind = theSlot->kind;
		break;	
	case XS_TYPED_ARRAY_KIND:
		theResult->value.typedArray.dispatch = (txTypeDispatch*)&gxTypeDispatches[theSlot->value.integer];
		theResult->value.typedArray.atomics = (txTypeAtomics*)&gxTypeAtomics[theSlot->value.integer];
		theResult->kind = theSlot->kind;
		break;
		
	case XS_REFERENCE_KIND:
		theResult->kind = theSlot->kind;
		fxDemarshallReference(the, theSlot->value.reference, &theResult->value.reference, theSymbolMap, alien);
		break;
	case XS_INSTANCE_KIND:
		theSlot->value.instance.garbage = theResult;
		theResult->value.instance.garbage = C_NULL;
		theResult->kind = theSlot->kind;
		aSlot = theSlot->next;
		if (!alien && theSlot->value.instance.prototype) {
        	theResult->value.instance.prototype = theSlot->value.instance.prototype;
		}
		else {
        	theResult->value.instance.prototype = mxObjectPrototype.value.reference;
			if (aSlot) {
				if (aSlot->flag & XS_INTERNAL_FLAG) {
					if (aSlot->ID == XS_ARRAY_BEHAVIOR)
						theResult->value.instance.prototype = mxArrayPrototype.value.reference;
					else {
						switch (aSlot->kind) {
						case XS_ARRAY_BUFFER_KIND: theResult->value.instance.prototype = mxArrayBufferPrototype.value.reference; break;
						case XS_BOOLEAN_KIND: theResult->value.instance.prototype = mxBooleanPrototype.value.reference; break;
						case XS_DATA_VIEW_KIND: theResult->value.instance.prototype = mxDataViewPrototype.value.reference; break;
						case XS_DATE_KIND: theResult->value.instance.prototype = mxDatePrototype.value.reference; break;
						case XS_ERROR_KIND: theResult->value.instance.prototype = mxErrorPrototypes(aSlot->value.error.which).value.reference; break;
						case XS_HOST_KIND: theResult->value.instance.prototype = mxSharedArrayBufferPrototype.value.reference; break;
						case XS_MAP_KIND: theResult->value.instance.prototype = mxMapPrototype.value.reference; break;
						case XS_NUMBER_KIND: theResult->value.instance.prototype = mxNumberPrototype.value.reference; break;
						case XS_REGEXP_KIND: theResult->value.instance.prototype = mxRegExpPrototype.value.reference; break;
						case XS_SET_KIND: theResult->value.instance.prototype = mxSetPrototype.value.reference; break;
						case XS_STRING_KIND: theResult->value.instance.prototype = mxStringPrototype.value.reference; break;
						case XS_TYPED_ARRAY_KIND: {
							txTypeDispatch* dispatch = (txTypeDispatch*)&gxTypeDispatches[aSlot->value.integer];
							mxPush(the->stackPrototypes[-1 - (txInteger)dispatch->constructorID]);
							mxGetID(mxID(_prototype));
							theResult->value.instance.prototype = the->stack->value.reference;
							mxPop();
							} break;
						}
					}
				}
			}
		}
		aSlotAddress = &(theResult->next);
		while (aSlot) {
			*aSlotAddress = fxNewSlot(the);
			fxDemarshallSlot(the, aSlot, *aSlotAddress, theSymbolMap, alien);
			aSlot = aSlot->next;
			aSlotAddress = &((*aSlotAddress)->next);
		}
		aSlot = theResult->next;
		if (aSlot) {
			if (aSlot->kind == XS_MAP_KIND) {
				txSlot* key = aSlot->next->value.list.first;
				while (key) {
					txSlot* value = key->next;
					txU4 sum = fxSumEntry(the, key);
					txU4 modulo = sum % aSlot->value.table.length;
					txSlot** address = &(aSlot->value.table.address[modulo]);
					txSlot* entry = fxNewSlot(the);
					entry->next = *address;
					entry->kind = XS_ENTRY_KIND;
					entry->value.entry.slot = key;
					entry->value.entry.sum = sum;
					*address = entry;
					key = value->next;
				}
			}
			else if (aSlot->kind == XS_SET_KIND) {
				txSlot* key = aSlot->next->value.list.first;
				while (key) {
					txU4 sum = fxSumEntry(the, key);
					txU4 modulo = sum % aSlot->value.table.length;
					txSlot** address = &(aSlot->value.table.address[modulo]);
					txSlot* entry = fxNewSlot(the);
					entry->next = *address;
					entry->kind = XS_ENTRY_KIND;
					entry->value.entry.slot = key;
					entry->value.entry.sum = sum;
					*address = entry;
					key = key->next;
				}
			}
		}
		break;	
	case XS_ARRAY_KIND:
		theResult->value.array.length = 0;
		theResult->value.array.address = C_NULL;
		theResult->kind = theSlot->kind;
		if ((aLength = theSlot->value.array.length)) {
			theResult->value.array.length = aLength;
			theResult->value.array.address = (txSlot *)fxNewChunk(the, aLength * sizeof(txSlot));
			c_memset(theResult->value.array.address, 0, aLength * sizeof(txSlot));
			aSlot = theSlot->value.array.address;
			aResult = theResult->value.array.address;
			index = 0;
			while (aLength) {
				fxDemarshallSlot(the, aSlot, aResult, theSymbolMap, alien);
				aLength--;
				aSlot = aSlot->next;
				*((txIndex*)aResult) = index;
				aResult++;
				index++;
			}
		}
		break;
	case XS_ERROR_KIND:
		theResult->kind = theSlot->kind;
		if (theSlot->value.error.info)
			fxDemarshallReference(the, theSlot->value.error.info, &theResult->value.error.info, theSymbolMap, alien);
		else
			theResult->value.error.info = C_NULL;
		theResult->value.error.which = theSlot->value.error.which;
		break;
	case XS_PROXY_KIND:
		theResult->kind = theSlot->kind;
		if (theSlot->value.proxy.handler)
			fxDemarshallReference(the, theSlot->value.proxy.handler, &theResult->value.proxy.handler, theSymbolMap, alien);
		else
			theResult->value.proxy.handler = C_NULL;
		if (theSlot->value.proxy.target)
			fxDemarshallReference(the, theSlot->value.proxy.target, &theResult->value.proxy.target, theSymbolMap, alien);
		else
			theResult->value.proxy.target = C_NULL;
		break;	

	case XS_LIST_KIND:
		aSlot = theSlot->value.list.first;
		aSlotAddress = &(theResult->value.list.first);
		while (aSlot) {
			theResult->value.list.last = *aSlotAddress = fxNewSlot(the);
			fxDemarshallSlot(the, aSlot, *aSlotAddress, theSymbolMap, alien);
			aSlot = aSlot->next;
			aSlotAddress = &((*aSlotAddress)->next);
		}
		theResult->kind = theSlot->kind;
		break;	
	case XS_PRIVATE_KIND:
		aSlot = theSlot->value.private.first;
		aSlotAddress = &(theResult->value.private.first);
		while (aSlot) {
			*aSlotAddress = fxNewSlot(the);
			fxDemarshallSlot(the, aSlot, *aSlotAddress, theSymbolMap, alien);
			aSlot = aSlot->next;
			aSlotAddress = &((*aSlotAddress)->next);
		}
		theResult->value.private.check = theSlot->value.private.check;
		theResult->kind = theSlot->kind;
		break;	
	default:
		break;
	}
}

void* fxMarshall(txMachine* the, txBoolean alien)
{
	txMarshallBuffer aBuffer = { C_NULL, C_NULL, C_NULL, C_NULL, 0, 0, 0 };
	txSlot* aSlot;
	txSlot* bSlot;
	txSlot* cSlot;
	
	mxTry(the) {
		size_t mapSize = alien ? the->keyIndex : (the->keyIndex - the->keyOffset);
		aBuffer.symbolSize = sizeof(txSize) + sizeof(txID);
		aBuffer.symbolMap = c_calloc(mapSize, sizeof(txID));
		if (mapSize && !aBuffer.symbolMap)
			mxUnknownError("out of memory");
        the->stack->ID = XS_NO_ID;
        aBuffer.stack = the->stack;
		fxMeasureSlot(the, the->stack, &aBuffer, alien);
		
		aBuffer.size += aBuffer.symbolSize;
		mxMarshallAlign(aBuffer.size, aBuffer.symbolSize);
		aBuffer.base = aBuffer.current = (txByte *)c_malloc(aBuffer.size);
		if (!aBuffer.base)
			mxUnknownError("out of memory");
		*((txSize*)(aBuffer.current)) = aBuffer.size;
		aBuffer.current += sizeof(txSize);
		*((txID*)(aBuffer.current)) = aBuffer.symbolCount;
		aBuffer.current += sizeof(txID);
		if (aBuffer.symbolCount) {
			txID* lengths = (txID*)aBuffer.current;
			txID* map = aBuffer.symbolMap;
			txID dstIndex = 1;
			txSlot** p;
			txSlot** q;
			aBuffer.current += aBuffer.symbolCount * sizeof(txID);
			if (alien) {
				p = the->keyArrayHost;
				q = p + the->keyOffset;
				while (p < q) {
					txID length = *map;
					if (length) {
						*map = dstIndex;
						if ((*p) && ((*p)->flag & XS_DONT_ENUM_FLAG)) {
							*lengths++ = length;
							c_memcpy(aBuffer.current, (*p)->value.key.string, length);
						}
						else {
							*lengths++ = mxSymbolBit | length;
							if (length > 1)
								c_memcpy(aBuffer.current, (*p)->value.string, length);
							else
								*aBuffer.current = 0;
						}
						aBuffer.current += length;
						dstIndex++;
					}
					map++;
					p++;
				}
			}
			else 
				dstIndex = the->keyOffset;
			p = the->keyArray;
			q = p + the->keyIndex - the->keyOffset;
			while (p < q) {
				txID length = *map;
				if (length) {
					*map = dstIndex;
					if ((*p) && ((*p)->flag & XS_DONT_ENUM_FLAG)) {
						*lengths++ = length;
						c_memcpy(aBuffer.current, (*p)->value.key.string, length);
					}
					else {
						*lengths++ = mxSymbolBit | length;
						if (length > 1)
							c_memcpy(aBuffer.current, (*p)->value.string, length);
						else
							*aBuffer.current = 0;
					}
					aBuffer.current += length;
					dstIndex++;
				}
				map++;
				p++;
			}
		}
		mxMarshallAlign(aBuffer.current, aBuffer.symbolSize);
		
		fxMarshallSlot(the, the->stack, &aSlot, &aBuffer, alien);
		aSlot = aBuffer.link;
		while (aSlot) {
			bSlot = aSlot->value.instance.garbage;
			aSlot->flag &= ~XS_MARK_FLAG;
			aSlot->value.instance.garbage = C_NULL;
			aSlot = bSlot;
		}
		
		mxCheck(the, aBuffer.current == aBuffer.base + aBuffer.size);
	}
	mxCatch(the) {
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
		if (aBuffer.base)
			c_free(aBuffer.base);
		if (aBuffer.symbolMap)
			c_free(aBuffer.symbolMap);
		fxJump(the);
	}
	mxPop();
	c_free(aBuffer.symbolMap);
	return aBuffer.base;
}

void fxMarshallChunk(txMachine* the, void* theData, void** theDataAddress, txMarshallBuffer* theBuffer)
{
	txChunk* aChunk = ((txChunk*)(((txByte*)theData) - sizeof(txChunk)));
	txSize aSize = aChunk->size & 0x7FFFFFFF;
	txByte* aResult = theBuffer->current;
	theBuffer->current += aSize;
	c_memcpy(aResult, aChunk, aSize);
	aChunk = (txChunk*)aResult;
	aChunk->size &= 0x7FFFFFFF;
	*theDataAddress = aResult + sizeof(txChunk);
	mxMarshallAlign(theBuffer->current, aSize);
}

txID fxMarshallKey(txMachine* the, txID id, txMarshallBuffer* theBuffer, txBoolean alien)
{
	if (id != XS_NO_ID) {
		if (alien) {
            id = theBuffer->symbolMap[id];
		}
		else if (id >= the->keyOffset) {
            id = theBuffer->symbolMap[id - the->keyOffset];
        }
	}
	return id;
}

void fxMarshallReference(txMachine* the, txSlot* theSlot, txSlot** theSlotAddress, txMarshallBuffer* theBuffer, txBoolean alien)
{
	if (!alien && (theSlot->flag & XS_DONT_MARSHALL_FLAG))
		*theSlotAddress = theSlot;
	else if (theSlot->value.instance.garbage)
		*theSlotAddress = theSlot->value.instance.garbage;
	else
		fxMarshallSlot(the, theSlot, theSlotAddress, theBuffer, alien);
}

txBoolean fxMarshallSlot(txMachine* the, txSlot* theSlot, txSlot** theSlotAddress, txMarshallBuffer* theBuffer, txBoolean alien)
{
	txSlot* aResult;
	txSlot* aSlot;
	txSlot** aSlotAddress;
	
	aResult = (txSlot*)(theBuffer->current);
	theBuffer->current += sizeof(txSlot);
	if (!(theSlot->flag & XS_INTERNAL_FLAG))
		aResult->ID = fxMarshallKey(the, theSlot->ID, theBuffer, alien);
	else
		aResult->ID = theSlot->ID;
	aResult->flag = theSlot->flag;
	aResult->kind = theSlot->kind;
	aResult->value = theSlot->value;
	*theSlotAddress = aResult;
	switch (theSlot->kind) {
	case XS_UNDEFINED_KIND:
	case XS_NULL_KIND:
	case XS_BOOLEAN_KIND:
	case XS_INTEGER_KIND:
	case XS_NUMBER_KIND:
	case XS_DATE_KIND:
	case XS_STRING_X_KIND:
	case XS_BIGINT_X_KIND:
	case XS_DATA_VIEW_KIND:
	case XS_KEY_X_KIND:
	case XS_BUFFER_INFO_KIND:
		break;
		
	case XS_STRING_KIND:
		fxMarshallChunk(the, theSlot->value.string, (void **)&(aResult->value.string), theBuffer);
		break;
	case XS_BIGINT_KIND:
		fxMarshallChunk(the, theSlot->value.bigint.data, (void **)&(aResult->value.bigint.data), theBuffer);
		break;
	case XS_ARRAY_BUFFER_KIND: 
		if (theSlot->value.arrayBuffer.address)
			fxMarshallChunk(the, theSlot->value.arrayBuffer.address, (void **) &(aResult->value.arrayBuffer.address), theBuffer);
		break;
	case XS_REGEXP_KIND:
		if (theSlot->value.regexp.code)
			fxMarshallChunk(the, theSlot->value.regexp.code, (void**)&(aResult->value.regexp.code), theBuffer);
		if (theSlot->value.regexp.data)
			fxMarshallChunk(the, theSlot->value.regexp.data, (void**)&(aResult->value.regexp.data), theBuffer);
		break;	
	case XS_KEY_KIND:
		if (theSlot->value.key.string)
			fxMarshallChunk(the, theSlot->value.key.string, (void **)&(aResult->value.key.string), theBuffer);
		break;
		
	case XS_SYMBOL_KIND:
		aResult->value.symbol = fxMarshallKey(the, theSlot->value.symbol, theBuffer, alien);
		break;
		
	case XS_HOST_KIND: 
		break;
	case XS_MAP_KIND:
	case XS_SET_KIND:
		aResult->value.table.address = C_NULL;
		break;	
	case XS_TYPED_ARRAY_KIND:
		aResult->value.integer = (txInteger)(theSlot->value.typedArray.dispatch - &gxTypeDispatches[0]);
		break;
		
	case XS_REFERENCE_KIND:
		fxMarshallReference(the, theSlot->value.reference, &aResult->value.reference,  theBuffer, alien);
		break;
	case XS_INSTANCE_KIND:
		aResult->value.instance.garbage = theBuffer->link;
		aResult->value.instance.prototype = C_NULL;
		aSlot = theSlot->value.instance.prototype;
		if (!alien && aSlot && (aSlot->flag & XS_DONT_MARSHALL_FLAG))
			aResult->value.instance.prototype = aSlot;
		theSlot->value.instance.garbage = aResult;
		theBuffer->link = theSlot;
		aSlotAddress = &(aResult->next);
		aSlot = theSlot->next;
		while (aSlot) {
			if (fxMarshallSlot(the, aSlot, aSlotAddress, theBuffer, alien))
				aSlotAddress = &((*aSlotAddress)->next);
			aSlot = aSlot->next;
		}
		*aSlotAddress = C_NULL;
		break;	
	case XS_ARRAY_KIND:  {
		txIndex length = theSlot->value.array.length;
		txIndex size = fxGetIndexSize(the, theSlot);
		txSlot* slot = theSlot->value.array.address;
		txSlot* limit = slot + size;
		txIndex former = 0, current;	
		aSlotAddress = &(aResult->value.array.address);
		while (slot < limit) {
			current = *((txIndex*)slot);
			while (former < current) {
				fxMarshallSlot(the, &mxUndefined, aSlotAddress, theBuffer, alien);
				aSlotAddress = &((*aSlotAddress)->next);
				former++;
			}
			fxMarshallSlot(the, slot, aSlotAddress, theBuffer, alien);
			aSlotAddress = &((*aSlotAddress)->next);
			slot++;
			former++;
		}
		while (former < length) {
			fxMarshallSlot(the, &mxUndefined, aSlotAddress, theBuffer, alien);
			aSlotAddress = &((*aSlotAddress)->next);
			former++;
		}
		} break;
	case XS_ERROR_KIND:
		if (theSlot->value.error.info)
			fxMarshallReference(the, theSlot->value.error.info, &aResult->value.error.info, theBuffer, alien);
		break;	
	case XS_PROXY_KIND:
		if (theSlot->value.proxy.handler)
			fxMarshallReference(the, theSlot->value.proxy.handler, &aResult->value.proxy.handler, theBuffer, alien);
		if (theSlot->value.proxy.target)
			fxMarshallReference(the, theSlot->value.proxy.target, &aResult->value.proxy.target, theBuffer, alien);
		break;	

	case XS_LIST_KIND:
		aSlot = theSlot->value.list.first;
		aSlotAddress = &(aResult->value.list.first);
		while (aSlot) {
			fxMarshallSlot(the, aSlot, aSlotAddress, theBuffer, alien);
			aResult->value.list.last = *aSlotAddress;
			aSlot = aSlot->next;
			aSlotAddress = &((*aSlotAddress)->next);
		}
		*aSlotAddress = C_NULL;
		break;	
	case XS_PRIVATE_KIND:
		aSlot = theSlot->value.private.check;
		if (!alien && (aSlot->flag & XS_DONT_MARSHALL_FLAG)) {
			aSlot = theSlot->value.private.first;
			aSlotAddress = &(aResult->value.private.first);
			while (aSlot) {
				fxMarshallSlot(the, aSlot, aSlotAddress, theBuffer, alien);
				aSlot = aSlot->next;
				aSlotAddress = &((*aSlotAddress)->next);
			}
			*aSlotAddress = C_NULL;
		}
		else {
			theBuffer->current -= sizeof(txSlot);
			return 0;
		}
		break;	
	default:
		break;
	}
	return 1;
}

void fxMeasureChunk(txMachine* the, void* theData, txMarshallBuffer* theBuffer)
{
	txChunk* aChunk = ((txChunk*)(((txByte*)theData) - sizeof(txChunk)));
	txSize aSize = aChunk->size & 0x7FFFFFFF;
	theBuffer->size += aSize;
	mxMarshallAlign(theBuffer->size, aSize);
}

void fxMeasureKey(txMachine* the, txID theID, txMarshallBuffer* theBuffer, txBoolean alien)
{
	txSlot* aSlot;
	txSize aLength;
	if (theID != XS_NO_ID) {
		if (alien) {
			if (theBuffer->symbolMap[theID])
				return;
			if (theID >= the->keyOffset)
				aSlot = the->keyArray[theID - the->keyOffset];
			else
				aSlot = the->keyArrayHost[theID];
		}
		else if (theID >= the->keyOffset) {
			theID -= the->keyOffset;
			if (theBuffer->symbolMap[theID])
				return;
			aSlot = the->keyArray[theID];
		}
		else
			return;
		if (aSlot) {
			if (aSlot->flag & XS_DONT_ENUM_FLAG) {
				aLength = mxStringLength(aSlot->value.key.string) + 1;
			}
			else if (aSlot->kind != XS_UNDEFINED_KIND) {
				aLength = mxStringLength(aSlot->value.string) + 1;
			}
			else
				aLength = 1;
		}
		else
			aLength = 1;
// 		if (aLength > XS_ID_MASK)
// 			mxRangeError("marshall: key overflow");
		theBuffer->symbolMap[theID] = (txID)aLength;
		theBuffer->symbolSize += sizeof(txID);
		theBuffer->symbolSize += aLength;
		theBuffer->symbolCount++;
	}
}

void fxMeasureReference(txMachine* the, txSlot* theSlot, txMarshallBuffer* theBuffer, txBoolean alien)
{
	if (theSlot->flag & XS_DONT_MARSHALL_FLAG) {
		if (alien)
			if (theSlot->value.host.variant.destructor != fxReleaseSharedChunk)
				fxMeasureThrow(the, theBuffer, "read only object");
	}
	else if ((theSlot->flag & XS_MARK_FLAG) == 0)
		fxMeasureSlot(the, theSlot, theBuffer, alien);
}

void fxMeasureSlot(txMachine* the, txSlot* theSlot, txMarshallBuffer* theBuffer, txBoolean alien)
{
	txSlot* aSlot;

	if (!(theSlot->flag & XS_INTERNAL_FLAG))
		fxMeasureKey(the, theSlot->ID, theBuffer, alien);
	theBuffer->size += sizeof(txSlot);
	switch (theSlot->kind) {
	case XS_UNDEFINED_KIND:
	case XS_NULL_KIND:
	case XS_BOOLEAN_KIND:
	case XS_INTEGER_KIND:
	case XS_NUMBER_KIND:
	case XS_DATE_KIND:
	case XS_STRING_X_KIND:
	case XS_BIGINT_X_KIND:
	case XS_DATA_VIEW_KIND:
	case XS_KEY_X_KIND:
	case XS_BUFFER_INFO_KIND:
		break;
		
	case XS_STRING_KIND:
		fxMeasureChunk(the, theSlot->value.string, theBuffer);
		break;
	case XS_BIGINT_KIND:
		fxMeasureChunk(the, theSlot->value.bigint.data, theBuffer);
		break;
	case XS_ARRAY_BUFFER_KIND: 
		if (theSlot->value.arrayBuffer.address)
			fxMeasureChunk(the, theSlot->value.arrayBuffer.address, theBuffer);
		break;
	case XS_REGEXP_KIND:
		if (theSlot->value.regexp.code)
			fxMeasureChunk(the, theSlot->value.regexp.code, theBuffer);
		if (theSlot->value.regexp.data)
			fxMeasureChunk(the, theSlot->value.regexp.data, theBuffer);
		break;	
	case XS_KEY_KIND:
		if (theSlot->value.key.string)
			fxMeasureChunk(the, theSlot->value.string, theBuffer);
		break;
		
	case XS_SYMBOL_KIND:
		fxMeasureKey(the, theSlot->value.symbol, theBuffer, alien);
		break;
		
	case XS_HOST_KIND: 
		if (theSlot->value.host.variant.destructor != fxReleaseSharedChunk)
			fxMeasureThrow(the, theBuffer, "host object");
		break;
	case XS_MAP_KIND:
	case XS_SET_KIND:
		break;	
	case XS_TYPED_ARRAY_KIND:
		break;
		
	case XS_REFERENCE_KIND:
		fxMeasureReference(the, theSlot->value.reference, theBuffer, alien);
		break;
	case XS_INSTANCE_KIND:
		theSlot->flag |= XS_MARK_FLAG;
		theSlot->value.instance.garbage = C_NULL;
		aSlot = theSlot->next;
		while (aSlot) {
			if (aSlot->flag & XS_INTERNAL_FLAG)
				mxPushUndefined();
			else
				mxPushAt(aSlot->ID, 0);
			fxMeasureSlot(the, aSlot, theBuffer, alien);
			mxPop();
			aSlot = aSlot->next;
		}
		break;	
	case XS_ARRAY_KIND: {
		txIndex length = theSlot->value.array.length;
		txIndex size = fxGetIndexSize(the, theSlot);
		txSlot* slot = theSlot->value.array.address;
		txSlot* limit = slot + size;
		txIndex former = 0, current;	
		while (slot < limit) {
			current = *((txIndex*)slot);
			while (former < current) {
				theBuffer->size += sizeof(txSlot);
				former++;
			}
			mxPushAt(XS_NO_ID, *((txIndex*)slot));
			fxMeasureSlot(the, slot, theBuffer, alien);
			mxPop();
			slot++;
			former++;
		}
		while (former < length) {
			theBuffer->size += sizeof(txSlot);
			former++;
		}
		} break;
	case XS_ERROR_KIND:
		if (theSlot->value.error.info)
			fxMeasureReference(the, theSlot->value.error.info, theBuffer, alien);
		break;
	case XS_PROXY_KIND:
		if (theSlot->value.proxy.handler)
			fxMeasureReference(the, theSlot->value.proxy.handler, theBuffer, alien);
		if (theSlot->value.proxy.target)
			fxMeasureReference(the, theSlot->value.proxy.target, theBuffer, alien);
		break;	

	case XS_LIST_KIND:
		aSlot = theSlot->value.list.first;
		while (aSlot) {
			fxMeasureSlot(the, aSlot, theBuffer, alien);
			aSlot = aSlot->next;
		}
		break;
	case XS_PRIVATE_KIND:
		aSlot = theSlot->value.private.check;
		if (!alien && (aSlot->flag & XS_DONT_MARSHALL_FLAG)) {
			aSlot = theSlot->value.private.first;
			while (aSlot) {
				fxMeasureSlot(the, aSlot, theBuffer, alien);
				aSlot = aSlot->next;
			}
		}
		else
			theBuffer->size -= sizeof(txSlot);
		break;
		
	case XS_ARGUMENTS_SLOPPY_KIND:
	case XS_ARGUMENTS_STRICT_KIND:
		fxMeasureThrow(the, theBuffer, "arguments");
		break;
	case XS_CALLBACK_KIND:
	case XS_CALLBACK_X_KIND:
	case XS_CODE_KIND:
	case XS_CODE_X_KIND:
#ifdef mxHostFunctionPrimitive
	case XS_HOST_FUNCTION_KIND:
#endif
		fxMeasureThrow(the, theBuffer, "function");
		break;
	case XS_FINALIZATION_CELL_KIND:
	case XS_FINALIZATION_REGISTRY_KIND:
		fxMeasureThrow(the, theBuffer, "finalization");
		break;
	case XS_MODULE_KIND:
	case XS_PROGRAM_KIND:
		fxMeasureThrow(the, theBuffer, "module");
		break;
	case XS_PROMISE_KIND:
		fxMeasureThrow(the, theBuffer, "promise");
		break;
	case XS_WEAK_MAP_KIND:
		fxMeasureThrow(the, theBuffer, "weak map");
		break;
	case XS_WEAK_REF_KIND:
		fxMeasureThrow(the, theBuffer, "weak ref");
		break;
	case XS_WEAK_SET_KIND:
		fxMeasureThrow(the, theBuffer, "weak set");
		break;
	case XS_ACCESSOR_KIND:
		fxMeasureThrow(the, theBuffer, "accessor");
		break;
	case XS_STACK_KIND:
		fxMeasureThrow(the, theBuffer, "generator");
		break;
	default:
		fxMeasureThrow(the, theBuffer, "no way");
		break;
	}
}

void fxMeasureThrow(txMachine* the, txMarshallBuffer* theBuffer, txString message)
{
	char buffer[128] = "";
	txSlot* slot = theBuffer->stack;
	txInteger i = 0;
	txInteger c = sizeof(buffer);
	i += c_snprintf(buffer, c, "marshall ");
	while (slot > the->stack) {
		slot--;
		if (slot->kind == XS_AT_KIND) {
			if (slot->value.at.id != XS_NO_ID) {
				txSlot* key = fxGetKey(the, slot->value.at.id);
				if (key) {
					txKind kind = mxGetKeySlotKind(key);
					if ((kind == XS_KEY_KIND) || (kind == XS_KEY_X_KIND)) {
						if (i < c) i += c_snprintf(buffer + i, c - i, ".%s", key->value.string);
					}
				}
			}
			else {
				if (i < c) i += c_snprintf(buffer + i, c - i, "[%d]", (int)slot->value.at.index);
			}
		}
	}
	if (i < c)
		i += c_snprintf(buffer + i, c - i, ": %s", message);
	mxTypeError(buffer);
}

