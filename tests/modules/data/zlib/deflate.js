/*---
description: 
flags: [module]
---*/

import Deflate from "deflate";

let def;
try {
	def = new Deflate({});
}
catch {
}
finally {
	assert(def !== undefined, "not enough memory to test Deflate");
	def.close();
	def = undefined;
}

const compressed1 = Uint8Array.of(
	0x78, 0x01, 0x63, 0x60, 0x64, 0x62, 0x66, 0x61,
	0x65, 0x63, 0x67, 0x80, 0xd2, 0x00, 0x01, 0x98,
	0x00, 0x39);

const compressed2 = Uint8Array.of(
	0x78, 0x01, 0xed, 0xc5, 0x39, 0x0d, 0x00, 0x40, 0x08, 0x00, 0x30, 0xee, 0x01, 0xfc, 0x3b, 0x46,
	0x07, 0x49, 0xa7, 0xc6, 0xb9, 0xef, 0x67, 0x75, 0x90, 0x24, 0x49, 0x92, 0x24, 0x49, 0x92, 0x24,
	0x49, 0x92, 0x24, 0xc9, 0xb5, 0x0e, 0x19, 0x00, 0x70, 0x01); 

let deflator = new Deflate;
deflator.push(Uint8Array.of(0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7), true);

assert.sameValue(compressed1.length, deflator.result.length, "deflated data1 bad length");
for (let i = 0; i < compressed1.length; i++)
	assert.sameValue(compressed1[i], deflator.result[i], "deflated data1 mismatch @ " + i);
deflator.close();


deflator = new Deflate;
let bytes = new Uint8Array(8192);
for (let i = 0; i < 8192; i++)
	bytes[i] = i & 7;
deflator.push(bytes, true);

assert.sameValue(compressed2.length, deflator.result.length, "deflated data2 bad length");
for (let i = 0; i < compressed2.length; i++)
	assert.sameValue(compressed2[i], deflator.result[i], "deflated data2 mismatch @ " + i);
deflator.close();
