#include "xsAll.h"
#include "xs.h"
#include "mc.xs.h"
#include "mc.defines.h"

extern txInteger _xsmcGetBuffer(txMachine *the, txSlot *slot, void **data, txUnsigned *count, txBoolean writable);

typedef struct ModArchiveStruct ModArchiveRecord, *ModArchive;
struct ModArchiveStruct {
	xsSlot* reference;
	xsSlot* buffer;
	void* archive;
};

static void ModArchiveMark(xsMachine* the, void* it, xsMarkRoot markRoot);

static xsHostHooks ModArchiveHooks = {
	ModArchiveDelete,
	ModArchiveMark,
	NULL
};

static xsBooleanValue fxArchiveRead(void* src, size_t offset, void* buffer, size_t size)
{
	c_memcpy(buffer, ((txU1*)src) + offset, size);
	return 1;
}

static xsBooleanValue fxArchiveWrite(void* dst, size_t offset, void* buffer, size_t size)
{
	c_memcpy(((txU1*)dst) + offset, buffer, size);
	return 1;
}

void ModArchiveCreate(xsMachine* the) 
{
	ModArchiveRecord record;
	ModArchive self = &record;
	void* preparation = xsPreparation();
	void* data;
	txUnsigned size;
	
	c_memset(self, 0, sizeof(record));
	self->reference = xsToReference(xsThis);
	self->buffer = xsToReference(xsArg(0));

	_xsmcGetBuffer(the, &xsArg(0), &data, &size, 1);
    self->archive = fxMapArchive(preparation, data, data, size, fxArchiveRead, fxArchiveWrite);
	
	xsSetHostChunk(xsThis, self, sizeof(record));
	xsSetHostHooks(xsThis, &ModArchiveHooks);
	xsResult = xsThis;
}

void ModArchiveDelete(void* it)
{
}

void ModArchiveMark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	ModArchive self = it;
	if (self->buffer) (*markRoot)(the, self->buffer);
}

#define mxArchiveHeaderSize (sizeof(Atom) + sizeof(Atom) + XS_VERSION_SIZE + sizeof(Atom) + XS_DIGEST_SIZE + sizeof(Atom) + XS_DIGEST_SIZE)

void ModArchive_get_buffer(xsMachine *the)
{
	ModArchive* self = xsGetHostHandle(xsThis);
	xsResult = xsReference((*self)->buffer);
}

void ModArchive_get_modulePaths(xsMachine *the)
{
	ModArchive* self = xsGetHostHandle(xsThis);
	uint8_t *p = (*self)->archive;
	xsResult = xsNewArray(0);
	if (p) {
		uint8_t *q;
		int i = 0;
		p += mxArchiveHeaderSize;
		// NAME
		p += c_read32be(p);
		// SYMB
		p += c_read32be(p);
		// MODS
		q = p + c_read32be(p);
		p += sizeof(Atom);
		while (p < q) {
			int atomSize = c_read32be(p);
			xsSetIndex(xsResult, i++, xsString((txString)(p + sizeof(Atom))));
			p += atomSize;
			p += c_read32be(p);
		}
	}
}

void ModArchive_get_resourcePaths(xsMachine *the)
{
	ModArchive* self = xsGetHostHandle(xsThis);
	uint8_t *p = (*self)->archive;
	xsResult = xsNewArray(0);
	if (p) {
		uint8_t *q;
		int i = 0;
		p += mxArchiveHeaderSize;
		// NAME
		p += c_read32be(p);
		// SYMB
		p += c_read32be(p);
		// MODS
		p += c_read32be(p);
		// RSRC
		q = p + c_read32be(p);
		p += sizeof(Atom);
		while (p < q) {
			int atomSize = c_read32be(p);
			xsSetIndex(xsResult, i++, xsString((txString)(p + sizeof(Atom))));
			p += atomSize;
			p += c_read32be(p);
		}
	}
}
