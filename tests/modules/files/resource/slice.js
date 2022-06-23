/*---
description: 
flags: [module]
---*/

import Resource from "Resource";

const r = new Resource("image-info.txt");
assert.sameValue(r.byteLength, 368, "bad resource size");

let arrayBuffer = r.slice(0, r.byteLength >> 1, true);
assert.sameValue(arrayBuffer.byteLength, r.byteLength >> 1, "bad buffer size");
assert(arrayBuffer instanceof ArrayBuffer, "not arraybuffer");

arrayBuffer = r.slice(0, r.byteLength, false);
assert.sameValue(arrayBuffer.byteLength, 368, "bad buffer size");
assert(!(arrayBuffer instanceof ArrayBuffer), "not hostbuffer");

arrayBuffer = r.slice();
assert.sameValue(arrayBuffer.byteLength, 368, "bad buffer size");
assert(arrayBuffer instanceof ArrayBuffer, "not arraybuffer");

assert.sameValue(String.fromArrayBuffer(r.slice(r.byteLength - 8)), "Moddable");
assert.sameValue(String.fromArrayBuffer(r.slice(r.byteLength - 8, r.byteLength - 5)), "Mod");

assert.sameValue(0, r.slice(r.byteLength).byteLength, "start at end")
assert.sameValue(r.byteLength, r.slice(-1).byteLength, "negative start")
assert.sameValue(0, r.slice(2,1).byteLength, "end before start")
assert.sameValue(0, r.slice(2, -1).byteLength, "negative end")

assert.sameValue(r.slice(2, r.byteLength + 3).byteLength, r.byteLength - 2, "end should be pinned to byteLength");

assert.throws(SyntaxError, () => r.slice.call(new $TESTMC.HostObject));
