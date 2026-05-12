/*---
description:
flags: [module]
---*/

import files from "./files_FIXTURE.js";

const dirPath = "nonempty";
files.delete(dirPath + "/file.bin");
files.delete(dirPath);
files.createDirectory(dirPath);
files.openFile({path: dirPath + "/file.bin", mode: "w+"}).close();

// delete a non-empty directory throws
assert.throws(Error, () => files.delete(dirPath));

// after removing the file, delete succeeds
assert(files.delete(dirPath + "/file.bin"));
assert(files.delete(dirPath));
assert(!files.status(dirPath).isDirectory());
