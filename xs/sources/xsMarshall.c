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

typedef struct sxMarshallBuffer txMarshallBuffer; 
struct sxMarshallBuffer {
	txByte* base;
	txByte* current;
	txSlot* link;
	txID* symbolMap;
	txSize size;
	txID symbolCount;
	txIndex symbolSize;
	txBoolean shared;
};

static void fxDemarshallChunk(txMachine* the, void* theData, void** theDataAddress);
static void fxDemarshallSlot(txMachine* the, txSlot* theSlot, txSlot* theResult, txID* theSymbolMap, txBoolean alien);
static void fxMarshallChunk(txMachine* the, void* theData, void** theDataAddress, txMarshallBuffer* theBuffer);
static void fxMarshallSlot(txMachine* the, txSlot* theSlot, txSlot** theSlotAddress, txMarshallBuffer* theBuffer, txBoolean alien);
static void fxMeasureChunk(txMachine* the, void* theData, txMarshallBuffer* theBuffer);
static void fxMeasureSlot(txMachine* the, txSlot* theSlot, txMarshallBuffer* theBuffer, txBoolean alien);

#define mxMarshallAlign(POINTER,SIZE) \
	if (((SIZE) &= ((sizeof(txNumber) - 1)))) (POINTER) += sizeof(txNumber) - (SIZE)

void fxDemarshall(txMachine* the, void* theData, txBoolean alien)
{
	txFlag aFlag;
	txByte* p = (txByte*)theData;
	txByte* q = p + *((txSize*)(p));
	txID aSymbolCount;
	txID aSymbolLength;
	txID* aSymbolMap;
	txID* aSymbolPointer;
	txSlot* aSlot;
	txChunk* aChunk;
	txInteger skipped;
	txIndex aLength;
	
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
				txID id = fxNewNameC(the, (char *)p);
				aSymbolLength = *aSymbolPointer;
				*aSymbolPointer++ = id;
				aSymbolCount--;
				p += aSymbolLength;
			}
			aLength = p - (txByte*)theData;
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
			case XS_ARRAY_BUFFER_KIND:
				aChunk = (txChunk*)p;
				p += aChunk->size;
				mxMarshallAlign(p, aChunk->size);
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

void fxDemarshallSlot(txMachine* the, txSlot* theSlot, txSlot* theResult, txID* theSymbolMap, txBoolean alien)
{
	txID anID;
	txSlot* aSlot;
	txIndex aLength, index;
	txSlot* aResult;
	txSlot** aSlotAddress;
	
	anID = theSlot->ID;
	if (anID < XS_NO_ID) {
		txID anIndex = anID & 0x7FFF;
		if (alien)
			anID = theSymbolMap[anIndex];
		else if (anIndex >= the->keyOffset)
			anID = theSymbolMap[anIndex - the->keyOffset];
	}
	theResult->ID = anID;
	theResult->flag = theSlot->flag;
	switch (theSlot->kind) {
	case XS_UNDEFINED_KIND:
	case XS_NULL_KIND:
	case XS_BOOLEAN_KIND:
	case XS_INTEGER_KIND:
	case XS_NUMBER_KIND:
	case XS_DATE_KIND:
	case XS_STRING_X_KIND:
		theResult->value = theSlot->value;
		theResult->kind = theSlot->kind;
		break;
	case XS_STRING_KIND: {
		char **s = &theResult->value.string;
		fxDemarshallChunk(the, theSlot->value.string, (void **)s);
		theResult->kind = theSlot->kind;
		}
		break;
	case XS_ARRAY_BUFFER_KIND: 
		if (theSlot->value.arrayBuffer.address) {
			txByte **s = &theResult->value.arrayBuffer.address;
			fxDemarshallChunk(the, theSlot->value.arrayBuffer.address, (void **)s);
            theResult->value.arrayBuffer.length = theSlot->value.arrayBuffer.length;
            theResult->kind = theSlot->kind;
		}
		break;
	case XS_HOST_KIND:
		theResult->value.host.data = fxRetainSharedChunk(theSlot->value.host.data);
		theResult->value.host.variant.destructor = fxReleaseSharedChunk;
		theResult->kind = theSlot->kind;
		break;
	case XS_REFERENCE_KIND:
		aSlot = theSlot->value.reference;
		if (aSlot->value.instance.garbage) {
			theResult->value.reference = aSlot->value.instance.garbage;
			theResult->kind = theSlot->kind;
		}
		else {
			theResult->value.reference = fxNewSlot(the);
			theResult->kind = theSlot->kind;
			fxDemarshallSlot(the, aSlot, theResult->value.reference, theSymbolMap, alien);
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
	case XS_INSTANCE_KIND:
		theSlot->value.instance.garbage = theResult;
		theResult->value.instance.garbage = C_NULL;
		theResult->value.instance.prototype = mxObjectPrototype.value.reference;
		theResult->kind = theSlot->kind;
		aSlot = theSlot->next;
		if (aSlot) {
			if (aSlot->ID == XS_ARRAY_BEHAVIOR)
				theResult->value.instance.prototype = mxArrayPrototype.value.reference;
			else if (aSlot->flag & XS_INTERNAL_FLAG) {
				switch (aSlot->kind) {
				case XS_ARRAY_KIND: theResult->value.instance.prototype = mxArrayPrototype.value.reference; break;
				case XS_BOOLEAN_KIND: theResult->value.instance.prototype = mxBooleanPrototype.value.reference; break;
				case XS_DATE_KIND: theResult->value.instance.prototype = mxDatePrototype.value.reference; break;
				case XS_NUMBER_KIND: theResult->value.instance.prototype = mxNumberPrototype.value.reference; break;
				case XS_STRING_KIND: theResult->value.instance.prototype = mxStringPrototype.value.reference; break;
				case XS_ARRAY_BUFFER_KIND: theResult->value.instance.prototype = mxArrayBufferPrototype.value.reference; break;
				case XS_HOST_KIND: theResult->value.instance.prototype = mxSharedArrayBufferPrototype.value.reference; break;
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
		aBuffer.symbolSize = sizeof(txSize) + sizeof(txID);
		if (alien) {
			aBuffer.symbolMap = c_calloc(the->keyIndex, sizeof(txID));
			if (!aBuffer.symbolMap)
				mxRangeError("marshall: cannot allocate buffer");
		}
        the->stack->ID = XS_NO_ID;
		fxMeasureSlot(the, the->stack, &aBuffer, alien);
		aBuffer.size += aBuffer.symbolSize;
		mxMarshallAlign(aBuffer.size, aBuffer.symbolSize);
		aBuffer.base = aBuffer.current = (txByte *)c_malloc(aBuffer.size);
		if (!aBuffer.base)
			mxRangeError("marshall: cannot allocate buffer");
		*((txSize*)(aBuffer.current)) = aBuffer.size;
		aBuffer.current += sizeof(txSize);
		*((txID*)(aBuffer.current)) = aBuffer.symbolCount;
		aBuffer.current += sizeof(txID);
		if (aBuffer.symbolCount) {
			txID* lengths = (txID*)aBuffer.current;
			txSlot** p;
			txSlot** q;
			aBuffer.current += aBuffer.symbolCount * sizeof(txID);
			if (alien) {
				txID* map = aBuffer.symbolMap;
				txID dstIndex = 0;
				p = the->keyArrayHost;
				q = p + the->keyOffset;
				while (p < q) {
					txID length = *map;
					if (length) {
						*lengths++ = length;
						*map = dstIndex | 0x8000;
						c_memcpy(aBuffer.current, (*p)->value.key.string, length);
						aBuffer.current += length;
						dstIndex++;
					}
					map++;
					p++;
				}
				p = the->keyArray;
				q = p + the->keyIndex - the->keyOffset;
				while (p < q) {
					txID length = *map;
					if (length) {
						*lengths++ = length;
						*map = dstIndex | 0x8000;
						c_memcpy(aBuffer.current, (*p)->value.key.string, length);
						aBuffer.current += length;
						dstIndex++;
					}
					map++;
					p++;
				}
			}
			else {
				txID dstIndex = the->keyOffset;
				p = the->keyArray;
				q = p + the->keyIndex - the->keyOffset;
				while (p < q) {
					aSlot = *p;
					if (aSlot->flag & XS_STRICT_FLAG) {
						txID length = aSlot->ID;
						*lengths++ = length;
						aSlot->ID = dstIndex | 0x8000;
						c_memcpy(aBuffer.current, aSlot->value.key.string, length);
						aBuffer.current += length;
						dstIndex++;
					}
					p++;
				}
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
		if (aBuffer.base) {
			c_free(aBuffer.base);
			aBuffer.base = C_NULL;
		}
		break;
	}
	if (!alien && aBuffer.symbolCount) {
		txID anIndex = the->keyOffset;
		txSlot** p = the->keyArray;
		txSlot** q = p + the->keyIndex - the->keyOffset;
		while (p < q) {
			aSlot = *p;
			if (aSlot->flag & XS_STRICT_FLAG) {
				aSlot->flag &= ~XS_STRICT_FLAG;
				aSlot->ID = anIndex | 0x8000;
			}
			p++;
			anIndex++;
		}
	}
	the->stack++;
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

void fxMarshallSlot(txMachine* the, txSlot* theSlot, txSlot** theSlotAddress, txMarshallBuffer* theBuffer, txBoolean alien)
{
	txSlot* aResult;
	txID anID;
	txSlot* aSlot;
	txSlot** aSlotAddress;
	
	aResult = (txSlot*)(theBuffer->current);
	theBuffer->current += sizeof(txSlot);
	anID = theSlot->ID;
	if (anID < XS_NO_ID) {
		txID anIndex = anID & 0x7FFF;
		if (alien) {
            anID = theBuffer->symbolMap[anIndex];
		}
		else if (anIndex >= the->keyOffset) {
            aSlot = the->keyArray[anIndex - the->keyOffset];
            anID = aSlot->ID;
        }
	}
	aResult->ID = anID;
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
		break;
	case XS_STRING_KIND: {
		char **s = &aResult->value.string;
		fxMarshallChunk(the, theSlot->value.string, (void **)s, theBuffer);
		}
		break;
	case XS_ARRAY_BUFFER_KIND: 
		if (theSlot->value.arrayBuffer.address) {
			txByte **s = &aResult->value.arrayBuffer.address;
			fxMarshallChunk(the, theSlot->value.arrayBuffer.address, (void **)s, theBuffer);
		}
		break;
	case XS_HOST_KIND: 
		break;
	case XS_REFERENCE_KIND:
		aSlot = theSlot->value.reference;
		if (aSlot->value.instance.garbage)
			aResult->value.reference = aSlot->value.instance.garbage;
		else
			fxMarshallSlot(the, aSlot, &(aResult->value.reference), theBuffer, alien);
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
	case XS_INSTANCE_KIND:
		aResult->value.instance.garbage = theBuffer->link;
		aResult->value.instance.prototype = C_NULL;
		theSlot->value.instance.garbage = aResult;
		theBuffer->link = theSlot;
		aSlot = theSlot->next;
		aSlotAddress = &(aResult->next);
		while (aSlot) {
			fxMarshallSlot(the, aSlot, aSlotAddress, theBuffer, alien);
			aSlot = aSlot->next;
			aSlotAddress = &((*aSlotAddress)->next);
		}
		*aSlotAddress = C_NULL;
		break;	
	default:
		break;
	}
}

void fxMeasureChunk(txMachine* the, void* theData, txMarshallBuffer* theBuffer)
{
	txChunk* aChunk = ((txChunk*)(((txByte*)theData) - sizeof(txChunk)));
	txSize aSize = aChunk->size & 0x7FFFFFFF;
	theBuffer->size += aSize;
	mxMarshallAlign(theBuffer->size, aSize);
}

void fxMeasureSlot(txMachine* the, txSlot* theSlot, txMarshallBuffer* theBuffer, txBoolean alien)
{
	txID anID;
	txSlot* aSlot;
	txIndex aLength;

	anID = theSlot->ID;
	if (anID < XS_NO_ID) {
		txID anIndex = anID & 0x7FFF;
		if (alien) {
			if (!theBuffer->symbolMap[anIndex]) {
				if (anIndex >= the->keyOffset)
					aSlot = the->keyArray[anIndex - the->keyOffset];
				else
					aSlot = the->keyArrayHost[anIndex];
				if (!(aSlot->flag & XS_DONT_ENUM_FLAG))
					mxTypeError("marshall: no symbols");
				aLength = c_strlen(aSlot->value.key.string) + 1;
				if (aLength > 0x7FFF)
					mxRangeError("marshall: property name overflow");
				theBuffer->symbolMap[anIndex] = (txID)aLength;
				theBuffer->symbolSize += sizeof(txID);
				theBuffer->symbolSize += aLength;
				theBuffer->symbolCount++;
			}
		}
		else if (anIndex >= the->keyOffset) {
			aSlot = the->keyArray[anIndex - the->keyOffset];
			if (!(aSlot->flag & XS_STRICT_FLAG)) {
				if (!(aSlot->flag & XS_DONT_ENUM_FLAG))
					mxTypeError("marshall: no symbols");
				aLength = c_strlen(aSlot->value.key.string) + 1;
				if (aLength > 0x7FFF)
					mxRangeError("marshall: property name overflow");
				aSlot->flag |= XS_STRICT_FLAG;
				aSlot->ID = (txID)aLength;
				theBuffer->symbolSize += sizeof(txID);
				theBuffer->symbolSize += aLength;
				theBuffer->symbolCount++;
			}
		}
	}
	theBuffer->size += sizeof(txSlot);
	switch (theSlot->kind) {
	case XS_UNDEFINED_KIND:
	case XS_NULL_KIND:
	case XS_BOOLEAN_KIND:
	case XS_INTEGER_KIND:
	case XS_NUMBER_KIND:
	case XS_DATE_KIND:
	case XS_STRING_X_KIND:
		break;
	case XS_STRING_KIND:
		fxMeasureChunk(the, theSlot->value.string, theBuffer);
		break;
	case XS_ARRAY_BUFFER_KIND: 
		if (theSlot->value.arrayBuffer.address)
			fxMeasureChunk(the, theSlot->value.arrayBuffer.address, theBuffer);
		break;
	case XS_HOST_KIND: 
		if (theSlot->value.host.variant.destructor != fxReleaseSharedChunk)
			mxDebugID(XS_TYPE_ERROR, "marshall %s: no way", theSlot->ID);
		break;
	case XS_REFERENCE_KIND:
		aSlot = theSlot->value.reference;
		if ((aSlot->flag & XS_MARK_FLAG) == 0)
			fxMeasureSlot(the, aSlot, theBuffer, alien);
		else if (aSlot->flag & XS_DONT_MARSHALL_FLAG)
			mxDebugID(XS_TYPE_ERROR, "marshall %s: no way", theSlot->ID);
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
			fxMeasureSlot(the, slot, theBuffer, alien);
			slot++;
			former++;
		}
		while (former < length) {
			theBuffer->size += sizeof(txSlot);
			former++;
		}
		} break;
	case XS_INSTANCE_KIND:
		theSlot->flag |= XS_MARK_FLAG;
		theSlot->value.instance.garbage = C_NULL;
		aSlot = theSlot->next;
		while (aSlot) {
			fxMeasureSlot(the, aSlot, theBuffer, alien);
			aSlot = aSlot->next;
		}
		break;	
	default:
		mxDebugID(XS_TYPE_ERROR, "marshall %s: no way", theSlot->ID);
		break;
	}
}

