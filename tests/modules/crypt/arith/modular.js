/*---
description: Modular inverse agrees with a reference
flags: [module]
---*/

import Modular from "modular";

const p = (1n << 255n) - 19n;
function modinv(a, m) {
	let r0 = m, r1 = a % m, s0 = 0n, s1 = 1n;
	while (r1) {
		const q = r0 / r1;
		[r0, r1] = [r1, r0 - q * r1];
		[s0, s1] = [s1, s0 - q * s1];
	}
	return ((s0 % m) + m) % m;
}
const mod = new Modular(p);
for (const a of [2n, 3n, 123456789n, p - 1n, 0x4a5d9d5ba4ce2de1728e3bf480350f25e07e21c947d19e3376f09b3c1e161742n]) {
	const inverse = mod.mulinv(a);
	assert.sameValue(inverse, modinv(a, p), `inverse of ${a}`);
	assert.sameValue(mod.mul(a, inverse), 1n, `a * inverse of ${a}`);
}
