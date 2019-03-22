/*
 * Copyright (c) 2018-2019  Moddable Tech, Inc.
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
 
 /*-------------------------------------------------------------------------
 * Include Files
 *-----------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "qapi_types.h"

#include "qurt_error.h"
#include "qurt_thread.h"
#include "qurt_signal.h"
#include "qurt_timer.h"

void xs_start();
 
 /*-------------------------------------------------------------------------
 * Function Definitions
 *-----------------------------------------------------------------------*/
void *_sbrk(int x) { return NULL; }

void debugger_write(char*foo, int len);
#define xty "vApplicationStackOverflowHook"
void vApplicationStackOverflowHook(void *xTask, char *pcTaskName)
{
	debugger_write(xty, strlen(xty));
	debugger_write(pcTaskName, strlen(pcTaskName));
}

#define xtx "vApplicationMallocFailedHook"
void vApplicationMallocFailedHook(void)
{
	debugger_write(xtx, strlen(xtx));
}
 

void som_app_init(void) { }
void som_app_entry(void) {
}

void app_init(qbool_t ColdBoot) { }
void app_start(qbool_t ColdBoot) {
	xs_start();	
}

extern void fs_init(void);
extern void fs_deinit(void);
extern void qca_module_init(void);

typedef void (*PLATFORM_FUNCTION_TYPE)(void);

PLATFORM_FUNCTION_TYPE init_coldboot_functions[] = {
	fs_init,
	qca_module_init,
	0,
};

PLATFORM_FUNCTION_TYPE deinit_coldboot_functions[] = {
	fs_deinit,
	0,
};

PLATFORM_FUNCTION_TYPE init_sensormode_functions[] =
{
	0,
};

PLATFORM_FUNCTION_TYPE deinit_sensormode_functions[] =
{
	0,
};

PLATFORM_FUNCTION_TYPE init_mom_functions[] =
{
	0,
};
