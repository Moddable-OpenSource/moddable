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
	RSA keys through the Security framework, shared by the Signature and Cipher classes on macOS
	(CryptoKit has no RSA; SecKey is Apple's current RSA API)
*/

#ifndef __SECKEY_RSA_H__
#define __SECKEY_RSA_H__

#include <Security/Security.h>

// a SecKey from PKCS#1 DER, or NULL
extern SecKeyRef xs_crypt_rsaKey(const void *der, long derLength, int isPrivate, long bits);

// the digest algorithms SecKey names: index into these by hash name, -1 if unsupported (MD5 is not a SecKey digest)
extern int xs_crypt_rsaHashIndex(const char *name);
extern const uint16_t xs_crypt_rsaHashSizes[];

#endif
