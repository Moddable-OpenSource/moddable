/*---
description: 
flags: [module]
---*/

import storage from "./storage_FIXTURE.js";
import {emptyDomain, keys} from "./storage_FIXTURE.js";

const path = "test";
let store = storage.open({path}); 

emptyDomain(store);
store.write("a", ArrayBuffer.fromString("a"));
store.write("b", ArrayBuffer.fromString("b"));
store.close();

store = storage.open({path, mode: "r"}); 

assert.sameValue(String.fromArrayBuffer(store.read("a")), "a");
assert.sameValue(String.fromArrayBuffer(store.read("b")), "b");

assert.throws(Error, () => store.write("a", ArrayBuffer.fromString("A")));
assert.sameValue(String.fromArrayBuffer(store.read("a")), "a");

assert.throws(Error, () => store.delete("b"));
assert.sameValue(String.fromArrayBuffer(store.read("b")), "b");

assert.sameValue(keys(store).toString(), 'a,b');

store.close();
