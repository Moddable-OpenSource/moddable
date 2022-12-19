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

#include "xsPlatform.h"
#include "xsmc.h"
#include "mc.xs.h"			// for xsID_ values

extern void resolveBuffer(xsMachine *the, xsSlot *slot, uint8_t **data, uint32_t *count);

void xs_securesocket_read(xsMachine *the)
{
	xsType dstType;
	int argc = xsmcArgc;
	uint32_t srcBytes;
	unsigned char *srcData;
	uint32_t position, end;

	xsmcVars(4);

	xsmcGet(xsVar(3), xsThis, xsID_data);
	xsmcGet(xsVar(1), xsVar(3), xsID_buffer);

	xsmcGet(xsVar(2), xsVar(3), xsID_position);
	position = xsmcToInteger(xsVar(2));

	xsmcGet(xsVar(2), xsVar(3), xsID_end);
	end = xsmcToInteger(xsVar(2));

	srcBytes = end - position;
	if (!srcBytes) {
		if (0 == argc)
			xsmcSetInteger(xsResult, 0);
		return;
	}

	if (0 == argc) {
		xsmcSetInteger(xsResult, srcBytes);
		return;
	}

	// address limiter argument (count or terminator)
	if (argc > 1) {
		xsType limiterType = xsmcTypeOf(xsArg(1));
		if ((xsNumberType == limiterType) || (xsIntegerType == limiterType)) {
			uint16_t count = xsmcToInteger(xsArg(1));
			if (count < srcBytes)
				srcBytes = count;
		}
		else
		if (xsStringType == limiterType) {
			char *str = xsmcToString(xsArg(1));
			char terminator = c_read8(str);
			if (terminator) {
				unsigned char *t;

				resolveBuffer(the, &xsVar(1), &srcData, NULL);
				srcData += position;
				t = (unsigned char *)c_strchr((char *)srcData, terminator);		//@@ does't end??!?!
				if (t) {
					uint16_t count = (t - srcData) + 1;		// terminator included in result
					if (count < srcBytes)
						srcBytes = count;
				}
			}
		}
		else if (xsUndefinedType == limiterType)
			;
	}

	// generate output
	dstType = xsmcTypeOf(xsArg(0));

	if (xsNullType == dstType)
		xsResult = xsInteger(srcBytes);
	else if (xsReferenceType == dstType) {
		xsSlot *s1, *s2;

		s1 = &xsArg(0);

		xsmcVars(1);
		xsmcGet(xsVar(0), xsGlobal, xsID_String);
		s2 = &xsVar(0);
		if (s1->data[2] == s2->data[2]) {
			xsResult = xsStringBuffer((char *)NULL, srcBytes);

			resolveBuffer(the, &xsVar(1), &srcData, NULL);
			c_memmove(xsmcToString(xsResult), srcData + position, srcBytes);
		}
		else {
			xsmcGet(xsVar(0), xsGlobal, xsID_Number);
			s2 = &xsVar(0);
			if (s1->data[2] == s2->data[2]) {		//@@
				resolveBuffer(the, &xsVar(1), &srcData, NULL);
				xsResult = xsInteger(*(srcData + position));
				srcBytes = 1;
			}
			else {
				xsmcGet(xsVar(0), xsGlobal, xsID_ArrayBuffer);
				s2 = &xsVar(0);
				if (s1->data[2] == s2->data[2])	{	//@@
					xsmcSetArrayBuffer(xsResult, NULL, srcBytes);
					resolveBuffer(the, &xsVar(1), &srcData, NULL);
					c_memmove(xsmcToArrayBuffer(xsResult), srcData + position, srcBytes);
				}
				else
					xsUnknownError("unsupported output type");
			}
		}
	}

	position += srcBytes;
	xsmcSetInteger(xsVar(1), position);
	xsmcSet(xsVar(3), xsID_position, xsVar(1));
}

void xs_securesocket_write(xsMachine *the)
{
	int argc = xsmcArgc;
	uint8_t *dst;
	int available;
	uint16_t needed = 0, outputOffset = 0;
	unsigned char pass, arg;

	xsmcVars(3);

	xsmcGet(xsVar(2), xsThis, xsID_sock);
	xsResult = xsCall0(xsVar(2), xsID_write);

	available = xsmcToInteger(xsResult);
	if (available < 128)		// 128 is an estimate of TLS overhead
		available = 0;
	else
		available -= 128;
	if (0 == argc) {
		xsmcSetInteger(xsResult, available);
		return;
	}

	for (pass = 0; pass < 2; pass++ ) {
		if (1 == pass)
			xsmcSetArrayBuffer(xsVar(0), NULL, needed);

		for (arg = 0; arg < argc; arg++) {
			xsType t = xsmcTypeOf(xsArg(arg));

			if (xsStringType == t) {
				char *msg = xsmcToString(xsArg(arg));
				int msgLen = c_strlen(msg);
				if (0 == pass)
					needed += msgLen;
				else {
					dst = (uint8_t *)xsmcToArrayBuffer(xsVar(0)) + outputOffset;
					c_memcpy(dst, msg, msgLen);
					outputOffset += msgLen;
				}
			}
			else if ((xsNumberType == t) || (xsIntegerType == t)) {
				if (0 == pass)
					needed += 1;
				else {
					dst = (uint8_t *)xsmcToArrayBuffer(xsVar(0)) + outputOffset;
					*dst = (unsigned char)xsmcToInteger(xsArg(arg));
					outputOffset += 1;
				}
			}
			else if (xsReferenceType == t) {
				void *msg;
				xsUnsignedValue msgLen;

				xsmcGetBufferReadable(xsArg(arg), (void **)&msg, &msgLen);
				if (0 == pass)
					needed += msgLen;
				else {
					dst = (uint8_t *)xsmcToArrayBuffer(xsVar(0)) + outputOffset;
					c_memcpy(dst, msg, msgLen);
					outputOffset += msgLen;
				}
			}
			else
				xsUnknownError("unsupported type for write");
		}

		if ((0 == pass) && (needed > available))
			xsUnknownError("can't write all data");
	}

	xsmcGet(xsVar(1), xsThis, xsID_ssl);
	xsCall2(xsVar(1), xsID_write, xsVar(2), xsVar(0));
}
