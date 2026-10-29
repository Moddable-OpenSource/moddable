/*---
description: TLS 1.3 key schedule and record protection against the RFC 8448 1-RTT trace, over the ECMA-419 classes
flags: [module]
---*/

import {hkdfExtract, hkdfLabel, hkdfExpandLabel, deriveSecret, trafficKeys, finishedVerifyData, emptyHash, KeySchedule} from "ssl/keyschedule";
import {block, blocks} from "./rfc8448_FIXTURE.js";
import Digest from "embedded:crypt/digest";
import Cipher from "embedded:crypt/cipher";
import Key from "embedded:crypt/key";
import KeyAgreement from "embedded:crypt/keyagreement";

const hex = s => Uint8Array.fromHex(s);
const toHex = b => (b instanceof ArrayBuffer ? new Uint8Array(b) : b).toHex();
const hash = "SHA-256";
const b64url = bytes => bytes.toBase64({alphabet: "base64url", omitPadding: true});

function transcriptHash(...messages) {
	const digest = new Digest({algorithm: hash});
	for (let i = 0; i < messages.length; i++)
		digest.write(hex(messages[i]));
	const result = digest.read();
	digest.close();
	return result;
}

// (EC)DHE: the client's x25519 private key and the server's public key agree on the IKM of the handshake secret
{
	const client = block("create an ephemeral x25519 key pair", "client"), server = block("create an ephemeral x25519 key pair", "server");
	using priv = new Key({algorithm: "EC", format: "jwk", data: {kty: "OKP", crv: "X25519", d: b64url(hex(client["private key"])), x: b64url(hex(client["public key"]))}});
	using pub = new Key({algorithm: "EC", format: "raw", namedCurve: "X25519", data: hex(server["public key"])});
	using agreement = new KeyAgreement({algorithm: "ECDH", key: priv});
	assert.sameValue(toHex(agreement.derive(pub)), block('extract secret "handshake"').IKM, "x25519 shared secret");
}

// the secrets, step by step
const early = block('extract secret "early"');
const earlySecret = hkdfExtract(hash, undefined, hex(early.IKM));
assert.sameValue(toHex(earlySecret), early.secret, "early secret: HKDF-Extract(0, 0)");

const derivedForHandshake = block('derive secret for handshake "tls13 derived"');
assert.sameValue(toHex(hkdfLabel("derived", emptyHash(hash), 32)), derivedForHandshake.info, "HkdfLabel of \"derived\"");
assert.sameValue(toHex(deriveSecret(hash, hex(derivedForHandshake.PRK), "derived", hex(derivedForHandshake.hash))), derivedForHandshake.expanded, "derived (handshake)");

const handshake = block('extract secret "handshake"');
const handshakeSecret = hkdfExtract(hash, hex(handshake.salt), hex(handshake.IKM));
assert.sameValue(toHex(handshakeSecret), handshake.secret, "handshake secret");

for (const label of ["c hs traffic", "s hs traffic", "c ap traffic", "s ap traffic", "exp master", "res master"]) {
	const b = block(`derive secret "tls13 ${label}"`);
	assert.sameValue(toHex(hkdfLabel(label, hex(b.hash), 32)), b.info, `HkdfLabel of "${label}"`);
	assert.sameValue(toHex(deriveSecret(hash, hex(b.PRK), label, hex(b.hash))), b.expanded, `derive secret "${label}"`);
}

const master = block('extract secret "master"');
assert.sameValue(toHex(hkdfExtract(hash, hex(master.salt), hex(master.IKM))), master.secret, "master secret");

// the chain as the class walks it
{
	const schedule = new KeySchedule(hash);
	assert.sameValue(toHex(schedule.early()), early.secret, "KeySchedule.early");
	assert.sameValue(toHex(schedule.handshake(hex(handshake.IKM))), handshake.secret, "KeySchedule.handshake");
	const hs = block('derive secret "tls13 s hs traffic"');
	assert.sameValue(toHex(schedule.traffic("s hs traffic", hex(hs.hash))), hs.expanded, "KeySchedule.traffic");
	assert.sameValue(toHex(schedule.master()), master.secret, "KeySchedule.master");
	const ap = block('derive secret "tls13 c ap traffic"');
	assert.sameValue(toHex(schedule.traffic("c ap traffic", hex(ap.hash))), ap.expanded, "KeySchedule.traffic (application)");
}

// transcript hashes: the trace's contexts are the hashes of the messages it shows
const messages = ["ClientHello", "ServerHello", "EncryptedExtensions", "Certificate", "CertificateVerify"].map(name => block(`construct ${"EncryptedExtensions" === name ? "an" : "a"} ${name} handshake message`)[name]);
assert.sameValue(toHex(transcriptHash(messages[0], messages[1])), block('derive secret "tls13 s hs traffic"').hash, "transcript through ServerHello");
const serverFinishedMessage = block("construct a Finished handshake message", "server").Finished;
assert.sameValue(toHex(transcriptHash(...messages, serverFinishedMessage)), block('derive secret "tls13 c ap traffic"').hash, "transcript through server Finished");

// traffic keys and finished
for (const [title, side] of [["derive write traffic keys for handshake data", "server"], ["derive write traffic keys for application data", "server"], ["derive read traffic keys for handshake data", "server"], ["derive write traffic keys for application data", "client"]]) {
	const b = block(title, side);
	const keys = trafficKeys(hash, hex(b.PRK), 16);
	assert.sameValue(toHex(hkdfLabel("key", undefined, 16)), b["key info"], `${side} ${title}: key info`);
	assert.sameValue(toHex(keys.key), b["key expanded"], `${side} ${title}: key`);
	assert.sameValue(toHex(keys.iv), b["iv expanded"], `${side} ${title}: iv`);
}
{
	const b = block('calculate finished "tls13 finished"', "server");
	assert.sameValue(toHex(hkdfExpandLabel(hash, hex(b.PRK), "finished", undefined, 32)), b.expanded, "server finished key");
	assert.sameValue(toHex(finishedVerifyData(hash, hex(b.PRK), transcriptHash(...messages))), b.finished, "server Finished verify_data");
	const c = block('calculate finished "tls13 finished"', "client");
	assert.sameValue(toHex(finishedVerifyData(hash, hex(c.PRK), transcriptHash(...messages, serverFinishedMessage))), c.finished, "client Finished verify_data");
}

// record protection (section 5.2): nonce = iv xor sequence, additional data = the record header, plaintext = content || type
function protect(direction, keys, sequence, header, content, type) {
	const iv = new Uint8Array(keys.iv.slice(0));		// a copy: the XOR must not touch the traffic IV
	for (let i = 0; i < 8; i++) {		// byte by byte: a shift count of 32 or more wraps in JavaScript
		iv[11 - i] ^= sequence & 0xff;
		sequence = Math.floor(sequence / 256);
	}
	using cipher = new Cipher({algorithm: "AES-GCM", key: keys.key, direction, iv, additionalData: header, tagLength: 16});
	if ("encrypt" === direction) {
		const plaintext = new Uint8Array(content.byteLength + 1);
		plaintext.set(content);
		plaintext[content.byteLength] = type;
		return cipher.encrypt(plaintext);
	}
	return cipher.decrypt(content);
}
const serverHandshakeKeys = trafficKeys(hash, hex(block("derive write traffic keys for handshake data", "server").PRK), 16);
const clientHandshakeKeys = trafficKeys(hash, hex(block("derive read traffic keys for handshake data", "server").PRK), 16);
const serverApplicationKeys = trafficKeys(hash, hex(block("derive write traffic keys for application data", "server").PRK), 16);
const clientApplicationKeys = trafficKeys(hash, hex(block("derive write traffic keys for application data", "client").PRK), 16);
const serverHandshakeRecords = blocks("send handshake record", "server");
for (const [label, record, keys, sequence, type] of [
	["server handshake flight", serverHandshakeRecords[1], serverHandshakeKeys, 0, 0x16],
	["client Finished", blocks("send handshake record", "client")[1], clientHandshakeKeys, 0, 0x16],
	["NewSessionTicket", serverHandshakeRecords[2], serverApplicationKeys, 0, 0x16],
	["client application data", block("send application_data record", "client"), clientApplicationKeys, 0, 0x17],
	["server application data", block("send application_data record", "server"), serverApplicationKeys, 1, 0x17],
	["client alert", block("send alert record", "client"), clientApplicationKeys, 1, 0x15],
	["server alert", block("send alert record", "server"), serverApplicationKeys, 2, 0x15],
]) {
	const complete = hex(record["complete record"]);
	const header = complete.subarray(0, 5), body = complete.subarray(5);
	assert.sameValue(toHex(protect("encrypt", keys, sequence, header, hex(record.payload), type)), toHex(body), `${label}: encrypt`);
	const plaintext = new Uint8Array(protect("decrypt", keys, sequence, header, body));
	assert.sameValue(plaintext[plaintext.byteLength - 1], type, `${label}: inner content type`);
	assert.sameValue(toHex(plaintext.subarray(0, plaintext.byteLength - 1)), record.payload, `${label}: decrypt`);
}
