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

static txSlot* fxGetModule(txMachine* the, txID moduleID);
static void fxImportModule(txMachine* the, txID moduleID, txSlot* name);
static void fxOrderModule(txMachine* the, txSlot* module);

static txSlot* fxQueueModule(txMachine* the, txID moduleID, txSlot* name);
static void fxRecurseExports(txMachine* the, txID moduleID, txSlot* circularities, txSlot* exports);
static void fxResolveExports(txMachine* the, txSlot* module);
static void fxResolveLocals(txMachine* the, txSlot* module);
static void fxResolveModules(txMachine* the);
static void fxResolveNamespace(txMachine* the, txID fromID, txSlot* transfer);
static void fxResolveTransfer(txMachine* the, txID fromID, txID importID, txSlot* transfer);
static void fxRunModules(txMachine* the);

static txBoolean fxModuleDefineOwnProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* slot, txFlag mask);
static txBoolean fxModuleDeleteProperty(txMachine* the, txSlot* instance, txID id, txIndex index);
static txBoolean fxModuleGetOwnProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* slot);
static txSlot* fxModuleGetProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txFlag flag);
static txBoolean fxModuleGetPropertyValue(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* receiver, txSlot* value);
static txBoolean fxModuleGetPrototype(txMachine* the, txSlot* instance, txSlot* result);
static txBoolean fxModuleHasProperty(txMachine* the, txSlot* instance, txID id, txIndex index); 
static txBoolean fxModuleIsExtensible(txMachine* the, txSlot* instance);
static void fxModuleOwnKeys(txMachine* the, txSlot* instance, txFlag flag, txSlot* keys);
static int fxModuleOwnKeysCompare(const void* p, const void* q);
static txBoolean fxModulePreventExtensions(txMachine* the, txSlot* instance);
static txSlot* fxModuleSetProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txFlag flag);
static txBoolean fxModuleSetPropertyValue(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* value, txSlot* receiver);
static txBoolean fxModuleSetPrototype(txMachine* the, txSlot* instance, txSlot* prototype);

const txBehavior ICACHE_FLASH_ATTR gxModuleBehavior = {
	fxModuleGetProperty,
	fxModuleSetProperty,
	fxOrdinaryCall,
	fxOrdinaryConstruct,
	fxModuleDefineOwnProperty,
	fxModuleDeleteProperty,
	fxModuleGetOwnProperty,
	fxModuleGetPropertyValue,
	fxModuleGetPrototype,
	fxModuleHasProperty,
	fxModuleIsExtensible,
	fxModuleOwnKeys,
	fxModulePreventExtensions,
	fxModuleSetPropertyValue,
	fxModuleSetPrototype,
};

void fxBuildModule(txMachine* the)
{
	txSlot* slot;
	
	slot = fxLastProperty(the, fxNewHostFunctionGlobal(the, mxCallback(fx_require), 1, mxID(_require), XS_DONT_ENUM_FLAG));
	slot = fxNextHostAccessorProperty(the, slot, mxCallback(fx_require_get_busy), mxCallback(fx_require_set_busy), mxID(_busy), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG);
	slot = fxNextHostAccessorProperty(the, slot, mxCallback(fx_require_get_cache), C_NULL, mxID(_cache), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG);
	slot = fxNextHostAccessorProperty(the, slot, mxCallback(fx_require_get_uri), C_NULL, mxID(_uri), XS_DONT_DELETE_FLAG | XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_require_resolve), 1, mxID(_resolve), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_require_weak), 1, mxID(_weak), XS_DONT_ENUM_FLAG);
	the->stack++;
	
	mxPush(mxObjectPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextStringProperty(the, slot, "Module", mxID(_Symbol_toStringTag), XS_GET_ONLY);
	mxModulePrototype = *the->stack;
	fxNewHostConstructor(the, mxCallback(fx_Module), 1, XS_NO_ID);
	mxPull(mxModuleConstructor);
	
	mxPush(mxObjectPrototype);
	fxNewObjectInstance(the);
	mxTransferPrototype = *the->stack;
	fxNewHostConstructor(the, mxCallback(fx_Transfer), 1, XS_NO_ID);
	mxPull(mxTransferConstructor);
}

txID fxCurrentModuleID(txMachine* the)
{
	txSlot* frame = the->frame;
	while (frame) {
		txSlot* function = frame + 3;
		if (function->kind == XS_REFERENCE_KIND) {
			txSlot* home = mxFunctionInstanceHome(function->value.reference);
            if (home->value.home.module) {
                txSlot* slot = mxModuleInstanceInternal(home->value.home.module);
				return slot->value.symbol;
            }
		}
		frame = frame->next;
	}
	return XS_NO_ID;
}

txBoolean fxIsLoadingModule(txMachine* the, txID moduleID)
{
	txSlot** address = &(mxLoadingModules.value.reference->next);
	txSlot* slot;
	while ((slot = *address)) {
		if (slot->ID == moduleID)
			return 1;
		address = &(slot->next);
	}
	return 0;
}

txSlot* fxRequireModule(txMachine* the, txID moduleID, txSlot* name)
{
	txSlot* module;
	moduleID = fxFindModule(the, moduleID, name);
	if (moduleID == XS_NO_ID) {
		fxToStringBuffer(the, name, the->nameBuffer, sizeof(the->nameBuffer));
		mxReferenceError("module \"%s\" not found", the->nameBuffer);
	}
	module = fxGetModule(the, moduleID);
	if (module)
		return module;
	module = mxBehaviorGetProperty(the, mxLoadedModules.value.reference, moduleID, XS_NO_ID, XS_OWN);
	if (!module) {
		module = mxBehaviorGetProperty(the, mxLoadingModules.value.reference, moduleID, XS_NO_ID, XS_OWN);
		if (!module) {
			module = fxQueueModule(the, moduleID, name);
		}
	}
	while ((module = mxLoadingModules.value.reference->next)) {
		#if mxReport
			fxReport(the, "# Loading module \"%s\"\n", fxGetKeyName(the, module->ID));
		#endif
		fxLoadModule(the, module->ID);
	}
	fxResolveModules(the);
	fxRunModules(the);
	mxImportingModules.value.reference->next = C_NULL;
	return fxGetModule(the, moduleID);
}

void fxResolveModule(txMachine* the, txID moduleID, txScript* script, void* data, txDestructor destructor)
{
	txSlot** fromAddress = &(mxLoadingModules.value.reference->next);
	txSlot** toAddress = &(mxLoadedModules.value.reference->next);
	txSlot* module;
	txSlot* slot;
	txSlot* transfer;
	txSlot* from;
	while ((module = *fromAddress)) {
		if (module->ID == moduleID) {
			*fromAddress = module->next;
			module->next = C_NULL;
			break;
		}
        fromAddress = &(module->next);
	}
	mxPushClosure(module);
	fxRunScript(the, script, module, C_NULL, C_NULL, C_NULL, module->value.reference);
	the->stack++;
	while ((module = *toAddress))
        toAddress = &(module->next);
	*toAddress = module = the->stack->value.closure;
	the->stack++;
	transfer = mxModuleTransfers(module)->value.reference->next;
	while (transfer) {
		from = mxTransferFrom(transfer);
		if (from->kind != XS_NULL_KIND) {
			fxImportModule(the, module->ID, from);
			if (from->kind != XS_SYMBOL_KIND) {
				txString path = C_NULL;
				txInteger line = 0;
			#ifdef mxDebug
				slot = mxTransferClosure(transfer)->next;
				if (slot) {
					path = slot->value.string;
					line = slot->next->value.integer;
				}
			#endif	
				fxToStringBuffer(the, from, the->nameBuffer, sizeof(the->nameBuffer));
				fxThrowMessage(the, path, line, XS_REFERENCE_ERROR, "module \"%s\" not found", the->nameBuffer); 
			}
		}
		transfer = transfer->next;
	}
}

void fxRunModule(txMachine* the, txID moduleID, txScript* script)
{
	fxQueueModule(the, moduleID, C_NULL);
	fxResolveModule(the, moduleID, script, C_NULL, C_NULL);
	fxResolveModules(the);
	fxRunModules(the);
	mxImportingModules.value.reference->next = C_NULL;
}

void fx_require_get_busy(txMachine* the)
{
	txID moduleID = fxCurrentModuleID(the);
	txSlot* module = the->sharedModules;
	mxResult->value.boolean = 1;
	mxResult->kind = XS_BOOLEAN_KIND;
	while (module) {
		if (module->ID == moduleID)
			return;
		module = module->next;
	}
	if (!mxBehaviorGetProperty(the, mxRequiredModules.value.reference, moduleID, XS_NO_ID, XS_OWN))
		mxResult->value.boolean = 0;
}

void fx_require_get_cache(txMachine* the)
{
	txSlot* table = mxModules.value.reference->next;
	txSlot** address = table->value.table.address;
	txInteger modulo = table->value.table.length;
	txSlot* list;
	txSlot* item;
	mxPush(mxObjectPrototype);
	list = fxNewObjectInstance(the);
	mxPullSlot(mxResult);
	while (modulo) {
		txSlot* entry = *address;
		while (entry) {
			txSlot* module = entry->value.entry.slot;
			txID moduleID = mxModuleInternal(module)->value.symbol;
			txSlot* key = fxGetKey(the, moduleID);
			txSlot* property = mxBehaviorGetProperty(the, mxRequiredModules.value.reference, moduleID, XS_NO_ID, XS_OWN);
			
			mxPush(mxObjectPrototype);
			item = fxNewObjectInstance(the);

			if (mxGetKeySlotKind(key) == XS_KEY_KIND)
				mxPushString(key->value.key.string);
			else
				mxPushStringX(key->value.key.string);
			mxPushReference(item);
			fxSetID(the, mxID(_name));
			mxPop();
			
			mxPushBoolean(property ? 1 : 0);
			mxPushReference(item);
			fxSetID(the, mxID(_busy));
			mxPop();
			
			mxPushReference(list);
			fxSetID(the, moduleID);
			mxPop();
			entry = entry->next;
		}
		address++;
		modulo--;
	}
	{
		txSlot* module = the->sharedModules;
		while (module) {
			txID moduleID = mxModuleInternal(module)->value.symbol;
			txSlot* key = fxGetKey(the, moduleID);

			mxPush(mxObjectPrototype);
			item = fxNewObjectInstance(the);

			if (mxGetKeySlotKind(key) == XS_KEY_KIND)
				mxPushString(key->value.key.string);
			else
				mxPushStringX(key->value.key.string);
			mxPushReference(item);
			fxSetID(the, mxID(_name));
			mxPop();
			
			mxPushBoolean(1);
			mxPushReference(item);
			fxSetID(the, mxID(_busy));
			mxPop();
			
			mxPushReference(list);
			fxSetID(the, moduleID);
			mxPop();
			
			module = module->next;
		}
	}
}

void fx_require_get_uri(txMachine* the)
{
	txID moduleID = fxCurrentModuleID(the);
	txSlot* key = fxGetKey(the, moduleID);
	if (key) {
		if (key->kind == XS_KEY_KIND)
			mxResult->kind = XS_STRING_KIND;
		else
			mxResult->kind = XS_STRING_X_KIND;
		mxResult->value.string = key->value.key.string;
	}
}

void fx_require_set_busy(txMachine* the)
{
	txID moduleID = fxCurrentModuleID(the);
	txSlot* module = the->sharedModules;
	while (module) {
		if (module->ID == moduleID)
			return;
		module = module->next;
	}
	mxPushSlot(mxArgv(0));
	if (fxRunTest(the)) {
		txSlot* module = fxGetModule(the, moduleID);
		txSlot* property = mxBehaviorSetProperty(the, mxRequiredModules.value.reference, module->ID, XS_NO_ID, XS_OWN);
		property->value = module->value;
		property->kind = XS_REFERENCE_KIND;
	}
	else {
		fxOrdinaryDeleteProperty(the, mxRequiredModules.value.reference, moduleID, XS_NO_ID);
	}
}

void fx_require(txMachine* the)
{
	txSlot* module;
	txID defaultID;
	txSlot* export;
	mxTry(the) {
		if (mxArgc == 0)
			mxSyntaxError("no module id");
		fxToString(the, mxArgv(0));
		the->requireFlag |= XS_REQUIRE_FLAG;
		module = fxRequireModule(the, fxCurrentModuleID(the), mxArgv(0));
		
		defaultID = mxID(_default);
		export = mxModuleExports(module)->value.reference->next;
		while (export) {
			mxCheck(the, export->kind == XS_EXPORT_KIND);
			if (export->ID == defaultID) {
				mxResult->kind = export->value.export.closure->kind;
				mxResult->value = export->value.export.closure->value;
				break;
			}
			export = export->next;
		}
				
		the->requireFlag &= ~XS_REQUIRE_FLAG;
	}
	mxCatch(the) {
		mxImportingModules.value.reference->next = C_NULL;
		mxLoadingModules.value.reference->next = C_NULL;
		mxLoadedModules.value.reference->next = C_NULL;
		mxResolvingModules.value.reference->next = C_NULL;
		the->requireFlag &= ~XS_REQUIRE_FLAG;
		fxJump(the);
	}
}

void fx_require_resolve(txMachine* the)
{
	txID moduleID;
	txSlot* key;
	if (mxArgc == 0)
		mxSyntaxError("no module id");
	fxToString(the, mxArgv(0));
	moduleID = fxFindModule(the, fxCurrentModuleID(the), mxArgv(0));
	if (moduleID == XS_NO_ID)
		return;
	key = fxGetKey(the, moduleID);
	if (mxGetKeySlotKind(key) == XS_KEY_KIND)
		mxResult->kind = XS_STRING_KIND;
	else
		mxResult->kind = XS_STRING_X_KIND;
	mxResult->value.string = key->value.key.string;
}

void fx_require_weak(txMachine* the)
{
	mxTry(the) {
		if (mxArgc == 0)
			mxSyntaxError("no module id");
		the->requireFlag |= XS_REQUIRE_WEAK_FLAG;
		
		mxPushSlot(mxArgv(0));
		mxPushInteger(1);
		mxPushUndefined();
		mxPushSlot(mxThis);
		fxCall(the);
		mxPullSlot(mxResult);
		
		the->requireFlag &= ~XS_REQUIRE_WEAK_FLAG;
	}
	mxCatch(the) {
		the->requireFlag &= ~XS_REQUIRE_WEAK_FLAG;
		fxJump(the);
	}
}

void fx_Module(txMachine* the)
{
	txInteger c = mxArgc, i;
	txSlot* slot;
	txSlot* property;
	slot = mxArgv(0);
	property = mxModuleFunction(mxThis);	
	property->kind = slot->kind;
	property->value = slot->value;
	mxPush(mxObjectPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	for (i = 1; i < c; i++)
		slot = fxNextSlotProperty(the, slot, mxArgv(i), XS_NO_ID, XS_DONT_ENUM_FLAG);
	property = mxModuleTransfers(mxThis);	
	mxPullSlot(property);
	*mxResult = *mxThis;
}

void fx_Transfer(txMachine* the)
{
	txInteger c = mxArgc, i;
	txSlot* property;
	txSlot* slot;
	mxPush(mxTransferPrototype);
	property = fxNewObjectInstance(the);
	mxPullSlot(mxResult);
	property = fxNextSlotProperty(the, property, mxArgv(0), mxID(_local), XS_DONT_ENUM_FLAG);
	property = fxNextSlotProperty(the, property, mxArgv(1), mxID(_from), XS_DONT_ENUM_FLAG);
	property = fxNextSlotProperty(the, property, mxArgv(2), mxID(_import), XS_DONT_ENUM_FLAG);
	if (c > 3) {
		mxPush(mxObjectPrototype);
		slot = fxLastProperty(the, fxNewObjectInstance(the));
		for (i = 3; i < c; i++)
			slot = fxNextSlotProperty(the, slot, mxArgv(i), XS_NO_ID, XS_DONT_ENUM_FLAG);
		property = fxNextSlotProperty(the, property, the->stack, mxID(_aliases), XS_DONT_ENUM_FLAG);
		the->stack++;
	}
	else {
		property = fxNextNullProperty(the, property, mxID(_aliases), XS_DONT_ENUM_FLAG);
	}
	property = fxNextNullProperty(the, property, mxID(_closure), XS_DONT_ENUM_FLAG);
#ifdef mxDebug
	slot = the->frame->next;
	if (slot) {
		slot = slot - 1;
		if (slot->next) {
			property = fxNextSlotProperty(the, property, slot->next, XS_NO_ID, XS_DONT_ENUM_FLAG);
			property = fxNextIntegerProperty(the, property, slot->ID, XS_NO_ID, XS_DONT_ENUM_FLAG);
		}
	}
#endif
}

txSlot* fxGetModule(txMachine* the, txID moduleID)
{
	txSlot* key = fxGetKey(the, moduleID);
	txU4 sum = key->value.key.sum;
	txSlot* table = mxModules.value.reference->next;
	txU4 modulo = sum % table->value.table.length;
	txSlot** address = &(table->value.table.address[modulo]);
	txSlot* entry;
	txSlot* result;
	while ((entry = *address)) {
		if (entry->value.entry.sum == sum) {
			result = entry->value.entry.slot;
			if (result->ID == moduleID)
				return result;
		}
		address = &entry->next;
	}
	result = the->sharedModules;
	while (result) {
		if (result->ID == moduleID)
			return result;
		result = result->next;
	}
	return C_NULL;
}

void fxImportModule(txMachine* the, txID moduleID, txSlot* name)
{
	txSlot* module;
	moduleID = fxFindModule(the, moduleID, name);
	if (moduleID == XS_NO_ID)
		return;
	module = fxGetModule(the, moduleID);
	if (module) {
		txSlot* slot = fxNewSlot(the);
		slot->next = mxImportingModules.value.reference->next;
		slot->kind = module->kind;
		slot->value = module->value;
		mxImportingModules.value.reference->next = slot;
	}
	else {
		module = mxBehaviorGetProperty(the, mxLoadedModules.value.reference, moduleID, XS_NO_ID, XS_OWN);
		if (!module) {
			module = mxBehaviorGetProperty(the, mxLoadingModules.value.reference, moduleID, XS_NO_ID, XS_OWN);
			if (!module) {
				module = fxQueueModule(the, moduleID, name);
			}
		}
	}
	name->kind = XS_SYMBOL_KIND;
	name->value.symbol = moduleID;
	while ((module = mxLoadingModules.value.reference->next)) {
		#if mxReport
			fxReport(the, "# Loading module \"%s\"\n", fxGetKeyName(the, module->ID));
		#endif
		fxLoadModule(the, module->ID);
	}
}

void fxOrderModule(txMachine* the, txSlot* module)
{
	txSlot* transfer = mxModuleTransfers(module)->value.reference->next;
	txSlot** fromAddress = &(mxLoadedModules.value.reference->next);
	txSlot** toAddress = &(mxResolvingModules.value.reference->next);
	txSlot* from;
	txSlot* to;
	while ((from = *fromAddress)) {
		if (from == module) {
			*fromAddress = module->next;
			module->next = C_NULL;
			break;
		}
        fromAddress = &(from->next);
	}
	while (transfer) {
		from = mxTransferFrom(transfer);
		if (from->kind != XS_NULL_KIND) {
			to = mxLoadedModules.value.reference->next;
			while (to) {
				if (to->ID == from->value.symbol)
					break;
				to = to->next;
			}
			if (to)
				fxOrderModule(the, to);
		}
		transfer = transfer->next;
	}
	while ((to = *toAddress)) {
		if (to->ID == module->ID)
			return;
        toAddress = &(to->next);
	}
	*toAddress = module;
}

txSlot* fxQueueModule(txMachine* the, txID moduleID, txSlot* name)
{
	txSlot** address = &(mxLoadingModules.value.reference->next);
	txSlot* slot;
	while ((slot = *address)) {
		if (slot->ID == moduleID) {
			break;
		}
		address = &(slot->next);
	}
	mxCheck(the, slot == C_NULL);
	
	mxPush(mxModulePrototype);
	slot = fxNewObjectInstance(the);
	slot->flag |= XS_EXOTIC_FLAG;
	/* HOST */
	slot = slot->next = fxNewSlot(the);
	slot->ID = XS_MODULE_BEHAVIOR;
	slot->flag = XS_INTERNAL_FLAG;
	slot->kind = XS_MODULE_KIND;
	slot->value.symbol = moduleID;
	/* EXPORTS */
	slot = fxNextNullProperty(the, slot, XS_NO_ID, XS_DONT_ENUM_FLAG);
	/* TRANSFERS */
	slot = fxNextNullProperty(the, slot, XS_NO_ID, XS_DONT_ENUM_FLAG);
	/* FUNCTION */
	slot = fxNextNullProperty(the, slot, XS_NO_ID, XS_DONT_ENUM_FLAG);
	/* HOSTS */
	slot = fxNextNullProperty(the, slot, XS_NO_ID, XS_DONT_ENUM_FLAG);

	*address = slot = fxNewSlot(the);
	slot->ID = moduleID;
	slot->kind = the->stack->kind;
	slot->value = the->stack->value;
	the->stack++;
	return slot;
}

void fxRecurseExports(txMachine* the, txID moduleID, txSlot* circularities, txSlot* exports)
{
	txSlot* circularitiesInstance;
	txSlot* circularity;
	txSlot* module;
	txSlot* transfers;
	txSlot* exportsInstance;
	txSlot* export;
	txSlot* transfer;
	txSlot* aliases;
	txSlot* alias;
	txSlot* local;
	txSlot* circularitiesCopy;
	txSlot* circularityCopy;
	txSlot* from;
	txSlot* stars;
	txSlot* star;
	circularitiesInstance = circularities->value.reference;
	if (mxBehaviorGetProperty(the, circularitiesInstance, moduleID, XS_NO_ID, XS_OWN))
		return;
	circularity = fxNewSlot(the);
	circularity->next = circularitiesInstance->next;
	circularity->ID = moduleID;
	circularitiesInstance->next = circularity;
	module = fxGetModule(the, moduleID);
	exportsInstance = exports->value.reference;
	export = exportsInstance;
	transfers = mxModuleExports(module);
	if (transfers->kind == XS_REFERENCE_KIND) {
		transfer = transfers->value.reference->next;
		while (transfer) {
			export = export->next = fxNewSlot(the);
			export->ID = transfer->ID;
			export->kind = transfer->kind;
			export->value = transfer->value;
			transfer = transfer->next;
		}
		return;
	}
	transfers = mxModuleTransfers(module);
	if (transfers->kind == XS_REFERENCE_KIND) {
		transfer = transfers->value.reference->next;
		while (transfer) {
			aliases = mxTransferAliases(transfer);
			if (aliases->kind == XS_REFERENCE_KIND) {
				alias = aliases->value.reference->next;
				while (alias) {
					export = export->next = fxNewSlot(the);
					export->ID = alias->value.symbol;
					export->kind = transfer->kind;
					export->value = transfer->value;
					alias = alias->next;
				}
			}
			transfer = transfer->next;
		}
		transfer = transfers->value.reference->next;
		while (transfer) {
			local = mxTransferLocal(transfer);
			aliases = mxTransferAliases(transfer);
			if ((local->kind == XS_NULL_KIND) && (aliases->kind == XS_NULL_KIND)) {
				from = mxTransferFrom(transfer);
				fxNewInstance(the);
				circularitiesCopy = the->stack;
				circularity = circularitiesInstance->next;
				circularityCopy = circularitiesCopy->value.reference;
				while (circularity) {
					circularityCopy = circularityCopy->next = fxNewSlot(the);
					circularityCopy->ID = circularity->ID;
					circularity = circularity->next;
				}
				fxNewInstance(the);
				stars = the->stack;
				fxRecurseExports(the, from->value.symbol, circularitiesCopy, stars);
				star = stars->value.reference->next;
				while (star) {
					if (star->ID != mxID(_default)) {
                        txSlot* ambiguous = mxBehaviorGetProperty(the, exportsInstance, star->ID, XS_NO_ID, XS_OWN);
						if (ambiguous) {
							txSlot* ambiguousModule;
							txSlot* starModule;
							if (ambiguous->kind == XS_EXPORT_KIND)
								ambiguousModule = ambiguous->value.export.module;
							else
								ambiguousModule = mxTransferClosure(ambiguous)->value.export.module;
							if (star->kind == XS_EXPORT_KIND)
								starModule = star->value.export.module;
							else
								starModule = mxTransferClosure(star)->value.export.module;
							if (ambiguousModule != starModule) {
								ambiguous->kind = XS_EXPORT_KIND;
								ambiguous->value.export.closure = C_NULL;
								ambiguous->value.export.module = C_NULL;
							}
						}
						else {
							export = export->next = fxNewSlot(the);
							export->ID = star->ID;
							export->kind = star->kind;
							export->value = star->value;
						}
					}
					star = star->next;
				}
				the->stack++;
				the->stack++;
			}
			transfer = transfer->next;
		}
	}
}

void fxResolveExports(txMachine* the, txSlot* module)
{
	txSlot* transfer;
	txSlot* from;
	txSlot* import;
	txSlot* closure;

	transfer = mxModuleExports(module)->value.reference->next;
	while (transfer) {
		if (transfer->kind != XS_EXPORT_KIND) {
			from = mxTransferFrom(transfer);
			if (from->kind != XS_NULL_KIND) {
				import = mxTransferImport(transfer);
				if (import->kind != XS_NULL_KIND)
					fxResolveTransfer(the, from->value.symbol, import->value.symbol, transfer);
				else
					fxResolveNamespace(the, from->value.symbol, transfer);
			}
			closure = mxTransferClosure(transfer);
			transfer->kind = closure->kind;
			transfer->value = closure->value;
		}
		transfer = transfer->next;
	}
}

void fxResolveLocals(txMachine* the, txSlot* module)
{
	txSlot* transfer;
	txSlot* local;
	txSlot* from;
	txSlot* import;
	txSlot* closure;

	transfer = mxModuleTransfers(module)->value.reference->next;
	while (transfer) {
		local = mxTransferLocal(transfer);
		if (local->kind != XS_NULL_KIND) {
			from = mxTransferFrom(transfer);
			if (from->kind != XS_NULL_KIND) {
				import = mxTransferImport(transfer);
				if (import->kind != XS_NULL_KIND)
					fxResolveTransfer(the, from->value.symbol, import->value.symbol, transfer);
				else
					fxResolveNamespace(the, from->value.symbol, transfer);
				closure = mxTransferClosure(transfer);
				closure->flag |= XS_DONT_SET_FLAG;
			}
		}
		transfer = transfer->next;
	}
}

void fxResolveModules(txMachine* the)
{
	txSlot* modules;
	txSlot* module;
	txSlot* transfer;
	txSlot* local;
	txSlot* from;
	txSlot* import;
	txSlot* closure;
	txSlot* property;
	txSlot* circularities;
	txSlot* exports;
	txSlot* closures;
	txSlot* export;
	
	modules = mxLoadedModules.value.reference;
	while ((module = modules->next)) {
		fxOrderModule(the, module);
	}
	modules = mxResolvingModules.value.reference;
	
	module = modules->next;
	while (module) {
		#if mxReport
			fxIDToString(the, module->ID, the->nameBuffer, sizeof(the->nameBuffer));
			fxReport(the, "# Resolving module \"%s\"\n", the->nameBuffer);
		#endif
		transfer = mxModuleTransfers(module)->value.reference->next;
		while (transfer) {
			local = mxTransferLocal(transfer);
			if (local->kind != XS_NULL_KIND) {
				from = mxTransferFrom(transfer);
				import = mxTransferImport(transfer);
				if ((from->kind == XS_NULL_KIND) || (import->kind == XS_NULL_KIND)) {
					closure = mxTransferClosure(transfer);
					closure->value.export.closure = fxNewSlot(the);
					closure->value.export.closure->kind = XS_UNINITIALIZED_KIND;
					closure->value.export.module = module->value.reference;
                    closure->kind = XS_EXPORT_KIND;
				}
			}
			transfer = transfer->next;
		}
		fxSetModule(the, module->ID, module);
		if (!(the->requireFlag & XS_REQUIRE_WEAK_FLAG)) {
			property = mxBehaviorSetProperty(the, mxRequiredModules.value.reference, module->ID, XS_NO_ID, XS_OWN);
			property->value = module->value;
			property->kind = XS_REFERENCE_KIND;
		}
		module = module->next;
	}

	module = modules->next;
	while (module) {
		fxNewInstance(the);
		circularities = the->stack;
		fxNewInstance(the);
		exports = the->stack;
		fxRecurseExports(the, module->ID, circularities, exports);
		mxModuleExports(module)->kind = XS_REFERENCE_KIND;
		mxModuleExports(module)->value.reference = exports->value.reference;
		the->stack++;
		the->stack++;
		module = module->next;
	}

	module = modules->next;
	while (module) {
		fxResolveExports(the, module);
		module = module->next;
	}
	module = modules->next;
	while (module) {
		fxResolveLocals(the, module);
		module = module->next;
	}
	
	module = modules->next;
	while (module) {
		txSlot* function = mxModuleFunction(module);
		if (function->kind == XS_REFERENCE_KIND) {
			mxPushUndefined();
			closures = fxNewEnvironmentInstance(the, &mxClosures);
			closure = closures->next;
			transfer = mxModuleTransfers(module)->value.reference->next;
			while (transfer) {
				local = mxTransferLocal(transfer);
				if (local->kind != XS_NULL_KIND) {
					export = mxTransferClosure(transfer);
					mxCheck(the, export->kind == XS_EXPORT_KIND);
					closure = closure->next = fxNewSlot(the);
					closure->ID = local->value.symbol;
					closure->flag = export->flag;
					closure->kind = XS_CLOSURE_KIND;
					closure->value.closure = export->value.export.closure;
				}
				transfer = transfer->next;
			}
			function->value.reference->next->value.code.closures = closures;
			the->stack++;
		}
		mxModuleTransfers(module)->kind = XS_NULL_KIND;
		module = module->next;
	}
}

void fxResolveNamespace(txMachine* the, txID fromID, txSlot* transfer)
{
	txSlot* export = mxTransferClosure(transfer);
	mxCheck(the, export->kind == XS_EXPORT_KIND);
	export->value.export.closure = fxGetModule(the, fromID);
}

void fxResolveTransfer(txMachine* the, txID fromID, txID importID, txSlot* transfer)
{
	txSlot* module;
	txSlot* export;
	txSlot* exportClosure;
	txSlot* transferClosure;
	txSlot* from;
	txSlot* import;
	
	module = fxGetModule(the, fromID);
	export = mxBehaviorGetProperty(the, mxModuleExports(module)->value.reference, importID, XS_NO_ID, XS_OWN);
	if (export) {
		if (export->kind == XS_EXPORT_KIND) {
			if (export->value.export.closure == C_NULL) {
				txString path = C_NULL;
				txInteger line = 0;
			#ifdef mxDebug
				txSlot* slot = mxTransferClosure(transfer)->next;
				if (slot) {
					path = slot->value.string;
					line = slot->next->value.integer;
				}
			#endif	
				fxIDToString(the, importID, the->nameBuffer, sizeof(the->nameBuffer));
				fxThrowMessage(the, path, line, XS_SYNTAX_ERROR, "import %s ambiguous", the->nameBuffer);
			}
			transferClosure = mxTransferClosure(transfer);
			transferClosure->value = export->value;
			transferClosure->kind = export->kind;
		}
		else {
			exportClosure = mxTransferClosure(export);
			if (exportClosure->kind != XS_NULL_KIND) {
				transferClosure = mxTransferClosure(transfer);
				transferClosure->value = exportClosure->value;
				transferClosure->kind = exportClosure->kind;
			}
			else {
				from = mxTransferFrom(export);
				import = mxTransferImport(export);
				if ((from->value.symbol != mxTransferFrom(transfer)->value.symbol) || (import->value.symbol != mxTransferImport(transfer)->value.symbol))
					fxResolveTransfer(the, from->value.symbol, import->value.symbol, transfer);
                else {
					txString path = C_NULL;
					txInteger line = 0;
				#ifdef mxDebug
					txSlot* slot = mxTransferClosure(transfer)->next;
					if (slot) {
						path = slot->value.string;
						line = slot->next->value.integer;
					}
				#endif	
					fxIDToString(the, importID, the->nameBuffer, sizeof(the->nameBuffer));
					fxThrowMessage(the, path, line, XS_SYNTAX_ERROR, "import %s circular", the->nameBuffer);
                }
			}
		}
	}
	else {
		txString path = C_NULL;
		txInteger line = 0;
	#ifdef mxDebug
		txSlot* slot = mxTransferClosure(transfer)->next;
		if (slot) {
			path = slot->value.string;
			line = slot->next->value.integer;
		}
	#endif	
		fxIDToString(the, importID, the->nameBuffer, sizeof(the->nameBuffer));
		fxThrowMessage(the, path, line, XS_SYNTAX_ERROR, "import %s not found", the->nameBuffer);
	}
}

void fxRunModules(txMachine* the)
{
	txSlot* modules = fxNewInstance(the);
	txSlot* module = modules->next = mxResolvingModules.value.reference->next;
	mxResolvingModules.value.reference->next = C_NULL;
	while (module) {
		txSlot* function = mxModuleFunction(module);
		if (function->kind == XS_REFERENCE_KIND) {
			#if mxReport
				fxReport(the, "# Running module \"%s\"\n", fxGetKeyName(the, module->ID));
			#endif
			mxPushSlot(mxModuleHosts(module));
			mxPull(mxHosts);
			mxPushInteger(0);
			mxPushUndefined();
			mxPushSlot(function);
			fxCall(the);
			the->stack++;
			mxPushUndefined();
			mxPull(mxHosts);
		}
		mxModuleExports(module)->next = C_NULL;
		module = module->next;
	}
	the->stack++;
}

void fxSetModule(txMachine* the, txID moduleID, txSlot* module)
{
	txSlot* table = mxModules.value.reference->next;
	txSlot* key = fxGetKey(the, moduleID);
	txU4 sum = key->value.key.sum;
	txU4 modulo = sum % table->value.table.length;
	txSlot** address = &(table->value.table.address[modulo]);
	txSlot* result;
	txSlot* entry;
    
    result = fxNewSlot(the);
    result->ID = moduleID;
    result->kind = module->kind;
    result->value = module->value;
	mxPushClosure(result);    
    
	entry = fxNewSlot(the);
	entry->next = *address;
	entry->kind = XS_ENTRY_KIND;
	entry->value.entry.slot = result;
	entry->value.entry.sum = sum;
	*address = entry;
	
	mxPop();
#if mxInstrument
	the->loadedModulesCount++;
#endif
}

txBoolean fxModuleDefineOwnProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* slot, txFlag mask)
{
	txSlot* property = fxModuleGetProperty(the, instance, id, index, XS_OWN);
	if (property) {
		if ((mask & XS_DONT_DELETE_FLAG) && !(slot->flag & XS_DONT_DELETE_FLAG))
			return 0;
		if ((mask & XS_DONT_ENUM_FLAG) && ((property->flag & XS_DONT_ENUM_FLAG) != (slot->flag & XS_DONT_ENUM_FLAG)))
			return 0;
		if (mask & XS_ACCESSOR_FLAG) {
			if (property->kind != XS_ACCESSOR_KIND)
				return 0;
			if (mask & XS_GETTER_FLAG) {
				if (property->value.accessor.getter != slot->value.accessor.getter)
					return 0;
			}
			if (mask & XS_SETTER_FLAG) {
				if (property->value.accessor.setter != slot->value.accessor.setter)
					return 0;
			}
			return 1;
		}
		if ((mask & XS_DONT_SET_FLAG) && ((property->flag & XS_DONT_SET_FLAG) != (slot->flag & XS_DONT_SET_FLAG)))
			return 0;
		if (property->kind == XS_ACCESSOR_KIND)
			return 0;
		if ((slot->kind != XS_UNINITIALIZED_KIND) && !fxIsSameValue(the, property, slot, 0))
			return 0;
		return 1;
	}
	return 0;
}

txBoolean fxModuleDeleteProperty(txMachine* the, txSlot* instance, txID id, txIndex index)
{
	txSlot* exports = mxModuleInstanceExports(instance);
	if (mxIsReference(exports)) {
		txSlot* property = exports->value.reference->next;
		while (property) {
			if (property->ID == id)
				return 0;
			property = property->next;
		}
	}
	if (id == mxID(_Symbol_toStringTag))
		return 0;
	return 1;
}

txBoolean fxModuleGetOwnProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* slot)
{
	txSlot* property = fxModuleGetProperty(the, instance, id, index, XS_OWN);
	if (property) {
		slot->flag = property->flag | XS_DONT_DELETE_FLAG;
		slot->kind = property->kind;
		slot->value = property->value;
		return 1;
	}
	slot->flag = XS_NO_FLAG;
	slot->kind = XS_UNDEFINED_KIND;
	return 0;
}

txSlot* fxModuleGetProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txFlag flag)
{
	txSlot* exports = mxModuleInstanceExports(instance);
	if (mxIsReference(exports)) {
		txSlot* property = exports->value.reference->next;
		while (property) {
			if (property->ID == id) {
				property = property->value.export.closure;
				if (property && (property->kind == XS_UNINITIALIZED_KIND)) {
					fxIDToString(the, id, the->nameBuffer, sizeof(the->nameBuffer));
					mxReferenceError("get %s: not initialized yet", the->nameBuffer);
				}
				return property;
			}
			property = property->next;
		}
	}
	if (id == mxID(_Symbol_toStringTag))
		return mxModulePrototype.value.reference->next;
	return C_NULL;
}

txBoolean fxModuleGetPropertyValue(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* receiver, txSlot* value)
{
	txSlot* property = fxModuleGetProperty(the, instance, id, index, XS_OWN);
	if (property) {
		value->kind = property->kind;
		value->value = property->value;
		return 1;
	}
	value->kind = XS_UNDEFINED_KIND;
	return 0;
}

txBoolean fxModuleGetPrototype(txMachine* the, txSlot* instance, txSlot* result)
{
	result->kind = XS_NULL_KIND;
	return 0;
}

txBoolean fxModuleHasProperty(txMachine* the, txSlot* instance, txID id, txIndex index) 
{
	txSlot* exports = mxModuleInstanceExports(instance);
	if (mxIsReference(exports)) {
		txSlot* property = exports->value.reference->next;
		while (property) {
			if (property->ID == id) {
				property = property->value.export.closure;
				return property ? 1 : 0;
			}
			property = property->next;
		}
	}
	if (id == mxID(_Symbol_toStringTag))
		return 1;
	return 0;
}

txBoolean fxModuleIsExtensible(txMachine* the, txSlot* instance)
{
	return 0;
}

void fxModuleOwnKeys(txMachine* the, txSlot* instance, txFlag flag, txSlot* keys)
{
	txSlot* exports = mxModuleInstanceExports(instance);
	if (mxIsReference(exports)) {
		if (flag & XS_EACH_NAME_FLAG) {
			txSlot* property = exports->value.reference->next;
			txSlot* stack = the->stack;
			while (property) {
				if (property->value.export.closure) {
					txSlot* key = fxGetKey(the, property->ID);
					mxPushSlot(key);
					the->stack->ID = key->ID;
				}
				property = property->next;
			}
			c_qsort(the->stack, stack - the->stack, sizeof(txSlot), fxModuleOwnKeysCompare);
			while (the->stack < stack) {
				keys = fxQueueKey(the, the->stack->ID, XS_NO_ID, keys);
				mxPop();
			}	
		}
		if (flag & XS_EACH_SYMBOL_FLAG) {
			txSlot* property = mxModulePrototype.value.reference->next;
			keys = fxQueueKey(the, property->ID, XS_NO_ID, keys);
		}
	}
}

int fxModuleOwnKeysCompare(const void* p, const void* q)
{
	return c_strcmp(((txSlot*)p)->value.key.string, ((txSlot*)q)->value.key.string);
}

txBoolean fxModulePreventExtensions(txMachine* the, txSlot* instance)
{
	return 1;
}

txSlot* fxModuleSetProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txFlag flag)
{
	return C_NULL;
}

txBoolean fxModuleSetPropertyValue(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* value, txSlot* receiver)
{
	return 0;
}

txBoolean fxModuleSetPrototype(txMachine* the, txSlot* instance, txSlot* prototype)
{
	return (prototype->kind == XS_NULL_KIND) ? 1 : 0;
}



