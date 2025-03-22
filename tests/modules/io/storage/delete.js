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

store.delete("aValue");
assert.sameValue(store.read("aValue"), undefined);
assert.sameValue(keys(store).includes("aValue"), false, "don't expect aValue");
assert.sameValue(keys(store).includes("bValue"), true, "expect bValue");

store.delete("bValue");
assert.sameValue(store.read("bValue"), undefined);
assert.sameValue(keys(store).length, 0, "expect noKeys");

store.delete("aValue");
store.delete("bValue");

store.write(1, aValue);
assert.sameValue(keys(store).includes("1"), true, "expect key '1'");

assert.throws(SyntaxError, () => store.delete(), "delete with no argument");
assert.throws(TypeError, () => store.delete(Symbol()), "delete with symbol key");

emptyDomain(store);

store.close();
