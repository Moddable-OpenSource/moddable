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
	The PSA hash table of the ECMA-419 Digest, shared with the MAC and KeyDerivation classes
*/

#ifndef __DIGEST_PSA_H__
#define __DIGEST_PSA_H__

#include <stdint.h>
#include "psa/crypto.h"
#include "mc.defines.h"

typedef struct {
	const char *name;
	psa_algorithm_t alg;
	uint16_t outputSize;
	uint16_t blockSize;
} DigestAlgEntry;


// a build may omit algorithms (manifest "defines": crypt.digest.<name> 0); every one is present unless omitted
#ifndef MODDEF_CRYPT_DIGEST_MD5
	#define MODDEF_CRYPT_DIGEST_MD5 1
#endif
#ifndef MODDEF_CRYPT_DIGEST_SHA1
	#define MODDEF_CRYPT_DIGEST_SHA1 1
#endif
#ifndef MODDEF_CRYPT_DIGEST_SHA224
	#define MODDEF_CRYPT_DIGEST_SHA224 1
#endif
#ifndef MODDEF_CRYPT_DIGEST_SHA256
	#define MODDEF_CRYPT_DIGEST_SHA256 1
#endif
#ifndef MODDEF_CRYPT_DIGEST_SHA384
	#define MODDEF_CRYPT_DIGEST_SHA384 1
#endif
#ifndef MODDEF_CRYPT_DIGEST_SHA512
	#define MODDEF_CRYPT_DIGEST_SHA512 1
#endif

extern const DigestAlgEntry xs_crypt_digestAlgorithms[];		// NULL-named terminator
extern const DigestAlgEntry *xs_crypt_digestAlgorithm(const char *name);		// NULL if unknown

#endif
