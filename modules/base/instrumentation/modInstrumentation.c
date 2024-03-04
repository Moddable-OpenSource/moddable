/*
 * Copyright (c) 2016-2023  Moddable Tech, Inc.
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


static const char *gInstrumentationNames[kModInstrumentationLast + 1 + 1 + 1] = {
	"",
	"Pixels Drawn",
	"Frames Drawn",
#if kModInstrumentationHasNetwork
	"Network Bytes Read",
	"Network Bytes Written",
	"Network Sockets",
#endif
	"Timers",
	"Files",

	"Poco Display List Used",
	"Piu Command List Used",
	
#if kModInstrumentationHasSPIFlashErases
	"SPI Flash Erases",
#endif
#if kModInstrumentationHasTurns
	"Turns",
#endif
	"System Free Memory",
#if kModInstrumentationHasCPU
	"CPU 0",
	#if kTargetCPUCount > 1
		"CPU 1",
	#endif
#endif
	"XS Slot Heap Used",
	"XS Chunk Heap Used",
	"XS Keys Used",
	"XS Garbage Collection Count",
	"XS Modules Loaded",
	"XS Stack Used",
	"XS Promises Settled",
	NULL
};

void xs_instrumentation_map(xsMachine *the)
{
	char *name = xsmcToString(xsArg(0));
	const char **walker = gInstrumentationNames;
	int i = 0;

	while (walker[i]) {
		if (0 == c_strcmp(walker[i], name)) {
			xsmcSetInteger(xsResult, i);
			return;
		}
		i++;
	}
}

void xs_instrumentation_name(xsMachine *the)
{
	int what = xsmcToInteger(xsArg(0));

	if ((0 <= what) && (what <= kModInstrumentationLast) && gInstrumentationNames[what])
		xsmcSetString(xsResult, (char *)gInstrumentationNames[what]);
}


#else

void xs_instrumentation_get(xsMachine *the)
{
	// returns undefined
}

void xs_instrumentation_map(xsMachine *the)
{
	// returns undefined
}

void xs_instrumentation_name(xsMachine *the)
{
	// returns undefined
}


#endif
