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

static void fxCompleteModule(txMachine* the, txSlot* module, txSlot* exception);
static void fxCopyModuleMeta(txMachine* the, txSlot* srcModule, txSlot* dstModule);
static void fxDuplicateModuleTransfers(txMachine* the, txSlot* srcModule, txSlot* dstModule);

static void fxExecuteModules(txMachine* the, txSlot* queue);
static txID fxExecuteModulesFrom(txMachine* the, txSlot* queue, txSlot* module, txSlot** exception);

static txSlot* fxGetModule(txMachine* the, txSlot* realm, txID moduleID);

static void fxLinkCircularities(txMachine* the, txSlot* module, txSlot* circularities, txSlot* exports);
static void fxLinkExports(txMachine* the, txSlot* module);
static void fxLinkLocals(txMachine* the, txSlot* module);
static void fxLinkModules(txMachine* the, txSlot* queue);
static void fxLinkNamespace(txMachine* the, txSlot* module, txSlot* transfer);
static void fxLinkTransfer(txMachine* the, txSlot* module, txID importID, txSlot* transfer);

static void fxLoadModules(txMachine* the, txSlot* queue);
static void fxLoadModulesFrom(txMachine* the, txSlot* queue, txSlot* module, txBoolean now);

static void fxMapModule(txMachine* the, txSlot* realm, txID moduleID, txSlot* module);
static void fxNewModule(txMachine* the, txSlot* realm, txID moduleID, txSlot* module);
static void fxOrderModule(txMachine* the, txSlot* queue, txSlot* order, txSlot* module);
static txBoolean fxQueueModule(txMachine* the, txSlot* queue, txSlot* module);

static txID fxResolveSpecifier(txMachine* the, txSlot* realm, txID moduleID, txSlot* name);

static void fxRunImportFulfilled(txMachine* the, txSlot* module);
static void fxRunImportRejected(txMachine* the, txSlot* module);

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
// static void fxObjectToModule(txMachine* the, txSlot* realm, txID id);

static txSlot* fxCheckStaticModuleRecordInstance(txMachine* the, txSlot* slot);
static txSlot* fxNewStaticModuleRecordInstance(txMachine* the);

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
	XS_MODULE_STATUS_NEW = 0,
	XS_MODULE_STATUS_LOADING,
	XS_MODULE_STATUS_LOADED,
	XS_MODULE_STATUS_LINKING,
	XS_MODULE_STATUS_LINKED,
	XS_MODULE_STATUS_EXECUTING,
	XS_MODULE_STATUS_EXECUTED,
	XS_MODULE_STATUS_ERROR,
};


#define mxIsModule(THE_SLOT) \
	(((THE_SLOT)->next) && ((THE_SLOT)->next->flag & XS_INTERNAL_FLAG) && ((THE_SLOT)->next->kind == XS_MODULE_KIND))
#define mxIsStaticModuleRecord(THE_SLOT) \
	(((THE_SLOT)->next) && ((THE_SLOT)->next->flag & XS_INTERNAL_FLAG) && ((THE_SLOT)->next->kind == XS_STATIC_MODULE_RECORD_KIND))

#define mxModuleInstanceStatus(MODULE)		((MODULE)->next->next->ID)
#define mxModuleStatus(MODULE) 				mxModuleInstanceStatus((MODULE)->value.reference)

#if mxReport
#define mxModuleName(ID) c_strrchr(fxGetKeyName(the, (ID)), '/')
#define mxReportModuleQueue(LABEL) fxReportModuleQueue(the, queue, LABEL)
static void fxReportModuleQueue(txMachine* the, txSlot* queue, txString label)
{
	txSlot* module = queue->next;
	fprintf(stderr, "%s", label);
	while (module) {
		fprintf(stderr, " %s %d", fxGetKeyName(the, module->ID), mxModuleStatus(module));
		module = module->next;
	}
	fprintf(stderr, "\n");
}
#else
#define mxReportModuleQueue(LABEL)
#endif	

static void fxPushKeyString(txMachine* the, txID id)
{
	txSlot* key = fxGetKey(the, id);
	if (key->kind == XS_KEY_X_KIND)
		mxPushStringX(key->value.key.string);
	else
		mxPushString(key->value.key.string);
}

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
	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Compartment_prototype_module), 1, mxID(_module), XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "Compartment", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxCompartmentPrototype = *the->stack;
	slot = fxBuildHostConstructor(the, mxCallback(fx_Compartment), 1, mxID(_Compartment));
	mxCompartmentConstructor = *the->stack;
    mxPop();
    
	mxPush(mxObjectPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostAccessorProperty(the, slot, mxCallback(fx_StaticModuleRecord_prototype_get_bindings), C_NULL, mxID(_bindings), XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "StaticModuleRecord", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxStaticModuleRecordPrototype = *the->stack;
	slot = fxBuildHostConstructor(the, mxCallback(fx_StaticModuleRecord), 1, mxID(_StaticModuleRecord));
	mxStaticModuleRecordConstructor = *the->stack;
    mxPop();
}

void fxCompleteModule(txMachine* the, txSlot* module, txSlot* exception)
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

void fxCopyModuleMeta(txMachine* the, txSlot* srcModule, txSlot* dstModule)
{
	txSlot* srcSlot;
	txSlot* dstSlot;
	srcSlot = mxModuleInternal(srcModule);
	dstSlot = mxModuleInternal(dstModule);
	if (srcSlot->value.module.id != XS_NO_ID)
		dstSlot->value.module.id = srcSlot->value.module.id;
	srcSlot = mxModuleMeta(srcModule);
	dstSlot = mxModuleMeta(dstModule);
	fxDuplicateInstance(the, srcSlot->value.reference);
	mxPullSlot(dstSlot);
}

void fxDuplicateModuleTransfers(txMachine* the, txSlot* srcModule, txSlot* dstModule)
{
	txSlot* srcSlot;
	txSlot* dstSlot;
	txSlot* transfer;
	txSlot* aliases;
	txSlot* function;
	srcSlot = mxModuleTransfers(srcModule);
	dstSlot = mxModuleTransfers(dstModule);
	fxDuplicateInstance(the, srcSlot->value.reference);
	mxPullSlot(dstSlot);
	transfer = dstSlot->value.reference->next;
	while (transfer) {
		fxDuplicateInstance(the, transfer->value.reference);
		mxPullSlot(transfer);
		aliases = mxTransferAliases(transfer);
		if (aliases->kind == XS_REFERENCE_KIND) {
			fxDuplicateInstance(the, aliases->value.reference);
			mxPullSlot(aliases);
		}		
		transfer = transfer->next;
	}
	srcSlot = mxModuleInitialize(srcModule);
	dstSlot = mxModuleInitialize(dstModule);
	if (srcSlot->kind == XS_REFERENCE_KIND) {
		function = fxDuplicateInstance(the, srcSlot->value.reference);
		mxPullSlot(dstSlot);
		mxFunctionInstanceHome(function)->value.home.module = 	dstModule->value.reference;
	}
	else {
		dstSlot->kind = srcSlot->kind;
		dstSlot->value = srcSlot->value;
	}
	srcSlot = mxModuleExecute(srcModule);
	dstSlot = mxModuleExecute(dstModule);
	function = fxDuplicateInstance(the, srcSlot->value.reference);
	mxPullSlot(dstSlot);
	mxFunctionInstanceHome(function)->value.home.module = 	dstModule->value.reference;
}

void fxExecuteModules(txMachine* the, txSlot* queue)
{
	txBoolean done = 1;
	txSlot* module = queue->next;
	mxReportModuleQueue("INIT");
	while (module) {
		txID status = mxModuleStatus(module);
		if (status == XS_MODULE_STATUS_LINKED) {
			txSlot* exception;
			txID fromStatus = fxExecuteModulesFrom(the, queue, module, &exception);
			if (fromStatus == XS_MODULE_STATUS_ERROR) {
				fxCompleteModule(the, module->value.reference, exception);
				fxRunImportRejected(the, module->value.reference);
			}
			else if (fromStatus == XS_MODULE_STATUS_LINKED) {
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
							home->value.home.object = queue;
							home->value.home.module = module->value.reference;
	
							function = fxNewHostFunction(the, fxExecuteModulesRejected, 1, XS_NO_ID);
							home = mxFunctionInstanceHome(function);
							home->value.home.object = queue;
							home->value.home.module = module->value.reference;
				
							mxRunCount(2);
							mxPop();
							done = 0;
						}
						else {
							mxPop();
							fxCompleteModule(the, module->value.reference, C_NULL);
							fxRunImportFulfilled(the, module->value.reference);
						}
					}
					mxCatch(the) {
						mxPush(mxException);
						mxException = mxUndefined;
						fxCompleteModule(the, module->value.reference, the->stack);
						fxRunImportRejected(the, module->value.reference);
						mxPop();
					}
				}
				mxPushUndefined();
				mxPull(mxHosts);
			}
			else
				done = 0;
		}
		else if (status < XS_MODULE_STATUS_EXECUTED)
			done = 0;
		module = module->next;
	}
	if (done) {
		mxReportModuleQueue("INIT");
		queue->next = C_NULL;
	}
}

txID fxExecuteModulesFrom(txMachine* the, txSlot* queue, txSlot* module, txSlot** exception)
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
	txSlot* queue = home->value.home.object;
	txSlot* module = home->value.home.module;
	fxCompleteModule(the, module, C_NULL);
	fxRunImportFulfilled(the, module);
	fxExecuteModules(the, queue);
}

void fxExecuteModulesRejected(txMachine* the)
{
	txSlot* home = mxFunctionInstanceHome(mxFunction->value.reference);
	txSlot* queue = home->value.home.object;
	txSlot* module = home->value.home.module;
	fxCompleteModule(the, module,  mxArgv(0));
	fxRunImportRejected(the, module);
	fxExecuteModules(the, queue);
}

txSlot* fxGetModule(txMachine* the, txSlot* realm, txID moduleID)
{
	txSlot* result = mxBehaviorGetProperty(the, mxOwnModules(realm)->value.reference, moduleID, 0, XS_OWN);
	return result;
}

void fxLinkCircularities(txMachine* the, txSlot* module, txSlot* circularities, txSlot* exports)
{
	txSlot* circularitiesInstance;
	txSlot* circularity;
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
	circularity = circularitiesInstance->next;
	while (circularity) {
		if (circularity->value.reference == module->value.reference)
			return;
		circularity = circularity->next;
	}
	circularity = fxDuplicateSlot(the, module);
	circularity->next = circularitiesInstance->next;
	circularitiesInstance->next = circularity;
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
					circularityCopy = circularityCopy->next = fxDuplicateSlot(the, circularity);
					circularity = circularity->next;
				}
				fxNewInstance(the);
				stars = the->stack;
				fxLinkCircularities(the, from, circularitiesCopy, stars);
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

void fxLinkExports(txMachine* the, txSlot* module)
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
					fxLinkTransfer(the, from, import->value.symbol, transfer);
				else
					fxLinkNamespace(the, from, transfer);
			}
			closure = mxTransferClosure(transfer);
			transfer->kind = closure->kind;
			transfer->value = closure->value;
		}
		transfer->flag |= XS_DONT_DELETE_FLAG | XS_DONT_SET_FLAG;
		transfer = transfer->next;
	}
}

void fxLinkLocals(txMachine* the, txSlot* module)
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
					fxLinkTransfer(the, from, import->value.symbol, transfer);
				else
					fxLinkNamespace(the, from, transfer);
				closure = mxTransferClosure(transfer);
				closure->flag |= XS_DONT_SET_FLAG;
			}
		}
		transfer = transfer->next;
	}
}

void fxLinkModules(txMachine* the, txSlot* queue)
{
	txSlot* module;
	txSlot* order;
	txSlot* transfer;
	txSlot* local;
	txSlot* from;
	txSlot* import;
	txSlot* closure;
	txSlot* property;
	txSlot* circularities;
	txSlot* exports;
	txSlot* realm;
	txSlot* closures;
	txSlot* export;
	
	order = fxNewInstance(the);
	while ((module = queue->next))
		fxOrderModule(the, queue, order, module);
	queue->next = order->next;
	order->next = C_NULL;
	mxPop();

	mxReportModuleQueue("LINK");
	module = queue->next;
	while (module) {
		if (mxModuleStatus(module) == XS_MODULE_STATUS_LINKING) {
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
			fxLinkCircularities(the, module, circularities, exports);
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
			fxLinkExports(the, module);
		module = module->next;
	}
	module = queue->next;
	while (module) {
		if (mxModuleStatus(module) == XS_MODULE_STATUS_LINKING)
			fxLinkLocals(the, module);
		module = module->next;
	}

	module = queue->next;
	while (module) {
		if (mxModuleStatus(module) == XS_MODULE_STATUS_LINKING) {
			txInteger count = 0;
			mxPushUndefined();
			realm = mxModuleInternal(module)->value.module.realm;
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
							if (slot->value.reference == from->value.reference)
								break;
							address = &slot->next;
						}
						if (!slot)
							slot = *address = fxDuplicateSlot(the, from);
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

void fxLinkNamespace(txMachine* the, txSlot* module, txSlot* transfer)
{
	txSlot* export = mxTransferClosure(transfer);
	mxCheck(the, export->kind == XS_EXPORT_KIND);
	export->value.export.closure = fxDuplicateSlot(the, module);
	export->value.export.closure->ID = XS_NO_ID;
}

void fxLinkTransfer(txMachine* the, txSlot* module, txID importID, txSlot* transfer)
{
	txSlot* export;
	txSlot* exportClosure;
	txSlot* transferClosure;
	txSlot* from;
	txSlot* import;
	
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
				if ((from->value.reference != mxTransferFrom(transfer)->value.reference) || (import->value.symbol != mxTransferImport(transfer)->value.symbol))
					fxLinkTransfer(the, from, import->value.symbol, transfer);
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

void fxLoadModules(txMachine* the, txSlot* queue)
{
	txSlot* module = queue->next;
	txBoolean done = 1;
	mxReportModuleQueue("LOAD");
	while (module) {
		if (mxModuleStatus(module) == XS_MODULE_STATUS_NEW) {
			txSlot* meta = mxModuleMeta(module);
			txSlot* internal = meta->value.reference->next;
			txID moduleID = internal->value.module.id;
			txSlot* realm = internal->value.module.realm;
			txSlot* loadHook = mxLoadHook(realm);
			mxTry(the) {
				mxModuleStatus(module) = XS_MODULE_STATUS_LOADING;
				if (mxIsUndefined(loadHook)) {
					if (realm != mxModuleInstanceInternal(mxProgram.value.reference)->value.module.realm)
						mxTypeError("no loadHook");
					fxLoadModule(the, module, moduleID);
					if (mxModuleExecute(module)->kind == XS_NULL_KIND)
						mxTypeError("no module");
					mxModuleStatus(module) = XS_MODULE_STATUS_LOADED;
					module = queue;
					done = 1;
					mxReportModuleQueue("LOAD");
				}
				else {
					txSlot* function;
					txSlot* home;
					done = 0;
					mxModuleStatus(module) = XS_MODULE_STATUS_LOADING;
					mxPushUndefined();
					mxPushSlot(loadHook);
					mxCall();
					fxPushKeyString(the, moduleID);
					mxRunCount(1);
					mxDub();
					mxGetID(mxID(_then));
					mxCall();
		
					function = fxNewHostFunction(the, fxLoadModulesFulfilled, 1, XS_NO_ID);
					home = mxFunctionInstanceHome(function);
					home->value.home.object = queue;
					home->value.home.module = module->value.reference;

					function = fxNewHostFunction(the, fxLoadModulesRejected, 1, XS_NO_ID);
					home = mxFunctionInstanceHome(function);
					home->value.home.object = queue;
					home->value.home.module = module->value.reference;
	
					mxRunCount(2);
					mxPop();
				}
			}
			mxCatch(the) {
				mxPush(mxException);
				mxException = mxUndefined;
				fxCompleteModule(the, module->value.reference, the->stack);
				fxRunImportRejected(the, module->value.reference);
			}
		}
		else if (mxModuleStatus(module) == XS_MODULE_STATUS_LOADING) {
			done = 0;
		}
		else if (mxModuleStatus(module) == XS_MODULE_STATUS_LOADED) {
			mxTry(the) {
				fxLoadModulesFrom(the, queue, module->value.reference, 0);
				mxModuleStatus(module) = XS_MODULE_STATUS_LINKING;
				module = queue;
				done = 1;
			}
			mxCatch(the) {
				mxPush(mxException);
				mxException = mxUndefined;
				fxCompleteModule(the, module->value.reference, the->stack);
				fxRunImportRejected(the, module->value.reference);
			}
		}
		module = module->next;
	}
	if (done) {
		mxTry(the) {
			fxLinkModules(the, queue);
		}
		mxCatch(the) {
			mxPush(mxException);
			mxException = mxUndefined;
			module = queue->next;
			while (module) {
				if (mxModuleStatus(module) == XS_MODULE_STATUS_LINKING) {
					fxCompleteModule(the, module->value.reference, the->stack);
					fxRunImportRejected(the, module->value.reference);
				}
				module = module->next;
			}
			mxPop();
		}
		fxExecuteModules(the, queue);
	}
}

void fxLoadModulesFrom(txMachine* the, txSlot* queue, txSlot* module, txBoolean now)
{
	txSlot* internal = mxModuleInstanceInternal(module);
	txID moduleID = internal->value.module.id;
	txSlot* realm = internal->value.module.realm;
	txSlot* transfer = mxModuleInstanceTransfers(module)->value.reference->next;
	while (transfer) {
		txSlot* from = mxTransferFrom(transfer);
		if (from->kind != XS_NULL_KIND) {
			txID importModuleID = fxResolveSpecifier(the, realm, moduleID, from);
			txSlot* importModule = fxGetModule(the, realm, importModuleID);
			txID status;
			if (!importModule) {
				importModule = mxBehaviorSetProperty(the, mxOwnModules(realm)->value.reference, importModuleID, 0, XS_OWN);
				fxMapModule(the, realm, importModuleID, importModule);
			}
			from->kind = XS_REFERENCE_KIND;
			from->value.reference = importModule->value.reference;
			status = mxModuleStatus(importModule);
			if ((status == XS_MODULE_STATUS_NEW) || (status == XS_MODULE_STATUS_LOADED)) {
				fxQueueModule(the, queue, importModule);
			}
			else if (now) {
				if (status == XS_MODULE_STATUS_LINKING) {
					txSlot* slot = queue->next;
					while (slot) {
						if (slot->value.reference == importModule->value.reference)
							break;
						slot= slot->next;
					}
					if (!slot)
						mxTypeError("async module");
				}
				else if (status == XS_MODULE_STATUS_ERROR) {
					mxPushSlot(mxModuleMeta(module));
					mxPull(mxException);
					fxJump(the);
				}
				else if (status != XS_MODULE_STATUS_EXECUTED) {
					mxTypeError("async module");
				}
			}
		}
		transfer = transfer->next;
	}
}

void fxLoadModulesFulfilled(txMachine* the)
{
	txSlot* home = mxFunctionInstanceHome(mxFunction->value.reference);
	txSlot* queue = home->value.home.object;
	txSlot* module = home->value.home.module;
	mxTry(the) {
		txSlot* record = mxArgv(0);
		if (!mxIsReference(record))
			mxTypeError("no module");
		if (!mxIsStaticModuleRecord(record->value.reference)) {
			mxPush(mxStaticModuleRecordConstructor);
			mxNew();
			mxPushSlot(record);
			mxRunCount(1);
			mxPullSlot(record);
		}
		mxPushReference(module);
		fxDuplicateModuleTransfers(the, record, the->stack);
		fxCopyModuleMeta(the, record, the->stack);
		mxPop();
		mxModuleInstanceStatus(module) = XS_MODULE_STATUS_LOADED;
	}
	mxCatch(the) {
		mxPush(mxException);
		mxException = mxUndefined;
		fxCompleteModule(the, module, the->stack);
		fxRunImportRejected(the, module);
		mxPop();
	}
	fxLoadModules(the, queue);
}

void fxLoadModulesRejected(txMachine* the)
{
	txSlot* home = mxFunctionInstanceHome(mxFunction->value.reference);
	txSlot* queue = home->value.home.object;
	txSlot* module = home->value.home.module;
	fxCompleteModule(the, module, mxArgv(0));
	fxRunImportRejected(the, module);
	fxLoadModules(the, queue);
}

void fxMapModule(txMachine* the, txSlot* realm, txID moduleID, txSlot* module)
{
	txSlot* moduleMap = mxModuleMap(realm);
	mxPushSlot(moduleMap);
	mxGetID(moduleID);
	if (mxIsUndefined(the->stack)) {
		txSlot* moduleMapHook = mxModuleMapHook(realm);
		if (!mxIsUndefined(moduleMapHook)) {
			mxPushSlot(moduleMapHook);
			mxCall();
			fxPushKeyString(the, moduleID);
			mxRunCount(1);
		}
	}
	if (mxIsUndefined(the->stack)) {
		txSlot* meta;
		txSlot* loader;
		fxNewModule(the, realm, moduleID, module);
		meta = mxModuleMeta(module)->value.reference;
		loader = fxNewSlot(the);
		loader->next = meta->next;
		loader->flag = XS_INTERNAL_FLAG;
		loader->kind = XS_MODULE_KIND;
		loader->value.module.realm = realm;
		loader->value.module.id = moduleID;
		meta->next = loader;
	}
	else if (mxIsReference(the->stack)) {
		if (mxIsModule(the->stack->value.reference)) {
			module->kind = the->stack->kind;
			module->value = the->stack->value;
		}
		else if (mxIsStaticModuleRecord(the->stack->value.reference)) {
			fxNewModule(the, realm, moduleID, module);
			fxDuplicateModuleTransfers(the, the->stack, module);
			fxCopyModuleMeta(the, the->stack, module);
			mxModuleStatus(module) = XS_MODULE_STATUS_LOADED;
		}
		else {
			txSlot* descriptor = the->stack;
			mxPush(mxStaticModuleRecordConstructor);
			mxNew();
			mxPushSlot(descriptor);
			mxRunCount(1);
			mxPullSlot(descriptor);
			fxNewModule(the, realm, moduleID, module);
			fxDuplicateModuleTransfers(the, the->stack, module);
			fxCopyModuleMeta(the, the->stack, module);
			mxModuleStatus(module) = XS_MODULE_STATUS_LOADED;
		}
	}
	else if (mxIsStringPrimitive(the->stack)) {
		txSlot* parent = mxRealmParent(realm);
		if (mxIsReference(parent)) {
			txSlot* meta;
			txSlot* loader;
			txSlot* loaderRealm = parent->value.reference;
			txSlot* loaderSlot = the->stack;
			fxResolveSpecifier(the, loaderRealm, XS_NO_ID, loaderSlot);
			fxNewModule(the, realm, moduleID, module);
			meta = mxModuleMeta(module)->value.reference;
			loader = fxNewSlot(the);
			loader->next = meta->next;
			loader->flag = XS_INTERNAL_FLAG;
			loader->kind = XS_MODULE_KIND;
			loader->value.module.realm = loaderRealm;
			loader->value.module.id = loaderSlot->value.symbol;
			meta->next = loader;
		}
		else
			mxTypeError("no compartment");
	}	
	else
		mxTypeError("no specifier");
}

void fxNewModule(txMachine* the, txSlot* realm, txID moduleID, txSlot* module)
{
	txSlot* slot;
	txSlot* meta;
	
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
	meta = fxNewInstance(the);
	meta->flag |= XS_DONT_PATCH_FLAG;
	slot = fxNextReferenceProperty(the, slot, meta, XS_NO_ID, XS_INTERNAL_FLAG);
	mxPop();
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

void fxOrderModule(txMachine* the, txSlot* queue, txSlot* order, txSlot* module)
{
	txSlot** fromAddress = &(queue->next);
	txSlot** toAddress = &(order->next);
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
	if (mxModuleStatus(module) == XS_MODULE_STATUS_LINKING) {
		txSlot* transfer = mxModuleTransfers(module)->value.reference->next;
		while (transfer) {
			from = mxTransferFrom(transfer);
			if (from->kind != XS_NULL_KIND) {
				to = queue->next;
				while (to) {
					if (to->value.reference == from->value.reference)
						break;
					to = to->next;
				}
				if (to)
					fxOrderModule(the, queue, order, to);
			}
			transfer = transfer->next;
		}
	}
	while ((to = *toAddress)) {
		if (to->value.reference == module->value.reference)
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

txBoolean fxQueueModule(txMachine* the, txSlot* queue, txSlot* module)
{
	txSlot** address = &(queue->next);
	txSlot* slot;
	while ((slot = *address)) {
		if (slot->value.reference == module->value.reference)
			return 0;
		address = &(slot->next);
	}
	slot = *address = fxNewSlot(the);
	slot->ID = mxModuleInternal(module)->value.module.id; //??
	slot->kind = XS_REFERENCE_KIND;
	slot->value.reference = module->value.reference;
	return 1;
}

void fxResolveModule(txMachine* the, txSlot* module, txID moduleID, txScript* script, void* data, txDestructor destructor)
{
	if (script->codeBuffer) {
		txSlot* key = fxGetKey(the, moduleID);
		if (key) {
			txSlot* meta = mxModuleMeta(module);
			txSlot* slot = fxLastProperty(the, meta->value.reference);
			slot = slot->next = fxNewSlot(the);
			slot->value.string = key->value.key.string;
			if (key->kind == XS_KEY_KIND)
				slot->kind = XS_STRING_KIND;
			else
				slot->kind = XS_STRING_X_KIND;
			slot->ID = mxID(_uri);
			slot->flag |= XS_DONT_DELETE_FLAG | XS_DONT_SET_FLAG;
		}
		mxPushClosure(module);
		fxRunScript(the, script, module, C_NULL, C_NULL, C_NULL, module->value.reference);
		mxPop();
	}
}

txID fxResolveSpecifier(txMachine* the, txSlot* realm, txID moduleID, txSlot* name)
{
	if (moduleID == XS_NO_ID) {
		if (realm == mxModuleInstanceInternal(mxProgram.value.reference)->value.module.realm) {
			moduleID = fxFindModule(the, realm, moduleID, name);
			if (moduleID == XS_NO_ID) {
				fxToStringBuffer(the, name, the->nameBuffer, sizeof(the->nameBuffer));
				mxReferenceError("module \"%s\" not found", the->nameBuffer);
			}
		}
		else {
			if (name->kind == XS_STRING_X_KIND)
				moduleID = fxNewNameX(the, name->value.string);
			else
				moduleID = fxNewName(the, name);
		}
	}
	else {
		txSlot* resolveHook = mxResolveHook(realm);
		while (mxIsUndefined(resolveHook)) {
			txSlot* parent = mxRealmParent(realm);
			if (mxIsReference(parent)) {
				realm = parent->value.reference;
				resolveHook = mxResolveHook(realm);
			}
			else
				break;
		}
		if (mxIsUndefined(resolveHook)) {
			mxCheck(the, realm == mxModuleInstanceInternal(mxProgram.value.reference)->value.module.realm);
			moduleID = fxFindModule(the, realm, moduleID, name);
			if (moduleID == XS_NO_ID) {
				fxToStringBuffer(the, name, the->nameBuffer, sizeof(the->nameBuffer));
				mxReferenceError("module \"%s\" not found", the->nameBuffer);
			}
		}
		else {
			mxPushUndefined();
			mxPushSlot(resolveHook);
			mxCall();
			mxPushSlot(name);
			fxPushKeyString(the, moduleID);
			mxRunCount(2);
			fxToString(the, the->stack);
			moduleID = fxNewName(the, the->stack);
			mxPop();
		}
	}
	name->kind = XS_SYMBOL_KIND;
	name->value.symbol = moduleID;
	return moduleID;
}

void fxRunImport(txMachine* the, txSlot* realm, txID moduleID)
{
	txSlot* stack = the->stack;
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
			if (!module) {
				module = mxBehaviorSetProperty(the, mxOwnModules(realm)->value.reference, moduleID, 0, XS_OWN);
				fxMapModule(the, realm, moduleID, module);
			}
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
				if ((status == XS_MODULE_STATUS_NEW) || (status == XS_MODULE_STATUS_LOADED)) {
					txSlot* queue = mxModuleQueue.value.reference;
					if (fxQueueModule(the, queue, module))
						fxLoadModules(the, queue);
				}
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

void fxRunImportFulfilled(txMachine* the, txSlot* module)
{
	txSlot* stack = the->stack;
	txSlot* slot = mxModuleInstanceFulfill(module);
	while (slot) {
		mxPushSlot(slot);
		slot = slot->next;
		slot = slot->next;
	}
	mxModuleInstanceMeta(module)->next = C_NULL;
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

void fxRunImportRejected(txMachine* the, txSlot* module)
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

void fxRunImportNow(txMachine* the, txSlot* realm, txID moduleID)
{
	txSlot* stack = the->stack;
	txSlot* module;
	txID status;
	txSlot* slot;
	fxToString(the, stack);
	moduleID = fxResolveSpecifier(the, realm, moduleID, stack);
	module = fxGetModule(the, realm, moduleID);
	if (!module) {
		module = mxBehaviorSetProperty(the, mxOwnModules(realm)->value.reference, moduleID, 0, XS_OWN);
		fxMapModule(the, realm, moduleID, module);
	}
	status = mxModuleStatus(module);
	if ((status == XS_MODULE_STATUS_NEW) || (status == XS_MODULE_STATUS_LOADED)) {
		txSlot* queue = fxNewInstance(the);
		mxTry(the) {
			stack->kind = module->kind;
			stack->value = module->value;

			slot = mxBehaviorSetProperty(the, queue, moduleID, 0, XS_OWN);;
			slot->flag |= XS_BASE_FLAG;
			slot->kind = XS_REFERENCE_KIND;
			slot->value.reference = module->value.reference;
		
			mxReportModuleQueue("LOAD");
			module = queue->next;
			while (module) {
				if (mxModuleStatus(module) == XS_MODULE_STATUS_NEW) {
					txSlot* meta = mxModuleMeta(module);
					txSlot* internal = meta->value.reference->next;
					txID moduleID = internal->value.module.id;
					txSlot* realm = internal->value.module.realm;
					txSlot* loadNowHook = mxLoadNowHook(realm);
					if (mxIsUndefined(loadNowHook)) {
						if (realm != mxModuleInstanceInternal(mxProgram.value.reference)->value.module.realm)
							mxTypeError("no loadNowHook");
						fxLoadModule(the, module, moduleID);
						if (mxModuleExecute(module)->kind == XS_NULL_KIND)
							mxTypeError("no module");
					}
					else {
						txSlot* record;
						mxModuleStatus(module) = XS_MODULE_STATUS_LOADING;
						mxPushSlot(module);
						mxPushSlot(loadNowHook);
						mxCall();
						fxPushKeyString(the, moduleID);
						mxRunCount(1);
						record = the->stack;
						if (!mxIsReference(record))
							mxTypeError("no module");
						if (!mxIsStaticModuleRecord(record->value.reference)) {
							mxPush(mxStaticModuleRecordConstructor);
							mxNew();
							mxPushSlot(record);
							mxRunCount(1);
							mxPullSlot(record);
						}
						fxDuplicateModuleTransfers(the, record, module);
						fxCopyModuleMeta(the, record, module);
						mxPop();
					}
					mxModuleStatus(module) = XS_MODULE_STATUS_LOADED;
				}
				if (mxModuleStatus(module) == XS_MODULE_STATUS_LOADED) {
					fxLoadModulesFrom(the, queue, module->value.reference, 1);
					mxModuleStatus(module) = XS_MODULE_STATUS_LINKING;
					mxReportModuleQueue("LOAD");
					module = queue->next;
				}
				else
					module = module->next;
			}
			fxLinkModules(the, queue);
			mxReportModuleQueue("INIT");
			module = queue->next;
			while (module) {
				if (mxModuleStatus(module) == XS_MODULE_STATUS_LINKED) {
					mxModuleStatus(module) = XS_MODULE_STATUS_EXECUTING;
					if (mxModuleExecute(module)->value.reference->value.instance.prototype == mxAsyncFunctionPrototype.value.reference)
						mxTypeError("async module");
					mxPushSlot(mxModuleHosts(module));
					mxPull(mxHosts);
					mxPushUndefined();
					mxPushSlot(mxModuleExecute(module));
					mxCall();
					mxRunCount(0);
					mxPop();
					fxCompleteModule(the, module->value.reference, C_NULL);
					mxModuleMeta(module)->next = C_NULL;
					mxPushUndefined();
					mxPull(mxHosts);
					mxReportModuleQueue("INIT");
				}
				module = module->next;
			}
			mxPop();
		}
		mxCatch(the) {
			module = queue->next;
			while (module) {
				if (mxModuleStatus(module) < XS_MODULE_STATUS_EXECUTED) {
					fxCompleteModule(the, module->value.reference, &mxException);
					mxModuleMeta(module)->next = C_NULL;
				}
				module = module->next;
			}
			mxPushUndefined();
			mxPull(mxHosts);
			fxJump(the);
		}
	}
	else if (status == XS_MODULE_STATUS_EXECUTED) {
		stack->kind = module->kind;
		stack->value = module->value;
	}
	else if (status == XS_MODULE_STATUS_ERROR) {
		mxPushSlot(mxModuleMeta(module));
		mxPull(mxException);
		fxJump(the);
	}
	else {
		mxTypeError("async module");
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
	if (fxIsKeySymbol(the, id)) {
		if (id == mxID(_Symbol_toStringTag))
			return 0;
	}
	if (mxModuleInstanceStatus(instance) < XS_MODULE_STATUS_LINKED)
		mxReferenceError("module not initialized yet");
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
    }
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
	if (fxIsKeySymbol(the, id)) {
		if (id == mxID(_Symbol_toStringTag))
			return mxModulePrototype.value.reference->next;
	}
	if (mxModuleInstanceStatus(instance) < XS_MODULE_STATUS_LINKED)
		mxReferenceError("module not initialized yet");
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
	}
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
	if (fxIsKeySymbol(the, id)) {
		if (id == mxID(_Symbol_toStringTag))
			return 1;
	}
	if (mxModuleInstanceStatus(instance) < XS_MODULE_STATUS_LINKED)
		mxReferenceError("module not initialized yet");
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
    }
	return 0;
}

txBoolean fxModuleIsExtensible(txMachine* the, txSlot* instance)
{
	return 0;
}

void fxModuleOwnKeys(txMachine* the, txSlot* instance, txFlag flag, txSlot* keys)
{
	if (flag & XS_EACH_NAME_FLAG) {
		if (mxModuleInstanceStatus(instance) < XS_MODULE_STATUS_LINKED)
			mxReferenceError("module not initialized yet");
		{
			txSlot* exports = mxModuleInstanceExports(instance);
			if (mxIsReference(exports)) {
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
		}
	}
	if (flag & XS_EACH_SYMBOL_FLAG) {
		txSlot* property = mxModulePrototype.value.reference->next;
		keys = fxQueueKey(the, property->ID, 0, keys);
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
	if (slot->kind == XS_REFERENCE_KIND) {
		txSlot* instance = slot->value.reference;
		if (((slot = instance->next)) && (slot->flag & XS_INTERNAL_FLAG) && (slot->kind == XS_PROGRAM_KIND)) {
			return instance;
		}
	}
	mxTypeError("this is no Compartment instance");
	return C_NULL;
}

void fx_Compartment(txMachine* the)
{
	txSlot* module = mxFunctionInstanceHome(mxFunction->value.reference)->value.home.module;
	txSlot* program = C_NULL;
	txSlot* global = C_NULL;
	txSlot* closures = C_NULL;
	txSlot* slot;
	txSlot* own;
	txInteger id;
	
	if (!module) module = mxProgram.value.reference;
	mxTry(the) {
		if (mxIsUndefined(mxTarget))
			mxTypeError("call Compartment");
			
		mxPushSlot(mxTarget);
		fxGetPrototypeFromConstructor(the, &mxCompartmentPrototype);
		program = fxNewProgramInstance(the);
		mxPullSlot(mxResult);
		
		// PARENT
		mxPushReference(mxModuleInstanceInternal(module)->value.module.realm);
		
		// GLOBALS
		
		if (the->sharedMachine == C_NULL) {
			txSlot* instance;
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
			
			mxPush(mxCompartmentPrototype);
			instance = fxBuildHostConstructor(the, mxCallback(fx_Compartment), 1, mxID(_Compartment));
			mxFunctionInstanceHome(instance)->value.home.module = program;
			slot = fxNextSlotProperty(the, slot, the->stack, mxID(_Compartment), XS_DONT_ENUM_FLAG);
			mxPop();
			
			mxPush(mxFunctionPrototype);
			instance = fxBuildHostConstructor(the, mxCallback(fx_Function), 1, mxID(_Function));
			mxFunctionInstanceHome(instance)->value.home.module = program;
			slot = fxNextSlotProperty(the, slot, the->stack, mxID(_Function), XS_DONT_ENUM_FLAG);
			mxPop();
			
			instance = fxBuildHostFunction(the, mxCallback(fx_eval), 1, mxID(_eval));
			mxFunctionInstanceHome(instance)->value.home.module = program;
			slot = fxNextSlotProperty(the, slot, the->stack, mxID(_eval), XS_DONT_ENUM_FLAG);
			mxPop();
		}
		else {
			mxPush(mxCompartmentGlobal);
			global = fxNewObjectInstance(the);
			slot = fxLastProperty(the, global);
			id = _Compartment;
			for (; id < XS_INTRINSICS_COUNT; id++) {
				txSlot* instance = fxDuplicateInstance(the, the->stackPrototypes[-1 - id].value.reference);
				mxFunctionInstanceHome(instance)->value.home.module = program;
				slot = fxNextSlotProperty(the, slot, the->stack, mxID(id), XS_DONT_ENUM_FLAG);
				mxPop();
			}
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
            mxPop();
		}
		
		// MODULE MAP
		
		if ((mxArgc > 1) && (mxIsReference(mxArgv(1)))) {
			txSlot* target;
			txSlot* source;
			txSlot* at;
			txSlot* property;
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
						if (mxIsModule(the->stack->value.reference) || mxIsStaticModuleRecord(the->stack->value.reference)) {
							target = target->next = fxNewSlot(the);
							target->ID = at->value.at.id;
							target->kind = the->stack->kind;
							target->value = the->stack->value;
						}
						else {
							txSlot* descriptor = the->stack;
							mxPush(mxStaticModuleRecordConstructor);
							mxNew();
							mxPushSlot(descriptor);
							mxRunCount(1);
							mxPullSlot(descriptor);
							target = target->next = fxNewSlot(the);
							target->ID = at->value.at.id;
							target->kind = the->stack->kind;
							target->value = the->stack->value;
						}
					}
					else if (mxIsStringPrimitive(the->stack)) {
						target = target->next = fxNewSlot(the);
						target->ID = at->value.at.id;
						target->kind = the->stack->kind;
						target->value = the->stack->value;
					}
					else
						mxTypeError("no specifier");
					mxPop();
				}
			}
			mxPop(); // property
			mxPop(); // at
			mxPop(); // source
		}
		else {
			fxNewInstance(the);
			fxNewInstance(the);
		}
		
		// OPTIONS
		
		mxPushUndefined();
		closures = fxNewEnvironmentInstance(the, C_NULL);
		if ((mxArgc > 2) && (mxIsReference(mxArgv(2)))) {
			mxPushSlot(mxArgv(2));
			mxGetID(fxID(the, "globalLexicals"));
			slot = the->stack;
			if (slot->kind != XS_UNDEFINED_KIND) {
				txSlot* target = fxLastProperty(the, closures);
				txSlot* source;
				txSlot* at;
				txSlot* property;
				source = fxToInstance(the, slot);
				at = fxNewInstance(the);
				mxBehaviorOwnKeys(the, source, XS_EACH_NAME_FLAG, at);
				mxTemporary(property);
				while ((at = at->next)) {
					if (mxBehaviorGetOwnProperty(the, source, at->value.at.id, at->value.at.index, property) && !(property->flag & XS_DONT_ENUM_FLAG)) {
						mxPushReference(source);
						mxGetAll(at->value.at.id, at->value.at.index);
						target = target->next = fxNewSlot(the);
						target->value.closure = fxNewSlot(the);;
						target->kind = XS_CLOSURE_KIND;
						target->ID = at->value.at.id;
						if (property->kind == XS_ACCESSOR_KIND)
							target->value.closure->flag = property->value.accessor.setter ? XS_NO_FLAG : XS_DONT_SET_FLAG;
						else
							target->value.closure->flag = property->flag & XS_DONT_SET_FLAG;
						target->value.closure->kind = the->stack->kind;
						target->value.closure->value = the->stack->value;
						mxPop();
					}
				}
				mxPop(); // property
				mxPop(); // at
			}
			mxPop(); // globalLexicals
	
			mxPushSlot(mxArgv(2));
			mxGetID(fxID(the, "resolveHook"));
			slot = the->stack;
			if (slot->kind != XS_UNDEFINED_KIND) {
				if (!fxIsCallable(the, slot))
					mxTypeError("resolveHook is no function");
			}
            mxPushSlot(mxArgv(2));
			mxGetID(fxID(the, "moduleMapHook"));
			slot = the->stack;
			if (slot->kind != XS_UNDEFINED_KIND) {
				if (!fxIsCallable(the, slot))
					mxTypeError("moduleMapHook is no function");
			}
            mxPushSlot(mxArgv(2));
			mxGetID(fxID(the, "loadHook"));
			slot = the->stack;
			if (slot->kind != XS_UNDEFINED_KIND) {
				if (!fxIsCallable(the, slot))
					mxTypeError("loadHook is no function");
			}
            mxPushSlot(mxArgv(2));
			mxGetID(fxID(the, "loadNowHook"));
			slot = the->stack;
			if (slot->kind != XS_UNDEFINED_KIND) {
				if (!fxIsCallable(the, slot))
					mxTypeError("loadNowHook is no function");
			}
		}
		else {
			mxPushUndefined();
			mxPushUndefined();
			mxPushUndefined();
			mxPushUndefined();
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
		fxRunScript(the, fxParseScript(the, &stream, fxStringGetter, mxStrictFlag | mxProgramFlag | mxDebugFlag), mxRealmGlobal(realm), C_NULL, mxRealmClosures(realm)->value.reference, C_NULL, program);
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

void fx_Compartment_prototype_module(txMachine* the)
{
	txSlot* program = fxCheckCompartmentInstance(the, mxThis);
	txSlot* realm = mxModuleInstanceInternal(program)->value.module.realm;
	txSlot* stack;
	txID moduleID;
	txSlot* module;
	if (mxArgc > 0)
		mxPushSlot(mxArgv(0));
	else
		mxPushUndefined();
	stack = the->stack;
	fxToString(the, stack);
	moduleID = fxResolveSpecifier(the, realm, XS_NO_ID, stack);
	module = fxGetModule(the, realm, moduleID);
	if (!module) {
		module = mxBehaviorSetProperty(the, mxOwnModules(realm)->value.reference, moduleID, 0, XS_OWN);
		fxMapModule(the, realm, moduleID, module);
	}
	stack->value.reference = module->value.reference;
	stack->kind = XS_REFERENCE_KIND;
    the->stack = stack;
	mxPullSlot(mxResult);
}

txSlot* fxCheckStaticModuleRecordInstance(txMachine* the, txSlot* slot)
{
	if (slot->kind == XS_REFERENCE_KIND) {
		txSlot* instance = slot->value.reference;
		if (((slot = instance->next)) && (slot->flag & XS_INTERNAL_FLAG) && (slot->kind == XS_STATIC_MODULE_RECORD_KIND)) {
			return instance;
		}
	}
	mxTypeError("this is no StaticModuleRecord instance");
	return C_NULL;
}

txSlot* fxNewStaticModuleRecordInstance(txMachine* the)
{
	txSlot* instance;
	txSlot* slot;
	txSlot* meta;
	instance = fxNewSlot(the);
	instance->kind = XS_INSTANCE_KIND;
	instance->value.instance.garbage = C_NULL;
	instance->value.instance.prototype = the->stack->value.reference;
	the->stack->kind = XS_REFERENCE_KIND;
	the->stack->value.reference = instance;
	/* HOST */
	slot = instance->next = fxNewSlot(the);
	slot->flag = XS_INTERNAL_FLAG;
	slot->kind = XS_STATIC_MODULE_RECORD_KIND;
	slot->value.module.realm = C_NULL;
	slot->value.module.id = XS_NO_ID;
	/* EXPORTS */
	slot = fxNextNullProperty(the, slot, XS_NO_ID, XS_INTERNAL_FLAG);
	/* META */
	meta = fxNewInstance(the);
	slot = fxNextReferenceProperty(the, slot, meta, XS_NO_ID, XS_INTERNAL_FLAG);
	mxPop();
	/* TRANSFERS */
	slot = fxNextNullProperty(the, slot, XS_NO_ID, XS_INTERNAL_FLAG);
	/* INITIALIZE */
	slot = fxNextNullProperty(the, slot, XS_NO_ID, XS_INTERNAL_FLAG);
	/* FUNCTION */
	slot = fxNextNullProperty(the, slot, XS_NO_ID, XS_INTERNAL_FLAG);
	/* HOSTS */
	slot = fxNextNullProperty(the, slot, XS_NO_ID, XS_INTERNAL_FLAG);
 	return instance;
}

void fx_StaticModuleRecord_initialize(txMachine* the)
{
	txSlot* instance = mxFunction->value.reference;
	txSlot* function = fxLastProperty(the, instance);
	txSlot* home = mxFunctionInstanceHome(instance);
	txSlot* module = home->value.home.module;
	txSlot* meta = mxModuleInstanceMeta(module);
	txSlot* closures = mxFunctionInstanceCode(instance)->value.code.closures;
	txSlot* property;
// 	if (mxIsReference(function)) {
// 		instance = function->value.reference;
// 		if (mxIsFunction(instance))
// 			mxFunctionInstanceHome(instance)->value.home.module = module;
// 	}
	closures->flag |= XS_DONT_PATCH_FLAG;
	property = closures->next->next;
	while (property) {
		property->flag |= XS_DONT_DELETE_FLAG;
		property = property->next;
	}
	/* THIS */
	mxPushReference(home->value.home.object);
	/* FUNCTION */
	mxPushSlot(function);
	mxCall();
	/* ARGUMENTS */
	mxPushReference(closures);
	mxPushSlot(meta);
	mxRunCount(2);
	mxPullSlot(mxResult);
}

void fx_StaticModuleRecord(txMachine* the)
{
	txSlot* instance;
	txSlot* slot;
	if (mxIsUndefined(mxTarget))
		mxTypeError("call: StaticModuleRecord");
	mxPushSlot(mxTarget);
	fxGetPrototypeFromConstructor(the, &mxStaticModuleRecordPrototype);
	instance = fxNewStaticModuleRecordInstance(the);
	mxPullSlot(mxResult);
	if (mxArgc == 0)
		mxTypeError("no descriptor");
		
	mxPushSlot(mxArgv(0));
	mxGetID(fxID(the, "specifier"));
	slot = the->stack;
	if (slot->kind != XS_UNDEFINED_KIND) {
		txSlot* internal = mxModuleInstanceInternal(instance);
		fxToString(the, slot);
		if (slot->kind == XS_STRING_X_KIND)
			internal->value.module.id = fxNewNameX(the, slot->value.string);
		else
			internal->value.module.id = fxNewName(the, slot);
	}
	mxPop();

	mxPushSlot(mxArgv(0));
	mxGetID(fxID(the, "meta"));
	slot = the->stack;
	if (slot->kind != XS_UNDEFINED_KIND) {
		txSlot* meta = mxModuleInstanceMeta(instance);
		mxPushUndefined();
		mxPush(mxAssignObjectFunction);
		mxCall();
		mxPushSlot(meta);
		mxPushSlot(slot);
		mxRunCount(2);
		mxPop();
	}
	mxPop();
		
#ifdef mxParse
	mxPushSlot(mxArgv(0));
	mxGetID(fxID(the, "source"));
	slot = the->stack;
	if (slot->kind != XS_UNDEFINED_KIND) {
		txStringStream stream;
		txScript* script;
		stream.slot = slot;
		stream.offset = 0;
		stream.size = mxStringLength(fxToString(the, slot));
		script = fxParseScript(the, &stream, fxStringGetter, mxDebugFlag);
		mxPushClosure(mxResult);
		fxRunScript(the, script, mxResult, C_NULL, C_NULL, C_NULL, instance);
		mxPop();
		if (mxModuleInstanceExecute(instance)->kind == XS_NULL_KIND)
			mxTypeError("no module");
		mxPop();
		return;
	}
	mxPop();
#endif
	mxPushSlot(mxArgv(0));
	mxGetID(fxID(the, "record"));
	slot = the->stack;
	if (!mxIsUndefined(slot)) {
		fxCheckStaticModuleRecordInstance(the, slot);
		fxDuplicateModuleTransfers(the, slot, mxResult);
		return;
	}
	mxPop();

	mxPushSlot(mxArgv(0));
	mxGetID(fxID(the, "initialize"));
	slot = the->stack;
	if (!mxIsUndefined(slot)) {
		txSlot* transfers;
		txSlot* transfer;
		if (!fxIsCallable(the, slot))
			mxTypeError("initialize is no function");
			
		transfers = fxNewHostFunction(the, fx_StaticModuleRecord_initialize, 0, XS_NO_ID);
		mxFunctionInstanceHome(transfers)->value.home.object = fxToInstance(the, mxArgv(0));
		transfer = fxLastProperty(the, transfers);
		transfer = fxNextSlotProperty(the, transfer, mxArgv(0), XS_NO_ID, XS_INTERNAL_FLAG);
		transfer = fxNextSlotProperty(the, transfer, slot, XS_NO_ID, XS_INTERNAL_FLAG);
		mxPullSlot(mxModuleInstanceExecute(instance));
			
		mxPush(mxObjectPrototype);
		transfers = fxNewObjectInstance(the);
		transfer = fxLastProperty(the, transfers);
		
		mxPushSlot(mxArgv(0));
		mxGetID(mxID(_bindings));
		if (!mxIsUndefined(the->stack)) {
			txSlot* array;
			txInteger length, index;
			
			array = the->stack;
			mxPushSlot(array);
			mxGetID(mxID(_length));
			length = fxToInteger(the, the->stack);
			mxPop();
			
			for (index = 0; index < length; index++) {
				txSlot* item;
				txBoolean export = 0;
				txBoolean import = 0;
				txID nameID, asID;
				txSlot* specifier;
				txSlot* former;
	
				mxPushSlot(array);
				mxGetIndex(index);
				item = 	the->stack;
				
				mxPushSlot(item);
				mxGetID(mxID(_export));
				if (!mxIsUndefined(the->stack))
					export = 1;
				mxPushSlot(item);
				mxGetID(mxID(_import));
				if (!mxIsUndefined(the->stack))
					import = 1;
				if (export) {
					if (import)
						mxSyntaxError("export and import");
					mxPop();
				}
				else {
					if (!import)
						mxSyntaxError("neither export nor import");
				}	
				fxToString(the, the->stack);
				if (c_strcmp(fxToString(the, the->stack), "*"))
					nameID = fxNewName(the, the->stack);
				else
					nameID = XS_NO_ID;
				if (import)
					mxPop();
				mxPop();
			
				mxPushSlot(item);
				mxGetID(mxID(_as));
				if (!mxIsUndefined(the->stack)) {
					fxToString(the, the->stack);
					asID = fxNewName(the, the->stack);
				}
				else
					asID = nameID;
				mxPop();
			
				mxPushSlot(item);
				mxGetID(mxID(_from));
				if (!mxIsUndefined(the->stack)) {
					fxToString(the, the->stack);
					specifier = the->stack;
				}
				else
					specifier = C_NULL;
				
				if (export) {
					if (asID != XS_NO_ID) {
						former = transfers->next;
						while (former) {
							txSlot* aliases = mxTransferAliases(former);
							if (!mxIsNull(aliases)) {
								txSlot* alias = aliases->value.reference->next;
								while (alias) {
									if (alias->value.symbol == asID) {
										fxIDToString(the, asID, the->nameBuffer, sizeof(the->nameBuffer));
										mxSyntaxError("duplicate export %s", the->nameBuffer);
									}
									alias = alias->next;
								}
							}
							former = former->next;
						}
						if (specifier) {
							former = transfers->next;
							while (former) {
								txSlot* aliases = mxTransferAliases(former);
								if (!mxIsNull(aliases)) {
									txSlot* local = mxTransferLocal(former);
									if (mxIsNull(local)) {
										txSlot* from = mxTransferFrom(former);
										if (!mxIsNull(from) && !c_strcmp(from->value.string, specifier->value.string)) {
											txSlot* import = mxTransferImport(former);
											if (mxIsNull(import)) {
												if (nameID == XS_NO_ID) 
													break;
											}
											else {
												if (nameID == import->value.symbol)
													break;
											}
										}
									}
								}
								former = former->next;
							}
						}
						else {
							former = transfers->next;
							while (former) {
								txSlot* local = mxTransferLocal(former);
								if ((local->kind == XS_SYMBOL_KIND) && (local->value.symbol == nameID))
									break;
								former = former->next;
							}
						}
						if (former) {
							txSlot* aliases = mxTransferAliases(former);
							txSlot* alias;
							if (mxIsNull(aliases)) {
								mxPush(mxObjectPrototype);
								alias = fxLastProperty(the, fxNewObjectInstance(the));
								mxPullSlot(mxTransferAliases(former));
							}
							else {
								alias = fxLastProperty(the, aliases->value.reference);
							}
							fxNextSymbolProperty(the, alias, asID, XS_NO_ID, XS_DONT_ENUM_FLAG);
						}
						else if (specifier) {
							mxPushNull();
							mxPushSlot(specifier);
							if ((nameID != XS_NO_ID) || (asID != XS_NO_ID)) {
								if (nameID != XS_NO_ID) 
									mxPushSymbol(nameID);
								else
									mxPushNull();
								if (asID != XS_NO_ID) 
									mxPushSymbol(asID);
								else			
									mxPushSymbol(nameID);
								mxPushInteger(4);
							}
							else {	
								mxPushNull();
								mxPushInteger(3);
							}
							fxPrepareTransfer(the);
							transfer = fxNextSlotProperty(the, transfer, the->stack, XS_NO_ID, XS_DONT_ENUM_FLAG);
							mxPop(); // transfer
						}
						else {
							mxPushSymbol(nameID);
							mxPushNull();
							mxPushNull();
							mxPushSymbol(asID);
							mxPushInteger(4);
							fxPrepareTransfer(the);
							transfer = fxNextSlotProperty(the, transfer, the->stack, XS_NO_ID, XS_DONT_ENUM_FLAG);
							mxPop(); // transfer
						}
					}
					else if (specifier) {
						former = transfers->next;
						while (former) {
							txSlot* from = mxTransferFrom(former);
							if (!mxIsNull(from) && !c_strcmp(from->value.string, specifier->value.string)) {
								txSlot* local = mxTransferLocal(former);
								if (mxIsNull(local)) {
									txSlot* import = mxTransferImport(former);
									if (mxIsNull(import)) {
										txSlot* aliases = mxTransferAliases(former);
										if (mxIsNull(aliases)) {
											mxSyntaxError("duplicate export *");
										}
									}
								}
							}
							former = former->next;
						}
						mxPushNull();
						mxPushSlot(specifier);
						mxPushNull();
						mxPushInteger(3);
						fxPrepareTransfer(the);
						transfer = fxNextSlotProperty(the, transfer, the->stack, XS_NO_ID, XS_DONT_ENUM_FLAG);
						mxPop(); // transfer
					}
					else
						mxSyntaxError("invalid export *");
				}
				else {
					if (!specifier) {
						if (asID == XS_NO_ID)
							mxSyntaxError("invalid import *");
						else {
							fxIDToString(the, asID, the->nameBuffer, sizeof(the->nameBuffer));
							mxSyntaxError("invalid import %s", the->nameBuffer);
						}
					}
					else if (asID == XS_NO_ID)
						mxSyntaxError("invalid import * from %s", specifier->value.string);
				
					former = transfers->next;
					while (former) {
						txSlot* local = mxTransferLocal(former);
						if ((local->kind == XS_SYMBOL_KIND) && (local->value.symbol == asID)) {
							txSlot* from = mxTransferFrom(former);
							if (!mxIsNull(from)) {
								fxIDToString(the, asID, the->nameBuffer, sizeof(the->nameBuffer));
								mxSyntaxError("duplicate import %s", the->nameBuffer);
							}
							break;
						}
						former = former->next;
					}
					if (former) {
						txSlot* from = mxTransferFrom(former);
						mxPushSlot(specifier);
						mxPullSlot(from);	
						if (nameID != XS_NO_ID) {
							txSlot* import = mxTransferImport(former);
							import->kind = XS_SYMBOL_KIND;
							import->value.symbol = nameID;
						}
					}
					else {
						mxPushSymbol(asID);
						mxPushSlot(specifier);
						if (nameID != XS_NO_ID)
							mxPushSymbol(nameID);
						else			
							mxPushNull();
						mxPushInteger(3);
						fxPrepareTransfer(the);
						transfer = fxNextSlotProperty(the, transfer, the->stack, XS_NO_ID, XS_DONT_ENUM_FLAG);
						mxPop(); // transfer
					}
				}	

				mxPop(); // specifier
				mxPop(); // item
			}
		}
		mxPop(); // array
		mxPullSlot(mxModuleInstanceTransfers(instance));
		return;
	}
	mxPop();
	
	mxTypeError("invalid options");
}

void fx_StaticModuleRecord_prototype_get_bindings(txMachine* the)
{
	txSlot* record = fxCheckStaticModuleRecordInstance(the, mxThis);
	txSlot* resultInstance;
	txSlot* resultArray;
	txSlot* resultItem;
	txSlot* transfer;

	mxPush(mxArrayPrototype);
	resultInstance = fxNewArrayInstance(the);
	mxPullSlot(mxResult);
	resultArray = resultInstance->next;
	resultItem = resultArray;
	
	transfer = mxModuleInstanceTransfers(record)->value.reference->next;
	while (transfer) {
		txSlot* local = mxTransferLocal(transfer);
		txSlot* from = mxTransferFrom(transfer);
		txSlot* import = mxTransferImport(transfer);
		txSlot* aliases = mxTransferAliases(transfer);
		
		if (local->kind != XS_NULL_KIND) {
			if (from->kind != XS_NULL_KIND) {
				txSlot* slot = fxNewObject(the);
				if (import->kind == XS_NULL_KIND) {
					mxPushStringX("*");
					slot = fxNextSlotProperty(the, slot, the->stack, mxID(_import), XS_NO_FLAG);
					mxPop();
					fxPushKeyString(the, local->value.symbol);
					slot = fxNextSlotProperty(the, slot, the->stack, mxID(_as), XS_NO_FLAG);
					mxPop();
				}
				else {
					fxPushKeyString(the, import->value.symbol);
					slot = fxNextSlotProperty(the, slot, the->stack, mxID(_import), XS_NO_FLAG);
					mxPop();
					if (local->value.symbol != import->value.symbol) {
						fxPushKeyString(the, local->value.symbol);
						slot = fxNextSlotProperty(the, slot, the->stack, mxID(_as), XS_NO_FLAG);
						mxPop();
					}
				}
				mxPushString(from->value.string);
				slot = fxNextSlotProperty(the, slot, the->stack, mxID(_from), XS_NO_FLAG);
				mxPop();
				resultItem = resultItem->next = fxNewSlot(the);
				mxPullSlot(resultItem);
				resultArray->value.array.length++;
			}
			if (aliases->kind == XS_REFERENCE_KIND) {
				txSlot* alias = aliases->value.reference->next;
				while (alias) {
					txSlot* slot = fxNewObject(the);
					fxPushKeyString(the, local->value.symbol);
					slot = fxNextSlotProperty(the, slot, the->stack, mxID(_export), XS_NO_FLAG);
					mxPop();
					if (local->value.symbol != alias->value.symbol) {
						fxPushKeyString(the, alias->value.symbol);
						slot = fxNextSlotProperty(the, slot, the->stack, mxID(_as), XS_NO_FLAG);
						mxPop();
					}
					resultItem = resultItem->next = fxNewSlot(the);
					mxPullSlot(resultItem);
					resultArray->value.array.length++;
					alias = alias->next;
				}
			}
		}
		else if (aliases->kind == XS_REFERENCE_KIND) {
			txSlot* alias = aliases->value.reference->next;
			while (alias) {
				txSlot* slot = fxNewObject(the);
				if (from->kind != XS_NULL_KIND) {
					if (import->kind == XS_NULL_KIND) {
						mxPushStringX("*");
						slot = fxNextSlotProperty(the, slot, the->stack, mxID(_export), XS_NO_FLAG);
						mxPop();
						fxPushKeyString(the, alias->value.symbol);
						slot = fxNextSlotProperty(the, slot, the->stack, mxID(_as), XS_NO_FLAG);
						mxPop();
					}
					else {
						fxPushKeyString(the, import->value.symbol);
						slot = fxNextSlotProperty(the, slot, the->stack, mxID(_export), XS_NO_FLAG);
						mxPop();
						if (alias->value.symbol != import->value.symbol) {
							fxPushKeyString(the, alias->value.symbol);
							slot = fxNextSlotProperty(the, slot, the->stack, mxID(_as), XS_NO_FLAG);
							mxPop();
						}
					}
					mxPushString(from->value.string);
					slot = fxNextSlotProperty(the, slot, the->stack, mxID(_from), XS_NO_FLAG);
					mxPop();
					resultItem = resultItem->next = fxNewSlot(the);
					mxPullSlot(resultItem);
					resultArray->value.array.length++;
				}
				alias = alias->next;
			}
		}
		else if ((from->kind != XS_NULL_KIND) && (import->kind == XS_NULL_KIND)) {
			txSlot* slot = fxNewObject(the);
			mxPushStringX("*");
			slot = fxNextSlotProperty(the, slot, the->stack, mxID(_export), XS_NO_FLAG);
			mxPop();
			mxPushString(from->value.string);
			slot = fxNextSlotProperty(the, slot, the->stack, mxID(_from), XS_NO_FLAG);
			mxPop();
			resultItem = resultItem->next = fxNewSlot(the);
			mxPullSlot(resultItem);
			resultArray->value.array.length++;
		}
		transfer = transfer->next;
	}
	
	fxCacheArray(the, resultInstance);
}


