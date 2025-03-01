/*---
description: 
flags: [module]
---*/

import flash from "./flash-FIXTURE.js";
import {path} from "./flash-FIXTURE.js";

let f = flash.open({path});
f.close();
f.close();
assert.throws(SyntaxError, () => f.read(12, 0));
assert.throws(SyntaxError, () => f.write(new ArrayBuffer(12)));
assert.throws(SyntaxError, () => f.eraseBlock(0));
assert.throws(SyntaxError, () => f.status());
