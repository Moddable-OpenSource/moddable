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
import SSLStream from "ssl/stream";
import supportedCipherSuites from "ssl/ciphersuites";
import Mont from "mont";
import PRF from "ssl/prf";
import TLSError from "ssl/error";
import Bin from "bin";
import RNG from "rng";
import PKCS1_5 from "pkcs1_5";
import DSA from "dsa";
import ECDSA from "ecdsa";
import Curve from "curve";
import {Digest} from "crypt";
import {CERT_RSA, CERT_DSA, DH_ANON, DH_DSS, DH_RSA, DHE_DSS, DHE_RSA, ECDHE_RSA, RSA, supportedCompressionMethods} from "ssl/constants";

const hello_request = 0;
const client_hello = 1;
const server_hello = 2;
const certificate = 11;
const server_key_exchange = 12;
const certificate_request = 13;
const server_hello_done = 14;
const certificate_verify = 15;
const client_key_exchange = 16;
const finished = 20;

const master_secret_label = "master secret";
const client_finished_label = "client finished";
const server_finished_label = "server finished";

const MD5 = 1;
const SHA1 = 2;
const SHA256 = 4;

const extension_type = Object.freeze({
	tls_server_name: 0,
	tls_max_fragment_length: 1,
	tls_client_certification_url: 2,
	tls_trusted_ca_keys: 3,
	tls_trusted_hmac: 4,
	tls_status_request: 5,
	tls_elliptic_curves: 10,
	tls_ec_point_formats: 11,
	tls_signature_algorithms: 13,
	tls_application_layer_protocol_negotiation: 16,
});

const named_curves = Object.freeze({
	secp256r1: 23,
	secp384r1: 24,
	secp521r1: 25,
	x25519: 29,
	x448: 30,
});

const options = Object.freeze({
	tls_elliptic_curves: [named_curves.secp256r1],
	tls_ec_point_formats: [0],	// uncompressed
	tls_signature_algorithms: [
		{hash: 4, sig: 3},		// SHA256, ECDSA
		{hash: 4, sig: 1}		// SHA256, RSA
	]
}, true);

function handshakeDigestUpdate(session, data)
{
	if (session.handshakeDigests) {
		for (let digest in session.handshakeDigests)
			session.handshakeDigests[digest].write(data);
		return;
	}

	if (!session.handshakeMessages)
		session.handshakeMessages = new SSLStream();
	return session.handshakeMessages.writeChunk(data);
}

function handshakeDigestResult(session, which)
{
	let H, res;

	if (which & MD5)
		H = session.handshakeDigests.MD5.close(true);
	if (which & SHA1) {
		res = session.handshakeDigests.SHA1.close(true);
		H = H ? H.concat(res) : res;
	}
	if (which & SHA256) {
		res = session.handshakeDigests.SHA256.close(true);
		H = H ? H.concat(res) : res;
	}
	return H;
}

const handshakeProtocol = {
	name: "handshakeProtocol",

	unpacketize(session, s) {
		session.traceProtocol(this);
		let tbuf = new Uint8Array(4);
		tbuf[0] = s.readChar(0);		// msgType
		let body = s.readChars(3);
		body = body > 0 ? s.readChunk(body, true) : undefined;
		switch (tbuf[0]) {
		case 0 /* hello_request */:
			s = this.helloRequest;
			break;
		case 1 /* client_hello */:
			s = this.clientHello;
			break;
		case 2 /* server_hello */:
			s = this.serverHello;
			break;
		case 11 /* certificate */:
			s = this.certificate;
			break;
		case 12 /* server_key_exchange */:
			s = this.serverKeyExchange;
			break;
		case 13 /* certificate_request */:
			s = this.certificateRequest;
			break;
		case 14 /* server_hello_done */:
			s = this.serverHelloDone;
			break;
		case 15 /* certificate_verify */:
			s = this.certificateVerify;
			break;
		case 16 /* client_key_exchange */:
			s = this.clientKeyExchange;
			break;
		case 20 /* finished */:
			s = this.finished;
			break;
		default:
			throw new TLSError("handshake: unknown type: " + tbuf[0]);
		}
		s.unpacketize(session, body ? new SSLStream(body) : undefined);

		session.handshakeProcess = tbuf[0];

		// update the digest of the whole handshake message
		if (body) {
			tbuf[1] = body.byteLength >>> 16;
			tbuf[2] = body.byteLength >>> 8;
			tbuf[3] = body.byteLength;
		}

		handshakeDigestUpdate(session, tbuf.buffer);
		if (body)
			handshakeDigestUpdate(session, body);
	},
	packetize(session, msgType, body) {
		session.traceProtocol(this);
		let s = new SSLStream();
		s.writeChar(msgType);
		s.writeChars(body.bytesWritten, 3);
		s.writeChunk(body.getChunk());
		let msg = s.getChunk();
		handshakeDigestUpdate(session, msg);
		return recordProtocol.packetize(session, recordProtocol.handshake, msg);
	},

	// protocols
	helloRequest: {
		name: "helloRequest",
		msgType: hello_request,
		unpacketize(session) {
			session.traceProtocol(this);
			// nothing to unpacketize
		},
		packetize(session) {
			// should not be called
			debugger;
		},
	},

	helloProtocol: {
		random: {
			parse(s) {
				let t = s.readChars(4);
				return {
					qmt_unix_time: t,
					random_bytes: s.readChunk(28)
				};
			},
			serialize() {
				let qmt_unix_time = ((new Date()).getTime() / 1000) | 0;	// in sec
				let a = new Uint8Array(RNG.get(32));
				a[0] = (qmt_unix_time >>> 24) & 255;
				a[1] = (qmt_unix_time >>> 16) & 255;
				a[2] = (qmt_unix_time >>>  8) & 255;
				a[3] =  qmt_unix_time         & 255;
				return a.buffer;
			},
		},
		selectCipherSuite(peerSuites) {
			for (var i = 0, suites = supportedCipherSuites; i < suites.length; i++) {
				var mySuite = suites[i].value;
				for (var j = 0; j < peerSuites.length; j++) {
					if (mySuite[0] == peerSuites[j][0] && mySuite[1] == peerSuites[j][1])
						return suites[i];
				}
			}
			throw new TLSError("handshake: unsuppoorted cipher");
		},
		selectCompressionMethod(peerMethods) {
			for (var i = 0, methods = supportedCompressionMethods; i < methods.length; i++) {
				for (var j = 0; j < peerMethods.length; j++) {
					if (methods[i] == peerMethods[j])
						return methods[i];
				}
			}
			throw new TLSError("handshake: unsupported compression");
		},
		unpacketize(session, s, msgType) {
			var ver = s.readChars(2);
			if (ver < session.minProtocolVersion) {
				session.alert = {level: 2, description: 70};
				return;
			}
			else if (ver > session.maxProtocolVersion)
				session.protocolVersion = session.maxProtocolVersion;
			else
				session.protocolVersion = ver;	// should support all versions between [min, max]
			session.handshakeDigests = {};
			if (session.protocolVersion >= 0x303)
				session.handshakeDigests.SHA256 = new Digest("SHA256");
			else {
				session.handshakeDigests.MD5 = new Digest("MD5");
				session.handshakeDigests.SHA1 = new Digest("SHA1");
			}

			let data = session.handshakeMessages.getChunk();
			for (let digest in session.handshakeDigests)
				session.handshakeDigests[digest].write(data);
			delete session.handshakeMessages;

			var random = s.readChunk(32);
			var sessionIDLen = s.readChar();
			var sessionID = sessionIDLen > 0 ? s.readChunk(sessionIDLen) : undefined;
			var suites = [];
			var compressionMethods = [];
			if (msgType === client_hello) {
				session.clientRandom = random;
				session.clientSessionID = sessionID;
				for (let nsuites = s.readChars(2) / 2; --nsuites >= 0;) {
					let c1 = s.readChar();
					let c2 = s.readChar();
					suites.push([c1, c2]);
				}
				// select the most suitable one
				for (var nmethods = s.readChar(); --nmethods >= 0;)
					compressionMethods.push(s.readChar());
			}
			else {
				session.serverRandom = random;
				session.serverSessionID = sessionID;
				let c1 = s.readChar();
				let c2 = s.readChar();
				suites.push([c1, c2]);
				compressionMethods.push(s.readChar());
			}
			session.chosenCipher = this.selectCipherSuite(suites);
			session.compressionMethod = this.selectCompressionMethod(compressionMethods);
			if (msgType === server_hello && s.byteAvailable) {
				let type = s.readChars(2);
				switch (type) {
				case extension_type.tls_signature_algorithms: {
					/* let len = */ s.readChars(2);
					let n = s.readChars(2);
					session.signature_algorithms = [];
					for (let i = 0; i < n; i++) {
						let hash = s.readChar();
						let sig = s.readChar();
						session.signature_algorithms.push({hash, sig});
					}
					} break;
				default:
					break;
				}
			}
		},
		packetize(session, cipherSuites, compressionMethods, msgType) {
			const s = new SSLStream;
			s.writeChars(session.protocolVersion, 2);
			let random = this.random.serialize();
			let sessionID;
			s.writeChunk(random);
			if (msgType === client_hello) {
				session.clientRandom = random;
				sessionID = session.clientSessionID;
			}
			else {
				session.serverRandom = random;
				sessionID = session.serverSessionID;
			}
			if (!sessionID || !sessionID.byteLength)
				s.writeChar(0);
			else {
				s.writeChar(sessionID.byteLength);
				s.writeChunk(sessionID);
			}
			if (msgType === client_hello) {
				s.writeChars(cipherSuites.length * 2, 2);
				for (let i = 0; i < cipherSuites.length; i++) {
					let val = cipherSuites[i].value;
					s.writeChar(val[0]);
					s.writeChar(val[1]);
				}
				s.writeChar(compressionMethods.length);
				for (let i = 0; i < compressionMethods.length; i++)
					s.writeChar(compressionMethods[i]);
				//
				// TLS extensions
				//
				const es = new SSLStream;
				session.options = {...options, ...session.options};
				for (let i in session.options) {
					if (!(i in extension_type))
						continue;
					const type = extension_type[i];
					const ext = session.options[i];
					switch (type) {
					case extension_type.tls_server_name: {
						es.writeChars(type, 2);
						let len = 1 + 2 + ext.length;
						es.writeChars(2 + len, 2);
						es.writeChars(len, 2);
						es.writeChar(0);		// name_type, 0 -- host_name
						es.writeChars(ext.length, 2);
						es.writeString(ext);
						}
						break;
					case extension_type.tls_max_fragment_length: {
						es.writeChars(type, 2);
						es.writeChars(1, 2);
						let j;
						for (j = 1; j <= 4; j++) {
							if ((ext >>> (j + 9)) == 0)		// start with 2^9
								break;
						}
						if (j > 4)
							j = 4;
						es.writeChar(j);
						}
						break;
					case extension_type.tls_signature_algorithms: {
						es.writeChars(type, 2);
						es.writeChars(2 + ext.length * 2, 2);
						es.writeChars(ext.length * 2, 2);
						for (let j = 0; j < ext.length; j++) {
							es.writeChar(ext[j].hash);
							es.writeChar(ext[j].sig);
						}
						}
						break;
					case extension_type.tls_application_layer_protocol_negotiation: {
						es.writeChars(type, 2);
						let byteLength = 0;
						const items = Array.isArray(ext) ? ext.slice() : [ext];
						for (let j = 0; j < items.length; j++) {
							let item = items[j];
							if ("string" === typeof item)
								items[j] = item = ArrayBuffer.fromString(item);
							const length = item.byteLength;
							if ((length < 1) || (length > 255))
								throw new Error("invalid protocol");
							byteLength += length + 1;
						}
						es.writeChars(byteLength + 2, 2);
						es.writeChars(byteLength, 2);
						for (let j = 0; j < items.length; j++) {
							const item = items[j];
							es.writeChars(item.byteLength, 1);
							es.writeChunk(item);
						}
						}
						break;
					case extension_type.tls_elliptic_curves:
						es.writeChars(type, 2);
						es.writeChars(ext.length * 2 + 2, 2);
						es.writeChars(ext.length * 2, 2);
						for (let j = 0; j < ext.length; j++)
							es.writeChars(ext[j], 2);
						break;
					case extension_type.tls_ec_point_formats:
						es.writeChars(type, 2);
						es.writeChars(ext.length + 1, 2);
						es.writeChars(ext.length, 1);
						for (let j = 0; j < ext.length; j++)
							es.writeChars(ext[j], 1);
						break;
					default:
						// not supported yet
						break;
					}
				}
				if (es.bytesAvailable) {
					s.writeChars(es.bytesAvailable, 2);
					s.writeChunk(es.getChunk());
				}
			}
			else {
				const val = cipherSuites[0].value;
				s.writeChar(val[0]);
				s.writeChar(val[1]);
				s.writeChar(compressionMethods[0]);
			}
			return handshakeProtocol.packetize(session, msgType, s);
		},
	},

	clientHello: {
		name: "clientHello",
		msgType: client_hello,
		unpacketize(session, s) {
			session.traceProtocol(this);
			return handshakeProtocol.helloProtocol.unpacketize(session, s, this.msgType);	// tail call optimization
		},
		packetize(session) {
			session.traceProtocol(this);
			return handshakeProtocol.helloProtocol.packetize(session, supportedCipherSuites, supportedCompressionMethods, this.msgType);
		},
	},

	serverHello: {
		name: "serverHello",
		msgType: server_hello,
		unpacketize(session, s) {
			session.traceProtocol(this);
			return handshakeProtocol.helloProtocol.unpacketize(session, s, this.msgType);
		},
		packetize(session) {
			session.traceProtocol(this);
			return handshakeProtocol.helloProtocol.packetize(session, [session.chosenCipher], [session.compressionMethod], this.msgType);
		},
	},

	certificate: {
		name: "certificate",
		msgType: certificate,
//		matchName(re, name) {
//			re = re.replace(/\./g, "\\.").replace(/\*/g, "[^.]*");
//			var a = name.match(new RegExp("^" + re + "$", "i"));
//			return a && a.length == 1;
//		},
//		verifyHost(session, cert) {
//			//@@ this fails because session.socket.host doesn't exist
//			var altNames = X509.decodeExtension(cert, 'subjectAlternativeName');
//			var hostname = session.socket.host;
//			for (var i = 0; i < altNames.length; i++) {
//				var name = altNames[i];
//				if (typeof name == "string" && this.matchName(name, hostname))
//					return true;
//			}
//			var arr = X509.decodeTBS(cert).subject.match(/CN=([^,]*)/);
//			return arr && arr.length > 1 && this.matchName(arr[1], hostname);
//		},

		unpacketize(session, s) {
			session.traceProtocol(this);
			let certs = [];
			let ttlSize = s.readChars(3);
			while (ttlSize > 0 && s.bytesAvailable > 0) {
				let certSize = s.readChars(3);
				certs.push(s.readChunk(certSize, true));
				ttlSize -= certSize + 3;
			}
			if (!session.certificateManager.verify(certs))
				throw new TLSError("certificate: auth err");

/*
			if (session.options.verifyHost) {
				if (!this.verifyHost(session, certs[0]))
					throw new TLSError("certificate: bad host");
			}
*/
			session.peerCert = certs[0].slice(0).buffer;		// could we store only the key?
			return session.certificateManager.register(session.peerCert);	// tail call optimization
		},
		packetize(session, certs) {
			session.traceProtocol(this);
			const s = new SSLStream();
			let length = 0;
			for (let i = 0; i < certs.length; i++)
				length += certs[i].byteLength + 3;
			s.writeChars(length, 3);		// total size of certificates
			for (let i = 0; i < certs.length; i++) {
				const c = certs[i];
				s.writeChars(c.byteLength, 3);
				s.writeChunk(c);
			}
			if (certs.length > 0)
				session.myCert = certs[0];	// the first one
			return handshakeProtocol.packetize(session, certificate, s);
		},
	},

	serverKeyExchange: {
		name: "serverKeyExchange",
		msgType: server_key_exchange,
		// hash algorithms
		none: 0,
		md5: 1,
		sha1: 2,
		sha224: 3,
		sha256: 4,
		sha384: 5,
		sha512: 6,
		// signature algorithms
		anonymous: 0,
		rsa: 1,
		dsa: 2,
		ecdsa: 3,
		ed25519: 7,
		ed448: 8,

		unpacketize(session, s) {
			session.traceProtocol(this);
			switch (session.chosenCipher.keyExchangeAlgorithm) {
			case DHE_DSS:
			case DHE_RSA:
			case ECDHE_RSA: {
				let tbs = new SSLStream();
				let dhparams = {};
				if (session.chosenCipher.keyExchangeAlgorithm == ECDHE_RSA) {
					// ServerECDHParams
					//	ECParameters
					//		ECCurveType
					let curve_type = s.readChars(1);
					if (curve_type != 3)	// named_curve
						throw new TLSError("invalid curve type");
					//		NamedCurve
					tbs.writeChars(curve_type, 1)
					let named_curve = s.readChars(2);
					if (!session.options.tls_elliptic_curves.includes(named_curve))
						throw new TLSError("unsupported curve");
					dhparams.named_curve = named_curve;
					tbs.writeChars(named_curve, 2);
					//	ECPoint
					let n = s.readChars(1);
					tbs.writeChars(n, 1);
					let ecpoint = s.readChunk(n);
					tbs.writeChunk(ecpoint);
					dhparams.dh_Ys = ecpoint;
				} else {	// DHE_DSS / DHE_RSA
					let n = s.readChars(2);
					tbs.writeChars(n, 2);
					dhparams.dh_p = s.readChunk(n);
					tbs.writeChunk(dhparams.dh_p);
					n = s.readChars(2);
					tbs.writeChars(n, 2);
					dhparams.dh_g = s.readChunk(n);
					tbs.writeChunk(dhparams.dh_g);
					n = s.readChars(2);
					tbs.writeChars(n, 2);
					dhparams.dh_Ys = s.readChunk(n);
					tbs.writeChunk(dhparams.dh_Ys);
				}
				session.dhparams = dhparams;
				let hash_algo = s.readChar();
				let sig_algo = s.readChar();
				let n = s.readChars(2);
				let sig = s.readChunk(n);
				let hash, v;
				switch (hash_algo) {
				default:
				case this.none: break;
				case this.md5: hash = new Digest("MD5"); break;
				case this.sha1: hash = new Digest("SHA1"); break;
				case this.sha224: hash = new Digest("SHA224"); break;
				case this.sha256: hash = new Digest("SHA256"); break;
				case this.sha384: hash = new Digest("SHA384"); break;
				case this.sha512: hash = new Digest("SHA512"); break;
				}
				let key = session.certificateManager.getKey(session.peerCert);
				switch (sig_algo) {
				default:
				case this.anonymous: break;
				case this.rsa: v = new PKCS1_5(key, false, []); break;
				case this.dsa: v = new DSA(key, false); break;
				case this.ecdsa: v = new ECDSA(key.pub, key.curve, false); break;
				}
				if (hash && v && sig) {
					let H = hash.process(session.clientRandom, session.serverRandom, tbs.getChunk());
					if (!v.verify(H, sig, true)) {
						// should send an alert, probably...
						throw new TLSError("serverKeyExchange: failed to verify signature");
					}
				}
				hash = v = sig = tbs = null;
				} break;
			case RSA:
				// no server key exchange info
				break;
			case DH_ANON:
			case DH_DSS:
			case DH_RSA:
			default:
				// not supported
				throw new TLSError("serverKeyExchange: unsupported algorithm: " + session.chosenCipher.keyExchangeAlgorithm);
			}
		},
		packetize(session) {
			let pkt;
			session.traceProtocol(this);
			switch (session.chosenCipher.keyExchangeAlgorithm) {
			case DHE_DSS:
			case DHE_RSA:
			case DH_ANON: {
				let s = new SSLStream();
				let dh = session.certificateManager.getDH();
				let mod = new Mont({m: dh.p, method: Mont.SW});
				dh.x = BigInt.fromArrayBuffer(RNG.get(BigInt.bitLength(dh.p) >>> 3));
				let y = mod.exp(dh.g, dh.x);
				let tbs = new SSLStream(), c;
				// server DH params
				c = ArrayBuffer.fromBigInt(dh.p);
				tbs.writeChars(c.byteLength, 2);
				tbs.writeChunk(c);
				c = ArrayBuffer.fromBigInt(dh.g);
				tbs.writeChars(c.byteLength, 2);
				tbs.writeChunk(c);
				c = ArrayBuffer.fromBigInt(y);
				tbs.writeChars(c.byteLength, 2);
				tbs.writeChunk(c);
				s.writeChunk(tbs.getChunk());
				if (session.chosenCipher.keyExchangeAlgorithm != DH_ANON) {
					// digitally-signed
					let hash_algo = -1, sig_algo = -1, hash, pk, oid;
					if (session.signature_algorithms && session.signature_algorithms.length > 0) {
						// take the first one
						hash_algo = session.signature_algorithms[0].hash;
						sig_algo = session.signature_algorithms[0].sig;
					}
					else {
						hash_algo = this.sha1;
						sig_algo = session.chosenCipher.keyExchangeAlgorithm == DHE_RSA ? this.rsa : this.dsa;
					}
					switch (hash_algo) {
					default:
					case this.none: throw new TLSError("serverKeyExchange: no hash algorithm"); break;
					case this.sha1: hash = Crypt.SHA1; oid = [1, 3, 14, 3, 2, 26]; break;
					case this.md5: hash = Crypt.MD5; oid = [1, 2, 840, 113549, 2, 5]; break;
					case this.sha224: hash = Crypt.SHA224; oid = [2, 16, 840, 1, 101, 3, 4, 2, 4]; break;
					case this.sha256: hash = Crypt.SHA256; oid = [2, 16, 840, 1, 101, 3, 4, 2, 1]; break;
					case this.sha384: hash = Crypt.SHA384; oid = [2, 16, 840, 1, 101, 3, 4, 2, 2]; break;
					case this.sha512: hash = Crypt.SHA512; oid = [2, 16, 840, 1, 101, 3, 4, 2, 3]; break;
					}
					switch (sig_algo) {
					default:
					case this.anonymous: throw new TLSError("serverKeyExchange: no signature algorithm");
					case this.rsa: pk = PKCS1_5; break;
					case this.dsa: pk = DSA; break;
					case this.ecdsa: pk = ECDSA; break;
					}
					let key = session.certificateManager.getKey(/* self */);
					let sig = new pk(key, true, oid);
					let H = (new hash()).process(session.clientRandom, session.serverRandom, tbs.getChunk());
					let signature = sig.sign(H);
					s.writeChar(hash_algo);
					s.writeChar(sig_algo);
					s.writeChars(signature.byteLength, 2);
					s.writeChunk(signature);
				}
				session.dh = dh;
				pkt = handshakeProtocol.packetize(session, server_key_exchange, s);
				} break;
			case RSA:
				// no server key exchange info
				break;
			case DH_DSS:
			case DH_RSA:
				pkt = handshakeProtocol.packetize(session, server_key_exchange, new SSLStream());	// empty body
				break;
			default:
				break;
			}
			return pkt;
		},
	},

	certificateRequest: {
		name: "certificateRequest",
		msgType: certificate_request,
		rsa_sign: 1,
		dss_sign: 2,
		rsa_fixed_dh: 3,
		dss_fixed_dh: 4,

		unpacketize(session, s) {
			session.traceProtocol(this);
			const nCertTypes = s.readChar();
			const types = [];
			for (let i = 0; i < nCertTypes; i++) {
				const type = s.readChar();
				switch (type) {
				case this.rsa_sign:
					types.push(CERT_RSA);
					break;
				case this.dss_sign:
					types.push(CERT_DSA);
					break;
				default:
					// trace("SSL: certificateRequest: unsupported cert type: " + type + "\n");
					break;
				}
			}
			let ttlSize = s.readChars(2);
			const names = [];
			while (ttlSize > 0) {
				let bytes = s.readChars(2);
				names.push(s.readChunk(bytes));
				ttlSize -= bytes + 2;
			}
			session.clientCerts = session.certificateManager.findPreferredCert(types, names);
			// if no clientCerts, proceed to the "certificate" protocol with a null list
		},
		packetize(session, types, authorities) {
			session.traceProtocol(this);
			var s = new SSLStream();
			s.writeChar(types.length);
			for (let i = 0; i < types.length; i++) {
				switch (types[i]) {
				case CERT_RSA:
					s.writeChar(this.rsa_sign);
					break;
				case CERT_DSA:
					s.writeChar(this.dss_sign);
					break;
				}
			}
			var ttlSize = 0;
			for (let i = 0; i < authorities.length; i++)
				ttlSize += authorities[i].length + 2;
			s.writeChars(ttlSize, 2);
			for (let i = 0; i < authorities.length; i++) {
				s.writeChars(authorities[i].length, 2);
				s.writeChunk(authorities[i]);
			}
			return handshakeProtocol.packetize(session, certificate_request, s);
		},
	},

	serverHelloDone: {
		name: "serverHelloDone",
		msgType: server_hello_done,
		unpacketize(session, s) {
			session.traceProtocol(this);
			// nothing to unpacketize
		},
		packetize(session) {
			session.traceProtocol(this);
			// nothing to packetize
			return handshakeProtocol.packetize(session, server_hello_done, new SSLStream());
		},
	},

	clientKeyExchange: {
		name: "clientKeyExchange",
		msgType: client_key_exchange,
		generateMasterSecret(session, preMasterSecret) {
			session.masterSecret = PRF(session, preMasterSecret, master_secret_label,
				session.clientRandom.concat(session.serverRandom), 48);
		},
		unpacketize(session, s) {
			session.traceProtocol(this);
			var n = s.readChars(2);
			var cipher = s.readChunk(n);
			let preMasterSecret;
			switch (session.chosenCipher.keyExchangeAlgorithm) {
			case RSA:
				// PKCS1.5
				if (!session.myCert)
					throw new TLSError("clientKeyExchange: no cert");	// out of sequence
				var key = session.certificateManager.getKey(/* self */);
				var rsa = new PKCS1_5(key, true);
				var plain = rsa.decrypt(cipher);
				// the first 2 bytes must be client_version
				var version = new Uint8Array(plain);
				if (((version[0] << 8) | version[1]) != session.protocolVersion) {
					session.alert = {level: 2, description: 70};
					return;
				}
				preMasterSecret = plain;
				break;
			case DHE_DSS:
			case DHE_RSA:
			case DH_ANON:
			case DH_DSS:
			case DH_RSA: {
				// implicit DH is not supported
				let dh = session.dh;
				let y = BigInt.fromArrayBuffer(cipher);
				let mod = new Mont({m: dh.p, method: Mont.SW});
				y = mod.exp(y, dh.x);
				preMasterSecret = ArrayBuffer.fromBigInt(y);
				} break;
			default:
				throw new TLSError("clientKeyExchange: unsupported algorithm");
			}
			return this.generateMasterSecret(session, preMasterSecret);		// tail call optimization
		},
		packetize(session) {
			session.traceProtocol(this);
			let s = new SSLStream();
			let preMasterSecret;
			switch (session.chosenCipher.keyExchangeAlgorithm) {
			case RSA:
				var plain = new SSLStream();
				plain.writeChars(session.protocolVersion, 2);
				plain.writeChunk(RNG.get(46));
				preMasterSecret = plain.getChunk();
				var cipher = (new PKCS1_5(session.certificateManager.getKey(session.peerCert))).encrypt(preMasterSecret);
				s.writeChars(cipher.byteLength, 2);
				s.writeChunk(cipher);
				break;
			case DHE_DSS:
			case DHE_RSA:
			case DH_ANON:
			case DH_DSS:
			case DH_RSA: {
				// we don't support fixed key DH in cert so should send DH anyway
				if (!session.dhparams)
					throw new TLSError("clientKeyExchange: no DH params");
				let dh = session.dhparams;
				let r = RNG.get(dh.dh_p.byteLength);
				let x = BigInt.fromArrayBuffer(r);
				let g = BigInt.fromArrayBuffer(dh.dh_g);
				let p = BigInt.fromArrayBuffer(dh.dh_p);
				let mod = new Mont({m: p, method: Mont.SW});
				let y = mod.exp(g, x);
				let Yc = ArrayBuffer.fromBigInt(y);
				s.writeChars(Yc.byteLength, 2);
				s.writeChunk(Yc);
				y = BigInt.fromArrayBuffer(dh.dh_Ys);
				y = mod.exp(y, x);
				preMasterSecret = ArrayBuffer.fromBigInt(y);
				} break;
			case ECDHE_RSA:
				if (!session.dhparams)
					throw new TLSError("clientKeyExchange: no DH params");
				for (let name in named_curves) {
					if (named_curves[name] == session.dhparams.named_curve) {
						let curve = new Curve(name);
						let x = RNG.get(curve.orderSize);
						let Yc = curve.dh(x);
						s.writeChars(Yc.byteLength, 1);
						s.writeChunk(Yc);
						let y = curve.dh(x, session.dhparams.dh_Ys);
						preMasterSecret = curve.Z(y);
						break;
					}
				}
				break;
			default:
				throw new TLSError("clientKeyExchange: unsupported algorithm");
			}
			this.generateMasterSecret(session, preMasterSecret);
			return handshakeProtocol.packetize(session, client_key_exchange, s);
		},
	},

	certificateVerify: {
		name: "certificateVerify",
		msgType: certificate_verify,
		calculateDigest(session) {
			return handshakeDigestResult(session,
						session.protocolVersion >= 0x303 ? SHA256 :
						(session.chosenCipher.keyExchangeAlgorithm == RSA ? MD5 | SHA1 : SHA1));
		},
		unpacketize(session, s) {
			session.traceProtocol(this);
			const sig = s.readChunk(s.readChars(2));
			if (session.chosenCipher.keyExchangeAlgorithm == RSA) {
				const key = session.certificateManager.getKey(session.peerCert);
				const rsa = new PKCS1_5(key);
				if (!rsa.verify(this.calculateDigest(session), sig))
					throw new TLSError("certificateVerify: auth err");
			}
			else
				throw new TLSError("certificateVerify: unimplemented");	// unimplemented
		},
		packetize(session, cert) {
			session.traceProtocol(this);
			const s = new SSLStream;
			const keyExchangeAlgorithm = session.chosenCipher.keyExchangeAlgorithm;
			if ((keyExchangeAlgorithm === RSA) || (keyExchangeAlgorithm === ECDHE_RSA)) {
				if (!session.myCert)
					throw new TLSError("certificateVerify: no cert");	// out of sequence
				const key = session.certificateManager.getKey(cert);

				let rsa;
				if (session.protocolVersion >= 0x303) {
					rsa = new PKCS1_5(key, true, [2, 16, 840, 1, 101, 3, 4, 2, 1]);
					s.writeChars(4, 1);	// hash algorithm SHA256
					s.writeChars(1, 1);	// hash algorithm signature RSA
				}
				else {
					rsa = new PKCS1_5(key, true);
				}

				const sig = rsa.sign(this.calculateDigest(session));
				s.writeChars(sig.byteLength, 2);
				s.writeChunk(sig);
			}
			else
				throw new TLSError("certificateVerify: unimplemented");	// unimplemented
			return handshakeProtocol.packetize(session, certificate_verify, s);
		},
	},

	finished: {
		name: "finished",
		msgType: finished,

		calculateVerifyData(session, flag) {
			let finishLabel = (session.connectionEnd ^ flag) ? client_finished_label : server_finished_label;
			let digest = handshakeDigestResult(session, session.protocolVersion <= 0x302 ? MD5 | SHA1 : SHA256);
			return PRF(session, session.masterSecret, finishLabel, digest, 12);
		},
		unpacketize(session, s) {
			session.traceProtocol(this);
			delete session.peerCert;
			delete session.certificateManager;
			let verify = this.calculateVerifyData(session, 1);
			if (Bin.comp(verify, s.readChunk(verify.byteLength)) != 0) {
				session.masterSecret = null;
				throw new TLSError("finished: auth err");
			}
		},
		packetize(session) {
			session.traceProtocol(this);
			let verify = this.calculateVerifyData(session, 0);
			return handshakeProtocol.packetize(session, finished, new SSLStream(verify));
		},
	},
};

Object.freeze(handshakeProtocol, true);

export default handshakeProtocol;
