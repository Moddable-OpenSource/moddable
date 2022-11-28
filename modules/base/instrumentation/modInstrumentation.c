/*
 * Copyright (c) 2016-2022  Moddable Tech, Inc.
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


#include "xsPlatform.h"
#include "xsmc.h"

#include "modInstrumentation.h"

#if MODINSTRUMENTATION

static uintptr_t gInstrumentationValues[kModInstrumentationLast + 1];

void modInstrumentationInit(void)
{
	c_memset(gInstrumentationValues, 0, sizeof(gInstrumentationValues));
}

void modInstrumentationSetCallback_(uint8_t what, ModInstrumentationGetter getter)
{
	if ((kModInstrumentationCallbacksBegin <= what) && (what <= kModInstrumentationCallbacksEnd))
		gInstrumentationValues[what] = (uintptr_t)getter;
}

void modInstrumentationSet_(uint8_t what, int32_t value)
{
	if (what < kModInstrumentationCallbacksBegin)
		gInstrumentationValues[what] = value;
}

void modInstrumentationMin_(uint8_t what, int32_t value)
{
	if (what < kModInstrumentationCallbacksBegin) {
		if (value < (int32_t)gInstrumentationValues[what])
			gInstrumentationValues[what] = value;
	}
}

void modInstrumentationMax_(uint8_t what, int32_t value)
{
	if (what < kModInstrumentationCallbacksBegin) {
		if (value > (int32_t)gInstrumentationValues[what])
			gInstrumentationValues[what] = value;
	}
}

int32_t modInstrumentationGet_(void *the, uint8_t what)
{
	if (what <= kModInstrumentationLast) {
		if ((kModInstrumentationCallbacksBegin <= what) && (what <= kModInstrumentationCallbacksEnd)) {
			if (!gInstrumentationValues[what])
				return 0;

			return ((ModInstrumentationGetter)gInstrumentationValues[what])(the);
		}
		return gInstrumentationValues[what];
	}

	return 0;
}

void modInstrumentationAdjust_(uint8_t what, int32_t value)
{
	if (what < kModInstrumentationCallbacksBegin)
		gInstrumentationValues[what] += value;
}

void xs_instrumentation_get(xsMachine *the)
{
	int what = xsmcToInteger(xsArg(0));

	if ((0 <= what) && (what <= kModInstrumentationLast))
		xsmcSetInteger(xsResult, modInstrumentationGet_(the, what));
}

#else

void xs_instrumentation_get(xsMachine *the)
{
	// returns undefined
}

#endif
