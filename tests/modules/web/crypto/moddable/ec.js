/*---
description:
flags: [async, module]
---*/

import {subtle, hex, toHex, assertRejects} from "./webcrypto_FIXTURE.js";
import {V as E} from "../../../io/crypt/key/ec_FIXTURE.js";

const message = hex(E.message);
(async () => {
	for (const curve of ["P-256", "P-384"]) {
		const v = E[curve];
		// ECDSA: import, known-answer verify, round trip, export
		const pub = await subtle.importKey("spki", hex(v.spki), {name: "ECDSA", namedCurve: curve}, true, ["verify"]);
		assert.sameValue(pub.algorithm.name, "ECDSA");
		assert.sameValue(pub.algorithm.namedCurve, curve);
		assert.sameValue(pub.type, "public");
		const priv = await subtle.importKey("pkcs8", hex(v.pkcs8), {name: "ECDSA", namedCurve: curve}, true, ["sign"]);
		for (const vec of v.ecdsa) {
			assert.sameValue(await subtle.verify({name: "ECDSA", hash: vec.hash}, pub, hex(vec.raw), message), true, `${curve} verify ${vec.hash}`);
			const sig = await subtle.sign({name: "ECDSA", hash: vec.hash}, priv, message);
			assert.sameValue(sig.byteLength, 2 * v.bytes, `${curve} signature length`);
			assert.sameValue(await subtle.verify({name: "ECDSA", hash: vec.hash}, pub, sig, message), true, `${curve} round trip ${vec.hash}`);
		}
		assert.sameValue(await subtle.verify({name: "ECDSA", hash: "SHA-256"}, pub, hex(v.ecdsa[0].der), message), false, "DER signature rejected");
		assert.sameValue(toHex(await subtle.exportKey("spki", pub)), v.spki, `${curve} spki export`);
		assert.sameValue(toHex(await subtle.exportKey("raw", pub)), v.raw, `${curve} raw export`);
		assert.sameValue(toHex(await subtle.exportKey("pkcs8", priv)), v.pkcs8, `${curve} pkcs8 export`);
		const jwk = await subtle.exportKey("jwk", priv);
		assert.sameValue(jwk.crv, curve);
		assert.sameValue(jwk.alg, ("P-256" === curve) ? "ES256" : "ES384");
		assert.sameValue(jwk.d, v.jwk_private.d, `${curve} jwk d`);
		const raw = await subtle.importKey("raw", hex(v.raw), {name: "ECDSA", namedCurve: curve}, true, ["verify"]);
		assert.sameValue(toHex(await subtle.exportKey("spki", raw)), v.spki, `${curve} raw import`);
		// ECDH: shared secret against Python, bit-length truncation
		const me = await subtle.importKey("pkcs8", hex(v.pkcs8), {name: "ECDH", namedCurve: curve}, false, ["deriveBits", "deriveKey"]);
		const peer = await subtle.importKey("spki", hex(v.peer_spki), {name: "ECDH", namedCurve: curve}, true, []);
		assert.sameValue(toHex(await subtle.deriveBits({name: "ECDH", public: peer}, me, null)), v.shared, `${curve} ECDH shared secret`);
		assert.sameValue(toHex(await subtle.deriveBits({name: "ECDH", public: peer}, me, 64)), v.shared.slice(0, 16), `${curve} ECDH 64 bits`);
		await assertRejects(subtle.deriveBits({name: "ECDH", public: peer}, me, 8 * v.bytes + 8), "OperationError", "length too long");
		const derived = await subtle.deriveKey({name: "ECDH", public: peer}, me, {name: "AES-GCM", length: 256}, true, ["encrypt"]);
		assert.sameValue(toHex(await subtle.exportKey("raw", derived)), v.shared.slice(0, 64), `${curve} deriveKey AES-256`);
		// generated pairs agree
		const a = await subtle.generateKey({name: "ECDH", namedCurve: curve}, false, ["deriveBits"]);
		const b = await subtle.generateKey({name: "ECDH", namedCurve: curve}, false, ["deriveBits"]);
		assert.sameValue(a.publicKey.usages.length, 0, "ECDH public key has no usages");
		assert.sameValue(toHex(await subtle.deriveBits({name: "ECDH", public: b.publicKey}, a.privateKey, null)),
			toHex(await subtle.deriveBits({name: "ECDH", public: a.publicKey}, b.privateKey, null)), `${curve} generated pairs agree`);
		const signer = await subtle.generateKey({name: "ECDSA", namedCurve: curve}, true, ["sign", "verify"]);
		const sig = await subtle.sign({name: "ECDSA", hash: "SHA-256"}, signer.privateKey, message);
		assert.sameValue(await subtle.verify({name: "ECDSA", hash: "SHA-256"}, signer.publicKey, sig, message), true, `${curve} generated pair signs`);
		await assertRejects(subtle.deriveBits({name: "ECDH", public: pub}, me, null), "InvalidAccessError", "ECDSA key as ECDH public");
	}
	// X25519
	{
		const v = E.X25519;
		const me = await subtle.importKey("pkcs8", hex(v.pkcs8), {name: "X25519"}, true, ["deriveBits"]);
		const peer = await subtle.importKey("raw", hex(v.peer_raw), {name: "X25519"}, true, []);
		assert.sameValue(toHex(await subtle.deriveBits({name: "X25519", public: peer}, me, null)), v.shared, "X25519 shared secret");
		assert.sameValue(toHex(await subtle.exportKey("pkcs8", me)), v.pkcs8, "X25519 pkcs8 export");
		assert.sameValue(toHex(await subtle.exportKey("raw", peer)), v.peer_raw, "X25519 raw export");
		const jwk = await subtle.exportKey("jwk", me);
		assert.sameValue(jwk.kty, "OKP");
		assert.sameValue(jwk.crv, "X25519");
		assert.sameValue(jwk.d, v.jwk_private.d, "X25519 jwk d");
		const again = await subtle.importKey("jwk", jwk, {name: "X25519"}, true, ["deriveBits"]);
		assert.sameValue(toHex(await subtle.deriveBits({name: "X25519", public: peer}, again, null)), v.shared, "X25519 jwk round trip");
		const a = await subtle.generateKey({name: "X25519"}, false, ["deriveBits"]);
		const b = await subtle.generateKey({name: "X25519"}, false, ["deriveBits"]);
		assert.sameValue(toHex(await subtle.deriveBits({name: "X25519", public: b.publicKey}, a.privateKey, null)),
			toHex(await subtle.deriveBits({name: "X25519", public: a.publicKey}, b.privateKey, null)), "X25519 generated pairs agree");
	}
	// errors
	await assertRejects(subtle.importKey("spki", hex(E["P-256"].spki), {name: "ECDSA"}, true, ["verify"]), TypeError, "namedCurve required");
	await assertRejects(subtle.generateKey({name: "ECDSA", namedCurve: "P-521"}, true, ["sign"]), "NotSupportedError", "P-521 unsupported");
	await assertRejects(subtle.importKey("spki", hex(E["P-256"].spki), {name: "ECDSA", namedCurve: "P-384"}, true, ["verify"]), "DataError", "curve mismatch");
	await assertRejects(subtle.importKey("spki", hex(E["P-256"].spki), {name: "ECDSA", namedCurve: "P-256"}, true, ["sign"]), "SyntaxError", "public key cannot sign");
	await assertRejects(subtle.generateKey({name: "ECDSA", namedCurve: "P-256"}, true, ["verify"]), "SyntaxError", "private key needs a usage");
})().then($DONE, $DONE);
