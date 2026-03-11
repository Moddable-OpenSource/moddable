/*---
description: 
flags: [module]
---*/

import files from "./files_FIXTURE.js";

const path = "testfile.dat";

files.delete(path);
assert(!files.status(path).isFile());

let f = files.openFile({path, mode: "w+"});
assert.throws(SyntaxError, () => f.setSize());
f.setSize(99);
assert.sameValue(f.status().size, 99);
f.setSize(5001);
assert.sameValue(f.status().size, 5001);
f.setSize(1);
assert.sameValue(f.status().size, 1);
f.setSize(0);
assert.sameValue(f.status().size, 0);
f.close();
assert.sameValue(files.status(path).size, 0);

assert.throws(SyntaxError, () => f.setSize(1));


f = files.openFile({path, mode: "r"});
assert.throws(Error, () => f.setSize(12));
f.close();



