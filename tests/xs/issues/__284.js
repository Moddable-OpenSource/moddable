/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/284
flags: [onlyStrict]
---*/

let expected = [
	`boolean`,
	`boolean`,
	`number`,
	`number`,
	`string`,
	`string`,
	`symbol`,
	`symbol`
];

for (const P of [Boolean, Number, String, Symbol]) {
	P.prototype.method = testThis
	P().method()
	Object.defineProperty(P.prototype, 'prop', {get: testThis, configurable: true})
	P().prop
}

function testThis() {
	'use strict'
	assert.sameValue(expected.shift(), typeof this);
}
