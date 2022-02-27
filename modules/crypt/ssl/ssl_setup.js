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
import HMAC from "hmac";
import SSLStream from "ssl/stream";
import TLSError from "ssl/error";
import {BlockCipher, Digest, Mode, StreamCipher} from "crypt";
import {AES, CBC, DES, GCM, MD5, NONE, RC4, SHA1, SHA256, SHA384, TDES} from "ssl/constants";
import Gcm from "gcm";

function setupSub(o, cipher)
{
	let enc, h;

	switch (cipher.cipherAlgorithm) {
	case DES:
		enc = new BlockCipher("DES", o.key);
		break;
	case TDES:
		enc = new BlockCipher("TDES", o.key);
		break;
	case AES:
		enc = new BlockCipher("AES", o.key);
		break;
	case RC4:
		enc = new StreamCipher("RC4", o.key);
		break;
	default:
		throw new TLSError("SetupCipher: unkown encryption algorithm");
	}
	switch (cipher.encryptionMode) {
	case CBC:
	case NONE:
		switch (cipher.hashAlgorithm) {
		case MD5: h = "MD5"; break;
		case SHA1: h = "SHA1"; break;
		case SHA256: h = "SHA256"; break;
		case SHA384: h = "SHA384"; break;
		default:
			throw new TLSError("SetupCipher: unknown hash algorithm");
		}
		o.hmac = new HMAC(new Digest(h), o.macSecret);
		if (cipher.encryptionMode == CBC)
			o.enc = new Mode("CBC", enc, o.iv);	// no padding -- SSL 3.2 requires padding process beyond RFC2630
		else
			o.enc = enc;
		break;
	case GCM:
		o.enc = new Gcm(enc);
		o.nonce = BigInt(1);
		break;
	default:
		o.enc = enc;
		break;
	}
}

function SetupCipher(session, connectionEnd)
{
	var random = session.serverRandom;
	random = random.concat(session.clientRandom);
	var chosenCipher = session.chosenCipher;
	var macSize = chosenCipher.encryptionMode == GCM ? 0 : chosenCipher.hashSize;
	var ivSize = session.protocolVersion <= 0x301 ? chosenCipher.cipherBlockSize : (chosenCipher.saltSize || 0);
	var nbytes = chosenCipher.cipherKeySize * 2 + macSize * 2 + ivSize * 2;
	var keyBlock = PRF(session, session.masterSecret, "key expansion", random, nbytes);
	var s = new SSLStream(keyBlock);
	var o = {};
	if (connectionEnd) {
		if (macSize > 0) {
			o.macSecret = s.readChunk(macSize);
			void s.readChunk(macSize);
		}
		o.key = s.readChunk(chosenCipher.cipherKeySize);
		void s.readChunk(chosenCipher.cipherKeySize);
		if (ivSize > 0)
			o.iv = s.readChunk(ivSize);
		else
			o.iv = undefined;
		setupSub(o, chosenCipher);
		session.clientCipher = o;
	}
	else {
		if (macSize > 0) {
			void s.readChunk(macSize);
			o.macSecret = s.readChunk(macSize);
		}
		void s.readChunk(chosenCipher.cipherKeySize);
		o.key = s.readChunk(chosenCipher.cipherKeySize);
		if (ivSize > 0) {
			void s.readChunk(ivSize);
			o.iv = s.readChunk(ivSize);
		}
		else
			o.iv = undefined;
		setupSub(o, chosenCipher);
		session.serverCipher = o;
	}
}

export default SetupCipher;
