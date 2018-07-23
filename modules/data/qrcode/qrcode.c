/*
 * Copyright (c) 2018  Moddable Tech, Inc.
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

#ifdef __ets__
	#include "xsesp.h"
#endif

void xs_qrcode(xsMachine *the)
{
	uint8_t qr0[qrcodegen_BUFFER_LEN_MAX];
	uint8_t tempBuffer[qrcodegen_BUFFER_LEN_MAX];
	void *data;
	xsType type;
	uint8_t ok;
	int x, y;
	uint8_t *module;

	xsmcVars(3);

	xsmcGet(xsVar(0), xsArg(0), xsID_input);
	type = xsmcTypeOf(xsVar(0));
	if (xsStringType == type) {
		data = xsmcToString(xsVar(0));

		ok = qrcodegen_encodeText(data, tempBuffer, qr0,
				qrcodegen_Ecc_MEDIUM, qrcodegen_VERSION_MIN, qrcodegen_VERSION_MAX, qrcodegen_Mask_AUTO, true);
	}
	else {
		unsigned int dataSize;

		if (xsmcIsInstanceOf(xsVar(0), xsArrayBufferPrototype)) {
			data = xsmcToArrayBuffer(xsVar(0));
			dataSize = xsGetArrayBufferLength(xsVar(0));
		}
		else {
			data = xsmcGetHostData(xsVar(0));
			xsmcGet(xsVar(0), xsVar(0), xsID_byteLength);
			dataSize = xsmcToInteger(xsVar(0));
		}

		if (dataSize > sizeof(tempBuffer))
			xsUnknownError("input too big");
		c_memcpy(tempBuffer, data, dataSize);

		ok = qrcodegen_encodeBinary(tempBuffer, dataSize, qr0,
				qrcodegen_Ecc_MEDIUM, qrcodegen_VERSION_MIN, qrcodegen_VERSION_MAX, qrcodegen_Mask_AUTO, true);
	}

	if (!ok)
		xsUnknownError("qtcode failed");

	int size = qrcodegen_getSize(qr0);

	xsResult = xsArrayBuffer(NULL, size * size);

	xsmcSetInteger(xsVar(0), size);
	xsmcSet(xsResult, xsID_size, xsVar(0));

	module = xsmcToArrayBuffer(xsResult);
	for (y = 0; y < size; y++) {
		for (x = 0; x < size; x++)
			*module++ = qrcodegen_getModule(qr0, x, y);
	}
}
