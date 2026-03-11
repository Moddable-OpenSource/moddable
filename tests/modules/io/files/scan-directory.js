/*---
description: 
flags: [module]
---*/

import files from "./files_FIXTURE.js";
import {deleteTree, scan} from "./files_FIXTURE.js";

const paths = [
	"scantest",
	"scantest/scantest.bin",
	"scantest/foo",
	"scantest/foo/bar",
	"scantest/foo/bar/test",
	"scantest/foo/bar/test1.bin",
	"scantest/foo/bar/test2.bin",
	"scantest/foo/bar/test3.bin"
];

deleteTree(files, paths[0]);

paths.forEach(path => {
	if (path.includes(".")) 
		files.openFile({path, mode: "w+"}).close();
	else
		files.createDirectory(path);
});

assert.compareArray(scan(files, "scantest/foo/bar"), ["test", "test1.bin", "test2.bin", "test3.bin"]);
assert.compareArray(scan(files, "scantest/foo"), ["bar"]);
assert.compareArray(scan(files, "scantest"), ["foo", "scantest.bin"]);

deleteTree(files, paths[0]);
