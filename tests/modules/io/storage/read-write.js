/*---
description: 
flags: [module]
---*/

import storage from "./storage_FIXTURE.js";
import {emptyDomain, keys} from "./storage_FIXTURE.js";

const aValue = Uint8Array.of(0, 1, 2, 3);
const bValue = Uint8Array.of(0, 1, 2, 3, 4, 5, 6, 7, 8);

const path = "test";
let store = storage.open({path}); 

emptyDomain(store);

store.write("aValue", aValue);
store.write("bValue", bValue);
assert.sameValue(new Uint8Array(store.read("aValue")).toString(), aValue.toString(), "uint8array aValue");
assert.sameValue(new Uint8Array(store.read("bValue")).toString(), bValue.toString(), "uint8array bValue");
assert.sameValue(keys(store).toString(), ["aValue", "bValue"].toString(), "exepcted keys");

store.write("aValue", bValue.buffer);
store.write("bValue", aValue.buffer);
assert.sameValue(new Uint8Array(store.read("aValue")).toString(), bValue.toString(), "arraybuffer bValue");
assert.sameValue(new Uint8Array(store.read("bValue")).toString(), aValue.toString(), "arraybuffer aValue");
assert.sameValue(keys(store).toString(), ["aValue", "bValue"].toString(), "exepcted keys");

assert.sameValue(store.read("xyzzy"), undefined, "missing key");

assert.throws(SyntaxError, () => store.read(), "read with no argument");
assert.throws(TypeError, () => store.read(Symbol()), "read with symbol key");
assert.throws(SyntaxError, () => store.write(), "write with no argument");
assert.throws(SyntaxError, () => store.write("xyzzy"), "write with no key");
assert.throws(TypeError, () => store.write("xyzzy", "string"), "write with non-buffer value (string)");
assert.throws(TypeError, () => store.write("xyzzy", 12), "write with non-buffer value (number)");
assert.throws(ReferenceError, () => store.write("xyzzy", Uint16Array,of(1, 2)), "write with non-byte buffer value");
assert.throws(TypeError, () => store.write(Symbol(), new ArrayBuffer(12)), "write with symbol key");

emptyDomain(store);

store.close();
