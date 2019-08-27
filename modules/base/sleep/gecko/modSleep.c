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

#include "xs.h"
#include "xsHost.h"

#include "em_chip.h"
#include "em_cmu.h"
#include "em_emu.h"
#include "em_burtc.h"
#include "em_rmu.h"
#include "mc.defines.h"

extern uint32_t gModIdleSleep;
int setMaxSleep(int sleepLevel);

void xs_get_persistent_value(xsMachine *the) {
#if MODDEF_SLEEP_RETENTION_MEMORY
	uint32_t reg = xsToInteger(xsArg(0));
	uint32_t val;
	val = geckoGetPersistentValue(reg);
	xsResult = xsInteger(val);
#else
	xsUnknownError("retention values off");
#endif
}

void xs_set_persistent_value(xsMachine *the) {
#if MODDEF_SLEEP_RETENTION_MEMORY
	uint32_t reg = xsToInteger(xsArg(0));
	uint32_t val = xsToInteger(xsArg(1));
	geckoSetPersistentValue(reg, val);
#else
	xsUnknownError("retention values off");
#endif
}

void xs_sleep_enter_em4(xsMachine *the) {
	uint32_t delay = xsToInteger(xsArg(0));
	geckoSleepEM4(delay);
}

#define kSleepPowerOnReset	    0x01
#define kSleepExternalReset     0x02
#define kSleepSysRequestReset   0x04
#define kSleepEM4WakeupReset    0x08
#define kSleepBrownOutReset	    0x10
#define kSleepLockupReset	    0x20
#define kSleepWatchdogReset	    0x40

void xs_sleep_get_reset_cause(xsMachine *the) {
	uint32_t resetCause = geckoGetResetCause();
	uint32_t ret = 0;
	if (resetCause & RMU_RSTCAUSE_PORST)
		ret += kSleepPowerOnReset;
	if (resetCause & RMU_RSTCAUSE_EXTRST)
		ret += kSleepExternalReset;
	if (resetCause & RMU_RSTCAUSE_SYSREQRST)
		ret += kSleepSysRequestReset;
	if (resetCause & RMU_RSTCAUSE_EM4RST)
		ret += kSleepEM4WakeupReset;
	if (resetCause & (RMU_RSTCAUSE_DECBOD | RMU_RSTCAUSE_DVDDBOD | RMU_RSTCAUSE_AVDDBOD))
		ret += kSleepBrownOutReset;
	if (resetCause & RMU_RSTCAUSE_LOCKUPRST)
		ret += kSleepLockupReset;
	if (resetCause & RMU_RSTCAUSE_WDOGRST)
		ret += kSleepWatchdogReset;

//	xsLog("reset cause %d\n", resetCause);

	xsResult = xsInteger(ret);
}

extern uint32_t gWakeupPin;
void xs_sleep_get_wakeup_pin(xsMachine *the) {
	xsResult = xsInteger(gWakeupPin);
}

void xs_sleep_get_idle_sleep_level(xsMachine *the) {
	xsResult = xsInteger(gModIdleSleep);
}

void xs_sleep_set_idle_sleep_level(xsMachine *the) {
	uint32_t level = xsToInteger(xsArg(0));
	xsResult = xsInteger(setMaxSleep(level));
}
