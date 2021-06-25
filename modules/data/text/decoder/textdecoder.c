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
	#define xsID_ignoreBOM (xsID("ignoreBOM"))
	#define xsID_fatal (xsID("fatal"))
#endif

typedef struct {
	uint8_t		ignoreBOM;
	uint8_t		fatal;
} modTextDecoderRecord, *modTextDecoder;

void xs_textdecoder_destructor(void *data)
{
}

void xs_textdecoder(xsMachine *the)
{
	modTextDecoderRecord decoder;
	int argc = xsmcArgc;

	if (argc && c_strcmp(xsmcToString(xsArg(0)), "utf-8"))
		xsRangeError("unsuppoorted encoding");

	decoder.ignoreBOM = false;
	decoder.fatal = false;
	if (argc >= 2) {
		xsmcVars(1);

		xsmcGet(xsVar(0), xsArg(1), xsID_ignoreBOM);
		decoder.ignoreBOM = xsmcTest(xsVar(0));

		xsmcGet(xsVar(0), xsArg(1), xsID_fatal);
		decoder.fatal = xsmcTest(xsVar(0));
	}

	xsmcSetHostChunk(xsThis, &decoder, sizeof(decoder));
}

/*
	UTF-8 BOM is sequence 0xEF,0xBB,0xBF
	Replacement character sequence in UTF-8 is 0xEF 0xBF 0xBD
	null character maps to 0xF4, 0x90, 0x80, 0x80
*/

void xs_textdecoder_decode(xsMachine *the)
{
	uint8_t *src, *srcEnd, *dst;
	xsUnsignedValue srcLength;
	modTextDecoder td;
	uint8_t srcOffset = 0;
	uint32_t outLength = 0;

	if (xsmcArgc > 1)
		xsUnknownError("options not supported");

	xsmcGetBuffer(xsArg(0), (void **)&src, &srcLength);
	srcEnd = src + srcLength;

	td = xsmcGetHostChunk(xsThis);
	if (td->ignoreBOM && (srcLength >= 3)) {
		if ((0xEF == c_read8(src + 0)) && (0xBB == c_read8(src + 1)) && (0xBF == c_read8(src + 2))) {
			srcOffset = 3;
			src += 3;
		}
	}

	while (src < srcEnd) {
		unsigned char first = c_read8(src++), clen, i;
		if (first < 0x80) {
			outLength += (0 == first) ? 4 : 1;
			continue;
		}

		if (0xC0 == (first & 0xE0))
			clen = 1;
		else if (0xE0 == (first & 0xF0))
			clen = 2;
		else if (0xF0 == (first & 0xF0))
			clen = 3;
		else if (td->fatal)
			goto fatal;
		else {
			outLength += 3;
			continue;
		}

		if ((src + clen) > srcEnd) {
			if (td->fatal)
				goto fatal;

			outLength += 3;
			continue;
		}

		for (i = 0; i < clen; i++) {
			if (0x80 == (0xC0 & c_read8(src + i)))
				continue;

			if (td->fatal)
				goto fatal;

			outLength += 3;
			clen = 0;
			break;
		}
		src += clen;
		outLength += clen ? clen + 1 : 0;
	}

	xsmcSetStringBuffer(xsResult, NULL, outLength + 1);

	xsmcGetBuffer(xsArg(0), (void **)&src, NULL);
	srcEnd = src + srcLength;
	src += srcOffset;

	dst = (uint8_t *)xsmcToString(xsResult);

	while (src < srcEnd) {
		unsigned char first = c_read8(src++), clen, i;
		if (first < 0x80) {
			if (first)
				*dst++ = first;
			else {
				*dst++ = 0xF4;
				*dst++ = 0x90;
				*dst++ = 0x80;
				*dst++ = 0x80;
			}
			continue;
		}

		if (0xC0 == (first & 0xE0))
			clen = 1;
		else if (0xE0 == (first & 0xF0))
			clen = 2;
		else if (0xF0 == (first & 0xF0))
			clen = 3;
		else {
			*dst++ = 0xEF;
			*dst++ = 0xBF;
			*dst++ = 0xBD;
			continue;
		}

		if ((src + clen) > srcEnd) {
			*dst++ = 0xEF;
			*dst++ = 0xBF;
			*dst++ = 0xBD;
			continue;
		}

		for (i = 0; i < clen; i++) {
			if (0x80 == (0xC0 & c_read8(src + i)))
				continue;

			*dst++ = 0xEF;
			*dst++ = 0xBF;
			*dst++ = 0xBD;

			clen = 0;
			break;
		}

		if (clen) {
			*dst++ = first;
			do {
				*dst++ = c_read8(src++);
			} while (--clen);
		}
	}
	*dst++ = 0;

	return;

fatal:
	xsTypeError("invalid utf-8");
}

void xs_textdecoder_get_enccoding(xsMachine *the)
{
	xsmcSetString(xsResult, "utf-8");
}

void xs_textdecoder_get_ignoreBOM(xsMachine *the)
{
	modTextDecoder td = xsmcGetHostChunk(xsThis);
	xsmcSetBoolean(xsResult, td->ignoreBOM);
}

void xs_textdecoder_get_fatal(xsMachine *the)
{
	modTextDecoder td = xsmcGetHostChunk(xsThis);
	xsmcSetBoolean(xsResult, td->fatal);
}
