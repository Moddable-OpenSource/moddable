/*---
description: 
flags: [module]
---*/

import Resource from "Resource";

const r = new Resource("image-info.txt");
assert.sameValue(Resource.exists("image-info.txt"), true, "not found");
assert.sameValue(Resource.exists("NOT FOUND"), false, "found");
assert.throws(SyntaxError, () => Resource.exists(), "no arguments")
