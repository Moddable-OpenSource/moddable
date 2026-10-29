/*---
description: AES-128-GCM over record-sized messages (the TLS 1.3 server flight is one record of several KB): tag and ciphertext against Node's OpenSSL
flags: [module]
---*/

import Cipher, {hex, toHex} from "./cipher_FIXTURE.js";
import Digest from "embedded:crypt/digest";

// key 07 x16, iv 09 x12, additional data 17 03 03 0b c8, plaintext byte i = i & 255; the SHA-256 of the ciphertext (without the tag) and the tag
const vectors = [
	{length: 1500, tag: "3a2caa13909564be74986cf14b061698", ct: "cb3a9f38d4e0b78fd3c156fea7c57b491a7abf5dc36c9402a70648a9e26406fe"},
	{length: 3000, tag: "3fcc672411d77c7544b4c4f27525fb3f", ct: "795fcbb0e03aa6775ea289664024b388201f17c0146cba57e2d7e58fde7e31d3"},
	{length: 4096, tag: "eb2e481874c9b348144355db6b8238d2", ct: "705d4b5873826a7244850731c741f7256104d688182fa53819ca886f450fc1a4"},
	{length: 5000, tag: "742877c25af460b9e6b1ace79120f847", ct: "7121d007f962e3b2ae307c82794ea3b709a5df2a0befa7929be46bffe19e78ad"},
];
const key = new Uint8Array(16).fill(7);
const iv = new Uint8Array(12).fill(9);
const additionalData = Uint8Array.of(0x17, 3, 3, 0x0b, 0xc8);

for (const vec of vectors) {
	const plaintext = new Uint8Array(vec.length);
	for (let i = 0; i < vec.length; i++)
		plaintext[i] = i & 255;

	const encrypt = new Cipher({algorithm: "AES-GCM", key, iv, additionalData, direction: "encrypt"});
	const sealed = new Uint8Array(encrypt.encrypt(plaintext));
	encrypt.close();
	assert.sameValue(sealed.byteLength, vec.length + 16, `${vec.length}: sealed length`);
	assert.sameValue(toHex(sealed.subarray(vec.length)), vec.tag, `${vec.length}: tag`);
	const digest = new Digest({algorithm: "SHA-256"});
	digest.write(sealed.subarray(0, vec.length));
	assert.sameValue(toHex(digest.read()), vec.ct, `${vec.length}: ciphertext`);
	digest.close();

	// decrypt in place, as the TLS record layer does
	const decrypt = new Cipher({algorithm: "AES-GCM", key, iv, additionalData, direction: "decrypt"});
	const n = decrypt.decrypt(sealed, {output: sealed});
	decrypt.close();
	assert.sameValue(n, vec.length, `${vec.length}: decrypted length`);
	for (let i = 0; i < vec.length; i++) {
		if (sealed[i] !== (i & 255))
			throw new Test262Error(`${vec.length}: plaintext differs at ${i}`);
	}
}
