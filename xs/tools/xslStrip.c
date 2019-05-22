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
static void fxUseSymbol(txLinker* linker, txID id);

txFlag fxIsLinkerSymbolUsed(txLinker* linker, txID id)
{
	txLinkerSymbol* linkerSymbol = linker->symbolArray[id & 0x7FFF];
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
		fxStripObject(linker, the, &mxMathObject);
	// Date
	if (!fxIsLinkerSymbolUsed(linker, mxID(_Date)))
		fxStripClass(linker, the, &mxDateConstructor);
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
			fxStripClass(linker, the, &mxRegExpConstructor);
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
		fxStripClass(linker, the, &mxMapConstructor);
		fxStripInstance(linker, the, mxMapEntriesIteratorPrototype.value.reference);
		fxStripInstance(linker, the, mxMapKeysIteratorPrototype.value.reference);
		fxStripInstance(linker, the, mxMapValuesIteratorPrototype.value.reference);
	}
	else {
		fxUnstripCallback(linker, fx_Map_prototype_entries);
		fxUnstripCallback(linker, fx_Map_prototype_entries_next);
		fxUnstripCallback(linker, fx_Map_prototype_set);
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
		fxStripClass(linker, the, &mxSetConstructor);
		fxStripInstance(linker, the, mxSetEntriesIteratorPrototype.value.reference);
		fxStripInstance(linker, the, mxSetKeysIteratorPrototype.value.reference);
		fxStripInstance(linker, the, mxSetValuesIteratorPrototype.value.reference);
	}
	else {
		fxUnstripCallback(linker, fx_Set_prototype_add);
		if (fxIsLinkerSymbolUsed(linker, mxID(_entries))) {
			fxUnstripCallback(linker, fx_Set_prototype_entries);
			fxUnstripCallback(linker, fx_Set_prototype_entries_next);
		}
		fxUnstripCallback(linker, fx_Set_prototype_values);
		fxUnstripCallback(linker, fx_Set_prototype_values_next);
	}
	// WeakMap
	if (!fxIsLinkerSymbolUsed(linker, mxID(_WeakMap)))
		fxStripClass(linker, the, &mxWeakMapConstructor);
	// WeakSet
	if (!fxIsLinkerSymbolUsed(linker, mxID(_WeakSet)))
		fxStripClass(linker, the, &mxWeakSetConstructor);
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
	if (!fxIsLinkerSymbolUsed(linker, mxID(_DataView)))
		fxStripClass(linker, the, &mxDataViewConstructor);
	// Atomics
	if (!fxIsLinkerSymbolUsed(linker, mxID(_SharedArrayBuffer)))
		fxStripClass(linker, the, &mxSharedArrayBufferConstructor);
	if (!fxIsLinkerSymbolUsed(linker, mxID(_Atomics)))
		fxStripObject(linker, the, &mxAtomicsObject);
	// BigInt
	if (!fxIsLinkerSymbolUsed(linker, mxID(_BigInt)) && !fxIsCodeUsed(XS_CODE_BIGINT_1) && !fxIsCodeUsed(XS_CODE_BIGINT_2))
		fxStripClass(linker, the, &mxBigIntConstructor);
	// JSON
	if (!fxIsLinkerSymbolUsed(linker, mxID(_JSON)))
		fxStripObject(linker, the, &mxJSONObject);
	// Promise
	if (!fxIsLinkerSymbolUsed(linker, mxID(_Promise)) && !fxIsCodeUsed(XS_CODE_ASYNC_FUNCTION) && !fxIsCodeUsed(XS_CODE_ASYNC_GENERATOR_FUNCTION)) {
		fxStripClass(linker, the, &mxPromiseConstructor);
	}
	else {
		fxUnstripCallback(linker, fx_Promise);
		fxUnstripCallback(linker, fx_Promise_prototype_then);
		fxUnstripCallback(linker, fx_Promise_resolve);
		fxUnstripCallback(linker, fxOnRejectedPromise);
		fxUnstripCallback(linker, fxOnResolvedPromise);
		fxUnstripCallback(linker, fxRejectPromise);
		fxUnstripCallback(linker, fxResolvePromise);
	}
	// Reflect
	if (!fxIsLinkerSymbolUsed(linker, mxID(_Reflect)))
		fxStripObject(linker, the, &mxReflectObject);
	// Proxy
	if (!fxIsLinkerSymbolUsed(linker, mxID(_Proxy)))
		fxStripObject(linker, the, &mxProxyConstructor);
	else {
		fxUnstripCallback(linker, fxProxyGetter);
		fxUnstripCallback(linker, fxProxySetter);
	}
	// Modules
	fxUnstripCallback(linker, fx_Module);
	fxUnstripCallback(linker, fx_Transfer);
	fxUnstripCallback(linker, fx_require);
	fxUnstripCallback(linker, fx_Compartment);
	fxUnstripCallback(linker, fx_Set_prototype_add);
	fxUnstripCallback(linker, fx_Set_prototype_values);
	fxUnstripCallback(linker, fx_Set_prototype_values_next);
	// constructors
	fxUnstripCallback(linker, fx_species_get);
	// object rest/spread
	if (linker->intrinsicFlags[mxCopyObjectIntrinsic] || fxIsLinkerSymbolUsed(linker, mxID(_Compartment)))
		fxUnstripCallback(linker, fxCopyObject);
	// parser
	if (!fxIsLinkerSymbolUsed(linker, mxID(_eval)))
		fxStripCallback(linker, fx_eval);
	if (!fxIsLinkerSymbolUsed(linker, mxID(_Function)))
		fxStripCallback(linker, fx_Function);
	if (!fxIsLinkerSymbolUsed(linker, mxID(_AsyncFunction)))
		fxStripCallback(linker, fx_AsyncFunction);
	if (!fxIsLinkerSymbolUsed(linker, mxID(_GeneratorFunction)))
		fxStripCallback(linker, fx_GeneratorFunction);
	if (!fxIsLinkerSymbolUsed(linker, mxID(_AsyncGeneratorFunction)))
		fxStripCallback(linker, fx_AsyncGeneratorFunction);
}

void fxStripClass(txLinker* linker, txMachine* the, txSlot* slot)
{
	mxPushSlot(slot);
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
	if (fxIsCodeUsed(XS_CODE_ARGUMENTS_SLOPPY))
		fprintf(file, "\tfxNewArgumentsSloppyInstance,\n");
	else
		fprintf(file, "\tC_NULL,\n");
	if (fxIsCodeUsed(XS_CODE_ARGUMENTS_STRICT))
		fprintf(file, "\tfxNewArgumentsStrictInstance,\n");
	else
		fprintf(file, "\tC_NULL,\n");
	if (fxIsCodeUsed(XS_CODE_EVAL)) {
		if (fxIsLinkerSymbolUsed(linker, mxID(_eval))) {
			fprintf(file, "\tfxRunEval,\n");
			fprintf(file, "\tfxRunEvalEnvironment,\n");
		}
		else {
			fprintf(file, "\tfxDeadStrip,\n");
			fprintf(file, "\tC_NULL,\n");
		}
	}
	else {
		fprintf(file, "\tC_NULL,\n");
		fprintf(file, "\tC_NULL,\n");
	}
	if (fxIsCodeUsed(XS_CODE_PROGRAM_ENVIRONMENT))
		fprintf(file, "\tfxRunProgramEnvironment,\n");
	else
		fprintf(file, "\tC_NULL,\n");
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
	if (fxIsLinkerSymbolUsed(linker, mxID(_Proxy)))
		fprintf(file, "\t&gxProxyBehavior,\n");
	else
		fprintf(file, "\tC_NULL,\n");
	fprintf(file, "\t&gxStringBehavior,\n");
	fprintf(file, "\t&gxTypedArrayBehavior\n");
	fprintf(file, "};\n\n");

	fprintf(file, "const txTypeDispatch ICACHE_FLASH_ATTR gxTypeDispatches[mxTypeArrayCount] = {\n");
	if (fxIsLinkerSymbolUsed(linker, mxID(_BigInt64Array)))
		fprintf(file, "\t{ 8, fxBigInt64Getter, fxBigInt64Setter, fxBigIntCoerce, fxBigInt64Compare, _getBigInt64, _setBigInt64, _BigInt64Array },\n");
	else
		fprintf(file, "\t{ 8, C_NULL, C_NULL, C_NULL, C_NULL, _getBigInt64, _setBigInt64, _BigInt64Array },\n");
	if (fxIsLinkerSymbolUsed(linker, mxID(_BigUint64Array)))
		fprintf(file, "\t{ 8, fxBigUint64Getter, fxBigUint64Setter, fxBigIntCoerce, fxBigUint64Compare, _getBigUint64, _setBigUint64, _BigUint64Array },\n");
	else
		fprintf(file, "\t{ 8, C_NULL, C_NULL, C_NULL, C_NULL, _getBigUint64, _setBigUint64, _BigUint64Array },\n");
	if (fxIsLinkerSymbolUsed(linker, mxID(_Float32Array)))
		fprintf(file, "\t{ 4, fxFloat32Getter, fxFloat32Setter, fxNumberCoerce, fxFloat32Compare, _getFloat32, _setFloat32, _Float32Array },\n");
	else
		fprintf(file, "\t{ 4, C_NULL, C_NULL, C_NULL, C_NULL, _getFloat32, _setFloat32, _Float32Array },\n");
	if (fxIsLinkerSymbolUsed(linker, mxID(_Float64Array)))
		fprintf(file, "\t{ 8, fxFloat64Getter, fxFloat64Setter, fxNumberCoerce, fxFloat64Compare, _getFloat64, _setFloat64, _Float64Array },\n");
	else
		fprintf(file, "\t{ 8, C_NULL, C_NULL, C_NULL, C_NULL, _getFloat64, _setFloat64, _Float64Array },\n");
	if (fxIsLinkerSymbolUsed(linker, mxID(_Int8Array)))
		fprintf(file, "\t{ 1, fxInt8Getter, fxInt8Setter, fxNumberCoerce, fxInt8Compare, _getInt8, _setInt8, _Int8Array },\n");
	else
		fprintf(file, "\t{ 1, C_NULL, C_NULL, C_NULL, C_NULL, _getInt8, _setInt8, _Int8Array },\n");
	if (fxIsLinkerSymbolUsed(linker, mxID(_Int16Array)))
		fprintf(file, "\t{ 2, fxInt16Getter, fxInt16Setter, fxNumberCoerce, fxInt16Compare, _getInt16, _setInt16, _Int16Array },\n");
	else
		fprintf(file, "\t{ 2, C_NULL, C_NULL, C_NULL, C_NULL, _getInt16, _setInt16, _Int16Array },\n");
	if (fxIsLinkerSymbolUsed(linker, mxID(_Int32Array)))
		fprintf(file, "\t{ 4, fxInt32Getter, fxInt32Setter, fxNumberCoerce, fxInt32Compare, _getInt32, _setInt32, _Int32Array },\n");
	else
		fprintf(file, "\t{ 4, C_NULL, C_NULL, C_NULL, C_NULL, _getInt32, _setInt32, _Int32Array },\n");
	if (fxIsLinkerSymbolUsed(linker, mxID(_Uint8Array)))
		fprintf(file, "\t{ 1, fxUint8Getter, fxUint8Setter, fxNumberCoerce, fxUint8Compare, _getUint8, _setUint8, _Uint8Array },\n");
	else
		fprintf(file, "\t{ 1, C_NULL, C_NULL, C_NULL, C_NULL, _getUint8, _setUint8, _Uint8Array },\n");
	if (fxIsLinkerSymbolUsed(linker, mxID(_Uint16Array)))
		fprintf(file, "\t{ 2, fxUint16Getter, fxUint16Setter, fxNumberCoerce, fxUint16Compare, _getUint16, _setUint16, _Uint16Array },\n");
	else
		fprintf(file, "\t{ 2, C_NULL, C_NULL, C_NULL, C_NULL, _getUint16, _setUint16, _Uint16Array },\n");
	if (fxIsLinkerSymbolUsed(linker, mxID(_Uint32Array)))
		fprintf(file, "\t{ 4, fxUint32Getter, fxUint32Setter, fxNumberCoerce, fxUint32Compare, _getUint32, _setUint32, _Uint32Array },\n");
	else
		fprintf(file, "\t{ 4, C_NULL, C_NULL, C_NULL, C_NULL, _getUint32, _setUint32, _Uint32Array },\n");
	if (fxIsLinkerSymbolUsed(linker, mxID(_Uint8ClampedArray)))
		fprintf(file, "\t{ 1, fxUint8Getter, fxUint8ClampedSetter, fxNumberCoerce, fxUint8Compare, _getUint8Clamped, _setUint8Clamped, _Uint8ClampedArray }\n");
	else
		fprintf(file, "\t{ 1, C_NULL, C_NULL, C_NULL, C_NULL, _getUint8Clamped, _setUint8Clamped, _Uint8ClampedArray }\n");
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
	if (fxIsLinkerSymbolUsed(linker, mxID(_BigInt)) || fxIsCodeUsed(XS_CODE_BIGINT_1) || fxIsCodeUsed(XS_CODE_BIGINT_2)) {
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

void fxUseSymbol(txLinker* linker, txID id)
{
	txLinkerSymbol* linkerSymbol = linker->symbolArray[id & 0x7FFF];
	linkerSymbol->flag = 1;
}
