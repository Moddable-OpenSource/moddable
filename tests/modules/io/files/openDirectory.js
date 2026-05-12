/*---
description:
flags: [module]
---*/

import files from "./files_FIXTURE.js";
import {deleteTree} from "./files_FIXTURE.js";

const dirPath = "opendir-test";
deleteTree(files, dirPath);
files.createDirectory(dirPath);
files.createDirectory(dirPath + "/sub");
files.openFile({path: dirPath + "/sub/file.bin", mode: "w+"}).close();
files.openFile({path: dirPath + "/top.bin", mode: "w+"}).close();

// open sub-directory and scan it
const sub = files.openDirectory({path: dirPath + "/sub"});
const subEntries = [];
for (const name of sub)
	subEntries.push(name);
assert.compareArray(subEntries.sort(), ["file.bin"]);

// openDirectory from within an opened directory
const top = files.openDirectory({path: dirPath});
const nested = top.openDirectory({path: "sub"});
const nestedEntries = [];
for (const name of nested)
	nestedEntries.push(name);
assert.compareArray(nestedEntries.sort(), ["file.bin"]);

// status is relative to the opened directory
assert(top.status("top.bin").isFile());
assert(top.status("sub").isDirectory());

// files opened from sub-directory work correctly
const f = sub.openFile({path: "file.bin"});
assert(f.status().isFile());
f.close();

// double-close is safe
sub.close();
sub.close();

deleteTree(files, dirPath);
