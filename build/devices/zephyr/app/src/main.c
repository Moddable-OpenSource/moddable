/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#include "xsmc.h"
#include "xsHost.h"
#include "xsHosts.h"

#include "modInstrumentation.h"

#include "modTimer.h"
void modTimersExecute(void);
int modTimersNext(void);
void setupDebugger(uint32_t *running);

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS   1000

xsMachine *gThe = NULL;

static void runLoop(void *p1, void *p2, void *p3)
{
	uint32_t running = 0;
	setupDebugger(&running);

	while (running == 0) {
		modDelayMilliseconds(10);
	}

	while(1) {
		gThe = modCloneMachine(NULL, NULL);
		modRunMachineSetup(gThe);

#if MODDEF_XS_TEST
		xsMachine *the = gThe;
		while (gThe) {
#if mxDebug
			fxReceiveLoop();
#endif
			modTimersExecute();
			modMessageService(gThe, modTimersNext());
			modInstrumentationAdjust(Turns, +1);
		}
		xsDeleteMachine(the);
#else
		while (true) {
#if mxDebug
	//		fxReceiveLoop();
#endif
			modTimersExecute();
			modMessageService(gThe, modTimersNext());
			modInstrumentationAdjust(Turns, +1);
		}
#endif
	}

printf("      -- end\n");
}

#define PRIORITY 3
K_THREAD_DEFINE(main_thread, 4096, runLoop, NULL, NULL, NULL, PRIORITY, 0, 0);

int main(void)
{
	int ret;

	// nuthin.

	return 0;
}
