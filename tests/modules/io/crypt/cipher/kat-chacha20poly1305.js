/*---
description:
flags: [module]
---*/

import {V, hex, toHex, optionsFor, expectedCiphertext, tryConstruct} from "./cipher_FIXTURE.js";

// RFC 8439 section 2.8.2, then generated vectors covering empty and unaligned plaintext and additional data
const supported = (() => {
	using c = tryConstruct(optionsFor("ChaCha20-Poly1305", V.chacha20poly1305[0], "encrypt"));
	if (c) {
		assert.sameValue(c.blockSize, 64, "blockSize");
		assert.sameValue(c.tagLength, 16, "tagLength");
	}
	return !!c;
})();

if (supported) {
	for (const vec of V.chacha20poly1305) {
		const label = `aad${vec.aad.length / 2} pt${vec.pt.length / 2}`;
		{
			using c = tryConstruct(optionsFor("ChaCha20-Poly1305", vec, "encrypt"));
			assert.sameValue(toHex(c.encrypt(hex(vec.pt))), expectedCiphertext(vec), `${label} encrypt`);
		}
		{
			using c = tryConstruct(optionsFor("ChaCha20-Poly1305", vec, "decrypt"));
			assert.sameValue(toHex(c.decrypt(hex(vec.ct + vec.tag))), vec.pt, `${label} decrypt`);
		}
	}

	const vec = V.chacha20poly1305[0];

	// RFC 8439 defines only the full tag: unlike AES-GCM, no shortened tags
	{
		using c = tryConstruct(optionsFor("ChaCha20-Poly1305", vec, "encrypt", {tagLength: 16}));
		assert.sameValue(c.tagLength, 16, "tagLength 16 accepted");
		for (const tagLength of [12, 8, 4]) {
			assert.throws(RangeError, () => tryConstruct(optionsFor("ChaCha20-Poly1305", vec, "encrypt", {tagLength})), `tagLength ${tagLength} rejected`);
			assert.throws(RangeError, () => c.reset({tagLength}), `reset tagLength ${tagLength} rejected`);
		}
	}

	// a changed ciphertext, tag, or additional data fails, and the output of the final call is zeroed
	for (const [what, index] of [["ciphertext", 3], ["tag", -1]]) {
		const bad = hex(vec.ct + vec.tag);
		bad[(index < 0) ? bad.byteLength + index : index] ^= 1;
		const out = new Uint8Array(bad.byteLength).fill(0xee);
		using c = tryConstruct(optionsFor("ChaCha20-Poly1305", vec, "decrypt"));
		assert.throws(Error, () => c.decrypt(bad, {output: out}), `bad ${what} throws`);
		assert.sameValue(out.subarray(0, bad.byteLength - 16).every(b => 0 === b), true, `plaintext zeroed on bad ${what}`);
	}
	{
		const aad = hex(vec.aad); aad[0] ^= 1;
		using c = tryConstruct({...optionsFor("ChaCha20-Poly1305", vec, "decrypt"), additionalData: aad});
		assert.throws(Error, () => c.decrypt(hex(vec.ct + vec.tag)), "bad additional data throws");
	}

	// each operation authenticates the additional data under its own key, so reset() replays it
	{
		using c = tryConstruct(optionsFor("ChaCha20-Poly1305", vec, "encrypt"));
		const a = toHex(c.encrypt(hex(vec.pt)));
		c.reset();
		assert.sameValue(toHex(c.encrypt(hex(vec.pt))), a, "same output after reset()");

		const other = V.chacha20poly1305[4];
		c.reset({iv: hex(other.iv)});
		using d = tryConstruct({...optionsFor("ChaCha20-Poly1305", vec, "encrypt"), iv: hex(other.iv)});
		assert.sameValue(toHex(c.encrypt(hex(vec.pt))), toHex(d.encrypt(hex(vec.pt))), "additional data retained across reset({iv})");

		c.reset({iv: hex(vec.iv), additionalData: new Uint8Array(0)});
		using e = tryConstruct({algorithm: "ChaCha20-Poly1305", key: hex(vec.key), direction: "encrypt", iv: hex(vec.iv)});
		assert.sameValue(toHex(c.encrypt(hex(vec.pt))), toHex(e.encrypt(hex(vec.pt))), "reset with empty additional data equals none");

		c.reset({additionalData: hex(vec.aad)});
		assert.sameValue(toHex(c.encrypt(hex(vec.pt))), a, "reset({additionalData}) restores the vector");
	}

	assert.throws(RangeError, () => tryConstruct({...optionsFor("ChaCha20-Poly1305", vec, "encrypt"), iv: new Uint8Array(16)}), "iv must be 12 bytes");
	assert.throws(RangeError, () => tryConstruct({...optionsFor("ChaCha20-Poly1305", vec, "encrypt"), key: new Uint8Array(16)}), "key must be 32 bytes");
}
