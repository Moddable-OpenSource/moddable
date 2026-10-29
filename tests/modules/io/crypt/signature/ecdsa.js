/*---
description: ECDSA on P-256 and P-384: Python signatures verify, round trips, digest truncation, r || s encoding
flags: [module]
---*/

import Signature, {hex, toHex} from "./signature_FIXTURE.js";
import Key, {V, rfc6979} from "../key/ec_FIXTURE.js";

for (const name of ["P-256", "P-384"]) {
	const v = V[name];
	using pub = new Key({algorithm: "EC", format: "spki", data: hex(v.spki)});
	using priv = new Key({algorithm: "EC", format: "pkcs8", data: hex(v.pkcs8)});
	using verifier = new Signature({algorithm: "ECDSA", key: pub});
	if (!Signature.supports({algorithm: "ECDSA", hash: "SHA-256"}))
		assert.throws(RangeError, () => new Signature({algorithm: "ECDSA", key: pub, hash: "SHA-256"}), `${name}: deterministic ECDSA (hash) is not provided`);
	assert.throws(RangeError, () => new Signature({algorithm: "ECDSA", key: pub, hash: "SHA-3"}), `${name}: unknown hash`);
	assert.throws(RangeError, () => new Signature({algorithm: "ECDSA", key: pub, saltLength: 32}), `${name}: saltLength is not accepted by ECDSA`);
	assert.sameValue(verifier.algorithm, "ECDSA");
	assert.sameValue(verifier.outputSize, 2 * v.bytes, `${name} signature size`);
	using signer = new Signature({algorithm: "ECDSA", key: priv});
	for (const vec of v.ecdsa) {		// SHA-512 digests are longer than the P-256 order: truncated per FIPS 186-4
		assert.sameValue(verifier.verify(hex(vec.digest), hex(vec.raw)), true, `${name} verify ${vec.hash}`);
		const bad = hex(vec.raw); bad[3] ^= 1;
		assert.sameValue(verifier.verify(hex(vec.digest), bad), false, `${name} tampered ${vec.hash}`);
		const sig = signer.sign(hex(vec.digest));
		assert.sameValue(sig.byteLength, 2 * v.bytes, `${name} signature length`);
		assert.sameValue(verifier.verify(hex(vec.digest), sig), true, `${name} round trip ${vec.hash}`);
		assert.notSameValue(toHex(sig), toHex(signer.sign(hex(vec.digest))), "randomized");
	}
	assert.sameValue(verifier.verify(hex(v.ecdsa[0].digest), hex(v.ecdsa[0].der)), false, "DER is not r || s");
	assert.sameValue(verifier.verify(hex(v.ecdsa[0].digest), new Uint8Array(2 * v.bytes)), false, "all zero");
	assert.sameValue(verifier.verify(hex(v.ecdsa[0].digest), signer.sign(hex(v.ecdsa[0].digest))), true, "fresh signature verifies");
	assert.throws(Error, () => verifier.sign(hex(v.ecdsa[0].digest)), "private key required");
}
using x = new Key({algorithm: "EC", format: "spki", data: hex(V["X25519"].spki)});
assert.throws(TypeError, () => new Signature({algorithm: "ECDSA", key: x}), "X25519 cannot sign");

// deterministic ECDSA (RFC 6979): a hash selects it; the signature of a digest repeats, and verifies as ordinary ECDSA
if (Signature.supports({algorithm: "ECDSA", hash: "SHA-256"})) {
	using priv = new Key({algorithm: "EC", format: "jwk", data: rfc6979.jwk, extractable: true});
	using pub = new Key({algorithm: "EC", format: "raw", namedCurve: "P-256", data: priv.extract("raw")});
	using signer = new Signature({algorithm: "ECDSA", key: priv, hash: "SHA-256"});
	assert.sameValue(signer.hash, "SHA-256", "hash of a deterministic signer");
	using verifier = new Signature({algorithm: "ECDSA", key: pub});
	using hashedVerifier = new Signature({algorithm: "ECDSA", key: pub, hash: "SHA-256"});
	for (const vec of rfc6979.vectors) {
		const sig = signer.sign(hex(vec.digest));
		assert.sameValue(toHex(sig), vec.signature, `RFC 6979 ${vec.message}`);
		assert.sameValue(toHex(signer.sign(hex(vec.digest))), vec.signature, `${vec.message} repeats`);
		assert.sameValue(verifier.verify(hex(vec.digest), sig), true, `${vec.message} verifies without a hash`);
		assert.sameValue(hashedVerifier.verify(hex(vec.digest), sig), true, `${vec.message} verifies with the hash`);
	}
	assert.throws(RangeError, () => signer.sign(new Uint8Array(20)), "the digest must be the hash's output");
}
