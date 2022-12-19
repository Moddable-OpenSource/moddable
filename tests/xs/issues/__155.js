/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/155
flags: [onlyStrict]
---*/

let a;

// legal
({ ...a } = {});
({ ...(a) } = {});
({ ...((a)) } = {});
({ ...a.b } = {});
({ ...(a.b) } = {});
let { ...c } = {};


// forbidden
const sources = [
	`({ ...{} } = {});`,
	`({ ...[] } = {});`,
	`({ ...a = 0 } = {});`,
	`let { ...{} } = {};`,
	`let { ...[] } = {};`,
	`let { ...(a) } = {};`,
	`let { ...a.b } = {};`
];

sources.forEach(source => {
	assert.throws(SyntaxError, () => eval(source), `${source}`);
});
