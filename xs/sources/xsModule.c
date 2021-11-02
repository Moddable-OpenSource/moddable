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

static void fxExecuteModules(txMachine* the, txSlot* realm, txSlot* queue);
static txID fxExecuteModulesFrom(txMachine* the, txSlot* realm, txSlot* queue, txSlot* module, txSlot** exception);

static txSlot* fxGetModule(txMachine* the, txSlot* realm, txID moduleID);

static void fxLinkCircularities(txMachine* the, txSlot* realm, txID moduleID, txSlot* circularities, txSlot* exports);
static void fxLinkExports(txMachine* the, txSlot* realm, txSlot* module);
static void fxLinkLocals(txMachine* the, txSlot* realm, txSlot* module);
static void fxLinkModules(txMachine* the, txSlot* realm, txSlot* queue);
static void fxLinkNamespace(txMachine* the, txSlot* realm, txID fromID, txSlot* transfer);
static void fxLinkTransfer(txMachine* the, txSlot* realm, txID fromID, txID importID, txSlot* transfer);

static void fxLoadModuleScript(txMachine* the, txID moduleID, txSlot* module, txScript* script);
static void fxLoadModules(txMachine* the, txSlot* realm, txSlot* queue);
static void fxLoadModulesFrom(txMachine* the, txSlot* realm, txSlot* queue, txSlot* module);

static void fxNewModule(txMachine* the, txSlot* realm, txID moduleID, txSlot* module);
static txID fxResolveSpecifier(txMachine* the, txSlot* realm, txID moduleID, txSlot* name);

static void fxRunImportFulfilled(txMachine* the, txSlot* realm, txSlot* module);
static void fxRunImportRejected(txMachine* the, txSlot* realm, txSlot* module);

static void fxRunImportNow(txMachine* the, txSlot* realm, txID id);

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

enum {
	XS_MODULE_STATUS_NONE = 0,
	XS_MODULE_STATUS_LOADING,
	XS_MODULE_STATUS_LOADED,
	XS_MODULE_STATUS_LINKING,
	XS_MODULE_STATUS_LINKED,
	XS_MODULE_STATUS_EXECUTING,
	XS_MODULE_STATUS_EXECUTED,
	XS_MODULE_STATUS_ERROR,
};

#define mxModuleInstanceStatus(MODULE)		((MODULE)->next->next->ID)
#define mxModuleStatus(MODULE) 				mxModuleInstanceStatus((MODULE)->value.reference)

void fxBuildModule(txMachine* the)
{
	txSlot* slot;
	
	mxPush(mxObjectPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextStringXProperty(the, slot, "Module", mxID(_Symbol_toStringTag), XS_GET_ONLY);
	mxModulePrototype = *the->stack;
    mxPop();
	
	mxPush(mxObjectPrototype);
	fxNewObjectInstance(the);
	mxTransferPrototype = *the->stack;
    mxPop();

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
    mxPop();
}

void fxCompleteModule(txMachine* the, txSlot* realm, txSlot* module, txSlot* exception)
{
	if (exception) {
		txSlot* exports = mxModuleInstanceExports(module);
		txSlot* meta = mxModuleInstanceMeta(module);
		if (exports->kind == XS_REFERENCE_KIND) {
			exports->value.reference->next = C_NULL;
		}
		else {
			fxNewInstance(the);
			mxPullSlot(exports);
		}
		meta->value = exception->value;
		meta->kind = exception->kind;
		mxModuleInstanceStatus(module) = XS_MODULE_STATUS_ERROR;
	}
	else {
		mxModuleInstanceStatus(module) = XS_MODULE_STATUS_EXECUTED;
	}
}

#if mxReport
#define mxModuleName(ID) c_strrchr(fxGetKeyName(the, (ID)), '/')
#endif	

void fxExecuteModules(txMachine* the, txSlot* realm, txSlot* queue)
{
#if mxReport
	{
		txSlot* module = queue->next;
		fprintf(stderr, "EXEC");
		while (module) {
			fprintf(stderr, " %s %d", mxModuleName(module->ID), mxModuleStatus(module));
			module = module->next;
		}
		fprintf(stderr, "\n");
	}
#endif	
	txSlot* module = queue->next;
	while (module) {
		if (mxModuleStatus(module) == XS_MODULE_STATUS_LINKED) {
			txSlot* exception;
			txID status = fxExecuteModulesFrom(the, realm, queue, module, &exception);
			if (status == XS_MODULE_STATUS_ERROR) {
				fxCompleteModule(the, realm, module->value.reference, exception);
				fxRunImportRejected(the, realm, module->value.reference);
			}
			else if (status == XS_MODULE_STATUS_LINKED) {
			#if mxReport
				fprintf(stderr, "# Executing module \"%s\"\n", mxModuleName(module->ID));
			#endif
				mxModuleStatus(module) = XS_MODULE_STATUS_EXECUTING;
				mxPushSlot(mxModuleHosts(module));
				mxPull(mxHosts);
				{
					mxTry(the) {
						txSlot* result;
						mxPushUndefined();
						mxPushSlot(mxModuleExecute(module));
						mxCall();
						mxRunCount(0);
						result = the->stack;
						if (mxIsReference(result) && mxIsPromise(result->value.reference)) {
							txSlot* function;
							txSlot* home;
							mxDub();
							mxGetID(mxID(_then));
							mxCall();
					
							function = fxNewHostFunction(the, fxExecuteModulesFulfilled, 1, XS_NO_ID);
							home = mxFunctionInstanceHome(function);
							home->value.home.object = realm;
							home->value.home.module = module->value.reference;
	
							function = fxNewHostFunction(the, fxExecuteModulesRejected, 1, XS_NO_ID);
							home = mxFunctionInstanceHome(function);
							home->value.home.object = realm;
							home->value.home.module = module->value.reference;
				
							mxRunCount(2);
							mxPop();
						}
						else {
							mxPop();
							fxCompleteModule(the, realm, module->value.reference, C_NULL);
							fxRunImportFulfilled(the, realm, module->value.reference);
						}
					}
					mxCatch(the) {
						mxPush(mxException);
						mxException = mxUndefined;
						fxCompleteModule(the, realm, module->value.reference, the->stack);
						fxRunImportRejected(the, realm, module->value.reference);
						mxPop();
					}
				}
				mxPushUndefined();
				mxPull(mxHosts);
			}
		}
		module = module->next;
	}
}

txID fxExecuteModulesFrom(txMachine* the, txSlot* realm, txSlot* queue, txSlot* module, txSlot** exception)
{
	txSlot* fromModules = mxModuleTransfers(module)->value.reference;
	txSlot* fromModule;
	if (mxModuleTransfers(module)->kind == XS_NULL_KIND)
		return XS_MODULE_STATUS_LINKED;
	fromModule = fromModules->next;
	while (fromModule) {
		if (mxModuleStatus(fromModule) == XS_MODULE_STATUS_ERROR) {
			*exception = mxModuleMeta(fromModule);
			return XS_MODULE_STATUS_ERROR;
		}
		fromModule = fromModule->next;
	}
	fromModule = fromModules->next;
	while (fromModule) {
		if (mxModuleStatus(fromModule) == XS_MODULE_STATUS_EXECUTING) {
			return XS_MODULE_STATUS_EXECUTING;
		}
		fromModule = fromModule->next;
	}
	fromModule = fromModules->next;
	while (fromModule) {
		if (mxModuleStatus(fromModule) == XS_MODULE_STATUS_LINKED) {
			txSlot* waitingModule = queue->next;
			while (waitingModule) {
				if (module->value.reference == waitingModule->value.reference)
					break;
				if (fromModule->value.reference == waitingModule->value.reference)
					return XS_MODULE_STATUS_EXECUTING;
				waitingModule = waitingModule->next;
			}
		}
		fromModule = fromModule->next;
	}
	return XS_MODULE_STATUS_LINKED;
}

void fxExecuteModulesFulfilled(txMachine* the)
{
	txSlot* home = mxFunctionInstanceHome(mxFunction->value.reference);
	txSlot* realm = home->value.home.object;
	txSlot* module = home->value.home.module;
	fxCompleteModule(the, realm, module, C_NULL);
	fxRunImportFulfilled(the, realm, module);
	fxExecuteModules(the, realm, mxOwnModules(realm)->value.reference);
}

void fxExecuteModulesRejected(txMachine* the)
{
	txSlot* home = mxFunctionInstanceHome(mxFunction->value.reference);
	txSlot* realm = home->value.home.object;
	txSlot* module = home->value.home.module;
	fxCompleteModule(the, realm, module,  mxArgv(0));
	fxRunImportRejected(the, realm, module);
	fxExecuteModules(the, realm, mxOwnModules(realm)->value.reference);
}

txSlot* fxGetModule(txMachine* the, txSlot* realm, txID moduleID)
{
	txSlot* result = mxBehaviorGetProperty(the, mxOwnModules(realm)->value.reference, moduleID, 0, XS_OWN);
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

void fxLinkCircularities(txMachine* the, txSlot* realm, txID moduleID, txSlot* circularities, txSlot* exports)
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
	if (mxBehaviorGetProperty(the, circularitiesInstance, moduleID, 0, XS_OWN))
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
				fxLinkCircularities(the, realm, from->value.symbol, circularitiesCopy, stars);
				star = stars->value.reference->next;
				while (star) {
					if (star->ID != mxID(_default)) {
                        txSlot* ambiguous = mxBehaviorGetProperty(the, exportsInstance, star->ID, 0, XS_OWN);
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
				mxPop();
				mxPop();
			}
			transfer = transfer->next;
		}
	}
}

void fxLinkExports(txMachine* the, txSlot* realm, txSlot* module)
{
	txSlot* exports;
	txSlot* transfer;
	txSlot* from;
	txSlot* import;
	txSlot* closure;

	exports = mxModuleExports(module)->value.reference;
	exports->flag |= XS_DONT_PATCH_FLAG;
	transfer = exports->next;
	while (transfer) {
		if (transfer->kind != XS_EXPORT_KIND) {
			from = mxTransferFrom(transfer);
			if (from->kind != XS_NULL_KIND) {
				import = mxTransferImport(transfer);
				if (import->kind != XS_NULL_KIND)
					fxLinkTransfer(the, realm, from->value.symbol, import->value.symbol, transfer);
				else
					fxLinkNamespace(the, realm, from->value.symbol, transfer);
			}
			closure = mxTransferClosure(transfer);
			transfer->kind = closure->kind;
			transfer->value = closure->value;
		}
		transfer->flag |= XS_DONT_DELETE_FLAG | XS_DONT_SET_FLAG;
		transfer = transfer->next;
	}
}

void fxLinkLocals(txMachine* the, txSlot* realm, txSlot* module)
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
					fxLinkTransfer(the, realm, from->value.symbol, import->value.symbol, transfer);
				else
					fxLinkNamespace(the, realm, from->value.symbol, transfer);
				closure = mxTransferClosure(transfer);
				closure->flag |= XS_DONT_SET_FLAG;
			}
		}
		transfer = transfer->next;
	}
}

void fxLinkModules(txMachine* the, txSlot* realm, txSlot* queue)
{
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
	
	module = queue->next;
	while (module) {
		if (mxModuleStatus(module) == XS_MODULE_STATUS_LINKING) {
			#if mxReport
				fxIDToString(the, module->ID, the->nameBuffer, sizeof(the->nameBuffer));
				fprintf(stderr, "# Linking module \"%s\"\n", the->nameBuffer);
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
		}
		module = module->next;
	}

	module = queue->next;
	while (module) {
		if (mxModuleStatus(module) == XS_MODULE_STATUS_LINKING) {
			fxNewInstance(the);
			circularities = the->stack;
			fxNewInstance(the);
			exports = the->stack;
			fxLinkCircularities(the, realm, module->ID, circularities, exports);
			mxModuleExports(module)->kind = XS_REFERENCE_KIND;
			mxModuleExports(module)->value.reference = exports->value.reference;
			mxPop();
			mxPop();
		}
		module = module->next;
	}

	module = queue->next;
	while (module) {
		if (mxModuleStatus(module) == XS_MODULE_STATUS_LINKING)
			fxLinkExports(the, realm, module);
		module = module->next;
	}
	module = queue->next;
	while (module) {
		if (mxModuleStatus(module) == XS_MODULE_STATUS_LINKING)
			fxLinkLocals(the, realm, module);
		module = module->next;
	}

	module = queue->next;
	while (module) {
		if (mxModuleStatus(module) == XS_MODULE_STATUS_LINKING) {
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
			mxPop();
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
		}
		module = module->next;
	}

	module = queue->next;
	while (module) {
		if (mxModuleStatus(module) == XS_MODULE_STATUS_LINKING) {
			mxModuleStatus(module) = XS_MODULE_STATUS_LINKED;
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
		}
		module = module->next;
	}
}

void fxLinkNamespace(txMachine* the, txSlot* realm, txID fromID, txSlot* transfer)
{
	txSlot* export = mxTransferClosure(transfer);
	mxCheck(the, export->kind == XS_EXPORT_KIND);
	export->value.export.closure = fxDuplicateSlot(the, fxGetModule(the, realm, fromID));
	export->value.export.closure->ID = XS_NO_ID;
}

void fxLinkTransfer(txMachine* the, txSlot* realm, txID fromID, txID importID, txSlot* transfer)
{
	txSlot* module;
	txSlot* export;
	txSlot* exportClosure;
	txSlot* transferClosure;
	txSlot* from;
	txSlot* import;
	
	module = fxGetModule(the, realm, fromID);
	export = mxBehaviorGetProperty(the, mxModuleExports(module)->value.reference, importID, 0, XS_OWN);
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
					fxLinkTransfer(the, realm, from->value.symbol, import->value.symbol, transfer);
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

void fxLoadModuleScript(txMachine* the, txID moduleID, txSlot* module, txScript* script)
{
	txSlot* slot;
	txSlot* key;

	mxPushReference(module);
	mxPushClosure(the->stack);
	fxRunScript(the, script, the->stack + 1, C_NULL, C_NULL, C_NULL, module);
//	module = the->stack->value.closure->value.reference; // ??
	mxPop();
	mxPop();
	
	slot = mxModuleInstanceExecute(module);	
	if (slot->kind == XS_NULL_KIND) {
		fxIDToString(the, moduleID, the->nameBuffer, sizeof(the->nameBuffer));
		mxReferenceError("\"%s\" is no module", the->nameBuffer);
	}

	slot = fxNewInstance(the);
	slot->flag |= XS_DONT_PATCH_FLAG;
	key = fxGetKey(the, moduleID);
	if (key) {
		slot = slot->next = fxNewSlot(the);
		slot->value.string = key->value.key.string;
		if (key->kind == XS_KEY_KIND)
			slot->kind = XS_STRING_KIND;
		else
			slot->kind = XS_STRING_X_KIND;
		slot->ID = mxID(_uri);
		slot->flag |= XS_DONT_DELETE_FLAG | XS_DONT_SET_FLAG;
	}
	slot = mxModuleInstanceMeta(module);	
	mxPullSlot(slot);
}

void fxLoadModules(txMachine* the, txSlot* realm, txSlot* queue)
{
#if mxReport
	{
		txSlot* module = queue->next;
		fprintf(stderr, "LOAD");
		while (module) {
			fprintf(stderr, " %s %d", mxModuleName(module->ID), mxModuleStatus(module));
			module = module->next;
		}
		fprintf(stderr, "\n");
	}
#endif
	txSlot* module = queue->next;
	txBoolean done = 1;
	while (module) {
		if (mxModuleStatus(module) == XS_MODULE_STATUS_NONE) {
			txSlot* key;
			txSlot* function;
			txSlot* home;
		#if mxReport
			fprintf(stderr, "# Loading module \"%s\"\n", mxModuleName(module->ID));
		#endif
			done = 0;
			mxModuleStatus(module) = XS_MODULE_STATUS_LOADING;
			mxPushUndefined();
			mxPush(mxGlobal);
			mxGetID(fxID(the, "loadModuleScript"));
			mxCall();
			key = fxGetKey(the, module->ID);
			if (key->kind == XS_KEY_X_KIND)
				mxPushStringX(key->value.key.string);
			else
				mxPushString(key->value.key.string);
			mxRunCount(1);
			mxDub();
			mxGetID(mxID(_then));
			mxCall();
		
			function = fxNewHostFunction(the, fxLoadModulesFulfilled, 1, XS_NO_ID);
			home = mxFunctionInstanceHome(function);
			home->value.home.object = realm;
			home->value.home.module = module->value.reference;

			function = fxNewHostFunction(the, fxLoadModulesRejected, 1, XS_NO_ID);
			home = mxFunctionInstanceHome(function);
			home->value.home.object = realm;
			home->value.home.module = module->value.reference;
	
			mxRunCount(2);
			mxPop();
		}
		else if (mxModuleStatus(module) == XS_MODULE_STATUS_LOADING) {
			done = 0;
		}
		else if (mxModuleStatus(module) == XS_MODULE_STATUS_LOADED) {
			if (done && (module->flag & XS_BASE_FLAG)) {
				module = queue->next;
				while (module) {
					if (mxModuleStatus(module) == XS_MODULE_STATUS_LOADED)
						mxModuleStatus(module) = XS_MODULE_STATUS_LINKING;
					if (module->flag & XS_BASE_FLAG)
						break;
					module = module->next;
				}
				module->flag &= ~XS_BASE_FLAG;
				mxTry(the) {
					fxLinkModules(the, realm, queue);
				}
				mxCatch(the) {
					mxPush(mxException);
					mxException = mxUndefined;
					module = queue->next;
					while (module) {
						if (mxModuleStatus(module) == XS_MODULE_STATUS_LINKING) {
							fxCompleteModule(the, realm, module->value.reference, the->stack);
							fxRunImportRejected(the, realm, module->value.reference);
						}
						module = module->next;
					}
					mxPop();
				}
				fxExecuteModules(the, realm, queue);
				module = queue;
			}
		}
		module = module->next;
	}
}

void fxLoadModulesFrom(txMachine* the, txSlot* realm, txSlot* queue, txSlot* module)
{
    txID moduleID = mxModuleInstanceInternal(module)->value.module.id;
    txSlot** address = &(queue->next);
  	txSlot* slot;
    while ((slot = *address)) {
    	if (slot->ID == moduleID)
    		break;
    	address = &(slot->next);
    }
    if (slot) {
		txSlot* transfer = mxModuleInstanceTransfers(module)->value.reference->next;
		while (transfer) {
			txSlot* from = mxTransferFrom(transfer);
			if (from->kind != XS_NULL_KIND) {
				txID importID = fxResolveSpecifier(the, realm, moduleID, from);
				if (!mxBehaviorGetProperty(the, queue, importID, 0, XS_OWN)) {
					slot = fxNewSlot(the);
					slot->next = *address;
					slot->ID = importID;
					*address = slot;
					address = &(slot->next);
					fxNewModule(the, realm, importID, slot);
				}
			}
			transfer = transfer->next;
		}
	}
}
void fxLoadModulesFulfilled(txMachine* the)
{
	txSlot* home = mxFunctionInstanceHome(mxFunction->value.reference);
	txSlot* realm = home->value.home.object;
	txSlot* queue = mxOwnModules(realm)->value.reference;
	txSlot* module = home->value.home.module;
	txID moduleID = mxModuleInstanceInternal(module)->value.module.id;
	mxTry(the) {
		txScript* script = fxGetHostData(the, mxArgv(0));
		fxSetHostData(the, mxArgv(0), C_NULL);
		fxLoadModuleScript(the, moduleID, module, script);
		mxModuleInstanceStatus(module) = XS_MODULE_STATUS_LOADED;
		fxLoadModulesFrom(the, realm, queue, module);
	}
	mxCatch(the) {
		mxPush(mxException);
		mxException = mxUndefined;
		fxCompleteModule(the, realm, module, the->stack);
		fxRunImportRejected(the, realm, module);
		mxPop();
	}
	fxLoadModules(the, realm, queue);
}

void fxLoadModulesRejected(txMachine* the)
{
	txSlot* home = mxFunctionInstanceHome(mxFunction->value.reference);
	txSlot* realm = home->value.home.object;
	txSlot* queue = mxOwnModules(realm)->value.reference;
	txSlot* module = home->value.home.module;
	fxCompleteModule(the, realm, module, mxArgv(0));
	fxRunImportRejected(the, realm, module);
	fxLoadModules(the, realm, queue);
}

void fxNewModule(txMachine* the, txSlot* realm, txID moduleID, txSlot* module)
{
	txSlot* slot;
	
	mxPush(mxModulePrototype);
	slot = fxNewObjectInstance(the);
	slot->flag |= XS_EXOTIC_FLAG | XS_DONT_PATCH_FLAG;
	/* HOST */
	slot = slot->next = fxNewSlot(the);
	slot->ID = XS_MODULE_BEHAVIOR;
	slot->flag = XS_INTERNAL_FLAG;
	slot->kind = XS_MODULE_KIND;
	slot->value.module.realm = realm;
	slot->value.module.id = moduleID;
	/* EXPORTS */
	slot = fxNextNullProperty(the, slot, XS_NO_ID, XS_INTERNAL_FLAG);
	/* META */
	slot = fxNextNullProperty(the, slot, XS_NO_ID, XS_INTERNAL_FLAG);
	/* TRANSFERS */
	slot = fxNextNullProperty(the, slot, XS_NO_ID, XS_INTERNAL_FLAG);
	/* INITIALIZE */
	slot = fxNextNullProperty(the, slot, XS_NO_ID, XS_INTERNAL_FLAG);
	/* FUNCTION */
	slot = fxNextNullProperty(the, slot, XS_NO_ID, XS_INTERNAL_FLAG);
	/* HOSTS */
	slot = fxNextNullProperty(the, slot, XS_NO_ID, XS_INTERNAL_FLAG);
	module->kind = the->stack->kind;
	module->value = the->stack->value;
	mxPop();
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

txID fxResolveSpecifier(txMachine* the, txSlot* realm, txID moduleID, txSlot* name)
{
	moduleID = fxFindModule(the, realm, moduleID, name); // ??
	if (moduleID == XS_NO_ID) {
		fxToStringBuffer(the, name, the->nameBuffer, sizeof(the->nameBuffer));
		mxReferenceError("module \"%s\" not found", the->nameBuffer);
	}
	name->kind = XS_SYMBOL_KIND;
	name->value.symbol = moduleID;
	return moduleID;
}

void fxRunImport(txMachine* the, txSlot* realm, txID moduleID)
{
	txSlot* stack = the->stack;
	txSlot* queue = mxOwnModules(realm)->value.reference;
	txSlot* promise;
	txSlot* fulfillFunction;
	txSlot* rejectFunction;
	txSlot* module;
	txID status;
	txSlot* slot;
	
	fxBeginHost(the);
	mxPush(mxPromisePrototype);
	promise = fxNewPromiseInstance(the);
	mxPromiseStatus(promise)->value.integer = mxPendingStatus;
	fxPushPromiseFunctions(the, promise);
	fulfillFunction = the->stack + 1;
	rejectFunction = the->stack;
	{
		mxTry(the) {
			fxToString(the, stack);
			moduleID = fxResolveSpecifier(the, realm, moduleID, stack);
			module = fxGetModule(the, realm, moduleID);
			if (module) {
				status = mxModuleStatus(module);
				if (status == XS_MODULE_STATUS_ERROR) {
					/* THIS */
					mxPushUndefined();
					/* FUNCTION */
					mxPushSlot(rejectFunction);
					mxCall();
					/* ARGUMENTS */
					mxPushSlot(mxModuleMeta(module));
					mxRunCount(1);
				}
				else if (status == XS_MODULE_STATUS_EXECUTED) {
					/* THIS */
					mxPushUndefined();
					/* FUNCTION */
					mxPushSlot(fulfillFunction);
					mxCall();
					/* ARGUMENTS */
					mxPushSlot(module);
					mxRunCount(1);
				}
				else {
					slot = fxLastProperty(the, module->value.reference);
					slot = fxNextSlotProperty(the, slot, fulfillFunction, XS_NO_ID, XS_NO_FLAG);
					slot = fxNextSlotProperty(the, slot, rejectFunction, XS_NO_ID, XS_NO_FLAG);
				}
			}
			else {
				module = mxBehaviorSetProperty(the, queue, moduleID, 0, XS_OWN);
				fxNewModule(the, realm, moduleID, module);
				slot = fxLastProperty(the, module->value.reference);
				slot = fxNextSlotProperty(the, slot, fulfillFunction, XS_NO_ID, XS_NO_FLAG);
				slot = fxNextSlotProperty(the, slot, rejectFunction, XS_NO_ID, XS_NO_FLAG);
				module->flag |= XS_BASE_FLAG;
				fxLoadModules(the, realm, queue);
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

void fxRunImportFulfilled(txMachine* the, txSlot* realm, txSlot* module)
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
	fprintf(stderr, "# Fullfilled module \"%s\"\n", mxModuleName(mxModuleInstanceInternal(module)->value.module.id));
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

void fxRunImportRejected(txMachine* the, txSlot* realm, txSlot* module)
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
	fprintf(stderr, "# Rejected module \"%s\"\n", mxModuleName(mxModuleInstanceInternal(module)->value.module.id));
#endif
	exception = mxModuleInstanceMeta(module);
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

/* NOW */

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
			fxRunImportNow(the, realm, XS_NO_ID);
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

void fxResolveModule(txMachine* the, txSlot* realm, txID moduleID, txScript* script, void* data, txDestructor destructor)
{
	txSlot* module = fxGetModule(the, realm, moduleID);
	module = module->value.reference;
	fxLoadModuleScript(the, moduleID, module, script);
	mxModuleInstanceStatus(module) = XS_MODULE_STATUS_LOADED;
	fxLoadModulesFrom(the, realm, mxOwnModules(realm)->value.reference, module);
}

void fxRunImportNow(txMachine* the, txSlot* realm, txID id)
{
	txSlot* stack = the->stack;
	txSlot* queue = mxOwnModules(realm)->value.reference;
	txID moduleID;
	txSlot* module;
	mxTry(the) {
		fxToString(the, stack);
		moduleID = fxResolveSpecifier(the, realm, moduleID, stack);
		module = fxGetModule(the, realm, moduleID);
		if (module) {
			if (mxModuleStatus(module) == XS_MODULE_STATUS_ERROR) {
				mxPushSlot(mxModuleMeta(module));
				mxPull(mxException);
				fxJump(the);
			}
			stack->kind = module->kind;
			stack->value = module->value;
		}
		else {
			module = mxBehaviorSetProperty(the, queue, moduleID, 0, XS_OWN);
			fxNewModule(the, realm, moduleID, module);
			stack->kind = module->kind;
			stack->value = module->value;
			module = queue->next;
			while (module) {
				if (mxModuleStatus(module) == XS_MODULE_STATUS_NONE) {
					mxModuleStatus(module) = XS_MODULE_STATUS_LOADING;
					fxLoadModule(the, realm, module->ID);
					module = queue->next;
				}
				else
					module = module->next;
			}
			module = queue->next;
			while (module) {
				if (mxModuleStatus(module) == XS_MODULE_STATUS_LOADED)
					mxModuleStatus(module) = XS_MODULE_STATUS_LINKING;
				module = module->next;
			}
			fxLinkModules(the, realm, queue);
			module = queue->next;
			while (module) {
				if (mxModuleStatus(module) == XS_MODULE_STATUS_LINKED) {
					mxModuleStatus(module) = XS_MODULE_STATUS_EXECUTING;
					mxPushUndefined();
					mxPushSlot(mxModuleExecute(module));
					mxCall();
					mxRunCount(0);
					mxPop();
					fxCompleteModule(the, realm, module->value.reference, C_NULL);
					mxModuleMeta(module)->next = C_NULL;
				}
				module = module->next;
			}
		}
	}
	mxCatch(the) {
		module = queue->next;
		while (module) {
			if (mxModuleStatus(module) < XS_MODULE_STATUS_EXECUTED) {
				fxCompleteModule(the, realm, module->value.reference, &mxException);
				mxModuleMeta(module)->next = C_NULL;
			}
			module = module->next;
		}
		fxJump(the);
	}
	the->stack = stack;
}

/* BEHAVIOR */

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
		slot->flag = (id == mxID(_Symbol_toStringTag)) ? property->flag : XS_DONT_DELETE_FLAG;
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
				keys = fxQueueKey(the, the->stack->ID, 0, keys);
				mxPop();
			}	
		}
		if (flag & XS_EACH_SYMBOL_FLAG) {
			txSlot* property = mxModulePrototype.value.reference->next;
			keys = fxQueueKey(the, property->ID, 0, keys);
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

/* COMPARTMENT */

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
	txInteger id;
	
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
		
		if (the->sharedMachine == C_NULL) {
			mxPush(mxObjectPrototype);
	#ifdef mxLink
			global = fxNewObjectInstance(the);
	#else
			global = fxNewGlobalInstance(the);
	#endif
			slot = fxLastProperty(the, global);
			if (mxCompartmentGlobal.kind == XS_UNDEFINED_KIND) {
				for (id = XS_SYMBOL_ID_COUNT; id < _Infinity; id++)
					slot = fxNextSlotProperty(the, slot, &the->stackPrototypes[-1 - id], mxID(id), XS_DONT_ENUM_FLAG);
				for (; id < _Compartment; id++)
					slot = fxNextSlotProperty(the, slot, &the->stackPrototypes[-1 - id], mxID(id), XS_GET_ONLY);
			}
			else {
				txSlot* item = mxCompartmentGlobal.value.reference->next->value.array.address;
				for (id = XS_SYMBOL_ID_COUNT; id < _Infinity; id++)
					slot = fxNextSlotProperty(the, slot, item + id, mxID(id), XS_DONT_ENUM_FLAG);
				for (; id < _Compartment; id++)
					slot = fxNextSlotProperty(the, slot, item + id, mxID(id), XS_GET_ONLY);
			}
		}
		else {
			mxPush(mxCompartmentGlobal);
			global = fxNewObjectInstance(the);
			slot = fxLastProperty(the, global);
			id = _Compartment;
		}
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
					mxGetAll(at->value.at.id, at->value.at.index);
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
		stream.size = mxStringLength(fxToString(the, mxArgv(0)));
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
	fxRunImportNow(the, realm, XS_NO_ID);
	mxPullSlot(mxResult);
}

