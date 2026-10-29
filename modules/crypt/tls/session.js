/*
 * Copyright (c) 2016-2026  Moddable Tech, Inc.
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

import recordProtocol from "ssl/record";
import handshakeProtocol from "ssl/handshake";
import changeCipherSpec from "ssl/changecipher";
import SSLAlert from "ssl/alert";
import cacheManager from "ssl/cache";
import CertificateManager from "ssl/cert";
import TLSError from "ssl/error";
import Bin from "bin";
import SSLStream from "ssl/stream";
import Random from "embedded:crypt/random";
import Modules from "modules";
import {protocolVersion} from "ssl/constants";
const TLS13 = "ssl/handshake13";
import {ECDHE_ECDSA, ECDHE_RSA, RSA, AES, CBC, GCM, SHA1, SHA256, SHA384} from "ssl/constants";

const maxFragmentSize = 16384	// maximum record layer framgment size (not a packet size): 2^14

function closeCipher(o) {
	if (o) {
		o.enc?.close();
		o.hmac?.close();
	}
}

// a TLS 1.2 client session
class SSLSession {
	static get maximumVersion() {		// TLS 1.3 when ssl/handshake13 is in the build
		return Modules.has(TLS13) ? 0x304 : protocolVersion;
	}
	#buffer;
	constructor(options) {
		options = {...options};	// shallow copy so it can be safely modified
		const maximum = SSLSession.maximumVersion;
		if (((options.minProtocolVersion ?? protocolVersion) > maximum) || ((options.maxProtocolVersion ?? maximum) < protocolVersion))
			throw new TLSError((maximum > protocolVersion) ? "TLS 1.2 and 1.3 are the only versions supported" : "TLS 1.2 is the only version supported");
		this.options = options;
		this.packetBuffer = null;
		this.handshakeMessages = undefined;
		this.handshakeDigests = undefined;
		this.clientSessionID = this.serverSessionID = undefined;
		this.clientCerts = undefined;
		this.myCert = undefined;
		this.handshakeProcess = -1;
		this.clientCipher = null;
		this.serverCipher = null;
		this.random = new Random({});
		this.alert = undefined;
		this.protocolVersion = protocolVersion;
		if (this.options.trace)
			this.traceLevel = 0;
//		this.applicationData = undefined;
		this.cacheManager = (this.options.cache === undefined || this.options.cache) && cacheManager;

		const certificates = options.certificate;		// may be a single certificate or an array of them
		if (Array.isArray(certificates))
			delete options.certificate;
		this.certificateManager = new CertificateManager(options);
		if (Array.isArray(certificates)) {
			for (let i = 0; i < certificates.length; i++)
				this.certificateManager.register(certificates[i]);
		}
	}
	initiateHandshake() {
		this.handshakeProcess = handshakeProtocol.helloRequest.msgType;
	}
	handshake(s) {
		let state = 0;
		switch (this.handshakeProcess) {
		case handshakeProtocol.helloRequest.msgType: {
			let cache = this.cacheManager && this.cacheManager.getByHost(this.options.tls_server_name);
			if (cache) {
				this.clientSessionID = cache.id;
				this.masterSecret = cache.secret;
			}
			handshakeProtocol.clientHello.prepare(this);
			this.doProtocol(s, handshakeProtocol.clientHello);
			} break;
		case handshakeProtocol.serverHelloDone.msgType:
			if (this.clientCerts !== undefined)
				this.doProtocol(s, handshakeProtocol.certificate, this.clientCerts);
			this.doProtocol(s, handshakeProtocol.clientKeyExchange);
			if (this.myCert)	// client cert request && and the certs is not empty
				this.doProtocol(s, handshakeProtocol.certificateVerify, this.myCert);
			this.doProtocol(s, changeCipherSpec);
			this.doProtocol(s, handshakeProtocol.finished);
			break;
		case handshakeProtocol.finished.msgType: {		// the server's Finished: a resumed session answers with ours
			if (this.tls13?.suite) {		// TLS 1.3: our flight, then the application keys
				Modules.importNow(TLS13).clientFlight(this, s);
				const digests = this.handshakeDigests;
				for (let name in digests)
					digests[name].close();
				this.handshakeDigests = null;
				delete this.handshakeMessages;
				delete this.clientSessionID;
				delete this.serverSessionID;
				delete this.clientCerts;
				delete this.myCert;
				delete this.certificateManager;
				delete this.cacheManager;
				state = 2;
				break;
			}
			let resumed = this.clientSessionID && Bin.comp(this.clientSessionID, this.serverSessionID) == 0;
			if (resumed) {
				this.doProtocol(s, changeCipherSpec);
				this.doProtocol(s, handshakeProtocol.finished);
			}
			state = 2;
			if (this.cacheManager) {
				if (this.serverSessionID && this.options.tls_server_name)
					this.cacheManager.saveSession(this.options.tls_server_name, this.serverSessionID, this.masterSecret);
				if (this.clientSessionID && Bin.comp(this.clientSessionID, this.serverSessionID) != 0)
					// tried to resume but got rejected
					this.cacheManager.deleteSessionID(this.clientSessionID);
				delete this.cacheManager;
			}
			// remove to instance variables that are no longer necessary
			const digests = this.handshakeDigests;		// closed explicitly: a for-in here would put its enumerator on the stack at the handshake's deepest point
			digests.SHA256.close();
			digests.SHA384?.close();
			this.handshakeDigests = null;
			delete this.handshakeMessages;
			delete this.clientSessionID;
			delete this.serverSessionID;
			delete this.clientCerts;
			delete this.myCert;
			delete this.certificateManager;
			} break;
		default: {
			if (0 == s.readable)
				break;
			// process a packet once even if more than one packets are available
			let buf = this.readPacket(s);
			if (buf) {
				this.startTrace("unpacketize");
				recordProtocol.unpacketize(this, buf);
			}
			state = 1;
			} break;
		}
		if (this.alert) {		// the server closed the connection during the handshake (its other alerts throw where they are received)
			this.stopTrace();
			throw new TLSError("handshake: closed by server");
		}
		if (state == 0)
			this.handshakeProcess = -1;
		if (state == 2) {
			this.stopTrace();
			return true;
		}
		else
			return false;
	}
	read(s) {
		while (s.readable) {
			if (!this.applicationData) {
				if ((handshakeProtocol.finished.msgType === this.handshakeProcess) && this.tls13?.suite && (null !== this.handshakeDigests))
					return;		// TLS 1.3: the server's Finished has been read (here or by handshake); our flight switches the keys before anything that follows it (a server may send a NewSessionTicket at once)
				// read at least one packet and just keep it
				const buf = this.readPacket(s);
				if (!buf)
					return;
				this.startTrace("unpacketize");
				recordProtocol.unpacketize(this, buf);
				if (this.alert)
					return null;	// probably the session has been closed by the peer
			}
			if (this.applicationData) {
				const applicationData = this.applicationData;
				delete this.applicationData;
				return new Uint8Array(applicationData.buffer, applicationData.position, applicationData.end - applicationData.position);
			}
		}
	}
	write(s, data, options) {
		if (options?.more) {
			this.#buffer ??= new SSLStream(undefined, options.byteLength);
			this.#buffer.writeChunk(data);
			return Math.max(0, s.writable - this.#buffer.bytesAvailable - 32);
		}
		if (this.#buffer) {
			this.#buffer.writeChunk(data);
			data = this.#buffer.getChunk();
			this.#buffer = undefined;
		}

		if (data.byteLength > maxFragmentSize)
			throw new Error("too large");
		if (this.tls13?.updateRequested)
			Modules.importNow(TLS13).updateKeys(this, s);
		this.startTrace("packetize");
		const packet = recordProtocol.packetize(this, recordProtocol.application_data, data);
		s.writable = s.write(packet);
		return Math.max(0, s.writable - 32);
	}
	close(s) {
		this.startTrace("packetize");
		const packet = SSLAlert.packetize(this, 1, SSLAlert.close_notify);
		s.writable = s.write(packet);
		this.release();
	}
	release() {		// the session's crypto objects and secrets, whether or not the handshake completed: ciphers hold keys until closed
		if (this.tls13)
			Modules.importNow(TLS13).release(this);
		closeCipher(this.clientCipher);
		closeCipher(this.serverCipher);
		this.clientCipher = this.serverCipher = undefined;
		const digests = this.handshakeDigests;
		for (let name in digests)
			digests[name].close();
		this.handshakeDigests = null;
		this.random?.close();
		this.random = undefined;
	}
	doProtocol(s, protocol, param1, param2) {
		this.startTrace("packetize");
		const packet = protocol.packetize(this, param1, param2);
		if (!packet) return;
		s.write(packet);
		s.writable -= packet.byteLength; 
	}
	readPacket(s) {
		let packetBuffer = this.packetBuffer;
		if (!packetBuffer || (packetBuffer.offset < 5)) {
			if (!packetBuffer) {
				packetBuffer = this.packetBuffer = new Uint8Array(new ArrayBuffer(5, {maxByteLength: 32768}));
				packetBuffer.offset = 0;
			}

			const need = Math.min(5 - packetBuffer.offset, s.readable);
			s.read(this.packetBuffer.subarray(packetBuffer.offset, packetBuffer.offset + need));
			s.readable -= need;
			packetBuffer.offset += need;
			if (packetBuffer.offset < 5)
				return;

			const length = (packetBuffer[3] << 8) | packetBuffer[4];
			if (length > (this.tls13?.suite ? (16384 + 256) : (16384 + 2048)))		// record_overflow (RFC 8446 section 5.2; RFC 5246 section 6.2.3)
				throw new TLSError("recordProtocol: record overflow");
			this.packetBuffer.buffer.resize(length + 5);		/* packet length including 5 byte header */
		}
		let need = packetBuffer.length - packetBuffer.offset;
		if (need) {
			if (!s.readable)
				return;
			if (need > s.readable)
				need = s.readable;
			s.read(packetBuffer.subarray(packetBuffer.offset, packetBuffer.offset + need)); 
			s.readable -= need;
			packetBuffer.offset += need;
			if (packetBuffer.offset < packetBuffer.length)
				return;
		}
		delete this.packetBuffer;
		return packetBuffer.buffer;
	}
	putData(data) {
		this.applicationData = data;
		data.position = data.byteOffset;
		data.end = data.byteOffset + data.byteLength;
	}

	startTrace(direction) {
		if (undefined === this.traceLevel)
			return;

		this.traceLevel = 0;
		this.traceDirection = direction;
	}
	traceProtocol(protocol) {
		if (undefined === this.traceLevel)
			return;

		trace(">".repeat(this.traceLevel + 1) + " " + protocol.name + "." + this.traceDirection + "\n");
		this.traceLevel++;
	}
	stopTrace() {
		if (this.alert) {
			trace("ALERT! (" + this.alert.level + ", " + this.alert.description + "\n");
			return;
		}
		if (undefined === this.traceLevel)
			return;

		if (this.tls13?.suite) {
			trace("FINISHED: TLS 1.3 " + this.tls13.suite.name + (this.tls13.retried ? " after HelloRetryRequest" : "") + (this.tls13.resumed ? " resumed" : "") + "\n");
			return;
		}
		let algo = "";
		switch (this.chosenCipher.keyExchangeAlgorithm) {
		case RSA:	algo = "RSA"; break;
		case ECDHE_RSA:	algo = "ECDHE_RSA"; break;
		case ECDHE_ECDSA:	algo = "ECDHE_ECDSA"; break;
		}
		let enc = "";
		switch (this.chosenCipher.cipherAlgorithm) {
		case AES:	enc = "AES"; break;
		}
		let mode = "";
		switch (this.chosenCipher.encryptionMode) {
		case CBC:	mode = "CBC"; break;
		case GCM:	mode = "GCM"; break;
		}
		let hash = "";
		switch (this.chosenCipher.hashAlgorithm) {
		case SHA1:	hash = "SHA1"; break;
		case SHA256:	hash = "SHA256"; break;
		case SHA384:	hash = "SHA384"; break;
		}
		trace("FINISHED: " + algo + "-" + enc + "-" + mode + "-" + hash + "\n");
	}
}

export default SSLSession;
