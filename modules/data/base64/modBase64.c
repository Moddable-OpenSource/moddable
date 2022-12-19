/*
 * Copyright (c) 2016-2021  Moddable Tech, Inc.
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
 * This file incorporates work covered by the following copyright and  
 * permission notice:  
 *
 *       Copyright (C) 2010-2016 Marvell International Ltd.
 *       Copyright (C) 2002-2010 Kinoma, Inc.
 *
 *       Licensed under the Apache License, Version 2.0 (the "License");
 *       you may not use this file except in compliance with the License.
 *       You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *       Unless required by applicable law or agreed to in writing, software
 *       distributed under the License is distributed on an "AS IS" BASIS,
 *       WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *       See the License for the specific language governing permissions and
 *       limitations under the License.
 */

/*
	modeled on FskStrB64Decode and FskStrB64Encode from KPR.
*/

#include "xsmc.h"
#include "xsHost.h"

void xs_base64_encode(xsMachine *the)
{
	static const char b64[] ICACHE_XS6RO2_ATTR = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	xsType		srcType;
	uint8_t		*src, *dst;
	uint32_t	srcSize, dstSize;
	uint8_t		a, b, c;

	srcType = xsmcTypeOf(xsArg(0));
	if (xsStringType == srcType) {
		src = (uint8_t *)xsmcToString(xsArg(0));
		srcSize = c_strlen((char *)src);
	}
	else
		xsmcGetBufferReadable(xsArg(0), (void **)&src, &srcSize);
	dstSize = (((srcSize + 2) / 3) * 4) + 1;

	xsResult = xsStringBuffer(NULL, dstSize);
	if (xsStringType == srcType)
		src = (uint8_t *)xsmcToString(xsArg(0));		// refresh pointer
	else
		xsmcGetBufferReadable(xsArg(0), (void **)&src, &srcSize);
	dst = (uint8_t *)xsmcToString(xsResult);

	while (srcSize > 2) {
		a = c_read8(src++);
		b = c_read8(src++);
		c = c_read8(src++);
		*dst++ = c_read8(b64 + ((a & 0xfc) >> 2));
		*dst++ = c_read8(b64 + (((a & 0x3) << 4) | ((b & 0xf0) >> 4)));
		*dst++ = c_read8(b64 + (((b & 0xf) << 2) | ((c & 0xc0) >> 6)));
		*dst++ = c_read8(b64 + (c & 0x3f));
		srcSize -= 3;
	}

	if (srcSize == 2) {
		a = c_read8(src++);
		b = c_read8(src++);
		*dst++ = c_read8(b64 + ((a & 0xfc) >> 2));
		*dst++ = c_read8(b64 + (((a & 0x3) << 4) | ((b & 0xf0) >> 4)));
		*dst++ = c_read8(b64 + ((b & 0xf) << 2));
		*dst++ = '=';
	}
	else if (srcSize == 1) {
		a = c_read8(src++);
		*dst++ = c_read8(b64 + ((a & 0xfc) >> 2));
		*dst++ = c_read8(b64 + ((a & 0x3) << 4));
		*dst++ = '=';
		*dst++ = '=';
	}

	*dst++ = 0;
}

void xs_base64_decode(xsMachine *the)
{
	uint8_t		*src;
	uint32_t	srcSize, dstSize, srcIndex, dstIndex;
	uint8_t		aFlag = 0;
	uint8_t		aByte;
	uint8_t		aBuffer[4];
	uint8_t		*dst, *dstStart;

	src = (uint8_t *)xsmcToString(xsArg(0));
	srcSize = c_strlen((char *)src);

	dstSize = (srcSize / 4) * 3;
	if (c_read8(src + srcSize - 1) == '=')
		dstSize--;
	if (c_read8(src + srcSize - 2) == '=')
		dstSize--;
	srcIndex = 0;

	xsmcSetArrayBufferResizable(xsResult, NULL, dstSize, dstSize);
	dst = dstStart = xsmcToArrayBuffer(xsResult);

	src = (uint8_t *)xsmcToString(xsArg(0));	// refresh pointer

	dstIndex = 3;
	while ((aByte = c_read8(src++))) {
		if (('A' <= aByte) && (aByte <= 'Z'))
			aByte = aByte - 'A';
		else if (('a' <= aByte) && (aByte <= 'z'))
			aByte = aByte - 'a' + 26;
		else if (('0' <= aByte) && (aByte <= '9'))
			aByte = aByte - '0' + 52;
		else if (aByte == '+')
			aByte = 62;
		else if (aByte == '/')
			aByte = 63;
		else if (aByte == '=') {
			if (srcIndex == 2) {
				if (c_read8(src) == '=') {
					aBuffer[srcIndex++] = 0;
					dstIndex = 1;
					aByte = 0;
					aFlag = 1;
				}
				else
					continue;
			}
			else if (srcIndex == 3) {
				dstIndex = 2;
				aByte = 0;
				aFlag = 1;
			}
			else
				continue;
		}
		else
			continue;
		aBuffer[srcIndex++] = aByte;
		if (srcIndex == 4) {
			*dst++ = (aBuffer[0] << 2) | ((aBuffer[1] & 0x30) >> 4);
			if (dstIndex > 1)
				*dst++ = ((aBuffer[1] & 0x0F) << 4) | ((aBuffer[2] & 0x3C) >> 2);
			if (dstIndex > 2)
				*dst++ = ((aBuffer[2] & 0x03) << 6) | (aBuffer[3] & 0x3F);
			srcIndex = 0;
		}
		if (aFlag)
			break;
	}

	xsmcSetArrayBufferLength(xsResult, dst - dstStart);
}

void modInstallBase64(xsMachine *the)
{
	#define kNamespace (0)
	#define kScratch (1)

	xsBeginHost(the);
	xsmcVars(2);

	xsVar(kNamespace) = xsNewObject();
	xsmcSet(xsGlobal, xsID("Base64"), xsVar(kNamespace));
	xsVar(kScratch) = xsNewHostFunction(xs_base64_encode, 1);
	xsmcSet(xsVar(kNamespace), xsID("encode"), xsVar(kScratch));
	xsVar(kScratch) = xsNewHostFunction(xs_base64_decode, 1);
	xsmcSet(xsVar(kNamespace), xsID("decode"), xsVar(kScratch));

	xsEndHost(the);
}
