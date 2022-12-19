/*---
description: 
flags: [module]
---*/

import BER from "ber";

assert.sameValue("1,1,0", new Uint8Array(BER.encode([0x01, false])).toString(), "false");
assert.sameValue("1,1,1", new Uint8Array(BER.encode([0x01, true])).toString(), "true");

berRoundTrip(false);
berRoundTrip(true);

function berRoundTrip(value) {
	let ber = BER.encode([0x01, value]);
	let v = BER.decode(ber);
	assert.sameValue(value, v[1]);
}
