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
import SSLStream from "ssl/stream";
import supportedCipherSuites from "ssl/ciphersuites";
import PRF from "ssl/prf";
import TLSError from "ssl/error";
import Bin from "bin";
import Key from "embedded:crypt/key";
import Signature from "embedded:crypt/signature";
import Cipher from "embedded:crypt/cipher";
import Digest from "embedded:crypt/digest";
import KeyAgreement from "embedded:crypt/keyagreement";
import {ecdsaSignatureToRaw} from "ssl/cert";
import Modules from "modules";
import {ECDHE_ECDSA, ECDHE_RSA, GCM, RSA, SHA256 as cipherSHA256, SHA384 as cipherSHA384, protocolVersion, supportedCompressionMethods} from "ssl/constants";

// TLS 1.2 (RFC 5246), client side. TLS 1.3 is offered when the build includes ssl/handshake13, which takes over at the ServerHello.
const TLS13 = "ssl/handshake13";
function tls13()
{
	return Modules.has(TLS13) ? Modules.importNow(TLS13) : undefined;
}
const downgradeSentinel12 = Uint8Array.fromHex("444f574e47524401");

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

const curveNames = Object.freeze({23: "P-256", 24: "P-384", 29: "X25519"});		// TLS NamedCurve -> Key namedCurve
const named_curves = Object.freeze({
	secp256r1: 23,
	secp384r1: 24,
	secp521r1: 25,
	x25519: 29,
	x448: 30,
});

const options = Object.freeze({
	tls_elliptic_curves: [named_curves.x25519, named_curves.secp256r1, named_curves.secp384r1],
	tls_ec_point_formats: [0],	// uncompressed
	tls_signature_algorithms: [
		{hash: 4, sig: 3},		// SHA256, ECDSA
		{hash: 4, sig: 1}		// SHA256, RSA
	]
}, true);
// with TLS 1.3 offered: ECDSA P-384 and the RSA-PSS schemes 1.3 requires of RSA certificates (also valid in a 1.2 ServerKeyExchange)
const signatureAlgorithms13 = Object.freeze([
	{hash: 4, sig: 3},		// ecdsa_secp256r1_sha256
	{hash: 5, sig: 3},		// ecdsa_secp384r1_sha384
	{hash: 8, sig: 4},		// rsa_pss_rsae_sha256
	{hash: 8, sig: 5},		// rsa_pss_rsae_sha384
	{hash: 8, sig: 6},		// rsa_pss_rsae_sha512
	{hash: 4, sig: 1},		// rsa_pkcs1_sha256
	{hash: 5, sig: 1},		// rsa_pkcs1_sha384
	{hash: 6, sig: 1},		// rsa_pkcs1_sha512
], true);
function signatureAlgorithms(offer13)
{
	if (!offer13)
		return options.tls_signature_algorithms;
	const pss = Signature.supports({algorithm: "RSA-PSS"});
	// ECDSA only when the build can verify the chains dual-certificate servers then send, whose roots are P-384 (as supportedSuites decides for 1.2)
	const ecdsa = Signature.supports({algorithm: "ECDSA"}) && Key.supports({algorithm: "EC", namedCurve: "P-256"}) && Key.supports({algorithm: "EC", namedCurve: "P-384"});
	const result = [];
	for (let i = 0; i < signatureAlgorithms13.length; i++) {
		const scheme = signatureAlgorithms13[i];
		if ((3 === scheme.sig) ? ecdsa : (pss || (8 !== scheme.hash)))
			result.push(scheme);
	}
	return result;
}

// the suites and curves this build can actually run: a platform may omit algorithms (see Digest.supports and friends)
function supportedSuites(suites)
{
	const result = [];
	for (let i = 0; i < suites.length; i++) {
		const suite = suites[i];
		const hash = (cipherSHA384 === suite.hashAlgorithm) ? "SHA-384" : (cipherSHA256 === suite.hashAlgorithm) ? "SHA-256" : "SHA-1";
		if (!Digest.supports({algorithm: hash}))
			continue;
		if (!Cipher.supports({algorithm: (GCM === suite.encryptionMode) ? "AES-GCM" : "AES-CBC"}))
			continue;
		if (((ECDHE_RSA === suite.keyExchangeAlgorithm) || (ECDHE_ECDSA === suite.keyExchangeAlgorithm)) && !KeyAgreement.supports({algorithm: "ECDH"}))
			continue;
		if ((ECDHE_ECDSA === suite.keyExchangeAlgorithm) && !(Signature.supports({algorithm: "ECDSA"}) &&
				Key.supports({algorithm: "EC", namedCurve: "P-256"}) && Key.supports({algorithm: "EC", namedCurve: "P-384"})))
			continue;		// public ECDSA chains end in P-384 roots: without P-384, dual-certificate servers must send their RSA chain
		result.push(suite);
	}
	return result;
}

function supportedCurves(curves)
{
	const result = [];
	for (let i = 0; i < curves.length; i++) {
		const name = curveNames[curves[i]];
		if (name && KeyAgreement.supports({algorithm: "ECDH", namedCurve: name}))
			result.push(curves[i]);
	}
	return result;
}

function handshakeDigestUpdate(session, data)
{
	const digests = session.handshakeDigests;
	if (null === digests)
		return;

	if (digests) {
		for (let digest in digests)
			digests[digest].write(data);
		return;
	}

	session.handshakeMessages ??= new SSLStream();
	return session.handshakeMessages.writeChunk(data);
}

function hashAlgorithmName(algorithm)		// TLS HashAlgorithm -> Digest algorithm
{
	switch (algorithm) {
	case 1: return "MD5";
	case 2: return "SHA-1";
	case 3: return "SHA-224";
	case 4: return "SHA-256";
	case 5: return "SHA-384";
	case 6: return "SHA-512";
	}
	throw new TLSError("unsupported hash algorithm");
}

const handshakeProtocol = {
	name: "handshakeProtocol",

	// one message: its 4-byte header from h, its body from s (the record layer loops, so that no frame sits between it and the message)
	unpacketize(session, h, s) {		// the messages a server sends
		session.traceProtocol(this);
		let tbuf = new Uint8Array(4);
		tbuf[0] = h.readChar();		// msgType
		let body = h.readChars(3);
		body = body > 0 ? s.readChunk(body, true) : undefined;
		if (session.tls13?.suite && (server_hello !== tbuf[0])) {		// a 1.3 handshake is under way
			// the transcript hash before this message, for CertificateVerify and Finished; then the message joins the transcript,
			// so that the handler can be a tail call: its frame replaces this one at the handshake's deepest point
			const digest = session.handshakeDigests?.[session.tls13.suite.hash];		// none once the handshake is complete
			const transcript = (digest && ((certificate_verify === tbuf[0]) || (finished === tbuf[0]))) ? digest.read() : undefined;
			session.handshakeProcess = tbuf[0];
			if (body) {
				tbuf[1] = body.byteLength >>> 16;
				tbuf[2] = body.byteLength >>> 8;
				tbuf[3] = body.byteLength;
			}
			handshakeDigestUpdate(session, tbuf.buffer);
			if (body)
				handshakeDigestUpdate(session, body);
			return tls13().unpacketize(session, tbuf[0], new SSLStream(body ?? new Uint8Array(0)), transcript);
		}
		switch (tbuf[0]) {
		case 0 /* hello_request */:
			s = this.helloRequest;
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
		case 20 /* finished */:
			s = this.finished;
			break;
		default:
			throw new TLSError("handshake: unexpected type: " + tbuf[0]);
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
		if (session.tls13?.suite && (server_hello === tbuf[0]))		// 1.3 negotiated: the secrets follow from the transcript through this message
			tls13().afterServerHello(session);
	},
	// extensions<0..2^16-1> -> [{type, data}]. The vector is optional only in a 1.2 ServerHello (RFC 5246 section 7.4.1.4): present
	// when any bytes follow the compression method. Anything that does not decode exactly is a decode_error (RFC 8446 section 6.2).
	parseExtensions(s, required) {
		const result = [];
		if (!required && (0 === s.bytesAvailable))
			return result;
		let length = s.readChars(2);
		while (length > 0) {
			if (length < 4)
				throw new TLSError("extensions: length");
			const type = s.readChars(2);
			const size = s.readChars(2);
			result.push({type, data: s.readChunk(size, true)});
			length -= 4 + size;
		}
		if (length)		// the last extension overran the vector
			throw new TLSError("extensions: length");
		return result;
	},
	digestUpdate(session, data) {		// for a message the 1.3 module packetizes itself (a ClientHello with a binder)
		return handshakeDigestUpdate(session, data);
	},
	packetize(session, msgType, body) {
		session.traceProtocol(this);
		body = body.getChunk();
		let s = new SSLStream(undefined, 1 + 3 + body.byteLength);
		s.writeChar(msgType);
		s.writeChars(body.byteLength, 3);
		s.writeChunk(body);
		let msg = s.getChunk();
		handshakeDigestUpdate(session, msg);
		return recordProtocol.packetize(session, recordProtocol.handshake, msg);
	},

	// protocols
	helloRequest: {
		name: "helloRequest",
		msgType: hello_request,
		unpacketize(session) {		// a request to renegotiate, which is not supported: ignored, as RFC 5246 permits
			session.traceProtocol(this);
		},
	},

	clientHello: {
		name: "clientHello",
		msgType: client_hello,
		// before packetize, from the session's shallowest point: the options, and the key shares of TLS 1.3 (its deepest native call)
		prepare(session) {
			session.options = {...options, ...session.options};
			session.options.tls_elliptic_curves = supportedCurves(session.options.tls_elliptic_curves);
			const offer13 = tls13() && ((session.options.maxProtocolVersion ?? protocolVersion) > protocolVersion);
			session.options.tls_signature_algorithms = signatureAlgorithms(offer13);
			if (offer13)
				tls13().generateShares(session, session.options.tls_elliptic_curves);
		},
		packetize(session) {
			session.traceProtocol(this);
			const s = new SSLStream;
			const minimum = session.options.minProtocolVersion ?? protocolVersion;
			const offer13 = tls13() && ((session.options.maxProtocolVersion ?? protocolVersion) > protocolVersion);
			const retry = session.tls13?.retried;		// after a HelloRetryRequest the ClientHello repeats, with a new key share
			s.writeChars(protocolVersion, 2);
			if (!retry) {
				const random = new Uint8Array(session.random.read(32));
				const gmt_unix_time = ((new Date()).getTime() / 1000) | 0;	// in sec
				random[0] = (gmt_unix_time >>> 24) & 255;
				random[1] = (gmt_unix_time >>> 16) & 255;
				random[2] = (gmt_unix_time >>>  8) & 255;
				random[3] =  gmt_unix_time         & 255;
				session.clientRandom = random.buffer;
				if (offer13 && !session.clientSessionID)
					session.clientSessionID = session.random.read(32);		// legacy_session_id: middlebox compatibility (section D.4)
			}
			s.writeChunk(session.clientRandom);
			const sessionID = session.clientSessionID;
			if (!sessionID || !sessionID.byteLength)
				s.writeChar(0);
			else {
				s.writeChar(sessionID.byteLength);
				s.writeChunk(sessionID);
			}
			let cipherSuites = (minimum > protocolVersion) ? [] : supportedSuites(supportedCipherSuites);
			if (offer13)
				cipherSuites = tls13().supportedSuites().concat(cipherSuites);
			s.writeChars(cipherSuites.length * 2, 2);
			for (let i = 0; i < cipherSuites.length; i++) {
				let val = cipherSuites[i].value;
				s.writeChar(val[0]);
				s.writeChar(val[1]);
			}
			s.writeChar(supportedCompressionMethods.length);
			for (let i = 0; i < supportedCompressionMethods.length; i++)
				s.writeChar(supportedCompressionMethods[i]);
			//
			// TLS extensions
			//
			const es = new SSLStream;
			if (offer13) {
				tls13().writeClientHelloExtensions(session, es);
				session.tls13.offered = true;
			}
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
			if (offer13)
				tls13().writePreSharedKey(session, es);		// last, as its binder requires
			if (es.bytesAvailable) {
				s.writeChars(es.bytesAvailable, 2);
				s.writeChunk(es.getChunk());
			}
			if (session.tls13?.offer)
				return tls13().packetizeClientHello(session, s);
			return handshakeProtocol.packetize(session, client_hello, s);
		},
	},

	serverHello: {
		name: "serverHello",
		msgType: server_hello,
		selectCipherSuite(c1, c2) {
			for (let i = 0, suites = supportedCipherSuites; i < suites.length; i++) {
				const value = suites[i].value;
				if ((value[0] === c1) && (value[1] === c2))
					return suites[i];
			}
			throw new TLSError("serverHello: unsupported cipher");
		},
		unpacketize(session, s) {
			session.traceProtocol(this);
			if (protocolVersion !== s.readChars(2))		// TLS 1.3 keeps 1.2 here, and says 1.3 in supported_versions
				throw new TLSError("serverHello: TLS 1.2 required");
			session.serverRandom = s.readChunk(32);
			const sessionIDLen = s.readChar();
			session.serverSessionID = sessionIDLen > 0 ? s.readChunk(sessionIDLen) : undefined;
			const c1 = s.readChar(), c2 = s.readChar();
			session.compressionMethod = s.readChar();
			const extensions = handshakeProtocol.parseExtensions(s);
			if (s.bytesAvailable)
				throw new TLSError("serverHello: trailing data");
			let version = protocolVersion;
			for (let i = 0; i < extensions.length; i++) {
				if (43 === extensions[i].type) {		// supported_versions: the selected version
					if (2 !== extensions[i].data.byteLength)
						throw new TLSError("serverHello: supported_versions");
					version = (extensions[i].data[0] << 8) | extensions[i].data[1];
				}
			}
			if (session.tls13?.offered) {
				if ((version > protocolVersion) || session.tls13.suite)		// 1.3 selected, or already under way: a 1.2 ServerHello after a HelloRetryRequest is a downgrade
					return tls13().serverHello(session, {version, random: session.serverRandom, sessionID: session.serverSessionID, suite: [c1, c2], compression: session.compressionMethod, extensions});
				const random = new Uint8Array(session.serverRandom);
				for (let i = 0; i < 8; i++) {		// a 1.3 server that chose 1.2 marks its random: not one we offered 1.3 to
					if (random[24 + i] !== downgradeSentinel12[i])
						break;
					if (7 === i)
						throw new TLSError("serverHello: downgrade");
				}
				delete session.tls13;
			}
			if ((session.options.minProtocolVersion ?? protocolVersion) > protocolVersion)
				throw new TLSError("serverHello: TLS 1.3 required");
			session.chosenCipher = this.selectCipherSuite(c1, c2);
			if (!supportedCompressionMethods.includes(session.compressionMethod))
				throw new TLSError("serverHello: unsupported compression");

			session.handshakeDigests = {};
			session.handshakeDigests.SHA256 = new Digest({algorithm: "SHA-256"});	// CertificateVerify always signs SHA256 here
			if (cipherSHA384 === session.chosenCipher.hashAlgorithm)
				session.handshakeDigests.SHA384 = new Digest({algorithm: "SHA-384"});

			let data = session.handshakeMessages.getChunk();
			for (let digest in session.handshakeDigests)
				session.handshakeDigests[digest].write(data);
			delete session.handshakeMessages;
		},
	},

	certificate: {
		name: "certificate",
		msgType: certificate,

		unpacketize(session, s) {
			session.traceProtocol(this);
			const certs = [];
			let ttlSize = s.readChars(3);
			while (ttlSize > 0 && s.bytesAvailable > 0) {
				const size = s.readChars(3);
				certs.push(s.readChunk(size, true));
				ttlSize -= size + 3;
			}
			if (!session.certificateManager.verify(certs, session.options))
				throw new TLSError("certificate: auth err");

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
		// signature algorithms
		rsa: 1,
		ecdsa: 3,

		unpacketize(session, s) {
			session.traceProtocol(this);
			switch (session.chosenCipher.keyExchangeAlgorithm) {
			case ECDHE_RSA:
			case ECDHE_ECDSA: {
				let tbs = new SSLStream();
				let dhparams = {};
				// ServerECDHParams
				//	ECParameters
				//		ECCurveType
				let curve_type = s.readChars(1);
				if (curve_type != 3)	// named_curve
					throw new TLSError("invalid curve type");
				//		NamedCurve
				tbs.writeChars(curve_type, 1)
				let named_curve = s.readChars(2);
				if (!session.options.tls_elliptic_curves.includes(named_curve) || !curveNames[named_curve])
					throw new TLSError("unsupported curve");
				dhparams.named_curve = named_curve;
				dhparams.curve = curveNames[named_curve];
				tbs.writeChars(named_curve, 2);
				//	ECPoint
				let n = s.readChars(1);
				tbs.writeChars(n, 1);
				let ecpoint = s.readChunk(n);
				tbs.writeChunk(ecpoint);
				dhparams.dh_Ys = ecpoint;
				session.dhparams = dhparams;
				const hash_algo = s.readChar();
				const sig_algo = s.readChar();
				let sig = s.readChunk(s.readChars(2));
				const pss = (8 === hash_algo);		// rsa_pss_rsae_sha256..512 (0x0804..0x0806): the hash is in the second byte
				const hash = pss ? hashAlgorithmName(sig_algo) : hashAlgorithmName(hash_algo);
				let verified;
				const digest = new Digest({algorithm: hash});
				digest.write(session.clientRandom);
				digest.write(session.serverRandom);
				digest.write(tbs.getChunk());
				tbs = null;
				const H = digest.read();
				digest.close();
				const key = session.certificateManager.getKey(session.peerCert);
				if (!pss && (sig_algo !== this.rsa) && (sig_algo !== this.ecdsa))
					throw new TLSError("serverKeyExchange: unsupported signature algorithm");
				if (!pss && (sig_algo === this.ecdsa))
					sig = ecdsaSignatureToRaw(sig, key);		// before the Signature, which holds a key until closed
				const signature = pss ? new Signature({algorithm: "RSA-PSS", key, hash}) : (sig_algo === this.rsa) ? new Signature({algorithm: "RSASSA-PKCS1-v1_5", key, hash}) : new Signature({algorithm: "ECDSA", key});
				verified = signature.verify(H, sig);
				signature.close();
				key.close();
				if (!verified) {
					// should send an alert, probably...
					throw new TLSError("serverKeyExchange: failed to verify signature");
				}
				} break;
			case RSA:
				// no server key exchange info
				break;
			default:
				throw new TLSError("serverKeyExchange: unsupported algorithm: " + session.chosenCipher.keyExchangeAlgorithm);
			}
		},
	},

	certificateRequest: {
		name: "certificateRequest",
		msgType: certificate_request,
		unpacketize(session, s) {		// the types, signature algorithms, and authorities are not consulted: the client has the one certificate it was given
			session.traceProtocol(this);
			s.readChunk(s.readChar());		// certificate_types
			s.readChunk(s.readChars(2));		// supported_signature_algorithms
			s.readChunk(s.readChars(2));		// certificate_authorities
			session.clientCerts = session.certificateManager.findPreferredCert();
			// if no clientCerts, proceed to the "certificate" protocol with a null list
		},
	},

	serverHelloDone: {
		name: "serverHelloDone",
		msgType: server_hello_done,
		unpacketize(session, s) {
			session.traceProtocol(this);
			// nothing to unpacketize
		},
	},

	clientKeyExchange: {
		name: "clientKeyExchange",
		msgType: client_key_exchange,
		generateMasterSecret(session, preMasterSecret) {
			session.masterSecret = PRF(session, preMasterSecret, master_secret_label,
				session.clientRandom.concat(session.serverRandom), 48);
		},
		packetize(session) {
			session.traceProtocol(this);
			let s = new SSLStream();
			let preMasterSecret;
			switch (session.chosenCipher.keyExchangeAlgorithm) {
			case RSA:
				var plain = new SSLStream();
				plain.writeChars(protocolVersion, 2);
				plain.writeChunk(session.random.read(46));
				preMasterSecret = plain.getChunk();
				{
					const key = session.certificateManager.getKey(session.peerCert);
					const rsa = new Cipher({algorithm: "RSAES-PKCS1-v1_5", key, direction: "encrypt"});
					const cipher = rsa.encrypt(preMasterSecret);
					rsa.close();
					key.close();
					s.writeChars(cipher.byteLength, 2);
					s.writeChunk(cipher);
				}
				break;
			case ECDHE_RSA:
			case ECDHE_ECDSA:
				if (!session.dhparams)
					throw new TLSError("clientKeyExchange: no DH params");
				{
					const namedCurve = session.dhparams.curve;
					const peer = new Key({algorithm: "EC", format: "raw", namedCurve, data: session.dhparams.dh_Ys});
					let mine, agreement;
					try {		// the keys are released whether or not the agreement succeeds
						mine = new Key({algorithm: "EC", namedCurve, extractable: true});		// the ephemeral key for this handshake: its public point is sent
						const Yc = mine.extract("raw");
						s.writeChars(Yc.byteLength, 1);
						s.writeChunk(Yc);
						agreement = new KeyAgreement({algorithm: "ECDH", key: mine});
						preMasterSecret = agreement.derive(peer);
					}
					finally {
						agreement?.close();
						mine?.close();
						peer.close();
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

	certificateVerify: {		// signs the handshake with the client certificate's RSA key, whatever the key exchange
		name: "certificateVerify",
		msgType: certificate_verify,
		packetize(session, cert) {
			session.traceProtocol(this);
			if (!session.myCert)
				throw new TLSError("certificateVerify: no cert");	// out of sequence
			const s = new SSLStream;
			const key = session.certificateManager.getKey(/* self */);
			const rsa = new Signature({algorithm: "RSASSA-PKCS1-v1_5", key, hash: "SHA-256"});
			s.writeChars(4, 1);	// hash algorithm SHA256
			s.writeChars(1, 1);	// signature algorithm RSA
			const sig = rsa.sign(session.handshakeDigests.SHA256.read());
			rsa.close();
			key.close();
			s.writeChars(sig.byteLength, 2);
			s.writeChunk(sig);
			return handshakeProtocol.packetize(session, certificate_verify, s);
		},
	},

	finished: {
		name: "finished",
		msgType: finished,

		calculateVerifyData(session, server) {
			const digest = ((cipherSHA384 === session.chosenCipher.hashAlgorithm) ? session.handshakeDigests.SHA384 : session.handshakeDigests.SHA256).read();
			return PRF(session, session.masterSecret, server ? server_finished_label : client_finished_label, digest, 12);
		},
		unpacketize(session, s) {
			session.traceProtocol(this);
			delete session.peerCert;
			delete session.certificateManager;
			let verify = this.calculateVerifyData(session, true);
			if (Bin.comp(verify, s.readChunk(verify.byteLength)) != 0) {
				session.masterSecret = null;
				throw new TLSError("finished: auth err");
			}
		},
		packetize(session) {
			session.traceProtocol(this);
			let verify = this.calculateVerifyData(session, false);
			return handshakeProtocol.packetize(session, finished, new SSLStream(verify));
		},
	},
};

Object.freeze(handshakeProtocol, true);

export default handshakeProtocol;
