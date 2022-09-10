/*---
description:
flags: [module]
---*/

import structuredClone from "structuredClone";

const ta0 = new Uint8Array([0,1,2]);
const ta1 = structuredClone(ta0);
ta0[0] = 255;
const ta2 = structuredClone(ta0, { transfer:[ ta0.buffer ] });
assert.throws(TypeError, ()  => { ta0.toString() }, "detached");
assert.sameValue(ta1.toString(), "0,1,2", "copy");
assert.sameValue(ta2.toString(), "255,1,2", "original");
