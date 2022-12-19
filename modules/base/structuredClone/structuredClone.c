/*
 * Copyright (c) 2018-2022  Moddable Tech, Inc.
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
 */

#include "xsAll.h"
#include "xsScript.h"

typedef struct sx_structuredCloneLink tx_structuredCloneLink;
typedef struct sx_structuredCloneList tx_structuredCloneList;

struct sx_structuredCloneLink {
	tx_structuredCloneLink* previous;
	tx_structuredCloneLink* next;
	txID ID;
	txIndex index;
};

struct sx_structuredCloneList {
	tx_structuredCloneLink *first;
	tx_structuredCloneLink* last;
};

static void fx_structuredCloneEntries(txMachine* the, tx_structuredCloneList* list, txSlot* from, txSlot* to, txBoolean paired);
static void fx_structuredCloneInstance(txMachine* the, tx_structuredCloneList* list);
static txSlot* fx_structuredCloneProperty(txMachine* the, tx_structuredCloneList* list, txSlot* from, txSlot* to, txID id);
static void fx_structuredCloneSlot(txMachine* the, tx_structuredCloneList* list);
static void fx_structuredCloneThrow(txMachine* the, tx_structuredCloneList* list, txString message);
static txBoolean fx_structuredCloneTransferable(txMachine* the, txSlot* instance);

#if 0
static txID doNotStrip[3] = [ xsID_Map, xsID_get, xsID_set ];
#endif

void fx_structuredClone(txMachine* the)
{
	tx_structuredCloneLink link = { C_NULL, C_NULL, mxID(_value), 0 };
	tx_structuredCloneList list = { &link, &link };
	fxVars(the, 4);
	
	if (mxArgc == 0)
		mxTypeError("no value");
	
	mxPush(mxMapConstructor);
	mxNew();
	mxRunCount(0);
	mxPullSlot(mxVarv(0));
	
	mxPushSlot(mxVarv(0));
	mxGetID(mxID(_get));
	mxPullSlot(mxVarv(1));
	
	mxPushSlot(mxVarv(0));
	mxGetID(mxID(_set));
	mxPullSlot(mxVarv(2));
	
	mxPushList();
	if (mxArgc > 1) {
		mxPushSlot(mxArgv(1));
		if (fxRunTest(the)) {
			mxPushSlot(mxArgv(1));
			if (mxHasID(mxID(_transfer))) {
				txSlot* transferList = the->stack;
				txSlot* transferArray;
				txSlot* transferItem;
				txSlot* transferArrayBuffer;
				txSlot** transferAddress;
				txSlot* transferLink;
				txIndex c, i;
				mxPushSlot(mxArgv(1));
				mxGetID(mxID(_transfer));
				transferArray = the->stack;
				mxPushSlot(transferArray);
				mxGetID(mxID(_length));
				c = (txIndex)fxToInteger(the, the->stack);
				mxPop();
				for (i = 0; i < c; i++) {
					mxPushSlot(transferArray);
					mxGetIndex(i);
					transferItem = the->stack;
					if (!mxIsReference(transferItem))
						mxTypeError("tranfer[%d]: no object", i);
					transferArrayBuffer = transferItem->value.reference->next;
					if (!transferArrayBuffer || (transferArrayBuffer->kind != XS_ARRAY_BUFFER_KIND))
						mxTypeError("tranfer[%d]: no array buffer", i);
					transferAddress = &transferList->value.list.first;
					while ((transferLink = *transferAddress)) {
						if (transferLink->value.reference == transferItem->value.reference)
							mxTypeError("tranfer[%d]: duplicate", i);
						transferAddress = &transferLink->next;
					}
					transferLink = *transferAddress = fxNewSlot(the);
					transferLink->value.reference = transferItem->value.reference;
					transferLink->kind = transferItem->kind;
					mxPop();
				}
				transferList->value.list.last = transferLink;
				mxPop();
			}
		}
	}
	mxPullSlot(mxVarv(3));

	mxPushSlot(mxArgv(0));
	fx_structuredCloneSlot(the, &list);
	mxPullSlot(mxResult);
}

void fx_structuredCloneEntries(txMachine* the, tx_structuredCloneList* list, txSlot* from, txSlot* to, txBoolean paired) 
{
	tx_structuredCloneLink link = { C_NULL, C_NULL, mxID(_entries), 0 };
	tx_structuredCloneLink link0 = { C_NULL, C_NULL, XS_NO_ID, 0 };
	tx_structuredCloneLink link1 = { C_NULL, C_NULL, XS_NO_ID, 0 };
	txSlot* fromTable;
	txSlot* toTable;
	txSlot* fromList;
	txSlot* toList;
	txSlot** address;

	fromTable = from;

	toTable = to->next = fxNewSlot(the);
	toTable->value.table.address = fxNewChunk(the, fromTable->value.table.length * sizeof(txSlot*));
	toTable->value.table.length = fromTable->value.table.length;
	c_memset(toTable->value.table.address, 0, toTable->value.table.length * sizeof(txSlot*));
    toTable->kind = fromTable->kind;
	toTable->flag = fromTable->flag;

	fromList = fromTable->next;
	
	toList = toTable->next = fxNewSlot(the);
	toList->value.list.first = C_NULL;
	toList->value.list.last = C_NULL;
	toList->kind = XS_LIST_KIND;
	toList->flag = XS_INTERNAL_FLAG;

	from = fromList->value.list.first;
	address = &toList->value.list.first;
	while (from) {
		to = *address = fxDuplicateSlot(the, from);
		from = from->next;
		address = &to->next;
	}
	toList->value.list.last = to;

	link.previous = list->last;
	list->last->next = &link;
	list->last = &link;
	link0.previous = list->last;
	list->last->next = &link0;
	list->last = &link0;
	if (paired) {
		link1.previous = list->last;
		list->last->next = &link1;
		list->last = &link1;
	}
	
	to = toList->value.list.first;
	while (to) {
		mxPushSlot(to);
		fx_structuredCloneSlot(the, list);
		mxPullSlot(to);
		if (paired) {
			if (link1.index) {
				link0.index++;
				link1.index = 0;
			}
			else
				link1.index++;
		}
		else
			link0.index++;
		to = to->next;
	}
	
	list->last = link.previous;
	
	to = toList->value.list.first;
	while (to) {
		txSlot* entry = fxNewSlot(the);
		txU4 sum = fxSumEntry(the, to);
		txU4 index = sum & (toTable->value.table.length - 1);
		address = &(toTable->value.table.address[index]);
		entry->next = *address;
		entry->kind = XS_ENTRY_KIND;
		entry->value.entry.slot = to;
		entry->value.entry.sum = sum;
		*address = entry;
		to = to->next;
		if (paired)
			to = to->next;
	}
	
	toList->next = fxDuplicateSlot(the, fromList->next);
}

void fx_structuredCloneInstance(txMachine* the, tx_structuredCloneList* list) 
{
	tx_structuredCloneLink link = { C_NULL, C_NULL, XS_NO_ID, 0 };

	txSlot* instance = the->stack->value.reference;
	txSlot* result;
	txSlot* from;
	txSlot* to;
	txSize size;
	txSlot* toArray;
	txSlot* toBase;
	
	mxPushSlot(mxVarv(0));
	mxPushSlot(mxVarv(1));
	mxCall();
	mxPushReference(instance);
	mxRunCount(1);
	if (mxIsReference(the->stack))
		return;
	mxPop();
	
	
	result = fxNewInstance(the);
	result->value.instance.prototype = mxObjectPrototype.value.reference;
	
	mxPushSlot(mxVarv(0));
	mxPushSlot(mxVarv(2));
	mxCall();
	mxPushReference(instance);
	mxPushReference(result);
	mxRunCount(2);
	mxPop();
	
	from = instance->next;
	to = result;

	if (from && (from->flag & XS_INTERNAL_FLAG)) {
		switch (from->kind) {
		case XS_BOOLEAN_KIND:
			result->value.instance.prototype = mxBooleanPrototype.value.reference;
			to = to->next = fxDuplicateSlot(the, from);
			from = from->next;
			break;
		case XS_NUMBER_KIND:
			result->value.instance.prototype = mxNumberPrototype.value.reference;
			to = to->next = fxDuplicateSlot(the, from);
			from = from->next;
			break;
		case XS_STRING_KIND:
		case XS_STRING_X_KIND:
			result->value.instance.prototype = mxStringPrototype.value.reference;
			result->flag |= XS_EXOTIC_FLAG;
			to = to->next = fxDuplicateSlot(the, from);
			from = from->next;
			break;
		case XS_SYMBOL_KIND:
			fx_structuredCloneThrow(the, list, "symbol");
			break;
		case XS_BIGINT_KIND:
		case XS_BIGINT_X_KIND:
			result->value.instance.prototype = mxBigIntPrototype.value.reference;
			to = to->next = fxDuplicateSlot(the, from);
			from = from->next;
			break;
			
		case XS_ARRAY_KIND:
			if (instance->flag & XS_EXOTIC_FLAG) {
				if ((from->ID == XS_ARGUMENTS_SLOPPY_BEHAVIOR) || (from->ID == XS_ARGUMENTS_STRICT_BEHAVIOR))
					fx_structuredCloneThrow(the, list, "arguments");
				result->value.instance.prototype = mxArrayPrototype.value.reference;
				result->flag |= XS_EXOTIC_FLAG;
			}
			break;
		case XS_ARRAY_BUFFER_KIND:
			result->value.instance.prototype = mxArrayBufferPrototype.value.reference;
			if (!from->value.arrayBuffer.address)
				fx_structuredCloneThrow(the, list, "detached array buffer");
			to = to->next = fxDuplicateSlot(the, from);
			if (fx_structuredCloneTransferable(the, instance)) {
				if (!(from->flag & XS_DONT_SET_FLAG)) {
					from->value.arrayBuffer.address = C_NULL;
					from->next->value.bufferInfo.length = 0;
				}
			}
			else {
				size = from->next->value.bufferInfo.length;
				to->value.arrayBuffer.address = fxNewChunk(the, size);
				c_memcpy(to->value.arrayBuffer.address, from->value.arrayBuffer.address, size);
			}
			from = from->next;
			to = to->next = fxDuplicateSlot(the, from);
			from = from->next;
			break;
		case XS_CALLBACK_KIND:
		case XS_CALLBACK_X_KIND:
		case XS_CODE_KIND:
		case XS_CODE_X_KIND:
			fx_structuredCloneThrow(the, list, "function");
			break;
		case XS_DATA_VIEW_KIND:
			result->value.instance.prototype = mxDataViewPrototype.value.reference;
			to = to->next = fxDuplicateSlot(the, from);
			from = from->next;
			mxPushSlot(from);
			fx_structuredCloneSlot(the, list);
			to = to->next = fxNewSlot(the);
			mxPullSlot(to);
			to->flag = XS_INTERNAL_FLAG;
			from = from->next;
			break;
		case XS_DATE_KIND:
			result->value.instance.prototype = mxDatePrototype.value.reference;
			to = to->next = fxDuplicateSlot(the, from);
			from = from->next;
			break;
		case XS_ERROR_KIND:
			result->value.instance.prototype = mxErrorPrototypes(from->value.error.which).value.reference;
			to = to->next = fxDuplicateSlot(the, from); // info?
			
			link.previous = list->last;
			list->last->next = &link;
			list->last = &link;
			
			to = fx_structuredCloneProperty(the, list, from, to, mxID(_message));
			to = fx_structuredCloneProperty(the, list, from, to, mxID(_cause));
			if (from->value.error.which == XS_AGGREGATE_ERROR)
				to = fx_structuredCloneProperty(the, list, from, to, mxID(_errors));
				
			list->last = link.previous;
			
			from = from->next;
			break;
		case XS_CLOSURE_KIND:
			if (from->value.closure->kind == XS_FINALIZATION_REGISTRY_KIND)
				fx_structuredCloneThrow(the, list, "finalization registry");
			break;
		case XS_GLOBAL_KIND:
			fx_structuredCloneThrow(the, list, "globalThis");
			break;
		case XS_HOST_KIND: 
			if ((from->next == C_NULL) || (from->next->kind != XS_BUFFER_INFO_KIND) || (from->value.host.variant.destructor != fxReleaseSharedChunk))
				fx_structuredCloneThrow(the, list, "host object");
			to = to->next = fxDuplicateSlot(the, from);
			to->value.host.data = fxRetainSharedChunk(from->value.host.data);
			from = from->next;
			to = to->next = fxDuplicateSlot(the, from);
			from = from->next;
			break;
		case XS_MAP_KIND:
			result->value.instance.prototype = mxMapPrototype.value.reference;
			fx_structuredCloneEntries(the, list, from, to, 1);
			from = from->next->next->next;
			to = to->next->next->next;
			break;
		case XS_MODULE_KIND:
			fx_structuredCloneThrow(the, list, "module namespace");
			break;
		case XS_PROGRAM_KIND:
			fx_structuredCloneThrow(the, list, "compartment");
			break;
		case XS_PROMISE_KIND:
			fx_structuredCloneThrow(the, list, "promise");
			break;
		case XS_PROXY_KIND:
			fx_structuredCloneThrow(the, list, "proxy");
			break;
		case XS_REGEXP_KIND:
			result->value.instance.prototype = mxRegExpPrototype.value.reference;
			{
				txInteger flags = from->value.regexp.code[0];
				char modifiers[7];
				char* modifier = &modifiers[0];
				if (flags & XS_REGEXP_D)
					*modifier++ = 'd';
				if (flags & XS_REGEXP_G)
					*modifier++ = 'g';
				if (flags & XS_REGEXP_I)
					*modifier++ = 'i';
				if (flags & XS_REGEXP_M)
					*modifier++ = 'm';
				if (flags & XS_REGEXP_S)
					*modifier++ = 's';
				if (flags & XS_REGEXP_U)
					*modifier++ = 'u';
				if (flags & XS_REGEXP_Y)
					*modifier++ = 'y';
				*modifier = 0;
				
				to = to->next = fxDuplicateSlot(the, from);
				from = from->next;
			
				if (!fxCompileRegExp(the, from->value.key.string, modifiers, &to->value.regexp.code, &to->value.regexp.data, the->nameBuffer, sizeof(the->nameBuffer)))
					mxSyntaxError("invalid regular expression: %s", the->nameBuffer);
			}
			to = to->next = fxDuplicateSlot(the, from);
			from = from->next;
			to = to->next = fxDuplicateSlot(the, from);
			to->value.integer = 0;
			from = from->next;
			break;
		case XS_SET_KIND:
			result->value.instance.prototype = mxSetPrototype.value.reference;
			fx_structuredCloneEntries(the, list, from, to, 0);
			from = from->next->next->next;
			to = to->next->next->next;
			break;
		case XS_STACK_KIND:
			fx_structuredCloneThrow(the, list, "generator");
			break;
		case XS_TYPED_ARRAY_KIND:
			mxPush(the->stackPrototypes[-1 - (txInteger)from->value.typedArray.dispatch->constructorID]);
			mxGetID(mxID(_prototype));
			result->value.instance.prototype = fxToInstance(the, the->stack);
			mxPop();
			result->flag |= XS_EXOTIC_FLAG;
			to = to->next = fxDuplicateSlot(the, from);
			from = from->next;
			to = to->next = fxDuplicateSlot(the, from);
			from = from->next;
			mxPushSlot(from);
			fx_structuredCloneSlot(the, list);
			to = to->next = fxNewSlot(the);
			mxPullSlot(to);
			to->flag = XS_INTERNAL_FLAG;
			from = from->next;
			break;
		case XS_WEAK_MAP_KIND:
			fx_structuredCloneThrow(the, list, "weak map");
			break;
		case XS_WEAK_REF_KIND:
			fx_structuredCloneThrow(the, list, "weak ref");
			break;
		case XS_WEAK_SET_KIND:
			fx_structuredCloneThrow(the, list, "weak set");
			break;
		}
	}
	link.previous = list->last;
	list->last->next = &link;
	list->last = &link;

	toArray = C_NULL;
	while (from) {
		if (from->flag & XS_INTERNAL_FLAG) {
			if (from->kind == XS_ARRAY_KIND) {
				toArray = to = to->next = fxDuplicateSlot(the, from);
				if (from->value.array.address) {
					size = (((txChunk*)(((txByte*)from->value.array.address) - sizeof(txChunk)))->size) / sizeof(txSlot);
					to->value.array.address = C_NULL;
					to->value.array.address = fxNewChunk(the, size * sizeof(txSlot));
					c_memcpy(toArray->value.array.address, from->value.array.address, size * sizeof(txSlot));
				}
			}
			// skip weak entries and private properties
		}
		else
			break;
		from = from->next;
	}	
	toBase = to;
	while (from) {
		if (!(from->flag & XS_DONT_ENUM_FLAG) && fxIsKeyName(the, from->ID)) {
			to = to->next = fxDuplicateSlot(the, from);
			to->flag = XS_NO_FLAG;
		}
		from = from->next;
	}	
	
	link.ID = XS_NO_ID;
	if (toArray && toArray->value.array.address) {
		txSize fromOffset = 0, toOffset = 0;
		while (fromOffset < size) {
			txSlot* fromItem = toArray->value.array.address + fromOffset;
			txSlot* toItem;
			txIndex index = *((txIndex*)fromItem);
			if (!(fromItem->flag & XS_DONT_ENUM_FLAG)) {
				if (fromItem->kind == XS_ACCESSOR_KIND) {
					mxPushSlot(instance);
					mxGetIndex(index);
				}
				else
					mxPushSlot(fromItem);
				link.index = index;
				fx_structuredCloneSlot(the, list);
				toItem = toArray->value.array.address + toOffset;
				mxPullSlot(toItem);
				*((txIndex*)toItem) = index;
				toOffset++;
			}
			fromOffset++;
		}
		if (toOffset < size) {
			c_memset(toArray->value.array.address + toOffset, 0, (size - toOffset) * sizeof(txSlot));
		}
	}
	link.index = 0;
	to = toBase->next;
	while (to) {
		if (to->kind == XS_ACCESSOR_KIND) {
			mxPushReference(instance);
			mxGetID(to->ID);
		}
		else
			mxPushSlot(to);
		link.ID = to->ID;
		fx_structuredCloneSlot(the, list);
		mxPullSlot(to);
		to = to->next;
	}
	list->last = link.previous;
}

txSlot* fx_structuredCloneProperty(txMachine* the, tx_structuredCloneList* list, txSlot* from, txSlot* to, txID id) 
{
	while (from) {
		if (from->ID == id)
			break;
		from = from->next;
	}
	if (from && (from->kind != XS_ACCESSOR_KIND)) {
		mxPushSlot(from);
		list->last->ID = from->ID;
		fx_structuredCloneSlot(the, list);
		to = to->next = fxNewSlot(the);
		mxPullSlot(to);
		to->ID = id;
		to->flag |= XS_DONT_ENUM_FLAG;
	}
	return to;
}

void fx_structuredCloneSlot(txMachine* the, tx_structuredCloneList* list) 
{
	txSlot* slot = the->stack;
	switch (slot->kind) {
	case XS_UNDEFINED_KIND:
	case XS_NULL_KIND:
	case XS_BOOLEAN_KIND:
	case XS_INTEGER_KIND:
	case XS_NUMBER_KIND:
	case XS_STRING_KIND:
	case XS_STRING_X_KIND:
	case XS_BIGINT_KIND:
	case XS_BIGINT_X_KIND:
		mxPushSlot(slot);
		break;
	case XS_SYMBOL_KIND:
		fx_structuredCloneThrow(the, list, "symbol");
		break;
	case XS_REFERENCE_KIND:
		fx_structuredCloneInstance(the, list);
		break;
#ifdef mxHostFunctionPrimitive
	case XS_HOST_FUNCTION_KIND:
		fx_structuredCloneThrow(the, list, "function");
		break;
#endif
	default:
		fx_structuredCloneThrow(the, list, "slot");
		break;
	}
	mxPullSlot(slot);
}

void fx_structuredCloneThrow(txMachine* the, tx_structuredCloneList* list, txString message) 
{
	char buffer[128] = "";
	txInteger i = 0;
	txInteger c = sizeof(buffer);
	tx_structuredCloneLink* link = list->first;
	while (link) {
		if (link->ID) {
			txSlot* key = fxGetKey(the, link->ID);
			if (key) {
				txKind kind = mxGetKeySlotKind(key);
				if ((kind == XS_KEY_KIND) || (kind == XS_KEY_X_KIND)) {
					if (i < c) i += c_snprintf(buffer + i, c - i, "%s%s", (link == list->first) ? "" : ".", key->value.string);
				}
			}
		}
		else {
			if (i < c) i += c_snprintf(buffer + i, c - i, "[%d]", (int)link->index);
		}
		link = link->next;
	}
	if (i < c)
		i += c_snprintf(buffer + i, c - i, ": %s", message);
	mxTypeError(buffer);
}

txBoolean fx_structuredCloneTransferable(txMachine* the, txSlot* instance) 
{
	txSlot* transferList = mxVarv(3);
	txSlot* transferLink = transferList->value.list.first;
	while (transferLink) {	
		if (transferLink->value.reference == instance)
			return 1;
		transferLink = transferLink->next;
	}
	return 0;
}

