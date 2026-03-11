/*---
description: 
flags: [module]
---*/

import files from "./files_FIXTURE.js";

const path = "testdir";
files.delete(path);
files.createDirectory(path);

assert(files.status(path).isDirectory());

assert(files.delete(path), "returns true if directory exists when deleting");
assert(!files.status(path).isDirectory(), "directory not deleted");
assert(!files.delete(path), "returns false if directory does not exist when deleting");
