/*
 * Copyright (c) 2016-2019  Moddable Tech, Inc.
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

#include "xsmc.h"
#include "mc.xs.h"			// for xsID_ values
#include "xsHost.h"
#include "driver/pcnt.h"

typedef struct {
	int			unit;
	int			base;
} PulseCountRecord, *PulseCount;

void xs_pulsecount_destructor(void *data)
{
}

void xs_pulsecount(xsMachine *the)
{
	PulseCountRecord pc;
	int signal, control, filter = 100;

	xsmcVars(1);

	xsmcGet(xsVar(0), xsArg(0), xsID_signal);
	signal = xsmcToInteger(xsVar(0));

	xsmcGet(xsVar(0), xsArg(0), xsID_control);
	control = xsmcToInteger(xsVar(0));

	pc.unit = PCNT_UNIT_0;
	if (xsmcHas(xsArg(0), xsID_unit)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_unit);
		pc.unit = xsmcToInteger(xsVar(0));
		if ((pc.unit < 0) || (pc.unit > PCNT_UNIT_MAX))
			xsUnknownError("invalid unit");
	}

	if (xsmcHas(xsArg(0), xsID_filter)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_filter);
		filter = xsmcToInteger(xsVar(0));
	}

	pc.base = 0;
	xsmcSetHostChunk(xsThis, &pc, sizeof(PulseCountRecord));

	/* Prepare configuration for the PCNT unit */
	pcnt_config_t pcnt_config = {
		// Set PCNT input signal and control GPIOs
		.pulse_gpio_num = 0,
		.ctrl_gpio_num = 0,
		.channel = PCNT_CHANNEL_0,
		.unit = 0,
		// What to do on the positive / negative edge of pulse input?
		.pos_mode = PCNT_COUNT_INC,   // Count up on the positive edge
		.neg_mode = PCNT_COUNT_DIS,   // Keep the counter value on the negative edge
		// What to do when control input is low or high?
		.lctrl_mode = PCNT_MODE_REVERSE, // Reverse counting direction if low
		.hctrl_mode = PCNT_MODE_KEEP,    // Keep the primary counter mode if high
		// Set the maximum and minimum limit values to watch
		.counter_h_lim = 32767,
		.counter_l_lim = -32768
	};

	pcnt_config.pulse_gpio_num = signal;
	pcnt_config.ctrl_gpio_num = control;
	pcnt_config.unit = pc.unit;

	pcnt_unit_config(&pcnt_config);

	pcnt_set_filter_value(pc.unit, filter);
	pcnt_filter_enable(pc.unit);

	pcnt_counter_pause(pc.unit);
	pcnt_counter_clear(pc.unit);
	pcnt_counter_resume(pc.unit);
}

void xs_pulsecount_close(xsMachine *the)
{
	PulseCount pc = xsmcGetHostChunk(xsThis);
	if (!pc) return;
	xs_pulsecount_destructor(pc);
	xsmcSetHostData(xsThis, NULL);
}

void xs_pulsecount_get(xsMachine *the)
{
	PulseCount pc = xsmcGetHostChunk(xsThis);
	int16_t count;

	if (!pc) return;

	pcnt_get_counter_value(pc->unit, &count);
	xsmcSetInteger(xsResult, count + pc->base);
}

void xs_pulsecount_set(xsMachine *the)
{
	PulseCount pc = xsmcGetHostChunk(xsThis);
	if (!pc) return;

	pc->base = xsmcToInteger(xsArg(0));
	pcnt_counter_clear(pc->unit);
}
