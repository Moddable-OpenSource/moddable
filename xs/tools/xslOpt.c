#include "xsl.h"

typedef struct sxColorEntry txColorEntry;
typedef struct sxColorTable txColorTable;
typedef struct sxConflictEntry txConflictEntry;
typedef struct sxConflictTable txConflictTable;

struct sxColorEntry {
	txColorEntry* next;
	txSlot* property;
	txConflictTable* conflictTable;
};

struct sxColorTable {
	txColorEntry* first;
	txColorEntry* last;
	txSlot* instance;
	txSize size;
};

struct sxConflictEntry {
	txConflictEntry* next;
	txColorTable* colorTable;
};

struct sxConflictTable {
	txConflictEntry* first;
	txConflictEntry* last;
	txID id;
	txSize count;
	txID color;
};

static int fxCompareColorTable(const void* p, const void* q);
static void fxFindColor(txConflictTable* conflictTable, txFlag* flags, txID count);
static txInteger fxFindHoles(txSlot** table, txFlag* flags, txID count);
static void fxIndexSlot(txLinker* linker, txSlot* slot, txInteger index);
static void fxLinkColorEntry(txColorTable* colorTable, txColorEntry* colorEntry);
static void fxLinkConflictEntry(txConflictTable* colorTable, txConflictEntry* colorEntry);
static txColorEntry* fxNewColorEntry(txLinker* linker, txSlot* property);
static txColorTable* fxNewColorTable(txLinker* linker, txSlot* instance);
static txConflictEntry* fxNewConflictEntry(txLinker* linker, txColorTable* colorTable);
static txConflictTable* fxNewConflictTable(txLinker* linker, txID id);

int fxCompareColorTable(const void* p, const void* q)
{
	txColorTable* a = *((txColorTable**)p);
	txColorTable* b = *((txColorTable**)q);
	return (b->size) - (a->size);
}

int fxCompareConflictTable(const void* p, const void* q)
{
	txConflictTable* a = *((txConflictTable**)p);
	txConflictTable* b = *((txConflictTable**)q);
	if (a) {
		if (b) {
			return b->count - a->count;
		}
		return -1;
	}
	if (b) {
		return 1;
	}
	return 0;
}

void fxFindColor(txConflictTable* conflictTable, txFlag* flags, txID count)
{
	txConflictEntry* conflictEntry = conflictTable->first;
	memset(flags, 0, count);
	flags[0] = 1;
	while (conflictEntry) {
		txColorTable* colorTable = conflictEntry->colorTable;
		txColorEntry* colorEntry = colorTable->first;
		while (colorEntry) {
			txID color = colorEntry->conflictTable->color;
			flags[color] = 1;
			colorEntry = colorEntry->next;
		}
		conflictEntry = conflictEntry->next;
	}
	conflictTable->color = (txID)(((txFlag*)memchr(flags, 0, count)) - flags);
}

txInteger fxFindHoles(txSlot** table, txFlag* flags, txID count)
{
	txInteger src = 0;
	txInteger dst = 0;
	txInteger result = 0;
	while (src < count) {
		if (flags[src] && table[dst]) {
			src = 0;
			while (table[dst])
				dst++;
			result = dst;
		}
		src++;
		dst++;
	}
	return result;
}

void fxIndexSlot(txLinker* linker, txSlot* slot, txInteger index)
{
	txLinkerProjection* projection = linker->firstProjection;
	while (projection) {
		if ((projection->heap < slot) && (slot < projection->limit)) {
			projection->indexes[slot - projection->heap] = index;
			linker->layout[index] = slot;
			return;
		}
		projection = projection->nextProjection;
	}
}

void fxLinkColorEntry(txColorTable* colorTable, txColorEntry* colorEntry)
{
	if (colorTable->last)
		colorTable->last->next = colorEntry;
	else
		colorTable->first = colorEntry;
	colorTable->last = colorEntry;
}

void fxLinkConflictEntry(txConflictTable* conflictTable, txConflictEntry* conflictEntry)
{
	if (conflictTable->last)
		conflictTable->last->next = conflictEntry;
	else
		conflictTable->first = conflictEntry;
	conflictTable->last = conflictEntry;
	conflictTable->count++;
}

txColorEntry* fxNewColorEntry(txLinker* linker, txSlot* property)
{
	txColorEntry* result = fxNewLinkerChunkClear(linker, sizeof(txColorEntry));
	result->property = property;
	return result;
}

txColorTable* fxNewColorTable(txLinker* linker, txSlot* instance)
{
	txColorTable* result = fxNewLinkerChunkClear(linker, sizeof(txColorTable));
	result->instance = instance;
	result->size = 1;
	return result;
}

txConflictEntry* fxNewConflictEntry(txLinker* linker, txColorTable* colorTable)
{
	txConflictEntry* result = fxNewLinkerChunkClear(linker, sizeof(txConflictEntry));
	result->colorTable = colorTable;
	return result;
}

txConflictTable* fxNewConflictTable(txLinker* linker, txID id)
{
	txConflictTable* result = fxNewLinkerChunkClear(linker, sizeof(txConflictTable));
	result->id = id;
	return result;
}

void fxOptimize(txMachine* the)
{
	txLinker* linker = (txLinker*)(the->context);
	txInteger instanceCount = 0, instanceIndex;
	txID keyCount = the->keyIndex, keyIndex;
	txConflictTable** conflictTables = fxNewLinkerChunkClear(linker, keyCount * sizeof(txConflictTable*));
	txColorTable** colorTables = NULL;
	txFlag* flags = fxNewLinkerChunk(linker, keyCount * sizeof(txFlag));
	txSlot* heap = the->firstHeap;
	txLinkerProjection** projectionAddress = &linker->firstProjection;
	txLinkerProjection* projection;
	txInteger projectionIndex = 0, projectionCount, holes, color;
	while (heap) {
		txSlot* slot = heap + 1;
		txSlot* limit = heap->value.reference;
		projection = fxNewLinkerChunkClear(linker, sizeof(txLinkerProjection) + (mxPtrDiff(limit - slot) * sizeof(txInteger)));
		projection->heap = heap;
		projection->limit = limit;
		*projectionAddress = projection;
		projectionAddress = &projection->nextProjection;
		while (slot < limit) {
			if (slot->kind == XS_INSTANCE_KIND)
				instanceCount++;
			slot++;
		}
		heap = heap->next;
	}
	colorTables = fxNewLinkerChunkClear(linker, instanceCount * sizeof(txColorTable*));
	instanceIndex = 0;
	heap = the->firstHeap;
	while (heap) {
		txSlot* slot = heap + 1;
		txSlot* limit = heap->value.reference;
		while (slot < limit) {
			if (slot->kind == XS_INSTANCE_KIND) {
				txColorTable* colorTable = colorTables[instanceIndex] = fxNewColorTable(linker, slot);
				txSlot* property = slot->next;
				while (property) {
					if (((property->flag & XS_INTERNAL_FLAG) == 0) && (property->ID != XS_NO_ID)) {
						txColorEntry* colorEntry = fxNewColorEntry(linker, property);
						txConflictEntry* conflictEntry = fxNewConflictEntry(linker, colorTable);
						txID id = property->ID;
						txConflictTable* conflictTable = conflictTables[id];
						if (!conflictTable) {
							conflictTable = conflictTables[id] = fxNewConflictTable(linker, id);
						}
						fxLinkColorEntry(colorTable, colorEntry);
						fxLinkConflictEntry(conflictTable, conflictEntry);
						colorEntry->conflictTable = conflictTable;
					}
					property = property->next;
				}
				instanceIndex++;
			}
			slot++;
		}
		heap = heap->next;
	}
	// most conflicted key first
	c_qsort(conflictTables, keyCount, sizeof(txConflictTable*), fxCompareConflictTable);
	linker->colors = fxNewLinkerChunkClear(linker, keyCount * sizeof(txID));
	keyIndex = 0;
	color = 0;
	while (keyIndex < keyCount) {
		txConflictTable* conflictTable = conflictTables[keyIndex];
		if (conflictTable) {
			fxFindColor(conflictTable, flags, keyCount);
			linker->colors[conflictTable->id] = conflictTable->color;
			if (color < conflictTable->color)
				color = conflictTable->color;
// 			fprintf(stderr, "%s %d %d\n", fxGetKeyName(the, conflictTable->id), conflictTable->count, conflictTable->color);
		}
		else
			break;
		keyIndex++;
	}
	instanceIndex = 0;
	projectionIndex = 1;
	while (instanceIndex < instanceCount) {
		txColorTable* colorTable = colorTables[instanceIndex];
		txColorEntry* colorEntry = colorTable->first;
		while (colorEntry) {
			txConflictTable* conflictTable = colorEntry->conflictTable;
			if (colorTable->size <= conflictTable->color)
				colorTable->size = conflictTable->color + 1;
			colorEntry = colorEntry->next;
		}
		projectionIndex += colorTable->size;
		instanceIndex++;
	}
	// largest instance first
	c_qsort(colorTables, instanceCount, sizeof(txColorTable*), fxCompareColorTable);
	
	linker->layout = fxNewLinkerChunkClear(linker, projectionIndex * sizeof(txSlot*));
	linker->layout[0] = the->firstHeap;
	instanceIndex = 0;
	projectionIndex = 1;
	projectionCount = 1;
	while (instanceIndex < instanceCount) {
		txColorTable* colorTable = colorTables[instanceIndex];
		txColorEntry* colorEntry = colorTable->first;
		memset(flags, 0, colorTable->size);
		flags[0] = 1;
		while (colorEntry) {
			flags[colorEntry->conflictTable->color] = 1;
			colorEntry = colorEntry->next;
		}
		projectionIndex = fxFindHoles(linker->layout, flags, colorTable->size);

		colorEntry = colorTable->first;
		fxIndexSlot(linker, colorTable->instance, projectionIndex);
		while (colorEntry) {
			fxIndexSlot(linker, colorEntry->property, projectionIndex + colorEntry->conflictTable->color);
			colorEntry = colorEntry->next;
		}
		projectionIndex += colorTable->size;
		if (projectionCount < projectionIndex)
			projectionCount = projectionIndex;
		instanceIndex++;
	}
	linker->projectionIndex = projectionCount;

	projectionIndex = 1;
	holes = 0;
	while (projectionIndex < projectionCount) {
		if (!linker->layout[projectionIndex])
			holes++;
		projectionIndex++;
	}
	fprintf(stderr, "### %d instances, %d keys, %d colors, %d holes\n", instanceCount, keyCount, color, holes);
}
