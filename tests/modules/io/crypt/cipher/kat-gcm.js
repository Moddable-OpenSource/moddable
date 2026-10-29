/*---
description:
flags: [module]
---*/

import Cipher, {V, hex, toHex, oneShot, optionsFor, expectedCiphertext} from "./cipher_FIXTURE.js";

for (const vec of V.gcm) {
	if (!Cipher.supports({algorithm: "AES-GCM", iv: hex(vec.iv)})) continue;		// CryptoKit: a nonce of at least 12 bytes
	const label = `GCM-${vec.key.length * 4} iv${vec.iv.length / 2} aad${vec.aad.length / 2} pt${vec.pt.length / 2}`;
	assert.sameValue(toHex(oneShot(optionsFor("AES-GCM", vec, "encrypt"), hex(vec.pt))), expectedCiphertext(vec), `${label} encrypt`);
	assert.sameValue(toHex(oneShot(optionsFor("AES-GCM", vec, "decrypt"), hex(vec.ct + vec.tag))), vec.pt, `${label} decrypt`);
}

// shortened tags are a prefix of the full tag
for (const tagLength of [12, 8, 4]) {
	if (!Cipher.supports({algorithm: "AES-GCM", tagLength})) continue;		// CryptoKit: the 16-byte tag only
	const vec = V.gcm[1];
	const ct = oneShot(optionsFor("AES-GCM", vec, "encrypt", {tagLength}), hex(vec.pt));
	assert.sameValue(toHex(ct), expectedCiphertext(vec, tagLength), `tagLength ${tagLength} encrypt`);
	assert.sameValue(toHex(oneShot(optionsFor("AES-GCM", vec, "decrypt", {tagLength}), ct)), vec.pt, `tagLength ${tagLength} decrypt`);
}

// NIST GCM test case 2: K = 0, IV = 0, P = 0^128
{
	const zero = new Uint8Array(16);
	const out = oneShot({algorithm: "AES-GCM", key: zero, direction: "encrypt", iv: new Uint8Array(12)}, zero);
	assert.sameValue(toHex(out), V.gcm_tc2.ct + V.gcm_tc2.tag, "GCM TC2");
}

// authentication failure throws and zeroes the output of the final call
{
	const vec = V.gcm[1];
	const bad = hex(vec.ct + vec.tag);
	bad[bad.byteLength - 1] ^= 1;
	const out = new Uint8Array(bad.byteLength);
	out.fill(0xee);
	using c = new Cipher(optionsFor("AES-GCM", vec, "decrypt"));
	assert.throws(Error, () => c.decrypt(bad, {output: out}), "bad tag throws");
	assert.sameValue(out.subarray(0, bad.byteLength - 16).every(b => 0 === b), true, "plaintext zeroed on failure");
	assert.throws(Error, () => c.decrypt(new Uint8Array(0)), "operation complete after failure");
}

// modified AAD fails
{
	const vec = V.gcm[1];
	const aad = hex(vec.aad); aad[0] ^= 1;
	using c = new Cipher({...optionsFor("AES-GCM", vec, "decrypt"), additionalData: aad});
	assert.throws(Error, () => c.decrypt(hex(vec.ct + vec.tag)), "bad aad throws");
}
