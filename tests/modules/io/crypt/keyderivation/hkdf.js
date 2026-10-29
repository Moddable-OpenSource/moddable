/*---
description: HKDF (RFC 5869), its Extract and Expand steps, streamed reads and the output limit
flags: [module]
---*/

import KeyDerivation, {HKDF, hex, toHex} from "./keyderivation_FIXTURE.js";

for (const v of HKDF) {
	const options = {algorithm: "HKDF", hash: v.hash, key: hex(v.ikm), info: hex(v.info)};
	if (v.salt.length)
		options.salt = hex(v.salt);
	const kdf = new KeyDerivation(options);
	assert.sameValue(kdf.algorithm, "HKDF");
	assert.sameValue(kdf.hash, v.hash);
	assert.sameValue(toHex(kdf.read(v.length)), v.okm, `${v.hash} HKDF`);
	kdf.close();

	if (!v.salt.length) {		// an explicitly empty salt is the same as none
		const emptySalt = new KeyDerivation({...options, salt: new Uint8Array(0)});
		assert.sameValue(toHex(emptySalt.read(v.length)), v.okm, "empty salt");
		emptySalt.close();
	}

	const extract = new KeyDerivation({algorithm: "HKDF-Extract", hash: v.hash, key: hex(v.ikm), ...(v.salt.length ? {salt: hex(v.salt)} : {})});
	const prk = extract.read(v.prk.length / 2);
	assert.sameValue(toHex(prk), v.prk, "HKDF-Extract");
	assert.sameValue(extract.read(1), undefined, "the PRK is the whole output of Extract");
	extract.close();

	const expand = new KeyDerivation({algorithm: "HKDF-Expand", hash: v.hash, key: prk, info: hex(v.info)});
	const first = v.length >> 1;
	const a = new Uint8Array(expand.read(first)), b = new Uint8Array(v.length - first);
	assert.sameValue(expand.read(b), b.byteLength, "read(buffer) returns the count");
	assert.sameValue(toHex(a) + toHex(b), v.okm, "HKDF-Expand streamed in two reads");
	expand.close();
}

// output limit: 255 blocks
{
	const kdf = new KeyDerivation({algorithm: "HKDF", hash: "SHA-256", key: hex("0b".repeat(22)), info: new Uint8Array(0)});
	assert.sameValue(kdf.read(255 * 32).byteLength, 255 * 32, "255 blocks");
	assert.sameValue(kdf.read(1), undefined, "no 256th block");
	assert.sameValue(kdf.read(new Uint8Array(1)), undefined, "no 256th block into a buffer");
	kdf.close();
}
{	// a read that reaches the limit returns what remains
	const kdf = new KeyDerivation({algorithm: "HKDF", hash: "SHA-256", key: hex("0b".repeat(22)), info: new Uint8Array(0)});
	assert.sameValue(kdf.read(255 * 32 - 10).byteLength, 255 * 32 - 10, "all but 10 bytes");
	assert.sameValue(kdf.read(32).byteLength, 10, "the last 10 bytes");
	assert.sameValue(kdf.read(32), undefined, "then nothing");
	kdf.close();
}
{
	const kdf = new KeyDerivation({algorithm: "HKDF", hash: "SHA-256", key: hex("0b".repeat(22)), info: new Uint8Array(0)});
	kdf.read(255 * 32 - 3);
	const tail = new Uint8Array(8);
	assert.sameValue(kdf.read(tail), 3, "read(buffer) at the limit stores what remains");
	assert.sameValue(kdf.read(tail), undefined, "then nothing");
	kdf.close();
	assert.throws(SyntaxError, () => kdf.read(1), "read after close");
}
