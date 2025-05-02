/*---
description: 
flags: [module]
---*/

import flash from "./flash-FIXTURE.js";
import {path} from "./flash-FIXTURE.js";

let f = flash.open({path});
f.close();

f = flash.open({path, mode: "r"});
f.close();

f = flash.open({path, mode: "r+"});
f.close();

assert.throws(Error, () => flash.open({path, mode: "rw"}), "unsupport mode");
assert.throws(Error, () => flash.open({path, mode: "xyzzy"}), "invalid mode");

f = flash.open({path, format: "buffer"});
f.close();
assert.throws(RangeError, () => flash.open({path, format: "xyzzy"}), "invalid format");

assert.throws(Error, () => flash.open({}), "no path");
assert.throws(Error, () => flash.open({path: "xyzzy"}), "invalid path");
assert.throws(TypeError, () => flash.open()), "no options object";

assert.throws(TypeError, () => flash.open({path: Symbol()}), "symbol path");
assert.throws(TypeError, () => flash.open({path, mode: Symbol()}), "symbol mode");
