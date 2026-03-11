/*---
description: 
flags: [module]
---*/

import files from "./files_FIXTURE.js";

const pathFile = "testfile.dat";
const pathDir = "testdir";

files.delete(pathFile);
files.delete(pathDir);

assert(!files.status(pathFile).isFile());
assert(!files.status(pathDir).isDirectory());

files.createDirectory(pathDir);
files.openFile({path: pathFile, mode: "w+"}).close();

assert.sameValue(files.status(pathFile).isFile(), true);
assert.sameValue(files.status(pathFile).isDirectory(), false);
assert.sameValue(files.status(pathFile).size, 0);

assert.sameValue(files.status(pathDir).isFile(), false);
assert.sameValue(files.status(pathDir).isDirectory(), true);

files.delete(pathFile);
files.delete(pathDir);
