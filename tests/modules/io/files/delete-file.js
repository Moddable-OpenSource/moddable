/*---
description: 
flags: [module]
---*/

import files from "./files_FIXTURE.js";

const path = "test.bin";
files.delete(path);
const f = files.openFile({
	path,
	mode: "w+"
});
f.write(new ArrayBuffer(512), 0);
f.close();

assert(files.status(path).isFile());

assert(files.delete(path), "returns true if file exists when deleting");
assert(!files.status(path).isFile(), "file not deleted");
assert(!files.delete(path), "returns false if file does not exist when deleting");
