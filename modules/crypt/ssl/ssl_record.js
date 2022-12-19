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

import handshakeProtocol from "ssl/handshake";
import changeCipherSpec from "ssl/changecipher";
import SSLStream from "ssl/stream";
import SSLAlert from "ssl/alert";
import TLSError from "ssl/error";
import Bin from "bin";
import RNG from "rng";
import {CBC, GCM, NONE} from "ssl/constants";

const recordProtocol = {
	name: "recordProtocol",
	// global constants
	change_cipher_spec: 20,
	alert: 21,
	handshake: 22,
	application_data: 23,

	// protocols
	unpacketize(session, buf) {
		session.traceProtocol(this);
		return this.tlsCipherText.unpacketize(session, new SSLStream(buf));		// tail call optimization
	},
	packetize(session, type, fragment) {
		session.traceProtocol(this);
		return this.tlsPlainText.packetize(session, type, fragment);
	},

	tlsPlainText: {
		name: "tlsPlainText",
		unpacketize(session, type, fragment) {
			session.traceProtocol(this);
			switch (type) {
			case recordProtocol.change_cipher_spec:
				changeCipherSpec.unpacketize(session, fragment);
				break;
			case recordProtocol.alert:
				SSLAlert.unpacketize(session, fragment);
				break;
			case recordProtocol.handshake:
				var s = new SSLStream(fragment);
				while (s.bytesAvailable > 0)
					handshakeProtocol.unpacketize(session, s);
				break;
			case recordProtocol.application_data:
				session.putData(fragment);
				break;
			default:
				throw new TLSError("recordProtocol: bad data");
			}
		},
		packetize(session, type, fragment) {
			session.traceProtocol(this);
			return recordProtocol.tlsCompressed.packetize(session, type, fragment);
		},
	},

	tlsCompressed: {
		name: "tlsCompressed",
		unpacketize(session, type, fragment) {
			session.traceProtocol(this);
			// unsupported -- just pass through
			return recordProtocol.tlsPlainText.unpacketize(session, type, fragment);		// tail call optimization
		},
		packetize(session, type, fragment) {
			session.traceProtocol(this);
			// unsupported -- just pass through
			return recordProtocol.tlsCipherText.packetize(session, type, fragment);
		},
	},

	tlsCipherText: {
		name: "tlsCipherText",
		calculateMac(hmac, seqNum, type, version, content) {
			hmac.reset();
			seqNum = ArrayBuffer.fromBigInt(seqNum);
			let stream = new SSLStream();
			for (let len = 8 - seqNum.byteLength; len > 0; len--)
				stream.writeChar(0);
			stream.writeChunk(seqNum);
			stream.writeChar(type);
			stream.writeChars(version, 2);
			stream.writeChars(content.byteLength, 2);
			hmac.update(stream.getChunk());
			hmac.update(content);
			return hmac.close();
		},
		aeadAdditionalData(seqNum, type, version, len) {
			let tmps = new SSLStream();
			let c = ArrayBuffer.fromBigInt(seqNum, 8);
			tmps.writeChunk(c);
			tmps.writeChar(type);
			tmps.writeChars(version, 2);
			tmps.writeChars(len, 2);
			return tmps.readChunk(tmps.bytesAvailable);
		},
		unpacketize(session, s) {
			session.traceProtocol(this);
			let type = s.readChar();
			let version = s.readChars(2);
			let fragmentLen = s.readChars(2);
			let fragment;
			let cipher = session.connectionEnd ? session.serverCipher : session.clientCipher;
			if (cipher) {
				switch (session.chosenCipher.encryptionMode) {
				case NONE:
				case CBC: {
					let cipherBlockSize = session.chosenCipher.cipherBlockSize;
					let hashSize = session.chosenCipher.hashSize;
					if (version >= 0x302 && cipherBlockSize > 0) { // 3.2 or higher && block cipher
						let iv = s.readChunk(cipherBlockSize);
						cipher.enc.setIV(iv);
						fragmentLen -= cipherBlockSize;
					}
					fragment = s.readChunk(fragmentLen, true);
					cipher.enc.decrypt(fragment, fragment);
					let padLen = cipherBlockSize ? (fragment[fragment.byteLength - 1] + 1) : 0;
					fragmentLen -= hashSize + padLen;
					let mac = fragment.slice(fragmentLen, fragmentLen + hashSize).buffer;
					if (fragment.byteLength > fragmentLen)
						fragment = new Uint8Array(fragment.buffer, fragment.byteOffset, fragmentLen);
					if (cipher.hmac) {
						if (Bin.comp(mac, this.calculateMac(cipher.hmac, session.readSeqNum, type, version, fragment)) != 0)
							throw new TLSError("recordProtocol: auth failed");
					}
					} break;
				case GCM: {
					let nonce = s.readChunk(session.chosenCipher.ivSize);
					fragmentLen -= session.chosenCipher.ivSize;
					nonce = cipher.iv.concat(nonce);
					fragment = s.readChunk(fragmentLen, true);
					const additional_data = this.aeadAdditionalData(session.readSeqNum, type, version, fragmentLen - cipher.enc.tagLength);
					if (!(fragment = cipher.enc.process(fragment, fragment, nonce, additional_data, false))) {
						// @@ should send an alert
						throw new TLSError("recordProtocol auth failed");
					}
					} break;
				}
				session.readSeqNum++;
			}
			else
				fragment = s.readChunk(fragmentLen, true);

			return recordProtocol.tlsCompressed.unpacketize(session, type, fragment);		// tail call optimization
		},
		packetize(session, type, fragment) {
			session.traceProtocol(this);
			let cipher = session.connectionEnd ? session.clientCipher : session.serverCipher;
			let stream;
			if (cipher) {
				switch (session.chosenCipher.encryptionMode) {
				case NONE:
				case CBC: {
					let mac = this.calculateMac(cipher.hmac, session.writeSeqNum, type, session.protocolVersion, fragment);
					let blksz = session.chosenCipher.cipherBlockSize, iv;
					let stream = new SSLStream();
					stream.writeChunk(fragment);
					stream.writeChunk(mac);
					if (blksz) {
						let length = stream.bytesWritten + 1;
						let padSize = length % blksz;
						if (padSize > 0)
							padSize = blksz - padSize;
						for (let i = 0; i < padSize; i++)
							stream.writeChar(padSize);
						stream.writeChar(padSize);
					}
					if (session.protocolVersion >= 0x302 && blksz) { // 3.2 or higher && block cipher
						iv = RNG.get(blksz);
						cipher.enc.setIV(iv);
					}
					fragment = cipher.enc.encrypt(stream.getChunk());
					if (iv)
						fragment = iv.concat(fragment);
					}
					break;
				case GCM: {
					let explicit_nonce = ArrayBuffer.fromBigInt(cipher.nonce, session.chosenCipher.ivSize);
					cipher.nonce++;
					let nonce = cipher.iv.concat(explicit_nonce);
					let additional_data = this.aeadAdditionalData(session.writeSeqNum, type, session.protocolVersion, fragment.byteLength);
					fragment = cipher.enc.process(fragment, null, nonce, additional_data, true);
					fragment = explicit_nonce.concat(fragment);
					}
					break;
				}
				session.writeSeqNum++;
			}
			stream = new SSLStream();
			stream.writeChar(type);
			stream.writeChars(session.protocolVersion, 2);
			stream.writeChars(fragment.byteLength, 2);
			stream.writeChunk(fragment);
			return stream.getChunk();
		},
	},
}

Object.freeze(recordProtocol, true);

export default recordProtocol;
