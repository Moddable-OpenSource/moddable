/*---
description: 
flags: [module]
---*/

import files from "./files_FIXTURE.js";

const path = "testfile.dat";

files.delete(path);
assert.throws(Error, () => files.status(path));

let f = files.openFile({path, mode: "w+"});

f.write(new Uint8Array(256), 0);
f.flush();			// no good way to validate that this did something!
f.close();
