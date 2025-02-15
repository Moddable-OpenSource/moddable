/*---
description: 
flags: [module]
---*/

import storage from "./storage_FIXTURE.js";
import {emptyDomain} from "./storage_FIXTURE.js";

const path = "test";
let store = storage.open({path}); 
emptyDomain(store);
store.write("aValue", Uint8Array.of(1));
store.write("bValue", Uint8Array.of(2));
store.write("bValue", Uint8Array.of(3));
store.write("cValue", Uint8Array.of(4));

let found = false;
let s = new Set;
for (let key of store) {
	assert.sameValue(typeof key, "string", "key should be string")
	assert(key !== "", "key cannot be empty string");
	assert (!s.has(key), "duplicate key");
	s.add(key);
}
assert.sameValue(s.size, 3, "expect 3 keys");

for (let key of store)
	break;

store.close();
