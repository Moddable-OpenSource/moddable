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

#include "piuMC.h"
#include "mc.defines.h"

#ifndef MODDEF_AUDIOOUT_BITSPERSAMPLE
	#define MODDEF_AUDIOOUT_BITSPERSAMPLE (16)
#endif
#ifndef MODDEF_AUDIOOUT_NUMCHANNELS
	#define MODDEF_AUDIOOUT_NUMCHANNELS (1)
#endif
#ifndef MODDEF_AUDIOOUT_SAMPLERATE
	#define MODDEF_AUDIOOUT_SAMPLERATE (11025)
#endif

void PiuSound_get_bitsPerSample(xsMachine* the)
{
	xsResult = xsInteger(MODDEF_AUDIOOUT_BITSPERSAMPLE);
}

void PiuSound_get_numChannels(xsMachine* the)
{
	xsResult = xsInteger(MODDEF_AUDIOOUT_NUMCHANNELS);
}

void PiuSound_get_sampleRate(xsMachine* the)
{
	xsResult = xsInteger(MODDEF_AUDIOOUT_SAMPLERATE);
}

