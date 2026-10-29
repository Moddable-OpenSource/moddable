/*---
description:
flags: [module]
---*/

import Cipher, {V, hex, toHex, bytes, tryConstruct} from "./cipher_FIXTURE.js";

const key = hex(V.ecb[2].key), iv16 = hex(V.cbc[0].iv), iv12 = hex(V.gcm[0].iv), aad = hex(V.gcm[1].aad);

for (const options of [
	{algorithm: "AES-ECB"},
	{algorithm: "AES-CBC", iv: iv16},
	{algorithm: "AES-CBC", iv: iv16, padding: "pkcs7"},
	{algorithm: "AES-CTR", iv: iv16},
	{algorithm: "AES-GCM", iv: iv12, additionalData: aad},
	{algorithm: "ChaCha20", iv: iv12},
	{algorithm: "ChaCha20-Poly1305", iv: iv12, additionalData: aad},
]) {
	const plain = bytes(128, 3);
	using e = tryConstruct({...options, key, direction: "encrypt"});
	if (!e) continue;
	const expected = toHex(e.encrypt(plain));
	e.reset();

	// one-shot in place: the buffer has room for the trailer
	const buf = new Uint8Array(128 + 16);
	buf.set(plain);
	const n = e.encrypt(buf.subarray(0, 128), {output: buf});
	assert.sameValue(toHex(buf.subarray(0, n)), expected, `${options.algorithm}: encrypt in place`);

	using d = new Cipher({...options, key, direction: "decrypt"});
	const m = d.decrypt(buf.subarray(0, n), {output: buf});
	assert.sameValue(toHex(buf.subarray(0, m)), toHex(plain), `${options.algorithm}: decrypt in place`);

	// TLS-style: decrypt record in place, result shorter by the trailer
	d.reset();
	const rec = hex(expected);
	const p = d.decrypt(rec, {output: rec});
	assert.sameValue(p, 128, `${options.algorithm}: in-place decrypt count`);
	assert.sameValue(toHex(rec.subarray(0, p)), toHex(plain), `${options.algorithm}: in-place decrypt`);

	// aligned streaming encrypt in place
	if (!Cipher.supports({algorithm: options.algorithm, more: true})) continue;		// CryptoKit's authenticated modes are one shot
	e.reset();
	buf.set(plain);
	const bs = e.blockSize;
	let o = e.encrypt(buf.subarray(0, bs), {output: buf, more: true});
	o += e.encrypt(buf.subarray(bs, 128), {output: buf.subarray(bs)});
	assert.sameValue(toHex(buf.subarray(0, o)), expected, `${options.algorithm}: streaming encrypt in place`);
}
