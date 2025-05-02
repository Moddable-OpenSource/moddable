/*---
description: 
flags: [module]
---*/

import files from "./files_FIXTURE.js";

const pathFrom = "testdir";
const pathTo = "dirtest";

assert.throws(Error, () => files.move(pathFrom, pathTo));
assert.throws(SyntaxError, () => files.move(pathFrom));
assert.throws(SyntaxError, () => files.move());

files.delete(pathFrom);
files.delete(pathTo);

files.createDirectory(pathFrom);
assert.throws(SyntaxError, () => files.move.call(new $TESTMC.HostObject, pathFrom, pathTo));
assert.throws(Error, () => files.move(pathFrom, "../" + pathTo));
assert.throws(Error, () => files.move("../" + pathFrom, pathTo));
files.move(pathFrom, pathTo);
assert(files.delete(pathTo));
