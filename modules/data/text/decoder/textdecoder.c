/*
* Copyright (c) 2021-2022  Moddable Tech, Inc.
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
#ifdef kPocoRotation
	// Moddable SDK
	#include "mc.xs.h"			// for xsID_ values

	#define VALIDATE 1
#else
	// xst, xsnap, etc
	#include <stdbool.h>

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

static uint8_t isLegalUTF8(const uint8_t *source, int length);

void xs_textdecoder_destructor(void *data)
{
}

void xs_textdecoder(xsMachine *the)
{
	modTextDecoderRecord decoder;
	int argc = xsmcArgc;

	if (argc && c_strcmp(xsmcToString(xsArg(0)), "utf-8"))
		xsRangeError("unsupported encoding");

#if !VALIDATE
	xsmcGet(xsResult, xsTarget, xsID("prototype"));
	xsResult = xsNewHostInstance(xsResult);
	xsThis = xsResult;
	xsmcSetHostDestructor(xsThis, NULL);
	c_memset(&decoder, 0, sizeof(decoder));
#endif

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
	null character maps to 0xC0, 0x80
	
	implementation overallocates by 3 bytes if BOM is present and ignoreBOM is false
*/

void xs_textdecoder_decode(xsMachine *the)
{
	uint8_t *src, *srcEnd, *dst, *dst3;
	uint8_t *buffer;
	xsUnsignedValue srcLength, bufferLength;
	modTextDecoder td;
	uint8_t srcOffset = 0;
	uint32_t outLength = 0;
	uint8_t stream = 0;
	int argc = xsmcArgc;

	if (argc > 1) {
		xsmcVars(1);

		xsmcGet(xsVar(0), xsArg(1), xsID_stream);
		stream = xsmcToBoolean(xsVar(0));
	}

	if (argc) {
		xsmcGetBufferReadable(xsArg(0), (void **)&src, &srcLength);
		srcEnd = src + srcLength;
	}
	else
		src = srcEnd = NULL;

#if VALIDATE
	td = xsmcGetHostChunkValidate(xsThis, xs_textdecoder_destructor);
#else
	td = xsmcGetHostChunk(xsThis);
#endif
	buffer = td->buffer;
	bufferLength = td->bufferLength;

	while ((src < srcEnd) || bufferLength) {
		unsigned char first, clen, i;
		uint8_t utf8[4];

		if (bufferLength) {
			bufferLength--;
			first = *buffer++;
		}
		else
			first = c_read8(src++);
		if (first < 0x80) {
			outLength += (0 == first) ? 2 : 1;
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

		utf8[0] = first;
		for (i = 0; i < clen; i++) {
			if (i < bufferLength)
				utf8[i + 1] = buffer[i];
			else
				utf8[i + 1] = c_read8(src + i - bufferLength);
		}

		if (!isLegalUTF8(utf8, clen + 1)) {
			if (td->fatal)
				goto fatal;

			uint8_t lower = 0x80, upper = 0xBF;
			if (0xE0 == first)
				lower = 0xA0;
			else if (0xED == first)
				lower = 0x9F;
			else if (0xF0 == first)
				lower = 0x90;
			else if (0xF4 == first)
				lower = 0x8F;
			else if (first > 0xF4)	// no valid next byte
				clen = 0;

			while (clen-- > 0) {
				uint8_t c = c_read8(src);
				if ((lower <= c) && (c <= upper))
					src++;
				else
					break;
			}

			outLength += 3;
			continue;
		}

#if mxCESU8
		outLength += (3 == clen) ? 6 : (clen + 1);
#else
		outLength += clen + 1;
#endif
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

	xsmcSetStringBuffer(xsResult, NULL, outLength + 1);

	if (argc) {
		xsmcGetBufferReadable(xsArg(0), (void **)&src, &srcLength);
		srcEnd = src + srcLength;
		src += srcOffset;
	}
	else
		src = srcEnd = NULL;

#if VALIDATE
	td = xsmcGetHostChunkValidate(xsThis, xs_textdecoder_destructor);
#else
	td = xsmcGetHostChunk(xsThis);
#endif
	buffer = td->buffer;
	bufferLength = td->bufferLength;

	dst = (uint8_t *)xsmcToString(xsResult);
	dst3 = td->ignoreBOM ? NULL : (dst + 3);

	while ((src < srcEnd) || bufferLength) {
		unsigned char first, clen, i, firstFromBuffer;
		uint8_t utf8[4];

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
				*dst++ = 0xC0;
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

		utf8[0] = first;
		for (i = 0; i < clen; i++) {
			if (i < bufferLength)
				utf8[i + 1] = buffer[i];
			else
				utf8[i + 1] = c_read8(src + i - bufferLength);
		}

		if (!isLegalUTF8(utf8, clen + 1)) {
			*dst++ = 0xEF;
			*dst++ = 0xBF;
			*dst++ = 0xBD;

			uint8_t lower = 0x80, upper = 0xBF;
			if (0xE0 == first)
				lower = 0xA0;
			else if (0xED == first)
				lower = 0x9F;
			else if (0xF0 == first)
				lower = 0x90;
			else if (0xF4 == first)
				lower = 0x8F;
			else if (first > 0xF4)	// no valid next byte
				clen = 0;

			while (clen-- > 0) {
				uint8_t c = c_read8(src);
				if ((lower <= c) && (c <= upper))
					src++;
				else
					break;
			}

			continue;
		}

#if mxCESU8
		if (3 != clen) {
			*dst++ = first;
			for (i = 0; i < clen; i++)
				*dst++ = utf8[i + 1];
		}
		else {
			xsIntegerValue c;
			fxUTF8Decode((xsStringValue)utf8, &c);
			c -= 0x10000;
			fxUTF8Encode((xsStringValue)dst, 0xD800 + (c >> 10));
			dst += 3;
			fxUTF8Encode((xsStringValue)dst, 0xDC00 + (c & 0x3FF));
			dst += 3;
		}
#else
		*dst++ = first;
		for (i = 0; i < clen; i++)
			*dst++ = utf8[i + 1];
#endif
		
		if ((0xEF == first) && (dst == dst3)) {
			if ((0xBF == dst[-1]) && (0xBB == dst[-2]))
				dst -= 3;
		}

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
	*dst++ = 0;

	c_memcpy(td->buffer, buffer, bufferLength);
	c_memcpy(td->buffer + bufferLength, src, srcEnd - src);
	td->bufferLength = bufferLength + (srcEnd - src);

	return;

fatal:
	xsTypeError("invalid utf-8");
}

void xs_textdecoder_get_encoding(xsMachine *the)
{
	xsmcSetString(xsResult, "utf-8");
}

void xs_textdecoder_get_ignoreBOM(xsMachine *the)
{
#if VALIDATE
	modTextDecoder td = xsmcGetHostChunkValidate(xsThis, xs_textdecoder_destructor);
#else
	modTextDecoder td = xsmcGetHostChunk(xsThis);
#endif
	xsmcSetBoolean(xsResult, td->ignoreBOM);
}

void xs_textdecoder_get_fatal(xsMachine *the)
{
#if VALIDATE
	modTextDecoder td = xsmcGetHostChunkValidate(xsThis, xs_textdecoder_destructor);
#else
	modTextDecoder td = xsmcGetHostChunk(xsThis);
#endif
	xsmcSetBoolean(xsResult, td->fatal);
}

#if !VALIDATE
void modInstallTextDecoder(xsMachine *the)
{
	#define kPrototype (0)
	#define kConstructor (1)
	#define kScratch (2)

	xsBeginHost(the);
	xsmcVars(3);

	xsVar(kPrototype) = xsNewHostObject(NULL);
	xsVar(kConstructor) = xsNewHostConstructor(xs_textdecoder, 2, xsVar(kPrototype));
	xsmcSet(xsGlobal, xsID("TextDecoder"), xsVar(kConstructor));

	xsVar(kScratch) = xsNewHostFunction(xs_textdecoder_decode, 1);
	xsmcSet(xsVar(kPrototype), xsID("decode"), xsVar(kScratch));
	xsVar(kScratch) = xsNewHostFunction(xs_textdecoder_get_encoding, 0);
	xsmcDefine(xsVar(kPrototype), xsID("encoding"), xsVar(kScratch), xsIsGetter);
	xsVar(kScratch) = xsNewHostFunction(xs_textdecoder_get_ignoreBOM, 0);
	xsmcDefine(xsVar(kPrototype), xsID("ignoreBOM"), xsVar(kScratch), xsIsGetter);
	xsVar(kScratch) = xsNewHostFunction(xs_textdecoder_get_fatal, 0);
	xsmcDefine(xsVar(kPrototype), xsID("fatal"), xsVar(kScratch), xsIsGetter);

	xsEndHost(the);
}
#endif

/*
 * Copyright 2001-2004 Unicode, Inc.
 * 
 * Disclaimer
 * 
 * This source code is provided as is by Unicode, Inc. No claims are
 * made as to fitness for any particular purpose. No warranties of any
 * kind are expressed or implied. The recipient agrees to determine
 * applicability of information provided. If this file has been
 * purchased on magnetic or optical media from Unicode, Inc., the
 * sole remedy for any claim will be exchange of defective media
 * within 90 days of receipt.
 * 
 * Limitations on Rights to Redistribute This Code
 * 
 * Unicode, Inc. hereby grants the right to freely use the information
 * supplied in this file in the creation of products supporting the
 * Unicode Standard, and to make copies of this file in any form
 * for internal or external distribution as long as this notice
 * remains attached.
 */
 
uint8_t isLegalUTF8(const uint8_t *source, int length) {
    uint8_t a;
    const uint8_t *srcptr = source+length;
    switch (length) {
    default: return false;
	/* Everything else falls through when "true"... */
    case 4: if ((a = (*--srcptr)) < 0x80 || a > 0xBF) return false;
    case 3: if ((a = (*--srcptr)) < 0x80 || a > 0xBF) return false;
    case 2: if ((a = (*--srcptr)) > 0xBF) return false;

	switch (*source) {
	    /* no fall-through in this inner switch */
	    case 0xE0: if (a < 0xA0) return false; break;
	    case 0xED: if (a > 0x9F) return false; break;
	    case 0xF0: if (a < 0x90) return false; break;
	    case 0xF4: if (a > 0x8F) return false; break;
	    default:   if (a < 0x80) return false;
	}

    case 1: if (*source >= 0x80 && *source < 0xC2) return false;
    }
    if (*source > 0xF4) return false;
    return true;
}

