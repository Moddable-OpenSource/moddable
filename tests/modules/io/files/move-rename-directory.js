/*---
description: 
flags: [module]
---*/

import files from "./files_FIXTURE.js";

const pathFrom = "testdir";
const pathTo = "dirtest";

files.delete(pathFrom);
files.delete(pathTo);

files.createDirectory(pathFrom);
assert(files.status(pathFrom).isDirectory());
files.move(pathFrom, pathTo);

assert(files.status(pathTo).isDirectory());
assert(!files.status(pathFrom).isDirectory());

files.delete(pathTo);
