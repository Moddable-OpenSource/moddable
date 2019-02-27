/*
 * NEEDS BOILERPLATE
 *     Copyright (C) 2016-2017 Moddable Tech, Inc.
 *     All rights reserved.
 */

#include "xs.h"
#include "xsgecko.h"

#include "em_chip.h"
#include "em_cmu.h"
#include "em_emu.h"
#include "em_burtc.h"
#include "em_rmu.h"
#include "mc.defines.h"

void xs_get_persistent_value(xsMachine *the) {
#if MODDEF_RETENTION
	uint32_t reg = xsToInteger(xsArg(0));
	uint32_t val;
	val = geckoGetPersistentValue(reg);
	xsResult = xsInteger(val);
#else
	xsUnknownError("retention values off");
#endif
}

void xs_set_persistent_value(xsMachine *the) {
#if MODDEF_RETENTION
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

void xs_sleep_get_reset_cause(xsMachine *the) {
	uint32_t resetCause = geckoGetResetCause();
	xsResult = xsInteger(resetCause);
}

extern uint32_t wakeupPin;
void xs_sleep_get_wakeup_pin(xsMachine *the) {
	xsResult = xsInteger(wakeupPin);
}
