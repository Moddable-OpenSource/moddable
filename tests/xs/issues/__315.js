/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/330
flags: [onlyStrict]
---*/

const source = [
	`0,;`,
	`0,`,
];

source.forEach(source => {
	assert.throws(SyntaxError, () => eval(source), `"${source}" should be SyntaxError`);
});
