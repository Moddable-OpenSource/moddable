/*---
description: PBKDF2 (RFC 6070, RFC 7914) and streamed reads
flags: [module]
---*/

import KeyDerivation, {PBKDF2, hex, toHex} from "./keyderivation_FIXTURE.js";

for (const v of PBKDF2) {
	if (v.iterations > 4096 && !globalThis.slowTests)
		continue;		// 80000 iterations: minutes on a microcontroller
	const kdf = new KeyDerivation({algorithm: "PBKDF2", hash: v.hash, key: hex(v.password), salt: hex(v.salt), iterations: v.iterations});
	assert.sameValue(kdf.algorithm, "PBKDF2");
	assert.sameValue(toHex(kdf.read(v.length)), v.dk, `${v.hash} c=${v.iterations} dkLen=${v.length}`);
	kdf.close();
}

// streamed: the output continues across reads and block boundaries
{
	const v = PBKDF2[3];
	const kdf = new KeyDerivation({algorithm: "PBKDF2", hash: v.hash, key: hex(v.password), salt: hex(v.salt), iterations: v.iterations});
	let out = "";
	for (let remaining = v.length; remaining > 0; remaining -= 7)
		out += toHex(kdf.read(Math.min(7, remaining)));
	assert.sameValue(out, v.dk, "7-byte reads");
	kdf.close();
}

// iterations is coerced and must be at least 1
assert.sameValue(toHex(new KeyDerivation({algorithm: "PBKDF2", hash: "SHA-1", key: hex(PBKDF2[0].password), salt: hex(PBKDF2[0].salt), iterations: "1"}).read(20)), PBKDF2[0].dk, "iterations from a string");
assert.throws(RangeError, () => new KeyDerivation({algorithm: "PBKDF2", hash: "SHA-1", key: new Uint8Array(8), salt: new Uint8Array(8), iterations: 0}), "iterations 0");
assert.throws(Error, () => new KeyDerivation({algorithm: "PBKDF2", hash: "SHA-1", key: new Uint8Array(8), salt: new Uint8Array(8)}), "iterations required");
