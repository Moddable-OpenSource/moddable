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
 */

#include "xsmc.h"
#include "xsHost.h"
#include "mc.xs.h"      // for xsID_ values

#include "applib/ui/vibes.h"

void xs_vibes_shortPulse(xsMachine *the)
{
	vibes_short_pulse();
}

void xs_vibes_longPulse(xsMachine *the)
{
	vibes_long_pulse();
}

void xs_vibes_doublePulse(xsMachine *the)
{
	vibes_double_pulse();
}

void xs_vibes_pattern(xsMachine *the)
{
	VibePattern pattern;
	xsSlot tmp;

	xsmcGet(tmp, xsArg(0), xsID_length);
	pattern.num_segments = xsmcToInteger(tmp);
	uint32_t *durations = c_malloc(sizeof(uint32_t) * pattern.num_segments);
	if (!durations)
		xsUnknownError("no memory");
	pattern.durations = durations;

	for (uint32_t i = 0; i < pattern.num_segments; i++) {
		xsmcGetIndex(tmp, xsArg(0), i);
		durations[i] = xsmcToInteger(tmp);
	}

	vibes_enqueue_custom_pattern(pattern);
	c_free(durations);
}

void xs_vibes_cancel(xsMachine *the)
{
	vibes_cancel();
}
