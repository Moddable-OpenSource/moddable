/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
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

import PRF from "ssl/prf";
import MAC from "embedded:crypt/MAC";
import SSLStream from "ssl/stream";
import TLSError from "ssl/error";
import Cipher from "embedded:crypt/cipher";
import {AES, CBC, GCM, SHA1, SHA256, SHA384} from "ssl/constants";

function setupSub(o, cipher, direction)
{
	if (AES !== cipher.cipherAlgorithm)
		throw new TLSError("SetupCipher: unsupported cipher algorithm");
	switch (cipher.encryptionMode) {
	case CBC: {
		let h;
		switch (cipher.hashAlgorithm) {
		case SHA1: h = "SHA-1"; break;
		case SHA256: h = "SHA-256"; break;
		case SHA384: h = "SHA-384"; break;
		default:
			throw new TLSError("SetupCipher: unknown hash algorithm");
		}
		o.hmac = new MAC({algorithm: "HMAC", hash: h, key: o.macSecret});
		// no padding: TLS pads the record itself; the IV is explicit in every record
		o.enc = new Cipher({algorithm: "AES-CBC", key: o.key, direction, iv: new Uint8Array(16)});
		} break;
	case GCM:
		// the nonce (salt || explicit) and the additional data are set per record
		o.enc = new Cipher({algorithm: "AES-GCM", key: o.key, direction, iv: new Uint8Array(12), tagLength: 16});
		o.nonce = BigInt(1);
		break;
	default:
		throw new TLSError("SetupCipher: unsupported encryption mode");
	}
}

// the key block is client MAC, server MAC, client key, server key, client IV, server IV (RFC 5246 section 6.3);
// client: our write keys (encrypt), otherwise the server's (decrypt)
function SetupCipher(session, client)
{
	const chosenCipher = session.chosenCipher;
	const macSize = (GCM === chosenCipher.encryptionMode) ? 0 : chosenCipher.hashSize;
	const keySize = chosenCipher.cipherKeySize;
	const ivSize = chosenCipher.saltSize ?? 0;		// GCM's implicit nonce; a CBC record carries its own IV
	const keyBlock = PRF(session, session.masterSecret, "key expansion", session.serverRandom.concat(session.clientRandom), (macSize + keySize + ivSize) * 2);
	const s = new SSLStream(keyBlock);
	const o = {};
	if (macSize > 0) {
		if (client) {
			o.macSecret = s.readChunk(macSize);
			void s.readChunk(macSize);
		}
		else {
			void s.readChunk(macSize);
			o.macSecret = s.readChunk(macSize);
		}
	}
	if (client) {
		o.key = s.readChunk(keySize);
		void s.readChunk(keySize);
	}
	else {
		void s.readChunk(keySize);
		o.key = s.readChunk(keySize);
	}
	if (ivSize > 0) {
		if (client)
			o.iv = s.readChunk(ivSize);
		else {
			void s.readChunk(ivSize);
			o.iv = s.readChunk(ivSize);
		}
	}
	setupSub(o, chosenCipher, client ? "encrypt" : "decrypt");
	if (client)
		session.clientCipher = o;
	else
		session.serverCipher = o;
}

export default SetupCipher;
