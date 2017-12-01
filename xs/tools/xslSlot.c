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

static txString fxGetBuilderName(txMachine* the, const txHostFunctionBuilder* which);
static txString fxGetCallbackName(txMachine* the, txCallback callback); 
static txString fxGetCodeName(txMachine* the, txByte* which);
static txInteger fxGetTypeDispatchIndex(txTypeDispatch* dispatch);
static void fxPrintAddress(txMachine* the, FILE* file, txSlot* slot);
static void fxPrintNumber(txMachine* the, FILE* file, txNumber value);
static void fxPrintSlot(txMachine* the, FILE* file, txSlot* slot, txFlag flag, txBoolean dummy);

txCallback gxFakeCallback = (txCallback)1;

txString fxGetBuilderName(txMachine* the, const txHostFunctionBuilder* which) 
{
	txLinker* linker = xsGetContext(the);
	static char buffer[1024];
	const txHostFunctionBuilder* builder;
	{
		txLinkerBuilder* linkerBuilder = linker->firstBuilder;
		while (linkerBuilder) {
			builder = &(linkerBuilder->host);
			if (builder == which) {
				sprintf(buffer, "&gxBuilders[%d]", linkerBuilder->builderIndex);
				return buffer;
			}
			linkerBuilder = linkerBuilder->nextBuilder;
		}
	}
	{
		txLinkerScript* linkerScript = linker->firstScript;
		while (linkerScript) {
			txInteger count = linkerScript->hostsCount;
			if (count) {
				txInteger index;
				for (builder = linkerScript->builders, index = 0; index < count; builder++, index++) {
					if (builder == which) {
						sprintf(buffer, "&gxBuilders%d[%d]", linkerScript->scriptIndex, index);
						return buffer;
					}
				}
			}
			linkerScript = linkerScript->nextScript;
		}
	}
	return "OOPS";
}

txString fxGetCallbackName(txMachine* the, txCallback which) 
{
	txLinker* linker = xsGetContext(the);
	if (!which)
		return "NULL";
	{
		txLinkerCallback* linkerCallback = fxGetLinkerCallbackByAddress(linker, which);
		if (linkerCallback) {
			if (linkerCallback->flag)
				return linkerCallback->name;
			return "fxDeadStrip";
		}
	}
	{
		txLinkerScript* linkerScript = linker->firstScript;
		while (linkerScript) {
			txInteger count = linkerScript->hostsCount;
			if (count) {
				txCallbackName* address = &(linkerScript->callbackNames[0]);
				while (count) {
					if (address->callback == which)
						return address->name;
					address++;
					count--;
				}
			}
			linkerScript = linkerScript->nextScript;
		}
	}
	return "OOPS";
}

txString fxGetCodeName(txMachine* the, txByte* which) 
{
	if (which == gxNoCode)
		return "(txByte*)(gxNoCode)";
	{
		static char buffer[1024];
		txLinker* linker = xsGetContext(the);
		txLinkerScript* linkerScript = linker->firstScript;
		while (linkerScript) {
			txS1* from = linkerScript->codeBuffer;
			txS1* to = from + linkerScript->codeSize;
			if ((from <= which) && (which < to)) {
				sprintf(buffer, "(txByte*)(&gxCode%d[%ld])", linkerScript->scriptIndex, (long)(which - from));
				return buffer;
			}
			linkerScript = linkerScript->nextScript;
		}
	}
	return "OOPS";
}

extern const txTypeDispatch gxTypeDispatches[];
txInteger fxGetTypeDispatchIndex(txTypeDispatch* dispatch) 
{
	txInteger i = 0;
	while (i < 9) {
		if (dispatch == &gxTypeDispatches[i])
			return i;
		i++;
	}
	fprintf(stderr, "dispatch %p %p\n", dispatch, &gxTypeDispatches[0]);
	return -1;
}

extern const txTypeAtomics gxTypeAtomics[];
txInteger fxGetTypeAtomicsIndex(txTypeAtomics* atomics) 
{
	txInteger i = 0;
	while (i < 9) {
		if (atomics == &gxTypeAtomics[i])
			return i;
		i++;
	}
	fprintf(stderr, "atomics %p %p\n", atomics, &gxTypeAtomics[0]);
	return -1;
}

void fxLinkerScriptCallback(txMachine* the)
{
	txLinker* linker = xsGetContext(the);
    txSlot* slot = mxModuleInternal(mxThis);
    txSlot* key = fxGetKey(the, slot->value.symbol);
    txString path = key->value.key.string + linker->baseLength;
	txLinkerScript* linkerScript = linker->firstScript;
	mxPush(mxArrayPrototype);
	fxNewArrayInstance(the);
	fxArrayCacheBegin(the, the->stack);
	while (linkerScript) {
		if (!c_strcmp(linkerScript->path, path)) {
			txByte* p = linkerScript->hostsBuffer;
			if (p) {
				txID c, i, id;
				mxDecode2(p, c);
				linkerScript->builders = fxNewLinkerChunk(linker, c * sizeof(txHostFunctionBuilder));
				linkerScript->callbackNames = fxNewLinkerChunk(linker, c * sizeof(txCallbackName));
				linkerScript->hostsCount = c;
				for (i = 0; i < c; i++) {
					txCallback callback = (txCallback)(((txU1*)gxFakeCallback)++);
					txHostFunctionBuilder* builder = &linkerScript->builders[i];
					txCallbackName* callbackName = &linkerScript->callbackNames[i];
					txS1 length = *p++;
					mxDecode2(p, id);
					builder->callback = callback;
					builder->length = length;
					builder->id = id;
					callbackName->callback = callback;
					callbackName->name = (char*)p;
					if (length < 0) {
						fxNewHostObject(the, (txDestructor)callback);
					}
					else {
						mxPushUndefined();
						the->stack->kind = XS_HOST_FUNCTION_KIND;
						the->stack->value.hostFunction.builder = builder;
						the->stack->value.hostFunction.IDs = NULL;
					}
					fxArrayCacheItem(the, the->stack + 1, the->stack);
					the->stack++;
					p += c_strlen((char*)p) + 1;
				}
			}
			break;
		}	
		linkerScript = linkerScript->nextScript;
	}
	fxArrayCacheEnd(the, the->stack);
	mxPullSlot(mxResult);
	mxPop();
}

txSlot* fxNextHostAccessorProperty(txMachine* the, txSlot* property, txCallback get, txCallback set, txID id, txFlag flag)
{
	txLinker* linker = (txLinker*)(the->context);
	txSlot *getter = NULL, *setter = NULL, *home = the->stack, *slot;
	if (get) {
		fxNewLinkerBuilder(linker, get, 0, id);
		getter = fxNewHostFunction(the, get, 0, XS_NO_ID);
		slot = mxFunctionInstanceHome(getter);
		slot->value.home.object = home->value.reference;
		fxRenameFunction(the, getter, id, XS_NO_ID, "get ");
	}
	if (set) {
		fxNewLinkerBuilder(linker, set, 1, id);
		setter = fxNewHostFunction(the, set, 1, XS_NO_ID);
		slot = mxFunctionInstanceHome(setter);
		slot->value.home.object = home->value.reference;
		fxRenameFunction(the, setter, id, XS_NO_ID, "set ");
	}
	property = property->next = fxNewSlot(the);
	property->flag = flag;
	property->ID = id;
	property->kind = XS_ACCESSOR_KIND;
	property->value.accessor.getter = getter;
	property->value.accessor.setter = setter;
	if (set)
		the->stack++;
	if (get)
		the->stack++;
	return property;
}

txSlot* fxNextHostFunctionProperty(txMachine* the, txSlot* property, txCallback call, txInteger length, txID id, txFlag flag)
{
	txLinker* linker = (txLinker*)(the->context);
	property = property->next = fxNewSlot(the);
	property->flag = flag;
	property->ID = id;
	property->kind = XS_HOST_FUNCTION_KIND;
	property->value.hostFunction.builder = fxNewLinkerBuilder(linker, call, length, id);
	property->value.hostFunction.IDs = (txID*)the->code;
	return property;
}

txSlot* fxNewHostConstructorGlobal(txMachine* the, txCallback call, txInteger length, txID id, txFlag flag)
{
	txLinker* linker = (txLinker*)(the->context);
	txSlot *function, *property;
	fxNewLinkerBuilder(linker, call, length, id);
	function = fxNewHostConstructor(the, call, length, id);
	property = fxGlobalSetProperty(the, mxGlobal.value.reference, id, XS_NO_ID, XS_OWN);
	property->flag = flag;
	property->kind = the->stack->kind;
	property->value = the->stack->value;
	return function;
}

txSlot* fxNewHostFunctionGlobal(txMachine* the, txCallback call, txInteger length, txID id, txFlag flag)
{
	txLinker* linker = (txLinker*)(the->context);
	txSlot *function, *property;
	fxNewLinkerBuilder(linker, call, length, id);
	function = fxNewHostFunction(the, call, length, id);
	property = fxGlobalSetProperty(the, mxGlobal.value.reference, id, XS_NO_ID, XS_OWN);
	property->flag = flag;
	property->kind = the->stack->kind;
	property->value = the->stack->value;
	return function;
}

txInteger fxPrepareHeap(txMachine* the)
{
	txLinker* linker = (txLinker*)(the->context);
	txID aliasCount = 0;
	txInteger index = 1;
	txSlot *heap, *slot, *limit, *item;
	txLinkerProjection* projection;
	txSlot* home = NULL;

	heap = the->firstHeap;
	while (heap) {
		slot = heap + 1;
		limit = heap->value.reference;
		while (slot < limit) {
			txSlot* next = slot->next;
			if (next && (next->next == NULL) && (next->ID == XS_NO_ID) && (next->kind == XS_HOME_KIND)) {
				if (home && (home->flag == next->flag) && (home->value.home.object == next->value.home.object) && (home->value.home.module == next->value.home.module))
					slot->next = home;
				else
					home = next;
			}
			slot++;
		}
		heap = heap->next;
	}
	xsCollectGarbage();

	slot = the->freeHeap;
	while (slot) {
		slot->flag |= XS_MARK_FLAG;
		slot = slot->next;
	}
	
	heap = the->firstHeap;
	while (heap) {
		slot = heap + 1;
		limit = heap->value.reference;
		projection = fxNewLinkerChunkClear(linker, sizeof(txLinkerProjection) + (limit - slot) * sizeof(txInteger));
		projection->nextProjection = linker->firstProjection;
		projection->heap = heap;
		projection->limit = limit;
		linker->firstProjection = projection;
		while (slot < limit) {
			if (!(slot->flag & XS_MARK_FLAG)) {
				projection->indexes[slot - heap] = index;
				index++;
				if ((slot->kind == XS_ARRAY_KIND) && ((item = slot->value.array.address))) {
					txInteger size = (txInteger)fxGetIndexSize(the, slot);
					index++;
					while (size) {
						index++;
						if (item->kind != XS_ACCESSOR_KIND) 
							item->flag |= XS_DONT_SET_FLAG;
						item->flag |= XS_DONT_DELETE_FLAG;
						item++;
						size--;
					}
					slot->flag |= XS_DONT_DELETE_FLAG | XS_DONT_SET_FLAG;
				}
				else if (slot->kind == XS_CLOSURE_KIND) {
					slot->value.closure->flag |= XS_DONT_SET_FLAG;
				}
				else if (slot->kind == XS_EXPORT_KIND) {
					if (slot->value.export.closure)
						slot->value.export.closure->flag |= XS_DONT_SET_FLAG;
				}
				else if (slot->kind == XS_INSTANCE_KIND) {
					txSlot *property = slot->next;
					if (property && ((property->kind == XS_ARRAY_KIND) || (property->kind == XS_CALLBACK_KIND) || (property->kind == XS_CALLBACK_X_KIND) || (property->kind == XS_CODE_KIND) || (property->kind == XS_CODE_X_KIND))) {
						if (slot != mxArrayPrototype.value.reference)
							slot->ID = XS_NO_ID;
						else
							slot->ID = aliasCount++;
						while (property) {
							if (property->kind != XS_ACCESSOR_KIND) 
								property->flag |= XS_DONT_SET_FLAG;
							property->flag |= XS_DONT_DELETE_FLAG;
							property = property->next;
						}
						slot->flag |= XS_DONT_PATCH_FLAG;
					}
					else if (property && (property->kind == XS_MODULE_KIND)) {
						property = property->next;
						slot->ID = XS_NO_ID;
					}
					else if (property && (property->kind == XS_PROXY_KIND)) {
						slot->ID = aliasCount++;
					}
					else if (property && (property->kind == XS_WITH_KIND)) {
						slot->ID = XS_NO_ID;
					}
					else {
						txBoolean frozen = (slot->flag & XS_DONT_PATCH_FLAG) ? 1 : 0;
						if (frozen) {
							while (property) {
								if (property->kind != XS_ACCESSOR_KIND) 
									if (!(property->flag & XS_DONT_SET_FLAG))
										frozen = 0;
								if (!(property->flag & XS_DONT_DELETE_FLAG))
									frozen = 0;
								property = property->next;
							}
						}
						if (frozen)
							slot->ID = XS_NO_ID;
						else
							slot->ID = aliasCount++;
					}
				}
			}
			slot++;
		}
		heap = heap->next;
	}
	index++;
	
	the->aliasCount = aliasCount;
	return index;
}

void fxPrintAddress(txMachine* the, FILE* file, txSlot* slot) 
{
	if (slot) {
		txLinker* linker = (txLinker*)(the->context);
		txLinkerProjection* projection = linker->firstProjection;
		while (projection) {
			if ((projection->heap < slot) && (slot < projection->limit)) {
				fprintf(file, "(txSlot*)&gxHeap[%d]", (int)projection->indexes[slot - projection->heap]);
				return;
			}
			projection = projection->nextProjection;
		}
		fprintf(file, "OOPS");
	}
	else
		fprintf(file, "NULL");
}

void fxPrintBuilders(txMachine* the, FILE* file)
{
	txLinker* linker = (txLinker*)(the->context);
	txLinkerBuilder* linkerBuilder = linker->firstBuilder;
	fprintf(file, "static const xsHostBuilder gxBuilders[%d] = {\n", linker->builderCount);
	while (linkerBuilder) {
		txString name = fxGetCallbackName(the, linkerBuilder->host.callback);
		fprintf(file, "\t{ %s, %d, %d },\n", name, linkerBuilder->host.length, linkerBuilder->host.id);
		linkerBuilder = linkerBuilder->nextBuilder;
	}
	fprintf(file, "};\n\n");
}

void fxPrintHeap(txMachine* the, FILE* file, txInteger count, txBoolean dummy) 
{
	txLinker* linker = (txLinker*)(the->context);
	txLinkerProjection* projection = linker->firstProjection;
	txSlot *slot, *limit, *item;
	txInteger index = 0;
	fprintf(file, "/* %.4d */", index);
	fprintf(file, "\t{ NULL, XS_NO_ID, XS_NO_FLAG, XS_REFERENCE_KIND, ");
	if (dummy) fprintf(file, "0, ");
	fprintf(file, "{ .reference = (txSlot*)&gxHeap[%d] } },\n", (int)count);
	index++;
	while (projection) {
		slot = projection->heap + 1;
		limit = projection->limit;
		while (slot < limit) {
			if (!(slot->flag & XS_MARK_FLAG)) {
				fprintf(file, "/* %.4d */", index);
				index++;
				if ((slot->kind == XS_ARRAY_KIND) && ((item = slot->value.array.address))) {
					txInteger size = (txInteger)fxGetIndexSize(the, slot);
					fprintf(file, "\t{ ");
					fxPrintAddress(the, file, slot->next);
					if (slot->ID == XS_NO_ID)
						fprintf(file, ", XS_NO_ID");
					else
						fprintf(file, ", %d", slot->ID);
					fprintf(file, ", 0x%x", slot->flag | XS_MARK_FLAG);
					fprintf(file, ", XS_ARRAY_KIND, { .array = { (txSlot*)&gxHeap[%d], %d } }},\n", (int)index + 1, (int)slot->value.array.length);
					fprintf(file, "/* %.4d */", index);
					index++;
					fprintf(file, "\t{ NULL, XS_NO_ID, 0x80, XS_INTEGER_KIND, { .integer = %d } },\n", (int)(size * sizeof(txSlot))); // fake chunk
					while (size) {
						fprintf(file, "/* %.4d */", index);
						index++;
						item->flag |= XS_DEBUG_FLAG;
						fxPrintSlot(the, file, item, XS_MARK_FLAG, dummy);
						item++;
						size--;
					}
				}
				else {
					fxPrintSlot(the, file, slot, XS_MARK_FLAG, dummy);
				}
			}
			slot++;
		}
		projection = projection->nextProjection;
	}	
	fprintf(file, "/* %.4d */", index);
	fprintf(file, "\t{ NULL, XS_NO_ID, XS_NO_FLAG, XS_REFERENCE_KIND, ");
	if (dummy) fprintf(file, "0, ");
	fprintf(file, "{ .reference = NULL } }");
}

void fxPrintNumber(txMachine* the, FILE* file, txNumber value) 
{
	switch (c_fpclassify(value)) {
	case C_FP_INFINITE:
		if (value < 0)
			fprintf(file, "-C_INFINITY");
		else
			fprintf(file, "C_INFINITY");
		break;
	case C_FP_NAN:
		fprintf(file, "C_NAN");
		break;
	default:
		fprintf(file, "%e", value);
		break;
	}
}

void fxPrintSlot(txMachine* the, FILE* file, txSlot* slot, txFlag flag, txBoolean dummy) 
{
	fprintf(file, "\t{ ");
	if (slot->flag & XS_DEBUG_FLAG) {
		slot->flag &= ~XS_DEBUG_FLAG;
		fprintf(file, "(txSlot*)%ld", (long int)slot->next);
	}
	else
		fxPrintAddress(the, file, slot->next);
	fprintf(file, ", ");
	if (slot->ID == XS_NO_ID)
		fprintf(file, "XS_NO_ID, ");
	else
		fprintf(file, "%d, ", slot->ID);
	fprintf(file, "0x%x, ", slot->flag | flag);
	switch (slot->kind) {
	case XS_UNINITIALIZED_KIND: {
		fprintf(file, "XS_UNINITIALIZED_KIND, ");
		if (dummy) fprintf(file, "0, ");
		fprintf(file, "{ .number = 0 } ");
	} break;
	case XS_UNDEFINED_KIND: {
		fprintf(file, "XS_UNDEFINED_KIND, ");
		if (dummy) fprintf(file, "0, ");
		fprintf(file, "{ .number = 0 } ");
	} break;
	case XS_NULL_KIND: {
		fprintf(file, "XS_NULL_KIND, ");
		if (dummy) fprintf(file, "0, ");
		fprintf(file, "{ .number = 0 } ");
	} break;
	case XS_BOOLEAN_KIND: {
		fprintf(file, "XS_BOOLEAN_KIND, ");
		if (dummy) fprintf(file, "0, ");
		fprintf(file, "{ .boolean = %d } ", slot->value.boolean);
	} break;
	case XS_INTEGER_KIND: {
		fprintf(file, "XS_INTEGER_KIND, ");
		if (dummy) fprintf(file, "0, ");
		fprintf(file, "{ .integer = %d } ", slot->value.integer);
	} break;
	case XS_NUMBER_KIND: {
		fprintf(file, "XS_NUMBER_KIND, ");
		if (dummy) fprintf(file, "0, ");
		fprintf(file, "{ .number = ");
		fxPrintNumber(the, file, slot->value.number);
		fprintf(file, " } ");
	} break;
	case XS_STRING_KIND:
	case XS_STRING_X_KIND: {
		fprintf(file, "XS_STRING_X_KIND, ");
		if (dummy) fprintf(file, "0, ");
		fprintf(file, "{ .string = ");
		fxWriteCString(file, slot->value.string);
		fprintf(file, " } ");
	} break;
	case XS_SYMBOL_KIND: {
		fprintf(file, "XS_SYMBOL_KIND, ");
		if (dummy) fprintf(file, "0, ");
		fprintf(file, "{ .symbol = %d } ", slot->value.symbol);
	} break;
	case XS_REFERENCE_KIND: {
		fprintf(file, "XS_REFERENCE_KIND, ");
		if (dummy) fprintf(file, "0, ");
		fprintf(file, "{ .reference = ");
		fxPrintAddress(the, file, slot->value.reference);
		fprintf(file, " } ");
	} break;
	case XS_CLOSURE_KIND: {
		fprintf(file, "XS_CLOSURE_KIND, ");
		if (dummy) fprintf(file, "0, ");
		fprintf(file, "{ .closure = ");
		fxPrintAddress(the, file, slot->value.closure);
		fprintf(file, " } ");
	} break; 
	case XS_INSTANCE_KIND: {
		fprintf(file, "XS_INSTANCE_KIND, ");
		if (dummy) fprintf(file, "0, ");
		fprintf(file, "{ .instance = { NULL, ");
		fxPrintAddress(the, file, slot->value.instance.prototype);
		fprintf(file, " } } ");
	} break;
	case XS_ARRAY_KIND: {
		fprintf(file, "XS_ARRAY_KIND, ");
		if (dummy) fprintf(file, "0, ");
		fprintf(file, "{ .array = { NULL, 0 } }");
	} break;
	case XS_ARRAY_BUFFER_KIND: {
		fprintf(file, "XS_ARRAY_BUFFER_KIND, ");
		if (dummy) fprintf(file, "0, ");
		fprintf(file, "{ .arrayBuffer = { NULL, 0 } }");
	} break;
	case XS_CALLBACK_KIND: {
		fprintf(file, "XS_CALLBACK_X_KIND, ");
		if (dummy) fprintf(file, "0, ");
		fprintf(file, "{ .callback = { %s, NULL } }", fxGetCallbackName(the, slot->value.callback.address));
	} break;
	case XS_CODE_KIND:  {
		fprintf(file, "XS_CODE_X_KIND, ");
		if (dummy) fprintf(file, "0, ");
		fprintf(file, "{ .code = { %s, ", fxGetCodeName(the, slot->value.code.address));
		fxPrintAddress(the, file, slot->value.code.closures);
		fprintf(file, " } } ");
	} break;
	case XS_CODE_X_KIND: {
		fprintf(file, "XS_CODE_X_KIND, ");
		if (dummy) fprintf(file, "0, ");
		fprintf(file, "{ .code = { %s, ", fxGetCodeName(the, slot->value.code.address));
		fxPrintAddress(the, file, slot->value.code.closures);
		fprintf(file, " } } ");
	} break;
	case XS_DATE_KIND: {
		fprintf(file, "XS_DATE_KIND, ");
		if (dummy) fprintf(file, "0, ");
		fprintf(file, "{ .number = ");
		fxPrintNumber(the, file, slot->value.number);
		fprintf(file, " } ");
	} break;
	case XS_DATA_VIEW_KIND: {
		fprintf(file, "XS_DATA_VIEW_KIND, ");
		if (dummy) fprintf(file, "0, ");
		fprintf(file, "{ .dataView = { %d, %d } }", slot->value.dataView.offset, slot->value.dataView.size);
	} break;
	case XS_GLOBAL_KIND: {
		fprintf(file, "XS_GLOBAL_KIND, ");
		if (dummy) fprintf(file, "0, ");
		fprintf(file, "{ .table = { (txSlot**)(gxGlobals), %d } }", slot->value.table.length);
	} break;
	case XS_HOST_KIND: {
		fprintf(file, "XS_HOST_KIND, ");
		if (dummy) fprintf(file, "0, ");
		fprintf(file, "{ .host = { NULL, { .destructor = %s } } }", fxGetCallbackName(the, (txCallback)slot->value.host.variant.destructor ));
	} break;
	case XS_MAP_KIND: {
		fprintf(file, "XS_MAP_KIND, ");
		if (dummy) fprintf(file, "0, ");
		fprintf(file, "{ .table = { NULL, %d } }", slot->value.table.length);
	} break;
	case XS_MODULE_KIND: {
		fprintf(file, "XS_MODULE_KIND, ");
		if (dummy) fprintf(file, "0, ");
		fprintf(file, "{ .symbol = %d } ", slot->value.symbol);
	} break;
	case XS_PROMISE_KIND: {
		fprintf(file, "XS_PROMISE_KIND, ");
		if (dummy) fprintf(file, "0, ");
	} break;
	case XS_PROXY_KIND: {
		fprintf(file, "XS_PROXY_KIND, ");
		if (dummy) fprintf(file, "0, ");
		fprintf(file, "{ .instance = { ");
		fxPrintAddress(the, file, slot->value.proxy.handler);
		fprintf(file, ", ");
		fxPrintAddress(the, file, slot->value.proxy.target);
		fprintf(file, " } } ");
	} break;
	case XS_REGEXP_KIND: {
		fprintf(file, "XS_REGEXP_KIND, ");
		if (dummy) fprintf(file, "0, ");
	} break;
	case XS_SET_KIND: {
		fprintf(file, "XS_SET_KIND, ");
		if (dummy) fprintf(file, "0, ");
		fprintf(file, "{ .table = { NULL, %d } }", slot->value.table.length);
	} break;
	case XS_TYPED_ARRAY_KIND: {
		fprintf(file, "XS_TYPED_ARRAY_KIND, ");
		if (dummy) fprintf(file, "0, ");
		fprintf(file, "{ .typedArray = { (txTypeDispatch*)(&gxTypeDispatches[%d]), (txTypeAtomics*)(&gxTypeAtomics[%d]) } }", fxGetTypeDispatchIndex(slot->value.typedArray.dispatch), fxGetTypeAtomicsIndex(slot->value.typedArray.atomics));
	} break;
	case XS_WEAK_MAP_KIND: {
		fprintf(file, "XS_WEAK_MAP_KIND, ");
		if (dummy) fprintf(file, "0, ");
		fprintf(file, "{ .table = { NULL, %d } }", slot->value.table.length);
	} break;
	case XS_WEAK_SET_KIND: {
		fprintf(file, "XS_WEAK_SET_KIND, ");
		if (dummy) fprintf(file, "0, ");
		fprintf(file, "{ .table = { NULL, %d } }", slot->value.table.length);
	} break;
	case XS_WITH_KIND: {
		fprintf(file, "XS_WITH_KIND, ");
		if (dummy) fprintf(file, "0, ");
		fprintf(file, "{ .reference = ");
		fxPrintAddress(the, file, slot->value.reference);
		fprintf(file, " } ");
	} break;
	case XS_ACCESSOR_KIND: {
		fprintf(file, "XS_ACCESSOR_KIND, ");
		if (dummy) fprintf(file, "0, ");
		fprintf(file, "{ .accessor = { ");
		fxPrintAddress(the, file, slot->value.accessor.getter);
		fprintf(file, ", ");
		fxPrintAddress(the, file, slot->value.accessor.setter);
		fprintf(file, " } }");
	} break;
	case XS_AT_KIND: {
		fprintf(file, "XS_AT_KIND, ");
		if (dummy) fprintf(file, "0, ");
		fprintf(file, "{ .at = { 0x%x, %d } }", slot->value.at.index, slot->value.at.id);
	} break;
	case XS_ENTRY_KIND: {
		fprintf(file, "XS_ENTRY_KIND, ");
		if (dummy) fprintf(file, "0, ");
		fprintf(file, "{ .entry = { ");
		fxPrintAddress(the, file, slot->value.entry.slot);
		fprintf(file, ", 0x%x } }", slot->value.entry.sum);
	} break;
	case XS_ERROR_KIND: {
		fprintf(file, "XS_ERROR_KIND, ");
		if (dummy) fprintf(file, "0, ");
		fprintf(file, "{ .number = 0 } ");
	} break;
	case XS_HOME_KIND: {
		fprintf(file, "XS_HOME_KIND, ");
		if (dummy) fprintf(file, "0, ");
		fprintf(file, "{ .home = { ");
		fxPrintAddress(the, file, slot->value.home.object);
		fprintf(file, ", ");
		fxPrintAddress(the, file, slot->value.home.module);
		fprintf(file, " } }");
	} break;
	case XS_EXPORT_KIND: {
		fprintf(file, "XS_EXPORT_KIND, ");
		if (dummy) fprintf(file, "0, ");
		fprintf(file, "{ .export = { ");
		fxPrintAddress(the, file, slot->value.export.closure);
		fprintf(file, ", ");
		fxPrintAddress(the, file, slot->value.export.module);
		fprintf(file, " } }");
	} break;
	case XS_KEY_KIND:
	case XS_KEY_X_KIND: {
		fprintf(file, "XS_KEY_X_KIND, ");
		if (dummy) fprintf(file, "0, ");
		fprintf(file, "{ .key = { ");
		fxWriteCString(file, slot->value.key.string);
		fprintf(file, ", 0x%x } }", slot->value.key.sum);
	} break;
	case XS_LIST_KIND: {
		fprintf(file, "XS_LIST_KIND, ");
		if (dummy) fprintf(file, "0, ");
		fprintf(file, "{ .list = { ");
		fxPrintAddress(the, file, slot->value.list.first);
		fprintf(file, ", ");
		fxPrintAddress(the, file, slot->value.list.last);
		fprintf(file, " } }");
	} break;
	case XS_STACK_KIND: {
		fprintf(file, "XS_STACK_KIND, ");
		if (dummy) fprintf(file, "0, ");
	} break;
#ifdef mxHostFunctionPrimitive
	case XS_HOST_FUNCTION_KIND: {
		fprintf(file, "XS_HOST_FUNCTION_KIND, ");
		if (dummy) fprintf(file, "0, ");
		fprintf(file, "{ .hostFunction = { %s, NULL } }", fxGetBuilderName(the, slot->value.hostFunction.builder));
	} break;
#endif
	default:
		break;
	}
	fprintf(file, "},\n");
}

void fxPrintStack(txMachine* the, FILE* file, txBoolean dummy) 
{
	txSlot *slot = the->stack;
	txSlot *limit = the->stackTop - 1;
	while (slot < limit) {
		fxPrintSlot(the, file, slot, XS_NO_FLAG, dummy);
		slot++;
	}
	fxPrintSlot(the, file, slot, XS_NO_FLAG, dummy);
}


void fxPrintTable(txMachine* the, FILE* file, txSize modulo, txSlot** table) 
{
	while (modulo > 1) {
		fprintf(file, "\t");
		fxPrintAddress(the, file, *table);
		fprintf(file, ",\n");
		modulo--;
		table++;
	}
	fprintf(file, "\t");
	fxPrintAddress(the, file, *table);
	fprintf(file, "\n");
}

