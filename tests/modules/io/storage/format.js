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

assert.sameValue(store.format, "buffer", "default format is buffer");

store.format = "buffer";
assert.sameValue(store.format, "buffer", "reset to buffer");

assert.throws(RangeError, () => store.format = "xyzzy", "invalid format");
assert.sameValue(store.format, "buffer", "should remain bufer");

store.close();
