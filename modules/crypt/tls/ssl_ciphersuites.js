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
import {AES, CBC, ECDHE_ECDSA, ECDHE_RSA, GCM, RSA, SHA1, SHA256, SHA384} from "ssl/constants";

// in order of preference: ephemeral elliptic-curve Diffie-Hellman (forward secrecy) first, then static RSA, the cheapest
// exchange but without forward secrecy, kept last for servers that offer nothing else
const supportedCipherSuites = [];
if (config.tls.ECDHE_RSA) {
	supportedCipherSuites.push(
		{
			// TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256 (RFC844, 5289)
			value: [0xc0, 0x27],
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
			// TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384 (RFC 5289)
			value: [0xc0, 0x30],
			keyExchangeAlgorithm: ECDHE_RSA,
			cipherAlgorithm: AES,
			cipherKeySize: 32,
			cipherBlockSize: 16,
			hashAlgorithm: SHA384,
			hashSize: 48,
			encryptionMode: GCM,
			ivSize: 8,	// explicit nonce size
			saltSize: 4,	// implicit part
		},
		{
			// TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256: offered only when the build can verify the ECDSA chains dual-certificate
			// servers then send, whose roots are P-384 (see supportedSuites in ssl/handshake)
			value: [0xc0, 0x2b],
			keyExchangeAlgorithm: ECDHE_ECDSA,
			cipherAlgorithm: AES,
			cipherKeySize: 16,
			cipherBlockSize: 16,
			hashAlgorithm: SHA256,
			hashSize: 32,
			encryptionMode: GCM,
			ivSize: 8,	// explicit nonce size
			saltSize: 4,	// implicit part
		}
	);
}
supportedCipherSuites.push(
	{
		// TLS_RSA_WITH_AES_128_CBC_SHA
		value: [0x00, 0x2f],
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
		keyExchangeAlgorithm: RSA,
		cipherAlgorithm: AES,
		cipherKeySize: 32,
		cipherBlockSize: 16,
		hashAlgorithm: SHA256,
		hashSize: 32,
		encryptionMode: CBC,
	}
);

export default Object.freeze(supportedCipherSuites, true);
