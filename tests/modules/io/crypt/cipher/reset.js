/*---
description:
flags: [module]
---*/

import Cipher, {V, hex, toHex, bytes} from "./cipher_FIXTURE.js";

const key = hex(V.ecb[0].key), iv16 = hex(V.cbc[0].iv), iv12 = hex(V.gcm[0].iv), aad = hex(V.gcm[1].aad);
const plain = bytes(48, 9);

// reset() restarts with the same iv and aad
{
	using c = new Cipher({algorithm: "AES-GCM", key, direction: "encrypt", iv: iv12, additionalData: aad});
	const a = toHex(c.encrypt(plain));
	c.reset();
	assert.sameValue(toHex(c.encrypt(plain)), a, "same output after reset()");
	if (Cipher.supports({algorithm: "AES-GCM", more: true})) {
		c.reset();
		c.encrypt(plain.subarray(0, 16), {more: true});
		c.reset();		// abandon a streaming operation
		assert.sameValue(toHex(c.encrypt(plain)), a, "reset mid-stream");
	}
}

// reset with a new iv changes the output; aad retained
{
	using c = new Cipher({algorithm: "AES-GCM", key, direction: "encrypt", iv: iv12, additionalData: aad});
	const a = toHex(c.encrypt(plain));
	const iv2 = hex(V.gcm[0].iv); iv2[0] ^= 0xff;
	c.reset({iv: iv2});
	const b = toHex(c.encrypt(plain));
	assert.notSameValue(b, a, "new iv");
	using d = new Cipher({algorithm: "AES-GCM", key, direction: "encrypt", iv: iv2, additionalData: aad});
	assert.sameValue(toHex(d.encrypt(plain)), b, "aad retained across reset({iv})");
	c.reset({iv: iv12, additionalData: new Uint8Array(0)});
	using e = new Cipher({algorithm: "AES-GCM", key, direction: "encrypt", iv: iv12});
	assert.sameValue(toHex(c.encrypt(plain)), toHex(e.encrypt(plain)), "reset with empty aad equals no aad");
}

// reset with tagLength
if (!Cipher.supports({algorithm: "AES-GCM", tagLength: 8})) {		// CryptoKit: the 16-byte tag only
	using c = new Cipher({algorithm: "AES-GCM", key, direction: "encrypt", iv: iv12});
	assert.throws(RangeError, () => c.reset({tagLength: 8}), "shortened tag unsupported");
}
else {
	using c = new Cipher({algorithm: "AES-GCM", key, direction: "encrypt", iv: iv12});
	const full = new Uint8Array(c.encrypt(plain));
	c.reset({tagLength: 8});
	assert.sameValue(c.tagLength, 8, "tagLength after reset");
	const short = new Uint8Array(c.encrypt(plain));
	assert.sameValue(short.byteLength, 48 + 8, "short tag length");
	assert.sameValue(toHex(short), toHex(full.subarray(0, 56)), "short tag is a prefix");
	assert.throws(RangeError, () => c.reset({tagLength: 2}), "bad tagLength");
}

// CBC and CTR reset reuse the iv; ChaCha20 too
for (const options of [{algorithm: "AES-CBC", iv: iv16}, {algorithm: "AES-CTR", iv: iv16}]) {
	using c = new Cipher({...options, key, direction: "encrypt"});
	const a = toHex(c.encrypt(plain));
	c.reset();
	assert.sameValue(toHex(c.encrypt(plain)), a, `${options.algorithm}: reset`);
	assert.throws(RangeError, () => c.reset({iv: iv12}), `${options.algorithm}: bad iv on reset`);
	assert.throws(RangeError, () => c.reset({tagLength: 16}), `${options.algorithm}: tagLength on reset`);
	assert.throws(RangeError, () => c.reset({additionalData: aad}), `${options.algorithm}: additionalData on reset`);
}

// reset options, when passed, must be an object
{
	using c = new Cipher({algorithm: "AES-CTR", key, direction: "encrypt", iv: iv16});
	c.reset();
	c.reset({});
	assert.throws(TypeError, () => c.reset(undefined), "reset(undefined)");
	assert.throws(TypeError, () => c.reset(null), "reset(null)");
}
