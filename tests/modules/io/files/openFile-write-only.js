/*---
description:
flags: [module]
---*/

import files from "./files_FIXTURE.js";

const path = "writeonly.bin";
files.delete(path);

// mode "w" — write succeeds, read throws
let f = files.openFile({path, mode: "w"});
f.write(new ArrayBuffer(64), 0);
assert.throws(Error, () => f.read(32, 0));
f.close();

files.delete(path);
