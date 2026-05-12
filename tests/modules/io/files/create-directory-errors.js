/*---
description:
flags: [module]
---*/

import files from "./files_FIXTURE.js";

// already exists returns false
files.delete("existingdir");
files.createDirectory("existingdir");
assert.sameValue(files.createDirectory("existingdir"), false);
files.delete("existingdir");

// file exists at the path throws
files.delete("existingfile");
files.openFile({path: "existingfile", mode: "w+"}).close();
assert.throws(Error, () => files.createDirectory("existingfile"));
files.delete("existingfile");

// invalid paths throw
assert.throws(Error, () => files.createDirectory("../badpath"));
assert.throws(Error, () => files.createDirectory("./badpath"));
