/*---
description: HMAC known answers over every hash, incremental and one-shot, read into a buffer, verify
flags: [module]
---*/

import MAC, {V, hex, toHex} from "./mac_FIXTURE.js";

for (const v of V) {
	const mac = new MAC({algorithm: "HMAC", hash: v.hash, key: hex(v.key)});
	const message = hex(v.message);
	mac.write(message);
	assert.sameValue(toHex(mac.read()), v.tag, `${v.hash} key ${v.key.length / 2}B message ${message.byteLength}B`);
	assert.sameValue(mac.verify(hex(v.tag)), true, "verify");
	const bad = hex(v.tag);
	bad[bad.byteLength - 1] ^= 1;
	assert.sameValue(mac.verify(bad), false, "verify tampered");
	assert.sameValue(mac.verify(bad.subarray(1)), false, "verify wrong length");
	mac.close();

	// in pieces, into a caller's buffer
	const piecewise = new MAC({algorithm: "HMAC", hash: v.hash, key: hex(v.key)});
	const half = message.byteLength >> 1;
	piecewise.write(message.subarray(0, half));
	piecewise.write(message.subarray(half));
	const out = new Uint8Array(64);
	assert.sameValue(piecewise.read(out), piecewise.outputSize, "read(buffer) returns the tag length");
	assert.sameValue(toHex(out.subarray(0, piecewise.outputSize)), v.tag, "read(buffer)");
	piecewise.close();
}
