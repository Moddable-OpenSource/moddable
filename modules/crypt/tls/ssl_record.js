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
import {CBC, GCM} from "ssl/constants";

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
				if (session.tls13?.suite)		// TLS 1.3: a compatibility record, dropped (section 5); after the keys are set tlsCipherText drops them
					break;
				changeCipherSpec.unpacketize(session, fragment);
				break;
			case recordProtocol.alert:
				SSLAlert.unpacketize(session, fragment);
				break;
			case recordProtocol.handshake: {		// the messages of the record; a message may continue in the next record
				let s;
				if (session.handshakeFragment) {
					const held = session.handshakeFragment;
					delete session.handshakeFragment;
					const joined = new Uint8Array(held.byteLength + fragment.byteLength);
					joined.set(held);
					joined.set(fragment, held.byteLength);
					s = new SSLStream(joined);
				}
				else
					s = new SSLStream(fragment);
				while (s.bytesAvailable > 0) {
					const available = s.bytesAvailable;
					if (available >= 4) {
						const header = s.readChunk(4, true);
						if (((header[1] << 16) | (header[2] << 8) | header[3]) <= s.bytesAvailable) {
							handshakeProtocol.unpacketize(session, new SSLStream(header), s);
							continue;
						}
						session.handshakeFragment = new Uint8Array(4 + s.bytesAvailable);		// incomplete
						session.handshakeFragment.set(header);
						session.handshakeFragment.set(s.readChunk(s.bytesAvailable, true), 4);
						break;
					}
					session.handshakeFragment = s.readChunk(available, true).slice(0);
					break;
				}
				} break;
			case recordProtocol.application_data:
				if (null !== session.handshakeDigests)		// the digests are closed when the handshake completes
					throw new TLSError("recordProtocol: application data before the handshake");
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
		calculateMac(hmac, seqNum, type, version, content, expected) {		// the MAC, or with expected, whether it matches (constant time)
			seqNum = ArrayBuffer.fromBigInt(seqNum);
			let stream = new SSLStream();
			for (let len = 8 - seqNum.byteLength; len > 0; len--)
				stream.writeChar(0);
			stream.writeChunk(seqNum);
			stream.writeChar(type);
			stream.writeChars(version, 2);
			stream.writeChars(content.byteLength, 2);
			hmac.write(stream.getChunk());
			hmac.write(content);
			const result = expected ? hmac.verify(expected) : hmac.read();		// read() leaves the message in place: reset for the next record
			hmac.reset();
			return result;
		},
		nonce13(iv, seqNum) {		// TLS 1.3 (section 5.3): the traffic IV xor the sequence number, right aligned
			const nonce = new Uint8Array(iv.slice(0));
			const seq = new Uint8Array(ArrayBuffer.fromBigInt(seqNum, 8));
			for (let i = 0; i < 8; i++)
				nonce[4 + i] ^= seq[i];
			return nonce;
		},
		aeadAdditionalData(seqNum, type, version, len) {
			const c = ArrayBuffer.fromBigInt(seqNum, 8);
			let tmps = new SSLStream(undefined, c.byteLength + 1 + 2 + 2);
			tmps.writeChunk(c);
			tmps.writeChar(type);
			tmps.writeChars(version, 2);
			tmps.writeChars(len, 2);
			return tmps.getChunk().buffer;
		},
		unpacketize(session, s) {		// a record from the server, under the server's keys once it has changed cipher spec
			session.traceProtocol(this);
			let type = s.readChar();
			let version = s.readChars(2);
			let fragmentLen = s.readChars(2);
			let fragment;
			const cipher = session.serverCipher;
			if (cipher && session.tls13?.suite) {		// TLS 1.3 (section 5.2): the whole record is one AEAD operation; the type is the last plaintext byte
				if (recordProtocol.change_cipher_spec === type) {		// a compatibility record stays unencrypted even now - during the handshake only
					if (null === session.handshakeDigests)
						throw new TLSError("recordProtocol: change_cipher_spec after the handshake");
					s.readChunk(fragmentLen);
					return;
				}
				const header = Uint8Array.of(type, version >> 8, version & 0xff, fragmentLen >> 8, fragmentLen & 0xff);
				fragment = s.readChunk(fragmentLen, true);
				cipher.enc.reset({iv: this.nonce13(cipher.iv, session.readSeqNum), additionalData: header});
				let n = cipher.enc.decrypt(fragment, {output: fragment});		// throws on authentication failure
				session.readSeqNum++;
				while ((n > 0) && (0 === fragment[n - 1]))		// zero padding
					n--;
				if (!n)
					throw new TLSError("recordProtocol: empty record");
				return recordProtocol.tlsCompressed.unpacketize(session, fragment[n - 1], fragment.subarray(0, n - 1));
			}
			if (cipher) {
				switch (session.chosenCipher.encryptionMode) {
				case CBC: {		// explicit IV || ciphertext of (content || MAC || padding)
					const cipherBlockSize = session.chosenCipher.cipherBlockSize;
					const hashSize = session.chosenCipher.hashSize;
					cipher.enc.reset({iv: s.readChunk(cipherBlockSize)});
					fragmentLen -= cipherBlockSize;
					fragment = s.readChunk(fragmentLen, true);
					cipher.enc.decrypt(fragment, {output: fragment});
					fragmentLen -= hashSize + fragment[fragment.byteLength - 1] + 1;
					const mac = fragment.slice(fragmentLen, fragmentLen + hashSize).buffer;
					fragment = new Uint8Array(fragment.buffer, fragment.byteOffset, fragmentLen);
					if (!this.calculateMac(cipher.hmac, session.readSeqNum, type, version, fragment, mac))
						throw new TLSError("recordProtocol: auth failed");
					} break;
				case GCM: {
					let nonce = s.readChunk(session.chosenCipher.ivSize);
					fragmentLen -= session.chosenCipher.ivSize;
					nonce = cipher.iv.concat(nonce);
					fragment = s.readChunk(fragmentLen, true);
					cipher.enc.reset({iv: nonce, additionalData: this.aeadAdditionalData(session.readSeqNum, type, version, fragmentLen - cipher.enc.tagLength)});
					fragment = fragment.subarray(0, cipher.enc.decrypt(fragment, {output: fragment}));		// throws on authentication failure
					} break;
				}
				session.readSeqNum++;
			}
			else
				fragment = s.readChunk(fragmentLen, true);

			return recordProtocol.tlsCompressed.unpacketize(session, type, fragment);		// tail call optimization
		},
		packetize(session, type, fragment) {		// a record to the server, under our keys once we have changed cipher spec
			session.traceProtocol(this);
			const cipher = session.clientCipher;
			let prefix;
			if (cipher && session.tls13?.suite) {		// TLS 1.3: content || type, under the record header as additional data
				const inner = new Uint8Array(fragment.byteLength + 1);
				inner.set((fragment instanceof Uint8Array) ? fragment : new Uint8Array(fragment));
				inner[fragment.byteLength] = type;
				type = recordProtocol.application_data;
				const length = inner.byteLength + cipher.enc.tagLength;
				cipher.enc.reset({iv: this.nonce13(cipher.iv, session.writeSeqNum), additionalData: Uint8Array.of(type, 3, 3, length >> 8, length & 0xff)});
				fragment = cipher.enc.encrypt(inner);
				session.writeSeqNum++;
			}
			else if (cipher) {
				switch (session.chosenCipher.encryptionMode) {
				case CBC: {		// explicit IV || ciphertext of (content || MAC || padding)
					const mac = this.calculateMac(cipher.hmac, session.writeSeqNum, type, session.protocolVersion, fragment);
					const blksz = session.chosenCipher.cipherBlockSize;
					const stream = new SSLStream(undefined, fragment.byteLength + mac.byteLength + blksz);
					stream.writeChunk(fragment);
					stream.writeChunk(mac);
					let padSize = (stream.bytesWritten + 1) % blksz;
					if (padSize > 0)
						padSize = blksz - padSize;
					for (let i = 0; i <= padSize; i++)		// padSize bytes of padding and the length byte, all padSize
						stream.writeChar(padSize);
					prefix = session.random.read(blksz);
					cipher.enc.reset({iv: prefix});
					fragment = stream.getChunk();
					cipher.enc.encrypt(fragment, {output: fragment});
					}
					break;
				case GCM: {
					let explicit_nonce = ArrayBuffer.fromBigInt(cipher.nonce, session.chosenCipher.ivSize);
					cipher.nonce++;
					let nonce = cipher.iv.concat(explicit_nonce);
					cipher.enc.reset({iv: nonce, additionalData: this.aeadAdditionalData(session.writeSeqNum, type, session.protocolVersion, fragment.byteLength)});
					fragment = cipher.enc.encrypt(fragment);
					prefix = explicit_nonce;
					}
					break;
				}
				session.writeSeqNum++;
			}

			const stream = new SSLStream(undefined, 1 + 2 + 2 + (prefix?.byteLength ?? 0) + fragment.byteLength);
			stream.writeChar(type);
			stream.writeChars(session.protocolVersion, 2);
			stream.writeChars(fragment.byteLength + (prefix?.byteLength ?? 0), 2);
			if (prefix)
				stream.writeChunk(prefix);
			stream.writeChunk(fragment);
			return stream.getChunk();
		},
	},
}

Object.freeze(recordProtocol, true);

export default recordProtocol;
