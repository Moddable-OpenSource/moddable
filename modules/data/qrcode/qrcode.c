/*
 * Copyright (c) 2018-2022  Moddable Tech, Inc.
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
	uint8_t *qr0 = NULL;
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

		xsmcSetArrayBuffer(xsResult, NULL, size * size);

		xsmcSetInteger(xsVar(0), size);
		xsmcSet(xsResult, xsID_size, xsVar(0));

		module = xsmcToArrayBuffer(xsResult);
		for (y = 0; y < size; y++) {
			for (x = 0; x < size; x++)
				*module++ = qrcodegen_getModule(qr0, x, y);
		}

		c_free(qr0);
	}
	xsCatch {
		c_free(qr0);

		xsThrow(xsException);
	}
}
