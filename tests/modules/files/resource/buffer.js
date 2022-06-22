/*---
description: 
flags: [module]
---*/

import Resource from "Resource";

const r = new Resource("image-info.txt");
let bytes = new Uint8Array(r, 0, 5);
let str = ""
for (let i = 0; i < bytes.length; i++)
	str += String.fromCharCode(bytes[i]);
assert.sameValue(str, "Image");
assert.throws(TypeError, () => bytes[1] = 1, "immutable")

bytes = new Uint8Array(r.slice(1, 5));
str = ""
for (let i = 0; i < bytes.length; i++)
	str += String.fromCharCode(bytes[i]);
assert.sameValue(str, "mage");
bytes[1] = 1;
assert.sameValue(bytes[1], 1);

bytes = new Uint8Array(r.slice(2, 5, false));
str = ""
for (let i = 0; i < bytes.length; i++)
	str += String.fromCharCode(bytes[i]);
assert.sameValue(str, "age");
assert.throws(TypeError, () => bytes[1] = 1, "immutable")
