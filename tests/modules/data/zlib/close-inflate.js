/*---
description: 
flags: [module]
---*/

import Inflate from "inflate";

let inf = new Inflate();

assert.throws(SyntaxError, () => inf.close.call(new $TESTMC.HostObjectChunk), "close invalid this - HostObjectChunk");
assert.throws(SyntaxError, () => inf.close.call(new $TESTMC.HostObject), "close invalid this - HostObject");
assert.throws(SyntaxError, () => inf.close.call(new $TESTMC.HostBuffer), "close invalid this - HostBuffer");

inf.close();
inf.close();
