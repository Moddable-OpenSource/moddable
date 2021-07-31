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
	#define xsID_stream (xsID("stream"))
#endif

typedef struct {
	uint8_t		ignoreBOM;
	uint8_t		fatal;

	// left over when streaming
	uint8_t		bufferLength;
	uint8_t		buffer[12];
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
	decoder.bufferLength = 0;
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
	uint8_t *buffer;
	xsUnsignedValue srcLength, bufferLength;
	modTextDecoder td;
	uint8_t srcOffset = 0;
	uint32_t outLength = 0;
	uint8_t stream = 0;

	if (xsmcArgc > 1) {
		xsmcVars(1);

		xsmcGet(xsVar(0), xsArg(1), xsID_stream);
		stream = xsmcToBoolean(xsVar(0));
	}

	xsmcGetBuffer(xsArg(0), (void **)&src, &srcLength);
	srcEnd = src + srcLength;

	td = xsmcGetHostChunk(xsThis);
	buffer = td->buffer;
	bufferLength = td->bufferLength;
	//@@ different on first chunk? depends on streaming?
	if (td->ignoreBOM && (srcLength >= 3)) {
		if ((0xEF == c_read8(src + 0)) && (0xBB == c_read8(src + 1)) && (0xBF == c_read8(src + 2))) {
			srcOffset = 3;
			src += 3;
		}
	}

	while (src < srcEnd) {
		unsigned char first, clen, i;
		if (bufferLength) {
			bufferLength--;
			first = *buffer++;
		}
		else
			first = c_read8(src++);
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

		if (clen > ((srcEnd - src) + bufferLength)) {
			if (stream)
				break;	// decode to here. remainder saved below

			if (td->fatal)
				goto fatal;

			outLength += 3;
			continue;
		}

		for (i = 0; i < clen; i++) {
			unsigned char c;
			if (i < bufferLength)
				c = buffer[i];
			else
				c = c_read8(src + i - bufferLength);
			if (0x80 == (0xC0 & c))
				continue;

			if (td->fatal)
				goto fatal;

			outLength += 3;
			clen = 0;
			break;
		}

		if (clen) {
			outLength += clen + 1;

			if (bufferLength) {
				if (bufferLength >= clen) {
					bufferLength -= clen;
					buffer += clen;
				}
				else {
					src += clen - bufferLength; 
					bufferLength = 0;
				}
			}
			else
				src += clen;
		}
	}

	xsmcSetStringBuffer(xsResult, NULL, outLength + 1);

	xsmcGetBuffer(xsArg(0), (void **)&src, NULL);
	srcEnd = src + srcLength;
	src += srcOffset;

	td = xsmcGetHostChunk(xsThis);
	buffer = td->buffer;
	bufferLength = td->bufferLength;

	dst = (uint8_t *)xsmcToString(xsResult);

	while (src < srcEnd) {
		unsigned char first, clen, i, firstFromBuffer;
		if (bufferLength) {
			bufferLength--;
			first = *buffer++;
			firstFromBuffer = 1;
		}
		else {
			first = c_read8(src++);
			firstFromBuffer = 0;
		}
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

		if (clen > ((srcEnd - src) + bufferLength)) {
			if (stream) {
				// put back "first". remainder saved below.
				if (firstFromBuffer) {
					buffer--;
					bufferLength++;
				}
				else
					src--;
				break;
				
			}

			*dst++ = 0xEF;
			*dst++ = 0xBF;
			*dst++ = 0xBD;
			continue;
		}

		for (i = 0; i < clen; i++) {
			unsigned char c;
			if (i < bufferLength)
				c = buffer[i];
			else
				c = c_read8(src + i - bufferLength);
			if (0x80 == (0xC0 & c))
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
				if (bufferLength) {
					bufferLength--;
					*dst++ = *buffer++;
				}
				else
					*dst++ = c_read8(src++);
			} while (--clen);
		}
	}
	*dst++ = 0;

	c_memmove(td->buffer, buffer, bufferLength);
	c_memmove(td->buffer + bufferLength, src, srcEnd - src);
	td->bufferLength = bufferLength + (srcEnd - src);

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
