/*---
description: 
flags: [module]
---*/

import files from "./files_FIXTURE.js";
import {deleteTree} from "./files_FIXTURE.js";

const path = "test";
deleteTree(files, path);
let f = files.openFile({path, mode: "w+"});
f.write(new ArrayBuffer(123), 0);
f.close();
f.close();
assert.throws(SyntaxError, () => f.status());
assert.throws(SyntaxError, () => f.read(12, 0));
assert.throws(SyntaxError, () => f.write(new ArrayBuffer(12)));
assert.throws(SyntaxError, () => f.setSize(5));
assert.throws(SyntaxError, () => f.flush());
