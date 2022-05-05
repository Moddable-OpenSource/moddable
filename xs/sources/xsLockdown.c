#include "xsAll.h"		//@@ add copyright notice!
#include "xsScript.h"

void fxSetHostFunctionProperty(txMachine* the, txSlot* property, txCallback call, txInteger length, txID id)
{
	txSlot* home = the->stack;
	txSlot* function = fxNewHostFunction(the, call, length, id);
	txSlot* slot = mxFunctionInstanceHome(function);
	slot->value.home.object = home->value.reference;
	property->kind = the->stack->kind;
	property->value = the->stack->value;
	mxPop();
}

void fx_lockdown(txMachine* the)
{
#define mxHardenBuiltInCall \
	mxPush(mxGlobal); \
	mxPushSlot(harden); \
	mxCall()
#define mxHardenBuiltInRun \
	mxRunCount(1); \
	mxPop()

	txSlot* instance;
	txSlot* property;
	txSlot* item;
	txSlot* harden;
	txInteger id;
	
	if (mxProgram.value.reference->flag & XS_DONT_MARSHALL_FLAG)
		mxTypeError("lockdown already called");
	mxProgram.value.reference->flag |= XS_DONT_MARSHALL_FLAG;

	property = mxBehaviorGetProperty(the, mxAsyncFunctionPrototype.value.reference, mxID(_constructor), 0, XS_OWN);
	property->kind = mxThrowTypeErrorFunction.kind;
	property->value = mxThrowTypeErrorFunction.value;
	property = mxBehaviorGetProperty(the, mxAsyncGeneratorFunctionPrototype.value.reference, mxID(_constructor), 0, XS_OWN);
	property->kind = mxThrowTypeErrorFunction.kind;
	property->value = mxThrowTypeErrorFunction.value;
	property = mxBehaviorGetProperty(the, mxFunctionPrototype.value.reference, mxID(_constructor), 0, XS_OWN);
	property->kind = mxThrowTypeErrorFunction.kind;
	property->value = mxThrowTypeErrorFunction.value;
	property = mxBehaviorGetProperty(the, mxGeneratorFunctionPrototype.value.reference, mxID(_constructor), 0, XS_OWN);
	property->kind = mxThrowTypeErrorFunction.kind;
	property->value = mxThrowTypeErrorFunction.value;
	property = mxBehaviorGetProperty(the, mxCompartmentPrototype.value.reference, mxID(_constructor), 0, XS_OWN);
	property->kind = mxThrowTypeErrorFunction.kind;
	property->value = mxThrowTypeErrorFunction.value;

	instance = fxNewArray(the, _Compartment);
	property = the->stackPrototypes - 1;
	item = instance->next->value.array.address;
	for (id = 0; id < XS_SYMBOL_ID_COUNT; id++) {
		*((txIndex*)item) = id;
		property--;
		item++;
	}
	for (; id < _Compartment; id++) {
		*((txIndex*)item) = id;
		item->kind = property->kind;
		item->value = property->value;
		property--;
		item++;
	}
	
	fxDuplicateInstance(the, mxDateConstructor.value.reference);
	property = mxFunctionInstanceCode(the->stack->value.reference);
	property->value.callback.address = mxCallback(fx_Date_secure);
	property = mxBehaviorGetProperty(the, the->stack->value.reference, mxID(_now), 0, XS_OWN);
	fxSetHostFunctionProperty(the, property, mxCallback(fx_Date_now_secure), 0, mxID(_now));
	property = mxBehaviorGetProperty(the, mxDatePrototype.value.reference, mxID(_constructor), 0, XS_OWN);
	property->kind = the->stack->kind;
	property->value = the->stack->value;
	mxPull(instance->next->value.array.address[_Date]);
	
	fxDuplicateInstance(the, mxMathObject.value.reference);
	property = mxBehaviorGetProperty(the, the->stack->value.reference, mxID(_random), 0, XS_OWN);
	fxSetHostFunctionProperty(the, property, mxCallback(fx_Math_random_secure), 0, mxID(_random));
	mxPull(instance->next->value.array.address[_Math]);

	mxPull(mxCompartmentGlobal);

	mxTemporary(harden);
	mxPush(mxGlobal);
	fxGetID(the, fxID(the, "harden"));
	mxPullSlot(harden);
	
	for (id = XS_SYMBOL_ID_COUNT; id < _Infinity; id++) {
		mxHardenBuiltInCall; mxPush(the->stackPrototypes[-1 - id]); mxHardenBuiltInRun;
	}
	for (id = _Compartment; id < XS_INTRINSICS_COUNT; id++) {
		mxHardenBuiltInCall; mxPush(the->stackPrototypes[-1 - id]); mxHardenBuiltInRun;
	}
	
	mxHardenBuiltInCall; mxPush(mxArgumentsSloppyPrototype); mxHardenBuiltInRun;
	mxHardenBuiltInCall; mxPush(mxArgumentsStrictPrototype); mxHardenBuiltInRun;
	mxHardenBuiltInCall; mxPush(mxArrayIteratorPrototype); mxHardenBuiltInRun;
	mxHardenBuiltInCall; mxPush(mxAsyncFromSyncIteratorPrototype); mxHardenBuiltInRun;
	mxHardenBuiltInCall; mxPush(mxAsyncFunctionPrototype); mxHardenBuiltInRun;
	mxHardenBuiltInCall; mxPush(mxAsyncGeneratorFunctionPrototype); mxHardenBuiltInRun;
	mxHardenBuiltInCall; mxPush(mxAsyncGeneratorPrototype); mxHardenBuiltInRun;
	mxHardenBuiltInCall; mxPush(mxAsyncIteratorPrototype); mxHardenBuiltInRun;
	mxHardenBuiltInCall; mxPush(mxGeneratorFunctionPrototype); mxHardenBuiltInRun;
	mxHardenBuiltInCall; mxPush(mxGeneratorPrototype); mxHardenBuiltInRun;
	mxHardenBuiltInCall; mxPush(mxHostPrototype); mxHardenBuiltInRun;
	mxHardenBuiltInCall; mxPush(mxIteratorPrototype); mxHardenBuiltInRun;
	mxHardenBuiltInCall; mxPush(mxMapIteratorPrototype); mxHardenBuiltInRun;
	mxHardenBuiltInCall; mxPush(mxModulePrototype); mxHardenBuiltInRun;
	mxHardenBuiltInCall; mxPush(mxRegExpStringIteratorPrototype); mxHardenBuiltInRun;
	mxHardenBuiltInCall; mxPush(mxSetIteratorPrototype); mxHardenBuiltInRun;
	mxHardenBuiltInCall; mxPush(mxStringIteratorPrototype); mxHardenBuiltInRun;
	mxHardenBuiltInCall; mxPush(mxTransferPrototype); mxHardenBuiltInRun;
	mxHardenBuiltInCall; mxPush(mxTypedArrayPrototype); mxHardenBuiltInRun;

	mxHardenBuiltInCall; mxPush(mxAssignObjectFunction); mxHardenBuiltInRun;
	mxHardenBuiltInCall; mxPush(mxCopyObjectFunction); mxHardenBuiltInRun;
	mxHardenBuiltInCall; mxPush(mxEnumeratorFunction); mxHardenBuiltInRun;
	mxHardenBuiltInCall; mxPush(mxInitializeRegExpFunction); mxHardenBuiltInRun;
	mxHardenBuiltInCall; mxPush(mxOnRejectedPromiseFunction); mxHardenBuiltInRun;
	mxHardenBuiltInCall; mxPush(mxOnResolvedPromiseFunction); mxHardenBuiltInRun;
	mxHardenBuiltInCall; mxPush(mxOnThenableFunction); mxHardenBuiltInRun;
	
	mxHardenBuiltInCall; mxPushReference(mxArrayLengthAccessor.value.accessor.getter); mxHardenBuiltInRun;
	mxHardenBuiltInCall; mxPushReference(mxArrayLengthAccessor.value.accessor.setter); mxHardenBuiltInRun;
	mxHardenBuiltInCall; mxPushReference(mxModuleAccessor.value.accessor.getter); mxHardenBuiltInRun;
	mxHardenBuiltInCall; mxPushReference(mxStringAccessor.value.accessor.getter); mxHardenBuiltInRun;
	mxHardenBuiltInCall; mxPushReference(mxStringAccessor.value.accessor.setter); mxHardenBuiltInRun;
	mxHardenBuiltInCall; mxPushReference(mxProxyAccessor.value.accessor.getter); mxHardenBuiltInRun;
	mxHardenBuiltInCall; mxPushReference(mxProxyAccessor.value.accessor.setter); mxHardenBuiltInRun;
	mxHardenBuiltInCall; mxPushReference(mxTypedArrayAccessor.value.accessor.getter); mxHardenBuiltInRun;
	mxHardenBuiltInCall; mxPushReference(mxTypedArrayAccessor.value.accessor.setter); mxHardenBuiltInRun;
	
	mxHardenBuiltInCall; mxPush(mxArrayPrototype); fxGetID(the, mxID(_Symbol_unscopables)); mxHardenBuiltInRun;
	
	mxHardenBuiltInCall; mxPush(mxCompartmentGlobal); mxHardenBuiltInRun;
	
	mxFunctionInstanceCode(mxThrowTypeErrorFunction.value.reference)->ID = XS_NO_ID; 
	mxFunctionInstanceHome(mxThrowTypeErrorFunction.value.reference)->value.home.object = NULL;

	mxPop();
}

static void fx_hardenQueue(txMachine* the, txSlot* list, txSlot* instance, txFlag flag)
{
	txSlot* item;
	if (instance->flag & flag)
		return;
	item = fxNewSlot(the);
	item->value.reference = instance;
	item->kind = XS_REFERENCE_KIND;
	list->value.list.last->next = item;
	list->value.list.last = item;
}

static void fx_hardenFreezeAndTraverse(txMachine* the, txSlot* reference, txSlot* freeze, txSlot* list, txFlag flag)
{
	txSlot* instance = reference->value.reference;
	txSlot* property;
	txBoolean useIndexes = 1;
	txSlot* at;
// 	txSlot* slot;

	if (!mxBehaviorPreventExtensions(the, instance))
		mxTypeError("extensible object");
		
	property = instance->next;	
	if (property && (property->flag & XS_INTERNAL_FLAG) && (property->kind == XS_TYPED_ARRAY_KIND)) {
		 useIndexes = 0;
	}

	at = fxNewInstance(the);
	mxBehaviorOwnKeys(the, instance, XS_EACH_NAME_FLAG | XS_EACH_SYMBOL_FLAG, at);
	mxPushUndefined();
	property = the->stack;
	while ((at = at->next)) {
		if ((at->value.at.id != XS_NO_ID) || useIndexes) {
			if (mxBehaviorGetOwnProperty(the, instance, at->value.at.id, at->value.at.index, property)) {
				txFlag mask = XS_DONT_DELETE_FLAG;
				property->flag |= XS_DONT_DELETE_FLAG;
				if (property->kind != XS_ACCESSOR_KIND) {
					mask |= XS_DONT_SET_FLAG;
					property->flag |= XS_DONT_SET_FLAG;
				}
				property->kind = XS_UNINITIALIZED_KIND;
				if (!mxBehaviorDefineOwnProperty(the, instance, at->value.at.id, at->value.at.index, property, mask))
					mxTypeError("cannot configure property");
			}
		}
	}
	mxPop();
	mxPop();
	
// 	if (flag == XS_DONT_MODIFY_FLAG) {
// 		property = instance->next;
// 		while (property) {
// 			if (property->flag & XS_INTERNAL_FLAG) {
// 				switch (property->kind) {
// 				case XS_ARRAY_BUFFER_KIND:
// 				case XS_DATE_KIND:
// 				case XS_MAP_KIND:
// 				case XS_SET_KIND:
// 				case XS_WEAK_MAP_KIND:
// 				case XS_WEAK_SET_KIND:
// 					property->flag |= XS_DONT_SET_FLAG;
// 					break;				
// 				case XS_PRIVATE_KIND:
// 					slot = property->value.private.first;
// 					while (slot) {
// 						if (slot->kind != XS_ACCESSOR_KIND) 
// 							slot->flag |= XS_DONT_SET_FLAG;
// 						slot->flag |= XS_DONT_DELETE_FLAG;
// 						slot = slot->next;
// 					}
// 					break;
// 				}
// 			}	
// 			property = property->next;
// 		}	
// 	}
	instance->flag |= flag;

	at = fxNewInstance(the);
	mxBehaviorOwnKeys(the, instance, XS_EACH_NAME_FLAG | XS_EACH_SYMBOL_FLAG, at);
	
	mxTemporary(property);
	mxBehaviorGetPrototype(the, instance, property);
	if (property->kind == XS_REFERENCE_KIND)
		fx_hardenQueue(the, list, property->value.reference, flag);
	
	while ((at = at->next)) {
		if (mxBehaviorGetOwnProperty(the, instance, at->value.at.id, at->value.at.index, property)) {
			if (property->kind == XS_REFERENCE_KIND)
				fx_hardenQueue(the, list, property->value.reference, flag);
			else if (property->kind == XS_ACCESSOR_KIND) {
				if (property->value.accessor.getter)
					fx_hardenQueue(the, list, property->value.accessor.getter, flag);
				if (property->value.accessor.setter)
					fx_hardenQueue(the, list, property->value.accessor.setter, flag);
			}
		}
	}
	
// 	if (flag == XS_DONT_MODIFY_FLAG) {
// 		property = instance->next;
// 		while (property) {
// 			if (property->flag & XS_INTERNAL_FLAG) {
// 				if (property->kind == XS_PRIVATE_KIND) {
// 					txSlot* item = property->value.private.first;
// 					while (item) {
// 						if (property->kind == XS_REFERENCE_KIND)
// 							fx_hardenQueue(the, list, property->value.reference, flag);
// 						else if (property->kind == XS_ACCESSOR_KIND) {
// 							if (property->value.accessor.getter)
// 								fx_hardenQueue(the, list, property->value.accessor.getter, flag);
// 							if (property->value.accessor.setter)
// 								fx_hardenQueue(the, list, property->value.accessor.setter, flag);
// 						}
// 						item = item->next;
// 					}
// 				}
// 				else if (property->kind == XS_DATA_VIEW_KIND) {
// 					property = property->next;
// 					fx_hardenQueue(the, list, property->value.reference, flag);
// 				}
// 			}
// 			property = property->next;
// 		}
// 	}
	
	mxPop();
	mxPop();
}

void fx_harden(txMachine* the)
{
	txFlag flag = XS_DONT_MARSHALL_FLAG;
	txSlot* freeze;
	txSlot* slot;
	txSlot* list;
	txSlot* item;

	if (!(mxProgram.value.reference->flag & XS_DONT_MARSHALL_FLAG))
		mxTypeError("call lockdown before harden");

	if (mxArgc == 0)
		return;
		
	*mxResult = *mxArgv(0);
		
	slot = mxArgv(0);	
	if (slot->kind != XS_REFERENCE_KIND)
		return;
// 	if (mxArgc > 1) {
// 		txString string = fxToString(the, mxArgv(1));
// 		if (c_strcmp(string, "freeze") == 0)
// 			flag = XS_DONT_MARSHALL_FLAG;
// 		else if (c_strcmp(string, "petrify") == 0)
// 			flag = XS_DONT_MODIFY_FLAG;
// 		else
// 			mxTypeError("invalid integrity");
// 	}
	slot = slot->value.reference;
	if (slot->flag & flag)
		return;

	mxTemporary(freeze);
	mxPush(mxObjectConstructor);
	mxGetID(mxID(_freeze));
	mxPullSlot(freeze);
	
	mxTemporary(list);
	list->value.list.first = C_NULL;	
	list->value.list.last = C_NULL;	
	list->kind = XS_LIST_KIND;
		
	item = fxNewSlot(the);
	item->value.reference = slot;
	item->kind = XS_REFERENCE_KIND;
	list->value.list.first = item;
	list->value.list.last = item;
		
	{
		mxTry(the) {
			while (item) {
				fx_hardenFreezeAndTraverse(the, item, freeze, list, flag);
				item = item->next;
			}
		}
		mxCatch(the) {
			item = list->value.list.first;
			while (item) {
				item->value.reference->flag &= ~flag;
				item = item->next;
			}
			fxJump(the);
		}
	}
		
	mxPop();
	mxPop();
}

void fx_petrify(txMachine* the)
{
	txSlot* slot;
	txSlot* instance;
	txBoolean useIndexes = 1;
	txSlot* at;
	txSlot* property;
	if (mxArgc == 0)
		return;
	slot = mxArgv(0);	
	*mxResult = *slot;
	if (slot->kind != XS_REFERENCE_KIND)
		return;
	instance = slot->value.reference;
	if (!mxBehaviorPreventExtensions(the, instance))
		mxTypeError("extensible object");
	slot = instance->next;	
	if (slot && (slot->flag & XS_INTERNAL_FLAG)) {
		 if ((slot->kind == XS_STRING_KIND) || (slot->kind == XS_STRING_X_KIND) || (slot->kind == XS_TYPED_ARRAY_KIND))
		 	useIndexes = 0;
	}
		
	at = fxNewInstance(the);
	mxBehaviorOwnKeys(the, instance, XS_EACH_NAME_FLAG | XS_EACH_SYMBOL_FLAG, at);
	mxPushUndefined();
	property = the->stack;
	while ((at = at->next)) {
		if ((at->value.at.id != XS_NO_ID) || useIndexes) {
			if (mxBehaviorGetOwnProperty(the, instance, at->value.at.id, at->value.at.index, property)) {
				txFlag mask = XS_DONT_DELETE_FLAG;
				property->flag |= XS_DONT_DELETE_FLAG;
				if (property->kind != XS_ACCESSOR_KIND) {
					mask |= XS_DONT_SET_FLAG;
					property->flag |= XS_DONT_SET_FLAG;
				}
				property->kind = XS_UNINITIALIZED_KIND;
				if (!mxBehaviorDefineOwnProperty(the, instance, at->value.at.id, at->value.at.index, property, mask))
					mxTypeError("cannot configure property");
			}
		}
	}
	mxPop();
	
	property = instance->next;
	while (property) {
		if (property->flag & XS_INTERNAL_FLAG) {
			switch (property->kind) {
			case XS_ARRAY_BUFFER_KIND:
			case XS_DATE_KIND:
			case XS_MAP_KIND:
			case XS_SET_KIND:
			case XS_WEAK_MAP_KIND:
			case XS_WEAK_SET_KIND:
				property->flag |= XS_DONT_SET_FLAG;
				break;				
			case XS_PRIVATE_KIND:
				slot = property->value.private.first;
				while (slot) {
					if (slot->kind != XS_ACCESSOR_KIND) 
						slot->flag |= XS_DONT_SET_FLAG;
					slot->flag |= XS_DONT_DELETE_FLAG;
					slot = slot->next;
				}
				break;
			}
		}	
		property = property->next;
	}	
}

static void fxVerifyCode(txMachine* the, txSlot* list, txSlot* path, txByte* codeBuffer, txSize codeSize);
static void fxVerifyError(txMachine* the, txSlot* path, txID id, txIndex index, txString name);
static void fxVerifyErrorString(txMachine* the, txSlot* slot, txID id, txIndex index, txString name);
static void fxVerifyInstance(txMachine* the, txSlot* list, txSlot* path, txSlot* instance);
static void fxVerifyProperty(txMachine* the, txSlot *list, txSlot *path, txSlot* property, txID id);
static void fxVerifyPropertyError(txMachine* the, txSlot *list, txSlot *path, txSlot* property, txID id, txIndex index);
static void fxVerifyQueue(txMachine* the, txSlot* list, txSlot* path, txSlot* instance, txID id, txIndex index, txString name);

void fx_mutabilities(txMachine* the)
{
	txSlot* instance;
	txSlot* module;
	txSlot* realm;
	txSlot* slot;
	txSlot* list;
	txSlot* item;
	
	fxVars(the, 2);
	
	mxPush(mxArrayPrototype);
	instance = fxNewArrayInstance(the);
	mxPullSlot(mxResult);

	fxNewHostObject(the, NULL);
	fxSetHostChunk(the, the->stack, NULL, the->keyIndex);
	mxPullSlot(mxVarv(0));
	
	module = mxFunctionInstanceHome(mxFunction->value.reference)->value.home.module;
	if (!module) module = mxProgram.value.reference;
	realm = mxModuleInstanceInternal(module)->value.module.realm;
	mxPushSlot(mxRealmGlobal(realm));
	mxPullSlot(mxVarv(1));

	if (mxArgc == 0)
		return;
	slot = mxArgv(0);	
	if (slot->kind != XS_REFERENCE_KIND)
		return;
	
	mxTemporary(list);
	list->value.list.first = C_NULL;	
	list->value.list.last = C_NULL;	
	list->kind = XS_LIST_KIND;
		
	item = fxNewSlot(the);
	item->value.list.first = C_NULL;
	item->value.list.last = slot->value.reference;
	item->kind = XS_LIST_KIND;
	list->value.list.first = item;
	list->value.list.last = item;
		
	{
		mxTry(the) {
			while (item) {
				fxVerifyInstance(the, list, item->value.list.first, item->value.list.last);
				item = item->next;
			}
			item = list->value.list.first;
			while (item) {
				item->value.list.last->flag &= ~XS_LEVEL_FLAG;
				item = item->next;
			}
		}
		mxCatch(the) {
			item = list->value.list.first;
			while (item) {
				item->value.list.last->flag &= ~XS_LEVEL_FLAG;
				item = item->next;
			}
			fxJump(the);
		}
	}
	
	mxPop(); // list
	
	fxCacheArray(the, instance);
	mxPushSlot(mxResult);
	mxPushSlot(mxResult);
	mxGetID(mxID(_sort));
	mxCall();
	mxRunCount(0);
	mxPullSlot(mxResult);
}

void fxVerifyCode(txMachine* the, txSlot* list, txSlot* path, txByte* codeBuffer, txSize codeSize)
{
	const txS1* bytes = gxCodeSizes;
	txByte* p = codeBuffer;
	txByte* q = p + codeSize;
	txU1 byte;
	txS1 offset;
	txID id;
	txInteger count = 0;
	txByte flag = 0;
	txByte* flags = fxGetHostChunk(the, mxVarv(0));
	while (p < q) {
// 		fprintf(stderr, "%s", gxCodeNames[*((txU1*)p)]);
		byte = (txU1)c_read8(p);
		offset = (txS1)c_read8(bytes + byte);
		if (0 < offset) {
			p += offset;
		}
		else if (0 == offset) {
			p++;
			mxDecodeID(p, id);
			if (byte == XS_CODE_PROGRAM_REFERENCE) {
				flag = 1;
				flags[id] = 1;
			}
		}
		else if (-1 == offset) {
			txU1 index;
			p++;
			index = *((txU1*)p);
			p += 1 + index;
		}
        else if (-2 == offset) {
			txU2 index;
            p++;
            mxDecode2(p, index);
            p += index;
        }
        else if (-4 == offset) {
			txS4 index;
            p++;
            mxDecode4(p, index);
            p += index;
        }
// 		fprintf(stderr, "\n");
		if ((XS_CODE_BEGIN_SLOPPY <= byte) && (byte <= XS_CODE_BEGIN_STRICT_FIELD)) {
			count++;
		}
		else if ((XS_CODE_END <= byte) && (byte <= XS_CODE_END_DERIVED)) {
			count--;
			if (count == 0)
				break;
		}
	}
	if (flag) {
		txSlot* instance = fxGetInstance(the, mxVarv(1));
		txSlot* item;
		txSlot* name;
		
		mxTemporary(item);
		
		item->value.list.first = name = fxNewSlot(the);
		item->value.list.last = C_NULL;
		item->kind = XS_LIST_KIND;

		name->value.string = "GlobalEnvironment";
		name->kind = XS_STRING_X_KIND;
		name->next = path;
		
		flags = fxGetHostChunk(the, mxVarv(0));
		id = 0;
		while (id < the->keyIndex) {
			if (flags[id]) {
				txSlot* property = mxBehaviorGetProperty(the, instance, id, 0, XS_OWN);
				if (property)
					fxVerifyProperty(the, list, name, property, id);
				flags = fxGetHostChunk(the, mxVarv(0));
				flags[id]= 0;
			}
			id++;
		}
		
		mxPop();
	}
}

void fxVerifyError(txMachine* the, txSlot* path, txID id, txIndex index, txString string)
{
	txSlot* array;
	txSlot* slot;
	txSlot* current;
	txSlot* next;
	txSlot* previous;
	
	array = mxResult->value.reference->next;
	slot = fxNewSlot(the);
	slot->next = array->next;
	array->next = slot;
	array->value.array.length++;
	fxString(the, slot, "");
	
	current = path;
	next = C_NULL;
	previous = C_NULL;
	while (current) {
		next = current->next;
		current->next = previous;
		previous = current;
		current = next;
	}
	
	path = previous;
	current = path;
	while (current) {
		if (current->kind == XS_STRING_X_KIND) {
			fxVerifyErrorString(the, slot, XS_NO_ID, 0, current->value.string);
		}
		else {
			fxVerifyErrorString(the, slot, current->value.at.id, current->value.at.index, C_NULL);
		}
		current = current->next;
	}
	fxVerifyErrorString(the, slot, id, index, string);
	
	current = path;
	next = C_NULL;
	previous = C_NULL;
	while (current) {
		next = current->next;
		current->next = previous;
		previous = current;
		current = next;
	}
}

void fxVerifyErrorString(txMachine* the, txSlot* slot, txID id, txIndex index, txString string)
{
	if (string) {
		fxConcatStringC(the, slot, "[[");
		fxConcatStringC(the, slot, string);
		fxConcatStringC(the, slot, "]]");
	}
	else if (id != XS_NO_ID) {
		txSlot* key = fxGetKey(the, id);
		if (key) {
			if (key->flag & XS_DONT_ENUM_FLAG) {
				c_snprintf(the->nameBuffer, sizeof(the->nameBuffer), "%s", key->value.key.string);
				fxConcatStringC(the, slot, ".");
				fxConcatStringC(the, slot, the->nameBuffer);
			}
			else {
				if ((key->kind == XS_KEY_KIND) || (key->kind == XS_KEY_X_KIND))
					c_snprintf(the->nameBuffer, sizeof(the->nameBuffer), "%s", key->value.key.string);
				else if ((key->kind == XS_STRING_KIND) || (key->kind == XS_STRING_X_KIND))
					c_snprintf(the->nameBuffer, sizeof(the->nameBuffer), "%s", key->value.string);
				else
					the->nameBuffer[0] = 0;
				fxConcatStringC(the, slot, "[Symbol(");
				fxConcatStringC(the, slot, the->nameBuffer);
				fxConcatStringC(the, slot, ")]");
			}
		}
		else {
			fxConcatStringC(the, slot, "[Symbol()]");
		}
	}
	else {
		fxNumberToString(the->dtoa, index, the->nameBuffer, sizeof(the->nameBuffer), 0, 0);
		fxConcatStringC(the, slot, "[");
		fxConcatStringC(the, slot, the->nameBuffer);
		fxConcatStringC(the, slot, "]");
	}
}

void fxVerifyInstance(txMachine* the, txSlot* list, txSlot* path, txSlot* instance)
{
	txSlot* property;
	txSlot* prototype;
	
	instance->flag |= XS_LEVEL_FLAG;
	
	if (instance->next && (instance->next->ID == XS_ENVIRONMENT_BEHAVIOR)) {
		property = instance->next->next;
		while (property) {
			if ((property->kind == XS_CLOSURE_KIND) && (property->ID != XS_NO_ID)) { // skip private fields initializers
				txSlot* closure = property->value.closure;
				if (!(closure->flag & XS_DONT_SET_FLAG)) {
					fxVerifyError(the, path, property->ID, 0, C_NULL);
				}
				if (closure->kind == XS_REFERENCE_KIND) {
					fxVerifyQueue(the, list, path, closure->value.reference, property->ID, 0, C_NULL);
				}
			}
			property = property->next;
		}
		return;
	}
	
	if (!(instance->flag & XS_DONT_PATCH_FLAG)) {
		fxVerifyError(the, path, XS_NO_ID, 0, "Extensible");
	}

	prototype = fxGetPrototype(the, instance);
	if (prototype) {
		fxVerifyQueue(the, list, path, prototype, mxID(___proto__), 0, C_NULL);
	}
	
	property = instance->next;
	while (property) {
		if (property->flag & XS_INTERNAL_FLAG) {
			switch (property->kind) {
			case XS_ARRAY_KIND: 
				{
					txSlot* address = property->value.array.address;
					if (address) {
						txIndex index, offset = 0, size = (((txChunk*)(((txByte*)address) - sizeof(txChunk)))->size) / sizeof(txSlot);
						while (offset < size) {
							address = property->value.array.address + offset;
							index = *((txIndex*)address);
							fxVerifyPropertyError(the, list, path, address, XS_NO_ID, index);
							address = property->value.array.address + offset;
							if (address->kind == XS_REFERENCE_KIND)
								fxVerifyQueue(the, list, path, address->value.reference, XS_NO_ID, index, C_NULL);
							else if (address->kind == XS_ACCESSOR_KIND) {
								if (address->value.accessor.getter)
									fxVerifyQueue(the, list, path, address->value.accessor.getter, XS_NO_ID, index, C_NULL);
								address = property->value.array.address + offset;
								if (address->value.accessor.setter)
									fxVerifyQueue(the, list, path, address->value.accessor.setter, XS_NO_ID, index, C_NULL);
							}
							offset++;
						}
					}
				} 
				break;
			case XS_ARRAY_BUFFER_KIND:
				if (!(property->flag & XS_DONT_SET_FLAG)) {
					if (property->value.arrayBuffer.address != C_NULL)
						fxVerifyError(the, path, XS_NO_ID, 0, "ArrayBufferData");
				}
				break;
			case XS_CODE_KIND:
			case XS_CODE_X_KIND:
				if (property->value.code.closures)
					fxVerifyQueue(the, list, path, property->value.code.closures, XS_NO_ID, 0, "Environment");
                fxVerifyCode(the, list, path, property->value.code.address, ((txChunk*)(((txByte*)(property->value.code.address)) - sizeof(txChunk)))->size);
				break;
			case XS_DATA_VIEW_KIND:
				property = property->next;
				fxVerifyQueue(the, list, path, property->value.reference, XS_NO_ID, 0, "ViewedArrayBuffer");
				break;
			case XS_DATE_KIND:
				if (!(property->flag & XS_DONT_SET_FLAG))
					fxVerifyError(the, path, XS_NO_ID, 0, "DateValue");
				break;
			case XS_REGEXP_KIND:
				break;
			case XS_MAP_KIND:
				if (!(property->flag & XS_DONT_SET_FLAG))
					fxVerifyError(the, path, XS_NO_ID, 0, "MapData");
				break;
			case XS_MODULE_KIND:
				{
					txSlot* exports = mxModuleInstanceExports(instance);
					if (mxIsReference(exports)) {
						txSlot* property = exports->value.reference->next;
						while (property) {
							if (property->value.export.closure) {
								txSlot* closure = property->value.export.closure;
								closure->flag |= XS_DONT_DELETE_FLAG;
								fxVerifyProperty(the, list, path, closure, property->ID);
								closure->flag &= ~XS_DONT_DELETE_FLAG;
							}
							property = property->next;
						}
					}
				}
				break;
			case XS_PRIVATE_KIND:
				{
					txSlot* item = property->value.private.first;
					while (item) {
						fxVerifyProperty(the, list, path, item, item->ID);
						item = item->next;
					}
				}
				break;
			case XS_PROXY_KIND:
				if (property->value.proxy.handler) {
					fxVerifyQueue(the, list, path, property->value.proxy.target, XS_NO_ID, 0, "ProxyHandler");
				}
				if (property->value.proxy.target) {
					fxVerifyQueue(the, list, path, property->value.proxy.target, XS_NO_ID, 0, "ProxyTarget");
				}
				break;
			case XS_SET_KIND:
				if (!(property->flag & XS_DONT_SET_FLAG))
					fxVerifyError(the, path, XS_NO_ID, 0, "SetData");
				break;
			case XS_WEAK_MAP_KIND:
				if (!(property->flag & XS_DONT_SET_FLAG))
					fxVerifyError(the, path, XS_NO_ID, 0, "WeakMapData");
				break;
			case XS_WEAK_SET_KIND:
				if (!(property->flag & XS_DONT_SET_FLAG))
					fxVerifyError(the, path, XS_NO_ID, 0, "WeakSetData");
				break;
			}
		}
		else {
			fxVerifyProperty(the, list, path, property, property->ID);
		}
		property = property->next;
	}
}

void fxVerifyProperty(txMachine* the, txSlot *list, txSlot *path, txSlot* property, txID id)
{
	fxVerifyPropertyError(the, list, path, property, id, 0);
	if (property->kind == XS_REFERENCE_KIND)
		fxVerifyQueue(the, list, path, property->value.reference, id, 0, C_NULL);
	else if (property->kind == XS_ACCESSOR_KIND) {
		if (property->value.accessor.getter)
			fxVerifyQueue(the, list, path, property->value.accessor.getter, id, 0, C_NULL);
		if (property->value.accessor.setter)
			fxVerifyQueue(the, list, path, property->value.accessor.setter, id, 0, C_NULL);
	}
}

void fxVerifyPropertyError(txMachine* the, txSlot *list, txSlot *path, txSlot* property, txID id, txIndex index)
{
	txBoolean immutable = 1;
	if (property->kind != XS_ACCESSOR_KIND) 
		if (!(property->flag & XS_DONT_SET_FLAG))
			immutable = 0;
	if (!(property->flag & XS_DONT_DELETE_FLAG))
		immutable = 0;
	if (!immutable)
		fxVerifyError(the, path, id, index, C_NULL);
}

void fxVerifyQueue(txMachine* the, txSlot* list, txSlot* path, txSlot* instance, txID id, txIndex index, txString string)
{
	txSlot* item;
	txSlot* name;
	if (instance->kind != XS_INSTANCE_KIND)
		return;
	if (instance->flag & XS_LEVEL_FLAG)
		return;
	item = fxNewSlot(the);
	list->value.list.last->next = item;
	list->value.list.last = item;
	
	item->value.list.first = name = fxNewSlot(the);
	if (string) {
		name->value.string = string;
		name->kind = XS_STRING_X_KIND;
	}
	else {
		name->value.at.id = id;
		name->value.at.index = index;
		name->kind = XS_AT_KIND;
	}
	name->next = path;
	item->value.list.last = instance;
	item->kind = XS_LIST_KIND;
}
