/*---
description: 
flags: [module]
---*/

import TextEncoder from "text/encoder";

const encoder = new TextEncoder;

assert.throws(SyntaxError, () => encoder.encodeInto(), "two arguments required");
assert.throws(SyntaxError, () => encoder.encodeInto("A"), "two arguments required");

assert.throws(Error, () => encoder.encodeInto("A", {}), "Uint8Array destination required");
assert.throws(Error, () => encoder.encodeInto("A", new ArrayBuffer(20)), "Uint8Array destination required");
assert.throws(Error, () => encoder.encodeInto("A", []), "Uint8Array destination required");
assert.throws(Error, () => encoder.encodeInto("A", new SharedArrayBuffer(20)), "Uint8Array destination required");
assert.throws(Error, () => encoder.encodeInto("A", 20), "Uint8Array destination required");

let buffer = new Uint8Array(8);
check("ABCD", buffer, {read: 4, written: 4}, 65, 66, 67, 68, 0, 0, 0, 0);
check("a", buffer, {read: 1, written: 1}, 97, 66, 67, 68, 0, 0, 0, 0);
check("©!", buffer, {read: 2, written: 3}, 0xC2, 0xA9, 33, 68, 0, 0, 0, 0);
check("\0\0\0\0", buffer, {read: 4, written: 4}, 0, 0, 0, 0, 0, 0, 0, 0);
check(" \u1234 ", buffer, {read: 3, written: 5}, 32, 0xE1, 0x88, 0xB4, 32, 0, 0, 0);

encoder.encodeInto("123", new Uint8Array(buffer.buffer, 1));
check("", buffer, {read: 0, written: 0}, 32, 49, 50, 51, 32, 0, 0, 0);

check("\u1234\u1234\u1234", buffer, {read: 2, written: 6}, 0xE1, 0x88, 0xB4, 0xE1, 0x88, 0xB4, 0, 0);
check(" \u1234\u1234\u1234", buffer, {read: 3, written: 7}, 32, 0xE1, 0x88, 0xB4, 0xE1, 0x88, 0xB4, 0);
check("  \u1234\u1234\u1234", buffer, {read: 4, written: 8}, 32, 32, 0xE1, 0x88, 0xB4, 0xE1, 0x88, 0xB4);
check("0123456789", buffer, {read: 8, written: 8}, 48, 49, 50, 51, 52, 53, 54, 55);

function check(string, buffer, expected, ...bytes) {
	const result = encoder.encodeInto(string, buffer); 

	assert.sameValue(expected.read, result.read, "incorrect read");
	assert.sameValue(expected.written, result.written, "incorrect written");

	assert.sameValue(bytes.length, buffer.length, "different length buffers");
	
	for (let i = 0; i < bytes.length; i++)
		assert.sameValue(bytes[i], buffer[i], "mismatch at offset " + i)
}
