/*---
description: 
flags: [module]
---*/

import BER from "ber";

assert.sameValue("4,0", new Uint8Array(BER.encode([0x04, new ArrayBuffer])).toString(), "empty");
assert.sameValue("4,3,1,2,3", new Uint8Array(BER.encode([0x04, Uint8Array.of(1,2,3).buffer])).toString(), "1,2,3");
assert.sameValue("4,3,128,0,255", new Uint8Array(BER.encode([0x04, Uint8Array.of(128,0,255).buffer])).toString(), "128,0,255");

berRoundTrip(new ArrayBuffer);
berRoundTrip(Uint8Array.of(1,2,3).buffer);
berRoundTrip(Uint8Array.of(128,0,255).buffer);

function berRoundTrip(value) {
	let ber = BER.encode([0x04, value]);
	let v = BER.decode(ber);
	assert.sameValue(new Uint8Array(value).toString(), new Uint8Array(v[1]).toString());
}
