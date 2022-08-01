/*---
description: 
flags: [module]
---*/

import BER from "ber";

assert.sameValue("5,0", new Uint8Array(BER.encode([0x05])).toString(), "null");

berRoundTrip();
berRoundTrip();

function berRoundTrip() {
	let ber = BER.encode([0x05]);
	let v = BER.decode(ber);
	assert.sameValue(null, v[1]);
}
