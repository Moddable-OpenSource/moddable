/*---
description:
flags: [module]
---*/

import {V, hex, toHex, oneShot, optionsFor} from "./cipher_FIXTURE.js";

for (const vec of V.ecb) {
	assert.sameValue(toHex(oneShot(optionsFor("AES-ECB", vec, "encrypt"), hex(vec.pt))), vec.ct, `ECB-${vec.key.length * 4} encrypt`);
	assert.sameValue(toHex(oneShot(optionsFor("AES-ECB", vec, "decrypt"), hex(vec.ct))), vec.pt, `ECB-${vec.key.length * 4} decrypt`);
}
for (const vec of V.ecb_pkcs7) {
	assert.sameValue(toHex(oneShot(optionsFor("AES-ECB", vec, "encrypt", {padding: "pkcs7"}), hex(vec.pt))), vec.ct, `ECB pkcs7 encrypt ${vec.pt.length / 2}B`);
	assert.sameValue(toHex(oneShot(optionsFor("AES-ECB", vec, "decrypt", {padding: "pkcs7"}), hex(vec.ct))), vec.pt, `ECB pkcs7 decrypt ${vec.pt.length / 2}B`);
}
