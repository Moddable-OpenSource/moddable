/*
 * Copyright (c) 2016-2018  Moddable Tech, Inc.
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

#define __XS6PLATFORMMINIMAL__
#include "xs.h"
#include "xsqca4020.h"
#include "xsmain.h"
#include "modTimer.h"

#include "xsPlatform.h"
#include "modInstrumentation.h"

#include "qapi_types.h"
#include "qurt_error.h"
#include "qurt_signal.h"
#include "qurt_thread.h"

#define xsMain_THREAD_PRIORITY		(20)
#define xsMain_THREAD_STACK_SIZE	(1024 * 20)


xsMachine *gThe = NULL;        // the one XS6 virtual machine running

xsCallback xsHostModuleAt(xsIndex i)
{
    return NULL;
}

extern void mc_setup(xsMachine *the);
extern void setupDebugger();
extern void fxReceiveLoop();

qurt_thread_t gxsThread;
qurt_signal_t gMainSignal;

void xs_setup() {
	setupDebugger();

    gThe = ESP_cloneMachine(0, 0, 0, 0);

	mc_setup(gThe);
}

void xs_loop(void)
{
    if (!gThe)
        return;

    modTimersExecute();
	modMessageService(gThe, modTimersNext());
}

static void xs_task(void *Param)
{
	qurt_signal_set(&gMainSignal, kSIG_THREAD_CREATED);
	xs_setup();
	while (1) {
#if mxDebug
		uint32_t out;
		if (QURT_EOK == qurt_signal_wait_timed(&gMainSignal, kSIG_SERVICE_DEBUGGER, QURT_SIGNAL_ATTR_WAIT_ANY | QURT_SIGNAL_ATTR_CLEAR_MASK, &out, QURT_TIME_NO_WAIT))
			fxReceiveLoop();
#endif
		xs_loop();
	}

}

void xs_start() {
	qurt_thread_attr_t	attr;
	int ret;

	qurt_signal_create(&gMainSignal);

	qurt_thread_attr_init(&attr);
	qurt_thread_attr_set_name(&attr, "xsMain");
	qurt_thread_attr_set_priority(&attr, xsMain_THREAD_PRIORITY);
	qurt_thread_attr_set_stack_size(&attr, xsMain_THREAD_STACK_SIZE);
	ret = qurt_thread_create(&gxsThread, &attr, xs_task, NULL);

	if (QURT_EOK == ret) {
		uint32_t sigAttr, sigWaiting;
		sigAttr = QURT_SIGNAL_ATTR_WAIT_ANY | QURT_SIGNAL_ATTR_CLEAR_MASK;

		if (QURT_EOK != qurt_signal_wait_timed(&gMainSignal, kSIG_THREAD_CREATED, sigAttr, &sigWaiting, QURT_TIME_WAIT_FOREVER)) {
			debugger_write("xs thread create failed.\n", 25);
		}
	}
}

