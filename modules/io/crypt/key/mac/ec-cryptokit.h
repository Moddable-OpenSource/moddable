/*
 * Copyright (c) 2026  Moddable Tech, Inc.
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

/*
	The curve table of the ECMA-419 Key on macOS, shared with the Signature and KeyAgreement classes
*/

#ifndef __EC_CRYPTOKIT_H__
#define __EC_CRYPTOKIT_H__

#include <stdint.h>

typedef struct {
	const char *name;
	int32_t curve;			// the ckCurve* value
	uint8_t scalarBytes;	// private scalar, and the shared secret
	uint8_t pointBytes;		// public point as the key family encodes it
} CurveEntry;

extern const CurveEntry *xs_crypt_curve(const char *name);		// NULL if unknown or omitted

#endif
