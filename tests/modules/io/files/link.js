/*---
description: 
flags: [module]
---*/

import files from "./files_FIXTURE.js";
import {deleteTree} from "./files_FIXTURE.js";

const supportsLink = files.createLink ? true : false;

const dirPath = "test";
deleteTree(files, dirPath);
const target = "test.bin";
const link = "link.bin";

files.delete(target);
files.delete(link);

let f = files.openFile({path: target, mode: "w"});
f.write(new ArrayBuffer(123), 0);
f.close();

if (supportsLink) {
	files.createLink(link, target);

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

	stat = files.status(link, {resolveTarget: true});
	assert(stat.isFile(), "expected file");
	assert(123 === stat.size, "expected file length of 123");
}
else
	trace("file systenm links unsupported - skipping tests")
