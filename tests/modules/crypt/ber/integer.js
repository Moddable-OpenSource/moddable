/*---
description: 
flags: [module]
---*/

import BER from "ber";

// https://luca.ntop.org/Teaching/Appunti/asn1.html - Section 5.7
assert.sameValue("2,1,0", new Uint8Array(BER.encode([0x02, 0n])).toString(), "0");
assert.sameValue("2,1,127", new Uint8Array(BER.encode([0x02, 127n])).toString(), "127");
assert.sameValue("2,2,0,128", new Uint8Array(BER.encode([0x02, 128n])).toString(), "128");
assert.sameValue("2,2,1,0", new Uint8Array(BER.encode([0x02, 256n])).toString(), "256");
assert.sameValue("2,1,128", new Uint8Array(BER.encode([0x02, -128n])).toString(), "-128");
assert.sameValue("2,2,255,127", new Uint8Array(BER.encode([0x02, -129n])).toString(), "-129");

berRoundTrip(0n);
berRoundTrip(127n);
berRoundTrip(128n);
berRoundTrip(256n);
berRoundTrip(-128n);
berRoundTrip(-129n);

function berRoundTrip(value) {
	let ber = BER.encode([0x02, value]);
	let v = BER.decode(ber);
	assert.sameValue(value, v[1]);
}
