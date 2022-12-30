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

import recordProtocol from "ssl/record";
import handshakeProtocol from "ssl/handshake";
import changeCipherSpec from "ssl/changecipher";
import SSLAlert from "ssl/alert";
import cacheManager from "ssl/cache";
import CertificateManager from "ssl/cert";
import TLSError from "ssl/error";
import Bin from "bin";
import {minProtocolVersion, maxProtocolVersion, protocolVersion} from "ssl/constants";
import {DHE_DSS, DHE_RSA, ECDHE_RSA, RSA, AES, CBC, DES, GCM, MD5, NONE, RC4, SHA1, SHA256, SHA384, TDES} from "ssl/constants";

const maxFragmentSize = 16384	// maximum record layer framgment size (not a packet size): 2^14

class SSLSession {
	constructor(options) {
		if (options.protocolVersion) {
			if ((options.protocolVersion < minProtocolVersion) || (options.protocolVersion > maxProtocolVersion))
				throw new TLSError("protocolVersion: not supported");
		}
		if ('serverName' in options) {
			options.tls_server_name = options.serverName;
			delete options.serverName;
		}
		if ('applicationLayerProtocolNegotiation' in options) {
			options.tls_application_layer_protocol_negotiation = options.applicationLayerProtocolNegotiation;
			delete options.applicationLayerProtocolNegotiation;
		}
		this.options = options;
		this.packetBuffer = null;
		this.handshakeMessages = undefined;
		this.handshakeDigests = undefined;
		this.clientSessionID = this.serverSessionID = undefined;
		this.clientCerts = undefined;
		this.myCert = undefined;
		this.handshakeProcess = -1;
		this.connectionEnd = false;
		this.clientCipher = null;
		this.serverCipher = null;
		this.alert = undefined;
		this.protocolVersion = this.options.protocolVersion || protocolVersion;
		this.minProtocolVersion = this.options.protocolVersion || minProtocolVersion;	// only for the server side
		this.maxProtocolVersion = maxProtocolVersion;	// ditto
		if (this.options.trace)
			this.traceLevel = 0;
//		this.applicationData = undefined;
		this.cacheManager = (this.options.cache === undefined || this.options.cache) && cacheManager;
		this.certificateManager = new CertificateManager(options);
	}
	initiateHandshake() {
		this.connectionEnd = true;
		this.handshakeProcess = handshakeProtocol.helloRequest.msgType;
	}
	handshake(s) {
		let state = 0;
		switch (this.handshakeProcess) {
		case handshakeProtocol.helloRequest.msgType: {		// C
			let cache = this.cacheManager && this.cacheManager.getByHost(this.options.tls_server_name);
			if (cache) {
				this.clientSessionID = cache.id;
				this.masterSecret = cache.secret;
			}
			this.doProtocol(s, handshakeProtocol.clientHello);
			} break;
		case handshakeProtocol.clientHello.msgType: {		// S
			let masterSecret = this.clientSessionID && this.cacheManager && this.cacheManager.getByID(this.clientSessionID);
			if (masterSecret) {
				// resumed handshake
				this.serverSessionID = this.clientSessionID;
				this.masterSecret = masterSecret;
				this.doProtocol(s, handshakeProtocol.serverHello);
				this.doProtocol(s, changeCipherSpec);
				this.doProtocol(s, handshakeProtocol.finished);
			}
			else {
				// assign a new one
				this.serverSessionID = ArrayBuffer.fromBigInt(BigInt(((new Date()).valueOf()).toString()));
				this.doProtocol(s, handshakeProtocol.serverHello);
				// S -> C: Certificate   (always -- i.e. not support anonymous auth.)
				let certs = this.certificateManager.getCerts();
				if (!certs || !certs.length)
					throw new TLSError("client_hello: no certificate");
				this.doProtocol(s, handshakeProtocol.certificate, certs);
				// S -> C: ServerKeyExchange (may not do anything depending on the chosen cipher)
				this.doProtocol(s, handshakeProtocol.serverKeyExchange);
				if (this.options.clientAuth)
					// S -> C: CertificateRequest
					this.doProtocol(s, handshakeProtocol.certificateRequest, this.options.clientAuth.cipherSuites, this.options.clientAuth.subjectDN);
				// S -> C: ServerHelloDone
				this.doProtocol(s, handshakeProtocol.serverHelloDone);
			}
			} break;
		case handshakeProtocol.serverHelloDone.msgType:		// C
			if (this.clientCerts !== undefined)
				this.doProtocol(s, handshakeProtocol.certificate, this.clientCerts);
			this.doProtocol(s, handshakeProtocol.clientKeyExchange);
			if (this.myCert)	// client cert request && and the certs is not empty
				this.doProtocol(s, handshakeProtocol.certificateVerify, this.myCert);
			this.doProtocol(s, changeCipherSpec);
			this.doProtocol(s, handshakeProtocol.finished);
			break;
		case handshakeProtocol.finished.msgType: {		// C, S
			let resumed = this.clientSessionID && Bin.comp(this.clientSessionID, this.serverSessionID) == 0;
			if (!(this.connectionEnd ^ resumed)) {
				this.doProtocol(s, changeCipherSpec);
				this.doProtocol(s, handshakeProtocol.finished);
			}
			state = 2;
			if (this.cacheManager) {
				if (this.serverSessionID && this.options.tls_server_name)
					this.cacheManager.saveSession(this.options.tls_server_name, this.serverSessionID, this.masterSecret);
				if (this.connectionEnd && this.clientSessionID && Bin.comp(this.clientSessionID, this.serverSessionID) != 0)
					// C: tried to resume but got rejected
					this.cacheManager.deleteSessionID(this.clientSessionID);
				delete this.cacheManager;
			}
			// remove to instance variables that are no longer necessary
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
		if (this.alert) {
			if (this.alert.description != SSLAlert.close_notify)
				this.doProtocol(s, SSLAlert, this.alert.level, this.alert.description);
			this.stopTrace();
			return true;	// stop handshaking right away
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
	write(s, data) {
		if (data.byteLength > maxFragmentSize)
			return -1;	// too large
		this.startTrace("packetize");
		const packet = recordProtocol.packetize(this, recordProtocol.application_data, data);
		s.write(packet);
		s.writable -= packet.byteLength;
	}
	close(s) {
		this.startTrace("packetize");
		const packet = SSLAlert.packetize(this, 0, SSLAlert.close_notify);
		s.write(packet);
		s.writable -= packet.byteLength
	}
	doProtocol(s, protocol, param1, param2) {
		this.startTrace("packetize");
		const packet = protocol.packetize(this, param1, param2);
		if (packet) {
			s.write(packet);
			s.writable -= packet.byteLength; 
		}
	}
	readPacket(s) {
		let packetBuffer = this.packetBuffer;
		if (!packetBuffer || (packetBuffer.length < 5)) {
			if (!packetBuffer) {
				let need = Math.min(5, s.readable);
				this.packetBuffer = new Uint8Array(s.read(need));
				s.readable -= need;
			}
			else {
				let need = Math.min(5 - packetBuffer.length, s.readable);
				let c = new Uint8Array(s.read(need));
				s.readable -= need;
				this.packetBuffer = new Uint8Array(packetBuffer.length + c.length);
				this.packetBuffer.set(packetBuffer);
				this.packetBuffer.set(c, packetBuffer.length);
			}
			packetBuffer = this.packetBuffer;
			if (packetBuffer.length < 5)
				return;

			this.packetBuffer = new Uint8Array(((packetBuffer[3] << 8) | packetBuffer[4]) + 5);		/* packet length including 5 byte header */
			this.packetBuffer.set(packetBuffer);
			packetBuffer = this.packetBuffer;
			packetBuffer.offset = 5;
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
		this.packetBuffer = null;
		return packetBuffer.buffer;
	}
//	get bytesAvailable() {
//		return this.applicationData ? (this.applicationData.end - this.applicationData.position) : 0;
//	}
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

		let algo = "";
		switch (this.chosenCipher.keyExchangeAlgorithm) {
		case RSA:	algo = "RSA"; break;
		case DHE_DSS:	algo = "DHE_DSS"; break;
		case DHE_RSA:	algo = "DHE_RSA"; break;
		case ECDHE_RSA:	algo = "ECDHE_RSA"; break;
		}
		let enc = "";
		switch (this.chosenCipher.cipherAlgorithm) {
		case AES:	enc = "AES"; break;
		case DES:	enc = "DES"; break;
		case TDES:	enc = "TDES"; break;
		case RC4:	enc = "RC4"; break;
		}
		let mode = "";
		switch (this.chosenCipher.encryptionMode) {
		case CBC:	mode = "CBC"; break;
		case GCM:	mode = "GCM"; break;
		case NONE:	mode = "NONE"; break;
		}
		let hash = "";
		switch (this.chosenCipher.hashAlgorithm) {
		case SHA1:	hash = "SHA1"; break;
		case MD5:	hash = "MD5"; break;
		case SHA256:	hash = "SHA256"; break;
		case SHA384:	hash = "SHA384"; break;
		}
		trace("FINISHED: " + algo + "-" + enc + "-" + mode + "-" + hash + "\n");
	}
}

export default SSLSession;
