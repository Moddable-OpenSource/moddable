/*---
description:
flags: [module]
---*/

import {V, hex, toHex, oneShot, optionsFor} from "./cipher_FIXTURE.js";

for (const vec of V.cbc) {
	assert.sameValue(toHex(oneShot(optionsFor("AES-CBC", vec, "encrypt"), hex(vec.pt))), vec.ct, `CBC-${vec.key.length * 4} encrypt`);
	assert.sameValue(toHex(oneShot(optionsFor("AES-CBC", vec, "decrypt"), hex(vec.ct))), vec.pt, `CBC-${vec.key.length * 4} decrypt`);
}
for (const vec of V.cbc_pkcs7) {
	assert.sameValue(toHex(oneShot(optionsFor("AES-CBC", vec, "encrypt", {padding: "pkcs7"}), hex(vec.pt))), vec.ct, `CBC pkcs7 encrypt ${vec.pt.length / 2}B`);
	assert.sameValue(toHex(oneShot(optionsFor("AES-CBC", vec, "decrypt", {padding: "pkcs7"}), hex(vec.ct))), vec.pt, `CBC pkcs7 decrypt ${vec.pt.length / 2}B`);
}
// empty plaintext without padding is a valid (empty) operation
assert.sameValue(toHex(oneShot(optionsFor("AES-CBC", V.cbc[0], "encrypt"), new Uint8Array(0))), "", "CBC empty");
