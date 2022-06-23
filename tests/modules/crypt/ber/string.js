/*---
description: 
flags: [module]
---*/

import BER from "ber";

assert.sameValue("12,0", new Uint8Array(BER.encode([0x0c, ""])).toString(), "");
assert.sameValue("12,1,97", new Uint8Array(BER.encode([0x0c, "a"])).toString(), "");
assert.sameValue("12,2,97,98", new Uint8Array(BER.encode([0x0c, "ab"])).toString(), "");
assert.sameValue("12,3,195,161,98", new Uint8Array(BER.encode([0x0c, "áb"])).toString(), "");

berRoundTrip("");
berRoundTrip("a");
berRoundTrip("ab");
berRoundTrip("áb");

function berRoundTrip(value) {
	let ber = BER.encode([0x0c, value]);
	let v = BER.decode(ber);
	assert.sameValue(value, v[1]);
}
