/*
 * Copyright (c) 2016-2024  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Tools.
 * 
 *   The Moddable SDK Tools is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 * 
 *   The Moddable SDK Tools is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 * 
 *   You should have received a copy of the GNU General Public License
 *   along with the Moddable SDK Tools.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "mc.xs.h"
#include "adpcm-lib.h"

void Tool_compressIMA(xsMachine* the)
{
	static void *context;
	size_t inputBufferBytes = xsToInteger(xsArg(1)) << 1;
	void *uncompressedSamples = xsToArrayBuffer(xsArg(0));
	size_t outputSize = 0;
	uint8_t imaBuffer[256];
	int sample_rate = xsToInteger(xsArg(2));

	if ((inputBufferBytes >> 1) > sizeof(imaBuffer))		// very conservative check
		xsUnknownError("too many samples");

	if (!context) {
		const int lookahead = 8;
		const int noise_shaping = NOISE_SHAPING_DYNAMIC;
		context = adpcm_create_context(1, sample_rate, lookahead, noise_shaping);
	}

	adpcm_encode_block(context, imaBuffer, &outputSize, uncompressedSamples, inputBufferBytes >> 1);

	xsResult = xsArrayBuffer(imaBuffer, outputSize);
}
