/*---
description:
flags: [module]
---*/

import Cipher, {V, hex, bytes} from "./cipher_FIXTURE.js";

const key = hex(V.ecb[0].key), iv16 = hex(V.cbc[0].iv), iv12 = hex(V.gcm[0].iv);

// the direction is fixed at construction: the other method throws
{
	using e = new Cipher({algorithm: "AES-GCM", key, direction: "encrypt", iv: iv12});
	assert.throws(Error, () => e.decrypt(bytes(32)), "decrypt on an encrypting instance");
	using d = new Cipher({algorithm: "AES-CBC", key, direction: "decrypt", iv: iv16});
	assert.throws(Error, () => d.encrypt(bytes(16)), "encrypt on a decrypting instance");
}

// non-final chunk must be a blockSize multiple
{
	using c = new Cipher({algorithm: "AES-CTR", key, direction: "encrypt", iv: iv16});
	assert.throws(RangeError, () => c.encrypt(bytes(15), {more: true}), "15 bytes with more");
	assert.throws(RangeError, () => c.encrypt(bytes(17), {more: true}), "17 bytes with more");
	c.encrypt(bytes(16), {more: true});
	c.encrypt(bytes(7));
	assert.throws(Error, () => c.encrypt(bytes(16)), "encrypt after final");
	assert.throws(Error, () => c.encrypt(), "flush after final");
	c.reset();
	c.encrypt(bytes(7));
}

// block mode without padding requires an aligned total
{
	using c = new Cipher({algorithm: "AES-CBC", key, direction: "encrypt", iv: iv16});
	assert.throws(RangeError, () => c.encrypt(bytes(20)), "CBC none 20 bytes");
	using d = new Cipher({algorithm: "AES-ECB", key, direction: "decrypt"});
	assert.throws(RangeError, () => d.decrypt(bytes(20)), "ECB none decrypt 20 bytes");
}

// output too small
{
	using c = new Cipher({algorithm: "AES-GCM", key, direction: "encrypt", iv: iv12});
	assert.throws(RangeError, () => c.encrypt(bytes(32), {output: new Uint8Array(47)}), "GCM encrypt needs data + tag");
	c.reset();
	c.encrypt(bytes(32), {output: new Uint8Array(48)});
	using d = new Cipher({algorithm: "AES-CBC", key, direction: "encrypt", iv: iv16, padding: "pkcs7"});
	assert.throws(RangeError, () => d.encrypt(bytes(32), {output: new Uint8Array(47)}), "pkcs7 encrypt needs data + block");
	using e = new Cipher({algorithm: "AES-CTR", key, direction: "encrypt", iv: iv16});
	assert.throws(RangeError, () => e.encrypt(bytes(32), {output: new Uint8Array(31)}), "CTR needs data length");
}

// decrypt with less data than the trailer
{
	using c = new Cipher({algorithm: "AES-GCM", key, direction: "decrypt", iv: iv12});
	assert.throws(RangeError, () => c.decrypt(bytes(15)), "GCM decrypt 15 bytes");
	using d = new Cipher({algorithm: "AES-CBC", key, direction: "decrypt", iv: iv16, padding: "pkcs7"});
	assert.throws(RangeError, () => d.decrypt(new Uint8Array(0)), "pkcs7 decrypt empty");
	assert.throws(RangeError, () => d.decrypt(bytes(20)), "pkcs7 decrypt unaligned");
}

// invalid padding on decrypt
{
	using e = new Cipher({algorithm: "AES-CBC", key, direction: "encrypt", iv: iv16});
	const ct = new Uint8Array(e.encrypt(bytes(32)));		// plaintext ends in 0x.. not valid pkcs7 (random)
	using d = new Cipher({algorithm: "AES-CBC", key, direction: "decrypt", iv: iv16, padding: "pkcs7"});
	assert.throws(Error, () => d.decrypt(ct), "bad padding throws");
}

// non-buffer arguments
{
	using c = new Cipher({algorithm: "AES-CTR", key, direction: "encrypt", iv: iv16});
	assert.throws(TypeError, () => c.encrypt("abc"), "string data");
	assert.throws(TypeError, () => c.encrypt(42), "number data");
	assert.throws(TypeError, () => c.encrypt(null), "null data");
	assert.throws(TypeError, () => c.encrypt(bytes(16), {output: "abc"}), "string output");
	assert.throws(TypeError, () => c.encrypt(bytes(16), {output: 16}), "number output");
	assert.throws(TypeError, () => c.encrypt(bytes(16), {output: undefined}), "undefined output");
	assert.throws(TypeError, () => c.encrypt(bytes(16), undefined), "options undefined");
	assert.throws(TypeError, () => c.encrypt(bytes(16), null), "options null");
}

// in-place not permitted on non-final decrypt calls once bytes are deferred
{
	using c = new Cipher({algorithm: "AES-GCM", key, direction: "decrypt", iv: iv12});
	const buf = bytes(64);
	if (Cipher.supports({algorithm: "AES-GCM", more: true})) {
		c.decrypt(buf.subarray(0, 32), {output: buf, more: true});		// first call: nothing deferred yet, allowed
		assert.throws(RangeError, () => c.decrypt(buf.subarray(32, 64), {output: buf.subarray(32), more: true}), "in-place with deferred bytes");
	}
	else
		assert.throws(RangeError, () => c.decrypt(buf.subarray(0, 32), {output: buf, more: true}), "more is not supported for an authenticated mode");
}
