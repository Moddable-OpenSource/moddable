/*
 * Copyright (c) 2018-2026  Moddable Tech, Inc.
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
#include "mc.xs.h"			// for xsID_ values

#include "qrcodegen.h"
#include "xsHost.h"

void xs_qrcode(xsMachine *the)
{
	size_t bufferSize;
	uint8_t *qr0 = C_NULL;
	void *data;
	xsType type;
	uint8_t ok;
	int x, y;
	uint8_t *module;
	int maxVersion = qrcodegen_VERSION_MAX;

	xsmcVars(3);

	if (xsmcHas(xsArg(0), xsID_maxVersion)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_maxVersion);
		maxVersion = xsmcToInteger(xsVar(0));
		if ((maxVersion > qrcodegen_VERSION_MAX) || (maxVersion < qrcodegen_VERSION_MIN))
			xsRangeError("invalid");
	}

	uint8_t padding = 0;
	if (xsmcHas(xsArg(0), xsID_bitmap)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_bitmap);
		if (xsBooleanType == xsmcTypeOf(xsVar(0))) {
			if (xsmcTest(xsVar(0)))
				padding = 8;
		}
		else {
			int t = xsmcToInteger(xsVar(0));
			if ((8 != t) && (16 != t) && (32 != t))
				xsRangeError("invalid");
			padding = (uint8_t)t;
		}
	}

	uint16_t fit = 0;
	if (padding && xsmcHas(xsArg(0), xsID_fit)) {
		xsmcGet(xsVar(0), xsArg(0), xsID_fit);
		int t = xsmcToInteger(xsVar(0));
		if (t <= 0)
			xsRangeError("invalid");
		fit = (uint16_t)t;
	}
	
	bufferSize = qrcodegen_BUFFER_LEN_FOR_VERSION(maxVersion);
	qr0 = c_malloc(bufferSize << 1);
	if (!qr0)
		xsUnknownError("no memory");

	xsTry {
		xsmcGet(xsVar(0), xsArg(0), xsID_input);
		type = xsmcTypeOf(xsVar(0));
		if (xsStringType == type) {
			data = xsmcToString(xsVar(0));
			ok = qrcodegen_encodeText(data, qr0 + bufferSize, qr0,
					qrcodegen_Ecc_MEDIUM, qrcodegen_VERSION_MIN, maxVersion, qrcodegen_Mask_AUTO, true);
		}
		else {
			xsUnsignedValue dataSize;

			xsmcGetBufferReadable(xsVar(0), &data, &dataSize);
			if (dataSize > bufferSize)
				xsUnknownError("invalid");
			c_memcpy(qr0 + bufferSize, data, dataSize);
			ok = qrcodegen_encodeBinary(qr0 + bufferSize, dataSize, qr0,
					qrcodegen_Ecc_MEDIUM, qrcodegen_VERSION_MIN, maxVersion, qrcodegen_Mask_AUTO, true);
		}

		if (!ok)
			xsUnknownError("qrcode failed");

		int size = qrcodegen_getSize(qr0);
		if (0 == padding) {
			xsmcSetArrayBuffer(xsResult, C_NULL, size * size);

			module = xsmcToArrayBuffer(xsResult);
			for (y = 0; y < size; y++) {
				for (x = 0; x < size; x++)
					*module++ = qrcodegen_getModule(qr0, x, y);
			}
		}
		else {
			int scale = fit ? (fit / size) : 1;
			if (scale < 1)
				xsUnknownError("can't fit");
			int scaledSize = size * scale;
			int rowBytes = ((scaledSize + (padding - 1)) & ~(padding - 1)) >> 3;
			xsmcSetArrayBuffer(xsResult, C_NULL, rowBytes * scaledSize);

			module = xsmcToArrayBuffer(xsResult);
			for (y = 0; y < size; y++, module += rowBytes * scale) {
				uint8_t *line = module;
				for (x = 0; x < size; x++) {
					if (qrcodegen_getModule(qr0, x, y)) {
						for (int j = 0, xOut = x * scale; j < scale; j++, xOut++)
							line[xOut >> 3] |= 1 << (xOut & 7);
					}
				}
				for (x = 1; x < scale; x++)
					c_memcpy(module + rowBytes * x, module, rowBytes);
			}
			size = scaledSize;
		}

		xsmcSetInteger(xsVar(0), size);
		xsmcSet(xsResult, xsID_size, xsVar(0));

		c_free(qr0);
	}
	xsCatch {
		c_free(qr0);

		xsThrow(xsException);
	}
}
