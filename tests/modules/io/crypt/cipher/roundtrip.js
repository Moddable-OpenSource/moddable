/*---
description:
flags: [module]
---*/

import Cipher, {V, hex, toHex, bytes, tryConstruct} from "./cipher_FIXTURE.js";

const key = hex(V.ecb[2].key), iv16 = hex(V.cbc[0].iv), iv12 = hex(V.gcm[0].iv), aad = hex(V.gcm[1].aad);
const cases = [
	{algorithm: "AES-ECB", padding: "pkcs7"},
	{algorithm: "AES-CBC", iv: iv16, padding: "pkcs7"},
	{algorithm: "AES-CTR", iv: iv16},
	{algorithm: "AES-GCM", iv: iv12, additionalData: aad},
	...(Cipher.supports({algorithm: "AES-GCM", tagLength: 12}) ? [{algorithm: "AES-GCM", iv: iv12, tagLength: 12}] : []),		// CryptoKit: the 16-byte tag only
	{algorithm: "ChaCha20", iv: iv12},
	{algorithm: "ChaCha20-Poly1305", iv: iv12, additionalData: aad},
];

for (const options of cases) {
	for (const length of [0, 1, 15, 16, 17, 61, 64, 65, 1000]) {
		const plain = bytes(length, length + 1);
		using e = tryConstruct({...options, key, direction: "encrypt"});
		if (!e) break;
		const ct = new Uint8Array(e.encrypt(plain));
		const trailer = e.tagLength || (options.padding ? 16 - (length % 16) : 0);
		assert.sameValue(ct.byteLength, length + trailer, `${options.algorithm} ${length}B ciphertext length`);
		using d = tryConstruct({...options, key, direction: "decrypt"});
		assert.sameValue(toHex(d.decrypt(ct)), toHex(plain), `${options.algorithm} ${length}B round trip`);
	}
}
