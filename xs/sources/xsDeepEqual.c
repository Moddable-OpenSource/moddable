#include "xsAll.h"		//@@ add copyright notice!
#include "xsScript.h"

typedef struct sx_structuredCloneLink tx_structuredCloneLink;
typedef struct sx_structuredCloneList tx_structuredCloneList;

struct sx_structuredCloneLink {
	tx_structuredCloneLink* previous;
	tx_structuredCloneLink* next;
	txID ID;
	txIndex index;
};

struct sx_structuredCloneList {
	tx_structuredCloneLink *first;
	tx_structuredCloneLink* last;
};

static void fx_structuredCloneArray(txMachine* the, tx_structuredCloneList* list, txSlot* fromArray, txSlot* toArray);
static void fx_structuredCloneEntries(txMachine* the, tx_structuredCloneList* list, txSlot* from, txSlot* to, txBoolean paired);
static void fx_structuredCloneInstance(txMachine* the, tx_structuredCloneList* list);
static void fx_structuredCloneSlot(txMachine* the, tx_structuredCloneList* list);
static void fx_structuredCloneThrow(txMachine* the, tx_structuredCloneList* list, txString message);

void fx_structuredClone(txMachine* the)
{
	tx_structuredCloneLink link = { C_NULL, C_NULL, mxID(_value), 0 };
	tx_structuredCloneList list = { &link, &link };
	fxVars(the, 3);
	
	mxPush(mxMapConstructor);
	mxNew();
	mxRunCount(0);
	mxPullSlot(mxVarv(0));
	
	mxPushSlot(mxVarv(0));
	mxGetID(mxID(_get));
	mxPullSlot(mxVarv(1));
	
	mxPushSlot(mxVarv(0));
	mxGetID(mxID(_set));
	mxPullSlot(mxVarv(2));

	if (mxArgc > 0)
		mxPushSlot(mxArgv(0));
	else
		mxPushUndefined();
	if (mxArgc > 1) {
		// transferable?
	}
	if (mxArgc > 2) {
		// options?
	}
	
	fx_structuredCloneSlot(the, &list);
	mxPullSlot(mxResult);
}

void fx_structuredCloneArray(txMachine* the, tx_structuredCloneList* list, txSlot* fromArray, txSlot* toArray) 
{
	list->last->ID = XS_NO_ID;
	toArray->flag = XS_INTERNAL_FLAG;
	toArray->kind = XS_ARRAY_KIND;
	toArray->value.array.address = C_NULL;
	toArray->value.array.length = fromArray->value.array.length;
	if (fromArray->value.array.address) {
		txSize size = (((txChunk*)(((txByte*)fromArray->value.array.address) - sizeof(txChunk)))->size) / sizeof(txSlot);
		txSize fromOffset = 0, toOffset = 0;
		toArray->value.array.address = fxNewChunk(the, size * sizeof(txSlot));
		c_memset(toArray->value.array.address, 0, size * sizeof(txSlot));
		while (fromOffset < size) {
			txSlot* from = fromArray->value.array.address + fromOffset;
			txSlot* to;
			txIndex index = *((txIndex*)from);
			if (!(from->flag & XS_DONT_ENUM_FLAG)) {
				if (from->kind == XS_ACCESSOR_KIND) {
					txSlot* instance = the->stack + 1;
					mxPushSlot(instance);
					mxGetIndex(index);
				}
				else
					mxPushSlot(from);
				list->last->index = index;
				fx_structuredCloneSlot(the, list);
				to = toArray->value.array.address + toOffset;
				mxPullSlot(to);
				*((txIndex*)to) = index;
				toOffset++;
			}
			fromOffset++;
		}
	}
}

void fx_structuredCloneEntries(txMachine* the, tx_structuredCloneList* list, txSlot* from, txSlot* to, txBoolean paired) 
{
	txSlot* fromTable;
	txSlot* toTable;
	txSlot* fromList;
	txSlot* toList;
	txSlot** address;

	fromTable = from;

	toTable = to->next = fxNewSlot(the);
	toTable->flag = XS_INTERNAL_FLAG;
	toTable->kind = fromTable->kind;
	toTable->value.table.address = fxNewChunk(the, fromTable->value.table.length * sizeof(txSlot*));
	toTable->value.table.length = fromTable->value.table.length;
	c_memset(toTable->value.table.address, 0, toTable->value.table.length * sizeof(txSlot*));

	fromList = fromTable->next;
	
	toList = toTable->next = fxNewSlot(the);
	toList->flag = XS_INTERNAL_FLAG;
	toList->kind = XS_LIST_KIND;
	toList->value.list.first = C_NULL;
	toList->value.list.last = C_NULL;

	from = fromList->value.list.first;
	address = &toList->value.list.first;
	while (from) {
		to = *address = fxNewSlot(the);
		mxPushSlot(from);
		fx_structuredCloneSlot(the, list);
		mxPullSlot(to);
		from = from->next;
		address = &to->next;
	}
	toList->value.list.last = to;
	
	to = toList->value.list.first;
	while (to) {
		txSlot* entry = fxNewSlot(the);
		txU4 sum = fxSumEntry(the, to);
		txU4 index = sum & (toTable->value.table.length - 1);
		address = &(toTable->value.table.address[index]);
		entry->next = *address;
		entry->kind = XS_ENTRY_KIND;
		entry->value.entry.slot = to;
		entry->value.entry.sum = sum;
		*address = entry;
		to = to->next;
		if (paired)
			to = to->next;
	}
	
	toList->next = fxDuplicateSlot(the, fromList->next);
}

void fx_structuredCloneInstance(txMachine* the, tx_structuredCloneList* list) 
{
	tx_structuredCloneLink link = { C_NULL, C_NULL, XS_NO_ID, 0 };

	txSlot* instance = the->stack->value.reference;
	txSlot* result;
	txSlot* from;
	txSlot* to;
	txSize size;
	
	mxPushSlot(mxVarv(0));
	mxPushSlot(mxVarv(1));
	mxCall();
	mxPushReference(instance);
	mxRunCount(1);
	if (mxIsReference(the->stack))
		return;
	mxPop();
	
	link.previous = list->last;
	list->last->next = &link;
	list->last = &link;
	
	result = fxNewInstance(the);
	result->value.instance.prototype = mxObjectPrototype.value.reference;
	
	mxPushSlot(mxVarv(0));
	mxPushSlot(mxVarv(2));
	mxCall();
	mxPushReference(instance);
	mxPushReference(result);
	mxRunCount(2);
	mxPop();
	
	from = instance->next;
	to = result;

	if (from->flag & XS_INTERNAL_FLAG) {
		switch (from->kind) {
		case XS_BOOLEAN_KIND:
			result->value.instance.prototype = mxBooleanPrototype.value.reference;
			to = to->next = fxDuplicateSlot(the, from);
			from = from->next;
			break;
		case XS_NUMBER_KIND:
			result->value.instance.prototype = mxNumberPrototype.value.reference;
			to = to->next = fxDuplicateSlot(the, from);
			from = from->next;
			break;
		case XS_STRING_KIND:
		case XS_STRING_X_KIND:
			result->value.instance.prototype = mxStringPrototype.value.reference;
			result->flag |= XS_EXOTIC_FLAG;
			to = to->next = fxDuplicateSlot(the, from);
			from = from->next;
			break;
		case XS_BIGINT_KIND:
		case XS_BIGINT_X_KIND:
			result->value.instance.prototype = mxBigIntPrototype.value.reference;
			to = to->next = fxDuplicateSlot(the, from);
			from = from->next;
			break;
		case XS_DATE_KIND:
			result->value.instance.prototype = mxDatePrototype.value.reference;
			to = to->next = fxDuplicateSlot(the, from);
			from = from->next;
			break;
		case XS_ERROR_KIND:
// 			for (error = XS_EVAL_ERROR; error < XS_ERROR_COUNT, error++) {
// 				if (instance->value.instance.prototype == mxErrorPrototype.value.references[error]) {
// 					result->value.instance.prototype = mxErrorPrototype.value.references[error];
// 				}
// 			}
// 			if (error == XS_ERROR_COUNT)
// 				result->value.instance.prototype = mxErrorPrototype.value.reference;
			to = to->next = fxDuplicateSlot(the, from);
			from = from->next;
// 			fx_structuredCloneProperty(the, mxID(_message));
// 			fx_structuredCloneProperty(the, mxID(_cause));
			break;
		case XS_REGEXP_KIND:
			result->value.instance.prototype = mxRegExpPrototype.value.reference;
			to = to->next = fxDuplicateSlot(the, from);
			from = from->next;
			to = to->next = fxDuplicateSlot(the, from);
			from = from->next;
			to = to->next = fxDuplicateSlot(the, from);
			to->value.integer = 0;
			from = from->next;
			break;
		
		case XS_ARRAY_KIND:
			if (from->flag & XS_EXOTIC_FLAG) {
				if ((from->ID == XS_ARGUMENTS_SLOPPY_BEHAVIOR) || (from->ID == XS_ARGUMENTS_STRICT_BEHAVIOR))
					fx_structuredCloneThrow(the, list, "arguments");
				result->value.instance.prototype = mxArrayPrototype.value.reference;
				result->flag |= XS_EXOTIC_FLAG;
			}
			to = to->next = fxNewSlot(the);
			fx_structuredCloneArray(the, list, from, to);
			from = from->next;
			break;
		case XS_CALLBACK_KIND:
		case XS_CALLBACK_X_KIND:
		case XS_CODE_KIND:
		case XS_CODE_X_KIND:
			fx_structuredCloneThrow(the, list, "function");
			break;
		case XS_FINALIZATION_CELL_KIND:
		case XS_FINALIZATION_REGISTRY_KIND:
			fx_structuredCloneThrow(the, list, "finalization");
			break;
		case XS_MODULE_KIND:
		case XS_PROGRAM_KIND:
			fx_structuredCloneThrow(the, list, "module");
			break;
		case XS_PROMISE_KIND:
			fx_structuredCloneThrow(the, list, "promise");
			break;
		case XS_WEAK_MAP_KIND:
			fx_structuredCloneThrow(the, list, "weak map");
			break;
		case XS_WEAK_REF_KIND:
			fx_structuredCloneThrow(the, list, "weak ref");
			break;
		case XS_WEAK_SET_KIND:
			fx_structuredCloneThrow(the, list, "weak set");
			break;
		case XS_STACK_KIND:
			fx_structuredCloneThrow(the, list, "generator");
			break;
		case XS_SYMBOL_KIND:
			fx_structuredCloneThrow(the, list, "symbol");
			break;

		case XS_ARRAY_BUFFER_KIND:
			result->value.instance.prototype = mxArrayBufferPrototype.value.reference;
			if (!from->value.arrayBuffer.address)
				fx_structuredCloneThrow(the, list, "detached array buffer");
			to = to->next = fxDuplicateSlot(the, from);
			size = from->next->value.bufferInfo.length;
			to->value.arrayBuffer.address = fxNewChunk(the, size);
			c_memcpy(to->value.arrayBuffer.address, from->value.arrayBuffer.address, size);
			from = from->next;
			to = to->next = fxDuplicateSlot(the, from);
			from = from->next;
			break;
		case XS_TYPED_ARRAY_KIND:
			mxPush(the->stackPrototypes[-1 - from->value.typedArray.dispatch->constructorID]);
			mxGetID(mxID(_prototype));
			result->value.instance.prototype = fxToInstance(the, the->stack);
			mxPop();
			result->flag |= XS_EXOTIC_FLAG;
			to = to->next = fxDuplicateSlot(the, from);
			from = from->next;
			to = to->next = fxDuplicateSlot(the, from);
			from = from->next;
			to = to->next = fxNewSlot(the);
			mxPushSlot(from);
			fx_structuredCloneSlot(the, list);
			mxPullSlot(to);
			from = from->next;
			break;
		case XS_DATA_VIEW_KIND:
			result->value.instance.prototype = mxDataViewPrototype.value.reference;
			to = to->next = fxDuplicateSlot(the, from);
			from = from->next;
			to = to->next = fxNewSlot(the);
			mxPushSlot(from);
			fx_structuredCloneSlot(the, list);
			mxPullSlot(to);
			from = from->next;
			break;
			
		case XS_MAP_KIND:
			result->value.instance.prototype = mxMapPrototype.value.reference;
			fx_structuredCloneEntries(the, list, from, to, 1);
			from = from->next->next->next;
			to = to->next->next->next;
			break;
		case XS_SET_KIND:
			result->value.instance.prototype = mxSetPrototype.value.reference;
			fx_structuredCloneEntries(the, list, from, to, 0);
			from = from->next->next->next;
			to = to->next->next->next;
			break;
			
		default:
			
			break;
		}
	}
	while (from) {
		if (from->flag & XS_INTERNAL_FLAG) {
			if (from->kind == XS_ARRAY_KIND) {
				to = to->next = fxNewSlot(the);
				fx_structuredCloneArray(the, list, from, to);
			}
		}
		else
			break;
		from = from->next;
	}	
	
	link.index = 0;
	while (from) {
		if (!(from->flag & XS_DONT_ENUM_FLAG) && fxIsKeyName(the, from->ID)) {
			to = to->next = fxNewSlot(the);
			if (from->kind == XS_ACCESSOR_KIND) {
				mxPushReference(instance);
				mxGetID(from->ID);
			}
			else
				mxPushSlot(from);
			link.ID = from->ID;
			fx_structuredCloneSlot(the, list);
			mxPullSlot(to);
			to->ID = from->ID;
		}
		from = from->next;
	}
	list->last = link.previous;
}

void fx_structuredCloneSlot(txMachine* the, tx_structuredCloneList* list) 
{
	txSlot* slot = the->stack;
	switch (slot->kind) {
	case XS_UNDEFINED_KIND:
	case XS_NULL_KIND:
	case XS_BOOLEAN_KIND:
	case XS_INTEGER_KIND:
	case XS_NUMBER_KIND:
	case XS_STRING_KIND:
	case XS_STRING_X_KIND:
	case XS_BIGINT_KIND:
	case XS_BIGINT_X_KIND:
		mxPushSlot(slot);
		break;
	case XS_SYMBOL_KIND:
		fx_structuredCloneThrow(the, list, "symbol");
		break;
	case XS_REFERENCE_KIND:
		fx_structuredCloneInstance(the, list);
		break;
#ifdef mxHostFunctionPrimitive
	case XS_HOST_FUNCTION_KIND:
		fx_structuredCloneThrow(the, list, "function");
		break;
#endif
	default:
		fx_structuredCloneThrow(the, list, "slot");
		break;
	}
	mxPullSlot(slot);
}

void fx_structuredCloneThrow(txMachine* the, tx_structuredCloneList* list, txString message) 
{
	char buffer[128] = "";
	txInteger i = 0;
	txInteger c = sizeof(buffer);
	tx_structuredCloneLink* link = list->first;
	while (link) {
		if (link->ID) {
			txSlot* key = fxGetKey(the, link->ID);
			if (key) {
				txKind kind = mxGetKeySlotKind(key);
				if ((kind == XS_KEY_KIND) || (kind == XS_KEY_X_KIND)) {
					if (i < c) i += c_snprintf(buffer + i, c - i, "%s%s", (link == list->first) ? "" : ".", key->value.string);
				}
			}
		}
		else {
			if (i < c) i += c_snprintf(buffer + i, c - i, "[%d]", (int)link->index);
		}
		link = link->next;
	}
	if (i < c)
		i += c_snprintf(buffer + i, c - i, ": %s", message);
	mxTypeError(buffer);
}


