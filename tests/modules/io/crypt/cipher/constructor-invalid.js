/*---
description:
flags: [module]
---*/

import Cipher, {V, hex} from "./cipher_FIXTURE.js";

const key16 = hex(V.ecb[0].key), key32 = hex(V.ecb[2].key);
const iv16 = hex(V.cbc[0].iv), iv12 = hex(V.gcm[0].iv);

assert.throws(Error, () => new Cipher({key: key16, direction: "encrypt"}), "algorithm required");
assert.throws(Error, () => new Cipher({algorithm: "AES-XYZ", key: key16, direction: "encrypt"}), "unknown algorithm");
assert.throws(Error, () => new Cipher({algorithm: "AES-ECB", direction: "encrypt"}), "key required");
assert.throws(Error, () => new Cipher({algorithm: "AES-ECB", key: key16}), "direction required");
assert.throws(RangeError, () => new Cipher({algorithm: "AES-ECB", key: key16, direction: "both"}), "bad direction");
assert.throws(RangeError, () => new Cipher({algorithm: "AES-CBC", key: key16, direction: "encrypt", iv: iv16, padding: "pkcs5"}), "bad padding");
assert.throws(Error, () => new Cipher({algorithm: "AES-CBC", key: key16, direction: "encrypt"}), "iv required for CBC");
assert.throws(Error, () => new Cipher({algorithm: "AES-GCM", key: key16, direction: "encrypt"}), "iv required for GCM");

assert.throws(RangeError, () => new Cipher({algorithm: "AES-ECB", key: new Uint8Array(15), direction: "encrypt"}), "AES key 15 bytes");
assert.throws(RangeError, () => new Cipher({algorithm: "AES-ECB", key: new Uint8Array(0), direction: "encrypt"}), "AES key empty");
assert.throws(RangeError, () => new Cipher({algorithm: "AES-CBC", key: key16, direction: "encrypt", iv: iv12}), "CBC iv 12 bytes");
assert.throws(RangeError, () => new Cipher({algorithm: "AES-CTR", key: key16, direction: "encrypt", iv: new Uint8Array(8)}), "CTR iv 8 bytes");
assert.throws(RangeError, () => new Cipher({algorithm: "AES-GCM", key: key16, direction: "encrypt", iv: new Uint8Array(0)}), "GCM iv empty");
assert.throws(RangeError, () => new Cipher({algorithm: "AES-GCM", key: key16, direction: "encrypt", iv: iv12, tagLength: 3}), "tagLength 3");
assert.throws(RangeError, () => new Cipher({algorithm: "AES-GCM", key: key16, direction: "encrypt", iv: iv12, tagLength: 17}), "tagLength 17");

// options that the algorithm does not use are errors, not ignored
assert.throws(RangeError, () => new Cipher({algorithm: "AES-CTR", key: key16, direction: "encrypt", iv: iv16, padding: "pkcs7"}), "padding on CTR");
assert.throws(RangeError, () => new Cipher({algorithm: "AES-GCM", key: key16, direction: "encrypt", iv: iv12, padding: "pkcs7"}), "padding on GCM");
assert.throws(RangeError, () => new Cipher({algorithm: "AES-CTR", key: key16, direction: "encrypt", iv: iv16, tagLength: 16}), "tagLength on CTR");
assert.throws(RangeError, () => new Cipher({algorithm: "AES-CBC", key: key16, direction: "encrypt", iv: iv16, additionalData: new Uint8Array(4)}), "additionalData on CBC");
assert.throws(RangeError, () => new Cipher({algorithm: "AES-ECB", key: key16, direction: "encrypt", iv: iv16}), "iv on ECB");
assert.throws(RangeError, () => new Cipher({algorithm: "AES-GCM", key: key16, direction: "encrypt", iv: iv12, hash: "SHA-256"}), "hash on GCM");
assert.throws(RangeError, () => new Cipher({algorithm: "AES-CBC", key: key16, direction: "encrypt", iv: iv16, label: new Uint8Array(4)}), "label on CBC");
{
	using ecb = new Cipher({algorithm: "AES-ECB", key: key16, direction: "encrypt"});
	assert.throws(RangeError, () => ecb.reset({iv: iv16}), "iv on ECB reset");
}

assert.throws(TypeError, () => new Cipher({algorithm: "AES-ECB", key: "0123456789abcdef", direction: "encrypt"}), "key string");
assert.throws(TypeError, () => new Cipher({algorithm: "AES-CBC", key: key16, direction: "encrypt", iv: 16}), "iv number");
assert.throws(TypeError, () => new Cipher({algorithm: "AES-GCM", key: key16, direction: "encrypt", iv: iv12, additionalData: "aad"}), "additionalData string");

// ChaCha20 key and iv sizes (when supported): 32-byte key, 12-byte iv only
for (const [label, options] of [
	["16-byte key", {algorithm: "ChaCha20", key: key16, direction: "encrypt", iv: iv12}],
	["16-byte iv", {algorithm: "ChaCha20", key: key32, direction: "encrypt", iv: iv16}],
	["ChaCha20-Poly1305 16-byte iv", {algorithm: "ChaCha20-Poly1305", key: key32, direction: "encrypt", iv: iv16}],
]) {
	try {
		new Cipher(options);
		throw new Test262Error(`ChaCha20 accepted a ${label}`);
	}
	catch (e) {
		if (!(e instanceof RangeError) && !/unsupported algorithm|not enabled/.test(e.message))
			throw e;
	}
}
