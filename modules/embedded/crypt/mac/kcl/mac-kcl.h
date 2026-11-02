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
	HMAC core over the KCL hash primitives, shared by the MAC and KeyDerivation classes
*/

#ifndef __MAC_KCL_H__
#define __MAC_KCL_H__

#include "digest-kcl.h"

typedef struct {
	uint8_t alg;
	uint8_t reserved;
	uint16_t outputSize;
	uint16_t blockSize;
	uint16_t ctxSize;
	union DigestCtx ctx[];		// [0] running inner, [1] inner initial state, [2] outer initial state; each ctxSize bytes
} HMACRecord, *HMAC;

#define xs_crypt_hmacContext(h, i) ((union DigestCtx *)((uint8_t *)(h)->ctx + (i) * (h)->ctxSize))

extern uint32_t xs_crypt_hmacSize(const DigestAlgEntry *entry);		// bytes of record for this hash
extern void xs_crypt_hmacInit(HMAC h, const DigestAlgEntry *entry, const void *key, uint32_t keyLen);
extern void xs_crypt_hmacUpdate(HMAC h, const void *data, uint32_t size);
extern void xs_crypt_hmacFinish(HMAC h, uint8_t *out);		// non-destructive: the running state is cloned
extern void xs_crypt_hmacReset(HMAC h);

#endif
