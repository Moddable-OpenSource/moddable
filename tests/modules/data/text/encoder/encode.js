/*---
description: 
flags: [module]
---*/

import TextEncoder from "text/encoder";

const encoder = new TextEncoder;

assert.throws(SyntaxError, () => encoder.encode(), "one argument required");
assert(encoder.encode("A") instanceof Uint8Array, "result is Uint8Array");

equal(encoder.encode("A"), 65);
equal(encoder.encode("AB"), 65, 66);

equal(encoder.encode("A\0B"), 65, 0, 66);
equal(encoder.encode("©!"), 0xC2, 0xA9, 33);
equal(encoder.encode(" \u1234 "), 32, 0xE1, 0x88, 0xB4, 32);
equal(encoder.encode(" \uD800\uDC00!"), 32, 0xf0, 0x90, 0x80, 0x80, 33);
equal(encoder.encode(" \uDC00\uD800!"), 32, 0xED, 0xB0, 0x80, 0xED, 0xA0, 0x80, 33);

equal(encoder.encode(1), 49);
equal(encoder.encode(123n), 49, 50, 51);

function equal(buffer, ...bytes) {
	assert.sameValue(bytes.length, buffer.length, "different length buffers");
	
	for (let i = 0; i < bytes.length; i++)
		assert.sameValue(bytes[i], buffer[i], "mismatch at offset " + i)
}
