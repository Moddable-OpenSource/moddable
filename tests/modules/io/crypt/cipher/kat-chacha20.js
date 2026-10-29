/*---
description:
flags: [module]
---*/

import {V, hex, toHex, optionsFor, tryConstruct} from "./cipher_FIXTURE.js";

for (const vec of V.chacha20) {
	for (const [direction, input, expected] of [["encrypt", vec.pt, vec.ct], ["decrypt", vec.ct, vec.pt]]) {
		using c = tryConstruct(optionsFor("ChaCha20", vec, direction));
		if (!c) continue;
		assert.sameValue(toHex(c[direction](hex(input))), expected, `ChaCha20 ${direction}`);
	}
}
