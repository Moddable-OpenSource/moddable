/*---
description: 
flags: [module]
---*/

import files from "./files_FIXTURE.js";
import {deleteTree} from "./files_FIXTURE.js";

const path = "test";
deleteTree(files, path);
files.createDirectory(path);
let d = files.openDirectory({path});
d.delete("123");
d.close();
d.close();
assert.throws(SyntaxError, () => d.delete("123"));
assert.throws(SyntaxError, () => d.status("123"));
assert.throws(SyntaxError, () => d.createDirectory("123"));
assert.throws(SyntaxError, () => d.move("123", "234"));
if (d.readLink)
	assert.throws(SyntaxError, () => d.readLink("123"));
assert.throws(SyntaxError, () => d.scan("123"));
assert.throws(SyntaxError, () => d.openDirectory({path: "123"}));
assert.throws(SyntaxError, () => d.openFile({path: "123"}));
