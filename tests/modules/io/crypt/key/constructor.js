/*---
description:
flags: [module]
---*/

import Key, {V, hex, toHex, publicKey, privateKey} from "./rsa_FIXTURE.js";

for (const [format, data, type] of [
	["pkcs1", hex(V.key.pkcs1_public), "public"],
	["pkcs1", hex(V.key.pkcs1_private), "private"],
	["spki", hex(V.key.spki), "public"],
	["pkcs8", hex(V.key.pkcs8), "private"],
	["jwk", V.key.jwk_public, "public"],
	["jwk", V.key.jwk_private, "private"],
]) {
	using k = new Key({algorithm: "RSA", format, data});
	assert.sameValue(k.algorithm, "RSA", `${format} algorithm`);
	assert.sameValue(k.type, type, `${format} type`);
	assert.sameValue(k.keySize, 2048, `${format} size`);
	assert.sameValue(k.extractable, false, `${format}: not extractable by default`);
}
{
	using k = new Key({algorithm: "RSA", format: "pkcs8", data: hex(V.key.pkcs8), extractable: true});
	assert.sameValue(k.extractable, true, "extractable private key");
	using small = new Key({algorithm: "RSA", format: "spki", data: hex(V.key1024.spki)});
	assert.sameValue(small.keySize, 1024, "1024-bit key");
}
// every encoding of the same key yields the same PKCS#1 bytes
{
	const expected = V.key.pkcs1_public;
	for (const [format, data] of [["spki", hex(V.key.spki)], ["jwk", V.key.jwk_public], ["pkcs1", new Uint8Array(hex(V.key.pkcs1_public))]]) {
		using k = new Key({algorithm: "RSA", format, data, extractable: true});
		assert.sameValue(toHex(k.extract("pkcs1")), expected, `${format} normalizes to PKCS#1`);
	}
	const expectedPrivate = V.key.pkcs1_private;
	for (const [format, data] of [["pkcs8", hex(V.key.pkcs8)], ["jwk", V.key.jwk_private]]) {
		using k = new Key({algorithm: "RSA", format, data, extractable: true});
		assert.sameValue(toHex(k.extract("pkcs1")), expectedPrivate, `${format} normalizes to PKCS#1 private`);
	}
}
assert.throws(RangeError, () => new Key({algorithm: "RSA", format: "spki", data: hex(V.key.spki), namedCurve: "P-256"}), "namedCurve is not accepted by RSA");
assert.sameValue(Key.prototype[Symbol.dispose], Key.prototype.close, "Symbol.dispose is close");
