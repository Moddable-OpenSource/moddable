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

static void fxCompleteModule(txMachine* the, txSlot* realm, txSlot* module, txSlot* exception);
static void fxFulfillImport(txMachine* the, txSlot* realm, txSlot* module);
static txSlot* fxGetModule(txMachine* the, txSlot* realm, txID moduleID);
static txInteger fxGetModuleStatus(txMachine* the, txSlot* realm, txSlot* module);
static txSlot* fxLinkModule(txMachine* the, txSlot* realm, txID moduleID, txSlot* name);
static void fxOrderModule(txMachine* the, txSlot* realm, txSlot* module);
static txSlot* fxQueueModule(txMachine* the, txSlot* realm, txID moduleID);
static void fxRecurseExports(txMachine* the, txSlot* realm, txID moduleID, txSlot* circularities, txSlot* exports);
static void fxRejectImport(txMachine* the, txSlot* realm, txSlot* module);
static void fxResolveExports(txMachine* the, txSlot* realm, txSlot* module);
static void fxResolveFrom(txMachine* the, txSlot* realm, txID moduleID, txSlot* name);
static void fxResolveLocals(txMachine* the, txSlot* realm, txSlot* module);
static void fxResolveModules(txMachine* the, txSlot* realm);
static void fxResolveNamespace(txMachine* the, txSlot* realm, txID fromID, txSlot* transfer);
static void fxResolveTransfer(txMachine* the, txSlot* realm, txID fromID, txID importID, txSlot* transfer);
static void fxRunImportSync(txMachine* the, txSlot* realm, txID id);

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

static txSlot* fxCheckCompartmentInstance(txMachine* the, txSlot* slot);
static void fxObjectToModule(txMachine* the, txSlot* realm, txID id);

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
	
	mxPush(mxObjectPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextStringXProperty(the, slot, "Module", mxID(_Symbol_toStringTag), XS_GET_ONLY);
	mxModulePrototype = *the->stack;
	
	mxPush(mxObjectPrototype);
	fxNewObjectInstance(the);
	mxTransferPrototype = *the->stack;

	mxPush(mxObjectPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostAccessorProperty(the, slot, mxCallback(fx_Compartment_prototype_get_globalThis), C_NULL, mxID(_globalThis), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Compartment_prototype_evaluate), 1, mxID(_evaluate), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Compartment_prototype_import), 1, mxID(_import), XS_DONT_ENUM_FLAG);
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Compartment_prototype_importNow), 1, mxID(_importNow), XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "Compartment", mxID(_Symbol_toStringTag), XS_GET_ONLY);
	mxCompartmentPrototype = *the->stack;
	slot = fxBuildHostConstructor(the, mxCallback(fx_Compartment), 1, mxID(_Compartment));
	mxCompartmentConstructor = *the->stack;
    the->stack++;
}

txBoolean fxIsLoadingModule(txMachine* the, txSlot* realm, txID moduleID)
{
	txSlot** address = &(mxLoadingModules(realm)->value.reference->next);
	txSlot* slot;
	while ((slot = *address)) {
		if (slot->ID == moduleID)
			return 1;
		address = &(slot->next);
	}
	return 0;
}

void fxRunModule(txMachine* the, txSlot* realm, txID moduleID, txScript* script)
{
	fxQueueModule(the, realm, moduleID);
	fxResolveModule(the, realm, moduleID, script, C_NULL, C_NULL);
	fxResolveModules(the, realm);
	gxDefaults.executeModules(the, realm, XS_NO_FLAG);
}

void fxAwaitImport(txMachine* the, txBoolean defaultFlag)
{
	txSlot* stack = the->stack;
	txSlot* realm = mxModuleInstanceInternal(mxProgram.value.reference)->value.module.realm;
	txID defaultID;
	txSlot* export;
	mxTry(the) {
		fxToString(the, stack);
		if (defaultFlag & XS_IMPORT_PREFLIGHT) {
			txID moduleID = fxFindModule(the, realm, XS_NO_ID, stack);
			stack->kind = XS_BOOLEAN_KIND;
			stack->value.boolean = moduleID != XS_NO_ID;
		}
		else {
			fxRunImportSync(the, realm, XS_NO_ID);
			if (defaultFlag) {
				defaultID = mxID(_default);
				export = mxModuleExports(stack)->value.reference->next;
				while (export) {
					mxCheck(the, export->kind == XS_EXPORT_KIND);
					if (export->ID == defaultID) {
						stack->kind = export->value.export.closure->kind;
						stack->value = export->value.export.closure->value;
						break;
					}
					export = export->next;
				}
			}
		}
	}
	mxCatch(the) {
		fxJump(the);
	}
	the->stack = stack;
}

void fxCompleteModule(txMachine* the, txSlot* realm, txSlot* module, txSlot* exception)
{
	txSlot** address = &(mxRunningModules(realm)->value.reference->next);
	txSlot* property;
	while ((property = *address)) {
		if (property->value.reference == module) {
			*address = property->next;
			break;
		}
		address = &(property->next);
	}
	if (exception) {
		property = mxBehaviorSetProperty(the, mxRejectedModules(realm)->value.reference, mxModuleInstanceInternal(module)->value.module.id, XS_NO_ID, XS_OWN);
		property->value = exception->value;
		property->kind = exception->kind;
	}
}

void fxExecuteModules(txMachine* the, txSlot* realm, txFlag flag)
{
	txSlot* runningModules = mxRunningModules(realm)->value.reference;
	txSlot* waitingModules = mxWaitingModules(realm)->value.reference;
	txSlot** waitingAddress = &(waitingModules->next);
	txSlot* module;
	txSlot* property;
	txSlot* slot;
	txSlot* result;
	txSlot* function;
	txSlot* home;
	while ((module = *waitingAddress)) {
		property = mxModuleInitialize(module);
		if (property->kind == XS_REFERENCE_KIND) {
			mxPushSlot(mxModuleHosts(module));
			mxPull(mxHosts);
			mxPushUndefined();
			mxPushSlot(property);
			mxCall();
			mxRunCount(0);
			mxPop();
			mxPushUndefined();
			mxPull(mxHosts);
		}
		waitingAddress = &(module->next);
	}
    waitingAddress = &(waitingModules->next);
	while ((module = *waitingAddress)) {
		txInteger status = fxGetModuleStatus(the, realm, module);
		if (status < 0) {
			*waitingAddress = module->next;
			module->next = C_NULL;
			if (flag == XS_ASYNC_FLAG)
				fxRejectImport(the, realm, module->value.reference);
			else
				fxJump(the);
		}
		else if (status > 0) {
		#if mxReport
			fxReport(the, "# Executing module \"%s\"\n", fxGetKeyName(the, module->ID));
		#endif
			*waitingAddress = module->next;
			module->next = C_NULL;
			slot = fxLastProperty(the, runningModules);
			slot->next = module;
			
			mxPushSlot(mxModuleHosts(module));
			mxPull(mxHosts);
			{
				mxTry(the) {
					mxPushUndefined();
					mxPushSlot(mxModuleExecute(module));
					mxCall();
					mxRunCount(0);
					result = the->stack;
					if (mxIsReference(result) && mxIsPromise(result->value.reference)) {
						mxDub();
						fxGetID(the, mxID(_then));
						mxCall();
					
						function = fxNewHostFunction(the, fxFulfillModule, 1, XS_NO_ID);
						home = mxFunctionInstanceHome(function);
						home->value.home.object = realm;
						home->value.home.module = module->value.reference;
	
						function = fxNewHostFunction(the, fxRejectModule, 1, XS_NO_ID);
						home = mxFunctionInstanceHome(function);
						home->value.home.object = realm;
						home->value.home.module = module->value.reference;
				
						mxRunCount(2);
						mxPop();
					}
					else {
						mxPop();
						fxCompleteModule(the, realm, module->value.reference, C_NULL);
						fxFulfillImport(the, realm, module->value.reference);
					}
				}
				mxCatch(the) {
					fxCompleteModule(the, realm, module->value.reference, &mxException);
					if (flag == XS_ASYNC_FLAG)
						fxRejectImport(the, realm, module->value.reference);
					else
						fxJump(the);
				}
			}
			mxPushUndefined();
			mxPull(mxHosts);
		}
		else
			waitingAddress = &(module->next);
	}
}

void fxExecuteModulesSync(txMachine* the, txSlot* realm, txFlag flag)
{
	txSlot* runningModules = mxRunningModules(realm)->value.reference;
	txSlot* waitingModules = mxWaitingModules(realm)->value.reference;
	txSlot** waitingAddress = &(waitingModules->next);
	txSlot* module;
	txSlot* property;
	txSlot* slot;
	while ((module = *waitingAddress)) {
		property = mxModuleInitialize(module);
		if (property->kind == XS_REFERENCE_KIND) {
			mxPushSlot(mxModuleHosts(module));
			mxPull(mxHosts);
			mxPushUndefined();
			mxPushSlot(property);
			mxCall();
			mxRunCount(0);
			mxPop();
			mxPushUndefined();
			mxPull(mxHosts);
		}
		waitingAddress = &(module->next);
	}
    waitingAddress = &(waitingModules->next);
	while ((module = *waitingAddress)) {
		txInteger status = fxGetModuleStatus(the, realm, module);
		if (status < 0) {
			*waitingAddress = module->next;
			module->next = C_NULL;
			fxJump(the);
		}
		else if (status > 0) {
		#if mxReport
			fxReport(the, "# Executing module \"%s\"\n", fxGetKeyName(the, module->ID));
		#endif
			*waitingAddress = module->next;
			module->next = C_NULL;
			slot = fxLastProperty(the, runningModules);
			slot->next = module;
			
			mxPushSlot(mxModuleHosts(module));
			mxPull(mxHosts);
			{
				mxTry(the) {
					mxPushUndefined();
					mxPushSlot(mxModuleExecute(module));
					mxCall();
					mxRunCount(0);
					mxPop();
					fxCompleteModule(the, realm, module->value.reference, C_NULL);
					mxModuleMeta(module)->next = C_NULL;
				}
				mxCatch(the) {
					fxCompleteModule(the, realm, module->value.reference, &mxException);
					mxModuleMeta(module)->next = C_NULL;
					fxJump(the);
				}
			}
			mxPushUndefined();
			mxPull(mxHosts);
		}
		else
			waitingAddress = &(module->next);
	}
}

void fxFulfillImport(txMachine* the, txSlot* realm, txSlot* module)
{
	txSlot* stack = the->stack;
	txSlot* slot = mxModuleInstanceFulfill(module);
	while (slot) {
		mxPushSlot(slot);
		slot = slot->next;
		slot = slot->next;
	}
	mxModuleInstanceMeta(module)->next = C_NULL;
#if mxReport
	fxReport(the, "# Fullfilled module \"%s\"\n", fxGetKeyName(the, mxModuleInstanceInternal(module)->value.module.id));
#endif
	slot = stack;
	while (slot > the->stack) {
		slot--;
		/* THIS */
		mxPushUndefined();
		/* FUNCTION */
		mxPushSlot(slot);
		mxCall();
		/* ARGUMENTS */
		mxPushReference(module);
		mxRunCount(1);
		mxPop();
	}
	the->stack = stack;
}

void fxFulfillModule(txMachine* the)
{
	txSlot* home = mxFunctionInstanceHome(mxFunction->value.reference);
	txSlot* realm = home->value.home.object;
	txSlot* module = home->value.home.module;
	fxCompleteModule(the, realm, module, C_NULL);
	fxFulfillImport(the, realm, module);
	gxDefaults.executeModules(the, realm, XS_ASYNC_FLAG);
}

txSlot* fxGetModule(txMachine* the, txSlot* realm, txID moduleID)
{
	txSlot* result = mxBehaviorGetProperty(the, mxOwnModules(realm)->value.reference, moduleID, XS_NO_ID, XS_OWN);
	if (result)
		return result;
	result = the->sharedModules;
	while (result) {
		if (result->ID == moduleID)
			return result;
		result = result->next;
	}
	return C_NULL;
}

txInteger fxGetModuleStatus(txMachine* the, txSlot* realm, txSlot* module)
{
	txSlot* rejectedModules = mxRejectedModules(realm)->value.reference;
	txSlot* runningModules = mxRunningModules(realm)->value.reference;
	txSlot* waitingModules = mxWaitingModules(realm)->value.reference;
	txSlot* fromModules = mxModuleTransfers(module)->value.reference;
	txSlot* fromModule;
	if (mxModuleTransfers(module)->kind == XS_NULL_KIND)
		return 1;
	fromModule = fromModules->next;
	while (fromModule) {
		txSlot* exception = mxBehaviorGetProperty(the, rejectedModules, fromModule->ID, XS_NO_ID, XS_OWN);
		if (exception) {
			txSlot* property = mxBehaviorSetProperty(the, rejectedModules, mxModuleInternal(module)->value.module.id, XS_NO_ID, XS_OWN);
			property->value = exception->value;
			property->kind = exception->kind;
			return -1;
		}
		fromModule = fromModule->next;
	}
	fromModule = fromModules->next;
	while (fromModule) {
		txSlot* runningModule = runningModules->next;
		while (runningModule) {
			if (fromModule->value.reference == runningModule->value.reference)
				return 0;
			runningModule = runningModule->next;
		}
		fromModule = fromModule->next;
	}
	fromModule = fromModules->next;
	while (fromModule) {
		txSlot* waitingModule = waitingModules->next;
		while (waitingModule) {
            if (module->value.reference == waitingModule->value.reference)
                break;
			if (fromModule->value.reference == waitingModule->value.reference)
				return 0;
			waitingModule = waitingModule->next;
		}
		fromModule = fromModule->next;
	}
	return 1;
}

txSlot* fxLinkModule(txMachine* the, txSlot* realm, txID moduleID, txSlot* name)
{
	mxTry(the) {
		fxResolveFrom(the, realm, moduleID, name);
		mxCheck(the, mxLoadingModules(realm)->value.reference->next == C_NULL);
		fxResolveModules(the, realm);
		mxCheck(the, mxLoadingModules(realm)->value.reference->next == C_NULL);
		mxCheck(the, mxLoadedModules(realm)->value.reference->next == C_NULL);
	}
	mxCatch(the) {
		mxLoadingModules(realm)->value.reference->next = C_NULL;
		mxLoadedModules(realm)->value.reference->next = C_NULL;
		fxJump(the);
	}
	return fxGetModule(the, realm, name->value.symbol);
}

void fxOrderModule(txMachine* the, txSlot* realm, txSlot* module)
{
	txSlot* transfer = mxModuleTransfers(module)->value.reference->next;
	txSlot** fromAddress = &(mxLoadedModules(realm)->value.reference->next);
	txSlot** toAddress = &(mxLoadingModules(realm)->value.reference->next);
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
			to = mxLoadedModules(realm)->value.reference->next;
			while (to) {
				if (to->ID == from->value.symbol)
					break;
				to = to->next;
			}
			if (to)
				fxOrderModule(the, realm, to);
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

void fxPrepareModule(txMachine* the)
{
	txSlot* module = mxFunctionInstanceHome(mxFunction->value.reference)->value.home.module;
	txInteger c = the->stack->value.integer, i;
	txSlot* argument = the->stack + c;
	txSlot* result = the->stack + c;
	txSlot* slot;
	txSlot* property;
	slot = argument--;
	property = mxModuleInstanceInitialize(module);	
	property->kind = slot->kind;
	property->value = slot->value;
	slot = argument--;
	property = mxModuleInstanceExecute(module);	
	property->kind = slot->kind;
	property->value = slot->value;
	slot = &mxHosts;	
	property = mxModuleInstanceHosts(module);	
	property->kind = slot->kind;
	property->value = slot->value;
	mxPush(mxObjectPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	for (i = 2; i < c; i++)
		slot = fxNextSlotProperty(the, slot, argument--, XS_NO_ID, XS_DONT_ENUM_FLAG);
	property = mxModuleInstanceTransfers(module);	
	mxPullSlot(property);
	result->kind = XS_REFERENCE_KIND;
	result->value.reference = module;
	the->stack = result;
}

void fxPrepareTransfer(txMachine* the)
{
	txInteger c = the->stack->value.integer, i;
	txSlot* argument = the->stack + c;
	txSlot* result = the->stack + c;
	txSlot* property;
	txSlot* slot;
	mxPush(mxTransferPrototype);
	property = fxNewObjectInstance(the);
	property = fxNextSlotProperty(the, property, argument--, mxID(_local), XS_DONT_ENUM_FLAG);
	property = fxNextSlotProperty(the, property, argument--, mxID(_from), XS_DONT_ENUM_FLAG);
	property = fxNextSlotProperty(the, property, argument--, mxID(_import), XS_DONT_ENUM_FLAG);
	if (c > 3) {
		mxPush(mxObjectPrototype);
		slot = fxLastProperty(the, fxNewObjectInstance(the));
		for (i = 3; i < c; i++)
			slot = fxNextSlotProperty(the, slot, argument--, XS_NO_ID, XS_DONT_ENUM_FLAG);
		property = fxNextSlotProperty(the, property, the->stack, mxID(_aliases), XS_DONT_ENUM_FLAG);
		mxPop();
	}
	else {
		property = fxNextNullProperty(the, property, mxID(_aliases), XS_DONT_ENUM_FLAG);
	}
	property = fxNextNullProperty(the, property, mxID(_closure), XS_DONT_ENUM_FLAG);
	result->kind = the->stack->kind;
	result->value = the->stack->value;
	the->stack = result;
	
// #ifdef mxDebug
// 	slot = the->frame->next;
// 	if (slot) {
// 		slot = slot - 1;
// 		if (slot->next) {
// 			property = fxNextSlotProperty(the, property, slot->next, XS_NO_ID, XS_DONT_ENUM_FLAG);
// 			property = fxNextIntegerProperty(the, property, slot->ID, XS_NO_ID, XS_DONT_ENUM_FLAG);
// 		}
// 	}
// #endif
}

txSlot* fxQueueModule(txMachine* the, txSlot* realm, txID moduleID)
{
	txSlot** address = &(mxLoadingModules(realm)->value.reference->next);
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
	slot->value.module.realm = realm;
	slot->value.module.id = moduleID;
	/* EXPORTS */
	slot = fxNextNullProperty(the, slot, XS_NO_ID, XS_DONT_ENUM_FLAG);
	/* META */
	slot = fxNextNullProperty(the, slot, XS_NO_ID, XS_DONT_ENUM_FLAG);
	/* TRANSFERS */
	slot = fxNextNullProperty(the, slot, XS_NO_ID, XS_DONT_ENUM_FLAG);
	/* INITIALIZE */
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

void fxRecurseExports(txMachine* the, txSlot* realm, txID moduleID, txSlot* circularities, txSlot* exports)
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
	module = fxGetModule(the, realm, moduleID);
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
				fxRecurseExports(the, realm, from->value.symbol, circularitiesCopy, stars);
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

void fxRejectImport(txMachine* the, txSlot* realm, txSlot* module)
{
	txSlot* stack = the->stack;
	txSlot* slot = mxModuleInstanceFulfill(module);
	txSlot* exception;
	while (slot) {
		slot = slot->next;
		mxPushSlot(slot);
		slot = slot->next;
	}
	mxModuleInstanceMeta(module)->next = C_NULL;
#if mxReport
	fxReport(the, "# Rejected module \"%s\"\n", fxGetKeyName(the, mxModuleInstanceInternal(module)->value.module.id));
#endif
	exception = mxBehaviorGetProperty(the, mxRejectedModules(realm)->value.reference, mxModuleInstanceInternal(module)->value.module.id, XS_NO_ID, XS_OWN);
    slot = stack;
	while (slot > the->stack) {
		slot--;
		/* THIS */
		mxPushUndefined();
		/* FUNCTION */
		mxPushSlot(slot);
		mxCall();
		/* ARGUMENTS */
		mxPushSlot(exception);
		mxRunCount(1);
		mxPop();
	}
	the->stack = stack;
}

void fxRejectModule(txMachine* the)
{
	txSlot* home = mxFunctionInstanceHome(mxFunction->value.reference);
	txSlot* realm = home->value.home.object;
	txSlot* module = home->value.home.module;
	fxCompleteModule(the, realm, module, mxArgv(0));
	fxRejectImport(the, realm, module);
	gxDefaults.executeModules(the, realm, XS_ASYNC_FLAG);
}

void fxResolveExports(txMachine* the, txSlot* realm, txSlot* module)
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
					fxResolveTransfer(the, realm, from->value.symbol, import->value.symbol, transfer);
				else
					fxResolveNamespace(the, realm, from->value.symbol, transfer);
			}
			closure = mxTransferClosure(transfer);
			transfer->kind = closure->kind;
			transfer->value = closure->value;
		}
		transfer = transfer->next;
	}
}

void fxResolveFrom(txMachine* the, txSlot* realm, txID moduleID, txSlot* name)
{
	txSlot* module;
	moduleID = fxFindModule(the, realm, moduleID, name);
	if (moduleID == XS_NO_ID) {
		fxToStringBuffer(the, name, the->nameBuffer, sizeof(the->nameBuffer));
		mxReferenceError("module \"%s\" not found", the->nameBuffer);
	}
	name->kind = XS_SYMBOL_KIND;
	name->value.symbol = moduleID;
	module = fxGetModule(the, realm, moduleID);
	if (module)
		return;
	module = mxBehaviorGetProperty(the, mxLoadedModules(realm)->value.reference, moduleID, XS_NO_ID, XS_OWN);
	if (!module) {
		module = mxBehaviorGetProperty(the, mxLoadingModules(realm)->value.reference, moduleID, XS_NO_ID, XS_OWN);
		if (!module) {
			module = fxQueueModule(the, realm, moduleID);
		}
	}
	while ((module = mxLoadingModules(realm)->value.reference->next)) {
		#if mxReport
			fxReport(the, "# Loading module \"%s\"\n", fxGetKeyName(the, module->ID));
		#endif
		fxLoadModule(the, realm, module->ID);
	}
}

void fxResolveLocals(txMachine* the, txSlot* realm, txSlot* module)
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
					fxResolveTransfer(the, realm, from->value.symbol, import->value.symbol, transfer);
				else
					fxResolveNamespace(the, realm, from->value.symbol, transfer);
				closure = mxTransferClosure(transfer);
				closure->flag |= XS_DONT_SET_FLAG;
			}
		}
		transfer = transfer->next;
	}
}

void fxResolveModule(txMachine* the, txSlot* realm, txID moduleID, txScript* script, void* data, txDestructor destructor)
{
	txSlot** fromAddress = &(mxLoadingModules(realm)->value.reference->next);
	txSlot** toAddress = &(mxLoadedModules(realm)->value.reference->next);
	txSlot* module;
	txSlot* slot;
	txSlot* key;
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
	
	slot = mxModuleExecute(module);	
	if (slot->kind == XS_NULL_KIND) {
		mxReferenceError("\"%s\" is no module", fxGetKeyName(the, module->ID));
	}
	
	slot = fxNewInstance(the);
	key = fxGetKey(the, moduleID);
	if (key) {
		slot = slot->next = fxNewSlot(the);
		slot->value.string = key->value.key.string;
		if (key->kind == XS_KEY_KIND)
			slot->kind = XS_STRING_KIND;
		else
			slot->kind = XS_STRING_X_KIND;
		slot->ID = mxID(_uri);
	}
	slot = mxModuleMeta(module);	
	mxPullSlot(slot);
	
	transfer = mxModuleTransfers(module)->value.reference->next;
	while (transfer) {
		from = mxTransferFrom(transfer);
		if (from->kind != XS_NULL_KIND) {
			fxResolveFrom(the, realm, module->ID, from);
		}
		transfer = transfer->next;
	}
}

void fxResolveModules(txMachine* the, txSlot* realm)
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
	
	modules = mxLoadedModules(realm)->value.reference;
	while ((module = modules->next)) {
		fxOrderModule(the, realm, module);
	}
	modules = mxLoadingModules(realm)->value.reference;
	
	module = modules->next;
	while (module) {
		#if mxReport
			fxIDToString(the, module->ID, the->nameBuffer, sizeof(the->nameBuffer));
			fxReport(the, "# Resolving module \"%s\"\n", the->nameBuffer);
		#endif
		transfer = mxModuleTransfers(module)->value.reference->next;
		while (transfer) {
			local = mxTransferLocal(transfer);
			from = mxTransferFrom(transfer);
			import = mxTransferImport(transfer);
			if (local->kind != XS_NULL_KIND) {
				if ((from->kind == XS_NULL_KIND) || (import->kind == XS_NULL_KIND)) {
					closure = mxTransferClosure(transfer);
					closure->value.export.closure = fxNewSlot(the);
					closure->value.export.closure->kind = XS_UNINITIALIZED_KIND;
					closure->value.export.module = module->value.reference;
                	closure->kind = XS_EXPORT_KIND;
				}
			}
			else {
				if ((from->kind != XS_NULL_KIND) && (import->kind == XS_NULL_KIND)) {
					closure = mxTransferClosure(transfer);
					closure->value.export.closure = fxNewSlot(the);
					closure->value.export.closure->kind = XS_UNINITIALIZED_KIND;
					closure->value.export.module = module->value.reference;
                	closure->kind = XS_EXPORT_KIND;
				}
			}
			transfer = transfer->next;
		}
		property = mxBehaviorSetProperty(the, mxOwnModules(realm)->value.reference, module->ID, XS_NO_ID, XS_OWN);
		property->value = module->value;
		property->kind = XS_REFERENCE_KIND;
		module = module->next;
	}

	module = modules->next;
	while (module) {
		fxNewInstance(the);
		circularities = the->stack;
		fxNewInstance(the);
		exports = the->stack;
		fxRecurseExports(the, realm, module->ID, circularities, exports);
		mxModuleExports(module)->kind = XS_REFERENCE_KIND;
		mxModuleExports(module)->value.reference = exports->value.reference;
		the->stack++;
		the->stack++;
		module = module->next;
	}

	module = modules->next;
	while (module) {
		fxResolveExports(the, realm, module);
		module = module->next;
	}
	module = modules->next;
	while (module) {
		fxResolveLocals(the, realm, module);
		module = module->next;
	}
	
	module = modules->next;
	while (module) {
		txInteger count = 0;
		mxPushUndefined();
		closures = fxNewEnvironmentInstance(the, mxRealmClosures(realm));
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
			from = mxTransferFrom(transfer);
			if (from->kind != XS_NULL_KIND)
				count++;
			transfer = transfer->next;
		}
		property = mxModuleInitialize(module);
		if (property->kind == XS_REFERENCE_KIND)
			property->value.reference->next->value.code.closures = closures;
		property = mxModuleExecute(module);
		if (property->kind == XS_REFERENCE_KIND)
			property->value.reference->next->value.code.closures = closures;
		the->stack++;
		if (count > 0) {
			txSlot* instance = fxNewInstance(the);
			transfer = mxModuleTransfers(module)->value.reference->next;
			while (transfer) {
				from = mxTransferFrom(transfer);
				if (from->kind != XS_NULL_KIND) {
					txSlot** address = &(instance->next);
					txSlot* slot;
					while ((slot = *address)) {
						if (slot->ID == from->value.symbol)
							break;
						address = &slot->next;
					}
					if (!slot)
						slot = *address = fxDuplicateSlot(the, fxGetModule(the, realm, from->value.symbol));
				}
				transfer = transfer->next;
			}
			mxModuleTransfers(module)->value.reference = instance;
			mxPop();
		}
		else
			mxModuleTransfers(module)->kind = XS_NULL_KIND;
		module = module->next;
	}
	module = fxLastProperty(the, mxWaitingModules(realm)->value.reference);
	module->next = modules->next;
	modules->next = C_NULL;
}

void fxResolveNamespace(txMachine* the, txSlot* realm, txID fromID, txSlot* transfer)
{
	txSlot* export = mxTransferClosure(transfer);
	mxCheck(the, export->kind == XS_EXPORT_KIND);
	export->value.export.closure = fxGetModule(the, realm, fromID);
}

void fxResolveTransfer(txMachine* the, txSlot* realm, txID fromID, txID importID, txSlot* transfer)
{
	txSlot* module;
	txSlot* export;
	txSlot* exportClosure;
	txSlot* transferClosure;
	txSlot* from;
	txSlot* import;
	
	module = fxGetModule(the, realm, fromID);
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
					fxResolveTransfer(the, realm, from->value.symbol, import->value.symbol, transfer);
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

void fxRunImport(txMachine* the, txSlot* realm, txID id)
{
	txSlot* stack = the->stack;
    txSlot* function = mxFunction;
    txSlot* module = (function->kind == XS_REFERENCE_KIND) ? mxFunctionInstanceHome(function->value.reference)->value.home.module : C_NULL;
	txSlot* promise;
	txSlot* status;
	txSlot* fulfillFunction;
	txSlot* rejectFunction;
	txSlot* slot;
	
	fxBeginHost(the);
	mxPush(mxPromisePrototype);
	promise = fxNewPromiseInstance(the);
	status = mxPromiseStatus(promise);
	status->value.integer = mxPendingStatus;
	fxPushPromiseFunctions(the, promise);
	fulfillFunction = the->stack + 1;
	rejectFunction = the->stack;
	{
		mxTry(the) {
			txSlot* waitingModule = mxWaitingModules(realm)->value.reference->next;
			fxToString(the, stack);
			module = fxLinkModule(the, realm, id, stack);
			if (mxModuleMeta(module)->next == C_NULL) {
				txSlot* exception = mxBehaviorGetProperty(the, mxRejectedModules(realm)->value.reference, mxModuleInternal(module)->value.module.id, XS_NO_ID, XS_OWN);
				if (exception) {
					/* THIS */
					mxPushUndefined();
					/* FUNCTION */
					mxPushSlot(rejectFunction);
					mxCall();
					/* ARGUMENTS */
					mxPushSlot(exception);
					mxRunCount(1);
				}
				else {
					/* THIS */
					mxPushUndefined();
					/* FUNCTION */
					mxPushSlot(fulfillFunction);
					mxCall();
					/* ARGUMENTS */
					mxPushSlot(module);
					mxRunCount(1);
				}
			}
			else {
				slot = fxLastProperty(the, module->value.reference);
				slot = fxNextSlotProperty(the, slot, fulfillFunction, XS_NO_ID, XS_NO_FLAG);
				slot = fxNextSlotProperty(the, slot, rejectFunction, XS_NO_ID, XS_NO_FLAG);
				if (!waitingModule)
					gxDefaults.executeModules(the, realm, XS_ASYNC_FLAG);
			}
		}
		mxCatch(the) {
			fxRejectException(the, rejectFunction);
		}
	}
	fxEndHost(the);
	stack->value.reference = promise;
	stack->kind = XS_REFERENCE_KIND;
	the->stack = stack;
}

void fxRunImportSync(txMachine* the, txSlot* realm, txID id)
{
	txSlot* stack = the->stack;
	txSlot* module = C_NULL;
	mxTry(the) {
		module = fxLinkModule(the, realm, XS_NO_ID, stack);
		gxDefaults.executeModules(the, realm, XS_NO_FLAG);
		stack->kind = module->kind;
		stack->value = module->value;
	}
	mxCatch(the) {
		fxJump(the);
	}
	the->stack = stack;
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

txSlot* fxCheckCompartmentInstance(txMachine* the, txSlot* slot)
{
	txSlot* instance = slot->value.reference;
	if (((slot = instance->next)) && (slot->flag & XS_INTERNAL_FLAG) && (slot->kind == XS_PROGRAM_KIND)) {
		return instance;
	}
	mxTypeError("this is no Compartment instance");
	return C_NULL;
}

void fxObjectToModule(txMachine* the, txSlot* realm, txID id)
{
	txSlot* stack = the->stack;
	txSlot* object;
	txSlot* module;
	txSlot* slot;
	txSlot* export;
	txSlot* meta;
	txSlot* key;
	txSlot* at;
	txSlot* property;
	object = stack->value.reference;
	mxPush(mxModulePrototype);
	module = fxNewObjectInstance(the);
	module->flag |= XS_EXOTIC_FLAG;
	/* HOST */
	slot = module->next = fxNewSlot(the);
	slot->ID = XS_MODULE_BEHAVIOR;
	slot->flag = XS_INTERNAL_FLAG;
	slot->kind = XS_MODULE_KIND;
	slot->value.module.realm = realm;
	slot->value.module.id = id;
	/* EXPORTS */
	export = fxNewInstance(the);
	slot = fxNextSlotProperty(the, slot, the->stack, XS_NO_ID, XS_DONT_ENUM_FLAG);
	mxPop();
	/* META */
	meta = fxNewInstance(the);
	key = fxGetKey(the, id);
	if (key) {
		meta = meta->next = fxNewSlot(the);
		meta->value.string = key->value.key.string;
		if (key->kind == XS_KEY_KIND)
			meta->kind = XS_STRING_KIND;
		else
			meta->kind = XS_STRING_X_KIND;
		meta->ID = mxID(_uri);
	}
	slot = fxNextSlotProperty(the, slot, the->stack, XS_NO_ID, XS_DONT_ENUM_FLAG);
	mxPop();
	
	at = fxNewInstance(the);
	mxBehaviorOwnKeys(the, object, XS_EACH_NAME_FLAG, at);
	mxTemporary(property);
	while ((at = at->next)) {
		if (mxBehaviorGetOwnProperty(the, object, at->value.at.id, at->value.at.index, property) && !(property->flag & XS_DONT_ENUM_FLAG)) {
			export = export->next = fxNewSlot(the);
			export->ID = at->value.at.id;
			export->kind = XS_EXPORT_KIND;
			export->value.export.closure = fxNewSlot(the);
			export->value.export.closure->kind = property->kind;
			export->value.export.closure->value = property->value;
			export->value.export.module = module;
		}
	}
	stack->value.reference = module;
	the->stack = stack;
}

#define mxIsModule(THE_SLOT) \
	(((THE_SLOT)->next) && ((THE_SLOT)->next->flag & XS_INTERNAL_FLAG) && ((THE_SLOT)->next->kind == XS_MODULE_KIND))

void fx_Compartment(txMachine* the)
{
	txSlot* module = mxFunctionInstanceHome(mxFunction->value.reference)->value.home.module;
	txSlot* realm = C_NULL;
	txSlot* filter = C_NULL;
	txSlot* program = C_NULL;
	txSlot* global = C_NULL;
	txSlot* slot;
	txSlot* own;
	txString string;
	txID id;
	
	if (!module) module = mxProgram.value.reference;
	realm = mxModuleInstanceInternal(module)->value.module.realm;
	filter = mxAvailableModules(realm)->value.reference;
	mxTry(the) {
// 		if (the->sharedMachine == C_NULL)
// 			mxTypeError("no compartments");
		if (mxIsUndefined(mxTarget))
			mxTypeError("call Compartment");
			
		mxPushSlot(mxTarget);
		fxGetPrototypeFromConstructor(the, &mxCompartmentPrototype);
		program = fxNewProgramInstance(the);
		mxPullSlot(mxResult);
		
		mxPush(mxObjectPrototype);
		global = fxNewObjectInstance(the);
		slot = fxLastProperty(the, global);
		for (id = XS_SYMBOL_ID_COUNT; id < _Infinity; id++)
			slot = fxNextSlotProperty(the, slot, &the->stackPrototypes[-1 - id], mxID(id), XS_DONT_ENUM_FLAG);
		for (; id < _Compartment; id++)
			slot = fxNextSlotProperty(the, slot, &the->stackPrototypes[-1 - id], mxID(id), XS_GET_ONLY);
		for (; id < XS_INTRINSICS_COUNT; id++) {
			txSlot* instance = fxDuplicateInstance(the, the->stackPrototypes[-1 - id].value.reference);
			mxFunctionInstanceHome(instance)->value.home.module = program;
			slot = fxNextSlotProperty(the, slot, the->stack, mxID(id), XS_DONT_ENUM_FLAG);
			mxPop();
		}
		slot = fxNextSlotProperty(the, slot, the->stack, mxID(_global), XS_DONT_ENUM_FLAG);
		slot = fxNextSlotProperty(the, slot, the->stack, mxID(_globalThis), XS_DONT_ENUM_FLAG);
		if (mxArgc > 0) {
			mxPushUndefined();
			mxPush(mxAssignObjectFunction);
			mxCall();
			mxPushReference(global);
			mxPushSlot(mxArgv(0));
			mxRunCount(2);
		}
		
		if (mxArgc > 1) {
			txSlot* target;
			txSlot* source;
			txSlot* at;
			txSlot* property;
			txID moduleID;
			target = fxNewInstance(the);
			own = fxNewInstance(the);
			mxPushSlot(mxArgv(1));
			source = fxToInstance(the, the->stack);
			at = fxNewInstance(the);
			mxBehaviorOwnKeys(the, source, XS_EACH_NAME_FLAG, at);
			mxTemporary(property);
			while ((at = at->next)) {
				if (mxBehaviorGetOwnProperty(the, source, at->value.at.id, at->value.at.index, property) && !(property->flag & XS_DONT_ENUM_FLAG)) {
					mxPushReference(source);
					fxGetAll(the, at->value.at.id, at->value.at.index);
					if (mxIsReference(the->stack)) {
						if (mxIsModule(the->stack->value.reference)) {
							string = C_NULL;
							moduleID = mxModuleInternal(the->stack)->value.module.id;
							slot = filter->next;
							while (slot) {
								if (slot->value.symbol == moduleID)
									break;
								slot = slot->next;
							}
							if (slot) {
								target = target->next = fxNewSlot(the);
								target->ID = at->value.at.id;
								target->kind = XS_SYMBOL_KIND;
								target->value.symbol = moduleID;
								own = own->next = fxNewSlot(the);
								own->ID = moduleID;
								own->kind = XS_REFERENCE_KIND;
								own->value.reference = the->stack->value.reference;
							}
							else {
								mxReferenceError("module \"%s\" not available", fxGetKeyName(the, moduleID));
							}
						}
						else {
							fxObjectToModule(the, realm, at->value.at.id);
							target = target->next = fxNewSlot(the);
							target->ID = at->value.at.id;
							target->kind = XS_SYMBOL_KIND;
							target->value.symbol = at->value.at.id;
							own = own->next = fxNewSlot(the);
							own->ID = at->value.at.id;
							own->kind = XS_REFERENCE_KIND;
							own->value.reference = the->stack->value.reference;
						}
					}
					else {
						string = fxToString(the, the->stack);
						if (the->stack->kind == XS_STRING_X_KIND)
							moduleID = fxNewNameX(the, string);
						else
							moduleID = fxNewName(the, the->stack);
						slot = filter->next;
						while (slot) {
							if (slot->ID == moduleID)
								break;
							slot = slot->next;
						}
						if (slot) {
							target = target->next = fxNewSlot(the);
							target->ID = at->value.at.id;
							target->kind = XS_SYMBOL_KIND;
							target->value.symbol = slot->value.symbol;
						}
						else {
							mxReferenceError("module \"%s\" not available", fxGetKeyName(the, moduleID));
						}
					}
					mxPop();
				}
			}
			mxPop(); // property
			mxPop(); // at
			mxPop(); // source
		}
		else {
			mxPushReference(filter);
			fxNewInstance(the);
		}
		mxModuleInstanceInternal(program)->value.module.realm = fxNewRealmInstance(the);
		mxPop();
	}
	mxCatch(the) {
		fxJump(the);
	}
}

void fx_Compartment_prototype_get_globalThis(txMachine* the)
{
	txSlot* program = fxCheckCompartmentInstance(the, mxThis);
	txSlot* realm = mxModuleInstanceInternal(program)->value.module.realm;
	txSlot* global = mxRealmGlobal(realm);
	mxResult->kind = global->kind;
	mxResult->value = global->value;
}

void fx_Compartment_prototype_evaluate(txMachine* the)
{
	txSlot* program = fxCheckCompartmentInstance(the, mxThis);
	txSlot* realm = mxModuleInstanceInternal(program)->value.module.realm;
	if (mxArgc > 0) {
		txStringStream stream;
		stream.slot = mxArgv(0);
		stream.offset = 0;
		stream.size = c_strlen(fxToString(the, mxArgv(0)));
		fxRunScript(the, fxParseScript(the, &stream, fxStringGetter, mxStrictFlag | mxProgramFlag | mxEvalFlag | mxDebugFlag), mxRealmGlobal(realm), C_NULL, mxRealmClosures(realm)->value.reference, C_NULL, program);
		mxPullSlot(mxResult);
	}
}

void fx_Compartment_prototype_import(txMachine* the)
{
	txSlot* program = fxCheckCompartmentInstance(the, mxThis);
	txSlot* realm = mxModuleInstanceInternal(program)->value.module.realm;
	if (mxArgc > 0)
		mxPushSlot(mxArgv(0));
	else
		mxPushUndefined();
	fxToString(the, the->stack);
	gxDefaults.runImport(the, realm, XS_NO_ID);
	mxPullSlot(mxResult);
}

void fx_Compartment_prototype_importNow(txMachine* the)
{
	txSlot* program = fxCheckCompartmentInstance(the, mxThis);
	txSlot* realm = mxModuleInstanceInternal(program)->value.module.realm;
	if (mxArgc > 0)
		mxPushSlot(mxArgv(0));
	else
		mxPushUndefined();
	fxToString(the, the->stack);
	fxRunImportSync(the, realm, XS_NO_ID);
	mxPullSlot(mxResult);
}
