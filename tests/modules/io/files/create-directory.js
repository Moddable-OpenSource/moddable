/*---
description: 
flags: [module]
---*/

import files from "./files_FIXTURE.js";
import {deleteTree} from "./files_FIXTURE.js";

const paths = [
	"test",
	"test/dir1",
	"test/dir2",
	"test/dir2/test",
	"test/dir2/test/testing"
];

deleteTree(files, paths[0]);
paths.forEach(path => files.createDirectory(path));
paths.reverse();
paths.forEach(path => assert(files.delete(path)));
paths.forEach(path => {
	assert(!files.status(path).isFile());
	assert(!files.status(path).isDirectory());
});
