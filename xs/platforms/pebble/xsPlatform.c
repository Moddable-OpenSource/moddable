/*
 * Copyright (c) 2026  Moddable Tech, Inc.
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

#include "xsAll.h"
#include "stdio.h"

#include "xsPlatform.h"

#include "mc.defines.h"
#include "moddableAppState.h"
#include "applib/moddable/moddable.h"

#include "pbl/services/comm_session/protocol.h"
#include "pbl/services/comm_session/session.h"
#include "pbl/services/comm_session/session_receive_router.h"
#include "pbl/services/comm_session/session_send_buffer.h"
#include "pbl/services/comm_session/session_send_queue.h"
#include "drivers/task_watchdog.h"
#include "drivers/watchdog.h"

#include "xs.h"
#include "xsHosts.h"
#include "modTimer.h"
#include "FreeRTOS.h"
#include "light_mutex.h"
#include "semphr.h"

#include "applib/app_logging.h"
#include "system/logging.h"

LightMutexHandle_t gDebugMutex;
#define mxDebugMutexTake() xLightMutexLock(gDebugMutex, portMAX_DELAY)
#define mxDebugMutexGive() xLightMutexUnlock(gDebugMutex)
#define mxDebugMutexAllocated() (NULL != gDebugMutex)

extern void modMachineTaskInit(txMachine *the);
extern void modMachineTaskUninit(txMachine *the);

static void doDebugCommand(modTimer timer, void *refcon, int refconSize);

#define XSBUG_CTRL_ENDPOINT (51967)

#include "system/reboot_reason.h"

void fxCreateMachinePlatform(txMachine* the)
{
	modMachineTaskInit(the);
#ifdef mxDebug
	if (!gDebugMutex)
		gDebugMutex = xLightMutexCreate();
	the->debugNotifyTimer = modTimerAdd(100, 100, doDebugCommand, &the, sizeof(the));
	the->state = app_state_get_js_memory_api_context();
#endif
}

void fxDeleteMachinePlatform(txMachine* the)
{
#ifdef mxDebug
	 if (the->debugNotifyTimer)
	 	modTimerRemove(the->debugNotifyTimer);
#endif

#ifdef mxInstrument
	modInstrumentMachineEnd(the);
#endif

	modMachineTaskUninit(the);
}

void fxAbort(txMachine* the, int status)
{
	char *msg = (char*)fxAbortString(status);
	char *reason = "";
#ifdef mxDebug
	if (XS_DEBUGGER_EXIT == status)
		return;
#endif
	if (XS_UNHANDLED_EXCEPTION_EXIT == status) {
		xsSlot errorStack = xsGet(xsException, mxID(_stack));
		xsStringValue stackStr = xsToString(errorStack);
		APP_LOG(APP_LOG_LEVEL_ERROR, "%s", stackStr);

		mxPush(mxException);
		mxGetID(mxID(_message));
		if ((the->stack->kind == XS_STRING_KIND) || (the->stack->kind == XS_STRING_X_KIND))
			reason = the->stack->value.string;
	}

	APP_LOG(APP_LOG_LEVEL_ERROR, "fxAbort %s: %s", msg, reason);

	extern void moddable_cleanup(void);
	moddable_cleanup();

	c_exit(status);
}

#ifdef mxDebug

void fxConnect(txMachine* the)
{
	if (getModdableAppState(creationFlags) & kModdableCreationFlagDebug)
		the->connected = C_NULL != comm_session_get_system_session();
}

void fxDisconnect(txMachine* the)
{
	the->echoBuffer[0] = 0;
	the->echoOffset = 1;
	fxSend(the, 0);

	the->connected = false;
	if (the->debugNotifyTimer) {
		modTimerRemove(the->debugNotifyTimer);
		the->debugNotifyTimer = C_NULL;
	}
}

txBoolean fxIsConnected(txMachine* the)
{
	return the->connected;
}

txBoolean fxIsReadable(txMachine* the)
{
	return ((ModdablePebbleAppState)((ModdablePebbleAppState)the->state))->debugFragments ? 1 : 0;
}

void fxReceive(txMachine* the)
{
	ModdablePebbleAppState state = the->state;
	if (!the->debugConnectionVerified) {
		uint32_t start = modMilliseconds();

		while (!the->debugOffset && !state->debugFragments) {
			if (((int)(modMilliseconds() - start)) >= 2000) {
				fxDisconnect(the);
				break;
			}
			vTaskDelay(10);
		}
		the->debugConnectionVerified = 1;
	}

	DebugFragment f = state->debugFragments;
	if (C_NULL == f) {
		task_watchdog_pause(5);
		vTaskDelay(10);					// 10 ms
		return;
	}

	int space = sizeof(the->debugBuffer) - the->debugOffset;
	if (f->remaining <= space) {
		c_memmove(the->debugBuffer + the->debugOffset, f->bytes + f->offset, f->remaining);
		the->debugOffset += f->remaining;
		mxDebugMutexTake();
		state->debugFragments = f->next;
		kernel_free(f);
		mxDebugMutexGive();
	}
	else {
		c_memmove(the->debugBuffer + the->debugOffset, f->bytes + f->offset, space);
		the->debugOffset += space;
		f->offset += space;
		f->remaining -= space;
	}
}

void doDebugCommand(modTimer timer, void *refcon, int refconSize)
{
	txMachine* the = *(txMachine **)refcon;

	if (!((ModdablePebbleAppState)the->state)->debugFragments)
		return;

	fxDebugCommand(the);
	if (the->breakOnStartFlag) {
		fxBeginHost(the);
		fxDebugger(the, (char *)__FILE__, __LINE__);
		fxEndHost(the);
	}
}

void xsbug_protocol_msg_callback(CommSession *session, const uint8_t* msg, size_t length)
{
	ModdablePebbleAppState state = (ModdablePebbleAppState)app_state_get_js_memory_api_context();
	if (!state || !mxDebugMutexAllocated())
		return;

	mxDebugMutexTake();

	DebugFragment fragment = kernel_malloc(sizeof(DebugFragmentRecord) + length);
	if (C_NULL == fragment) {
		if (state->the)
			fxDisconnect(state->the);
		mxDebugMutexGive();
		return;
	}

	fragment->next = C_NULL;
	fragment->remaining = length;
	fragment->offset = 0;
	c_memmove(fragment->bytes, msg, length);

	if (NULL == state->debugFragments)
		state->debugFragments = fragment;
	else {
		DebugFragment walker = state->debugFragments;
		while (walker->next)
			walker = walker->next;
		walker->next = fragment;
	}

	mxDebugMutexGive();
}

void fxSend(txMachine* the, txBoolean flags)
{
	CommSession *session = comm_session_get_system_session();
	if (!session)
		return;

	int remaining = the->echoOffset;
	uint8_t *src = (uint8_t *)the->echoBuffer;
	do {
		if (!the->send_buffer) {
			the->send_buffer_remain = comm_session_send_buffer_get_max_payload_length(session) - 128;
			the->send_buffer = comm_session_send_buffer_begin_write(session, XSBUG_CTRL_ENDPOINT, the->send_buffer_remain, COMM_SESSION_DEFAULT_TIMEOUT);
			if (!the->send_buffer) {
				fxDisconnect(the);
				return;
			}
		}

		int use = (remaining > the->send_buffer_remain) ? the->send_buffer_remain : remaining;
		if (false == comm_session_send_buffer_write(the->send_buffer, src, use)) 
			the->send_buffer_remain = 0;
		else  {
			the->send_buffer_remain -= use;
			remaining -= use;
			src += use;
		}

		if (0 == the->send_buffer_remain) {
			comm_session_send_buffer_end_write(the->send_buffer);
			the->send_buffer = C_NULL;
			continue;
		}
	} while (remaining);

	txBoolean more = 0 != (flags & 1);
	if (the->send_buffer && !more) {
		comm_session_send_buffer_end_write(the->send_buffer);
		the->send_buffer = C_NULL;
	}
}

#endif /* mxDebug */
