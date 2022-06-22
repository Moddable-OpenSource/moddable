/*---
description: 
flags: [module]
---*/

import Flash from "flash";

const f = new Flash($TESTMC.config.flashParition);
const {byteLength, blockSize} = f;

assert.throws(SyntaxError, () => f.write(), "write requires 3 arguments");
assert.throws(SyntaxError, () => f.write(0, 64), "write requires 3 arguments");
assert.throws(TypeError, () => f.write(0, 64, 128), "write buffer for third argument, number");
assert.throws(TypeError, () => f.write(0, 64, "123"), "write buffer for third argument, string");
assert.throws(TypeError, () => f.write(0, 64, Symbol()), "write buffer for third argument, symbol");
assert.throws(TypeError, () => f.write(0, 64, {}), "write buffer for third argument, object");
assert.throws(TypeError, () => f.write(0, 64, []), "write buffer for third argument, array");
assert.throws(Error, () => f.write(0, 64, new $TESTMC.HostObject), "write buffer for third argument, host object");

f.write(0, 64, new Uint8Array(64));
f.write(0, 64, new DataView(new ArrayBuffer(64)));
f.write(0, 64, new SharedArrayBuffer(64));
f.write(0, 64, new $TESTMC.HostBuffer(64));

assert.throws(Error, () => f.write(0, 65, new ArrayBuffer(64)), "write length exceeds buffer");
assert.throws(Error, () => f.write(0, 25, new Uint8Array(new ArrayBuffer(64), 32, 24)), "write length exceeds view");
f.write(0, 24, new Uint8Array(new ArrayBuffer(64), 32, 24));

assert.throws(Error, () => f.write(byteLength, 64, new ArrayBuffer(64)), "write at end");
assert.throws(Error, () => f.write(byteLength - 32, 64, new ArrayBuffer(64)), "write straddles end");
assert.throws(Error, () => f.write(-1, 64, new ArrayBuffer(64)), "write at negative offset");
assert.throws(Error, () => f.write(0, -1, new ArrayBuffer(64)), "write negative length");

assert.throws(SyntaxError, () => f.write.call(new $TESTMC.HostObject, 0, 64, new ArrayBuffer(64)), "write with non-flash this");


