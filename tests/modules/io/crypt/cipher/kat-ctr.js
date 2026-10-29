/*---
description:
flags: [module]
---*/

import {V, hex, toHex, oneShot, optionsFor} from "./cipher_FIXTURE.js";

for (const vec of V.ctr) {
	assert.sameValue(toHex(oneShot(optionsFor("AES-CTR", vec, "encrypt"), hex(vec.pt))), vec.ct, `CTR-${vec.key.length * 4} encrypt`);
	assert.sameValue(toHex(oneShot(optionsFor("AES-CTR", vec, "decrypt"), hex(vec.ct))), vec.pt, `CTR-${vec.key.length * 4} decrypt`);
}
// counter block of all 0xff wraps through the whole 128-bit block
assert.sameValue(toHex(oneShot(optionsFor("AES-CTR", V.ctr_wrap, "encrypt"), hex(V.ctr_wrap.pt))), V.ctr_wrap.ct, "CTR full-block wrap");
