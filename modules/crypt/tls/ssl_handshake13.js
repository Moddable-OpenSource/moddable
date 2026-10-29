/*
 * Copyright (c) 2026  Moddable Tech, Inc.
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
 */

/*
	TLS 1.3 (RFC 8446), client side: the handshake after the ClientHello, the traffic keys, and the post-handshake messages

	The 1.2 module builds the ClientHello (calling here for the 1.3 suites and extensions), parses the ServerHello and
	dispatches to serverHello() when the server selects 1.3. From then on the record layer carries the 1.3 record format
	and handshake records come to unpacketize(). A build without this module offers TLS 1.2 only.
*/

import SSLStream from "ssl/stream";
import TLSError from "ssl/error";
import recordProtocol from "ssl/record";
import handshakeProtocol from "ssl/handshake";
import {ecdsaSignatureToRaw} from "ssl/cert";
import {KeySchedule, trafficKeys, finishedVerifyData, hkdfExpandLabel, deriveSecret, emptyHash, hashSize} from "ssl/keyschedule";
import {load as loadTicket, store as storeTicket} from "ssl/tickets";
import Digest from "embedded:crypt/digest";
import Cipher from "embedded:crypt/cipher";
import Key from "embedded:crypt/key";
import Signature from "embedded:crypt/signature";
import KeyAgreement from "embedded:crypt/keyagreement";
import {ECDHE_ECDSA, AES, GCM, SHA256, SHA384} from "ssl/constants";

export const version = 0x304;

// handshake message types (server_hello is 1.2's too: the expected-message check names it)
const server_hello = 2;
const new_session_ticket = 4;
const encrypted_extensions = 8;
const certificate = 11;
const certificate_request = 13;
const certificate_verify = 15;
const finished = 20;
const key_update = 24;
const message_hash = 254;

// extensions
const extension = Object.freeze({
	pre_shared_key: 41,
	supported_versions: 43,
	cookie: 44,
	psk_key_exchange_modes: 45,
	key_share: 51,
});

// the suites, in order of preference; fields the 1.2 code reads (for its trace) carry the nearest 1.2 values
const suites = Object.freeze([
	{ value: [0x13, 0x01], name: "TLS_AES_128_GCM_SHA256", cipher: "AES-GCM", keySize: 16, hash: "SHA-256", hashSize: 32, keyExchangeAlgorithm: ECDHE_ECDSA, cipherAlgorithm: AES, encryptionMode: GCM, hashAlgorithm: SHA256 },
	{ value: [0x13, 0x03], name: "TLS_CHACHA20_POLY1305_SHA256", cipher: "ChaCha20-Poly1305", keySize: 32, hash: "SHA-256", hashSize: 32, keyExchangeAlgorithm: ECDHE_ECDSA, cipherAlgorithm: AES, encryptionMode: GCM, hashAlgorithm: SHA256 },
	{ value: [0x13, 0x02], name: "TLS_AES_256_GCM_SHA384", cipher: "AES-GCM", keySize: 32, hash: "SHA-384", hashSize: 48, keyExchangeAlgorithm: ECDHE_ECDSA, cipherAlgorithm: AES, encryptionMode: GCM, hashAlgorithm: SHA384 },
], true);

const groupNames = Object.freeze({29: "X25519", 23: "P-256", 24: "P-384"});		// NamedGroup -> Key namedCurve

const helloRetryRequestRandom = Uint8Array.fromHex("cf21ad74e59a6111be1d8c021e65b891c2a211167abb8c5e079e09e2c8a8339c");
const serverContext = ArrayBuffer.fromString("TLS 1.3, server CertificateVerify");
const clientContext = ArrayBuffer.fromString("TLS 1.3, client CertificateVerify");

// the suites this build can run
export function supportedSuites()
{
	const result = [];
	for (let i = 0; i < suites.length; i++) {
		const suite = suites[i];
		if (Digest.supports({algorithm: suite.hash}) && Cipher.supports({algorithm: suite.cipher}) && KeyAgreement.supports({algorithm: "ECDH"}))
			result.push(suite);
	}
	return result;
}

function findSuite(c1, c2)
{
	for (let i = 0; i < suites.length; i++) {
		const value = suites[i].value;
		if ((value[0] === c1) && (value[1] === c2))
			return suites[i];
	}
}

/*
	The (EC)DHE shares of the ClientHello, kept for the ServerHello: one for each of the first options.tls13_key_shares of
	groups (the ClientHello's supported_groups), or for the group a HelloRetryRequest selected. The session calls this
	before the ClientHello is packetized, from its shallowest point: the key generation is the deepest native call of the
	handshake, and the stack of the smallest targets is 256 slots.
*/
export function generateShares(session, groups)
{
	const state = session.tls13 ??= {};
	if (state.shares)
		return;
	let wanted;
	if (undefined !== state.retryGroup)
		wanted = [state.retryGroup];
	else
		wanted = groups.slice(0, session.options.tls13_key_shares ?? 1);
	const shares = [];
	for (let i = 0; i < wanted.length; i++) {
		const namedCurve = groupNames[wanted[i]];
		if (!namedCurve)
			continue;
		shares.push({group: wanted[i], key: new Key({algorithm: "EC", namedCurve, extractable: true})});
	}
	state.shares = shares;
}

function closeShares(shares)
{
	for (let i = 0; i < shares.length; i++)
		shares[i].key.close();
}

/*
	The session is closing, or its handshake was given up at any point: the key shares are closed and the secrets dropped
	now rather than when the session is collected.
*/
export function release(session)
{
	const state = session.tls13;
	if (state?.shares)
		closeShares(state.shares);
	session.tls13 = undefined;
}

/*
	ClientHello: the extensions of TLS 1.3 (supported_versions, key_share from the shares generateShares made, and a cookie
	after a HelloRetryRequest).
*/
export function writeClientHelloExtensions(session, es)
{
	const state = session.tls13;
	state.expect = server_hello;
	const also12 = (session.options.minProtocolVersion ?? 0x303) <= 0x303;		// a 1.2 server that reads supported_versions picks from it
	es.writeChars(extension.supported_versions, 2);
	es.writeChars(also12 ? 5 : 3, 2);
	es.writeChar(also12 ? 4 : 2);
	es.writeChars(version, 2);
	if (also12)
		es.writeChars(0x303, 2);
	if (session.options.tickets) {		// tickets wanted: how we would use one (servers issue tickets only to clients that say, section 4.2.9), and a live one for this host
		es.writeChars(extension.psk_key_exchange_modes, 2);
		es.writeChars(2, 2);
		es.writeChar(1);
		es.writeChar(1);		// psk_dhe_ke alone: a resumed connection keeps forward secrecy
		if (!state.retried)
			state.offer = loadTicket(session);
		else if (state.offer && (state.offer.hash !== state.suite.hash))		// after a HelloRetryRequest, only for the suite the server chose
			delete state.offer;
	}

	if (state.cookie) {
		es.writeChars(extension.cookie, 2);
		es.writeChars(2 + state.cookie.byteLength, 2);
		es.writeChars(state.cookie.byteLength, 2);
		es.writeChunk(state.cookie);
	}

	const shares = state.shares;
	let length = 0;
	for (let i = 0; i < shares.length; i++) {
		shares[i].point = shares[i].key.extract("raw");
		length += 2 + 2 + shares[i].point.byteLength;
	}
	es.writeChars(extension.key_share, 2);
	es.writeChars(2 + length, 2);
	es.writeChars(length, 2);
	for (let i = 0; i < shares.length; i++) {
		es.writeChars(shares[i].group, 2);
		es.writeChars(shares[i].point.byteLength, 2);
		es.writeChunk(shares[i].point);
	}
}

// pre_shared_key, the last extension (section 4.2.11): one identity, and room for the binder packetizeClientHello computes
export function writePreSharedKey(session, es)
{
	const offer = session.tls13.offer;
	if (!offer)
		return;
	const size = hashSize(offer.hash);
	const identities = 2 + offer.ticket.byteLength + 4;
	es.writeChars(extension.pre_shared_key, 2);
	es.writeChars(2 + identities + 2 + 1 + size, 2);
	es.writeChars(identities, 2);
	es.writeChars(offer.ticket.byteLength, 2);
	es.writeChunk(offer.ticket);
	es.writeChars(offer.obfuscatedAge, 4);
	es.writeChars(1 + size, 2);
	es.writeChar(size);
	es.writeChunk(new ArrayBuffer(size));
}

/*
	the ClientHello that offers a ticket: its binder is an HMAC, under a key from the early secret of the PSK, of the transcript
	through the message up to (not including) the binders list (section 4.2.11.2). After a HelloRetryRequest the transcript
	already holds message_hash and the retry request; for the first ClientHello it is the truncated message alone.
*/
export function packetizeClientHello(session, body)
{
	const offer = session.tls13.offer;
	const hash = offer.hash;
	const size = hashSize(hash);
	body = body.getChunk();
	const msg = new Uint8Array(4 + body.byteLength);
	msg[0] = 1;		// client_hello
	msg[1] = body.byteLength >> 16;
	msg[2] = body.byteLength >> 8;
	msg[3] = body.byteLength;
	msg.set(body, 4);
	const truncated = msg.byteLength - (2 + 1 + size);
	handshakeProtocol.digestUpdate(session, msg.subarray(0, truncated));
	let transcript;
	const digest = session.handshakeDigests?.[hash];
	if (digest)
		transcript = digest.read();
	else {
		const first = new Digest({algorithm: hash});
		first.write(msg.subarray(0, truncated));
		transcript = first.read();
		first.close();
	}
	const schedule = new KeySchedule(hash);
	const binderKey = deriveSecret(hash, schedule.early(offer.psk), "res binder", emptyHash(hash));
	msg.set(new Uint8Array(finishedVerifyData(hash, binderKey, transcript)), truncated + 2 + 1);
	handshakeProtocol.digestUpdate(session, msg.subarray(truncated));
	return recordProtocol.packetize(session, recordProtocol.handshake, msg);
}

function findExtension(extensions, type)
{
	for (let i = 0; i < extensions.length; i++) {
		if (extensions[i].type === type)
			return extensions[i].data;
	}
}

// a hash of the whole transcript so far
function transcriptHash(session)
{
	return session.handshakeDigests[session.tls13.suite.hash].read();
}

// the traffic keys of a secret, as the record layer uses them: {enc, iv}, with the sequence number restarted
function setKeys(session, secret, direction)
{
	const state = session.tls13;
	const keys = trafficKeys(state.suite.hash, secret, state.suite.keySize);
	const cipher = {
		enc: new Cipher({algorithm: state.suite.cipher, key: keys.key, direction, iv: new Uint8Array(12), tagLength: 16}),
		iv: keys.iv,
	};
	if ("encrypt" === direction) {
		session.clientCipher?.enc.close();
		session.clientCipher = cipher;
		session.writeSeqNum = BigInt(0);
	}
	else {
		session.serverCipher?.enc.close();
		session.serverCipher = cipher;
		session.readSeqNum = BigInt(0);
	}
}

/*
	ServerHello, parsed by the 1.2 module: either a HelloRetryRequest or the start of a 1.3 handshake
*/
export function serverHello(session, hello)
{
	const state = session.tls13;
	if (!state || !state.offered)
		throw new TLSError("serverHello: TLS 1.3 was not offered");
	if (server_hello !== state.expect)
		throw new TLSError("serverHello: unexpected");
	if (version !== hello.version)		// after a HelloRetryRequest nothing but 1.3 is legal (section 4.1.4)
		throw new TLSError("serverHello: version");
	const suite = findSuite(hello.suite[0], hello.suite[1]);
	if (!suite || (state.suite && (state.suite !== suite)))
		throw new TLSError("serverHello: unsupported cipher");
	if (0 !== hello.compression)
		throw new TLSError("serverHello: unsupported compression");
	if (!session.clientSessionID || !hello.sessionID || (session.clientSessionID.byteLength !== hello.sessionID.byteLength) || !equalBytes(session.clientSessionID, hello.sessionID))
		throw new TLSError("serverHello: session id");
	state.suite = suite;
	session.chosenCipher = suite;
	const selected = findExtension(hello.extensions, extension.pre_shared_key);		// our ticket accepted: the only identity offered is 0
	if (selected) {
		if (!state.offer)
			throw new TLSError("serverHello: pre_shared_key not offered");
		if ((2 !== selected.byteLength) || selected[0] || selected[1])
			throw new TLSError("serverHello: pre_shared_key identity");
		if (suite.hash !== state.offer.hash)
			throw new TLSError("serverHello: pre_shared_key hash");
	}

	// the transcript: the suite's hash over the ClientHello so far (the 1.2 code appends this message once we return);
	// after a HelloRetryRequest it already exists and holds message_hash || HelloRetryRequest || ClientHello2
	let digest = session.handshakeDigests?.[suite.hash];
	if (!digest) {
		digest = new Digest({algorithm: suite.hash});
		digest.write(session.handshakeMessages.getChunk());
		delete session.handshakeMessages;
		session.handshakeDigests = {};
		session.handshakeDigests[suite.hash] = digest;
	}

	const keyShare = findExtension(hello.extensions, extension.key_share);
	if (!keyShare)
		throw new TLSError("serverHello: no key share");
	const ks = new SSLStream(keyShare);
	const group = ks.readChars(2);

	if (equalBytes(helloRetryRequestRandom, hello.random)) {		// HelloRetryRequest: the ClientHello again, with the share it asks for
		if (state.retried)
			throw new TLSError("serverHello: second HelloRetryRequest");
		if (selected)
			throw new TLSError("serverHello: pre_shared_key in HelloRetryRequest");
		state.retried = true;
		if (ks.bytesAvailable)		// a retry request's key_share is the group alone (section 4.2.8)
			throw new TLSError("serverHello: key share");
		if (!groupNames[group] || !KeyAgreement.supports({algorithm: "ECDH", namedCurve: groupNames[group]}))
			throw new TLSError("serverHello: unsupported group");
		for (let i = 0; i < state.shares.length; i++) {
			if (state.shares[i].group === group)		// a retry must ask for a share we did not send (section 4.1.4)
				throw new TLSError("serverHello: HelloRetryRequest for a group offered");
		}
		closeShares(state.shares);
		state.shares = undefined;
		state.retryGroup = group;
		const cookie = findExtension(hello.extensions, extension.cookie);
		if (cookie) {
			const cs = new SSLStream(cookie);
			state.cookie = cs.readChunk(cs.readChars(2));
			if (cs.bytesAvailable)
				throw new TLSError("serverHello: cookie");
		}
		// the transcript restarts from message_hash(ClientHello1) (section 4.4.1); the retry request itself follows
		const hash = new Uint8Array(digest.read());
		digest.reset();
		const mh = new SSLStream(undefined, 4 + hash.byteLength);
		mh.writeChar(message_hash);
		mh.writeChars(hash.byteLength, 3);
		mh.writeChunk(hash);
		digest.write(mh.getChunk());
		state.helloRetry = true;
		return;
	}

	// the shared secret, from the share we sent for the server's group
	let share;
	for (let i = 0; i < state.shares.length; i++) {
		if (state.shares[i].group === group)
			share = state.shares[i];
	}
	if (!share)
		throw new TLSError("serverHello: key share group");
	const point = ks.readChunk(ks.readChars(2));
	if (ks.bytesAvailable)
		throw new TLSError("serverHello: key share");
	const peer = new Key({algorithm: "EC", format: "raw", namedCurve: groupNames[group], data: point});
	let agreement;
	try {		// released whether or not the agreement succeeds; the shares, on failure, by release()
		agreement = new KeyAgreement({algorithm: "ECDH", key: share.key});
		state.sharedSecret = agreement.derive(peer);
	}
	finally {
		agreement?.close();
		peer.close();
	}
	closeShares(state.shares);
	delete state.shares;
	state.serverRandom = hello.random;
	if (selected)
		state.resumed = true;
	state.expect = encrypted_extensions;
}

// after the ServerHello joined the transcript: the handshake secrets and keys (section 7.1)
export function afterServerHello(session)
{
	const state = session.tls13;
	if (state.helloRetry) {		// the ClientHello goes again
		delete state.helloRetry;
		session.handshakeProcess = handshakeProtocol.helloRequest.msgType;
		return;
	}
	const schedule = state.schedule = new KeySchedule(state.suite.hash);
	schedule.early(state.resumed ? state.offer.psk : undefined);
	delete state.offer;
	schedule.handshake(state.sharedSecret);
	delete state.sharedSecret;
	const hash = transcriptHash(session);
	state.clientHandshakeSecret = schedule.traffic("c hs traffic", hash);
	state.serverHandshakeSecret = schedule.traffic("s hs traffic", hash);
	setKeys(session, state.serverHandshakeSecret, "decrypt");		// the server's next records are encrypted
}

/*
	the server's encrypted flight, and the post-handshake messages: type and body of a handshake message, and the transcript
	hash before it (CertificateVerify and Finished). The message functions are tail calls: the certificate chain verification
	beneath them is the handshake's deepest point, and the smallest targets have 256 stack slots.

	state.expect is the one message type the handshake may receive next (section 4.4: EncryptedExtensions, an optional
	CertificateRequest, Certificate, CertificateVerify, Finished, in that order; EncryptedExtensions then Finished when the
	server accepted our ticket, section 4.2.11, since the PSK authenticates it); undefined once the handshake is complete,
	when only NewSessionTicket and KeyUpdate may follow. Each handler sets it for the next. Nothing else is accepted: the
	server's Finished proves only possession of the handshake secret, which any peer holding a key share has, so a Finished
	that did not follow a verified CertificateVerify would complete an unauthenticated connection.
*/
export function unpacketize(session, type, s, transcript)
{
	const state = session.tls13;
	if (type !== state.expect) {
		const expected = (undefined === state.expect) ? ((new_session_ticket === type) || (key_update === type)) : ((certificate === type) && (certificate_request === state.expect));
		if (!expected)
			throw new TLSError("handshake: unexpected message " + type);
	}
	switch (type) {
	case encrypted_extensions:
		session.traceProtocol(encryptedExtensions);
		state.expect = state.resumed ? finished : certificate_request;		// or Certificate; a resumed connection is authenticated by its PSK (section 4.3.2)
		handshakeProtocol.parseExtensions(s, true);		// nothing requested that needs an answer
		if (s.bytesAvailable)
			throw new TLSError("encryptedExtensions: trailing data");
		return;
	case certificate_request:
		session.traceProtocol(certificateRequest);
		state.expect = certificate;
		state.certificateRequestContext = s.readChunk(s.readChar());
		handshakeProtocol.parseExtensions(s, true);		// the signature algorithms and authorities are not consulted: the client has the one certificate it was given
		if (s.bytesAvailable)
			throw new TLSError("certificateRequest: trailing data");
		session.clientCerts = session.certificateManager.findPreferredCert();
		return;
	case certificate:
		state.expect = certificate_verify;
		return certificateMessage.unpacketize(session, s);
	case certificate_verify:
		state.expect = finished;
		return certificateVerify.unpacketize(session, s, transcript);
	case finished:
		delete state.expect;		// post-handshake messages only, from here
		return finishedMessage.unpacketize(session, s, transcript);
	case new_session_ticket: {		// section 4.6.1: kept for resumption when the socket has a ticket store, discarded otherwise
		session.traceProtocol(newSessionTicket);
		const lifetime = s.readChars(4) >>> 0;
		const ageAdd = s.readChars(4) >>> 0;
		const nonce = s.readChunk(s.readChar());
		const ticketLength = s.readChars(2);		// ticket<1..2^16-1>
		if (!ticketLength)
			throw new TLSError("newSessionTicket: empty ticket");
		const ticket = s.readChunk(ticketLength);
		handshakeProtocol.parseExtensions(s, true);
		if (s.bytesAvailable)
			throw new TLSError("newSessionTicket: trailing data");
		if (state.resumptionMasterSecret)
			storeTicket(session, lifetime, ageAdd, nonce, ticket);
		return;
		}
	case key_update:
		return keyUpdate.unpacketize(session, s);
	default:
		throw new TLSError("handshake: unexpected type: " + type);
	}
}

const certificateMessage = {
	name: "certificate",
	unpacketize(session, s) {
		session.traceProtocol(this);
		if (s.readChar())		// certificate_request_context: zero length for the server's (section 4.4.2)
			throw new TLSError("certificate: context");
		const certs = [];
		let total = s.readChars(3);
		while (total > 0) {
			const size = s.readChars(3);
			certs.push(s.readChunk(size, true));
			const before = s.bytesAvailable;
			handshakeProtocol.parseExtensions(s, true);		// the entry's extensions are not consulted
			total -= 3 + size + (before - s.bytesAvailable);
		}
		if (total || s.bytesAvailable)		// an entry overran the list, or bytes follow it
			throw new TLSError("certificate: length");
		if (!certs.length || !session.certificateManager.verify(certs, session.options))
			throw new TLSError("certificate: auth err");
		session.peerCert = certs[0].slice(0).buffer;
		session.certificateManager.register(session.peerCert);
	},
};

const certificateVerify = {		// section 4.4.3: the signature covers 64 spaces, the context string, a zero, and the transcript hash
	name: "certificateVerify",
	unpacketize(session, s, transcript) {
		session.traceProtocol(this);
		const scheme = s.readChars(2);
		const signature = s.readChunk(s.readChars(2));
		if (s.bytesAvailable)
			throw new TLSError("certificateVerify: trailing data");
		let hash, ecdsa = false;
		switch (scheme) {
		case 0x0403: hash = "SHA-256"; ecdsa = true; break;		// ecdsa_secp256r1_sha256
		case 0x0503: hash = "SHA-384"; ecdsa = true; break;		// ecdsa_secp384r1_sha384
		case 0x0804: hash = "SHA-256"; break;		// rsa_pss_rsae_sha256
		case 0x0805: hash = "SHA-384"; break;		// rsa_pss_rsae_sha384
		case 0x0806: hash = "SHA-512"; break;		// rsa_pss_rsae_sha512
		default:
			throw new TLSError("certificateVerify: unsupported signature scheme " + scheme);
		}
		const key = session.certificateManager.getKey(session.peerCert);
		if (ecdsa !== ("EC" === key.algorithm)) {
			key.close();
			throw new TLSError("certificateVerify: signature scheme does not match the certificate");
		}
		const digest = signedDigest(hash, serverContext, transcript);		// what can throw comes before the Signature, which holds a key until closed
		const raw = ecdsa ? ecdsaSignatureToRaw(signature, key) : signature;
		const verifier = ecdsa ? new Signature({algorithm: "ECDSA", key}) : new Signature({algorithm: "RSA-PSS", key, hash});
		const verified = verifier.verify(digest, raw);
		verifier.close();
		key.close();
		if (!verified)
			throw new TLSError("certificateVerify: failed to verify signature");
	},
};

const finishedMessage = {
	name: "finished",
	unpacketize(session, s, transcript) {
		session.traceProtocol(this);
		const state = session.tls13;
		const expected = new Uint8Array(finishedVerifyData(state.suite.hash, state.serverHandshakeSecret, transcript));
		if (expected.byteLength !== s.bytesAvailable)		// verify_data[Hash.length], and nothing else
			throw new TLSError("finished: length");
		if (!equalBytes(expected, s.readChunk(expected.byteLength, true)))
			throw new TLSError("finished: auth err");
	},
	packetize(session) {		// the client's Finished
		session.traceProtocol(this);
		const state = session.tls13;
		const verify = finishedVerifyData(state.suite.hash, state.clientHandshakeSecret, transcriptHash(session));
		return handshakeProtocol.packetize(session, finished, new SSLStream(verify));
	},
};

const keyUpdate = {
	name: "keyUpdate",
	unpacketize(session, s) {
		session.traceProtocol(this);
		const state = session.tls13;
		const request = s.readChar();
		if ((request > 1) || s.bytesAvailable)
			throw new TLSError("keyUpdate: bad request");
		state.serverApplicationSecret = hkdfExpandLabel(state.suite.hash, state.serverApplicationSecret, "traffic upd", undefined, state.suite.hashSize);
		setKeys(session, state.serverApplicationSecret, "decrypt");
		if (request)
			state.updateRequested = true;		// answered by the session when it next writes
	},
	packetize(session) {		// ours: update_not_requested
		session.traceProtocol(this);
		const s = new SSLStream(undefined, 1);
		s.writeChar(0);
		return handshakeProtocol.packetize(session, key_update, s);
	},
};

// our flight after the server's Finished: [Certificate, CertificateVerify,] Finished; then the application keys
export function clientFlight(session, s)
{
	const state = session.tls13;
	const schedule = state.schedule;
	const serverFinishedHash = transcriptHash(session);		// through the server's Finished: the application secrets' context
	if (session.clientSessionID) {		// middlebox compatibility: a ClientHello with a session id is followed by a dummy change_cipher_spec
		const ccs = new SSLStream(undefined, 1);
		ccs.writeChar(1);
		session.doProtocol(s, {packetize: session => recordProtocol.packetize(session, recordProtocol.change_cipher_spec, ccs.getChunk())});
	}
	setKeys(session, state.clientHandshakeSecret, "encrypt");
	if (session.clientCerts !== undefined) {
		session.doProtocol(s, clientCertificate, session.clientCerts);
		if (session.myCert)
			session.doProtocol(s, clientCertificateVerify);
	}
	session.doProtocol(s, finishedMessage);

	schedule.master();
	state.clientApplicationSecret = schedule.traffic("c ap traffic", serverFinishedHash);
	state.serverApplicationSecret = schedule.traffic("s ap traffic", serverFinishedHash);
	setKeys(session, state.clientApplicationSecret, "encrypt");
	setKeys(session, state.serverApplicationSecret, "decrypt");
	if (session.options.tickets)		// for the tickets the server may now send: the transcript through our Finished (section 7.1)
		state.resumptionMasterSecret = schedule.traffic("res master", transcriptHash(session));
	delete state.clientHandshakeSecret;
	delete state.serverHandshakeSecret;
	delete state.schedule;
	delete state.certificateRequestContext;
	delete session.peerCert;		// the certificates are done with once our flight is out (unlike 1.2, ours follows the server's Finished)
	delete session.certificateManager;
}

// a KeyUpdate the server requested, before our next application record
export function updateKeys(session, s)
{
	const state = session.tls13;
	delete state.updateRequested;
	session.doProtocol(s, keyUpdate);
	state.clientApplicationSecret = hkdfExpandLabel(state.suite.hash, state.clientApplicationSecret, "traffic upd", undefined, state.suite.hashSize);
	setKeys(session, state.clientApplicationSecret, "encrypt");
}

// CertificateVerify (section 4.4.3): the digest of 64 spaces, the context string, a zero, and the transcript hash
function signedDigest(hash, contextString, transcript)
{
	const digest = new Digest({algorithm: hash});
	const spaces = new Uint8Array(64);
	spaces.fill(0x20);
	digest.write(spaces);
	digest.write(contextString);
	digest.write(Uint8Array.of(0));
	digest.write(transcript);
	const result = digest.read();
	digest.close();
	return result;
}

function equalBytes(a, b)
{
	a = new Uint8Array(a);
	b = new Uint8Array(b);
	if (a.byteLength !== b.byteLength)
		return false;
	let diff = 0;
	for (let i = 0; i < a.byteLength; i++)
		diff |= a[i] ^ b[i];
	return 0 === diff;
}

// the messages we send, in the shape session.doProtocol expects
const clientCertificate = {
	name: "certificate",
	packetize(session, certs) {
		session.traceProtocol(this);
		const s = new SSLStream;
		const context = session.tls13.certificateRequestContext;
		s.writeChar(context.byteLength);
		s.writeChunk(context);
		let length = 0;
		for (let i = 0; i < certs.length; i++)
			length += 3 + certs[i].byteLength + 2;
		s.writeChars(length, 3);
		for (let i = 0; i < certs.length; i++) {
			s.writeChars(certs[i].byteLength, 3);
			s.writeChunk(certs[i]);
			s.writeChars(0, 2);		// no extensions
		}
		if (certs.length > 0)
			session.myCert = certs[0];
		return handshakeProtocol.packetize(session, certificate, s);
	},
};

const clientCertificateVerify = {		// rsa_pss_rsae_sha256 with the client's RSA key
	name: "certificateVerify",
	packetize(session) {
		session.traceProtocol(this);
		const key = session.certificateManager.getKey(/* self */);
		const signer = new Signature({algorithm: "RSA-PSS", key, hash: "SHA-256"});
		const signature = signer.sign(signedDigest("SHA-256", clientContext, transcriptHash(session)));
		signer.close();
		key.close();
		const s = new SSLStream(undefined, 4 + signature.byteLength);
		s.writeChars(0x0804, 2);
		s.writeChars(signature.byteLength, 2);
		s.writeChunk(signature);
		return handshakeProtocol.packetize(session, certificate_verify, s);
	},
};

// names for the trace
const encryptedExtensions = Object.freeze({name: "encryptedExtensions"});
const certificateRequest = Object.freeze({name: "certificateRequest"});
const newSessionTicket = Object.freeze({name: "newSessionTicket"});

Object.freeze({certificateMessage, certificateVerify, finishedMessage, keyUpdate, clientCertificate, clientCertificateVerify, groupNames}, true);
