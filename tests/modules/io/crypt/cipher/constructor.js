/*---
description:
flags: [module]
---*/

import Cipher, {V, hex, tryConstruct} from "./cipher_FIXTURE.js";

const key16 = hex(V.ecb[0].key), key24 = hex(V.ecb[1].key), key32 = hex(V.ecb[2].key);
const iv16 = hex(V.cbc[0].iv), iv12 = hex(V.gcm[0].iv);

for (const [algorithm, blockSize, iv, keys] of [
	["AES-ECB", 16, undefined, [key16, key24, key32]],
	["AES-CBC", 16, iv16, [key16, key24, key32]],
	["AES-CTR", 16, iv16, [key16, key24, key32]],
	["AES-GCM", 16, iv12, [key16, key24, key32]],
	["ChaCha20", 64, iv12, [key32]],
]) {
	for (const key of keys) {
		for (const direction of ["encrypt", "decrypt"]) {
			const options = {algorithm, key, direction};
			if (iv) options.iv = iv;
			const c = tryConstruct(options);
			if (!c) continue;
			assert.sameValue(c.blockSize, blockSize, `${algorithm}: blockSize`);
			assert.sameValue(c.keySize, key.byteLength * 8, `${algorithm}: keySize in bits`);
			assert.sameValue(c.tagLength, ("AES-GCM" === algorithm) ? 16 : 0, `${algorithm}: tagLength`);
			c.close();
		}
	}
}

// tagLength option (AEAD only)
if (Cipher.supports({algorithm: "AES-GCM", tagLength: 12})) {
	const c = new Cipher({algorithm: "AES-GCM", key: key16, direction: "encrypt", iv: iv12, tagLength: 12});
	assert.sameValue(c.tagLength, 12, "GCM tagLength 12");
	c.close();
}

// padding option accepted for block modes
for (const padding of ["none", "pkcs7"]) {
	const c = new Cipher({algorithm: "AES-CBC", key: key16, direction: "encrypt", iv: iv16, padding});
	c.close();
	const d = new Cipher({algorithm: "AES-ECB", key: key16, direction: "decrypt", padding});
	d.close();
}
// padding "none" is accepted by algorithms that do not pad
for (const [algorithm, iv] of [["AES-CTR", iv16], ["AES-GCM", iv12]])
	new Cipher({algorithm, key: key16, direction: "encrypt", iv, padding: "none"}).close();

// iv forms
{
	for (const iv of [hex(V.gcm[8].iv), hex(V.gcm[9].iv)]) {	// 8 and 16 byte GCM nonces
		if (Cipher.supports({algorithm: "AES-GCM", iv}))
			new Cipher({algorithm: "AES-GCM", key: key16, direction: "encrypt", iv}).close();
		else
			assert.throws(RangeError, () => new Cipher({algorithm: "AES-GCM", key: key16, direction: "encrypt", iv}), `${iv.byteLength}-byte nonce unsupported`);
	}
}

// Symbol.dispose aliases close
assert.sameValue(Cipher.prototype[Symbol.dispose], Cipher.prototype.close, "Symbol.dispose is close");
{
	let c;
	{
		using inner = new Cipher({algorithm: "AES-ECB", key: key16, direction: "encrypt"});
		c = inner;
	}
	assert.throws(SyntaxError, () => c.blockSize, "disposed by using");
}

// an option property that is present is used as given: undefined is not a byte buffer
assert.throws(TypeError, () => new Cipher({algorithm: "AES-ECB", key: key16, direction: "encrypt", iv: undefined}), "iv: undefined");
assert.throws(TypeError, () => new Cipher({algorithm: "AES-GCM", key: key16, direction: "encrypt", iv: iv12, additionalData: undefined}), "additionalData: undefined");
