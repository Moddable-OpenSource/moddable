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
#include "kernel/pebble_tasks.h"
#include "kernel/pbl_malloc.h"
#include "process_state/app_state/app_state.h"
#include <pbl/kernel/syscall.h>

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
	if (pebble_task_get_current() != PebbleTask_App)
		pbl_syscall_fail();

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
	mxTry(the) {
		moddable_call_unprivileged(prv_invoke_unprivileged, &ctx);
	}
	mxCatch(the) {
		// An XS API call made by the native function threw past it.
		pbl_user_call_unwind();
		pebble_task_set_app_ram_executable(false);
		fxJump(the);
	}
}

// Each trampoline elevates, validates its arguments against the per-task
// moddable state, calls the underlying fx*, and drops back. Slots must be on
// the XS stack (the FFI only hands out this, result and arguments), and C
// strings and buffers must be in app memory.
//
// The machine runs privileged with App RAM non-executable, so that it faults
// instead of running app code. Keep it that way while these run XS code, and
// restore it on the way back to the native caller.
static void prv_enter(txMachine *the)
{
	prv_check_machine(the);
	pebble_task_set_app_ram_executable(false);
}

static void prv_leave(void)
{
	pebble_task_set_app_ram_executable(true);
}

static void prv_check_slot(txMachine *the, txSlot *slot)
{
	if ((slot < the->stack) || (slot >= the->stackTop) ||
	    (((uintptr_t)slot - (uintptr_t)the->stack) % sizeof(txSlot))) {
		PBL_LOG_ERR("XS API: invalid slot pointer %p", slot);
		pbl_syscall_fail();
	}
}

PBL_SYSCALL(txSlot *, sys_moddable_xs_this, (VAL, txMachine *, the))
{
	prv_enter(the);
	txSlot *result = fxThis(the);
	prv_leave();
	return result;
}

PBL_SYSCALL(txInteger, sys_moddable_xs_argc, (VAL, txMachine *, the))
{
	prv_enter(the);
	txInteger result = fxArgc(the);
	prv_leave();
	return result;
}

PBL_SYSCALL(txSlot *, sys_moddable_xs_argv, (VAL, txMachine *, the), (VAL, txInteger, index))
{
	prv_enter(the);
	txSlot *result = fxArgv(the, index);
	prv_leave();
	return result;
}

PBL_SYSCALL(void, sys_moddable_xs_pop, (VAL, txMachine *, the))
{
	prv_enter(the);
	fxPop(the);
	prv_leave();
}

PBL_SYSCALL(void, sys_moddable_xs_push, (VAL, txMachine *, the), (VAL, txSlot *, slot))
{
	prv_enter(the);
	prv_check_slot(the, slot);
	fxPush(the, slot);
	prv_leave();
}

PBL_SYSCALL(txSlot *, sys_moddable_xs_result, (VAL, txMachine *, the))
{
	prv_enter(the);
	txSlot *result = fxResult(the);
	prv_leave();
	return result;
}

PBL_SYSCALL(void, sys_moddable_xs_abort, (VAL, txMachine *, the), (VAL, int, status))
{
	prv_enter(the);
	fxAbort(the, status);
	prv_leave();
}

PBL_SYSCALL(void, sys_moddable_xs_defineID, (VAL, txMachine *, the), (VAL, txID, id), (VAL, txFlag, flag), (VAL, txFlag, mask))
{
	prv_enter(the);
	fxDefineID(the, id, flag, mask);
	prv_leave();
}

PBL_SYSCALL(txID, sys_moddable_xs_id, (VAL, txMachine *, the), (STR, txString, name))
{
	prv_enter(the);
	txID result = fxID(the, name);
	prv_leave();
	return result;
}

PBL_SYSCALL(txSlot *, sys_moddable_xs_newHostFunction, (VAL, txMachine *, the), (VAL, txCallback, user_cb), (VAL, txInteger, length), (VAL, txInteger, name), (VAL, txInteger, profileID))
{
	prv_enter(the);
	if (s_ffi_binding_count >= FFI_MAX_BINDINGS)
		xsRangeError("too many FFI bindings");

	txSlot *instance = fxNewHostFunction(the, prv_ffi_shim, length, name, profileID);
	s_ffi_bindings[s_ffi_binding_count].instance = instance;
	s_ffi_bindings[s_ffi_binding_count].user_cb = user_cb;
	s_ffi_binding_count++;
	prv_leave();
	return instance;
}

PBL_SYSCALL(void, sys_moddable_xs_fromInteger, (VAL, txMachine *, the), (VAL, txSlot *, slot), (VAL, txInteger, value))
{
	prv_enter(the);
	prv_check_slot(the, slot);
	fxInteger(the, slot, value);
	prv_leave();
}

PBL_SYSCALL(txInteger, sys_moddable_xs_toInteger, (VAL, txMachine *, the), (VAL, txSlot *, slot))
{
	prv_enter(the);
	prv_check_slot(the, slot);
	txInteger result = fxToInteger(the, slot);
	prv_leave();
	return result;
}

PBL_SYSCALL(void, sys_moddable_xs_fromNumber, (VAL, txMachine *, the), (VAL, txSlot *, slot), (VAL, txNumber, value))
{
	prv_enter(the);
	prv_check_slot(the, slot);
	fxNumber(the, slot, value);
	prv_leave();
}

PBL_SYSCALL(txNumber, sys_moddable_xs_toNumber, (VAL, txMachine *, the), (VAL, txSlot *, slot))
{
	prv_enter(the);
	prv_check_slot(the, slot);
	txNumber result = fxToNumber(the, slot);
	prv_leave();
	return result;
}

PBL_SYSCALL(void, sys_moddable_xs_fromUnsigned, (VAL, txMachine *, the), (VAL, txSlot *, slot), (VAL, txUnsigned, value))
{
	prv_enter(the);
	prv_check_slot(the, slot);
	fxUnsigned(the, slot, value);
	prv_leave();
}

PBL_SYSCALL(txUnsigned, sys_moddable_xs_toUnsigned, (VAL, txMachine *, the), (VAL, txSlot *, slot))
{
	prv_enter(the);
	prv_check_slot(the, slot);
	txUnsigned result = fxToUnsigned(the, slot);
	prv_leave();
	return result;
}

PBL_SYSCALL(void, sys_moddable_xs_fromBigInt64, (VAL, txMachine *, the), (VAL, txSlot *, slot), (VAL, int64_t, value))
{
	prv_enter(the);
	prv_check_slot(the, slot);
	fxFromBigInt64(the, slot, value);
	prv_leave();
}

PBL_SYSCALL(int64_t, sys_moddable_xs_toBigInt64, (VAL, txMachine *, the), (VAL, txSlot *, slot))
{
	prv_enter(the);
	prv_check_slot(the, slot);
	int64_t result = fxToBigInt64(the, slot);
	prv_leave();
	return result;
}

PBL_SYSCALL(void, sys_moddable_xs_fromBigUint64, (VAL, txMachine *, the), (VAL, txSlot *, slot), (VAL, uint64_t, value))
{
	prv_enter(the);
	prv_check_slot(the, slot);
	fxFromBigUint64(the, slot, value);
	prv_leave();
}

PBL_SYSCALL(uint64_t, sys_moddable_xs_toBigUint64, (VAL, txMachine *, the), (VAL, txSlot *, slot))
{
	prv_enter(the);
	prv_check_slot(the, slot);
	uint64_t result = fxToBigUint64(the, slot);
	prv_leave();
	return result;
}

// Strings and ArrayBuffers. Their XS-side backing lives in app RAM
// (unprivileged-readable), so the to*Handle entries hand the generated
// mc.ffi.c direct pointers into XS storage — no copy-out ABI. fromStringX
// copies like fromString: the app could rewrite a string it lent to XS.
PBL_SYSCALL(void, sys_moddable_xs_fromString, (VAL, txMachine *, the), (VAL, txSlot *, slot), (STR, char *, value))
{
	prv_enter(the);
	prv_check_slot(the, slot);
	fxString(the, slot, value);
	prv_leave();
}

PBL_SYSCALL(void, sys_moddable_xs_fromStringX, (VAL, txMachine *, the), (VAL, txSlot *, slot), (STR, char *, value))
{
	prv_enter(the);
	prv_check_slot(the, slot);
	fxString(the, slot, value);
	prv_leave();
}

PBL_SYSCALL(char **, sys_moddable_xs_toStringHandle, (VAL, txMachine *, the), (VAL, txSlot *, slot))
{
	prv_enter(the);
	prv_check_slot(the, slot);
	char **result = fxToStringHandle(the, slot);
	prv_leave();
	return result;
}

PBL_SYSCALL(void *, sys_moddable_xs_fromArrayBuffer, (VAL, txMachine *, the), (VAL, txSlot *, slot), (VAL, void *, data), (VAL, txInteger, byteLength), (VAL, txInteger, maxByteLength))
{
	prv_enter(the);
	prv_check_slot(the, slot);
	if (data && (byteLength > 0))
		pbl_syscall_verify_user(data, byteLength);
	void *result = fxArrayBuffer(the, slot, data, byteLength, maxByteLength);
	prv_leave();
	return result;
}

PBL_SYSCALL(void **, sys_moddable_xs_toArrayBufferHandle, (VAL, txMachine *, the), (VAL, txSlot *, slot), (VAL, size_t, size))
{
	prv_enter(the);
	prv_check_slot(the, slot);
	void **result = fxToArrayBufferHandle(the, slot, size);
	prv_leave();
	return result;
}

void FFI_constructor(xsMachine* the)
{
	if (!getModdableAppState(fxBuildFFI)) {
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
	mxTry(the) {
		moddable_build_ffi(the, api);
	}
	mxCatch(the) {
		pbl_user_call_unwind();
		pebble_task_set_app_ram_executable(false);
		fxJump(the);
	}
}

void FFI_destructor(void* /* it */)
{
}
