/*---
description: 
flags: [module]
---*/

import files from "./files_FIXTURE.js";
import {deleteTree} from "./files_FIXTURE.js";

const dirPath = "test";
const path = "test.bin";

files.delete(path);
assert(!files.status(path).isFile(), "file not deleted");

assert.throws(Error, () => files.openFile({path}));
assert(!files.delete(path));
assert.throws(Error, () => files.openFile({path, mode: "r"}));
assert(!files.delete(path));
assert.throws(Error, () => files.openFile({path, mode: "r+"}));
files.openFile({path, mode: "w"}).close();
assert(files.delete(path));

files.openFile({path, mode: "w+"}).close();

let f = files.openFile({path, mode: "r"});
assert.throws(Error, () => f.write(new ArrayBuffer(100), 0));
f.close();

f = files.openFile({path, mode: "r+"});
f.write(new ArrayBuffer(100), 0);
f.read(25, 75);
f.close();

f = files.openFile({path, mode: "r"});
assert.throws(Error, () => f.write(new ArrayBuffer(100), 0));
f.read(25, 75);
f.close();

assert.throws(TypeError, () => f.openFile(".."));
assert.throws(TypeError, () => f.openFile());
assert.throws(TypeError, () => f.openFile.call(new $TESTMC.HostObject, {path}));

deleteTree(files, dirPath);
files.createDirectory(dirPath);
assert.throws(Error, () => files.openFile({path: dirPath}));
