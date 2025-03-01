/*---
description: 
flags: [module]
---*/

import flash from "./flash-FIXTURE.js";

let found = false;
let s = new Set;
for (let path of flash) {
	assert.sameValue(typeof path, "string", "path should be string")
	assert(path !== "", "path cannot be empty string");
	assert (!s.has(path), "duplicate path");
	s.add(path);
	found ||= (path === "xs_test");
}

assert(found, "expected xs_test partition");

for (let path of flash)
	break;
