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

#include "xsAll.h"
#include "xsmc.h"
#include "mc.xs.h"

#include "xsffi.h"
#include "moddableAppState.h"

#include "applib/app_logging.h"
#include "logging/logging_private.h"
#include "kernel/pbl_malloc.h"
#include "mcu/privilege.h"
#include "process_state/app_state/app_state.h"
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

static void prv_check_machine(txMachine *the)
{
	ModdablePebbleAppState state = (ModdablePebbleAppState)app_state_get_js_memory_api_context();
	if (!state || state->the != the) {
		PBL_LOG_ERR("XS API: invalid machine pointer %p", the);
		syscall_failed();
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
	mcu_call_unprivileged(prv_invoke_unprivileged, &ctx);
}

// Each trampoline elevates, validates the txMachine pointer against per-task
// moddable state, calls the underlying fx*, and drops back. Slot-pointer
// bounds checking is TODO; a bogus slot from a malicious app faults the
// kernel-side fx* rather than escaping.

DEFINE_SYSCALL(txSlot *, sys_moddable_xs_this, txMachine *the) { prv_check_machine(the); return fxThis(the); }
DEFINE_SYSCALL(txInteger, sys_moddable_xs_argc, txMachine *the) { prv_check_machine(the); return fxArgc(the); }
DEFINE_SYSCALL(txSlot *, sys_moddable_xs_argv, txMachine *the, txInteger index) { prv_check_machine(the); return fxArgv(the, index); }
DEFINE_SYSCALL(void, sys_moddable_xs_pop, txMachine *the) { prv_check_machine(the); fxPop(the); }
DEFINE_SYSCALL(void, sys_moddable_xs_push, txMachine *the, txSlot *slot) { prv_check_machine(the); fxPush(the, slot); }
DEFINE_SYSCALL(txSlot *, sys_moddable_xs_result, txMachine *the) { prv_check_machine(the); return fxResult(the); }
DEFINE_SYSCALL(void, sys_moddable_xs_abort, txMachine *the, int status) { prv_check_machine(the); fxAbort(the, status); }
DEFINE_SYSCALL(void, sys_moddable_xs_defineID, txMachine *the, txID id, txFlag flag, txFlag mask) { prv_check_machine(the); fxDefineID(the, id, flag, mask); }
DEFINE_SYSCALL(txID, sys_moddable_xs_id, txMachine *the, txString name) { prv_check_machine(the); return fxID(the, name); }

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

DEFINE_SYSCALL(void, sys_moddable_xs_fromInteger, txMachine *the, txSlot *slot, txInteger value) { prv_check_machine(the); fxInteger(the, slot, value); }
DEFINE_SYSCALL(txInteger, sys_moddable_xs_toInteger, txMachine *the, txSlot *slot) { prv_check_machine(the); return fxToInteger(the, slot); }
DEFINE_SYSCALL(void, sys_moddable_xs_fromNumber, txMachine *the, txSlot *slot, txNumber value) { prv_check_machine(the); fxNumber(the, slot, value); }
DEFINE_SYSCALL(txNumber, sys_moddable_xs_toNumber, txMachine *the, txSlot *slot) { prv_check_machine(the); return fxToNumber(the, slot); }
DEFINE_SYSCALL(void, sys_moddable_xs_fromUnsigned, txMachine *the, txSlot *slot, txUnsigned value) { prv_check_machine(the); fxUnsigned(the, slot, value); }
DEFINE_SYSCALL(txUnsigned, sys_moddable_xs_toUnsigned, txMachine *the, txSlot *slot) { prv_check_machine(the); return fxToUnsigned(the, slot); }
DEFINE_SYSCALL(void, sys_moddable_xs_fromBigInt64, txMachine *the, txSlot *slot, int64_t value) { prv_check_machine(the); fxFromBigInt64(the, slot, value); }
DEFINE_SYSCALL(int64_t, sys_moddable_xs_toBigInt64, txMachine *the, txSlot *slot) { prv_check_machine(the); return fxToBigInt64(the, slot); }
DEFINE_SYSCALL(void, sys_moddable_xs_fromBigUint64, txMachine *the, txSlot *slot, uint64_t value) { prv_check_machine(the); fxFromBigUint64(the, slot, value); }
DEFINE_SYSCALL(uint64_t, sys_moddable_xs_toBigUint64, txMachine *the, txSlot *slot) { prv_check_machine(the); return fxToBigUint64(the, slot); }

// Strings and ArrayBuffers. Their XS-side backing now lives in app RAM
// (unprivileged-readable), so the from*/to*Handle entries hand the generated
// mc.ffi.c direct pointers into XS storage — no copy-out ABI. Slot-pointer
// bounds checking is still the shared TODO noted above.
DEFINE_SYSCALL(void, sys_moddable_xs_fromString, txMachine *the, txSlot *slot, char *value) { prv_check_machine(the); fxString(the, slot, value); }
DEFINE_SYSCALL(void, sys_moddable_xs_fromStringX, txMachine *the, txSlot *slot, char *value) { prv_check_machine(the); fxStringX(the, slot, value); }
DEFINE_SYSCALL(char **, sys_moddable_xs_toStringHandle, txMachine *the, txSlot *slot) { prv_check_machine(the); return fxToStringHandle(the, slot); }
DEFINE_SYSCALL(void *, sys_moddable_xs_fromArrayBuffer, txMachine *the, txSlot *slot, void *data, txInteger byteLength, txInteger maxByteLength) { prv_check_machine(the); return fxArrayBuffer(the, slot, data, byteLength, maxByteLength); }
DEFINE_SYSCALL(void **, sys_moddable_xs_toArrayBufferHandle, txMachine *the, txSlot *slot, size_t size) { prv_check_machine(the); return fxToArrayBufferHandle(the, slot, size); }

void FFI_constructor(xsMachine* the)
{
	txBuildFFI fxBuildFFI = getModdableAppState(fxBuildFFI);
	if (!fxBuildFFI) {
		xsResult = xsUndefined;
		return;
	}

	// The user's mc.ffi.c stores this pointer as its XS global and
	// dereferences XS->* from unprivileged thread mode (the xs_* host
	// functions run via prv_ffi_shim → mcu_call_unprivileged). Allocate the
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
