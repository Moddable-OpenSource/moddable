/*---
description: 
flags: [module]
---*/

import storage from "./storage_FIXTURE.js";

const path = "test";
let store = storage.open({path});
store.close();

assert.throws(SyntaxError, () => store.write("key", new ArrayBuffer(12)), "write");
assert.throws(SyntaxError, () => store.read("key"), "read");
assert.throws(SyntaxError, () => store.delete("key"), "delete");
assert.throws(SyntaxError, () => store.format = "buffer", "buffer");
assert.throws(SyntaxError, () => Array.from(store), "iterator");
