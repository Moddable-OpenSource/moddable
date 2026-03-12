/*---
description: 
flags: [module]
---*/

import files from "./files_FIXTURE.js";
import {deleteTree} from "./files_FIXTURE.js";

const dirPath = "test";
deleteTree(files, dirPath);
const target = "test.bin";
const link = "link.bin";

files.delete(target);
files.delete(link);

let f = files.openFile({path: target, mode: "w"});
f.write(new ArrayBuffer(123), 0);
f.close();

let supportsLink = true;
try {
	files.createLink(link, target);
}
catch (e) {
	supportsLink = false;
	trace(`createLink failed - assuming unimplemented: ${e}\n`);
}

if (supportsLink) {
	f = files.openFile({path: link, mode: "r"});
	let stat = f.status();
	assert(stat.isFile(), "expected file");
	assert(123 === stat.size, "expected file length of 123");
	f.close();

	stat = files.status(link);
	assert(stat.isFile(), "expected file");
	assert(123 === stat.size, "expected file length of 123");

	stat = files.status(link, {resolveTarget: false});
	assert(stat.isSymbolicLink(), "expected link");
	assert(!stat.isFile(), "expected link");
	assert(!stat.isDirectory(), "expected link");
}
