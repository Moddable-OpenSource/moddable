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
static void fxStripClass(txLinker* linker, txMachine* the, txID id);
static void fxStripInstance(txLinker* linker, txMachine* the, txSlot* instance);
static void fxStripObject(txLinker* linker, txMachine* the, txID id);
static void fxUnstripCallback(txLinker* linker, txCallback which);
static void fxUseSymbol(txLinker* linker, txID id);

txFlag fxIsLinkerSymbolUsed(txLinker* linker, txID id)
{
	txLinkerSymbol* linkerSymbol = linker->symbolArray[id & 0x7FFF];
	return linkerSymbol->flag;
}

void fxReferenceLinkerSymbol(txLinker* linker, txID id)
{
	txLinkerSymbol* linkerSymbol = linker->symbolArray[id & 0x7FFF];
	linkerSymbol->flag |= 1;
}

void fxStripCallback(txLinker* linker, txCallback which)
{
	txLinkerCallback* linkerCallback = fxGetLinkerCallbackByAddress(linker, which);
	if (linkerCallback)
		linkerCallback->flag = 0;
}

void fxStripCallbacks(txLinker* linker, txMachine* the)
{

	// for of ...
	fxUseSymbol(linker, mxID(_Symbol_iterator));
	// type conversion
	fxUseSymbol(linker, mxID(_Symbol_toPrimitive));
	fxUseSymbol(linker, mxID(_toString));
	fxUseSymbol(linker, mxID(_valueOf));
	{
		txLinkerBuilder* linkerBuilder = linker->firstBuilder;
		while (linkerBuilder) {
			txID id = linkerBuilder->host.id & 0x7FFF;
			txLinkerSymbol* symbol = linker->symbolArray[id];
			if (symbol->flag)
				fxUnstripCallback(linker, linkerBuilder->host.callback);
			linkerBuilder = linkerBuilder->nextBuilder;
		}
	}
	// for in
	fxUnstripCallback(linker, fx_Enumerator);
	fxUnstripCallback(linker, fx_Enumerator_next);
	// arguments
	fxUnstripCallback(linker, fxThrowTypeError);
	// Object?
	// Function
	fxUnstripCallback(linker, fx_Function_prototype_hasInstance);
	// Boolean?
	// Symbol?
	// Error?
	// Number?
	// Math
	if (!fxIsLinkerSymbolUsed(linker, mxID(_Math)))
		fxStripObject(linker, the, mxID(_Math));
	// Date
	if (!fxIsLinkerSymbolUsed(linker, mxID(_Date)))
		fxStripClass(linker, the, mxID(_Date));
	// String
	fxUnstripCallback(linker, fxStringAccessorGetter);
	fxUnstripCallback(linker, fxStringAccessorSetter);
	// RegExp
	int match = fxIsLinkerSymbolUsed(linker, mxID(_match));
	int search = fxIsLinkerSymbolUsed(linker, mxID(_search));
	if (fxIsLinkerSymbolUsed(linker, mxID(_RegExp))) {
		fxUnstripCallback(linker, fxInitializeRegExp);
		if (fxIsLinkerSymbolUsed(linker, mxID(_replace))) {
			fxUnstripCallback(linker, fx_RegExp_prototype_get_global);
			fxUnstripCallback(linker, fx_RegExp_prototype_get_unicode);
			fxUnstripCallback(linker, fx_RegExp_prototype_replace);
			fxUnstripCallback(linker, fx_RegExp_prototype_exec);
		}
		if (fxIsLinkerSymbolUsed(linker, mxID(_split))) {
			fxUnstripCallback(linker, fx_RegExp_prototype_split);
			fxUnstripCallback(linker, fx_RegExp_prototype_exec);
		}
		if (fxIsLinkerSymbolUsed(linker, mxID(_test))) {
			fxUnstripCallback(linker, fx_RegExp_prototype_exec);
		}
	}
	else {
		if (!match && !search) {
			fxStripClass(linker, the, mxID(_RegExp));
		}
	}
	if (match) {
		fxUnstripCallback(linker, fxInitializeRegExp);
		fxUnstripCallback(linker, fx_RegExp_prototype_get_global);
		fxUnstripCallback(linker, fx_RegExp_prototype_get_unicode);
		fxUnstripCallback(linker, fx_RegExp_prototype_match);
		fxUnstripCallback(linker, fx_RegExp_prototype_exec);
	}
	if (search) {
		fxUnstripCallback(linker, fxInitializeRegExp);
		fxUnstripCallback(linker, fx_RegExp_prototype_search);
		fxUnstripCallback(linker, fx_RegExp_prototype_exec);
	}
	// Array ?
	if (fxIsCodeUsed(XS_CODE_ARRAY))
		fxUnstripCallback(linker, fx_Array);
	fxUnstripCallback(linker, fxArrayLengthGetter);
	fxUnstripCallback(linker, fxArrayLengthSetter);
	fxUnstripCallback(linker, fx_Array_prototype_join);
	fxUnstripCallback(linker, fx_Array_prototype_values);
	fxUnstripCallback(linker, fx_ArrayIterator_prototype_next);
	// TypedArray ?
	fxUnstripCallback(linker, fxTypedArrayGetter);
	fxUnstripCallback(linker, fxTypedArraySetter);
	fxUnstripCallback(linker, fx_TypedArray_prototype_join);
	fxUnstripCallback(linker, fx_TypedArray_prototype_values);
	// Map
	if (!fxIsLinkerSymbolUsed(linker, mxID(_Map))) {
		fxStripClass(linker, the, mxID(_Map));
		fxStripInstance(linker, the, mxMapEntriesIteratorPrototype.value.reference);
		fxStripInstance(linker, the, mxMapKeysIteratorPrototype.value.reference);
		fxStripInstance(linker, the, mxMapValuesIteratorPrototype.value.reference);
	}
	else {
		fxUnstripCallback(linker, fx_Map_prototype_entries);
		fxUnstripCallback(linker, fx_Map_prototype_entries_next);
		if (fxIsLinkerSymbolUsed(linker, mxID(_keys))) {
			fxUnstripCallback(linker, fx_Map_prototype_keys);
			fxUnstripCallback(linker, fx_Map_prototype_keys_next);
		}
		if (fxIsLinkerSymbolUsed(linker, mxID(_values))) {
			fxUnstripCallback(linker, fx_Map_prototype_values);
			fxUnstripCallback(linker, fx_Map_prototype_values_next);
		}
	}
	// Set
	if (!fxIsLinkerSymbolUsed(linker, mxID(_Set))) {
		fxStripClass(linker, the, mxID(_Set));
		fxStripInstance(linker, the, mxSetEntriesIteratorPrototype.value.reference);
		fxStripInstance(linker, the, mxSetKeysIteratorPrototype.value.reference);
		fxStripInstance(linker, the, mxSetValuesIteratorPrototype.value.reference);
	}
	else {
		if (fxIsLinkerSymbolUsed(linker, mxID(_entries))) {
			fxUnstripCallback(linker, fx_Set_prototype_entries);
			fxUnstripCallback(linker, fx_Set_prototype_entries_next);
		}
		fxUnstripCallback(linker, fx_Set_prototype_values);
		fxUnstripCallback(linker, fx_Set_prototype_values_next);
	}
	// WeakMap
	if (!fxIsLinkerSymbolUsed(linker, mxID(_WeakMap)))
		fxStripClass(linker, the, mxID(_WeakMap));
	// WeakSet
	if (!fxIsLinkerSymbolUsed(linker, mxID(_WeakSet)))
		fxStripClass(linker, the, mxID(_WeakSet));
	// Iterator ?
	if (!fxIsCodeUsed(XS_CODE_FOR_AWAIT_OF))
		fxStripInstance(linker, the, mxAsyncFromSyncIteratorPrototype.value.reference);
	// Generator
	if (!fxIsCodeUsed(XS_CODE_GENERATOR_FUNCTION)) {
		fxStripInstance(linker, the, mxGeneratorPrototype.value.reference);
		fxStripInstance(linker, the, mxGeneratorFunctionPrototype.value.reference);
	}
	if (!fxIsCodeUsed(XS_CODE_ASYNC_GENERATOR_FUNCTION)) {
		fxStripInstance(linker, the, mxAsyncGeneratorPrototype.value.reference);
		fxStripInstance(linker, the, mxAsyncGeneratorFunctionPrototype.value.reference);
	}
	// ArrayBuffer ?
	fxUnstripCallback(linker, fx_ArrayBuffer);
	// DataView
	if (!fxIsLinkerSymbolUsed(linker, mxID(_SharedArrayBuffer)))
		fxStripClass(linker, the, mxID(_SharedArrayBuffer));
	// DataView
	if (!fxIsLinkerSymbolUsed(linker, mxID(_DataView)))
		fxStripClass(linker, the, mxID(_DataView));
	// Reflect
	if (!fxIsLinkerSymbolUsed(linker, mxID(_Atomics)))
		fxStripObject(linker, the, mxID(_Atomics));
	// JSON
	if (!fxIsLinkerSymbolUsed(linker, mxID(_JSON)))
		fxStripObject(linker, the, mxID(_JSON));
	// Promise
	if (!fxIsLinkerSymbolUsed(linker, mxID(_Promise)) && !fxIsCodeUsed(XS_CODE_ASYNC_FUNCTION) && !fxIsCodeUsed(XS_CODE_ASYNC_GENERATOR_FUNCTION)) {
		fxStripClass(linker, the, mxID(_Promise));
	}
	else {
		fxUnstripCallback(linker, fx_Promise_prototype_then);
		fxUnstripCallback(linker, fx_Promise_resolve);
		fxUnstripCallback(linker, fxOnRejectedPromise);
		fxUnstripCallback(linker, fxOnResolvedPromise);
		fxUnstripCallback(linker, fxRejectPromise);
		fxUnstripCallback(linker, fxResolvePromise);
	}
	// Reflect
	if (!fxIsLinkerSymbolUsed(linker, mxID(_Reflect)))
		fxStripObject(linker, the, mxID(_Reflect));
	// Proxy
	if (!fxIsLinkerSymbolUsed(linker, mxID(_Proxy)))
		fxStripObject(linker, the, mxID(_Proxy));
	else {
		fxUnstripCallback(linker, fxProxyGetter);
		fxUnstripCallback(linker, fxProxySetter);
	}
	// Modules
	fxUnstripCallback(linker, fx_Module);
	fxUnstripCallback(linker, fx_Transfer);
	fxUnstripCallback(linker, fx_require);
	fxUnstripCallback(linker, fx_require_weak);
	fxUnstripCallback(linker, fx_Set_prototype_add);
	fxUnstripCallback(linker, fx_Set_prototype_values);
	fxUnstripCallback(linker, fx_Set_prototype_values_next);
	// constructors
	fxUnstripCallback(linker, fx_species_get);
	// eval
	if (linker->intrinsicFlags[mxEvalIntrinsic])
		fxUnstripCallback(linker, fx_eval);
	// object rest/spread
	if (linker->intrinsicFlags[mxCopyObjectIntrinsic])
		fxUnstripCallback(linker, fxCopyObject);
}

void fxStripClass(txLinker* linker, txMachine* the, txID id)
{
	mxPush(mxGlobal);
	fxGetID(the, id);
	fxStripInstance(linker, the, the->stack->value.reference);
	fxGetID(the, mxID(_prototype));
	fxStripInstance(linker, the, the->stack->value.reference);
	mxPop();
}

void fxStripDefaults(txLinker* linker, FILE* file)
{
	fprintf(file, "const txDefaults ICACHE_FLASH_ATTR gxDefaults  = {\n");
	if (fxIsCodeUsed(XS_CODE_START_ASYNC)) {
		fprintf(file, "\tfxNewAsyncInstance,\n");
		fprintf(file, "\tfxRunAsync,\n");
	}
	else {
		fprintf(file, "\tC_NULL,\n");
		fprintf(file, "\tC_NULL,\n");
	}
	if (fxIsCodeUsed(XS_CODE_START_GENERATOR))
		fprintf(file, "\tfxNewGeneratorInstance,\n");
	else
		fprintf(file, "\tC_NULL,\n");
	if (fxIsCodeUsed(XS_CODE_GENERATOR_FUNCTION))
		fprintf(file, "\tfxNewGeneratorFunctionInstance,\n");
	else
		fprintf(file, "\tC_NULL,\n");
	if (fxIsCodeUsed(XS_CODE_START_ASYNC_GENERATOR))
		fprintf(file, "\tfxNewAsyncGeneratorInstance,\n");
	else
		fprintf(file, "\tC_NULL,\n");
	if (fxIsCodeUsed(XS_CODE_ASYNC_GENERATOR_FUNCTION))
		fprintf(file, "\tfxNewAsyncGeneratorFunctionInstance,\n");
	else
		fprintf(file, "\tC_NULL,\n");
	if (fxIsCodeUsed(XS_CODE_EVAL_ENVIRONMENT))
		fprintf(file, "\tfxRunEvalEnvironment,\n");
	else
		fprintf(file, "\tC_NULL,\n");
	if (fxIsCodeUsed(XS_CODE_PROGRAM_ENVIRONMENT))
		fprintf(file, "\tfxRunProgramEnvironment,\n");
	else
		fprintf(file, "\tC_NULL,\n");
	fprintf(file, "};\n\n");

	fprintf(file, "const txBehavior* ICACHE_RAM_ATTR gxBehaviors[XS_BEHAVIOR_COUNT]  = {\n");
	fprintf(file, "\t&gxOrdinaryBehavior,\n");
	fprintf(file, "\tC_NULL,\n");
	fprintf(file, "\t&gxArgumentsStrictBehavior,\n");
	fprintf(file, "\t&gxArrayBehavior,\n");
	fprintf(file, "\t&gxEnvironmentBehavior,\n");
	fprintf(file, "\t&gxGlobalBehavior,\n");
	fprintf(file, "\t&gxModuleBehavior,\n");
	if (fxIsLinkerSymbolUsed(linker, mxID(_Proxy)))
		fprintf(file, "\t&gxProxyBehavior,\n");
	else
		fprintf(file, "\tC_NULL,\n");
	fprintf(file, "\t&gxStringBehavior,\n");
	fprintf(file, "\t&gxTypedArrayBehavior\n");
	fprintf(file, "};\n\n");

	fprintf(file, "const txTypeDispatch ICACHE_FLASH_ATTR gxTypeDispatches[mxTypeArrayCount] = {\n");
	if (fxIsLinkerSymbolUsed(linker, mxID(_Float32Array)))
		fprintf(file, "\t{ 4, fxFloat32Getter, fxFloat32Setter, fxFloat32Compare, _getFloat32, _setFloat32, _Float32Array },\n");
	else
		fprintf(file, "\t{ 4, C_NULL, C_NULL, C_NULL, _getFloat32, _setFloat32, _Float32Array },\n");
	if (fxIsLinkerSymbolUsed(linker, mxID(_Float64Array)))
		fprintf(file, "\t{ 8, fxFloat64Getter, fxFloat64Setter, fxFloat64Compare, _getFloat64, _setFloat64, _Float64Array },\n");
	else
		fprintf(file, "\t{ 8, C_NULL, C_NULL, C_NULL, _getFloat64, _setFloat64, _Float64Array },\n");
	if (fxIsLinkerSymbolUsed(linker, mxID(_Int8Array)))
		fprintf(file, "\t{ 1, fxInt8Getter, fxInt8Setter, fxInt8Compare, _getInt8, _setInt8, _Int8Array },\n");
	else
		fprintf(file, "\t{ 1, C_NULL, C_NULL, C_NULL, _getInt8, _setInt8, _Int8Array },\n");
	if (fxIsLinkerSymbolUsed(linker, mxID(_Int16Array)))
		fprintf(file, "\t{ 2, fxInt16Getter, fxInt16Setter, fxInt16Compare, _getInt16, _setInt16, _Int16Array },\n");
	else
		fprintf(file, "\t{ 2, C_NULL, C_NULL, C_NULL, _getInt16, _setInt16, _Int16Array },\n");
	if (fxIsLinkerSymbolUsed(linker, mxID(_Int32Array)))
		fprintf(file, "\t{ 4, fxInt32Getter, fxInt32Setter, fxInt32Compare, _getInt32, _setInt32, _Int32Array },\n");
	else
		fprintf(file, "\t{ 4, C_NULL, C_NULL, C_NULL, _getInt32, _setInt32, _Int32Array },\n");
	if (fxIsLinkerSymbolUsed(linker, mxID(_Uint8Array)))
		fprintf(file, "\t{ 1, fxUint8Getter, fxUint8Setter, fxUint8Compare, _getUint8, _setUint8, _Uint8Array },\n");
	else
		fprintf(file, "\t{ 1, C_NULL, C_NULL, C_NULL, _getUint8, _setUint8, _Uint8Array },\n");
	if (fxIsLinkerSymbolUsed(linker, mxID(_Uint16Array)))
		fprintf(file, "\t{ 2, fxUint16Getter, fxUint16Setter, fxUint16Compare, _getUint16, _setUint16, _Uint16Array },\n");
	else
		fprintf(file, "\t{ 2, C_NULL, C_NULL, C_NULL, _getUint16, _setUint16, _Uint16Array },\n");
	if (fxIsLinkerSymbolUsed(linker, mxID(_Uint32Array)))
		fprintf(file, "\t{ 4, fxUint32Getter, fxUint32Setter, fxUint32Compare, _getUint32, _setUint32, _Uint32Array },\n");
	else
		fprintf(file, "\t{ 4, C_NULL, C_NULL, C_NULL, _getUint32, _setUint32, _Uint32Array },\n");
	if (fxIsLinkerSymbolUsed(linker, mxID(_Uint8ClampedArray)))
		fprintf(file, "\t{ 1, fxUint8Getter, fxUint8ClampedSetter, fxUint8Compare, _getUint8Clamped, _setUint8Clamped, _Uint8ClampedArray }\n");
	else
		fprintf(file, "\t{ 1, C_NULL, C_NULL, C_NULL, _getUint8Clamped, _setUint8Clamped, _Uint8ClampedArray }\n");
	fprintf(file, "};\n\n");
	
	fprintf(file, "const txTypeAtomics ICACHE_FLASH_ATTR gxTypeAtomics[mxTypeArrayCount] = {\n");
	fprintf(file, "\t{ C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL },\n");
	fprintf(file, "\t{ C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL },\n");
	if (fxIsLinkerSymbolUsed(linker, mxID(_Atomics)) && fxIsLinkerSymbolUsed(linker, mxID(_Int8Array)))
		fprintf(file, "\t{ fxInt8Add, fxInt8And, fxInt8CompareExchange, fxInt8Exchange, fxInt8Load, fxInt8Or, fxInt8Store, fxInt8Sub, fxInt8Xor },\n");
	else
		fprintf(file, "\t{ C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL },\n");
	if (fxIsLinkerSymbolUsed(linker, mxID(_Atomics)) && fxIsLinkerSymbolUsed(linker, mxID(_Int16Array)))
		fprintf(file, "\t{ fxInt16Add, fxInt16And, fxInt16CompareExchange, fxInt16Exchange, fxInt16Load, fxInt16Or, fxInt16Store, fxInt16Sub, fxInt16Xor },\n");
	else
		fprintf(file, "\t{ C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL },\n");
	if (fxIsLinkerSymbolUsed(linker, mxID(_Atomics)) && fxIsLinkerSymbolUsed(linker, mxID(_Int32Array)))
		fprintf(file, "\t{ fxInt32Add, fxInt32And, fxInt32CompareExchange, fxInt32Exchange, fxInt32Load, fxInt32Or, fxInt32Store, fxInt32Sub, fxInt32Xor },\n");
	else
		fprintf(file, "\t{ C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL },\n");
	if (fxIsLinkerSymbolUsed(linker, mxID(_Atomics)) && fxIsLinkerSymbolUsed(linker, mxID(_Uint8Array)))
		fprintf(file, "\t{ fxUint8Add, fxUint8And, fxUint8CompareExchange, fxUint8Exchange, fxUint8Load, fxUint8Or, fxUint8Store, fxUint8Sub, fxUint8Xor },\n");
	else
		fprintf(file, "\t{ C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL },\n");
	if (fxIsLinkerSymbolUsed(linker, mxID(_Atomics)) && fxIsLinkerSymbolUsed(linker, mxID(_Uint16Array)))
		fprintf(file, "\t{ fxUint16Add, fxUint16And, fxUint16CompareExchange, fxUint16Exchange, fxUint16Load, fxUint16Or, fxUint16Store, fxUint16Sub, fxUint16Xor },\n");
	else
		fprintf(file, "\t{ C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL },\n");
	if (fxIsLinkerSymbolUsed(linker, mxID(_Atomics)) && fxIsLinkerSymbolUsed(linker, mxID(_Uint32Array)))
		fprintf(file, "\t{ fxUint32Add, fxUint32And, fxUint32CompareExchange, fxUint32Exchange, fxUint32Load, fxUint32Or, fxUint32Store, fxUint32Sub, fxUint32Xor },\n");
	else
		fprintf(file, "\t{ C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL },\n");
	fprintf(file, "\t{ C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL, C_NULL }\n");
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

void fxStripObject(txLinker* linker, txMachine* the, txID id)
{
	mxPush(mxGlobal);
	fxGetID(the, id);
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

void fxUseSymbol(txLinker* linker, txID id)
{
	txLinkerSymbol* linkerSymbol = linker->symbolArray[id & 0x7FFF];
	linkerSymbol->flag = 1;
}
