/*---
flags: [onlyStrict]
---*/

let bytes = new Uint8Array(new ArrayBuffer(3));
bytes[0] = '1'.charCodeAt();
bytes[1] = '2'.charCodeAt();
bytes[2] = '!'.charCodeAt();
assert.sameValue("12!", String.fromArrayBuffer(bytes.buffer), "simple ArrayBuffer");

bytes = new Uint8Array(new SharedArrayBuffer(3));
bytes[0] = 'a'.charCodeAt();
bytes[1] = 'b'.charCodeAt();
bytes[2] = '-'.charCodeAt();
assert.sameValue("ab-", String.fromArrayBuffer(bytes.buffer), "simple SharedArrayBuffer");

bytes = new Uint8Array(new $TESTMC.HostBuffer(3));
bytes[0] = '2'.charCodeAt();
bytes[1] = '3'.charCodeAt();
bytes[2] = '@'.charCodeAt();
assert.sameValue("23@", String.fromArrayBuffer(bytes.buffer), "simple HostBuffer");

assert.throws(TypeError, () => String.fromArrayBuffer(new $TESTMC.HostObject), "hostobject");
assert.throws(TypeError, () => String.fromArrayBuffer(new $TESTMC.HostObjectChunk), "HostObjectChunk");
assert.throws(TypeError, () => String.fromArrayBuffer({}), "object");
assert.throws(TypeError, () => String.fromArrayBuffer(1), "number");
assert.throws(TypeError, () => String.fromArrayBuffer(1n), "bigint");
assert.throws(TypeError, () => String.fromArrayBuffer("string"), "string");
assert.throws(TypeError, () => String.fromArrayBuffer(), "no argument");
assert.throws(TypeError, () => String.fromArrayBuffer(String.fromArrayBuffer), "host function");
assert.throws(TypeError, () => String.fromArrayBuffer(new Uint8Array(12)), "Uint8Array");
assert.sameValue("", String.fromArrayBuffer(new $TESTMC.HostBuffer(16)), "empty HostBuffer");
