/*---
description:
flags: [module]
---*/

import files from "./files_FIXTURE.js";
import {deleteTree} from "./files_FIXTURE.js";

const dirPath = "scan-return-test";
deleteTree(files, dirPath);
files.createDirectory(dirPath);
["a.bin", "b.bin", "c.bin"].forEach(name =>
	files.openFile({path: dirPath + "/" + name, mode: "w+"}).close()
);

// for...of with break triggers iterator return()
let count = 0;
for (const name of files.scan(dirPath)) {
	count++;
	break;
}
assert.sameValue(count, 1);

// explicit return() mid-iteration
const s = files.scan(dirPath);
s.next();
const ret = s.return();
assert.sameValue(ret.done, true);
assert.sameValue(s.next().done, true);

// a fresh scan of the same directory works after early termination
const entries = [];
for (const name of files.scan(dirPath))
	entries.push(name);
assert.sameValue(entries.length, 3);

deleteTree(files, dirPath);
