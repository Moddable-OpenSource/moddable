/*---
description: 
flags: [module]
---*/

import storage from "./storage_FIXTURE.js";

const path = "test";
let store = storage.open({path}); 
store.close();

store = storage.open({path}); 
let b = storage.open({path});
store.close();
b.close();

assert.throws(TypeError, () => storage.open(), "no options object");
assert.throws(Error, () => storage.open({}), "no path");

store = storage.open({path, mode: "r"}); 
assert.sameValue(store.format, "buffer", "default format is buffer");
store.close();

store = storage.open({path, mode: "r+"}); 
store.close();

assert.throws(Error, () => storage.open({mode: "r"}), "mode but no path");
assert.throws(Error, () => storage.open({path, mode: "xyzzy"}), "invalid mode");

assert.throws(Error, () => storage.open({format: "buffer"}), "format but no path");

store = storage.open({path, format: "buffer", mode: "r"}); 
store.close();

store = storage.open({path, format: "string"}); 
store.close();

assert.throws(RangeError, () => storage.open({path, format: "xyzzy"}), "invalid format");
