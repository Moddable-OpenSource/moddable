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

assert.throws(Error, () => files.status());

files.createDirectory(pathDir);
files.openFile({path: pathFile, mode: "w+"}).close();

assert.throws(SyntaxError, () => files.status.call(new $TESTMC.HostObject, pathFile));
assert.throws(SyntaxError, () => files.status.call(new $TESTMC.HostObject, pathDir));
assert.throws(Error, () => files.status("./" + pathFile));
assert.throws(Error, () => files.status("./" + pathDir));
assert.throws(Error, () => files.status("../"));

files.delete(pathFile);
files.delete(pathDir);
