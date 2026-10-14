/*
 * Copyright (c) 2016-2026  Moddable Tech, Inc.
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

#include <stdint.h>

#include "xsAll.h"
#include "xsmc.h"
#include "mc.xs.h"

#include "xsffi.h"
#include "moddableAppState.h"

#include "applib/app_logging.h"
#include "applib/moddable/moddable_private.h"
#include "logging/logging_private.h"
#include "kernel/memory_layout.h"
#include "kernel/pbl_malloc.h"
#include "process_state/app_state/app_state.h"
#include <pbl/kernel/syscall.h>
#include "syscall/syscall_internal.h"

// xs.h provides fxPop()/fxPush() macros that shadow the functions in xsffi.c;
// we need the function symbols for the gxAPI table entries.
#undef fxPop
#undef fxPush
extern txSlot *fxThis(txMachine *the);
extern txInteger fxArgc(txMachine *the);
extern txSlot *fxArgv(txMachine *the, txInteger index);
extern txSlot *fxResult(txMachine *the);
extern void fxPop(txMachine *the);
extern void fxPush(txMachine *the, txSlot *slot);
extern char **fxToStringHandle(txMachine *the, txSlot *slot);
extern void **fxToArrayBufferHandle(txMachine *the, txSlot *slot, size_t size);

#define FFI_MAX_BINDINGS 32

typedef struct {
	txSlot		*instance;
	txCallback	user_cb;
} FFIBinding;

static FFIBinding s_ffi_bindings[FFI_MAX_BINDINGS];
static int s_ffi_binding_count;

// The machine is in App RAM, where the app can rewrite it, and its XS stack
// may be in the kernel heap: check both against the kernel's record.
static void prv_check_machine(txMachine *the)
{
	if (!moddable_machine_is_current(the) || (the->stack < the->stackBottom) ||
	    (the->stackTop <= the->stack)) {
		PBL_LOG_ERR("XS API: invalid machine pointer %p", the);
		pbl_syscall_fail();
	}
}

typedef struct {
	txMachine	*the;
	txCallback	fn;
} FFIInvokeCtx;

static void prv_invoke_unprivileged(void *raw)
{
	FFIInvokeCtx *ctx = raw;
	(ctx->fn)(ctx->the);
}

static void prv_ffi_shim(txMachine *the)
{
	txSlot *instance = mxFunction->value.reference;
	txCallback user_cb = C_NULL;
	for (int i = 0; i < s_ffi_binding_count; i++) {
		if (s_ffi_bindings[i].instance == instance) {
			user_cb = s_ffi_bindings[i].user_cb;
			break;
		}
	}
	if (C_NULL == user_cb)
		xsUnknownError("FFI binding not resolved");

	FFIInvokeCtx ctx = { the, user_cb };
	pbl_user_call(prv_invoke_unprivileged, &ctx);
}

// Each trampoline elevates, validates its arguments against the per-task
// moddable state, calls the underlying fx*, and drops back. Slots must be on
// the XS stack (the FFI only hands out this, result and arguments), and C
// strings and buffers must be in app memory.

static void prv_check_slot(txMachine *the, txSlot *slot)
{
	if ((slot < the->stack) || (slot >= the->stackTop) ||
	    (((uintptr_t)slot - (uintptr_t)the->stack) % sizeof(txSlot))) {
		PBL_LOG_ERR("XS API: invalid slot pointer %p", slot);
		pbl_syscall_fail();
	}
}

static void prv_check_string(txString string)
{
	if (!memory_layout_is_cstring_in_region(memory_layout_get_app_region(), string, SIZE_MAX)) {
		PBL_LOG_ERR("XS API: invalid string pointer %p", string);
		pbl_syscall_fail();
	}
}

DEFINE_SYSCALL(txSlot *, sys_moddable_xs_this, txMachine *the) { prv_check_machine(the); return fxThis(the); }
DEFINE_SYSCALL(txInteger, sys_moddable_xs_argc, txMachine *the) { prv_check_machine(the); return fxArgc(the); }
DEFINE_SYSCALL(txSlot *, sys_moddable_xs_argv, txMachine *the, txInteger index) { prv_check_machine(the); return fxArgv(the, index); }
DEFINE_SYSCALL(void, sys_moddable_xs_pop, txMachine *the) { prv_check_machine(the); fxPop(the); }
DEFINE_SYSCALL(void, sys_moddable_xs_push, txMachine *the, txSlot *slot) { prv_check_machine(the); prv_check_slot(the, slot); fxPush(the, slot); }
DEFINE_SYSCALL(txSlot *, sys_moddable_xs_result, txMachine *the) { prv_check_machine(the); return fxResult(the); }
DEFINE_SYSCALL(void, sys_moddable_xs_abort, txMachine *the, int status) { prv_check_machine(the); fxAbort(the, status); }
DEFINE_SYSCALL(void, sys_moddable_xs_defineID, txMachine *the, txID id, txFlag flag, txFlag mask) { prv_check_machine(the); fxDefineID(the, id, flag, mask); }
DEFINE_SYSCALL(txID, sys_moddable_xs_id, txMachine *the, txString name) { prv_check_machine(the); prv_check_string(name); return fxID(the, name); }

DEFINE_SYSCALL(txSlot *, sys_moddable_xs_newHostFunction, txMachine *the, txCallback user_cb, txInteger length, txInteger name, txInteger profileID)
{
	prv_check_machine(the);
	if (s_ffi_binding_count >= FFI_MAX_BINDINGS)
		xsRangeError("too many FFI bindings");

	txSlot *instance = fxNewHostFunction(the, prv_ffi_shim, length, name, profileID);
	s_ffi_bindings[s_ffi_binding_count].instance = instance;
	s_ffi_bindings[s_ffi_binding_count].user_cb = user_cb;
	s_ffi_binding_count++;
	return instance;
}

DEFINE_SYSCALL(void, sys_moddable_xs_fromInteger, txMachine *the, txSlot *slot, txInteger value) { prv_check_machine(the); prv_check_slot(the, slot); fxInteger(the, slot, value); }
DEFINE_SYSCALL(txInteger, sys_moddable_xs_toInteger, txMachine *the, txSlot *slot) { prv_check_machine(the); prv_check_slot(the, slot); return fxToInteger(the, slot); }
DEFINE_SYSCALL(void, sys_moddable_xs_fromNumber, txMachine *the, txSlot *slot, txNumber value) { prv_check_machine(the); prv_check_slot(the, slot); fxNumber(the, slot, value); }
DEFINE_SYSCALL(txNumber, sys_moddable_xs_toNumber, txMachine *the, txSlot *slot) { prv_check_machine(the); prv_check_slot(the, slot); return fxToNumber(the, slot); }
DEFINE_SYSCALL(void, sys_moddable_xs_fromUnsigned, txMachine *the, txSlot *slot, txUnsigned value) { prv_check_machine(the); prv_check_slot(the, slot); fxUnsigned(the, slot, value); }
DEFINE_SYSCALL(txUnsigned, sys_moddable_xs_toUnsigned, txMachine *the, txSlot *slot) { prv_check_machine(the); prv_check_slot(the, slot); return fxToUnsigned(the, slot); }
DEFINE_SYSCALL(void, sys_moddable_xs_fromBigInt64, txMachine *the, txSlot *slot, int64_t value) { prv_check_machine(the); prv_check_slot(the, slot); fxFromBigInt64(the, slot, value); }
DEFINE_SYSCALL(int64_t, sys_moddable_xs_toBigInt64, txMachine *the, txSlot *slot) { prv_check_machine(the); prv_check_slot(the, slot); return fxToBigInt64(the, slot); }
DEFINE_SYSCALL(void, sys_moddable_xs_fromBigUint64, txMachine *the, txSlot *slot, uint64_t value) { prv_check_machine(the); prv_check_slot(the, slot); fxFromBigUint64(the, slot, value); }
DEFINE_SYSCALL(uint64_t, sys_moddable_xs_toBigUint64, txMachine *the, txSlot *slot) { prv_check_machine(the); prv_check_slot(the, slot); return fxToBigUint64(the, slot); }

// Strings and ArrayBuffers. Their XS-side backing lives in app RAM
// (unprivileged-readable), so the to*Handle entries hand the generated
// mc.ffi.c direct pointers into XS storage — no copy-out ABI. fromStringX
// copies like fromString: the app could rewrite a string it lent to XS.
DEFINE_SYSCALL(void, sys_moddable_xs_fromString, txMachine *the, txSlot *slot, char *value) { prv_check_machine(the); prv_check_slot(the, slot); prv_check_string(value); fxString(the, slot, value); }
DEFINE_SYSCALL(void, sys_moddable_xs_fromStringX, txMachine *the, txSlot *slot, char *value) { prv_check_machine(the); prv_check_slot(the, slot); prv_check_string(value); fxString(the, slot, value); }
DEFINE_SYSCALL(char **, sys_moddable_xs_toStringHandle, txMachine *the, txSlot *slot) { prv_check_machine(the); prv_check_slot(the, slot); return fxToStringHandle(the, slot); }
DEFINE_SYSCALL(void *, sys_moddable_xs_fromArrayBuffer, txMachine *the, txSlot *slot, void *data, txInteger byteLength, txInteger maxByteLength)
{
	prv_check_machine(the);
	prv_check_slot(the, slot);
	if (data && (byteLength > 0))
		pbl_syscall_verify_user(data, byteLength);
	return fxArrayBuffer(the, slot, data, byteLength, maxByteLength);
}
DEFINE_SYSCALL(void **, sys_moddable_xs_toArrayBufferHandle, txMachine *the, txSlot *slot, size_t size) { prv_check_machine(the); prv_check_slot(the, slot); return fxToArrayBufferHandle(the, slot, size); }

void FFI_constructor(xsMachine* the)
{
	txBuildFFI fxBuildFFI = getModdableAppState(fxBuildFFI);
	if (!fxBuildFFI) {
		xsResult = xsUndefined;
		return;
	}

	// The user's mc.ffi.c stores this pointer as its XS global and
	// dereferences XS->* from unprivileged thread mode (the xs_* host
	// functions run via prv_ffi_shim → pbl_user_call). Allocate the
	// table in task-scoped heap (unprivileged-readable); it's reclaimed when
	// the task exits.
	txAPI *api = task_zalloc(sizeof(txAPI));
	if (!api) {
		xsResult = xsUndefined;
		return;
	}
	api->_this = sys_moddable_xs_this;
	api->argc = sys_moddable_xs_argc;
	api->argv = sys_moddable_xs_argv;
	api->pop = sys_moddable_xs_pop;
	api->push = sys_moddable_xs_push;
	api->result = sys_moddable_xs_result;
	api->abort = sys_moddable_xs_abort;
	api->defineID = sys_moddable_xs_defineID;
	api->id = sys_moddable_xs_id;
	api->newHostFunction = sys_moddable_xs_newHostFunction;
	api->fromBigInt64 = sys_moddable_xs_fromBigInt64;
	api->fromBigUint64 = sys_moddable_xs_fromBigUint64;
	api->fromInteger = sys_moddable_xs_fromInteger;
	api->fromNumber = sys_moddable_xs_fromNumber;
	api->fromUnsigned = sys_moddable_xs_fromUnsigned;
	api->toBigInt64 = sys_moddable_xs_toBigInt64;
	api->toBigUint64 = sys_moddable_xs_toBigUint64;
	api->toInteger = sys_moddable_xs_toInteger;
	api->toNumber = sys_moddable_xs_toNumber;
	api->toUnsigned = sys_moddable_xs_toUnsigned;
	api->fromString = sys_moddable_xs_fromString;
	api->fromStringX = sys_moddable_xs_fromStringX;
	api->toStringHandle = sys_moddable_xs_toStringHandle;
	api->fromArrayBuffer = sys_moddable_xs_fromArrayBuffer;
	api->toArrayBufferHandle = sys_moddable_xs_toArrayBufferHandle;

	s_ffi_binding_count = 0;
	(fxBuildFFI)(the, api);
}

void FFI_destructor(void* /* it */)
{
}
