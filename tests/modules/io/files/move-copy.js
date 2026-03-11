/*---
description: 
flags: [module]
---*/

import files from "./files_FIXTURE.js";
import {deleteTree} from "./files_FIXTURE.js";

// 3 argument move
{
	deleteTree(files, "test");

	files.createDirectory("test");
	files.createDirectory("test/subdir");
	const testRoot = files.openDirectory({path: "test"});
	const subdir = testRoot.openDirectory({path: "subdir"});

	subdir.openFile({path: "test.bin", mode: "w+"}).close();
	assert(testRoot.status("subdir/test.bin").isFile());
	subdir.move("test.bin", "test2.bin", testRoot);

	assert(testRoot.status("test2.bin").isFile());
	assert(!subdir.status("test2.bin").isFile());
}

// 2 argument move
{
	deleteTree(files, "test");

	files.createDirectory("test");
	files.createDirectory("test/subdir");
	const testRoot = files.openDirectory({path: "test"});

	testRoot.openFile({path: "subdir/test.bin", mode: "w+"}).close();
	assert(testRoot.status("subdir/test.bin").isFile());
	testRoot.move("subdir/test.bin", "test2.bin");

	assert(testRoot.status("test2.bin").isFile());
	assert(!testRoot.status("subdir/test2.bin").isFile());
}
