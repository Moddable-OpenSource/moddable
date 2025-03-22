/*---
description: 
flags: [module]
---*/

import files from "./files_FIXTURE.js";

const fails = [
	"/foo",
	"../foo",
	"./foo",
	"foo/..",
	"foo/../",
	"foo/.",
	"foo/../",
	"foo/../bar",
	"foo/./bar",
	"foo//bar",
	"foo///bar"
];

fails.forEach(path => {
	assert.throws(Error, () => files.delete(path), `should reject invalid path: "${path}"`);
});
