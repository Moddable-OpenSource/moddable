/*
 * Copyright (c) 2016-2026  Moddable Tech, Inc.
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
#include "mc.xs.h"      // for xsID_ values

void xs_crypt_bin_xor(xsMachine *the)
{
	xsUnsignedValue sizeA, sizeB, sizeR, i;
	uint8_t *dataA, *dataB, *dataR;

	xsmcGetBufferReadable(xsArg(0), (void **)&dataA, &sizeA);
	xsmcSetArrayBuffer(xsResult, C_NULL, sizeA);
	
	xsmcGetBufferReadable(xsArg(0), (void **)&dataA, &sizeA);
	xsmcGetBufferReadable(xsArg(1), (void **)&dataB, &sizeB);
	xsmcGetBufferWritable(xsResult, (void **)&dataR, &sizeR);

	if (sizeB < sizeA)
		xsUnknownError("mismatch");

	for (i = 0; i < sizeA; i++)
		dataR[i] = c_read8(&dataA[i]) ^ c_read8(&dataB[i % sizeB]);
}

void xs_crypt_bin_comp(xsMachine *the)
{
	xsUnsignedValue sizeA, sizeB;
	uint8_t *dataA, *dataB;
	int i, n = (xsmcArgc > 2) ? xsmcToInteger(xsArg(2)) : -1, result = 0;

	xsmcGetBufferReadable(xsArg(0), (void **)&dataA, &sizeA);
	xsmcGetBufferReadable(xsArg(1), (void **)&dataB, &sizeB);

	if (n >= 0) {
		if ((int)sizeA < n && (int)sizeB < n)
			;	// fall thru
		else if ((int)sizeA < n) {
			result = -1;
			goto done;
		}
		else if ((int)sizeB < n) {
			result = +1;
			goto done;
		}
	}
	else {
		if (sizeA > sizeB) {
			result = +1;
			goto done;
		}
		else if (sizeA < sizeB) {
			result = -1;
			goto done;
		}
		n = sizeA;
	}
	for (i = 0; i < n; i++) {
		if (c_read8(&dataA[i]) != c_read8(&dataB[i])) {
			result = c_read8(&dataA[i]) - c_read8(&dataB[i]);
			break;
		}
	}

done:
	xsmcSetInteger(xsResult, result);
}
