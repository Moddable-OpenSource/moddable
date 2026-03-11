/*---
description: 
flags: [module]
---*/

import files from "./files_FIXTURE.js";

files.scan();
assert.throws(Error, () => files.scan(".."));

assert.throws(SyntaxError, () => files.scan.call(new $TESTMC.HostObject, ""));

let s = files.scan();

assert.throws(SyntaxError, () => s.next.call(new $TESTMC.HostObject));
assert.throws(SyntaxError, () => s.return.call(new $TESTMC.HostObject));
