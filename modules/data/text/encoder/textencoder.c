/*
* Copyright (c) 2021  Moddable Tech, Inc.
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
#if mxNoFunctionLength
	#include "mc.xs.h"			// for xsID_ values
#else
	#define xsID_String (xsID("String"))
	#define xsID_Uint8Array (xsID("Uint8Array"))
#endif

/*
	null character maps to 0xF4, 0x90, 0x80, 0x80
*/

void xs_textencoder_encode(xsMachine *the)
{
	uint8_t *src, *dst;
	int length = 0;
	uint8_t hasNull = 0;

	xsArg(0) = xsCall1(xsGlobal, xsID_String, xsArg(0));
	src = (uint8_t *)xsmcToString(xsArg(0));

	while (true) {
		uint8_t c = c_read8(src++);
		if (!c) break;

		length += 1;
		if ((0xF4 == c) && (0x90 == c_read8(src)) && (0x80 == c_read8(src + 1)) && (0x80 == c_read8(src + 2))) {
			src += 3;
			hasNull = 1;
		}
	}

	xsmcSetArrayBuffer(xsResult, NULL, length);
	src = (uint8_t *)xsmcToString(xsArg(0));
	dst = xsmcToArrayBuffer(xsResult);
	if (hasNull) {
		while (true) {
			uint8_t c = c_read8(src++);
			if (!c) break;

			if ((0xF4 == c) && (0x90 == c_read8(src)) && (0x80 == c_read8(src + 1)) && (0x80 == c_read8(src + 2))) {
				*dst++ = 0;
				src += 3;
			}
			else
				*dst++ = c;
		}
	}
	else
		c_memmove(dst, src, length);

	xsResult = xsNew1(xsGlobal, xsID_Uint8Array, xsResult);
}
