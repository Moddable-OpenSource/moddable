/*---
description: 
flags: [module]
---*/

import AudioIn from "embedded:io/audio/in";

let input = new AudioIn({});
assert.sameValue(input.target, undefined, "no target");
input.close();

const target = {};
input = new AudioIn({target});
assert.sameValue(input.target, target, "target mismatch");
delete input.target;
assert.sameValue(input.target, undefined, "didn't delete target");
input.close();
