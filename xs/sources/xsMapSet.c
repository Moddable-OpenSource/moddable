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

#ifndef mxMapSetLength
	#define mxMapSetLength (127)
#endif

static txSlot* fxCheckMapInstance(txMachine* the, txSlot* slot, txBoolean mutable);
static txSlot* fxCheckMapKey(txMachine* the);

static txSlot* fxCheckSetInstance(txMachine* the, txSlot* slot, txBoolean mutable);
static txSlot* fxCheckSetValue(txMachine* the);

static txSlot* fxCheckWeakMapInstance(txMachine* the, txSlot* slot, txBoolean mutable);
static txSlot* fxCheckWeakMapKey(txMachine* the);

static txSlot* fxCheckWeakSetInstance(txMachine* the, txSlot* slot, txBoolean mutable);
static txSlot* fxCheckWeakSetValue(txMachine* the);

static void fxClearEntries(txMachine* the, txSlot* table, txSlot* list, txBoolean paired);
static txInteger fxCountEntries(txMachine* the, txSlot* list, txBoolean paired);
static txBoolean fxDeleteEntry(txMachine* the, txSlot* table, txSlot* list, txSlot* slot, txBoolean paired); 
static txSlot* fxGetEntry(txMachine* the, txSlot* table, txSlot* slot);
static txSlot* fxNewEntryIteratorInstance(txMachine* the, txSlot* iterable, txID id);
//static void fxPurgeEntries(txMachine* the, txSlot* list);
static void fxSetEntry(txMachine* the, txSlot* table, txSlot* list, txSlot* slot, txSlot* pair); 
static txBoolean fxTestEntry(txMachine* the, txSlot* a, txSlot* b);

static void fxKeepDuringJobs(txMachine* the, txSlot* target);
static txSlot* fxNewWeakRefInstance(txMachine* the);

static void fx_FinalizationRegistryCleanup(txMachine* the, txSlot* registry, txSlot* callback);

void fxBuildMapSet(txMachine* the)
{
	txSlot* slot;
	txSlot* property;
	
	/* MAP */
	mxPush(mxObjectPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostAccessorProperty(the, slot, mxCallback(fx_Map_prototype_size), C_NULL, mxID(_size), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Map_prototype_clear), 0, mxID(_clear), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Map_prototype_delete), 1, mxID(_delete), XS_DONT_ENUM_FLAG);
	property = slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Map_prototype_entries), 0, mxID(_entries), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Map_prototype_forEach), 1, mxID(_forEach), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Map_prototype_get), 1, mxID(_get), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Map_prototype_has), 1, mxID(_has), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Map_prototype_keys), 0, mxID(_keys), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Map_prototype_set), 2, mxID(_set), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Map_prototype_values), 0, mxID(_values), XS_DONT_ENUM_FLAG);
	slot = fxNextSlotProperty(the, slot, property, mxID(_Symbol_iterator), XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "Map", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxMapPrototype = *the->stack;
	slot = fxBuildHostConstructor(the, mxCallback(fx_Map), 0, mxID(_Map));
	mxMapConstructor = *the->stack;
	slot = fxLastProperty(the, slot);
	slot = fxNextHostAccessorProperty(the, slot, mxCallback(fx_species_get), C_NULL, mxID(_Symbol_species), XS_DONT_ENUM_FLAG);
	the->stack++;
	
	mxPush(mxIteratorPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Map_prototype_entries_next), 0, mxID(_next), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "Map Iterator", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxPull(mxMapEntriesIteratorPrototype);
	mxPush(mxIteratorPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Map_prototype_keys_next), 0, mxID(_next), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "Map Iterator", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxPull(mxMapKeysIteratorPrototype);
	mxPush(mxIteratorPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Map_prototype_values_next), 0, mxID(_next), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "Map Iterator", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxPull(mxMapValuesIteratorPrototype);
	
	/* SET */
	mxPush(mxObjectPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostAccessorProperty(the, slot, mxCallback(fx_Set_prototype_size), C_NULL, mxID(_size), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Set_prototype_add), 1, mxID(_add), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Set_prototype_clear), 0, mxID(_clear), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Set_prototype_delete), 1, mxID(_delete), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Set_prototype_entries), 0, mxID(_entries), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Set_prototype_forEach), 1, mxID(_forEach), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Set_prototype_has), 1, mxID(_has), XS_DONT_ENUM_FLAG);
	property = slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Set_prototype_values), 0, mxID(_values), XS_DONT_ENUM_FLAG);
	slot = fxNextSlotProperty(the, slot, property, mxID(_keys), XS_DONT_ENUM_FLAG);
	slot = fxNextSlotProperty(the, slot, property, mxID(_Symbol_iterator), XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "Set", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxSetPrototype = *the->stack;
	slot = fxBuildHostConstructor(the, mxCallback(fx_Set), 0, mxID(_Set));
	mxSetConstructor = *the->stack;
	slot = fxLastProperty(the, slot);
	slot = fxNextHostAccessorProperty(the, slot, mxCallback(fx_species_get), C_NULL, mxID(_Symbol_species), XS_DONT_ENUM_FLAG);
	the->stack++;
	
	mxPush(mxIteratorPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Set_prototype_entries_next), 0, mxID(_next), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "Set Iterator", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxPull(mxSetEntriesIteratorPrototype);
	mxPush(mxIteratorPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Set_prototype_values_next), 0, mxID(_next), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "Set Iterator", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxPull(mxSetKeysIteratorPrototype);
	mxPush(mxIteratorPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Set_prototype_values_next), 0, mxID(_next), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "Set Iterator", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxPull(mxSetValuesIteratorPrototype);

	/* WEAK MAP */
	mxPush(mxObjectPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_WeakMap_prototype_delete), 1, mxID(_delete), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_WeakMap_prototype_get), 1, mxID(_get), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_WeakMap_prototype_has), 1, mxID(_has), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_WeakMap_prototype_set), 2, mxID(_set), XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "WeakMap", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxWeakMapPrototype = *the->stack;
	slot = fxBuildHostConstructor(the, mxCallback(fx_WeakMap), 0, mxID(_WeakMap));
	mxWeakMapConstructor = *the->stack;
	the->stack++;
	
	/* WEAK SET */
	mxPush(mxObjectPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_WeakSet_prototype_add), 1, mxID(_add), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_WeakSet_prototype_delete), 1, mxID(_delete), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_WeakSet_prototype_has), 1, mxID(_has), XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "WeakSet", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxWeakSetPrototype = *the->stack;
	slot = fxBuildHostConstructor(the, mxCallback(fx_WeakSet), 0, mxID(_WeakSet));
	mxWeakSetConstructor = *the->stack;
	the->stack++;
	
	/* WEAK REF */
	mxPush(mxObjectPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_WeakRef_prototype_deref), 0, mxID(_deref), XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "WeakRef", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxWeakRefPrototype = *the->stack;
	slot = fxBuildHostConstructor(the, mxCallback(fx_WeakRef), 1, mxID(_WeakRef));
	mxWeakRefConstructor = *the->stack;
	the->stack++;
	
	/* FINALIZATION REGISTRY */
	mxPush(mxObjectPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_FinalizationRegistry_prototype_cleanupSome), 0, mxID(_cleanupSome), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_FinalizationRegistry_prototype_register), 2, mxID(_register), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_FinalizationRegistry_prototype_unregister), 1, mxID(_unregister), XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "FinalizationRegistry", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxFinalizationRegistryPrototype = *the->stack;
	slot = fxBuildHostConstructor(the, mxCallback(fx_FinalizationRegistry), 1, mxID(_FinalizationRegistry));
	mxFinalizationRegistryConstructor = *the->stack;
	the->stack++;
}

txSlot* fxCheckMapInstance(txMachine* the, txSlot* slot, txBoolean mutable)
{
	if (slot->kind == XS_REFERENCE_KIND) {
		txSlot* instance = slot->value.reference;
		if (((slot = instance->next)) && (slot->flag & XS_INTERNAL_FLAG) && (slot->kind == XS_MAP_KIND) && (instance != mxMapPrototype.value.reference)) {
			if (mutable && (slot->flag & XS_MARK_FLAG))
				mxTypeError("Map instance is read-only");
			return instance;
		}
	}
	mxTypeError("this is no Map instance");
	return C_NULL;
}

txSlot* fxCheckMapKey(txMachine* the)
{
	if (mxArgc > 0) {
		txSlot* slot = mxArgv(0);
		return slot;
	}
	mxTypeError("no key");
	return C_NULL;
}

txSlot* fxNewMapInstance(txMachine* the)
{
	txSlot* map;
	txSlot* table;
	txSlot* list;
	txSlot** address;
	map = fxNewSlot(the);
	map->kind = XS_INSTANCE_KIND;
	map->value.instance.garbage = C_NULL;
	map->value.instance.prototype = the->stack->value.reference;
	the->stack->kind = XS_REFERENCE_KIND;
	the->stack->value.reference = map;
	table = map->next = fxNewSlot(the);
	list = table->next = fxNewSlot(the);
	address = (txSlot**)fxNewChunk(the, mxMapSetLength * sizeof(txSlot*));
	c_memset(address, 0, mxMapSetLength * sizeof(txSlot*));
	/* TABLE */
	table->flag = XS_INTERNAL_FLAG | XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	table->kind = XS_MAP_KIND;
	table->value.table.address = address;
	table->value.table.length = mxMapSetLength;
	/* LIST */
	list->flag = XS_INTERNAL_FLAG | XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	list->kind = XS_LIST_KIND;
	list->value.list.first = C_NULL;
	list->value.list.last = C_NULL;
 	return map;
}

void fx_Map(txMachine* the)
{
	txSlot *function, *iterable, *iterator, *next, *value;
	if (mxIsUndefined(mxTarget))
		mxTypeError("call: Map");
	mxPushSlot(mxTarget);
	fxGetPrototypeFromConstructor(the, &mxMapPrototype);
	fxNewMapInstance(the);
	mxPullSlot(mxResult);
	if (mxArgc < 1)
		return;
	iterable = mxArgv(0);
	if ((iterable->kind == XS_UNDEFINED_KIND) || (iterable->kind == XS_NULL_KIND))
		return;
	mxGetID(mxResult, mxID(_set));	
	function = the->stack;	
	if (!fxIsCallable(the, function))	
		mxTypeError("set is no function");
	mxTemporary(iterator);
	mxTemporary(next);
	fxGetIterator(the, iterable, iterator, next, 0);	
	mxTemporary(value);
	while (fxIteratorNext(the, iterator, next, value)) {
		mxTry(the) {
			if (value->kind != XS_REFERENCE_KIND)
				mxTypeError("item is no object");
			mxPushSlot(mxResult);
			mxPushSlot(function);
			mxCall();
			mxPushSlot(value);
			fxGetIndex(the, 0);
			mxPushSlot(value);
			fxGetIndex(the, 1);
			mxRunCount(2);
			mxPop();
		}
		mxCatch(the) {
			fxIteratorReturn(the, iterator);
			fxJump(the);
		}
	}
}

void fx_Map_prototype_clear(txMachine* the)
{
	txSlot* instance = fxCheckMapInstance(the, mxThis, XS_MUTABLE);
	txSlot* table = instance->next;
	txSlot* list = table->next;
	fxClearEntries(the, table, list, 1);
}

void fx_Map_prototype_delete(txMachine* the)
{
	txSlot* instance = fxCheckMapInstance(the, mxThis, XS_MUTABLE);
	txSlot* table = instance->next;
	txSlot* list = table->next;
	txSlot* key = fxCheckMapKey(the);
	mxResult->value.boolean = fxDeleteEntry(the, table, list, key, 1);
	mxResult->kind = XS_BOOLEAN_KIND;
}

void fx_Map_prototype_entries(txMachine* the)
{
	fxCheckMapInstance(the, mxThis, XS_IMMUTABLE);
	mxPush(mxMapEntriesIteratorPrototype);
	fxNewEntryIteratorInstance(the, mxThis, mxID(_Map));
	mxPullSlot(mxResult);
}

void fx_Map_prototype_entries_next(txMachine* the)
{
	txSlot* iterator = fxCheckIteratorInstance(the, mxThis, mxID(_Map));
	txSlot* result = iterator->next;
	txSlot* iterable = result->next;
	txSlot* index = iterable->next;
	txSlot* key = index->value.closure;
	txSlot* value;
	while (key && (key->flag & XS_DONT_ENUM_FLAG)) {
		key = key->next->next;
	}
	mxResult->kind = result->kind;
	mxResult->value = result->value;
	result = result->value.reference->next;
	if (key) {
		value = key->next;
		mxPushSlot(key);
		mxPushSlot(value);
		fxConstructArrayEntry(the, result);
		index->value.closure = value->next;
	}
	else {
		result->kind = XS_UNDEFINED_KIND;
		result->next->value.boolean = 1;
		index->value.closure = C_NULL;
	}
}

void fx_Map_prototype_forEach(txMachine* the)
{
	txSlot* instance = fxCheckMapInstance(the, mxThis, XS_IMMUTABLE);
	txSlot* table = instance->next;
	txSlot* list = table->next;
	txSlot* function = fxArgToCallback(the, 0);
	txSlot* key = list->value.list.first;
	while (key) {
		txSlot* value = key->next;
		if (!(key->flag & XS_DONT_ENUM_FLAG)) {
			/* THIS */
			if (mxArgc > 1)
				mxPushSlot(mxArgv(1));
			else
				mxPushUndefined();
			/* FUNCTION */
			mxPushSlot(function);
			mxCall();
			/* ARGUMENTS */
			mxPushSlot(value);
			mxPushSlot(key);
			mxPushSlot(mxThis);
			mxRunCount(3);
			the->stack++;
		}
		key = value->next;
	}
}

void fx_Map_prototype_get(txMachine* the)
{
	txSlot* instance = fxCheckMapInstance(the, mxThis, XS_IMMUTABLE);
	txSlot* table = instance->next;
	txSlot* key = fxCheckMapKey(the);
	txSlot* result = fxGetEntry(the, table, key);
	if (result) {
		txSlot* value = result->next;
		mxResult->kind = value->kind;
		mxResult->value = value->value;
	}
}

void fx_Map_prototype_has(txMachine* the)
{
	txSlot* instance = fxCheckMapInstance(the, mxThis, XS_IMMUTABLE);
	txSlot* table = instance->next;
	txSlot* key = fxCheckMapKey(the);
	txSlot* result = fxGetEntry(the, table, key);
	mxResult->kind = XS_BOOLEAN_KIND;
	mxResult->value.boolean = (result) ? 1 : 0;
}

void fx_Map_prototype_keys(txMachine* the)
{
	fxCheckMapInstance(the, mxThis, XS_IMMUTABLE);
	mxPush(mxMapKeysIteratorPrototype);
	fxNewEntryIteratorInstance(the, mxThis, mxID(_Map));
	mxPullSlot(mxResult);
}

void fx_Map_prototype_keys_next(txMachine* the)
{
	txSlot* iterator = fxCheckIteratorInstance(the, mxThis, mxID(_Map));
	txSlot* result = iterator->next;
	txSlot* iterable = result->next;
	txSlot* index = iterable->next;
	txSlot* key = index->value.closure;
	txSlot* value;
	while (key && (key->flag & XS_DONT_ENUM_FLAG)) {
		key = key->next->next;
	}
	mxResult->kind = result->kind;
	mxResult->value = result->value;
	result = result->value.reference->next;
	if (key) {
		value = key->next;
		result->kind = key->kind;
		result->value = key->value;
		index->value.closure = value->next;
	}
	else {
		result->kind = XS_UNDEFINED_KIND;
		result->next->value.boolean = 1;
		index->value.closure = C_NULL;
	}
}

void fx_Map_prototype_set(txMachine* the)
{
	txSlot* instance = fxCheckMapInstance(the, mxThis, XS_MUTABLE);
	txSlot* table = instance->next;
	txSlot* list = table->next;
	txSlot* key = fxCheckMapKey(the);
	fxSetEntry(the, table, list, key, (mxArgc > 1) ? mxArgv(1) : &mxUndefined);
	*mxResult = *mxThis;
}

void fx_Map_prototype_size(txMachine* the)
{
	txSlot* instance = fxCheckMapInstance(the, mxThis, XS_IMMUTABLE);
	txSlot* table = instance->next;
	txSlot* list = table->next;
	mxResult->kind = XS_INTEGER_KIND;
	mxResult->value.integer = fxCountEntries(the, list, 1);
}

void fx_Map_prototype_values(txMachine* the)
{
	fxCheckMapInstance(the, mxThis, XS_IMMUTABLE);
	mxPush(mxMapValuesIteratorPrototype);
	fxNewEntryIteratorInstance(the, mxThis, mxID(_Map));
	mxPullSlot(mxResult);
}

void fx_Map_prototype_values_next(txMachine* the)
{
	txSlot* iterator = fxCheckIteratorInstance(the, mxThis, mxID(_Map));
	txSlot* result = iterator->next;
	txSlot* iterable = result->next;
	txSlot* index = iterable->next;
	txSlot* key = index->value.closure;
	txSlot* value;
	while (key && (key->flag & XS_DONT_ENUM_FLAG)) {
		key = key->next->next;
	}
	mxResult->kind = result->kind;
	mxResult->value = result->value;
	result = result->value.reference->next;
	if (key) {
		value = key->next;
		result->kind = value->kind;
		result->value = value->value;
		index->value.closure = value->next;
	}
	else {
		result->kind = XS_UNDEFINED_KIND;
		result->next->value.boolean = 1;
		index->value.closure = C_NULL;
	}
}

txSlot* fxCheckSetInstance(txMachine* the, txSlot* slot, txBoolean mutable)
{
	if (slot->kind == XS_REFERENCE_KIND) {
		txSlot* instance = slot->value.reference;
		if (((slot = instance->next)) && (slot->flag & XS_INTERNAL_FLAG) && (slot->kind == XS_SET_KIND) && (instance != mxSetPrototype.value.reference)) {
			if (mutable && (slot->flag & XS_MARK_FLAG))
				mxTypeError("Set instance is read-only");
			return instance;
		}
	}
	mxTypeError("this is no Set instance");
	return C_NULL;
}

txSlot* fxCheckSetValue(txMachine* the)
{
	if (mxArgc > 0) {
		txSlot* slot = mxArgv(0);
		return slot;
	}
	mxTypeError("no value");
	return C_NULL;
}

txSlot* fxNewSetInstance(txMachine* the)
{
	txSlot* set;
	txSlot* table;
	txSlot* list;
	txSlot** address;
	set = fxNewSlot(the);
	set->kind = XS_INSTANCE_KIND;
	set->value.instance.garbage = C_NULL;
	set->value.instance.prototype = the->stack->value.reference;
	the->stack->kind = XS_REFERENCE_KIND;
	the->stack->value.reference = set;
	table = set->next = fxNewSlot(the);
	list = table->next = fxNewSlot(the);
	address = (txSlot**)fxNewChunk(the, mxMapSetLength * sizeof(txSlot*));
	c_memset(address, 0, mxMapSetLength * sizeof(txSlot*));
	/* TABLE */
	table->flag = XS_INTERNAL_FLAG | XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	table->kind = XS_SET_KIND;
	table->value.table.address = address;
	table->value.table.length = mxMapSetLength;
	/* LIST */
	list->flag = XS_INTERNAL_FLAG | XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	list->kind = XS_LIST_KIND;
	list->value.list.first = C_NULL;
	list->value.list.last = C_NULL;
 	return set;
}

void fx_Set(txMachine* the)
{
	txSlot *function, *iterable, *iterator, *next, *value;
	if (mxIsUndefined(mxTarget))
		mxTypeError("call: Set");
	mxPushSlot(mxTarget);
	fxGetPrototypeFromConstructor(the, &mxSetPrototype);
	fxNewSetInstance(the);
	mxPullSlot(mxResult);
	if (mxArgc < 1)
		return;
	iterable = mxArgv(0);
	if ((iterable->kind == XS_UNDEFINED_KIND) || (iterable->kind == XS_NULL_KIND))
		return;
	mxGetID(mxResult, mxID(_add));	
	function = the->stack;	
	if (!fxIsCallable(the, function))	
		mxTypeError("add is no function");
	mxTemporary(iterator);
	mxTemporary(next);
	fxGetIterator(the, iterable, iterator, next, 0);	
	mxTemporary(value);
	while (fxIteratorNext(the, iterator, next, value)) {
		mxTry(the) {
			mxPushSlot(mxResult);
			mxPushSlot(function);
			mxCall();
			mxPushSlot(value);
			mxRunCount(1);
			mxPop();
		}
		mxCatch(the) {
			fxIteratorReturn(the, iterator);
			fxJump(the);
		}
	}
}

void fx_Set_prototype_add(txMachine* the)
{
	txSlot* instance = fxCheckSetInstance(the, mxThis, XS_MUTABLE);
	txSlot* table = instance->next;
	txSlot* list = table->next;
	txSlot* value = fxCheckSetValue(the);
	fxSetEntry(the, table, list, value, C_NULL);
	*mxResult = *mxThis;
}

void fx_Set_prototype_clear(txMachine* the)
{
	txSlot* instance = fxCheckSetInstance(the, mxThis, XS_MUTABLE);
	txSlot* table = instance->next;
	txSlot* list = table->next;
	fxClearEntries(the, table, list, 0);
}

void fx_Set_prototype_delete(txMachine* the)
{
	txSlot* instance = fxCheckSetInstance(the, mxThis, XS_MUTABLE);
	txSlot* table = instance->next;
	txSlot* list = table->next;
	txSlot* value = fxCheckSetValue(the);
	mxResult->value.boolean = fxDeleteEntry(the, table, list, value, 0);
	mxResult->kind = XS_BOOLEAN_KIND;
}

void fx_Set_prototype_entries(txMachine* the)
{
	fxCheckSetInstance(the, mxThis, XS_IMMUTABLE);
	mxPush(mxSetEntriesIteratorPrototype);
	fxNewEntryIteratorInstance(the, mxThis, mxID(_Set));
	mxPullSlot(mxResult);
}

void fx_Set_prototype_entries_next(txMachine* the)
{
	txSlot* iterator = fxCheckIteratorInstance(the, mxThis, mxID(_Set));
	txSlot* result = iterator->next;
	txSlot* iterable = result->next;
	txSlot* index = iterable->next;
	txSlot* value = index->value.closure;
	while (value && (value->flag & XS_DONT_ENUM_FLAG))
		value = value->next;
	mxResult->kind = result->kind;
	mxResult->value = result->value;
	result = result->value.reference->next;
	if (value) {
		mxPushSlot(value);
		mxPushSlot(value);
		fxConstructArrayEntry(the, result);
		index->value.closure = value->next;
	}
	else {
		result->kind = XS_UNDEFINED_KIND;
		result->next->value.boolean = 1;
		index->value.closure = C_NULL;
	}
}

void fx_Set_prototype_forEach(txMachine* the)
{
	txSlot* instance = fxCheckSetInstance(the, mxThis, XS_IMMUTABLE);
	txSlot* table = instance->next;
	txSlot* list = table->next;
	txSlot* function = fxArgToCallback(the, 0);
	txSlot* value = list->value.list.first;
	while (value) {
		if (!(value->flag & XS_DONT_ENUM_FLAG)) {
			/* THIS */
			if (mxArgc > 1)
				mxPushSlot(mxArgv(1));
			else
				mxPushUndefined();
			/* FUNCTION */
			mxPushSlot(function);
			mxCall();
			/* ARGUMENTS */
			mxPushSlot(value);
			mxPushSlot(value);
			mxPushSlot(mxThis);
			mxRunCount(3);
			mxPop();
		}
		value = value->next;
	}
}

void fx_Set_prototype_has(txMachine* the)
{
	txSlot* instance = fxCheckSetInstance(the, mxThis, XS_IMMUTABLE);
	txSlot* table = instance->next;
	txSlot* value = fxCheckSetValue(the);
	txSlot* result = fxGetEntry(the, table, value);
	mxResult->kind = XS_BOOLEAN_KIND;
	mxResult->value.boolean = (result) ? 1 : 0;
}

void fx_Set_prototype_size(txMachine* the)
{
	txSlot* instance = fxCheckSetInstance(the, mxThis, XS_IMMUTABLE);
	txSlot* table = instance->next;
	txSlot* list = table->next;
	mxResult->kind = XS_INTEGER_KIND;
	mxResult->value.integer = fxCountEntries(the, list, 0);
}

void fx_Set_prototype_values(txMachine* the)
{
	fxCheckSetInstance(the, mxThis, XS_IMMUTABLE);
	mxPush(mxSetValuesIteratorPrototype);
	fxNewEntryIteratorInstance(the, mxThis, mxID(_Set));
	mxPullSlot(mxResult);
}

void fx_Set_prototype_values_next(txMachine* the)
{
	txSlot* iterator = fxCheckIteratorInstance(the, mxThis, mxID(_Set));
	txSlot* result = iterator->next;
	txSlot* iterable = result->next;
	txSlot* index = iterable->next;
	txSlot* value = index->value.closure;
	while (value && (value->flag & XS_DONT_ENUM_FLAG))
		value = value->next;
	mxResult->kind = result->kind;
	mxResult->value = result->value;
	result = result->value.reference->next;
	if (value) {
		result->kind = value->kind;
		result->value = value->value;
		index->value.closure = value->next;
	}
	else {
		result->kind = XS_UNDEFINED_KIND;
		result->next->value.boolean = 1;
		index->value.closure = C_NULL;
	}
}

txSlot* fxCheckWeakMapInstance(txMachine* the, txSlot* slot, txBoolean mutable)
{
	if (slot->kind == XS_REFERENCE_KIND) {
		txSlot* instance = slot->value.reference;
		if (((slot = instance->next)) && (slot->flag & XS_INTERNAL_FLAG) && (slot->kind == XS_WEAK_MAP_KIND) && (instance != mxWeakMapPrototype.value.reference)) {
			if (mutable && (slot->flag & XS_MARK_FLAG))
				mxTypeError("WeakMap instance is read-only");
			return instance;
		}
	}
	mxTypeError("this is no WeakMap instance");
	return C_NULL;
}

txSlot* fxCheckWeakMapKey(txMachine* the)
{
	if (mxArgc > 0) {
		txSlot* slot = mxArgv(0);
		if (slot->kind == XS_REFERENCE_KIND)
			return slot;
	}
	return C_NULL;
}

txSlot* fxNewWeakMapInstance(txMachine* the)
{
	txSlot* map;
	txSlot* table;
	txSlot** address;
	map = fxNewSlot(the);
	map->kind = XS_INSTANCE_KIND;
	map->value.instance.garbage = C_NULL;
	map->value.instance.prototype = the->stack->value.reference;
	the->stack->kind = XS_REFERENCE_KIND;
	the->stack->value.reference = map;
	table = map->next = fxNewSlot(the);
	address = (txSlot**)fxNewChunk(the, (mxMapSetLength + 1) * sizeof(txSlot*)); // one more slot for the collector weak table list
	c_memset(address, 0, (mxMapSetLength + 1) * sizeof(txSlot*));
	/* TABLE */
	table->flag = XS_INTERNAL_FLAG | XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	table->kind = XS_WEAK_MAP_KIND;
	table->value.table.address = address;
	table->value.table.length = mxMapSetLength;
 	return map;
}

void fx_WeakMap(txMachine* the)
{
	txSlot *function, *iterable, *iterator, *next, *value;
	if (mxIsUndefined(mxTarget))
		mxTypeError("call: WeakMap");
	mxPushSlot(mxTarget);
	fxGetPrototypeFromConstructor(the, &mxWeakMapPrototype);
	fxNewWeakMapInstance(the);
	mxPullSlot(mxResult);
	if (mxArgc < 1)
		return;
	iterable = mxArgv(0);
	if ((iterable->kind == XS_UNDEFINED_KIND) || (iterable->kind == XS_NULL_KIND))
		return;
	mxGetID(mxResult, mxID(_set));	
	function = the->stack;	
	if (!fxIsCallable(the, function))	
		mxTypeError("set is no function");
	mxTemporary(iterator);
	mxTemporary(next);
	fxGetIterator(the, iterable, iterator, next, 0);	
	mxTemporary(value);
	while (fxIteratorNext(the, iterator, next, value)) {
		mxTry(the) {
			mxPushSlot(mxResult);
			mxPushSlot(function);
			mxCall();
			if (value->kind != XS_REFERENCE_KIND)
				mxTypeError("item is no object");
			mxPushSlot(value);
			fxGetIndex(the, 0);
			mxPushSlot(value);
			fxGetIndex(the, 1);
			mxRunCount(2);
			mxPop();
		}
		mxCatch(the) {
			fxIteratorReturn(the, iterator);
			fxJump(the);
		}
	}
}

void fx_WeakMap_prototype_delete(txMachine* the)
{
	txSlot* instance = fxCheckWeakMapInstance(the, mxThis, XS_MUTABLE);
	txSlot* table = instance->next;
	txSlot* key = fxCheckWeakMapKey(the);
	mxResult->value.boolean = (key) ? fxDeleteEntry(the, table, C_NULL, key, 1) : 0;
	mxResult->kind = XS_BOOLEAN_KIND;
}

void fx_WeakMap_prototype_get(txMachine* the)
{
	txSlot* instance = fxCheckWeakMapInstance(the, mxThis, XS_IMMUTABLE);
	txSlot* table = instance->next;
	txSlot* key = fxCheckWeakMapKey(the);
	txSlot* result = (key) ? fxGetEntry(the, table, key) : C_NULL;
	if (result) {
		txSlot* value = result->next;
		mxResult->kind = value->kind;
		mxResult->value = value->value;
	}
}

void fx_WeakMap_prototype_has(txMachine* the)
{
	txSlot* instance = fxCheckWeakMapInstance(the, mxThis, XS_IMMUTABLE);
	txSlot* table = instance->next;
	txSlot* key = fxCheckWeakMapKey(the);
	txSlot* result = (key) ? fxGetEntry(the, table, key) : C_NULL;
	mxResult->kind = XS_BOOLEAN_KIND;
	mxResult->value.boolean = (result) ? 1 : 0;
}

void fx_WeakMap_prototype_set(txMachine* the)
{
	txSlot* instance = fxCheckWeakMapInstance(the, mxThis, XS_MUTABLE);
	txSlot* table = instance->next;
	txSlot* key = fxCheckWeakMapKey(the);
	if (!key)
		mxTypeError("key is no object");
	fxSetEntry(the, table, C_NULL, key, (mxArgc > 1) ? mxArgv(1) : &mxUndefined);
	*mxResult = *mxThis;
}

txSlot* fxCheckWeakSetInstance(txMachine* the, txSlot* slot, txBoolean mutable)
{
	if (slot->kind == XS_REFERENCE_KIND) {
		txSlot* instance = slot->value.reference;
		if (((slot = instance->next)) && (slot->flag & XS_INTERNAL_FLAG) && (slot->kind == XS_WEAK_SET_KIND) && (instance != mxWeakSetPrototype.value.reference)) {
			if (mutable && (slot->flag & XS_MARK_FLAG))
				mxTypeError("WeakSet instance is read-only");
			return instance;
		}
	}
	mxTypeError("this is no WeakSet instance");
	return C_NULL;
}

txSlot* fxCheckWeakSetValue(txMachine* the)
{
	if (mxArgc > 0) {
		txSlot* slot = mxArgv(0);
		if (slot->kind == XS_REFERENCE_KIND)
			return slot;
	}
	return C_NULL;
}

txSlot* fxNewWeakSetInstance(txMachine* the)
{
	txSlot* set;
	txSlot* table;
	txSlot** address;
	set = fxNewSlot(the);
	set->kind = XS_INSTANCE_KIND;
	set->value.instance.garbage = C_NULL;
	set->value.instance.prototype = the->stack->value.reference;
	the->stack->kind = XS_REFERENCE_KIND;
	the->stack->value.reference = set;
	table = set->next = fxNewSlot(the);
	address = (txSlot**)fxNewChunk(the, (mxMapSetLength + 1) * sizeof(txSlot*)); // one more slot for the collector weak table list
	c_memset(address, 0, (mxMapSetLength + 1) * sizeof(txSlot*));
	/* TABLE */
	table->flag = XS_INTERNAL_FLAG | XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	table->kind = XS_WEAK_SET_KIND;
	table->value.table.address = address;
	table->value.table.length = mxMapSetLength;
 	return set;
}

void fx_WeakSet(txMachine* the)
{
	txSlot *function, *iterable, *iterator, *next, *value;
	if (mxIsUndefined(mxTarget))
		mxTypeError("call: WeakSet");
	mxPushSlot(mxTarget);
	fxGetPrototypeFromConstructor(the, &mxWeakSetPrototype);
	fxNewWeakSetInstance(the);
	mxPullSlot(mxResult);
	if (mxArgc < 1)
		return;
	iterable = mxArgv(0);
	if ((iterable->kind == XS_UNDEFINED_KIND) || (iterable->kind == XS_NULL_KIND))
		return;
	mxGetID(mxResult, mxID(_add));	
	function = the->stack;	
	if (!fxIsCallable(the, function))	
		mxTypeError("add is no function");
	mxTemporary(iterator);
	mxTemporary(next);
	fxGetIterator(the, iterable, iterator, next, 0);	
	mxTemporary(value);
	while (fxIteratorNext(the, iterator, next, value)) {
		mxTry(the) {
			mxPushSlot(mxResult);
			mxPushSlot(function);
			mxCall();
			mxPushSlot(value);
			mxRunCount(1);
			mxPop();
		}
		mxCatch(the) {
			fxIteratorReturn(the, iterator);
			fxJump(the);
		}
	}
}

void fx_WeakSet_prototype_add(txMachine* the)
{
	txSlot* instance = fxCheckWeakSetInstance(the, mxThis, XS_MUTABLE);
	txSlot* table = instance->next;
	txSlot* value = fxCheckWeakSetValue(the);
	if (!value)
		mxTypeError("value is no object");
	fxSetEntry(the, table, C_NULL, value, C_NULL);
	*mxResult = *mxThis;
}

void fx_WeakSet_prototype_has(txMachine* the)
{
	txSlot* instance = fxCheckWeakSetInstance(the, mxThis, XS_IMMUTABLE);
	txSlot* table = instance->next;
	txSlot* value = fxCheckWeakSetValue(the);
	txSlot* result = (value) ? fxGetEntry(the, table, value) : C_NULL;
	mxResult->kind = XS_BOOLEAN_KIND;
	mxResult->value.boolean = (result) ? 1 : 0;
}

void fx_WeakSet_prototype_delete(txMachine* the)
{
	txSlot* instance = fxCheckWeakSetInstance(the, mxThis, XS_MUTABLE);
	txSlot* table = instance->next;
	txSlot* value = fxCheckWeakSetValue(the);
	mxResult->value.boolean = (value) ? fxDeleteEntry(the, table, C_NULL, value, 0) : 0;
	mxResult->kind = XS_BOOLEAN_KIND;
}

void fxClearEntries(txMachine* the, txSlot* table, txSlot* list, txBoolean paired)
{
	txSlot* slot = list->value.list.first;
	while (slot) {
		slot->flag = XS_DONT_ENUM_FLAG;
		slot->kind = XS_UNDEFINED_KIND;
		slot = slot->next;
	}
	c_memset(table->value.table.address, 0, table->value.table.length * sizeof(txSlot*));
}

txInteger fxCountEntries(txMachine* the, txSlot* list, txBoolean paired) 
{
	txInteger count = 0;
	txSlot* slot = list->value.list.first;
	while (slot) {
		if (!(slot->flag & XS_DONT_ENUM_FLAG))
			count++;
		slot = slot->next;
	}
	if (paired)
		count >>= 1;
	return count;
}

txBoolean fxDeleteEntry(txMachine* the, txSlot* table, txSlot* list, txSlot* slot, txBoolean paired) 
{
	txU4 sum = fxSumEntry(the, slot);
	txU4 modulo = sum % table->value.table.length;
	txSlot** address = &(table->value.table.address[modulo]);
	txSlot* entry;
	txSlot* result;
	while ((entry = *address)) {
		if (entry->value.entry.sum == sum) {
			result = entry->value.entry.slot;
			if (fxTestEntry(the, result, slot)) {
				if (list) {
					result->flag = XS_DONT_ENUM_FLAG;
					result->kind = XS_UNDEFINED_KIND;
					if (paired) {
						slot = result->next;
						slot->flag = XS_DONT_ENUM_FLAG;
						slot->kind = XS_UNDEFINED_KIND;
					}
				}
				*address = entry->next;
				entry->next = C_NULL;
				return 1;
			}
		}
		address = &entry->next;
	}
	return 0;
}

txSlot* fxGetEntry(txMachine* the, txSlot* table, txSlot* slot) 
{
	txU4 sum = fxSumEntry(the, slot);
	txU4 modulo = sum % table->value.table.length;
	txSlot* entry = table->value.table.address[modulo];
	txSlot* result;
	while (entry) {
		if (entry->value.entry.sum == sum) {
			result = entry->value.entry.slot;
			if (fxTestEntry(the, result, slot))
				return result;
		}
		entry = entry->next;
	}
	return C_NULL;
}

txSlot* fxNewEntryIteratorInstance(txMachine* the, txSlot* iterable, txID id) 
{
	txSlot* instance;
	txSlot* property;
	instance = fxNewIteratorInstance(the, iterable, id);
	property = fxLastProperty(the, instance);
	property->kind = XS_CLOSURE_KIND;
	property->value.closure = iterable->value.reference->next->next->value.list.first;
	return instance;
}

#if 0
void fxPurgeEntries(txMachine* the, txSlot* list) 
{
	txSlot* former = C_NULL;
	txSlot** address = &(list->value.list.first);
	txSlot* slot;
	while ((slot = *address)) {
		if (slot->flag & XS_DONT_ENUM_FLAG) {
			*address = slot->next;
		}
		else {
			former = slot;
			address = &slot->next;
		}
	}
	list->value.list.last = former;
}
#endif

void fxSetEntry(txMachine* the, txSlot* table, txSlot* list, txSlot* slot, txSlot* pair) 
{
	txU4 sum = fxSumEntry(the, slot);
	txU4 modulo = sum % table->value.table.length;
	txSlot** address = &(table->value.table.address[modulo]);
	txSlot* entry;
	txSlot* result;
	while ((entry = *address)) {
		if (entry->value.entry.sum == sum) {
			result = entry->value.entry.slot;
			if (fxTestEntry(the, result, slot)) {
				if (pair) {
					slot = result->next;
					slot->kind = pair->kind;
					slot->value = pair->value;
				}
				return;
			}
		}
		address = &entry->next;
	}
	result = fxNewSlot(the);
	result->kind = slot->kind;
	result->value = slot->value;
	mxPushClosure(result);
	if (pair) {
		result->next = slot = fxNewSlot(the);
		slot->kind = pair->kind;
		slot->value = pair->value;
		mxPushClosure(slot);
	}
	*address = entry = fxNewSlot(the);
	entry->kind = XS_ENTRY_KIND;
	entry->value.entry.slot = result;
	entry->value.entry.sum = sum;
	if (list) {
		if (list->value.list.last)
			list->value.list.last->next = result;
		else
			list->value.list.first = result;
		if (pair)
			list->value.list.last = slot;
		else
			list->value.list.last = result;
	}
	if (pair)
		mxPop();
	mxPop();
}

txU4 fxSumEntry(txMachine* the, txSlot* slot) 
{
	txU1 kind, *address;
	txU4 sum, size;
	
	kind = slot->kind;
	sum = 0;
	if ((XS_STRING_KIND == kind) || (XS_STRING_X_KIND == kind)) {
		address = (txU1*)slot->value.string;
		while ((kind = c_read8(address++)))
			sum = (sum << 1) + kind;
		sum = (sum << 1) + XS_STRING_KIND;
	}
	else {
		if (XS_BOOLEAN_KIND == kind) {
			address = (txU1*)&slot->value.boolean;
			size = sizeof(txBoolean);
		}
		else if (XS_INTEGER_KIND == kind) {
			fxToNumber(the, slot);
			kind = slot->kind;
			address = (txU1*)&slot->value.number;
			size = sizeof(txNumber);
		}
		else if (XS_NUMBER_KIND == kind) {
			if (slot->value.number == 0)
				slot->value.number = 0;
			address = (txU1*)&slot->value.number;
			size = sizeof(txNumber);
		}
		else if (XS_SYMBOL_KIND == kind) {
			address = (txU1*)&slot->value.symbol;
			size = sizeof(txID);
		}
		else if (XS_REFERENCE_KIND == kind) {
			address = (txU1*)&slot->value.reference;
			size = sizeof(txSlot*);
		}
		else {
			address = C_NULL;
			size = 0;
		}
		while (size) {
			sum = (sum << 1) + *address++;
			size--;
		}
		sum = (sum << 1) + kind;
	}
	sum &= 0x7FFFFFFF;
	return sum;
}

txBoolean fxTestEntry(txMachine* the, txSlot* a, txSlot* b)
{	
	txBoolean result = 0;
	if (a->kind == b->kind) {
		if ((XS_UNDEFINED_KIND == a->kind) || (XS_NULL_KIND == a->kind))
			result = 1;
		else if (XS_BOOLEAN_KIND == a->kind)
			result = a->value.boolean == b->value.boolean;
		else if (XS_INTEGER_KIND == a->kind)
			result = a->value.integer == b->value.integer;
        else if (XS_NUMBER_KIND == a->kind)
			result = ((c_isnan(a->value.number) && c_isnan(b->value.number)) || (a->value.number == b->value.number));
		else if ((XS_STRING_KIND == a->kind) || (XS_STRING_X_KIND == a->kind))
			result = c_strcmp(a->value.string, b->value.string) == 0;
		else if (XS_SYMBOL_KIND == a->kind)
			result = a->value.symbol == b->value.symbol;
		else if (XS_REFERENCE_KIND == a->kind)
			result = a->value.reference == b->value.reference;
	}
	else if ((XS_INTEGER_KIND == a->kind) && (XS_NUMBER_KIND == b->kind))
		result = (!c_isnan(b->value.number)) && ((txNumber)(a->value.integer) == b->value.number);
	else if ((XS_NUMBER_KIND == a->kind) && (XS_INTEGER_KIND == b->kind))
		result = (!c_isnan(a->value.number)) && (a->value.number == (txNumber)(b->value.integer));
	else if ((XS_STRING_KIND == a->kind) && (XS_STRING_X_KIND == b->kind))
		result = c_strcmp(a->value.string, b->value.string) == 0;
	else if ((XS_STRING_X_KIND == a->kind) && (XS_STRING_KIND == b->kind))
		result = c_strcmp(a->value.string, b->value.string) == 0;
	return result;
}

void fxKeepDuringJobs(txMachine* the, txSlot* target)
{
	txSlot* instance = mxDuringJobs.value.reference;
	txSlot** address = &(instance->next);
	txSlot* slot;
	while ((slot = *address)) {
		if (slot->value.reference == target)
			return;
		address = &(slot->next);
	}
	*address = slot = fxNewSlot(the);
	slot->value.reference = target;
	slot->kind = XS_REFERENCE_KIND;
}

txSlot* fxCheckWeakRefInstance(txMachine* the, txSlot* slot)
{
	if (slot->kind == XS_REFERENCE_KIND) {
		txSlot* instance = slot->value.reference;
		if (((slot = instance->next)) && (slot->flag & XS_INTERNAL_FLAG) && (slot->kind == XS_WEAK_REF_KIND))
			return instance;
	}
	mxTypeError("this is no WeakRef instance");
	return C_NULL;
}

txSlot* fxNewWeakRefInstance(txMachine* the)
{
	txSlot* slot;
	txSlot* instance = fxNewSlot(the);
	instance->kind = XS_INSTANCE_KIND;
	instance->value.instance.garbage = C_NULL;
	instance->value.instance.prototype = the->stack->value.reference;
	the->stack->kind = XS_REFERENCE_KIND;
	the->stack->value.reference = instance;
	slot = instance->next = fxNewSlot(the);
	slot->flag = XS_INTERNAL_FLAG | XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	slot->kind = XS_WEAK_REF_KIND;
	slot->value.weakRef.target = C_NULL;
	slot->value.weakRef.link = C_NULL;
	return instance;
}

void fx_WeakRef(txMachine* the)
{
	txSlot* target;
	txSlot* instance;
	if (mxIsUndefined(mxTarget))
		mxTypeError("call: WeakRef");
	if (mxArgc < 1)
		mxTypeError("new WeakRef: no target");
	target = mxArgv(0);
	if (!mxIsReference(target))
		mxTypeError("new WeakRef: target is no object");
	target = target->value.reference;
	mxPushSlot(mxTarget);
	fxGetPrototypeFromConstructor(the, &mxWeakRefPrototype);
	instance = fxNewWeakRefInstance(the);
	mxPullSlot(mxResult);
	fxKeepDuringJobs(the, target);
	instance->next->value.weakRef.target = target;
}

void fx_WeakRef_prototype_deref(txMachine* the)
{
	txSlot* instance = fxCheckWeakRefInstance(the, mxThis);
	txSlot* target = instance->next->value.weakRef.target;
	if (target) {
		fxKeepDuringJobs(the, target);
		mxResult->value.reference = target;
		mxResult->kind = XS_REFERENCE_KIND;
	}
}

txSlot* fxCheckFinalizationRegistryInstance(txMachine* the, txSlot* slot)
{
	if (slot->kind == XS_REFERENCE_KIND) {
		txSlot* instance = slot->value.reference;
		if (((slot = instance->next)) && (slot->flag & XS_INTERNAL_FLAG) && (slot->kind == XS_CLOSURE_KIND) && (slot->value.closure->kind == XS_FINALIZATION_REGISTRY_KIND)) {
			if (slot->flag & XS_MARK_FLAG)
				mxTypeError("FinalizationRegistry instance is read-only");
			return instance;
		}
	}
	mxTypeError("this is no FinalizationRegistry instance");
	return C_NULL;
}

void fx_FinalizationRegistry(txMachine* the)
{
	txSlot* callback;
	txSlot* instance;
	txSlot* property;
	txSlot* registry;
	txSlot* slot;
	if (mxIsUndefined(mxTarget))
		mxTypeError("call: FinalizationRegistry");
	if (mxArgc < 1)
		mxTypeError("no callback");
	callback = mxArgv(0);
	if (!fxIsCallable(the, callback))
		mxTypeError("callback is no function");
	mxPushSlot(mxTarget);
	fxGetPrototypeFromConstructor(the, &mxFinalizationRegistryPrototype);
	instance = fxNewSlot(the);
	instance->kind = XS_INSTANCE_KIND;
	instance->value.instance.garbage = C_NULL;
	instance->value.instance.prototype = the->stack->value.reference;
	the->stack->kind = XS_REFERENCE_KIND;
	the->stack->value.reference = instance;
	mxPullSlot(mxResult);
	property = instance->next = fxNewSlot(the);
	property->flag = XS_INTERNAL_FLAG | XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG;
	property->kind = XS_CLOSURE_KIND;
	property->value.closure = C_NULL;
	registry = fxNewSlot(the);
	registry->kind = XS_FINALIZATION_REGISTRY_KIND;
	registry->value.finalizationRegistry.callback = C_NULL;
	registry->value.finalizationRegistry.flags = XS_NO_FLAG;
	property->value.closure = registry;
	slot = fxNewSlot(the);
	slot->kind = callback->kind;
	slot->value = callback->value;
	registry->value.finalizationRegistry.callback = slot;
}

void fx_FinalizationRegistry_prototype_cleanupSome(txMachine* the)
{
	txSlot* instance;
	txSlot* registry;
	txSlot* callback = C_NULL;
	txSlot** address;
	txSlot* slot;
	if (!mxIsReference(mxThis))
		mxTypeError("this is no object");
	instance = fxCheckFinalizationRegistryInstance(the, mxThis);
	registry = instance->next->value.closure;
	if (mxArgc > 0) {
		callback = mxArgv(0);
		if (mxIsUndefined(callback))
			callback = C_NULL;
		else if (!fxIsCallable(the, callback))
			mxTypeError("callback is no function");
	}
	fx_FinalizationRegistryCleanup(the, registry, callback);
	callback = registry->value.finalizationRegistry.callback;
	if (callback->next == C_NULL) {
		address = &(mxFinalizationRegistries.value.reference->next);
		while ((slot = *address)) {
			if (slot->value.closure == registry) {
				*address = slot->next;
				return;
			}
			address = &(slot->next);
		}
	}
}

void fx_FinalizationRegistry_prototype_register(txMachine* the)
{
	txSlot* instance;
	txSlot* registry;
	txSlot* target;
	txSlot* token = C_NULL;
	txSlot* callback;
	txSlot** address;
	txSlot* slot;
	if (!mxIsReference(mxThis))
		mxTypeError("this is no object");
	instance = fxCheckFinalizationRegistryInstance(the, mxThis);
	registry = instance->next->value.closure;
	if (mxArgc < 1)
		mxTypeError("no target");
	target = mxArgv(0);
	if (!mxIsReference(target))
		mxTypeError("target is no object");
	if (mxArgc > 1) {
		if (fxIsSameValue(the, target, mxArgv(1), 1))
			mxTypeError("target and holdings are the same");
	}
	target = target->value.reference;
	if (mxArgc > 2) {
		token = mxArgv(2);
		if (mxIsUndefined(token))
			token = C_NULL;
		else if (mxIsReference(token))
			token = token->value.reference;
		else
			mxTypeError("token is no object");
	}
	callback = registry->value.finalizationRegistry.callback;
	address = &(callback->next);
	while ((slot = *address))
		address = &(slot->next);
	slot = *address = fxNewSlot(the);
	if (mxArgc > 1) {
		slot->kind = mxArgv(1)->kind;
		slot->value = mxArgv(1)->value;
	}
	slot = slot->next = fxNewSlot(the);
	slot->kind = XS_FINALIZATION_CELL_KIND;
	slot->value.finalizationCell.target = target;
	slot->value.finalizationCell.token = token;
	
	address = &(mxFinalizationRegistries.value.reference->next);
	while ((slot = *address)) {
		if (slot->value.closure == registry)
			return;
		address = &(slot->next);
	}
	slot = *address = fxNewSlot(the);
	slot->kind = XS_CLOSURE_KIND;
	slot->value.closure = registry;
}	

void fx_FinalizationRegistry_prototype_unregister(txMachine* the)
{
	txSlot* instance;
	txSlot* token;
	txSlot* registry;
	txSlot* callback;
	txSlot** address;
	txSlot* slot;
	if (!mxIsReference(mxThis))
		mxTypeError("this is no object");
	instance = fxCheckFinalizationRegistryInstance(the, mxThis);
	if (mxArgc < 1)
		mxTypeError("no token");
	token = mxArgv(0);
	if (!mxIsReference(token))
		mxTypeError("token is no object");
	token = token->value.reference;
	mxResult->kind = XS_BOOLEAN_KIND;
	mxResult->value.boolean = 0;
	registry = instance->next->value.closure;
	callback = registry->value.finalizationRegistry.callback;
	address = &(callback->next);
	while ((slot = *address)) {
		slot = slot->next;
		if (slot->value.finalizationCell.token && fxIsSameInstance(the, slot->value.finalizationCell.token, token)) {
			*address = slot->next;
			mxResult->value.boolean = 1;
		}
		else
			address = &(slot->next);
	}
	if (callback->next == C_NULL) {
		address = &(mxFinalizationRegistries.value.reference->next);
		while ((slot = *address)) {
			if (slot->value.closure == registry) {
				*address = slot->next;
				return;
			}
			address = &(slot->next);
		}
	}
}

void fx_FinalizationRegistryCleanup(txMachine* the, txSlot* registry, txSlot* callback)
{
	txSlot* slot;
	txUnsigned flags;
	txSlot** address;
	txSlot* value;

	if (!(registry->value.finalizationRegistry.flags & XS_FINALIZATION_REGISTRY_CHANGED))
		return;
		
	slot = registry->value.finalizationRegistry.callback->next;
	flags = 0;
	while (slot) {
		slot = slot->next;
		if (slot->value.finalizationCell.target == C_NULL) {
			flags = 1;
			break;
		}
		slot = slot->next;
	}
	if (!flags)
		return;
	if (!callback)
		callback = registry->value.finalizationRegistry.callback;
	flags = registry->value.finalizationRegistry.flags;
	{
		mxTry(the) {
			address = &(registry->value.finalizationRegistry.callback->next);
			while ((value = *address)) {
				slot = value->next;
				if (slot->value.finalizationCell.target == C_NULL) {
					*address = slot->next;
					mxPushUndefined();
					mxPushSlot(callback);
					mxCall();
					mxPushSlot(value);
					mxRunCount(1);
					mxPop();
				}
				else
					address = &(slot->next);
			}
			registry->value.finalizationRegistry.flags = flags;
		}
		mxCatch(the) {
			registry->value.finalizationRegistry.flags = flags;
			fxJump(the);
		}
	}
	
	slot = registry->value.finalizationRegistry.callback->next;
	while (slot) {
		if (slot->value.finalizationCell.target == C_NULL)
			break;
		slot = slot->next;
	}
	if (!slot)
		registry->value.finalizationRegistry.flags &= ~XS_FINALIZATION_REGISTRY_CHANGED;
}

void fxCleanupFinalizationRegistries(txMachine* the)
{
	txSlot** address = &(mxFinalizationRegistries.value.reference->next);
	txSlot* closure;
	while ((closure = *address)) {
		txSlot* registry = closure->value.closure;
		fx_FinalizationRegistryCleanup(the, registry, C_NULL);
		if (registry->value.finalizationRegistry.callback->next == C_NULL)
			*address = closure->next;
		else
			address = &(closure->next);
	}
}


