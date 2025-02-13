/*---
description: 
flags: [module]
---*/

import files from "./files_FIXTURE.js";

const path = "testfile.dat";

files.delete(path);

let f = files.openFile({path, mode: "w+"});

let status = f.status();
assert.sameValue(status.size, 0);
assert(status.isFile());
assert(!status.isDirectory());
assert(!status.isSymbolicLink());

f.write(new ArrayBuffer(100), 200);
assert.sameValue(f.status().size, 300);
f.setSize(25);
assert.sameValue(f.status().size, 25);

assert.throws(SyntaxError, () => f.status.call(new $TESTMC.HostObject));
f.close();
