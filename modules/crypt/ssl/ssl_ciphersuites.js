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

import config from "mc/config";
import {AES, CBC, DHE_RSA, ECDHE_RSA, GCM, RSA, SHA1, SHA256 /* , SHA384, TDES */} from "ssl/constants";

const supportedCipherSuites = [
	{
		// TLS_RSA_WITH_AES_128_CBC_SHA
		value: [0x00, 0x2f],
		isExportable: false,
		keyExchangeAlgorithm: RSA,
		cipherAlgorithm: AES,
		cipherKeySize: 16,
		cipherBlockSize: 16,
		hashAlgorithm: SHA1,
		hashSize: 20,
		encryptionMode: CBC,
	},
	{
		// TLS_RSA_WITH_AES_256_CBC_SHA
		value: [0x00, 0x35],
		isExportable: false,
		keyExchangeAlgorithm: RSA,
		cipherAlgorithm: AES,
		cipherKeySize: 32,
		cipherBlockSize: 16,
		hashAlgorithm: SHA1,
		hashSize: 20,
		encryptionMode: CBC,
	},
	{
		// TLS_RSA_WITH_AES_128_CBC_SHA256
		value: [0x00, 0x3c],
		isExportable: false,
		keyExchangeAlgorithm: RSA,
		cipherAlgorithm: AES,
		cipherKeySize: 16,
		cipherBlockSize: 16,
		hashAlgorithm: SHA256,
		hashSize: 32,
		encryptionMode: CBC,
	},
	{
		// TLS_RSA_WITH_AES_256_CBC_SHA256
		value: [0x00, 0x3d],
		isExportable: false,
		keyExchangeAlgorithm: RSA,
		cipherAlgorithm: AES,
		cipherKeySize: 32,
		cipherBlockSize: 16,
		hashAlgorithm: SHA256,
		hashSize: 32,
		encryptionMode: CBC,
	}
];
if (config.tls.DHE_RSA) {
	supportedCipherSuites.push(
		{
			// TLS_DHE_RSA_WITH_AES_128_CBC_SHA256
			value: [0x00, 0x67],
			isExportable: false,
			keyExchangeAlgorithm: DHE_RSA,
			cipherAlgorithm: AES,
			cipherKeySize: 16,
			cipherBlockSize: 16,
			hashAlgorithm: SHA256,
			hashSize: 32,
			encryptionMode: CBC,
		},
		{
			// TLS_DHE_RSA_WITH_AES_256_CBC_SHA256
			value: [0x00, 0x6b],
			isExportable: false,
			keyExchangeAlgorithm: DHE_RSA,
			cipherAlgorithm: AES,
			cipherKeySize: 32,
			cipherBlockSize: 16,
			hashAlgorithm: SHA256,
			hashSize: 32,
			encryptionMode: CBC,
		},
		{
			// TLS_DHE_RSA_WITH_AES_128_GCM_SHA256 (RFC 5288)
			value: [0x00, 0x9e],
			isExportable: false,
			keyExchangeAlgorithm: DHE_RSA,
			cipherAlgorithm: AES,
			cipherKeySize: 16,
			cipherBlockSize: 16,
			hashAlgorithm: SHA256,
			hashSize: 32,
			encryptionMode: GCM,
			ivSize: 8,	// explicit nonce size
			saltSize: 4,	// implicit part
		},
//		{
//			// TLS_DHE_RSA_WITH_AES_256_GCM_SHA384 (RFC 5288)
//			value: [0x00, 0x9f],
//			isExportable: false,
//			keyExchangeAlgorithm: DHE_RSA,
//			cipherAlgorithm: AES,
//			cipherKeySize: 32,
//			cipherBlockSize: 16,
//			hashAlgorithm: SHA384,
//			hashSize: 48,
//			encryptionMode: GCM,
//			ivSize: 8,	// explicit nonce size
//			saltSize: 4,	// implicit part
//		}
	);
}
if (config.tls.ECDHE_RSA) {
	supportedCipherSuites.push(
		{
			// TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256 (RFC844, 5289)
			value: [0xc0, 0x27],
			isExportable: false,
			keyExchangeAlgorithm: ECDHE_RSA,
			cipherAlgorithm: AES,
			cipherKeySize: 16,
			cipherBlockSize: 16,
			hashAlgorithm: SHA256,
			hashSize: 32,
			encryptionMode: CBC,
		},
		{
			// TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256 (RFC 8422)
			value: [0xc0, 0x2f],
			isExportable: false,
			keyExchangeAlgorithm: ECDHE_RSA,
			cipherAlgorithm: AES,
			cipherKeySize: 16,
			cipherBlockSize: 16,
			hashAlgorithm: SHA256,
			hashSize: 32,
			encryptionMode: GCM,
			ivSize: 8,	// explicit nonce size
			saltSize: 4,	// implicit part
		},
		{
			// TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256
			value: [0xc0, 0x2b],
			isExportable: false,
			keyExchangeAlgorithm: ECDHE_RSA,
			cipherAlgorithm: AES,
			cipherKeySize: 16,
			cipherBlockSize: 16,
			hashAlgorithm: SHA256,
			hashSize: 32,
			encryptionMode: GCM,
			ivSize: 8,	// explicit nonce size
			saltSize: 4,	// implicit part
		},
	);
}

//	{
//		// TLS_RSA_WITH_DES_CBC_SHA
//		value: [0x00, 0x09],
//		isExportable: false,
//		keyExchangeAlgorithm: RSA,
//		cipherAlgorithm: DES,
//		cipherKeySize: 8,
//		cipherBlockSize: 8,
//		hashAlgorithm: SHA1,
//		hashSize: 20,
//		encryptionMode: CBC,
//	},
//	{
//		// TLS_RSA_WITH_3DES_EDE_CBC_SHA
//		value: [0x00, 0x0A],
//		isExportable: false,
//		keyExchangeAlgorithm: RSA,
//		cipherAlgorithm: TDES,
//		cipherKeySize: 24,
//		cipherBlockSize: 8,
//		hashAlgorithm: SHA1,
//		hashSize: 20,
//		encryptionMode: CBC,
//	},
//	{
//		// TLS_RSA_WITH_RC4_128_MD5
//		value: [0x00, 0x04],
//		isExportable: false,
//		keyExchangeAlgorithm: RSA,
//		cipherAlgorithm: RC4,
//		cipherKeySize: 16,
//		cipherBlockSize: 0,
//		hashAlgorithm: MD5,
//		hashSize: 16,
//		encryptionMode: NONE,
//	},
//	{
//		// TLS_RSA_WITH_RC4_128_SHA
//		value: [0x00, 0x05],
//		isExportable: false,
//		keyExchangeAlgorithm: RSA,
//		cipherAlgorithm: RC4,
//		cipherKeySize: 16,
//		cipherBlockSize: 0,
//		hashAlgorithm: SHA1,
//		hashSize: 20,
//		encryptionMode: NONE,
//	},
//	{
//		// TLS_NULL_WITH_NULL_NULL
//		value: [0x00, 0x00],
//		isExportable: true,	// ?
//		keyExchangeAlgorithm: NULL,
//		cipherAlgorithm: NULL,
//		cipherKeySize: 0,
//		cipherBlockSize: 0,
//		hashAlgorithm: NULL,
//		hashSize: 0,
//		encryptionMode: NONE,
//	},

export default Object.freeze(supportedCipherSuites, true);
