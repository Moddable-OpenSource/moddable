/*
 * Copyright (c) 2016-2018  Moddable Tech, Inc.
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

#define _GNU_SOURCE
#include "xsSnapshot.h"
#if mxMacOSX || mxLinux
#include <dlfcn.h>
#endif

static void fxIndexInstance(txMachine* the, txSnapshot* snapshot, txSlot* theCurrent);
static void fxIndexReference(txMachine* the, txSnapshot* snapshot, txSlot* theSlot);
static void fxIndexSlot(txMachine* the, txSnapshot* snapshot, txSlot* slot);
static void fxIndexSlots(txMachine* the, txSnapshot* snapshot);

static void fxLinkChunks(txMachine* the);

static void fxMeasureSlot(txMachine* the, txSnapshot* snapshot, txSlot* slot, txSize* chunkSize);
static void fxMeasureChunk(txMachine* the, txSnapshot* snapshot, void* address, txSize* chunkSize);
static void fxMeasureChunkArray(txMachine* the, txSnapshot* snapshot, txSlot* address, txSize* chunkSize);

static txCallback fxProjectCallback(txMachine* the, txSnapshot* snapshot, txCallback callback);
static void* fxProjectChunk(txMachine* the, void* address);
static txSlot* fxProjectSlot(txMachine* the, txProjection* firstProjection, txSlot* slot);
static txTypeAtomics* fxProjectTypeAtomics(txMachine* the, txTypeAtomics* atomics);
static txTypeDispatch* fxProjectTypeDispatch(txMachine* the, txTypeDispatch* dispatch);

static void fxReadAtom(txMachine* the, txSnapshot* snapshot, Atom* atom, txString type);
static void fxReadMapSet(txMachine* the, txSlot* table, txBoolean paired);
static void fxReadSlot(txMachine* the, txSnapshot* snapshot, txSlot* slot, txFlag flag);
static void fxReadSlotArray(txMachine* the, txSnapshot* snapshot, txSlot* address, txSize length);
static void fxReadSlotTable(txMachine* the, txSnapshot* snapshot, txSlot** address, txSize length);

static void fxUnlinkChunks(txMachine* the);

#define mxUnprojectChunk(ADDRESS) (snapshot->firstChunk + ((size_t)ADDRESS));
static txCallback fxUnprojectCallback(txMachine* the, txSnapshot* snapshot, txCallback callback);
static txSlot* fxUnprojectSlot(txMachine* the, txSnapshot* snapshot, txSlot* slot);

static void fxWriteChunk(txMachine* the, txSnapshot* snapshot, txSlot* slot);
static void fxWriteChunkArray(txMachine* the, txSnapshot* snapshot, txSlot* address, txSize length, txFlag flag);
static void fxWriteChunkBigInt(txMachine* the, txSnapshot* snapshot, void* address, txSize size);
static void fxWriteChunkData(txMachine* the, txSnapshot* snapshot, void* address);
static void fxWriteChunkTable(txMachine* the, txSnapshot* snapshot, txSlot** address, txSize length);
static void fxWriteChunkZero(txMachine* the, txSnapshot* snapshot, txSize size);
static void fxWriteChunks(txMachine* the, txSnapshot* snapshot);
static void fxWriteSlot(txMachine* the, txSnapshot* snapshot, txSlot* slot, txFlag flag);
static void fxWriteSlotTable(txMachine* the, txSnapshot* snapshot, txSlot** address, txSize length);
static void fxWriteSlots(txMachine* the, txSnapshot* snapshot);
static void fxWriteStack(txMachine* the, txSnapshot* snapshot);

#define mxAssert(_ASSERTION,...) { if (!(_ASSERTION)) { fxReport(the, __VA_ARGS__); snapshot->error = C_EINVAL; fxJump(the); } }
#define mxThrowIf(_ERROR) { if (_ERROR) { snapshot->error = _ERROR; fxJump(the); } }
#define mxChunkFlag 0x80000000

#define mxCallbacksLength 496
static txCallback gxCallbacks[mxCallbacksLength] = {
	fx_AggregateError,
	fx_Array_from,
	fx_Array_isArray,
	fx_Array_of,
	fx_Array_prototype_at,
	fx_Array_prototype_concat,
	fx_Array_prototype_copyWithin,
	fx_Array_prototype_entries,
	fx_Array_prototype_every,
	fx_Array_prototype_fill,
	fx_Array_prototype_filter,
	fx_Array_prototype_find,
	fx_Array_prototype_findIndex,
	fx_Array_prototype_findLast,
	fx_Array_prototype_findLastIndex,
	fx_Array_prototype_flat,
	fx_Array_prototype_flatMap,
	fx_Array_prototype_forEach,
	fx_Array_prototype_includes,
	fx_Array_prototype_indexOf,
	fx_Array_prototype_join,
	fx_Array_prototype_keys,
	fx_Array_prototype_lastIndexOf,
	fx_Array_prototype_map,
	fx_Array_prototype_pop,
	fx_Array_prototype_push,
	fx_Array_prototype_reduce,
	fx_Array_prototype_reduceRight,
	fx_Array_prototype_reverse,
	fx_Array_prototype_shift,
	fx_Array_prototype_slice,
	fx_Array_prototype_some,
	fx_Array_prototype_sort,
	fx_Array_prototype_splice,
	fx_Array_prototype_toLocaleString,
	fx_Array_prototype_toString,
	fx_Array_prototype_unshift,
	fx_Array_prototype_values,
	fx_Array,
	fx_ArrayBuffer_fromBigInt,
	fx_ArrayBuffer_isView,
	fx_ArrayBuffer_prototype_concat,
	fx_ArrayBuffer_prototype_get_byteLength,
	fx_ArrayBuffer_prototype_get_maxByteLength,
	fx_ArrayBuffer_prototype_get_resizable,
	fx_ArrayBuffer_prototype_resize,
	fx_ArrayBuffer_prototype_slice,
	fx_ArrayBuffer_prototype_transfer,
	fx_ArrayBuffer,
	fx_ArrayIterator_prototype_next,
	fx_AsyncFromSyncIterator_prototype_next,
	fx_AsyncFromSyncIterator_prototype_return,
	fx_AsyncFromSyncIterator_prototype_throw,
	fx_AsyncFunction,
	fx_AsyncGenerator_prototype_next,
	fx_AsyncGenerator_prototype_return,
	fx_AsyncGenerator_prototype_throw,
	fx_AsyncGenerator,
	fx_AsyncGeneratorFunction,
	fx_AsyncIterator_asyncIterator,
	fx_Atomics_add,
	fx_Atomics_and,
	fx_Atomics_compareExchange,
	fx_Atomics_exchange,
	fx_Atomics_isLockFree,
	fx_Atomics_load,
	fx_Atomics_notify,
	fx_Atomics_or,
	fx_Atomics_store,
	fx_Atomics_sub,
	fx_Atomics_wait,
	fx_Atomics_xor,
	fx_BigInt_asIntN,
	fx_BigInt_asUintN,
	fx_BigInt_bitLength,
	fx_BigInt_fromArrayBuffer,
	fx_BigInt_prototype_toString,
	fx_BigInt_prototype_valueOf,
	fx_BigInt,
	fx_Boolean_prototype_toString,
	fx_Boolean_prototype_valueOf,
	fx_Boolean,
	fx_Compartment_prototype_evaluate,
	fx_Compartment_prototype_get_globalThis,
	fx_Compartment_prototype_import,
	fx_Compartment_prototype_importNow,
	fx_Compartment,
	fx_DataView_prototype_buffer_get,
	fx_DataView_prototype_byteLength_get,
	fx_DataView_prototype_byteOffset_get,
	fx_DataView_prototype_getBigInt64,
	fx_DataView_prototype_getBigUint64,
	fx_DataView_prototype_getFloat32,
	fx_DataView_prototype_getFloat64,
	fx_DataView_prototype_getInt8,
	fx_DataView_prototype_getInt16,
	fx_DataView_prototype_getInt32,
	fx_DataView_prototype_getUint8,
	fx_DataView_prototype_getUint16,
	fx_DataView_prototype_getUint32,
	fx_DataView_prototype_setBigInt64,
	fx_DataView_prototype_setBigUint64,
	fx_DataView_prototype_setFloat32,
	fx_DataView_prototype_setFloat64,
	fx_DataView_prototype_setInt8,
	fx_DataView_prototype_setInt16,
	fx_DataView_prototype_setInt32,
	fx_DataView_prototype_setUint8,
	fx_DataView_prototype_setUint16,
	fx_DataView_prototype_setUint32,
	fx_DataView,
	fx_Date_now,
	fx_Date_now_secure,
	fx_Date_parse,
	fx_Date_prototype_getDate,
	fx_Date_prototype_getDay,
	fx_Date_prototype_getFullYear,
	fx_Date_prototype_getHours,
	fx_Date_prototype_getMilliseconds,
	fx_Date_prototype_getMinutes,
	fx_Date_prototype_getMonth,
	fx_Date_prototype_getSeconds,
	fx_Date_prototype_getTimezoneOffset,
	fx_Date_prototype_getUTCDate,
	fx_Date_prototype_getUTCDay,
	fx_Date_prototype_getUTCFullYear,
	fx_Date_prototype_getUTCHours,
	fx_Date_prototype_getUTCMilliseconds,
	fx_Date_prototype_getUTCMinutes,
	fx_Date_prototype_getUTCMonth,
	fx_Date_prototype_getUTCSeconds,
	fx_Date_prototype_getYear,
	fx_Date_prototype_setDate,
	fx_Date_prototype_setFullYear,
	fx_Date_prototype_setHours,
	fx_Date_prototype_setMilliseconds,
	fx_Date_prototype_setMinutes,
	fx_Date_prototype_setMonth,
	fx_Date_prototype_setSeconds,
	fx_Date_prototype_setTime,
	fx_Date_prototype_setUTCDate,
	fx_Date_prototype_setUTCFullYear,
	fx_Date_prototype_setUTCHours,
	fx_Date_prototype_setUTCMilliseconds,
	fx_Date_prototype_setUTCMinutes,
	fx_Date_prototype_setUTCMonth,
	fx_Date_prototype_setUTCSeconds,
	fx_Date_prototype_setYear,
	fx_Date_prototype_toDateString,
	fx_Date_prototype_toISOString,
	fx_Date_prototype_toJSON,
	fx_Date_prototype_toPrimitive,
	fx_Date_prototype_toString,
	fx_Date_prototype_toTimeString,
	fx_Date_prototype_toUTCString,
	fx_Date_prototype_valueOf,
	fx_Date_UTC,
	fx_Date,
	fx_Date_secure,
	fx_decodeURI,
	fx_decodeURIComponent,
	fx_encodeURI,
	fx_encodeURIComponent,
	fx_Enumerator_next,
	fx_Enumerator,
	fx_Error_prototype_get_stack,
	fx_Error_toString,
	fx_Error,
	fx_escape,
	fx_eval,
	fx_EvalError,
	// fx_FinalizationRegistry_prototype_cleanupSome,
	fx_FinalizationRegistry_prototype_register,
	fx_FinalizationRegistry_prototype_unregister,
	fx_FinalizationRegistry,
	fx_Function_prototype_apply,
	fx_Function_prototype_bind,
	fx_Function_prototype_bound,
	fx_Function_prototype_call,
	fx_Function_prototype_hasInstance,
	fx_Function_prototype_toString,
	fx_Function,
	fx_Generator_prototype_next,
	fx_Generator_prototype_return,
	fx_Generator_prototype_throw,
	fx_Generator,
	fx_GeneratorFunction,
	fx_isFinite,
	fx_isNaN,
	fx_Iterator_iterator,
	fx_JSON_parse,
	fx_JSON_stringify,
	fx_Map_prototype_clear,
	fx_Map_prototype_delete,
	fx_Map_prototype_entries,
	fx_Map_prototype_forEach,
	fx_Map_prototype_get,
	fx_Map_prototype_has,
	fx_Map_prototype_keys,
	fx_Map_prototype_set,
	fx_Map_prototype_size,
	fx_Map_prototype_values,
	fx_Map,
	fx_MapIterator_prototype_next,
	fx_Math_abs,
	fx_Math_acos,
	fx_Math_acosh,
	fx_Math_asin,
	fx_Math_asinh,
	fx_Math_atan,
	fx_Math_atan2,
	fx_Math_atanh,
	fx_Math_cbrt,
	fx_Math_ceil,
	fx_Math_clz32,
	fx_Math_cos,
	fx_Math_cosh,
	fx_Math_exp,
	fx_Math_expm1,
	fx_Math_floor,
	fx_Math_fround,
	fx_Math_hypot,
	fx_Math_idiv,
	fx_Math_idivmod,
	fx_Math_imod,
	fx_Math_imul,
	fx_Math_imuldiv,
	fx_Math_irem,
	fx_Math_log,
	fx_Math_log1p,
	fx_Math_log2,
	fx_Math_log10,
	fx_Math_max,
	fx_Math_min,
	fx_Math_mod,
	fx_Math_pow,
	fx_Math_random,
	fx_Math_random_secure,
	fx_Math_round,
	fx_Math_sign,
	fx_Math_sin,
	fx_Math_sinh,
	fx_Math_sqrt,
	fx_Math_tan,
	fx_Math_tanh,
	fx_Math_trunc,
	fx_ModuleSource,
	fx_ModuleSource_prototype_get_bindings,
	fx_ModuleSource_prototype_get_needsImport,
	fx_ModuleSource_prototype_get_needsImportMeta,
	fx_Number_isFinite,
	fx_Number_isInteger,
	fx_Number_isNaN,
	fx_Number_isSafeInteger,
	fx_Number_prototype_toExponential,
	fx_Number_prototype_toFixed,
	fx_Number_prototype_toLocaleString,
	fx_Number_prototype_toPrecision,
	fx_Number_prototype_toString,
	fx_Number_prototype_valueOf,
	fx_Number,
	fx_Object_assign,
	fx_Object_copy,
	fx_Object_create,
	fx_Object_defineProperties,
	fx_Object_defineProperty,
	fx_Object_entries,
	fx_Object_freeze,
	fx_Object_fromEntries,
	fx_Object_getOwnPropertyDescriptor,
	fx_Object_getOwnPropertyDescriptors,
	fx_Object_getOwnPropertyNames,
	fx_Object_getOwnPropertySymbols,
	fx_Object_getPrototypeOf,
	fx_Object_hasOwn,
	fx_Object_is,
	fx_Object_isExtensible,
	fx_Object_isFrozen,
	fx_Object_isSealed,
	fx_Object_keys,
	fx_Object_preventExtensions,
	fx_Object_prototype___defineGetter__,
	fx_Object_prototype___defineSetter__,
	fx_Object_prototype___lookupGetter__,
	fx_Object_prototype___lookupSetter__,
	fx_Object_prototype___proto__get,
	fx_Object_prototype___proto__set,
	fx_Object_prototype_hasOwnProperty,
	fx_Object_prototype_isPrototypeOf,
	fx_Object_prototype_propertyIsEnumerable,
	fx_Object_prototype_toLocaleString,
	fx_Object_prototype_toString,
	fx_Object_prototype_valueOf,
	fx_Object_seal,
	fx_Object_setPrototypeOf,
	fx_Object_values,
	fx_Object,
	fx_parseFloat,
	fx_parseInt,
	fx_Promise_all,
	fx_Promise_allSettled,
	fx_Promise_any,
	fx_Promise_prototype_catch,
	fx_Promise_prototype_finally,
	fx_Promise_prototype_then,
	fx_Promise_race,
	fx_Promise_reject,
	fx_Promise_resolve,
	fx_Promise,
	fx_Proxy_revocable,
	fx_Proxy_revoke,
	fx_Proxy,
	fx_RangeError,
	fx_ReferenceError,
	fx_Reflect_apply,
	fx_Reflect_construct,
	fx_Reflect_defineProperty,
	fx_Reflect_deleteProperty,
	fx_Reflect_get,
	fx_Reflect_getOwnPropertyDescriptor,
	fx_Reflect_getPrototypeOf,
	fx_Reflect_has,
	fx_Reflect_isExtensible,
	fx_Reflect_ownKeys,
	fx_Reflect_preventExtensions,
	fx_Reflect_set,
	fx_Reflect_setPrototypeOf,
	fx_RegExp_prototype_compile,
	fx_RegExp_prototype_exec,
	fx_RegExp_prototype_get_dotAll,
	fx_RegExp_prototype_get_flags,
	fx_RegExp_prototype_get_global,
	fx_RegExp_prototype_get_hasIndices,
	fx_RegExp_prototype_get_ignoreCase,
	fx_RegExp_prototype_get_multiline,
	fx_RegExp_prototype_get_source,
	fx_RegExp_prototype_get_sticky,
	fx_RegExp_prototype_get_unicode,
	fx_RegExp_prototype_match,
	fx_RegExp_prototype_matchAll_next,
	fx_RegExp_prototype_matchAll,
	fx_RegExp_prototype_replace,
	fx_RegExp_prototype_search,
	fx_RegExp_prototype_split,
	fx_RegExp_prototype_test,
	fx_RegExp_prototype_toString,
	fx_RegExp,
	fx_Set_prototype_add,
	fx_Set_prototype_clear,
	fx_Set_prototype_delete,
	fx_Set_prototype_entries,
	fx_Set_prototype_forEach,
	fx_Set_prototype_has,
	fx_Set_prototype_size,
	fx_Set_prototype_values,
	fx_Set,
	fx_SetIterator_prototype_next,
	fx_SharedArrayBuffer_prototype_get_byteLength,
	fx_SharedArrayBuffer_prototype_get_growable,
	fx_SharedArrayBuffer_prototype_get_maxByteLength,
	fx_SharedArrayBuffer_prototype_grow,
	fx_SharedArrayBuffer_prototype_slice,
	fx_SharedArrayBuffer,
	fx_species_get,
	fx_String_fromCharCode,
	fx_String_fromCodePoint,
	fx_String_prototype_at,
	fx_String_prototype_charAt,
	fx_String_prototype_charCodeAt,
	fx_String_prototype_codePointAt,
	fx_String_prototype_compare,
	fx_String_prototype_concat,
	fx_String_prototype_endsWith,
	fx_String_prototype_includes,
	fx_String_prototype_indexOf,
	fx_String_prototype_iterator_next,
	fx_String_prototype_iterator,
	fx_String_prototype_lastIndexOf,
	fx_String_prototype_localeCompare,
	fx_String_prototype_match,
	fx_String_prototype_matchAll,
	fx_String_prototype_normalize,
	fx_String_prototype_padEnd,
	fx_String_prototype_padStart,
	fx_String_prototype_repeat,
	fx_String_prototype_replace,
	fx_String_prototype_replaceAll,
	fx_String_prototype_search,
	fx_String_prototype_slice,
	fx_String_prototype_split,
	fx_String_prototype_startsWith,
	fx_String_prototype_substr,
	fx_String_prototype_substring,
	fx_String_prototype_toLowerCase,
	fx_String_prototype_toUpperCase,
	fx_String_prototype_trim,
	fx_String_prototype_trimEnd,
	fx_String_prototype_trimStart,
	fx_String_prototype_valueOf,
	fx_String_raw,
	fx_String,
	fx_Symbol_for,
	fx_Symbol_keyFor,
	fx_Symbol_prototype_get_description,
	fx_Symbol_prototype_toPrimitive,
	fx_Symbol_prototype_toString,
	fx_Symbol_prototype_valueOf,
	fx_Symbol,
	fx_SyntaxError,
	fx_trace_center,
	fx_trace_left,
	fx_trace_right,
	fx_trace,
	fx_TypedArray_from,
	fx_TypedArray_of,
	fx_TypedArray_prototype_at,
	fx_TypedArray_prototype_buffer_get,
	fx_TypedArray_prototype_byteLength_get,
	fx_TypedArray_prototype_byteOffset_get,
	fx_TypedArray_prototype_copyWithin,
	fx_TypedArray_prototype_entries,
	fx_TypedArray_prototype_every,
	fx_TypedArray_prototype_fill,
	fx_TypedArray_prototype_filter,
	fx_TypedArray_prototype_find,
	fx_TypedArray_prototype_findIndex,
	fx_TypedArray_prototype_findLast,
	fx_TypedArray_prototype_findLastIndex,
	fx_TypedArray_prototype_forEach,
	fx_TypedArray_prototype_includes,
	fx_TypedArray_prototype_indexOf,
	fx_TypedArray_prototype_join,
	fx_TypedArray_prototype_keys,
	fx_TypedArray_prototype_lastIndexOf,
	fx_TypedArray_prototype_length_get,
	fx_TypedArray_prototype_map,
	fx_TypedArray_prototype_reduce,
	fx_TypedArray_prototype_reduceRight,
	fx_TypedArray_prototype_reverse,
	fx_TypedArray_prototype_set,
	fx_TypedArray_prototype_slice,
	fx_TypedArray_prototype_some,
	fx_TypedArray_prototype_sort,
	fx_TypedArray_prototype_subarray,
	fx_TypedArray_prototype_toLocaleString,
	fx_TypedArray_prototype_toStringTag_get,
	fx_TypedArray_prototype_values,
	fx_TypedArray,
	fx_TypeError,
	fx_unescape,
	fx_URIError,
	fx_WeakMap_prototype_delete,
	fx_WeakMap_prototype_get,
	fx_WeakMap_prototype_has,
	fx_WeakMap_prototype_set,
	fx_WeakMap,
	fx_WeakRef_prototype_deref,
	fx_WeakRef,
	fx_WeakSet_prototype_add,
	fx_WeakSet_prototype_delete,
	fx_WeakSet_prototype_has,
	fx_WeakSet,
	fxAsyncGeneratorRejectAwait,
	fxAsyncGeneratorRejectYield,
	fxAsyncGeneratorResolveAwait,
	fxAsyncGeneratorResolveYield,
	fxAsyncFromSyncIteratorDone,
	fxArrayLengthGetter,
	fxArrayLengthSetter,
	fxExecuteModulesFulfilled,
	fxExecuteModulesRejected,
	fxExecuteVirtualModuleSource,
	fxExecuteVirtualModuleSourceImport,
	fxInitializeRegExp,
	fxLoadModulesFulfilled,
	fxLoadModulesRejected,
	fxModuleGetter,
	fxOnRejectedPromise,
	fxOnResolvedPromise,
	fxOnThenable,
	fxOrdinaryToPrimitive,
	fxProxyGetter,
	fxProxySetter,
	fxStringAccessorGetter,
	fxStringAccessorSetter,
	fxThrowTypeError,
	fxTypedArrayGetter,
	fxTypedArraySetter,
	fxCombinePromisesCallback,
	fxNewPromiseCapabilityCallback,
	fxRejectAwait,
	fxRejectPromise,
	fxResolveAwait,
	fxResolvePromise,
	fx_Promise_prototype_finallyAux,
	fx_Promise_prototype_finallyReturn,
	fx_Promise_prototype_finallyThrow,
};
extern const txTypeDispatch gxTypeDispatches[];
extern const txTypeAtomics gxTypeAtomics[];

void fxIndexInstance(txMachine* the, txSnapshot* snapshot, txSlot* theCurrent)
{
	txSlot* aProperty;
	txSlot* aTemporary;

	mxCheck(the, theCurrent->kind == XS_INSTANCE_KIND);
	aProperty = theCurrent;
	theCurrent->value.instance.garbage = C_NULL;
	for (;;) {
		if (aProperty) {
			if (!(aProperty->flag & XS_MARK_FLAG)) {
				fxIndexSlot(the, snapshot, aProperty);
				
				switch (aProperty->kind) {
				case XS_INSTANCE_KIND:
					aTemporary = aProperty->value.instance.prototype;
					if (aTemporary && !(aTemporary->flag & XS_MARK_FLAG)) {
						aProperty->value.instance.prototype = theCurrent;
						theCurrent = aTemporary;
						theCurrent->value.instance.garbage = aProperty;
						aProperty = theCurrent;
					}
					else
						aProperty = aProperty->next;
					break;
				case XS_REFERENCE_KIND:
					aTemporary = aProperty->value.reference;
					if (!(aTemporary->flag & XS_MARK_FLAG)) {
						aProperty->value.reference = theCurrent;
						theCurrent = aTemporary;
						theCurrent->value.instance.garbage = aProperty;
						aProperty = theCurrent;
					}
					else
						aProperty = aProperty->next;
					break;
					
				case XS_PROXY_KIND:
					aTemporary = aProperty->value.proxy.handler;
					if (aTemporary && !(aTemporary->flag & XS_MARK_FLAG)) {
						aProperty->flag |= XS_INSPECTOR_FLAG;
						aProperty->value.proxy.handler = theCurrent;
						theCurrent = aTemporary;
						theCurrent->value.instance.garbage = aProperty;
						aProperty = theCurrent;
					}
					else {
						aTemporary = aProperty->value.proxy.target;
						if (aTemporary && !(aTemporary->flag & XS_MARK_FLAG)) {
							aProperty->value.proxy.target = theCurrent;
							theCurrent = aTemporary;
							theCurrent->value.instance.garbage = aProperty;
							aProperty = theCurrent;
						}
						else
							aProperty = aProperty->next;
					}
					break;
					
				case XS_CLOSURE_KIND:
					aTemporary = aProperty->value.closure;
					if (aTemporary && !(aTemporary->flag & XS_MARK_FLAG)) {
						fxIndexSlot(the, snapshot, aTemporary); 
						if (aTemporary->kind == XS_REFERENCE_KIND) {
							aTemporary = aTemporary->value.reference;
							if (!(aTemporary->flag & XS_MARK_FLAG)) {
								aProperty->value.closure->value.reference = theCurrent;
								theCurrent = aTemporary;
								theCurrent->value.instance.garbage = aProperty;
								aProperty = theCurrent;
						
							}
						}
						else {
							fxIndexReference(the, snapshot, aTemporary);
							aProperty = aProperty->next;
						}
					}
					else
						aProperty = aProperty->next;
					break;
					
				default:
					fxIndexReference(the, snapshot, aProperty);
					aProperty = aProperty->next;
					break;	
				}
			}
			else
				aProperty = aProperty->next;
		}
		else if (theCurrent->value.instance.garbage) {
			aProperty = theCurrent->value.instance.garbage;
			theCurrent->value.instance.garbage = C_NULL;
			switch (aProperty->kind) {
			case XS_INSTANCE_KIND:
				aTemporary = aProperty->value.instance.prototype;
				aProperty->value.instance.prototype = theCurrent;
				theCurrent = aTemporary;
				aProperty = aProperty->next;
				break;
			case XS_REFERENCE_KIND:
				aTemporary = aProperty->value.reference;
				aProperty->value.reference = theCurrent;
				theCurrent = aTemporary;
				aProperty = aProperty->next;
				break;
			case XS_PROXY_KIND:
				if (aProperty->flag & XS_INSPECTOR_FLAG) {
					aProperty->flag &= ~XS_INSPECTOR_FLAG;
					aTemporary = aProperty->value.proxy.handler;
					aProperty->value.proxy.handler = theCurrent;
					theCurrent = aTemporary;
					
					aTemporary = aProperty->value.proxy.target;
					if (aTemporary && !(aTemporary->flag & XS_MARK_FLAG)) {
						aProperty->value.proxy.target = theCurrent;
						theCurrent = aTemporary;
						theCurrent->value.instance.garbage = aProperty;
						aProperty = theCurrent;
					}
					else {
						aProperty = aProperty->next;
					}
				}
				else {
					aTemporary = aProperty->value.proxy.target;
					aProperty->value.proxy.target = theCurrent;
					theCurrent = aTemporary;
					aProperty = aProperty->next;
				}
				break;
			case XS_CLOSURE_KIND:
				aTemporary = aProperty->value.closure->value.reference;
				aProperty->value.closure->value.reference = theCurrent;
				theCurrent = aTemporary;
				aProperty = aProperty->next;
				break;
			}
		}
		else
			break;
	}
}

void fxIndexReference(txMachine* the, txSnapshot* snapshot, txSlot* theSlot)
{
	txSlot* aSlot;
	switch (theSlot->kind) {
	case XS_REFERENCE_KIND:
		aSlot = theSlot->value.reference;
		if (!(aSlot->flag & XS_MARK_FLAG))
			fxIndexInstance(the, snapshot, aSlot);
		break;
	case XS_CLOSURE_KIND:
		aSlot = theSlot->value.closure;
		if (aSlot && (!(aSlot->flag & XS_MARK_FLAG))) {
			fxIndexSlot(the, snapshot, aSlot); 
			fxIndexReference(the, snapshot, aSlot);
		}
		break;
	case XS_INSTANCE_KIND:
		if (!(theSlot->flag & XS_MARK_FLAG))
			fxIndexInstance(the, snapshot, theSlot);
		break;
	case XS_ACCESSOR_KIND:
		aSlot = theSlot->value.accessor.getter;
		if (aSlot && !(aSlot->flag & XS_MARK_FLAG))
			fxIndexInstance(the, snapshot, aSlot);
		aSlot = theSlot->value.accessor.setter;
		if (aSlot && !(aSlot->flag & XS_MARK_FLAG))
			fxIndexInstance(the, snapshot, aSlot);
		break;
	case XS_ARGUMENTS_SLOPPY_KIND:
	case XS_ARGUMENTS_STRICT_KIND:
	case XS_ARRAY_KIND:
	case XS_STACK_KIND:
		fxCheckCStack(the);
		if ((aSlot = theSlot->value.array.address)) {
			txIndex aLength = (((txChunk*)(((txByte*)aSlot) - sizeof(txChunk)))->size) / sizeof(txSlot);
			while (aLength) {
				fxIndexReference(the, snapshot, aSlot);
				aSlot++;
				aLength--;
			}
		}
		break;
	case XS_CODE_KIND:
	case XS_CODE_X_KIND:
		aSlot = theSlot->value.code.closures;
		if (aSlot && !(aSlot->flag & XS_MARK_FLAG)) {
			fxCheckCStack(the);
			fxIndexInstance(the, snapshot, aSlot);
		}
		break;
	case XS_HOME_KIND:
		aSlot = theSlot->value.home.object;
		if (aSlot && !(aSlot->flag & XS_MARK_FLAG)) {
			fxCheckCStack(the);
			fxIndexInstance(the, snapshot, aSlot);
		}
		aSlot = theSlot->value.home.module;
		if (aSlot && !(aSlot->flag & XS_MARK_FLAG)) {
			fxCheckCStack(the);
			fxIndexInstance(the, snapshot, aSlot);
		}
		break;
	case XS_MODULE_KIND:
	case XS_PROGRAM_KIND:
		fxCheckCStack(the);
		aSlot = theSlot->value.module.realm;
		if (aSlot && !(aSlot->flag & XS_MARK_FLAG))
			fxIndexInstance(the, snapshot, aSlot);
		break;
	case XS_EXPORT_KIND:
		aSlot = theSlot->value.export.closure;
		if (aSlot && !(aSlot->flag & XS_MARK_FLAG)) {
			fxIndexSlot(the, snapshot, aSlot); 
			fxIndexReference(the, snapshot, aSlot);
		}
		aSlot = theSlot->value.export.module;
		if (aSlot && !(aSlot->flag & XS_MARK_FLAG))
			fxIndexInstance(the, snapshot, aSlot);
		break;
	case XS_HOST_KIND:
		// ??
		break;
	case XS_PROXY_KIND:
		aSlot = theSlot->value.proxy.handler;
		if (aSlot && !(aSlot->flag & XS_MARK_FLAG))
			fxIndexInstance(the, snapshot, aSlot);
		aSlot = theSlot->value.proxy.target;
		if (aSlot && !(aSlot->flag & XS_MARK_FLAG))
			fxIndexInstance(the, snapshot, aSlot);
		break;
		
	case XS_ERROR_KIND:
		aSlot = theSlot->value.error.info;
		if (aSlot && (!(aSlot->flag & XS_MARK_FLAG)))
			fxIndexInstance(the, snapshot, aSlot);
		break;
	case XS_LIST_KIND:
		aSlot = theSlot->value.list.first;
		while (aSlot) {
			if (!(aSlot->flag & XS_MARK_FLAG)) {
				fxIndexSlot(the, snapshot, aSlot);
				fxIndexReference(the, snapshot, aSlot);
			}
			aSlot = aSlot->next;
		}
		break;
		
	case XS_PRIVATE_KIND:
		aSlot = theSlot->value.private.check;
		if (!(aSlot->flag & XS_MARK_FLAG))
			fxIndexInstance(the, snapshot, aSlot);
		aSlot = theSlot->value.private.first;
		while (aSlot) {
			fxIndexSlot(the, snapshot, aSlot);
			fxIndexReference(the, snapshot, aSlot);
			aSlot = aSlot->next;
		}
		break;

	case XS_WEAK_MAP_KIND:
	case XS_WEAK_SET_KIND:
		aSlot = theSlot->value.weakList.first;
		while (aSlot) {
			if (!(aSlot->flag & XS_MARK_FLAG)) {
				fxIndexSlot(the, snapshot, aSlot);
				fxIndexReference(the, snapshot, aSlot);
			}
			aSlot = aSlot->next;
		}
		break;
	case XS_WEAK_ENTRY_KIND:
		aSlot = theSlot->value.weakEntry.check;
		if (aSlot->flag & XS_MARK_FLAG) {
			aSlot = theSlot->value.weakEntry.value;
			if (!(aSlot->flag & XS_MARK_FLAG)) {
				fxIndexSlot(the, snapshot, aSlot); 
				fxIndexReference(the, snapshot, aSlot);
			}
		}
		break;
	case XS_FINALIZATION_REGISTRY_KIND:
		aSlot = theSlot->value.finalizationRegistry.callback;
		if (aSlot) {
			fxIndexSlot(the, snapshot, aSlot);
			fxIndexReference(the, snapshot, aSlot);
			aSlot = aSlot->next;
			while (aSlot) {
				fxIndexSlot(the, snapshot, aSlot);
				fxIndexReference(the, snapshot, aSlot); // holdings
				aSlot = aSlot->next;
				if (aSlot) {
					fxIndexSlot(the, snapshot, aSlot);
					// weak target and token
					aSlot = aSlot->next;
				}
			}
		}
		break;
		
	case XS_HOST_INSPECTOR_KIND:
		aSlot = theSlot->value.hostInspector.cache;
		if (!(aSlot->flag & XS_MARK_FLAG))
			fxIndexInstance(the, snapshot, aSlot);
		break;	
	}
}

void fxIndexSlot(txMachine* the, txSnapshot* snapshot, txSlot* slot)
{
	slot->flag |= XS_MARK_FLAG;
	txProjection* projection = snapshot->firstProjection;
	while (projection) {
		txSlot* heap = projection->heap;
		txSlot* limit = projection->limit;
		if ((heap < slot) && (slot < limit)) {
			projection->indexes[slot - heap] = snapshot->slotSize;
			snapshot->slotSize++;
			return;
		}
		projection = projection->nextProjection;
	}
}

void fxIndexSlots(txMachine* the, txSnapshot* snapshot)
{
	txSlot* heap;
	txSlot* limit;
	txProjection** projectionAddress = &(snapshot->firstProjection);
	txProjection* projection;
	txSlot* slot;
	txSlot** slotAddress;
	txID id;
	size_t* indexAddress;

	heap = the->firstHeap;
	while (heap) {
		limit = heap->value.reference;
		projection = c_calloc(sizeof(txProjection) + (limit - heap - 1) * sizeof(size_t), 1);
		if (!projection) { snapshot->error = C_ENOMEM; fxJump(the); }
		projection->heap = heap;
		projection->limit = limit;
		*projectionAddress = projection;
		projectionAddress = &projection->nextProjection;
		heap = heap->next;
	}
	
	snapshot->slotSize = 1;
	slotAddress = the->keyArray;
	id = the->keyIndex;
	while (id) {
		if ((slot = *slotAddress)) {
			fxIndexSlot(the, snapshot, slot);
			fxIndexReference(the, snapshot, slot);
		}
		slotAddress++;
		id--;
	}	
	slot = the->stackTop;
	while (slot > the->stack) {
        slot--;
		fxIndexReference(the, snapshot, slot);
	}
	
	snapshot->slots = c_calloc(sizeof(txSlot*) * snapshot->slotSize, 1);
	if (!snapshot->slots) { snapshot->error = C_ENOMEM; fxJump(the); }
	projection = snapshot->firstProjection;
	while (projection) {
		heap = projection->heap;
		limit = projection->limit;
		slot = heap + 1;
		indexAddress = &projection->indexes[1];
		while (slot < limit) {
			if (slot->flag & XS_MARK_FLAG) {
				slot->flag &= ~XS_MARK_FLAG; 
				snapshot->slots[*indexAddress] = slot;
			}
			slot++;
			indexAddress++;
		}
		projection = projection->nextProjection;
	}
	
	slot = the->stackTop;
	while (slot > the->stack) {
        slot--;
		slot->flag &= ~XS_MARK_FLAG; 
	}
}

void fxLinkChunks(txMachine* the)
{
	txBlock* block = the->firstBlock;
	while (block) {
		txByte* current = ((txByte*)block) + sizeof(txBlock);
		txByte* limit = block->current;
		while (current < limit) {
			txSize size = ((txChunk*)current)->size;
			size &= ~mxChunkFlag;
			((txChunk*)current)->size = size;
			txByte* next = current + size;
			((txChunk*)current)->temporary = next;
			current = next;
		}	
		block = block->nextBlock;
	}
}

void fxMeasureSlot(txMachine* the, txSnapshot* snapshot, txSlot* slot, txSize* chunkSize)
{
	switch (slot->kind) {
	case XS_STRING_KIND:
		fxMeasureChunk(the, snapshot, slot->value.string, chunkSize);
		break;
	case XS_BIGINT_KIND:
		fxMeasureChunk(the, snapshot, slot->value.bigint.data, chunkSize);
		break;
	case XS_ARGUMENTS_SLOPPY_KIND:
	case XS_ARGUMENTS_STRICT_KIND:
	case XS_ARRAY_KIND:
	case XS_STACK_KIND:
		if (slot->value.array.address)
			fxMeasureChunkArray(the, snapshot, slot->value.array.address, chunkSize);
		break;
	case XS_ARRAY_BUFFER_KIND:
		if (slot->value.arrayBuffer.address)
			fxMeasureChunk(the, snapshot, slot->value.arrayBuffer.address, chunkSize);
		break;
	
	case XS_CODE_KIND:
		fxMeasureChunk(the, snapshot, slot->value.code.address, chunkSize);
		break;
	
	case XS_REGEXP_KIND:
		if (slot->value.regexp.code)
			fxMeasureChunk(the, snapshot, slot->value.regexp.code, chunkSize);
		if (slot->value.regexp.data)
			fxMeasureChunk(the, snapshot, slot->value.regexp.data, chunkSize);
		break;
	case XS_KEY_KIND:
		if (slot->value.key.string)
			fxMeasureChunk(the, snapshot, slot->value.key.string, chunkSize);
		break;
	case XS_GLOBAL_KIND:
	case XS_MAP_KIND:
	case XS_SET_KIND:
		fxMeasureChunk(the, snapshot, slot->value.table.address, chunkSize);
		break;
		
	case XS_HOST_KIND:
#if mxMacOSX || mxLinux
		if (slot->value.host.variant.destructor) {
			txDestructor destructor;
			Dl_info info;
			if (slot->flag & XS_HOST_HOOKS_FLAG)
				destructor = slot->value.host.variant.hooks->destructor;
			else
				destructor = slot->value.host.variant.destructor;
		    if (dladdr(destructor, &info)) {
				mxAssert(0, "# snapshot: no host destructor: %s!\n", info.dli_sname);
			}
			else {
				mxAssert(0, "# snapshot: no host destructor!\n");
			}
		}
#else
		mxAssert(slot->value.host.variant.destructor == C_NULL, "# snapshot: no host destructor!\n");
#endif
		if (slot->value.host.data) {
			mxAssert(slot->flag & XS_HOST_CHUNK_FLAG, "# snapshot: no host data!\n");
			fxMeasureChunk(the, snapshot, slot->value.host.data, chunkSize);
		}
		break;
// 	case XS_PROMISE_KIND:
// 		mxAssert(slot->value.integer != mxPendingStatus, "# snapshot: no pending promise instances!\n");
// 		break;
	case XS_STRING_X_KIND:
		mxAssert(0, "# snapshot: external string %s!\n", slot->value.string);
		break;
	}
}

void fxMeasureChunk(txMachine* the, txSnapshot* snapshot, void* address, txSize* chunkSize)
{
	txChunk* chunk = (txChunk*)(((txByte*)(address)) - sizeof(txChunk));
	if (!(chunk->size & mxChunkFlag)) {
		chunk->temporary = (txByte*)(*chunkSize + sizeof(txChunk));
		*chunkSize += chunk->size;
		chunk->size |= mxChunkFlag;
	}
}

void fxMeasureChunkArray(txMachine* the, txSnapshot* snapshot, txSlot* address, txSize* chunkSize)
{
	txChunk* chunk = (txChunk*)(((txByte*)(address)) - sizeof(txChunk));
	if (!(chunk->size & mxChunkFlag)) {
		txSize size = chunk->size - sizeof(txChunk);
		chunk->temporary = (txByte*)(*chunkSize + sizeof(txChunk));
		*chunkSize += chunk->size;
		chunk->size |= mxChunkFlag;
		while (size > 0) {
			fxMeasureSlot(the, snapshot, address, chunkSize);
			address++;
			size -= sizeof(txSlot);
		}
	}
}

txCallback fxProjectCallback(txMachine* the, txSnapshot* snapshot, txCallback callback) 
{
	if (callback) {
		size_t callbackIndex = 0;
		txCallback* callbackItem = (txCallback*)(gxCallbacks);
		while (callbackIndex < mxCallbacksLength) {
			if (*callbackItem == callback)
				return (txCallback)callbackIndex;
			callbackIndex++;
			callbackItem++;
		}
		callbackIndex = 0;
		callbackItem = snapshot->callbacks;
		while (callbackIndex < (size_t)snapshot->callbacksLength) {
			if (*callbackItem == callback)
				return (txCallback)(mxCallbacksLength + callbackIndex);
			callbackIndex++;
			callbackItem++;
		}
		{
#if mxMacOSX || mxLinux
			Dl_info info;
		    if (dladdr(callback, &info)) {
				mxAssert(0, "# snapshot: unknown callback: %s!\n", info.dli_sname);
			}
		    else
#endif
				mxAssert(0, "# snapshot: unknown callback!\n");
		}
	}
	return C_NULL;
}

void* fxProjectChunk(txMachine* the, void* address) 
{
    if (address) {
        txChunk* chunk = (txChunk*)(((txByte*)(address)) - sizeof(txChunk));
        return chunk->temporary;
    }
    return C_NULL;
}

txSlot* fxProjectSlot(txMachine* the, txProjection* firstProjection, txSlot* slot) 
{
	txSlot* address = NULL;
	if (slot) {
		txProjection* projection = firstProjection;
		while (projection) {
			if ((projection->heap < slot) && (slot < projection->limit)) {
				address = (txSlot*)(projection->indexes[slot - projection->heap]);
				break;
			}
			projection = projection->nextProjection;
		}
	}
	return address;
}

txTypeDispatch* fxProjectTypeDispatch(txMachine* the, txTypeDispatch* dispatch) 
{
	size_t i = 0;
	while (i < mxTypeArrayCount) {
		if (dispatch == &gxTypeDispatches[i])
			return (txTypeDispatch*)i;
		i++;
	}
	fprintf(stderr, "dispatch %p %p\n", dispatch, &gxTypeDispatches[0]);
	return C_NULL;
}

txTypeAtomics* fxProjectTypeAtomics(txMachine* the, txTypeAtomics* atomics) 
{
	size_t i = 0;
	while (i < mxTypeArrayCount) {
		if (atomics == &gxTypeAtomics[i])
			return (txTypeAtomics*)i;
		i++;
	}
	fprintf(stderr, "atomics %p %p\n", atomics, &gxTypeAtomics[0]);
	return C_NULL;
}

void fxReadAtom(txMachine* the, txSnapshot* snapshot, Atom* atom, txString type)
{
	txString check = (txString)&(atom->atomType);
	mxThrowIf((*snapshot->read)(snapshot->stream, atom, sizeof(Atom)));
	atom->atomSize = ntohl(atom->atomSize) - 8;
	mxAssert((check[0] == type[0]) && (check[1] == type[1]) && (check[2] == type[2]) && (check[3] == type[3]), "snapshot: invalid atom %s\n", type);
}

void fxReadMapSet(txMachine* the, txSlot* table, txBoolean paired)
{
	txSlot* list = table->next;
	txSlot* key = list->value.list.first;
	while (key) {
		txU4 sum = fxSumEntry(the, key);
		txU4 index = sum & (table->value.table.length - 1);
		txSlot*  entry = fxNewSlot(the);
		txSlot** address = &(table->value.table.address[index]);
		entry->next = *address;
		entry->kind = XS_ENTRY_KIND;
		entry->value.entry.slot = key;
		entry->value.entry.sum = sum;
		*address = entry;
		key = key->next;
		if (paired)
			key = key->next;
	}
}

txMachine* fxReadSnapshot(txSnapshot* snapshot, txString theName, void* theContext)
{
	Atom atom;
	txByte byte;
	txString signature;
	txCreation creation;
	txSlot* slot;
	
	txMachine* the = (txMachine* )c_calloc(sizeof(txMachine), 1);
	if (the) {
		txJump aJump;
		snapshot->error = 0;
		aJump.nextJump = C_NULL;
		aJump.stack = C_NULL;
		aJump.scope = C_NULL;
		aJump.frame = C_NULL;
		aJump.code = C_NULL;
		aJump.flag = 0;
		the->firstJump = &aJump;
		if (c_setjmp(aJump.buffer) == 0) {
			if (gxDefaults.initializeSharedCluster)
				gxDefaults.initializeSharedCluster();
				
			the->dtoa = fxNew_dtoa(the);
			the->context = theContext;
			fxCreateMachinePlatform(the);

		#ifdef mxDebug
			the->name = theName;
		#endif

			fxReadAtom(the, snapshot, &atom, "XS_M");
			fxReadAtom(the, snapshot, &atom, "VERS");
			mxThrowIf((*snapshot->read)(snapshot->stream, &byte, 1));
			mxAssert(byte == XS_MAJOR_VERSION, "snapshot: invalid major version %d\n", byte);
			mxThrowIf((*snapshot->read)(snapshot->stream, &byte, 1));
			mxAssert(byte == XS_MINOR_VERSION, "snapshot: invalid minor version %d\n", byte);
			mxThrowIf((*snapshot->read)(snapshot->stream, &byte, 1));
// 			mxAssert(byte == XS_PATCH_VERSION, "snapshot: invalid patch version %d\n", byte);
			mxThrowIf((*snapshot->read)(snapshot->stream, &byte, 1));
		#if mxBigEndian
			mxAssert(byte == -((txByte)sizeof(txSlot)), "snapshot: invalid architecture %d\n", byte);
		#else
			mxAssert(byte == (txByte)sizeof(txSlot), "snapshot: invalid architecture %d\n", byte);
		#endif
	
			fxReadAtom(the, snapshot, &atom, "SIGN");
			mxAssert(atom.atomSize == snapshot->signatureLength, "snapshot: invalid signature length %d\n", atom.atomSize);
			signature = snapshot->signature;
			while (atom.atomSize) {
				mxThrowIf((*snapshot->read)(snapshot->stream, &byte, 1));
				mxAssert(byte == *signature, "snapshot: invalid signature byte %d\n", byte);
				atom.atomSize--;
				signature++;
			}
			
			fxReadAtom(the, snapshot, &atom, "CREA");
			mxThrowIf((*snapshot->read)(snapshot->stream, &creation, sizeof(txCreation)));
			fxAllocate(the, &creation);
			mxThrowIf((*snapshot->read)(snapshot->stream, &(the->profileID), sizeof(txID)));
			mxThrowIf((*snapshot->read)(snapshot->stream, &(the->tag), sizeof(txInteger)));
	
			snapshot->firstChunk = the->firstBlock->current;
			snapshot->firstSlot = the->firstHeap;
	
			fxReadAtom(the, snapshot, &atom, "BLOC");
			mxThrowIf((*snapshot->read)(snapshot->stream, the->firstBlock->current, atom.atomSize));
			the->currentChunksSize = atom.atomSize;
			the->firstBlock->current += atom.atomSize;
	
			fxReadAtom(the, snapshot, &atom, "HEAP");
			mxThrowIf((*snapshot->read)(snapshot->stream, the->freeHeap, atom.atomSize));
			the->currentHeapCount = (atom.atomSize / sizeof(txSlot));
			the->freeHeap = the->freeHeap + the->currentHeapCount;
	
            slot = the->firstHeap + 1;
			while (slot < the->freeHeap) {
				fxReadSlot(the, snapshot, slot, 1);
				slot++;
			}
				
			slot = the->firstHeap + 1;
			while (slot < the->freeHeap) {
				switch (slot->kind) {
				case XS_MAP_KIND:
					fxReadMapSet(the, slot, 1);
					break;
				case XS_SET_KIND:
					fxReadMapSet(the, slot, 0);
					break;
				}
				slot++;
			}

			fxReadAtom(the, snapshot, &atom, "STAC");
			the->stack = the->stackTop - (atom.atomSize / sizeof(txSlot));
			mxThrowIf((*snapshot->read)(snapshot->stream, the->stack, atom.atomSize));
	
			slot = the->stack;
			while (slot < the->stackTop) {
				fxReadSlot(the, snapshot, slot, 0);
				slot++;
			}

			fxReadAtom(the, snapshot, &atom, "KEYS");
			the->keyIndex = atom.atomSize / sizeof(txSlot*);
			mxThrowIf((*snapshot->read)(snapshot->stream, the->keyArray, atom.atomSize));
			fxReadSlotTable(the, snapshot, the->keyArray, the->keyIndex);
	
			fxReadAtom(the, snapshot, &atom, "NAME");
			the->nameModulo = atom.atomSize / sizeof(txSlot*);
			mxThrowIf((*snapshot->read)(snapshot->stream, the->nameTable, atom.atomSize));
			fxReadSlotTable(the, snapshot, the->nameTable, the->nameModulo);
	
			fxReadAtom(the, snapshot, &atom, "SYMB");
			the->symbolModulo = atom.atomSize / sizeof(txSlot*);
			mxThrowIf((*snapshot->read)(snapshot->stream, the->symbolTable, atom.atomSize));
			fxReadSlotTable(the, snapshot, the->symbolTable, the->symbolModulo);
			
			slot = &mxDuringJobs;
            the->collectFlag = XS_COLLECTING_FLAG;

			fxLinkChunks(the);

		#ifdef mxDebug
			fxLogin(the);
		#endif

			the->firstJump = C_NULL;
		}
		else {
			fxFree(the);
			c_free(the);
			the = NULL;
			
			if (gxDefaults.terminateSharedCluster)
				gxDefaults.terminateSharedCluster();
		}
	}
	else {
		snapshot->error = C_ENOMEM;
	}
	return the;
}


void fxReadSlot(txMachine* the, txSnapshot* snapshot, txSlot* slot, txFlag flag)
{
	if (flag)
		slot->next = fxUnprojectSlot(the, snapshot, slot->next);
	switch (slot->kind) {
	case XS_STRING_KIND:
		slot->value.string = (txString)mxUnprojectChunk(slot->value.string);
		break;
	case XS_BIGINT_KIND:
		slot->value.bigint.data = (txU4*)mxUnprojectChunk(slot->value.bigint.data);
		break;
	case XS_REFERENCE_KIND:
		slot->value.reference = fxUnprojectSlot(the, snapshot, slot->value.reference);
		break;
	case XS_CLOSURE_KIND:
		slot->value.closure = fxUnprojectSlot(the, snapshot, slot->value.closure);
		break;
	case XS_INSTANCE_KIND:
		slot->value.instance.prototype = fxUnprojectSlot(the, snapshot, slot->value.instance.prototype);
		break;
		
	case XS_ARRAY_KIND:
	case XS_ARGUMENTS_SLOPPY_KIND:
	case XS_ARGUMENTS_STRICT_KIND:
	case XS_STACK_KIND:
		if (slot->value.array.address) {
			slot->value.array.address = (txSlot*)mxUnprojectChunk(slot->value.array.address);
			fxReadSlotArray(the, snapshot, slot->value.array.address, slot->value.array.length);
		}
		break;
	case XS_ARRAY_BUFFER_KIND:
		if (slot->value.array.address)
			slot->value.arrayBuffer.address = (txByte*)mxUnprojectChunk(slot->value.arrayBuffer.address);
		break;
	case XS_CALLBACK_KIND:
		slot->value.callback.address = fxUnprojectCallback(the, snapshot, slot->value.callback.address);
		break;
	case XS_CODE_KIND:
		slot->value.code.address = (txByte*)mxUnprojectChunk(slot->value.code.address);
		slot->value.code.closures = fxUnprojectSlot(the, snapshot, slot->value.code.closures);
		break;
	case XS_CODE_X_KIND:
		slot->value.code.address = (txByte*)gxNoCode;
		break;
	case XS_FINALIZATION_CELL_KIND:
		slot->value.finalizationCell.target = fxUnprojectSlot(the, snapshot, slot->value.finalizationCell.target);
		slot->value.finalizationCell.token = fxUnprojectSlot(the, snapshot, slot->value.finalizationCell.token);
		break;
	case XS_FINALIZATION_REGISTRY_KIND:
		slot->value.finalizationRegistry.callback = fxUnprojectSlot(the, snapshot, slot->value.finalizationRegistry.callback);
		break;
	case XS_TYPED_ARRAY_KIND:
		slot->value.typedArray.dispatch = (txTypeDispatch*)&gxTypeDispatches[(size_t)slot->value.typedArray.dispatch];
		slot->value.typedArray.atomics = (txTypeAtomics*)&gxTypeAtomics[(size_t)slot->value.typedArray.atomics];
		break;
	case XS_GLOBAL_KIND:
	case XS_MAP_KIND:
	case XS_SET_KIND:
		slot->value.table.address = (txSlot**)mxUnprojectChunk(slot->value.table.address);
		fxReadSlotTable(the, snapshot, slot->value.table.address, slot->value.table.length);
		break;
		
	case XS_WEAK_MAP_KIND:
	case XS_WEAK_SET_KIND:
		slot->value.weakList.first = fxUnprojectSlot(the, snapshot, slot->value.weakList.first);
		slot->value.weakList.link = the->firstWeakListLink;
		the->firstWeakListLink = slot;
		break;
	case XS_WEAK_ENTRY_KIND:
		slot->value.weakEntry.check = fxUnprojectSlot(the, snapshot, slot->value.weakEntry.check);
		slot->value.weakEntry.value = fxUnprojectSlot(the, snapshot, slot->value.weakEntry.value);
		break;
	case XS_WEAK_REF_KIND:
		slot->value.weakRef.target = fxUnprojectSlot(the, snapshot, slot->value.weakRef.target);
		slot->value.weakRef.link = fxUnprojectSlot(the, snapshot, slot->value.weakRef.link);
		break;

	case XS_MODULE_KIND:
	case XS_PROGRAM_KIND:
	case XS_MODULE_SOURCE_KIND:
		slot->value.module.realm = fxUnprojectSlot(the, snapshot, slot->value.module.realm);
		break;
		
	case XS_PROXY_KIND:
		slot->value.proxy.handler = fxUnprojectSlot(the, snapshot, slot->value.proxy.handler);
		slot->value.proxy.target = fxUnprojectSlot(the, snapshot, slot->value.proxy.target);
		break;
	case XS_REGEXP_KIND:
		if (slot->value.regexp.code)
			slot->value.regexp.code = (void*)mxUnprojectChunk(slot->value.regexp.code);
		if (slot->value.regexp.data)
			slot->value.regexp.data = (void*)mxUnprojectChunk(slot->value.regexp.data);
		break;
		
	case XS_ACCESSOR_KIND:
		slot->value.accessor.getter = fxUnprojectSlot(the, snapshot, slot->value.accessor.getter);
		slot->value.accessor.setter = fxUnprojectSlot(the, snapshot, slot->value.accessor.setter);
		break;
	case XS_ENTRY_KIND:
		slot->value.entry.slot = fxUnprojectSlot(the, snapshot, slot->value.entry.slot);
		break;
	case XS_ERROR_KIND:
		slot->value.error.info = fxUnprojectSlot(the, snapshot, slot->value.error.info);
		break;
	case XS_HOME_KIND:
		slot->value.home.object = fxUnprojectSlot(the, snapshot, slot->value.home.object);
		slot->value.home.module = fxUnprojectSlot(the, snapshot, slot->value.home.module);
		break;
	case XS_KEY_KIND:
		if (slot->value.key.string)
			slot->value.key.string = (txString)mxUnprojectChunk(slot->value.key.string);
		break;
	case XS_LIST_KIND:
		slot->value.list.first = fxUnprojectSlot(the, snapshot, slot->value.list.first);
		slot->value.list.last = fxUnprojectSlot(the, snapshot, slot->value.list.last);
		break;
	case XS_PRIVATE_KIND:
		slot->value.private.check = fxUnprojectSlot(the, snapshot, slot->value.private.check);
		slot->value.private.first = fxUnprojectSlot(the, snapshot, slot->value.private.first);
		break;
	case XS_EXPORT_KIND:
		slot->value.export.closure = fxUnprojectSlot(the, snapshot, slot->value.export.closure);
		slot->value.export.module = fxUnprojectSlot(the, snapshot, slot->value.export.module);
		break;
		
	case XS_HOST_KIND:
		if (slot->value.host.data)
			slot->value.host.data = (txByte*)mxUnprojectChunk(slot->value.host.data);
		break;
	}
}

void fxReadSlotArray(txMachine* the, txSnapshot* snapshot, txSlot* address, txSize length)
{
	txChunk* chunk = (txChunk*)(((txByte*)(address)) - sizeof(txChunk));
	txSize size = chunk->size - sizeof(txChunk);
	while (size > 0) {
		fxReadSlot(the, snapshot, address, 0);
		address++;
		size -= sizeof(txSlot);
	}
}

void fxReadSlotTable(txMachine* the, txSnapshot* snapshot, txSlot** address, txSize length)
{
	while (length > 0) {
		*address = fxUnprojectSlot(the, snapshot, *address);
		address++;
		length--;
	}
}

void fxUnlinkChunks(txMachine* the)
{
	txBlock* block = the->firstBlock;
	while (block) {
		txByte* current = ((txByte*)block) + sizeof(txBlock);
		txByte* limit = block->current;
		while (current < limit) {
			txSize size = ((txChunk*)current)->size;
			txByte* next = current + size;
			((txChunk*)current)->temporary = C_NULL;
			current = next;
		}	
		block = block->nextBlock;
	}
}

txCallback fxUnprojectCallback(txMachine* the, txSnapshot* snapshot, txCallback callback) 
{
	size_t callbackIndex = (size_t)callback;
	if (callbackIndex < mxCallbacksLength)
		return gxCallbacks[callbackIndex];
	callbackIndex -= mxCallbacksLength;
	if (callbackIndex < (size_t)snapshot->callbacksLength)
		return snapshot->callbacks[callbackIndex];
	mxAssert(0, "# snapshot: unknown callback!\n");
	return C_NULL;
}

txSlot* fxUnprojectSlot(txMachine* the, txSnapshot* snapshot, txSlot* slot)
{
	if (slot)
		slot = snapshot->firstSlot + ((size_t)slot);
	return slot;
}

int fxUseSnapshot(txMachine* the, txSnapshot* snapshot)
{
	Atom atom;
	txByte byte;
	txCreation creation;
	txID profileID;
	txInteger tag;
	txByte* buffer;
	txSize size;
	txSlot* slot;
	mxTry(the) {
		snapshot->error = 0;
		
		fxReadAtom(the, snapshot, &atom, "XS_M");
		fxReadAtom(the, snapshot, &atom, "VERS");
		mxThrowIf((*snapshot->read)(snapshot->stream, &byte, 1));
		mxThrowIf((*snapshot->read)(snapshot->stream, &byte, 1));
		mxThrowIf((*snapshot->read)(snapshot->stream, &byte, 1));
		mxThrowIf((*snapshot->read)(snapshot->stream, &byte, 1));
		fxReadAtom(the, snapshot, &atom, "SIGN");
		while (atom.atomSize) {
			mxThrowIf((*snapshot->read)(snapshot->stream, &byte, 1));
			atom.atomSize--;
		}
		fxReadAtom(the, snapshot, &atom, "CREA");
		mxThrowIf((*snapshot->read)(snapshot->stream, &creation, sizeof(txCreation)));
		mxThrowIf((*snapshot->read)(snapshot->stream, &profileID, sizeof(txID)));
		mxThrowIf((*snapshot->read)(snapshot->stream, &tag, sizeof(txInteger)));
		
		buffer = (txByte*)the->firstBlock;
		size = the->firstBlock->limit - buffer;
		c_memset(buffer, 0, size);
		the->firstBlock->current = buffer + sizeof(txBlock);
		the->firstBlock->limit = buffer + size;

		the->freeHeap = slot = the->firstHeap + 1;
		size = the->maximumHeapCount - 1;
	    while (size--) {
			txSlot* next = slot + 1;
			slot->next = next;
			slot->flag = XS_NO_FLAG;
			slot->kind = XS_UNDEFINED_KIND;
			slot = next;
		}
		slot->next = C_NULL;
		slot->flag = XS_NO_FLAG;
		slot->kind = XS_UNDEFINED_KIND;
		the->firstWeakListLink = C_NULL;

		snapshot->firstChunk = the->firstBlock->current;
		snapshot->firstSlot = the->firstHeap;
		
		fxReadAtom(the, snapshot, &atom, "BLOC");
		mxThrowIf((*snapshot->read)(snapshot->stream, the->firstBlock->current, atom.atomSize));
		the->currentChunksSize = atom.atomSize;
		the->firstBlock->current += atom.atomSize;

		fxReadAtom(the, snapshot, &atom, "HEAP");
		mxThrowIf((*snapshot->read)(snapshot->stream, the->freeHeap, atom.atomSize));
		the->currentHeapCount = (atom.atomSize / sizeof(txSlot));
		the->freeHeap = the->freeHeap + the->currentHeapCount;

		slot = the->firstHeap + 1;
		while (slot < the->freeHeap) {
			fxReadSlot(the, snapshot, slot, 1);
			slot++;
		}
			
		slot = the->firstHeap + 1;
		while (slot < the->freeHeap) {
			switch (slot->kind) {
			case XS_MAP_KIND:
				fxReadMapSet(the, slot, 1);
				break;
			case XS_SET_KIND:
				fxReadMapSet(the, slot, 0);
				break;
			}
			slot++;
		}

		fxReadAtom(the, snapshot, &atom, "STAC");
		the->stack = the->stackTop - (atom.atomSize / sizeof(txSlot));
		mxThrowIf((*snapshot->read)(snapshot->stream, the->stack, atom.atomSize));

		slot = the->stack;
		while (slot < the->stackTop) {
			fxReadSlot(the, snapshot, slot, 0);
			slot++;
		}

		fxReadAtom(the, snapshot, &atom, "KEYS");
		the->keyIndex = atom.atomSize / sizeof(txSlot*);
		mxThrowIf((*snapshot->read)(snapshot->stream, the->keyArray, atom.atomSize));
		fxReadSlotTable(the, snapshot, the->keyArray, the->keyIndex);

		fxReadAtom(the, snapshot, &atom, "NAME");
		the->nameModulo = atom.atomSize / sizeof(txSlot*);
		mxThrowIf((*snapshot->read)(snapshot->stream, the->nameTable, atom.atomSize));
		fxReadSlotTable(the, snapshot, the->nameTable, the->nameModulo);

		fxReadAtom(the, snapshot, &atom, "SYMB");
		the->symbolModulo = atom.atomSize / sizeof(txSlot*);
		mxThrowIf((*snapshot->read)(snapshot->stream, the->symbolTable, atom.atomSize));
		fxReadSlotTable(the, snapshot, the->symbolTable, the->symbolModulo);
	
		fxLinkChunks(the);
	}
	mxCatch(the) {
		
	}
	return (snapshot->error) ? 0 : 1;
}

void fxWriteChunk(txMachine* the, txSnapshot* snapshot, txSlot* slot)
{
	switch (slot->kind) {
	case XS_STRING_KIND:
		fxWriteChunkData(the, snapshot, slot->value.string);
		break;
	case XS_BIGINT_KIND:
		fxWriteChunkBigInt(the, snapshot, slot->value.bigint.data, slot->value.bigint.size);
		break;
	case XS_ARGUMENTS_SLOPPY_KIND:
	case XS_ARGUMENTS_STRICT_KIND:
	case XS_ARRAY_KIND:
		if (slot->value.array.address)
			fxWriteChunkArray(the, snapshot, slot->value.array.address, slot->value.array.length, 2);
		break;
	case XS_STACK_KIND:
		if (slot->value.array.address)
			fxWriteChunkArray(the, snapshot, slot->value.array.address, slot->value.array.length, 0);
		break;
	case XS_ARRAY_BUFFER_KIND:
		if (slot->value.arrayBuffer.address)
			fxWriteChunkData(the, snapshot, slot->value.arrayBuffer.address);
		break;
	
	case XS_CODE_KIND:
		fxWriteChunkData(the, snapshot, slot->value.code.address);
		break;
	
	case XS_REGEXP_KIND:
		if (slot->value.regexp.code)
			fxWriteChunkData(the, snapshot, slot->value.regexp.code);
		if (slot->value.regexp.data)
			fxWriteChunkData(the, snapshot, slot->value.regexp.data);
		break;
	case XS_KEY_KIND:
		if (slot->value.key.string)
			fxWriteChunkData(the, snapshot, slot->value.key.string);
		break;
	case XS_GLOBAL_KIND:
	case XS_MAP_KIND:
	case XS_SET_KIND:
		fxWriteChunkTable(the, snapshot, slot->value.table.address, slot->value.table.length);
		break;
		
	case XS_HOST_KIND:
		if (slot->value.host.data)
			fxWriteChunkData(the, snapshot, slot->value.host.data);
		break;
	}
}

void fxWriteChunkArray(txMachine* the, txSnapshot* snapshot, txSlot* address, txSize length, txFlag flag)
{
	txChunk* chunk = (txChunk*)(((txByte*)(address)) - sizeof(txChunk));
	if (chunk->size & mxChunkFlag) {
		txByte* temporary = chunk->temporary;
		chunk->size &= ~mxChunkFlag;
		chunk->temporary = C_NULL;
		txSize size;
		mxThrowIf((*snapshot->write)(snapshot->stream, chunk, sizeof(txChunk)));
		chunk->temporary = temporary;
		size = chunk->size - sizeof(txChunk);
		while (size > 0) {
			fxWriteSlot(the, snapshot, address, flag);
			address++;
			size -= sizeof(txSlot);
		}
		address = (txSlot*)(((txByte*)(chunk)) + sizeof(txChunk));
		size = chunk->size - sizeof(txChunk);
		while (size > 0) {
			fxWriteChunk(the, snapshot, address);
			address++;
			size -= sizeof(txSlot);
		}
	}
}

void fxWriteChunkBigInt(txMachine* the, txSnapshot* snapshot, void* address, txSize size)
{
	txChunk* chunk = (txChunk*)(((txByte*)(address)) - sizeof(txChunk));
	if (chunk->size & mxChunkFlag) {
		txByte* temporary = chunk->temporary;
		size <<= 2;
		chunk->size &= ~mxChunkFlag;
		chunk->temporary = C_NULL;
		mxThrowIf((*snapshot->write)(snapshot->stream, chunk, sizeof(txChunk) + size));
		chunk->temporary = temporary;
		fxWriteChunkZero(the, snapshot, chunk->size - sizeof(txChunk) - size);
	}
}

void fxWriteChunkData(txMachine* the, txSnapshot* snapshot, void* address)
{
	txChunk* chunk = (txChunk*)(((txByte*)(address)) - sizeof(txChunk));
	if (chunk->size & mxChunkFlag) {
		txByte* temporary = chunk->temporary;
		chunk->size &= ~mxChunkFlag;
		chunk->temporary = C_NULL;
		mxThrowIf((*snapshot->write)(snapshot->stream, chunk, chunk->size));
		chunk->temporary = temporary;
	}
}

void fxWriteChunkTable(txMachine* the, txSnapshot* snapshot, txSlot** address, txSize length)
{
	txChunk* chunk = (txChunk*)(((txByte*)(address)) - sizeof(txChunk));
	if (chunk->size & mxChunkFlag) {
		txByte* temporary = chunk->temporary;
		chunk->size &= ~mxChunkFlag;
		chunk->temporary = C_NULL;
		mxThrowIf((*snapshot->write)(snapshot->stream, chunk, sizeof(txChunk)));
		chunk->temporary = temporary;
		fxWriteSlotTable(the, snapshot, address, length);
		fxWriteChunkZero(the, snapshot, chunk->size - sizeof(txChunk) - (length * sizeof(txSlot*)));
	}
}

void fxWriteChunkZero(txMachine* the, txSnapshot* snapshot, txSize size)
{
	txByte zero = 0;
	while (size > 0) {
		mxThrowIf((*snapshot->write)(snapshot->stream, &zero, sizeof(txByte)));
		size--;
	}
}

void fxWriteChunks(txMachine* the, txSnapshot* snapshot)
{
	size_t slotSize = snapshot->slotSize;
	txSlot** slots = snapshot->slots;
	txSlot* stack = the->stackTop - 1;
	while (stack >= the->stack) {
		fxWriteChunk(the, snapshot, stack);
		stack--;
	}
	slotSize--;
	slots++;
	while (slotSize) {
		txSlot* slot = *slots;
		fxWriteChunk(the, snapshot, slot);
		slotSize--;
		slots++;
	}
}

void fxWriteSlot(txMachine* the, txSnapshot* snapshot, txSlot* slot, txFlag flag)
{
	txSlot buffer;
	c_memset(&buffer, 0, sizeof(buffer));
	if (flag == 1)
		buffer.next = fxProjectSlot(the, snapshot->firstProjection, slot->next);
	else if (flag == 2)
		*((txIndex*)&buffer) = *((txIndex*)slot);
	buffer.ID = slot->ID;
	buffer.flag = slot->flag;
	buffer.kind = slot->kind;
	switch (slot->kind) {
	case XS_UNINITIALIZED_KIND:
	case XS_UNDEFINED_KIND:
	case XS_NULL_KIND:
		break;
	case XS_BOOLEAN_KIND:
		buffer.value.boolean = slot->value.boolean;
		break;
	case XS_INTEGER_KIND:
		buffer.value.integer = slot->value.integer;
		break;
	case XS_NUMBER_KIND:
		buffer.value.number = slot->value.number;
		break;
	case XS_STRING_KIND:
		buffer.value.string = (txString)fxProjectChunk(the, slot->value.string);
		break;
	case XS_SYMBOL_KIND:
		buffer.value.symbol = slot->value.symbol;
		break;
	case XS_BIGINT_KIND:
		buffer.value.bigint.data = (txU4*)fxProjectChunk(the, slot->value.bigint.data);
		buffer.value.bigint.size = slot->value.bigint.size;
		buffer.value.bigint.sign = slot->value.bigint.sign;
		break;
	case XS_REFERENCE_KIND:
		buffer.value.reference = fxProjectSlot(the, snapshot->firstProjection, slot->value.reference);
		break;
	case XS_CLOSURE_KIND:
		buffer.value.closure = fxProjectSlot(the, snapshot->firstProjection, slot->value.closure);
		break;
	case XS_INSTANCE_KIND:
		buffer.value.instance.prototype = fxProjectSlot(the, snapshot->firstProjection, slot->value.instance.prototype);
		break;
		
	case XS_ARRAY_KIND:
	case XS_ARGUMENTS_SLOPPY_KIND:
	case XS_ARGUMENTS_STRICT_KIND:
	case XS_STACK_KIND:
		buffer.value.array.address = (txSlot*)fxProjectChunk(the, slot->value.array.address);
		buffer.value.array.length = slot->value.array.length;
		break;
	case XS_ARRAY_BUFFER_KIND:
		buffer.value.arrayBuffer.address = (txByte*)fxProjectChunk(the, slot->value.arrayBuffer.address);
		break;
	case XS_BUFFER_INFO_KIND:
		buffer.value.bufferInfo.length = slot->value.bufferInfo.length;
		buffer.value.bufferInfo.maxLength = slot->value.bufferInfo.maxLength;
		break;
	case XS_CALLBACK_KIND:
		buffer.value.callback.address = fxProjectCallback(the, snapshot, slot->value.callback.address);
		break;
	case XS_CODE_KIND:
		buffer.value.code.address = (txByte*)fxProjectChunk(the, slot->value.code.address);
		buffer.value.code.closures = fxProjectSlot(the, snapshot->firstProjection, slot->value.code.closures);
		break;
	case XS_CODE_X_KIND:
		break;
	case XS_DATE_KIND:
		buffer.value.number = slot->value.number;
		break;
	case XS_DATA_VIEW_KIND:
		buffer.value.dataView.offset = slot->value.dataView.offset;
		buffer.value.dataView.size = slot->value.dataView.size;
		break;
	case XS_FINALIZATION_CELL_KIND:
		buffer.value.finalizationCell.target = fxProjectSlot(the, snapshot->firstProjection, slot->value.finalizationCell.target);
		buffer.value.finalizationCell.token = fxProjectSlot(the, snapshot->firstProjection, slot->value.finalizationCell.token);
		break;
	case XS_FINALIZATION_REGISTRY_KIND:
		buffer.value.finalizationRegistry.callback = fxProjectSlot(the, snapshot->firstProjection, slot->value.finalizationRegistry.callback);
		buffer.value.finalizationRegistry.flags = slot->value.finalizationRegistry.flags;
		break;
	case XS_TYPED_ARRAY_KIND:
		buffer.value.typedArray.dispatch = fxProjectTypeDispatch(the, slot->value.typedArray.dispatch);
		buffer.value.typedArray.atomics = fxProjectTypeAtomics(the, slot->value.typedArray.atomics);
		break;
	case XS_GLOBAL_KIND:
	case XS_MAP_KIND:
	case XS_SET_KIND:
		buffer.value.table.address = (txSlot**)fxProjectChunk(the, slot->value.table.address);
		buffer.value.table.length = slot->value.table.length;
		break;
		
	case XS_WEAK_MAP_KIND:
	case XS_WEAK_SET_KIND:
		buffer.value.weakList.first = fxProjectSlot(the, snapshot->firstProjection, slot->value.weakList.first);
		buffer.value.weakList.link = C_NULL;
		break;
	case XS_WEAK_ENTRY_KIND:
		buffer.value.weakEntry.check = fxProjectSlot(the, snapshot->firstProjection, slot->value.weakEntry.check);
		buffer.value.weakEntry.value = fxProjectSlot(the, snapshot->firstProjection, slot->value.weakEntry.value);
		break;
	case XS_WEAK_REF_KIND:
		buffer.value.weakRef.target = fxProjectSlot(the, snapshot->firstProjection, slot->value.weakRef.target);
		buffer.value.weakRef.link = fxProjectSlot(the, snapshot->firstProjection, slot->value.weakRef.link);
		break;

	case XS_MODULE_KIND:
	case XS_PROGRAM_KIND:
	case XS_MODULE_SOURCE_KIND:
		buffer.value.module.realm = fxProjectSlot(the, snapshot->firstProjection, slot->value.module.realm);
		buffer.value.module.id = slot->value.module.id;
		break;
		
	case XS_PROMISE_KIND:
		buffer.value.integer = slot->value.integer;
		break;
	case XS_PROXY_KIND:
		buffer.value.proxy.handler = fxProjectSlot(the, snapshot->firstProjection, slot->value.proxy.handler);
		buffer.value.proxy.target = fxProjectSlot(the, snapshot->firstProjection, slot->value.proxy.target);
		break;
	case XS_REGEXP_KIND:
		buffer.value.regexp.code = (void*)fxProjectChunk(the, slot->value.regexp.code);
		buffer.value.regexp.data = (void*)fxProjectChunk(the, slot->value.regexp.data);
		break;
		
	case XS_ACCESSOR_KIND:
		buffer.value.accessor.getter = fxProjectSlot(the, snapshot->firstProjection, slot->value.accessor.getter);
		buffer.value.accessor.setter = fxProjectSlot(the, snapshot->firstProjection, slot->value.accessor.setter);
		break;
	case XS_AT_KIND:
		buffer.value.at.index = slot->value.at.index;
		buffer.value.at.id = slot->value.at.id;
		break;
	case XS_ENTRY_KIND:
		buffer.value.entry.slot = fxProjectSlot(the, snapshot->firstProjection, slot->value.entry.slot);
		buffer.value.entry.sum = slot->value.entry.sum;
		break;
	case XS_ERROR_KIND:
		buffer.value.error.info = fxProjectSlot(the, snapshot->firstProjection, slot->value.error.info);
		buffer.value.error.which = slot->value.error.which;
		break;
	case XS_HOME_KIND:
		buffer.value.home.object = fxProjectSlot(the, snapshot->firstProjection, slot->value.home.object);
		buffer.value.home.module = fxProjectSlot(the, snapshot->firstProjection, slot->value.home.module);
		break;
	case XS_KEY_KIND:
	case XS_KEY_X_KIND:
		buffer.value.key.string = (txString)fxProjectChunk(the, slot->value.key.string);
		buffer.value.key.sum = slot->value.key.sum;
		break;
	case XS_LIST_KIND:
		buffer.value.list.first = fxProjectSlot(the, snapshot->firstProjection, slot->value.list.first);
		buffer.value.list.last = fxProjectSlot(the, snapshot->firstProjection, slot->value.list.last);
		break;
	case XS_PRIVATE_KIND:
		buffer.value.private.check = fxProjectSlot(the, snapshot->firstProjection, slot->value.private.check);
		buffer.value.private.first = fxProjectSlot(the, snapshot->firstProjection, slot->value.private.first);
		break;
	case XS_EXPORT_KIND:
		buffer.value.export.closure = fxProjectSlot(the, snapshot->firstProjection, slot->value.export.closure);
		buffer.value.export.module = fxProjectSlot(the, snapshot->firstProjection, slot->value.export.module);
		break;
	case XS_FRAME_KIND:
		buffer.next = C_NULL;
		break;
		
	case XS_HOST_KIND:
		buffer.value.host.data = fxProjectChunk(the, slot->value.host.data);
		buffer.value.host.variant.destructor = C_NULL;
		break;
	default:
		fxReport(the, "# snapshot: invalid slot %d!\n", slot->kind);
		break;
	}
	mxThrowIf((*snapshot->write)(snapshot->stream, &buffer, sizeof(buffer)));
}

void fxWriteSlotTable(txMachine* the, txSnapshot* snapshot, txSlot** address, txSize length)
{
	while (length > 0) {
		txSlot* buffer = fxProjectSlot(the, snapshot->firstProjection, *address);
		mxThrowIf((*snapshot->write)(snapshot->stream, &buffer, sizeof(txSlot*)));
		address++;
		length--;
	}
}

void fxWriteSlots(txMachine* the, txSnapshot* snapshot)
{
	size_t slotSize = snapshot->slotSize;
	txSlot** slots = snapshot->slots;
	slotSize--;
	slots++;
	while (slotSize) {
		txSlot* slot = *slots;
		fxWriteSlot(the, snapshot, slot, 1);
		slotSize--;
		slots++;
	}
}

void fxWriteStack(txMachine* the, txSnapshot* snapshot)
{
	txSlot* slot = the->stack;
	while (slot < the->stackTop) {
		fxWriteSlot(the, snapshot, slot, 0);
		slot++;
	}
}

int fxWriteSnapshot(txMachine* the, txSnapshot* snapshot)
{
	txSlot* heap;
	txSlot* stack;
	txSlot** slots;
	txSize size;
	txByte byte;
	txProjection** projectionAddress = &(snapshot->firstProjection);
	txProjection* projection;
	txSize chunkSize = 0;
	txSize namesSize = the->nameModulo * sizeof(txSlot*);
	txSize keysSize = the->keyIndex * sizeof(txSlot*);
	txSize slotSize = 0;
	txSize stackSize = (txSize)((the->stackTop - the->stack) * sizeof(txSlot));
	txSize symbolsSize = the->symbolModulo * sizeof(txSlot*);
	txCreation creation;
	
	mxTry(the) {
		snapshot->error = 0;
		fxCollectGarbage(the);
		fxUnlinkChunks(the);
		
		fxIndexSlots(the, snapshot);
	
		stack = the->stackTop - 1;
		while (stack >= the->stack) {
			fxMeasureSlot(the, snapshot, stack, &chunkSize);
			stack--;
		}
		slotSize = snapshot->slotSize;
		slots = snapshot->slots;
		slotSize--;
		slots++;
		while (slotSize) {
			txSlot* slot = *slots;
			fxMeasureSlot(the, snapshot, slot, &chunkSize);
			slotSize--;
			slots++;
		}

		slotSize = (snapshot->slotSize - 1) * sizeof(txSlot);
	
		creation.initialChunkSize = the->maximumChunksSize;
		creation.incrementalChunkSize = the->minimumChunksSize;
		creation.initialHeapCount = the->maximumHeapCount + 1;
		creation.incrementalHeapCount = the->minimumHeapCount;
		creation.stackCount = (txSize)(the->stackTop - the->stackBottom);
		creation.initialKeyCount = the->keyCount;
		creation.incrementalKeyCount = the->keyDelta;
		creation.nameModulo = the->nameModulo;
		creation.symbolModulo = the->symbolModulo;
		creation.parserBufferSize = the->parserBufferSize;
		creation.parserTableModulo = the->parserTableModulo;
		creation.staticSize = 0;
	
		size = 8 
			+ 8 + 4 
			+ 8 + snapshot->signatureLength
			+ 8 + sizeof(txCreation) 
			+ 8 + chunkSize
			+ 8 + slotSize
			+ 8 + stackSize
			+ 8 + keysSize
			+ 8 + namesSize 
			+ 8 + symbolsSize 
			;

		size = htonl(size);
		mxThrowIf((*snapshot->write)(snapshot->stream, &size, 4));
		mxThrowIf((*snapshot->write)(snapshot->stream, "XS_M", 4));

		size = 8 + 4;
		size = htonl(size);
		mxThrowIf((*snapshot->write)(snapshot->stream, &size, 4));
		mxThrowIf((*snapshot->write)(snapshot->stream, "VERS", 4));
		byte = XS_MAJOR_VERSION;
		mxThrowIf((*snapshot->write)(snapshot->stream, &byte, 1));
		byte = XS_MINOR_VERSION;
		mxThrowIf((*snapshot->write)(snapshot->stream, &byte, 1));
		byte = XS_PATCH_VERSION;
		mxThrowIf((*snapshot->write)(snapshot->stream, &byte, 1));
	#if mxBigEndian
		byte = -((txByte)sizeof(txSlot));
	#else
		byte = (txByte)sizeof(txSlot);
	#endif
		mxThrowIf((*snapshot->write)(snapshot->stream, &byte, 1));

		size = 8 + snapshot->signatureLength;
		size = htonl(size);
		mxThrowIf((*snapshot->write)(snapshot->stream, &size, 4));
		mxThrowIf((*snapshot->write)(snapshot->stream, "SIGN", 4));
		mxThrowIf((*snapshot->write)(snapshot->stream, snapshot->signature, snapshot->signatureLength));

		size = 8 + sizeof(txCreation) + sizeof(txID) + sizeof(txInteger);
		size = htonl(size);
		mxThrowIf((*snapshot->write)(snapshot->stream, &size, 4));
		mxThrowIf((*snapshot->write)(snapshot->stream, "CREA", 4));
		mxThrowIf((*snapshot->write)(snapshot->stream, &creation, sizeof(txCreation)));
		mxThrowIf((*snapshot->write)(snapshot->stream, &(the->profileID), sizeof(txID)));
		mxThrowIf((*snapshot->write)(snapshot->stream, &(the->tag), sizeof(txInteger)));

		size = 8 + chunkSize;
		size = htonl(size);
		mxThrowIf((*snapshot->write)(snapshot->stream, &size, 4));
		mxThrowIf((*snapshot->write)(snapshot->stream, "BLOC", 4));
		fxWriteChunks(the, snapshot);
	
		size = 8 + slotSize;
		size = htonl(size);
		mxThrowIf((*snapshot->write)(snapshot->stream, &size, 4));
		mxThrowIf((*snapshot->write)(snapshot->stream, "HEAP", 4));
		fxWriteSlots(the, snapshot);

		size = 8 + stackSize;
		size = htonl(size);
		mxThrowIf((*snapshot->write)(snapshot->stream, &size, 4));
		mxThrowIf((*snapshot->write)(snapshot->stream, "STAC", 4));
		fxWriteStack(the, snapshot);
	
		size = 8 + keysSize;
		size = htonl(size);
		mxThrowIf((*snapshot->write)(snapshot->stream, &size, 4));
		mxThrowIf((*snapshot->write)(snapshot->stream, "KEYS", 4));
		fxWriteSlotTable(the, snapshot, the->keyArray, the->keyIndex);

		size = 8 + namesSize;
		size = htonl(size);
		mxThrowIf((*snapshot->write)(snapshot->stream, &size, 4));
		mxThrowIf((*snapshot->write)(snapshot->stream, "NAME", 4));
		fxWriteSlotTable(the, snapshot, the->nameTable, the->nameModulo);

		size = 8 + symbolsSize;
		size = htonl(size);
		mxThrowIf((*snapshot->write)(snapshot->stream, &size, 4));
		mxThrowIf((*snapshot->write)(snapshot->stream, "SYMB", 4));
		fxWriteSlotTable(the, snapshot, the->symbolTable, the->symbolModulo);
	}
	mxCatch(the) {
		
	}
	if (snapshot->slots)
		c_free(snapshot->slots);
	
	projectionAddress = &(snapshot->firstProjection);
	while ((projection = *projectionAddress)) {
		*projectionAddress = projection->nextProjection;
		c_free(projection);
	}

	heap = the->freeHeap;
	while (heap) {
		heap->flag &= ~XS_MARK_FLAG;
		heap = heap->next;
	}
	
	fxLinkChunks(the);
	
	return (snapshot->error) ? 0 : 1;
}
