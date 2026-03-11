/*
 * Copyright (c) 2025-2026  Moddable Tech, Inc.
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

#include "applib/applib_resource.h"
#include "applib/applib_resource_private.h"
#include "syscall/syscall.h"
#include "resource/resource_storage.h"

#include "xsAll.h"
#include "xs.h"
#include "mc.xs.h"
#include "mc.defines.h"

#undef TARGET_QEMU

typedef struct ArchivePebbleResourceStruct ArchivePebbleResourceRecord, *ArchivePebbleResource;
struct ArchivePebbleResourceStruct {
	xsSlot* archive;
	void* address;
	size_t size;
};

#if TARGET_QEMU
	// use armv7m.ccm
	#define kReservedMemory ((uint8_t *)0x10000000)
	#define kReservedMemorySize (65536)
#endif

static void ArchivePebbleResourceMark(xsMachine* the, void* it, xsMarkRoot markRoot);

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

static const xsHostHooks ArchivePebbleResourceHooks = {
	ArchivePebbleResourceDelete,
	ArchivePebbleResourceMark,
	NULL
};

void ArchivePebbleResourceCreate(xsMachine* the) 
{
	void* preparation = xsPreparation();
	ArchivePebbleResourceRecord record;
	ArchivePebbleResource self = &record;
	c_memset(self, 0, sizeof(record));

	xsTry {
		uint32_t resource_id = 0;
		ResAppNum app_num = sys_get_current_resource_num();
		for (uint32_t id = resource_storage_get_num_entries(app_num, 1); id >= 1; id--) {
			uint8_t header[8];
			const size_t read = resource_load_byte_range_system(app_num, id, 0, header, sizeof(header));
			if (read) {
				if (('X' == header[4]) && ('S' == header[5]) && ('_' == header[6]) && ('A' == header[7])) {
					resource_id = id;
					break;
				}
			}
		}
		if (!resource_id)
			xsUnknownError("no mod");

		self->size = sys_resource_size(app_num, resource_id);

#if TARGET_QEMU
		if (self->size > kReservedMemorySize)
			xsUnknownError("mod too big");

		self->address = kReservedMemory;
		uint8_t *dst = self->address;
		for (size_t offset = 0; offset < self->size; ) {
			size_t use = 4096;
			if (use > (self->size - offset))
				use = self->size - offset;
			void *address = applib_resource_mmap_or_load(app_num, resource_id, offset, use, false);
			c_memcpy(dst, address, use);
			applib_resource_munmap_or_free(address);
			dst += use;
			offset += use;
		}
#else
		self->address = applib_resource_mmap_or_load(app_num, resource_id, 0, self->size, false);

		if (C_NULL == self->address)
			xsUnknownError("load resource failed");
#endif

		if (C_NULL == fxMapArchive(the, preparation, self->address, 256, fxArchiveRead, fxArchiveWrite)) {
			self->address = NULL;
			xsUnknownError("fxMapArchive failed");
		}
		fxSetArchive(the, self->address);

		mxPushReference(mxFunctionInstanceHome(mxFunction->value.reference)->value.home.module);
		mxGetID(xsID_Archive);
		mxGetID(xsID_prototype);
		mxPullSlot(mxResult);	
			
		xsResult = xsNewHostInstance(xsResult);
		xsSetHostBuffer(xsResult, self->address, self->size);
		xsSet(xsResult, xsID_fileMapping, xsThis);
		self->archive = xsToReference(xsResult);
		xsSetHostChunk(xsThis, self, sizeof(record));
		xsSetHostHooks(xsThis, &ArchivePebbleResourceHooks);
		xsResult = xsThis;
	}
	xsCatch {
		ArchivePebbleResourceDelete(self);
		xsThrow(xsException);
	}
}

void ArchivePebbleResourceDelete(void* it)
{
	if (it) {
		ArchivePebbleResource self = it;
		self->size = 0;
#if !TARGET_QEMU
		if (self->address) {
			applib_resource_munmap_or_free(self->address);
			self->address = NULL;
		}
#endif
	}
}

void ArchivePebbleResourceMark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	ArchivePebbleResource self = it;
	if (self->archive) (*markRoot)(the, self->archive);
}

void ArchivePebbleResource_close(xsMachine *the)
{
	ArchivePebbleResource* self = xsGetHostHandle(xsThis);
	(*self)->archive = NULL;
	ArchivePebbleResourceDelete(*self);
}

void ArchivePebbleResource_get_archive(xsMachine *the)
{
	ArchivePebbleResource* self = xsGetHostHandle(xsThis);
	xsResult = xsReference((*self)->archive);
}
