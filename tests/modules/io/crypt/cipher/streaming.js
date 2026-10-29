/*---
description:
flags: [module]
---*/

import Cipher, {V, hex, toHex, bytes, tryConstruct} from "./cipher_FIXTURE.js";

const key = hex(V.ecb[2].key), iv16 = hex(V.cbc[0].iv), iv12 = hex(V.gcm[0].iv), aad = hex(V.gcm[1].aad);

// chunked encrypt equals one-shot; non-final chunks are blockSize multiples, final any length
function chunkedEncrypt(options, plain, sizes) {
	using c = tryConstruct({...options, key, direction: "encrypt"});
	if (!c) return undefined;
	const parts = [];
	let offset = 0;
	for (const size of sizes) {
		assert.sameValue(size % c.blockSize, 0, "test chunking");
		const out = new Uint8Array(c.encrypt(plain.subarray(offset, offset + size), {more: true}));
		assert.sameValue(out.byteLength, size, `${options.algorithm}: encrypt chunk output equals input`);
		parts.push(out);
		offset += size;
	}
	parts.push(new Uint8Array(c.encrypt(plain.subarray(offset))));
	return parts.map(toHex).join("");
}

function oneShot(options, direction, data) {
	using c = tryConstruct({...options, key, direction});
	return c ? toHex(c[direction](data)) : undefined;
}

const plain = bytes(200, 7);
for (const options of [
	{algorithm: "AES-ECB", padding: "pkcs7"},
	{algorithm: "AES-CBC", iv: iv16},
	{algorithm: "AES-CBC", iv: iv16, padding: "pkcs7"},
	{algorithm: "AES-CTR", iv: iv16},
	{algorithm: "AES-GCM", iv: iv12, additionalData: aad},
	{algorithm: "ChaCha20", iv: iv12},
	{algorithm: "ChaCha20-Poly1305", iv: iv12},
	{algorithm: "ChaCha20-Poly1305", iv: iv12, additionalData: aad},
]) {
	if (!Cipher.supports({algorithm: options.algorithm, more: true})) continue;		// CryptoKit's authenticated modes are one shot
	const bs = ("ChaCha20" === options.algorithm || "ChaCha20-Poly1305" === options.algorithm) ? 64 : 16;
	const aligned = plain.subarray(0, 192);		// 12 blocks of 16, 3 blocks of 64
	const expectedAligned = oneShot(options, "encrypt", aligned);
	if (undefined === expectedAligned) continue;
	assert.sameValue(chunkedEncrypt(options, aligned, [bs, bs * 2]), expectedAligned, `${options.algorithm}: chunked encrypt (aligned total)`);
	if (!("AES-CBC" === options.algorithm && !options.padding)) {
		const expected = oneShot(options, "encrypt", plain);
		assert.sameValue(chunkedEncrypt(options, plain, [bs, bs * 2]), expected, `${options.algorithm}: chunked encrypt (unaligned final)`);
	}

	// flush with no data after more: true
	{
		using c = new Cipher({...options, key, direction: "encrypt"});
		const a = new Uint8Array(c.encrypt(aligned, {more: true}));
		const b = new Uint8Array(c.encrypt());
		assert.sameValue(toHex(a) + toHex(b), expectedAligned, `${options.algorithm}: encrypt() flush`);
		const trailer = c.tagLength || (options.padding ? 16 : 0);
		assert.sameValue(b.byteLength, trailer, `${options.algorithm}: flush emits only the trailer`);
	}

	// chunked decrypt: implementation defers a possible trailer; sum of counts equals plaintext length
	{
		const src = ("AES-CBC" === options.algorithm && !options.padding) ? aligned : plain;
		const ct = hex(oneShot(options, "encrypt", src));
		using c = new Cipher({...options, key, direction: "decrypt"});
		const trailerHold = c.tagLength || (options.padding ? 16 : 0);
		const out = new Uint8Array(ct.byteLength);
		let n = 0, offset = 0;
		for (const size of [bs, bs * 2]) {
			const count = c.decrypt(ct.subarray(offset, offset + size), {output: out.subarray(n), more: true});
			const expectedCount = Math.max(0, offset + size - trailerHold) - n;
			assert.sameValue(count, expectedCount, `${options.algorithm}: deferred output count`);
			n += count;
			offset += size;
		}
		n += c.decrypt(ct.subarray(offset), {output: out.subarray(n)});
		assert.sameValue(toHex(out.subarray(0, n)), toHex(src), `${options.algorithm}: chunked decrypt`);
	}

	// streaming decrypt ending with a data-free flush
	if (options.padding || options.additionalData) {
		const body = plain.subarray(0, 176);
		const ct = hex(oneShot(options, "encrypt", body));		// body + 16-byte trailer: 192 bytes, a multiple of every blockSize
		using c = new Cipher({...options, key, direction: "decrypt"});
		const a = new Uint8Array(c.decrypt(ct, {more: true}));
		const b = new Uint8Array(c.decrypt());
		assert.sameValue(toHex(a) + toHex(b), toHex(body), `${options.algorithm}: streaming decrypt, flush`);
	}

	// AEAD: final chunk shorter than the tag, so the tag spans deferred bytes and new data
	if (options.additionalData) {
		const ct = hex(oneShot(options, "encrypt", plain));		// 200 + 16 bytes
		const cut = 208 - (208 % bs);
		using c = new Cipher({...options, key, direction: "decrypt"});
		const a = new Uint8Array(c.decrypt(ct.subarray(0, cut), {more: true}));
		const b = new Uint8Array(c.decrypt(ct.subarray(cut)));
		assert.sameValue(toHex(a) + toHex(b), toHex(plain), `${options.algorithm}: trailer split across calls`);
	}
}

// BYOB flush with undefined data
if (Cipher.supports({algorithm: "AES-GCM", more: true})) {
	using c = new Cipher({algorithm: "AES-GCM", key, direction: "encrypt", iv: iv12});
	c.encrypt(plain.subarray(0, 32), {more: true});
	const out = new Uint8Array(16);
	assert.sameValue(c.encrypt(undefined, {output: out}), 16, "encrypt(undefined, {output}) emits the tag");
}
