/*
 * Copyright (c) 2016-2018  Moddable Tech, Inc.
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

typedef struct sxAliasIDLink txAliasIDLink;
typedef struct sxAliasIDList txAliasIDList;

struct sxAliasIDLink {
	txAliasIDLink* previous;
	txAliasIDLink* next;
	txInteger id;
	txInteger flag;
};

struct sxAliasIDList {
	txAliasIDLink* first;
	txAliasIDLink* last;
	txFlag* aliases;
	txInteger errorCount;
};

static void fxCheckAliasesError(txMachine* the, txAliasIDList* list, txFlag flag);
static void fxCheckEnvironmentAliases(txMachine* the, txSlot* environment, txAliasIDList* list);
static void fxCheckInstanceAliases(txMachine* the, txSlot* instance, txAliasIDList* list);
static void fxCheckPropertyAliases(txMachine* the, txSlot* property, txAliasIDList* list);
static txString fxGetBuilderName(txMachine* the, const txHostFunctionBuilder* which);
static txString fxGetCallbackName(txMachine* the, txCallback callback); 
static txString fxGetCodeName(txMachine* the, txByte* which);
static txInteger fxGetTypeDispatchIndex(txTypeDispatch* dispatch);
static void fxPrepareInstance(txMachine* the, txSlot* instance);
static void fxPrintAddress(txMachine* the, FILE* file, txSlot* slot);
static void fxPrintID(txMachine* the, FILE* file, txID id);
static void fxPrintNumber(txMachine* the, FILE* file, txNumber value);
static void fxPrintSlot(txMachine* the, FILE* file, txSlot* slot, txFlag flag);

enum {
	XSL_MODULE_FLAG,
	XSL_EXPORT_FLAG,
	XSL_ENVIRONMENT_FLAG,
	XSL_PROPERTY_FLAG,
	XSL_ITEM_FLAG,
	XSL_GETTER_FLAG,
	XSL_SETTER_FLAG,
	XSL_PROXY_HANDLER_FLAG,
	XSL_PROXY_TARGET_FLAG,
	XSL_GLOBAL_FLAG,
};

#define mxPushLink(name,ID,FLAG) \
	txAliasIDLink name = { C_NULL, C_NULL, ID, FLAG }; \
	name.previous = list->last; \
	if (list->last) \
		list->last->next = &name; \
	else \
		list->first = &name; \
	list->last = &name

#define mxPopLink(name) \
	if (name.previous) \
		name.previous->next = C_NULL; \
	else \
		list->first = C_NULL; \
	list->last = name.previous
	
#define mxRegExpSize(SLOT) \
	(((txChunk*)(((txByte*)SLOT->value.regexp.code) - sizeof(txChunk)))->size / sizeof(txInteger))

txSlot* fxBuildHostConstructor(txMachine* the, txCallback call, txInteger length, txInteger id)
{
	txLinker* linker = (txLinker*)(the->context);
	fxNewLinkerBuilder(linker, call, length, id);
	return fxNewHostConstructor(the, call, length, id);
}

txSlot* fxBuildHostFunction(txMachine* the, txCallback call, txInteger length, txInteger id)
{
	txLinker* linker = (txLinker*)(the->context);
	fxNewLinkerBuilder(linker, call, length, id);
	return fxNewHostFunction(the, call, length, id, XS_NO_ID);
}

txInteger fxCheckAliases(txMachine* the) 
{
	txLinker* linker = xsGetContext(the);
	txAliasIDList _list = { C_NULL, C_NULL }, *list = &_list;
	txSlot* module = mxProgram.value.reference->next; //@@
	list->aliases = fxNewLinkerChunkClear(linker, the->aliasCount * sizeof(txFlag));
	while (module) {
		txSlot* export = mxModuleExports(module)->value.reference->next;
		if (export) {
			mxPushLink(moduleLink, module->ID, XSL_MODULE_FLAG);
			while (export) {
				txSlot* closure = export->value.export.closure;
				if (closure) {
					mxPushLink(exportLink, export->ID, XSL_EXPORT_FLAG);
					if (closure->ID != XS_NO_ID) {
						if (list->aliases[closure->ID] == 0) {
							list->aliases[closure->ID] = 1;
							fxCheckAliasesError(the, list, 0);
						}
					}
					if (closure->kind == XS_REFERENCE_KIND) {
						fxCheckInstanceAliases(the, closure->value.reference, list);
					}
					mxPopLink(exportLink);
				}
				export = export->next;
			}
			mxPopLink(moduleLink);
		}
		module = module->next;
	}
	{
		txSlot* global = mxGlobal.value.reference->next->next;
		while (global) {
			if ((global->ID != mxID(_global)) && (global->ID != mxID(_globalThis))) {
				mxPushLink(globalLink, global->ID, XSL_GLOBAL_FLAG);
				if (global->kind == XS_REFERENCE_KIND) {
					fxCheckInstanceAliases(the, global->value.reference, list);
				}
				mxPopLink(globalLink);
			}
			global = global->next;
		}
	}
	{
		txSlot *heap, *slot, *limit;
		heap = the->firstHeap;
		while (heap) {
			slot = heap + 1;
			limit = heap->value.reference;
			while (slot < limit) {
				if (slot->kind == XS_INSTANCE_KIND)
					slot->flag &= ~XS_LEVEL_FLAG;
				slot++;
			}
			heap = heap->next;
		}
	}
	return list->errorCount;
}

void fxCheckAliasesError(txMachine* the, txAliasIDList* list, txFlag flag) 
{
	txAliasIDLink* link = list->first;
	if (flag > 1)
		fprintf(stderr, "### error");
	else
		fprintf(stderr, "### warning");
	while (link) {
		switch (link->flag) {
		case XSL_PROPERTY_FLAG: fprintf(stderr, "."); break;
		case XSL_ITEM_FLAG: fprintf(stderr, "["); break;
		case XSL_GETTER_FLAG: fprintf(stderr, ".get "); break;
		case XSL_SETTER_FLAG: fprintf(stderr, ".set "); break;
		case XSL_ENVIRONMENT_FLAG: fprintf(stderr, "() "); break;
		case XSL_PROXY_HANDLER_FLAG: fprintf(stderr, ".(handler)"); break;
		case XSL_PROXY_TARGET_FLAG: fprintf(stderr, ".(target)"); break;
		default: fprintf(stderr, ": "); break;
		}
		if (link->id != XS_NO_ID) {
			char* string = fxGetKeyName(the, link->id);
			if (string) {
				if (link->flag == XSL_GLOBAL_FLAG) {
					fprintf(stderr, "globalThis."); 
					fprintf(stderr, "%s", string);
				}
				else
					fprintf(stderr, "%s", string);
			}
			else
				fprintf(stderr, "%d", link->id);
		}
		else 
			fprintf(stderr, "%d", link->id);
		if (link->flag == XSL_ITEM_FLAG)
			fprintf(stderr, "]");
		link = link->next;
	}
	if (flag == 3) {
		fprintf(stderr, ": generator\n");
		list->errorCount++;
	}
	else if (flag == 2) {
		fprintf(stderr, ": regexp\n");
		list->errorCount++;
	}
	else if (flag)
		fprintf(stderr, ": not frozen\n");
	else
		fprintf(stderr, ": no const\n");
}

void fxCheckEnvironmentAliases(txMachine* the, txSlot* environment, txAliasIDList* list) 
{
	txSlot* closure = environment->next;
	if (environment->flag & XS_LEVEL_FLAG)
		return;
	environment->flag |= XS_LEVEL_FLAG;
	if (environment->value.instance.prototype)
		fxCheckEnvironmentAliases(the, environment->value.instance.prototype, list);
	while (closure) {
		if (closure->kind == XS_CLOSURE_KIND) {
			txSlot* slot = closure->value.closure;
			mxPushLink(closureLink, closure->ID, XSL_ENVIRONMENT_FLAG);
			if (slot->ID != XS_NO_ID) {
				if (list->aliases[slot->ID] == 0) {
					list->aliases[slot->ID] = 1;
					fxCheckAliasesError(the, list, 0);
				}
			}
			if (slot->kind == XS_REFERENCE_KIND) {
				fxCheckInstanceAliases(the, slot->value.reference, list);
			}
			mxPopLink(closureLink);
		}
		closure = closure->next;
	}
	//environment->flag &= ~XS_LEVEL_FLAG;
}

void fxCheckInstanceAliases(txMachine* the, txSlot* instance, txAliasIDList* list) 
{
	txSlot* property = instance->next;
	if (instance->flag & XS_LEVEL_FLAG)
		return;
	instance->flag |= XS_LEVEL_FLAG;
	if (instance->value.instance.prototype) {
		mxPushLink(propertyLink, mxID(___proto__), XSL_PROPERTY_FLAG);
		fxCheckInstanceAliases(the, instance->value.instance.prototype, list);
		mxPopLink(propertyLink);
	}
	if (instance->ID != XS_NO_ID) {
		if (list->aliases[instance->ID] == 0) {
			list->aliases[instance->ID] = 1;
			fxCheckAliasesError(the, list, 1);
		}
	}
	while (property && (property->flag & XS_INTERNAL_FLAG)) {
		if (property->kind == XS_ARRAY_KIND) {
			txSlot* item = property->value.array.address;
			txInteger length = (txInteger)fxGetIndexSize(the, property);
			while (length > 0) {
				mxPushLink(propertyLink, *((txInteger*)item), XSL_ITEM_FLAG);
				fxCheckPropertyAliases(the, item, list);
				mxPopLink(propertyLink);
				item++;
				length--;
			}
		}
		else if ((property->kind == XS_CODE_KIND) || (property->kind == XS_CODE_X_KIND)) {
			if (property->value.code.closures)
				fxCheckEnvironmentAliases(the, property->value.code.closures, list);
		}
		else if (property->kind == XS_PRIVATE_KIND) {
			txSlot* item = property->value.private.first;
			while (item) {
				mxPushLink(propertyLink, item->ID, XSL_PROPERTY_FLAG);
				fxCheckPropertyAliases(the, item, list);
				mxPopLink(propertyLink);
				item = item->next;
			}
		}
		else if (property->kind == XS_PROXY_KIND) {
			if (property->value.proxy.handler) {
				mxPushLink(propertyLink, XS_NO_ID, XSL_PROXY_HANDLER_FLAG);
				fxCheckInstanceAliases(the, property->value.proxy.handler, list);
				mxPopLink(propertyLink);
			}
			if (property->value.proxy.target) {
				mxPushLink(propertyLink, XS_NO_ID, XSL_PROXY_TARGET_FLAG);
				fxCheckInstanceAliases(the, property->value.proxy.target, list);
				mxPopLink(propertyLink);
			}
		}
		else if (property->kind == XS_STACK_KIND) {
			fxCheckAliasesError(the, list, 3);
		}
		property = property->next;
	}
	while (property) {
		mxPushLink(propertyLink, property->ID, XSL_PROPERTY_FLAG);
		fxCheckPropertyAliases(the, property, list);
		mxPopLink(propertyLink);
		property = property->next;
	}
}

void fxCheckPropertyAliases(txMachine* the, txSlot* property, txAliasIDList* list)
{
	if (property->kind == XS_REFERENCE_KIND) {
		fxCheckInstanceAliases(the, property->value.reference, list);
	}
	else if (property->kind == XS_ACCESSOR_KIND) {
		if (property->value.accessor.getter) {
			list->last->flag = XSL_GETTER_FLAG;
			fxCheckInstanceAliases(the, property->value.accessor.getter, list);
		}
		if (property->value.accessor.setter) {
			list->last->flag = XSL_SETTER_FLAG;
			fxCheckInstanceAliases(the, property->value.accessor.setter, list);
		}
	}
}

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
	while (i < mxTypeArrayCount) {
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
	while (i < mxTypeArrayCount) {
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
    txSlot* key = fxGetKey(the, slot->value.module.id);
 	txString path = key->value.key.string;
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
					txCallback callback = the->fakeCallback;
					txHostFunctionBuilder* builder = &linkerScript->builders[i];
					txCallbackName* callbackName = &linkerScript->callbackNames[i];
					txS1 length = *p++;
					the->fakeCallback = (txCallback)(((txU1*)the->fakeCallback) + 1);
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
						the->stack->value.hostFunction.profileID = fxGenerateProfileID(the);
					}
					fxArrayCacheItem(the, the->stack + 1, the->stack);
					mxPop();
					p += mxStringLength((char*)p) + 1;
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

txSlot* fxNewFunctionLength(txMachine* the, txSlot* instance, txNumber length)
{
	txLinker* linker = (txLinker*)(the->context);
	txSlot* property;
	if (linker->stripFlag)
		return C_NULL;
	property = mxBehaviorGetProperty(the, instance, mxID(_length), 0, XS_OWN);
	if (!property)
		property = fxNextIntegerProperty(the, fxLastProperty(the, instance), 0, mxID(_length), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	if (length <= 0x7FFFFFFF) {
		property->kind = XS_INTEGER_KIND;
		property->value.integer = (txInteger)length;
	}
	else {
		property->kind = XS_NUMBER_KIND;
		property->value.number = length;
	}
	return property;
}

txSlot* fxNewFunctionName(txMachine* the, txSlot* instance, txID id, txIndex index, txID former, txString prefix)
{
	txLinker* linker = (txLinker*)(the->context);
	txSlot* property;
	txSlot* key;
	if (linker->stripFlag)
		return C_NULL;
	property = mxBehaviorGetProperty(the, instance, mxID(_name), 0, XS_OWN);
	if (property) {
		if ((property->kind != mxEmptyString.kind) || (property->value.string != mxEmptyString.value.string))
			return property;
	}
	else
		property = fxNextSlotProperty(the, fxLastProperty(the, instance), &mxEmptyString, mxID(_name), XS_DONT_ENUM_FLAG | XS_DONT_SET_FLAG);
	if (id != XS_NO_ID) {
		key = fxGetKey(the, (txID)id);
		if (key) {
			txKind kind = mxGetKeySlotKind(key);
			if (kind == XS_KEY_KIND) {
				property->kind = XS_STRING_KIND;
				property->value.string = key->value.key.string;
			}
			else if (kind == XS_KEY_X_KIND) {
				property->kind = XS_STRING_X_KIND;
				property->value.string = key->value.key.string;
			}
			else if ((kind == XS_STRING_KIND) || (kind == XS_STRING_X_KIND)) {
				property->kind = kind;
				property->value.string = key->value.string;
				fxAdornStringC(the, "[", property, "]");
			}
			else {
				property->kind = mxEmptyString.kind;
				property->value = mxEmptyString.value;
			}
		}
		else {
			property->kind = mxEmptyString.kind;
			property->value = mxEmptyString.value;
		}
	}
	else if (former) {
		char buffer[16];
		fxCopyStringC(the, property, fxNumberToString(the->dtoa, index, buffer, sizeof(buffer), 0, 0));	
	}
	if (prefix) 
		fxAdornStringC(the, prefix, property, C_NULL);
	return property;
}

txSlot* fxNextHostAccessorProperty(txMachine* the, txSlot* property, txCallback get, txCallback set, txID id, txFlag flag)
{
	txLinker* linker = (txLinker*)(the->context);
	txSlot *getter = NULL, *setter = NULL, *home = the->stack, *slot;
	if (get) {
		fxNewLinkerBuilder(linker, get, 0, id);
		getter = fxNewHostFunction(the, get, 0, XS_NO_ID, XS_NO_ID);
		slot = mxFunctionInstanceHome(getter);
		slot->value.home.object = home->value.reference;
		fxRenameFunction(the, getter, id, 0, XS_NO_ID, "get ");
	}
	if (set) {
		fxNewLinkerBuilder(linker, set, 1, id);
		setter = fxNewHostFunction(the, set, 1, XS_NO_ID, XS_NO_ID);
		slot = mxFunctionInstanceHome(setter);
		slot->value.home.object = home->value.reference;
		fxRenameFunction(the, setter, id, 0, XS_NO_ID, "set ");
	}
	property = property->next = fxNewSlot(the);
	property->flag = flag;
	property->ID = id;
	property->kind = XS_ACCESSOR_KIND;
	property->value.accessor.getter = getter;
	property->value.accessor.setter = setter;
	if (set)
		mxPop();
	if (get)
		mxPop();
	return property;
}

txSlot* fxNextHostFunctionProperty(txMachine* the, txSlot* property, txCallback call, txInteger length, txID id, txFlag flag)
{
	property = property->next = fxNewSlot(the);
	property->flag = flag;
	property->ID = id;
	fxSetHostFunctionProperty(the, property, call, length, id);
	return property;
}

void fxPetrifyInstance(txMachine* the, txSlot* instance)
{
	txSlot* property = instance->next;
	txSlot* private;
	while (property && (property->flag & XS_INTERNAL_FLAG)) {
		switch (property->kind) {
		case XS_ARRAY_BUFFER_KIND:
		case XS_DATE_KIND:
		case XS_MAP_KIND:
		case XS_REGEXP_KIND:
		case XS_SET_KIND:
		case XS_WEAK_MAP_KIND:
		case XS_WEAK_SET_KIND:
			property->flag |= XS_DONT_SET_FLAG;
			break;				
		case XS_PRIVATE_KIND:
			private = property->value.private.first;
			while (private) {
				if (private->kind != XS_ACCESSOR_KIND) 
					private->flag |= XS_DONT_SET_FLAG;
				private->flag |= XS_DONT_DELETE_FLAG;
				private = private->next;
			}
			break;
		}
		property = property->next;
	}
}

void fxPrepareInstance(txMachine* the, txSlot* instance)
{
	txLinker* linker = (txLinker*)(the->context);
	if (linker->freezeFlag) {
		txSlot *property = instance->next;
		while (property) {
			if (property->kind != XS_ACCESSOR_KIND) 
				property->flag |= XS_DONT_SET_FLAG;
			property->flag |= XS_DONT_DELETE_FLAG;
			property = property->next;
		}
		instance->flag |= XS_DONT_PATCH_FLAG;
	}
}

txInteger fxPrepareHeap(txMachine* the)
{
	txLinker* linker = (txLinker*)(the->context);
	txID aliasCount = 1;
	txInteger index = linker->projectionIndex;
	txSlot *heap, *slot, *limit, *item;
	txLinkerProjection* projection;

	slot = the->freeHeap;
	while (slot) {
		slot->flag |= XS_MARK_FLAG;
		slot = slot->next;
	}
	
	heap = the->firstHeap;
	projection = linker->firstProjection;
	while (heap) {
		slot = heap + 1;
		limit = heap->value.reference;
		while (slot < limit) {
			if (!(slot->flag & XS_MARK_FLAG)) {
				if (projection->indexes[slot - heap] == 0) {
					projection->indexes[slot - heap] = index;
					index++;
				}
				if ((slot->kind == XS_ARRAY_KIND) && ((item = slot->value.array.address))) {
					index++; // fake chunk
					index += (txInteger)fxGetIndexSize(the, slot);
				}
				else if (slot->kind == XS_BIGINT_KIND) {
					linker->bigintSize += slot->value.bigint.size;
				}
				else if (slot->kind == XS_REGEXP_KIND) {
					linker->regexpSize += mxRegExpSize(slot);
				}
				else if (slot->kind == XS_INSTANCE_KIND) {
					txSlot *property = slot->next;
					if (property && (property->flag & XS_INTERNAL_FLAG)) {
						// @@ freeze environments
						if (property->ID == XS_ENVIRONMENT_BEHAVIOR)
							fxPrepareInstance(the, slot);
						// @@ freeze functions and prototypes
						else if ((property->kind == XS_CALLBACK_KIND) || (property->kind == XS_CALLBACK_X_KIND) || (property->kind == XS_CODE_KIND) || (property->kind == XS_CODE_X_KIND)) {
							fxPrepareInstance(the, slot);
							if (linker->freezeFlag) {
								if (slot->flag & XS_CAN_CONSTRUCT_FLAG) {
									property = property->next;
									while (property) {
										if ((property->ID == mxID(_prototype)) && (property->kind == XS_REFERENCE_KIND)) {
											fxPrepareInstance(the, property->value.reference);
											break;
										}
										property = property->next;
									}
								}
							}
						}
						// @@ freeze instances that cannot be aliased yet
						
						else if ((property->kind == XS_MAP_KIND) || (property->kind == XS_SET_KIND)) {
							fxPrepareInstance(the, slot);
							linker->slotSize += property->value.table.length;
						}
						else if ((property->kind == XS_WEAK_MAP_KIND) || (property->kind == XS_WEAK_SET_KIND))
							fxPrepareInstance(the, slot);
						else if (property->kind == XS_WEAK_REF_KIND)
							fxPrepareInstance(the, slot);
						else if ((property->kind == XS_CLOSURE_KIND) && (property->value.closure->kind == XS_FINALIZATION_REGISTRY_KIND))
							fxPrepareInstance(the, slot);
							
						else if (property->kind == XS_ARRAY_BUFFER_KIND)
							fxPrepareInstance(the, slot);
						else if (property->kind == XS_DATA_VIEW_KIND)
							fxPrepareInstance(the, slot);
						else if (property->kind == XS_TYPED_ARRAY_KIND)
							fxPrepareInstance(the, slot);
							
						else if (property->kind == XS_PROMISE_KIND) {
							fxPrepareInstance(the, slot);
							property = property->next;
							fxPrepareInstance(the, property->value.reference); // thens
						}
						else if (property->kind == XS_PROXY_KIND)
							fxPrepareInstance(the, slot);
						else if (property->kind == XS_REGEXP_KIND)
							fxPrepareInstance(the, slot);
					}
				}
			}
			slot++;
		}
		heap = heap->next;
		projection = projection->nextProjection;
	}
	index++;
	
	heap = the->firstHeap;
	while (heap) {
		slot = heap + 1;
		limit = heap->value.reference;
		while (slot < limit) {
			if (!(slot->flag & XS_MARK_FLAG)) {
				if (slot->kind == XS_INSTANCE_KIND) {
					txBoolean frozen = (slot->flag & XS_DONT_PATCH_FLAG) ? 1 : 0;
					if (frozen) {
						txSlot *property = slot->next;
						while (property && (property->flag & XS_INTERNAL_FLAG)) {
							if (property->kind == XS_ARRAY_KIND) {
								txSlot* item = property->value.array.address;
								txInteger length = (txInteger)fxGetIndexSize(the, property);
								while (length > 0) {
									if (item->kind != XS_ACCESSOR_KIND) 
										if (!(item->flag & XS_DONT_SET_FLAG))
											frozen = 0;
									if (!(item->flag & XS_DONT_DELETE_FLAG))
										frozen = 0;
									item++;
									length--;
								}
							}
							property = property->next;
						}
						while (property) {	
							if (property->kind != XS_ACCESSOR_KIND) 
								if (!(property->flag & XS_DONT_SET_FLAG))
									frozen = 0;
							if (!(property->flag & XS_DONT_DELETE_FLAG))
								frozen = 0;
							property = property->next;
						}
					}
					if (frozen) {
						slot->ID = XS_NO_ID;
						fxPetrifyInstance(the, slot);
					}
					else
						slot->ID = aliasCount++;
				}
			}
			slot++;
		}
		heap = heap->next;
	}
	
	heap = the->firstHeap;
	while (heap) {
		slot = heap + 1;
		limit = heap->value.reference;
		while (slot < limit) {
			if (!(slot->flag & XS_MARK_FLAG)) {
				if (slot->kind == XS_CLOSURE_KIND) {
					txSlot* closure = slot->value.closure;
					if (linker->freezeFlag) {
						// @@ const functions closures
						if (closure->kind == XS_REFERENCE_KIND) {
							txSlot* internal = closure->value.reference->next;
							if (internal && ((internal->kind == XS_CALLBACK_KIND) || (internal->kind == XS_CALLBACK_X_KIND) || (internal->kind == XS_CODE_KIND) || (internal->kind == XS_CODE_X_KIND))) {
								closure->flag |= XS_DONT_SET_FLAG;
							}
						}
						else if (closure->kind == XS_HOST_FUNCTION_KIND) {
							closure->flag |= XS_DONT_SET_FLAG;
						}
					}
					if (closure->flag & XS_DONT_SET_FLAG) {
						if (closure->ID != XS_NO_ID)
							closure->flag |= XS_DONT_DELETE_FLAG;
					}
					else {
						if (closure->ID == XS_NO_ID)
							closure->ID = aliasCount++;
						slot->flag &= ~XS_DONT_SET_FLAG; // ??
					}
				}
			}
			slot++;
		}
		heap = heap->next;
	}
	
	the->aliasCount = aliasCount;
	
	return index;
}

void fxPrepareHome(txMachine* the)
{
	txSlot *heap, *slot, *limit;
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
}

void fxPrepareProjection(txMachine* the)
{
	txLinker* linker = (txLinker*)(the->context);
	txLinkerProjection** projectionAddress = &linker->firstProjection;
	txLinkerProjection* projection;
	txSlot* heap = the->firstHeap;
	while (heap) {
		txSlot* slot = heap + 1;
		txSlot* limit = heap->value.reference;
		projection = fxNewLinkerChunkClear(linker, sizeof(txLinkerProjection) + (mxPtrDiff(limit - slot) * sizeof(txInteger)));
		projection->heap = heap;
		projection->limit = limit;
		*projectionAddress = projection;
		projectionAddress = &projection->nextProjection;
		heap = heap->next;
	}
	linker->projectionIndex = 1;
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
		fprintf(file, "\t{ %s, %d, ", name, linkerBuilder->host.length);
		fxPrintID(the, file, linkerBuilder->host.id);
		fprintf(file, " },\n");
		linkerBuilder = linkerBuilder->nextBuilder;
	}
	fprintf(file, "};\n\n");
}

void fxPrintHeap(txMachine* the, FILE* file, txInteger count)
{
	txLinker* linker = (txLinker*)(the->context);
	txLinkerProjection* projection = linker->firstProjection;
	txSlot *slot, *limit, *item;
	txInteger index = 0;
	fprintf(file, "/* %.4d */", index);
	fprintf(file, "\t{ NULL, {.ID = XS_NO_ID, .flag = XS_NO_FLAG, .kind = XS_REFERENCE_KIND}, ");
	fprintf(file, ".value = { .reference = (txSlot*)&gxHeap[%d] } },\n", (int)count);
	index++;
	while (index < linker->projectionIndex) {
		fprintf(file, "/* %.4d */", index);
		slot = linker->layout[index];
		if (slot)
			fxPrintSlot(the, file, slot, XS_MARK_FLAG);
		else
			fprintf(file, "\t{ NULL, {.ID = XS_NO_ID, .flag = XS_NO_FLAG, .kind = XS_UNINITIALIZED_KIND }, .value = { .number = 0 } },\n");
		index++;
	}
	while (projection) {
		slot = projection->heap + 1;
		limit = projection->limit;
		while (slot < limit) {
			if (!(slot->flag & XS_MARK_FLAG)) {
				if ((slot->kind == XS_ARRAY_KIND) && ((item = slot->value.array.address))) {
					fprintf(file, "/* %.4d */", index);
					index++;
					txInteger size = (txInteger)fxGetIndexSize(the, slot);
					fprintf(file, "\t{ ");
					fxPrintAddress(the, file, slot->next);
					fprintf(file, ", {.ID = %d", slot->ID);
					fprintf(file, ", .flag = 0x%x", slot->flag | XS_MARK_FLAG);
					fprintf(file, ", .kind = XS_ARRAY_KIND}, .value = { .array = { (txSlot*)&gxHeap[%d], %d } }},\n", (int)index + 1, (int)slot->value.array.length);
					fprintf(file, "/* %.4d */", index);
					index++;
					fprintf(file, "\t{ NULL, {.ID = XS_NO_ID, .flag = 0x80, .kind = XS_INTEGER_KIND}, .value = { .integer = %d * sizeof(txSlot) } },\n", (int)size); // fake chunk
					while (size) {
						fprintf(file, "/* %.4d */", index);
						index++;
						fxPrintSlot(the, file, item, XS_MARK_FLAG | XS_DEBUG_FLAG);
						item++;
						size--;
					}
				}
				else if (projection->indexes[slot - projection->heap] >= linker->projectionIndex) {
					fprintf(file, "/* %.4d */", index);
					index++;
					fxPrintSlot(the, file, slot, XS_MARK_FLAG);
				}
			}
			slot++;
		}
		projection = projection->nextProjection;
	}	
	fprintf(file, "/* %.4d */", index);
	fprintf(file, "\t{ NULL, {.ID = XS_NO_ID, .flag = XS_NO_FLAG, .kind = XS_REFERENCE_KIND}, ");
	fprintf(file, ".value = { .reference = NULL } }");
}

void fxPrintID(txMachine* the, FILE* file, txID id) 
{
	txLinker* linker = (txLinker*)(the->context);
	if (id == XS_NO_ID)
		fprintf(file, "XS_NO_ID");
	else {
		txLinkerSymbol* linkerSymbol = linker->symbolArray[id];
		if (linkerSymbol) {
			if (fxIsCIdentifier(linker, linkerSymbol->string))
				fprintf(file, "xsID_%s", linkerSymbol->string);
			else
				fprintf(file, "%d /* %s */", id, linkerSymbol->string);
		}
		else {
			char* string = fxGetKeyName(the, id);
			if (string)
				fprintf(file, "%d /* %s */", id, string);
			else
				fprintf(file, "%d /* ? */", id);
		}
	}
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
		fprintf(file, "%.20e", value);
		break;
	}
}

void fxPrintSlot(txMachine* the, FILE* file, txSlot* slot, txFlag flag)
{
	txLinker* linker = (txLinker*)(the->context);
	fprintf(file, "\t{ ");
	if (flag & XS_DEBUG_FLAG) {
		flag &= ~XS_DEBUG_FLAG;
		fprintf(file, "(txSlot*)%d", *((txInteger*)slot));
	}
	else
		fxPrintAddress(the, file, slot->next);
	fprintf(file, ", {.ID = ");
	if (slot->kind == XS_INSTANCE_KIND) {
		if (slot->ID)
			fprintf(file, "%d /* ALIAS */", slot->ID);
		else
			fprintf(file, "XS_NO_ID");
	}
	else if ((slot->kind == XS_CODE_KIND) || (slot->kind == XS_CODE_X_KIND) || (slot->kind == XS_CALLBACK_KIND) || (slot->kind == XS_CALLBACK_X_KIND))
		fxPrintID(the, file, slot->ID);
	else if (slot->flag & XS_INTERNAL_FLAG)
		fprintf(file, "%d", slot->ID);
	else
		fxPrintID(the, file, slot->ID);
	fprintf(file, ", ");
	if (slot->kind == 	XS_INSTANCE_KIND)
		fprintf(file, ".flag = 0x%x, ", slot->flag | flag | XS_DONT_MARSHALL_FLAG);
	else
		fprintf(file, ".flag = 0x%x, ", slot->flag | flag);
	switch (slot->kind) {
	case XS_UNINITIALIZED_KIND: {
		fprintf(file, ".kind = XS_UNINITIALIZED_KIND}, ");
		fprintf(file, ".value = { .number = 0 } ");
	} break;
	case XS_UNDEFINED_KIND: {
		fprintf(file, ".kind = XS_UNDEFINED_KIND}, ");
		fprintf(file, ".value = { .number = 0 } ");
	} break;
	case XS_NULL_KIND: {
		fprintf(file, ".kind = XS_NULL_KIND}, ");
		fprintf(file, ".value = { .number = 0 } ");
	} break;
	case XS_BOOLEAN_KIND: {
		fprintf(file, ".kind = XS_BOOLEAN_KIND}, ");
		fprintf(file, ".value = { .boolean = %d } ", slot->value.boolean);
	} break;
	case XS_INTEGER_KIND: {
		fprintf(file, ".kind = XS_INTEGER_KIND}, ");
		fprintf(file, ".value = { .integer = %d } ", slot->value.integer);
	} break;
	case XS_NUMBER_KIND: {
		fprintf(file, ".kind = XS_NUMBER_KIND}, ");
		fprintf(file, ".value = { .number = ");
		fxPrintNumber(the, file, slot->value.number);
		fprintf(file, " } ");
	} break;
	case XS_STRING_KIND:
	case XS_STRING_X_KIND: {
		fprintf(file, ".kind = XS_STRING_X_KIND}, ");
		fprintf(file, ".value = { .string = ");
		fxWriteCString(file, slot->value.string);
		fprintf(file, " } ");
	} break;
	case XS_SYMBOL_KIND: {
		fprintf(file, ".kind = XS_SYMBOL_KIND}, ");
		fprintf(file, ".value = { .symbol = %d } ", slot->value.symbol);
	} break;
	case XS_BIGINT_KIND:
	case XS_BIGINT_X_KIND: {
		fprintf(file, ".kind = XS_BIGINT_X_KIND}, ");
		fprintf(file, ".value = { .bigint = { ");
		fprintf(file, ".data = (txU4*)&gxBigIntData[%d], ", linker->bigintSize);
		fprintf(file, ".size = %d, ", slot->value.bigint.size);
		fprintf(file, ".sign = %d, ", slot->value.bigint.sign);
		fprintf(file, " } } ");
		c_memcpy(linker->bigintData + linker->bigintSize, slot->value.bigint.data, slot->value.bigint.size * sizeof(txU4));
		linker->bigintSize += slot->value.bigint.size;
	} break;
	case XS_REFERENCE_KIND: {
		fprintf(file, ".kind = XS_REFERENCE_KIND}, ");
		fprintf(file, ".value = { .reference = ");
		fxPrintAddress(the, file, slot->value.reference);
		fprintf(file, " } ");
	} break;
	case XS_CLOSURE_KIND: {
		fprintf(file, ".kind = XS_CLOSURE_KIND}, ");
		fprintf(file, ".value = { .closure = ");
		fxPrintAddress(the, file, slot->value.closure);
		fprintf(file, " } ");
	} break; 
	case XS_INSTANCE_KIND: {
		fprintf(file, ".kind = XS_INSTANCE_KIND}, ");
		fprintf(file, ".value = { .instance = { NULL, ");
		fxPrintAddress(the, file, slot->value.instance.prototype);
		fprintf(file, " } } ");
	} break;
	case XS_ARRAY_KIND: {
		fprintf(file, ".kind = XS_ARRAY_KIND}, ");
		fprintf(file, ".value = { .array = { NULL, 0 } }");
	} break;
	case XS_ARRAY_BUFFER_KIND: {
		fprintf(file, ".kind = XS_ARRAY_BUFFER_KIND}, ");
		fprintf(file, ".value = { .arrayBuffer = { (txByte*)");
		fxWriteCData(file, slot->value.arrayBuffer.address, slot->next->value.bufferInfo.length);
		fprintf(file, ", NULL } } ");
	} break;
	case XS_BUFFER_INFO_KIND: {
		fprintf(file, ".kind = XS_BUFFER_INFO_KIND}, ");
		fprintf(file, ".value = { .bufferInfo = {  %d, %d } } ", slot->value.bufferInfo.length, slot->value.bufferInfo.maxLength);
	} break;
	case XS_CALLBACK_KIND: {
		fprintf(file, ".kind = XS_CALLBACK_X_KIND}, ");
		fprintf(file, ".value = { .callback = { %s, NULL } }", fxGetCallbackName(the, slot->value.callback.address));
	} break;
	case XS_CODE_KIND:  {
		fprintf(file, ".kind = XS_CODE_X_KIND}, ");
		fprintf(file, ".value = { .code = { (txByte*)");
		{
			txChunk* chunk = (txChunk*)(slot->value.code.address - sizeof(txChunk));
			fxWriteCData(file, slot->value.code.address, chunk->size - sizeof(txChunk));
		}
		fprintf(file, ", ");
		fxPrintAddress(the, file, slot->value.code.closures);
		fprintf(file, " } } ");
	} break;
	case XS_CODE_X_KIND: {
		fprintf(file, ".kind = XS_CODE_X_KIND}, ");
		fprintf(file, ".value = { .code = { %s, ", fxGetCodeName(the, slot->value.code.address));
		fxPrintAddress(the, file, slot->value.code.closures);
		fprintf(file, " } } ");
	} break;
	case XS_DATE_KIND: {
		fprintf(file, ".kind = XS_DATE_KIND}, ");
		fprintf(file, ".value = { .number = ");
		fxPrintNumber(the, file, slot->value.number);
		fprintf(file, " } ");
	} break;
	case XS_DATA_VIEW_KIND: {
		fprintf(file, ".kind = XS_DATA_VIEW_KIND}, ");
		fprintf(file, ".value = { .dataView = { %d, %d } }", slot->value.dataView.offset, slot->value.dataView.size);
	} break;
	case XS_FINALIZATION_CELL_KIND: {
		fprintf(file, ".kind = XS_FINALIZATION_CELL_KIND}, ");
		fprintf(file, ".value = { .finalizationCell = { ");
		fxPrintAddress(the, file, slot->value.finalizationCell.target);
		fprintf(file, ", ");
		fxPrintAddress(the, file, slot->value.finalizationCell.token);
		fprintf(file, " } }");
	} break;
	case XS_FINALIZATION_REGISTRY_KIND: {
		fprintf(file, ".kind = XS_FINALIZATION_REGISTRY_KIND}, ");
		fprintf(file, ".value = { .finalizationRegistry = { ");
		fxPrintAddress(the, file, slot->value.finalizationRegistry.callback);
		fprintf(file, ", %d } }", slot->value.finalizationRegistry.flags);
	} break;
	case XS_GLOBAL_KIND: {
		fprintf(file, ".kind = XS_GLOBAL_KIND}, ");
		fprintf(file, ".value = { .table = { NULL, 0 } }");
	} break;
	case XS_HOST_KIND: {
		fprintf(file, ".kind = XS_HOST_KIND}, ");
		fprintf(file, ".value = { .host = { NULL, { .destructor = %s } } }", fxGetCallbackName(the, (txCallback)slot->value.host.variant.destructor ));
	} break;
	case XS_MAP_KIND: {
		fprintf(file, ".kind = XS_MAP_KIND}, ");
		fprintf(file, ".value = { .table = { (txSlot**)&gxSlotData[%d], %d } }", linker->slotSize, slot->value.table.length);
		c_memcpy(linker->slotData + linker->slotSize, slot->value.table.address, slot->value.table.length * sizeof(txSlot*));
		linker->slotSize += slot->value.table.length;
	} break;
	case XS_MODULE_KIND:  {
		fprintf(file, ".kind = XS_MODULE_KIND}, ");
		fprintf(file, ".value = { .module = { ");
		fxPrintAddress(the, file, slot->value.module.realm);
		fprintf(file, ", %d } }", slot->value.module.id);
	} break;
	case XS_MODULE_SOURCE_KIND: {
		fprintf(file, ".kind = XS_MODULE_SOURCE_KIND }, ");
		fprintf(file, ".value = { .module = { ");
		fxPrintAddress(the, file, slot->value.module.realm);
		fprintf(file, ", %d } }", slot->value.module.id);
	} break;
	case XS_PROGRAM_KIND: {
		fprintf(file, ".kind = XS_PROGRAM_KIND}, ");
		fprintf(file, ".value = { .module = { ");
		fxPrintAddress(the, file, slot->value.module.realm);
		fprintf(file, ", %d } }", slot->value.module.id);
	} break;
	case XS_PROMISE_KIND: {
		fprintf(file, ".kind = XS_PROMISE_KIND}, ");
		fprintf(file, ".value = { .integer = %d } ", slot->value.integer);
	} break;
	case XS_PROXY_KIND: {
		fprintf(file, ".kind = XS_PROXY_KIND}, ");
		fprintf(file, ".value = { .instance = { ");
		fxPrintAddress(the, file, slot->value.proxy.handler);
		fprintf(file, ", ");
		fxPrintAddress(the, file, slot->value.proxy.target);
		fprintf(file, " } } ");
	} break;
	case XS_REGEXP_KIND: {
		fprintf(file, ".kind = XS_REGEXP_KIND}, ");
		fprintf(file, ".value = { .regexp = { (txInteger*)&gxRegExpData[%d], (txInteger*)NULL } } ", linker->regexpSize);
		{
			txSize size = mxRegExpSize(slot);
			c_memcpy(linker->regexpData + linker->regexpSize, slot->value.regexp.code, size * sizeof(txInteger));
			linker->regexpSize += size;
		}
	} break;
	case XS_SET_KIND: {
		fprintf(file, ".kind = XS_SET_KIND}, ");
		fprintf(file, ".value = { .table = { (txSlot**)&gxSlotData[%d], %d } }", linker->slotSize, slot->value.table.length);
		c_memcpy(linker->slotData + linker->slotSize, slot->value.table.address, slot->value.table.length * sizeof(txSlot*));
		linker->slotSize += slot->value.table.length;
	} break;
	case XS_TYPED_ARRAY_KIND: {
		fprintf(file, ".kind = XS_TYPED_ARRAY_KIND}, ");
		fprintf(file, ".value = { .typedArray = { (txTypeDispatch*)(&gxTypeDispatches[%d]), (txTypeAtomics*)(&gxTypeAtomics[%d]) } }", fxGetTypeDispatchIndex(slot->value.typedArray.dispatch), fxGetTypeAtomicsIndex(slot->value.typedArray.atomics));
	} break;
	case XS_WEAK_MAP_KIND: {
		fprintf(file, ".kind = XS_WEAK_MAP_KIND}, ");
		fprintf(file, ".value = { .weakList = { ");
		fxPrintAddress(the, file, slot->value.weakList.first);
		fprintf(file, ", NULL } }");
	} break;
	case XS_WEAK_SET_KIND: {
		fprintf(file, ".kind = XS_WEAK_SET_KIND}, ");
		fprintf(file, ".value = { .weakList = { ");
		fxPrintAddress(the, file, slot->value.weakList.first);
		fprintf(file, ", NULL } }");
	} break;
	case XS_WEAK_REF_KIND: {
		fprintf(file, ".kind = XS_WEAK_REF_KIND}, ");
		fprintf(file, ".value = { .weakRef = { ");
		fxPrintAddress(the, file, slot->value.weakRef.target);
		fprintf(file, ", ");
		fxPrintAddress(the, file, slot->value.weakRef.link);
		fprintf(file, " } }");
	} break;
	case XS_ACCESSOR_KIND: {
		fprintf(file, ".kind = XS_ACCESSOR_KIND}, ");
		fprintf(file, ".value = { .accessor = { ");
		fxPrintAddress(the, file, slot->value.accessor.getter);
		fprintf(file, ", ");
		fxPrintAddress(the, file, slot->value.accessor.setter);
		fprintf(file, " } }");
	} break;
	case XS_AT_KIND: {
		fprintf(file, ".kind = XS_AT_KIND}, ");
		fprintf(file, ".value = { .at = { 0x%x, %d } }", slot->value.at.index, slot->value.at.id);
	} break;
	case XS_ENTRY_KIND: {
		fprintf(file, ".kind = XS_ENTRY_KIND}, ");
		fprintf(file, ".value = { .entry = { ");
		fxPrintAddress(the, file, slot->value.entry.slot);
		fprintf(file, ", 0x%x } }", slot->value.entry.sum);
	} break;
	case XS_ERROR_KIND: {
		fprintf(file, ".kind = XS_ERROR_KIND}, ");
		fprintf(file, ".value = { .error = { NULL, %d } }", slot->value.error.which);
	} break;
	case XS_EXPORT_KIND: {
		fprintf(file, ".kind = XS_EXPORT_KIND}, ");
		fprintf(file, ".value = { .export = { ");
		fxPrintAddress(the, file, slot->value.export.closure);
		fprintf(file, ", ");
		fxPrintAddress(the, file, slot->value.export.module);
		fprintf(file, " } }");
	} break;
	case XS_HOME_KIND: {
		fprintf(file, ".kind = XS_HOME_KIND}, ");
		fprintf(file, ".value = { .home = { ");
		fxPrintAddress(the, file, slot->value.home.object);
		fprintf(file, ", ");
		fxPrintAddress(the, file, slot->value.home.module);
		fprintf(file, " } }");
	} break;
	case XS_KEY_KIND:
	case XS_KEY_X_KIND: {
		fprintf(file, ".kind = XS_KEY_X_KIND}, ");
		fprintf(file, ".value = { .key = { ");
		if (slot->value.key.string) {
			fxWriteCString(file, slot->value.key.string);
			fprintf(file, ", 0x%x } }", slot->value.key.sum);
		}
		else {
			fprintf(file, "NULL, 0x0 } }");
		}
	} break;
	case XS_LIST_KIND: {
		fprintf(file, ".kind = XS_LIST_KIND}, ");
		fprintf(file, ".value = { .list = { ");
		fxPrintAddress(the, file, slot->value.list.first);
		fprintf(file, ", ");
		fxPrintAddress(the, file, slot->value.list.last);
		fprintf(file, " } }");
	} break;
	case XS_PRIVATE_KIND: {
		fprintf(file, ".kind = XS_PRIVATE_KIND}, ");
		fprintf(file, ".value = { .private = { ");
		fxPrintAddress(the, file, slot->value.private.check);
		fprintf(file, ", ");
		fxPrintAddress(the, file, slot->value.private.first);
		fprintf(file, " } }");
	} break;
	case XS_STACK_KIND: {
		fprintf(file, ".kind = XS_STACK_KIND}, ");
	} break;
	case XS_WEAK_ENTRY_KIND: {
		fprintf(file, ".kind = XS_WEAK_ENTRY_KIND}, ");
		fprintf(file, ".value = { .weakEntry = { ");
		fxPrintAddress(the, file, slot->value.weakEntry.check);
		fprintf(file, ", ");
		fxPrintAddress(the, file, slot->value.weakEntry.value);
		fprintf(file, " } }");
	} break;
#ifdef mxHostFunctionPrimitive
	case XS_HOST_FUNCTION_KIND: {
		fprintf(file, ".kind = XS_HOST_FUNCTION_KIND}, ");
		fprintf(file, ".value = { .hostFunction = { %s, %d } }", fxGetBuilderName(the, slot->value.hostFunction.builder), slot->value.hostFunction.profileID);
	} break;
#endif
	default:
		break;
	}
	fprintf(file, "},\n");
}

void fxPrintStack(txMachine* the, FILE* file)
{
	txSlot *slot = the->stack;
	txSlot *limit = the->stackTop - 1;
	while (slot < limit) {
		fxPrintSlot(the, file, slot, XS_NO_FLAG);
		slot++;
	}
	fxPrintSlot(the, file, slot, XS_NO_FLAG);
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

void fxSetHostFunctionProperty(txMachine* the, txSlot* property, txCallback call, txInteger length, txID id)
{
	txLinker* linker = (txLinker*)(the->context);
	if (linker->stripFlag) {
		property->kind = XS_HOST_FUNCTION_KIND;
		property->value.hostFunction.builder = fxNewLinkerBuilder(linker, call, length, id);
		property->value.hostFunction.profileID = fxGenerateProfileID(the);
	}
	else {
		txSlot* home = the->stack;
		txSlot* function = fxNewHostFunction(the, call, length, id, XS_NO_ID);
		txSlot* slot = mxFunctionInstanceHome(function);
		slot->value.home.object = home->value.reference;
		property->kind = the->stack->kind;
		property->value = the->stack->value;
		mxPop();
	}
}
