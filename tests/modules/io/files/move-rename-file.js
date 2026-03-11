/*---
description: 
flags: [module]
---*/

import files from "./files_FIXTURE.js";

const pathFrom = "testfile.bin";
const pathTo = "filetest.bin";

files.delete(pathFrom);
files.delete(pathTo);

const f = files.openFile({path: pathFrom, mode: "w+"});
f.write(new ArrayBuffer(400), 0);
f.close();
assert(files.status(pathFrom).isFile());
files.move(pathFrom, pathTo);

assert(files.status(pathTo).isFile());
assert(!files.status(pathFrom).isFile());

files.delete(pathTo);
