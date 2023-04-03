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

static txBoolean fxMapModule(txMachine* the, txSlot* realm, txID moduleID, txSlot* module, txSlot* queue, txSlot* result);
static void fxMapModuleDescriptor(txMachine* the, txSlot* realm, txID moduleID, txSlot* module, txSlot* queue, txSlot* result, txSlot* descriptor);
static void fxNewModule(txMachine* the, txSlot* realm, txID moduleID, txSlot* module);
static void fxOrderModule(txMachine* the, txSlot* queue, txSlot* order, txSlot* module);
static void fxOverrideModule(txMachine* the, txSlot* queue, txSlot* result, txSlot* module, txSlot* record);
static txBoolean fxQueueModule(txMachine* the, txSlot* queue, txSlot* module);

static txID fxResolveSpecifier(txMachine* the, txSlot* realm, txID moduleID, txSlot* name);

static void fxRunImportFulfilled(txMachine* the, txSlot* module, txSlot* with);
static void fxRunImportRejected(txMachine* the, txSlot* module, txSlot* with);

static void fxRunImportNow(txMachine* the, txSlot* realm, txID id);

static txBoolean fxModuleDefineOwnProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* slot, txFlag mask);
static txBoolean fxModuleDeleteProperty(txMachine* the, txSlot* instance, txID id, txIndex index);
static txSlot* fxModuleFindProperty(txMachine* the, txSlot* instance, txID id, txIndex index);
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
static void fxPrepareCompartmentFunction(txMachine* the, txSlot* program, txSlot* instance);

static txSlot* fxCheckModuleSourceInstance(txMachine* the, txSlot* slot);
static txSlot* fxNewModuleSourceInstance(txMachine* the);

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
#define mxIsModuleSource(THE_SLOT) \
	(((THE_SLOT)->next) && ((THE_SLOT)->next->flag & XS_INTERNAL_FLAG) && ((THE_SLOT)->next->kind == XS_MODULE_SOURCE_KIND))

#define mxModuleInstanceStatus(MODULE)		((MODULE)->next->next->ID)
#define mxModuleStatus(MODULE) 				mxModuleInstanceStatus((MODULE)->value.reference)

#if mxReport
#define mxModuleName(ID) c_strrchr(fxGetKeyName(the, (ID)), '/')
#define mxReportModuleQueue(LABEL) fxReportModuleQueue(the, queue, LABEL)
static void fxReportModuleQueue(txMachine* the, txSlot* queue, txString label)
{
	txSlot* module = queue->next;
	fprintf(stderr, "%s %p", label, queue);
	while (module) {
		fprintf(stderr, " '%s' %p %d", fxGetKeyName(the, module->ID), module->value.reference, mxModuleStatus(module));
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
	
	fxNewHostFunction(the, mxCallback(fxModuleGetter), 0, XS_NO_ID, XS_NO_ID);
	mxPushUndefined();
	the->stack->flag = XS_DONT_DELETE_FLAG;
	the->stack->kind = XS_ACCESSOR_KIND;
	the->stack->value.accessor.getter = (the->stack + 1)->value.reference;
	the->stack->value.accessor.setter = C_NULL;
	mxPull(mxModuleAccessor);
	the->stack += 1;
	
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
// 	slot = fxNextHostFunctionProperty(the, slot, mxCallback(fx_Compartment_prototype_module), 1, mxID(_module), XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "Compartment", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxCompartmentPrototype = *the->stack;
	slot = fxBuildHostConstructor(the, mxCallback(fx_Compartment), 1, mxID(_Compartment));
	mxCompartmentConstructor = *the->stack;
    mxPop();
    
	mxPush(mxObjectPrototype);
	slot = fxLastProperty(the, fxNewObjectInstance(the));
	slot = fxNextHostAccessorProperty(the, slot, mxCallback(fx_ModuleSource_prototype_get_bindings), C_NULL, mxID(_bindings), XS_DONT_ENUM_FLAG);
	slot = fxNextHostAccessorProperty(the, slot, mxCallback(fx_ModuleSource_prototype_get_needsImport), C_NULL, mxID(_needsImport), XS_DONT_ENUM_FLAG);
	slot = fxNextHostAccessorProperty(the, slot, mxCallback(fx_ModuleSource_prototype_get_needsImportMeta), C_NULL, mxID(_needsImportMeta), XS_DONT_ENUM_FLAG);
	slot = fxNextStringXProperty(the, slot, "ModuleSource", mxID(_Symbol_toStringTag), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	mxModuleSourcePrototype = *the->stack;
	slot = fxBuildHostConstructor(the, mxCallback(fx_ModuleSource), 1, mxID(_ModuleSource));
	mxModuleSourceConstructor = *the->stack;
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

void fxDuplicateModuleTransfers(txMachine* the, txSlot* srcModule, txSlot* dstModule)
{
	txSlot* srcSlot;
	txSlot* dstSlot;
	txSlot* transfer;
	txSlot* aliases;
	txSlot* function;
	srcSlot = mxModuleInternal(srcModule);
	dstSlot = mxModuleInternal(dstModule);
	dstSlot->flag |= (srcSlot->flag & (XS_IMPORT_FLAG | XS_IMPORT_META_FLAG));
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
				fxRunImportRejected(the, module->value.reference, module->value.reference);
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
							txSlot* resolveExecuteFunction;
							txSlot* rejectExecuteFunction;
							
							function = fxNewHostFunction(the, fxExecuteModulesFulfilled, 1, XS_NO_ID, mxExecuteModulesFulfilledProfileID);
							home = mxFunctionInstanceHome(function);
							home->value.home.object = queue;
							home->value.home.module = module->value.reference;
							resolveExecuteFunction = the->stack;
	
							function = fxNewHostFunction(the, fxExecuteModulesRejected, 1, XS_NO_ID, mxExecuteModulesRejectedProfileID);
							home = mxFunctionInstanceHome(function);
							home->value.home.object = queue;
							home->value.home.module = module->value.reference;
							rejectExecuteFunction = the->stack;
				
							fxPromiseThen(the, result->value.reference, resolveExecuteFunction, rejectExecuteFunction, C_NULL, C_NULL);
						
							mxPop();
							mxPop();
							mxPop();
							done = 0;
						}
						else {
							mxPop();
							fxCompleteModule(the, module->value.reference, C_NULL);
							fxRunImportFulfilled(the, module->value.reference, module->value.reference);
						}
					}
					mxCatch(the) {
						mxPush(mxException);
						mxException = mxUndefined;
						fxCompleteModule(the, module->value.reference, the->stack);
						fxRunImportRejected(the, module->value.reference, module->value.reference);
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
		queue->next = C_NULL;
		mxReportModuleQueue("DONE");
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
	fxRunImportFulfilled(the, module, module);
	fxExecuteModules(the, queue);
}

void fxExecuteModulesRejected(txMachine* the)
{
	txSlot* home = mxFunctionInstanceHome(mxFunction->value.reference);
	txSlot* queue = home->value.home.object;
	txSlot* module = home->value.home.module;
	fxCompleteModule(the, module,  mxArgv(0));
	fxRunImportRejected(the, module, module);
	fxExecuteModules(the, queue);
}

void fxExecuteVirtualModuleSource(txMachine* the)
{
	txSlot* instance = mxFunction->value.reference;
	txSlot* function = fxLastProperty(the, instance);
	txSlot* home = mxFunctionInstanceHome(instance);
	txSlot* module = home->value.home.module;
	txSlot* internal = mxModuleInstanceInternal(module);
	txSlot* meta = mxModuleInstanceMeta(module);
	txSlot* closures = mxFunctionInstanceCode(instance)->value.callback.closures;
	txSlot* property;
	if (mxIsUndefined(function))
		return;
	closures->flag |= XS_DONT_PATCH_FLAG;
	closures->value.instance.prototype = C_NULL;
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
	if (internal->flag & XS_IMPORT_FLAG) {
		function = fxNewHostFunction(the, fxExecuteVirtualModuleSourceImport, 1, XS_NO_ID, mxExecuteVirtualModuleSourceImportProfileID);
		mxFunctionInstanceHome(function)->value.home.module = module;
	}
	else
		mxPushUndefined();
	if (internal->flag & XS_IMPORT_META_FLAG)
		mxPushSlot(meta);
	else
		mxPushUndefined();
	mxRunCount(3);
	mxPullSlot(mxResult);
}

txSlot* fxGetModule(txMachine* the, txSlot* realm, txID moduleID)
{
	txSlot* result = mxBehaviorGetProperty(the, mxOwnModules(realm)->value.reference, moduleID, 0, XS_ANY);
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
			if (property->kind == XS_REFERENCE_KIND) {
				property = property->value.reference->next;
				if ((property->kind == XS_CODE_KIND) || (property->kind == XS_CODE_X_KIND))
					property->value.code.closures = closures;
				else if (property->kind == XS_CALLBACK_KIND)
					property->value.callback.closures = closures;
			}
			property = mxModuleExecute(module);
			if (property->kind == XS_REFERENCE_KIND) {
				property = property->value.reference->next;
				if ((property->kind == XS_CODE_KIND) || (property->kind == XS_CODE_X_KIND))
					property->value.code.closures = closures;
				else if (property->kind == XS_CALLBACK_KIND)
					property->value.callback.closures = closures;
			}
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
				if (((from->value.reference == module->value.reference) && (import->value.symbol == importID))
					|| ((from->value.reference == mxTransferFrom(transfer)->value.reference) && (import->value.symbol == mxTransferImport(transfer)->value.symbol))) {
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
				fxCheckCStack(the);
				fxLinkTransfer(the, from, import->value.symbol, transfer);
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
	if (queue->flag & XS_LEVEL_FLAG)
		return;
	queue->flag |= XS_LEVEL_FLAG;
	mxReportModuleQueue("LOAD");
	while (module) {
		if (mxModuleStatus(module) == XS_MODULE_STATUS_NEW) {
			txSlot* loader = mxModuleLoader(module);
			txID moduleID = loader->value.module.id;
			txSlot* realm = loader->value.module.realm;
			mxTry(the) {
				if (fxMapModule(the, realm, moduleID, module, queue, C_NULL)) {
					module = queue;
					done = 0;
				}
				else {
					txSlot* loadHook = mxLoadHook(realm);
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
						txSlot* promise;
						txSlot* function;
						txSlot* home;
						txSlot* resolveLoadFunction;
						txSlot* rejectLoadFunction;
						done = 0;
						mxModuleStatus(module) = XS_MODULE_STATUS_LOADING;
						
						mxPushUndefined();
						mxPushSlot(loadHook);
						mxCall();
						fxPushKeyString(the, moduleID);
						mxRunCount(1);
                        if (!mxIsReference(the->stack))
                            mxTypeError("loadHook returned no object");
						promise = the->stack->value.reference;
						if (!mxIsPromise(promise))
							mxTypeError("loadHook returned no promise");

						function = fxNewHostFunction(the, fxLoadModulesFulfilled, 1, XS_NO_ID, mxLoadModulesFulfilledProfileID);
						home = mxFunctionInstanceHome(function);
						home->value.home.object = queue;
						home->value.home.module = module->value.reference;
						resolveLoadFunction = the->stack;

						function = fxNewHostFunction(the, fxLoadModulesRejected, 1, XS_NO_ID, mxLoadModulesRejectedProfileID);
						home = mxFunctionInstanceHome(function);
						home->value.home.object = queue;
						home->value.home.module = module->value.reference;
						rejectLoadFunction = the->stack;
					
						fxPromiseThen(the, promise, resolveLoadFunction, rejectLoadFunction, C_NULL, C_NULL);
						
						mxPop();
						mxPop();
						mxPop();
					}
				}
			}
			mxCatch(the) {
				mxPush(mxException);
				mxException = mxUndefined;
				fxCompleteModule(the, module->value.reference, the->stack);
				fxRunImportRejected(the, module->value.reference, module->value.reference);
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
				fxRunImportRejected(the, module->value.reference, module->value.reference);
			}
		}
		module = module->next;
	}
	queue->flag &= ~XS_LEVEL_FLAG;
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
					fxRunImportRejected(the, module->value.reference, module->value.reference);
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
				fxNewModule(the, realm, importModuleID, importModule);
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
					mxPushSlot(mxModuleInstanceMeta(module));
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
	txSlot* internal = mxModuleInstanceInternal(module);
	txSlot* realm = internal->value.module.realm;
	txID moduleID = internal->value.module.id;
	mxTry(the) {
		mxPushReference(module);
		fxMapModuleDescriptor(the, realm, moduleID, the->stack, queue, C_NULL, mxArgv(0));
		mxPop(); // module
	}
	mxCatch(the) {
		mxPush(mxException);
		mxException = mxUndefined;
		fxCompleteModule(the, module, the->stack);
		fxRunImportRejected(the, module, module);
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
	fxRunImportRejected(the, module, module);
	fxLoadModules(the, queue);
}

void fxLoadVirtualModuleNamespace(txMachine* the, txSlot* object, txSlot* module)
{
	txSlot* exports = fxNewInstance(the);
	txSlot* export = exports;
	txSlot* at;
	txSlot* property;
	at = fxNewInstance(the);
	mxBehaviorOwnKeys(the, object, XS_EACH_NAME_FLAG, at);
	mxTemporary(property);
	while ((at = at->next)) {
		if (at->value.at.id != XS_NO_ID) {
			if (mxBehaviorGetOwnProperty(the, object, at->value.at.id, at->value.at.index, property) && !(property->flag & XS_DONT_ENUM_FLAG)) {
				mxPushReference(object);
				mxGetAll(at->value.at.id, at->value.at.index);
				export = export->next = fxNewSlot(the);
				export->ID = at->value.at.id;
				export->value.export.closure = fxNewSlot(the);
				export->value.export.closure->kind = the->stack->kind;
				export->value.export.closure->value = the->stack->value;
				export->value.export.module = module;
				export->kind = XS_EXPORT_KIND;
				mxPop();
			}
		}
	}
	mxPop();
	mxPop();
	mxPullSlot(mxModuleInstanceExports(module));
}

void fxLoadVirtualModuleSource(txMachine* the, txSlot* record, txSlot* instance)
{
	txSlot* slot;
	txSlot* function;
	txSlot* property;
	txSlot* transfers;
	txSlot* transfer;
	
	mxPushSlot(record);
	mxGetID(fxID(the, "execute"));
	slot = the->stack;
	if (!mxIsUndefined(slot)) {
		if (!fxIsCallable(the, slot))
			mxTypeError("execute is no function");
	}
	function = fxNewHostFunction(the, fxExecuteVirtualModuleSource, 0, XS_NO_ID, mxExecuteVirtualModuleSourceProfileID);
	property = mxFunctionInstanceHome(function);
	property->value.home.object = fxToInstance(the, record);
	property->value.home.module = instance;
	property = fxLastProperty(the, function);
	property = fxNextSlotProperty(the, property, slot, XS_NO_ID, XS_INTERNAL_FLAG);
	mxPullSlot(mxModuleInstanceExecute(instance));
	mxPop(); // initialize
			
	mxPush(mxObjectPrototype);
	transfers = fxNewObjectInstance(the);
	transfer = fxLastProperty(the, transfers);
	
	mxPushSlot(record);
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
			txSlot* temporary;
			txInteger from = 0;
			txInteger export = 0;
			txInteger import = 0;
			txID nameID, asID;
			txSlot* specifier = C_NULL;
			txSlot* former;

			mxPushSlot(array);
			mxGetIndex(index);
			item = the->stack;
		
			mxTemporary(temporary);
		
			mxPushSlot(item);
			mxGetID(mxID(_from));
			if (!mxIsUndefined(the->stack)) {
				from++;
				fxToString(the, the->stack);
				mxPullSlot(temporary);
				specifier = temporary;
			}
			else
				mxPop();
				
			mxPushSlot(item);
			mxGetID(fxID(the, "exportAllFrom"));
			if (!mxIsUndefined(the->stack)) {
				from++;
				export++;
				fxToString(the, the->stack);
				mxPullSlot(temporary);
				specifier = temporary;
				nameID = XS_NO_ID;
			}
			else
				mxPop();
		
			mxPushSlot(item);
			mxGetID(fxID(the, "importAllFrom"));
			if (!mxIsUndefined(the->stack)) {
				from++;
				import++;
				fxToString(the, the->stack);
				mxPullSlot(temporary);
				specifier = temporary;
				nameID = XS_NO_ID;
			}
			else
				mxPop();
		
			mxPushSlot(item);
			mxGetID(mxID(_export));
			if (!mxIsUndefined(the->stack)) {
				export++;
				fxToString(the, the->stack);
				nameID = fxNewName(the, the->stack);
			}
			mxPop();
			
			mxPushSlot(item);
			mxGetID(mxID(_import));
			if (!mxIsUndefined(the->stack)) {
				import++;
				fxToString(the, the->stack);
				nameID = fxNewName(the, the->stack);
			}
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
				
			if (from > 1)
				mxSyntaxError("too many from");
			else if (export > 1)
				mxSyntaxError("too many export");
			else if (import > 1)
				mxSyntaxError("too many import");
			else if (export && import)
				mxSyntaxError("export and import");
			else if (!export && !import)
				mxSyntaxError("neither export nor import");
			
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

			mxPop(); // temporary
			mxPop(); // item
		}
	}
	mxPop(); // bindings

#if mxReport
	{
		fxIDToString(the, mxModuleInstanceInternal(instance)->value.module.id, the->nameBuffer, sizeof(the->nameBuffer));
		fprintf(stderr, "### %s\n", the->nameBuffer);
		txSlot* transfer = the->stack->value.reference->next;
		while (transfer) {
			txSlot* local = mxTransferLocal(transfer);
			txSlot* from = mxTransferFrom(transfer);
			txSlot* import = mxTransferImport(transfer);
			txSlot* aliases = mxTransferAliases(transfer);
			if (local->kind != XS_NULL_KIND) {
				fxIDToString(the, local->value.symbol, the->nameBuffer, sizeof(the->nameBuffer));
				fprintf(stderr, " local %s", the->nameBuffer);
			}
			if (from->kind != XS_NULL_KIND) {
				fprintf(stderr, " from %s", from->value.string);
			}
			if (import->kind != XS_NULL_KIND) {
				fxIDToString(the, import->value.symbol, the->nameBuffer, sizeof(the->nameBuffer));
				fprintf(stderr, " import %s", the->nameBuffer);
			}
			if (!mxIsNull(aliases)) {
				txSlot* alias = aliases->value.reference->next;
				while (alias) {
					fxIDToString(the, alias->value.symbol, the->nameBuffer, sizeof(the->nameBuffer));
					fprintf(stderr, " alias %s", the->nameBuffer);
					alias = alias->next;
				}
			}
			fprintf(stderr, "\n");
			transfer = transfer->next;
		}
	}
#endif
	
	mxPullSlot(mxModuleInstanceTransfers(instance));

	mxPushSlot(record);
	mxGetID(mxID(_needsImport));
	if (!mxIsUndefined(the->stack)) {
		if (fxToBoolean(the, the->stack)) {
			txSlot* internal = mxModuleInstanceInternal(instance);
			internal->flag |= XS_IMPORT_FLAG;
		}
	}
	mxPop(); // needsImport
	
	mxPushSlot(record);
	mxGetID(mxID(_needsImportMeta));
	if (!mxIsUndefined(the->stack)) {
		if (fxToBoolean(the, the->stack)) {
			txSlot* internal = mxModuleInstanceInternal(instance);
			internal->flag |= XS_IMPORT_META_FLAG;
		}
	}
	mxPop(); // _needsImportMeta
}

txBoolean fxMapModule(txMachine* the, txSlot* realm, txID moduleID, txSlot* module, txSlot* queue, txSlot* result)
{
	txSlot* moduleMap = mxModuleMap(realm);
	txSlot* descriptor = moduleMap->value.reference->next;
	while (descriptor) {
		if (descriptor->ID == moduleID) {
			fxMapModuleDescriptor(the, realm, moduleID, module, queue, result,descriptor);
			return 1;
		}
		descriptor = descriptor->next;
	}
	return 0;
}

void fxMapModuleDescriptor(txMachine* the, txSlot* realm, txID moduleID, txSlot* module, txSlot* queue, txSlot* result, txSlot* descriptor)
{
	txSlot* property;
	if (!mxIsReference(descriptor))
		mxTypeError("descriptor is no object");

	mxPushSlot(descriptor);
	mxGetID(fxID(the, "namespace"));
	property = the->stack;
	if (!mxIsUndefined(property)) {
		if ((property->kind == XS_STRING_KIND) || (property->kind == XS_STRING_X_KIND)) {
			txSlot* aliasRealm;
			txSlot* aliasOwn;
			txID aliasModuleID;
			txSlot* aliasModule;
			mxPushSlot(descriptor);
			mxGetID(fxID(the, "compartment"));
			if (!mxIsUndefined(the->stack)) {
				txSlot* program = fxCheckCompartmentInstance(the, the->stack);
				aliasRealm = mxModuleInstanceInternal(program)->value.module.realm;
			}
			else {
				aliasRealm = mxRealmParent(realm)->value.reference;
			}
			aliasOwn = mxOwnModules(aliasRealm)->value.reference;
			mxPop();
			aliasModuleID = fxResolveSpecifier(the, aliasRealm, XS_NO_ID, property);
	//             if ((aliasRealm == realm) && (aliasModuleID == moduleID))
	//                 mxTypeError("descriptor.specifier is circular");
			
			aliasModule = mxBehaviorGetProperty(the, aliasOwn, aliasModuleID, 0, XS_ANY);
			if (aliasModule) {
				property = aliasModule;
			}
			else {
				property = mxBehaviorSetProperty(the, aliasOwn, aliasModuleID, 0, XS_OWN);
				fxNewModule(the, aliasRealm, aliasModuleID, property);
			}
			goto namespace;
		}
		else {
			txSlot* instance = module->value.reference;
			if (!mxIsReference(property))
				mxTypeError("descriptor.namespace is no object");
			if (mxIsModule(property->value.reference))
				goto namespace;
			fxLoadVirtualModuleNamespace(the, property->value.reference, instance);
			txID status = mxModuleInstanceStatus(instance);
			if ((status == XS_MODULE_STATUS_NEW) || (status == XS_MODULE_STATUS_LOADING)) {
				mxModuleInstanceStatus(instance) = XS_MODULE_STATUS_EXECUTED;
				if (result) {
			
				}
				else {
					fxCompleteModule(the, instance, C_NULL);
					fxRunImportFulfilled(the, instance, instance);
				}
			}
			goto done;
		}
		
namespace:
		if (module->kind == XS_UNDEFINED_KIND) {
			module->kind = property->kind;
			module->value = property->value;
		}
		else {
			txID status = mxModuleStatus(module);
			mxCheck(the, (status == XS_MODULE_STATUS_NEW) || (status == XS_MODULE_STATUS_LOADING));
			mxPushSlot(module);
			fxOverrideModule(the, queue, result, module->value.reference, property->value.reference);
			status = mxModuleStatus(property);
			if (result) {
				if (status == XS_MODULE_STATUS_ERROR) {
					mxPushSlot(mxModuleMeta(property));
					mxPull(mxException);
					fxJump(the);
				}
				else if (status == XS_MODULE_STATUS_EXECUTED) {
				}
				else {
					fxQueueModule(the, queue, property);
				}
			}
			else {
				if (status == XS_MODULE_STATUS_ERROR) {
					fxCompleteModule(the, module->value.reference, C_NULL);
					fxRunImportRejected(the, module->value.reference, property->value.reference);
				}
				else if (status == XS_MODULE_STATUS_EXECUTED) {
					fxCompleteModule(the, module->value.reference, C_NULL);
					fxRunImportFulfilled(the, module->value.reference, property->value.reference);
				}
				else {
					txSlot* srcSlot = mxModuleFulfill(module);
					txSlot* dstSlot = fxLastProperty(the, property->value.reference);
					while (srcSlot) {
						dstSlot = fxNextSlotProperty(the, dstSlot, srcSlot, XS_NO_ID, XS_NO_FLAG);
						srcSlot = srcSlot->next;
					}
					fxQueueModule(the, queue, property);
				}
			}
			mxPop();
		}
		goto done;
	}
	mxPop(); // property;

	if (module->kind == XS_UNDEFINED_KIND)
		fxNewModule(the, realm, moduleID, module);
	else {
		txID status = mxModuleStatus(module);
		mxCheck(the, (status == XS_MODULE_STATUS_NEW) || (status == XS_MODULE_STATUS_LOADING));
	}
	
	mxPushSlot(descriptor);
	mxGetID(fxID(the, "archive"));
	property = the->stack;
	if (!mxIsUndefined(property)) {
		void* archive = fxGetHostData(the, property);
		txString path;
		void* code;
		size_t size;
		txScript script;
		mxPushSlot(descriptor);
		mxGetID(fxID(the, "path"));
		property = the->stack;
		path = fxToString(the, property);
		code = fxGetArchiveCode(the, archive, path, &size);
		if (code == C_NULL)
			mxURIError("module not found: %s", path);
		mxPop();
		script.callback = NULL;
		script.symbolsBuffer = NULL;
		script.symbolsSize = 0;
		script.codeBuffer = code;
		script.codeSize = (txSize)size;
		script.hostsBuffer = NULL;
		script.hostsSize = 0;
		script.path = path;
		script.version[0] = XS_MAJOR_VERSION;
		script.version[1] = XS_MINOR_VERSION;
		script.version[2] = XS_PATCH_VERSION;
		script.version[3] = 0;
		fxRunScript(the, &script, module, C_NULL, C_NULL, C_NULL, module->value.reference);

// 		mxPushClosure(module);
		mxPop();
// 		mxPop(); // path
		if (mxModuleExecute(module)->kind == XS_NULL_KIND)
			mxTypeError("no module");
		goto importMeta;
	}
	mxPop(); // property;
	
	mxPushSlot(descriptor);
	mxGetID(fxID(the, "source"));
	property = the->stack;
	if (!mxIsUndefined(property)) {
		if ((property->kind == XS_STRING_KIND) || (property->kind == XS_STRING_X_KIND)) {
			txSlot* loader = mxModuleLoader(module);
			loader->value.module.realm = mxRealmParent(realm)->value.reference;
			if (property->kind == XS_STRING_X_KIND)
				loader->value.module.id = fxNewNameX(the, property->value.string);
			else
				loader->value.module.id = fxNewName(the, property);
			mxModuleStatus(module) = XS_MODULE_STATUS_NEW;
			goto done;
		}
		if (!mxIsReference(property))
			mxTypeError("descriptor.source is no object");
		if (mxIsModuleSource(property->value.reference))
			fxDuplicateModuleTransfers(the, property, module);
		else
// 			mxTypeError("descriptor.source is object");
			fxLoadVirtualModuleSource(the, property, module->value.reference);
		goto importMeta;
	}
	mxPop(); // property
	
	mxTypeError("invalid descriptor");
	
importMeta:
	mxPop(); // property
	mxModuleStatus(module) = XS_MODULE_STATUS_LOADED;
	
	mxPushSlot(descriptor);
	mxGetID(fxID(the, "importMeta"));
	property = the->stack;
	if (!mxIsUndefined(property)) {
		if (!mxIsReference(property))
			mxTypeError("descriptor.importMeta is no object");
		txSlot* meta = mxModuleMeta(module);
		meta->value.reference->flag &= ~XS_DONT_PATCH_FLAG;
		mxPushUndefined();
		mxPush(mxAssignObjectFunction);
		mxCall();
		mxPushSlot(meta);
		mxPushSlot(property);
		mxRunCount(2);
		mxPop();
		meta->value.reference->flag |= XS_DONT_PATCH_FLAG;
	}
	mxPop(); // property
	
	mxPushSlot(descriptor);
	mxGetID(fxID(the, "specifier"));
	property = the->stack;
	if (!mxIsUndefined(property)) {
		txSlot* internal = mxModuleInternal(module);
		fxToString(the, property);
		if (property->kind == XS_STRING_X_KIND)
			internal->value.module.id = fxNewNameX(the, property->value.string);
		else
			internal->value.module.id = fxNewName(the, property);
	}
	
	goto done;
	
done:
	mxPop(); // property
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
	/* LOADER */
	slot = fxNextNullProperty(the, slot, XS_NO_ID, XS_INTERNAL_FLAG);
	slot->kind = XS_MODULE_KIND;
	slot->value.module.realm = realm;
	slot->value.module.id = moduleID;	
	module->kind = the->stack->kind;
	module->value = the->stack->value;
	mxPop();
#if mxInstrument
	the->loadedModulesCount++;
#endif
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

void fxOverrideModule(txMachine* the, txSlot* queue, txSlot* result, txSlot* module, txSlot* record)
{
	txSlot* realm = mxModuleInstanceInternal(module)->value.module.realm;
	txSlot** address = &(queue->next);
	txSlot* slot;
	while ((slot = *address)) {
		if (slot->value.reference == module) {
//             slot->value.reference = record;
			*address = slot->next;
		}
		else {
			if (mxModuleStatus(slot) == XS_MODULE_STATUS_LINKING) {
				txSlot* transfer = mxModuleTransfers(slot)->value.reference->next;
				while (transfer) {
					txSlot* from = mxTransferFrom(transfer);
					if ((from->kind == XS_REFERENCE_KIND) && (from->value.reference == module))
						from->value.reference = record;
					transfer = transfer->next;
				}
			}
		}
		
		address = &(slot->next);
	}
	slot = mxOwnModules(realm)->value.reference->next;
	while (slot) {
		if (slot->value.reference == module) {
			slot->value.reference = record;
		}
		slot = slot->next;
	}
	if (result) {
		if (result->value.reference == module) {
			result->value.reference = record;
		}
	}
}

void fxPrepareModule(txMachine* the, txFlag flag)
{
	txSlot* module = mxFunctionInstanceHome(mxFunction->value.reference)->value.home.module;
	txInteger c = the->stack->value.integer, i;
	txSlot* argument = the->stack + c;
	txSlot* result = the->stack + c;
	txSlot* slot;
	txSlot* property;
	property = mxModuleInstanceInternal(module);	
	property->flag |= flag;
	slot = argument--;
	property = mxModuleInstanceInitialize(module);	
	property->kind = slot->kind;
	property->value = slot->value;
	slot = argument--;
	property = mxModuleInstanceExecute(module);	
	property->kind = slot->kind;
	property->value = slot->value;
	mxFunctionInstanceCode(slot->value.reference)->ID = mxModuleInstanceInternal(module)->value.module.id;
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
// 		mxPushClosure(module);
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
				fxNewModule(the, realm, moduleID, module);
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
				txSlot* queue = mxModuleQueue.value.reference;
				slot = fxLastProperty(the, module->value.reference);
				slot = fxNextSlotProperty(the, slot, fulfillFunction, XS_NO_ID, XS_NO_FLAG);
				slot = fxNextSlotProperty(the, slot, rejectFunction, XS_NO_ID, XS_NO_FLAG);
// 				if ((status == XS_MODULE_STATUS_NEW) || (status == XS_MODULE_STATUS_LOADED)) {
// 					txSlot* queue = fxNewInstance(the);
					if (fxQueueModule(the, queue, module))
						fxLoadModules(the, queue);
// 				}
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

void fxRunImportFulfilled(txMachine* the, txSlot* module, txSlot* with)
{
	if (mxModuleInstanceMeta(module)->next) {
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
			mxPushReference(with);
			mxRunCount(1);
			mxPop();
		}
		the->stack = stack;
	}
}

void fxRunImportRejected(txMachine* the, txSlot* module, txSlot* with)
{
	if (mxModuleInstanceMeta(module)->next) {
		txSlot* stack = the->stack;
		txSlot* slot = mxModuleInstanceFulfill(module);
		txSlot* exception;
		while (slot) {
			slot = slot->next;
			mxPushSlot(slot);
			slot = slot->next;
		}
		mxModuleInstanceMeta(module)->next = C_NULL;
		exception = mxModuleInstanceMeta(with);
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
			if (defaultFlag & XS_IMPORT_ASYNC) {
				gxDefaults.runImport(the, realm, XS_NO_ID);
			}
			else {
				fxRunImportNow(the, realm, XS_NO_ID);
				if (defaultFlag & XS_IMPORT_DEFAULT) {
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
		fxNewModule(the, realm, moduleID, module);
	}
	status = mxModuleStatus(module);
	if ((status == XS_MODULE_STATUS_NEW) || (status == XS_MODULE_STATUS_LOADED)) {
		txSlot* result = stack;
		txSlot* queue = fxNewInstance(the);
		mxTry(the) {
			txBoolean done = 1;
			result->kind = module->kind;
			result->value = module->value;

			slot = mxBehaviorSetProperty(the, queue, moduleID, 0, XS_OWN);;
			slot->flag |= XS_BASE_FLAG;
			slot->kind = XS_REFERENCE_KIND;
			slot->value.reference = module->value.reference;
		
			mxReportModuleQueue("LOAD");
			module = queue->next;
			while (module) {
				if (mxModuleStatus(module) == XS_MODULE_STATUS_NEW) {
					txSlot* loader = mxModuleLoader(module);
					txID moduleID = loader->value.module.id;
					txSlot* realm = loader->value.module.realm;
					if (!fxMapModule(the, realm, moduleID, module, queue, result)) {
						txSlot* loadNowHook = mxLoadNowHook(realm);
						if (mxIsUndefined(loadNowHook)) {
							if (realm != mxModuleInstanceInternal(mxProgram.value.reference)->value.module.realm)
								mxTypeError("no loadNowHook");
							fxLoadModule(the, module, moduleID);
							if (mxModuleExecute(module)->kind == XS_NULL_KIND)
								mxTypeError("no module");
							mxModuleStatus(module) = XS_MODULE_STATUS_LOADED;
						}
						else {
							txSlot* descriptor;
							txSlot* internal = mxModuleInternal(module);
							moduleID = internal->value.module.id;
							realm = internal->value.module.realm;
							done = 0;
							mxModuleStatus(module) = XS_MODULE_STATUS_LOADING;
							mxPushUndefined();
							mxPushSlot(loadNowHook);
							mxCall();
							fxPushKeyString(the, moduleID);
							mxRunCount(1);
							descriptor = the->stack;
							fxMapModuleDescriptor(the, realm, moduleID, module, queue, result, descriptor);
							mxPop(); // descriptor
						}
					}
					module = queue;
					done = 1;
					mxReportModuleQueue("LOAD");
				}
				else if (mxModuleStatus(module) == XS_MODULE_STATUS_LOADING) {
					done = 0;
				}
				else if (mxModuleStatus(module) == XS_MODULE_STATUS_LOADED) {
					fxLoadModulesFrom(the, queue, module->value.reference, 1);
					mxModuleStatus(module) = XS_MODULE_STATUS_LINKING;
					module = queue;
					done = 1;
				}
				module = module->next;
			}
			if (!done)
				mxTypeError("async queue");
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

void fxModuleGetter(txMachine* the) {
	mxReferenceError("not initialized yet");
}

txBoolean fxModuleDefineOwnProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* slot, txFlag mask)
{
	txSlot* property = fxModuleFindProperty(the, instance, id, index);
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
		return (id == mxID(_Symbol_toStringTag)) ? 0 : 1;
	}
// 	if (mxModuleInstanceStatus(instance) < XS_MODULE_STATUS_LINKED) {
// 		mxReferenceError("module not initialized yet");
// 	}
// 	else
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

txSlot* fxModuleFindProperty(txMachine* the, txSlot* instance, txID id, txIndex index)
{
	if (fxIsKeySymbol(the, id)) {
		if (id == mxID(_Symbol_toStringTag))
			return mxModulePrototype.value.reference->next;
	}
// 	if (mxModuleInstanceStatus(instance) < XS_MODULE_STATUS_LINKED) {
// 		mxReferenceError("module not initialized yet");
// 	}
// 	else
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

txBoolean fxModuleGetOwnProperty(txMachine* the, txSlot* instance, txID id, txIndex index, txSlot* slot)
{
	txSlot* property = fxModuleFindProperty(the, instance, id, index);
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
// 	if (mxModuleInstanceStatus(instance) < XS_MODULE_STATUS_LINKED) {
// 		return &mxModuleAccessor;
// 	}
// 	else
	{
		txSlot* exports = mxModuleInstanceExports(instance);
		if (mxIsReference(exports)) {
			txSlot* property = exports->value.reference->next;
			while (property) {
				if (property->ID == id) {
					property = property->value.export.closure;
					if (property && (property->kind == XS_UNINITIALIZED_KIND)) {
						return &mxModuleAccessor;
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
	txSlot* property = fxModuleFindProperty(the, instance, id, index);
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
		return (id == mxID(_Symbol_toStringTag)) ? 1 : 0;
	}
// 	if (mxModuleInstanceStatus(instance) < XS_MODULE_STATUS_LINKED) {
// 		mxReferenceError("module not initialized yet");
// 	}
// 	else
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
// 		if (mxModuleInstanceStatus(instance) < XS_MODULE_STATUS_LINKED) {
// 			mxReferenceError("module not initialized yet");
// 		}
// 		else
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

void fxPrepareCompartmentFunction(txMachine* the, txSlot* program, txSlot* instance)
{
	txSlot* property = mxFunctionInstanceHome(instance);
	property->value.home.module = program;
	if (mxCompartmentGlobal.kind != XS_UNDEFINED_KIND) {
		instance->flag |= XS_DONT_PATCH_FLAG;
		property = property->next;
		while (property) {
			if (!(property->flag & XS_INTERNAL_FLAG))
				property->flag |= XS_DONT_DELETE_FLAG | XS_DONT_SET_FLAG;
			property = property->next;
		}
	}
}

void fx_Compartment(txMachine* the)
{
	txSlot* module = mxFunctionInstanceHome(mxFunction->value.reference)->value.home.module;
	txSlot* program = C_NULL;
	txSlot* parent;
	txSlot* realm;
	txSlot* global = C_NULL;
	txSlot* closures = C_NULL;
	txSlot* slot;
	txSlot* target;
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
		parent = mxModuleInstanceInternal(module)->value.module.realm;
		mxPushReference(parent);
		
		// GLOBALS
		if (the->sharedMachine == C_NULL) {
			txSlot* instance;
			txSlot* property;
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
				txSlot* item = mxCompartmentGlobal.value.reference->next;
				for (id = XS_SYMBOL_ID_COUNT; id < _Infinity; id++) {
					mxPushSlot(item->value.array.address + id);
					slot = fxNextSlotProperty(the, slot, the->stack, mxID(id), XS_DONT_ENUM_FLAG);
					mxPop();
				}
				for (; id < _Compartment; id++) {
					mxPushSlot(item->value.array.address + id);
					slot = fxNextSlotProperty(the, slot, the->stack, mxID(id), XS_GET_ONLY);
					mxPop();
				}
			}
			
			instance = fxBuildHostFunction(the, mxCallback(fx_Compartment), 1, mxID(_Compartment));
			instance->flag |= XS_CAN_CONSTRUCT_FLAG;
			property = fxLastProperty(the, instance);
			fxNextSlotProperty(the, property, &mxCompartmentPrototype, mxID(_prototype), XS_GET_ONLY);
			fxPrepareCompartmentFunction(the, program, instance);
			slot = fxNextSlotProperty(the, slot, the->stack, mxID(_Compartment), XS_DONT_ENUM_FLAG);
			mxPop();
			
			instance = fxBuildHostFunction(the, mxCallback(fx_Function), 1, mxID(_Function));
			instance->flag |= XS_CAN_CONSTRUCT_FLAG;
			property = fxLastProperty(the, instance);
			fxNextSlotProperty(the, property, &mxFunctionPrototype, mxID(_prototype), XS_GET_ONLY);
			fxPrepareCompartmentFunction(the, program, instance);
			slot = fxNextSlotProperty(the, slot, the->stack, mxID(_Function), XS_DONT_ENUM_FLAG);
			mxPop();
			
			instance = fxBuildHostFunction(the, mxCallback(fx_eval), 1, mxID(_eval));
			fxPrepareCompartmentFunction(the, program, instance);
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
			if (!mxIsReference(mxArgv(0)))
				mxTypeError("invalid options");
		
			mxPushSlot(mxArgv(0));
			if (mxHasID(fxID(the, "globals"))) {
				mxPushSlot(mxArgv(0));
				mxGetID(fxID(the, "globals"));
				slot = the->stack;
				if (!mxIsReference(slot))
					mxTypeError("options.globals is no object");
				mxPushUndefined();
				mxPush(mxAssignObjectFunction);
				mxCall();
				mxPushReference(global);
				mxPushSlot(slot);
				mxRunCount(2);
				mxPop();
				mxPop(); // globals
			}
			
			target = fxNewInstance(the);
			own = fxNewInstance(the);
			mxPushSlot(mxArgv(0));
			if (mxHasID(fxID(the, "modules"))) {
				txSlot* source;
				txSlot* at;
				txSlot* temporary;
				mxPushSlot(mxArgv(0));
				mxGetID(fxID(the, "modules"));
				if (!mxIsReference(the->stack))
					mxTypeError("options.modules is no object");
				source = the->stack->value.reference;
				at = fxNewInstance(the);
				mxBehaviorOwnKeys(the, source, XS_EACH_NAME_FLAG, at);
				mxTemporary(temporary);
				while ((at = at->next)) {
					if (mxBehaviorGetOwnProperty(the, source, at->value.at.id, at->value.at.index, temporary) && !(temporary->flag & XS_DONT_ENUM_FLAG)) {
						txSlot* descriptor;
						txSlot* property;
						mxPushReference(source);
						mxGetAll(at->value.at.id, at->value.at.index);
						descriptor = the->stack;
						property = mxBehaviorSetProperty(the, target, at->value.at.id, at->value.at.index, XS_OWN);
						property->kind = descriptor->kind;
						property->value = descriptor->value;
						mxPop(); // descriptor
					}
				}
				mxPop(); // temporary
				mxPop(); // at
				mxPop(); // modules
			}
			
			mxPushUndefined();
			closures = fxNewEnvironmentInstance(the, C_NULL);
			mxPushSlot(mxArgv(0));
			if (mxHasID(fxID(the, "globalLexicals"))) {
				txSlot* target = fxLastProperty(the, closures);
				txSlot* source;
				txSlot* at;
				txSlot* property;
				mxPushSlot(mxArgv(0));
				mxGetID(fxID(the, "globalLexicals"));
				if (!mxIsReference(the->stack))
					mxTypeError("options.globalLexicals is no object");
				source = the->stack->value.reference;
				at = fxNewInstance(the);
				mxBehaviorOwnKeys(the, source, XS_EACH_NAME_FLAG, at);
				mxTemporary(property);
				while ((at = at->next)) {
					if ((at->value.at.id != XS_NO_ID) && mxBehaviorGetOwnProperty(the, source, at->value.at.id, at->value.at.index, property) && !(property->flag & XS_DONT_ENUM_FLAG)) {
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
				mxPop(); // globalLexicals
			}
	
			mxPushSlot(mxArgv(0));
			if (mxHasID(fxID(the, "resolveHook"))) {
				mxPushSlot(mxArgv(0));
				mxGetID(fxID(the, "resolveHook"));
				if (!fxIsCallable(the, the->stack))
					mxTypeError("options.resolveHook is no function");
			}
			else
				mxPushUndefined();
			
			mxPushUndefined(); // moduleMapHook;
			
			mxPushSlot(mxArgv(0));
			if (mxHasID(fxID(the, "loadHook"))) {
				mxPushSlot(mxArgv(0));
				mxGetID(fxID(the, "loadHook"));
				if (!fxIsCallable(the, the->stack))
					mxTypeError("options.loadHook is no function");
			}
			else
				mxPushUndefined();
				
			mxPushSlot(mxArgv(0));
			if (mxHasID(fxID(the, "loadNowHook"))) {
				mxPushSlot(mxArgv(0));
				mxGetID(fxID(the, "loadNowHook"));
				if (!fxIsCallable(the, the->stack))
					mxTypeError("options.loadNowHook is no function");
			}
			else
				mxPushUndefined();
			
			mxPushUndefined(); // importMetaHook;
		}
		else {
			target = fxNewInstance(the);
			own = fxNewInstance(the);
	
			mxPushUndefined();
			closures = fxNewEnvironmentInstance(the, C_NULL);

			mxPushUndefined();
			mxPushUndefined();
			mxPushUndefined();
			mxPushUndefined();
			mxPushUndefined();
		}
		
		realm = fxNewRealmInstance(the);
		mxModuleInstanceInternal(program)->value.module.realm = realm;
		
		slot = own->next;
		while (slot) {
			txSlot* internal = mxModuleInternal(slot);
			if (internal->value.module.realm == C_NULL) {
				if (!(internal->flag & XS_MARK_FLAG))
					internal->value.module.realm = realm;
			}
			slot = slot->next;
		}
		
		
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

txSlot* fxCheckModuleSourceInstance(txMachine* the, txSlot* slot)
{
	if (slot->kind == XS_REFERENCE_KIND) {
		txSlot* instance = slot->value.reference;
		if (((slot = instance->next)) && (slot->flag & XS_INTERNAL_FLAG) && (slot->kind == XS_MODULE_SOURCE_KIND)) {
			return instance;
		}
	}
	mxTypeError("this is no ModuleSource instance");
	return C_NULL;
}

txSlot* fxNewModuleSourceInstance(txMachine* the)
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
	slot->kind = XS_MODULE_SOURCE_KIND;
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
 	/* LOADER */
	slot = fxNextNullProperty(the, slot, XS_NO_ID, XS_INTERNAL_FLAG);
	return instance;
}

void fxExecuteVirtualModuleSourceImport(txMachine* the)
{
	txSlot* instance = mxFunction->value.reference;
	txSlot* home = mxFunctionInstanceHome(instance);
	txSlot* module = home->value.home.module;
	txSlot* internal = mxModuleInstanceInternal(module);
	mxPushSlot(mxArgv(0));
	fxRunImport(the, internal->value.module.realm, internal->value.module.id);
	mxPullSlot(mxResult);
}

void fx_ModuleSource(txMachine* the)
{
	txSlot* instance;
	txSlot* slot;
	txStringStream stream;
	txScript* script;
	if (mxIsUndefined(mxTarget))
		mxTypeError("call: ModuleSource");
	mxPushSlot(mxTarget);
	fxGetPrototypeFromConstructor(the, &mxModuleSourcePrototype);
	instance = fxNewModuleSourceInstance(the);
	mxPullSlot(mxResult);
	if (mxArgc == 0)
		mxPushUndefined();
	else		
		mxPushSlot(mxArgv(0));
	slot = the->stack;
	stream.slot = slot;
	stream.offset = 0;
	stream.size = mxStringLength(fxToString(the, slot));
	script = fxParseScript(the, &stream, fxStringGetter, mxDebugFlag);
// 		mxPushClosure(mxResult);
	fxRunScript(the, script, mxResult, C_NULL, C_NULL, C_NULL, instance);
	mxPop();
	if (mxModuleInstanceExecute(instance)->kind == XS_NULL_KIND)
		mxTypeError("no module");
	mxPop();
	mxPop();
}

void fx_ModuleSource_prototype_get_bindings(txMachine* the)
{
	txSlot* record = fxCheckModuleSourceInstance(the, mxThis);
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
					mxPushString(from->value.string);
					slot = fxNextSlotProperty(the, slot, the->stack, fxID(the, "importAllFrom"), XS_NO_FLAG);
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
					mxPushString(from->value.string);
					slot = fxNextSlotProperty(the, slot, the->stack, mxID(_from), XS_NO_FLAG);
					mxPop();
				}
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
						mxPushString(from->value.string);
						slot = fxNextSlotProperty(the, slot, the->stack, fxID(the, "exportAllFrom"), XS_NO_FLAG);
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
						mxPushString(from->value.string);
						slot = fxNextSlotProperty(the, slot, the->stack, mxID(_from), XS_NO_FLAG);
						mxPop();
					}
					resultItem = resultItem->next = fxNewSlot(the);
					mxPullSlot(resultItem);
					resultArray->value.array.length++;
				}
				alias = alias->next;
			}
		}
		else if ((from->kind != XS_NULL_KIND) && (import->kind == XS_NULL_KIND)) {
			txSlot* slot = fxNewObject(the);
			mxPushString(from->value.string);
			slot = fxNextSlotProperty(the, slot, the->stack, fxID(the, "exportAllFrom"), XS_NO_FLAG);
			mxPop();
			resultItem = resultItem->next = fxNewSlot(the);
			mxPullSlot(resultItem);
			resultArray->value.array.length++;
		}
		transfer = transfer->next;
	}
	
	fxCacheArray(the, resultInstance);
}

void fx_ModuleSource_prototype_get_needsImport(txMachine* the)
{
	txSlot* record = fxCheckModuleSourceInstance(the, mxThis);
	txFlag flag = mxModuleInstanceInternal(record)->flag;
	mxResult->value.boolean = (flag & XS_IMPORT_FLAG) ? 1 : 0;
	mxResult->kind = XS_BOOLEAN_KIND;
}

void fx_ModuleSource_prototype_get_needsImportMeta(txMachine* the)
{
	txSlot* record = fxCheckModuleSourceInstance(the, mxThis);
	txFlag flag = mxModuleInstanceInternal(record)->flag;
	mxResult->value.boolean = (flag & XS_IMPORT_META_FLAG) ? 1 : 0;
	mxResult->kind = XS_BOOLEAN_KIND;
}
