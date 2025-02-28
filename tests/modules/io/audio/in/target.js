/*---
description: 
flags: [module]
---*/

import AudioOut from "embedded:io/audioout";

let out = new AudioOut({});
assert.sameValue(out.target, undefined, "no target");
out.close();

const target = {};
out = new AudioOut({target});
assert.sameValue(out.target, target, "target mismatch");
delete out.target;
assert.sameValue(out.target, undefined, "didn't delete target");
out.close();
