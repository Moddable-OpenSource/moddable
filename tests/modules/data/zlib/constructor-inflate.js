/*---
description: 
flags: [module]
---*/

import Inflate from "inflate";

assert.throws(TypeError, () => Inflate(), "Inflate must be called as constructor");

let inf = new Inflate();
inf.close();

assert.throws(RangeError, () => new Inflate({windowBits: -1}), "windowBits -1");
assert.throws(RangeError, () => new Inflate({windowBits: 100}), "windowBits 100");
assert.throws(TypeError, () => new Inflate({windowBits: Symbol()}), "windowBits Symbol()");

