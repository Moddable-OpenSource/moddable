/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Tools.
 * 
 *   The Moddable SDK Tools is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 * 
 *   The Moddable SDK Tools is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 * 
 *   You should have received a copy of the GNU General Public License
 *   along with the Moddable SDK Tools.  If not, see <http://www.gnu.org/licenses/>.
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

#include "xsl.h"

static txFlag fxIsLinkerSymbolUsed(txLinker* linker, txID id);
static void fxStripCallback(txLinker* linker, txCallback which);
static void fxStripClass(txLinker* linker, txMachine* the, txSlot* slot);
static void fxStripInstance(txLinker* linker, txMachine* the, txSlot* slot);
static void fxStripObject(txLinker* linker, txMachine* the, txSlot* slot);
static void fxUnstripCallback(txLinker* linker, txCallback which);
static void fxUnuseSymbol(txLinker* linker, txID id);

txFlag fxIsCallbackStripped(txLinker* linker, txCallback which)
{
	txLinkerCallback* linkerCallback = fxGetLinkerCallbackByAddress(linker, which);
	if (linkerCallback)
		return !linkerCallback->flag;
	return 1;
}

txFlag fxIsLinkerSymbolUsed(txLinker* linker, txID id)
{
	txLinkerSymbol* linkerSymbol = linker->symbolArray[id];
	return linkerSymbol->flag;
}

void fxStripCallback(txLinker* linker, txCallback which)
{
	txLinkerCallback* linkerCallback = fxGetLinkerCallbackByAddress(linker, which);
	if (linkerCallback)
		linkerCallback->flag = 0;
}

void fxStripCallbacks(txLinker* linker, txMachine* the)
{
	txLinkerStrip* linkerStrip;
	txLinkerBuilder* linkerBuilder;
	txID id;
	char buffer[1024];
	
	if (linker->stripFlag == XS_STRIP_EXPLICIT_FLAG) {
		fxUseCodes();
		id = 0;
		while (id < linker->symbolIndex) {
			linker->symbolArray[id]->flag = 1;
			id++;
		}
		fxUnstripCallbacks(linker);
	}
	else {
		linkerBuilder = linker->firstBuilder;
		while (linkerBuilder) {
			txID id = linkerBuilder->host.id;
			txLinkerSymbol* symbol = linker->symbolArray[id];
			if (symbol->flag)
				fxUnstripCallback(linker, linkerBuilder->host.callback);
			linkerBuilder = linkerBuilder->nextBuilder;
		}
		
		if (fxIsCodeUsed(XS_CODE_ARRAY))
			fxUnstripCallback(linker, fx_Array);
			
		if (fxIsCodeUsed(XS_CODE_ASYNC_FUNCTION)|| fxIsCodeUsed(XS_CODE_ASYNC_GENERATOR_FUNCTION) || fxIsCodeUsed(XS_CODE_IMPORT))
			fxUnstripCallback(linker, fx_Promise);
			
		if (fxIsCodeUsed(XS_CODE_ASYNC_GENERATOR_FUNCTION))
			fxUnstripCallback(linker, fx_AsyncIterator_asyncIterator);
			
		if (fxIsCodeUsed(XS_CODE_BIGINT_1) || fxIsCodeUsed(XS_CODE_BIGINT_2))
			fxUnstripCallback(linker, fx_BigInt);
			
		if (fxIsCodeUsed(XS_CODE_REGEXP))
			fxUnstripCallback(linker, fx_RegExp);
	}
	linkerStrip = linker->firstStrip;
	while (linkerStrip) {
		txString name = linkerStrip->name;
		if (!c_strchr(name, '.')) {
			if (!c_strcmp(name, "Atomics"))
				fxUnuseSymbol(linker, mxID(_Atomics));
			else if (!c_strcmp(name, "BigInt")) {
				fxStripCallback(linker, fx_BigInt);
				fxUnuseSymbol(linker, mxID(_BigInt64Array));
				fxStripCallback(linker, fx_BigInt64Array);
				fxUnuseSymbol(linker, mxID(_BigUint64Array));
				fxStripCallback(linker, fx_BigUint64Array);
				fxStripCallback(linker, fx_DataView_prototype_getBigInt64);
				fxStripCallback(linker, fx_DataView_prototype_getBigUint64);
				fxStripCallback(linker, fx_DataView_prototype_setBigInt64);
				fxStripCallback(linker, fx_DataView_prototype_setBigUint64);
				fxUnuseCode(XS_CODE_BIGINT_1);
				fxUnuseCode(XS_CODE_BIGINT_2);
			}
			else if (!c_strcmp(name, "BigInt64Array")) {
				fxUnuseSymbol(linker, mxID(_BigInt64Array));
				fxStripCallback(linker, fx_BigInt64Array);
			}
			else if (!c_strcmp(name, "BigUint64Array")) {
				fxUnuseSymbol(linker, mxID(_BigUint64Array));
				fxStripCallback(linker, fx_BigUint64Array);
			}
			else if (!c_strcmp(name, "Compartment"))
				fxStripCallback(linker, fx_Compartment);
			else if (!c_strcmp(name, "DataView"))
				fxStripCallback(linker, fx_DataView);
			else if (!c_strcmp(name, "Date")) {
				fxStripCallback(linker, fx_Date);
				fxStripCallback(linker, fx_Date_secure);
			}
			else if (!c_strcmp(name, "FinalizationRegistry"))
				fxStripCallback(linker, fx_FinalizationRegistry);
			else if (!c_strcmp(name, "Float32Array")) {
				fxUnuseSymbol(linker, mxID(_Float32Array));
				fxStripCallback(linker, fx_Float32Array);
			}
			else if (!c_strcmp(name, "Float64Array")) {
				fxUnuseSymbol(linker, mxID(_Float64Array));
				fxStripCallback(linker, fx_Float64Array);
			}
			else if (!c_strcmp(name, "Generator")) {
				fxStripCallback(linker, fx_Generator_prototype_next);
				fxStripCallback(linker, fx_Generator_prototype_return);
				fxStripCallback(linker, fx_Generator_prototype_throw);
				fxStripCallback(linker, fx_AsyncGenerator_prototype_next);
				fxStripCallback(linker, fx_AsyncGenerator_prototype_return);
				fxStripCallback(linker, fx_AsyncGenerator_prototype_throw);
				fxUnuseCode(XS_CODE_ASYNC_GENERATOR_FUNCTION);
				fxUnuseCode(XS_CODE_GENERATOR_FUNCTION);
				fxUnuseCode(XS_CODE_START_ASYNC_GENERATOR);
				fxUnuseCode(XS_CODE_START_GENERATOR);
			}
			else if (!c_strcmp(name, "Int8Array")) {
				fxUnuseSymbol(linker, mxID(_Int8Array));
				fxStripCallback(linker, fx_Int8Array);
			}
			else if (!c_strcmp(name, "Int16Array")) {
				fxUnuseSymbol(linker, mxID(_Int16Array));
				fxStripCallback(linker, fx_Int16Array);
			}
			else if (!c_strcmp(name, "Int32Array")) {
				fxUnuseSymbol(linker, mxID(_Int32Array));
				fxStripCallback(linker, fx_Int32Array);
			}
			else if (!c_strcmp(name, "JSON"))
				fxUnuseSymbol(linker, mxID(_JSON));
			else if (!c_strcmp(name, "Map"))
				fxStripCallback(linker, fx_Map);
			else if (!c_strcmp(name, "Math"))
				fxUnuseSymbol(linker, mxID(_Math));
			else if (!c_strcmp(name, "ModuleSource")) {
				fxStripCallback(linker, fx_ModuleSource);
				fxStripCallback(linker, fx_ModuleSource_prototype_get_bindings);
				fxStripCallback(linker, fx_ModuleSource_prototype_get_needsImport);
				fxStripCallback(linker, fx_ModuleSource_prototype_get_needsImportMeta);
			}
			else if (!c_strcmp(name, "Promise")) {
				fxStripCallback(linker, fx_AsyncFromSyncIterator_prototype_next);
				fxStripCallback(linker, fx_AsyncFromSyncIterator_prototype_return);
				fxStripCallback(linker, fx_AsyncFromSyncIterator_prototype_throw);
				fxStripCallback(linker, fx_AsyncIterator_asyncIterator);
				fxStripCallback(linker, fx_AsyncGenerator_prototype_next);
				fxStripCallback(linker, fx_AsyncGenerator_prototype_return);
				fxStripCallback(linker, fx_AsyncGenerator_prototype_throw);
				fxStripCallback(linker, fx_Compartment_prototype_import);
				fxStripCallback(linker, fx_Promise);
				fxStripCallback(linker, fxOnRejectedPromise);
				fxStripCallback(linker, fxOnResolvedPromise);
				fxStripCallback(linker, fxOnThenable);
				fxUnuseCode(XS_CODE_ASYNC_FUNCTION);
				fxUnuseCode(XS_CODE_ASYNC_GENERATOR_FUNCTION);
				fxUnuseCode(XS_CODE_IMPORT);
				fxUnuseCode(XS_CODE_START_ASYNC);
				fxUnuseCode(XS_CODE_START_ASYNC_GENERATOR);
			}
			else if (!c_strcmp(name, "Proxy"))
				fxStripCallback(linker, fx_Proxy);
			else if (!c_strcmp(name, "Reflect"))
				fxUnuseSymbol(linker, mxID(_Reflect));
			else if (!c_strcmp(name, "RegExp")) {
				fxStripCallback(linker, fxInitializeRegExp);
				fxStripCallback(linker, fx_RegExp);
				fxStripCallback(linker, fx_String_prototype_match);
				fxStripCallback(linker, fx_String_prototype_matchAll);
				fxStripCallback(linker, fx_String_prototype_search);
			}
			else if (!c_strcmp(name, "Set"))
				fxStripCallback(linker, fx_Set);
			else if (!c_strcmp(name, "SharedArrayBuffer"))
				fxStripCallback(linker, fx_SharedArrayBuffer);
			else if (!c_strcmp(name, "Uint8Array")) {
				fxUnuseSymbol(linker, mxID(_Uint8Array));
				fxStripCallback(linker, fx_Uint8Array);
			}
			else if (!c_strcmp(name, "Uint16Array")) {
				fxUnuseSymbol(linker, mxID(_Uint16Array));
				fxStripCallback(linker, fx_Uint16Array);
			}
			else if (!c_strcmp(name, "Uint32Array")) {
				fxUnuseSymbol(linker, mxID(_Uint32Array));
				fxStripCallback(linker, fx_Uint32Array);
			}
			else if (!c_strcmp(name, "Uint8ClampedArray")) {
				fxUnuseSymbol(linker, mxID(_Uint8ClampedArray));
				fxStripCallback(linker, fx_Uint8ClampedArray);
			}
			else if (!c_strcmp(name, "WeakMap"))
				fxStripCallback(linker, fx_WeakMap);
			else if (!c_strcmp(name, "WeakRef"))
				fxStripCallback(linker, fx_WeakRef);
			else if (!c_strcmp(name, "WeakSet"))
				fxStripCallback(linker, fx_WeakSet);
			else if (!c_strcmp(name, "eval")) {
				fxStripCallback(linker, fx_Compartment_prototype_evaluate);
				fxStripCallback(linker, fx_Function);
				fxStripCallback(linker, fx_ModuleSource);
				fxStripCallback(linker, fx_eval);
				fxUnuseCode(XS_CODE_ARGUMENTS_SLOPPY);
				fxUnuseCode(XS_CODE_EVAL);
				fxUnuseCode(XS_CODE_EVAL_TAIL);
				fxUnuseCode(XS_CODE_PROGRAM_ENVIRONMENT);
			}
		}
		else {
			txString q = buffer;
			char c;
			*q++ = 'f';
			*q++ = 'x';
			*q++ = '_';
			while ((c = *name++)) {
				*q++ = (c == '.') ? '_' : c;
			}
			*q = 0;
			fxStripName(linker, buffer);
		}
		linkerStrip = linkerStrip->nextStrip;
	}
	
	fxStripCallback(linker, fx_AsyncFunction);
	fxStripCallback(linker, fx_AsyncGeneratorFunction);
	fxStripCallback(linker, fx_GeneratorFunction);
	
	fxUnstripCallback(linker, fx_Array_prototype_join);
	fxUnstripCallback(linker, fx_Array_prototype_values);
	fxUnstripCallback(linker, fx_Array_prototype_toString);
	fxUnstripCallback(linker, fx_ArrayBuffer);
	fxUnstripCallback(linker, fx_ArrayIterator_prototype_next);
	fxUnstripCallback(linker, fx_AsyncGenerator);
	fxUnstripCallback(linker, fx_Boolean_prototype_toString);
	fxUnstripCallback(linker, fx_Boolean_prototype_valueOf);
	fxUnstripCallback(linker, fx_Enumerator);
	fxUnstripCallback(linker, fx_Enumerator_next);
	fxUnstripCallback(linker, fx_Error_toString);
	fxUnstripCallback(linker, fx_Function_prototype_hasInstance);
	fxUnstripCallback(linker, fx_Function_prototype_toString);
	fxUnstripCallback(linker, fx_Generator);
	fxUnstripCallback(linker, fx_Iterator_iterator);
	fxUnstripCallback(linker, fx_Number_prototype_toString);
	fxUnstripCallback(linker, fx_Number_prototype_valueOf);
	fxUnstripCallback(linker, fx_Object_assign);
	fxUnstripCallback(linker, fx_Object_copy);
	fxUnstripCallback(linker, fx_Object_prototype_toString);
	fxUnstripCallback(linker, fx_Object_prototype_valueOf);
	fxUnstripCallback(linker, fx_String_prototype_iterator);
	fxUnstripCallback(linker, fx_String_prototype_iterator_next);
	fxUnstripCallback(linker, fx_String_prototype_valueOf);
	fxUnstripCallback(linker, fx_Symbol_prototype_toString);
	fxUnstripCallback(linker, fx_Symbol_prototype_toPrimitive);
	fxUnstripCallback(linker, fx_Symbol_prototype_valueOf);
	fxUnstripCallback(linker, fx_TypedArray_prototype_join);
	fxUnstripCallback(linker, fx_TypedArray_prototype_values);
	
	fxUnstripCallback(linker, fxOrdinaryToPrimitive);
	
	fxUnstripCallback(linker, fxArrayLengthGetter);
	fxUnstripCallback(linker, fxArrayLengthSetter);
	fxUnstripCallback(linker, fxModuleGetter);
	fxUnstripCallback(linker, fxStringAccessorGetter);
	fxUnstripCallback(linker, fxStringAccessorSetter);
	fxUnstripCallback(linker, fxThrowTypeError);
	fxUnstripCallback(linker, fxTypedArrayGetter);
	fxUnstripCallback(linker, fxTypedArraySetter);
	
	if (!fxIsLinkerSymbolUsed(linker, mxID(_Atomics)))
		fxStripObject(linker, the, &mxAtomicsObject);
	if (fxIsCallbackStripped(linker, fx_BigInt))
		fxStripClass(linker, the, &mxBigIntConstructor);
	else {
		fxUnstripCallback(linker, fx_BigInt_prototype_toString);
		fxUnstripCallback(linker, fx_BigInt_prototype_valueOf);
	}
	if (fxIsCallbackStripped(linker, fx_Compartment))
		fxStripClass(linker, the, &mxCompartmentConstructor);
	if (fxIsCallbackStripped(linker, fx_DataView))
		fxStripClass(linker, the, &mxDataViewConstructor);
	if (fxIsCallbackStripped(linker, fx_Date))
		fxStripClass(linker, the, &mxDateConstructor);
	else {
		fxUnstripCallback(linker, fx_Date_parse);
		fxUnstripCallback(linker, fx_Date_prototype_toString);
		fxUnstripCallback(linker, fx_Date_prototype_toPrimitive);
		fxUnstripCallback(linker, fx_Date_prototype_valueOf);
	}
	if (fxIsCallbackStripped(linker, fx_FinalizationRegistry)) {
		fxStripClass(linker, the, &mxFinalizationRegistryConstructor);
	}
	if (fxIsLinkerSymbolUsed(linker, mxID(_bind)))
		fxUnstripCallback(linker, fx_Function_prototype_bound);
	if (!fxIsLinkerSymbolUsed(linker, mxID(_JSON)))
		fxStripObject(linker, the, &mxJSONObject);
	if (fxIsCallbackStripped(linker, fx_Map)) {
		fxStripClass(linker, the, &mxMapConstructor);
		fxStripInstance(linker, the, mxMapIteratorPrototype.value.reference);
	}
	else {
		fxUnstripCallback(linker, fx_Map_prototype_entries);
		fxUnstripCallback(linker, fx_Map_prototype_set);
		fxUnstripCallback(linker, fx_MapIterator_prototype_next);
	}
	if (!fxIsLinkerSymbolUsed(linker, mxID(_Math)))
		fxStripObject(linker, the, &mxMathObject);
	if (fxIsLinkerSymbolUsed(linker, mxID(_freeze)))
		fxUnstripCallback(linker, fx_Object_isFrozen);
	if (fxIsCallbackStripped(linker, fx_Promise))
		fxStripClass(linker, the, &mxPromiseConstructor);
	else {
		fxUnstripCallback(linker, fx_Promise_prototype_then);
		fxUnstripCallback(linker, fx_Promise_resolve);
		fxUnstripCallback(linker, fxOnRejectedPromise);
		fxUnstripCallback(linker, fxOnResolvedPromise);
		fxUnstripCallback(linker, fxOnThenable);
	}
	if (!fxIsLinkerSymbolUsed(linker, mxID(_Reflect)))
		fxStripObject(linker, the, &mxReflectObject);
	if (fxIsCallbackStripped(linker, fx_Proxy))
		fxStripObject(linker, the, &mxProxyConstructor);
	else {
		fxUnstripCallback(linker, fxProxyGetter);
		fxUnstripCallback(linker, fxProxySetter);
		fxUnstripCallback(linker, fx_Proxy_revoke);
	}
	{
		txFlag match = fxIsCallbackStripped(linker, fx_String_prototype_match);
		txFlag matchAll = fxIsCallbackStripped(linker, fx_String_prototype_matchAll);
		txFlag search = fxIsCallbackStripped(linker, fx_String_prototype_search);
		if (fxIsCallbackStripped(linker, fx_RegExp)) {
			if (match && matchAll && search) {
				fxStripClass(linker, the, &mxRegExpConstructor);
				fxStripInstance(linker, the, mxRegExpStringIteratorPrototype.value.reference);
			}
		}
		else {
			fxUnstripCallback(linker, fx_RegExp_prototype_get_dotAll);
			fxUnstripCallback(linker, fx_RegExp_prototype_get_global);
			fxUnstripCallback(linker, fx_RegExp_prototype_get_hasIndices);
			fxUnstripCallback(linker, fx_RegExp_prototype_get_flags);
			fxUnstripCallback(linker, fx_RegExp_prototype_get_ignoreCase);
			fxUnstripCallback(linker, fx_RegExp_prototype_get_multiline);
			fxUnstripCallback(linker, fx_RegExp_prototype_get_source);
			fxUnstripCallback(linker, fx_RegExp_prototype_get_sticky);
			fxUnstripCallback(linker, fx_RegExp_prototype_get_unicode);
			fxUnstripCallback(linker, fx_RegExp_prototype_toString);
			fxUnstripCallback(linker, fxInitializeRegExp);
			if (!fxIsCallbackStripped(linker, fx_String_prototype_replace) || !fxIsCallbackStripped(linker, fx_String_prototype_replaceAll)) {
				fxUnstripCallback(linker, fx_RegExp_prototype_get_global);
				fxUnstripCallback(linker, fx_RegExp_prototype_get_unicode);
				fxUnstripCallback(linker, fx_RegExp_prototype_replace);
				fxUnstripCallback(linker, fx_RegExp_prototype_exec);
			}
			if (!fxIsCallbackStripped(linker, fx_String_prototype_split)) {
				fxUnstripCallback(linker, fx_RegExp_prototype_split);
				fxUnstripCallback(linker, fx_RegExp_prototype_exec);
			}
			if (!fxIsCallbackStripped(linker, fx_RegExp_prototype_test)) {
				fxUnstripCallback(linker, fx_RegExp_prototype_exec);
			}
		}
		if (!match) {
			fxUnstripCallback(linker, fxInitializeRegExp);
			fxUnstripCallback(linker, fx_RegExp_prototype_match);
			fxUnstripCallback(linker, fx_RegExp_prototype_exec);
		}
		if (!matchAll) {
			fxUnstripCallback(linker, fxInitializeRegExp);
			fxUnstripCallback(linker, fx_RegExp_prototype_matchAll);
			fxUnstripCallback(linker, fx_RegExp_prototype_matchAll_next);
			fxUnstripCallback(linker, fx_RegExp_prototype_exec);
		}
		if (!search) {
			fxUnstripCallback(linker, fxInitializeRegExp);
			fxUnstripCallback(linker, fx_RegExp_prototype_search);
			fxUnstripCallback(linker, fx_RegExp_prototype_exec);
		}
	}	
	if (fxIsCallbackStripped(linker, fx_Set)) {
		fxStripClass(linker, the, &mxSetConstructor);
		fxStripInstance(linker, the, mxSetIteratorPrototype.value.reference);
	}
	else {
		fxUnstripCallback(linker, fx_Set_prototype_add);
		fxUnstripCallback(linker, fx_Set_prototype_values);
		fxUnstripCallback(linker, fx_SetIterator_prototype_next);
	}
	if (fxIsCallbackStripped(linker, fx_SharedArrayBuffer))
		fxStripClass(linker, the, &mxSharedArrayBufferConstructor);
	if (fxIsCallbackStripped(linker, fx_WeakMap))
		fxStripClass(linker, the, &mxWeakMapConstructor);
	if (fxIsCallbackStripped(linker, fx_WeakRef))
		fxStripClass(linker, the, &mxWeakRefConstructor);
	if (fxIsCallbackStripped(linker, fx_WeakSet))
		fxStripClass(linker, the, &mxWeakSetConstructor);
		
	fxUnstripCallback(linker, fx_species_get); //@@
}

void fxStripClass(txLinker* linker, txMachine* the, txSlot* slot)
{
	mxPushSlot(slot);
	fxStripInstance(linker, the, the->stack->value.reference);
	mxGetID(mxID(_prototype));
	fxStripInstance(linker, the, the->stack->value.reference);
	mxPop();
}

void fxStripDefaults(txLinker* linker, FILE* file)
{
	if (!fxIsCodeUsed(XS_CODE_START_ASYNC)) {
		fprintf(file, "static txSlot* fxNewAsyncInstanceDeadStrip(txMachine* the) { mxUnknownError(\"dead strip\"); return NULL; }\n");
		fprintf(file, "static void fxRunAsyncDeadStrip(txMachine* the, txSlot* slot) { mxUnknownError(\"dead strip\"); }\n");
	}
	if (!fxIsCodeUsed(XS_CODE_START_GENERATOR))
		fprintf(file, "static txSlot* fxNewGeneratorInstanceDeadStrip(txMachine* the) { mxUnknownError(\"dead strip\"); return NULL; }\n");
	if (!fxIsCodeUsed(XS_CODE_GENERATOR_FUNCTION))
		fprintf(file, "static txSlot* fxNewGeneratorFunctionInstanceDeadStrip(txMachine* the, txID name) { mxUnknownError(\"dead strip\"); return NULL; }\n");
	if (!fxIsCodeUsed(XS_CODE_START_ASYNC_GENERATOR))
		fprintf(file, "static txSlot* fxNewAsyncGeneratorInstanceDeadStrip(txMachine* the) { mxUnknownError(\"dead strip\"); return NULL; }\n");
	if (!fxIsCodeUsed(XS_CODE_ASYNC_GENERATOR_FUNCTION))
		fprintf(file, "static txSlot* fxNewAsyncGeneratorFunctionInstanceDeadStrip(txMachine* the, txID name) { mxUnknownError(\"dead strip\"); return NULL; }\n");
	if (!fxIsCodeUsed(XS_CODE_IMPORT) && fxIsCallbackStripped(linker, fx_Compartment_prototype_import))
		fprintf(file, "static void fxRunImportDeadStrip(txMachine* the, txSlot* realm, txID id) { mxUnknownError(\"dead strip\"); }\n");
	if (!fxIsCodeUsed(XS_CODE_NEW_PRIVATE_1) && !fxIsCodeUsed(XS_CODE_NEW_PRIVATE_2))
		fprintf(file, "static txBoolean fxDefinePrivatePropertyDeadStrip(txMachine* the, txSlot* instance, txSlot* check, txID id, txSlot* slot, txFlag mask) { mxUnknownError(\"dead strip\"); return 0; }\n");
	if (!fxIsCodeUsed(XS_CODE_GET_PRIVATE_1) && !fxIsCodeUsed(XS_CODE_GET_PRIVATE_2))
		fprintf(file, "static txSlot* fxGetPrivatePropertyDeadStrip(txMachine* the, txSlot* instance, txSlot* check, txID id) { mxUnknownError(\"dead strip\"); return NULL; }\n");
	if (!fxIsCodeUsed(XS_CODE_SET_PRIVATE_1) && !fxIsCodeUsed(XS_CODE_SET_PRIVATE_2))
		fprintf(file, "static txSlot* fxSetPrivatePropertyDeadStrip(txMachine* the, txSlot* instance, txSlot* check, txID id) { mxUnknownError(\"dead strip\"); return NULL; }\n");
		
	if (fxIsCallbackStripped(linker, fx_BigInt))
		fprintf(file, "static void fxBigIntDecodeDeadStrip(txMachine* the, txSize size) { mxUnknownError(\"dead strip\"); }\n");

	fprintf(file, "\nconst txDefaults ICACHE_FLASH_ATTR gxDefaults  = {\n");
	if (fxIsCodeUsed(XS_CODE_START_ASYNC)) {
		fprintf(file, "\tfxNewAsyncInstance,\n");
		fprintf(file, "\tfxRunAsync,\n");
	}
	else {
		fprintf(file, "\tfxNewAsyncInstanceDeadStrip,\n");
		fprintf(file, "\tfxRunAsyncDeadStrip,\n");
	}
	if (fxIsCodeUsed(XS_CODE_START_GENERATOR))
		fprintf(file, "\tfxNewGeneratorInstance,\n");
	else
		fprintf(file, "\tfxNewGeneratorInstanceDeadStrip,\n");
	if (fxIsCodeUsed(XS_CODE_GENERATOR_FUNCTION))
		fprintf(file, "\tfxNewGeneratorFunctionInstance,\n");
	else
		fprintf(file, "\tfxNewGeneratorFunctionInstanceDeadStrip,\n");
	if (fxIsCodeUsed(XS_CODE_START_ASYNC_GENERATOR))
		fprintf(file, "\tfxNewAsyncGeneratorInstance,\n");
	else
		fprintf(file, "\tfxNewAsyncGeneratorInstanceDeadStrip,\n");
	if (fxIsCodeUsed(XS_CODE_ASYNC_GENERATOR_FUNCTION))
		fprintf(file, "\tfxNewAsyncGeneratorFunctionInstance,\n");
	else
		fprintf(file, "\tfxNewAsyncGeneratorFunctionInstanceDeadStrip,\n");
	if (fxIsCodeUsed(XS_CODE_FOR_AWAIT_OF))
		fprintf(file, "\tfxRunForAwaitOf,\n");
	else
		fprintf(file, "\tfxDeadStrip,\n");
	if (fxIsCodeUsed(XS_CODE_ARGUMENTS_SLOPPY))
		fprintf(file, "\tfxNewArgumentsSloppyInstance,\n");
	else
		fprintf(file, "\tC_NULL,\n");
	if (fxIsCodeUsed(XS_CODE_ARGUMENTS_STRICT))
		fprintf(file, "\tfxNewArgumentsStrictInstance,\n");
	else
		fprintf(file, "\tC_NULL,\n");
	if (fxIsCodeUsed(XS_CODE_EVAL) || fxIsCodeUsed(XS_CODE_EVAL_TAIL))
		fprintf(file, "\tfxRunEval,\n");
	else
		fprintf(file, "\tfxDeadStrip,\n");
	if (fxIsCodeUsed(XS_CODE_EVAL) || fxIsCodeUsed(XS_CODE_EVAL_TAIL) || !fxIsCallbackStripped(linker, fx_eval))
		fprintf(file, "\tfxRunEvalEnvironment,\n");
	else
		fprintf(file, "\tfxDeadStrip,\n");
	if (fxIsCodeUsed(XS_CODE_PROGRAM_ENVIRONMENT) || !fxIsCallbackStripped(linker, fx_Compartment_prototype_evaluate))
		fprintf(file, "\tfxRunProgramEnvironment,\n");
	else
		fprintf(file, "\tfxDeadStrip,\n");
	if (fxIsLinkerSymbolUsed(linker, mxID(_Atomics))) {
		fprintf(file, "\tfxInitializeSharedCluster,\n");
		fprintf(file, "\tfxTerminateSharedCluster,\n");
	}
	else {
		fprintf(file, "\tC_NULL,\n");
		fprintf(file, "\tC_NULL,\n");
	}
	fprintf(file, "\tC_NULL,\n");
	fprintf(file, "\tC_NULL,\n");
	if (fxIsCodeUsed(XS_CODE_IMPORT) || !fxIsCallbackStripped(linker, fx_Compartment_prototype_import))
		fprintf(file, "\tfxRunImport,\n");
	else
		fprintf(file, "\tfxRunImportDeadStrip,\n");
	if (fxIsCodeUsed(XS_CODE_NEW_PRIVATE_1) || fxIsCodeUsed(XS_CODE_NEW_PRIVATE_2))
		fprintf(file, "\tfxDefinePrivateProperty,\n");
	else
		fprintf(file, "\tfxDefinePrivatePropertyDeadStrip,\n");
	if (fxIsCodeUsed(XS_CODE_GET_PRIVATE_1) || fxIsCodeUsed(XS_CODE_GET_PRIVATE_2))
		fprintf(file, "\tfxGetPrivateProperty,\n");
	else
		fprintf(file, "\tfxGetPrivatePropertyDeadStrip,\n");
	if (fxIsCodeUsed(XS_CODE_SET_PRIVATE_1) || fxIsCodeUsed(XS_CODE_SET_PRIVATE_2))
		fprintf(file, "\tfxSetPrivateProperty,\n");
	else
		fprintf(file, "\tfxSetPrivatePropertyDeadStrip,\n");
	if (fxIsCallbackStripped(linker, fx_FinalizationRegistry))
		fprintf(file, "\tC_NULL,\n");
	else
		fprintf(file, "\tfxCleanupFinalizationRegistries,\n");
	if (fxIsCallbackStripped(linker, fx_Error_prototype_get_stack))
		fprintf(file, "\tC_NULL,\n");
	else
		fprintf(file, "\tfxCaptureErrorStack,\n");
	fprintf(file, "};\n\n");

	fprintf(file, "const txBehavior* ICACHE_RAM_ATTR gxBehaviors[XS_BEHAVIOR_COUNT]  = {\n");
	fprintf(file, "\t&gxOrdinaryBehavior,\n");
	if (fxIsCodeUsed(XS_CODE_ARGUMENTS_SLOPPY))
		fprintf(file, "\t&gxArgumentsSloppyBehavior,\n");
	else
		fprintf(file, "\tC_NULL,\n");
	if (fxIsCodeUsed(XS_CODE_ARGUMENTS_STRICT))
		fprintf(file, "\t&gxOrdinaryBehavior,\n");
	else
		fprintf(file, "\tC_NULL,\n");
	fprintf(file, "\t&gxArrayBehavior,\n");
	fprintf(file, "\t&gxEnvironmentBehavior,\n");
	fprintf(file, "\t&gxGlobalBehavior,\n");
	fprintf(file, "\t&gxModuleBehavior,\n");
	if (!fxIsCallbackStripped(linker, fx_Proxy))
		fprintf(file, "\t&gxProxyBehavior,\n");
	else
		fprintf(file, "\tC_NULL,\n");
	fprintf(file, "\t&gxStringBehavior,\n");
	fprintf(file, "\t&gxTypedArrayBehavior\n");
	fprintf(file, "};\n\n");

	fprintf(file, "const txTypeDispatch ICACHE_FLASH_ATTR gxTypeDispatches[mxTypeArrayCount] = {\n");
	if (fxIsLinkerSymbolUsed(linker, mxID(_BigInt64Array)))
		fprintf(file, "\t{ 8, 3, fxBigInt64Getter, fxBigInt64Setter, fxBigIntCoerce, fxBigInt64Compare, _getBigInt64, _setBigInt64, _BigInt64Array },\n");
	else
		fprintf(file, "\t{ 8, 3, C_NULL, C_NULL, C_NULL, C_NULL, _getBigInt64, _setBigInt64, _BigInt64Array },\n");
	if (fxIsLinkerSymbolUsed(linker, mxID(_BigUint64Array)))
		fprintf(file, "\t{ 8, 3, fxBigUint64Getter, fxBigUint64Setter, fxBigIntCoerce, fxBigUint64Compare, _getBigUint64, _setBigUint64, _BigUint64Array },\n");
	else
		fprintf(file, "\t{ 8, 3, C_NULL, C_NULL, C_NULL, C_NULL, _getBigUint64, _setBigUint64, _BigUint64Array },\n");
	if (fxIsLinkerSymbolUsed(linker, mxID(_Float32Array)))
		fprintf(file, "\t{ 4, 2, fxFloat32Getter, fxFloat32Setter, fxNumberCoerce, fxFloat32Compare, _getFloat32, _setFloat32, _Float32Array },\n");
	else
		fprintf(file, "\t{ 4, 2, C_NULL, C_NULL, C_NULL, C_NULL, _getFloat32, _setFloat32, _Float32Array },\n");
	if (fxIsLinkerSymbolUsed(linker, mxID(_Float64Array)))
		fprintf(file, "\t{ 8, 3, fxFloat64Getter, fxFloat64Setter, fxNumberCoerce, fxFloat64Compare, _getFloat64, _setFloat64, _Float64Array },\n");
	else
		fprintf(file, "\t{ 8, 3, C_NULL, C_NULL, C_NULL, C_NULL, _getFloat64, _setFloat64, _Float64Array },\n");
	if (fxIsLinkerSymbolUsed(linker, mxID(_Int8Array)))
		fprintf(file, "\t{ 1, 0, fxInt8Getter, fxInt8Setter, fxIntCoerce, fxInt8Compare, _getInt8, _setInt8, _Int8Array },\n");
	else
		fprintf(file, "\t{ 1, 0, C_NULL, C_NULL, C_NULL, C_NULL, _getInt8, _setInt8, _Int8Array },\n");
	if (fxIsLinkerSymbolUsed(linker, mxID(_Int16Array)))
		fprintf(file, "\t{ 2, 1, fxInt16Getter, fxInt16Setter, fxIntCoerce, fxInt16Compare, _getInt16, _setInt16, _Int16Array },\n");
	else
		fprintf(file, "\t{ 2, 1, C_NULL, C_NULL, C_NULL, C_NULL, _getInt16, _setInt16, _Int16Array },\n");
	if (fxIsLinkerSymbolUsed(linker, mxID(_Int32Array)))
		fprintf(file, "\t{ 4, 2, fxInt32Getter, fxInt32Setter, fxIntCoerce, fxInt32Compare, _getInt32, _setInt32, _Int32Array },\n");
	else
		fprintf(file, "\t{ 4, 2, C_NULL, C_NULL, C_NULL, C_NULL, _getInt32, _setInt32, _Int32Array },\n");
	if (fxIsLinkerSymbolUsed(linker, mxID(_Uint8Array)))
		fprintf(file, "\t{ 1, 0, fxUint8Getter, fxUint8Setter, fxUintCoerce, fxUint8Compare, _getUint8, _setUint8, _Uint8Array },\n");
	else
		fprintf(file, "\t{ 1, 0, C_NULL, C_NULL, C_NULL, C_NULL, _getUint8, _setUint8, _Uint8Array },\n");
	if (fxIsLinkerSymbolUsed(linker, mxID(_Uint16Array)))
		fprintf(file, "\t{ 2, 1, fxUint16Getter, fxUint16Setter, fxUintCoerce, fxUint16Compare, _getUint16, _setUint16, _Uint16Array },\n");
	else
		fprintf(file, "\t{ 2, 1, C_NULL, C_NULL, C_NULL, C_NULL, _getUint16, _setUint16, _Uint16Array },\n");
	if (fxIsLinkerSymbolUsed(linker, mxID(_Uint32Array)))
		fprintf(file, "\t{ 4, 2, fxUint32Getter, fxUint32Setter, fxUintCoerce, fxUint32Compare, _getUint32, _setUint32, _Uint32Array },\n");
	else
		fprintf(file, "\t{ 4, 2, C_NULL, C_NULL, C_NULL, C_NULL, _getUint32, _setUint32, _Uint32Array },\n");
	if (fxIsLinkerSymbolUsed(linker, mxID(_Uint8ClampedArray)))
		fprintf(file, "\t{ 1, 0, fxUint8Getter, fxUint8ClampedSetter, fxNumberCoerce, fxUint8Compare, _getUint8, _setUint8, _Uint8ClampedArray }\n");
	else
		fprintf(file, "\t{ 1, 0, C_NULL, C_NULL, C_NULL, C_NULL, _getUint8, _setUint8, _Uint8ClampedArray }\n");
	fprintf(file, "};\n\n");
	
	fprintf(file, "const txTypeAtomics ICACHE_FLASH_ATTR gxTypeAtomics[mxTypeArrayCount] = {\n");
	if (fxIsLinkerSymbolUsed(linker, mxID(_Atomics)) && fxIsLinkerSymbolUsed(linker, mxID(_BigInt64Array)))
		fprintf(file, "\t{ fxInt64Add, fxInt64And, fxInt64CompareExchange, fxInt64Exchange, fxInt64Load, fxInt64Or, fxInt64Store, fxInt64Sub, fxInt64Xor, fxInt64Wait },\n");
	else
		fprintf(file, "\t{ C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL },\n");
	if (fxIsLinkerSymbolUsed(linker, mxID(_Atomics)) && fxIsLinkerSymbolUsed(linker, mxID(_BigUint64Array)))
		fprintf(file, "\t{ fxUint64Add, fxUint64And, fxUint64CompareExchange, fxUint64Exchange, fxUint64Load, fxUint64Or, fxUint64Store, fxUint64Sub, fxUint64Xor, C_NULL },\n");
	else
		fprintf(file, "\t{ C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL },\n");
	fprintf(file, "\t{ C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL },\n");
	fprintf(file, "\t{ C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL },\n");
	if (fxIsLinkerSymbolUsed(linker, mxID(_Atomics)) && fxIsLinkerSymbolUsed(linker, mxID(_Int8Array)))
		fprintf(file, "\t{ fxInt8Add, fxInt8And, fxInt8CompareExchange, fxInt8Exchange, fxInt8Load, fxInt8Or, fxInt8Store, fxInt8Sub, fxInt8Xor, C_NULL },\n");
	else
		fprintf(file, "\t{ C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL },\n");
	if (fxIsLinkerSymbolUsed(linker, mxID(_Atomics)) && fxIsLinkerSymbolUsed(linker, mxID(_Int16Array)))
		fprintf(file, "\t{ fxInt16Add, fxInt16And, fxInt16CompareExchange, fxInt16Exchange, fxInt16Load, fxInt16Or, fxInt16Store, fxInt16Sub, fxInt16Xor, C_NULL },\n");
	else
		fprintf(file, "\t{ C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL },\n");
	if (fxIsLinkerSymbolUsed(linker, mxID(_Atomics)) && fxIsLinkerSymbolUsed(linker, mxID(_Int32Array)))
		fprintf(file, "\t{ fxInt32Add, fxInt32And, fxInt32CompareExchange, fxInt32Exchange, fxInt32Load, fxInt32Or, fxInt32Store, fxInt32Sub, fxInt32Xor, fxInt32Wait },\n");
	else
		fprintf(file, "\t{ C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL },\n");
	if (fxIsLinkerSymbolUsed(linker, mxID(_Atomics)) && fxIsLinkerSymbolUsed(linker, mxID(_Uint8Array)))
		fprintf(file, "\t{ fxUint8Add, fxUint8And, fxUint8CompareExchange, fxUint8Exchange, fxUint8Load, fxUint8Or, fxUint8Store, fxUint8Sub, fxUint8Xor, C_NULL },\n");
	else
		fprintf(file, "\t{ C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL },\n");
	if (fxIsLinkerSymbolUsed(linker, mxID(_Atomics)) && fxIsLinkerSymbolUsed(linker, mxID(_Uint16Array)))
		fprintf(file, "\t{ fxUint16Add, fxUint16And, fxUint16CompareExchange, fxUint16Exchange, fxUint16Load, fxUint16Or, fxUint16Store, fxUint16Sub, fxUint16Xor, C_NULL },\n");
	else
		fprintf(file, "\t{ C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL },\n");
	if (fxIsLinkerSymbolUsed(linker, mxID(_Atomics)) && fxIsLinkerSymbolUsed(linker, mxID(_Uint32Array)))
		fprintf(file, "\t{ fxUint32Add, fxUint32And, fxUint32CompareExchange, fxUint32Exchange, fxUint32Load, fxUint32Or, fxUint32Store, fxUint32Sub, fxUint32Xor, C_NULL },\n");
	else
		fprintf(file, "\t{ C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL },\n");
	fprintf(file, "\t{ C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL }\n");
	fprintf(file, "};\n\n");
	
	fprintf(file, "const txTypeBigInt ICACHE_FLASH_ATTR gxTypeBigInt = {\n");
	if (!fxIsCallbackStripped(linker, fx_BigInt)) {
		fprintf(file, "\tfxBigIntCompare,\n");
		fprintf(file, "\tfxBigIntDecode,\n");
		fprintf(file, "\tfxBigintToArrayBuffer,\n");
		fprintf(file, "\tfxBigIntToInstance,\n");
		fprintf(file, "\tfxBigIntToNumber,\n");
		fprintf(file, "\tfxBigintToString,\n");
		fprintf(file, "\tfxBigInt_add,\n");
		fprintf(file, "\tfxBigInt_and,\n");
		fprintf(file, "\tfxBigInt_dec,\n");
		fprintf(file, "\tfxBigInt_div,\n");
		fprintf(file, "\tfxBigInt_exp,\n");
		fprintf(file, "\tfxBigInt_inc,\n");
		fprintf(file, "\tfxBigInt_lsl,\n");
		fprintf(file, "\tfxBigInt_lsr,\n");
		fprintf(file, "\tfxBigInt_mul,\n");
		fprintf(file, "\tfxBigInt_neg,\n");
		fprintf(file, "\tfxBigInt_nop,\n");
		fprintf(file, "\tfxBigInt_not,\n");
		fprintf(file, "\tfxBigInt_or,\n");
		fprintf(file, "\tfxBigInt_rem,\n");
		fprintf(file, "\tfxBigInt_sub,\n");
		fprintf(file, "\tfxBigInt_xor,\n");
	}
	else {
		fprintf(file, "\tC_NULL,\n");
		fprintf(file, "\tfxBigIntDecodeDeadStrip,\n");
		fprintf(file, "\tC_NULL,\n");
		fprintf(file, "\tC_NULL,\n");
		fprintf(file, "\tC_NULL,\n");
		fprintf(file, "\tC_NULL,\n");
		fprintf(file, "\tC_NULL,\n");
		fprintf(file, "\tC_NULL,\n");
		fprintf(file, "\tC_NULL,\n");
		fprintf(file, "\tC_NULL,\n");
		fprintf(file, "\tC_NULL,\n");
		fprintf(file, "\tC_NULL,\n");
		fprintf(file, "\tC_NULL,\n");
		fprintf(file, "\tC_NULL,\n");
		fprintf(file, "\tC_NULL,\n");
		fprintf(file, "\tC_NULL,\n");
		fprintf(file, "\tC_NULL,\n");
		fprintf(file, "\tC_NULL,\n");
		fprintf(file, "\tC_NULL,\n");
		fprintf(file, "\tC_NULL,\n");
		fprintf(file, "\tC_NULL,\n");
	}
	fprintf(file, "};\n\n");
}

void fxStripInstance(txLinker* linker, txMachine* the, txSlot* instance)
{
	txSlot* slot = instance->next;
	while (slot) {
		if (slot->kind == XS_HOST_FUNCTION_KIND)
			fxStripCallback(linker, slot->value.hostFunction.builder->callback);
		else if (slot->kind == XS_ACCESSOR_KIND) {
			txSlot* function;
			function = slot->value.accessor.getter;
			if (function && (function->next->kind == XS_CALLBACK_KIND))
				fxStripCallback(linker, function->next->value.callback.address);
			function = slot->value.accessor.setter;
			if (function && (function->next->kind == XS_CALLBACK_KIND))
				fxStripCallback(linker, function->next->value.callback.address);
		}
		slot = slot->next;
	}
}

void fxStripName(txLinker* linker, txString name)
{
	txLinkerCallback* linkerCallback = fxGetLinkerCallbackByName(linker, name);
	if (linkerCallback)
		linkerCallback->flag = 0;
}

void fxStripObject(txLinker* linker, txMachine* the, txSlot* slot)
{
	mxPushSlot(slot);
	fxStripInstance(linker, the, the->stack->value.reference);
	mxPop();
}

void fxUnstripCallback(txLinker* linker, txCallback which)
{
	txLinkerCallback* linkerCallback = fxGetLinkerCallbackByAddress(linker, which);
	if (linkerCallback)
		linkerCallback->flag = 1;
}

void fxUnstripCallbacks(txLinker* linker)
{
	txLinkerCallback* linkerCallback = linker->firstCallback;
	while (linkerCallback) {
		linkerCallback->flag = 1;
		linkerCallback = linkerCallback->nextCallback;
	}
}

void fxUnuseSymbol(txLinker* linker, txID id)
{
	txLinkerSymbol* linkerSymbol = linker->symbolArray[id];
	linkerSymbol->flag = 0;
}
